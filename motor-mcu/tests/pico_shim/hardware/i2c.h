#ifndef MOTOR_TEST_HARDWARE_I2C_H
#define MOTOR_TEST_HARDWARE_I2C_H
#include <stddef.h>
#include <stdint.h>
typedef struct i2c_inst { unsigned int id; } i2c_inst_t;
extern i2c_inst_t fake_i2c0;
#define i2c0 (&fake_i2c0)
size_t i2c_get_read_available(i2c_inst_t *i2c);
uint8_t i2c_read_byte_raw(i2c_inst_t *i2c);
size_t i2c_get_write_available(i2c_inst_t *i2c);
void i2c_write_byte_raw(i2c_inst_t *i2c, uint8_t byte);
int i2c_init(i2c_inst_t *i2c, uint baudrate);
#endif
