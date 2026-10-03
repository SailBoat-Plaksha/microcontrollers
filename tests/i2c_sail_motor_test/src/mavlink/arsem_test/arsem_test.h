/** @file
 *  @brief MAVLink comm protocol generated from arsem_test.xml
 *  @see http://mavlink.org
 */
#pragma once
#ifndef MAVLINK_ARSEM_TEST_H
#define MAVLINK_ARSEM_TEST_H

#ifndef MAVLINK_H
    #error Wrong include order: MAVLINK_ARSEM_TEST.H MUST NOT BE DIRECTLY USED. Include mavlink.h from the same directory instead or set ALL AND EVERY defines from MAVLINK.H manually accordingly, including the #define MAVLINK_H call.
#endif

#define MAVLINK_ARSEM_TEST_XML_HASH 6028762105269038243

#ifdef __cplusplus
extern "C" {
#endif

// MESSAGE LENGTHS AND CRCS

#ifndef MAVLINK_MESSAGE_LENGTHS
#define MAVLINK_MESSAGE_LENGTHS {}
#endif

#ifndef MAVLINK_MESSAGE_CRCS
#define MAVLINK_MESSAGE_CRCS {{200090, 233, 10, 10, 3, 6, 7}, {200091, 123, 16, 16, 0, 0, 0}}
#endif

#include "../protocol.h"

#define MAVLINK_ENABLED_ARSEM_TEST

// ENUM DEFINITIONS



// MAVLINK VERSION

#ifndef MAVLINK_VERSION
#define MAVLINK_VERSION 3
#endif

#if (MAVLINK_VERSION == 0)
#undef MAVLINK_VERSION
#define MAVLINK_VERSION 3
#endif

// MESSAGE DEFINITIONS
#include "./mavlink_msg_sail_motor_test.h"
#include "./mavlink_msg_sail_motor_test_status.h"

// base include



#if MAVLINK_ARSEM_TEST_XML_HASH == MAVLINK_PRIMARY_XML_HASH
# define MAVLINK_MESSAGE_INFO {MAVLINK_MESSAGE_INFO_SAIL_MOTOR_TEST, MAVLINK_MESSAGE_INFO_SAIL_MOTOR_TEST_STATUS}
# define MAVLINK_MESSAGE_NAMES {{ "SAIL_MOTOR_TEST", 200090 }, { "SAIL_MOTOR_TEST_STATUS", 200091 }}
# if MAVLINK_COMMAND_24BIT
#  include "../mavlink_get_info.h"
# endif
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // MAVLINK_ARSEM_TEST_H
