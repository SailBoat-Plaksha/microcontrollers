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
#include "pico/time.h"

#include "include/helpers.h"


/* ---- Motor and Encoder Characteristics ---- */
#define GEAR_RATIO 4.0f // `n` for a ratio of 1:n
#define COUNTS_PER_MTR_REV 11
#define COUNTS_PER_REV 44
#define COUNTS_PER_DEGREE (float)COUNTS_PER_REV / 360.0f

/* ---- PWM Limits ---- */

#define PWM_WRAP 255
#define PWM_MAX 255


// ---- control params (tune if needed) ----
    const float   KP       = 4.0f;   // proportional gain (duty per deg)
    const float   KD       = 0.2f;   // derivative gain (deg/tick)
    const float   DEADBAND = 0.5f;   // stop within ±0.5°
    const uint8_t DUTY_MIN = 20;     // overcome stiction (~8%)
    const uint8_t DUTY_MAX = 255;    // full scale
    const uint32_t DT_MS   = 5;      // loop period (~200 Hz)
    const uint32_t TIMEOUT_MS = 3000;

static const int8_t DELTA[4][4] = {
/* prev\curr: 00  01  10  11 */
              { 0, +1, -1,  0 },  // prev 00
              {-1,  0,  0, +1 },  // prev 01
              {+1,  0,  0, -1 },  // prev 10
              { 0, -1, +1,  0 }   // prev 11
};

/* Status Integer */
int status = 0;
/* Status Key:
 * ...
 * ...
 */

int get_direction(bool A_prev, bool B_prev, bool A_curr, bool B_curr) {
    int prev = (A_prev << 1) | B_prev;
    int curr = (A_curr << 1) | B_curr;

    return DELTA[prev][curr];
}

struct encoderState {
    int A; // Unused
    int B; // Unused

    int lastA;
    int lastB;

    long encoder_count;
} eState;


struct motorState {
    int index;

    /* Characteristics */
    float angle;

    float min_angle;
    float max_angle;

    int default_pwm_255; // Default PWM value on PWM wrap 255

    eState encoder;

    /* Pins */
    uint calibration_pin;
    uint encoder_pin1;
    uint encoder_pin2;

    uint forward_pin;
    uint backward_pin;

    uint pwm_pin;

} mState;

/* ---- Helper Functions ---- */

int angle_to_counts(int angle) {
    return (int) (COUNTS_PER_DEGREE * angle);   
}

/* ---- Encoder Helpers ---- */
int encoder_init(mState* ms) {
    pinnit(ms->encoder_pin1);
    pinnit(ms->encoder_pin2);

    /* Input Filtering */
    gpio_set_input_hysteresis_enabled(ms->encoder_pin1);
    gpio_set_input_hysteresis_enabled(ms->encoder_pin2);

    /* Seed variables with dummy values */

}

inline int encoder_poll(mState *ms) {
    bool A = gpio_get(ms->encoder_pin1);
    bool B = gpio_get(ms->encoder_pin2);

    if (A != ms->encoder.lastA || B != ms->encoder.lastB) {
        int direction = get_direction(
            ms->encoder.lastA,
            ms->encoder.lastB,
            A,
            B
            );
    }

    return direction; // +1 or -1
}

/* ---- Motor Functions ---- */

void standby(bool flag){
    if (flag) gpio_put(STANDBY_PIN, 1);
    else gpio_put(STANDBY_PIN, 0);
}

int startRotate(mState *ms, int dir, int pwm) {
    standby(false);
    if (dir == 1) {
        gpio_put(ms->forward_pin, 1);
        gpio_put(ms->backward_pin, 0);
    } else if (dir == -1) {
        gpio_put(ms->forward_pin, 0);
        gpio_put(ms->backward_pin, 1);
    } else {
        return 2; /* ERROR: Incorrect direction argument */
    }

    if (pwm > PWM_MAX) pwm = PWM_MAX;
    if (pwm > PWM_WRAP) pwm = PWM_WRAP;

    pwmWrite(ms->pwm_pin, pwm);
}

int stopRotate(mState *ms) {
    standby(true);

    gpio_put(ms->forward_pin, 0);
    gpio_put(ms->backward_pin, 0);

    pwmWrite(ms->pwm_pin, 0);
}

/* ---- Servo Functions ---- */
void slowRotate(mState *ms, int dir);

void Servo(mState *ms, int set_angle) {
    if (set_angle > 360) {
        set_angle = set_angle % 360;
    }
    int current_angle = ms->angle;
    int rotation_dir;

    int abs_difference = (current_angle > set_angle) ? (current_angle - set_angle) : (set_angle - current_angle);

    if (abs_difference > 360) {
        // Something has fucked up -- handling added later (Error)
    }

    if (abs_difference <= 180) rotation_dir = 1; // Clockwise
    else rotation_dir = -1; // AntiClockwise

    int required_counts = rotation_dir * abs_difference;

    startRotate(ms, 00);

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
        counts = encoder_poll(ms);

        if (rotation_dir == 1 || counts >= required_counts) break;
        else if (rotation_dir == -1 || counts <= required_counts) break;
        else break; // wtf (Error)

    }

    stopRotate(ms);




}

int calibrate(mState* ms) {
    Servo(ms, 0.0); // Bring motor to .0 state first

    while(ms->calibration_pin != 0){
        slowRotate(ms, 1);
    }
    ms->angle = 0;
    return 0;
}