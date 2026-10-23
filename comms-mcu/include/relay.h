#ifndef BOAT_COMMS_RELAY_H
#define BOAT_COMMS_RELAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mavlink.h"
#include "mailbox.h"

#define RELAY_QUEUE_CAPACITY 8u
#define RELAY_MAX_FRAME ARSEM_MAILBOX_FRAME_BYTES

typedef struct {
    uint8_t bytes[RELAY_MAX_FRAME];
    uint8_t length;
    uint8_t priority;
    uint32_t msgid;
    uint8_t sysid;
    uint8_t compid;
    uint32_t order;
} relay_item_t;

typedef struct {
    relay_item_t entries[RELAY_QUEUE_CAPACITY];
    uint8_t count;
} relay_queue_t;

typedef struct {
    uint32_t ground_session;
    uint32_t request_id;
} relay_correlation_t;

typedef struct {
    relay_queue_t to_pi;
    relay_queue_t to_radio;
    relay_queue_t terminal_to_radio;
    relay_item_t terminal_inflight;
    bool terminal_inflight_valid;
    relay_correlation_t response_reservations[RELAY_QUEUE_CAPACITY];
    uint8_t response_reservation_count;
    uint8_t health_snapshot[RELAY_MAX_FRAME];
    uint8_t heartbeat_snapshot[RELAY_MAX_FRAME];
    uint8_t link_snapshot[RELAY_MAX_FRAME];
    uint8_t health_length;
    uint8_t heartbeat_length;
    uint8_t link_length;
    uint8_t snapshot_cursor;
    uint32_t sequence;
    uint32_t queue_drops;
    uint32_t malformed_frames;
    uint32_t forwarded_from_radio;
    uint32_t forwarded_from_pi;
    uint32_t radio_rx_packets;
    uint32_t radio_tx_packets;
    uint32_t radio_rx_errors;
    uint32_t radio_tx_errors;
    uint32_t last_radio_metrics_ms;
    int16_t radio_rssi_dbm;
    int16_t radio_snr_cdb;
    bool radio_metrics_valid;
    uint32_t boot_id;
    uint32_t started_ms;
    mavlink_message_t parser_message;
    mavlink_status_t parser_status;
} relay_t;

typedef enum {
    RELAY_ACCEPTED = 0,
    RELAY_REJECT_MALFORMED,
    RELAY_REJECT_SIGNED,
    RELAY_REJECT_OVERSIZE,
    RELAY_REJECT_CONCATENATED,
    RELAY_REJECT_UNSUPPORTED,
    RELAY_REJECT_QUEUE_FULL
} relay_result_t;

void relay_init(relay_t *relay, uint32_t boot_id, uint32_t now_ms);
relay_result_t relay_radio_frame(relay_t *relay, const uint8_t *frame, size_t length,
                                 uint32_t now_ms);
relay_result_t relay_i2c_write(relay_t *relay, const uint8_t mailbox[RELAY_MAX_FRAME]);
bool relay_i2c_read(relay_t *relay, uint8_t mailbox[RELAY_MAX_FRAME]);
bool relay_radio_next(relay_t *relay, uint8_t frame[RELAY_MAX_FRAME], size_t *length);
bool relay_radio_retain_terminal(relay_t *relay, const uint8_t *frame, size_t length);
void relay_radio_complete(relay_t *relay, bool accepted);
void relay_radio_reject_movement(relay_t *relay, const uint8_t *frame, size_t length);
void relay_refresh_heartbeat(relay_t *relay, uint32_t now_ms);
void relay_refresh_health(relay_t *relay, uint32_t now_ms);
void relay_refresh_link(relay_t *relay, uint32_t now_ms, bool connected,
                        uint16_t radio_address);

/* Validate one whole unsigned MAVLink 2 frame and zero padding to 64 bytes.
 * When padded is false, length must equal the MAVLink frame length exactly. */
relay_result_t relay_validate_frame(relay_t *relay, const uint8_t *bytes,
                                    size_t length, bool padded,
                                    mavlink_message_t *decoded,
                                    uint8_t *frame_length);

#endif
