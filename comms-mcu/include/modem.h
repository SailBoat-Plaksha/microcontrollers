#ifndef BOAT_COMMS_MODEM_H
#define BOAT_COMMS_MODEM_H

#include "relay.h"

#define MODEM_LINE_BYTES 300u
#define MODEM_LINE_QUEUE 4u

typedef struct {
    char current[MODEM_LINE_BYTES];
    size_t used;
    char lines[MODEM_LINE_QUEUE][MODEM_LINE_BYTES];
    uint16_t lengths[MODEM_LINE_QUEUE];
    uint8_t head;
    uint8_t count;
    uint32_t dropped_lines;
} modem_stream_t;

void modem_stream_init(modem_stream_t *stream);
void modem_stream_feed(modem_stream_t *stream, const uint8_t *bytes, size_t length);
bool modem_stream_pop(modem_stream_t *stream, char line[MODEM_LINE_BYTES]);
relay_result_t modem_parse_rcv_line(relay_t *relay, const char *line,
                                    uint16_t expected_source_address,
                                    uint32_t now_ms);
bool modem_build_send_command(uint16_t destination, const uint8_t *frame,
                              size_t frame_length, char *command,
                              size_t command_capacity);

#endif
