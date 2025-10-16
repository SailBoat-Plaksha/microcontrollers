/*
 * uart.h : UART Module
 * --------------------------
 * Esraaj Sarkar Gupta 2025
 *
 * Header for the UART driver for the Raspberry Pi Pico
 * (RP2040 / RP2350). Provides initialization, transmission,
 * reception, and ring buffer management.
 *
 */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==== Configuration ==== */

// Maximum buffer size for RX ring buffer
#define UART_RX_BUFFER_SIZE 256

// Default BAUD rate
#define UART_DEFAULT_BAUDRATE 115200

/* ----- Type Definitions ----- */

/**
 * @brief Ring buffer structure for UART RX.
 */
typedef struct {
    uint8_t data[UART_RX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} ringbuf_t;

/* ==== External Buffers ==== */

extern ringbuf_t rb0;
extern ringbuf_t rb1;

/* ==== Function Declarations ==== */

/**
 * @brief Initialize a UART instance with given parameters.
 *
 * @param uart_id UART instance (here uart0, uart1 ONLY)
 * @param tx_pin TX GPIO pin
 * @param rx_pin RX GPIO pin
 * @param baud Baud rate
 * @param enable_irq Enable interrupts for RX handling
 */
void uart_driver_init(uart_inst_t *uart_id,  uint baud, uint tx_pin, uint rx_pin);

/**
 * @brief Write a character over UART.
 *
 * @param uart_id UART instance
 * @param c Character to send
 */
void uart_send_char(uart_inst_t *uart_id, char c);

/**
 * @brief Write a string over UART.
 *
 * @param uart_id UART instance
 * @param str Null-terminated string
 */
void uart_send_str(uart_inst_t *uart_id, const char *str);

/**
 * @brief Read a character from the RX buffer (non-blocking).
 *
 * @param uart_id UART instance
 * @param c Pointer to store received character
 * @return true if a character was read, false if buffer empty
 */
bool uart_read_char(uart_inst_t *uart_id, char *c);

/**
 * @brief Clear the RX ring buffer.
 *
 * @param rb Pointer to ring buffer
 */
void uart_clear_buffer(ringbuf_t *rb);

/**
 * @brief Check if RX buffer is empty.
 *
 * @param rb Pointer to ring buffer
 * @return true if empty, false otherwise
 */
bool uart_buffer_empty(const ringbuf_t *rb);

/**
 * @brief ISR handler for UART0 RX.
 */
void on_uart0_rx(void);

/**
 * @brief ISR handler for UART1 RX.
 */
void on_uart1_rx(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_DRIVER_H */

