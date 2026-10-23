#include "modem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int base64_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static size_t base64_decode(const char *src, size_t length, uint8_t *dst, size_t cap) {
    if (!length || (length % 4u) != 0u) return 0u;
    size_t out = 0u;
    for (size_t i = 0; i < length; i += 4u) {
        int a = base64_value(src[i]), b = base64_value(src[i + 1u]);
        bool pad2 = src[i + 2u] == '=', pad3 = src[i + 3u] == '=';
        int c = pad2 ? 0 : base64_value(src[i + 2u]);
        int d = pad3 ? 0 : base64_value(src[i + 3u]);
        if (a < 0 || b < 0 || c < 0 || d < 0) return 0u;
        if ((pad2 && !pad3) || ((pad2 || pad3) && i + 4u != length)) return 0u;
        if (out + 1u > cap) return 0u;
        dst[out++] = (uint8_t)((a << 2) | (b >> 4));
        if (!pad2) {
            if (out + 1u > cap) return 0u;
            dst[out++] = (uint8_t)((b << 4) | (c >> 2));
        }
        if (!pad3) {
            if (out + 1u > cap) return 0u;
            dst[out++] = (uint8_t)((c << 6) | d);
        }
    }
    return out;
}

void modem_stream_init(modem_stream_t *stream) {
    memset(stream, 0, sizeof(*stream));
}

void modem_stream_feed(modem_stream_t *stream, const uint8_t *bytes, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        char c = (char)bytes[i];
        if (c == '\r' || c == '\n') {
            if (!stream->used) continue;
            if (stream->count == MODEM_LINE_QUEUE) {
                stream->dropped_lines++;
            } else {
                uint8_t at = (uint8_t)((stream->head + stream->count) % MODEM_LINE_QUEUE);
                memcpy(stream->lines[at], stream->current, stream->used);
                stream->lines[at][stream->used] = '\0';
                stream->lengths[at] = (uint16_t)stream->used;
                stream->count++;
            }
            stream->used = 0u;
        } else if (stream->used + 1u < MODEM_LINE_BYTES) {
            stream->current[stream->used++] = c;
        } else {
            stream->used = 0u;
            stream->dropped_lines++;
        }
    }
}

bool modem_stream_pop(modem_stream_t *stream, char line[MODEM_LINE_BYTES]) {
    if (!stream->count) return false;
    uint8_t at = stream->head;
    memcpy(line, stream->lines[at], stream->lengths[at] + 1u);
    stream->head = (uint8_t)((stream->head + 1u) % MODEM_LINE_QUEUE);
    stream->count--;
    return true;
}

static bool parse_uint_field(const char *start, size_t length, unsigned long max,
                             unsigned long *value) {
    if (!length || length >= 16u) return false;
    char text[16];
    memcpy(text, start, length);
    text[length] = '\0';
    for (size_t i = 0; i < length; ++i) if (text[i] < '0' || text[i] > '9') return false;
    char *end = NULL;
    unsigned long parsed = strtoul(text, &end, 10);
    if (!end || *end || parsed > max) return false;
    *value = parsed;
    return true;
}

static bool parse_signed_field(const char *start, size_t length, long min,
                               long max, long *value) {
    if (!length || length >= 16u) return false;
    char text[16];
    memcpy(text, start, length);
    text[length] = '\0';
    char *end = NULL;
    long parsed = strtol(text, &end, 10);
    if (!end || end == text || *end || parsed < min || parsed > max) return false;
    *value = parsed;
    return true;
}

static bool parse_snr_field(const char *start, size_t length, int16_t *value) {
    if (!length || length >= 20u) return false;
    char text[20];
    memcpy(text, start, length);
    text[length] = '\0';
    char *end = NULL;
    double parsed = strtod(text, &end);
    if (!end || end == text || *end || !isfinite(parsed) ||
        parsed < -327.68 || parsed > 327.67) return false;
    *value = (int16_t)(parsed * 100.0);
    return true;
}

static relay_result_t malformed_line(relay_t *relay) {
    if (relay) {
        relay->malformed_frames++;
        relay->radio_rx_errors++;
    }
    return RELAY_REJECT_MALFORMED;
}

relay_result_t modem_parse_rcv_line(relay_t *relay, const char *line,
                                    uint16_t expected_source_address,
                                    uint32_t now_ms) {
    if (!relay || !line || strncmp(line, "+RCV=", 5) != 0) return malformed_line(relay);
    const char *address = line + 5;
    const char *first = strchr(address, ',');
    const char *second = first ? strchr(first + 1, ',') : NULL;
    const char *last = strrchr(line, ',');
    const char *previous = last ? last - 1 : NULL;
    while (previous && previous > (second ? second + 1 : line) && *previous != ',') --previous;
    if (!first || !second || !last || previous <= second || last <= previous)
        return malformed_line(relay);
    unsigned long source_address, declared;
    if (!parse_uint_field(address, (size_t)(first - address), UINT16_MAX, &source_address) ||
        source_address != expected_source_address ||
        !parse_uint_field(first + 1, (size_t)(second - first - 1), 240u, &declared))
        return malformed_line(relay);
    const char *payload = second + 1;
    size_t payload_length = (size_t)(previous - payload);
    if (payload_length != declared) return malformed_line(relay);
    long rssi = 0;
    int16_t snr_cdb = INT16_MIN;
    bool metrics_valid = parse_signed_field(previous + 1,
        (size_t)(last - previous - 1), INT16_MIN + 1L, INT16_MAX, &rssi) &&
        parse_snr_field(last + 1, strlen(last + 1), &snr_cdb);
    uint8_t raw[180];
    size_t raw_length = base64_decode(payload, payload_length, raw, sizeof(raw));
    if (!raw_length) return malformed_line(relay);
    relay_result_t result = relay_radio_frame(relay, raw, raw_length, now_ms);
    if (result == RELAY_ACCEPTED) {
        relay->radio_metrics_valid = metrics_valid;
        if (metrics_valid) {
            relay->radio_rssi_dbm = (int16_t)rssi;
            relay->radio_snr_cdb = snr_cdb;
            relay->last_radio_metrics_ms = now_ms;
        }
    }
    return result;
}

bool modem_build_send_command(uint16_t destination, const uint8_t *frame,
                              size_t frame_length, char *command,
                              size_t command_capacity) {
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (!frame || !command || !frame_length || frame_length > RELAY_MAX_FRAME) return false;
    char encoded[241];
    size_t out = 0u;
    for (size_t i = 0; i < frame_length; i += 3u) {
        uint32_t value = (uint32_t)frame[i] << 16;
        bool have2 = i + 1u < frame_length;
        bool have3 = i + 2u < frame_length;
        if (have2) value |= (uint32_t)frame[i + 1u] << 8;
        if (have3) value |= frame[i + 2u];
        encoded[out++] = alphabet[(value >> 18) & 63u];
        encoded[out++] = alphabet[(value >> 12) & 63u];
        encoded[out++] = have2 ? alphabet[(value >> 6) & 63u] : '=';
        encoded[out++] = have3 ? alphabet[value & 63u] : '=';
    }
    encoded[out] = '\0';
    int result = snprintf(command, command_capacity, "AT+SEND=%u,%u,%s",
                          destination, (unsigned)out, encoded);
    return result > 0 && (size_t)result < command_capacity && out <= 240u;
}
