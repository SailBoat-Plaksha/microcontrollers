#include "mavlink_min.h"
#include <string.h>

/* CRC_EXTRA values generated from protocol/arsem.xml by protocol/codec.py. */
static int extra_for(uint32_t id) {
    static const uint8_t custom[] = {139, 49, 131, 143, 158, 201, 198, 121};
    if (id >= 200000u && id <= 200007u) return custom[id - 200000u];
    if (id == 0u) return 50; /* common.xml HEARTBEAT */
    return -1;
}

uint16_t mav_crc(const uint8_t *data, size_t length, uint16_t initial) {
    uint16_t crc = initial;
    for (size_t i = 0; i < length; ++i) {
        uint8_t tmp = data[i] ^ (uint8_t)crc;
        tmp ^= (uint8_t)(tmp << 4);
        crc = (uint16_t)((crc >> 8) ^ ((uint16_t)tmp << 8) ^
                         ((uint16_t)tmp << 3) ^ (tmp >> 4));
    }
    return crc;
}

uint16_t mav_u16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
uint32_t mav_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
void mav_put_u16(uint8_t *p, uint16_t n) { p[0] = (uint8_t)n; p[1] = (uint8_t)(n >> 8); }
void mav_put_u32(uint8_t *p, uint32_t n) {
    p[0] = (uint8_t)n; p[1] = (uint8_t)(n >> 8); p[2] = (uint8_t)(n >> 16); p[3] = (uint8_t)(n >> 24);
}

bool mav_valid(const uint8_t *f, size_t size) {
    if (!f || size < 13 || f[0] != 0xfd || (f[2] & ~1u) != 0) return false;
    if (size != (size_t)(12u + f[1] + ((f[2] & 1u) ? 13u : 0u))) return false;
    uint32_t id = (uint32_t)f[7] | ((uint32_t)f[8] << 8) | ((uint32_t)f[9] << 16);
    int extra = extra_for(id);
    if (extra < 0 || (f[2] & 1u)) return false; /* never accept unverified signatures */
    uint16_t calculated = mav_crc(f + 1, 9u + f[1], 0xffffu);
    uint8_t seed = (uint8_t)extra;
    calculated = mav_crc(&seed, 1, calculated);
    return calculated == mav_u16(f + 10u + f[1]);
}

bool mav_feed(mav_parser_t *p, uint8_t byte, uint8_t *frame, size_t *length) {
    if (!p || !frame || !length) return false;
    if (p->used == 0 && byte != 0xfd) return false;
    if (p->used >= MAV_FRAME_MAX) p->used = 0;
    p->bytes[p->used++] = byte;
    if (p->used < 10) return false;
    size_t expected = 12u + p->bytes[1] + ((p->bytes[2] & 1u) ? 13u : 0u);
    if (expected > MAV_FRAME_MAX || (p->bytes[2] & ~1u)) { p->used = 0; return false; }
    if (p->used < expected) return false;
    bool valid = mav_valid(p->bytes, expected);
    if (valid) { memcpy(frame, p->bytes, expected); *length = expected; }
    p->used = 0;
    return valid;
}

size_t mav_pack(uint32_t id, const uint8_t *payload, size_t length,
                uint8_t system, uint8_t component, uint8_t sequence, uint8_t *out) {
    int extra = extra_for(id);
    if (!out || !payload || length == 0 || length > 255 || extra < 0) return 0;
    while (length > 1 && payload[length - 1] == 0) --length;
    out[0] = 0xfd; out[1] = (uint8_t)length; out[2] = 0; out[3] = 0;
    out[4] = sequence; out[5] = system; out[6] = component;
    out[7] = (uint8_t)id; out[8] = (uint8_t)(id >> 8); out[9] = (uint8_t)(id >> 16);
    memcpy(out + 10, payload, length);
    uint16_t check = mav_crc(out + 1, 9u + length, 0xffffu);
    uint8_t seed = (uint8_t)extra;
    check = mav_crc(&seed, 1, check);
    mav_put_u16(out + 10 + length, check);
    return 12u + length;
}

static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
size_t mav_base64_encode(const uint8_t *input, size_t length, char *out, size_t cap) {
    size_t needed = 4u * ((length + 2u) / 3u);
    if (!input || !out || cap <= needed) return 0;
    for (size_t i = 0, j = 0; i < length; i += 3) {
        uint32_t value = (uint32_t)input[i] << 16;
        if (i + 1 < length) value |= (uint32_t)input[i + 1] << 8;
        if (i + 2 < length) value |= input[i + 2];
        out[j++] = alphabet[(value >> 18) & 63]; out[j++] = alphabet[(value >> 12) & 63];
        out[j++] = i + 1 < length ? alphabet[(value >> 6) & 63] : '=';
        out[j++] = i + 2 < length ? alphabet[value & 63] : '=';
    }
    out[needed] = 0;
    return needed;
}

static int digit(char c) {
    for (int i = 0; i < 64; ++i) if (alphabet[i] == c) return i;
    return -1;
}
size_t mav_base64_decode(const char *input, size_t length, uint8_t *out, size_t cap) {
    if (!input || !out || length == 0 || length % 4 != 0) return 0;
    size_t count = 0;
    for (size_t i = 0; i < length; i += 4) {
        int a = digit(input[i]), b = digit(input[i + 1]);
        bool last = i + 4 == length;
        int c = input[i + 2] == '=' ? -2 : digit(input[i + 2]);
        int d = input[i + 3] == '=' ? -2 : digit(input[i + 3]);
        if (a < 0 || b < 0 || c == -1 || d == -1 || (c == -2 && d != -2) ||
            (!last && (c == -2 || d == -2)) || count + 3 > cap) return 0;
        uint32_t value = ((uint32_t)a << 18) | ((uint32_t)b << 12) |
                         ((uint32_t)(c < 0 ? 0 : c) << 6) | (uint32_t)(d < 0 ? 0 : d);
        out[count++] = (uint8_t)(value >> 16);
        if (c >= 0) out[count++] = (uint8_t)(value >> 8);
        if (d >= 0) out[count++] = (uint8_t)value;
    }
    return count;
}
