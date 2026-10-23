#pragma once
// MESSAGE ACTUATOR_REQUEST PACKING

#define MAVLINK_MSG_ID_ACTUATOR_REQUEST 200008


typedef struct __mavlink_actuator_request_t {
 uint32_t request_id; /*<  Ground-generated identifier unique within ground_session, echoed in REQUEST_RESULT for correlation. Monotonic per session where practical.*/
 uint32_t ground_session; /*<  Random per-connection ground session identifier. A reconnect selects a new value; queued movement carrying an old session is invalid.*/
 int16_t delta_cdeg; /*< [cdeg] Absolute target angle for MOVEMENT_OPCODE_ABSOLUTE, signed relative delta resolved once against valid fresh feedback for MOVEMENT_OPCODE_STEP, and zero for MOVEMENT_OPCODE_STOP.*/
 uint16_t timeout_ms; /*< [ms] Freshness timeout after reception. Zero uses the receiver's configured default.*/
 uint16_t max_rate_cdeg_s; /*< [cdeg/s] Maximum slew rate for this movement. Zero uses the receiver's configured default.*/
 uint8_t target_system; /*<  System ID of the intended recipient.*/
 uint8_t target_component; /*<  Component ID of the intended recipient.*/
 uint8_t actuator; /*<  Actuator to command.*/
 uint8_t opcode; /*<  Requested operation.*/
 uint8_t control_source; /*<  Authority under which this request was produced.*/
} mavlink_actuator_request_t;

#define MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN 19
#define MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN 19
#define MAVLINK_MSG_ID_200008_LEN 19
#define MAVLINK_MSG_ID_200008_MIN_LEN 19

#define MAVLINK_MSG_ID_ACTUATOR_REQUEST_CRC 188
#define MAVLINK_MSG_ID_200008_CRC 188



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ACTUATOR_REQUEST { \
    200008, \
    "ACTUATOR_REQUEST", \
    10, \
    {  { "request_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_actuator_request_t, request_id) }, \
         { "ground_session", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_actuator_request_t, ground_session) }, \
         { "delta_cdeg", NULL, MAVLINK_TYPE_INT16_T, 0, 8, offsetof(mavlink_actuator_request_t, delta_cdeg) }, \
         { "timeout_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 10, offsetof(mavlink_actuator_request_t, timeout_ms) }, \
         { "max_rate_cdeg_s", NULL, MAVLINK_TYPE_UINT16_T, 0, 12, offsetof(mavlink_actuator_request_t, max_rate_cdeg_s) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 14, offsetof(mavlink_actuator_request_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 15, offsetof(mavlink_actuator_request_t, target_component) }, \
         { "actuator", NULL, MAVLINK_TYPE_UINT8_T, 0, 16, offsetof(mavlink_actuator_request_t, actuator) }, \
         { "opcode", NULL, MAVLINK_TYPE_UINT8_T, 0, 17, offsetof(mavlink_actuator_request_t, opcode) }, \
         { "control_source", NULL, MAVLINK_TYPE_UINT8_T, 0, 18, offsetof(mavlink_actuator_request_t, control_source) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ACTUATOR_REQUEST { \
    "ACTUATOR_REQUEST", \
    10, \
    {  { "request_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_actuator_request_t, request_id) }, \
         { "ground_session", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_actuator_request_t, ground_session) }, \
         { "delta_cdeg", NULL, MAVLINK_TYPE_INT16_T, 0, 8, offsetof(mavlink_actuator_request_t, delta_cdeg) }, \
         { "timeout_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 10, offsetof(mavlink_actuator_request_t, timeout_ms) }, \
         { "max_rate_cdeg_s", NULL, MAVLINK_TYPE_UINT16_T, 0, 12, offsetof(mavlink_actuator_request_t, max_rate_cdeg_s) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 14, offsetof(mavlink_actuator_request_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 15, offsetof(mavlink_actuator_request_t, target_component) }, \
         { "actuator", NULL, MAVLINK_TYPE_UINT8_T, 0, 16, offsetof(mavlink_actuator_request_t, actuator) }, \
         { "opcode", NULL, MAVLINK_TYPE_UINT8_T, 0, 17, offsetof(mavlink_actuator_request_t, opcode) }, \
         { "control_source", NULL, MAVLINK_TYPE_UINT8_T, 0, 18, offsetof(mavlink_actuator_request_t, control_source) }, \
         } \
}
#endif

/**
 * @brief Pack a actuator_request message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param request_id  Ground-generated identifier unique within ground_session, echoed in REQUEST_RESULT for correlation. Monotonic per session where practical.
 * @param ground_session  Random per-connection ground session identifier. A reconnect selects a new value; queued movement carrying an old session is invalid.
 * @param delta_cdeg [cdeg] Absolute target angle for MOVEMENT_OPCODE_ABSOLUTE, signed relative delta resolved once against valid fresh feedback for MOVEMENT_OPCODE_STEP, and zero for MOVEMENT_OPCODE_STOP.
 * @param timeout_ms [ms] Freshness timeout after reception. Zero uses the receiver's configured default.
 * @param max_rate_cdeg_s [cdeg/s] Maximum slew rate for this movement. Zero uses the receiver's configured default.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param actuator  Actuator to command.
 * @param opcode  Requested operation.
 * @param control_source  Authority under which this request was produced.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_actuator_request_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t request_id, uint32_t ground_session, int16_t delta_cdeg, uint16_t timeout_ms, uint16_t max_rate_cdeg_s, uint8_t target_system, uint8_t target_component, uint8_t actuator, uint8_t opcode, uint8_t control_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN];
    _mav_put_uint32_t(buf, 0, request_id);
    _mav_put_uint32_t(buf, 4, ground_session);
    _mav_put_int16_t(buf, 8, delta_cdeg);
    _mav_put_uint16_t(buf, 10, timeout_ms);
    _mav_put_uint16_t(buf, 12, max_rate_cdeg_s);
    _mav_put_uint8_t(buf, 14, target_system);
    _mav_put_uint8_t(buf, 15, target_component);
    _mav_put_uint8_t(buf, 16, actuator);
    _mav_put_uint8_t(buf, 17, opcode);
    _mav_put_uint8_t(buf, 18, control_source);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN);
#else
    mavlink_actuator_request_t packet;
    packet.request_id = request_id;
    packet.ground_session = ground_session;
    packet.delta_cdeg = delta_cdeg;
    packet.timeout_ms = timeout_ms;
    packet.max_rate_cdeg_s = max_rate_cdeg_s;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.actuator = actuator;
    packet.opcode = opcode;
    packet.control_source = control_source;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ACTUATOR_REQUEST;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_CRC);
}

/**
 * @brief Pack a actuator_request message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param request_id  Ground-generated identifier unique within ground_session, echoed in REQUEST_RESULT for correlation. Monotonic per session where practical.
 * @param ground_session  Random per-connection ground session identifier. A reconnect selects a new value; queued movement carrying an old session is invalid.
 * @param delta_cdeg [cdeg] Absolute target angle for MOVEMENT_OPCODE_ABSOLUTE, signed relative delta resolved once against valid fresh feedback for MOVEMENT_OPCODE_STEP, and zero for MOVEMENT_OPCODE_STOP.
 * @param timeout_ms [ms] Freshness timeout after reception. Zero uses the receiver's configured default.
 * @param max_rate_cdeg_s [cdeg/s] Maximum slew rate for this movement. Zero uses the receiver's configured default.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param actuator  Actuator to command.
 * @param opcode  Requested operation.
 * @param control_source  Authority under which this request was produced.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_actuator_request_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t request_id, uint32_t ground_session, int16_t delta_cdeg, uint16_t timeout_ms, uint16_t max_rate_cdeg_s, uint8_t target_system, uint8_t target_component, uint8_t actuator, uint8_t opcode, uint8_t control_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN];
    _mav_put_uint32_t(buf, 0, request_id);
    _mav_put_uint32_t(buf, 4, ground_session);
    _mav_put_int16_t(buf, 8, delta_cdeg);
    _mav_put_uint16_t(buf, 10, timeout_ms);
    _mav_put_uint16_t(buf, 12, max_rate_cdeg_s);
    _mav_put_uint8_t(buf, 14, target_system);
    _mav_put_uint8_t(buf, 15, target_component);
    _mav_put_uint8_t(buf, 16, actuator);
    _mav_put_uint8_t(buf, 17, opcode);
    _mav_put_uint8_t(buf, 18, control_source);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN);
#else
    mavlink_actuator_request_t packet;
    packet.request_id = request_id;
    packet.ground_session = ground_session;
    packet.delta_cdeg = delta_cdeg;
    packet.timeout_ms = timeout_ms;
    packet.max_rate_cdeg_s = max_rate_cdeg_s;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.actuator = actuator;
    packet.opcode = opcode;
    packet.control_source = control_source;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ACTUATOR_REQUEST;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN);
#endif
}

/**
 * @brief Pack a actuator_request message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param request_id  Ground-generated identifier unique within ground_session, echoed in REQUEST_RESULT for correlation. Monotonic per session where practical.
 * @param ground_session  Random per-connection ground session identifier. A reconnect selects a new value; queued movement carrying an old session is invalid.
 * @param delta_cdeg [cdeg] Absolute target angle for MOVEMENT_OPCODE_ABSOLUTE, signed relative delta resolved once against valid fresh feedback for MOVEMENT_OPCODE_STEP, and zero for MOVEMENT_OPCODE_STOP.
 * @param timeout_ms [ms] Freshness timeout after reception. Zero uses the receiver's configured default.
 * @param max_rate_cdeg_s [cdeg/s] Maximum slew rate for this movement. Zero uses the receiver's configured default.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param actuator  Actuator to command.
 * @param opcode  Requested operation.
 * @param control_source  Authority under which this request was produced.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_actuator_request_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t request_id,uint32_t ground_session,int16_t delta_cdeg,uint16_t timeout_ms,uint16_t max_rate_cdeg_s,uint8_t target_system,uint8_t target_component,uint8_t actuator,uint8_t opcode,uint8_t control_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN];
    _mav_put_uint32_t(buf, 0, request_id);
    _mav_put_uint32_t(buf, 4, ground_session);
    _mav_put_int16_t(buf, 8, delta_cdeg);
    _mav_put_uint16_t(buf, 10, timeout_ms);
    _mav_put_uint16_t(buf, 12, max_rate_cdeg_s);
    _mav_put_uint8_t(buf, 14, target_system);
    _mav_put_uint8_t(buf, 15, target_component);
    _mav_put_uint8_t(buf, 16, actuator);
    _mav_put_uint8_t(buf, 17, opcode);
    _mav_put_uint8_t(buf, 18, control_source);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN);
#else
    mavlink_actuator_request_t packet;
    packet.request_id = request_id;
    packet.ground_session = ground_session;
    packet.delta_cdeg = delta_cdeg;
    packet.timeout_ms = timeout_ms;
    packet.max_rate_cdeg_s = max_rate_cdeg_s;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.actuator = actuator;
    packet.opcode = opcode;
    packet.control_source = control_source;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ACTUATOR_REQUEST;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_CRC);
}

/**
 * @brief Encode a actuator_request struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param actuator_request C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_actuator_request_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_actuator_request_t* actuator_request)
{
    return mavlink_msg_actuator_request_pack(system_id, component_id, msg, actuator_request->request_id, actuator_request->ground_session, actuator_request->delta_cdeg, actuator_request->timeout_ms, actuator_request->max_rate_cdeg_s, actuator_request->target_system, actuator_request->target_component, actuator_request->actuator, actuator_request->opcode, actuator_request->control_source);
}

/**
 * @brief Encode a actuator_request struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param actuator_request C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_actuator_request_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_actuator_request_t* actuator_request)
{
    return mavlink_msg_actuator_request_pack_chan(system_id, component_id, chan, msg, actuator_request->request_id, actuator_request->ground_session, actuator_request->delta_cdeg, actuator_request->timeout_ms, actuator_request->max_rate_cdeg_s, actuator_request->target_system, actuator_request->target_component, actuator_request->actuator, actuator_request->opcode, actuator_request->control_source);
}

/**
 * @brief Encode a actuator_request struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param actuator_request C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_actuator_request_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_actuator_request_t* actuator_request)
{
    return mavlink_msg_actuator_request_pack_status(system_id, component_id, _status, msg,  actuator_request->request_id, actuator_request->ground_session, actuator_request->delta_cdeg, actuator_request->timeout_ms, actuator_request->max_rate_cdeg_s, actuator_request->target_system, actuator_request->target_component, actuator_request->actuator, actuator_request->opcode, actuator_request->control_source);
}

/**
 * @brief Send a actuator_request message
 * @param chan MAVLink channel to send the message
 *
 * @param request_id  Ground-generated identifier unique within ground_session, echoed in REQUEST_RESULT for correlation. Monotonic per session where practical.
 * @param ground_session  Random per-connection ground session identifier. A reconnect selects a new value; queued movement carrying an old session is invalid.
 * @param delta_cdeg [cdeg] Absolute target angle for MOVEMENT_OPCODE_ABSOLUTE, signed relative delta resolved once against valid fresh feedback for MOVEMENT_OPCODE_STEP, and zero for MOVEMENT_OPCODE_STOP.
 * @param timeout_ms [ms] Freshness timeout after reception. Zero uses the receiver's configured default.
 * @param max_rate_cdeg_s [cdeg/s] Maximum slew rate for this movement. Zero uses the receiver's configured default.
 * @param target_system  System ID of the intended recipient.
 * @param target_component  Component ID of the intended recipient.
 * @param actuator  Actuator to command.
 * @param opcode  Requested operation.
 * @param control_source  Authority under which this request was produced.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_actuator_request_send(mavlink_channel_t chan, uint32_t request_id, uint32_t ground_session, int16_t delta_cdeg, uint16_t timeout_ms, uint16_t max_rate_cdeg_s, uint8_t target_system, uint8_t target_component, uint8_t actuator, uint8_t opcode, uint8_t control_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN];
    _mav_put_uint32_t(buf, 0, request_id);
    _mav_put_uint32_t(buf, 4, ground_session);
    _mav_put_int16_t(buf, 8, delta_cdeg);
    _mav_put_uint16_t(buf, 10, timeout_ms);
    _mav_put_uint16_t(buf, 12, max_rate_cdeg_s);
    _mav_put_uint8_t(buf, 14, target_system);
    _mav_put_uint8_t(buf, 15, target_component);
    _mav_put_uint8_t(buf, 16, actuator);
    _mav_put_uint8_t(buf, 17, opcode);
    _mav_put_uint8_t(buf, 18, control_source);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_REQUEST, buf, MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_CRC);
#else
    mavlink_actuator_request_t packet;
    packet.request_id = request_id;
    packet.ground_session = ground_session;
    packet.delta_cdeg = delta_cdeg;
    packet.timeout_ms = timeout_ms;
    packet.max_rate_cdeg_s = max_rate_cdeg_s;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.actuator = actuator;
    packet.opcode = opcode;
    packet.control_source = control_source;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_REQUEST, (const char *)&packet, MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_CRC);
#endif
}

/**
 * @brief Send a actuator_request message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_actuator_request_send_struct(mavlink_channel_t chan, const mavlink_actuator_request_t* actuator_request)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_actuator_request_send(chan, actuator_request->request_id, actuator_request->ground_session, actuator_request->delta_cdeg, actuator_request->timeout_ms, actuator_request->max_rate_cdeg_s, actuator_request->target_system, actuator_request->target_component, actuator_request->actuator, actuator_request->opcode, actuator_request->control_source);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_REQUEST, (const char *)actuator_request, MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_CRC);
#endif
}

#if MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_actuator_request_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t request_id, uint32_t ground_session, int16_t delta_cdeg, uint16_t timeout_ms, uint16_t max_rate_cdeg_s, uint8_t target_system, uint8_t target_component, uint8_t actuator, uint8_t opcode, uint8_t control_source)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, request_id);
    _mav_put_uint32_t(buf, 4, ground_session);
    _mav_put_int16_t(buf, 8, delta_cdeg);
    _mav_put_uint16_t(buf, 10, timeout_ms);
    _mav_put_uint16_t(buf, 12, max_rate_cdeg_s);
    _mav_put_uint8_t(buf, 14, target_system);
    _mav_put_uint8_t(buf, 15, target_component);
    _mav_put_uint8_t(buf, 16, actuator);
    _mav_put_uint8_t(buf, 17, opcode);
    _mav_put_uint8_t(buf, 18, control_source);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_REQUEST, buf, MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_CRC);
#else
    mavlink_actuator_request_t *packet = (mavlink_actuator_request_t *)msgbuf;
    packet->request_id = request_id;
    packet->ground_session = ground_session;
    packet->delta_cdeg = delta_cdeg;
    packet->timeout_ms = timeout_ms;
    packet->max_rate_cdeg_s = max_rate_cdeg_s;
    packet->target_system = target_system;
    packet->target_component = target_component;
    packet->actuator = actuator;
    packet->opcode = opcode;
    packet->control_source = control_source;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ACTUATOR_REQUEST, (const char *)packet, MAVLINK_MSG_ID_ACTUATOR_REQUEST_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN, MAVLINK_MSG_ID_ACTUATOR_REQUEST_CRC);
#endif
}
#endif

#endif

// MESSAGE ACTUATOR_REQUEST UNPACKING


/**
 * @brief Get field request_id from actuator_request message
 *
 * @return  Ground-generated identifier unique within ground_session, echoed in REQUEST_RESULT for correlation. Monotonic per session where practical.
 */
static inline uint32_t mavlink_msg_actuator_request_get_request_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field ground_session from actuator_request message
 *
 * @return  Random per-connection ground session identifier. A reconnect selects a new value; queued movement carrying an old session is invalid.
 */
static inline uint32_t mavlink_msg_actuator_request_get_ground_session(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  4);
}

/**
 * @brief Get field delta_cdeg from actuator_request message
 *
 * @return [cdeg] Absolute target angle for MOVEMENT_OPCODE_ABSOLUTE, signed relative delta resolved once against valid fresh feedback for MOVEMENT_OPCODE_STEP, and zero for MOVEMENT_OPCODE_STOP.
 */
static inline int16_t mavlink_msg_actuator_request_get_delta_cdeg(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  8);
}

/**
 * @brief Get field timeout_ms from actuator_request message
 *
 * @return [ms] Freshness timeout after reception. Zero uses the receiver's configured default.
 */
static inline uint16_t mavlink_msg_actuator_request_get_timeout_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  10);
}

/**
 * @brief Get field max_rate_cdeg_s from actuator_request message
 *
 * @return [cdeg/s] Maximum slew rate for this movement. Zero uses the receiver's configured default.
 */
static inline uint16_t mavlink_msg_actuator_request_get_max_rate_cdeg_s(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  12);
}

/**
 * @brief Get field target_system from actuator_request message
 *
 * @return  System ID of the intended recipient.
 */
static inline uint8_t mavlink_msg_actuator_request_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  14);
}

/**
 * @brief Get field target_component from actuator_request message
 *
 * @return  Component ID of the intended recipient.
 */
static inline uint8_t mavlink_msg_actuator_request_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  15);
}

/**
 * @brief Get field actuator from actuator_request message
 *
 * @return  Actuator to command.
 */
static inline uint8_t mavlink_msg_actuator_request_get_actuator(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  16);
}

/**
 * @brief Get field opcode from actuator_request message
 *
 * @return  Requested operation.
 */
static inline uint8_t mavlink_msg_actuator_request_get_opcode(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  17);
}

/**
 * @brief Get field control_source from actuator_request message
 *
 * @return  Authority under which this request was produced.
 */
static inline uint8_t mavlink_msg_actuator_request_get_control_source(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  18);
}

/**
 * @brief Decode a actuator_request message into a struct
 *
 * @param msg The message to decode
 * @param actuator_request C-struct to decode the message contents into
 */
static inline void mavlink_msg_actuator_request_decode(const mavlink_message_t* msg, mavlink_actuator_request_t* actuator_request)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    actuator_request->request_id = mavlink_msg_actuator_request_get_request_id(msg);
    actuator_request->ground_session = mavlink_msg_actuator_request_get_ground_session(msg);
    actuator_request->delta_cdeg = mavlink_msg_actuator_request_get_delta_cdeg(msg);
    actuator_request->timeout_ms = mavlink_msg_actuator_request_get_timeout_ms(msg);
    actuator_request->max_rate_cdeg_s = mavlink_msg_actuator_request_get_max_rate_cdeg_s(msg);
    actuator_request->target_system = mavlink_msg_actuator_request_get_target_system(msg);
    actuator_request->target_component = mavlink_msg_actuator_request_get_target_component(msg);
    actuator_request->actuator = mavlink_msg_actuator_request_get_actuator(msg);
    actuator_request->opcode = mavlink_msg_actuator_request_get_opcode(msg);
    actuator_request->control_source = mavlink_msg_actuator_request_get_control_source(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN? msg->len : MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN;
        memset(actuator_request, 0, MAVLINK_MSG_ID_ACTUATOR_REQUEST_LEN);
    memcpy(actuator_request, _MAV_PAYLOAD(msg), len);
#endif
}
