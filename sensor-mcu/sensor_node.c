#include "include/sensor_node.h"
#include "mailbox.h"

#include <string.h>

typedef struct {
    bool used;
    bool complete;
    bool local;
    bool response_ready;
    uint32_t session;
    uint32_t request;
    uint32_t expires;
    uint32_t started;
    uint32_t deadline;
    uint32_t digest;
    uint16_t max_age;
    uint8_t source_system;
    uint8_t source_component;
    uint8_t response[SENSOR_FRAME_BYTES];
} request_entry_t;

static request_entry_t entries[SENSOR_CACHE_CAPACITY];
static sensor_config_t cfg;
static uint32_t sensor_boot_id;
static uint32_t last_health;
static uint32_t last_heartbeat;
static uint32_t cached_at;
static uint32_t pi_boot_id;
static uint32_t pi_time;
static uint32_t pi_seq;
static uint16_t valid_fields;
static uint16_t cached_age_ms;
static int16_t axes[9];
static uint8_t have_sample;

uint32_t sensor_node_now(const sensor_node_t *node) {
    (void)node;
    return cfg.clock_ms ? cfg.clock_ms(cfg.clock_context) : 0u;
}

static uint32_t payload_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xffffffffu;
    while (length--) {
        crc ^= *data++;
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ ((0u - (crc & 1u)) & 0xedb88320u);
        }
    }
    return ~crc;
}

/* kind: 0 correlated result/reply, 1 dependency request, 2 replaceable telemetry. */
static bool enqueue(sensor_node_t *node,
                    const uint8_t frame[SENSOR_FRAME_BYTES],
                    uint8_t kind) {
    if (node->count == SENSOR_QUEUE_CAPACITY) {
        if (kind == 2) {
            for (uint8_t i = 0; i < node->count; i++) {
                uint8_t at = (uint8_t)((node->head + i) % SENSOR_QUEUE_CAPACITY);
                if (node->telemetry[at]) {
                    memcpy(node->frames[at], frame, SENSOR_FRAME_BYTES);
                    node->telemetry_coalesced++;
                    return true;
                }
            }
        }

        if (kind == 2) {
            node->telemetry_coalesced++;
        } else if (kind == 1) {
            node->dependency_drops++;
        } else {
            node->result_drops++;
        }
        return false;
    }

    uint8_t at = (uint8_t)((node->head + node->count) % SENSOR_QUEUE_CAPACITY);
    memcpy(node->frames[at], frame, SENSOR_FRAME_BYTES);
    node->telemetry[at] = kind == 2;
    node->count++;
    return true;
}

static bool pack_frame(const mavlink_message_t *message,
                       uint8_t frame[SENSOR_FRAME_BYTES]) {
    uint8_t bytes[MAVLINK_MAX_PACKET_LEN];
    uint16_t length = mavlink_msg_to_send_buffer(bytes, message);
    if (length > SENSOR_FRAME_BYTES) {
        return false;
    }

    memset(frame, 0, SENSOR_FRAME_BYTES);
    memcpy(frame, bytes, length);
    return true;
}

static bool queue_message(sensor_node_t *node,
                          const mavlink_message_t *message,
                          uint8_t kind) {
    uint8_t frame[SENSOR_FRAME_BYTES];
    return pack_frame(message, frame) && enqueue(node, frame, kind);
}

static request_entry_t *find_request(uint32_t session, uint32_t request) {
    for (unsigned i = 0; i < SENSOR_CACHE_CAPACITY; i++) {
        if (entries[i].used && entries[i].session == session &&
            entries[i].request == request) {
            return &entries[i];
        }
    }
    return NULL;
}

static request_entry_t *allocate_request(uint32_t now) {
    for (unsigned i = 0; i < SENSOR_CACHE_CAPACITY; i++) {
        if (!entries[i].used || (int32_t)(now - entries[i].expires) >= 0) {
            return &entries[i];
        }
    }
    return NULL;
}

static void expire_sample(uint32_t now) {
    uint32_t age = (uint32_t)cached_age_ms + (now - cached_at);
    if (have_sample && age > 2000u) {
        have_sample = 0;
        valid_fields = 0;
    }
}

static void emit_result(sensor_node_t *node,
                        request_entry_t *entry,
                        uint8_t status,
                        uint16_t age,
                        uint16_t mask,
                        const int16_t *values,
                        uint32_t time_boot_ms,
                        uint32_t sample_seq,
                        uint32_t acquisition_boot_id) {
    int16_t normalized[9] = {0};
    if (values) {
        memcpy(normalized, values, sizeof normalized);
    }
    if (!(mask & 1u)) {
        memset(normalized, 0, 3 * sizeof(int16_t));
    }
    if (!(mask & 2u)) {
        memset(normalized + 3, 0, 3 * sizeof(int16_t));
    }
    if (!(mask & 4u)) {
        memset(normalized + 6, 0, 3 * sizeof(int16_t));
    }

    uint8_t target_system = entry->local ? ARSEM_SYSTEM_BOAT : ARSEM_SYSTEM_GROUND;
    uint8_t target_component = entry->local ? ARSEM_COMPONENT_PI : ARSEM_COMPONENT_GROUND;
    mavlink_message_t message;
    mavlink_msg_imu_sample_pack(
        1, ARSEM_COMPONENT_SENSOR, &message, entry->session, entry->request,
        time_boot_ms, sample_seq, acquisition_boot_id, sensor_boot_id, age, mask,
        normalized[0], normalized[1], normalized[2], normalized[3],
        normalized[4], normalized[5], normalized[6], normalized[7],
        normalized[8], status, 1, 1, target_system, target_component);

    if (queue_message(node, &message, 0)) {
        (void)pack_frame(&message, entry->response);
        entry->response_ready = true;
    }
    entry->complete = true;
}

static void handle_ground_request(sensor_node_t *node,
                                  const mavlink_message_t *message,
                                  uint32_t now) {
    expire_sample(now);
    mavlink_imu_request_t request;
    mavlink_msg_imu_request_decode(message, &request);
    uint32_t digest = payload_crc32((const uint8_t *)_MAV_PAYLOAD(message),
                                    message->len);

    if (message->sysid != 1 || message->compid != 1 ||
        request.target_system != 1 || request.target_component != 159 ||
        request.path != 1 || request.timeout_ms < 1 ||
        request.timeout_ms > 5000 || request.ground_session == 0) {
        node->rejected++;
        return;
    }

    request_entry_t *entry = find_request(request.ground_session, request.request_id);
    if (entry) {
        if (entry->digest != digest) {
            node->rejected++;
        } else if (entry->response_ready) {
            (void)enqueue(node, entry->response, 0);
        }
        return;
    }

    entry = allocate_request(now);
    if (!entry) {
        node->rejected++;
        return;
    }
    memset(entry, 0, sizeof *entry);
    entry->used = true;
    entry->session = request.ground_session;
    entry->request = request.request_id;
    entry->digest = digest;
    entry->max_age = request.max_age_ms;
    entry->started = now;
    entry->deadline = now + request.timeout_ms;
    entry->expires = entry->deadline + 30000u;
    entry->source_system = message->sysid;
    entry->source_component = message->compid;

    if (request.max_age_ms > 0 && have_sample) {
        uint32_t age = (uint32_t)cached_age_ms + (now - cached_at);
        if (age <= request.max_age_ms) {
            emit_result(node, entry, 1, (uint16_t)age, valid_fields, axes,
                        pi_time, pi_seq, pi_boot_id);
            return;
        }
    }

    mavlink_message_t dependency;
    mavlink_msg_imu_request_pack(1, ARSEM_COMPONENT_SENSOR, &dependency,
                                 request.ground_session, request.request_id,
                                 request.timeout_ms, 0, 0, 1, 1);
    (void)queue_message(node, &dependency, 1);
}

static void handle_pi_sample(sensor_node_t *node,
                             const mavlink_message_t *message,
                             uint32_t now) {
    mavlink_imu_sample_t sample;
    mavlink_msg_imu_sample_decode(message, &sample);
    request_entry_t *entry = find_request(sample.ground_session, sample.request_id);

    if (message->sysid != 1 || message->compid != 1 ||
        sample.target_system != 1 || sample.target_component != 159 ||
        sample.path != 0 || sample.acquisition_component != 1 || !entry ||
        entry->complete || sample.status < 1 || sample.status > 5 ||
        !sample.acquisition_boot_id ||
        sample.source_boot_id != sample.acquisition_boot_id ||
        (sample.valid_fields & ~7u) ||
        (sample.status != 1 && sample.valid_fields) ||
        (pi_boot_id && pi_boot_id == sample.acquisition_boot_id &&
         (int32_t)(sample.sample_seq - pi_seq) < 0)) {
        node->rejected++;
        return;
    }
    if ((int32_t)(now - entry->deadline) >= 0) {
        node->rejected++;
        return;
    }

    if (pi_boot_id && pi_boot_id != sample.acquisition_boot_id) {
        have_sample = 0;
        valid_fields = 0;
    }
    pi_boot_id = sample.acquisition_boot_id;
    pi_time = sample.time_boot_ms;
    pi_seq = sample.sample_seq;

    valid_fields = sample.status == 1 ? (sample.valid_fields & 7u) : 0u;
    int16_t values[9] = {
        sample.xacc, sample.yacc, sample.zacc,
        sample.xgyro, sample.ygyro, sample.zgyro,
        sample.xmag, sample.ymag, sample.zmag
    };
    if (!(valid_fields & 1u)) {
        memset(values, 0, 3 * sizeof(int16_t));
    }
    if (!(valid_fields & 2u)) {
        memset(values + 3, 0, 3 * sizeof(int16_t));
    }
    if (!(valid_fields & 4u)) {
        memset(values + 6, 0, 3 * sizeof(int16_t));
    }

    memcpy(axes, values, sizeof axes);
    cached_at = now;
    cached_age_ms = sample.sample_age_ms;
    have_sample = valid_fields != 0;
    emit_result(node, entry, sample.status, sample.sample_age_ms, valid_fields,
                values, sample.time_boot_ms, sample.sample_seq,
                sample.acquisition_boot_id);
}

void sensor_node_init(sensor_node_t *node, const sensor_config_t *config) {
    memset(node, 0, sizeof *node);
    memset(entries, 0, sizeof entries);
    memset(&cfg, 0, sizeof cfg);
    if (config) {
        cfg = *config;
    }
    sensor_boot_id = cfg.boot_id ? cfg.boot_id : 1;
    have_sample = 0;
    valid_fields = 0;
    cached_age_ms = 0;
    cached_at = 0;
    pi_boot_id = 0;
    pi_time = 0;
    pi_seq = 0;
    last_health = 0;
    last_heartbeat = 0;
}

bool sensor_node_request_local(sensor_node_t *node,
                               uint32_t request_id,
                               uint16_t timeout_ms,
                               uint16_t max_age_ms) {
    uint32_t now = sensor_node_now(node);
    if (!request_id || timeout_ms < 1 || timeout_ms > 5000) {
        return false;
    }
    expire_sample(now);

    request_entry_t *entry = find_request(0, request_id);
    uint32_t digest = ((uint32_t)timeout_ms << 16) | max_age_ms;
    if (entry) {
        if (entry->digest != digest) {
            node->rejected++;
            return false;
        }
        if (entry->response_ready) {
            (void)enqueue(node, entry->response, 0);
        }
        return true;
    }

    entry = allocate_request(now);
    if (!entry) {
        node->rejected++;
        return false;
    }
    memset(entry, 0, sizeof *entry);
    entry->used = true;
    entry->local = true;
    entry->request = request_id;
    entry->digest = digest;
    entry->max_age = max_age_ms;
    entry->started = now;
    entry->deadline = now + timeout_ms;
    entry->expires = entry->deadline + 30000u;

    if (max_age_ms && have_sample) {
        uint32_t age = (uint32_t)cached_age_ms + (now - cached_at);
        if (age <= max_age_ms) {
            emit_result(node, entry, 1, (uint16_t)age, valid_fields, axes,
                        pi_time, pi_seq, pi_boot_id);
            return true;
        }
    }

    mavlink_message_t dependency;
    mavlink_msg_imu_request_pack(1, ARSEM_COMPONENT_SENSOR, &dependency,
                                 0, request_id, timeout_ms, 0, 0, 1, 1);
    return queue_message(node, &dependency, 1);
}

bool sensor_node_receive(sensor_node_t *node,
                         const uint8_t frame[SENSOR_FRAME_BYTES]) {
    mavlink_message_t message;
    mavlink_message_t parsed;
    mavlink_status_t status;
    memset(&status, 0, sizeof status);
    unsigned messages = 0;

    for (size_t i = 0; i < SENSOR_FRAME_BYTES; i++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, frame[i], &message, &status)) {
            messages++;
            parsed = message;
        }
    }
    if (messages == 0) {
        node->malformed++;
        return false;
    }
    if (messages != 1) {
        node->rejected++;
        return false;
    }

    message = parsed;
    if (message.incompat_flags & MAVLINK_IFLAG_SIGNED) {
        node->rejected++;
        return false;
    }

    uint32_t now = sensor_node_now(node);
    if (message.msgid == MAVLINK_MSG_ID_IMU_REQUEST) {
        handle_ground_request(node, &message, now);
    } else if (message.msgid == MAVLINK_MSG_ID_IMU_SAMPLE) {
        handle_pi_sample(node, &message, now);
    } else if (message.msgid == MAVLINK_MSG_ID_PING) {
        mavlink_ping_t ping;
        mavlink_msg_ping_decode(&message, &ping);
        bool known_source =
            (message.sysid == 1 && message.compid == 1) ||
            (message.sysid == ARSEM_SYSTEM_GROUND &&
             message.compid == ARSEM_COMPONENT_GROUND);
        bool addressed = (ping.target_system == 0 || ping.target_system == 1) &&
                         (ping.target_component == 0 || ping.target_component == 159);
        if (known_source && addressed) {
            mavlink_message_t reply;
            mavlink_msg_ping_pack(1, ARSEM_COMPONENT_SENSOR, &reply,
                                  ping.time_usec, ping.seq,
                                  message.sysid, message.compid);
            (void)queue_message(node, &reply, 0);
        } else {
            node->rejected++;
        }
    } else {
        node->rejected++;
    }
    return true;
}

void sensor_node_tick(sensor_node_t *node) {
    uint32_t now = sensor_node_now(node);
    for (unsigned i = 0; i < SENSOR_CACHE_CAPACITY; i++) {
        request_entry_t *entry = &entries[i];
        if (entry->used && !entry->complete &&
            (int32_t)(now - entry->deadline) >= 0) {
            node->timeouts++;
            emit_result(node, entry, 4, 0, 0, NULL, 0, 0, 0);
        }
    }

    expire_sample(now);
    if (now - last_heartbeat >= 1000u) {
        mavlink_message_t heartbeat;
        mavlink_msg_heartbeat_pack(1, ARSEM_COMPONENT_SENSOR, &heartbeat,
                                   18, 8, 0, 0, 4);
        (void)queue_message(node, &heartbeat, 2);
        last_heartbeat = now;
    }
    if (now - last_health >= 1000u) {
        uint32_t drops = node->dependency_drops + node->result_drops +
                         node->telemetry_coalesced + node->malformed +
                         node->rejected + node->timeouts;
        mavlink_message_t health;
        mavlink_msg_node_health_pack(
            1, ARSEM_COMPONENT_SENSOR, &health, sensor_boot_id, now, 0,
            valid_fields, (uint16_t)(drops > 65535u ? 65535u : drops), 0, 0);
        (void)queue_message(node, &health, 2);
        last_health = now;
    }
}

bool sensor_node_take_tx(sensor_node_t *node,
                         uint8_t frame[SENSOR_FRAME_BYTES]) {
    if (!node->count) {
        return false;
    }
    memcpy(frame, node->frames[node->head], SENSOR_FRAME_BYTES);
    node->head = (uint8_t)((node->head + 1) % SENSOR_QUEUE_CAPACITY);
    node->count--;
    return true;
}
