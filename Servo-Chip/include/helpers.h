/*
 * helpers.h -- Header for helpers.c
 * -----------------------------------------------
 * Author: Esraaj Sarkar Gupta
 * 
 */

#ifndef EXT_LED
#define EXT_LED 22
#endif

#include <stdlib.h>

/* ---- Pin Helpers ---- */

void pinnit(int pin);
void poutit(int pin);

/* ---- Indicative Helpers */

int led(int high_low);

/* ---- PWM Helpers ---- */

#include "hardware/pwm.h"

void pwmSetup(uint pin, uint16_t wrap_value, float clkdiv);
void pwmWrite(uint pin, uint16_t level);
void pwmWritePercent(uint pin, float duty);
