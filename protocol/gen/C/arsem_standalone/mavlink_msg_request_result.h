#pragma once
// MESSAGE REQUEST_RESULT PACKING

#define MAVLINK_MSG_ID_REQUEST_RESULT 200010


typedef struct __mavlink_request_result_t {
 uint32_t ground_session; /*<  ground_session from the tracked request.*/
 uint32_t request_id; /*<  request_id from the tracked request.*/
 uint32_t motor_boot_id; /*<  boot_id under which the motor node processed or rejected the request. A different boot_id means the request's outcome is unknown.*/
 uint32_t local_command_id; /*<  Motor-local command identifier of the accepted action, for cross-reference with ACTUATOR_STATUS.command_id. Zero when no action was accepted.*/
 uint8_t result; /*<  Correlation outcome of the request.*/
 uint8_t state; /*<  Motor-side state observation associated with this outcome.*/
 uint8_t target_system; /*<  System ID of the originating ground client.*/
 uint8_t target_component; /*<  Component ID of the originating ground client.*/
} mavlink_request_result_t;

#define MAVLINK_MSG_ID_REQUEST_RESULT_LEN 20
#define MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN 20
#define MAVLINK_MSG_ID_200010_LEN 20
#define MAVLINK_MSG_ID_200010_MIN_LEN 20

#define MAVLINK_MSG_ID_REQUEST_RESULT_CRC 56
#define MAVLINK_MSG_ID_200010_CRC 56



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_REQUEST_RESULT { \
    200010, \
    "REQUEST_RESULT", \
    8, \
    {  { "ground_session", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_request_result_t, ground_session) }, \
         { "request_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_request_result_t, request_id) }, \
         { "motor_boot_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_request_result_t, motor_boot_id) }, \
         { "local_command_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 12, offsetof(mavlink_request_result_t, local_command_id) }, \
         { "result", NULL, MAVLINK_TYPE_UINT8_T, 0, 16, offsetof(mavlink_request_result_t, result) }, \
         { "state", NULL, MAVLINK_TYPE_UINT8_T, 0, 17, offsetof(mavlink_request_result_t, state) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 18, offsetof(mavlink_request_result_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 19, offsetof(mavlink_request_result_t, target_component) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_REQUEST_RESULT { \
    "REQUEST_RESULT", \
    8, \
    {  { "ground_session", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_request_result_t, ground_session) }, \
         { "request_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_request_result_t, request_id) }, \
         { "motor_boot_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_request_result_t, motor_boot_id) }, \
         { "local_command_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 12, offsetof(mavlink_request_result_t, local_command_id) }, \
         { "result", NULL, MAVLINK_TYPE_UINT8_T, 0, 16, offsetof(mavlink_request_result_t, result) }, \
         { "state", NULL, MAVLINK_TYPE_UINT8_T, 0, 17, offsetof(mavlink_request_result_t, state) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 18, offsetof(mavlink_request_result_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 19, offsetof(mavlink_request_result_t, target_component) }, \
         } \
}
#endif

/**
 * @brief Pack a request_result message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param ground_session  ground_session from the tracked request.
 * @param request_id  request_id from the tracked request.
 * @param motor_boot_id  boot_id under which the motor node processed or rejected the request. A different boot_id means the request's outcome is unknown.
 * @param local_command_id  Motor-local command identifier of the accepted action, for cross-reference with ACTUATOR_STATUS.command_id. Zero when no action was accepted.
 * @param result  Correlation outcome of the request.
 * @param state  Motor-side state observation associated with this outcome.
 * @param target_system  System ID of the originating ground client.
 * @param target_component  Component ID of the originating ground client.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_request_result_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t ground_session, uint32_t request_id, uint32_t motor_boot_id, uint32_t local_command_id, uint8_t result, uint8_t state, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_REQUEST_RESULT_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint32_t(buf, 8, motor_boot_id);
    _mav_put_uint32_t(buf, 12, local_command_id);
    _mav_put_uint8_t(buf, 16, result);
    _mav_put_uint8_t(buf, 17, state);
    _mav_put_uint8_t(buf, 18, target_system);
    _mav_put_uint8_t(buf, 19, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_REQUEST_RESULT_LEN);
#else
    mavlink_request_result_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.motor_boot_id = motor_boot_id;
    packet.local_command_id = local_command_id;
    packet.result = result;
    packet.state = state;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_REQUEST_RESULT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_REQUEST_RESULT;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_CRC);
}

/**
 * @brief Pack a request_result message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param ground_session  ground_session from the tracked request.
 * @param request_id  request_id from the tracked request.
 * @param motor_boot_id  boot_id under which the motor node processed or rejected the request. A different boot_id means the request's outcome is unknown.
 * @param local_command_id  Motor-local command identifier of the accepted action, for cross-reference with ACTUATOR_STATUS.command_id. Zero when no action was accepted.
 * @param result  Correlation outcome of the request.
 * @param state  Motor-side state observation associated with this outcome.
 * @param target_system  System ID of the originating ground client.
 * @param target_component  Component ID of the originating ground client.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_request_result_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t ground_session, uint32_t request_id, uint32_t motor_boot_id, uint32_t local_command_id, uint8_t result, uint8_t state, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_REQUEST_RESULT_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint32_t(buf, 8, motor_boot_id);
    _mav_put_uint32_t(buf, 12, local_command_id);
    _mav_put_uint8_t(buf, 16, result);
    _mav_put_uint8_t(buf, 17, state);
    _mav_put_uint8_t(buf, 18, target_system);
    _mav_put_uint8_t(buf, 19, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_REQUEST_RESULT_LEN);
#else
    mavlink_request_result_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.motor_boot_id = motor_boot_id;
    packet.local_command_id = local_command_id;
    packet.result = result;
    packet.state = state;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_REQUEST_RESULT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_REQUEST_RESULT;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_LEN);
#endif
}

/**
 * @brief Pack a request_result message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param ground_session  ground_session from the tracked request.
 * @param request_id  request_id from the tracked request.
 * @param motor_boot_id  boot_id under which the motor node processed or rejected the request. A different boot_id means the request's outcome is unknown.
 * @param local_command_id  Motor-local command identifier of the accepted action, for cross-reference with ACTUATOR_STATUS.command_id. Zero when no action was accepted.
 * @param result  Correlation outcome of the request.
 * @param state  Motor-side state observation associated with this outcome.
 * @param target_system  System ID of the originating ground client.
 * @param target_component  Component ID of the originating ground client.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_request_result_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t ground_session,uint32_t request_id,uint32_t motor_boot_id,uint32_t local_command_id,uint8_t result,uint8_t state,uint8_t target_system,uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_REQUEST_RESULT_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint32_t(buf, 8, motor_boot_id);
    _mav_put_uint32_t(buf, 12, local_command_id);
    _mav_put_uint8_t(buf, 16, result);
    _mav_put_uint8_t(buf, 17, state);
    _mav_put_uint8_t(buf, 18, target_system);
    _mav_put_uint8_t(buf, 19, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_REQUEST_RESULT_LEN);
#else
    mavlink_request_result_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.motor_boot_id = motor_boot_id;
    packet.local_command_id = local_command_id;
    packet.result = result;
    packet.state = state;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_REQUEST_RESULT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_REQUEST_RESULT;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_CRC);
}

/**
 * @brief Encode a request_result struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param request_result C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_request_result_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_request_result_t* request_result)
{
    return mavlink_msg_request_result_pack(system_id, component_id, msg, request_result->ground_session, request_result->request_id, request_result->motor_boot_id, request_result->local_command_id, request_result->result, request_result->state, request_result->target_system, request_result->target_component);
}

/**
 * @brief Encode a request_result struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param request_result C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_request_result_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_request_result_t* request_result)
{
    return mavlink_msg_request_result_pack_chan(system_id, component_id, chan, msg, request_result->ground_session, request_result->request_id, request_result->motor_boot_id, request_result->local_command_id, request_result->result, request_result->state, request_result->target_system, request_result->target_component);
}

/**
 * @brief Encode a request_result struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param request_result C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_request_result_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_request_result_t* request_result)
{
    return mavlink_msg_request_result_pack_status(system_id, component_id, _status, msg,  request_result->ground_session, request_result->request_id, request_result->motor_boot_id, request_result->local_command_id, request_result->result, request_result->state, request_result->target_system, request_result->target_component);
}

/**
 * @brief Send a request_result message
 * @param chan MAVLink channel to send the message
 *
 * @param ground_session  ground_session from the tracked request.
 * @param request_id  request_id from the tracked request.
 * @param motor_boot_id  boot_id under which the motor node processed or rejected the request. A different boot_id means the request's outcome is unknown.
 * @param local_command_id  Motor-local command identifier of the accepted action, for cross-reference with ACTUATOR_STATUS.command_id. Zero when no action was accepted.
 * @param result  Correlation outcome of the request.
 * @param state  Motor-side state observation associated with this outcome.
 * @param target_system  System ID of the originating ground client.
 * @param target_component  Component ID of the originating ground client.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_request_result_send(mavlink_channel_t chan, uint32_t ground_session, uint32_t request_id, uint32_t motor_boot_id, uint32_t local_command_id, uint8_t result, uint8_t state, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_REQUEST_RESULT_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint32_t(buf, 8, motor_boot_id);
    _mav_put_uint32_t(buf, 12, local_command_id);
    _mav_put_uint8_t(buf, 16, result);
    _mav_put_uint8_t(buf, 17, state);
    _mav_put_uint8_t(buf, 18, target_system);
    _mav_put_uint8_t(buf, 19, target_component);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REQUEST_RESULT, buf, MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_CRC);
#else
    mavlink_request_result_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.motor_boot_id = motor_boot_id;
    packet.local_command_id = local_command_id;
    packet.result = result;
    packet.state = state;
    packet.target_system = target_system;
    packet.target_component = target_component;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REQUEST_RESULT, (const char *)&packet, MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_CRC);
#endif
}

/**
 * @brief Send a request_result message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_request_result_send_struct(mavlink_channel_t chan, const mavlink_request_result_t* request_result)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_request_result_send(chan, request_result->ground_session, request_result->request_id, request_result->motor_boot_id, request_result->local_command_id, request_result->result, request_result->state, request_result->target_system, request_result->target_component);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REQUEST_RESULT, (const char *)request_result, MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_CRC);
#endif
}

#if MAVLINK_MSG_ID_REQUEST_RESULT_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_request_result_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t ground_session, uint32_t request_id, uint32_t motor_boot_id, uint32_t local_command_id, uint8_t result, uint8_t state, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint32_t(buf, 8, motor_boot_id);
    _mav_put_uint32_t(buf, 12, local_command_id);
    _mav_put_uint8_t(buf, 16, result);
    _mav_put_uint8_t(buf, 17, state);
    _mav_put_uint8_t(buf, 18, target_system);
    _mav_put_uint8_t(buf, 19, target_component);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REQUEST_RESULT, buf, MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_CRC);
#else
    mavlink_request_result_t *packet = (mavlink_request_result_t *)msgbuf;
    packet->ground_session = ground_session;
    packet->request_id = request_id;
    packet->motor_boot_id = motor_boot_id;
    packet->local_command_id = local_command_id;
    packet->result = result;
    packet->state = state;
    packet->target_system = target_system;
    packet->target_component = target_component;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REQUEST_RESULT, (const char *)packet, MAVLINK_MSG_ID_REQUEST_RESULT_MIN_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_LEN, MAVLINK_MSG_ID_REQUEST_RESULT_CRC);
#endif
}
#endif

#endif

// MESSAGE REQUEST_RESULT UNPACKING


/**
 * @brief Get field ground_session from request_result message
 *
 * @return  ground_session from the tracked request.
 */
static inline uint32_t mavlink_msg_request_result_get_ground_session(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field request_id from request_result message
 *
 * @return  request_id from the tracked request.
 */
static inline uint32_t mavlink_msg_request_result_get_request_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  4);
}

/**
 * @brief Get field motor_boot_id from request_result message
 *
 * @return  boot_id under which the motor node processed or rejected the request. A different boot_id means the request's outcome is unknown.
 */
static inline uint32_t mavlink_msg_request_result_get_motor_boot_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  8);
}

/**
 * @brief Get field local_command_id from request_result message
 *
 * @return  Motor-local command identifier of the accepted action, for cross-reference with ACTUATOR_STATUS.command_id. Zero when no action was accepted.
 */
static inline uint32_t mavlink_msg_request_result_get_local_command_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  12);
}

/**
 * @brief Get field result from request_result message
 *
 * @return  Correlation outcome of the request.
 */
static inline uint8_t mavlink_msg_request_result_get_result(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  16);
}

/**
 * @brief Get field state from request_result message
 *
 * @return  Motor-side state observation associated with this outcome.
 */
static inline uint8_t mavlink_msg_request_result_get_state(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  17);
}

/**
 * @brief Get field target_system from request_result message
 *
 * @return  System ID of the originating ground client.
 */
static inline uint8_t mavlink_msg_request_result_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  18);
}

/**
 * @brief Get field target_component from request_result message
 *
 * @return  Component ID of the originating ground client.
 */
static inline uint8_t mavlink_msg_request_result_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  19);
}

/**
 * @brief Decode a request_result message into a struct
 *
 * @param msg The message to decode
 * @param request_result C-struct to decode the message contents into
 */
static inline void mavlink_msg_request_result_decode(const mavlink_message_t* msg, mavlink_request_result_t* request_result)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    request_result->ground_session = mavlink_msg_request_result_get_ground_session(msg);
    request_result->request_id = mavlink_msg_request_result_get_request_id(msg);
    request_result->motor_boot_id = mavlink_msg_request_result_get_motor_boot_id(msg);
    request_result->local_command_id = mavlink_msg_request_result_get_local_command_id(msg);
    request_result->result = mavlink_msg_request_result_get_result(msg);
    request_result->state = mavlink_msg_request_result_get_state(msg);
    request_result->target_system = mavlink_msg_request_result_get_target_system(msg);
    request_result->target_component = mavlink_msg_request_result_get_target_component(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_REQUEST_RESULT_LEN? msg->len : MAVLINK_MSG_ID_REQUEST_RESULT_LEN;
        memset(request_result, 0, MAVLINK_MSG_ID_REQUEST_RESULT_LEN);
    memcpy(request_result, _MAV_PAYLOAD(msg), len);
#endif
}
