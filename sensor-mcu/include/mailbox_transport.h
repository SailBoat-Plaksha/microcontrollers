#ifndef SENSOR_MAILBOX_TRANSPORT_H
#define SENSOR_MAILBOX_TRANSPORT_H
#include "sensor_node.h"
#include "pico/i2c_slave.h"
typedef struct {
    volatile uint8_t rx[SENSOR_FRAME_BYTES], tx[SENSOR_FRAME_BYTES];
    volatile bool rx_ready, rx_invalid, tx_available, read_started, read_reply_available;
    volatile size_t tx_position;
    volatile uint16_t dropped_writes;
    uint8_t write_staging[SENSOR_FRAME_BYTES];
    size_t write_count;
    bool write_overflow;
} sensor_mailbox_t;
void sensor_mailbox_init(sensor_mailbox_t *m);
void sensor_mailbox_bind(sensor_mailbox_t *m);
void sensor_mailbox_callback(i2c_inst_t *i2c, i2c_slave_event_t event);
void sensor_mailbox_service(sensor_mailbox_t *m, sensor_node_t *node);
#endif
