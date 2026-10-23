#pragma once
// MESSAGE BUS_GRANT PACKING

#define MAVLINK_MSG_ID_BUS_GRANT 200007


typedef struct __mavlink_bus_grant_t {
 uint32_t duration_us; /*< [us] Maximum time for which the recipient may drive the bus after receiving this grant.*/
 uint16_t max_bytes; /*< [bytes] Maximum encoded MAVLink bytes the recipient may transmit during this grant.*/
 uint16_t stream_flags; /*<  Traffic classes the recipient may transmit during this grant.*/
 uint8_t target_system; /*<  System ID of the intended recipient.*/
 uint8_t target_component; /*<  Component ID of the intended recipient.*/
 uint8_t max_frames; /*<  Maximum number of complete MAVLink frames the recipient may transmit during this grant.*/
} mavlink_bus_grant_t;

#define MAVLINK_MSG_ID_BUS_GRANT_LEN 11
#define MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN 11
#define MAVLINK_MSG_ID_200007_LEN 11
#define MAVLINK_MSG_ID_200007_MIN_LEN 11

#define MAVLINK_MSG_ID_BUS_GRANT_CRC 121
#define MAVLINK_MSG_ID_200007_CRC 121



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_BUS_GRANT { \
    200007, \
    "BUS_GRANT", \
    6, \
    {  { "duration_us", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_bus_grant_t, duration_us) }, \
         { "max_bytes", NULL, MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_bus_grant_t, max_bytes) }, \
         { "stream_flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 6, offsetof(mavlink_bus_grant_t, stream_flags) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_bus_grant_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_bus_grant_t, target_component) }, \
         { "max_frames", NULL, MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_bus_grant_t, max_frames) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_BUS_GRANT { \
    "BUS_GRANT", \
    6, \
    {  { "duration_us", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_bus_grant_t, duration_us) }, \
         { "max_bytes", NULL, MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_bus_grant_t, max_bytes) }, \
         { "stream_flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 6, offsetof(mavlink_bus_grant_t, stream_flags) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_bus_grant_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_bus_grant_t, target_component) }, \
         { "max_frames", NULL, MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_bus_grant_t, max_frames) }, \
         } \
}
#endif

/**
 * @brief Pack a bus_grant message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param duration_us [us] Maximum time for which the recipient may drive the bus after receiving this grant.
 * @param max_bytes [bytes] Maximum encoded MAVLink bytes the recipient may transmit during this grant.
 * @param stream_flags  Traffic classes the recipient may transmit during this grant.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param max_frames  Maximum number of complete MAVLink frames the recipient may transmit during this grant.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_bus_grant_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t duration_us, uint16_t max_bytes, uint16_t stream_flags, uint8_t target_system, uint8_t target_component, uint8_t max_frames)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BUS_GRANT_LEN];
    _mav_put_uint32_t(buf, 0, duration_us);
    _mav_put_uint16_t(buf, 4, max_bytes);
    _mav_put_uint16_t(buf, 6, stream_flags);
    _mav_put_uint8_t(buf, 8, target_system);
    _mav_put_uint8_t(buf, 9, target_component);
    _mav_put_uint8_t(buf, 10, max_frames);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_BUS_GRANT_LEN);
#else
    mavlink_bus_grant_t packet;
    packet.duration_us = duration_us;
    packet.max_bytes = max_bytes;
    packet.stream_flags = stream_flags;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.max_frames = max_frames;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_BUS_GRANT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_BUS_GRANT;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN, MAVLINK_MSG_ID_BUS_GRANT_LEN, MAVLINK_MSG_ID_BUS_GRANT_CRC);
}

/**
 * @brief Pack a bus_grant message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param duration_us [us] Maximum time for which the recipient may drive the bus after receiving this grant.
 * @param max_bytes [bytes] Maximum encoded MAVLink bytes the recipient may transmit during this grant.
 * @param stream_flags  Traffic classes the recipient may transmit during this grant.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param max_frames  Maximum number of complete MAVLink frames the recipient may transmit during this grant.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_bus_grant_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t duration_us, uint16_t max_bytes, uint16_t stream_flags, uint8_t target_system, uint8_t target_component, uint8_t max_frames)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BUS_GRANT_LEN];
    _mav_put_uint32_t(buf, 0, duration_us);
    _mav_put_uint16_t(buf, 4, max_bytes);
    _mav_put_uint16_t(buf, 6, stream_flags);
    _mav_put_uint8_t(buf, 8, target_system);
    _mav_put_uint8_t(buf, 9, target_component);
    _mav_put_uint8_t(buf, 10, max_frames);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_BUS_GRANT_LEN);
#else
    mavlink_bus_grant_t packet;
    packet.duration_us = duration_us;
    packet.max_bytes = max_bytes;
    packet.stream_flags = stream_flags;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.max_frames = max_frames;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_BUS_GRANT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_BUS_GRANT;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN, MAVLINK_MSG_ID_BUS_GRANT_LEN, MAVLINK_MSG_ID_BUS_GRANT_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN, MAVLINK_MSG_ID_BUS_GRANT_LEN);
#endif
}

/**
 * @brief Pack a bus_grant message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param duration_us [us] Maximum time for which the recipient may drive the bus after receiving this grant.
 * @param max_bytes [bytes] Maximum encoded MAVLink bytes the recipient may transmit during this grant.
 * @param stream_flags  Traffic classes the recipient may transmit during this grant.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param max_frames  Maximum number of complete MAVLink frames the recipient may transmit during this grant.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_bus_grant_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t duration_us,uint16_t max_bytes,uint16_t stream_flags,uint8_t target_system,uint8_t target_component,uint8_t max_frames)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BUS_GRANT_LEN];
    _mav_put_uint32_t(buf, 0, duration_us);
    _mav_put_uint16_t(buf, 4, max_bytes);
    _mav_put_uint16_t(buf, 6, stream_flags);
    _mav_put_uint8_t(buf, 8, target_system);
    _mav_put_uint8_t(buf, 9, target_component);
    _mav_put_uint8_t(buf, 10, max_frames);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_BUS_GRANT_LEN);
#else
    mavlink_bus_grant_t packet;
    packet.duration_us = duration_us;
    packet.max_bytes = max_bytes;
    packet.stream_flags = stream_flags;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.max_frames = max_frames;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_BUS_GRANT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_BUS_GRANT;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN, MAVLINK_MSG_ID_BUS_GRANT_LEN, MAVLINK_MSG_ID_BUS_GRANT_CRC);
}

/**
 * @brief Encode a bus_grant struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param bus_grant C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_bus_grant_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_bus_grant_t* bus_grant)
{
    return mavlink_msg_bus_grant_pack(system_id, component_id, msg, bus_grant->duration_us, bus_grant->max_bytes, bus_grant->stream_flags, bus_grant->target_system, bus_grant->target_component, bus_grant->max_frames);
}

/**
 * @brief Encode a bus_grant struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param bus_grant C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_bus_grant_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_bus_grant_t* bus_grant)
{
    return mavlink_msg_bus_grant_pack_chan(system_id, component_id, chan, msg, bus_grant->duration_us, bus_grant->max_bytes, bus_grant->stream_flags, bus_grant->target_system, bus_grant->target_component, bus_grant->max_frames);
}

/**
 * @brief Encode a bus_grant struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param bus_grant C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_bus_grant_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_bus_grant_t* bus_grant)
{
    return mavlink_msg_bus_grant_pack_status(system_id, component_id, _status, msg,  bus_grant->duration_us, bus_grant->max_bytes, bus_grant->stream_flags, bus_grant->target_system, bus_grant->target_component, bus_grant->max_frames);
}

/**
 * @brief Send a bus_grant message
 * @param chan MAVLink channel to send the message
 *
 * @param duration_us [us] Maximum time for which the recipient may drive the bus after receiving this grant.
 * @param max_bytes [bytes] Maximum encoded MAVLink bytes the recipient may transmit during this grant.
 * @param stream_flags  Traffic classes the recipient may transmit during this grant.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param max_frames  Maximum number of complete MAVLink frames the recipient may transmit during this grant.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_bus_grant_send(mavlink_channel_t chan, uint32_t duration_us, uint16_t max_bytes, uint16_t stream_flags, uint8_t target_system, uint8_t target_component, uint8_t max_frames)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BUS_GRANT_LEN];
    _mav_put_uint32_t(buf, 0, duration_us);
    _mav_put_uint16_t(buf, 4, max_bytes);
    _mav_put_uint16_t(buf, 6, stream_flags);
    _mav_put_uint8_t(buf, 8, target_system);
    _mav_put_uint8_t(buf, 9, target_component);
    _mav_put_uint8_t(buf, 10, max_frames);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BUS_GRANT, buf, MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN, MAVLINK_MSG_ID_BUS_GRANT_LEN, MAVLINK_MSG_ID_BUS_GRANT_CRC);
#else
    mavlink_bus_grant_t packet;
    packet.duration_us = duration_us;
    packet.max_bytes = max_bytes;
    packet.stream_flags = stream_flags;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.max_frames = max_frames;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BUS_GRANT, (const char *)&packet, MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN, MAVLINK_MSG_ID_BUS_GRANT_LEN, MAVLINK_MSG_ID_BUS_GRANT_CRC);
#endif
}

/**
 * @brief Send a bus_grant message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_bus_grant_send_struct(mavlink_channel_t chan, const mavlink_bus_grant_t* bus_grant)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_bus_grant_send(chan, bus_grant->duration_us, bus_grant->max_bytes, bus_grant->stream_flags, bus_grant->target_system, bus_grant->target_component, bus_grant->max_frames);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BUS_GRANT, (const char *)bus_grant, MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN, MAVLINK_MSG_ID_BUS_GRANT_LEN, MAVLINK_MSG_ID_BUS_GRANT_CRC);
#endif
}

#if MAVLINK_MSG_ID_BUS_GRANT_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_bus_grant_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t duration_us, uint16_t max_bytes, uint16_t stream_flags, uint8_t target_system, uint8_t target_component, uint8_t max_frames)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, duration_us);
    _mav_put_uint16_t(buf, 4, max_bytes);
    _mav_put_uint16_t(buf, 6, stream_flags);
    _mav_put_uint8_t(buf, 8, target_system);
    _mav_put_uint8_t(buf, 9, target_component);
    _mav_put_uint8_t(buf, 10, max_frames);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BUS_GRANT, buf, MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN, MAVLINK_MSG_ID_BUS_GRANT_LEN, MAVLINK_MSG_ID_BUS_GRANT_CRC);
#else
    mavlink_bus_grant_t *packet = (mavlink_bus_grant_t *)msgbuf;
    packet->duration_us = duration_us;
    packet->max_bytes = max_bytes;
    packet->stream_flags = stream_flags;
    packet->target_system = target_system;
    packet->target_component = target_component;
    packet->max_frames = max_frames;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BUS_GRANT, (const char *)packet, MAVLINK_MSG_ID_BUS_GRANT_MIN_LEN, MAVLINK_MSG_ID_BUS_GRANT_LEN, MAVLINK_MSG_ID_BUS_GRANT_CRC);
#endif
}
#endif

#endif

// MESSAGE BUS_GRANT UNPACKING


/**
 * @brief Get field duration_us from bus_grant message
 *
 * @return [us] Maximum time for which the recipient may drive the bus after receiving this grant.
 */
static inline uint32_t mavlink_msg_bus_grant_get_duration_us(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field max_bytes from bus_grant message
 *
 * @return [bytes] Maximum encoded MAVLink bytes the recipient may transmit during this grant.
 */
static inline uint16_t mavlink_msg_bus_grant_get_max_bytes(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  4);
}

/**
 * @brief Get field stream_flags from bus_grant message
 *
 * @return  Traffic classes the recipient may transmit during this grant.
 */
static inline uint16_t mavlink_msg_bus_grant_get_stream_flags(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  6);
}

/**
 * @brief Get field target_system from bus_grant message
 *
 * @return  System ID of the intended recipient.
 */
static inline uint8_t mavlink_msg_bus_grant_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  8);
}

/**
 * @brief Get field target_component from bus_grant message
 *
 * @return  Component ID of the intended recipient.
 */
static inline uint8_t mavlink_msg_bus_grant_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  9);
}

/**
 * @brief Get field max_frames from bus_grant message
 *
 * @return  Maximum number of complete MAVLink frames the recipient may transmit during this grant.
 */
static inline uint8_t mavlink_msg_bus_grant_get_max_frames(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  10);
}

/**
 * @brief Decode a bus_grant message into a struct
 *
 * @param msg The message to decode
 * @param bus_grant C-struct to decode the message contents into
 */
static inline void mavlink_msg_bus_grant_decode(const mavlink_message_t* msg, mavlink_bus_grant_t* bus_grant)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    bus_grant->duration_us = mavlink_msg_bus_grant_get_duration_us(msg);
    bus_grant->max_bytes = mavlink_msg_bus_grant_get_max_bytes(msg);
    bus_grant->stream_flags = mavlink_msg_bus_grant_get_stream_flags(msg);
    bus_grant->target_system = mavlink_msg_bus_grant_get_target_system(msg);
    bus_grant->target_component = mavlink_msg_bus_grant_get_target_component(msg);
    bus_grant->max_frames = mavlink_msg_bus_grant_get_max_frames(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_BUS_GRANT_LEN? msg->len : MAVLINK_MSG_ID_BUS_GRANT_LEN;
        memset(bus_grant, 0, MAVLINK_MSG_ID_BUS_GRANT_LEN);
    memcpy(bus_grant, _MAV_PAYLOAD(msg), len);
#endif
}
