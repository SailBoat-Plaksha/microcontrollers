#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "mailbox_transport.h"

static uint8_t fake_input[80];
static size_t fake_input_length, fake_input_position;
static uint8_t fake_output[80];
static size_t fake_output_length;

uint8_t i2c_read_byte_raw(i2c_inst_t *i2c) {
    (void)i2c;
    assert(fake_input_position < fake_input_length);
    return fake_input[fake_input_position++];
}
void i2c_write_byte_raw(i2c_inst_t *i2c, uint8_t value) {
    (void)i2c;
    assert(fake_output_length < sizeof(fake_output));
    fake_output[fake_output_length++] = value;
}

static size_t pack(mavlink_message_t *msg, uint8_t out[64]) {
    memset(out, 0, 64);
    return mavlink_msg_to_send_buffer(out, msg);
}
static size_t make_pi_frame(uint8_t out[64]) {
    mavlink_message_t msg;
    mavlink_msg_actuator_request_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_PI,
        &msg, 41, 9, 500, 1000, 200, 1, 154, 1, 1, 3);
    return pack(&msg, out);
}
static void issue_read(mailbox_transport_t *mailbox, const uint8_t *expected,
                       size_t count) {
    (void)mailbox;
    fake_output_length = 0;
    for (size_t i = 0; i < count; ++i)
        mailbox_transport_callback(NULL, I2C_SLAVE_REQUEST);
    mailbox_transport_callback(NULL, I2C_SLAVE_FINISH);
    assert(fake_output_length == count);
    if (expected) assert(memcmp(fake_output, expected, count) == 0);
}
static void issue_write(mailbox_transport_t *mailbox, const uint8_t *bytes,
                        size_t count) {
    (void)mailbox;
    memcpy(fake_input, bytes, count);
    fake_input_length = count;
    fake_input_position = 0;
    for (size_t i = 0; i < count; ++i)
        mailbox_transport_callback(NULL, I2C_SLAVE_RECEIVE);
    mailbox_transport_callback(NULL, I2C_SLAVE_FINISH);
}

int main(void) {
    relay_t relay;
    relay_init(&relay, 17, 0);
    uint8_t initial[64];
    memcpy(initial, relay.health_snapshot, sizeof(initial));
    mailbox_transport_t mailbox;
    mailbox_transport_init(&mailbox, initial);

    uint8_t frame[64];
    make_pi_frame(frame);
    assert(relay_radio_frame(&relay, frame, 31, 10) == RELAY_ACCEPTED);
    mailbox_transport_service(&mailbox, &relay);
    uint8_t staged[64];
    memcpy(staged, (const void *)mailbox.tx, 64);

    /* New telemetry arriving while a Pi read is short cannot replace the
     * staged command or pop another queue entry. */
    uint8_t second[64];
    mavlink_message_t ping;
    mavlink_msg_ping_pack(ARSEM_SYSTEM_GROUND, ARSEM_COMPONENT_GROUND,
        &ping, 123, 5, 0, 0);
    size_t ping_length = pack(&ping, second);
    assert(relay_radio_frame(&relay, second, ping_length, 11) == RELAY_ACCEPTED);
    for (size_t count = 1; count < 64; ++count) {
        issue_read(&mailbox, staged, count);
        mailbox_transport_service(&mailbox, &relay);
        assert(memcmp((const void *)mailbox.tx, staged, 64) == 0);
    }
    issue_read(&mailbox, staged, 64);
    mailbox_transport_service(&mailbox, &relay);
    assert(memcmp((const void *)mailbox.tx, staged, 64) != 0);
    assert(relay.to_pi.count == 1);

    /* A 65th request is a zero sentinel; fixed-length consumers complete at
     * byte 64 and need not request it. */
    uint8_t next[64];
    memcpy(next, (const void *)mailbox.tx, 64);
    fake_output_length = 0;
    for (size_t i = 0; i < 64; ++i)
        mailbox_transport_callback(NULL, I2C_SLAVE_REQUEST);
    mailbox_transport_callback(NULL, I2C_SLAVE_REQUEST);
    assert(fake_output_length == 65 && fake_output[64] == 0);
    mailbox_transport_callback(NULL, I2C_SLAVE_FINISH);
    mailbox_transport_service(&mailbox, &relay);
    assert(memcmp((const void *)mailbox.tx, next, 64) != 0);

    /* Write-only and repeated-start write phases are independent of TX reads. */
    make_pi_frame(frame);
    issue_write(&mailbox, frame, 64);
    mailbox_transport_service(&mailbox, &relay);
    assert(relay.forwarded_from_pi == 1);

    /* The SDK reports FINISH at a repeated START too. A write followed by a
     * read in one combined transaction stages the write without corrupting
     * the already prepared read response. */
    memcpy(fake_input, frame, 64);
    fake_input_length = 64;
    fake_input_position = 0;
    for (size_t i = 0; i < 64; ++i)
        mailbox_transport_callback(NULL, I2C_SLAVE_RECEIVE);
    mailbox_transport_callback(NULL, I2C_SLAVE_FINISH);
    uint8_t repeated_start_read[64];
    memcpy(repeated_start_read, (const void *)mailbox.tx, 64);
    issue_read(&mailbox, repeated_start_read, 64);
    mailbox_transport_service(&mailbox, &relay);
    assert(relay.forwarded_from_pi == 2);

    issue_write(&mailbox, frame, 65);
    mailbox_transport_service(&mailbox, &relay);
    assert(relay.radio_tx_errors >= 1);

    /* One pending RX mailbox is never torn by a second write. */
    issue_write(&mailbox, frame, 64);
    issue_write(&mailbox, frame, 64);
    assert(mailbox.rx_ready && mailbox.rx_invalid);
    mailbox_transport_service(&mailbox, &relay);
    assert(relay.forwarded_from_pi == 3);
    assert(relay.radio_tx_errors >= 2);
    puts("mailbox transport callbacks: exact, short/retry, write-only/repeated-start, overflow and staging passed");
    return 0;
}
