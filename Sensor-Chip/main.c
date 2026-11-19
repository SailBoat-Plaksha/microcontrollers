/*
 * main.c -- Read NMEA output from the PA1616D GPS and forward it to USB.
 */

#include "gps.h"

#include "pico/stdlib.h"

#include <stdio.h>

int main(void) {
    stdio_init_all();
    gps_init();

    char sentence[GPS_NMEA_SENTENCE_MAX];

    while (true) {
        if (gps_read_sentence(sentence, sizeof(sentence))) {
            printf("%s\r\n", sentence);
            fflush(stdout);
        } else {
            tight_loop_contents();
        }
    }

    return 0;
}
