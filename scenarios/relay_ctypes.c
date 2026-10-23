#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "modem.h"

static relay_t relay;
static uint32_t tick_ms;
static modem_stream_t modem;

void scenario_relay_init(uint32_t boot_id) {
    tick_ms = 100;
    relay_init(&relay, boot_id, tick_ms);
    modem_stream_init(&modem);
}

void scenario_modem_feed(const uint8_t *bytes, size_t length) {
    modem_stream_feed(&modem, bytes, length);
}

int scenario_modem_pop(char line[MODEM_LINE_BYTES]) {
    return modem_stream_pop(&modem, line) ? 1 : 0;
}

int scenario_relay_radio(const uint8_t *frame, size_t length) {
    return (int)relay_radio_frame(&relay, frame, length, tick_ms++);
}

int scenario_relay_modem_line(const char *line, uint16_t expected) {
    return (int)modem_parse_rcv_line(&relay, line, expected, tick_ms++);
}

int scenario_relay_i2c_write(const uint8_t *mailbox) {
    return (int)relay_i2c_write(&relay, mailbox);
}

int scenario_relay_i2c_read(uint8_t *mailbox) {
    return relay_i2c_read(&relay, mailbox) ? 1 : 0;
}

int scenario_relay_radio_next(uint8_t *frame, size_t *length) {
    return relay_radio_next(&relay, frame, length) ? 1 : 0;
}

void scenario_relay_radio_complete(int accepted) {
    relay_radio_complete(&relay, accepted != 0);
}

unsigned scenario_relay_forwarded_from_radio(void) { return relay.forwarded_from_radio; }
unsigned scenario_relay_forwarded_from_pi(void) { return relay.forwarded_from_pi; }
unsigned scenario_relay_malformed(void) { return relay.malformed_frames; }
unsigned scenario_relay_drops(void) { return relay.queue_drops; }

int scenario_modem_build(uint16_t destination, const uint8_t *frame, size_t length,
                         char *command, size_t capacity) {
    return modem_build_send_command(destination, frame, length, command, capacity) ? 1 : 0;
}
