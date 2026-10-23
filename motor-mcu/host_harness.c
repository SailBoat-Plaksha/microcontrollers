#include "adapter.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  uint32_t now;
  int32_t count[2];
  bool fresh;
  bool rc;
  unsigned drive_count;
  unsigned stop_count;
  int last_direction;
} fake_t;

static uint32_t fake_now(void *p) { return ((fake_t *)p)->now; }
static int32_t fake_count(void *p, uint8_t id) { return ((fake_t *)p)->count[id - 1]; }
static bool fake_fresh(void *p, uint8_t id, uint32_t now) { (void)id; (void)now; return ((fake_t *)p)->fresh; }
static void fake_drive(void *p, uint8_t id, int dir, uint8_t pwm) {
  (void)id; assert(pwm > 0); fake_t *f = p; f->drive_count++; f->last_direction = dir;
}
static void fake_stop(void *p, uint8_t id) { (void)id; ((fake_t *)p)->stop_count++; }
static bool fake_rc(void *p) { return ((fake_t *)p)->rc; }

static void request(motor_adapter_t *a, uint32_t session, uint32_t id, uint8_t actuator,
                    uint8_t opcode, int16_t delta, uint16_t timeout, uint8_t source,
                    uint8_t sysid, uint8_t compid, uint8_t target) {
  mavlink_message_t msg;
  mavlink_msg_actuator_request_pack(sysid, compid, &msg, id, session, delta, timeout,
      5000, target, 154, actuator, opcode, source);
  uint8_t frame[MOTOR_MAILBOX_BYTES] = {0};
  uint8_t raw[MAVLINK_MAX_PACKET_LEN];
  uint16_t len = mavlink_msg_to_send_buffer(raw, &msg);
  assert(len <= sizeof(frame));
  memcpy(frame, raw, len);
  if (!motor_adapter_receive(a, frame)) { fprintf(stderr, "receive false len=%u msgid=%u err=%u flags=%u state=%u\n", len, (unsigned)(raw[7] | ((uint32_t)raw[8] << 8) | ((uint32_t)raw[9] << 16)), a->rx_errors, frame[0], a->rx_frames); assert(0); }
}

static uint32_t queued_msgid(motor_adapter_t *a, uint8_t out[MOTOR_MAILBOX_BYTES]) {
  motor_adapter_mailbox_read(a, out);
  if (out[0] != MAVLINK_STX) return 0;
  return out[7] | ((uint32_t)out[8] << 8) | ((uint32_t)out[9] << 16);
}

static bool decode_message(const uint8_t frame[MOTOR_MAILBOX_BYTES], mavlink_message_t *out) {
  mavlink_message_t parser = {0}, message = {0};
  mavlink_status_t status = {0};
  bool complete = false;
  for (size_t i = 0; i < MOTOR_MAILBOX_BYTES; ++i) {
    if (complete) break;
    uint8_t result = mavlink_frame_char_buffer(&parser, &status, frame[i], &message, &status);
    if (result == MAVLINK_FRAMING_OK) complete = true;
  }
  if (!complete) return false;
  *out = message;
  return true;
}

static bool decode_result_state(const uint8_t frame[MOTOR_MAILBOX_BYTES], uint8_t *state) {
  mavlink_message_t message;
  if (!decode_message(frame, &message) || message.msgid != MAVLINK_MSG_ID_REQUEST_RESULT) return false;
  mavlink_request_result_t result;
  mavlink_msg_request_result_decode(&message, &result);
  *state = result.state;
  return true;
}

int main(void) {
  fake_t f = {.fresh = true};
  motor_adapter_io_t io = {&f, fake_now, fake_count, fake_fresh, fake_drive, fake_stop, fake_rc};
  motor_actuator_config_t cfg[2] = {
    {.actuator = 1, .zero_count = 0, .counts_per_revolution = 36000, .max_rate_cdeg_s = 5000,
     .max_travel_cdeg = 18000, .reference_valid = true},
    {.actuator = 2, .zero_count = 0, .counts_per_revolution = 36000, .max_rate_cdeg_s = 5000,
     .max_travel_cdeg = 18000, .reference_valid = true}
  };
  motor_adapter_t a;
  motor_adapter_init(&a, &io, cfg, 0x12345678);
  uint8_t frame[MOTOR_MAILBOX_BYTES];

  cfg[0].reference_valid = false;
  motor_adapter_init(&a, &io, cfg, 1);
  mavlink_message_t status_msg;
  assert(decode_message(a.snapshot[0], &status_msg));
  mavlink_actuator_status_t status;
  mavlink_msg_actuator_status_decode(&status_msg, &status);
  assert(status.measured_position_cdeg == 0 && status.fault_flags == 2);
  request(&a, 10, 1, 1, 1, 4500, 1000, 3, 1, 1, 1);
  assert(f.drive_count == 0);
  assert(queued_msgid(&a, frame) == MAVLINK_MSG_ID_REQUEST_RESULT);

  cfg[0].reference_valid = true;
  motor_adapter_init(&a, &io, cfg, 2);
  request(&a, 9, 1, 1, 1, 18100, 1000, 3, 1, 1, 1); /* hard travel ceiling */
  assert(!a.state[0].active && f.drive_count == 0);
  a.tx_count = 0;
  request(&a, 10, 1, 1, 1, 4500, 1000, 3, 1, 1, 1);
  assert(a.state[0].target_cdeg == 4500 && a.state[0].active);
  motor_adapter_poll(&a);
  assert(f.drive_count == 1 && f.last_direction == 1);
  request(&a, 10, 1, 1, 1, 4500, 1000, 3, 1, 1, 1); /* duplicate */
  assert(a.next_command_id == 2 && f.drive_count == 1);
  request(&a, 10, 1, 2, 1, 9000, 1000, 3, 1, 1, 1); /* same ID cannot target another actuator */
  assert(!a.state[1].active && a.next_command_id == 2);
  request(&a, 10, 2, 1, 1, 9000, 1000, 3, 1, 1, 1); /* busy motor cannot be restarted */
  assert(a.state[0].active && a.state[0].request_id == 1 && a.next_command_id == 2);
  request(&a, 10, 0, 1, 2, 500, 1000, 3, 1, 1, 1); /* reordered */
  assert(a.state[0].target_cdeg == 4500);
  f.count[0] = 4500;
  motor_adapter_poll(&a);
  assert(!a.state[0].active && a.state[0].state == 4);
  assert(decode_message(a.snapshot[0], &status_msg));
  mavlink_msg_actuator_status_decode(&status_msg, &status);
  assert(status.requested_position_cdeg == 4500 && status.measured_position_cdeg == 4500);
  a.tx_count = 0;

  /* STEP resolves once at fresh feedback and duplicate cannot add again. */
  f.count[0] = 1000;
  request(&a, 11, 2, 1, 2, 500, 1000, 3, 1, 1, 1);
  assert(a.state[0].target_cdeg == 1500);
  request(&a, 11, 2, 1, 2, 500, 1000, 3, 1, 1, 1);
  assert(a.state[0].target_cdeg == 1500);

  /* RC takes ownership and stops autonomous output before any further drive. */
  f.rc = true;
  unsigned drives_before = f.drive_count;
  motor_adapter_poll(&a);
  assert(f.drive_count == drives_before && !a.state[0].active);
  a.tx_count = 0;
  request(&a, 12, 1, 1, 1, 9000, 5000, 3, 1, 1, 1);
  assert(f.drive_count == drives_before);
  f.rc = false;
  request(&a, 10, 99, 1, 1, 9000, 5000, 3, 1, 1, 1); /* superseded session never replays */
  uint8_t stale_state;
  assert(decode_result_state(a.result_snapshot[0], &stale_state) && stale_state == 6);

  /* STOP is accepted and distinct from an absolute zero target. */
  a.tx_count = 0;
  request(&a, 13, 1, 2, 3, 0, 0, 3, 1, 1, 1);
  assert(a.state[1].state == 5 && !a.state[1].active);
  assert(queued_msgid(&a, frame) == MAVLINK_MSG_ID_REQUEST_RESULT);

  /* Deadline ends the move without a late/replayed drive. */
  a.tx_count = 0;
  request(&a, 14, 1, 1, 1, 10000, 20, 3, 1, 1, 1);
  f.now = 21;
  drives_before = f.drive_count;
  motor_adapter_poll(&a);
  assert(!a.state[0].active && f.drive_count == drives_before);

  /* Wrong authority/source/target and damaged frame do not drive. */
  a.tx_count = 0;
  request(&a, 15, 1, 1, 1, 1000, 1000, 2, 1, 1, 1);
  request(&a, 15, 2, 1, 1, 1000, 1000, 3, 4, 1, 1);
  request(&a, 15, 3, 1, 1, 1000, 1000, 3, 1, 1, 9);
  assert(f.drive_count == drives_before);
  mavlink_message_t msg;
  mavlink_msg_actuator_request_pack(1, 1, &msg, 1, 20, 100, 1000, 2000,
      1, 154, 1, 1, 3);
  uint8_t bad[MOTOR_MAILBOX_BYTES] = {0}, raw[MAVLINK_MAX_PACKET_LEN];
  uint16_t len = mavlink_msg_to_send_buffer(raw, &msg); memcpy(bad, raw, len); bad[14] ^= 0x01;
  assert(!motor_adapter_receive(&a, bad));
  assert(f.drive_count == drives_before);

  /* Host API's persistent snapshot and health/ping messages use generated C headers. */
  motor_adapter_poll(&a);
  assert(a.snapshot_valid[0] && a.snapshot_valid[1]);
  mavlink_message_t ping;
  mavlink_msg_ping_pack(1, 1, &ping, 77, 8, 0, 0);
  a.tx_count = 0;
  assert(motor_adapter_process_message(&a, &ping));
  assert(queued_msgid(&a, frame) == MAVLINK_MSG_ID_PING);

  /* Every accepted move exposes received, accepted, running and terminal states. */
  a.tx_count = 0;
  f.count[0] = 0;
  request(&a, 16, 1, 1, 1, 1000, 1000, 3, 1, 1, 1);
  motor_adapter_poll(&a);
  f.count[0] = 1000;
  motor_adapter_poll(&a);
  bool saw_received = false, saw_accepted = false, saw_running = false, saw_terminal = false;
  for (uint8_t i = 0; i < a.tx_count; ++i) {
    uint8_t slot = (uint8_t)((a.tx_head + i) % MOTOR_STATUS_QUEUE_CAPACITY);
    uint8_t state;
    if (decode_result_state(a.tx_queue[slot], &state)) {
      saw_received |= state == 1;
      saw_accepted |= state == 2;
      saw_running |= state == 3;
      saw_terminal |= state == 4;
    }
  }
  assert(saw_received && saw_accepted && saw_running && saw_terminal);

  /* A full terminal queue rejects new motion and leaves a persistent rejection snapshot. */
  uint8_t rejected_before = f.drive_count;
  for (uint8_t i = 0; i < MOTOR_STATUS_QUEUE_CAPACITY; ++i)
    memcpy(a.tx_queue[i], a.result_snapshot[0], MOTOR_MAILBOX_BYTES);
  a.tx_head = 0;
  a.tx_count = MOTOR_STATUS_QUEUE_CAPACITY;
  request(&a, 16, 2, 2, 1, 1000, 1000, 3, 1, 1, 1);
  uint8_t terminal_state;
  assert(f.drive_count == rejected_before);
  assert(decode_result_state(a.result_snapshot[1], &terminal_state) && terminal_state == 6);
  printf("motor adapter scenarios: PASS (reference, absolute, STEP, duplicate/reorder/cross-actuator, RC, STOP, deadline, source/authority/target, CRC, bounded queue, persistent snapshot, ping, request states)\n");
  return 0;
}
