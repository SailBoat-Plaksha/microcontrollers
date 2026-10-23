#pragma once
// MESSAGE ACTUATOR_STATUS PACKING

#define MAVLINK_MSG_ID_ACTUATOR_STATUS 200002


typedef struct __mavlink_actuator_status_t {
 uint32_t time_boot_ms; /*< [ms] Time since the reporting component booted.*/
 uint32_t command_id; /*<  Identifier of the most recently processed command.*/
 int16_t requested_position_cdeg; /*< [cdeg] Most recently accepted requested angle.*/
 int16_t measured_position_cdeg; /*< [cdeg] Measured actuator angle.*/
 int16_t velocity_cdeg_s; /*< [cdeg/s] Measured angular velocity.*/
 uint16_t fault_flags; /*<  Active faults. Zero means no reported fault.*/
 uint8_t actuator; /*<  Actuator being reported.*/
 uint8_t control_source; /*<  Source that currently has authority.*/
 uint8_t result; /*<  Result or progress of the most recently processed command.*/
 uint8_t enabled; /*<  One when output to the actuator is enabled, otherwise zero.*/
} mavlink_actuator_status_t;

#define MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN 20
#define MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN 20
#define MAVLINK_MSG_ID_200002_LEN 20
#define MAVLINK_MSG_ID_200002_MIN_LEN 20

#define MAVLINK_MSG_ID_ACTUATOR_STATUS_CRC 131
#define MAVLINK_MSG_ID_200002_CRC 131



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ACTUATOR_STATUS { \
    200002, \
    "ACTUATOR_STATUS", \
    10, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_actuator_status_t, time_boot_ms) }, \
         { "command_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_actuator_status_t, command_id) }, \
         { "requested_position_cdeg", NULL, MAVLINK_TYPE_INT16_T, 0, 8, offsetof(mavlink_actuator_status_t, requested_position_cdeg) }, \
         { "measured_position_cdeg", NULL, MAVLINK_TYPE_INT16_T, 0, 10, offsetof(mavlink_actuator_status_t, measured_position_cdeg) }, \
         { "velocity_cdeg_s", NULL, MAVLINK_TYPE_INT16_T, 0, 12, offsetof(mavlink_actuator_status_t, velocity_cdeg_s) }, \
         { "fault_flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 14, offsetof(mavlink_actuator_status_t, fault_flags) }, \
         { "actuator", NULL, MAVLINK_TYPE_UINT8_T, 0, 16, offsetof(mavlink_actuator_status_t, actuator) }, \
         { "control_source", NULL, MAVLINK_TYPE_UINT8_T, 0, 17, offsetof(mavlink_actuator_status_t, control_source) }, \
         { "result", NULL, MAVLINK_TYPE_UINT8_T, 0, 18, offsetof(mavlink_actuator_status_t, result) }, \
         { "enabled", NULL, MAVLINK_TYPE_UINT8_T, 0, 19, offsetof(mavlink_actuator_status_t, enabled) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ACTUATOR_STATUS { \
    "ACTUATOR_STATUS", \
    10, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_actuator_status_t, time_boot_ms) }, \
         { "command_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_actuator_status_t, command_id) }, \
         { "requested_position_cdeg", NULL, MAVLINK_TYPE_INT16_T, 0, 8, offsetof(mavlink_actuator_status_t, requested_position_cdeg) }, \
         { "measured_position_cdeg", NULL, MAVLINK_TYPE_INT16_T, 0, 10, offsetof(mavlink_actuator_status_t, measured_position_cdeg) }, \
         { "velocity_cdeg_s", NULL, MAVLINK_TYPE_INT16_T, 0, 12, offsetof(mavlink_actuator_status_t, velocity_cdeg_s) }, \
         { "fault_flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 14, offsetof(mavlink_actuator_status_t, fault_flags) }, \
         { "actuator", NULL, MAVLINK_TYPE_UINT8_T, 0, 16, offsetof(mavlink_actuator_status_t, actuator) }, \
         { "control_source", NULL, MAVLINK_TYPE_UINT8_T, 0, 17, offsetof(mavlink_actuator_status_t, control_source) }, \
         { "result", NULL, MAVLINK_TYPE_UINT8_T, 0, 18, offsetof(mavlink_actuator_status_t, result) }, \
         { "enabled", NULL, MAVLINK_TYPE_UINT8_T, 0, 19, offsetof(mavlink_actuator_status_t, enabled) }, \
         } \
}
#endif

/**
 * @brief Pack a actuator_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param command_id  Identifier of the most recently processed command.
 * @param requested_position_cdeg [cdeg] Most recently accepted requested angle.
 * @param measured_position_cdeg [cdeg] Measured actuator angle.
 * @param velocity_cdeg_s [cdeg/s] Measured angular velocity.
 * @param fault_flags  Active faults. Zero means no reported fault.
 * @param actuator  Actuator being reported.
 * @param control_source  Source that currently has authority.
 * @param result  Result or progress of the most recently processed command.
 * @param enabled  One when output to the actuator is enabled, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_actuator_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint32_t command_id, int16_t requested_position_cdeg, int16_t measured_position_cdeg, int16_t velocity_cdeg_s, uint16_t fault_flags, uint8_t actuator, uint8_t control_source, uint8_t result, uint8_t enabled)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, command_id);
    _mav_put_int16_t(buf, 8, requested_position_cdeg);
    _mav_put_int16_t(buf, 10, measured_position_cdeg);
    _mav_put_int16_t(buf, 12, velocity_cdeg_s);
    _mav_put_uint16_t(buf, 14, fault_flags);
    _mav_put_uint8_t(buf, 16, actuator);
    _mav_put_uint8_t(buf, 17, control_source);
    _mav_put_uint8_t(buf, 18, result);
    _mav_put_uint8_t(buf, 19, enabled);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN);
#else
    mavlink_actuator_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.command_id = command_id;
    packet.requested_position_cdeg = requested_position_cdeg;
    packet.measured_position_cdeg = measured_position_cdeg;
    packet.velocity_cdeg_s = velocity_cdeg_s;
    packet.fault_flags = fault_flags;
    packet.actuator = actuator;
    packet.control_source = control_source;
    packet.result = result;
    packet.enabled = enabled;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ACTUATOR_STATUS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_CRC);
}

/**
 * @brief Pack a actuator_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param command_id  Identifier of the most recently processed command.
 * @param requested_position_cdeg [cdeg] Most recently accepted requested angle.
 * @param measured_position_cdeg [cdeg] Measured actuator angle.
 * @param velocity_cdeg_s [cdeg/s] Measured angular velocity.
 * @param fault_flags  Active faults. Zero means no reported fault.
 * @param actuator  Actuator being reported.
 * @param control_source  Source that currently has authority.
 * @param result  Result or progress of the most recently processed command.
 * @param enabled  One when output to the actuator is enabled, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_actuator_status_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint32_t command_id, int16_t requested_position_cdeg, int16_t measured_position_cdeg, int16_t velocity_cdeg_s, uint16_t fault_flags, uint8_t actuator, uint8_t control_source, uint8_t result, uint8_t enabled)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, command_id);
    _mav_put_int16_t(buf, 8, requested_position_cdeg);
    _mav_put_int16_t(buf, 10, measured_position_cdeg);
    _mav_put_int16_t(buf, 12, velocity_cdeg_s);
    _mav_put_uint16_t(buf, 14, fault_flags);
    _mav_put_uint8_t(buf, 16, actuator);
    _mav_put_uint8_t(buf, 17, control_source);
    _mav_put_uint8_t(buf, 18, result);
    _mav_put_uint8_t(buf, 19, enabled);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN);
#else
    mavlink_actuator_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.command_id = command_id;
    packet.requested_position_cdeg = requested_position_cdeg;
    packet.measured_position_cdeg = measured_position_cdeg;
    packet.velocity_cdeg_s = velocity_cdeg_s;
    packet.fault_flags = fault_flags;
    packet.actuator = actuator;
    packet.control_source = control_source;
    packet.result = result;
    packet.enabled = enabled;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ACTUATOR_STATUS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN);
#endif
}

/**
 * @brief Pack a actuator_status message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param command_id  Identifier of the most recently processed command.
 * @param requested_position_cdeg [cdeg] Most recently accepted requested angle.
 * @param measured_position_cdeg [cdeg] Measured actuator angle.
 * @param velocity_cdeg_s [cdeg/s] Measured angular velocity.
 * @param fault_flags  Active faults. Zero means no reported fault.
 * @param actuator  Actuator being reported.
 * @param control_source  Source that currently has authority.
 * @param result  Result or progress of the most recently processed command.
 * @param enabled  One when output to the actuator is enabled, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_actuator_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t time_boot_ms,uint32_t command_id,int16_t requested_position_cdeg,int16_t measured_position_cdeg,int16_t velocity_cdeg_s,uint16_t fault_flags,uint8_t actuator,uint8_t control_source,uint8_t result,uint8_t enabled)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, command_id);
    _mav_put_int16_t(buf, 8, requested_position_cdeg);
    _mav_put_int16_t(buf, 10, measured_position_cdeg);
    _mav_put_int16_t(buf, 12, velocity_cdeg_s);
    _mav_put_uint16_t(buf, 14, fault_flags);
    _mav_put_uint8_t(buf, 16, actuator);
    _mav_put_uint8_t(buf, 17, control_source);
    _mav_put_uint8_t(buf, 18, result);
    _mav_put_uint8_t(buf, 19, enabled);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN);
#else
    mavlink_actuator_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.command_id = command_id;
    packet.requested_position_cdeg = requested_position_cdeg;
    packet.measured_position_cdeg = measured_position_cdeg;
    packet.velocity_cdeg_s = velocity_cdeg_s;
    packet.fault_flags = fault_flags;
    packet.actuator = actuator;
    packet.control_source = control_source;
    packet.result = result;
    packet.enabled = enabled;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ACTUATOR_STATUS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_CRC);
}

/**
 * @brief Encode a actuator_status struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param actuator_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_actuator_status_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_actuator_status_t* actuator_status)
{
    return mavlink_msg_actuator_status_pack(system_id, component_id, msg, actuator_status->time_boot_ms, actuator_status->command_id, actuator_status->requested_position_cdeg, actuator_status->measured_position_cdeg, actuator_status->velocity_cdeg_s, actuator_status->fault_flags, actuator_status->actuator, actuator_status->control_source, actuator_status->result, actuator_status->enabled);
}

/**
 * @brief Encode a actuator_status struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param actuator_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_actuator_status_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_actuator_status_t* actuator_status)
{
    return mavlink_msg_actuator_status_pack_chan(system_id, component_id, chan, msg, actuator_status->time_boot_ms, actuator_status->command_id, actuator_status->requested_position_cdeg, actuator_status->measured_position_cdeg, actuator_status->velocity_cdeg_s, actuator_status->fault_flags, actuator_status->actuator, actuator_status->control_source, actuator_status->result, actuator_status->enabled);
}

/**
 * @brief Encode a actuator_status struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param actuator_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_actuator_status_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_actuator_status_t* actuator_status)
{
    return mavlink_msg_actuator_status_pack_status(system_id, component_id, _status, msg,  actuator_status->time_boot_ms, actuator_status->command_id, actuator_status->requested_position_cdeg, actuator_status->measured_position_cdeg, actuator_status->velocity_cdeg_s, actuator_status->fault_flags, actuator_status->actuator, actuator_status->control_source, actuator_status->result, actuator_status->enabled);
}

/**
 * @brief Send a actuator_status message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param command_id  Identifier of the most recently processed command.
 * @param requested_position_cdeg [cdeg] Most recently accepted requested angle.
 * @param measured_position_cdeg [cdeg] Measured actuator angle.
 * @param velocity_cdeg_s [cdeg/s] Measured angular velocity.
 * @param fault_flags  Active faults. Zero means no reported fault.
 * @param actuator  Actuator being reported.
 * @param control_source  Source that currently has authority.
 * @param result  Result or progress of the most recently processed command.
 * @param enabled  One when output to the actuator is enabled, otherwise zero.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_actuator_status_send(mavlink_channel_t chan, uint32_t time_boot_ms, uint32_t command_id, int16_t requested_position_cdeg, int16_t measured_position_cdeg, int16_t velocity_cdeg_s, uint16_t fault_flags, uint8_t actuator, uint8_t control_source, uint8_t result, uint8_t enabled)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, command_id);
    _mav_put_int16_t(buf, 8, requested_position_cdeg);
    _mav_put_int16_t(buf, 10, measured_position_cdeg);
    _mav_put_int16_t(buf, 12, velocity_cdeg_s);
    _mav_put_uint16_t(buf, 14, fault_flags);
    _mav_put_uint8_t(buf, 16, actuator);
    _mav_put_uint8_t(buf, 17, control_source);
    _mav_put_uint8_t(buf, 18, result);
    _mav_put_uint8_t(buf, 19, enabled);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_STATUS, buf, MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_CRC);
#else
    mavlink_actuator_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.command_id = command_id;
    packet.requested_position_cdeg = requested_position_cdeg;
    packet.measured_position_cdeg = measured_position_cdeg;
    packet.velocity_cdeg_s = velocity_cdeg_s;
    packet.fault_flags = fault_flags;
    packet.actuator = actuator;
    packet.control_source = control_source;
    packet.result = result;
    packet.enabled = enabled;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_STATUS, (const char *)&packet, MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_CRC);
#endif
}

/**
 * @brief Send a actuator_status message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_actuator_status_send_struct(mavlink_channel_t chan, const mavlink_actuator_status_t* actuator_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_actuator_status_send(chan, actuator_status->time_boot_ms, actuator_status->command_id, actuator_status->requested_position_cdeg, actuator_status->measured_position_cdeg, actuator_status->velocity_cdeg_s, actuator_status->fault_flags, actuator_status->actuator, actuator_status->control_source, actuator_status->result, actuator_status->enabled);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_STATUS, (const char *)actuator_status, MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_CRC);
#endif
}

#if MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_actuator_status_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, uint32_t command_id, int16_t requested_position_cdeg, int16_t measured_position_cdeg, int16_t velocity_cdeg_s, uint16_t fault_flags, uint8_t actuator, uint8_t control_source, uint8_t result, uint8_t enabled)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, command_id);
    _mav_put_int16_t(buf, 8, requested_position_cdeg);
    _mav_put_int16_t(buf, 10, measured_position_cdeg);
    _mav_put_int16_t(buf, 12, velocity_cdeg_s);
    _mav_put_uint16_t(buf, 14, fault_flags);
    _mav_put_uint8_t(buf, 16, actuator);
    _mav_put_uint8_t(buf, 17, control_source);
    _mav_put_uint8_t(buf, 18, result);
    _mav_put_uint8_t(buf, 19, enabled);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_STATUS, buf, MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_CRC);
#else
    mavlink_actuator_status_t *packet = (mavlink_actuator_status_t *)msgbuf;
    packet->time_boot_ms = time_boot_ms;
    packet->command_id = command_id;
    packet->requested_position_cdeg = requested_position_cdeg;
    packet->measured_position_cdeg = measured_position_cdeg;
    packet->velocity_cdeg_s = velocity_cdeg_s;
    packet->fault_flags = fault_flags;
    packet->actuator = actuator;
    packet->control_source = control_source;
    packet->result = result;
    packet->enabled = enabled;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_STATUS, (const char *)packet, MAVLINK_MSG_ID_ACTUATOR_STATUS_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN, MAVLINK_MSG_ID_ACTUATOR_STATUS_CRC);
#endif
}
#endif

#endif

// MESSAGE ACTUATOR_STATUS UNPACKING


/**
 * @brief Get field time_boot_ms from actuator_status message
 *
 * @return [ms] Time since the reporting component booted.
 */
static inline uint32_t mavlink_msg_actuator_status_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field command_id from actuator_status message
 *
 * @return  Identifier of the most recently processed command.
 */
static inline uint32_t mavlink_msg_actuator_status_get_command_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  4);
}

/**
 * @brief Get field requested_position_cdeg from actuator_status message
 *
 * @return [cdeg] Most recently accepted requested angle.
 */
static inline int16_t mavlink_msg_actuator_status_get_requested_position_cdeg(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  8);
}

/**
 * @brief Get field measured_position_cdeg from actuator_status message
 *
 * @return [cdeg] Measured actuator angle.
 */
static inline int16_t mavlink_msg_actuator_status_get_measured_position_cdeg(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  10);
}

/**
 * @brief Get field velocity_cdeg_s from actuator_status message
 *
 * @return [cdeg/s] Measured angular velocity.
 */
static inline int16_t mavlink_msg_actuator_status_get_velocity_cdeg_s(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  12);
}

/**
 * @brief Get field fault_flags from actuator_status message
 *
 * @return  Active faults. Zero means no reported fault.
 */
static inline uint16_t mavlink_msg_actuator_status_get_fault_flags(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  14);
}

/**
 * @brief Get field actuator from actuator_status message
 *
 * @return  Actuator being reported.
 */
static inline uint8_t mavlink_msg_actuator_status_get_actuator(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  16);
}

/**
 * @brief Get field control_source from actuator_status message
 *
 * @return  Source that currently has authority.
 */
static inline uint8_t mavlink_msg_actuator_status_get_control_source(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  17);
}

/**
 * @brief Get field result from actuator_status message
 *
 * @return  Result or progress of the most recently processed command.
 */
static inline uint8_t mavlink_msg_actuator_status_get_result(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  18);
}

/**
 * @brief Get field enabled from actuator_status message
 *
 * @return  One when output to the actuator is enabled, otherwise zero.
 */
static inline uint8_t mavlink_msg_actuator_status_get_enabled(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  19);
}

/**
 * @brief Decode a actuator_status message into a struct
 *
 * @param msg The message to decode
 * @param actuator_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_actuator_status_decode(const mavlink_message_t* msg, mavlink_actuator_status_t* actuator_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    actuator_status->time_boot_ms = mavlink_msg_actuator_status_get_time_boot_ms(msg);
    actuator_status->command_id = mavlink_msg_actuator_status_get_command_id(msg);
    actuator_status->requested_position_cdeg = mavlink_msg_actuator_status_get_requested_position_cdeg(msg);
    actuator_status->measured_position_cdeg = mavlink_msg_actuator_status_get_measured_position_cdeg(msg);
    actuator_status->velocity_cdeg_s = mavlink_msg_actuator_status_get_velocity_cdeg_s(msg);
    actuator_status->fault_flags = mavlink_msg_actuator_status_get_fault_flags(msg);
    actuator_status->actuator = mavlink_msg_actuator_status_get_actuator(msg);
    actuator_status->control_source = mavlink_msg_actuator_status_get_control_source(msg);
    actuator_status->result = mavlink_msg_actuator_status_get_result(msg);
    actuator_status->enabled = mavlink_msg_actuator_status_get_enabled(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN? msg->len : MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN;
        memset(actuator_status, 0, MAVLINK_MSG_ID_ACTUATOR_STATUS_LEN);
    memcpy(actuator_status, _MAV_PAYLOAD(msg), len);
#endif
}
