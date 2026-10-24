#pragma once
// MESSAGE IMU_SAMPLE PACKING

#define MAVLINK_MSG_ID_IMU_SAMPLE 200012


typedef struct __mavlink_imu_sample_t {
 uint32_t ground_session; /*<  Request session.*/
 uint32_t request_id; /*<  Request identifier.*/
 uint32_t time_boot_ms; /*< [ms] Acquisition owner's monotonic boot-relative acquisition timestamp.*/
 uint32_t sample_seq; /*<  Acquisition owner's sample sequence, increments only on new successful reads.*/
 uint32_t acquisition_boot_id; /*<  Pi boot identity at acquisition.*/
 uint32_t source_boot_id; /*<  Reporting node boot identity.*/
 uint16_t sample_age_ms; /*< [ms] Sample age at emission, saturated at 65535.*/
 uint16_t valid_fields; /*<  Measured field bits: accel 1, gyro 2, magnetometer 4. Temperature is not included.*/
 int16_t xacc; /*< [mG] X acceleration, milligravity.*/
 int16_t yacc; /*< [mG] Y acceleration.*/
 int16_t zacc; /*< [mG] Z acceleration.*/
 int16_t xgyro; /*< [mrad/s] X angular rate.*/
 int16_t ygyro; /*< [mrad/s] Y angular rate.*/
 int16_t zgyro; /*< [mrad/s] Z angular rate.*/
 int16_t xmag; /*< [mgauss] X magnetic field.*/
 int16_t ymag; /*< [mgauss] Y magnetic field.*/
 int16_t zmag; /*< [mgauss] Z magnetic field.*/
 uint8_t status; /*<  Result; non-OK samples have valid_fields zero.*/
 uint8_t path; /*<  Response path.*/
 uint8_t acquisition_component; /*<  Pi component 1.*/
 uint8_t target_system; /*<  Recipient system.*/
 uint8_t target_component; /*<  Recipient component.*/
} mavlink_imu_sample_t;

#define MAVLINK_MSG_ID_IMU_SAMPLE_LEN 51
#define MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN 51
#define MAVLINK_MSG_ID_200012_LEN 51
#define MAVLINK_MSG_ID_200012_MIN_LEN 51

#define MAVLINK_MSG_ID_IMU_SAMPLE_CRC 32
#define MAVLINK_MSG_ID_200012_CRC 32



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_IMU_SAMPLE { \
    200012, \
    "IMU_SAMPLE", \
    22, \
    {  { "ground_session", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_imu_sample_t, ground_session) }, \
         { "request_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_imu_sample_t, request_id) }, \
         { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_imu_sample_t, time_boot_ms) }, \
         { "sample_seq", NULL, MAVLINK_TYPE_UINT32_T, 0, 12, offsetof(mavlink_imu_sample_t, sample_seq) }, \
         { "acquisition_boot_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 16, offsetof(mavlink_imu_sample_t, acquisition_boot_id) }, \
         { "source_boot_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 20, offsetof(mavlink_imu_sample_t, source_boot_id) }, \
         { "sample_age_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 24, offsetof(mavlink_imu_sample_t, sample_age_ms) }, \
         { "valid_fields", NULL, MAVLINK_TYPE_UINT16_T, 0, 26, offsetof(mavlink_imu_sample_t, valid_fields) }, \
         { "xacc", NULL, MAVLINK_TYPE_INT16_T, 0, 28, offsetof(mavlink_imu_sample_t, xacc) }, \
         { "yacc", NULL, MAVLINK_TYPE_INT16_T, 0, 30, offsetof(mavlink_imu_sample_t, yacc) }, \
         { "zacc", NULL, MAVLINK_TYPE_INT16_T, 0, 32, offsetof(mavlink_imu_sample_t, zacc) }, \
         { "xgyro", NULL, MAVLINK_TYPE_INT16_T, 0, 34, offsetof(mavlink_imu_sample_t, xgyro) }, \
         { "ygyro", NULL, MAVLINK_TYPE_INT16_T, 0, 36, offsetof(mavlink_imu_sample_t, ygyro) }, \
         { "zgyro", NULL, MAVLINK_TYPE_INT16_T, 0, 38, offsetof(mavlink_imu_sample_t, zgyro) }, \
         { "xmag", NULL, MAVLINK_TYPE_INT16_T, 0, 40, offsetof(mavlink_imu_sample_t, xmag) }, \
         { "ymag", NULL, MAVLINK_TYPE_INT16_T, 0, 42, offsetof(mavlink_imu_sample_t, ymag) }, \
         { "zmag", NULL, MAVLINK_TYPE_INT16_T, 0, 44, offsetof(mavlink_imu_sample_t, zmag) }, \
         { "status", NULL, MAVLINK_TYPE_UINT8_T, 0, 46, offsetof(mavlink_imu_sample_t, status) }, \
         { "path", NULL, MAVLINK_TYPE_UINT8_T, 0, 47, offsetof(mavlink_imu_sample_t, path) }, \
         { "acquisition_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 48, offsetof(mavlink_imu_sample_t, acquisition_component) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 49, offsetof(mavlink_imu_sample_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 50, offsetof(mavlink_imu_sample_t, target_component) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_IMU_SAMPLE { \
    "IMU_SAMPLE", \
    22, \
    {  { "ground_session", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_imu_sample_t, ground_session) }, \
         { "request_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_imu_sample_t, request_id) }, \
         { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_imu_sample_t, time_boot_ms) }, \
         { "sample_seq", NULL, MAVLINK_TYPE_UINT32_T, 0, 12, offsetof(mavlink_imu_sample_t, sample_seq) }, \
         { "acquisition_boot_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 16, offsetof(mavlink_imu_sample_t, acquisition_boot_id) }, \
         { "source_boot_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 20, offsetof(mavlink_imu_sample_t, source_boot_id) }, \
         { "sample_age_ms", NULL, MAVLINK_TYPE_UINT16_T, 0, 24, offsetof(mavlink_imu_sample_t, sample_age_ms) }, \
         { "valid_fields", NULL, MAVLINK_TYPE_UINT16_T, 0, 26, offsetof(mavlink_imu_sample_t, valid_fields) }, \
         { "xacc", NULL, MAVLINK_TYPE_INT16_T, 0, 28, offsetof(mavlink_imu_sample_t, xacc) }, \
         { "yacc", NULL, MAVLINK_TYPE_INT16_T, 0, 30, offsetof(mavlink_imu_sample_t, yacc) }, \
         { "zacc", NULL, MAVLINK_TYPE_INT16_T, 0, 32, offsetof(mavlink_imu_sample_t, zacc) }, \
         { "xgyro", NULL, MAVLINK_TYPE_INT16_T, 0, 34, offsetof(mavlink_imu_sample_t, xgyro) }, \
         { "ygyro", NULL, MAVLINK_TYPE_INT16_T, 0, 36, offsetof(mavlink_imu_sample_t, ygyro) }, \
         { "zgyro", NULL, MAVLINK_TYPE_INT16_T, 0, 38, offsetof(mavlink_imu_sample_t, zgyro) }, \
         { "xmag", NULL, MAVLINK_TYPE_INT16_T, 0, 40, offsetof(mavlink_imu_sample_t, xmag) }, \
         { "ymag", NULL, MAVLINK_TYPE_INT16_T, 0, 42, offsetof(mavlink_imu_sample_t, ymag) }, \
         { "zmag", NULL, MAVLINK_TYPE_INT16_T, 0, 44, offsetof(mavlink_imu_sample_t, zmag) }, \
         { "status", NULL, MAVLINK_TYPE_UINT8_T, 0, 46, offsetof(mavlink_imu_sample_t, status) }, \
         { "path", NULL, MAVLINK_TYPE_UINT8_T, 0, 47, offsetof(mavlink_imu_sample_t, path) }, \
         { "acquisition_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 48, offsetof(mavlink_imu_sample_t, acquisition_component) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 49, offsetof(mavlink_imu_sample_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 50, offsetof(mavlink_imu_sample_t, target_component) }, \
         } \
}
#endif

/**
 * @brief Pack a imu_sample message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param ground_session  Request session.
 * @param request_id  Request identifier.
 * @param time_boot_ms [ms] Acquisition owner's monotonic boot-relative acquisition timestamp.
 * @param sample_seq  Acquisition owner's sample sequence, increments only on new successful reads.
 * @param acquisition_boot_id  Pi boot identity at acquisition.
 * @param source_boot_id  Reporting node boot identity.
 * @param sample_age_ms [ms] Sample age at emission, saturated at 65535.
 * @param valid_fields  Measured field bits: accel 1, gyro 2, magnetometer 4. Temperature is not included.
 * @param xacc [mG] X acceleration, milligravity.
 * @param yacc [mG] Y acceleration.
 * @param zacc [mG] Z acceleration.
 * @param xgyro [mrad/s] X angular rate.
 * @param ygyro [mrad/s] Y angular rate.
 * @param zgyro [mrad/s] Z angular rate.
 * @param xmag [mgauss] X magnetic field.
 * @param ymag [mgauss] Y magnetic field.
 * @param zmag [mgauss] Z magnetic field.
 * @param status  Result; non-OK samples have valid_fields zero.
 * @param path  Response path.
 * @param acquisition_component  Pi component 1.
 * @param target_system  Recipient system.
 * @param target_component  Recipient component.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_imu_sample_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t ground_session, uint32_t request_id, uint32_t time_boot_ms, uint32_t sample_seq, uint32_t acquisition_boot_id, uint32_t source_boot_id, uint16_t sample_age_ms, uint16_t valid_fields, int16_t xacc, int16_t yacc, int16_t zacc, int16_t xgyro, int16_t ygyro, int16_t zgyro, int16_t xmag, int16_t ymag, int16_t zmag, uint8_t status, uint8_t path, uint8_t acquisition_component, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_IMU_SAMPLE_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint32_t(buf, 8, time_boot_ms);
    _mav_put_uint32_t(buf, 12, sample_seq);
    _mav_put_uint32_t(buf, 16, acquisition_boot_id);
    _mav_put_uint32_t(buf, 20, source_boot_id);
    _mav_put_uint16_t(buf, 24, sample_age_ms);
    _mav_put_uint16_t(buf, 26, valid_fields);
    _mav_put_int16_t(buf, 28, xacc);
    _mav_put_int16_t(buf, 30, yacc);
    _mav_put_int16_t(buf, 32, zacc);
    _mav_put_int16_t(buf, 34, xgyro);
    _mav_put_int16_t(buf, 36, ygyro);
    _mav_put_int16_t(buf, 38, zgyro);
    _mav_put_int16_t(buf, 40, xmag);
    _mav_put_int16_t(buf, 42, ymag);
    _mav_put_int16_t(buf, 44, zmag);
    _mav_put_uint8_t(buf, 46, status);
    _mav_put_uint8_t(buf, 47, path);
    _mav_put_uint8_t(buf, 48, acquisition_component);
    _mav_put_uint8_t(buf, 49, target_system);
    _mav_put_uint8_t(buf, 50, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_IMU_SAMPLE_LEN);
#else
    mavlink_imu_sample_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.time_boot_ms = time_boot_ms;
    packet.sample_seq = sample_seq;
    packet.acquisition_boot_id = acquisition_boot_id;
    packet.source_boot_id = source_boot_id;
    packet.sample_age_ms = sample_age_ms;
    packet.valid_fields = valid_fields;
    packet.xacc = xacc;
    packet.yacc = yacc;
    packet.zacc = zacc;
    packet.xgyro = xgyro;
    packet.ygyro = ygyro;
    packet.zgyro = zgyro;
    packet.xmag = xmag;
    packet.ymag = ymag;
    packet.zmag = zmag;
    packet.status = status;
    packet.path = path;
    packet.acquisition_component = acquisition_component;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_IMU_SAMPLE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_IMU_SAMPLE;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_CRC);
}

/**
 * @brief Pack a imu_sample message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param ground_session  Request session.
 * @param request_id  Request identifier.
 * @param time_boot_ms [ms] Acquisition owner's monotonic boot-relative acquisition timestamp.
 * @param sample_seq  Acquisition owner's sample sequence, increments only on new successful reads.
 * @param acquisition_boot_id  Pi boot identity at acquisition.
 * @param source_boot_id  Reporting node boot identity.
 * @param sample_age_ms [ms] Sample age at emission, saturated at 65535.
 * @param valid_fields  Measured field bits: accel 1, gyro 2, magnetometer 4. Temperature is not included.
 * @param xacc [mG] X acceleration, milligravity.
 * @param yacc [mG] Y acceleration.
 * @param zacc [mG] Z acceleration.
 * @param xgyro [mrad/s] X angular rate.
 * @param ygyro [mrad/s] Y angular rate.
 * @param zgyro [mrad/s] Z angular rate.
 * @param xmag [mgauss] X magnetic field.
 * @param ymag [mgauss] Y magnetic field.
 * @param zmag [mgauss] Z magnetic field.
 * @param status  Result; non-OK samples have valid_fields zero.
 * @param path  Response path.
 * @param acquisition_component  Pi component 1.
 * @param target_system  Recipient system.
 * @param target_component  Recipient component.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_imu_sample_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t ground_session, uint32_t request_id, uint32_t time_boot_ms, uint32_t sample_seq, uint32_t acquisition_boot_id, uint32_t source_boot_id, uint16_t sample_age_ms, uint16_t valid_fields, int16_t xacc, int16_t yacc, int16_t zacc, int16_t xgyro, int16_t ygyro, int16_t zgyro, int16_t xmag, int16_t ymag, int16_t zmag, uint8_t status, uint8_t path, uint8_t acquisition_component, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_IMU_SAMPLE_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint32_t(buf, 8, time_boot_ms);
    _mav_put_uint32_t(buf, 12, sample_seq);
    _mav_put_uint32_t(buf, 16, acquisition_boot_id);
    _mav_put_uint32_t(buf, 20, source_boot_id);
    _mav_put_uint16_t(buf, 24, sample_age_ms);
    _mav_put_uint16_t(buf, 26, valid_fields);
    _mav_put_int16_t(buf, 28, xacc);
    _mav_put_int16_t(buf, 30, yacc);
    _mav_put_int16_t(buf, 32, zacc);
    _mav_put_int16_t(buf, 34, xgyro);
    _mav_put_int16_t(buf, 36, ygyro);
    _mav_put_int16_t(buf, 38, zgyro);
    _mav_put_int16_t(buf, 40, xmag);
    _mav_put_int16_t(buf, 42, ymag);
    _mav_put_int16_t(buf, 44, zmag);
    _mav_put_uint8_t(buf, 46, status);
    _mav_put_uint8_t(buf, 47, path);
    _mav_put_uint8_t(buf, 48, acquisition_component);
    _mav_put_uint8_t(buf, 49, target_system);
    _mav_put_uint8_t(buf, 50, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_IMU_SAMPLE_LEN);
#else
    mavlink_imu_sample_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.time_boot_ms = time_boot_ms;
    packet.sample_seq = sample_seq;
    packet.acquisition_boot_id = acquisition_boot_id;
    packet.source_boot_id = source_boot_id;
    packet.sample_age_ms = sample_age_ms;
    packet.valid_fields = valid_fields;
    packet.xacc = xacc;
    packet.yacc = yacc;
    packet.zacc = zacc;
    packet.xgyro = xgyro;
    packet.ygyro = ygyro;
    packet.zgyro = zgyro;
    packet.xmag = xmag;
    packet.ymag = ymag;
    packet.zmag = zmag;
    packet.status = status;
    packet.path = path;
    packet.acquisition_component = acquisition_component;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_IMU_SAMPLE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_IMU_SAMPLE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_LEN);
#endif
}

/**
 * @brief Pack a imu_sample message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param ground_session  Request session.
 * @param request_id  Request identifier.
 * @param time_boot_ms [ms] Acquisition owner's monotonic boot-relative acquisition timestamp.
 * @param sample_seq  Acquisition owner's sample sequence, increments only on new successful reads.
 * @param acquisition_boot_id  Pi boot identity at acquisition.
 * @param source_boot_id  Reporting node boot identity.
 * @param sample_age_ms [ms] Sample age at emission, saturated at 65535.
 * @param valid_fields  Measured field bits: accel 1, gyro 2, magnetometer 4. Temperature is not included.
 * @param xacc [mG] X acceleration, milligravity.
 * @param yacc [mG] Y acceleration.
 * @param zacc [mG] Z acceleration.
 * @param xgyro [mrad/s] X angular rate.
 * @param ygyro [mrad/s] Y angular rate.
 * @param zgyro [mrad/s] Z angular rate.
 * @param xmag [mgauss] X magnetic field.
 * @param ymag [mgauss] Y magnetic field.
 * @param zmag [mgauss] Z magnetic field.
 * @param status  Result; non-OK samples have valid_fields zero.
 * @param path  Response path.
 * @param acquisition_component  Pi component 1.
 * @param target_system  Recipient system.
 * @param target_component  Recipient component.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_imu_sample_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t ground_session,uint32_t request_id,uint32_t time_boot_ms,uint32_t sample_seq,uint32_t acquisition_boot_id,uint32_t source_boot_id,uint16_t sample_age_ms,uint16_t valid_fields,int16_t xacc,int16_t yacc,int16_t zacc,int16_t xgyro,int16_t ygyro,int16_t zgyro,int16_t xmag,int16_t ymag,int16_t zmag,uint8_t status,uint8_t path,uint8_t acquisition_component,uint8_t target_system,uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_IMU_SAMPLE_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint32_t(buf, 8, time_boot_ms);
    _mav_put_uint32_t(buf, 12, sample_seq);
    _mav_put_uint32_t(buf, 16, acquisition_boot_id);
    _mav_put_uint32_t(buf, 20, source_boot_id);
    _mav_put_uint16_t(buf, 24, sample_age_ms);
    _mav_put_uint16_t(buf, 26, valid_fields);
    _mav_put_int16_t(buf, 28, xacc);
    _mav_put_int16_t(buf, 30, yacc);
    _mav_put_int16_t(buf, 32, zacc);
    _mav_put_int16_t(buf, 34, xgyro);
    _mav_put_int16_t(buf, 36, ygyro);
    _mav_put_int16_t(buf, 38, zgyro);
    _mav_put_int16_t(buf, 40, xmag);
    _mav_put_int16_t(buf, 42, ymag);
    _mav_put_int16_t(buf, 44, zmag);
    _mav_put_uint8_t(buf, 46, status);
    _mav_put_uint8_t(buf, 47, path);
    _mav_put_uint8_t(buf, 48, acquisition_component);
    _mav_put_uint8_t(buf, 49, target_system);
    _mav_put_uint8_t(buf, 50, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_IMU_SAMPLE_LEN);
#else
    mavlink_imu_sample_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.time_boot_ms = time_boot_ms;
    packet.sample_seq = sample_seq;
    packet.acquisition_boot_id = acquisition_boot_id;
    packet.source_boot_id = source_boot_id;
    packet.sample_age_ms = sample_age_ms;
    packet.valid_fields = valid_fields;
    packet.xacc = xacc;
    packet.yacc = yacc;
    packet.zacc = zacc;
    packet.xgyro = xgyro;
    packet.ygyro = ygyro;
    packet.zgyro = zgyro;
    packet.xmag = xmag;
    packet.ymag = ymag;
    packet.zmag = zmag;
    packet.status = status;
    packet.path = path;
    packet.acquisition_component = acquisition_component;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_IMU_SAMPLE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_IMU_SAMPLE;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_CRC);
}

/**
 * @brief Encode a imu_sample struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param imu_sample C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_imu_sample_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_imu_sample_t* imu_sample)
{
    return mavlink_msg_imu_sample_pack(system_id, component_id, msg, imu_sample->ground_session, imu_sample->request_id, imu_sample->time_boot_ms, imu_sample->sample_seq, imu_sample->acquisition_boot_id, imu_sample->source_boot_id, imu_sample->sample_age_ms, imu_sample->valid_fields, imu_sample->xacc, imu_sample->yacc, imu_sample->zacc, imu_sample->xgyro, imu_sample->ygyro, imu_sample->zgyro, imu_sample->xmag, imu_sample->ymag, imu_sample->zmag, imu_sample->status, imu_sample->path, imu_sample->acquisition_component, imu_sample->target_system, imu_sample->target_component);
}

/**
 * @brief Encode a imu_sample struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param imu_sample C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_imu_sample_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_imu_sample_t* imu_sample)
{
    return mavlink_msg_imu_sample_pack_chan(system_id, component_id, chan, msg, imu_sample->ground_session, imu_sample->request_id, imu_sample->time_boot_ms, imu_sample->sample_seq, imu_sample->acquisition_boot_id, imu_sample->source_boot_id, imu_sample->sample_age_ms, imu_sample->valid_fields, imu_sample->xacc, imu_sample->yacc, imu_sample->zacc, imu_sample->xgyro, imu_sample->ygyro, imu_sample->zgyro, imu_sample->xmag, imu_sample->ymag, imu_sample->zmag, imu_sample->status, imu_sample->path, imu_sample->acquisition_component, imu_sample->target_system, imu_sample->target_component);
}

/**
 * @brief Encode a imu_sample struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param imu_sample C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_imu_sample_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_imu_sample_t* imu_sample)
{
    return mavlink_msg_imu_sample_pack_status(system_id, component_id, _status, msg,  imu_sample->ground_session, imu_sample->request_id, imu_sample->time_boot_ms, imu_sample->sample_seq, imu_sample->acquisition_boot_id, imu_sample->source_boot_id, imu_sample->sample_age_ms, imu_sample->valid_fields, imu_sample->xacc, imu_sample->yacc, imu_sample->zacc, imu_sample->xgyro, imu_sample->ygyro, imu_sample->zgyro, imu_sample->xmag, imu_sample->ymag, imu_sample->zmag, imu_sample->status, imu_sample->path, imu_sample->acquisition_component, imu_sample->target_system, imu_sample->target_component);
}

/**
 * @brief Send a imu_sample message
 * @param chan MAVLink channel to send the message
 *
 * @param ground_session  Request session.
 * @param request_id  Request identifier.
 * @param time_boot_ms [ms] Acquisition owner's monotonic boot-relative acquisition timestamp.
 * @param sample_seq  Acquisition owner's sample sequence, increments only on new successful reads.
 * @param acquisition_boot_id  Pi boot identity at acquisition.
 * @param source_boot_id  Reporting node boot identity.
 * @param sample_age_ms [ms] Sample age at emission, saturated at 65535.
 * @param valid_fields  Measured field bits: accel 1, gyro 2, magnetometer 4. Temperature is not included.
 * @param xacc [mG] X acceleration, milligravity.
 * @param yacc [mG] Y acceleration.
 * @param zacc [mG] Z acceleration.
 * @param xgyro [mrad/s] X angular rate.
 * @param ygyro [mrad/s] Y angular rate.
 * @param zgyro [mrad/s] Z angular rate.
 * @param xmag [mgauss] X magnetic field.
 * @param ymag [mgauss] Y magnetic field.
 * @param zmag [mgauss] Z magnetic field.
 * @param status  Result; non-OK samples have valid_fields zero.
 * @param path  Response path.
 * @param acquisition_component  Pi component 1.
 * @param target_system  Recipient system.
 * @param target_component  Recipient component.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_imu_sample_send(mavlink_channel_t chan, uint32_t ground_session, uint32_t request_id, uint32_t time_boot_ms, uint32_t sample_seq, uint32_t acquisition_boot_id, uint32_t source_boot_id, uint16_t sample_age_ms, uint16_t valid_fields, int16_t xacc, int16_t yacc, int16_t zacc, int16_t xgyro, int16_t ygyro, int16_t zgyro, int16_t xmag, int16_t ymag, int16_t zmag, uint8_t status, uint8_t path, uint8_t acquisition_component, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_IMU_SAMPLE_LEN];
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint32_t(buf, 8, time_boot_ms);
    _mav_put_uint32_t(buf, 12, sample_seq);
    _mav_put_uint32_t(buf, 16, acquisition_boot_id);
    _mav_put_uint32_t(buf, 20, source_boot_id);
    _mav_put_uint16_t(buf, 24, sample_age_ms);
    _mav_put_uint16_t(buf, 26, valid_fields);
    _mav_put_int16_t(buf, 28, xacc);
    _mav_put_int16_t(buf, 30, yacc);
    _mav_put_int16_t(buf, 32, zacc);
    _mav_put_int16_t(buf, 34, xgyro);
    _mav_put_int16_t(buf, 36, ygyro);
    _mav_put_int16_t(buf, 38, zgyro);
    _mav_put_int16_t(buf, 40, xmag);
    _mav_put_int16_t(buf, 42, ymag);
    _mav_put_int16_t(buf, 44, zmag);
    _mav_put_uint8_t(buf, 46, status);
    _mav_put_uint8_t(buf, 47, path);
    _mav_put_uint8_t(buf, 48, acquisition_component);
    _mav_put_uint8_t(buf, 49, target_system);
    _mav_put_uint8_t(buf, 50, target_component);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_IMU_SAMPLE, buf, MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_CRC);
#else
    mavlink_imu_sample_t packet;
    packet.ground_session = ground_session;
    packet.request_id = request_id;
    packet.time_boot_ms = time_boot_ms;
    packet.sample_seq = sample_seq;
    packet.acquisition_boot_id = acquisition_boot_id;
    packet.source_boot_id = source_boot_id;
    packet.sample_age_ms = sample_age_ms;
    packet.valid_fields = valid_fields;
    packet.xacc = xacc;
    packet.yacc = yacc;
    packet.zacc = zacc;
    packet.xgyro = xgyro;
    packet.ygyro = ygyro;
    packet.zgyro = zgyro;
    packet.xmag = xmag;
    packet.ymag = ymag;
    packet.zmag = zmag;
    packet.status = status;
    packet.path = path;
    packet.acquisition_component = acquisition_component;
    packet.target_system = target_system;
    packet.target_component = target_component;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_IMU_SAMPLE, (const char *)&packet, MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_CRC);
#endif
}

/**
 * @brief Send a imu_sample message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_imu_sample_send_struct(mavlink_channel_t chan, const mavlink_imu_sample_t* imu_sample)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_imu_sample_send(chan, imu_sample->ground_session, imu_sample->request_id, imu_sample->time_boot_ms, imu_sample->sample_seq, imu_sample->acquisition_boot_id, imu_sample->source_boot_id, imu_sample->sample_age_ms, imu_sample->valid_fields, imu_sample->xacc, imu_sample->yacc, imu_sample->zacc, imu_sample->xgyro, imu_sample->ygyro, imu_sample->zgyro, imu_sample->xmag, imu_sample->ymag, imu_sample->zmag, imu_sample->status, imu_sample->path, imu_sample->acquisition_component, imu_sample->target_system, imu_sample->target_component);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_IMU_SAMPLE, (const char *)imu_sample, MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_CRC);
#endif
}

#if MAVLINK_MSG_ID_IMU_SAMPLE_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_imu_sample_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t ground_session, uint32_t request_id, uint32_t time_boot_ms, uint32_t sample_seq, uint32_t acquisition_boot_id, uint32_t source_boot_id, uint16_t sample_age_ms, uint16_t valid_fields, int16_t xacc, int16_t yacc, int16_t zacc, int16_t xgyro, int16_t ygyro, int16_t zgyro, int16_t xmag, int16_t ymag, int16_t zmag, uint8_t status, uint8_t path, uint8_t acquisition_component, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, ground_session);
    _mav_put_uint32_t(buf, 4, request_id);
    _mav_put_uint32_t(buf, 8, time_boot_ms);
    _mav_put_uint32_t(buf, 12, sample_seq);
    _mav_put_uint32_t(buf, 16, acquisition_boot_id);
    _mav_put_uint32_t(buf, 20, source_boot_id);
    _mav_put_uint16_t(buf, 24, sample_age_ms);
    _mav_put_uint16_t(buf, 26, valid_fields);
    _mav_put_int16_t(buf, 28, xacc);
    _mav_put_int16_t(buf, 30, yacc);
    _mav_put_int16_t(buf, 32, zacc);
    _mav_put_int16_t(buf, 34, xgyro);
    _mav_put_int16_t(buf, 36, ygyro);
    _mav_put_int16_t(buf, 38, zgyro);
    _mav_put_int16_t(buf, 40, xmag);
    _mav_put_int16_t(buf, 42, ymag);
    _mav_put_int16_t(buf, 44, zmag);
    _mav_put_uint8_t(buf, 46, status);
    _mav_put_uint8_t(buf, 47, path);
    _mav_put_uint8_t(buf, 48, acquisition_component);
    _mav_put_uint8_t(buf, 49, target_system);
    _mav_put_uint8_t(buf, 50, target_component);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_IMU_SAMPLE, buf, MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_CRC);
#else
    mavlink_imu_sample_t *packet = (mavlink_imu_sample_t *)msgbuf;
    packet->ground_session = ground_session;
    packet->request_id = request_id;
    packet->time_boot_ms = time_boot_ms;
    packet->sample_seq = sample_seq;
    packet->acquisition_boot_id = acquisition_boot_id;
    packet->source_boot_id = source_boot_id;
    packet->sample_age_ms = sample_age_ms;
    packet->valid_fields = valid_fields;
    packet->xacc = xacc;
    packet->yacc = yacc;
    packet->zacc = zacc;
    packet->xgyro = xgyro;
    packet->ygyro = ygyro;
    packet->zgyro = zgyro;
    packet->xmag = xmag;
    packet->ymag = ymag;
    packet->zmag = zmag;
    packet->status = status;
    packet->path = path;
    packet->acquisition_component = acquisition_component;
    packet->target_system = target_system;
    packet->target_component = target_component;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_IMU_SAMPLE, (const char *)packet, MAVLINK_MSG_ID_IMU_SAMPLE_MIN_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_LEN, MAVLINK_MSG_ID_IMU_SAMPLE_CRC);
#endif
}
#endif

#endif

// MESSAGE IMU_SAMPLE UNPACKING


/**
 * @brief Get field ground_session from imu_sample message
 *
 * @return  Request session.
 */
static inline uint32_t mavlink_msg_imu_sample_get_ground_session(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field request_id from imu_sample message
 *
 * @return  Request identifier.
 */
static inline uint32_t mavlink_msg_imu_sample_get_request_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  4);
}

/**
 * @brief Get field time_boot_ms from imu_sample message
 *
 * @return [ms] Acquisition owner's monotonic boot-relative acquisition timestamp.
 */
static inline uint32_t mavlink_msg_imu_sample_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  8);
}

/**
 * @brief Get field sample_seq from imu_sample message
 *
 * @return  Acquisition owner's sample sequence, increments only on new successful reads.
 */
static inline uint32_t mavlink_msg_imu_sample_get_sample_seq(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  12);
}

/**
 * @brief Get field acquisition_boot_id from imu_sample message
 *
 * @return  Pi boot identity at acquisition.
 */
static inline uint32_t mavlink_msg_imu_sample_get_acquisition_boot_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  16);
}

/**
 * @brief Get field source_boot_id from imu_sample message
 *
 * @return  Reporting node boot identity.
 */
static inline uint32_t mavlink_msg_imu_sample_get_source_boot_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  20);
}

/**
 * @brief Get field sample_age_ms from imu_sample message
 *
 * @return [ms] Sample age at emission, saturated at 65535.
 */
static inline uint16_t mavlink_msg_imu_sample_get_sample_age_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  24);
}

/**
 * @brief Get field valid_fields from imu_sample message
 *
 * @return  Measured field bits: accel 1, gyro 2, magnetometer 4. Temperature is not included.
 */
static inline uint16_t mavlink_msg_imu_sample_get_valid_fields(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  26);
}

/**
 * @brief Get field xacc from imu_sample message
 *
 * @return [mG] X acceleration, milligravity.
 */
static inline int16_t mavlink_msg_imu_sample_get_xacc(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  28);
}

/**
 * @brief Get field yacc from imu_sample message
 *
 * @return [mG] Y acceleration.
 */
static inline int16_t mavlink_msg_imu_sample_get_yacc(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  30);
}

/**
 * @brief Get field zacc from imu_sample message
 *
 * @return [mG] Z acceleration.
 */
static inline int16_t mavlink_msg_imu_sample_get_zacc(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  32);
}

/**
 * @brief Get field xgyro from imu_sample message
 *
 * @return [mrad/s] X angular rate.
 */
static inline int16_t mavlink_msg_imu_sample_get_xgyro(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  34);
}

/**
 * @brief Get field ygyro from imu_sample message
 *
 * @return [mrad/s] Y angular rate.
 */
static inline int16_t mavlink_msg_imu_sample_get_ygyro(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  36);
}

/**
 * @brief Get field zgyro from imu_sample message
 *
 * @return [mrad/s] Z angular rate.
 */
static inline int16_t mavlink_msg_imu_sample_get_zgyro(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  38);
}

/**
 * @brief Get field xmag from imu_sample message
 *
 * @return [mgauss] X magnetic field.
 */
static inline int16_t mavlink_msg_imu_sample_get_xmag(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  40);
}

/**
 * @brief Get field ymag from imu_sample message
 *
 * @return [mgauss] Y magnetic field.
 */
static inline int16_t mavlink_msg_imu_sample_get_ymag(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  42);
}

/**
 * @brief Get field zmag from imu_sample message
 *
 * @return [mgauss] Z magnetic field.
 */
static inline int16_t mavlink_msg_imu_sample_get_zmag(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  44);
}

/**
 * @brief Get field status from imu_sample message
 *
 * @return  Result; non-OK samples have valid_fields zero.
 */
static inline uint8_t mavlink_msg_imu_sample_get_status(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  46);
}

/**
 * @brief Get field path from imu_sample message
 *
 * @return  Response path.
 */
static inline uint8_t mavlink_msg_imu_sample_get_path(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  47);
}

/**
 * @brief Get field acquisition_component from imu_sample message
 *
 * @return  Pi component 1.
 */
static inline uint8_t mavlink_msg_imu_sample_get_acquisition_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  48);
}

/**
 * @brief Get field target_system from imu_sample message
 *
 * @return  Recipient system.
 */
static inline uint8_t mavlink_msg_imu_sample_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  49);
}

/**
 * @brief Get field target_component from imu_sample message
 *
 * @return  Recipient component.
 */
static inline uint8_t mavlink_msg_imu_sample_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  50);
}

/**
 * @brief Decode a imu_sample message into a struct
 *
 * @param msg The message to decode
 * @param imu_sample C-struct to decode the message contents into
 */
static inline void mavlink_msg_imu_sample_decode(const mavlink_message_t* msg, mavlink_imu_sample_t* imu_sample)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    imu_sample->ground_session = mavlink_msg_imu_sample_get_ground_session(msg);
    imu_sample->request_id = mavlink_msg_imu_sample_get_request_id(msg);
    imu_sample->time_boot_ms = mavlink_msg_imu_sample_get_time_boot_ms(msg);
    imu_sample->sample_seq = mavlink_msg_imu_sample_get_sample_seq(msg);
    imu_sample->acquisition_boot_id = mavlink_msg_imu_sample_get_acquisition_boot_id(msg);
    imu_sample->source_boot_id = mavlink_msg_imu_sample_get_source_boot_id(msg);
    imu_sample->sample_age_ms = mavlink_msg_imu_sample_get_sample_age_ms(msg);
    imu_sample->valid_fields = mavlink_msg_imu_sample_get_valid_fields(msg);
    imu_sample->xacc = mavlink_msg_imu_sample_get_xacc(msg);
    imu_sample->yacc = mavlink_msg_imu_sample_get_yacc(msg);
    imu_sample->zacc = mavlink_msg_imu_sample_get_zacc(msg);
    imu_sample->xgyro = mavlink_msg_imu_sample_get_xgyro(msg);
    imu_sample->ygyro = mavlink_msg_imu_sample_get_ygyro(msg);
    imu_sample->zgyro = mavlink_msg_imu_sample_get_zgyro(msg);
    imu_sample->xmag = mavlink_msg_imu_sample_get_xmag(msg);
    imu_sample->ymag = mavlink_msg_imu_sample_get_ymag(msg);
    imu_sample->zmag = mavlink_msg_imu_sample_get_zmag(msg);
    imu_sample->status = mavlink_msg_imu_sample_get_status(msg);
    imu_sample->path = mavlink_msg_imu_sample_get_path(msg);
    imu_sample->acquisition_component = mavlink_msg_imu_sample_get_acquisition_component(msg);
    imu_sample->target_system = mavlink_msg_imu_sample_get_target_system(msg);
    imu_sample->target_component = mavlink_msg_imu_sample_get_target_component(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_IMU_SAMPLE_LEN? msg->len : MAVLINK_MSG_ID_IMU_SAMPLE_LEN;
        memset(imu_sample, 0, MAVLINK_MSG_ID_IMU_SAMPLE_LEN);
    memcpy(imu_sample, _MAV_PAYLOAD(msg), len);
#endif
}
