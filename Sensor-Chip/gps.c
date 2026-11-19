/*
 * gps.c -- UART receive handling for the Adafruit Ultimate GPS PA1616D.
 */

#include "gps.h"

#include <stdint.h>

static char sentence[GPS_NMEA_SENTENCE_MAX];
static size_t sentence_length;
static bool discarding_sentence;

void gps_init(void) {
    uart_init(GPS_UART_ID, GPS_UART_BAUD);
    gpio_set_function(GPS_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(GPS_UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(GPS_UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(GPS_UART_ID, true);
}

bool gps_read_sentence(char *line, size_t line_capacity) {
    if (!line || line_capacity == 0) {
        return false;
    }

    while (uart_is_readable(GPS_UART_ID)) {
        uint8_t byte = (uint8_t)uart_getc(GPS_UART_ID);

        if (byte == '\r') {
            continue;
        }

        if (byte == '\n') {
            if (!discarding_sentence && sentence_length > 0) {
                size_t copy_length = sentence_length;
                if (copy_length >= line_capacity) {
                    copy_length = line_capacity - 1;
                }
                for (size_t i = 0; i < copy_length; i++) {
                    line[i] = sentence[i];
                }
                line[copy_length] = '\0';
                sentence_length = 0;
                return true;
            }

            sentence_length = 0;
            discarding_sentence = false;
            continue;
        }

        if (discarding_sentence) {
            continue;
        }

        if (sentence_length + 1 >= sizeof(sentence)) {
            sentence_length = 0;
            discarding_sentence = true;
            continue;
        }

        sentence[sentence_length++] = (char)byte;
    }

    return false;
}
