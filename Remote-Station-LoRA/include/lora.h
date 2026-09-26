/*
 * lora.h : LoRa Module
 * --------------------
 * Header for the REYAX RYLR998 helper layer used by the remote station.
 * Provides state tracking, command helpers, and receive-line parsing.
 */

#ifndef REMOTE_LORA_H
#define REMOTE_LORA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hardware/uart.h"

/* ==== Configuration ==== */

// Maximum payload size stored from a received LoRa frame
#define LORA_PAYLOAD_MAX 241

/* ----- Type Definitions ----- */

/**
 * @brief LoRa driver state.
 */
typedef struct {
    uart_inst_t *uart;
    uint tx_pin;
    uint rx_pin;
    uint baudrate;
} lora_state_t;

/**
 * @brief Parsed LoRa receive frame.
 */
typedef struct {
    uint16_t address;
    uint16_t length;
    int rssi;
    float snr;
    char payload[LORA_PAYLOAD_MAX];
} lora_rx_frame_t;

/* ==== Function Declarations ==== */

/**
 * @brief Initialize the LoRa helper state and UART driver.
 *
 * @param state LoRa state object
 * @param uart UART instance used by the module
 * @param tx_pin TX GPIO pin
 * @param rx_pin RX GPIO pin
 * @param baudrate UART baud rate
 */
void lora_init(lora_state_t *state, uart_inst_t *uart, uint tx_pin, uint rx_pin, uint baudrate);

/**
 * @brief Send a raw AT command to the LoRa module.
 *
 * @param state LoRa state object
 * @param command Null-terminated AT command
 * @return true if the command was queued for transmission, false otherwise
 */
bool lora_send_command(lora_state_t *state, const char *command);

/**
 * @brief Send a payload to a remote LoRa destination.
 *
 * @param state LoRa state object
 * @param destination Destination address
 * @param message Message payload
 * @return true if the command was queued for transmission, false otherwise
 */
bool lora_send_message(lora_state_t *state, uint16_t destination, const char *message);

/**
 * @brief Set the LoRa module address.
 */
bool lora_set_address(lora_state_t *state, uint16_t address);

/**
 * @brief Set the LoRa network ID.
 */
bool lora_set_network_id(lora_state_t *state, uint8_t network_id);

/**
 * @brief Set the LoRa band frequency.
 */
bool lora_set_band(lora_state_t *state, uint32_t band);

/**
 * @brief Set the LoRa parameter string.
 */
bool lora_set_parameter(lora_state_t *state, const char *parameter);

/**
 * @brief Parse a +RCV line into a receive frame.
 */
bool lora_parse_rcv_line(const char *line, lora_rx_frame_t *frame);

#endif
