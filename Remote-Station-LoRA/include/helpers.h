/*
 * helpers.h : Board Helpers
 * -------------------------
 * Shared board configuration and JSON/UART helper declarations for the
 * remote station.
 */

#ifndef REMOTE_HELPERS_H
#define REMOTE_HELPERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hardware/uart.h"
#include "cjson.h"

/* ==== Configuration ==== */

#ifndef EXT_LED
#define EXT_LED 22
#endif

#ifndef ERR_LED
#define ERR_LED 17
#endif

#ifndef MASTER_UART_ID
#define MASTER_UART_ID uart1
#endif

#ifndef MASTER_UART_TX_PIN
#define MASTER_UART_TX_PIN 4
#endif

#ifndef MASTER_UART_RX_PIN
#define MASTER_UART_RX_PIN 5
#endif

#ifndef LORA_UART_ID
#define LORA_UART_ID uart0
#endif

#ifndef LORA_UART_TX_PIN
#define LORA_UART_TX_PIN 0
#endif

#ifndef LORA_UART_RX_PIN
#define LORA_UART_RX_PIN 1
#endif

#ifndef MASTER_UART_BAUD
#define MASTER_UART_BAUD 115200
#endif

#ifndef LORA_UART_BAUD
#define LORA_UART_BAUD 115200
#endif

/* ==== Function Declarations ==== */

/**
 * @brief Configure a pin as input.
 */
void pinnit(int pin);

/**
 * @brief Configure a pin as output.
 */
void poutit(int pin);

/**
 * @brief Set the external LED state.
 */
void led(int high_low);

/**
 * @brief Set the error LED state.
 */
void err_led(int high_low);

/**
 * @brief Send a JSON object over stdout.
 */
void json_send_object(uart_inst_t *uart_id, const cJSON *object);

/**
 * @brief Send a JSON error event.
 */
void json_send_error(uart_inst_t *uart_id, const char *message);

/**
 * @brief Send a JSON status event.
 */
void json_send_status(uart_inst_t *uart_id, const char *status, const char *message);

#endif