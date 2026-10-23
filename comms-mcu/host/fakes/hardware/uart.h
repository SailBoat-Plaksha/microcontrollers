#ifndef HOST_FAKE_HARDWARE_UART_H
#define HOST_FAKE_HARDWARE_UART_H
#include <stdbool.h>
#include <stdint.h>
typedef struct uart_inst { int unused; } uart_inst_t;
extern uart_inst_t *uart1;
enum { UART_PARITY_NONE };
uint uart_init(uart_inst_t *uart, uint baud);
void uart_set_format(uart_inst_t *uart, uint data_bits, uint stop_bits, int parity);
void uart_set_fifo_enabled(uart_inst_t *uart, bool enabled);
bool uart_is_readable(uart_inst_t *uart);
uint8_t uart_getc(uart_inst_t *uart);
bool uart_is_writable(uart_inst_t *uart);
void uart_putc_raw(uart_inst_t *uart, uint8_t value);
#endif
