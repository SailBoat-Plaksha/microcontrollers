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

#include "include/cjson.h"

#include "include/operating_loops.h"

/* ---- Pins and Flags ----*/

/* -- Hardware Pins -- */
#ifndef RC_FLAG_PIN
#define RC_FLAG_PIN 9
#endif

#define RC_SWITCH_THRESHOLD 1500

extern volatile uint32_t mode_pulse_us;

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
    if (mode_pulse_us > RC_SWITCH_THRESHOLD) {
      rc_loop();
    }

    sleep_ms(10);
  }
   
}

/* ---- Input Output Loops ---- */
// TODO: Pretty much all of it
void start_serial_IO(void);

/* ---- RC Control Loops ---- */

void rc_loop(void) {
  standby(false);

  int rc_sail_error_flag = 0;
  int rc_rudder_error_flag = 0;

  while(true) {
    // Check for exit conditions, every 2 seconds
    if (mode_pulse_us <= RC_SWITCH_THRESHOLD) {
      standby(true); // Bring the motors back to low power mode
      return; 
    }

    // Check for exit conditions every 100 runs
    for (int i = 0; i < 100; i++) {

      rc_sail_error_flag = apply_rc_to_motor(sail_motor, sail_pulse_us);
      rc_rudder_error_flag = apply_rc_to_motor(rudder_motor, rudder_pulse_us);

      sleep_ms(20);
    }
  }
}