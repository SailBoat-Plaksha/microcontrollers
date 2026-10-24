/** @file
 *    @brief MAVLink comm protocol testsuite generated from arsem_standalone.xml
 *    @see https://mavlink.io/en/
 */
#pragma once
#ifndef ARSEM_STANDALONE_TESTSUITE_H
#define ARSEM_STANDALONE_TESTSUITE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAVLINK_TEST_ALL
#define MAVLINK_TEST_ALL

static void mavlink_test_arsem_standalone(uint8_t, uint8_t, mavlink_message_t *last_msg);

static void mavlink_test_all(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{

    mavlink_test_arsem_standalone(system_id, component_id, last_msg);
}
#endif




static void mavlink_test_node_info(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_NODE_INFO >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_node_info_t packet_in = {
        963497464,963497672,29,96,163,230,41,108,175
    };
    mavlink_node_info_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.capabilities = packet_in.capabilities;
        packet1.build_id = packet_in.build_id;
        packet1.schema_major = packet_in.schema_major;
        packet1.schema_minor = packet_in.schema_minor;
        packet1.schema_patch = packet_in.schema_patch;
        packet1.firmware_major = packet_in.firmware_major;
        packet1.firmware_minor = packet_in.firmware_minor;
        packet1.firmware_patch = packet_in.firmware_patch;
        packet1.role = packet_in.role;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_NODE_INFO_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_NODE_INFO_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_node_info_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_node_info_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_node_info_pack(system_id, component_id, &msg , packet1.capabilities , packet1.build_id , packet1.schema_major , packet1.schema_minor , packet1.schema_patch , packet1.firmware_major , packet1.firmware_minor , packet1.firmware_patch , packet1.role );
    mavlink_msg_node_info_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_node_info_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.capabilities , packet1.build_id , packet1.schema_major , packet1.schema_minor , packet1.schema_patch , packet1.firmware_major , packet1.firmware_minor , packet1.firmware_patch , packet1.role );
    mavlink_msg_node_info_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_node_info_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_node_info_send(MAVLINK_COMM_1 , packet1.capabilities , packet1.build_id , packet1.schema_major , packet1.schema_minor , packet1.schema_patch , packet1.firmware_major , packet1.firmware_minor , packet1.firmware_patch , packet1.role );
    mavlink_msg_node_info_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("NODE_INFO") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_NODE_INFO) != NULL);
#endif
}

static void mavlink_test_actuator_setpoint(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_ACTUATOR_SETPOINT >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_actuator_setpoint_t packet_in = {
        963497464,17443,17547,17651,163,230,41,108
    };
    mavlink_actuator_setpoint_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.command_id = packet_in.command_id;
        packet1.position_cdeg = packet_in.position_cdeg;
        packet1.timeout_ms = packet_in.timeout_ms;
        packet1.max_rate_cdeg_s = packet_in.max_rate_cdeg_s;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.actuator = packet_in.actuator;
        packet1.control_source = packet_in.control_source;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_setpoint_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_actuator_setpoint_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_setpoint_pack(system_id, component_id, &msg , packet1.command_id , packet1.position_cdeg , packet1.timeout_ms , packet1.max_rate_cdeg_s , packet1.target_system , packet1.target_component , packet1.actuator , packet1.control_source );
    mavlink_msg_actuator_setpoint_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_setpoint_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.command_id , packet1.position_cdeg , packet1.timeout_ms , packet1.max_rate_cdeg_s , packet1.target_system , packet1.target_component , packet1.actuator , packet1.control_source );
    mavlink_msg_actuator_setpoint_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_actuator_setpoint_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_setpoint_send(MAVLINK_COMM_1 , packet1.command_id , packet1.position_cdeg , packet1.timeout_ms , packet1.max_rate_cdeg_s , packet1.target_system , packet1.target_component , packet1.actuator , packet1.control_source );
    mavlink_msg_actuator_setpoint_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("ACTUATOR_SETPOINT") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_ACTUATOR_SETPOINT) != NULL);
#endif
}

static void mavlink_test_actuator_status(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_ACTUATOR_STATUS >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_actuator_status_t packet_in = {
        963497464,963497672,17651,17755,17859,17963,53,120,187,254
    };
    mavlink_actuator_status_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.command_id = packet_in.command_id;
        packet1.requested_position_cdeg = packet_in.requested_position_cdeg;
        packet1.measured_position_cdeg = packet_in.measured_position_cdeg;
        packet1.velocity_cdeg_s = packet_in.velocity_cdeg_s;
        packet1.fault_flags = packet_in.fault_flags;
        packet1.actuator = packet_in.actuator;
        packet1.control_source = packet_in.control_source;
        packet1.result = packet_in.result;
        packet1.enabled = packet_in.enabled;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_status_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_actuator_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_status_pack(system_id, component_id, &msg , packet1.time_boot_ms , packet1.command_id , packet1.requested_position_cdeg , packet1.measured_position_cdeg , packet1.velocity_cdeg_s , packet1.fault_flags , packet1.actuator , packet1.control_source , packet1.result , packet1.enabled );
    mavlink_msg_actuator_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_status_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_boot_ms , packet1.command_id , packet1.requested_position_cdeg , packet1.measured_position_cdeg , packet1.velocity_cdeg_s , packet1.fault_flags , packet1.actuator , packet1.control_source , packet1.result , packet1.enabled );
    mavlink_msg_actuator_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_actuator_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_status_send(MAVLINK_COMM_1 , packet1.time_boot_ms , packet1.command_id , packet1.requested_position_cdeg , packet1.measured_position_cdeg , packet1.velocity_cdeg_s , packet1.fault_flags , packet1.actuator , packet1.control_source , packet1.result , packet1.enabled );
    mavlink_msg_actuator_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("ACTUATOR_STATUS") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_ACTUATOR_STATUS) != NULL);
#endif
}

static void mavlink_test_control_authority(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_CONTROL_AUTHORITY >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_control_authority_t packet_in = {
        963497464,17443,151,218,29,96
    };
    mavlink_control_authority_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.authority_timeout_ms = packet_in.authority_timeout_ms;
        packet1.control_source = packet_in.control_source;
        packet1.rc_available = packet_in.rc_available;
        packet1.autonomy_available = packet_in.autonomy_available;
        packet1.failsafe_active = packet_in.failsafe_active;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_authority_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_control_authority_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_authority_pack(system_id, component_id, &msg , packet1.time_boot_ms , packet1.authority_timeout_ms , packet1.control_source , packet1.rc_available , packet1.autonomy_available , packet1.failsafe_active );
    mavlink_msg_control_authority_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_authority_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_boot_ms , packet1.authority_timeout_ms , packet1.control_source , packet1.rc_available , packet1.autonomy_available , packet1.failsafe_active );
    mavlink_msg_control_authority_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_control_authority_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_control_authority_send(MAVLINK_COMM_1 , packet1.time_boot_ms , packet1.authority_timeout_ms , packet1.control_source , packet1.rc_available , packet1.autonomy_available , packet1.failsafe_active );
    mavlink_msg_control_authority_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("CONTROL_AUTHORITY") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_CONTROL_AUTHORITY) != NULL);
#endif
}

static void mavlink_test_environmental_sample(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_environmental_sample_t packet_in = {
        93372036854775807ULL,17651,17755,17859,17963,18067,18171,65
    };
    mavlink_environmental_sample_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_usec = packet_in.time_usec;
        packet1.ph_milli = packet_in.ph_milli;
        packet1.water_temperature_cdeg = packet_in.water_temperature_cdeg;
        packet1.dissolved_oxygen_mg_l_x100 = packet_in.dissolved_oxygen_mg_l_x100;
        packet1.salinity_ppt_x100 = packet_in.salinity_ppt_x100;
        packet1.turbidity_ntu_x10 = packet_in.turbidity_ntu_x10;
        packet1.valid_fields = packet_in.valid_fields;
        packet1.sensor_instance = packet_in.sensor_instance;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_environmental_sample_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_environmental_sample_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_environmental_sample_pack(system_id, component_id, &msg , packet1.time_usec , packet1.ph_milli , packet1.water_temperature_cdeg , packet1.dissolved_oxygen_mg_l_x100 , packet1.salinity_ppt_x100 , packet1.turbidity_ntu_x10 , packet1.valid_fields , packet1.sensor_instance );
    mavlink_msg_environmental_sample_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_environmental_sample_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_usec , packet1.ph_milli , packet1.water_temperature_cdeg , packet1.dissolved_oxygen_mg_l_x100 , packet1.salinity_ppt_x100 , packet1.turbidity_ntu_x10 , packet1.valid_fields , packet1.sensor_instance );
    mavlink_msg_environmental_sample_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_environmental_sample_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_environmental_sample_send(MAVLINK_COMM_1 , packet1.time_usec , packet1.ph_milli , packet1.water_temperature_cdeg , packet1.dissolved_oxygen_mg_l_x100 , packet1.salinity_ppt_x100 , packet1.turbidity_ntu_x10 , packet1.valid_fields , packet1.sensor_instance );
    mavlink_msg_environmental_sample_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("ENVIRONMENTAL_SAMPLE") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE) != NULL);
#endif
}

static void mavlink_test_power_state(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_POWER_STATE >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_power_state_t packet_in = {
        963497464,17443,17547,17651,17755,41,108
    };
    mavlink_power_state_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.current_ma = packet_in.current_ma;
        packet1.voltage_mv = packet_in.voltage_mv;
        packet1.state_of_charge_cpercent = packet_in.state_of_charge_cpercent;
        packet1.fault_flags = packet_in.fault_flags;
        packet1.mode = packet_in.mode;
        packet1.external_power = packet_in.external_power;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_POWER_STATE_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_POWER_STATE_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_power_state_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_power_state_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_power_state_pack(system_id, component_id, &msg , packet1.time_boot_ms , packet1.current_ma , packet1.voltage_mv , packet1.state_of_charge_cpercent , packet1.fault_flags , packet1.mode , packet1.external_power );
    mavlink_msg_power_state_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_power_state_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_boot_ms , packet1.current_ma , packet1.voltage_mv , packet1.state_of_charge_cpercent , packet1.fault_flags , packet1.mode , packet1.external_power );
    mavlink_msg_power_state_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_power_state_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_power_state_send(MAVLINK_COMM_1 , packet1.time_boot_ms , packet1.current_ma , packet1.voltage_mv , packet1.state_of_charge_cpercent , packet1.fault_flags , packet1.mode , packet1.external_power );
    mavlink_msg_power_state_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("POWER_STATE") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_POWER_STATE) != NULL);
#endif
}

static void mavlink_test_link_status(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_LINK_STATUS >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_link_status_t packet_in = {
        963497464,963497672,963497880,17859,17963,18067,18171,18275,199,10
    };
    mavlink_link_status_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.rx_packets = packet_in.rx_packets;
        packet1.tx_packets = packet_in.tx_packets;
        packet1.rx_errors = packet_in.rx_errors;
        packet1.tx_errors = packet_in.tx_errors;
        packet1.address = packet_in.address;
        packet1.rssi_dbm = packet_in.rssi_dbm;
        packet1.snr_cdb = packet_in.snr_cdb;
        packet1.link_type = packet_in.link_type;
        packet1.connected = packet_in.connected;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_link_status_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_link_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_link_status_pack(system_id, component_id, &msg , packet1.time_boot_ms , packet1.rx_packets , packet1.tx_packets , packet1.rx_errors , packet1.tx_errors , packet1.address , packet1.rssi_dbm , packet1.snr_cdb , packet1.link_type , packet1.connected );
    mavlink_msg_link_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_link_status_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_boot_ms , packet1.rx_packets , packet1.tx_packets , packet1.rx_errors , packet1.tx_errors , packet1.address , packet1.rssi_dbm , packet1.snr_cdb , packet1.link_type , packet1.connected );
    mavlink_msg_link_status_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_link_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_link_status_send(MAVLINK_COMM_1 , packet1.time_boot_ms , packet1.rx_packets , packet1.tx_packets , packet1.rx_errors , packet1.tx_errors , packet1.address , packet1.rssi_dbm , packet1.snr_cdb , packet1.link_type , packet1.connected );
    mavlink_msg_link_status_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("LINK_STATUS") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_LINK_STATUS) != NULL);
#endif
}

static void mavlink_test_bus_grant(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_BUS_GRANT >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_bus_grant_t packet_in = {
        963497464,17443,17547,29,96,163
    };
    mavlink_bus_grant_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.duration_us = packet_in.duration_us;
        packet1.max_bytes = packet_in.max_bytes;
        packet1.stream_flags = packet_in.stream_flags;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.max_frames = packet_in.max_frames;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_bus_grant_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_bus_grant_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_bus_grant_pack(system_id, component_id, &msg , packet1.duration_us , packet1.max_bytes , packet1.stream_flags , packet1.target_system , packet1.target_component , packet1.max_frames );
    mavlink_msg_bus_grant_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_bus_grant_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.duration_us , packet1.max_bytes , packet1.stream_flags , packet1.target_system , packet1.target_component , packet1.max_frames );
    mavlink_msg_bus_grant_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_bus_grant_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_bus_grant_send(MAVLINK_COMM_1 , packet1.duration_us , packet1.max_bytes , packet1.stream_flags , packet1.target_system , packet1.target_component , packet1.max_frames );
    mavlink_msg_bus_grant_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("BUS_GRANT") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_BUS_GRANT) != NULL);
#endif
}

static void mavlink_test_actuator_request(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_ACTUATOR_REQUEST >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_actuator_request_t packet_in = {
        963497464,963497672,17651,17755,17859,175,242,53,120,187
    };
    mavlink_actuator_request_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.request_id = packet_in.request_id;
        packet1.ground_session = packet_in.ground_session;
        packet1.delta_cdeg = packet_in.delta_cdeg;
        packet1.timeout_ms = packet_in.timeout_ms;
        packet1.max_rate_cdeg_s = packet_in.max_rate_cdeg_s;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;
        packet1.actuator = packet_in.actuator;
        packet1.opcode = packet_in.opcode;
        packet1.control_source = packet_in.control_source;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_request_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_actuator_request_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_request_pack(system_id, component_id, &msg , packet1.request_id , packet1.ground_session , packet1.delta_cdeg , packet1.timeout_ms , packet1.max_rate_cdeg_s , packet1.target_system , packet1.target_component , packet1.actuator , packet1.opcode , packet1.control_source );
    mavlink_msg_actuator_request_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_request_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.request_id , packet1.ground_session , packet1.delta_cdeg , packet1.timeout_ms , packet1.max_rate_cdeg_s , packet1.target_system , packet1.target_component , packet1.actuator , packet1.opcode , packet1.control_source );
    mavlink_msg_actuator_request_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_actuator_request_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_actuator_request_send(MAVLINK_COMM_1 , packet1.request_id , packet1.ground_session , packet1.delta_cdeg , packet1.timeout_ms , packet1.max_rate_cdeg_s , packet1.target_system , packet1.target_component , packet1.actuator , packet1.opcode , packet1.control_source );
    mavlink_msg_actuator_request_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("ACTUATOR_REQUEST") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_ACTUATOR_REQUEST) != NULL);
#endif
}

static void mavlink_test_node_health(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_NODE_HEALTH >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_node_health_t packet_in = {
        963497464,963497672,17651,17755,17859,175,242
    };
    mavlink_node_health_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.boot_id = packet_in.boot_id;
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.fault_flags = packet_in.fault_flags;
        packet1.sensor_valid_fields = packet_in.sensor_valid_fields;
        packet1.queue_drops = packet_in.queue_drops;
        packet1.reset_reason = packet_in.reset_reason;
        packet1.feedback_valid = packet_in.feedback_valid;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_node_health_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_node_health_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_node_health_pack(system_id, component_id, &msg , packet1.boot_id , packet1.time_boot_ms , packet1.fault_flags , packet1.sensor_valid_fields , packet1.queue_drops , packet1.reset_reason , packet1.feedback_valid );
    mavlink_msg_node_health_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_node_health_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.boot_id , packet1.time_boot_ms , packet1.fault_flags , packet1.sensor_valid_fields , packet1.queue_drops , packet1.reset_reason , packet1.feedback_valid );
    mavlink_msg_node_health_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_node_health_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_node_health_send(MAVLINK_COMM_1 , packet1.boot_id , packet1.time_boot_ms , packet1.fault_flags , packet1.sensor_valid_fields , packet1.queue_drops , packet1.reset_reason , packet1.feedback_valid );
    mavlink_msg_node_health_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("NODE_HEALTH") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_NODE_HEALTH) != NULL);
#endif
}

static void mavlink_test_request_result(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_REQUEST_RESULT >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_request_result_t packet_in = {
        963497464,963497672,963497880,963498088,53,120,187,254
    };
    mavlink_request_result_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.ground_session = packet_in.ground_session;
        packet1.request_id = packet_in.request_id;
        packet1.motor_boot_id = packet_in.motor_boot_id;
        packet1.local_command_id = packet_in.local_command_id;
        packet1.result = packet_in.result;
        packet1.state = packet_in.state;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_request_result_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_request_result_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_request_result_pack(system_id, component_id, &msg , packet1.ground_session , packet1.request_id , packet1.motor_boot_id , packet1.local_command_id , packet1.result , packet1.state , packet1.target_system , packet1.target_component );
    mavlink_msg_request_result_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_request_result_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.ground_session , packet1.request_id , packet1.motor_boot_id , packet1.local_command_id , packet1.result , packet1.state , packet1.target_system , packet1.target_component );
    mavlink_msg_request_result_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_request_result_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_request_result_send(MAVLINK_COMM_1 , packet1.ground_session , packet1.request_id , packet1.motor_boot_id , packet1.local_command_id , packet1.result , packet1.state , packet1.target_system , packet1.target_component );
    mavlink_msg_request_result_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("REQUEST_RESULT") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_REQUEST_RESULT) != NULL);
#endif
}

static void mavlink_test_imu_request(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_IMU_REQUEST >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_imu_request_t packet_in = {
        963497464,963497672,17651,17755,41,108,175
    };
    mavlink_imu_request_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.ground_session = packet_in.ground_session;
        packet1.request_id = packet_in.request_id;
        packet1.timeout_ms = packet_in.timeout_ms;
        packet1.max_age_ms = packet_in.max_age_ms;
        packet1.path = packet_in.path;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_imu_request_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_imu_request_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_imu_request_pack(system_id, component_id, &msg , packet1.ground_session , packet1.request_id , packet1.timeout_ms , packet1.max_age_ms , packet1.path , packet1.target_system , packet1.target_component );
    mavlink_msg_imu_request_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_imu_request_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.ground_session , packet1.request_id , packet1.timeout_ms , packet1.max_age_ms , packet1.path , packet1.target_system , packet1.target_component );
    mavlink_msg_imu_request_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_imu_request_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_imu_request_send(MAVLINK_COMM_1 , packet1.ground_session , packet1.request_id , packet1.timeout_ms , packet1.max_age_ms , packet1.path , packet1.target_system , packet1.target_component );
    mavlink_msg_imu_request_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("IMU_REQUEST") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_IMU_REQUEST) != NULL);
#endif
}

static void mavlink_test_imu_sample(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_IMU_SAMPLE >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_imu_sample_t packet_in = {
        963497464,963497672,963497880,963498088,963498296,963498504,18483,18587,18691,18795,18899,19003,19107,19211,19315,19419,19523,15,82,149,216,27
    };
    mavlink_imu_sample_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.ground_session = packet_in.ground_session;
        packet1.request_id = packet_in.request_id;
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.sample_seq = packet_in.sample_seq;
        packet1.acquisition_boot_id = packet_in.acquisition_boot_id;
        packet1.source_boot_id = packet_in.source_boot_id;
        packet1.sample_age_ms = packet_in.sample_age_ms;
        packet1.valid_fields = packet_in.valid_fields;
        packet1.xacc = packet_in.xacc;
        packet1.yacc = packet_in.yacc;
        packet1.zacc = packet_in.zacc;
        packet1.xgyro = packet_in.xgyro;
        packet1.ygyro = packet_in.ygyro;
        packet1.zgyro = packet_in.zgyro;
        packet1.xmag = packet_in.xmag;
        packet1.ymag = packet_in.ymag;
        packet1.zmag = packet_in.zmag;
        packet1.status = packet_in.status;
        packet1.path = packet_in.path;
        packet1.acquisition_component = packet_in.acquisition_component;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_imu_sample_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_imu_sample_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_imu_sample_pack(system_id, component_id, &msg , packet1.ground_session , packet1.request_id , packet1.time_boot_ms , packet1.sample_seq , packet1.acquisition_boot_id , packet1.source_boot_id , packet1.sample_age_ms , packet1.valid_fields , packet1.xacc , packet1.yacc , packet1.zacc , packet1.xgyro , packet1.ygyro , packet1.zgyro , packet1.xmag , packet1.ymag , packet1.zmag , packet1.status , packet1.path , packet1.acquisition_component , packet1.target_system , packet1.target_component );
    mavlink_msg_imu_sample_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_imu_sample_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.ground_session , packet1.request_id , packet1.time_boot_ms , packet1.sample_seq , packet1.acquisition_boot_id , packet1.source_boot_id , packet1.sample_age_ms , packet1.valid_fields , packet1.xacc , packet1.yacc , packet1.zacc , packet1.xgyro , packet1.ygyro , packet1.zgyro , packet1.xmag , packet1.ymag , packet1.zmag , packet1.status , packet1.path , packet1.acquisition_component , packet1.target_system , packet1.target_component );
    mavlink_msg_imu_sample_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_imu_sample_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_imu_sample_send(MAVLINK_COMM_1 , packet1.ground_session , packet1.request_id , packet1.time_boot_ms , packet1.sample_seq , packet1.acquisition_boot_id , packet1.source_boot_id , packet1.sample_age_ms , packet1.valid_fields , packet1.xacc , packet1.yacc , packet1.zacc , packet1.xgyro , packet1.ygyro , packet1.zgyro , packet1.xmag , packet1.ymag , packet1.zmag , packet1.status , packet1.path , packet1.acquisition_component , packet1.target_system , packet1.target_component );
    mavlink_msg_imu_sample_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("IMU_SAMPLE") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_IMU_SAMPLE) != NULL);
#endif
}

static void mavlink_test_heartbeat(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_HEARTBEAT >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_heartbeat_t packet_in = {
        963497464,17,84,151,218,3
    };
    mavlink_heartbeat_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.custom_mode = packet_in.custom_mode;
        packet1.type = packet_in.type;
        packet1.autopilot = packet_in.autopilot;
        packet1.base_mode = packet_in.base_mode;
        packet1.system_status = packet_in.system_status;
        packet1.mavlink_version = packet_in.mavlink_version;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_HEARTBEAT_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_HEARTBEAT_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_heartbeat_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_heartbeat_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_heartbeat_pack(system_id, component_id, &msg , packet1.type , packet1.autopilot , packet1.base_mode , packet1.custom_mode , packet1.system_status );
    mavlink_msg_heartbeat_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_heartbeat_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.type , packet1.autopilot , packet1.base_mode , packet1.custom_mode , packet1.system_status );
    mavlink_msg_heartbeat_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_heartbeat_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_heartbeat_send(MAVLINK_COMM_1 , packet1.type , packet1.autopilot , packet1.base_mode , packet1.custom_mode , packet1.system_status );
    mavlink_msg_heartbeat_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("HEARTBEAT") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_HEARTBEAT) != NULL);
#endif
}

static void mavlink_test_ping(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_PING >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_ping_t packet_in = {
        93372036854775807ULL,963497880,41,108
    };
    mavlink_ping_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_usec = packet_in.time_usec;
        packet1.seq = packet_in.seq;
        packet1.target_system = packet_in.target_system;
        packet1.target_component = packet_in.target_component;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_PING_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_PING_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_ping_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_ping_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_ping_pack(system_id, component_id, &msg , packet1.time_usec , packet1.seq , packet1.target_system , packet1.target_component );
    mavlink_msg_ping_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_ping_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_usec , packet1.seq , packet1.target_system , packet1.target_component );
    mavlink_msg_ping_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_ping_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_ping_send(MAVLINK_COMM_1 , packet1.time_usec , packet1.seq , packet1.target_system , packet1.target_component );
    mavlink_msg_ping_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("PING") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_PING) != NULL);
#endif
}

static void mavlink_test_scaled_imu(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
        if ((status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) && MAVLINK_MSG_ID_SCALED_IMU >= 256) {
            return;
        }
#endif
    mavlink_message_t msg;
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t i;
    mavlink_scaled_imu_t packet_in = {
        963497464,17443,17547,17651,17755,17859,17963,18067,18171,18275,18379
    };
    mavlink_scaled_imu_t packet1, packet2;
        memset(&packet1, 0, sizeof(packet1));
        packet1.time_boot_ms = packet_in.time_boot_ms;
        packet1.xacc = packet_in.xacc;
        packet1.yacc = packet_in.yacc;
        packet1.zacc = packet_in.zacc;
        packet1.xgyro = packet_in.xgyro;
        packet1.ygyro = packet_in.ygyro;
        packet1.zgyro = packet_in.zgyro;
        packet1.xmag = packet_in.xmag;
        packet1.ymag = packet_in.ymag;
        packet1.zmag = packet_in.zmag;
        packet1.temperature = packet_in.temperature;


#ifdef MAVLINK_STATUS_FLAG_OUT_MAVLINK1
        if (status->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1) {
           // cope with extensions
           memset(MAVLINK_MSG_ID_SCALED_IMU_MIN_LEN + (char *)&packet1, 0, sizeof(packet1)-MAVLINK_MSG_ID_SCALED_IMU_MIN_LEN);
        }
#endif
        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_scaled_imu_encode(system_id, component_id, &msg, &packet1);
    mavlink_msg_scaled_imu_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_scaled_imu_pack(system_id, component_id, &msg , packet1.time_boot_ms , packet1.xacc , packet1.yacc , packet1.zacc , packet1.xgyro , packet1.ygyro , packet1.zgyro , packet1.xmag , packet1.ymag , packet1.zmag , packet1.temperature );
    mavlink_msg_scaled_imu_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_scaled_imu_pack_chan(system_id, component_id, MAVLINK_COMM_0, &msg , packet1.time_boot_ms , packet1.xacc , packet1.yacc , packet1.zacc , packet1.xgyro , packet1.ygyro , packet1.zgyro , packet1.xmag , packet1.ymag , packet1.zmag , packet1.temperature );
    mavlink_msg_scaled_imu_decode(&msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
        mavlink_msg_to_send_buffer(buffer, &msg);
        for (i=0; i<mavlink_msg_get_send_buffer_length(&msg); i++) {
            comm_send_ch(MAVLINK_COMM_0, buffer[i]);
        }
    mavlink_msg_scaled_imu_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

        memset(&packet2, 0, sizeof(packet2));
    mavlink_msg_scaled_imu_send(MAVLINK_COMM_1 , packet1.time_boot_ms , packet1.xacc , packet1.yacc , packet1.zacc , packet1.xgyro , packet1.ygyro , packet1.zgyro , packet1.xmag , packet1.ymag , packet1.zmag , packet1.temperature );
    mavlink_msg_scaled_imu_decode(last_msg, &packet2);
        MAVLINK_ASSERT(memcmp(&packet1, &packet2, sizeof(packet1)) == 0);

#ifdef MAVLINK_HAVE_GET_MESSAGE_INFO
    MAVLINK_ASSERT(mavlink_get_message_info_by_name("SCALED_IMU") != NULL);
    MAVLINK_ASSERT(mavlink_get_message_info_by_id(MAVLINK_MSG_ID_SCALED_IMU) != NULL);
#endif
}

static void mavlink_test_arsem_standalone(uint8_t system_id, uint8_t component_id, mavlink_message_t *last_msg)
{
    mavlink_test_node_info(system_id, component_id, last_msg);
    mavlink_test_actuator_setpoint(system_id, component_id, last_msg);
    mavlink_test_actuator_status(system_id, component_id, last_msg);
    mavlink_test_control_authority(system_id, component_id, last_msg);
    mavlink_test_environmental_sample(system_id, component_id, last_msg);
    mavlink_test_power_state(system_id, component_id, last_msg);
    mavlink_test_link_status(system_id, component_id, last_msg);
    mavlink_test_bus_grant(system_id, component_id, last_msg);
    mavlink_test_actuator_request(system_id, component_id, last_msg);
    mavlink_test_node_health(system_id, component_id, last_msg);
    mavlink_test_request_result(system_id, component_id, last_msg);
    mavlink_test_imu_request(system_id, component_id, last_msg);
    mavlink_test_imu_sample(system_id, component_id, last_msg);
    mavlink_test_heartbeat(system_id, component_id, last_msg);
    mavlink_test_ping(system_id, component_id, last_msg);
    mavlink_test_scaled_imu(system_id, component_id, last_msg);
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // ARSEM_STANDALONE_TESTSUITE_H
