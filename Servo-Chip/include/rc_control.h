/*
 * rc_control.h
 * Header file for the remote control module.
 * -------------------------------------------
 * Author: Esraaj Sarkar Gupta
 */

#ifndef RC_CONTROL_H
#define RC_CONTROL_H

#include <stdint.h>
#include "servo.h" 

/* ---- Global RC Data ---- */
// Allow external scripts (main.c) to read these variables -- we might use them.
extern volatile uint32_t rudder_pulse_us;
extern volatile uint32_t sail_pulse_us;

/* ---- Function Prototypes ---- */

/**
 * @brief Initializes the GPIO pins and hardware interrupts for the RC receiver.
 * @returns 0 on success.
 */
int setup_rc(void);

/**
 * @brief Maps an RC pulse width to a motor speed and direction.
 * * @param motor Pointer to the motor state structure.
 * @param pulse The incoming pulse width in microseconds (usually 1000 to 2000).
 * * @returns 0 : Success / Motor updated or stopped safely
 * @returns 1 : Error (Null motor pointer)
 * @returns 2 : Error (Pulse out of bounds)
 */
int apply_rc_to_motor(mState *motor, uint32_t pulse);

#endif // RC_CONTROL_H