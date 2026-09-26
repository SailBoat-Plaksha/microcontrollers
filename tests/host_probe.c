#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "mavlink_min.h"
#include "actuator_command.h"

int main(void) {
    uint8_t payload[14] = {0};
    mav_put_u32(payload, 42);
    mav_put_u16(payload + 4, 9000);
    mav_put_u16(payload + 6, 1000);
    payload[10] = 1; payload[11] = 154; payload[12] = 1; payload[13] = 2;
    uint8_t frame[MAV_FRAME_MAX];
    size_t length = mav_pack(MAV_ACTUATOR_SETPOINT, payload, sizeof(payload), 1, 1, 0, frame);
    assert(length == 26 && mav_valid(frame, length));
    for (size_t i = 0; i < length; ++i) printf("%02x", frame[i]);
    putchar('\n');

    mav_parser_t parser = {0};
    uint8_t recovered[MAV_FRAME_MAX];
    size_t received = 0;
    for (size_t i = 0; i < length; ++i) {
        bool complete = mav_feed(&parser, frame[i], recovered, &received);
        assert(complete == (i + 1 == length));
    }
    assert(received == length && memcmp(frame, recovered, length) == 0);

    char encoded[241];
    size_t text_length = mav_base64_encode(frame, length, encoded, sizeof(encoded));
    uint8_t decoded[MAV_FRAME_MAX];
    assert(text_length && mav_base64_decode(encoded, text_length, decoded, sizeof(decoded)) == length);
    assert(memcmp(frame, decoded, length) == 0);
    uint8_t damaged[MAV_FRAME_MAX];
    memcpy(damaged, frame, length);
    damaged[13] ^= 0x20;
    assert(!mav_valid(damaged, length));
    assert(mav_base64_decode("not-base64!", 11, decoded, sizeof(decoded)) == 0);

    actuator_guard_t guard = {0};
    assert(actuator_check(&guard, payload, 14, false, 1, 154) == 0);
    assert(actuator_check(&guard, payload, 14, false, 1, 154) == 4);
    mav_put_u32(payload, 43);
    assert(actuator_check(&guard, payload, 14, true, 1, 154) == 2);
    mav_put_u16(payload + 4, 19000);
    assert(actuator_check(&guard, payload, 14, false, 1, 154) == 3);
    mav_put_u16(payload + 4, 9000);
    payload[11] = 155;
    assert(actuator_check(&guard, payload, 14, false, 1, 154) == 3);
    puts("C frame, stream, Base64 and actuator guard: PASS");
    return 0;
}
