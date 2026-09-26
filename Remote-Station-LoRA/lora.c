/*
 * lora.c : LoRa Module
 * --------------------
 * Implementation of the REYAX RYLR998 helper layer used by the remote station.
 * Provides AT command helpers and receive-line parsing.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lora.h"
#include "uart.h"

/* ==== Internal Helpers ==== */

static const char *find_last_char(const char *text, char needle) {
    if (!text) {
        return NULL;
    }

    const char *cursor = text + strlen(text);
    while (cursor > text) {
        cursor--;
        if (*cursor == needle) {
            return cursor;
        }
    }

    return NULL;
}

static const char *find_prev_char(const char *start, const char *before, char needle) {
    if (!start || !before || before <= start) {
        return NULL;
    }

    const char *cursor = before;
    while (cursor > start) {
        cursor--;
        if (*cursor == needle) {
            return cursor;
        }
    }

    return NULL;
}

/* ==== Public API ==== */

void lora_init(lora_state_t *state, uart_inst_t *uart, uint tx_pin, uint rx_pin, uint baudrate) {
    if (!state) {
        return;
    }

    state->uart = uart;
    state->tx_pin = tx_pin;
    state->rx_pin = rx_pin;
    state->baudrate = baudrate;

    uart_driver_init(uart, baudrate, tx_pin, rx_pin);
}

bool lora_send_command(lora_state_t *state, const char *command) {
    if (!state || !state->uart || !command) {
        return false;
    }

    char line[300];
    size_t command_len = strlen(command);
    while (command_len > 0 && (command[command_len - 1] == '\r' || command[command_len - 1] == '\n')) {
        command_len--;
    }

    if (command_len + 2u >= sizeof(line)) {
        return false;
    }

    memcpy(line, command, command_len);
    line[command_len++] = '\r';
    line[command_len++] = '\n';
    line[command_len] = '\0';

    uart_send_str(state->uart, line);
    return true;
}

bool lora_send_message(lora_state_t *state, uint16_t destination, const char *message) {
    if (!state || !state->uart || !message) {
        return false;
    }

    size_t message_length = strlen(message);
    if (message_length > 240u) return false;
    char command[280];
    int written = snprintf(command, sizeof(command), "AT+SEND=%u,%u,%s", (unsigned)destination, (unsigned)message_length, message);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        return false;
    }

    return lora_send_command(state, command);
}

bool lora_set_address(lora_state_t *state, uint16_t address) {
    if (!state) {
        return false;
    }

    char command[64];
    int written = snprintf(command, sizeof(command), "AT+ADDRESS=%u", (unsigned)address);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        return false;
    }

    if (!lora_send_command(state, command)) {
        return false;
    }

    return true;
}

bool lora_set_network_id(lora_state_t *state, uint8_t network_id) {
    if (!state) {
        return false;
    }

    char command[64];
    int written = snprintf(command, sizeof(command), "AT+NETWORKID=%u", (unsigned)network_id);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        return false;
    }

    if (!lora_send_command(state, command)) {
        return false;
    }

    return true;
}

bool lora_set_band(lora_state_t *state, uint32_t band) {
    if (!state) {
        return false;
    }

    char command[64];
    int written = snprintf(command, sizeof(command), "AT+BAND=%lu", (unsigned long)band);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        return false;
    }

    if (!lora_send_command(state, command)) {
        return false;
    }

    return true;
}

bool lora_set_parameter(lora_state_t *state, const char *parameter) {
    if (!state || !parameter) {
        return false;
    }

    char command[96];
    int written = snprintf(command, sizeof(command), "AT+PARAMETER=%s", parameter);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        return false;
    }

    if (!lora_send_command(state, command)) {
        return false;
    }

    return true;
}

bool lora_parse_rcv_line(const char *line, lora_rx_frame_t *frame) {
    if (!line || !frame || strncmp(line, "+RCV=", 5) != 0) {
        return false;
    }

    const char *content = line + 5;
    const char *first = strchr(content, ',');
    const char *second = first ? strchr(first + 1, ',') : NULL;
    const char *last = find_last_char(content, ',');
    const char *prev = last ? find_prev_char(content, last, ',') : NULL;

    if (!first || !second || !prev || prev <= second || last <= prev) {
        return false;
    }

    char temp[32];

    size_t field_length = (size_t)(first - content);
    if (field_length == 0 || field_length >= sizeof(temp)) {
        return false;
    }
    memcpy(temp, content, field_length);
    temp[field_length] = '\0';
    frame->address = (uint16_t)strtoul(temp, NULL, 10);

    field_length = (size_t)(second - (first + 1));
    if (field_length == 0 || field_length >= sizeof(temp)) {
        return false;
    }
    memcpy(temp, first + 1, field_length);
    temp[field_length] = '\0';
    frame->length = (uint16_t)strtoul(temp, NULL, 10);

    field_length = (size_t)(prev - (second + 1));
    if (field_length >= sizeof(frame->payload) || field_length != frame->length) return false;
    memcpy(frame->payload, second + 1, field_length);
    frame->payload[field_length] = '\0';

    field_length = (size_t)(last - (prev + 1));
    if (field_length == 0 || field_length >= sizeof(temp)) {
        return false;
    }
    memcpy(temp, prev + 1, field_length);
    temp[field_length] = '\0';
    frame->rssi = (int)strtol(temp, NULL, 10);

    frame->snr = (float)strtod(last + 1, NULL);
    if (frame->length > 0 && frame->payload[0] == '\0') {
        return false;
    }

    return true;
}
