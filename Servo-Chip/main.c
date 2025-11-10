// main.c

#include "include/uart.h"
#include "include/helpers.h"
#include "include/servo.h"

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/uart.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define EXT_LED 22

/* ==== UART Config ==== */
#define UART_ID   uart0
#define BAUD      115200
#define TX_PIN    0
#define RX_PIN    1
#define EXTLED    22

/* ==== Motor Driver Config ==== */
#define STANDBY_PIN 21

#define M1_ENC_1 2
#define M1_ENC_2 3

#define PWM_A 20
#define INA_2 19
#define INA_1 18

#define M1_CALIBRATION_PIN -1

/* ---- Debug Helpers ---- */


/* Observe serial communication on USB */
static inline void usb_put_byte(uint8_t b) {
    putchar_raw((int)b);  // raw, no line discipline
}

/* ---- Global Pointers ---- */
static mState motor1_state;
mState *m1 = &motor1_state;

/* ===== Setup Functions ====== */

int setup(void) {
    stdio_init_all();
    poutit(EXT_LED);

    gpio_put(EXT_LED, 1);
    return 0;
}

int UART_setup(void) {
    uart_driver_init(UART_ID, BAUD, TX_PIN, RX_PIN);
    sleep_ms(500);
    uart_send_str(UART_ID, "[UART] Boot: USB CDC + UART1 link is up\r\n");

    return 0;
}

int motorSetup(void) {
    poutit(STANDBY_PIN);
    standby(true); // Hold driver in standby

    /* ---- Define Motor 1 ---- */
    motor1_state = (mState){
        .index      = 1,

        .angle      = 0.0f,
        .min_angle  = 0.0f,
        .max_angle  = 180.0f,

        .default_pwm_255 = 128,

        .encoder = {
            .A      = 0,
            .B      = 0,
            .lastA  = 0,
            .lastB  = 0,

            .encoder_count = 0 
        },

        .calibration_pin    = M1_CALIBRATION_PIN,
        .encoder_pin1       = M1_ENC_1,
        .encoder_pin2       = M1_ENC_2,
        .forward_pin        = INA_1,
        .backward_pin       = INA_2,
        .pwm_pin            = PWM_A
    };

    /* ---- Init Motor 1 ---- */
    motor_init(m1);

    pwmSetup(m1->pwm_pin, 255, 4.0f);

    pwmWrite(m1->pwm_pin, 0);
    gpio_put(m1->forward_pin, 0);
    gpio_put(m1->backward_pin, 0);

    encoder_init(m1);

    return 0;

}

int main(void) {
    setup();
    UART_setup();
    motorSetup();

    sleep_ms(1000);

    led(0); // Setup complete

    sleep_ms(1000);

    Servo(m1, 90);

    led(1);
    

    while(true) {
        tight_loop_contents();
    }



    return 0;
}