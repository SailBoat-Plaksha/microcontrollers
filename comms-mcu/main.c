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
#include "mailbox_transport.h"

#define MODEM_TX_BYTES 300u
#define MODEM_CONFIG_TIMEOUT_MS 3000u
#define MODEM_SEND_TIMEOUT_MS 5000u

typedef enum {
    MODEM_CONFIG_TX, MODEM_CONFIG_WAIT, MODEM_READY,
    MODEM_SEND_TX, MODEM_SEND_WAIT, MODEM_FAILED
} modem_state_t;

static relay_t relay;
static mailbox_transport_t mailbox;
static modem_stream_t modem_stream;
static uint32_t reported_modem_line_drops;
static char modem_tx[MODEM_TX_BYTES];
static size_t modem_tx_length;
static size_t modem_tx_position;
static bool modem_configured;
static uint8_t modem_config_index;
static modem_state_t modem_state = MODEM_CONFIG_TX;
static uint32_t modem_deadline;
static uint32_t modem_next_send_ms;
static uint8_t modem_inflight_frame[ARSEM_MAILBOX_FRAME_BYTES];
static size_t modem_inflight_length;
static bool modem_inflight_terminal;

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
    default: modem_configured = true; modem_state = MODEM_READY; return;
    }
    modem_queue_command(command);
    modem_state = MODEM_CONFIG_TX;
}

static bool modem_send_frame(const uint8_t *frame, size_t length) {
    char command[MODEM_TX_BYTES];
    if (!modem_build_send_command(COMMS_GROUND_RADIO_ADDRESS, frame, length,
                                  command, sizeof(command))) return false;
    modem_queue_command(command);
    modem_state = MODEM_SEND_TX;
    return true;
}

static bool modem_line_is(const char *line, const char *a, const char *b) {
    return strcmp(line, a) == 0 || strcmp(line, b) == 0;
}

static bool modem_line_error(const char *line) {
    return strncmp(line, "+ERR", 4) == 0 || strcmp(line, "ERROR") == 0;
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
        if (modem_state == MODEM_CONFIG_WAIT && modem_line_is(line, "+OK", "OK")) {
            modem_config_index++;
            modem_config_next();
            continue;
        }
        if (modem_state == MODEM_CONFIG_WAIT && modem_line_error(line)) {
            relay.radio_tx_errors++;
            modem_state = MODEM_FAILED;
            modem_configured = false;
            continue;
        }
        if (modem_state == MODEM_SEND_WAIT && modem_line_is(line, "+OK", "OK")) {
            relay.radio_tx_packets++;
            modem_next_send_ms = to_ms_since_boot(get_absolute_time()) + COMMS_RADIO_SEND_GAP_MS;
            if (modem_inflight_terminal) relay_radio_complete(&relay, true);
            modem_inflight_terminal = false;
            modem_state = MODEM_READY;
            continue;
        }
        if (modem_state == MODEM_SEND_WAIT && modem_line_error(line)) {
            relay.radio_tx_errors++;
            modem_next_send_ms = to_ms_since_boot(get_absolute_time()) + COMMS_RADIO_SEND_GAP_MS;
            if (modem_inflight_terminal) {
                (void)relay_radio_retain_terminal(&relay, modem_inflight_frame,
                                                  modem_inflight_length);
                modem_inflight_terminal = false;
                modem_state = MODEM_FAILED;
                modem_configured = false;
            } else {
                relay_radio_reject_movement(&relay, modem_inflight_frame,
                                            modem_inflight_length);
                modem_state = MODEM_READY;
            }
            continue;
        }
        if (strncmp(line, "+RCV=", 5) == 0)
            (void)modem_parse_rcv_line(&relay, line, COMMS_GROUND_RADIO_ADDRESS,
                to_ms_since_boot(get_absolute_time()));
    }
}

static void modem_service_tx(void) {
    while (modem_tx_position < modem_tx_length && uart_is_writable(COMMS_LORA_UART))
        uart_putc_raw(COMMS_LORA_UART, modem_tx[modem_tx_position++]);
    if (modem_tx_position == modem_tx_length) {
        modem_tx_position = 0;
        modem_tx_length = 0;
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (modem_state == MODEM_CONFIG_TX) {
            modem_state = MODEM_CONFIG_WAIT;
            modem_deadline = now + MODEM_CONFIG_TIMEOUT_MS;
        } else if (modem_state == MODEM_SEND_TX) {
            modem_state = MODEM_SEND_WAIT;
            modem_deadline = now + MODEM_SEND_TIMEOUT_MS;
        }
    }
}

static void modem_check_timeout(uint32_t now) {
    if ((modem_state == MODEM_CONFIG_WAIT || modem_state == MODEM_SEND_WAIT) &&
        (int32_t)(now - modem_deadline) >= 0) {
        relay.radio_tx_errors++;
        if (modem_state == MODEM_SEND_WAIT && modem_inflight_terminal) {
            (void)relay_radio_retain_terminal(&relay, modem_inflight_frame,
                                              modem_inflight_length);
            modem_inflight_terminal = false;
        }
        modem_state = MODEM_FAILED;
        modem_configured = false;
    }
}

int main(void) {
    stdio_init_all();
    modem_stream_init(&modem_stream);
    relay_init(&relay, random_boot_id(), to_ms_since_boot(get_absolute_time()));
    mailbox_transport_init(&mailbox, relay.health_snapshot);
    gpio_set_function(COMMS_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(COMMS_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(COMMS_I2C_SDA_PIN);
    gpio_pull_up(COMMS_I2C_SCL_PIN);
    i2c_init(COMMS_I2C, ARSEM_I2C_BUS_HZ);
    i2c_slave_init(COMMS_I2C, COMMS_I2C_ADDRESS, mailbox_transport_callback);
    modem_uart_init();
    modem_config_next();

    uint32_t last_health = 0, last_heartbeat = 0, last_link = 0;
    for (;;) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        mailbox_transport_service(&mailbox, &relay);
        if ((uint32_t)(now - last_health) >= 1000u) {
            relay_refresh_health(&relay, now);
            last_health = now;
        }
        if ((uint32_t)(now - last_heartbeat) >= 1000u) {
            relay_refresh_heartbeat(&relay, now);
            last_heartbeat = now;
        }
        if ((uint32_t)(now - last_link) >= 1000u) {
            relay_refresh_link(&relay, now, modem_configured, COMMS_RADIO_ADDRESS);
            last_link = now;
        }
        modem_service_rx();
        modem_service_tx();
        modem_check_timeout(now);
        if (modem_state == MODEM_READY && modem_tx_length == 0u &&
            (int32_t)(now - modem_next_send_ms) >= 0) {
            uint8_t frame[ARSEM_MAILBOX_FRAME_BYTES];
            size_t length;
            if (relay_radio_next(&relay, frame, &length)) {
                modem_inflight_length = length;
                memcpy(modem_inflight_frame, frame, length);
                modem_inflight_terminal = length >= 10u && frame[7] ==
                    (MAVLINK_MSG_ID_REQUEST_RESULT & 0xffu) && frame[8] ==
                    ((MAVLINK_MSG_ID_REQUEST_RESULT >> 8) & 0xffu) && frame[9] == 0u;
                if (!modem_send_frame(frame, length)) {
                    relay.radio_tx_errors++;
                    if (modem_inflight_terminal) {
                        (void)relay_radio_retain_terminal(&relay, frame, length);
                        modem_state = MODEM_FAILED;
                        modem_configured = false;
                    } else relay_radio_reject_movement(&relay, frame, length);
                    modem_inflight_terminal = false;
                }
            }
        }
        tight_loop_contents();
    }
}
