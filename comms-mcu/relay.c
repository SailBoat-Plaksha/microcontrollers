#include "relay.h"

#include <string.h>

static bool supported_id(uint32_t id) {
    switch (id) {
    case MAVLINK_MSG_ID_NODE_INFO:
    case MAVLINK_MSG_ID_ACTUATOR_SETPOINT:
    case MAVLINK_MSG_ID_ACTUATOR_STATUS:
    case MAVLINK_MSG_ID_CONTROL_AUTHORITY:
    case MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE:
    case MAVLINK_MSG_ID_POWER_STATE:
    case MAVLINK_MSG_ID_LINK_STATUS:
    case MAVLINK_MSG_ID_BUS_GRANT:
    case MAVLINK_MSG_ID_ACTUATOR_REQUEST:
    case MAVLINK_MSG_ID_NODE_HEALTH:
    case MAVLINK_MSG_ID_REQUEST_RESULT:
    case MAVLINK_MSG_ID_HEARTBEAT:
    case MAVLINK_MSG_ID_PING:
    case MAVLINK_MSG_ID_SCALED_IMU:
        return true;
    default:
        return false;
    }
}

static uint8_t frame_priority(const mavlink_message_t *msg) {
    if (msg->msgid == MAVLINK_MSG_ID_ACTUATOR_REQUEST) {
        mavlink_actuator_request_t request;
        mavlink_msg_actuator_request_decode(msg, &request);
        if (request.opcode == 3u) return 0u; /* STOP */
        return 1u;
    }
    if (msg->msgid == MAVLINK_MSG_ID_REQUEST_RESULT ||
        msg->msgid == MAVLINK_MSG_ID_ACTUATOR_STATUS ||
        msg->msgid == MAVLINK_MSG_ID_NODE_HEALTH ||
        msg->msgid == MAVLINK_MSG_ID_LINK_STATUS) return 1u;
    if (msg->msgid == MAVLINK_MSG_ID_HEARTBEAT ||
        msg->msgid == MAVLINK_MSG_ID_SCALED_IMU ||
        msg->msgid == MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE) return 3u;
    return 2u;
}

static uint8_t source_system(const mavlink_message_t *msg) {
    return msg->sysid;
}

static uint8_t source_component(const mavlink_message_t *msg) {
    return msg->compid;
}

static void count_queue_drop(relay_t *relay, const relay_queue_t *queue) {
    relay->queue_drops++;
    if (queue == &relay->to_pi) relay->radio_rx_errors++;
    else relay->radio_tx_errors++;
}

static bool queue_push(relay_t *relay, relay_queue_t *queue,
                       const uint8_t *bytes, uint8_t length, uint8_t priority,
                       const mavlink_message_t *msg) {
    /* Replace queued routine samples from the same sender before consuming
     * another slot. These messages describe current state, not events. */
    if (priority >= 3u) {
        for (uint8_t i = 0; i < queue->count; ++i) {
            if (queue->entries[i].msgid == msg->msgid &&
                queue->entries[i].sysid == msg->sysid &&
                queue->entries[i].compid == msg->compid) {
                memcpy(queue->entries[i].bytes, bytes, length);
                queue->entries[i].length = length;
                queue->entries[i].priority = priority;
                return true;
            }
        }
    }

    if (queue->count == RELAY_QUEUE_CAPACITY) {
        int victim = -1;
        uint8_t lowest = 0;
        for (uint8_t i = 0; i < queue->count; ++i) {
            if (queue->entries[i].priority >= lowest) {
                lowest = queue->entries[i].priority;
                victim = (int)i;
            }
        }
        if (victim < 0 || lowest <= priority) {
            count_queue_drop(relay, queue);
            return false;
        }
        for (uint8_t i = (uint8_t)victim; i + 1u < queue->count; ++i)
            queue->entries[i] = queue->entries[i + 1u];
        queue->count--;
        count_queue_drop(relay, queue);
    }

    relay_item_t *item = &queue->entries[queue->count++];
    memcpy(item->bytes, bytes, length);
    item->length = length;
    item->priority = priority;
    item->msgid = msg->msgid;
    item->sysid = source_system(msg);
    item->compid = source_component(msg);
    item->order = ++relay->sequence;
    return true;
}

static bool queue_pop(relay_queue_t *queue, uint8_t *bytes, size_t *length) {
    if (!queue->count) return false;
    uint8_t best = 0;
    for (uint8_t i = 1; i < queue->count; ++i) {
        if (queue->entries[i].priority < queue->entries[best].priority) best = i;
    }
    relay_item_t *item = &queue->entries[best];
    memset(bytes, 0, RELAY_MAX_FRAME);
    memcpy(bytes, item->bytes, item->length);
    *length = item->length;
    for (uint8_t i = best; i + 1u < queue->count; ++i)
        queue->entries[i] = queue->entries[i + 1u];
    queue->count--;
    return true;
}

relay_result_t relay_validate_frame(relay_t *relay, const uint8_t *bytes,
                                    size_t length, bool padded,
                                    mavlink_message_t *decoded,
                                    uint8_t *frame_length) {
    if (!relay || !bytes || !decoded || !frame_length) return RELAY_REJECT_MALFORMED;
    if (length > RELAY_MAX_FRAME) return RELAY_REJECT_OVERSIZE;
    if (length < 12u || bytes[0] != MAVLINK_STX) return RELAY_REJECT_MALFORMED;
    if (bytes[2] & MAVLINK_IFLAG_SIGNED) return RELAY_REJECT_SIGNED;
    if (bytes[2] != 0u) return RELAY_REJECT_MALFORMED;
    size_t actual = (size_t)bytes[1] + 12u;
    if (actual > RELAY_MAX_FRAME || actual > length) return RELAY_REJECT_OVERSIZE;
    if (!padded && actual != length) return RELAY_REJECT_MALFORMED;
    for (size_t i = actual; i < length; ++i) {
        if (bytes[i] != 0u) return RELAY_REJECT_CONCATENATED;
    }

    memset(&relay->parser_message, 0, sizeof(relay->parser_message));
    memset(&relay->parser_status, 0, sizeof(relay->parser_status));
    memset(decoded, 0, sizeof(*decoded));
    uint8_t completed = 0u;
    uint8_t bad_crc = 0u;
    for (size_t i = 0; i < actual; ++i) {
        uint8_t result = mavlink_frame_char_buffer(&relay->parser_message,
            &relay->parser_status, bytes[i], decoded, NULL);
        if (result == 1u) completed++;
        if (result == 2u) bad_crc++;
    }
    if (completed != 1u || bad_crc || decoded->magic != MAVLINK_STX)
        return RELAY_REJECT_MALFORMED;
    if (!supported_id(decoded->msgid)) return RELAY_REJECT_UNSUPPORTED;
    *frame_length = (uint8_t)actual;
    return RELAY_ACCEPTED;
}

static uint8_t pack_frame(mavlink_message_t *msg, uint8_t bytes[RELAY_MAX_FRAME]) {
    uint8_t raw[RELAY_MAX_FRAME];
    uint16_t length = mavlink_msg_to_send_buffer(raw, msg);
    if (!length || length > RELAY_MAX_FRAME) return 0u;
    memset(bytes, 0, RELAY_MAX_FRAME);
    memcpy(bytes, raw, length);
    return (uint8_t)length;
}

void relay_refresh_heartbeat(relay_t *relay, uint32_t now_ms) {
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_COMMS_SENSOR,
        &msg, MAV_TYPE_ONBOARD_CONTROLLER, MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);
    relay->heartbeat_length = pack_frame(&msg, relay->heartbeat_snapshot);
    if (relay->heartbeat_length)
        (void)queue_push(relay, &relay->to_radio, relay->heartbeat_snapshot,
                         relay->heartbeat_length, 3u, &msg);
    (void)now_ms;
}

void relay_refresh_health(relay_t *relay, uint32_t now_ms) {
    mavlink_message_t msg;
    mavlink_msg_node_health_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_COMMS_SENSOR,
        &msg, relay->boot_id, now_ms - relay->started_ms, 0, 0,
        (uint16_t)(relay->queue_drops > UINT16_MAX ? UINT16_MAX : relay->queue_drops),
        0, 0);
    relay->health_length = pack_frame(&msg, relay->health_snapshot);
    if (relay->health_length)
        (void)queue_push(relay, &relay->to_radio, relay->health_snapshot,
                         relay->health_length, 1u, &msg);
}

void relay_refresh_link(relay_t *relay, uint32_t now_ms, bool connected,
                        uint16_t radio_address) {
    mavlink_message_t msg;
    bool metrics_fresh = relay->radio_metrics_valid &&
                         (uint32_t)(now_ms - relay->last_radio_metrics_ms) <= 5000u;
    uint16_t rx_errors = (uint16_t)(relay->radio_rx_errors > UINT16_MAX ?
                                     UINT16_MAX : relay->radio_rx_errors);
    uint16_t tx_errors = (uint16_t)(relay->radio_tx_errors > UINT16_MAX ?
                                     UINT16_MAX : relay->radio_tx_errors);
    mavlink_msg_link_status_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_COMMS_SENSOR,
        &msg, now_ms - relay->started_ms, relay->radio_rx_packets,
        relay->radio_tx_packets, rx_errors, tx_errors, radio_address,
        metrics_fresh ? relay->radio_rssi_dbm : INT16_MIN,
        metrics_fresh ? relay->radio_snr_cdb : INT16_MIN,
        2u, connected ? 1u : 0u);
    relay->link_length = pack_frame(&msg, relay->link_snapshot);
    if (relay->link_length)
        (void)queue_push(relay, &relay->to_radio, relay->link_snapshot,
                         relay->link_length, 1u, &msg);
}

void relay_init(relay_t *relay, uint32_t boot_id, uint32_t now_ms) {
    memset(relay, 0, sizeof(*relay));
    relay->boot_id = boot_id ? boot_id : 1u;
    relay->started_ms = now_ms;
    relay_refresh_heartbeat(relay, now_ms);
    relay_refresh_health(relay, now_ms);
}

static bool ping_is_broadcast_request(const mavlink_message_t *msg) {
    mavlink_ping_t ping;
    mavlink_msg_ping_decode(msg, &ping);
    return ping.target_system == 0u && ping.target_component == 0u;
}

static void reply_to_ping(relay_t *relay, const mavlink_message_t *request,
                          bool over_radio) {
    mavlink_ping_t ping;
    mavlink_msg_ping_decode(request, &ping);
    mavlink_message_t reply;
    uint8_t raw[RELAY_MAX_FRAME];
    uint8_t length;
    mavlink_msg_ping_pack(ARSEM_SYSTEM_BOAT, ARSEM_COMPONENT_COMMS_SENSOR, &reply,
        ping.time_usec, ping.seq, request->sysid, request->compid);
    length = pack_frame(&reply, raw);
    relay_queue_t *queue = over_radio ? &relay->to_radio : &relay->to_pi;
    uint8_t priority = 1u;
    if (length) (void)queue_push(relay, queue, raw, length, priority, &reply);
}

relay_result_t relay_radio_frame(relay_t *relay, const uint8_t *frame, size_t length,
                                 uint32_t now_ms) {
    mavlink_message_t msg;
    uint8_t frame_length = 0u;
    relay_result_t result = relay_validate_frame(relay, frame, length, false,
                                                   &msg, &frame_length);
    if (result != RELAY_ACCEPTED) {
        relay->malformed_frames++;
        relay->radio_rx_errors++;
        return result;
    }
    if (msg.msgid == MAVLINK_MSG_ID_PING && ping_is_broadcast_request(&msg))
        reply_to_ping(relay, &msg, true);
    if (!queue_push(relay, &relay->to_pi, frame, frame_length,
                    frame_priority(&msg), &msg)) return RELAY_REJECT_QUEUE_FULL;
    relay->forwarded_from_radio++;
    relay->radio_rx_packets++;
    relay_refresh_health(relay, now_ms);
    return RELAY_ACCEPTED;
}

relay_result_t relay_i2c_write(relay_t *relay, const uint8_t mailbox[RELAY_MAX_FRAME]) {
    mavlink_message_t msg;
    uint8_t frame_length = 0u;
    relay_result_t result = relay_validate_frame(relay, mailbox, RELAY_MAX_FRAME,
                                                   true, &msg, &frame_length);
    if (result != RELAY_ACCEPTED) {
        relay->malformed_frames++;
        relay->radio_tx_errors++;
        return result;
    }
    if (msg.msgid == MAVLINK_MSG_ID_PING && msg.sysid == ARSEM_SYSTEM_BOAT &&
        msg.compid == ARSEM_COMPONENT_PI && ping_is_broadcast_request(&msg)) {
        reply_to_ping(relay, &msg, false);
        return RELAY_ACCEPTED;
    }
    /* Never relay our own local replies back to the radio. */
    if ((msg.sysid == ARSEM_SYSTEM_BOAT && msg.compid == ARSEM_COMPONENT_COMMS_SENSOR) ||
        (msg.sysid == ARSEM_SYSTEM_GROUND && msg.compid == ARSEM_COMPONENT_GROUND))
        return RELAY_REJECT_UNSUPPORTED;
    relay_queue_t *destination = msg.msgid == MAVLINK_MSG_ID_REQUEST_RESULT ?
                                 &relay->terminal_to_radio : &relay->to_radio;
    uint8_t priority = msg.msgid == MAVLINK_MSG_ID_REQUEST_RESULT ? 0u : frame_priority(&msg);
    if (!queue_push(relay, destination, mailbox, frame_length, priority, &msg))
        return RELAY_REJECT_QUEUE_FULL;
    relay->forwarded_from_pi++;
    return RELAY_ACCEPTED;
}

bool relay_i2c_read(relay_t *relay, uint8_t mailbox[RELAY_MAX_FRAME]) {
    size_t length = 0u;
    if (queue_pop(&relay->to_pi, mailbox, &length)) return true;
    const uint8_t *snapshots[] = {relay->health_snapshot, relay->heartbeat_snapshot,
        relay->link_snapshot};
    const uint8_t lengths[] = {relay->health_length, relay->heartbeat_length,
        relay->link_length};
    for (uint8_t offset = 0; offset < 3u; ++offset) {
        uint8_t index = (uint8_t)((relay->snapshot_cursor + offset) % 3u);
        if (lengths[index]) {
            memcpy(mailbox, snapshots[index], RELAY_MAX_FRAME);
            relay->snapshot_cursor = (uint8_t)((index + 1u) % 3u);
            return true;
        }
    }
    memset(mailbox, 0, RELAY_MAX_FRAME);
    return false;
}

bool relay_radio_next(relay_t *relay, uint8_t frame[RELAY_MAX_FRAME], size_t *length) {
    if (queue_pop(&relay->terminal_to_radio, frame, length)) {
        relay->radio_tx_packets++;
        return true;
    }
    if (!queue_pop(&relay->to_radio, frame, length)) return false;
    relay->radio_tx_packets++;
    return true;
}
