/*
 * rc_contro.c
 * This is the remote control module. Remote control takes presidence
 * in the hierarchy of communication protocols. The controller
 * must ALWAYS listen to the remote control should it be broadcasting.   
 *
 * -------------------------------------------------------------------
 * Author: Esraaj Sarkar Gupta
 */

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "include/helpers.h"
#include "include/servo.h"

#include "include/rc_control.h"

#define RUDDER_PIN  14
#define SAIL_PIN    15

#define MOTOR_PIN   13   // Currently unused

#define DEADBAND 25

/* -- Store the time variables -- */
volatile uint32_t rudder_pulse_us = 1500;
volatile uint32_t sail_pulse_us = 1500;

// Pulse start times
static absolute_time_t rudder_rise_time;
static absolute_time_t sail_rise_time;

/* ---- Interrupt Hanlder ---- */
void rc_irq_handler(uint gpio, uint32_t events) {
    /*
     * In this architecture, GPIO_IRQ_EDGE_RISE and GPIO_IRQ_EDGE_FALL (the events)
     * are defined as bits in various positions of a 32 bit register. We do not directly
     * equate events (and use the bitwise AND instead) since if both events take place
     * (although unlikely this is possible with fast hardware)  the direct == returns false.
     * 
     * For example, if
     * 
     * RISE: 00000100
     * FALL: 00001000
     * 
     * Both: 00001100
     * RISE (event) & BOTH = 0000100 == true
     * 
     */
    absolute_time_t current_time = get_absolute_time();

    if (gpio == RUDDER_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            rudder_rise_time = current_time;
        } else if (events & GPIO_IRQ_EDGE_FALL) {
            rudder_pulse_us = 
                (uint32_t)absolute_time_diff_us(rudder_rise_time, current_time);
        }
    }

    else if (gpio == SAIL_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            sail_rise_time = current_time;
        } else if (events & GPIO_IRQ_EDGE_FALL) {
            sail_pulse_us =
                (uint32_t)absolute_time_diff_us(sail_rise_time, current_time);
        }
    }
}

/* ---- Initialization Function ---- */
int setup_rc(void){
    pinnit(RUDDER_PIN);
    pinnit(SAIL_PIN);
    
    gpio_set_irq_enabled_with_callback(
        RUDDER_PIN,                                 // Pin
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,    // Even mask
        true,                                       // Turn the interrupt on
        &rc_irq_handler                             // Callback function
    );

    gpio_set_irq_enabled_with_callback(
        SAIL_PIN,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &rc_irq_handler
    );

    return 0;
}

/* ---- Control Mapping ---- */
/*
 *
 * Error Codes:
 * 0 : All good
 * 1 : No motor found error
 * 2 : Pulse out of bounds error
 * 
 */
int apply_rc_to_motor(mState *motor, uint32_t pulse) {
    /* -- Safety -- */
    if (!motor) return 1;

    if ( (pulse < 800) || (pulse > 2200) ) return 2;

    /* -- Motor Mathematics -- */
    int deviation = (int)pulse - 1500;

    // Stopping
    if (abs(deviation) < DEADBAND) {

        led(0);
        debug_led(1);

        stopRotate_noStandby(motor);
        return 0;
    }

    // Compute direction
    int dir = (deviation > 0) ? 1 : -1;

    // Map speed propotionate to deviation
    int speed = (abs(deviation) - DEADBAND) * 255 / (500 - DEADBAND);

    if (speed > 255) speed = 255;
    if (speed < 0) speed = 0;

    debug_led(0);
    led(1);

    startRotate(motor, dir, speed);

    return 0;
}