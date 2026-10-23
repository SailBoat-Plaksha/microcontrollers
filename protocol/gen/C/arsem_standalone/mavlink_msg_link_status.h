#pragma once
// MESSAGE LINK_STATUS PACKING

#define MAVLINK_MSG_ID_LINK_STATUS 200006


typedef struct __mavlink_link_status_t {
 uint32_t time_boot_ms; /*< [ms] Time since the reporting component booted.*/
 uint32_t rx_packets; /*<  Number of packets received on this link since boot.*/
 uint32_t tx_packets; /*<  Number of packets transmitted on this link since boot.*/
 uint16_t rx_errors; /*<  Number of receive errors since boot.*/
 uint16_t tx_errors; /*<  Number of transmit errors since boot.*/
 uint16_t address; /*<  Link-local address, such as the RYLR998 network address. Zero when not applicable.*/
 int16_t rssi_dbm; /*< [dBm] Received signal strength. INT16_MIN when unavailable.*/
 int16_t snr_cdb; /*< [cB] Signal-to-noise ratio in centibels. INT16_MIN when unavailable.*/
 uint8_t link_type; /*<  Physical link being reported.*/
 uint8_t connected; /*<  One when the link is ready to carry packets, otherwise zero.*/
} mavlink_link_status_t;

#define MAVLINK_MSG_ID_LINK_STATUS_LEN 24
#define MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN 24
#define MAVLINK_MSG_ID_200006_LEN 24
#define MAVLINK_MSG_ID_200006_MIN_LEN 24

#define MAVLINK_MSG_ID_LINK_STATUS_CRC 198
#define MAVLINK_MSG_ID_200006_CRC 198



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_LINK_STATUS { \
    200006, \
    "LINK_STATUS", \
    10, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_link_status_t, time_boot_ms) }, \
         { "rx_packets", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_link_status_t, rx_packets) }, \
         { "tx_packets", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_link_status_t, tx_packets) }, \
         { "rx_errors", NULL, MAVLINK_TYPE_UINT16_T, 0, 12, offsetof(mavlink_link_status_t, rx_errors) }, \
         { "tx_errors", NULL, MAVLINK_TYPE_UINT16_T, 0, 14, offsetof(mavlink_link_status_t, tx_errors) }, \
         { "address", NULL, MAVLINK_TYPE_UINT16_T, 0, 16, offsetof(mavlink_link_status_t, address) }, \
         { "rssi_dbm", NULL, MAVLINK_TYPE_INT16_T, 0, 18, offsetof(mavlink_link_status_t, rssi_dbm) }, \
         { "snr_cdb", NULL, MAVLINK_TYPE_INT16_T, 0, 20, offsetof(mavlink_link_status_t, snr_cdb) }, \
         { "link_type", NULL, MAVLINK_TYPE_UINT8_T, 0, 22, offsetof(mavlink_link_status_t, link_type) }, \
         { "connected", NULL, MAVLINK_TYPE_UINT8_T, 0, 23, offsetof(mavlink_link_status_t, connected) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_LINK_STATUS { \
    "LINK_STATUS", \
    10, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_link_status_t, time_boot_ms) }, \
         { "rx_packets", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_link_status_t, rx_packets) }, \
         { "tx_packets", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_link_status_t, tx_packets) }, \
         { "rx_errors", NULL, MAVLINK_TYPE_UINT16_T, 0, 12, offsetof(mavlink_link_status_t, rx_errors) }, \
         { "tx_errors", NULL, MAVLINK_TYPE_UINT16_T, 0, 14, offsetof(mavlink_link_status_t, tx_errors) }, \
         { "address", NULL, MAVLINK_TYPE_UINT16_T, 0, 16, offsetof(mavlink_link_status_t, address) }, \
         { "rssi_dbm", NULL, MAVLINK_TYPE_INT16_T, 0, 18, offsetof(mavlink_link_status_t, rssi_dbm) }, \
         { "snr_cdb", NULL, MAVLINK_TYPE_INT16_T, 0, 20, offsetof(mavlink_link_status_t, snr_cdb) }, \
         { "link_type", NULL, MAVLINK_TYPE_UINT8_T, 0, 22, offsetof(mavlink_link_status_t, link_type) }, \
         { "connected", NULL, MAVLINK_TYPE_UINT8_T, 0, 23, offsetof(mavlink_link_status_t, connected) }, \
         } \
}
#endif

/**
 * @brief Pack a link_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param rx_packets  Number of packets received on this link since boot.
 * @param tx_packets  Number of packets transmitted on this link since boot.
 * @param rx_errors  Number of receive errors since boot.
 * @param tx_errors  Number of transmit errors since boot.
 * @param address  Link-local address, such as the RYLR998 network address. Zero when not applicable.
 * @param rssi_dbm [dBm] Received signal strength. INT16_MIN when unavailable.
 * @param snr_cdb [cB] Signal-to-noise ratio in centibels. INT16_MIN when unavailable.
 * @param link_type  Physical link being reported.
 * @param connected  One when the link is ready to carry packets, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_link_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint32_t rx_packets, uint32_t tx_packets, uint16_t rx_errors, uint16_t tx_errors, uint16_t address, int16_t rssi_dbm, int16_t snr_cdb, uint8_t link_type, uint8_t connected)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LINK_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, rx_packets);
    _mav_put_uint32_t(buf, 8, tx_packets);
    _mav_put_uint16_t(buf, 12, rx_errors);
    _mav_put_uint16_t(buf, 14, tx_errors);
    _mav_put_uint16_t(buf, 16, address);
    _mav_put_int16_t(buf, 18, rssi_dbm);
    _mav_put_int16_t(buf, 20, snr_cdb);
    _mav_put_uint8_t(buf, 22, link_type);
    _mav_put_uint8_t(buf, 23, connected);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_LINK_STATUS_LEN);
#else
    mavlink_link_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.rx_packets = rx_packets;
    packet.tx_packets = tx_packets;
    packet.rx_errors = rx_errors;
    packet.tx_errors = tx_errors;
    packet.address = address;
    packet.rssi_dbm = rssi_dbm;
    packet.snr_cdb = snr_cdb;
    packet.link_type = link_type;
    packet.connected = connected;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_LINK_STATUS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_LINK_STATUS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN, MAVLINK_MSG_ID_LINK_STATUS_LEN, MAVLINK_MSG_ID_LINK_STATUS_CRC);
}

/**
 * @brief Pack a link_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param rx_packets  Number of packets received on this link since boot.
 * @param tx_packets  Number of packets transmitted on this link since boot.
 * @param rx_errors  Number of receive errors since boot.
 * @param tx_errors  Number of transmit errors since boot.
 * @param address  Link-local address, such as the RYLR998 network address. Zero when not applicable.
 * @param rssi_dbm [dBm] Received signal strength. INT16_MIN when unavailable.
 * @param snr_cdb [cB] Signal-to-noise ratio in centibels. INT16_MIN when unavailable.
 * @param link_type  Physical link being reported.
 * @param connected  One when the link is ready to carry packets, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_link_status_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint32_t rx_packets, uint32_t tx_packets, uint16_t rx_errors, uint16_t tx_errors, uint16_t address, int16_t rssi_dbm, int16_t snr_cdb, uint8_t link_type, uint8_t connected)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LINK_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, rx_packets);
    _mav_put_uint32_t(buf, 8, tx_packets);
    _mav_put_uint16_t(buf, 12, rx_errors);
    _mav_put_uint16_t(buf, 14, tx_errors);
    _mav_put_uint16_t(buf, 16, address);
    _mav_put_int16_t(buf, 18, rssi_dbm);
    _mav_put_int16_t(buf, 20, snr_cdb);
    _mav_put_uint8_t(buf, 22, link_type);
    _mav_put_uint8_t(buf, 23, connected);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_LINK_STATUS_LEN);
#else
    mavlink_link_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.rx_packets = rx_packets;
    packet.tx_packets = tx_packets;
    packet.rx_errors = rx_errors;
    packet.tx_errors = tx_errors;
    packet.address = address;
    packet.rssi_dbm = rssi_dbm;
    packet.snr_cdb = snr_cdb;
    packet.link_type = link_type;
    packet.connected = connected;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_LINK_STATUS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_LINK_STATUS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN, MAVLINK_MSG_ID_LINK_STATUS_LEN, MAVLINK_MSG_ID_LINK_STATUS_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN, MAVLINK_MSG_ID_LINK_STATUS_LEN);
#endif
}

/**
 * @brief Pack a link_status message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param rx_packets  Number of packets received on this link since boot.
 * @param tx_packets  Number of packets transmitted on this link since boot.
 * @param rx_errors  Number of receive errors since boot.
 * @param tx_errors  Number of transmit errors since boot.
 * @param address  Link-local address, such as the RYLR998 network address. Zero when not applicable.
 * @param rssi_dbm [dBm] Received signal strength. INT16_MIN when unavailable.
 * @param snr_cdb [cB] Signal-to-noise ratio in centibels. INT16_MIN when unavailable.
 * @param link_type  Physical link being reported.
 * @param connected  One when the link is ready to carry packets, otherwise zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_link_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t time_boot_ms,uint32_t rx_packets,uint32_t tx_packets,uint16_t rx_errors,uint16_t tx_errors,uint16_t address,int16_t rssi_dbm,int16_t snr_cdb,uint8_t link_type,uint8_t connected)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LINK_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, rx_packets);
    _mav_put_uint32_t(buf, 8, tx_packets);
    _mav_put_uint16_t(buf, 12, rx_errors);
    _mav_put_uint16_t(buf, 14, tx_errors);
    _mav_put_uint16_t(buf, 16, address);
    _mav_put_int16_t(buf, 18, rssi_dbm);
    _mav_put_int16_t(buf, 20, snr_cdb);
    _mav_put_uint8_t(buf, 22, link_type);
    _mav_put_uint8_t(buf, 23, connected);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_LINK_STATUS_LEN);
#else
    mavlink_link_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.rx_packets = rx_packets;
    packet.tx_packets = tx_packets;
    packet.rx_errors = rx_errors;
    packet.tx_errors = tx_errors;
    packet.address = address;
    packet.rssi_dbm = rssi_dbm;
    packet.snr_cdb = snr_cdb;
    packet.link_type = link_type;
    packet.connected = connected;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_LINK_STATUS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_LINK_STATUS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN, MAVLINK_MSG_ID_LINK_STATUS_LEN, MAVLINK_MSG_ID_LINK_STATUS_CRC);
}

/**
 * @brief Encode a link_status struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param link_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_link_status_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_link_status_t* link_status)
{
    return mavlink_msg_link_status_pack(system_id, component_id, msg, link_status->time_boot_ms, link_status->rx_packets, link_status->tx_packets, link_status->rx_errors, link_status->tx_errors, link_status->address, link_status->rssi_dbm, link_status->snr_cdb, link_status->link_type, link_status->connected);
}

/**
 * @brief Encode a link_status struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param link_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_link_status_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_link_status_t* link_status)
{
    return mavlink_msg_link_status_pack_chan(system_id, component_id, chan, msg, link_status->time_boot_ms, link_status->rx_packets, link_status->tx_packets, link_status->rx_errors, link_status->tx_errors, link_status->address, link_status->rssi_dbm, link_status->snr_cdb, link_status->link_type, link_status->connected);
}

/**
 * @brief Encode a link_status struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param link_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_link_status_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_link_status_t* link_status)
{
    return mavlink_msg_link_status_pack_status(system_id, component_id, _status, msg,  link_status->time_boot_ms, link_status->rx_packets, link_status->tx_packets, link_status->rx_errors, link_status->tx_errors, link_status->address, link_status->rssi_dbm, link_status->snr_cdb, link_status->link_type, link_status->connected);
}

/**
 * @brief Send a link_status message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms [ms] Time since the reporting component booted.
 * @param rx_packets  Number of packets received on this link since boot.
 * @param tx_packets  Number of packets transmitted on this link since boot.
 * @param rx_errors  Number of receive errors since boot.
 * @param tx_errors  Number of transmit errors since boot.
 * @param address  Link-local address, such as the RYLR998 network address. Zero when not applicable.
 * @param rssi_dbm [dBm] Received signal strength. INT16_MIN when unavailable.
 * @param snr_cdb [cB] Signal-to-noise ratio in centibels. INT16_MIN when unavailable.
 * @param link_type  Physical link being reported.
 * @param connected  One when the link is ready to carry packets, otherwise zero.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_link_status_send(mavlink_channel_t chan, uint32_t time_boot_ms, uint32_t rx_packets, uint32_t tx_packets, uint16_t rx_errors, uint16_t tx_errors, uint16_t address, int16_t rssi_dbm, int16_t snr_cdb, uint8_t link_type, uint8_t connected)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LINK_STATUS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, rx_packets);
    _mav_put_uint32_t(buf, 8, tx_packets);
    _mav_put_uint16_t(buf, 12, rx_errors);
    _mav_put_uint16_t(buf, 14, tx_errors);
    _mav_put_uint16_t(buf, 16, address);
    _mav_put_int16_t(buf, 18, rssi_dbm);
    _mav_put_int16_t(buf, 20, snr_cdb);
    _mav_put_uint8_t(buf, 22, link_type);
    _mav_put_uint8_t(buf, 23, connected);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LINK_STATUS, buf, MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN, MAVLINK_MSG_ID_LINK_STATUS_LEN, MAVLINK_MSG_ID_LINK_STATUS_CRC);
#else
    mavlink_link_status_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.rx_packets = rx_packets;
    packet.tx_packets = tx_packets;
    packet.rx_errors = rx_errors;
    packet.tx_errors = tx_errors;
    packet.address = address;
    packet.rssi_dbm = rssi_dbm;
    packet.snr_cdb = snr_cdb;
    packet.link_type = link_type;
    packet.connected = connected;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LINK_STATUS, (const char *)&packet, MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN, MAVLINK_MSG_ID_LINK_STATUS_LEN, MAVLINK_MSG_ID_LINK_STATUS_CRC);
#endif
}

/**
 * @brief Send a link_status message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_link_status_send_struct(mavlink_channel_t chan, const mavlink_link_status_t* link_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_link_status_send(chan, link_status->time_boot_ms, link_status->rx_packets, link_status->tx_packets, link_status->rx_errors, link_status->tx_errors, link_status->address, link_status->rssi_dbm, link_status->snr_cdb, link_status->link_type, link_status->connected);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LINK_STATUS, (const char *)link_status, MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN, MAVLINK_MSG_ID_LINK_STATUS_LEN, MAVLINK_MSG_ID_LINK_STATUS_CRC);
#endif
}

#if MAVLINK_MSG_ID_LINK_STATUS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_link_status_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, uint32_t rx_packets, uint32_t tx_packets, uint16_t rx_errors, uint16_t tx_errors, uint16_t address, int16_t rssi_dbm, int16_t snr_cdb, uint8_t link_type, uint8_t connected)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint32_t(buf, 4, rx_packets);
    _mav_put_uint32_t(buf, 8, tx_packets);
    _mav_put_uint16_t(buf, 12, rx_errors);
    _mav_put_uint16_t(buf, 14, tx_errors);
    _mav_put_uint16_t(buf, 16, address);
    _mav_put_int16_t(buf, 18, rssi_dbm);
    _mav_put_int16_t(buf, 20, snr_cdb);
    _mav_put_uint8_t(buf, 22, link_type);
    _mav_put_uint8_t(buf, 23, connected);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LINK_STATUS, buf, MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN, MAVLINK_MSG_ID_LINK_STATUS_LEN, MAVLINK_MSG_ID_LINK_STATUS_CRC);
#else
    mavlink_link_status_t *packet = (mavlink_link_status_t *)msgbuf;
    packet->time_boot_ms = time_boot_ms;
    packet->rx_packets = rx_packets;
    packet->tx_packets = tx_packets;
    packet->rx_errors = rx_errors;
    packet->tx_errors = tx_errors;
    packet->address = address;
    packet->rssi_dbm = rssi_dbm;
    packet->snr_cdb = snr_cdb;
    packet->link_type = link_type;
    packet->connected = connected;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LINK_STATUS, (const char *)packet, MAVLINK_MSG_ID_LINK_STATUS_MIN_LEN, MAVLINK_MSG_ID_LINK_STATUS_LEN, MAVLINK_MSG_ID_LINK_STATUS_CRC);
#endif
}
#endif

#endif

// MESSAGE LINK_STATUS UNPACKING


/**
 * @brief Get field time_boot_ms from link_status message
 *
 * @return [ms] Time since the reporting component booted.
 */
static inline uint32_t mavlink_msg_link_status_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field rx_packets from link_status message
 *
 * @return  Number of packets received on this link since boot.
 */
static inline uint32_t mavlink_msg_link_status_get_rx_packets(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  4);
}

/**
 * @brief Get field tx_packets from link_status message
 *
 * @return  Number of packets transmitted on this link since boot.
 */
static inline uint32_t mavlink_msg_link_status_get_tx_packets(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  8);
}

/**
 * @brief Get field rx_errors from link_status message
 *
 * @return  Number of receive errors since boot.
 */
static inline uint16_t mavlink_msg_link_status_get_rx_errors(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  12);
}

/**
 * @brief Get field tx_errors from link_status message
 *
 * @return  Number of transmit errors since boot.
 */
static inline uint16_t mavlink_msg_link_status_get_tx_errors(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  14);
}

/**
 * @brief Get field address from link_status message
 *
 * @return  Link-local address, such as the RYLR998 network address. Zero when not applicable.
 */
static inline uint16_t mavlink_msg_link_status_get_address(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  16);
}

/**
 * @brief Get field rssi_dbm from link_status message
 *
 * @return [dBm] Received signal strength. INT16_MIN when unavailable.
 */
static inline int16_t mavlink_msg_link_status_get_rssi_dbm(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  18);
}

/**
 * @brief Get field snr_cdb from link_status message
 *
 * @return [cB] Signal-to-noise ratio in centibels. INT16_MIN when unavailable.
 */
static inline int16_t mavlink_msg_link_status_get_snr_cdb(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  20);
}

/**
 * @brief Get field link_type from link_status message
 *
 * @return  Physical link being reported.
 */
static inline uint8_t mavlink_msg_link_status_get_link_type(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  22);
}

/**
 * @brief Get field connected from link_status message
 *
 * @return  One when the link is ready to carry packets, otherwise zero.
 */
static inline uint8_t mavlink_msg_link_status_get_connected(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  23);
}

/**
 * @brief Decode a link_status message into a struct
 *
 * @param msg The message to decode
 * @param link_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_link_status_decode(const mavlink_message_t* msg, mavlink_link_status_t* link_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    link_status->time_boot_ms = mavlink_msg_link_status_get_time_boot_ms(msg);
    link_status->rx_packets = mavlink_msg_link_status_get_rx_packets(msg);
    link_status->tx_packets = mavlink_msg_link_status_get_tx_packets(msg);
    link_status->rx_errors = mavlink_msg_link_status_get_rx_errors(msg);
    link_status->tx_errors = mavlink_msg_link_status_get_tx_errors(msg);
    link_status->address = mavlink_msg_link_status_get_address(msg);
    link_status->rssi_dbm = mavlink_msg_link_status_get_rssi_dbm(msg);
    link_status->snr_cdb = mavlink_msg_link_status_get_snr_cdb(msg);
    link_status->link_type = mavlink_msg_link_status_get_link_type(msg);
    link_status->connected = mavlink_msg_link_status_get_connected(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_LINK_STATUS_LEN? msg->len : MAVLINK_MSG_ID_LINK_STATUS_LEN;
        memset(link_status, 0, MAVLINK_MSG_ID_LINK_STATUS_LEN);
    memcpy(link_status, _MAV_PAYLOAD(msg), len);
#endif
}
