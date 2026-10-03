#pragma once
// MESSAGE SAIL_MOTOR_TEST PACKING

#define MAVLINK_MSG_ID_SAIL_MOTOR_TEST 200090


typedef struct __mavlink_sail_motor_test_t {
 uint32_t command_id; /*<  Nonzero increasing ID. Read status and use its last accepted ID plus one.*/
 uint16_t duration_ms; /*< [ms] Motor energisation duration, 1 through 5000 for a move, zero for stop.*/
 uint8_t target_system; /*<  Recipient system ID, exactly 1.*/
 uint8_t target_component; /*<  Recipient component ID, exactly 154.*/
 int8_t direction; /*<  1 forward, -1 backward, 0 stop.*/
 uint8_t pwm; /*<  PWM duty 1 through 128 out of 255 for a move, zero for stop.*/
} mavlink_sail_motor_test_t;

#define MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN 10
#define MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN 10
#define MAVLINK_MSG_ID_200090_LEN 10
#define MAVLINK_MSG_ID_200090_MIN_LEN 10

#define MAVLINK_MSG_ID_SAIL_MOTOR_TEST_CRC 233
#define MAVLINK_MSG_ID_200090_CRC 233



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_SAIL_MOTOR_TEST { \
    200090, \
    "SAIL_MOTOR_TEST", \
    6, \
    {  { "command_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_sail_motor_test_t, command_id) }, \
         { "duration_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_sail_motor_test_t, duration_ms) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_sail_motor_test_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 7, offsetof(mavlink_sail_motor_test_t, target_component) }, \
         { "direction", NULL, MAVLINK_TYPE_INT8_T, 0, 8, offsetof(mavlink_sail_motor_test_t, direction) }, \
         { "pwm", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_sail_motor_test_t, pwm) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_SAIL_MOTOR_TEST { \
    "SAIL_MOTOR_TEST", \
    6, \
    {  { "command_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_sail_motor_test_t, command_id) }, \
         { "duration_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_sail_motor_test_t, duration_ms) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_sail_motor_test_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 7, offsetof(mavlink_sail_motor_test_t, target_component) }, \
         { "direction", NULL, MAVLINK_TYPE_INT8_T, 0, 8, offsetof(mavlink_sail_motor_test_t, direction) }, \
         { "pwm", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_sail_motor_test_t, pwm) }, \
         } \
}
#endif

/**
 * @brief Pack a sail_motor_test message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param command_id  Nonzero increasing ID. Read status and use its last accepted ID plus one.
 * @param duration_ms [ms] Motor energisation duration, 1 through 5000 for a move, zero for stop.
 * @param target_system  Recipient system ID, exactly 1.
 * @param target_component  Recipient component ID, exactly 154.
 * @param direction  1 forward, -1 backward, 0 stop.
 * @param pwm  PWM duty 1 through 128 out of 255 for a move, zero for stop.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sail_motor_test_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t command_id, uint16_t duration_ms, uint8_t target_system, uint8_t target_component, int8_t direction, uint8_t pwm)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN];
    _mav_put_uint32_t(buf, 0, command_id);
    _mav_put_uint16_t(buf, 4, duration_ms);
    _mav_put_uint8_t(buf, 6, target_system);
    _mav_put_uint8_t(buf, 7, target_component);
    _mav_put_int8_t(buf, 8, direction);
    _mav_put_uint8_t(buf, 9, pwm);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN);
#else
    mavlink_sail_motor_test_t packet;
    packet.command_id = command_id;
    packet.duration_ms = duration_ms;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.direction = direction;
    packet.pwm = pwm;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SAIL_MOTOR_TEST;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_CRC);
}

/**
 * @brief Pack a sail_motor_test message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param command_id  Nonzero increasing ID. Read status and use its last accepted ID plus one.
 * @param duration_ms [ms] Motor energisation duration, 1 through 5000 for a move, zero for stop.
 * @param target_system  Recipient system ID, exactly 1.
 * @param target_component  Recipient component ID, exactly 154.
 * @param direction  1 forward, -1 backward, 0 stop.
 * @param pwm  PWM duty 1 through 128 out of 255 for a move, zero for stop.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sail_motor_test_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t command_id, uint16_t duration_ms, uint8_t target_system, uint8_t target_component, int8_t direction, uint8_t pwm)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN];
    _mav_put_uint32_t(buf, 0, command_id);
    _mav_put_uint16_t(buf, 4, duration_ms);
    _mav_put_uint8_t(buf, 6, target_system);
    _mav_put_uint8_t(buf, 7, target_component);
    _mav_put_int8_t(buf, 8, direction);
    _mav_put_uint8_t(buf, 9, pwm);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN);
#else
    mavlink_sail_motor_test_t packet;
    packet.command_id = command_id;
    packet.duration_ms = duration_ms;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.direction = direction;
    packet.pwm = pwm;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SAIL_MOTOR_TEST;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN);
#endif
}

/**
 * @brief Pack a sail_motor_test message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param command_id  Nonzero increasing ID. Read status and use its last accepted ID plus one.
 * @param duration_ms [ms] Motor energisation duration, 1 through 5000 for a move, zero for stop.
 * @param target_system  Recipient system ID, exactly 1.
 * @param target_component  Recipient component ID, exactly 154.
 * @param direction  1 forward, -1 backward, 0 stop.
 * @param pwm  PWM duty 1 through 128 out of 255 for a move, zero for stop.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sail_motor_test_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t command_id,uint16_t duration_ms,uint8_t target_system,uint8_t target_component,int8_t direction,uint8_t pwm)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN];
    _mav_put_uint32_t(buf, 0, command_id);
    _mav_put_uint16_t(buf, 4, duration_ms);
    _mav_put_uint8_t(buf, 6, target_system);
    _mav_put_uint8_t(buf, 7, target_component);
    _mav_put_int8_t(buf, 8, direction);
    _mav_put_uint8_t(buf, 9, pwm);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN);
#else
    mavlink_sail_motor_test_t packet;
    packet.command_id = command_id;
    packet.duration_ms = duration_ms;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.direction = direction;
    packet.pwm = pwm;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SAIL_MOTOR_TEST;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_CRC);
}

/**
 * @brief Encode a sail_motor_test struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param sail_motor_test C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sail_motor_test_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_sail_motor_test_t* sail_motor_test)
{
    return mavlink_msg_sail_motor_test_pack(system_id, component_id, msg, sail_motor_test->command_id, sail_motor_test->duration_ms, sail_motor_test->target_system, sail_motor_test->target_component, sail_motor_test->direction, sail_motor_test->pwm);
}

/**
 * @brief Encode a sail_motor_test struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param sail_motor_test C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sail_motor_test_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_sail_motor_test_t* sail_motor_test)
{
    return mavlink_msg_sail_motor_test_pack_chan(system_id, component_id, chan, msg, sail_motor_test->command_id, sail_motor_test->duration_ms, sail_motor_test->target_system, sail_motor_test->target_component, sail_motor_test->direction, sail_motor_test->pwm);
}

/**
 * @brief Encode a sail_motor_test struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param sail_motor_test C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sail_motor_test_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_sail_motor_test_t* sail_motor_test)
{
    return mavlink_msg_sail_motor_test_pack_status(system_id, component_id, _status, msg,  sail_motor_test->command_id, sail_motor_test->duration_ms, sail_motor_test->target_system, sail_motor_test->target_component, sail_motor_test->direction, sail_motor_test->pwm);
}

/**
 * @brief Send a sail_motor_test message
 * @param chan MAVLink channel to send the message
 *
 * @param command_id  Nonzero increasing ID. Read status and use its last accepted ID plus one.
 * @param duration_ms [ms] Motor energisation duration, 1 through 5000 for a move, zero for stop.
 * @param target_system  Recipient system ID, exactly 1.
 * @param target_component  Recipient component ID, exactly 154.
 * @param direction  1 forward, -1 backward, 0 stop.
 * @param pwm  PWM duty 1 through 128 out of 255 for a move, zero for stop.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_sail_motor_test_send(mavlink_channel_t chan, uint32_t command_id, uint16_t duration_ms, uint8_t target_system, uint8_t target_component, int8_t direction, uint8_t pwm)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN];
    _mav_put_uint32_t(buf, 0, command_id);
    _mav_put_uint16_t(buf, 4, duration_ms);
    _mav_put_uint8_t(buf, 6, target_system);
    _mav_put_uint8_t(buf, 7, target_component);
    _mav_put_int8_t(buf, 8, direction);
    _mav_put_uint8_t(buf, 9, pwm);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST, buf, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_CRC);
#else
    mavlink_sail_motor_test_t packet;
    packet.command_id = command_id;
    packet.duration_ms = duration_ms;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.direction = direction;
    packet.pwm = pwm;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST, (const char *)&packet, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_CRC);
#endif
}

/**
 * @brief Send a sail_motor_test message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_sail_motor_test_send_struct(mavlink_channel_t chan, const mavlink_sail_motor_test_t* sail_motor_test)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_sail_motor_test_send(chan, sail_motor_test->command_id, sail_motor_test->duration_ms, sail_motor_test->target_system, sail_motor_test->target_component, sail_motor_test->direction, sail_motor_test->pwm);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST, (const char *)sail_motor_test, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_CRC);
#endif
}

#if MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_sail_motor_test_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t command_id, uint16_t duration_ms, uint8_t target_system, uint8_t target_component, int8_t direction, uint8_t pwm)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, command_id);
    _mav_put_uint16_t(buf, 4, duration_ms);
    _mav_put_uint8_t(buf, 6, target_system);
    _mav_put_uint8_t(buf, 7, target_component);
    _mav_put_int8_t(buf, 8, direction);
    _mav_put_uint8_t(buf, 9, pwm);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST, buf, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_CRC);
#else
    mavlink_sail_motor_test_t *packet = (mavlink_sail_motor_test_t *)msgbuf;
    packet->command_id = command_id;
    packet->duration_ms = duration_ms;
    packet->target_system = target_system;
    packet->target_component = target_component;
    packet->direction = direction;
    packet->pwm = pwm;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST, (const char *)packet, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_CRC);
#endif
}
#endif

#endif

// MESSAGE SAIL_MOTOR_TEST UNPACKING


/**
 * @brief Get field command_id from sail_motor_test message
 *
 * @return  Nonzero increasing ID. Read status and use its last accepted ID plus one.
 */
static inline uint32_t mavlink_msg_sail_motor_test_get_command_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field duration_ms from sail_motor_test message
 *
 * @return [ms] Motor energisation duration, 1 through 5000 for a move, zero for stop.
 */
static inline uint16_t mavlink_msg_sail_motor_test_get_duration_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  4);
}

/**
 * @brief Get field target_system from sail_motor_test message
 *
 * @return  Recipient system ID, exactly 1.
 */
static inline uint8_t mavlink_msg_sail_motor_test_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  6);
}

/**
 * @brief Get field target_component from sail_motor_test message
 *
 * @return  Recipient component ID, exactly 154.
 */
static inline uint8_t mavlink_msg_sail_motor_test_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  7);
}

/**
 * @brief Get field direction from sail_motor_test message
 *
 * @return  1 forward, -1 backward, 0 stop.
 */
static inline int8_t mavlink_msg_sail_motor_test_get_direction(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int8_t(msg,  8);
}

/**
 * @brief Get field pwm from sail_motor_test message
 *
 * @return  PWM duty 1 through 128 out of 255 for a move, zero for stop.
 */
static inline uint8_t mavlink_msg_sail_motor_test_get_pwm(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  9);
}

/**
 * @brief Decode a sail_motor_test message into a struct
 *
 * @param msg The message to decode
 * @param sail_motor_test C-struct to decode the message contents into
 */
static inline void mavlink_msg_sail_motor_test_decode(const mavlink_message_t* msg, mavlink_sail_motor_test_t* sail_motor_test)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    sail_motor_test->command_id = mavlink_msg_sail_motor_test_get_command_id(msg);
    sail_motor_test->duration_ms = mavlink_msg_sail_motor_test_get_duration_ms(msg);
    sail_motor_test->target_system = mavlink_msg_sail_motor_test_get_target_system(msg);
    sail_motor_test->target_component = mavlink_msg_sail_motor_test_get_target_component(msg);
    sail_motor_test->direction = mavlink_msg_sail_motor_test_get_direction(msg);
    sail_motor_test->pwm = mavlink_msg_sail_motor_test_get_pwm(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN? msg->len : MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN;
        memset(sail_motor_test, 0, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN);
    memcpy(sail_motor_test, _MAV_PAYLOAD(msg), len);
#endif
}
