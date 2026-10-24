#include "sensor_node.h"
#include "mailbox_transport.h"
#include "pico/i2c_slave.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static uint32_t fake_ms;
static uint32_t clock_fn(void*x){(void)x;return fake_ms;}
static int hexval(int c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
static void print_frame(const uint8_t b[64]){for(int i=0;i<64;i++)printf("%02x",b[i]);putchar('\n');}
int main(void){sensor_node_t n;sensor_mailbox_t mb;sensor_config_t c={clock_fn,NULL,1,10,0,1};sensor_node_init(&n,&c);sensor_mailbox_init(&mb);char line[512];puts("READY sensor-mcu raw64 harness; commands: I <128 hex>, T <absolute_ms>, O");while(fgets(line,sizeof line,stdin)){if(line[0]=='T'){fake_ms=(uint32_t)strtoul(line+1,NULL,10);sensor_node_tick(&n);sensor_mailbox_service(&mb,&n);puts("OK");}else if(line[0]=='I'){uint8_t b[64];const char*p=line+1;int ok=1;for(int i=0;i<64;i++){while(*p==' ')p++;int h=hexval(*p++),l=hexval(*p++);if(h<0||l<0){ok=0;break;}b[i]=(uint8_t)((h<<4)|l);}if(!ok){puts("ERR hex");continue;}i2c_inst_t bus={.input=b,.input_len=64};for(int i=0;i<64;i++)sensor_mailbox_callback(&bus,I2C_SLAVE_RECEIVE);sensor_mailbox_callback(&bus,I2C_SLAVE_FINISH);sensor_mailbox_service(&mb,&n);puts("OK");}else if(line[0]=='O'){uint8_t b[64];sensor_node_tick(&n);sensor_mailbox_service(&mb,&n);if(!mb.tx_available){puts("EMPTY");continue;}i2c_inst_t bus={0};for(int i=0;i<64;i++)sensor_mailbox_callback(&bus,I2C_SLAVE_REQUEST);sensor_mailbox_callback(&bus,I2C_SLAVE_FINISH);memcpy(b,bus.output,64);print_frame(b);}else puts("ERR command");}return 0;}
