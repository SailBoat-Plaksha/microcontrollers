/*
 * servo.c -- Servo control module for Micrcontroller 3
 * ----------------------------------------------------
 * Author: Esraaj Sarkar Gupta 2025
 * 
 * Includes servo motor control for geared motors with
 * magnetic encoders on the armature. A callibration
 * function (meant to be called in regular intervals)
 * utlises an optical (IR) sensor. 
*/

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "include/helpers.h"

/* ---- Motor and Encoder Characteristics ---- */
#define COUNTS_PER_MTR_REV 44
#define COUNTS_PER_REV 460 // Rough experimental value
#define COUNTS_PER_DEGREE (float)COUNTS_PER_REV / 360.0f

/* ---- PWM Limits ---- */

#define PWM_WRAP 255
#define PWM_MAX 255

/* ---- Pins ---- */

#define STANDBY_PIN 21

/* ---- Helper Functions ---- */

int angle_to_counts(int angle) {
    /* Returns the number of integral encoder counts for a given angle. */
    return (int) (COUNTS_PER_DEGREE * angle);   
}

/*
 * Function to replace the if-else statements
 * for the encoder.
 */
int get_direction(int lastA, int lastB, int A, int B) {
    if (A == 1 && B == 1) {
        if (lastA == 1 && lastB == 0) return 1;
        else if (lastA == 0 && lastB == 1) return -1;
        else return 100;
    } 
    else if (A == 0 && B == 0) {
        if (lastA == 0 && lastB == 1) return 1;
        else if (lastA == 1 && lastB == 0) return -1;
        else return 200;
    } 
    else if (A == 1 && B == 0) {
        if (lastA == 0 && lastB == 0) return 1;
        else if (lastA == 1 && lastB == 1) return -1;
        else return 300;
    } 
    else if (A == 0 && B == 1) {
        if (lastA == 1 && lastB == 1) return 1;
        else if (lastA == 0 && lastB == 0) return -1;
        else return 400;
    } 
    else {
        return 2;
    }
}


/* ---- Encoder Helpers ---- */
int encoder_init(mState* ms) {
    /*
     * Initialise all the pins for a given motor.
     *
     * Pins are pulled from motor state structure.
     */
    pinnit(ms->encoder_pin1);
    pinnit(ms->encoder_pin2);

    /* Input Filtering */
    gpio_set_input_hysteresis_enabled(ms->encoder_pin1, true);
    gpio_set_input_hysteresis_enabled(ms->encoder_pin2, true);

    /* Seed variables with dummy values */
    ms->encoder.lastA = gpio_get(ms->encoder_pin1);
    ms->encoder.lastB = gpio_get(ms->encoder_pin2);

    return 0;
}

inline int encoder_poll(mState *ms) {
    /*
     * As of now this function carries out the same functionality
     * as raw_counter().
     * 
     * This function has to be programmed to report directions and
     * multiple count slips (although unlikely I think).
     * 
     * Alternatively we may 
     */
    bool A = gpio_get(ms->encoder_pin1);
    bool B = gpio_get(ms->encoder_pin2);

    int direction;
    if (A != ms->encoder.lastA || B != ms->encoder.lastB) {
        /*
        direction = get_direction(
            ms->encoder.lastA,
            ms->encoder.lastB,
            A,
            B
        );*/

        ms->encoder.lastA = A;
        ms->encoder.lastB = B;


        /*
        if (direction != 1 && direction != -1) {
            // Error handling needed
            return 10000;
        }

        return direction;
        */

        return 1;
    }

    return 0; // No state change

    
}

inline int raw_counter(mState* ms) {
    /*
     * Counts state changes indifferent to the direction of rotation.
     *
     * @returns 0 if no state change
     * @returns 1 if any (even if multiple) stage change(s).
     */
    bool A = gpio_get(ms->encoder_pin1);
    bool B = gpio_get(ms->encoder_pin2);

    int direction;
    if (A != ms->encoder.lastA || B != ms->encoder.lastB) {
        ms->encoder.lastA = A;
        ms->encoder.lastB = B;
        return 1;
    }

    return 0;
}

/* ---- Motor Functions ---- */

#ifndef STANDBY_PIN
#define STANDBY_PIN 21
#endif

void standby(bool enable) {
    /*
     * To enable standby mode, the standby pin
     * must be pulled low. Enable -> 0.
     */
    gpio_put(STANDBY_PIN, enable ? 0 : 1);
}

int startRotate(mState *ms, int dir, int pwm) {
    if (!ms) return 3; // null ptr

    // Ensure PWM pin is in PWM mode
    if (gpio_get_function(ms->pwm_pin) != GPIO_FUNC_PWM) {
        gpio_set_function(ms->pwm_pin, GPIO_FUNC_PWM);
    }

    // Bring driver out of standby
    standby(false);

    // Direction
    if (dir == 1) {
        gpio_put(ms->forward_pin, 1);
        gpio_put(ms->backward_pin, 0);
    } else if (dir == -1) {
        gpio_put(ms->forward_pin, 0);
        gpio_put(ms->backward_pin, 1);
    } else {
        return 2; // bad dir
    }

    uint slice = pwm_gpio_to_slice_num(ms->pwm_pin);
    uint16_t wrap = (uint16_t)pwm_hw->slice[slice].top; // current wrap
    if (pwm < 0) pwm = 0;
    if (pwm > wrap) pwm = wrap;

    pwmWrite(ms->pwm_pin, (uint16_t)pwm);
    return 0;
}


int stopRotate(mState *ms) {
    standby(true);

    gpio_put(ms->forward_pin, 0);
    gpio_put(ms->backward_pin, 0);

    pwmWrite(ms->pwm_pin, 0);

    return 0;
}

/* ---- Servo Functions ---- */
void slowRotate(mState *ms, int dir); // Similar to Servo, uses different pwm

void Servo(mState *ms, int set_angle) {
    // Check for angle values
    if (set_angle > 360 ) {
        set_angle = set_angle % 360;
    } else if (set_angle < 0) {
        set_angle =  - (set_angle % 360);
    }

    int current_angle = ms->angle;
    int rotation_dir;

    int abs_difference = (current_angle > set_angle) ? (current_angle - set_angle) : (set_angle - current_angle);

    if (abs_difference > 360) {
        // Something has fucked up -- handling added later (Error) <-- This should not be possible
    }

    if (abs_difference <= 180) rotation_dir = 1; // Clockwise
    else rotation_dir = -1; // AntiClockwise

    int required_counts = angle_to_counts(rotation_dir * abs_difference);

    startRotate(ms, rotation_dir, ms->default_pwm_255);

    absolute_time_t rotation_start_time = get_absolute_time();
    absolute_time_t rotation_current_time;
    int counts = 0;
    //int required_counts = angle_to_counts(set_angle);

    while(true) {

        rotation_current_time = get_absolute_time();
        if (absolute_time_diff_us(rotation_start_time, rotation_current_time) > 10000000) {
            // Stuck in this fuckass loop for too long
            break;
        }

        // ... count
        counts += raw_counter(ms);

        if (rotation_dir == 1 && counts >= required_counts) break;
        else if (rotation_dir == -1 && counts <= required_counts) break;
        //else break; // wtf (Error)

    }

    stopRotate(ms);
    ms->angle = set_angle;
}

/* ---- Callibration  Functions ---- */
int calibrate(mState* ms) {
    Servo(ms, 0.0); // Bring motor to .0 state first

    while(gpio_get(ms->calibration_pin) != 0){
        slowRotate(ms, 1);
    }
    ms->angle = 0;
    return 0;
}