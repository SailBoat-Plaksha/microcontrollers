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
