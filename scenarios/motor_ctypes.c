#include "adapter.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  motor_adapter_t adapter;
  uint32_t now;
  int32_t count[2];
  bool fresh[2];
  bool rc;
  int output_enabled[2];
  int direction[2];
  unsigned drive_calls[2];
  unsigned stop_calls[2];
} scenario_motor_t;

static uint32_t clock_ms(void *p) { return ((scenario_motor_t *)p)->now; }
static int slot(uint8_t actuator) { return actuator == 1 ? 0 : actuator == 2 ? 1 : -1; }
static int32_t encoder(void *p, uint8_t actuator) {
  int i = slot(actuator); return i < 0 ? 0 : ((scenario_motor_t *)p)->count[i];
}
static bool feedback(void *p, uint8_t actuator, uint32_t now) {
  (void)now; int i = slot(actuator); return i >= 0 && ((scenario_motor_t *)p)->fresh[i];
}
static void drive(void *p, uint8_t actuator, int direction, uint8_t pwm) {
  scenario_motor_t *m = p; int i = slot(actuator); if (i < 0 || !pwm) return;
  m->output_enabled[i] = 1; m->direction[i] = direction; m->drive_calls[i]++;
}
static void stop(void *p, uint8_t actuator) {
  scenario_motor_t *m = p; int i = slot(actuator); if (i < 0) return;
  m->output_enabled[i] = 0; m->stop_calls[i]++;
}
static bool rc_owned(void *p) { return ((scenario_motor_t *)p)->rc; }

void *scenario_motor_create(uint32_t boot_id) {
  scenario_motor_t *m = calloc(1, sizeof(*m)); if (!m) return NULL;
  m->fresh[0] = m->fresh[1] = true;
  motor_adapter_io_t io = {m, clock_ms, encoder, feedback, drive, stop, rc_owned};
  motor_actuator_config_t cfg[2] = {
    {.actuator=1, .zero_count=0, .counts_per_revolution=36000,
     .max_rate_cdeg_s=5000, .max_travel_cdeg=18000, .reference_valid=true},
    {.actuator=2, .zero_count=0, .counts_per_revolution=36000,
     .max_rate_cdeg_s=5000, .max_travel_cdeg=18000, .reference_valid=true}
  };
  motor_adapter_init(&m->adapter, &io, cfg, boot_id);
  return m;
}
void scenario_motor_destroy(void *p) { free(p); }
void scenario_motor_inputs(void *p, uint32_t now, int32_t c1, int32_t c2,
                           int f1, int f2, int rc) {
  scenario_motor_t *m = p; if (!m) return;
  m->now=now; m->count[0]=c1; m->count[1]=c2;
  m->fresh[0]=f1 != 0; m->fresh[1]=f2 != 0; m->rc=rc != 0;
}
int scenario_motor_receive(void *p, const uint8_t *data) {
  return p && data ? motor_adapter_receive(&((scenario_motor_t *)p)->adapter, data) : 0;
}
void scenario_motor_poll(void *p) { if (p) motor_adapter_poll(&((scenario_motor_t *)p)->adapter); }
void scenario_motor_read(void *p, uint8_t *out) {
  if (p && out) motor_adapter_mailbox_read(&((scenario_motor_t *)p)->adapter, out);
}
void scenario_motor_snapshot(void *p, int i, uint8_t *out) {
  if (p && out && i >= 0 && i < 2)
    memcpy(out, ((scenario_motor_t *)p)->adapter.snapshot[i], MOTOR_MAILBOX_BYTES);
}
int scenario_motor_active(void *p, int i) { return p && i>=0 && i<2 ? ((scenario_motor_t*)p)->adapter.state[i].active : 0; }
int scenario_motor_enabled(void *p, int i) { return p && i>=0 && i<2 ? ((scenario_motor_t*)p)->output_enabled[i] : 0; }
int scenario_motor_direction(void *p, int i) { return p && i>=0 && i<2 ? ((scenario_motor_t*)p)->direction[i] : 0; }
int32_t scenario_motor_target(void *p, int i) { return p && i>=0 && i<2 ? ((scenario_motor_t*)p)->adapter.state[i].target_cdeg : 0; }
int32_t scenario_motor_measured(void *p, int i) { return p && i>=0 && i<2 ? ((scenario_motor_t*)p)->adapter.state[i].last_count : 0; }
unsigned scenario_motor_drives(void *p, int i) { return p && i>=0 && i<2 ? ((scenario_motor_t*)p)->drive_calls[i] : 0; }
uint32_t scenario_motor_boot(void *p) { return p ? ((scenario_motor_t*)p)->adapter.boot_id : 0; }
unsigned scenario_motor_rx_errors(void *p) { return p ? ((scenario_motor_t*)p)->adapter.rx_errors : 0; }
void scenario_motor_set_reference(void *p, int slot, int valid) {
  if (p && slot >= 0 && slot < 2) ((scenario_motor_t *)p)->adapter.config[slot].reference_valid = valid != 0;
}
void scenario_motor_set_next_command(void *p, uint32_t next) {
  if (p) ((scenario_motor_t *)p)->adapter.next_command_id = next;
}
