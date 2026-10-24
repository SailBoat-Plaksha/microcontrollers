#include "pico/stdlib.h"
#include "pico/i2c_slave.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/structs/rosc.h"
#include "config.h"
#include "include/mailbox_transport.h"

static sensor_node_t node;
static sensor_mailbox_t mailbox;
static uint32_t boot_id(void){uint32_t v=time_us_32()^(uint32_t)(uintptr_t)&node;for(unsigned i=0;i<80;i++){v=(v<<1)^(uint32_t)(rosc_hw->randombit&1u);v^=v>>11;v*=0x45d9f3bu;}return v?v:1u;}
static uint32_t clock_ms(void *context){(void)context;return to_ms_since_boot(get_absolute_time());}
int main(void){
    sensor_config_t c={clock_ms,NULL,0,SENSOR_I2C_ADDRESS,SENSOR_I2C_SDA_PIN,SENSOR_I2C_SCL_PIN};c.boot_id=boot_id();
    sensor_node_init(&node,&c);sensor_mailbox_init(&mailbox);
    i2c_init(SENSOR_I2C,ARSEM_I2C_BUS_HZ);gpio_set_function(SENSOR_I2C_SDA_PIN,GPIO_FUNC_I2C);gpio_set_function(SENSOR_I2C_SCL_PIN,GPIO_FUNC_I2C);gpio_pull_up(SENSOR_I2C_SDA_PIN);gpio_pull_up(SENSOR_I2C_SCL_PIN);
    i2c_slave_init(SENSOR_I2C,SENSOR_I2C_ADDRESS,sensor_mailbox_callback);
    while(true){sensor_mailbox_service(&mailbox,&node);sensor_node_tick(&node);tight_loop_contents();}
}
