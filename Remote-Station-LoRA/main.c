/* Transparent MAVLink 2 gateway: raw USB serial <-> Base64 RYLR998 packets. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "helpers.h"
#include "lora.h"
#include "uart.h"
#include "mavlink_min.h"

static lora_state_t radio;
static mav_parser_t usb_parser;
static char radio_line[300];
static size_t radio_used;

static void service_usb(void) {
    int byte;
    uint8_t frame[MAV_FRAME_MAX];
    size_t length = 0;
    while ((byte = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
        if (!mav_feed(&usb_parser, (uint8_t)byte, frame, &length)) continue;
        char encoded[241];
        size_t text_length = mav_base64_encode(frame, length, encoded, sizeof(encoded));
        if (text_length == 0 || text_length > 240 || !lora_send_message(&radio, 0, encoded)) {
            err_led(1);
        } else {
            led(1);
        }
    }
}

static void handle_radio_line(const char *line) {
    lora_rx_frame_t received;
    if (!lora_parse_rcv_line(line, &received)) return;
    uint8_t frame[MAV_FRAME_MAX];
    size_t length = mav_base64_decode(received.payload, received.length, frame, sizeof(frame));
    if (!length || !mav_valid(frame, length)) { err_led(1); return; }
    for (size_t i = 0; i < length; ++i) putchar_raw(frame[i]);
    led(1);
}

static void service_radio(void) {
    uint8_t byte;
    while (uart_read_byte(LORA_UART_ID, &byte)) {
        if (byte == '\n' || byte == '\r') {
            if (radio_used) {
                radio_line[radio_used] = 0;
                handle_radio_line(radio_line);
                radio_used = 0;
            }
        } else if (radio_used + 1 < sizeof(radio_line)) {
            radio_line[radio_used++] = (char)byte;
        } else {
            radio_used = 0;
            err_led(1);
        }
    }
}

int main(void) {
    stdio_init_all();
    poutit(EXT_LED);
    poutit(ERR_LED);
    led(0);
    err_led(0);
    lora_init(&radio, LORA_UART_ID, LORA_UART_TX_PIN, LORA_UART_RX_PIN, LORA_UART_BAUD);
    while (true) {
        service_usb();
        service_radio();
        sleep_ms(1);
        led(0);
    }
}
