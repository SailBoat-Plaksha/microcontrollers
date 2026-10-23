/*
 * operating_loops.c
 *
 * This script handles all the different execution states of the machine.
 *  -> Idling Loop
 *  -> RC Control Loop
 *  -> Master serial IO loop
 * 
 *  Authors: Esraaj Sarkar Gupta, Armaan Raisinghani
 */

#include "include/uart.h"
#include "include/helpers.h"
#include "include/servo.h"
#include "include/rc_control.h"

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mavlink_min.h"
#include "actuator_command.h"

#include "include/operating_loops.h"
#ifdef BOAT_MIDSEM_DEMO
#include "../motor-mcu/pico_mailbox.h"
#endif

/* ---- Pins and Flags ----*/

/* -- Hardware Pins -- */
#ifndef RC_FLAG_PIN
#define RC_FLAG_PIN 9
#endif

#define RC_SWITCH_THRESHOLD 1500

extern volatile uint32_t mode_pulse_us;
static mav_parser_t parser;
static actuator_guard_t guard;
static uint8_t sequence;

static void report_setpoint(const uint8_t *command, uint8_t result) {
  uint8_t payload[20] = {0};
  uint8_t actuator = command[12];
  mState *motor = actuator == 1 ? sail_motor : rudder_motor;
  mav_put_u32(payload, to_ms_since_boot(get_absolute_time()));
  memcpy(payload + 4, command, 4);
  mav_put_u16(payload + 8, mav_u16(command + 4));
  mav_put_u16(payload + 10, (uint16_t)(motor->angle * 100));
  payload[16] = actuator;
  payload[17] = mode_pulse_us > RC_SWITCH_THRESHOLD ? 1 : 2;
  payload[18] = result;
  payload[19] = result == 0;
  uint8_t frame[MAV_FRAME_MAX];
  size_t length = mav_pack(MAV_ACTUATOR_STATUS, payload, sizeof(payload), 1, 154, sequence++, frame);
  for (size_t i = 0; i < length; ++i) putchar_raw(frame[i]);
}

#ifndef BOAT_MIDSEM_DEMO
static void service_mavlink(void) {
  uint8_t frame[MAV_FRAME_MAX];
  size_t length = 0;
  int next;
  while ((next = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
    if (!mav_feed(&parser, (uint8_t)next, frame, &length)) continue;
    uint32_t id = (uint32_t)frame[7] | ((uint32_t)frame[8] << 8) | ((uint32_t)frame[9] << 16);
    if (id != MAV_ACTUATOR_SETPOINT) continue;
    uint8_t payload[14] = {0};
    size_t count = frame[1] < sizeof(payload) ? frame[1] : sizeof(payload);
    memcpy(payload, frame + 10, count);
    uint8_t result = actuator_check(&guard, frame + 10, frame[1],
                                    mode_pulse_us > RC_SWITCH_THRESHOLD, 1, 154);
    report_setpoint(payload, result);
    if (result != 0) continue;
    mState *motor = payload[12] == 1 ? sail_motor : rudder_motor;
    standby(false);
    Servo(motor, (int)(mav_u16(payload + 4) / 100));
    standby(true);
  }
}
#else
/* The integrated demo accepts commands only through the I2C mailbox adapter. */
static void service_mavlink(void) { motor_mcu_service(); }
#endif

/* ---- Idling Loop ---- */
void idling_loop(void) {
  /*
  * The default loop every loop falls back to.
  * 
  * A core remains in the idling loop, searching for an
  * indication to jump to a different loop.
  * 
  * Pins :: Hardware flags
  * Flags :: Software flags
  *
  */

  led(1);
  while(true) {
    service_mavlink();
    if (mode_pulse_us > RC_SWITCH_THRESHOLD) {
      rc_loop();
    }

    sleep_ms(10);
  }
   
}

/* ---- Input Output Loops ---- */
void start_serial_IO(void) { service_mavlink(); }

/* ---- RC Control Loops ---- */

void rc_loop(void) {
  standby(false);

  int rc_sail_error_flag = 0;
  int rc_rudder_error_flag = 0;

  while(true) {
    service_mavlink();
    // Check for exit conditions, every 2 seconds
    if (mode_pulse_us <= RC_SWITCH_THRESHOLD) {
      standby(true); // Bring the motors back to low power mode
      return; 
    }

    // Check for exit conditions every 100 runs
    for (int i = 0; i < 100; i++) {
      service_mavlink();

      rc_sail_error_flag = apply_rc_to_motor(sail_motor, sail_pulse_us);
      rc_rudder_error_flag = apply_rc_to_motor(rudder_motor, rudder_pulse_us);

      sleep_ms(20);
    }
  }
}
