#pragma once
// MESSAGE ACTUATOR_SETPOINT PACKING

#define MAVLINK_MSG_ID_ACTUATOR_SETPOINT 200001


typedef struct __mavlink_actuator_setpoint_t {
 uint32_t command_id; /*<  Sender-generated monotonically increasing identifier used for acknowledgement and duplicate suppression.*/
 int16_t position_cdeg; /*< [cdeg] Requested actuator angle. The valid range is actuator-specific.*/
 uint16_t timeout_ms; /*< [ms] Time after reception at which this command becomes stale. Zero uses the receiver's configured timeout.*/
 uint16_t max_rate_cdeg_s; /*< [cdeg/s] Maximum actuator slew rate. Zero uses the receiver's configured rate.*/
 uint8_t target_system; /*<  System ID of the intended recipient.*/
 uint8_t target_component; /*<  Component ID of the intended recipient.*/
 uint8_t actuator; /*<  Actuator to command.*/
 uint8_t control_source; /*<  Authority under which this command was produced.*/
} mavlink_actuator_setpoint_t;

#define MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN 14
#define MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN 14
#define MAVLINK_MSG_ID_200001_LEN 14
#define MAVLINK_MSG_ID_200001_MIN_LEN 14

#define MAVLINK_MSG_ID_ACTUATOR_SETPOINT_CRC 49
#define MAVLINK_MSG_ID_200001_CRC 49



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ACTUATOR_SETPOINT { \
    200001, \
    "ACTUATOR_SETPOINT", \
    8, \
    {  { "command_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_actuator_setpoint_t, command_id) }, \
         { "position_cdeg", NULL, MAVLINK_TYPE_INT16_T, 0, 4, offsetof(mavlink_actuator_setpoint_t, position_cdeg) }, \
         { "timeout_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 6, offsetof(mavlink_actuator_setpoint_t, timeout_ms) }, \
         { "max_rate_cdeg_s", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_actuator_setpoint_t, max_rate_cdeg_s) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_actuator_setpoint_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_actuator_setpoint_t, target_component) }, \
         { "actuator", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_actuator_setpoint_t, actuator) }, \
         { "control_source", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_actuator_setpoint_t, control_source) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ACTUATOR_SETPOINT { \
    "ACTUATOR_SETPOINT", \
    8, \
    {  { "command_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_actuator_setpoint_t, command_id) }, \
         { "position_cdeg", NULL, MAVLINK_TYPE_INT16_T, 0, 4, offsetof(mavlink_actuator_setpoint_t, position_cdeg) }, \
         { "timeout_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 6, offsetof(mavlink_actuator_setpoint_t, timeout_ms) }, \
         { "max_rate_cdeg_s", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_actuator_setpoint_t, max_rate_cdeg_s) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_actuator_setpoint_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_actuator_setpoint_t, target_component) }, \
         { "actuator", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_actuator_setpoint_t, actuator) }, \
         { "control_source", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_actuator_setpoint_t, control_source) }, \
         } \
}
#endif

/**
 * @brief Pack a actuator_setpoint message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param command_id  Sender-generated monotonically increasing identifier used for acknowledgement and duplicate suppression.
 * @param position_cdeg [cdeg] Requested actuator angle. The valid range is actuator-specific.
 * @param timeout_ms [ms] Time after reception at which this command becomes stale. Zero uses the receiver's configured timeout.
 * @param max_rate_cdeg_s [cdeg/s] Maximum actuator slew rate. Zero uses the receiver's configured rate.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param actuator  Actuator to command.
 * @param control_source  Authority under which this command was produced.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_actuator_setpoint_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t command_id, int16_t position_cdeg, uint16_t timeout_ms, uint16_t max_rate_cdeg_s, uint8_t target_system, uint8_t target_component, uint8_t actuator, uint8_t control_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN];
    _mav_put_uint32_t(buf, 0, command_id);
    _mav_put_int16_t(buf, 4, position_cdeg);
    _mav_put_uint16_t(buf, 6, timeout_ms);
    _mav_put_uint16_t(buf, 8, max_rate_cdeg_s);
    _mav_put_uint8_t(buf, 10, target_system);
    _mav_put_uint8_t(buf, 11, target_component);
    _mav_put_uint8_t(buf, 12, actuator);
    _mav_put_uint8_t(buf, 13, control_source);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN);
#else
    mavlink_actuator_setpoint_t packet;
    packet.command_id = command_id;
    packet.position_cdeg = position_cdeg;
    packet.timeout_ms = timeout_ms;
    packet.max_rate_cdeg_s = max_rate_cdeg_s;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.actuator = actuator;
    packet.control_source = control_source;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ACTUATOR_SETPOINT;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_CRC);
}

/**
 * @brief Pack a actuator_setpoint message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param command_id  Sender-generated monotonically increasing identifier used for acknowledgement and duplicate suppression.
 * @param position_cdeg [cdeg] Requested actuator angle. The valid range is actuator-specific.
 * @param timeout_ms [ms] Time after reception at which this command becomes stale. Zero uses the receiver's configured timeout.
 * @param max_rate_cdeg_s [cdeg/s] Maximum actuator slew rate. Zero uses the receiver's configured rate.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param actuator  Actuator to command.
 * @param control_source  Authority under which this command was produced.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_actuator_setpoint_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t command_id, int16_t position_cdeg, uint16_t timeout_ms, uint16_t max_rate_cdeg_s, uint8_t target_system, uint8_t target_component, uint8_t actuator, uint8_t control_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN];
    _mav_put_uint32_t(buf, 0, command_id);
    _mav_put_int16_t(buf, 4, position_cdeg);
    _mav_put_uint16_t(buf, 6, timeout_ms);
    _mav_put_uint16_t(buf, 8, max_rate_cdeg_s);
    _mav_put_uint8_t(buf, 10, target_system);
    _mav_put_uint8_t(buf, 11, target_component);
    _mav_put_uint8_t(buf, 12, actuator);
    _mav_put_uint8_t(buf, 13, control_source);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN);
#else
    mavlink_actuator_setpoint_t packet;
    packet.command_id = command_id;
    packet.position_cdeg = position_cdeg;
    packet.timeout_ms = timeout_ms;
    packet.max_rate_cdeg_s = max_rate_cdeg_s;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.actuator = actuator;
    packet.control_source = control_source;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ACTUATOR_SETPOINT;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN);
#endif
}

/**
 * @brief Pack a actuator_setpoint message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param command_id  Sender-generated monotonically increasing identifier used for acknowledgement and duplicate suppression.
 * @param position_cdeg [cdeg] Requested actuator angle. The valid range is actuator-specific.
 * @param timeout_ms [ms] Time after reception at which this command becomes stale. Zero uses the receiver's configured timeout.
 * @param max_rate_cdeg_s [cdeg/s] Maximum actuator slew rate. Zero uses the receiver's configured rate.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param actuator  Actuator to command.
 * @param control_source  Authority under which this command was produced.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_actuator_setpoint_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t command_id,int16_t position_cdeg,uint16_t timeout_ms,uint16_t max_rate_cdeg_s,uint8_t target_system,uint8_t target_component,uint8_t actuator,uint8_t control_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN];
    _mav_put_uint32_t(buf, 0, command_id);
    _mav_put_int16_t(buf, 4, position_cdeg);
    _mav_put_uint16_t(buf, 6, timeout_ms);
    _mav_put_uint16_t(buf, 8, max_rate_cdeg_s);
    _mav_put_uint8_t(buf, 10, target_system);
    _mav_put_uint8_t(buf, 11, target_component);
    _mav_put_uint8_t(buf, 12, actuator);
    _mav_put_uint8_t(buf, 13, control_source);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN);
#else
    mavlink_actuator_setpoint_t packet;
    packet.command_id = command_id;
    packet.position_cdeg = position_cdeg;
    packet.timeout_ms = timeout_ms;
    packet.max_rate_cdeg_s = max_rate_cdeg_s;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.actuator = actuator;
    packet.control_source = control_source;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ACTUATOR_SETPOINT;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_CRC);
}

/**
 * @brief Encode a actuator_setpoint struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param actuator_setpoint C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_actuator_setpoint_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_actuator_setpoint_t* actuator_setpoint)
{
    return mavlink_msg_actuator_setpoint_pack(system_id, component_id, msg, actuator_setpoint->command_id, actuator_setpoint->position_cdeg, actuator_setpoint->timeout_ms, actuator_setpoint->max_rate_cdeg_s, actuator_setpoint->target_system, actuator_setpoint->target_component, actuator_setpoint->actuator, actuator_setpoint->control_source);
}

/**
 * @brief Encode a actuator_setpoint struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param actuator_setpoint C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_actuator_setpoint_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_actuator_setpoint_t* actuator_setpoint)
{
    return mavlink_msg_actuator_setpoint_pack_chan(system_id, component_id, chan, msg, actuator_setpoint->command_id, actuator_setpoint->position_cdeg, actuator_setpoint->timeout_ms, actuator_setpoint->max_rate_cdeg_s, actuator_setpoint->target_system, actuator_setpoint->target_component, actuator_setpoint->actuator, actuator_setpoint->control_source);
}

/**
 * @brief Encode a actuator_setpoint struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param actuator_setpoint C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_actuator_setpoint_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_actuator_setpoint_t* actuator_setpoint)
{
    return mavlink_msg_actuator_setpoint_pack_status(system_id, component_id, _status, msg,  actuator_setpoint->command_id, actuator_setpoint->position_cdeg, actuator_setpoint->timeout_ms, actuator_setpoint->max_rate_cdeg_s, actuator_setpoint->target_system, actuator_setpoint->target_component, actuator_setpoint->actuator, actuator_setpoint->control_source);
}

/**
 * @brief Send a actuator_setpoint message
 * @param chan MAVLink channel to send the message
 *
 * @param command_id  Sender-generated monotonically increasing identifier used for acknowledgement and duplicate suppression.
 * @param position_cdeg [cdeg] Requested actuator angle. The valid range is actuator-specific.
 * @param timeout_ms [ms] Time after reception at which this command becomes stale. Zero uses the receiver's configured timeout.
 * @param max_rate_cdeg_s [cdeg/s] Maximum actuator slew rate. Zero uses the receiver's configured rate.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param actuator  Actuator to command.
 * @param control_source  Authority under which this command was produced.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_actuator_setpoint_send(mavlink_channel_t chan, uint32_t command_id, int16_t position_cdeg, uint16_t timeout_ms, uint16_t max_rate_cdeg_s, uint8_t target_system, uint8_t target_component, uint8_t actuator, uint8_t control_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN];
    _mav_put_uint32_t(buf, 0, command_id);
    _mav_put_int16_t(buf, 4, position_cdeg);
    _mav_put_uint16_t(buf, 6, timeout_ms);
    _mav_put_uint16_t(buf, 8, max_rate_cdeg_s);
    _mav_put_uint8_t(buf, 10, target_system);
    _mav_put_uint8_t(buf, 11, target_component);
    _mav_put_uint8_t(buf, 12, actuator);
    _mav_put_uint8_t(buf, 13, control_source);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_SETPOINT, buf, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_CRC);
#else
    mavlink_actuator_setpoint_t packet;
    packet.command_id = command_id;
    packet.position_cdeg = position_cdeg;
    packet.timeout_ms = timeout_ms;
    packet.max_rate_cdeg_s = max_rate_cdeg_s;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.actuator = actuator;
    packet.control_source = control_source;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_SETPOINT, (const char *)&packet, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_CRC);
#endif
}

/**
 * @brief Send a actuator_setpoint message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_actuator_setpoint_send_struct(mavlink_channel_t chan, const mavlink_actuator_setpoint_t* actuator_setpoint)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_actuator_setpoint_send(chan, actuator_setpoint->command_id, actuator_setpoint->position_cdeg, actuator_setpoint->timeout_ms, actuator_setpoint->max_rate_cdeg_s, actuator_setpoint->target_system, actuator_setpoint->target_component, actuator_setpoint->actuator, actuator_setpoint->control_source);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_SETPOINT, (const char *)actuator_setpoint, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_CRC);
#endif
}

#if MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_actuator_setpoint_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t command_id, int16_t position_cdeg, uint16_t timeout_ms, uint16_t max_rate_cdeg_s, uint8_t target_system, uint8_t target_component, uint8_t actuator, uint8_t control_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, command_id);
    _mav_put_int16_t(buf, 4, position_cdeg);
    _mav_put_uint16_t(buf, 6, timeout_ms);
    _mav_put_uint16_t(buf, 8, max_rate_cdeg_s);
    _mav_put_uint8_t(buf, 10, target_system);
    _mav_put_uint8_t(buf, 11, target_component);
    _mav_put_uint8_t(buf, 12, actuator);
    _mav_put_uint8_t(buf, 13, control_source);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_SETPOINT, buf, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_CRC);
#else
    mavlink_actuator_setpoint_t *packet = (mavlink_actuator_setpoint_t *)msgbuf;
    packet->command_id = command_id;
    packet->position_cdeg = position_cdeg;
    packet->timeout_ms = timeout_ms;
    packet->max_rate_cdeg_s = max_rate_cdeg_s;
    packet->target_system = target_system;
    packet->target_component = target_component;
    packet->actuator = actuator;
    packet->control_source = control_source;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_SETPOINT, (const char *)packet, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_CRC);
#endif
}
#endif

#endif

// MESSAGE ACTUATOR_SETPOINT UNPACKING


/**
 * @brief Get field command_id from actuator_setpoint message
 *
 * @return  Sender-generated monotonically increasing identifier used for acknowledgement and duplicate suppression.
 */
static inline uint32_t mavlink_msg_actuator_setpoint_get_command_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field position_cdeg from actuator_setpoint message
 *
 * @return [cdeg] Requested actuator angle. The valid range is actuator-specific.
 */
static inline int16_t mavlink_msg_actuator_setpoint_get_position_cdeg(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  4);
}

/**
 * @brief Get field timeout_ms from actuator_setpoint message
 *
 * @return [ms] Time after reception at which this command becomes stale. Zero uses the receiver's configured timeout.
 */
static inline uint16_t mavlink_msg_actuator_setpoint_get_timeout_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  6);
}

/**
 * @brief Get field max_rate_cdeg_s from actuator_setpoint message
 *
 * @return [cdeg/s] Maximum actuator slew rate. Zero uses the receiver's configured rate.
 */
static inline uint16_t mavlink_msg_actuator_setpoint_get_max_rate_cdeg_s(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  8);
}

/**
 * @brief Get field target_system from actuator_setpoint message
 *
 * @return  System ID of the intended recipient.
 */
static inline uint8_t mavlink_msg_actuator_setpoint_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  10);
}

/**
 * @brief Get field target_component from actuator_setpoint message
 *
 * @return  Component ID of the intended recipient.
 */
static inline uint8_t mavlink_msg_actuator_setpoint_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  11);
}

/**
 * @brief Get field actuator from actuator_setpoint message
 *
 * @return  Actuator to command.
 */
static inline uint8_t mavlink_msg_actuator_setpoint_get_actuator(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  12);
}

/**
 * @brief Get field control_source from actuator_setpoint message
 *
 * @return  Authority under which this command was produced.
 */
static inline uint8_t mavlink_msg_actuator_setpoint_get_control_source(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  13);
}

/**
 * @brief Decode a actuator_setpoint message into a struct
 *
 * @param msg The message to decode
 * @param actuator_setpoint C-struct to decode the message contents into
 */
static inline void mavlink_msg_actuator_setpoint_decode(const mavlink_message_t* msg, mavlink_actuator_setpoint_t* actuator_setpoint)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    actuator_setpoint->command_id = mavlink_msg_actuator_setpoint_get_command_id(msg);
    actuator_setpoint->position_cdeg = mavlink_msg_actuator_setpoint_get_position_cdeg(msg);
    actuator_setpoint->timeout_ms = mavlink_msg_actuator_setpoint_get_timeout_ms(msg);
    actuator_setpoint->max_rate_cdeg_s = mavlink_msg_actuator_setpoint_get_max_rate_cdeg_s(msg);
    actuator_setpoint->target_system = mavlink_msg_actuator_setpoint_get_target_system(msg);
    actuator_setpoint->target_component = mavlink_msg_actuator_setpoint_get_target_component(msg);
    actuator_setpoint->actuator = mavlink_msg_actuator_setpoint_get_actuator(msg);
    actuator_setpoint->control_source = mavlink_msg_actuator_setpoint_get_control_source(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN? msg->len : MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN;
        memset(actuator_setpoint, 0, MAVLINK_MSG_ID_ACTUATOR_SETPOINT_LEN);
    memcpy(actuator_setpoint, _MAV_PAYLOAD(msg), len);
#endif
}
