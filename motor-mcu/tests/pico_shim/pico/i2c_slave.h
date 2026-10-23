#ifndef MOTOR_TEST_PICO_I2C_SLAVE_H
#define MOTOR_TEST_PICO_I2C_SLAVE_H
#include "hardware/i2c.h"
typedef enum { I2C_SLAVE_RECEIVE, I2C_SLAVE_REQUEST, I2C_SLAVE_FINISH } i2c_slave_event_t;
typedef void (*i2c_slave_handler_t)(i2c_inst_t *, i2c_slave_event_t);
void i2c_slave_init(i2c_inst_t *i2c, uint8_t address, i2c_slave_handler_t handler);
#endif
