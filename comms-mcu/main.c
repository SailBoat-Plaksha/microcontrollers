#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/i2c_slave.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/structs/rosc.h"

#include "mailbox.h"
#include "config.h"
#include "relay.h"
#include "modem.h"

#define MODEM_TX_BYTES 300u

static relay_t relay;
static volatile uint8_t mailbox_rx[ARSEM_MAILBOX_FRAME_BYTES];
static volatile bool mailbox_rx_ready;
static volatile bool mailbox_write_invalid;
static volatile uint16_t mailbox_dropped_writes;
static volatile uint8_t mailbox_tx[ARSEM_MAILBOX_FRAME_BYTES];
static volatile bool mailbox_tx_refresh;
static volatile bool mailbox_tx_in_use;
static uint8_t i2c_write_staging[ARSEM_MAILBOX_FRAME_BYTES];
static size_t i2c_write_count;
static bool i2c_write_overflow;
static size_t i2c_read_position;
static modem_stream_t modem_stream;
static uint32_t reported_modem_line_drops;
static char modem_tx[MODEM_TX_BYTES];
static size_t modem_tx_length;
static size_t modem_tx_position;
static bool modem_waiting_for_config;
static bool modem_configured;
static uint8_t modem_config_index;

static uint32_t random_boot_id(void) {
    /* RP2040 has no reset-cause API or TRNG. Mix ROSC samples and timer state;
     * reset_reason remains UNKNOWN because hardware cause is not exposed here. */
    uint32_t value = time_us_32() ^ (uint32_t)(uintptr_t)&relay;
    for (unsigned i = 0; i < 128; ++i) {
        value = (value << 1) ^ (uint32_t)(rosc_hw->randombit & 1u);
        value ^= value >> 13;
        value *= 0x5bd1e995u;
    }
    return value ? value : 1u;
}

static void i2c_callback(i2c_inst_t *i2c, i2c_slave_event_t event) {
    (void)i2c;
    switch (event) {
    case I2C_SLAVE_RECEIVE:
        if (i2c_write_count < sizeof(i2c_write_staging)) {
            i2c_write_staging[i2c_write_count++] = i2c_read_byte_raw(COMMS_I2C);
        } else {
            (void)i2c_read_byte_raw(COMMS_I2C);
            i2c_write_overflow = true;
            mailbox_write_invalid = true;
        }
        break;
    case I2C_SLAVE_REQUEST:
        mailbox_tx_in_use = true;
        if (i2c_read_position < ARSEM_MAILBOX_FRAME_BYTES)
            i2c_write_byte_raw(COMMS_I2C, mailbox_tx[i2c_read_position++]);
        else
            i2c_write_byte_raw(COMMS_I2C, 0);
        break;
    case I2C_SLAVE_FINISH:
        if (i2c_write_count || i2c_write_overflow) {
            if (i2c_write_count == ARSEM_MAILBOX_FRAME_BYTES && !i2c_write_overflow &&
                !mailbox_rx_ready) {
                for (size_t i = 0; i < ARSEM_MAILBOX_FRAME_BYTES; ++i)
                    mailbox_rx[i] = i2c_write_staging[i];
                mailbox_rx_ready = true;
            } else {
                mailbox_write_invalid = true;
                if (mailbox_dropped_writes != UINT16_MAX) mailbox_dropped_writes++;
            }
            i2c_write_count = 0;
            i2c_write_overflow = false;
        } else {
            i2c_read_position = 0;
            mailbox_tx_in_use = false;
            mailbox_tx_refresh = true;
        }
        break;
    }
}

static void modem_uart_init(void) {
    uart_init(COMMS_LORA_UART, COMMS_LORA_BAUD);
    gpio_set_function(COMMS_LORA_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(COMMS_LORA_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(COMMS_LORA_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(COMMS_LORA_UART, true);
}

static void modem_queue_command(const char *command) {
    if (modem_tx_position < modem_tx_length) return;
    size_t n = strlen(command);
    if (n + 2u >= sizeof(modem_tx)) return;
    memcpy(modem_tx, command, n);
    modem_tx[n++] = '\r';
    modem_tx[n++] = '\n';
    modem_tx_length = n;
    modem_tx_position = 0;
}

static void modem_config_next(void) {
    char command[80];
    switch (modem_config_index) {
    case 0: snprintf(command, sizeof(command), "AT+ADDRESS=%u", COMMS_RADIO_ADDRESS); break;
    case 1: snprintf(command, sizeof(command), "AT+NETWORKID=%u", COMMS_RADIO_NETWORK_ID); break;
    case 2: snprintf(command, sizeof(command), "AT+BAND=%u", COMMS_RADIO_BAND_HZ); break;
    case 3: snprintf(command, sizeof(command), "AT+PARAMETER=%s", COMMS_RADIO_PARAMETER); break;
    default: modem_waiting_for_config = false; modem_configured = true; return;
    }
    modem_waiting_for_config = true;
    modem_queue_command(command);
}

static bool modem_send_frame(const uint8_t *frame, size_t length) {
    char command[MODEM_TX_BYTES];
    if (!modem_build_send_command(COMMS_GROUND_RADIO_ADDRESS, frame, length,
                                  command, sizeof(command))) return false;
    modem_queue_command(command);
    return true;
}

static void modem_service_rx(void) {
    uint8_t incoming[64];
    size_t used = 0;
    while (uart_is_readable(COMMS_LORA_UART)) {
        incoming[used++] = uart_getc(COMMS_LORA_UART);
        if (used == sizeof(incoming)) break;
    }
    if (used) modem_stream_feed(&modem_stream, incoming, used);
    if (modem_stream.dropped_lines > reported_modem_line_drops) {
        uint32_t dropped = modem_stream.dropped_lines - reported_modem_line_drops;
        relay.malformed_frames += dropped;
        relay.radio_rx_errors += dropped;
        reported_modem_line_drops = modem_stream.dropped_lines;
    }
    char line[MODEM_LINE_BYTES];
    while (modem_stream_pop(&modem_stream, line)) {
        if (modem_waiting_for_config &&
            (strcmp(line, "+OK") == 0 || strcmp(line, "OK") == 0)) {
            modem_config_index++;
            modem_config_next();
            continue;
        }
        relay_result_t result = modem_parse_rcv_line(&relay, line,
            COMMS_GROUND_RADIO_ADDRESS, to_ms_since_boot(get_absolute_time()));
        if (result == RELAY_ACCEPTED) mailbox_tx_refresh = true;
    }
}

static void modem_service_tx(void) {
    while (modem_tx_position < modem_tx_length && uart_is_writable(COMMS_LORA_UART))
        uart_putc_raw(COMMS_LORA_UART, modem_tx[modem_tx_position++]);
    if (modem_tx_position == modem_tx_length) {
        modem_tx_position = 0;
        modem_tx_length = 0;
    }
}

int main(void) {
    stdio_init_all();
    modem_stream_init(&modem_stream);
    relay_init(&relay, random_boot_id(), to_ms_since_boot(get_absolute_time()));
    for (size_t i = 0; i < ARSEM_MAILBOX_FRAME_BYTES; ++i)
        mailbox_tx[i] = relay.health_snapshot[i];
    gpio_set_function(COMMS_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(COMMS_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(COMMS_I2C_SDA_PIN);
    gpio_pull_up(COMMS_I2C_SCL_PIN);
    i2c_init(COMMS_I2C, ARSEM_I2C_BUS_HZ);
    i2c_slave_init(COMMS_I2C, COMMS_I2C_ADDRESS, i2c_callback);
    modem_uart_init();
    modem_config_next();

    uint32_t last_health = 0, last_heartbeat = 0, last_link = 0;
    for (;;) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (mailbox_rx_ready) {
            uint8_t mailbox[ARSEM_MAILBOX_FRAME_BYTES];
            for (size_t i = 0; i < sizeof(mailbox); ++i) mailbox[i] = mailbox_rx[i];
            mailbox_rx_ready = false;
            (void)relay_i2c_write(&relay, mailbox);
        }
        if (mailbox_write_invalid) {
            uint32_t dropped = mailbox_dropped_writes ? mailbox_dropped_writes : 1u;
            relay.malformed_frames += dropped;
            relay.radio_tx_errors += dropped;
            mailbox_dropped_writes = 0;
            mailbox_write_invalid = false;
        }
        if (mailbox_tx_refresh && !mailbox_tx_in_use) {
            uint8_t mailbox[ARSEM_MAILBOX_FRAME_BYTES];
            (void)relay_i2c_read(&relay, mailbox);
            for (size_t i = 0; i < sizeof(mailbox); ++i) mailbox_tx[i] = mailbox[i];
            mailbox_tx_refresh = false;
        }
        if ((uint32_t)(now - last_health) >= 1000u) {
            relay_refresh_health(&relay, now);
            mailbox_tx_refresh = true;
            last_health = now;
        }
        if ((uint32_t)(now - last_heartbeat) >= 1000u) {
            relay_refresh_heartbeat(&relay, now);
            mailbox_tx_refresh = true;
            last_heartbeat = now;
        }
        if ((uint32_t)(now - last_link) >= 1000u) {
            relay_refresh_link(&relay, now, modem_configured, COMMS_RADIO_ADDRESS);
            mailbox_tx_refresh = true;
            last_link = now;
        }
        modem_service_rx();
        modem_service_tx();
        if (modem_configured && modem_tx_length == 0u) {
            uint8_t frame[ARSEM_MAILBOX_FRAME_BYTES];
            size_t length;
            if (relay_radio_next(&relay, frame, &length) &&
                !modem_send_frame(frame, length)) relay.radio_tx_errors++;
        }
        tight_loop_contents();
    }
}
