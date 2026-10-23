#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "relay.h"
#include "modem.h"

static size_t pack(mavlink_message_t *msg, uint8_t out[64]) {
    memset(out, 0, 64);
    return mavlink_msg_to_send_buffer(out, msg);
}

static size_t request_frame(uint8_t out[64], uint8_t opcode, uint32_t request_id) {
    mavlink_message_t msg;
    mavlink_msg_actuator_request_pack(ARSEM_SYSTEM_GROUND, ARSEM_COMPONENT_GROUND,
        &msg, request_id, 99, opcode == 3 ? 0 : 500, 1000, 200, 1, 154, 1,
        opcode, 3);
    return pack(&msg, out);
}

static size_t boat_request_frame(uint8_t out[64], uint8_t opcode, uint32_t request_id) {
    mavlink_message_t msg;
    mavlink_msg_actuator_request_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_PI,
        &msg, request_id, 99, opcode == 3 ? 0 : 500, 1000, 200, 1, 154, 1,
        opcode, 3);
    return pack(&msg, out);
}

int main(void) {
    relay_t relay;
    relay_init(&relay, 0x12345678u, 100u);
    uint8_t frame[64], padded[64], result[64];
    mavlink_message_t decoded;
    uint8_t decoded_length;
    size_t sent_length = 0;
    bool saw_startup_health = false, saw_startup_heartbeat = false;
    while (relay_radio_next(&relay, result, &sent_length)) {
        assert(relay_validate_frame(&relay, result, 64, true,
                                    &decoded, &decoded_length) == RELAY_ACCEPTED);
        assert(decoded.sysid == ARSEM_SYSTEM_BOAT &&
               decoded.compid == ARSEM_COMPONENT_COMMS_SENSOR);
        if (decoded.msgid == MAVLINK_MSG_ID_NODE_HEALTH) saw_startup_health = true;
        if (decoded.msgid == MAVLINK_MSG_ID_HEARTBEAT) saw_startup_heartbeat = true;
    }
    assert(saw_startup_health && saw_startup_heartbeat);
    size_t length = request_frame(frame, 1, 7);
    assert(length == 31);
    uint8_t invalid[64];
    memcpy(invalid, frame, length);
    invalid[length - 1u] ^= 1u;
    assert(relay_radio_frame(&relay, invalid, length, 199) == RELAY_REJECT_MALFORMED);
    memcpy(invalid, frame, length);
    invalid[2] = MAVLINK_IFLAG_SIGNED;
    assert(relay_radio_frame(&relay, invalid, length, 199) == RELAY_REJECT_SIGNED);
    memcpy(invalid, frame, length);
    invalid[7] = 0x28; invalid[8] = 0x23; invalid[9] = 0;
    assert(relay_radio_frame(&relay, invalid, length, 199) != RELAY_ACCEPTED);
    assert(relay_radio_frame(&relay, frame, length, 200) == RELAY_ACCEPTED);
    assert(relay_i2c_read(&relay, result));
    assert(memcmp(result, frame, length) == 0);
    for (size_t i = length; i < 64; ++i) assert(result[i] == 0);

    /* Inject fragmented UART modem bytes, decode +RCV, and collect the
     * unchanged raw frame from the Pi-facing mailbox read path. */
    modem_stream_t modem;
    modem_stream_init(&modem);
    char command[300];
    assert(modem_build_send_command(2, frame, length, command, sizeof(command)));
    char *payload = strchr(command, ',');
    payload = payload ? strchr(payload + 1, ',') : NULL;
    assert(payload);
    payload++;
    char line[300];
    int line_length = snprintf(line, sizeof(line), "+RCV=2,%zu,%s,-67,8.0\r\n",
                               strlen(payload), payload);
    assert(line_length > 0 && (size_t)line_length < sizeof(line));
    modem_stream_feed(&modem, (const uint8_t *)line, 7);
    modem_stream_feed(&modem, (const uint8_t *)line + 7, (size_t)line_length - 7);
    char decoded_line[MODEM_LINE_BYTES];
    assert(modem_stream_pop(&modem, decoded_line));
    assert(modem_parse_rcv_line(&relay, decoded_line, 2, 210) == RELAY_ACCEPTED);
    assert(relay.radio_metrics_valid && relay.radio_rssi_dbm == -67 && relay.radio_snr_cdb == 800);
    relay_refresh_link(&relay, 211, true, 1);
    assert(relay_validate_frame(&relay, relay.link_snapshot, 64, true,
                                &decoded, &decoded_length) == RELAY_ACCEPTED);
    mavlink_link_status_t link_status;
    mavlink_msg_link_status_decode(&decoded, &link_status);
    assert(link_status.rssi_dbm == -67 && link_status.snr_cdb == 800);
    while (relay_radio_next(&relay, result, &sent_length)) { }
    assert(relay_i2c_read(&relay, result));
    assert(memcmp(result, frame, length) == 0);

    /* Exact mailbox framing, damaged CRC, signed and concatenated transfers. */
    length = boat_request_frame(frame, 1, 8);
    memset(padded, 0, sizeof(padded));
    memcpy(padded, frame, length);
    assert(relay_i2c_write(&relay, padded) == RELAY_ACCEPTED);
    assert(relay_radio_next(&relay, result, &sent_length));
    assert(sent_length == length && memcmp(frame, result, length) == 0);
    padded[length] = 0xfd;
    assert(relay_i2c_write(&relay, padded) == RELAY_REJECT_CONCATENATED);
    padded[length] = 0;
    padded[2] = 1;
    assert(relay_i2c_write(&relay, padded) == RELAY_REJECT_SIGNED);
    padded[2] = 0;
    padded[length - 1] ^= 0x80;
    assert(relay_i2c_write(&relay, padded) == RELAY_REJECT_MALFORMED);
    memset(padded, 0, sizeof(padded));
    padded[0] = 0xfd;
    padded[1] = 53;
    assert(relay_i2c_write(&relay, padded) == RELAY_REJECT_OVERSIZE);

    /* Terminal request outcomes have a dedicated bounded queue and are read
     * before routine traffic headed for the modem. */
    mavlink_message_t terminal;
    mavlink_msg_request_result_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_PI,
        &terminal, 99, 8, 77, 5, 1, 4, ARSEM_SYSTEM_GROUND, ARSEM_COMPONENT_GROUND);
    memset(padded, 0, sizeof(padded));
    length = pack(&terminal, padded);
    assert(relay_i2c_write(&relay, padded) == RELAY_ACCEPTED);
    mavlink_message_t routine;
    mavlink_msg_heartbeat_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_PI, &routine,
        MAV_TYPE_ONBOARD_CONTROLLER, MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);
    memset(padded, 0, sizeof(padded));
    (void)pack(&routine, padded);
    assert(relay_i2c_write(&relay, padded) == RELAY_ACCEPTED);
    assert(relay_radio_next(&relay, result, &sent_length));
    assert(relay_validate_frame(&relay, result, 64, true, &decoded, &decoded_length) == RELAY_ACCEPTED);
    assert(decoded.msgid == MAVLINK_MSG_ID_REQUEST_RESULT);
    relay_radio_complete(&relay, true);
    assert(relay_radio_next(&relay, result, &sent_length));
    assert(relay_validate_frame(&relay, result, 64, true, &decoded, &decoded_length) == RELAY_ACCEPTED);
    assert(decoded.msgid == MAVLINK_MSG_ID_HEARTBEAT);

    /* A terminal correlation retains its latest state in one bounded slot. */
    mavlink_message_t terminal_update;
    mavlink_msg_request_result_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_PI,
        &terminal_update, 99, 8, 77, 5, 1, 4, ARSEM_SYSTEM_GROUND, ARSEM_COMPONENT_GROUND);
    memset(padded, 0, sizeof(padded));
    (void)pack(&terminal_update, padded);
    assert(relay_i2c_write(&relay, padded) == RELAY_ACCEPTED);
    mavlink_msg_request_result_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_PI,
        &terminal_update, 99, 8, 77, 5, 1, 5, ARSEM_SYSTEM_GROUND, ARSEM_COMPONENT_GROUND);
    memset(padded, 0, sizeof(padded));
    (void)pack(&terminal_update, padded);
    assert(relay_i2c_write(&relay, padded) == RELAY_ACCEPTED);
    assert(relay.terminal_to_radio.count == 1);
    assert(relay_radio_next(&relay, result, &sent_length));
    assert(relay_validate_frame(&relay, result, 64, true, &decoded, &decoded_length) == RELAY_ACCEPTED);
    mavlink_request_result_t latest_result;
    mavlink_msg_request_result_decode(&decoded, &latest_result);
    assert(latest_result.state == 5);
    relay_radio_complete(&relay, true);

    /* Broadcast ping returns one locally sourced addressed reply on radio and
     * is also forwarded to Pi. The addressed reply is never answered again. */
    mavlink_message_t ping;
    mavlink_msg_ping_pack(ARSEM_SYSTEM_GROUND, ARSEM_COMPONENT_GROUND, &ping,
                          987654321u, 42, 0, 0);
    length = pack(&ping, frame);
    assert(relay_radio_frame(&relay, frame, length, 300) == RELAY_ACCEPTED);
    assert(relay_radio_next(&relay, result, &sent_length));
    assert(relay_validate_frame(&relay, result, 64, true, &decoded, &decoded_length) == RELAY_ACCEPTED);
    assert(decoded.msgid == MAVLINK_MSG_ID_PING);
    assert(decoded.sysid == ARSEM_SYSTEM_BOAT && decoded.compid == ARSEM_COMPONENT_COMMS_SENSOR);
    mavlink_ping_t pong;
    mavlink_msg_ping_decode(&decoded, &pong);
    assert(pong.time_usec == 987654321u && pong.seq == 42);
    assert(pong.target_system == ARSEM_SYSTEM_GROUND && pong.target_component == ARSEM_COMPONENT_GROUND);
    assert(relay_i2c_read(&relay, result));
    assert(memcmp(result, frame, length) == 0);
    assert(relay.to_pi.count == 0);

    /* Only the ground identity may trigger a radio-side local PING reply. */
    mavlink_msg_ping_pack(1, 1, &ping, 100, 45, 0, 0);
    length = pack(&ping, frame);
    uint8_t local_replies_before = 0;
    for (uint8_t i = 0; i < relay.to_radio.count; ++i)
        if (relay.to_radio.entries[i].msgid == MAVLINK_MSG_ID_PING) local_replies_before++;
    assert(relay_radio_frame(&relay, frame, length, 301) == RELAY_ACCEPTED);
    uint8_t local_replies_after = 0;
    for (uint8_t i = 0; i < relay.to_radio.count; ++i)
        if (relay.to_radio.entries[i].msgid == MAVLINK_MSG_ID_PING) local_replies_after++;
    assert(local_replies_after == local_replies_before);
    assert(relay_i2c_read(&relay, result));
    assert(memcmp(result, frame, length) == 0);

    /* Persistent health snapshot survives empty event reads and reports boot
     * identity, UNKNOWN reset reason, and no locally acquired sensor fields. */
    assert(relay_i2c_read(&relay, result));
    assert(relay_validate_frame(&relay, result, 64, true, &decoded, &decoded_length) == RELAY_ACCEPTED);
    assert(decoded.msgid == MAVLINK_MSG_ID_NODE_HEALTH);
    mavlink_node_health_t health;
    mavlink_msg_node_health_decode(&decoded, &health);
    assert(health.boot_id == 0x12345678u && health.reset_reason == 0 && health.sensor_valid_fields == 0);

    /* The Pi can ping the comms node over I2C and gets an addressed local
     * reply through the mailbox. An addressed PING reply causes no new reply. */
    mavlink_message_t ping_request;
    mavlink_msg_ping_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_PI, &ping_request,
                          7654321u, 43, 0, 0);
    memset(padded, 0, sizeof(padded));
    (void)pack(&ping_request, padded);
    uint8_t radio_queue_before = relay.to_radio.count;
    assert(relay_i2c_write(&relay, padded) == RELAY_ACCEPTED);
    assert(relay.to_radio.count == radio_queue_before);
    assert(relay_i2c_read(&relay, result));
    assert(relay_validate_frame(&relay, result, 64, true, &decoded, &decoded_length) == RELAY_ACCEPTED);
    assert(decoded.msgid == MAVLINK_MSG_ID_PING && decoded.sysid == ARSEM_SYSTEM_BOAT &&
           decoded.compid == ARSEM_COMPONENT_COMMS_SENSOR);
    mavlink_msg_ping_decode(&decoded, &pong);
    assert(pong.time_usec == 7654321u && pong.seq == 43 &&
           pong.target_system == ARSEM_SYSTEM_BOAT && pong.target_component == ARSEM_COMPONENT_PI);

    mavlink_msg_ping_pack(ARSEM_SYSTEM_GROUND, ARSEM_COMPONENT_GROUND, &ping_request,
                          7654321u, 44, ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_COMMS_SENSOR);
    length = pack(&ping_request, frame);
    uint8_t ping_replies_before = 0;
    for (uint8_t i = 0; i < relay.to_radio.count; ++i)
        if (relay.to_radio.entries[i].msgid == MAVLINK_MSG_ID_PING) ping_replies_before++;
    assert(relay_radio_frame(&relay, frame, length, 400) == RELAY_ACCEPTED);
    uint8_t ping_replies_after = 0;
    for (uint8_t i = 0; i < relay.to_radio.count; ++i)
        if (relay.to_radio.entries[i].msgid == MAVLINK_MSG_ID_PING) ping_replies_after++;
    assert(ping_replies_after == ping_replies_before);

    /* Pico-local frames cannot be reflected back to the modem. */
    memset(padded, 0, sizeof(padded));
    mavlink_msg_heartbeat_pack(1, 158, &ping, MAV_TYPE_ONBOARD_CONTROLLER,
        MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);
    (void)pack(&ping, padded);
    assert(relay_i2c_write(&relay, padded) == RELAY_REJECT_UNSUPPORTED);

    relay_t priority_relay;
    relay_init(&priority_relay, 55, 0);
    for (uint32_t i = 0; i < RELAY_QUEUE_CAPACITY; ++i) {
        mavlink_message_t queued_ping;
        mavlink_msg_ping_pack((uint8_t)(10 + i), 42, &queued_ping,
                              100 + i, i, 1, 1);
        length = pack(&queued_ping, frame);
        assert(relay_radio_frame(&priority_relay, frame, length, 10 + i) == RELAY_ACCEPTED);
    }
    assert(priority_relay.to_pi.count == RELAY_QUEUE_CAPACITY);
    length = request_frame(frame, 3, 99);
    assert(relay_radio_frame(&priority_relay, frame, length, 30) == RELAY_ACCEPTED);
    assert(priority_relay.to_pi.count == RELAY_QUEUE_CAPACITY);
    assert(relay_i2c_read(&priority_relay, result));
    assert(relay_validate_frame(&priority_relay, result, 64, true,
                                &decoded, &decoded_length) == RELAY_ACCEPTED);
    assert(decoded.msgid == MAVLINK_MSG_ID_ACTUATOR_REQUEST);
    mavlink_actuator_request_t stop;
    mavlink_msg_actuator_request_decode(&decoded, &stop);
    assert(stop.opcode == 3 && stop.delta_cdeg == 0);

    /* Movement admission reserves a terminal-result slot until that correlated
     * result is sent onward; the bounded queue never silently overcommits. */
    relay_t capacity_relay;
    relay_init(&capacity_relay, 66, 0);
    for (uint32_t i = 1; i <= RELAY_QUEUE_CAPACITY; ++i) {
        length = boat_request_frame(frame, 1, i);
        memset(padded, 0, sizeof(padded));
        memcpy(padded, frame, 64);
        assert(relay_i2c_write(&capacity_relay, padded) == RELAY_ACCEPTED);
        assert(relay_radio_next(&capacity_relay, result, &sent_length));
    }
    assert(capacity_relay.response_reservation_count == RELAY_QUEUE_CAPACITY);
    length = boat_request_frame(frame, 1, 99);
    memset(padded, 0, sizeof(padded));
    memcpy(padded, frame, 64);
    assert(relay_i2c_write(&capacity_relay, padded) == RELAY_REJECT_QUEUE_FULL);
    assert(capacity_relay.queue_drops == 1 && capacity_relay.radio_tx_errors == 1);
    mavlink_msg_request_result_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_PI,
        &terminal_update, 99, 1, 77, 5, 1, 4, ARSEM_SYSTEM_GROUND, ARSEM_COMPONENT_GROUND);
    memset(padded, 0, sizeof(padded));
    (void)pack(&terminal_update, padded);
    assert(relay_i2c_write(&capacity_relay, padded) == RELAY_ACCEPTED);
    assert(capacity_relay.response_reservation_count == RELAY_QUEUE_CAPACITY - 1u);
    assert(relay_radio_next(&capacity_relay, result, &sent_length));
    relay_radio_complete(&capacity_relay, true);
    length = boat_request_frame(frame, 1, 99);
    memset(padded, 0, sizeof(padded));
    memcpy(padded, frame, 64);
    assert(relay_i2c_write(&capacity_relay, padded) == RELAY_ACCEPTED);
    assert(capacity_relay.response_reservation_count == RELAY_QUEUE_CAPACITY);

    printf("comms relay host harness: PASS\n");
    return 0;
}
