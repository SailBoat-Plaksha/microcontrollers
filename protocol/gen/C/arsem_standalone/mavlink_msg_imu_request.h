#pragma once
// MESSAGE IMU_REQUEST PACKING

#define MAVLINK_MSG_ID_IMU_REQUEST 200011


typedef struct __mavlink_imu_request_t {
 uint32_t ground_session; /*<  Ground session; zero for board-local requests.*/
 uint32_t request_id; /*<  Monotonic request identifier within the session.*/
 uint16_t timeout_ms; /*< [ms] Bounded request deadline, 1..5000 ms.*/
 uint16_t max_age_ms; /*< [ms] Maximum cached sample age; zero requires a new sensor read.*/
 uint8_t path; /*<  Requested response path.*/
 uint8_t target_system; /*<  Recipient system.*/
 uint8_t target_component; /*<  Recipient component.*/
} mavlink_imu_request_t;

#define MAVLINK_MSG_ID_IMU_REQUEST_LEN 15
#define MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN 15
#define MAVLINK_MSG_ID_200011_LEN 15
#define MAVLINK_MSG_ID_200011_MIN_LEN 15

#define MAVLINK_MSG_ID_IMU_REQUEST_CRC 71
#define MAVLINK_MSG_ID_200011_CRC 71



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_IMU_REQUEST { \
    200011, \
    "IMU_REQUEST", \
    7, \
    {  { "ground_session", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_imu_request_t, ground_session) }, \
         { "request_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_imu_request_t, request_id) }, \
         { "timeout_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_imu_request_t, timeout_ms) }, \
         { "max_age_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 10, offsetof(mavlink_imu_request_t, max_age_ms) }, \
         { "path", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_imu_request_t, path) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_imu_request_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 14, offsetof(mavlink_imu_request_t, target_component) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_IMU_REQUEST { \
    "IMU_REQUEST", \
    7, \
    {  { "ground_session", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_imu_request_t, ground_session) }, \
         { "request_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_imu_request_t, request_id) }, \
         { "timeout_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_imu_request_t, timeout_ms) }, \
         { "max_age_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 10, offsetof(mavlink_imu_request_t, max_age_ms) }, \
         { "path", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_imu_request_t, path) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_imu_request_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 14, offsetof(mavlink_imu_request_t, target_component) }, \
         } \
}
#endif

/**
 * @brief Pack a imu_request message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param ground_session  Ground session; zero for board-local requests.
 * @param request_id  Monotonic request identifier within the session.
 * @param timeout_ms [ms] Bounded request deadline, 1..5000 ms.
 * @param max_age_ms [ms] Maximum cached sample age; zero requires a new sensor read.
 * @param path  Requested response path.
 * @param target_system  Recipient system.
 * @param target_component  Recipient component.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_imu_request_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t ground_session, uint32_t request_id, uint16_t timeout_ms, uint16_t max_age_ms, uint8_t path, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_IMU_REQUEST_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint16_t(buf, 8, timeout_ms);
    _mav_put_uint16_t(buf, 10, max_age_ms);
    _mav_put_uint8_t(buf, 12, path);
    _mav_put_uint8_t(buf, 13, target_system);
    _mav_put_uint8_t(buf, 14, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_IMU_REQUEST_LEN);
#else
    mavlink_imu_request_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.timeout_ms = timeout_ms;
    packet.max_age_ms = max_age_ms;
    packet.path = path;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_IMU_REQUEST_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_IMU_REQUEST;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN, MAVLINK_MSG_ID_IMU_REQUEST_LEN, MAVLINK_MSG_ID_IMU_REQUEST_CRC);
}

/**
 * @brief Pack a imu_request message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param ground_session  Ground session; zero for board-local requests.
 * @param request_id  Monotonic request identifier within the session.
 * @param timeout_ms [ms] Bounded request deadline, 1..5000 ms.
 * @param max_age_ms [ms] Maximum cached sample age; zero requires a new sensor read.
 * @param path  Requested response path.
 * @param target_system  Recipient system.
 * @param target_component  Recipient component.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_imu_request_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t ground_session, uint32_t request_id, uint16_t timeout_ms, uint16_t max_age_ms, uint8_t path, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_IMU_REQUEST_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint16_t(buf, 8, timeout_ms);
    _mav_put_uint16_t(buf, 10, max_age_ms);
    _mav_put_uint8_t(buf, 12, path);
    _mav_put_uint8_t(buf, 13, target_system);
    _mav_put_uint8_t(buf, 14, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_IMU_REQUEST_LEN);
#else
    mavlink_imu_request_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.timeout_ms = timeout_ms;
    packet.max_age_ms = max_age_ms;
    packet.path = path;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_IMU_REQUEST_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_IMU_REQUEST;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN, MAVLINK_MSG_ID_IMU_REQUEST_LEN, MAVLINK_MSG_ID_IMU_REQUEST_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN, MAVLINK_MSG_ID_IMU_REQUEST_LEN);
#endif
}

/**
 * @brief Pack a imu_request message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param ground_session  Ground session; zero for board-local requests.
 * @param request_id  Monotonic request identifier within the session.
 * @param timeout_ms [ms] Bounded request deadline, 1..5000 ms.
 * @param max_age_ms [ms] Maximum cached sample age; zero requires a new sensor read.
 * @param path  Requested response path.
 * @param target_system  Recipient system.
 * @param target_component  Recipient component.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_imu_request_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t ground_session,uint32_t request_id,uint16_t timeout_ms,uint16_t max_age_ms,uint8_t path,uint8_t target_system,uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_IMU_REQUEST_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint16_t(buf, 8, timeout_ms);
    _mav_put_uint16_t(buf, 10, max_age_ms);
    _mav_put_uint8_t(buf, 12, path);
    _mav_put_uint8_t(buf, 13, target_system);
    _mav_put_uint8_t(buf, 14, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_IMU_REQUEST_LEN);
#else
    mavlink_imu_request_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.timeout_ms = timeout_ms;
    packet.max_age_ms = max_age_ms;
    packet.path = path;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_IMU_REQUEST_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_IMU_REQUEST;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN, MAVLINK_MSG_ID_IMU_REQUEST_LEN, MAVLINK_MSG_ID_IMU_REQUEST_CRC);
}

/**
 * @brief Encode a imu_request struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param imu_request C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_imu_request_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_imu_request_t* imu_request)
{
    return mavlink_msg_imu_request_pack(system_id, component_id, msg, imu_request->ground_session, imu_request->request_id, imu_request->timeout_ms, imu_request->max_age_ms, imu_request->path, imu_request->target_system, imu_request->target_component);
}

/**
 * @brief Encode a imu_request struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param imu_request C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_imu_request_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_imu_request_t* imu_request)
{
    return mavlink_msg_imu_request_pack_chan(system_id, component_id, chan, msg, imu_request->ground_session, imu_request->request_id, imu_request->timeout_ms, imu_request->max_age_ms, imu_request->path, imu_request->target_system, imu_request->target_component);
}

/**
 * @brief Encode a imu_request struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param imu_request C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_imu_request_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_imu_request_t* imu_request)
{
    return mavlink_msg_imu_request_pack_status(system_id, component_id, _status, msg,  imu_request->ground_session, imu_request->request_id, imu_request->timeout_ms, imu_request->max_age_ms, imu_request->path, imu_request->target_system, imu_request->target_component);
}

/**
 * @brief Send a imu_request message
 * @param chan MAVLink channel to send the message
 *
 * @param ground_session  Ground session; zero for board-local requests.
 * @param request_id  Monotonic request identifier within the session.
 * @param timeout_ms [ms] Bounded request deadline, 1..5000 ms.
 * @param max_age_ms [ms] Maximum cached sample age; zero requires a new sensor read.
 * @param path  Requested response path.
 * @param target_system  Recipient system.
 * @param target_component  Recipient component.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_imu_request_send(mavlink_channel_t chan, uint32_t ground_session, uint32_t request_id, uint16_t timeout_ms, uint16_t max_age_ms, uint8_t path, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_IMU_REQUEST_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint16_t(buf, 8, timeout_ms);
    _mav_put_uint16_t(buf, 10, max_age_ms);
    _mav_put_uint8_t(buf, 12, path);
    _mav_put_uint8_t(buf, 13, target_system);
    _mav_put_uint8_t(buf, 14, target_component);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_IMU_REQUEST, buf, MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN, MAVLINK_MSG_ID_IMU_REQUEST_LEN, MAVLINK_MSG_ID_IMU_REQUEST_CRC);
#else
    mavlink_imu_request_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.timeout_ms = timeout_ms;
    packet.max_age_ms = max_age_ms;
    packet.path = path;
    packet.target_system = target_system;
    packet.target_component = target_component;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_IMU_REQUEST, (const char *)&packet, MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN, MAVLINK_MSG_ID_IMU_REQUEST_LEN, MAVLINK_MSG_ID_IMU_REQUEST_CRC);
#endif
}

/**
 * @brief Send a imu_request message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_imu_request_send_struct(mavlink_channel_t chan, const mavlink_imu_request_t* imu_request)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_imu_request_send(chan, imu_request->ground_session, imu_request->request_id, imu_request->timeout_ms, imu_request->max_age_ms, imu_request->path, imu_request->target_system, imu_request->target_component);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_IMU_REQUEST, (const char *)imu_request, MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN, MAVLINK_MSG_ID_IMU_REQUEST_LEN, MAVLINK_MSG_ID_IMU_REQUEST_CRC);
#endif
}

#if MAVLINK_MSG_ID_IMU_REQUEST_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_imu_request_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t ground_session, uint32_t request_id, uint16_t timeout_ms, uint16_t max_age_ms, uint8_t path, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint16_t(buf, 8, timeout_ms);
    _mav_put_uint16_t(buf, 10, max_age_ms);
    _mav_put_uint8_t(buf, 12, path);
    _mav_put_uint8_t(buf, 13, target_system);
    _mav_put_uint8_t(buf, 14, target_component);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_IMU_REQUEST, buf, MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN, MAVLINK_MSG_ID_IMU_REQUEST_LEN, MAVLINK_MSG_ID_IMU_REQUEST_CRC);
#else
    mavlink_imu_request_t *packet = (mavlink_imu_request_t *)msgbuf;
    packet->ground_session = ground_session;
    packet->request_id = request_id;
    packet->timeout_ms = timeout_ms;
    packet->max_age_ms = max_age_ms;
    packet->path = path;
    packet->target_system = target_system;
    packet->target_component = target_component;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_IMU_REQUEST, (const char *)packet, MAVLINK_MSG_ID_IMU_REQUEST_MIN_LEN, MAVLINK_MSG_ID_IMU_REQUEST_LEN, MAVLINK_MSG_ID_IMU_REQUEST_CRC);
#endif
}
#endif

#endif

// MESSAGE IMU_REQUEST UNPACKING


/**
 * @brief Get field ground_session from imu_request message
 *
 * @return  Ground session; zero for board-local requests.
 */
static inline uint32_t mavlink_msg_imu_request_get_ground_session(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field request_id from imu_request message
 *
 * @return  Monotonic request identifier within the session.
 */
static inline uint32_t mavlink_msg_imu_request_get_request_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  4);
}

/**
 * @brief Get field timeout_ms from imu_request message
 *
 * @return [ms] Bounded request deadline, 1..5000 ms.
 */
static inline uint16_t mavlink_msg_imu_request_get_timeout_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  8);
}

/**
 * @brief Get field max_age_ms from imu_request message
 *
 * @return [ms] Maximum cached sample age; zero requires a new sensor read.
 */
static inline uint16_t mavlink_msg_imu_request_get_max_age_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  10);
}

/**
 * @brief Get field path from imu_request message
 *
 * @return  Requested response path.
 */
static inline uint8_t mavlink_msg_imu_request_get_path(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  12);
}

/**
 * @brief Get field target_system from imu_request message
 *
 * @return  Recipient system.
 */
static inline uint8_t mavlink_msg_imu_request_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  13);
}

/**
 * @brief Get field target_component from imu_request message
 *
 * @return  Recipient component.
 */
static inline uint8_t mavlink_msg_imu_request_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  14);
}

/**
 * @brief Decode a imu_request message into a struct
 *
 * @param msg The message to decode
 * @param imu_request C-struct to decode the message contents into
 */
static inline void mavlink_msg_imu_request_decode(const mavlink_message_t* msg, mavlink_imu_request_t* imu_request)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    imu_request->ground_session = mavlink_msg_imu_request_get_ground_session(msg);
    imu_request->request_id = mavlink_msg_imu_request_get_request_id(msg);
    imu_request->timeout_ms = mavlink_msg_imu_request_get_timeout_ms(msg);
    imu_request->max_age_ms = mavlink_msg_imu_request_get_max_age_ms(msg);
    imu_request->path = mavlink_msg_imu_request_get_path(msg);
    imu_request->target_system = mavlink_msg_imu_request_get_target_system(msg);
    imu_request->target_component = mavlink_msg_imu_request_get_target_component(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_IMU_REQUEST_LEN? msg->len : MAVLINK_MSG_ID_IMU_REQUEST_LEN;
        memset(imu_request, 0, MAVLINK_MSG_ID_IMU_REQUEST_LEN);
    memcpy(imu_request, _MAV_PAYLOAD(msg), len);
#endif
}
