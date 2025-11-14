/* 
 * uart.c -- UART Driver (Interrupt driven Rx)
 * --------------------------------------------
 * Esraaj Sarkar Gupta 2025
 *
 * This module provides UART driver for the Raspberry Pi Pico (RP2040/RP2350).
 *
 * Typical Usage:
 *
 *
 */
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <stdint.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef UART_RX_BUF_SIZE
#define UART_RX_BUF_SIZE 256
#endif

#ifndef UART_LED
#define UART_LED 22
#endif

/* ---- Interal Data Structures ---- */

/* Ring buffer to hold Rx data */
typedef struct {
  volatile uint8_t buffer[UART_RX_BUF_SIZE];
  volatile uint16_t head;
  volatile uint16_t tail;
} ringbuffer_t;

/* Static initialisations of Rx ring buffers for each UART instance*/
static ringbuffer_t rb0 = {0};
static ringbuffer_t rb1 = {0};

/* ---- Helper functions ---- */

void UART_error(int err_code);

/* Push a byte into a ring buffer */
static inline void rb_push(ringbuffer_t *rb, uint8_t character) {
  /* Developer's Note: The following if-else block can be made more efficient
   * by using bitwise operations if we are to constrain the buffer size to powers
   * of 2.
   */
  uint16_t next;
  if (rb->head + 1u == UART_RX_BUF_SIZE) {
    next = 0;
  } else {
    next = rb->head + 1u;
  }

  /* Overflow Error */
  if (next == rb->tail) {
    UART_error(5);
    return;
  }

  rb->buffer[rb->head] = character;
  rb->head = next;
}

/* Pop a byte if present; returns true if a byte was read */
static inline bool rb_pop(ringbuffer_t *rb, uint8_t *out) {
    if (rb->head == rb->tail) {
        return false;
    }

    *out = rb->buffer[rb->tail];
    if (rb->tail + 1u == UART_RX_BUF_SIZE) {
        rb->tail = 0;
    } else {
        rb->tail++;
    }
    return true;
}

/* ISR Logic for UART */
static void uart_isr(uart_inst_t *uid, ringbuffer_t *rb) {
  while (uart_is_readable(uid)) {
    uint8_t character = uart_getc(uid);
    rb_push(rb, character);
  }
}

/*
 * Interrupt Handlers -- ISR Wrappers
 * Required by NVIC (Nested Vectored Interrupt Controller)
 */

static void uart0_irq_handler(void) { uart_isr(uart0, &rb0); }
static void uart1_irq_handler(void) { uart_isr(uart1, &rb1); }

/* ---- API Implementation ---- */

void uart_driver_init(
  uart_inst_t *uid,
  uint baud,
  uint tx_pin,
  uint rx_pin
) {
  uart_init(uid, baud);
  gpio_set_function(tx_pin, GPIO_FUNC_UART);
  gpio_set_function(rx_pin, GPIO_FUNC_UART);

  uart_set_format(
    uid,
    8, // <-- Number of data bytes
    1, // <-- Stop bit
    UART_PARITY_NONE // <-- No error detection (using parity)
  );

  uart_set_hw_flow(
    uid,
    false, // <-- No CTS (Clear To Send) pin
    false // <-- No RTS (Request To Send) pin
  );

  uart_set_fifo_enabled(uid, true);

  if (uid == uart0) {
    irq_set_exclusive_handler(UART0_IRQ, uart0_irq_handler);
    irq_set_enabled(UART0_IRQ, true); // Enables selected interrupt in NVIC
  } else if (uid == uart1) {
    irq_set_exclusive_handler(UART1_IRQ, uart1_irq_handler);
    irq_set_enabled(UART1_IRQ, true);
  }

  uart_set_irq_enables(
    uid,
    true, // <-- Enables Rx interrupt
    false // <-- Disable Tx interrupt (ignore when Tx is empty)
  );
}

/* Read a byte from the ring buffer */
bool uart_read_byte(uart_inst_t *uid, uint8_t *out) {
  ringbuffer_t *rb = (uid == uart0) ? &rb0 : &rb1;
  return rb_pop(rb, out);
}

/* Send a byte via UART*/
void uart_send_byte(uart_inst_t *uid, uint8_t byte) {
  uart_write_blocking(uid, &byte, 1); // 1 is the length of the number of bytes being sent
}

/* Send a string via UART */
void uart_send_str(uart_inst_t *uid, const char *s) {
  gpio_put(UART_LED, 1); // UART indicator
  while (*s) uart_send_byte(uid, (uint8_t)(*s++));
  sleep_ms(10);
  gpio_put(UART_LED, 0);
}

/* Bulk send bytes via UART */
void uart_send_bulk(uart_inst_t *uid, uint8_t *bytes, int len) {
  uart_write_blocking(uid, bytes, len);
}

/* UART Error Handling */

void UART_error(int err_code) {
  // Pass;
}






