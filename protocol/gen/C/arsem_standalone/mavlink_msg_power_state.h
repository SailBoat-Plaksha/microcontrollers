#pragma once
// MESSAGE POWER_STATE PACKING

#define MAVLINK_MSG_ID_POWER_STATE 200005


typedef struct __mavlink_power_state_t {
 uint32_t time_boot_ms; /*< [ms] Time since the reporting component booted.*/
 int16_t current_ma; /*< [mA] Supply current. A negative value indicates charging.*/
 uint16_t voltage_mv; /*< [mV] Supply voltage.*/
 uint16_t state_of_charge_cpercent; /*< [c%] Remaining charge in centipercent, from 0 to 10000.*/
 uint16_t fault_flags; /*<  Active power-related and system faults. Zero means no reported fault.*/
 uint8_t mode; /*<  Current power operating mode.*/
 uint8_t external_power; /*<  One when an external charging or shore-power source is present, otherwise zero.*/
} mavlink_power_state_t;

#define MAVLINK_MSG_ID_POWER_STATE_LEN 14
#define MAVLINK_MSG_ID_POWER_STATE_MIN_LEN 14
#define MAVLINK_MSG_ID_200005_LEN 14
#define MAVLINK_MSG_ID_200005_MIN_LEN 14

#define MAVLINK_MSG_ID_POWER_STATE_CRC 201
#define MAVLINK_MSG_ID_200005_CRC 201



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_POWER_STATE { \
    200005, \
    "POWER_STATE", \
    7, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_power_state_t, time_boot_ms) }, \
         { "current_ma", NULL, MAVLINK_TYPE_INT16_T, 0, 4, offsetof(mavlink_power_state_t, current_ma) }, \
         { "voltage_mv", NULL, MAVLINK_TYPE_UINT16_T, 0, 6, offsetof(mavlink_power_state_t, voltage_mv) }, \
         { "state_of_charge_cpercent", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_power_state_t, state_of_charge_cpercent) }, \
         { "fault_flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 10, offsetof(mavlink_power_state_t, fault_flags) }, \
         { "mode", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_power_state_t, mode) }, \
         { "external_power", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_power_state_t, external_power) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_POWER_STATE { \
    "POWER_STATE", \
    7, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_power_state_t, time_boot_ms) }, \
         { "current_ma", NULL, MAVLINK_TYPE_INT16_T, 0, 4, offsetof(mavlink_power_state_t, current_ma) }, \
         { "voltage_mv", NULL, MAVLINK_TYPE_UINT16_T, 0, 6, offsetof(mavlink_power_state_t, voltage_mv) }, \
         { "state_of_charge_cpercent", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_power_state_t, state_of_charge_cpercent) }, \
         { "fault_flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 10, offsetof(mavlink_power_state_t, fault_flags) }, \
         { "mode", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_power_state_t, mode) }, \
         { "external_power", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_power_state_t, external_power) }, \
         } \
}
#endif

/**
 * @brief Pack a power_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param current_ma [mA] Supply current. A negative value indicates charging.
 * @param voltage_mv [mV] Supply voltage.
 * @param state_of_charge_cpercent [c%] Remaining charge in centipercent, from 0 to 10000.
 * @param fault_flags  Active power-related and system faults. Zero means no reported fault.
 * @param mode  Current power operating mode.
 * @param external_power  One when an external charging or shore-power source is present, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_power_state_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t time_boot_ms, int16_t current_ma, uint16_t voltage_mv, uint16_t state_of_charge_cpercent, uint16_t fault_flags, uint8_t mode, uint8_t external_power)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_POWER_STATE_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_int16_t(buf, 4, current_ma);
    _mav_put_uint16_t(buf, 6, voltage_mv);
    _mav_put_uint16_t(buf, 8, state_of_charge_cpercent);
    _mav_put_uint16_t(buf, 10, fault_flags);
    _mav_put_uint8_t(buf, 12, mode);
    _mav_put_uint8_t(buf, 13, external_power);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_POWER_STATE_LEN);
#else
    mavlink_power_state_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.current_ma = current_ma;
    packet.voltage_mv = voltage_mv;
    packet.state_of_charge_cpercent = state_of_charge_cpercent;
    packet.fault_flags = fault_flags;
    packet.mode = mode;
    packet.external_power = external_power;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_POWER_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_POWER_STATE;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_POWER_STATE_MIN_LEN, MAVLINK_MSG_ID_POWER_STATE_LEN, MAVLINK_MSG_ID_POWER_STATE_CRC);
}

/**
 * @brief Pack a power_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param current_ma [mA] Supply current. A negative value indicates charging.
 * @param voltage_mv [mV] Supply voltage.
 * @param state_of_charge_cpercent [c%] Remaining charge in centipercent, from 0 to 10000.
 * @param fault_flags  Active power-related and system faults. Zero means no reported fault.
 * @param mode  Current power operating mode.
 * @param external_power  One when an external charging or shore-power source is present, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_power_state_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t time_boot_ms, int16_t current_ma, uint16_t voltage_mv, uint16_t state_of_charge_cpercent, uint16_t fault_flags, uint8_t mode, uint8_t external_power)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_POWER_STATE_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_int16_t(buf, 4, current_ma);
    _mav_put_uint16_t(buf, 6, voltage_mv);
    _mav_put_uint16_t(buf, 8, state_of_charge_cpercent);
    _mav_put_uint16_t(buf, 10, fault_flags);
    _mav_put_uint8_t(buf, 12, mode);
    _mav_put_uint8_t(buf, 13, external_power);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_POWER_STATE_LEN);
#else
    mavlink_power_state_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.current_ma = current_ma;
    packet.voltage_mv = voltage_mv;
    packet.state_of_charge_cpercent = state_of_charge_cpercent;
    packet.fault_flags = fault_flags;
    packet.mode = mode;
    packet.external_power = external_power;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_POWER_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_POWER_STATE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_POWER_STATE_MIN_LEN, MAVLINK_MSG_ID_POWER_STATE_LEN, MAVLINK_MSG_ID_POWER_STATE_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_POWER_STATE_MIN_LEN, MAVLINK_MSG_ID_POWER_STATE_LEN);
#endif
}

/**
 * @brief Pack a power_state message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param current_ma [mA] Supply current. A negative value indicates charging.
 * @param voltage_mv [mV] Supply voltage.
 * @param state_of_charge_cpercent [c%] Remaining charge in centipercent, from 0 to 10000.
 * @param fault_flags  Active power-related and system faults. Zero means no reported fault.
 * @param mode  Current power operating mode.
 * @param external_power  One when an external charging or shore-power source is present, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_power_state_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t time_boot_ms,int16_t current_ma,uint16_t voltage_mv,uint16_t state_of_charge_cpercent,uint16_t fault_flags,uint8_t mode,uint8_t external_power)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_POWER_STATE_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_int16_t(buf, 4, current_ma);
    _mav_put_uint16_t(buf, 6, voltage_mv);
    _mav_put_uint16_t(buf, 8, state_of_charge_cpercent);
    _mav_put_uint16_t(buf, 10, fault_flags);
    _mav_put_uint8_t(buf, 12, mode);
    _mav_put_uint8_t(buf, 13, external_power);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_POWER_STATE_LEN);
#else
    mavlink_power_state_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.current_ma = current_ma;
    packet.voltage_mv = voltage_mv;
    packet.state_of_charge_cpercent = state_of_charge_cpercent;
    packet.fault_flags = fault_flags;
    packet.mode = mode;
    packet.external_power = external_power;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_POWER_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_POWER_STATE;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_POWER_STATE_MIN_LEN, MAVLINK_MSG_ID_POWER_STATE_LEN, MAVLINK_MSG_ID_POWER_STATE_CRC);
}

/**
 * @brief Encode a power_state struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param power_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_power_state_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_power_state_t* power_state)
{
    return mavlink_msg_power_state_pack(system_id, component_id, msg, power_state->time_boot_ms, power_state->current_ma, power_state->voltage_mv, power_state->state_of_charge_cpercent, power_state->fault_flags, power_state->mode, power_state->external_power);
}

/**
 * @brief Encode a power_state struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param power_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_power_state_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_power_state_t* power_state)
{
    return mavlink_msg_power_state_pack_chan(system_id, component_id, chan, msg, power_state->time_boot_ms, power_state->current_ma, power_state->voltage_mv, power_state->state_of_charge_cpercent, power_state->fault_flags, power_state->mode, power_state->external_power);
}

/**
 * @brief Encode a power_state struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param power_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_power_state_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_power_state_t* power_state)
{
    return mavlink_msg_power_state_pack_status(system_id, component_id, _status, msg,  power_state->time_boot_ms, power_state->current_ma, power_state->voltage_mv, power_state->state_of_charge_cpercent, power_state->fault_flags, power_state->mode, power_state->external_power);
}

/**
 * @brief Send a power_state message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param current_ma [mA] Supply current. A negative value indicates charging.
 * @param voltage_mv [mV] Supply voltage.
 * @param state_of_charge_cpercent [c%] Remaining charge in centipercent, from 0 to 10000.
 * @param fault_flags  Active power-related and system faults. Zero means no reported fault.
 * @param mode  Current power operating mode.
 * @param external_power  One when an external charging or shore-power source is present, otherwise zero.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_power_state_send(mavlink_channel_t chan, uint32_t time_boot_ms, int16_t current_ma, uint16_t voltage_mv, uint16_t state_of_charge_cpercent, uint16_t fault_flags, uint8_t mode, uint8_t external_power)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_POWER_STATE_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_int16_t(buf, 4, current_ma);
    _mav_put_uint16_t(buf, 6, voltage_mv);
    _mav_put_uint16_t(buf, 8, state_of_charge_cpercent);
    _mav_put_uint16_t(buf, 10, fault_flags);
    _mav_put_uint8_t(buf, 12, mode);
    _mav_put_uint8_t(buf, 13, external_power);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_POWER_STATE, buf, MAVLINK_MSG_ID_POWER_STATE_MIN_LEN, MAVLINK_MSG_ID_POWER_STATE_LEN, MAVLINK_MSG_ID_POWER_STATE_CRC);
#else
    mavlink_power_state_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.current_ma = current_ma;
    packet.voltage_mv = voltage_mv;
    packet.state_of_charge_cpercent = state_of_charge_cpercent;
    packet.fault_flags = fault_flags;
    packet.mode = mode;
    packet.external_power = external_power;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_POWER_STATE, (const char *)&packet, MAVLINK_MSG_ID_POWER_STATE_MIN_LEN, MAVLINK_MSG_ID_POWER_STATE_LEN, MAVLINK_MSG_ID_POWER_STATE_CRC);
#endif
}

/**
 * @brief Send a power_state message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_power_state_send_struct(mavlink_channel_t chan, const mavlink_power_state_t* power_state)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_power_state_send(chan, power_state->time_boot_ms, power_state->current_ma, power_state->voltage_mv, power_state->state_of_charge_cpercent, power_state->fault_flags, power_state->mode, power_state->external_power);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_POWER_STATE, (const char *)power_state, MAVLINK_MSG_ID_POWER_STATE_MIN_LEN, MAVLINK_MSG_ID_POWER_STATE_LEN, MAVLINK_MSG_ID_POWER_STATE_CRC);
#endif
}

#if MAVLINK_MSG_ID_POWER_STATE_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_power_state_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, int16_t current_ma, uint16_t voltage_mv, uint16_t state_of_charge_cpercent, uint16_t fault_flags, uint8_t mode, uint8_t external_power)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_int16_t(buf, 4, current_ma);
    _mav_put_uint16_t(buf, 6, voltage_mv);
    _mav_put_uint16_t(buf, 8, state_of_charge_cpercent);
    _mav_put_uint16_t(buf, 10, fault_flags);
    _mav_put_uint8_t(buf, 12, mode);
    _mav_put_uint8_t(buf, 13, external_power);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_POWER_STATE, buf, MAVLINK_MSG_ID_POWER_STATE_MIN_LEN, MAVLINK_MSG_ID_POWER_STATE_LEN, MAVLINK_MSG_ID_POWER_STATE_CRC);
#else
    mavlink_power_state_t *packet = (mavlink_power_state_t *)msgbuf;
    packet->time_boot_ms = time_boot_ms;
    packet->current_ma = current_ma;
    packet->voltage_mv = voltage_mv;
    packet->state_of_charge_cpercent = state_of_charge_cpercent;
    packet->fault_flags = fault_flags;
    packet->mode = mode;
    packet->external_power = external_power;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_POWER_STATE, (const char *)packet, MAVLINK_MSG_ID_POWER_STATE_MIN_LEN, MAVLINK_MSG_ID_POWER_STATE_LEN, MAVLINK_MSG_ID_POWER_STATE_CRC);
#endif
}
#endif

#endif

// MESSAGE POWER_STATE UNPACKING


/**
 * @brief Get field time_boot_ms from power_state message
 *
 * @return [ms] Time since the reporting component booted.
 */
static inline uint32_t mavlink_msg_power_state_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field current_ma from power_state message
 *
 * @return [mA] Supply current. A negative value indicates charging.
 */
static inline int16_t mavlink_msg_power_state_get_current_ma(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  4);
}

/**
 * @brief Get field voltage_mv from power_state message
 *
 * @return [mV] Supply voltage.
 */
static inline uint16_t mavlink_msg_power_state_get_voltage_mv(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  6);
}

/**
 * @brief Get field state_of_charge_cpercent from power_state message
 *
 * @return [c%] Remaining charge in centipercent, from 0 to 10000.
 */
static inline uint16_t mavlink_msg_power_state_get_state_of_charge_cpercent(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  8);
}

/**
 * @brief Get field fault_flags from power_state message
 *
 * @return  Active power-related and system faults. Zero means no reported fault.
 */
static inline uint16_t mavlink_msg_power_state_get_fault_flags(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  10);
}

/**
 * @brief Get field mode from power_state message
 *
 * @return  Current power operating mode.
 */
static inline uint8_t mavlink_msg_power_state_get_mode(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  12);
}

/**
 * @brief Get field external_power from power_state message
 *
 * @return  One when an external charging or shore-power source is present, otherwise zero.
 */
static inline uint8_t mavlink_msg_power_state_get_external_power(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  13);
}

/**
 * @brief Decode a power_state message into a struct
 *
 * @param msg The message to decode
 * @param power_state C-struct to decode the message contents into
 */
static inline void mavlink_msg_power_state_decode(const mavlink_message_t* msg, mavlink_power_state_t* power_state)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    power_state->time_boot_ms = mavlink_msg_power_state_get_time_boot_ms(msg);
    power_state->current_ma = mavlink_msg_power_state_get_current_ma(msg);
    power_state->voltage_mv = mavlink_msg_power_state_get_voltage_mv(msg);
    power_state->state_of_charge_cpercent = mavlink_msg_power_state_get_state_of_charge_cpercent(msg);
    power_state->fault_flags = mavlink_msg_power_state_get_fault_flags(msg);
    power_state->mode = mavlink_msg_power_state_get_mode(msg);
    power_state->external_power = mavlink_msg_power_state_get_external_power(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_POWER_STATE_LEN? msg->len : MAVLINK_MSG_ID_POWER_STATE_LEN;
        memset(power_state, 0, MAVLINK_MSG_ID_POWER_STATE_LEN);
    memcpy(power_state, _MAV_PAYLOAD(msg), len);
#endif
}
