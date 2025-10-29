/*
 * helpers.c -- Helper functions for Servo Driver
 * -----------------------------------------------
 * Author: Esraaj Sarkar Gupta
 * 
 */

#include "pico/stdlib.h"
#include "include/helpers.h"

/* ---- Pin Helpers ---- */

void pinnit(int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
}

void poutit(int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}

/* ---- Indicative Helpers ----*/

int led(int high_low) {
    if (EXT_LED == NULL) {return 1;}
    if (high_low > 1 || high_low < 0) {return 1;}
    gpio_put(EXT_LED, high_low);
    return 0;
}

/* ---- PWM Helpers ---- */

void pwmSetup(uint pin, uint16_t wrap_value, float clkdiv) {
    /*
     * `pin` -- PWM_A/B pin number
     * `wrap_value` -- Recommended wrap value is 255 for 8 bit resolution
     * `clkdiv` -- Recommended 4.0 -> 12 KHz PWM frequency (quiet and motor safe)
     */
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);

    pwm_set_wrap(slice, wrap_value);
    pwm_set_clkdiv(slice, clkdiv);
    pwm_set_enabled(slice, true);
} // -- GPT5

void pwmWrite(uint pin, uint16_t level) {
    uint slice = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);
    pwm_set_chan_level(slice, channel, level);
} // -- GPT5

void pwmWritePercent(uint pin, float duty) {
    /*
     * `pin` -- PWM_A/B pin number
     * `duty` -- Belongs to the interval [0, 1] (Unitary)
     */
    if (duty < 0) duty = 0;
    if (duty > 1) duty = 1;
    uint slice = pwm_gpio_to_slice_num(pin);
    uint wrap  = pwm_hw->slice[slice].top;
    pwmWrite(pin, (uint16_t)(wrap * duty));
} // -- GPT5
