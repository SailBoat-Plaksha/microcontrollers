#ifndef MOTOR_MCU_ADAPTER_H
#define MOTOR_MCU_ADAPTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mavlink.h"

#define MOTOR_MAILBOX_ADDR 0x08u
#define MOTOR_MAILBOX_BYTES 64u
#define MOTOR_NODE_SYSTEM 1u
#define MOTOR_NODE_COMPONENT 154u
#define MOTOR_PI_SYSTEM 1u
#define MOTOR_PI_COMPONENT 1u
#define MOTOR_MAX_ANGLE_CDEG 18000
#define MOTOR_FEEDBACK_MAX_AGE_MS 100u
#define MOTOR_DEFAULT_TIMEOUT_MS 10000u
#define MOTOR_DEFAULT_RATE_CDEG_S 3000u
#define MOTOR_STATUS_QUEUE_CAPACITY 16u

typedef struct {
  void *context;
  uint32_t (*now_ms)(void *context);
  int32_t (*read_encoder_count)(void *context, uint8_t actuator);
  bool (*feedback_is_fresh)(void *context, uint8_t actuator, uint32_t now_ms);
  void (*drive)(void *context, uint8_t actuator, int direction, uint8_t pwm);
  void (*stop)(void *context, uint8_t actuator);
  bool (*rc_owns_motor)(void *context);
} motor_adapter_io_t;

typedef struct {
  uint8_t actuator;
  int32_t zero_count;
  uint16_t counts_per_revolution;
  uint16_t max_rate_cdeg_s;
  uint16_t max_travel_cdeg;
  bool reference_valid;
} motor_actuator_config_t;

typedef struct {
  uint32_t request_id;
  uint32_t ground_session;
  uint32_t local_command_id;
  int16_t target_cdeg;
  uint16_t rate_cdeg_s;
  uint32_t started_ms;
  uint32_t deadline_ms;
  uint32_t last_feedback_ms;
  int32_t last_count;
  int16_t velocity_cdeg_s;
  uint8_t state;
  uint8_t result;
  uint8_t command_result;
  uint16_t fault_flags;
  bool active;
  bool output_enabled;
  bool seen;
  bool feedback_valid;
} motor_actuator_state_t;

typedef struct {
  motor_adapter_io_t io;
  motor_actuator_config_t config[2];
  motor_actuator_state_t state[2];
  uint32_t boot_id;
  uint32_t next_command_id;
  uint16_t queue_drops;
  uint32_t rx_frames;
  uint32_t rx_errors;
  uint32_t tx_frames;
  uint32_t last_health_ms;
  uint32_t current_ground_session;
  uint32_t last_request_id;
  bool have_request_id;
  uint32_t old_ground_sessions[4];
  uint8_t old_session_count;
  uint8_t old_session_next;
  uint8_t tx_queue[MOTOR_STATUS_QUEUE_CAPACITY][MOTOR_MAILBOX_BYTES];
  uint8_t tx_count;
  uint8_t tx_head;
  uint8_t snapshot[2][MOTOR_MAILBOX_BYTES];
  bool snapshot_valid[2];
  uint8_t snapshot_next;
  uint8_t result_snapshot[2][MOTOR_MAILBOX_BYTES];
  bool result_snapshot_valid[2];
  uint8_t persistent_next;
} motor_adapter_t;

/* Motor reference calibration must be explicit. Zero-initialized configuration disables moves. */
void motor_adapter_init(motor_adapter_t *adapter, const motor_adapter_io_t *io,
                        const motor_actuator_config_t configs[2], uint32_t boot_id);
/* Consume exactly one zero-padded 64-byte mailbox frame. */
bool motor_adapter_receive(motor_adapter_t *adapter, const uint8_t frame[MOTOR_MAILBOX_BYTES]);
/* Poll nonblocking moves and refresh the persistent snapshot. */
void motor_adapter_poll(motor_adapter_t *adapter);
/* Read next queued response; when empty, return the persistent snapshot (or zeros). */
void motor_adapter_mailbox_read(motor_adapter_t *adapter, uint8_t out[MOTOR_MAILBOX_BYTES]);
/* Deterministic API for harness and firmware event generation. */
bool motor_adapter_process_message(motor_adapter_t *adapter, const mavlink_message_t *message);

#endif
