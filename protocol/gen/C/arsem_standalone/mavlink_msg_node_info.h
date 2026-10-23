#pragma once
// MESSAGE NODE_INFO PACKING

#define MAVLINK_MSG_ID_NODE_INFO 200000


typedef struct __mavlink_node_info_t {
 uint32_t capabilities; /*<  Functions implemented by this component.*/
 uint32_t build_id; /*<  Implementation-defined build identifier. Use the low 32 bits of the source revision when available.*/
 uint8_t schema_major; /*<  Major version of this application message schema. A change indicates wire-level incompatibility.*/
 uint8_t schema_minor; /*<  Minor version of this application message schema. A change adds backward-compatible behavior.*/
 uint8_t schema_patch; /*<  Patch version of this application message schema. A change clarifies or fixes behavior without changing fields.*/
 uint8_t firmware_major; /*<  Firmware major version.*/
 uint8_t firmware_minor; /*<  Firmware minor version.*/
 uint8_t firmware_patch; /*<  Firmware patch version.*/
 uint8_t role; /*<  Logical role of this component.*/
} mavlink_node_info_t;

#define MAVLINK_MSG_ID_NODE_INFO_LEN 15
#define MAVLINK_MSG_ID_NODE_INFO_MIN_LEN 15
#define MAVLINK_MSG_ID_200000_LEN 15
#define MAVLINK_MSG_ID_200000_MIN_LEN 15

#define MAVLINK_MSG_ID_NODE_INFO_CRC 139
#define MAVLINK_MSG_ID_200000_CRC 139



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_NODE_INFO { \
    200000, \
    "NODE_INFO", \
    9, \
    {  { "capabilities", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_node_info_t, capabilities) }, \
         { "build_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_node_info_t, build_id) }, \
         { "schema_major", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_node_info_t, schema_major) }, \
         { "schema_minor", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_node_info_t, schema_minor) }, \
         { "schema_patch", NULL, MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_node_info_t, schema_patch) }, \
         { "firmware_major", NULL, MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_node_info_t, firmware_major) }, \
         { "firmware_minor", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_node_info_t, firmware_minor) }, \
         { "firmware_patch", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_node_info_t, firmware_patch) }, \
         { "role", NULL, MAVLINK_TYPE_UINT8_T, 0, 14, offsetof(mavlink_node_info_t, role) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_NODE_INFO { \
    "NODE_INFO", \
    9, \
    {  { "capabilities", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_node_info_t, capabilities) }, \
         { "build_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_node_info_t, build_id) }, \
         { "schema_major", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_node_info_t, schema_major) }, \
         { "schema_minor", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_node_info_t, schema_minor) }, \
         { "schema_patch", NULL, MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_node_info_t, schema_patch) }, \
         { "firmware_major", NULL, MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_node_info_t, firmware_major) }, \
         { "firmware_minor", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_node_info_t, firmware_minor) }, \
         { "firmware_patch", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_node_info_t, firmware_patch) }, \
         { "role", NULL, MAVLINK_TYPE_UINT8_T, 0, 14, offsetof(mavlink_node_info_t, role) }, \
         } \
}
#endif

/**
 * @brief Pack a node_info message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param capabilities  Functions implemented by this component.
 * @param build_id  Implementation-defined build identifier. Use the low 32 bits of the source revision when available.
 * @param schema_major  Major version of this application message schema. A change indicates wire-level incompatibility.
 * @param schema_minor  Minor version of this application message schema. A change adds backward-compatible behavior.
 * @param schema_patch  Patch version of this application message schema. A change clarifies or fixes behavior without changing fields.
 * @param firmware_major  Firmware major version.
 * @param firmware_minor  Firmware minor version.
 * @param firmware_patch  Firmware patch version.
 * @param role  Logical role of this component.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_node_info_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t capabilities, uint32_t build_id, uint8_t schema_major, uint8_t schema_minor, uint8_t schema_patch, uint8_t firmware_major, uint8_t firmware_minor, uint8_t firmware_patch, uint8_t role)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NODE_INFO_LEN];
    _mav_put_uint32_t(buf, 0, capabilities);
    _mav_put_uint32_t(buf, 4, build_id);
    _mav_put_uint8_t(buf, 8, schema_major);
    _mav_put_uint8_t(buf, 9, schema_minor);
    _mav_put_uint8_t(buf, 10, schema_patch);
    _mav_put_uint8_t(buf, 11, firmware_major);
    _mav_put_uint8_t(buf, 12, firmware_minor);
    _mav_put_uint8_t(buf, 13, firmware_patch);
    _mav_put_uint8_t(buf, 14, role);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_NODE_INFO_LEN);
#else
    mavlink_node_info_t packet;
    packet.capabilities = capabilities;
    packet.build_id = build_id;
    packet.schema_major = schema_major;
    packet.schema_minor = schema_minor;
    packet.schema_patch = schema_patch;
    packet.firmware_major = firmware_major;
    packet.firmware_minor = firmware_minor;
    packet.firmware_patch = firmware_patch;
    packet.role = role;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_NODE_INFO_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_NODE_INFO;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_NODE_INFO_MIN_LEN, MAVLINK_MSG_ID_NODE_INFO_LEN, MAVLINK_MSG_ID_NODE_INFO_CRC);
}

/**
 * @brief Pack a node_info message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param capabilities  Functions implemented by this component.
 * @param build_id  Implementation-defined build identifier. Use the low 32 bits of the source revision when available.
 * @param schema_major  Major version of this application message schema. A change indicates wire-level incompatibility.
 * @param schema_minor  Minor version of this application message schema. A change adds backward-compatible behavior.
 * @param schema_patch  Patch version of this application message schema. A change clarifies or fixes behavior without changing fields.
 * @param firmware_major  Firmware major version.
 * @param firmware_minor  Firmware minor version.
 * @param firmware_patch  Firmware patch version.
 * @param role  Logical role of this component.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_node_info_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t capabilities, uint32_t build_id, uint8_t schema_major, uint8_t schema_minor, uint8_t schema_patch, uint8_t firmware_major, uint8_t firmware_minor, uint8_t firmware_patch, uint8_t role)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NODE_INFO_LEN];
    _mav_put_uint32_t(buf, 0, capabilities);
    _mav_put_uint32_t(buf, 4, build_id);
    _mav_put_uint8_t(buf, 8, schema_major);
    _mav_put_uint8_t(buf, 9, schema_minor);
    _mav_put_uint8_t(buf, 10, schema_patch);
    _mav_put_uint8_t(buf, 11, firmware_major);
    _mav_put_uint8_t(buf, 12, firmware_minor);
    _mav_put_uint8_t(buf, 13, firmware_patch);
    _mav_put_uint8_t(buf, 14, role);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_NODE_INFO_LEN);
#else
    mavlink_node_info_t packet;
    packet.capabilities = capabilities;
    packet.build_id = build_id;
    packet.schema_major = schema_major;
    packet.schema_minor = schema_minor;
    packet.schema_patch = schema_patch;
    packet.firmware_major = firmware_major;
    packet.firmware_minor = firmware_minor;
    packet.firmware_patch = firmware_patch;
    packet.role = role;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_NODE_INFO_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_NODE_INFO;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_NODE_INFO_MIN_LEN, MAVLINK_MSG_ID_NODE_INFO_LEN, MAVLINK_MSG_ID_NODE_INFO_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_NODE_INFO_MIN_LEN, MAVLINK_MSG_ID_NODE_INFO_LEN);
#endif
}

/**
 * @brief Pack a node_info message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param capabilities  Functions implemented by this component.
 * @param build_id  Implementation-defined build identifier. Use the low 32 bits of the source revision when available.
 * @param schema_major  Major version of this application message schema. A change indicates wire-level incompatibility.
 * @param schema_minor  Minor version of this application message schema. A change adds backward-compatible behavior.
 * @param schema_patch  Patch version of this application message schema. A change clarifies or fixes behavior without changing fields.
 * @param firmware_major  Firmware major version.
 * @param firmware_minor  Firmware minor version.
 * @param firmware_patch  Firmware patch version.
 * @param role  Logical role of this component.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_node_info_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t capabilities,uint32_t build_id,uint8_t schema_major,uint8_t schema_minor,uint8_t schema_patch,uint8_t firmware_major,uint8_t firmware_minor,uint8_t firmware_patch,uint8_t role)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NODE_INFO_LEN];
    _mav_put_uint32_t(buf, 0, capabilities);
    _mav_put_uint32_t(buf, 4, build_id);
    _mav_put_uint8_t(buf, 8, schema_major);
    _mav_put_uint8_t(buf, 9, schema_minor);
    _mav_put_uint8_t(buf, 10, schema_patch);
    _mav_put_uint8_t(buf, 11, firmware_major);
    _mav_put_uint8_t(buf, 12, firmware_minor);
    _mav_put_uint8_t(buf, 13, firmware_patch);
    _mav_put_uint8_t(buf, 14, role);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_NODE_INFO_LEN);
#else
    mavlink_node_info_t packet;
    packet.capabilities = capabilities;
    packet.build_id = build_id;
    packet.schema_major = schema_major;
    packet.schema_minor = schema_minor;
    packet.schema_patch = schema_patch;
    packet.firmware_major = firmware_major;
    packet.firmware_minor = firmware_minor;
    packet.firmware_patch = firmware_patch;
    packet.role = role;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_NODE_INFO_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_NODE_INFO;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_NODE_INFO_MIN_LEN, MAVLINK_MSG_ID_NODE_INFO_LEN, MAVLINK_MSG_ID_NODE_INFO_CRC);
}

/**
 * @brief Encode a node_info struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param node_info C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_node_info_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_node_info_t* node_info)
{
    return mavlink_msg_node_info_pack(system_id, component_id, msg, node_info->capabilities, node_info->build_id, node_info->schema_major, node_info->schema_minor, node_info->schema_patch, node_info->firmware_major, node_info->firmware_minor, node_info->firmware_patch, node_info->role);
}

/**
 * @brief Encode a node_info struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param node_info C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_node_info_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_node_info_t* node_info)
{
    return mavlink_msg_node_info_pack_chan(system_id, component_id, chan, msg, node_info->capabilities, node_info->build_id, node_info->schema_major, node_info->schema_minor, node_info->schema_patch, node_info->firmware_major, node_info->firmware_minor, node_info->firmware_patch, node_info->role);
}

/**
 * @brief Encode a node_info struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param node_info C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_node_info_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_node_info_t* node_info)
{
    return mavlink_msg_node_info_pack_status(system_id, component_id, _status, msg,  node_info->capabilities, node_info->build_id, node_info->schema_major, node_info->schema_minor, node_info->schema_patch, node_info->firmware_major, node_info->firmware_minor, node_info->firmware_patch, node_info->role);
}

/**
 * @brief Send a node_info message
 * @param chan MAVLink channel to send the message
 *
 * @param capabilities  Functions implemented by this component.
 * @param build_id  Implementation-defined build identifier. Use the low 32 bits of the source revision when available.
 * @param schema_major  Major version of this application message schema. A change indicates wire-level incompatibility.
 * @param schema_minor  Minor version of this application message schema. A change adds backward-compatible behavior.
 * @param schema_patch  Patch version of this application message schema. A change clarifies or fixes behavior without changing fields.
 * @param firmware_major  Firmware major version.
 * @param firmware_minor  Firmware minor version.
 * @param firmware_patch  Firmware patch version.
 * @param role  Logical role of this component.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_node_info_send(mavlink_channel_t chan, uint32_t capabilities, uint32_t build_id, uint8_t schema_major, uint8_t schema_minor, uint8_t schema_patch, uint8_t firmware_major, uint8_t firmware_minor, uint8_t firmware_patch, uint8_t role)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NODE_INFO_LEN];
    _mav_put_uint32_t(buf, 0, capabilities);
    _mav_put_uint32_t(buf, 4, build_id);
    _mav_put_uint8_t(buf, 8, schema_major);
    _mav_put_uint8_t(buf, 9, schema_minor);
    _mav_put_uint8_t(buf, 10, schema_patch);
    _mav_put_uint8_t(buf, 11, firmware_major);
    _mav_put_uint8_t(buf, 12, firmware_minor);
    _mav_put_uint8_t(buf, 13, firmware_patch);
    _mav_put_uint8_t(buf, 14, role);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NODE_INFO, buf, MAVLINK_MSG_ID_NODE_INFO_MIN_LEN, MAVLINK_MSG_ID_NODE_INFO_LEN, MAVLINK_MSG_ID_NODE_INFO_CRC);
#else
    mavlink_node_info_t packet;
    packet.capabilities = capabilities;
    packet.build_id = build_id;
    packet.schema_major = schema_major;
    packet.schema_minor = schema_minor;
    packet.schema_patch = schema_patch;
    packet.firmware_major = firmware_major;
    packet.firmware_minor = firmware_minor;
    packet.firmware_patch = firmware_patch;
    packet.role = role;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NODE_INFO, (const char *)&packet, MAVLINK_MSG_ID_NODE_INFO_MIN_LEN, MAVLINK_MSG_ID_NODE_INFO_LEN, MAVLINK_MSG_ID_NODE_INFO_CRC);
#endif
}

/**
 * @brief Send a node_info message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_node_info_send_struct(mavlink_channel_t chan, const mavlink_node_info_t* node_info)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_node_info_send(chan, node_info->capabilities, node_info->build_id, node_info->schema_major, node_info->schema_minor, node_info->schema_patch, node_info->firmware_major, node_info->firmware_minor, node_info->firmware_patch, node_info->role);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NODE_INFO, (const char *)node_info, MAVLINK_MSG_ID_NODE_INFO_MIN_LEN, MAVLINK_MSG_ID_NODE_INFO_LEN, MAVLINK_MSG_ID_NODE_INFO_CRC);
#endif
}

#if MAVLINK_MSG_ID_NODE_INFO_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_node_info_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t capabilities, uint32_t build_id, uint8_t schema_major, uint8_t schema_minor, uint8_t schema_patch, uint8_t firmware_major, uint8_t firmware_minor, uint8_t firmware_patch, uint8_t role)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, capabilities);
    _mav_put_uint32_t(buf, 4, build_id);
    _mav_put_uint8_t(buf, 8, schema_major);
    _mav_put_uint8_t(buf, 9, schema_minor);
    _mav_put_uint8_t(buf, 10, schema_patch);
    _mav_put_uint8_t(buf, 11, firmware_major);
    _mav_put_uint8_t(buf, 12, firmware_minor);
    _mav_put_uint8_t(buf, 13, firmware_patch);
    _mav_put_uint8_t(buf, 14, role);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NODE_INFO, buf, MAVLINK_MSG_ID_NODE_INFO_MIN_LEN, MAVLINK_MSG_ID_NODE_INFO_LEN, MAVLINK_MSG_ID_NODE_INFO_CRC);
#else
    mavlink_node_info_t *packet = (mavlink_node_info_t *)msgbuf;
    packet->capabilities = capabilities;
    packet->build_id = build_id;
    packet->schema_major = schema_major;
    packet->schema_minor = schema_minor;
    packet->schema_patch = schema_patch;
    packet->firmware_major = firmware_major;
    packet->firmware_minor = firmware_minor;
    packet->firmware_patch = firmware_patch;
    packet->role = role;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NODE_INFO, (const char *)packet, MAVLINK_MSG_ID_NODE_INFO_MIN_LEN, MAVLINK_MSG_ID_NODE_INFO_LEN, MAVLINK_MSG_ID_NODE_INFO_CRC);
#endif
}
#endif

#endif

// MESSAGE NODE_INFO UNPACKING


/**
 * @brief Get field capabilities from node_info message
 *
 * @return  Functions implemented by this component.
 */
static inline uint32_t mavlink_msg_node_info_get_capabilities(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field build_id from node_info message
 *
 * @return  Implementation-defined build identifier. Use the low 32 bits of the source revision when available.
 */
static inline uint32_t mavlink_msg_node_info_get_build_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  4);
}

/**
 * @brief Get field schema_major from node_info message
 *
 * @return  Major version of this application message schema. A change indicates wire-level incompatibility.
 */
static inline uint8_t mavlink_msg_node_info_get_schema_major(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  8);
}

/**
 * @brief Get field schema_minor from node_info message
 *
 * @return  Minor version of this application message schema. A change adds backward-compatible behavior.
 */
static inline uint8_t mavlink_msg_node_info_get_schema_minor(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  9);
}

/**
 * @brief Get field schema_patch from node_info message
 *
 * @return  Patch version of this application message schema. A change clarifies or fixes behavior without changing fields.
 */
static inline uint8_t mavlink_msg_node_info_get_schema_patch(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  10);
}

/**
 * @brief Get field firmware_major from node_info message
 *
 * @return  Firmware major version.
 */
static inline uint8_t mavlink_msg_node_info_get_firmware_major(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  11);
}

/**
 * @brief Get field firmware_minor from node_info message
 *
 * @return  Firmware minor version.
 */
static inline uint8_t mavlink_msg_node_info_get_firmware_minor(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  12);
}

/**
 * @brief Get field firmware_patch from node_info message
 *
 * @return  Firmware patch version.
 */
static inline uint8_t mavlink_msg_node_info_get_firmware_patch(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  13);
}

/**
 * @brief Get field role from node_info message
 *
 * @return  Logical role of this component.
 */
static inline uint8_t mavlink_msg_node_info_get_role(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  14);
}

/**
 * @brief Decode a node_info message into a struct
 *
 * @param msg The message to decode
 * @param node_info C-struct to decode the message contents into
 */
static inline void mavlink_msg_node_info_decode(const mavlink_message_t* msg, mavlink_node_info_t* node_info)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    node_info->capabilities = mavlink_msg_node_info_get_capabilities(msg);
    node_info->build_id = mavlink_msg_node_info_get_build_id(msg);
    node_info->schema_major = mavlink_msg_node_info_get_schema_major(msg);
    node_info->schema_minor = mavlink_msg_node_info_get_schema_minor(msg);
    node_info->schema_patch = mavlink_msg_node_info_get_schema_patch(msg);
    node_info->firmware_major = mavlink_msg_node_info_get_firmware_major(msg);
    node_info->firmware_minor = mavlink_msg_node_info_get_firmware_minor(msg);
    node_info->firmware_patch = mavlink_msg_node_info_get_firmware_patch(msg);
    node_info->role = mavlink_msg_node_info_get_role(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_NODE_INFO_LEN? msg->len : MAVLINK_MSG_ID_NODE_INFO_LEN;
        memset(node_info, 0, MAVLINK_MSG_ID_NODE_INFO_LEN);
    memcpy(node_info, _MAV_PAYLOAD(msg), len);
#endif
}
