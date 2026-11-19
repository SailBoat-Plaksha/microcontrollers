/*
 * gps.h -- UART interface for the Adafruit Ultimate GPS PA1616D.
 */

#ifndef GPS_H
#define GPS_H

#include "hardware/uart.h"
#include "pico/stdlib.h"

#include <stdbool.h>
#include <stddef.h>

#ifndef GPS_UART_ID
#define GPS_UART_ID uart1
#endif

#ifndef GPS_UART_BAUD
#define GPS_UART_BAUD 9600
#endif

#ifndef GPS_UART_TX_PIN
#define GPS_UART_TX_PIN 4
#endif

#ifndef GPS_UART_RX_PIN
#define GPS_UART_RX_PIN 5
#endif

#define GPS_NMEA_SENTENCE_MAX 128

/** Initialize the GPS UART using 8 data bits, no parity, and 1 stop bit. */
void gps_init(void);

/**
 * Read bytes until a complete NMEA line is available.
 *
 * @return true when a sentence was copied to line, false when no full line is
 *         available yet. The returned line is null-terminated and excludes
 *         CR/LF. Overlong lines are discarded.
 */
bool gps_read_sentence(char *line, size_t line_capacity);

#endif
