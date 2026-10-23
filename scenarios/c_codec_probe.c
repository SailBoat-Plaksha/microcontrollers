#include <stdio.h>
#include "mavlink.h"

int main(void) {
    mavlink_message_t message;
    uint8_t bytes[MAVLINK_MAX_PACKET_LEN];
    uint16_t length = mavlink_msg_actuator_request_pack(
        255, 190, &message,
        0x10203040u, 0x50607080u, -500, 750, 1200,
        1, 154, 2, 2, 3);
    length = mavlink_msg_to_send_buffer(bytes, &message);
    for (uint16_t i = 0; i < length; ++i) {
        printf("%02x", bytes[i]);
    }
    putchar('\n');
    return 0;
}
