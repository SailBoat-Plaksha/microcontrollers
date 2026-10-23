#ifndef HOST_FAKE_I2C_SLAVE_H
#define HOST_FAKE_I2C_SLAVE_H
#include <stddef.h>
#include <stdint.h>
typedef unsigned int uint;
typedef struct i2c_inst { int unused; } i2c_inst_t;
extern i2c_inst_t *i2c0;
extern i2c_inst_t *i2c1;
typedef enum { I2C_SLAVE_RECEIVE, I2C_SLAVE_REQUEST, I2C_SLAVE_FINISH } i2c_slave_event_t;
typedef void (*i2c_slave_handler_t)(i2c_inst_t *, i2c_slave_event_t);
void i2c_init(i2c_inst_t *i2c, uint baud);
void i2c_slave_init(i2c_inst_t *i2c, uint8_t address, i2c_slave_handler_t handler);
uint8_t i2c_read_byte_raw(i2c_inst_t *i2c);
void i2c_write_byte_raw(i2c_inst_t *i2c, uint8_t value);
#endif
