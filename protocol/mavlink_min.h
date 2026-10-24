#ifndef ARSEM_MAVLINK_MIN_H
#define ARSEM_MAVLINK_MIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAV_FRAME_MAX 280
#define MAV_ACTUATOR_SETPOINT 200001u
#define MAV_ACTUATOR_STATUS 200002u
#define MAV_NODE_INFO 200000u
#define MAV_ACTUATOR_REQUEST 200008u
#define MAV_NODE_HEALTH 200009u
#define MAV_REQUEST_RESULT 200010u
#define MAV_IMU_REQUEST 200011u
#define MAV_IMU_SAMPLE 200012u
#define MAV_HEARTBEAT 0u
#define MAV_PING 4u
#define MAV_SCALED_IMU 26u

typedef struct {
    uint8_t bytes[MAV_FRAME_MAX];
    size_t used;
} mav_parser_t;

uint16_t mav_crc(const uint8_t *data, size_t length, uint16_t initial);
bool mav_valid(const uint8_t *frame, size_t length);
bool mav_feed(mav_parser_t *parser, uint8_t byte, uint8_t *frame, size_t *length);
size_t mav_pack(uint32_t message_id, const uint8_t *payload, size_t payload_length,
                uint8_t system, uint8_t component, uint8_t sequence, uint8_t *out);
uint16_t mav_u16(const uint8_t *bytes);
uint32_t mav_u32(const uint8_t *bytes);
void mav_put_u16(uint8_t *bytes, uint16_t value);
void mav_put_u32(uint8_t *bytes, uint32_t value);
size_t mav_base64_encode(const uint8_t *input, size_t length, char *output, size_t capacity);
size_t mav_base64_decode(const char *input, size_t length, uint8_t *output, size_t capacity);

#endif
