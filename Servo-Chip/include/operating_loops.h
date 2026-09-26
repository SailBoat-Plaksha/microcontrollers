#ifndef OPERATING_LOOPS_H
#define OPERATING_LOOPS_H

#include "servo.h"

/* * -------------------------------------------------------------------
 * Global Motor Pointers 
 * -------------------------------------------------------------------
 * These are defined in main.c and used in operating_loops.c
 */
extern mState *sail_motor;
extern mState *rudder_motor;

/* * -------------------------------------------------------------------
 * Function Prototypes
 * -------------------------------------------------------------------
 */

/**
 * @brief The default loop that checks for hardware/software flags
 * to switch between operating modes.
 */
void idling_loop(void);

/**
 * @brief Enters a dedicated loop for Radio Control. 
 * Stays in this loop until the RC_FLAG_PIN is pulled low.
 */
void rc_loop(void);

/**
 * @brief Services the USB serial MAVLink interface once.
 */
void start_serial_IO(void);

#endif // OPERATING_LOOPS_H
