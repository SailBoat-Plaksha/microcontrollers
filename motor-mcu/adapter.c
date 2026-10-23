#include "adapter.h"

#include <string.h>

#define STATE_UNKNOWN 0u
#define STATE_RECEIVED 1u
#define STATE_ACCEPTED 2u
#define STATE_RUNNING 3u
#define STATE_COMPLETED 4u
#define STATE_STOPPED 5u
#define STATE_REJECTED 6u
#define RESULT_UNKNOWN 0u
#define RESULT_COMPLETED 1u
#define RESULT_DENIED 2u
#define RESULT_INVALID 3u
#define RESULT_STALE 4u
#define RESULT_SUPERSEDED 5u
#define SOURCE_RC 1u
#define SOURCE_AUTONOMOUS 2u
#define SOURCE_REMOTE_OPERATOR 3u
#define COMMAND_ACCEPTED 0u
#define COMMAND_IN_PROGRESS 1u
#define COMMAND_DENIED 2u
#define COMMAND_INVALID 3u
#define COMMAND_TIMEOUT 4u
#define COMMAND_FAULT 5u
#define OPCODE_ABSOLUTE 1u
#define OPCODE_STEP 2u
#define OPCODE_STOP 3u
#define MOTOR_PWM 128u
#define MOTOR_MIN_PWM 32u
#define TARGET_SYSTEM 255u
#define TARGET_COMPONENT 190u

_Static_assert(MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN + 12u <= MOTOR_MAILBOX_BYTES,
               "ACTUATOR_REQUEST frame must fit the fixed mailbox");

static uint32_t now_ms(motor_adapter_t *a) {
  return a->io.now_ms ? a->io.now_ms(a->io.context) : 0;
}

static int actuator_slot(uint8_t actuator) {
  if (actuator == 1u) return 0;
  if (actuator == 2u) return 1;
  return -1;
}

static void frame_from_message(const mavlink_message_t *msg, uint8_t out[MOTOR_MAILBOX_BYTES]) {
  memset(out, 0, MOTOR_MAILBOX_BYTES);
  uint8_t wire[MAVLINK_MAX_PACKET_LEN];
  uint16_t len = mavlink_msg_to_send_buffer(wire, msg);
  if (len <= MOTOR_MAILBOX_BYTES) memcpy(out, wire, len);
}

static bool queued_frame_is_terminal(const uint8_t *frame) {
  uint32_t msgid = frame[7] | ((uint32_t)frame[8] << 8) | ((uint32_t)frame[9] << 16);
  if (msgid != MAVLINK_MSG_ID_REQUEST_RESULT) return false;
  uint8_t state = frame[27]; /* MAVLink v2 header (10 bytes) + REQUEST_RESULT.state offset. */
  return state == STATE_COMPLETED || state == STATE_STOPPED || state == STATE_REJECTED;
}

static bool same_request(const mavlink_actuator_request_t *left,
                         const mavlink_actuator_request_t *right) {
  return left->request_id == right->request_id &&
         left->ground_session == right->ground_session &&
         left->delta_cdeg == right->delta_cdeg && left->timeout_ms == right->timeout_ms &&
         left->max_rate_cdeg_s == right->max_rate_cdeg_s &&
         left->target_system == right->target_system &&
         left->target_component == right->target_component &&
         left->actuator == right->actuator && left->opcode == right->opcode &&
         left->control_source == right->control_source;
}

static void queue_remove(motor_adapter_t *a, uint8_t logical_index) {
  for (uint8_t i = logical_index; i + 1u < a->tx_count; ++i) {
    uint8_t dst = (uint8_t)((a->tx_head + i) % MOTOR_STATUS_QUEUE_CAPACITY);
    uint8_t src = (uint8_t)((a->tx_head + i + 1u) % MOTOR_STATUS_QUEUE_CAPACITY);
    memcpy(a->tx_queue[dst], a->tx_queue[src], MOTOR_MAILBOX_BYTES);
  }
  a->tx_count--;
}

static uint8_t active_move_count(const motor_adapter_t *a) {
  return (uint8_t)(a->state[0].active + a->state[1].active);
}

static void queue_message(motor_adapter_t *a, mavlink_message_t *msg) {
  uint8_t encoded[MOTOR_MAILBOX_BYTES];
  frame_from_message(msg, encoded);
  mavlink_request_result_t result_data;
  bool terminal = false;
  if (msg->msgid == MAVLINK_MSG_ID_REQUEST_RESULT) {
    mavlink_msg_request_result_decode(msg, &result_data);
    terminal = result_data.state == STATE_COMPLETED || result_data.state == STATE_STOPPED ||
               result_data.state == STATE_REJECTED;
  }
  uint8_t queue_limit = (uint8_t)(MOTOR_STATUS_QUEUE_CAPACITY - active_move_count(a));
  if (a->tx_count >= queue_limit) {
    uint8_t remove = MOTOR_STATUS_QUEUE_CAPACITY;
    for (uint8_t i = 0; i < a->tx_count; ++i) {
      uint8_t index = (uint8_t)((a->tx_head + i) % MOTOR_STATUS_QUEUE_CAPACITY);
      if (!queued_frame_is_terminal(a->tx_queue[index])) { remove = i; break; }
    }
    if (remove == MOTOR_STATUS_QUEUE_CAPACITY) {
      if (a->queue_drops != UINT16_MAX) a->queue_drops++;
      return;
    }
    queue_remove(a, remove);
    if (a->queue_drops != UINT16_MAX) a->queue_drops++;
  }
  if (terminal) {
    a->tx_head = (uint8_t)((a->tx_head + MOTOR_STATUS_QUEUE_CAPACITY - 1u) % MOTOR_STATUS_QUEUE_CAPACITY);
    memcpy(a->tx_queue[a->tx_head], encoded, MOTOR_MAILBOX_BYTES);
  } else {
    uint8_t tail = (uint8_t)((a->tx_head + a->tx_count) % MOTOR_STATUS_QUEUE_CAPACITY);
    memcpy(a->tx_queue[tail], encoded, MOTOR_MAILBOX_BYTES);
  }
  a->tx_count++;
  a->tx_frames++;
}

static void emit_result(motor_adapter_t *a, const mavlink_actuator_request_t *r,
                        uint32_t command, uint8_t result, uint8_t state) {
  mavlink_message_t msg;
  mavlink_msg_request_result_pack(MOTOR_NODE_SYSTEM, MOTOR_NODE_COMPONENT, &msg,
      r->ground_session, r->request_id, a->boot_id, command, result, state,
      TARGET_SYSTEM, TARGET_COMPONENT);
  int slot = actuator_slot(r->actuator);
  if (slot >= 0) {
    motor_actuator_state_t *accepted = &a->state[slot];
    if (accepted->seen && accepted->request_id == r->request_id &&
        accepted->ground_session == r->ground_session &&
        a->accepted_sysid[slot] == MOTOR_PI_SYSTEM &&
        a->accepted_compid[slot] == MOTOR_PI_COMPONENT &&
        same_request(&a->accepted_request[slot], r)) {
      frame_from_message(&msg, a->result_snapshot[slot]);
      a->result_snapshot_valid[slot] = true;
    } else if (state == STATE_REJECTED) {
      frame_from_message(&msg, a->rejected_snapshot[slot]);
      a->rejected_snapshot_valid[slot] = true;
    }
  }
  queue_message(a, &msg);
}

static void emit_status(motor_adapter_t *a, int slot, bool enqueue) {
  motor_actuator_state_t *s = &a->state[slot];
  motor_actuator_config_t *c = &a->config[slot];
  uint32_t now = now_ms(a);
  int32_t count = a->io.read_encoder_count ? a->io.read_encoder_count(a->io.context, c->actuator) : 0;
  int16_t measured = 0;
  bool fresh = c->reference_valid && a->io.feedback_is_fresh &&
               a->io.feedback_is_fresh(a->io.context, c->actuator, now);
  bool rc = a->io.rc_owns_motor && a->io.rc_owns_motor(a->io.context);
  if (fresh && c->counts_per_revolution != 0) {
    int64_t value = ((int64_t)(count - c->zero_count) * 36000) / c->counts_per_revolution;
    if (value < INT16_MIN) value = INT16_MIN;
    if (value > INT16_MAX) value = INT16_MAX;
    measured = (int16_t)value;
  }
  s->feedback_valid = fresh;
  mavlink_message_t msg;
  mavlink_msg_actuator_status_pack(MOTOR_NODE_SYSTEM, MOTOR_NODE_COMPONENT, &msg,
      now, s->local_command_id, s->target_cdeg, measured, s->velocity_cdeg_s,
      (uint16_t)(s->fault_flags | (fresh ? 0u : 2u)),
      c->actuator, rc ? SOURCE_RC : SOURCE_AUTONOMOUS,
      s->command_result, (uint8_t)(s->output_enabled && !rc));
  if (enqueue) queue_message(a, &msg);
  frame_from_message(&msg, a->snapshot[slot]);
  a->snapshot_valid[slot] = true;
}

static void emit_health(motor_adapter_t *a) {
  mavlink_message_t msg;
  bool valid = a->state[0].feedback_valid && a->state[1].feedback_valid;
  mavlink_msg_node_health_pack(MOTOR_NODE_SYSTEM, MOTOR_NODE_COMPONENT, &msg,
      a->boot_id, now_ms(a), valid ? 0u : 2u, 0, a->queue_drops, 0, (uint8_t)valid);
  queue_message(a, &msg);
}

static void emit_heartbeat(motor_adapter_t *a) {
  mavlink_message_t msg;
  mavlink_msg_heartbeat_pack(MOTOR_NODE_SYSTEM, MOTOR_NODE_COMPONENT, &msg,
      MAV_TYPE_GENERIC, MAV_AUTOPILOT_GENERIC, 0, 0, MAV_STATE_ACTIVE);
  queue_message(a, &msg);
}

void motor_adapter_init(motor_adapter_t *a, const motor_adapter_io_t *io,
                        const motor_actuator_config_t configs[2], uint32_t boot_id) {
  memset(a, 0, sizeof(*a));
  if (io) a->io = *io;
  if (configs) memcpy(a->config, configs, sizeof(a->config));
  a->boot_id = boot_id ? boot_id : 1u;
  a->next_command_id = 1;
  for (int i = 0; i < 2; ++i) {
    a->state[i].state = STATE_UNKNOWN;
    if (!a->config[i].max_rate_cdeg_s) a->config[i].max_rate_cdeg_s = MOTOR_DEFAULT_RATE_CDEG_S;
    if (!a->config[i].max_travel_cdeg) a->config[i].max_travel_cdeg = MOTOR_MAX_ANGLE_CDEG;
  }
  for (int i = 0; i < 2; ++i) emit_status(a, i, false);
}

static void stop_slot(motor_adapter_t *a, int slot, uint8_t state, uint8_t result) {
  motor_actuator_state_t *s = &a->state[slot];
  bool was_active = s->active;
  if (a->io.stop) a->io.stop(a->io.context, a->config[slot].actuator);
  s->active = false;
  s->state = state;
  s->result = result;
  s->output_enabled = false;
  s->velocity_cdeg_s = 0;
  s->command_result = result == RESULT_DENIED ? COMMAND_DENIED :
                      result == RESULT_INVALID ? COMMAND_INVALID :
                      result == RESULT_STALE ? COMMAND_TIMEOUT : COMMAND_ACCEPTED;
  s->fault_flags = result == RESULT_STALE ? 4u : 0u;
  if (s->seen && was_active) {
    mavlink_actuator_request_t r = a->accepted_request[slot];
    emit_result(a, &r, s->local_command_id, result, state);
  }
}

bool motor_adapter_process_message(motor_adapter_t *a, const mavlink_message_t *m) {
  if (!a || !m) return false;
  if (m->msgid == MAVLINK_MSG_ID_PING) {
    mavlink_ping_t ping;
    mavlink_msg_ping_decode(m, &ping);
    if (ping.target_system || ping.target_component) return true;
    mavlink_message_t reply;
    mavlink_msg_ping_pack(MOTOR_NODE_SYSTEM, MOTOR_NODE_COMPONENT, &reply,
        ping.time_usec, ping.seq, m->sysid, m->compid);
    queue_message(a, &reply);
    return true;
  }
  if (m->msgid != MAVLINK_MSG_ID_ACTUATOR_REQUEST) return false;
  a->rx_frames++;
  mavlink_actuator_request_t r;
  mavlink_msg_actuator_request_decode(m, &r);
  int slot = actuator_slot(r.actuator);
  if (m->sysid != MOTOR_PI_SYSTEM || m->compid != MOTOR_PI_COMPONENT ||
      r.target_system != MOTOR_NODE_SYSTEM || r.target_component != MOTOR_NODE_COMPONENT ||
      r.control_source != SOURCE_REMOTE_OPERATOR || slot < 0 ||
      r.request_id == 0 || r.ground_session == 0) {
    emit_result(a, &r, 0, RESULT_INVALID, STATE_REJECTED);
    return true;
  }
  motor_actuator_state_t *s = &a->state[slot];
  motor_actuator_config_t *c = &a->config[slot];
  uint32_t now = now_ms(a);
  for (uint8_t i = 0; i < a->old_session_count; ++i) {
    if (a->old_ground_sessions[i] == r.ground_session) {
      emit_result(a, &r, 0, RESULT_STALE, STATE_REJECTED);
      return true;
    }
  }
  if (a->current_ground_session == r.ground_session && a->have_request_id &&
      r.request_id == a->last_request_id) {
    const mavlink_actuator_request_t *prev = &a->last_request;
    bool same = a->last_request_sysid == m->sysid && a->last_request_compid == m->compid &&
                same_request(prev, &r);
    if (same && s->seen && r.request_id == s->request_id &&
        r.ground_session == s->ground_session) {
      emit_result(a, &r, s->local_command_id, s->result, s->state);
    } else {
      emit_result(a, &r, 0, RESULT_INVALID, STATE_REJECTED);
    }
    return true;
  }
  bool any_active = a->state[0].active || a->state[1].active;
  if (a->current_ground_session && r.ground_session != a->current_ground_session &&
      any_active && r.opcode != OPCODE_STOP) {
    emit_result(a, &r, 0, RESULT_DENIED, STATE_REJECTED);
    return true;
  }
  if (a->current_ground_session && r.ground_session != a->current_ground_session) {
    a->old_ground_sessions[a->old_session_next] = a->current_ground_session;
    a->old_session_next = (uint8_t)((a->old_session_next + 1u) % 4u);
    if (a->old_session_count < 4u) a->old_session_count++;
    a->current_ground_session = r.ground_session;
    a->have_request_id = false;
    for (int i = 0; i < 2; ++i) {
      if (i != slot && a->state[i].active) stop_slot(a, i, STATE_STOPPED, RESULT_STALE);
    }
  } else if (!a->current_ground_session) {
    a->current_ground_session = r.ground_session;
  }
  if (a->have_request_id && r.request_id <= a->last_request_id) {
    emit_result(a, &r, 0, RESULT_STALE, STATE_REJECTED);
    return true;
  }
  if (r.opcode != OPCODE_STOP &&
      (uint16_t)a->tx_count + active_move_count(a) + 4u > MOTOR_STATUS_QUEUE_CAPACITY) {
    emit_result(a, &r, 0, RESULT_DENIED, STATE_REJECTED);
    return true;
  }
  a->last_request_id = r.request_id;
  a->have_request_id = true;
  a->last_request = r;
  a->last_request_sysid = m->sysid;
  a->last_request_compid = m->compid;
  emit_result(a, &r, 0, RESULT_UNKNOWN, STATE_RECEIVED);
  if (r.opcode == OPCODE_STOP) {
    if (r.delta_cdeg != 0) {
      emit_result(a, &r, 0, RESULT_INVALID, STATE_REJECTED);
      return true;
    }
    if (a->io.rc_owns_motor && a->io.rc_owns_motor(a->io.context)) {
      emit_result(a, &r, 0, RESULT_DENIED, STATE_REJECTED);
      return true;
    }
    if (s->active) stop_slot(a, slot, STATE_STOPPED, RESULT_UNKNOWN);
    else if (a->io.stop) a->io.stop(a->io.context, c->actuator);
    s->request_id = r.request_id; s->ground_session = r.ground_session;
    s->local_command_id = (a->next_command_id && a->next_command_id != UINT32_MAX)
                              ? a->next_command_id++ : 0;
    s->seen = true;
    a->accepted_request[slot] = r;
    a->accepted_sysid[slot] = m->sysid;
    a->accepted_compid[slot] = m->compid;
    s->state = STATE_STOPPED; s->result = RESULT_UNKNOWN; s->active = false;
    s->command_result = COMMAND_ACCEPTED; s->fault_flags = 0;
    s->output_enabled = false; s->velocity_cdeg_s = 0;
    emit_result(a, &r, s->local_command_id, RESULT_UNKNOWN, STATE_STOPPED);
    emit_status(a, slot, true);
    return true;
  }
  if (a->io.rc_owns_motor && a->io.rc_owns_motor(a->io.context)) {
    if (a->io.stop) a->io.stop(a->io.context, c->actuator);
    emit_result(a, &r, 0, RESULT_DENIED, STATE_REJECTED);
    return true;
  }
  if (r.opcode != OPCODE_ABSOLUTE && r.opcode != OPCODE_STEP) {
    emit_result(a, &r, 0, RESULT_INVALID, STATE_REJECTED);
    return true;
  }
  if (s->active) {
    emit_result(a, &r, 0, RESULT_DENIED, STATE_REJECTED);
    return true;
  }
  if (!a->next_command_id || a->next_command_id == UINT32_MAX) {
    emit_result(a, &r, 0, RESULT_DENIED, STATE_REJECTED);
    return true;
  }
  if (!c->reference_valid || !c->counts_per_revolution || !a->io.read_encoder_count ||
      !a->io.feedback_is_fresh || !a->io.feedback_is_fresh(a->io.context, c->actuator, now)) {
    emit_result(a, &r, 0, RESULT_INVALID, STATE_REJECTED);
    return true;
  }
  int32_t count = a->io.read_encoder_count(a->io.context, c->actuator);
  int64_t current = ((int64_t)(count - c->zero_count) * 36000) / c->counts_per_revolution;
  int64_t target = r.delta_cdeg;
  if (r.opcode == OPCODE_STEP) target += current;
  if (target < 0 || target > c->max_travel_cdeg || target > MOTOR_MAX_ANGLE_CDEG) {
    emit_result(a, &r, 0, RESULT_INVALID, STATE_REJECTED);
    return true;
  }
  s->request_id = r.request_id;
  s->ground_session = r.ground_session;
  s->local_command_id = a->next_command_id++;
  s->target_cdeg = (int16_t)target;
  s->rate_cdeg_s = r.max_rate_cdeg_s ? r.max_rate_cdeg_s : c->max_rate_cdeg_s;
  if (s->rate_cdeg_s > c->max_rate_cdeg_s) s->rate_cdeg_s = c->max_rate_cdeg_s;
  s->started_ms = now;
  uint32_t requested_timeout = r.timeout_ms ? r.timeout_ms : MOTOR_DEFAULT_TIMEOUT_MS;
  if (requested_timeout > MOTOR_DEFAULT_TIMEOUT_MS) requested_timeout = MOTOR_DEFAULT_TIMEOUT_MS;
  s->deadline_ms = now + requested_timeout;
  s->last_feedback_ms = now;
  s->last_count = count;
  s->state = STATE_ACCEPTED; s->result = RESULT_UNKNOWN; s->active = true; s->seen = true;
  a->accepted_request[slot] = r;
  a->accepted_sysid[slot] = m->sysid;
  a->accepted_compid[slot] = m->compid;
  s->command_result = COMMAND_ACCEPTED; s->fault_flags = 0; s->output_enabled = false;
  emit_result(a, &r, s->local_command_id, RESULT_UNKNOWN, STATE_ACCEPTED);
  emit_status(a, slot, true);
  return true;
}

bool motor_adapter_receive(motor_adapter_t *a, const uint8_t frame[MOTOR_MAILBOX_BYTES]) {
  if (!a || !frame || frame[0] != MAVLINK_STX) return false;
  mavlink_message_t parse_msg;
  mavlink_message_t message;
  mavlink_status_t status;
  memset(&parse_msg, 0, sizeof(parse_msg));
  memset(&message, 0, sizeof(message));
  memset(&status, 0, sizeof(status));
  bool complete = false;
  size_t frame_len = 0;
  for (size_t i = 0; i < MOTOR_MAILBOX_BYTES; ++i) {
    if (complete) {
      if (frame[i] != 0) { a->rx_errors++; return false; }
      continue;
    }
    uint8_t result = mavlink_frame_char_buffer(&parse_msg, &status, frame[i], &message, &status);
    if (result == MAVLINK_FRAMING_BAD_CRC || result == MAVLINK_FRAMING_BAD_SIGNATURE) {
      a->rx_errors++; return false;
    }
    frame_len++;
    if (result == MAVLINK_FRAMING_OK) complete = true;
  }
  if (!complete || frame[0] != MAVLINK_STX ||
      (message.incompat_flags & MAVLINK_IFLAG_SIGNED) || frame_len > MOTOR_MAILBOX_BYTES) {
    a->rx_errors++; return false;
  }
  return motor_adapter_process_message(a, &message);
}

void motor_adapter_poll(motor_adapter_t *a) {
  if (!a) return;
  uint32_t now = now_ms(a);
  bool rc = a->io.rc_owns_motor && a->io.rc_owns_motor(a->io.context);
  for (int i = 0; i < 2; ++i) {
    motor_actuator_state_t *s = &a->state[i];
    motor_actuator_config_t *c = &a->config[i];
    if (s->active) {
      if (rc) { stop_slot(a, i, STATE_STOPPED, RESULT_DENIED); }
      else if ((int32_t)(now - s->deadline_ms) >= 0) { stop_slot(a, i, STATE_STOPPED, RESULT_STALE); }
      else if (!c->reference_valid || !a->io.feedback_is_fresh ||
               !a->io.feedback_is_fresh(a->io.context, c->actuator, now)) {
        stop_slot(a, i, STATE_STOPPED, RESULT_STALE);
      } else {
        int32_t count = a->io.read_encoder_count(a->io.context, c->actuator);
        int64_t current = ((int64_t)(count - c->zero_count) * 36000) / c->counts_per_revolution;
        int64_t error = (int64_t)s->target_cdeg - current;
        if (error >= -100 && error <= 100) stop_slot(a, i, STATE_COMPLETED, RESULT_COMPLETED);
        else {
          int direction = error > 0 ? 1 : -1;
          uint32_t elapsed = now - s->last_feedback_ms;
          if (elapsed) {
            int64_t velocity = ((int64_t)(count - s->last_count) * 3600000) /
                               ((int64_t)c->counts_per_revolution * elapsed);
            if (velocity < INT16_MIN) velocity = INT16_MIN;
            if (velocity > INT16_MAX) velocity = INT16_MAX;
            s->velocity_cdeg_s = (int16_t)velocity;
          }
          uint32_t speed = s->velocity_cdeg_s < 0 ? -(int32_t)s->velocity_cdeg_s : s->velocity_cdeg_s;
          if (speed > s->rate_cdeg_s) {
            if (a->io.stop) a->io.stop(a->io.context, c->actuator);
            s->output_enabled = false;
          } else if (a->io.drive) {
            uint32_t pwm = (uint32_t)s->rate_cdeg_s * MOTOR_PWM / c->max_rate_cdeg_s;
            if (pwm < MOTOR_MIN_PWM) pwm = MOTOR_MIN_PWM;
            if (pwm > MOTOR_PWM) pwm = MOTOR_PWM;
            a->io.drive(a->io.context, c->actuator, direction, (uint8_t)pwm);
            s->output_enabled = true;
          }
          if (s->state != STATE_RUNNING) {
            s->state = STATE_RUNNING;
            s->command_result = COMMAND_IN_PROGRESS;
            mavlink_actuator_request_t request = a->accepted_request[i];
            emit_result(a, &request, s->local_command_id, RESULT_UNKNOWN, STATE_RUNNING);
          }
          s->last_count = count; s->last_feedback_ms = now;
        }
      }
      emit_status(a, i, false);
    } else emit_status(a, i, false);
  }
  /* Health is a persistent event snapshot; do not flood the bounded queue. */
  if ((uint32_t)(now - a->last_health_ms) >= 1000u) {
    a->last_health_ms = now;
    emit_health(a);
    emit_heartbeat(a);
  }
}

void motor_adapter_mailbox_read(motor_adapter_t *a, uint8_t out[MOTOR_MAILBOX_BYTES]) {
  if (!a || !out) return;
  if (a->tx_count) {
    memcpy(out, a->tx_queue[a->tx_head], MOTOR_MAILBOX_BYTES);
    a->tx_head = (uint8_t)((a->tx_head + 1u) % MOTOR_STATUS_QUEUE_CAPACITY);
    a->tx_count--;
  } else {
    for (uint8_t attempts = 0; attempts < 6; ++attempts) {
      uint8_t selected = a->persistent_next;
      a->persistent_next = (uint8_t)((a->persistent_next + 1u) % 6u);
      if (selected < 2u && a->result_snapshot_valid[selected]) {
        memcpy(out, a->result_snapshot[selected], MOTOR_MAILBOX_BYTES);
        return;
      }
      if (selected < 4u && selected >= 2u && a->snapshot_valid[selected - 2u]) {
        memcpy(out, a->snapshot[selected - 2u], MOTOR_MAILBOX_BYTES);
        return;
      }
      if (selected >= 4u && a->rejected_snapshot_valid[selected - 4u]) {
        memcpy(out, a->rejected_snapshot[selected - 4u], MOTOR_MAILBOX_BYTES);
        return;
      }
    }
    memset(out, 0, MOTOR_MAILBOX_BYTES);
  }
}
