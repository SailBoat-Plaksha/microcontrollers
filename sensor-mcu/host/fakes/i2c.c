#include "pico/i2c_slave.h"
uint8_t i2c_read_byte_raw(i2c_inst_t*i){return i->input_pos<i->input_len?i->input[i->input_pos++]:0;}
void i2c_write_byte_raw(i2c_inst_t*i,uint8_t b){if(i->output_len<sizeof i->output)i->output[i->output_len++]=b;}
