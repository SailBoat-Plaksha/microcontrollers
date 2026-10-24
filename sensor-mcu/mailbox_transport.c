#include "include/mailbox_transport.h"
#include "hardware/sync.h"
#include <string.h>
static sensor_mailbox_t *active;
void sensor_mailbox_bind(sensor_mailbox_t*m){active=m;}
void sensor_mailbox_init(sensor_mailbox_t*m){memset(m,0,sizeof *m);sensor_mailbox_bind(m);}
void sensor_mailbox_callback(i2c_inst_t*i2c,i2c_slave_event_t event){sensor_mailbox_t*m=active;if(!m)return;
    switch(event){case I2C_SLAVE_RECEIVE: if(m->write_count<SENSOR_FRAME_BYTES)m->write_staging[m->write_count++]=i2c_read_byte_raw(i2c);else{(void)i2c_read_byte_raw(i2c);m->write_overflow=true;}break;
    case I2C_SLAVE_REQUEST: if(!m->read_started)m->read_reply_available=m->tx_available;m->read_started=true;if(m->read_reply_available&&m->tx_position<SENSOR_FRAME_BYTES)i2c_write_byte_raw(i2c,m->tx[m->tx_position++]);else i2c_write_byte_raw(i2c,0);break;
    case I2C_SLAVE_FINISH: if(m->write_count||m->write_overflow){if(m->write_count==SENSOR_FRAME_BYTES&&!m->write_overflow&&!m->rx_ready){for(size_t i=0;i<SENSOR_FRAME_BYTES;i++)m->rx[i]=m->write_staging[i];m->rx_ready=true;}else{m->rx_invalid=true;if(m->dropped_writes!=UINT16_MAX)m->dropped_writes++;}m->write_count=0;m->write_overflow=false;}else{if(m->read_reply_available&&m->tx_position==SENSOR_FRAME_BYTES)m->tx_available=false;m->tx_position=0;m->read_started=false;m->read_reply_available=false;}break;}
}
void sensor_mailbox_service(sensor_mailbox_t*m,sensor_node_t*n){uint8_t rx[64],tx[64];bool have=false,invalid=false;uint16_t dropped=0;uint32_t irq=save_and_disable_interrupts();if(m->rx_ready){for(size_t i=0;i<64;i++)rx[i]=m->rx[i];m->rx_ready=false;have=true;}invalid=m->rx_invalid;m->rx_invalid=false;dropped=m->dropped_writes;m->dropped_writes=0;if(!m->tx_available&&!m->read_started&&sensor_node_take_tx(n,tx)){for(size_t i=0;i<64;i++)m->tx[i]=tx[i];m->tx_position=0;m->tx_available=true;}restore_interrupts(irq);if(have)(void)sensor_node_receive(n,rx);if(invalid)n->malformed+=dropped?dropped:1;}
