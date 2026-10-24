#ifndef SENSOR_HOST_I2C_SLAVE_H
#define SENSOR_HOST_I2C_SLAVE_H
#include <stdint.h>
#include <stddef.h>
typedef struct i2c_inst { const uint8_t *input; size_t input_len,input_pos; uint8_t output[128]; size_t output_len; } i2c_inst_t;
typedef enum { I2C_SLAVE_RECEIVE, I2C_SLAVE_REQUEST, I2C_SLAVE_FINISH } i2c_slave_event_t;
uint8_t i2c_read_byte_raw(i2c_inst_t *i2c);
void i2c_write_byte_raw(i2c_inst_t *i2c, uint8_t byte);
#endif
