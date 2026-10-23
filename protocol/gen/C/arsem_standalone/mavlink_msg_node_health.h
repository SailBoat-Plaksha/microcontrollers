#pragma once
// MESSAGE NODE_HEALTH PACKING

#define MAVLINK_MSG_ID_NODE_HEALTH 200009


typedef struct __mavlink_node_health_t {
 uint32_t boot_id; /*<  Random per-boot identifier. All values from a previous boot are stale.*/
 uint32_t time_boot_ms; /*< [ms] Time since this node booted.*/
 uint16_t fault_flags; /*<  Active faults. Zero means no reported fault.*/
 uint16_t sensor_valid_fields; /*<  Measurements currently valid at this node. Unmeasured fields elsewhere, such as SCALED_IMU magnetometer or temperature, are invalid rather than zero.*/
 uint16_t queue_drops; /*<  Frames dropped by this node's bounded queues since boot.*/
 uint8_t reset_reason; /*<  Reason for the last reset. Report RESET_REASON_UNKNOWN rather than guessing.*/
 uint8_t feedback_valid; /*<  One when actuator position feedback is calibrated and fresh. Zero at nodes without actuator feedback.*/
} mavlink_node_health_t;

#define MAVLINK_MSG_ID_NODE_HEALTH_LEN 16
#define MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN 16
#define MAVLINK_MSG_ID_200009_LEN 16
#define MAVLINK_MSG_ID_200009_MIN_LEN 16

#define MAVLINK_MSG_ID_NODE_HEALTH_CRC 90
#define MAVLINK_MSG_ID_200009_CRC 90



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_NODE_HEALTH { \
    200009, \
    "NODE_HEALTH", \
    7, \
    {  { "boot_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_node_health_t, boot_id) }, \
         { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_node_health_t, time_boot_ms) }, \
         { "fault_flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_node_health_t, fault_flags) }, \
         { "sensor_valid_fields", NULL, MAVLINK_TYPE_UINT16_T, 0, 10, offsetof(mavlink_node_health_t, sensor_valid_fields) }, \
         { "queue_drops", NULL, MAVLINK_TYPE_UINT16_T, 0, 12, offsetof(mavlink_node_health_t, queue_drops) }, \
         { "reset_reason", NULL, MAVLINK_TYPE_UINT8_T, 0, 14, offsetof(mavlink_node_health_t, reset_reason) }, \
         { "feedback_valid", NULL, MAVLINK_TYPE_UINT8_T, 0, 15, offsetof(mavlink_node_health_t, feedback_valid) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_NODE_HEALTH { \
    "NODE_HEALTH", \
    7, \
    {  { "boot_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_node_health_t, boot_id) }, \
         { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_node_health_t, time_boot_ms) }, \
         { "fault_flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_node_health_t, fault_flags) }, \
         { "sensor_valid_fields", NULL, MAVLINK_TYPE_UINT16_T, 0, 10, offsetof(mavlink_node_health_t, sensor_valid_fields) }, \
         { "queue_drops", NULL, MAVLINK_TYPE_UINT16_T, 0, 12, offsetof(mavlink_node_health_t, queue_drops) }, \
         { "reset_reason", NULL, MAVLINK_TYPE_UINT8_T, 0, 14, offsetof(mavlink_node_health_t, reset_reason) }, \
         { "feedback_valid", NULL, MAVLINK_TYPE_UINT8_T, 0, 15, offsetof(mavlink_node_health_t, feedback_valid) }, \
         } \
}
#endif

/**
 * @brief Pack a node_health message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param boot_id  Random per-boot identifier. All values from a previous boot are stale.
 * @param time_boot_ms [ms] Time since this node booted.
 * @param fault_flags  Active faults. Zero means no reported fault.
 * @param sensor_valid_fields  Measurements currently valid at this node. Unmeasured fields elsewhere, such as SCALED_IMU magnetometer or temperature, are invalid rather than zero.
 * @param queue_drops  Frames dropped by this node's bounded queues since boot.
 * @param reset_reason  Reason for the last reset. Report RESET_REASON_UNKNOWN rather than guessing.
 * @param feedback_valid  One when actuator position feedback is calibrated and fresh. Zero at nodes without actuator feedback.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_node_health_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t boot_id, uint32_t time_boot_ms, uint16_t fault_flags, uint16_t sensor_valid_fields, uint16_t queue_drops, uint8_t reset_reason, uint8_t feedback_valid)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NODE_HEALTH_LEN];
    _mav_put_uint32_t(buf, 0, boot_id);
    _mav_put_uint32_t(buf, 4, time_boot_ms);
    _mav_put_uint16_t(buf, 8, fault_flags);
    _mav_put_uint16_t(buf, 10, sensor_valid_fields);
    _mav_put_uint16_t(buf, 12, queue_drops);
    _mav_put_uint8_t(buf, 14, reset_reason);
    _mav_put_uint8_t(buf, 15, feedback_valid);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_NODE_HEALTH_LEN);
#else
    mavlink_node_health_t packet;
    packet.boot_id = boot_id;
    packet.time_boot_ms = time_boot_ms;
    packet.fault_flags = fault_flags;
    packet.sensor_valid_fields = sensor_valid_fields;
    packet.queue_drops = queue_drops;
    packet.reset_reason = reset_reason;
    packet.feedback_valid = feedback_valid;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_NODE_HEALTH_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_NODE_HEALTH;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN, MAVLINK_MSG_ID_NODE_HEALTH_LEN, MAVLINK_MSG_ID_NODE_HEALTH_CRC);
}

/**
 * @brief Pack a node_health message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param boot_id  Random per-boot identifier. All values from a previous boot are stale.
 * @param time_boot_ms [ms] Time since this node booted.
 * @param fault_flags  Active faults. Zero means no reported fault.
 * @param sensor_valid_fields  Measurements currently valid at this node. Unmeasured fields elsewhere, such as SCALED_IMU magnetometer or temperature, are invalid rather than zero.
 * @param queue_drops  Frames dropped by this node's bounded queues since boot.
 * @param reset_reason  Reason for the last reset. Report RESET_REASON_UNKNOWN rather than guessing.
 * @param feedback_valid  One when actuator position feedback is calibrated and fresh. Zero at nodes without actuator feedback.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_node_health_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t boot_id, uint32_t time_boot_ms, uint16_t fault_flags, uint16_t sensor_valid_fields, uint16_t queue_drops, uint8_t reset_reason, uint8_t feedback_valid)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NODE_HEALTH_LEN];
    _mav_put_uint32_t(buf, 0, boot_id);
    _mav_put_uint32_t(buf, 4, time_boot_ms);
    _mav_put_uint16_t(buf, 8, fault_flags);
    _mav_put_uint16_t(buf, 10, sensor_valid_fields);
    _mav_put_uint16_t(buf, 12, queue_drops);
    _mav_put_uint8_t(buf, 14, reset_reason);
    _mav_put_uint8_t(buf, 15, feedback_valid);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_NODE_HEALTH_LEN);
#else
    mavlink_node_health_t packet;
    packet.boot_id = boot_id;
    packet.time_boot_ms = time_boot_ms;
    packet.fault_flags = fault_flags;
    packet.sensor_valid_fields = sensor_valid_fields;
    packet.queue_drops = queue_drops;
    packet.reset_reason = reset_reason;
    packet.feedback_valid = feedback_valid;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_NODE_HEALTH_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_NODE_HEALTH;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN, MAVLINK_MSG_ID_NODE_HEALTH_LEN, MAVLINK_MSG_ID_NODE_HEALTH_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN, MAVLINK_MSG_ID_NODE_HEALTH_LEN);
#endif
}

/**
 * @brief Pack a node_health message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param boot_id  Random per-boot identifier. All values from a previous boot are stale.
 * @param time_boot_ms [ms] Time since this node booted.
 * @param fault_flags  Active faults. Zero means no reported fault.
 * @param sensor_valid_fields  Measurements currently valid at this node. Unmeasured fields elsewhere, such as SCALED_IMU magnetometer or temperature, are invalid rather than zero.
 * @param queue_drops  Frames dropped by this node's bounded queues since boot.
 * @param reset_reason  Reason for the last reset. Report RESET_REASON_UNKNOWN rather than guessing.
 * @param feedback_valid  One when actuator position feedback is calibrated and fresh. Zero at nodes without actuator feedback.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_node_health_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t boot_id,uint32_t time_boot_ms,uint16_t fault_flags,uint16_t sensor_valid_fields,uint16_t queue_drops,uint8_t reset_reason,uint8_t feedback_valid)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NODE_HEALTH_LEN];
    _mav_put_uint32_t(buf, 0, boot_id);
    _mav_put_uint32_t(buf, 4, time_boot_ms);
    _mav_put_uint16_t(buf, 8, fault_flags);
    _mav_put_uint16_t(buf, 10, sensor_valid_fields);
    _mav_put_uint16_t(buf, 12, queue_drops);
    _mav_put_uint8_t(buf, 14, reset_reason);
    _mav_put_uint8_t(buf, 15, feedback_valid);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_NODE_HEALTH_LEN);
#else
    mavlink_node_health_t packet;
    packet.boot_id = boot_id;
    packet.time_boot_ms = time_boot_ms;
    packet.fault_flags = fault_flags;
    packet.sensor_valid_fields = sensor_valid_fields;
    packet.queue_drops = queue_drops;
    packet.reset_reason = reset_reason;
    packet.feedback_valid = feedback_valid;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_NODE_HEALTH_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_NODE_HEALTH;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN, MAVLINK_MSG_ID_NODE_HEALTH_LEN, MAVLINK_MSG_ID_NODE_HEALTH_CRC);
}

/**
 * @brief Encode a node_health struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param node_health C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_node_health_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_node_health_t* node_health)
{
    return mavlink_msg_node_health_pack(system_id, component_id, msg, node_health->boot_id, node_health->time_boot_ms, node_health->fault_flags, node_health->sensor_valid_fields, node_health->queue_drops, node_health->reset_reason, node_health->feedback_valid);
}

/**
 * @brief Encode a node_health struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param node_health C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_node_health_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_node_health_t* node_health)
{
    return mavlink_msg_node_health_pack_chan(system_id, component_id, chan, msg, node_health->boot_id, node_health->time_boot_ms, node_health->fault_flags, node_health->sensor_valid_fields, node_health->queue_drops, node_health->reset_reason, node_health->feedback_valid);
}

/**
 * @brief Encode a node_health struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param node_health C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_node_health_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_node_health_t* node_health)
{
    return mavlink_msg_node_health_pack_status(system_id, component_id, _status, msg,  node_health->boot_id, node_health->time_boot_ms, node_health->fault_flags, node_health->sensor_valid_fields, node_health->queue_drops, node_health->reset_reason, node_health->feedback_valid);
}

/**
 * @brief Send a node_health message
 * @param chan MAVLink channel to send the message
 *
 * @param boot_id  Random per-boot identifier. All values from a previous boot are stale.
 * @param time_boot_ms [ms] Time since this node booted.
 * @param fault_flags  Active faults. Zero means no reported fault.
 * @param sensor_valid_fields  Measurements currently valid at this node. Unmeasured fields elsewhere, such as SCALED_IMU magnetometer or temperature, are invalid rather than zero.
 * @param queue_drops  Frames dropped by this node's bounded queues since boot.
 * @param reset_reason  Reason for the last reset. Report RESET_REASON_UNKNOWN rather than guessing.
 * @param feedback_valid  One when actuator position feedback is calibrated and fresh. Zero at nodes without actuator feedback.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_node_health_send(mavlink_channel_t chan, uint32_t boot_id, uint32_t time_boot_ms, uint16_t fault_flags, uint16_t sensor_valid_fields, uint16_t queue_drops, uint8_t reset_reason, uint8_t feedback_valid)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NODE_HEALTH_LEN];
    _mav_put_uint32_t(buf, 0, boot_id);
    _mav_put_uint32_t(buf, 4, time_boot_ms);
    _mav_put_uint16_t(buf, 8, fault_flags);
    _mav_put_uint16_t(buf, 10, sensor_valid_fields);
    _mav_put_uint16_t(buf, 12, queue_drops);
    _mav_put_uint8_t(buf, 14, reset_reason);
    _mav_put_uint8_t(buf, 15, feedback_valid);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NODE_HEALTH, buf, MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN, MAVLINK_MSG_ID_NODE_HEALTH_LEN, MAVLINK_MSG_ID_NODE_HEALTH_CRC);
#else
    mavlink_node_health_t packet;
    packet.boot_id = boot_id;
    packet.time_boot_ms = time_boot_ms;
    packet.fault_flags = fault_flags;
    packet.sensor_valid_fields = sensor_valid_fields;
    packet.queue_drops = queue_drops;
    packet.reset_reason = reset_reason;
    packet.feedback_valid = feedback_valid;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NODE_HEALTH, (const char *)&packet, MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN, MAVLINK_MSG_ID_NODE_HEALTH_LEN, MAVLINK_MSG_ID_NODE_HEALTH_CRC);
#endif
}

/**
 * @brief Send a node_health message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_node_health_send_struct(mavlink_channel_t chan, const mavlink_node_health_t* node_health)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_node_health_send(chan, node_health->boot_id, node_health->time_boot_ms, node_health->fault_flags, node_health->sensor_valid_fields, node_health->queue_drops, node_health->reset_reason, node_health->feedback_valid);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NODE_HEALTH, (const char *)node_health, MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN, MAVLINK_MSG_ID_NODE_HEALTH_LEN, MAVLINK_MSG_ID_NODE_HEALTH_CRC);
#endif
}

#if MAVLINK_MSG_ID_NODE_HEALTH_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_node_health_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t boot_id, uint32_t time_boot_ms, uint16_t fault_flags, uint16_t sensor_valid_fields, uint16_t queue_drops, uint8_t reset_reason, uint8_t feedback_valid)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, boot_id);
    _mav_put_uint32_t(buf, 4, time_boot_ms);
    _mav_put_uint16_t(buf, 8, fault_flags);
    _mav_put_uint16_t(buf, 10, sensor_valid_fields);
    _mav_put_uint16_t(buf, 12, queue_drops);
    _mav_put_uint8_t(buf, 14, reset_reason);
    _mav_put_uint8_t(buf, 15, feedback_valid);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NODE_HEALTH, buf, MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN, MAVLINK_MSG_ID_NODE_HEALTH_LEN, MAVLINK_MSG_ID_NODE_HEALTH_CRC);
#else
    mavlink_node_health_t *packet = (mavlink_node_health_t *)msgbuf;
    packet->boot_id = boot_id;
    packet->time_boot_ms = time_boot_ms;
    packet->fault_flags = fault_flags;
    packet->sensor_valid_fields = sensor_valid_fields;
    packet->queue_drops = queue_drops;
    packet->reset_reason = reset_reason;
    packet->feedback_valid = feedback_valid;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NODE_HEALTH, (const char *)packet, MAVLINK_MSG_ID_NODE_HEALTH_MIN_LEN, MAVLINK_MSG_ID_NODE_HEALTH_LEN, MAVLINK_MSG_ID_NODE_HEALTH_CRC);
#endif
}
#endif

#endif

// MESSAGE NODE_HEALTH UNPACKING


/**
 * @brief Get field boot_id from node_health message
 *
 * @return  Random per-boot identifier. All values from a previous boot are stale.
 */
static inline uint32_t mavlink_msg_node_health_get_boot_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field time_boot_ms from node_health message
 *
 * @return [ms] Time since this node booted.
 */
static inline uint32_t mavlink_msg_node_health_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  4);
}

/**
 * @brief Get field fault_flags from node_health message
 *
 * @return  Active faults. Zero means no reported fault.
 */
static inline uint16_t mavlink_msg_node_health_get_fault_flags(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  8);
}

/**
 * @brief Get field sensor_valid_fields from node_health message
 *
 * @return  Measurements currently valid at this node. Unmeasured fields elsewhere, such as SCALED_IMU magnetometer or temperature, are invalid rather than zero.
 */
static inline uint16_t mavlink_msg_node_health_get_sensor_valid_fields(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  10);
}

/**
 * @brief Get field queue_drops from node_health message
 *
 * @return  Frames dropped by this node's bounded queues since boot.
 */
static inline uint16_t mavlink_msg_node_health_get_queue_drops(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  12);
}

/**
 * @brief Get field reset_reason from node_health message
 *
 * @return  Reason for the last reset. Report RESET_REASON_UNKNOWN rather than guessing.
 */
static inline uint8_t mavlink_msg_node_health_get_reset_reason(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  14);
}

/**
 * @brief Get field feedback_valid from node_health message
 *
 * @return  One when actuator position feedback is calibrated and fresh. Zero at nodes without actuator feedback.
 */
static inline uint8_t mavlink_msg_node_health_get_feedback_valid(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  15);
}

/**
 * @brief Decode a node_health message into a struct
 *
 * @param msg The message to decode
 * @param node_health C-struct to decode the message contents into
 */
static inline void mavlink_msg_node_health_decode(const mavlink_message_t* msg, mavlink_node_health_t* node_health)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    node_health->boot_id = mavlink_msg_node_health_get_boot_id(msg);
    node_health->time_boot_ms = mavlink_msg_node_health_get_time_boot_ms(msg);
    node_health->fault_flags = mavlink_msg_node_health_get_fault_flags(msg);
    node_health->sensor_valid_fields = mavlink_msg_node_health_get_sensor_valid_fields(msg);
    node_health->queue_drops = mavlink_msg_node_health_get_queue_drops(msg);
    node_health->reset_reason = mavlink_msg_node_health_get_reset_reason(msg);
    node_health->feedback_valid = mavlink_msg_node_health_get_feedback_valid(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_NODE_HEALTH_LEN? msg->len : MAVLINK_MSG_ID_NODE_HEALTH_LEN;
        memset(node_health, 0, MAVLINK_MSG_ID_NODE_HEALTH_LEN);
    memcpy(node_health, _MAV_PAYLOAD(msg), len);
#endif
}
