/*
 * helpers.h -- Header for helpers.c
 * -----------------------------------------------
 * Author: Esraaj Sarkar Gupta
 * 
 */

#ifndef EXT_LED
#define EXT_LED 22
#endif

/* ---- Pin Helpers ---- */

void pinnit(int pin);
void poutit(int pin);

/* ---- Indicative Helpers */

int led(int high_low);