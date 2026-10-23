#pragma once
// MESSAGE ENVIRONMENTAL_SAMPLE PACKING

#define MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE 200004


typedef struct __mavlink_environmental_sample_t {
 uint64_t time_usec; /*< [us] UNIX epoch time or time since system boot. The receiver can infer the time base from the magnitude.*/
 uint16_t ph_milli; /*<  pH multiplied by 1000.*/
 int16_t water_temperature_cdeg; /*< [cdegC] Water temperature in centidegrees Celsius.*/
 uint16_t dissolved_oxygen_mg_l_x100; /*<  Dissolved oxygen in milligrams per litre multiplied by 100.*/
 uint16_t salinity_ppt_x100; /*<  Salinity in parts per thousand multiplied by 100.*/
 uint16_t turbidity_ntu_x10; /*<  Turbidity in nephelometric turbidity units multiplied by 10.*/
 uint16_t valid_fields; /*<  Bitmask identifying measurements that are valid.*/
 uint8_t sensor_instance; /*<  Zero-based sensor-suite instance.*/
} mavlink_environmental_sample_t;

#define MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN 21
#define MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN 21
#define MAVLINK_MSG_ID_200004_LEN 21
#define MAVLINK_MSG_ID_200004_MIN_LEN 21

#define MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_CRC 158
#define MAVLINK_MSG_ID_200004_CRC 158



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ENVIRONMENTAL_SAMPLE { \
    200004, \
    "ENVIRONMENTAL_SAMPLE", \
    8, \
    {  { "time_usec", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_environmental_sample_t, time_usec) }, \
         { "ph_milli", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_environmental_sample_t, ph_milli) }, \
         { "water_temperature_cdeg", NULL, MAVLINK_TYPE_INT16_T, 0, 10, offsetof(mavlink_environmental_sample_t, water_temperature_cdeg) }, \
         { "dissolved_oxygen_mg_l_x100", NULL, MAVLINK_TYPE_UINT16_T, 0, 12, offsetof(mavlink_environmental_sample_t, dissolved_oxygen_mg_l_x100) }, \
         { "salinity_ppt_x100", NULL, MAVLINK_TYPE_UINT16_T, 0, 14, offsetof(mavlink_environmental_sample_t, salinity_ppt_x100) }, \
         { "turbidity_ntu_x10", NULL, MAVLINK_TYPE_UINT16_T, 0, 16, offsetof(mavlink_environmental_sample_t, turbidity_ntu_x10) }, \
         { "valid_fields", NULL, MAVLINK_TYPE_UINT16_T, 0, 18, offsetof(mavlink_environmental_sample_t, valid_fields) }, \
         { "sensor_instance", NULL, MAVLINK_TYPE_UINT8_T, 0, 20, offsetof(mavlink_environmental_sample_t, sensor_instance) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ENVIRONMENTAL_SAMPLE { \
    "ENVIRONMENTAL_SAMPLE", \
    8, \
    {  { "time_usec", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_environmental_sample_t, time_usec) }, \
         { "ph_milli", NULL, MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_environmental_sample_t, ph_milli) }, \
         { "water_temperature_cdeg", NULL, MAVLINK_TYPE_INT16_T, 0, 10, offsetof(mavlink_environmental_sample_t, water_temperature_cdeg) }, \
         { "dissolved_oxygen_mg_l_x100", NULL, MAVLINK_TYPE_UINT16_T, 0, 12, offsetof(mavlink_environmental_sample_t, dissolved_oxygen_mg_l_x100) }, \
         { "salinity_ppt_x100", NULL, MAVLINK_TYPE_UINT16_T, 0, 14, offsetof(mavlink_environmental_sample_t, salinity_ppt_x100) }, \
         { "turbidity_ntu_x10", NULL, MAVLINK_TYPE_UINT16_T, 0, 16, offsetof(mavlink_environmental_sample_t, turbidity_ntu_x10) }, \
         { "valid_fields", NULL, MAVLINK_TYPE_UINT16_T, 0, 18, offsetof(mavlink_environmental_sample_t, valid_fields) }, \
         { "sensor_instance", NULL, MAVLINK_TYPE_UINT8_T, 0, 20, offsetof(mavlink_environmental_sample_t, sensor_instance) }, \
         } \
}
#endif

/**
 * @brief Pack a environmental_sample message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_usec [us] UNIX epoch time or time since system boot. The receiver can infer the time base from the magnitude.
 * @param ph_milli  pH multiplied by 1000.
 * @param water_temperature_cdeg [cdegC] Water temperature in centidegrees Celsius.
 * @param dissolved_oxygen_mg_l_x100  Dissolved oxygen in milligrams per litre multiplied by 100.
 * @param salinity_ppt_x100  Salinity in parts per thousand multiplied by 100.
 * @param turbidity_ntu_x10  Turbidity in nephelometric turbidity units multiplied by 10.
 * @param valid_fields  Bitmask identifying measurements that are valid.
 * @param sensor_instance  Zero-based sensor-suite instance.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_environmental_sample_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint64_t time_usec, uint16_t ph_milli, int16_t water_temperature_cdeg, uint16_t dissolved_oxygen_mg_l_x100, uint16_t salinity_ppt_x100, uint16_t turbidity_ntu_x10, uint16_t valid_fields, uint8_t sensor_instance)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_uint16_t(buf, 8, ph_milli);
    _mav_put_int16_t(buf, 10, water_temperature_cdeg);
    _mav_put_uint16_t(buf, 12, dissolved_oxygen_mg_l_x100);
    _mav_put_uint16_t(buf, 14, salinity_ppt_x100);
    _mav_put_uint16_t(buf, 16, turbidity_ntu_x10);
    _mav_put_uint16_t(buf, 18, valid_fields);
    _mav_put_uint8_t(buf, 20, sensor_instance);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN);
#else
    mavlink_environmental_sample_t packet;
    packet.time_usec = time_usec;
    packet.ph_milli = ph_milli;
    packet.water_temperature_cdeg = water_temperature_cdeg;
    packet.dissolved_oxygen_mg_l_x100 = dissolved_oxygen_mg_l_x100;
    packet.salinity_ppt_x100 = salinity_ppt_x100;
    packet.turbidity_ntu_x10 = turbidity_ntu_x10;
    packet.valid_fields = valid_fields;
    packet.sensor_instance = sensor_instance;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_CRC);
}

/**
 * @brief Pack a environmental_sample message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_usec [us] UNIX epoch time or time since system boot. The receiver can infer the time base from the magnitude.
 * @param ph_milli  pH multiplied by 1000.
 * @param water_temperature_cdeg [cdegC] Water temperature in centidegrees Celsius.
 * @param dissolved_oxygen_mg_l_x100  Dissolved oxygen in milligrams per litre multiplied by 100.
 * @param salinity_ppt_x100  Salinity in parts per thousand multiplied by 100.
 * @param turbidity_ntu_x10  Turbidity in nephelometric turbidity units multiplied by 10.
 * @param valid_fields  Bitmask identifying measurements that are valid.
 * @param sensor_instance  Zero-based sensor-suite instance.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_environmental_sample_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint64_t time_usec, uint16_t ph_milli, int16_t water_temperature_cdeg, uint16_t dissolved_oxygen_mg_l_x100, uint16_t salinity_ppt_x100, uint16_t turbidity_ntu_x10, uint16_t valid_fields, uint8_t sensor_instance)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_uint16_t(buf, 8, ph_milli);
    _mav_put_int16_t(buf, 10, water_temperature_cdeg);
    _mav_put_uint16_t(buf, 12, dissolved_oxygen_mg_l_x100);
    _mav_put_uint16_t(buf, 14, salinity_ppt_x100);
    _mav_put_uint16_t(buf, 16, turbidity_ntu_x10);
    _mav_put_uint16_t(buf, 18, valid_fields);
    _mav_put_uint8_t(buf, 20, sensor_instance);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN);
#else
    mavlink_environmental_sample_t packet;
    packet.time_usec = time_usec;
    packet.ph_milli = ph_milli;
    packet.water_temperature_cdeg = water_temperature_cdeg;
    packet.dissolved_oxygen_mg_l_x100 = dissolved_oxygen_mg_l_x100;
    packet.salinity_ppt_x100 = salinity_ppt_x100;
    packet.turbidity_ntu_x10 = turbidity_ntu_x10;
    packet.valid_fields = valid_fields;
    packet.sensor_instance = sensor_instance;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN);
#endif
}

/**
 * @brief Pack a environmental_sample message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_usec [us] UNIX epoch time or time since system boot. The receiver can infer the time base from the magnitude.
 * @param ph_milli  pH multiplied by 1000.
 * @param water_temperature_cdeg [cdegC] Water temperature in centidegrees Celsius.
 * @param dissolved_oxygen_mg_l_x100  Dissolved oxygen in milligrams per litre multiplied by 100.
 * @param salinity_ppt_x100  Salinity in parts per thousand multiplied by 100.
 * @param turbidity_ntu_x10  Turbidity in nephelometric turbidity units multiplied by 10.
 * @param valid_fields  Bitmask identifying measurements that are valid.
 * @param sensor_instance  Zero-based sensor-suite instance.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_environmental_sample_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint64_t time_usec,uint16_t ph_milli,int16_t water_temperature_cdeg,uint16_t dissolved_oxygen_mg_l_x100,uint16_t salinity_ppt_x100,uint16_t turbidity_ntu_x10,uint16_t valid_fields,uint8_t sensor_instance)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_uint16_t(buf, 8, ph_milli);
    _mav_put_int16_t(buf, 10, water_temperature_cdeg);
    _mav_put_uint16_t(buf, 12, dissolved_oxygen_mg_l_x100);
    _mav_put_uint16_t(buf, 14, salinity_ppt_x100);
    _mav_put_uint16_t(buf, 16, turbidity_ntu_x10);
    _mav_put_uint16_t(buf, 18, valid_fields);
    _mav_put_uint8_t(buf, 20, sensor_instance);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN);
#else
    mavlink_environmental_sample_t packet;
    packet.time_usec = time_usec;
    packet.ph_milli = ph_milli;
    packet.water_temperature_cdeg = water_temperature_cdeg;
    packet.dissolved_oxygen_mg_l_x100 = dissolved_oxygen_mg_l_x100;
    packet.salinity_ppt_x100 = salinity_ppt_x100;
    packet.turbidity_ntu_x10 = turbidity_ntu_x10;
    packet.valid_fields = valid_fields;
    packet.sensor_instance = sensor_instance;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_CRC);
}

/**
 * @brief Encode a environmental_sample struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param environmental_sample C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_environmental_sample_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_environmental_sample_t* environmental_sample)
{
    return mavlink_msg_environmental_sample_pack(system_id, component_id, msg, environmental_sample->time_usec, environmental_sample->ph_milli, environmental_sample->water_temperature_cdeg, environmental_sample->dissolved_oxygen_mg_l_x100, environmental_sample->salinity_ppt_x100, environmental_sample->turbidity_ntu_x10, environmental_sample->valid_fields, environmental_sample->sensor_instance);
}

/**
 * @brief Encode a environmental_sample struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param environmental_sample C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_environmental_sample_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_environmental_sample_t* environmental_sample)
{
    return mavlink_msg_environmental_sample_pack_chan(system_id, component_id, chan, msg, environmental_sample->time_usec, environmental_sample->ph_milli, environmental_sample->water_temperature_cdeg, environmental_sample->dissolved_oxygen_mg_l_x100, environmental_sample->salinity_ppt_x100, environmental_sample->turbidity_ntu_x10, environmental_sample->valid_fields, environmental_sample->sensor_instance);
}

/**
 * @brief Encode a environmental_sample struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param environmental_sample C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_environmental_sample_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_environmental_sample_t* environmental_sample)
{
    return mavlink_msg_environmental_sample_pack_status(system_id, component_id, _status, msg,  environmental_sample->time_usec, environmental_sample->ph_milli, environmental_sample->water_temperature_cdeg, environmental_sample->dissolved_oxygen_mg_l_x100, environmental_sample->salinity_ppt_x100, environmental_sample->turbidity_ntu_x10, environmental_sample->valid_fields, environmental_sample->sensor_instance);
}

/**
 * @brief Send a environmental_sample message
 * @param chan MAVLink channel to send the message
 *
 * @param time_usec [us] UNIX epoch time or time since system boot. The receiver can infer the time base from the magnitude.
 * @param ph_milli  pH multiplied by 1000.
 * @param water_temperature_cdeg [cdegC] Water temperature in centidegrees Celsius.
 * @param dissolved_oxygen_mg_l_x100  Dissolved oxygen in milligrams per litre multiplied by 100.
 * @param salinity_ppt_x100  Salinity in parts per thousand multiplied by 100.
 * @param turbidity_ntu_x10  Turbidity in nephelometric turbidity units multiplied by 10.
 * @param valid_fields  Bitmask identifying measurements that are valid.
 * @param sensor_instance  Zero-based sensor-suite instance.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_environmental_sample_send(mavlink_channel_t chan, uint64_t time_usec, uint16_t ph_milli, int16_t water_temperature_cdeg, uint16_t dissolved_oxygen_mg_l_x100, uint16_t salinity_ppt_x100, uint16_t turbidity_ntu_x10, uint16_t valid_fields, uint8_t sensor_instance)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_uint16_t(buf, 8, ph_milli);
    _mav_put_int16_t(buf, 10, water_temperature_cdeg);
    _mav_put_uint16_t(buf, 12, dissolved_oxygen_mg_l_x100);
    _mav_put_uint16_t(buf, 14, salinity_ppt_x100);
    _mav_put_uint16_t(buf, 16, turbidity_ntu_x10);
    _mav_put_uint16_t(buf, 18, valid_fields);
    _mav_put_uint8_t(buf, 20, sensor_instance);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE, buf, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_CRC);
#else
    mavlink_environmental_sample_t packet;
    packet.time_usec = time_usec;
    packet.ph_milli = ph_milli;
    packet.water_temperature_cdeg = water_temperature_cdeg;
    packet.dissolved_oxygen_mg_l_x100 = dissolved_oxygen_mg_l_x100;
    packet.salinity_ppt_x100 = salinity_ppt_x100;
    packet.turbidity_ntu_x10 = turbidity_ntu_x10;
    packet.valid_fields = valid_fields;
    packet.sensor_instance = sensor_instance;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE, (const char *)&packet, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_CRC);
#endif
}

/**
 * @brief Send a environmental_sample message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_environmental_sample_send_struct(mavlink_channel_t chan, const mavlink_environmental_sample_t* environmental_sample)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_environmental_sample_send(chan, environmental_sample->time_usec, environmental_sample->ph_milli, environmental_sample->water_temperature_cdeg, environmental_sample->dissolved_oxygen_mg_l_x100, environmental_sample->salinity_ppt_x100, environmental_sample->turbidity_ntu_x10, environmental_sample->valid_fields, environmental_sample->sensor_instance);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE, (const char *)environmental_sample, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_CRC);
#endif
}

#if MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_environmental_sample_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint64_t time_usec, uint16_t ph_milli, int16_t water_temperature_cdeg, uint16_t dissolved_oxygen_mg_l_x100, uint16_t salinity_ppt_x100, uint16_t turbidity_ntu_x10, uint16_t valid_fields, uint8_t sensor_instance)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_uint16_t(buf, 8, ph_milli);
    _mav_put_int16_t(buf, 10, water_temperature_cdeg);
    _mav_put_uint16_t(buf, 12, dissolved_oxygen_mg_l_x100);
    _mav_put_uint16_t(buf, 14, salinity_ppt_x100);
    _mav_put_uint16_t(buf, 16, turbidity_ntu_x10);
    _mav_put_uint16_t(buf, 18, valid_fields);
    _mav_put_uint8_t(buf, 20, sensor_instance);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE, buf, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_CRC);
#else
    mavlink_environmental_sample_t *packet = (mavlink_environmental_sample_t *)msgbuf;
    packet->time_usec = time_usec;
    packet->ph_milli = ph_milli;
    packet->water_temperature_cdeg = water_temperature_cdeg;
    packet->dissolved_oxygen_mg_l_x100 = dissolved_oxygen_mg_l_x100;
    packet->salinity_ppt_x100 = salinity_ppt_x100;
    packet->turbidity_ntu_x10 = turbidity_ntu_x10;
    packet->valid_fields = valid_fields;
    packet->sensor_instance = sensor_instance;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE, (const char *)packet, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_CRC);
#endif
}
#endif

#endif

// MESSAGE ENVIRONMENTAL_SAMPLE UNPACKING


/**
 * @brief Get field time_usec from environmental_sample message
 *
 * @return [us] UNIX epoch time or time since system boot. The receiver can infer the time base from the magnitude.
 */
static inline uint64_t mavlink_msg_environmental_sample_get_time_usec(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field ph_milli from environmental_sample message
 *
 * @return  pH multiplied by 1000.
 */
static inline uint16_t mavlink_msg_environmental_sample_get_ph_milli(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  8);
}

/**
 * @brief Get field water_temperature_cdeg from environmental_sample message
 *
 * @return [cdegC] Water temperature in centidegrees Celsius.
 */
static inline int16_t mavlink_msg_environmental_sample_get_water_temperature_cdeg(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  10);
}

/**
 * @brief Get field dissolved_oxygen_mg_l_x100 from environmental_sample message
 *
 * @return  Dissolved oxygen in milligrams per litre multiplied by 100.
 */
static inline uint16_t mavlink_msg_environmental_sample_get_dissolved_oxygen_mg_l_x100(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  12);
}

/**
 * @brief Get field salinity_ppt_x100 from environmental_sample message
 *
 * @return  Salinity in parts per thousand multiplied by 100.
 */
static inline uint16_t mavlink_msg_environmental_sample_get_salinity_ppt_x100(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  14);
}

/**
 * @brief Get field turbidity_ntu_x10 from environmental_sample message
 *
 * @return  Turbidity in nephelometric turbidity units multiplied by 10.
 */
static inline uint16_t mavlink_msg_environmental_sample_get_turbidity_ntu_x10(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  16);
}

/**
 * @brief Get field valid_fields from environmental_sample message
 *
 * @return  Bitmask identifying measurements that are valid.
 */
static inline uint16_t mavlink_msg_environmental_sample_get_valid_fields(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  18);
}

/**
 * @brief Get field sensor_instance from environmental_sample message
 *
 * @return  Zero-based sensor-suite instance.
 */
static inline uint8_t mavlink_msg_environmental_sample_get_sensor_instance(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  20);
}

/**
 * @brief Decode a environmental_sample message into a struct
 *
 * @param msg The message to decode
 * @param environmental_sample C-struct to decode the message contents into
 */
static inline void mavlink_msg_environmental_sample_decode(const mavlink_message_t* msg, mavlink_environmental_sample_t* environmental_sample)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    environmental_sample->time_usec = mavlink_msg_environmental_sample_get_time_usec(msg);
    environmental_sample->ph_milli = mavlink_msg_environmental_sample_get_ph_milli(msg);
    environmental_sample->water_temperature_cdeg = mavlink_msg_environmental_sample_get_water_temperature_cdeg(msg);
    environmental_sample->dissolved_oxygen_mg_l_x100 = mavlink_msg_environmental_sample_get_dissolved_oxygen_mg_l_x100(msg);
    environmental_sample->salinity_ppt_x100 = mavlink_msg_environmental_sample_get_salinity_ppt_x100(msg);
    environmental_sample->turbidity_ntu_x10 = mavlink_msg_environmental_sample_get_turbidity_ntu_x10(msg);
    environmental_sample->valid_fields = mavlink_msg_environmental_sample_get_valid_fields(msg);
    environmental_sample->sensor_instance = mavlink_msg_environmental_sample_get_sensor_instance(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN? msg->len : MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN;
        memset(environmental_sample, 0, MAVLINK_MSG_ID_ENVIRONMENTAL_SAMPLE_LEN);
    memcpy(environmental_sample, _MAV_PAYLOAD(msg), len);
#endif
}
