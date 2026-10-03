#pragma once
// MESSAGE SAIL_MOTOR_TEST_STATUS PACKING

#define MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS 200091


typedef struct __mavlink_sail_motor_test_status_t {
 uint32_t time_boot_ms; /*< [ms] MCU uptime.*/
 uint32_t command_id; /*<  Last accepted command ID; zero at boot.*/
 uint16_t elapsed_ms; /*< [ms] Time energised for the last accepted move, frozen at stop.*/
 uint16_t remaining_ms; /*< [ms] Time remaining in the current move.*/
 uint8_t state; /*<  0 idle, 1 running, 2 completed, 3 explicitly stopped, 4 receive overflow stop.*/
 int8_t direction; /*<  Current output direction; zero when stopped.*/
 uint8_t pwm; /*<  Current output PWM; zero when stopped.*/
 uint8_t result; /*<  0 accepted/normal, 1 invalid fields, 2 busy, 3 stale/duplicate ID, 4 bad frame, 5 receive overflow, 6 wrong source/target.*/
} mavlink_sail_motor_test_status_t;

#define MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN 16
#define MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN 16
#define MAVLINK_MSG_ID_200091_LEN 16
#define MAVLINK_MSG_ID_200091_MIN_LEN 16

#define MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_CRC 123
#define MAVLINK_MSG_ID_200091_CRC 123



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_SAIL_MOTOR_TEST_STATUS { \
    200091, \
    "SAIL_MOTOR_TEST_STATUS", \
    8, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_sail_motor_test_status_t, time_boot_ms) }, \
         { "command_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_sail_motor_test_status_t, command_id) }, \
         { "elapsed_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_sail_motor_test_status_t, elapsed_ms) }, \
         { "remaining_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 10, offsetof(mavlink_sail_motor_test_status_t, remaining_ms) }, \
         { "state", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_sail_motor_test_status_t, state) }, \
         { "direction", NULL, MAVLINK_TYPE_INT8_T, 0, 13, offsetof(mavlink_sail_motor_test_status_t, direction) }, \
         { "pwm", NULL, MAVLINK_TYPE_UINT8_T, 0, 14, offsetof(mavlink_sail_motor_test_status_t, pwm) }, \
         { "result", NULL, MAVLINK_TYPE_UINT8_T, 0, 15, offsetof(mavlink_sail_motor_test_status_t, result) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_SAIL_MOTOR_TEST_STATUS { \
    "SAIL_MOTOR_TEST_STATUS", \
    8, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_sail_motor_test_status_t, time_boot_ms) }, \
         { "command_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_sail_motor_test_status_t, command_id) }, \
         { "elapsed_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_sail_motor_test_status_t, elapsed_ms) }, \
         { "remaining_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 10, offsetof(mavlink_sail_motor_test_status_t, remaining_ms) }, \
         { "state", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_sail_motor_test_status_t, state) }, \
         { "direction", NULL, MAVLINK_TYPE_INT8_T, 0, 13, offsetof(mavlink_sail_motor_test_status_t, direction) }, \
         { "pwm", NULL, MAVLINK_TYPE_UINT8_T, 0, 14, offsetof(mavlink_sail_motor_test_status_t, pwm) }, \
         { "result", NULL, MAVLINK_TYPE_UINT8_T, 0, 15, offsetof(mavlink_sail_motor_test_status_t, result) }, \
         } \
}
#endif

/**
 * @brief Pack a sail_motor_test_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] MCU uptime.
 * @param command_id  Last accepted command ID; zero at boot.
 * @param elapsed_ms [ms] Time energised for the last accepted move, frozen at stop.
 * @param remaining_ms [ms] Time remaining in the current move.
 * @param state  0 idle, 1 running, 2 completed, 3 explicitly stopped, 4 receive overflow stop.
 * @param direction  Current output direction; zero when stopped.
 * @param pwm  Current output PWM; zero when stopped.
 * @param result  0 accepted/normal, 1 invalid fields, 2 busy, 3 stale/duplicate ID, 4 bad frame, 5 receive overflow, 6 wrong source/target.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sail_motor_test_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint32_t command_id, uint16_t elapsed_ms, uint16_t remaining_ms, uint8_t state, int8_t direction, uint8_t pwm, uint8_t result)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, command_id);
    _mav_put_uint16_t(buf, 8, elapsed_ms);
    _mav_put_uint16_t(buf, 10, remaining_ms);
    _mav_put_uint8_t(buf, 12, state);
    _mav_put_int8_t(buf, 13, direction);
    _mav_put_uint8_t(buf, 14, pwm);
    _mav_put_uint8_t(buf, 15, result);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN);
#else
    mavlink_sail_motor_test_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.command_id = command_id;
    packet.elapsed_ms = elapsed_ms;
    packet.remaining_ms = remaining_ms;
    packet.state = state;
    packet.direction = direction;
    packet.pwm = pwm;
    packet.result = result;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_CRC);
}

/**
 * @brief Pack a sail_motor_test_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] MCU uptime.
 * @param command_id  Last accepted command ID; zero at boot.
 * @param elapsed_ms [ms] Time energised for the last accepted move, frozen at stop.
 * @param remaining_ms [ms] Time remaining in the current move.
 * @param state  0 idle, 1 running, 2 completed, 3 explicitly stopped, 4 receive overflow stop.
 * @param direction  Current output direction; zero when stopped.
 * @param pwm  Current output PWM; zero when stopped.
 * @param result  0 accepted/normal, 1 invalid fields, 2 busy, 3 stale/duplicate ID, 4 bad frame, 5 receive overflow, 6 wrong source/target.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sail_motor_test_status_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint32_t command_id, uint16_t elapsed_ms, uint16_t remaining_ms, uint8_t state, int8_t direction, uint8_t pwm, uint8_t result)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, command_id);
    _mav_put_uint16_t(buf, 8, elapsed_ms);
    _mav_put_uint16_t(buf, 10, remaining_ms);
    _mav_put_uint8_t(buf, 12, state);
    _mav_put_int8_t(buf, 13, direction);
    _mav_put_uint8_t(buf, 14, pwm);
    _mav_put_uint8_t(buf, 15, result);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN);
#else
    mavlink_sail_motor_test_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.command_id = command_id;
    packet.elapsed_ms = elapsed_ms;
    packet.remaining_ms = remaining_ms;
    packet.state = state;
    packet.direction = direction;
    packet.pwm = pwm;
    packet.result = result;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN);
#endif
}

/**
 * @brief Pack a sail_motor_test_status message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms [ms] MCU uptime.
 * @param command_id  Last accepted command ID; zero at boot.
 * @param elapsed_ms [ms] Time energised for the last accepted move, frozen at stop.
 * @param remaining_ms [ms] Time remaining in the current move.
 * @param state  0 idle, 1 running, 2 completed, 3 explicitly stopped, 4 receive overflow stop.
 * @param direction  Current output direction; zero when stopped.
 * @param pwm  Current output PWM; zero when stopped.
 * @param result  0 accepted/normal, 1 invalid fields, 2 busy, 3 stale/duplicate ID, 4 bad frame, 5 receive overflow, 6 wrong source/target.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sail_motor_test_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t time_boot_ms,uint32_t command_id,uint16_t elapsed_ms,uint16_t remaining_ms,uint8_t state,int8_t direction,uint8_t pwm,uint8_t result)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, command_id);
    _mav_put_uint16_t(buf, 8, elapsed_ms);
    _mav_put_uint16_t(buf, 10, remaining_ms);
    _mav_put_uint8_t(buf, 12, state);
    _mav_put_int8_t(buf, 13, direction);
    _mav_put_uint8_t(buf, 14, pwm);
    _mav_put_uint8_t(buf, 15, result);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN);
#else
    mavlink_sail_motor_test_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.command_id = command_id;
    packet.elapsed_ms = elapsed_ms;
    packet.remaining_ms = remaining_ms;
    packet.state = state;
    packet.direction = direction;
    packet.pwm = pwm;
    packet.result = result;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_CRC);
}

/**
 * @brief Encode a sail_motor_test_status struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param sail_motor_test_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sail_motor_test_status_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_sail_motor_test_status_t* sail_motor_test_status)
{
    return mavlink_msg_sail_motor_test_status_pack(system_id, component_id, msg, sail_motor_test_status->time_boot_ms, sail_motor_test_status->command_id, sail_motor_test_status->elapsed_ms, sail_motor_test_status->remaining_ms, sail_motor_test_status->state, sail_motor_test_status->direction, sail_motor_test_status->pwm, sail_motor_test_status->result);
}

/**
 * @brief Encode a sail_motor_test_status struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param sail_motor_test_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sail_motor_test_status_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_sail_motor_test_status_t* sail_motor_test_status)
{
    return mavlink_msg_sail_motor_test_status_pack_chan(system_id, component_id, chan, msg, sail_motor_test_status->time_boot_ms, sail_motor_test_status->command_id, sail_motor_test_status->elapsed_ms, sail_motor_test_status->remaining_ms, sail_motor_test_status->state, sail_motor_test_status->direction, sail_motor_test_status->pwm, sail_motor_test_status->result);
}

/**
 * @brief Encode a sail_motor_test_status struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param sail_motor_test_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sail_motor_test_status_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_sail_motor_test_status_t* sail_motor_test_status)
{
    return mavlink_msg_sail_motor_test_status_pack_status(system_id, component_id, _status, msg,  sail_motor_test_status->time_boot_ms, sail_motor_test_status->command_id, sail_motor_test_status->elapsed_ms, sail_motor_test_status->remaining_ms, sail_motor_test_status->state, sail_motor_test_status->direction, sail_motor_test_status->pwm, sail_motor_test_status->result);
}

/**
 * @brief Send a sail_motor_test_status message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms [ms] MCU uptime.
 * @param command_id  Last accepted command ID; zero at boot.
 * @param elapsed_ms [ms] Time energised for the last accepted move, frozen at stop.
 * @param remaining_ms [ms] Time remaining in the current move.
 * @param state  0 idle, 1 running, 2 completed, 3 explicitly stopped, 4 receive overflow stop.
 * @param direction  Current output direction; zero when stopped.
 * @param pwm  Current output PWM; zero when stopped.
 * @param result  0 accepted/normal, 1 invalid fields, 2 busy, 3 stale/duplicate ID, 4 bad frame, 5 receive overflow, 6 wrong source/target.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_sail_motor_test_status_send(mavlink_channel_t chan, uint32_t time_boot_ms, uint32_t command_id, uint16_t elapsed_ms, uint16_t remaining_ms, uint8_t state, int8_t direction, uint8_t pwm, uint8_t result)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, command_id);
    _mav_put_uint16_t(buf, 8, elapsed_ms);
    _mav_put_uint16_t(buf, 10, remaining_ms);
    _mav_put_uint8_t(buf, 12, state);
    _mav_put_int8_t(buf, 13, direction);
    _mav_put_uint8_t(buf, 14, pwm);
    _mav_put_uint8_t(buf, 15, result);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS, buf, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_CRC);
#else
    mavlink_sail_motor_test_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.command_id = command_id;
    packet.elapsed_ms = elapsed_ms;
    packet.remaining_ms = remaining_ms;
    packet.state = state;
    packet.direction = direction;
    packet.pwm = pwm;
    packet.result = result;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS, (const char *)&packet, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_CRC);
#endif
}

/**
 * @brief Send a sail_motor_test_status message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_sail_motor_test_status_send_struct(mavlink_channel_t chan, const mavlink_sail_motor_test_status_t* sail_motor_test_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_sail_motor_test_status_send(chan, sail_motor_test_status->time_boot_ms, sail_motor_test_status->command_id, sail_motor_test_status->elapsed_ms, sail_motor_test_status->remaining_ms, sail_motor_test_status->state, sail_motor_test_status->direction, sail_motor_test_status->pwm, sail_motor_test_status->result);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS, (const char *)sail_motor_test_status, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_CRC);
#endif
}

#if MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_sail_motor_test_status_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, uint32_t command_id, uint16_t elapsed_ms, uint16_t remaining_ms, uint8_t state, int8_t direction, uint8_t pwm, uint8_t result)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, command_id);
    _mav_put_uint16_t(buf, 8, elapsed_ms);
    _mav_put_uint16_t(buf, 10, remaining_ms);
    _mav_put_uint8_t(buf, 12, state);
    _mav_put_int8_t(buf, 13, direction);
    _mav_put_uint8_t(buf, 14, pwm);
    _mav_put_uint8_t(buf, 15, result);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS, buf, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_CRC);
#else
    mavlink_sail_motor_test_status_t *packet = (mavlink_sail_motor_test_status_t *)msgbuf;
    packet->time_boot_ms = time_boot_ms;
    packet->command_id = command_id;
    packet->elapsed_ms = elapsed_ms;
    packet->remaining_ms = remaining_ms;
    packet->state = state;
    packet->direction = direction;
    packet->pwm = pwm;
    packet->result = result;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS, (const char *)packet, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_MIN_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_CRC);
#endif
}
#endif

#endif

// MESSAGE SAIL_MOTOR_TEST_STATUS UNPACKING


/**
 * @brief Get field time_boot_ms from sail_motor_test_status message
 *
 * @return [ms] MCU uptime.
 */
static inline uint32_t mavlink_msg_sail_motor_test_status_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field command_id from sail_motor_test_status message
 *
 * @return  Last accepted command ID; zero at boot.
 */
static inline uint32_t mavlink_msg_sail_motor_test_status_get_command_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  4);
}

/**
 * @brief Get field elapsed_ms from sail_motor_test_status message
 *
 * @return [ms] Time energised for the last accepted move, frozen at stop.
 */
static inline uint16_t mavlink_msg_sail_motor_test_status_get_elapsed_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  8);
}

/**
 * @brief Get field remaining_ms from sail_motor_test_status message
 *
 * @return [ms] Time remaining in the current move.
 */
static inline uint16_t mavlink_msg_sail_motor_test_status_get_remaining_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  10);
}

/**
 * @brief Get field state from sail_motor_test_status message
 *
 * @return  0 idle, 1 running, 2 completed, 3 explicitly stopped, 4 receive overflow stop.
 */
static inline uint8_t mavlink_msg_sail_motor_test_status_get_state(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  12);
}

/**
 * @brief Get field direction from sail_motor_test_status message
 *
 * @return  Current output direction; zero when stopped.
 */
static inline int8_t mavlink_msg_sail_motor_test_status_get_direction(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int8_t(msg,  13);
}

/**
 * @brief Get field pwm from sail_motor_test_status message
 *
 * @return  Current output PWM; zero when stopped.
 */
static inline uint8_t mavlink_msg_sail_motor_test_status_get_pwm(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  14);
}

/**
 * @brief Get field result from sail_motor_test_status message
 *
 * @return  0 accepted/normal, 1 invalid fields, 2 busy, 3 stale/duplicate ID, 4 bad frame, 5 receive overflow, 6 wrong source/target.
 */
static inline uint8_t mavlink_msg_sail_motor_test_status_get_result(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  15);
}

/**
 * @brief Decode a sail_motor_test_status message into a struct
 *
 * @param msg The message to decode
 * @param sail_motor_test_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_sail_motor_test_status_decode(const mavlink_message_t* msg, mavlink_sail_motor_test_status_t* sail_motor_test_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    sail_motor_test_status->time_boot_ms = mavlink_msg_sail_motor_test_status_get_time_boot_ms(msg);
    sail_motor_test_status->command_id = mavlink_msg_sail_motor_test_status_get_command_id(msg);
    sail_motor_test_status->elapsed_ms = mavlink_msg_sail_motor_test_status_get_elapsed_ms(msg);
    sail_motor_test_status->remaining_ms = mavlink_msg_sail_motor_test_status_get_remaining_ms(msg);
    sail_motor_test_status->state = mavlink_msg_sail_motor_test_status_get_state(msg);
    sail_motor_test_status->direction = mavlink_msg_sail_motor_test_status_get_direction(msg);
    sail_motor_test_status->pwm = mavlink_msg_sail_motor_test_status_get_pwm(msg);
    sail_motor_test_status->result = mavlink_msg_sail_motor_test_status_get_result(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN? msg->len : MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN;
        memset(sail_motor_test_status, 0, MAVLINK_MSG_ID_SAIL_MOTOR_TEST_STATUS_LEN);
    memcpy(sail_motor_test_status, _MAV_PAYLOAD(msg), len);
#endif
}
