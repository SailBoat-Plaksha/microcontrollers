#ifndef MOTOR_TEST_HARDWARE_GPIO_H
#define MOTOR_TEST_HARDWARE_GPIO_H
#include <stdbool.h>
#include <stdint.h>
typedef unsigned int uint;
#define GPIO_OUT 1
#define GPIO_IN 0
#define GPIO_FUNC_I2C 3
#define GPIO_IRQ_EDGE_RISE 1u
#define GPIO_IRQ_EDGE_FALL 2u
void gpio_init(uint pin);
void gpio_set_dir(uint pin, bool out);
void gpio_put(uint pin, bool value);
void gpio_add_raw_irq_handler_masked(uint32_t mask, void (*handler)(void));
void gpio_set_input_hysteresis_enabled(uint pin, bool enabled);
void gpio_set_irq_enabled(uint pin, uint32_t events, bool enabled);
uint32_t gpio_get_irq_event_mask(uint pin);
void gpio_acknowledge_irq(uint pin, uint32_t events);
bool gpio_get(uint pin);
void gpio_set_function(uint pin, uint function);
void gpio_pull_up(uint pin);
#endif
