/** @file
 *    @brief MAVLink comm protocol testsuite generated from arsem_test.xml
 *    @see https://mavlink.io/en/
 */
#pragma once
#ifndef ARSEM_TEST_TESTSUITE_H
#define ARSEM_TEST_TESTSUITE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAVLINK_TEST_ALL
#define MAVLINK_TEST_ALL

static void mavlink_test_arsem_test(uint8_t, uint8_t, mavlink_message_t *last_msg);

static void mavlink_test_all(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{

    mavlink_test_arsem_test(system_id, component_id, last_msg);
}
#endif




static void mavlink_test_sail_motor_test(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_SAIL_MOTOR_TEST >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_sail_motor_test_t packet_in = {
        963497464,17443,151,218,29,96
    };
    mavlink_sail_motor_test_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.command_id = packet_in.command_id;
        packet1.duration_ms = packet_in.duration_ms;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.direction = packet_in.direction;
        packet1.pwm = packet_in.pwm;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_sail_motor_test_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_sail_motor_test_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_sail_motor_test_pack(system_id, component_id, &msg , packet1.command_id , packet1.duration_ms , packet1.target_system , packet1.target_component , packet1.direction , packet1.pwm );
    mavlink_msg_sail_motor_test_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_sail_motor_test_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.command_id , packet1.duration_ms , packet1.target_system , packet1.target_component , packet1.direction , packet1.pwm );
    mavlink_msg_sail_motor_test_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_sail_motor_test_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_sail_motor_test_send(MAVLINK_COMM_1 , packet1.command_id , packet1.duration_ms , packet1.target_system , packet1.target_component , packet1.direction , packet1.pwm );
    mavlink_msg_sail_motor_test_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("SAIL_MOTOR_TEST") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_SAIL_MOTOR_TEST) != NULL);
#endif
}

static void mavlink_test_sail_motor_test_status(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_sail_motor_test_status_t packet_in = {
        963497464,963497672,17651,17755,41,108,175,242
    };
    mavlink_sail_motor_test_status_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.command_id = packet_in.command_id;
        packet1.elapsed_ms = packet_in.elapsed_ms;
        packet1.remaining_ms = packet_in.remaining_ms;
        packet1.state = packet_in.state;
        packet1.direction = packet_in.direction;
        packet1.pwm = packet_in.pwm;
        packet1.result = packet_in.result;
        
        
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_sail_motor_test_status_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_sail_motor_test_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_sail_motor_test_status_pack(system_id, component_id, &msg , packet1.time_boot_ms , packet1.command_id , packet1.elapsed_ms , packet1.remaining_ms , packet1.state , packet1.direction , packet1.pwm , packet1.result );
    mavlink_msg_sail_motor_test_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_sail_motor_test_status_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_boot_ms , packet1.command_id , packet1.elapsed_ms , packet1.remaining_ms , packet1.state , packet1.direction , packet1.pwm , packet1.result );
    mavlink_msg_sail_motor_test_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_sail_motor_test_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);
        
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_sail_motor_test_status_send(MAVLINK_COMM_1 , packet1.time_boot_ms , packet1.command_id , packet1.elapsed_ms , packet1.remaining_ms , packet1.state , packet1.direction , packet1.pwm , packet1.result );
    mavlink_msg_sail_motor_test_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("SAIL_MOTOR_TEST_STATUS") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS) != NULL);
#endif
}

static void mavlink_test_arsem_test(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
    mavlink_test_sail_motor_test(system_id, component_id, last_msg);
    mavlink_test_sail_motor_test_status(system_id, component_id, last_msg);
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // ARSEM_TEST_TESTSUITE_H
