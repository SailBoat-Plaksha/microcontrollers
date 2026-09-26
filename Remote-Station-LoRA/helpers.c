/*
 * helpers.c : Board Helpers
 * -------------------------
 * Common GPIO helpers used by the remote station.
 */

#include "pico/stdlib.h"

#include "helpers.h"

/* ==== GPIO Helpers ==== */

void pinnit(int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
}

void poutit(int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}

void led(int high_low) {
    gpio_put(EXT_LED, high_low ? 1 : 0);
}

void err_led(int high_low) {
    gpio_put(ERR_LED, high_low ? 1 : 0);
}
