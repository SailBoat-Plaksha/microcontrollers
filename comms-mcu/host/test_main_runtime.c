#include <assert.h>
#include <stdio.h>
#include <string.h>

#define main comms_firmware_entry
#include "../main.c"
#undef main

static uart_inst_t fake_uart;
static i2c_inst_t fake_i2c0, fake_i2c1;
uart_inst_t *uart1 = &fake_uart;
i2c_inst_t *i2c0 = &fake_i2c0;
i2c_inst_t *i2c1 = &fake_i2c1;
static rosc_hw_t fake_rosc;
rosc_hw_t *rosc_hw = &fake_rosc;
static uint32_t fake_now;
static bool uart_writable = true;
static uint8_t uart_rx[1024], uart_tx[1024];
static size_t uart_rx_count, uart_rx_position, uart_tx_count;

void stdio_init_all(void) { }
absolute_time_t get_absolute_time(void) { return fake_now; }
uint32_t to_ms_since_boot(absolute_time_t value) { return (uint32_t)value; }
uint32_t time_us_32(void) { return fake_now * 1000u; }
void tight_loop_contents(void) { }
void gpio_set_function(uint pin, int function) { (void)pin; (void)function; }
void gpio_pull_up(uint pin) { (void)pin; }
uint uart_init(uart_inst_t *uart, uint baud) { (void)uart; return baud; }
void uart_set_format(uart_inst_t *uart, uint bits, uint stop, int parity) {
    (void)uart; (void)bits; (void)stop; (void)parity;
}
void uart_set_fifo_enabled(uart_inst_t *uart, bool enabled) { (void)uart; (void)enabled; }
bool uart_is_readable(uart_inst_t *uart) {
    (void)uart; return uart_rx_position < uart_rx_count;
}
uint8_t uart_getc(uart_inst_t *uart) { (void)uart; return uart_rx[uart_rx_position++]; }
bool uart_is_writable(uart_inst_t *uart) { (void)uart; return uart_writable; }
void uart_putc_raw(uart_inst_t *uart, uint8_t value) {
    (void)uart; assert(uart_tx_count < sizeof(uart_tx)); uart_tx[uart_tx_count++] = value;
}
void i2c_init(i2c_inst_t *i2c, uint baud) { (void)i2c; (void)baud; }
void i2c_slave_init(i2c_inst_t *i2c, uint8_t address, i2c_slave_handler_t handler) {
    (void)i2c; (void)address; (void)handler;
}
uint8_t i2c_read_byte_raw(i2c_inst_t *i2c) { (void)i2c; return 0; }
void i2c_write_byte_raw(i2c_inst_t *i2c, uint8_t value) { (void)i2c; (void)value; }

static void inject_uart(const char *line) {
    size_t length = strlen(line);
    assert(uart_rx_count + length < sizeof(uart_rx));
    memcpy(uart_rx + uart_rx_count, line, length);
    uart_rx_count += length;
    modem_service_rx();
    if (uart_rx_position == uart_rx_count) uart_rx_position = uart_rx_count = 0;
}

static void configure_modem(void) {
    modem_config_index = 0;
    modem_configured = false;
    modem_state = MODEM_CONFIG_TX;
    modem_tx_length = modem_tx_position = 0;
    modem_config_next();
    for (uint8_t i = 0; i < 4; ++i) {
        assert(modem_state == MODEM_CONFIG_TX);
        modem_service_tx();
        assert(modem_state == MODEM_CONFIG_WAIT);
        inject_uart("+OK\r\n");
    }
    assert(modem_state == MODEM_READY && modem_configured);
}

int main(void) {
    relay_init(&relay, 23, 0);
    uint8_t startup_frame[64];
    size_t startup_length;
    while (relay_radio_next(&relay, startup_frame, &startup_length)) { }
    modem_stream_init(&modem_stream);
    configure_modem();
    assert(relay.radio_tx_errors == 0);

    uint8_t ping_frame[64];
    mavlink_message_t ping;
    mavlink_msg_ping_pack(255, 190, &ping, 123, 9, 1, 1);
    size_t ping_length = mavlink_msg_to_send_buffer(ping_frame, &ping);
    assert(modem_send_frame(ping_frame, ping_length));
    uart_writable = false;
    size_t initial_tx = uart_tx_count;
    modem_service_tx();
    assert(uart_tx_count == initial_tx && modem_state == MODEM_SEND_TX);
    uart_writable = true;
    modem_service_tx();
    assert(modem_state == MODEM_SEND_WAIT && modem_tx_length == 0);
    uint32_t malformed_before = relay.malformed_frames;
    inject_uart("+OK\r\n");
    assert(modem_state == MODEM_READY && relay.radio_tx_packets == 1);
    assert(relay.malformed_frames == malformed_before);

    mavlink_message_t rejected_movement;
    uint8_t rejected_mailbox[64] = {0};
    mavlink_msg_actuator_request_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_PI,
        &rejected_movement, 43, 5, 500, 1000, 200, ARSEM_SYSTEM_BOAT,
        ARSEM_COMPONENT_MOTOR, 1, 1, 3);
    (void)mavlink_msg_to_send_buffer(rejected_mailbox, &rejected_movement);
    assert(relay_i2c_write(&relay, rejected_mailbox) == RELAY_ACCEPTED);
    assert(relay_radio_next(&relay, modem_inflight_frame, &modem_inflight_length));
    modem_inflight_terminal = false;
    assert(modem_send_frame(modem_inflight_frame, modem_inflight_length));
    modem_service_tx();
    inject_uart("+ERR=10\r\n");
    assert(modem_state == MODEM_READY && relay.radio_tx_errors == 1);
    assert(relay.response_reservation_count == 0);
    assert(relay.malformed_frames == malformed_before);

    mavlink_message_t result_msg;
    uint8_t result_mailbox[64] = {0};
    mavlink_msg_request_result_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_PI,
        &result_msg, 4, 8, 10, 12, 1, 4, ARSEM_SYSTEM_GROUND, ARSEM_COMPONENT_GROUND);
    size_t result_length = mavlink_msg_to_send_buffer(result_mailbox, &result_msg);
    assert(relay_i2c_write(&relay, result_mailbox) == RELAY_ACCEPTED);
    assert(relay_radio_next(&relay, modem_inflight_frame, &modem_inflight_length));
    assert(modem_inflight_length == result_length);
    modem_inflight_terminal = true;
    assert(modem_send_frame(modem_inflight_frame, modem_inflight_length));
    modem_service_tx();
    inject_uart("+ERR=10\r\n");
    assert(modem_state == MODEM_FAILED);
    assert(relay.terminal_to_radio.count == 1);

    configure_modem();
    assert(relay_radio_next(&relay, modem_inflight_frame, &modem_inflight_length));
    assert(modem_inflight_length == result_length);
    modem_inflight_terminal = true;
    assert(modem_send_frame(modem_inflight_frame, modem_inflight_length));
    modem_service_tx();
    inject_uart("+OK\r\n");
    assert(relay.terminal_to_radio.count == 0 && modem_state == MODEM_READY);

    mavlink_message_t movement;
    uint8_t movement_mailbox[64] = {0};
    mavlink_msg_actuator_request_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_PI,
        &movement, 44, 5, 500, 1000, 200, ARSEM_SYSTEM_BOAT,
        ARSEM_COMPONENT_MOTOR, 1, 2, 3);
    (void)mavlink_msg_to_send_buffer(movement_mailbox, &movement);
    assert(relay_i2c_write(&relay, movement_mailbox) == RELAY_ACCEPTED);
    assert(relay_radio_next(&relay, modem_inflight_frame, &modem_inflight_length));
    modem_inflight_terminal = false;
    assert(modem_send_frame(modem_inflight_frame, modem_inflight_length));
    modem_service_tx();
    uint32_t deadline = modem_deadline;
    modem_check_timeout(deadline);
    assert(modem_state == MODEM_FAILED && !modem_configured);
    assert(relay.radio_tx_errors == 3 && relay.radio_tx_packets == 2);
    assert(relay.to_radio.count == 0 && relay.terminal_to_radio.count == 0);
    assert(relay.response_reservation_count == 1);
    puts("actual main.c modem service: config ACK, UART backpressure, SEND ACK/ERR and ambiguous timeout passed");
    return 0;
}
