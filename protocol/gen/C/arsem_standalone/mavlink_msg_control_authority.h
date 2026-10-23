#pragma once
// MESSAGE CONTROL_AUTHORITY PACKING

#define MAVLINK_MSG_ID_CONTROL_AUTHORITY 200003


typedef struct __mavlink_control_authority_t {
 uint32_t time_boot_ms; /*< [ms] Time since the reporting component booted.*/
 uint16_t authority_timeout_ms; /*< [ms] Time remaining before the authority lease expires. Zero means no lease is active or the authority does not expire.*/
 uint8_t control_source; /*<  Source that currently has authority.*/
 uint8_t rc_available; /*<  One when valid local radio-control input is available, otherwise zero.*/
 uint8_t autonomy_available; /*<  One when commands from the main computer are available, otherwise zero.*/
 uint8_t failsafe_active; /*<  One when local failsafe behavior is active, otherwise zero.*/
} mavlink_control_authority_t;

#define MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN 10
#define MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN 10
#define MAVLINK_MSG_ID_200003_LEN 10
#define MAVLINK_MSG_ID_200003_MIN_LEN 10

#define MAVLINK_MSG_ID_CONTROL_AUTHORITY_CRC 143
#define MAVLINK_MSG_ID_200003_CRC 143



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_CONTROL_AUTHORITY { \
    200003, \
    "CONTROL_AUTHORITY", \
    6, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_control_authority_t, time_boot_ms) }, \
         { "authority_timeout_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_control_authority_t, authority_timeout_ms) }, \
         { "control_source", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_control_authority_t, control_source) }, \
         { "rc_available", NULL, MAVLINK_TYPE_UINT8_T, 0, 7, offsetof(mavlink_control_authority_t, rc_available) }, \
         { "autonomy_available", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_control_authority_t, autonomy_available) }, \
         { "failsafe_active", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_control_authority_t, failsafe_active) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_CONTROL_AUTHORITY { \
    "CONTROL_AUTHORITY", \
    6, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_control_authority_t, time_boot_ms) }, \
         { "authority_timeout_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_control_authority_t, authority_timeout_ms) }, \
         { "control_source", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_control_authority_t, control_source) }, \
         { "rc_available", NULL, MAVLINK_TYPE_UINT8_T, 0, 7, offsetof(mavlink_control_authority_t, rc_available) }, \
         { "autonomy_available", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_control_authority_t, autonomy_available) }, \
         { "failsafe_active", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_control_authority_t, failsafe_active) }, \
         } \
}
#endif

/**
 * @brief Pack a control_authority message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param authority_timeout_ms [ms] Time remaining before the authority lease expires. Zero means no lease is active or the authority does not expire.
 * @param control_source  Source that currently has authority.
 * @param rc_available  One when valid local radio-control input is available, otherwise zero.
 * @param autonomy_available  One when commands from the main computer are available, otherwise zero.
 * @param failsafe_active  One when local failsafe behavior is active, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_control_authority_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint16_t authority_timeout_ms, uint8_t control_source, uint8_t rc_available, uint8_t autonomy_available, uint8_t failsafe_active)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint16_t(buf, 4, authority_timeout_ms);
    _mav_put_uint8_t(buf, 6, control_source);
    _mav_put_uint8_t(buf, 7, rc_available);
    _mav_put_uint8_t(buf, 8, autonomy_available);
    _mav_put_uint8_t(buf, 9, failsafe_active);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN);
#else
    mavlink_control_authority_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.authority_timeout_ms = authority_timeout_ms;
    packet.control_source = control_source;
    packet.rc_available = rc_available;
    packet.autonomy_available = autonomy_available;
    packet.failsafe_active = failsafe_active;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_CONTROL_AUTHORITY;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_CRC);
}

/**
 * @brief Pack a control_authority message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param authority_timeout_ms [ms] Time remaining before the authority lease expires. Zero means no lease is active or the authority does not expire.
 * @param control_source  Source that currently has authority.
 * @param rc_available  One when valid local radio-control input is available, otherwise zero.
 * @param autonomy_available  One when commands from the main computer are available, otherwise zero.
 * @param failsafe_active  One when local failsafe behavior is active, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_control_authority_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint16_t authority_timeout_ms, uint8_t control_source, uint8_t rc_available, uint8_t autonomy_available, uint8_t failsafe_active)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint16_t(buf, 4, authority_timeout_ms);
    _mav_put_uint8_t(buf, 6, control_source);
    _mav_put_uint8_t(buf, 7, rc_available);
    _mav_put_uint8_t(buf, 8, autonomy_available);
    _mav_put_uint8_t(buf, 9, failsafe_active);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN);
#else
    mavlink_control_authority_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.authority_timeout_ms = authority_timeout_ms;
    packet.control_source = control_source;
    packet.rc_available = rc_available;
    packet.autonomy_available = autonomy_available;
    packet.failsafe_active = failsafe_active;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_CONTROL_AUTHORITY;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN);
#endif
}

/**
 * @brief Pack a control_authority message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param authority_timeout_ms [ms] Time remaining before the authority lease expires. Zero means no lease is active or the authority does not expire.
 * @param control_source  Source that currently has authority.
 * @param rc_available  One when valid local radio-control input is available, otherwise zero.
 * @param autonomy_available  One when commands from the main computer are available, otherwise zero.
 * @param failsafe_active  One when local failsafe behavior is active, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_control_authority_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t time_boot_ms,uint16_t authority_timeout_ms,uint8_t control_source,uint8_t rc_available,uint8_t autonomy_available,uint8_t failsafe_active)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint16_t(buf, 4, authority_timeout_ms);
    _mav_put_uint8_t(buf, 6, control_source);
    _mav_put_uint8_t(buf, 7, rc_available);
    _mav_put_uint8_t(buf, 8, autonomy_available);
    _mav_put_uint8_t(buf, 9, failsafe_active);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN);
#else
    mavlink_control_authority_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.authority_timeout_ms = authority_timeout_ms;
    packet.control_source = control_source;
    packet.rc_available = rc_available;
    packet.autonomy_available = autonomy_available;
    packet.failsafe_active = failsafe_active;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_CONTROL_AUTHORITY;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_CRC);
}

/**
 * @brief Encode a control_authority struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param control_authority C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_control_authority_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_control_authority_t* control_authority)
{
    return mavlink_msg_control_authority_pack(system_id, component_id, msg, control_authority->time_boot_ms, control_authority->authority_timeout_ms, control_authority->control_source, control_authority->rc_available, control_authority->autonomy_available, control_authority->failsafe_active);
}

/**
 * @brief Encode a control_authority struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param control_authority C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_control_authority_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_control_authority_t* control_authority)
{
    return mavlink_msg_control_authority_pack_chan(system_id, component_id, chan, msg, control_authority->time_boot_ms, control_authority->authority_timeout_ms, control_authority->control_source, control_authority->rc_available, control_authority->autonomy_available, control_authority->failsafe_active);
}

/**
 * @brief Encode a control_authority struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param control_authority C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_control_authority_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_control_authority_t* control_authority)
{
    return mavlink_msg_control_authority_pack_status(system_id, component_id, _status, msg,  control_authority->time_boot_ms, control_authority->authority_timeout_ms, control_authority->control_source, control_authority->rc_available, control_authority->autonomy_available, control_authority->failsafe_active);
}

/**
 * @brief Send a control_authority message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param authority_timeout_ms [ms] Time remaining before the authority lease expires. Zero means no lease is active or the authority does not expire.
 * @param control_source  Source that currently has authority.
 * @param rc_available  One when valid local radio-control input is available, otherwise zero.
 * @param autonomy_available  One when commands from the main computer are available, otherwise zero.
 * @param failsafe_active  One when local failsafe behavior is active, otherwise zero.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_control_authority_send(mavlink_channel_t chan, uint32_t time_boot_ms, uint16_t authority_timeout_ms, uint8_t control_source, uint8_t rc_available, uint8_t autonomy_available, uint8_t failsafe_active)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint16_t(buf, 4, authority_timeout_ms);
    _mav_put_uint8_t(buf, 6, control_source);
    _mav_put_uint8_t(buf, 7, rc_available);
    _mav_put_uint8_t(buf, 8, autonomy_available);
    _mav_put_uint8_t(buf, 9, failsafe_active);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CONTROL_AUTHORITY, buf, MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_CRC);
#else
    mavlink_control_authority_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.authority_timeout_ms = authority_timeout_ms;
    packet.control_source = control_source;
    packet.rc_available = rc_available;
    packet.autonomy_available = autonomy_available;
    packet.failsafe_active = failsafe_active;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CONTROL_AUTHORITY, (const char *)&packet, MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_CRC);
#endif
}

/**
 * @brief Send a control_authority message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_control_authority_send_struct(mavlink_channel_t chan, const mavlink_control_authority_t* control_authority)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_control_authority_send(chan, control_authority->time_boot_ms, control_authority->authority_timeout_ms, control_authority->control_source, control_authority->rc_available, control_authority->autonomy_available, control_authority->failsafe_active);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CONTROL_AUTHORITY, (const char *)control_authority, MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_CRC);
#endif
}

#if MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_control_authority_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, uint16_t authority_timeout_ms, uint8_t control_source, uint8_t rc_available, uint8_t autonomy_available, uint8_t failsafe_active)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint16_t(buf, 4, authority_timeout_ms);
    _mav_put_uint8_t(buf, 6, control_source);
    _mav_put_uint8_t(buf, 7, rc_available);
    _mav_put_uint8_t(buf, 8, autonomy_available);
    _mav_put_uint8_t(buf, 9, failsafe_active);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CONTROL_AUTHORITY, buf, MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_CRC);
#else
    mavlink_control_authority_t *packet = (mavlink_control_authority_t *)msgbuf;
    packet->time_boot_ms = time_boot_ms;
    packet->authority_timeout_ms = authority_timeout_ms;
    packet->control_source = control_source;
    packet->rc_available = rc_available;
    packet->autonomy_available = autonomy_available;
    packet->failsafe_active = failsafe_active;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CONTROL_AUTHORITY, (const char *)packet, MAVLINK_MSG_ID_CONTROL_AUTHORITY_MIN_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN, MAVLINK_MSG_ID_CONTROL_AUTHORITY_CRC);
#endif
}
#endif

#endif

// MESSAGE CONTROL_AUTHORITY UNPACKING


/**
 * @brief Get field time_boot_ms from control_authority message
 *
 * @return [ms] Time since the reporting component booted.
 */
static inline uint32_t mavlink_msg_control_authority_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field authority_timeout_ms from control_authority message
 *
 * @return [ms] Time remaining before the authority lease expires. Zero means no lease is active or the authority does not expire.
 */
static inline uint16_t mavlink_msg_control_authority_get_authority_timeout_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  4);
}

/**
 * @brief Get field control_source from control_authority message
 *
 * @return  Source that currently has authority.
 */
static inline uint8_t mavlink_msg_control_authority_get_control_source(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  6);
}

/**
 * @brief Get field rc_available from control_authority message
 *
 * @return  One when valid local radio-control input is available, otherwise zero.
 */
static inline uint8_t mavlink_msg_control_authority_get_rc_available(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  7);
}

/**
 * @brief Get field autonomy_available from control_authority message
 *
 * @return  One when commands from the main computer are available, otherwise zero.
 */
static inline uint8_t mavlink_msg_control_authority_get_autonomy_available(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  8);
}

/**
 * @brief Get field failsafe_active from control_authority message
 *
 * @return  One when local failsafe behavior is active, otherwise zero.
 */
static inline uint8_t mavlink_msg_control_authority_get_failsafe_active(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  9);
}

/**
 * @brief Decode a control_authority message into a struct
 *
 * @param msg The message to decode
 * @param control_authority C-struct to decode the message contents into
 */
static inline void mavlink_msg_control_authority_decode(const mavlink_message_t* msg, mavlink_control_authority_t* control_authority)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    control_authority->time_boot_ms = mavlink_msg_control_authority_get_time_boot_ms(msg);
    control_authority->authority_timeout_ms = mavlink_msg_control_authority_get_authority_timeout_ms(msg);
    control_authority->control_source = mavlink_msg_control_authority_get_control_source(msg);
    control_authority->rc_available = mavlink_msg_control_authority_get_rc_available(msg);
    control_authority->autonomy_available = mavlink_msg_control_authority_get_autonomy_available(msg);
    control_authority->failsafe_active = mavlink_msg_control_authority_get_failsafe_active(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN? msg->len : MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN;
        memset(control_authority, 0, MAVLINK_MSG_ID_CONTROL_AUTHORITY_LEN);
    memcpy(control_authority, _MAV_PAYLOAD(msg), len);
#endif
}
