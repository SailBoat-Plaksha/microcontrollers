#ifndef SERVO_H
#define SERVO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "pico/types.h"   // for 'uint'

/* ===== Tunables / Defaults (override before include if you like) ===== */
#ifndef COUNTS_PER_MTR_REV
#define COUNTS_PER_MTR_REV 44
#endif

#ifndef COUNTS_PER_REV
#define COUNTS_PER_REV 830
#endif

#ifndef COUNTS_PER_DEGREE
#define COUNTS_PER_DEGREE (float)COUNTS_PER_REV / 360.0f
#endif

#ifndef PWM_WRAP
#define PWM_WRAP 255
#endif

#ifndef PWM_MAX
#define PWM_MAX 255
#endif

#ifndef STANDBY_PIN
#define STANDBY_PIN 21
#endif

/* ============================================================
 *                       DATA STRUCTURES
 * ============================================================ */

typedef struct {
    /*
     * Structure to store the state of the motor encoder.
     *
     * Stores the current and previous states of the motor
     * encoder.
     */

    int  A;
    int  B;

    int  lastA;
    int  lastB;

    long encoder_count;
} eState;

typedef struct {
     /*
     * Structure to store the state of the motor.
     *
     * Characteristics stored: angle, minimum_angle,
     * maximum_angle, default pwm values.
     * 
     * Stores encoder states.
     * 
     * Stores the pin numbers corresponding to the motor.
     */
    int   index;

    /* Characteristics */
    float angle;
    float min_angle;
    float max_angle;
    int   default_pwm_255;

    eState encoder;

    /* Pins */
    uint calibration_pin;
    uint encoder_pin1;
    uint encoder_pin2;
    uint forward_pin;
    uint backward_pin;
    uint pwm_pin;
} mState;

/* ============================================================
 *                       FUNCTION PROTOTYPES
 * ============================================================ */

/* Angle/count helpers (match your signatures) */
int angle_to_counts(int angle);
int get_direction(int lastA, int lastB, int A, int B);

/* Encoder init/poll */
int encoder_init(mState* ms);
int encoder_poll(mState* ms);

/* Motor control (standby is static inline in your .c; not declared here) */
void standby(bool enable);

int startRotate(mState *ms, int dir, int pwm);
int stopRotate(mState *ms);
int stopRotate_noStandby(mState *ms);
/* Servo motion */
void slowRotate(mState *ms, int dir);   /* declared in .c, implemented later */
void Servo(mState *ms, int set_angle);

/* Calibration */
int calibrate(mState* ms);

/* ===== OPTIONAL: If you want standby visible everywhere, uncomment below =====
#include "pico/stdlib.h"
static inline void standby(bool enable) {
    // Your inverted logic: true => enter standby (LOW), false => run (HIGH)
    gpio_put(STANDBY_PIN, enable ? 0 : 1);
}
*/

#ifdef __cplusplus
}
#endif

#endif /* SERVO_H */
