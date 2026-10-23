#ifndef HOST_FAKE_HARDWARE_GPIO_H
#define HOST_FAKE_HARDWARE_GPIO_H
#include <stdint.h>
enum { GPIO_FUNC_UART, GPIO_FUNC_I2C };
void gpio_set_function(uint pin, int function);
void gpio_pull_up(uint pin);
#endif
