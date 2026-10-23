#include "adapter.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint32_t now;
  int32_t count[2];
  bool fresh[2];
  bool rc;
  unsigned drives[2];
  unsigned stops[2];
  int direction[2];
  uint8_t pwm[2];
} host_io_t;

static uint32_t host_now(void *context) { return ((host_io_t *)context)->now; }
static int32_t host_count(void *context, uint8_t actuator) {
  return ((host_io_t *)context)->count[actuator - 1u];
}
static bool host_fresh(void *context, uint8_t actuator, uint32_t now) {
  (void)now;
  return ((host_io_t *)context)->fresh[actuator - 1u];
}
static void host_drive(void *context, uint8_t actuator, int direction, uint8_t pwm) {
  host_io_t *io = context;
  io->drives[actuator - 1u]++;
  io->direction[actuator - 1u] = direction;
  io->pwm[actuator - 1u] = pwm;
}
static void host_stop(void *context, uint8_t actuator) {
  host_io_t *io = context;
  io->stops[actuator - 1u]++;
  io->pwm[actuator - 1u] = 0;
}
static bool host_rc(void *context) { return ((host_io_t *)context)->rc; }

static bool parse_frame(const char *hex, uint8_t frame[MOTOR_MAILBOX_BYTES]) {
  if (strlen(hex) != MOTOR_MAILBOX_BYTES * 2u) return false;
  for (size_t i = 0; i < MOTOR_MAILBOX_BYTES; ++i) {
    unsigned value;
    if (sscanf(hex + i * 2u, "%2x", &value) != 1) return false;
    frame[i] = (uint8_t)value;
  }
  return true;
}

static void print_frame(const uint8_t frame[MOTOR_MAILBOX_BYTES]) {
  for (size_t i = 0; i < MOTOR_MAILBOX_BYTES; ++i) printf("%02x", frame[i]);
  putchar('\n');
}

int main(void) {
  host_io_t fake = {.fresh = {true, true}};
  motor_adapter_io_t io = {&fake, host_now, host_count, host_fresh, host_drive, host_stop, host_rc};
  motor_actuator_config_t config[2] = {
    {.actuator = 1, .counts_per_revolution = 36000, .max_rate_cdeg_s = 5000,
     .max_travel_cdeg = 18000, .reference_valid = true},
    {.actuator = 2, .counts_per_revolution = 36000, .max_rate_cdeg_s = 5000,
     .max_travel_cdeg = 18000, .reference_valid = true}
  };
  motor_adapter_t adapter;
  motor_adapter_init(&adapter, &io, config, 0x12345678u);
  char line[512];
  while (fgets(line, sizeof(line), stdin)) {
    char command[24] = {0};
    if (sscanf(line, "%23s", command) != 1) continue;
    if (strcmp(command, "NOW") == 0) {
      unsigned now;
      if (sscanf(line, "%*s %u", &now) != 1) puts("ERR args");
      else { fake.now = now; puts("OK"); }
    } else if (strcmp(command, "COUNT") == 0) {
      unsigned actuator;
      int32_t count;
      if (sscanf(line, "%*s %u %" SCNd32, &actuator, &count) != 2 || actuator < 1 || actuator > 2)
        puts("ERR args");
      else { fake.count[actuator - 1u] = count; puts("OK"); }
    } else if (strcmp(command, "FRESH") == 0) {
      unsigned actuator, value;
      if (sscanf(line, "%*s %u %u", &actuator, &value) != 2 || actuator < 1 || actuator > 2 || value > 1)
        puts("ERR args");
      else { fake.fresh[actuator - 1u] = value != 0; puts("OK"); }
    } else if (strcmp(command, "RC") == 0) {
      unsigned value;
      if (sscanf(line, "%*s %u", &value) != 1 || value > 1) puts("ERR args");
      else { fake.rc = value != 0; puts("OK"); }
    } else if (strcmp(command, "POLL") == 0) {
      motor_adapter_poll(&adapter);
      puts("OK");
    } else if (strcmp(command, "ENCODE") == 0) {
      unsigned sys, comp, actuator, opcode, source, target_sys, target_comp;
      uint32_t request_id, session, timeout;
      int delta;
      unsigned rate;
      if (sscanf(line, "%*s %u %u %" SCNu32 " %" SCNu32 " %d %" SCNu32 " %u %u %u %u %u %u",
                 &sys, &comp, &request_id, &session, &delta, &timeout, &rate,
                 &target_sys, &target_comp, &actuator, &opcode, &source) != 12 ||
          sys > 255 || comp > 255 || actuator > 255 || opcode > 255 || source > 255 ||
          target_sys > 255 || target_comp > 255 || rate > UINT16_MAX ||
          delta < INT16_MIN || delta > INT16_MAX) {
        puts("ERR args");
      } else {
        mavlink_message_t message;
        mavlink_msg_actuator_request_pack((uint8_t)sys, (uint8_t)comp, &message,
            request_id, session, (int16_t)delta, timeout, (uint16_t)rate,
            (uint8_t)target_sys, (uint8_t)target_comp, (uint8_t)actuator,
            (uint8_t)opcode, (uint8_t)source);
        uint8_t frame[MOTOR_MAILBOX_BYTES] = {0};
        uint8_t raw[MAVLINK_MAX_PACKET_LEN];
        uint16_t length = mavlink_msg_to_send_buffer(raw, &message);
        if (length > sizeof(frame)) puts("ERR frame");
        else { memcpy(frame, raw, length); print_frame(frame); }
      }
    } else if (strcmp(command, "RX") == 0) {
      char hex[MOTOR_MAILBOX_BYTES * 2u + 2u] = {0};
      uint8_t frame[MOTOR_MAILBOX_BYTES];
      if (sscanf(line, "%*s %129s", hex) != 1 || !parse_frame(hex, frame)) puts("ERR frame");
      else printf("RX %u\n", motor_adapter_receive(&adapter, frame) ? 1u : 0u);
    } else if (strcmp(command, "READ") == 0) {
      uint8_t frame[MOTOR_MAILBOX_BYTES];
      motor_adapter_mailbox_read(&adapter, frame);
      print_frame(frame);
    } else if (strcmp(command, "STATE") == 0) {
      printf("STATE %" PRIu32 " %" PRIu32 " %u %u %u %u %u %u %u %u\n",
          adapter.next_command_id, adapter.last_request_id,
          adapter.state[0].active, adapter.state[0].output_enabled,
          adapter.state[0].state, (unsigned)(uint16_t)adapter.state[0].velocity_cdeg_s,
          adapter.state[1].active, adapter.state[1].output_enabled,
          fake.drives[0], fake.drives[1]);
    } else if (strcmp(command, "QUIT") == 0) {
      break;
    } else {
      puts("ERR command");
    }
    fflush(stdout);
  }
  return 0;
}
