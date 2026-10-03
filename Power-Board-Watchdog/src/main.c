/*
 * main.c
 * 
 * Checks if power from 3.3V (PB3) and 5V (PB4)
 * channels are high -- if they are active.
 * 
 * Debug feature: Holds PB0 HIGH if both PB1 and PB2 are LOW.
 */

#define F_CPU 1200000UL 

#include <avr/io.h>
#include <util/delay.h>

void setup(void) {
    /* ---- Pin Direction Configuration ---- */

    // PB0 (Debug LED), PB1 (5V Indicator), PB2 (3.3V Indicator) -> Outputs
    DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2);

    // PB3 (3.3V Sense), PB4 (5V Sense) -> Inputs
    DDRB &= ~((1 << DDB3) | (1 << DDB4));
}

int main(void) {
    setup();

    // Main polling loop
    while(1) {
        /* Monitor 3.3V Rail (PB3) -> Drive PB2 */
        if (PINB & (1 << PINB3)) {
            PORTB |= (1 << PORTB2);   // Set PB2 HIGH
        } else {
            PORTB &= ~(1 << PORTB2);  // Set PB2 LOW
        }

        /* Monitor 5V Rail (PB4) -> Drive PB1 */
        if (PINB & (1 << PINB4)) {
            PORTB |= (1 << PORTB1);   // Set PB1 HIGH
        } else {
            PORTB &= ~(1 << PORTB1);  // Set PB1 LOW
        }

        /* ---- Debug Logic: PB0 HIGH if both PB1 and PB2 are LOW ---- */
        // Check if both PB1 and PB2 bits in PORTB are 0
        if (!(PORTB & ((1 << PORTB1) | (1 << PORTB2)))) {
            PORTB |= (1 << PORTB0);   // Both outputs inactive -> Drive PB0 HIGH
        } else {
            PORTB &= ~(1 << PORTB0);  // At least one active -> Drive PB0 LOW
        }
    }

    return 0;
}