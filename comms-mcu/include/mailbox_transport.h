#ifndef BOAT_COMMS_MAILBOX_TRANSPORT_H
#define BOAT_COMMS_MAILBOX_TRANSPORT_H

#include "pico/i2c_slave.h"
#include "relay.h"

typedef struct {
    volatile uint8_t rx[ARSEM_MAILBOX_FRAME_BYTES];
    volatile bool rx_ready;
    volatile bool rx_invalid;
    volatile uint16_t dropped_writes;
    volatile uint8_t tx[ARSEM_MAILBOX_FRAME_BYTES];
    volatile bool tx_available;
    volatile bool read_started;
    volatile bool read_reply_available;
    volatile size_t tx_position;
    uint8_t write_staging[ARSEM_MAILBOX_FRAME_BYTES];
    size_t write_count;
    bool write_overflow;
} mailbox_transport_t;

void mailbox_transport_init(mailbox_transport_t *mailbox,
                            const uint8_t initial[ARSEM_MAILBOX_FRAME_BYTES]);
void mailbox_transport_callback(i2c_inst_t *i2c, i2c_slave_event_t event);
void mailbox_transport_bind(mailbox_transport_t *mailbox);
void mailbox_transport_service(mailbox_transport_t *mailbox, relay_t *relay);

#endif
