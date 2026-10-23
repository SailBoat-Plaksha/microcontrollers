#include "mailbox_transport.h"
#include "hardware/sync.h"

#include <string.h>

static mailbox_transport_t *active_mailbox;

void mailbox_transport_bind(mailbox_transport_t *mailbox) {
    active_mailbox = mailbox;
}

void mailbox_transport_init(mailbox_transport_t *mailbox,
                            const uint8_t initial[ARSEM_MAILBOX_FRAME_BYTES]) {
    memset(mailbox, 0, sizeof(*mailbox));
    if (initial) {
        memcpy((void *)mailbox->tx, initial, ARSEM_MAILBOX_FRAME_BYTES);
        mailbox->tx_available = true;
    }
    mailbox_transport_bind(mailbox);
}

void mailbox_transport_callback(i2c_inst_t *i2c, i2c_slave_event_t event) {
    mailbox_transport_t *mailbox = active_mailbox;
    if (!mailbox) return;
    switch (event) {
    case I2C_SLAVE_RECEIVE:
        if (mailbox->write_count < sizeof(mailbox->write_staging)) {
            mailbox->write_staging[mailbox->write_count++] = i2c_read_byte_raw(i2c);
        } else {
            (void)i2c_read_byte_raw(i2c);
            mailbox->write_overflow = true;
        }
        break;
    case I2C_SLAVE_REQUEST:
        /* The SDK invokes REQUEST for every RD_REQ. A 65th request gets a
         * zero sentinel; it is never treated as an extra mailbox byte. */
        if (mailbox->tx_available && mailbox->tx_position < ARSEM_MAILBOX_FRAME_BYTES)
            i2c_write_byte_raw(i2c, mailbox->tx[mailbox->tx_position++]);
        else
            i2c_write_byte_raw(i2c, 0u);
        break;
    case I2C_SLAVE_FINISH:
        if (mailbox->write_count || mailbox->write_overflow) {
            if (mailbox->write_count == ARSEM_MAILBOX_FRAME_BYTES &&
                !mailbox->write_overflow && !mailbox->rx_ready) {
                for (size_t i = 0; i < ARSEM_MAILBOX_FRAME_BYTES; ++i)
                    mailbox->rx[i] = mailbox->write_staging[i];
                mailbox->rx_ready = true;
            } else {
                mailbox->rx_invalid = true;
                if (mailbox->dropped_writes != UINT16_MAX) mailbox->dropped_writes++;
            }
            mailbox->write_count = 0;
            mailbox->write_overflow = false;
        } else {
            /* A short/aborted read is retried from byte zero. The staged
             * frame remains intact until a complete 64-byte read finishes. */
            if (mailbox->tx_position == ARSEM_MAILBOX_FRAME_BYTES)
                mailbox->tx_available = false;
            mailbox->tx_position = 0;
        }
        break;
    }
}

void mailbox_transport_service(mailbox_transport_t *mailbox, relay_t *relay) {
    uint8_t received[ARSEM_MAILBOX_FRAME_BYTES];
    uint16_t dropped = 0;
    bool have_received = false;
    bool invalid = false;
    uint32_t irq_state = save_and_disable_interrupts();
    if (mailbox->rx_ready) {
        for (size_t i = 0; i < sizeof(received); ++i) received[i] = mailbox->rx[i];
        mailbox->rx_ready = false;
        have_received = true;
    }
    invalid = mailbox->rx_invalid;
    mailbox->rx_invalid = false;
    dropped = mailbox->dropped_writes;
    mailbox->dropped_writes = 0;
    if (!mailbox->tx_available) {
        uint8_t next[ARSEM_MAILBOX_FRAME_BYTES];
        (void)relay_i2c_read(relay, next);
        for (size_t i = 0; i < sizeof(next); ++i) mailbox->tx[i] = next[i];
        mailbox->tx_position = 0;
        mailbox->tx_available = true;
    }
    restore_interrupts(irq_state);

    if (have_received) (void)relay_i2c_write(relay, received);
    if (invalid) {
        uint32_t count = dropped ? dropped : 1u;
        relay->malformed_frames += count;
        relay->radio_tx_errors += count;
    }
}
