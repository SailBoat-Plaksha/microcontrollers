
/*
 * main.c -- The controller for the servo motors.
 *
 * Functionality: <incomplete documentation>
 * 
 */

#include "include/uart.h"
#include "include/helpers.h"
#include "include/servo.h"
#include "include/rc_control.h"
#include "include/operating_loops.h"

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/cjson.h"

#define EXT_LED 22
#define ERR_LED 17

/* ==== Motor Driver Config ==== */
#define STANDBY_PIN 9

/* -- Encoders -- */
#define M1_ENC_1 2
#define M1_ENC_2 3

#define M2_ENC_1 4
#define M2_ENC_2 5

/* -- Pins -- */
#define PWM_A 10
#define INA_2 11
#define INA_1 12

#define PWM_B 15
#define INB_2 14
#define INB_1 13

#define M1_CALIBRATION_PIN -1
#define M2_CALIBRATION_PIN -2

/* ---- Pins and Flags ----*/

/* -- Hardware Pins -- */
#ifndef RC_FLAG_PIN
#define RC_FLAG_PIN 9
#endif

/* ---- Debug Helpers ---- */

// None...


/* Observe serial communication on USB */
static inline void usb_put_byte(uint8_t b) {
  putchar_raw((int)b); // raw, no line discipline
}
/* ---- Servo JSON Wrapper ---- */
static void servo_json_handler(mState *motor, const char *json_str) {
  cJSON *json = cJSON_Parse(json_str);
  if (!json) {
    printf("Error: Invalid JSON\r\n");
    fflush(stdout);
    return;
  }

  cJSON *angle_item = cJSON_GetObjectItem(json, "angle");
  if (angle_item && angle_item->type == cJSON_Number) {
    int angle = (int)angle_item->valuedouble;
    if (angle >= motor->min_angle && angle <= motor->max_angle) {
      Servo(motor, angle);
      printf("OK: Servo moved to %d degrees\r\n", angle);
      fflush(stdout);
    } else {
      printf("Error: Angle out of range\r\n");
      fflush(stdout);
    }
  } else {
    printf("Error: 'angle' field required\r\n");
    fflush(stdout);
  }

  cJSON_Delete(json);
}
/* ---- Global Pointers ---- */
static mState motor1_state;
mState *sail_motor = &motor1_state;

static mState motor2_state;
mState *rudder_motor = &motor2_state;

/* ===== Setup Functions ====== */

int setup(void) {
  stdio_init_all();

  poutit(EXT_LED);
  poutit(ERR_LED);
  poutit(DEBUG_LED);

  //pinnit(RC_FLAG_PIN); <-- This is done in rc_setup()

  // Test Indicators
  gpio_put(EXT_LED, 1);
  gpio_put(ERR_LED, 1);
  gpio_put(DEBUG_LED, 1);

  return 0;
}

int motorSetup(void) {
  poutit(STANDBY_PIN);
  standby(true); // Hold driver in standby

  /* ---- Define Motor 1 ---- */
  motor1_state = (mState){
    .index = 1,

    .angle = 0.0f,
    .min_angle = 0.0f,
    .max_angle = 180.0f,

    .default_pwm_255 = 128,

    .encoder = {
      .A = 0,
      .B = 0,
      .lastA = 0,
      .lastB = 0,

      .encoder_count = 0
      },

    .calibration_pin = M1_CALIBRATION_PIN,
    .encoder_pin1 = M1_ENC_1,
    .encoder_pin2 = M1_ENC_2,
    .forward_pin = INA_1,
    .backward_pin = INA_2,
    .pwm_pin = PWM_A
    };

  motor2_state = (mState) {
    .index = 2,

    .angle = 0.0f,
    .min_angle = 0.0f,
    .max_angle = 180.0f,

    .default_pwm_255 = 128,

    .encoder = {
      .A = 0,
      .B = 0,
      .lastA = 0,
      .lastB =0
    },

    .calibration_pin = M2_CALIBRATION_PIN,
    .encoder_pin1 = M2_ENC_1,
    .encoder_pin2 = M2_ENC_2,
    .forward_pin = INB_1,
    .backward_pin = INB_2,
    .pwm_pin = PWM_B
  };

  /* ---- Init Motor 1 (Sail) ---- */
  motor_init(sail_motor);

  pwmSetup(sail_motor->pwm_pin, 255, 4.0f);
  pwmWrite(sail_motor->pwm_pin, 0);

  gpio_put(sail_motor->forward_pin, 0);
  gpio_put(sail_motor->backward_pin, 0);

  encoder_init(sail_motor);

  /* --- Init Motor 2 (Rudder) ---- */
  motor_init(rudder_motor);

  pwmWrite(rudder_motor->pwm_pin, 0);
  pwmSetup(rudder_motor->pwm_pin, 255, 4.0f);

  gpio_put(rudder_motor->forward_pin, 0);
  gpio_put(rudder_motor->backward_pin, 0);

  encoder_init(rudder_motor);

  return 0;
}

int startup_tests(void) {
  for (int i = 0; i < 3; i++) {
    /* Test LEDS */
    led(1);
    err_led(1);
    debug_led(1);

    /* Test Motors */
    startRotate(sail_motor, 1, 255);
    startRotate(rudder_motor, 1, 255);

    sleep_ms(500);
    // TODO: Using encoders check if the motors are spinning

    stopRotate_noStandby(sail_motor);
    stopRotate(rudder_motor);
    led(0);
    err_led(0);
    debug_led(0);

    sleep_ms(500);
  }

  /* Communication Tests */

  return 0;
}

int main(void) {
  /* ---- Initialization Functions ---- */
  setup(); // Expect EXT LED ON
  motorSetup();
  setup_rc();

  /* ---- Indicators ---- */
  sleep_ms(1000);

  // Setup Complete 
  led(0);
  debug_led(0);
  err_led(0);

  sleep_ms(1000);

  // Ensure systems are running
  startup_tests();

  // Start looking for things to do
  idling_loop();
}