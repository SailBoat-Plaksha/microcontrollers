#include "pico_mailbox.h"

#include "adapter.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/sync.h"
#include "pico/i2c_slave.h"
#include "pico/rand.h"
#include "pico/stdlib.h"

#include <string.h>

#ifndef MOTOR_I2C_SDA_PIN
#define MOTOR_I2C_SDA_PIN 0
#endif
#ifndef MOTOR_I2C_SCL_PIN
#define MOTOR_I2C_SCL_PIN 1
#endif
#ifndef MOTOR_STANDBY_PIN
#define MOTOR_STANDBY_PIN 9
#endif
#ifndef MOTOR_M1_REFERENCE_VALID
#define MOTOR_M1_REFERENCE_VALID 0
#endif
#ifndef MOTOR_M2_REFERENCE_VALID
#define MOTOR_M2_REFERENCE_VALID 0
#endif
#ifndef MOTOR_M1_ZERO_COUNT
#define MOTOR_M1_ZERO_COUNT 0
#endif
#ifndef MOTOR_M2_ZERO_COUNT
#define MOTOR_M2_ZERO_COUNT 0
#endif
#ifndef MOTOR_M1_COUNTS_PER_REV
#define MOTOR_M1_COUNTS_PER_REV 830
#endif
#ifndef MOTOR_M2_COUNTS_PER_REV
#define MOTOR_M2_COUNTS_PER_REV 830
#endif
#ifndef MOTOR_M1_MAX_TRAVEL_CDEG
#define MOTOR_M1_MAX_TRAVEL_CDEG 18000
#endif
#ifndef MOTOR_M2_MAX_TRAVEL_CDEG
#define MOTOR_M2_MAX_TRAVEL_CDEG 18000
#endif
#ifndef MOTOR_M1_MAX_RATE_CDEG_S
#define MOTOR_M1_MAX_RATE_CDEG_S 3000
#endif
#ifndef MOTOR_M2_MAX_RATE_CDEG_S
#define MOTOR_M2_MAX_RATE_CDEG_S 3000
#endif
#ifndef MOTOR_FEEDBACK_FRESH_MS
#define MOTOR_FEEDBACK_FRESH_MS 100
#endif

typedef struct {
  mState *motors[2];
  int32_t counts[2];
  uint32_t feedback_ms[2];
  bool have_feedback[2];
} pico_motor_io_t;

static pico_motor_io_t motor_io;
static motor_adapter_t adapter;
static volatile uint8_t rx_stage[MOTOR_MAILBOX_BYTES];
static volatile uint8_t rx_pending[MOTOR_MAILBOX_BYTES];
static volatile uint8_t rx_length;
static volatile bool rx_pending_valid;
static volatile bool rx_overflow;
static volatile bool rx_error_pending;
static uint8_t tx_frame[MOTOR_MAILBOX_BYTES];
static uint8_t tx_position;
static volatile bool tx_loaded;

static uint32_t pico_now(void *context) { (void)context; return to_ms_since_boot(get_absolute_time()); }

static int32_t pico_read_count(void *context, uint8_t actuator) {
  pico_motor_io_t *io = context;
  int index = actuator - 1u;
  if (index > 1 || !io->motors[index]) return 0;
  int delta = encoder_poll(io->motors[index]);
  io->counts[index] += delta;
  io->feedback_ms[index] = pico_now(NULL);
  io->have_feedback[index] = true;
  return io->counts[index];
}

static bool pico_feedback_fresh(void *context, uint8_t actuator, uint32_t now) {
  pico_motor_io_t *io = context;
  int index = actuator - 1u;
  return index < 2 && io->have_feedback[index] &&
         (uint32_t)(now - io->feedback_ms[index]) <= MOTOR_FEEDBACK_FRESH_MS;
}

static void pico_drive(void *context, uint8_t actuator, int direction, uint8_t pwm) {
  pico_motor_io_t *io = context;
  int index = actuator - 1u;
  if (index > 1 || !io->motors[index]) return;
  mState *m = io->motors[index];
  gpio_put(MOTOR_STANDBY_PIN, 1);
  gpio_put(m->forward_pin, direction > 0);
  gpio_put(m->backward_pin, direction < 0);
  pwmWrite(m->pwm_pin, pwm);
}

static void pico_stop(void *context, uint8_t actuator) {
  pico_motor_io_t *io = context;
  int index = actuator - 1u;
  if (index > 1 || !io->motors[index]) return;
  mState *m = io->motors[index];
  pwmWrite(m->pwm_pin, 0);
  gpio_put(m->forward_pin, 0);
  gpio_put(m->backward_pin, 0);
  gpio_put(MOTOR_STANDBY_PIN, 0);
}

static bool pico_rc_owns(void *context) {
  (void)context;
  extern volatile uint32_t mode_pulse_us;
  return mode_pulse_us > 1500u;
}

static void i2c_event(i2c_inst_t *i2c, i2c_slave_event_t event) {
  if (event == I2C_SLAVE_RECEIVE) {
    while (i2c_get_read_available(i2c)) {
      uint8_t byte = i2c_read_byte_raw(i2c);
      if (rx_length < MOTOR_MAILBOX_BYTES) rx_stage[rx_length++] = byte;
      else rx_overflow = true;
    }
  } else if (event == I2C_SLAVE_REQUEST) {
    while (tx_loaded && i2c_get_write_available(i2c) && tx_position < MOTOR_MAILBOX_BYTES) {
      i2c_write_byte_raw(i2c, tx_frame[tx_position++]);
    }
  } else if (event == I2C_SLAVE_FINISH) {
    if (rx_length || rx_overflow) {
      if (!rx_pending_valid && rx_length == MOTOR_MAILBOX_BYTES && !rx_overflow) {
        for (uint8_t i = 0; i < MOTOR_MAILBOX_BYTES; ++i) rx_pending[i] = rx_stage[i];
        rx_pending_valid = true;
      } else rx_error_pending = true;
      rx_length = 0;
      rx_overflow = false;
    }
    tx_position = 0;
    tx_loaded = false;
  }
}

void motor_mcu_start(mState *sail, mState *rudder) {
  motor_io = (pico_motor_io_t){.motors = {sail, rudder}};
  /* Keep motor outputs disabled; no startup test runs in the demo build. */
  gpio_init(MOTOR_STANDBY_PIN);
  gpio_set_dir(MOTOR_STANDBY_PIN, GPIO_OUT);
  gpio_put(MOTOR_STANDBY_PIN, 0);
  for (int i = 0; i < 2; ++i) {
    mState *m = motor_io.motors[i];
    gpio_put(m->forward_pin, 0);
    gpio_put(m->backward_pin, 0);
    pwmWrite(m->pwm_pin, 0);
  }

  motor_actuator_config_t config[2] = {
    {.actuator = 1, .zero_count = MOTOR_M1_ZERO_COUNT, .counts_per_revolution = MOTOR_M1_COUNTS_PER_REV,
     .max_rate_cdeg_s = MOTOR_M1_MAX_RATE_CDEG_S, .max_travel_cdeg = MOTOR_M1_MAX_TRAVEL_CDEG,
     .reference_valid = MOTOR_M1_REFERENCE_VALID != 0},
    {.actuator = 2, .zero_count = MOTOR_M2_ZERO_COUNT, .counts_per_revolution = MOTOR_M2_COUNTS_PER_REV,
     .max_rate_cdeg_s = MOTOR_M2_MAX_RATE_CDEG_S, .max_travel_cdeg = MOTOR_M2_MAX_TRAVEL_CDEG,
     .reference_valid = MOTOR_M2_REFERENCE_VALID != 0}
  };
  motor_adapter_io_t io = {&motor_io, pico_now, pico_read_count, pico_feedback_fresh,
                           pico_drive, pico_stop, pico_rc_owns};
  motor_adapter_init(&adapter, &io, config, get_rand_32());
  motor_adapter_mailbox_read(&adapter, tx_frame);
  tx_position = 0;
  tx_loaded = true;

  gpio_set_function(MOTOR_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(MOTOR_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(MOTOR_I2C_SDA_PIN);
  gpio_pull_up(MOTOR_I2C_SCL_PIN);
  i2c_slave_init(i2c0, MOTOR_MAILBOX_ADDR, i2c_event);
}

void motor_mcu_service(void) {
  uint8_t incoming[MOTOR_MAILBOX_BYTES];
  bool ready = false;
  bool rx_error = false;
  uint32_t irq_state = save_and_disable_interrupts();
  if (rx_pending_valid) {
    for (uint8_t i = 0; i < MOTOR_MAILBOX_BYTES; ++i) incoming[i] = rx_pending[i];
    rx_pending_valid = false;
    ready = true;
  }
  if (rx_error_pending) {
    rx_error_pending = false;
    rx_error = true;
  }
  restore_interrupts(irq_state);
  if (rx_error) adapter.rx_errors++;
  if (ready) motor_adapter_receive(&adapter, incoming);
  motor_adapter_poll(&adapter);
  irq_state = save_and_disable_interrupts();
  if (!tx_loaded) {
    motor_adapter_mailbox_read(&adapter, tx_frame);
    tx_position = 0;
    tx_loaded = true;
  }
  restore_interrupts(irq_state);
}
