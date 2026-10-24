/** @file
 *  @brief MAVLink comm protocol generated from arsem_standalone.xml
 *  @see http://mavlink.org
 */
#pragma once
#ifndef MAVLINK_ARSEM_STANDALONE_H
#define MAVLINK_ARSEM_STANDALONE_H

#ifndef MAVLINK_H
    #error Wrong include order: MAVLINK_ARSEM_STANDALONE.H MUST NOT BE DIRECTLY USED. Include mavlink.h from the same directory instead or set ALL AND EVERY defines from MAVLINK.H manually accordingly, including the #define MAVLINK_H call.
#endif

#define MAVLINK_ARSEM_STANDALONE_XML_HASH 2675764913854604559

#ifdef __cplusplus
extern "C" {
#endif

// MESSAGE LENGTHS AND CRCS

#ifndef MAVLINK_MESSAGE_LENGTHS
#define MAVLINK_MESSAGE_LENGTHS {}
#endif

#ifndef MAVLINK_MESSAGE_CRCS
#define MAVLINK_MESSAGE_CRCS {{0, 50, 9, 9, 0, 0, 0}, {4, 237, 14, 14, 3, 12, 13}, {26, 170, 22, 24, 0, 0, 0}, {200000, 139, 15, 15, 0, 0, 0}, {200001, 49, 14, 14, 3, 10, 11}, {200002, 131, 20, 20, 0, 0, 0}, {200003, 143, 10, 10, 0, 0, 0}, {200004, 158, 21, 21, 0, 0, 0}, {200005, 201, 14, 14, 0, 0, 0}, {200006, 198, 24, 24, 0, 0, 0}, {200007, 121, 11, 11, 3, 8, 9}, {200008, 188, 19, 19, 3, 14, 15}, {200009, 90, 16, 16, 0, 0, 0}, {200010, 56, 20, 20, 3, 18, 19}, {200011, 71, 15, 15, 3, 13, 14}, {200012, 32, 51, 51, 3, 49, 50}}
#endif

#include "../protocol.h"

#define MAVLINK_ENABLED_ARSEM_STANDALONE

// ENUM DEFINITIONS


/** @brief Logical role of a component in the boat system. */
#ifndef HAVE_ENUM_COMPONENT_ROLE
#define HAVE_ENUM_COMPONENT_ROLE
typedef enum COMPONENT_ROLE
{
   COMPONENT_ROLE_UNKNOWN=0, /* Role is unknown or has not been configured. | */
   COMPONENT_ROLE_BRAIN=1, /* Main computer responsible for navigation and mission control. | */
   COMPONENT_ROLE_ACTUATOR=2, /* Servo and motor controller. | */
   COMPONENT_ROLE_COMMS_SENSOR=3, /* Communications and sensor controller. | */
   COMPONENT_ROLE_GROUND_STATION=4, /* Operator CLI, GUI, or ground-control station. | */
   COMPONENT_ROLE_ENUM_END=5, /*  | */
} COMPONENT_ROLE;
#endif

/** @brief Functions implemented by a component. */
#ifndef HAVE_ENUM_CAPABILITY_FLAGS
#define HAVE_ENUM_CAPABILITY_FLAGS
typedef enum CAPABILITY_FLAGS
{
   CAPABILITY_FLAGS_ACTUATOR_CONTROL=1, /* Accepts actuator setpoints. | */
   CAPABILITY_FLAGS_SENSOR_TELEMETRY=2, /* Publishes sensor telemetry. | */
   CAPABILITY_FLAGS_LORA_GATEWAY=4, /* Provides a LoRa data link. | */
   CAPABILITY_FLAGS_SATCOM_GATEWAY=8, /* Provides a satellite data link. | */
   CAPABILITY_FLAGS_POWER_MANAGEMENT=16, /* Controls or reports system power modes. | */
   CAPABILITY_FLAGS_LOCAL_LOGGING=32, /* Can store telemetry locally. | */
   CAPABILITY_FLAGS_RC_OVERRIDE=64, /* Can accept direct radio-control authority. | */
   CAPABILITY_FLAGS_BUS_MASTER=128, /* Schedules access to the shared half-duplex bus. | */
   CAPABILITY_FLAGS_BUS_CLIENT=256, /* Uses grants from the shared half-duplex bus master. | */
   CAPABILITY_FLAGS_ENUM_END=257, /*  | */
} CAPABILITY_FLAGS;
#endif

/** @brief Controllable actuator on the sailboat. */
#ifndef HAVE_ENUM_ACTUATOR_ID
#define HAVE_ENUM_ACTUATOR_ID
typedef enum ACTUATOR_ID
{
   ACTUATOR_ID_UNKNOWN=0, /* Unknown or unspecified actuator. | */
   ACTUATOR_ID_SAIL=1, /* Sail trim actuator. | */
   ACTUATOR_ID_RUDDER=2, /* Rudder actuator. | */
   ACTUATOR_ID_ENUM_END=3, /*  | */
} ACTUATOR_ID;
#endif

/** @brief Source currently permitted to command an actuator. */
#ifndef HAVE_ENUM_CONTROL_SOURCE
#define HAVE_ENUM_CONTROL_SOURCE
typedef enum CONTROL_SOURCE
{
   CONTROL_SOURCE_NONE=0, /* No source has control. | */
   CONTROL_SOURCE_RC=1, /* Direct local radio control. | */
   CONTROL_SOURCE_AUTONOMOUS=2, /* Autonomous control from the main computer. | */
   CONTROL_SOURCE_REMOTE_OPERATOR=3, /* Remote operator command relayed through the main computer. | */
   CONTROL_SOURCE_FAILSAFE=4, /* Locally generated failsafe command. | */
   CONTROL_SOURCE_ENUM_END=5, /*  | */
} CONTROL_SOURCE;
#endif

/** @brief Result of applying the latest actuator command. */
#ifndef HAVE_ENUM_COMMAND_RESULT
#define HAVE_ENUM_COMMAND_RESULT
typedef enum COMMAND_RESULT
{
   COMMAND_RESULT_ACCEPTED=0, /* Command was accepted and applied. | */
   COMMAND_RESULT_IN_PROGRESS=1, /* Command was accepted and the actuator is still moving. | */
   COMMAND_RESULT_DENIED=2, /* Command was denied by the current control-authority policy. | */
   COMMAND_RESULT_INVALID=3, /* Command contained an unsupported actuator, range, or value. | */
   COMMAND_RESULT_TIMEOUT=4, /* Command expired before or while it was being applied. | */
   COMMAND_RESULT_FAULT=5, /* Command could not be applied because of a hardware fault. | */
   COMMAND_RESULT_ENUM_END=6, /*  | */
} COMMAND_RESULT;
#endif

/** @brief Faults common to the boat computers, links, sensors, and actuators. */
#ifndef HAVE_ENUM_FAULT_FLAGS
#define HAVE_ENUM_FAULT_FLAGS
typedef enum FAULT_FLAGS
{
   FAULT_FLAGS_COMMUNICATION=1, /* Communication timeout or malformed traffic. | */
   FAULT_FLAGS_SENSOR=2, /* Sensor missing, stale, or outside its valid range. | */
   FAULT_FLAGS_ACTUATOR=4, /* Actuator feedback or motion fault. | */
   FAULT_FLAGS_UNDERVOLTAGE=8, /* Supply voltage is below its configured safe threshold. | */
   FAULT_FLAGS_OVERCURRENT=16, /* Current is above its configured safe threshold. | */
   FAULT_FLAGS_OVERTEMPERATURE=32, /* Temperature is above its configured safe threshold. | */
   FAULT_FLAGS_WATCHDOG=64, /* A watchdog reset or missed deadline occurred. | */
   FAULT_FLAGS_LEAK=128, /* Water ingress or leak was detected. | */
   FAULT_FLAGS_ENUM_END=129, /*  | */
} FAULT_FLAGS;
#endif

/** @brief Whole-boat power operating mode. */
#ifndef HAVE_ENUM_POWER_MODE_TYPE
#define HAVE_ENUM_POWER_MODE_TYPE
typedef enum POWER_MODE_TYPE
{
   POWER_MODE_TYPE_FULL_ACTIVE=0, /* All configured systems are powered and active. | */
   POWER_MODE_TYPE_MISSION=1, /* Normal autonomous mission power profile. | */
   POWER_MODE_TYPE_LOW_POWER=2, /* Nonessential loads and telemetry rates are reduced. | */
   POWER_MODE_TYPE_SAFE=3, /* Only systems required to remain safe and recover are active. | */
   POWER_MODE_TYPE_SHUTDOWN=4, /* Orderly shutdown is in progress. | */
   POWER_MODE_TYPE_ENUM_END=5, /*  | */
} POWER_MODE_TYPE;
#endif

/** @brief Fields that contain a valid measurement in ENVIRONMENTAL_SAMPLE. */
#ifndef HAVE_ENUM_ENVIRONMENTAL_VALIDITY
#define HAVE_ENUM_ENVIRONMENTAL_VALIDITY
typedef enum ENVIRONMENTAL_VALIDITY
{
   ENVIRONMENTAL_VALIDITY_PH=1, /* The pH field is valid. | */
   ENVIRONMENTAL_VALIDITY_WATER_TEMPERATURE=2, /* The water temperature field is valid. | */
   ENVIRONMENTAL_VALIDITY_DISSOLVED_OXYGEN=4, /* The dissolved oxygen field is valid. | */
   ENVIRONMENTAL_VALIDITY_SALINITY=8, /* The salinity field is valid. | */
   ENVIRONMENTAL_VALIDITY_TURBIDITY=16, /* The turbidity field is valid. | */
   ENVIRONMENTAL_VALIDITY_ENUM_END=17, /*  | */
} ENVIRONMENTAL_VALIDITY;
#endif

/** @brief Physical link represented by LINK_STATUS. */
#ifndef HAVE_ENUM_LINK_TYPE
#define HAVE_ENUM_LINK_TYPE
typedef enum LINK_TYPE
{
   LINK_TYPE_UNKNOWN=0, /* Unknown or unspecified link. | */
   LINK_TYPE_RS485=1, /* Onboard RS-485 bus. | */
   LINK_TYPE_LORA=2, /* Long-range LoRa radio link. | */
   LINK_TYPE_SATCOM=3, /* Satellite communications link. | */
   LINK_TYPE_USB=4, /* USB or local development serial link. | */
   LINK_TYPE_I2C=5, /* Onboard shared I2C bus between the single-controller computer and its addressed peers. | */
   LINK_TYPE_ENUM_END=6, /*  | */
} LINK_TYPE;
#endif

/** @brief Operation requested by one ACTUATOR_REQUEST movement envelope. */
#ifndef HAVE_ENUM_MOVEMENT_OPCODE
#define HAVE_ENUM_MOVEMENT_OPCODE
typedef enum MOVEMENT_OPCODE
{
   MOVEMENT_OPCODE_NONE=0, /* No operation. The request is invalid and must be rejected without moving the actuator. | */
   MOVEMENT_OPCODE_ABSOLUTE=1, /* Move to the absolute angle carried in delta_cdeg. | */
   MOVEMENT_OPCODE_STEP=2, /* Resolve the signed relative delta in delta_cdeg exactly once against valid fresh actuator feedback, then move to the resolved absolute target. | */
   MOVEMENT_OPCODE_STOP=3, /* Cancel the active or queued movement and hold. Distinct from a zero-degree absolute target. delta_cdeg must be zero. | */
   MOVEMENT_OPCODE_ENUM_END=4, /*  | */
} MOVEMENT_OPCODE;
#endif

/** @brief Progress state of one movement request at the reporting node. Reported states are distinct observations and must not be collapsed. */
#ifndef HAVE_ENUM_MOTOR_STATE
#define HAVE_ENUM_MOTOR_STATE
typedef enum MOTOR_STATE
{
   MOTOR_STATE_UNKNOWN=0, /* The state is not known, for example after a reboot or a lost response. | */
   MOTOR_STATE_RECEIVED=1, /* The request was received and is awaiting validation. | */
   MOTOR_STATE_ACCEPTED=2, /* The request passed validation and is queued, but motion has not started. | */
   MOTOR_STATE_RUNNING=3, /* The actuator is moving under this request. | */
   MOTOR_STATE_COMPLETED=4, /* The requested movement reached its target. | */
   MOTOR_STATE_STOPPED=5, /* The movement was stopped before completion, by STOP, timeout, or preemption. | */
   MOTOR_STATE_REJECTED=6, /* The request was rejected. The rejection is identified independently of the last accepted action. | */
   MOTOR_STATE_ENUM_END=7, /*  | */
} MOTOR_STATE;
#endif

/** @brief Correlated outcome of one ground-issued movement request. These values do not modify COMMAND_RESULT, which keeps its existing meaning in ACTUATOR_STATUS. */
#ifndef HAVE_ENUM_CORRELATION_RESULT
#define HAVE_ENUM_CORRELATION_RESULT
typedef enum CORRELATION_RESULT
{
   CORRELATION_RESULT_UNKNOWN=0, /* The outcome is not known. The next step is status inspection, never a repeat movement. | */
   CORRELATION_RESULT_COMPLETED=1, /* The request completed its requested movement. | */
   CORRELATION_RESULT_DENIED=2, /* The request was denied by authority policy, for example while RC owns the actuator. | */
   CORRELATION_RESULT_INVALID=3, /* The request was invalid, for example a bad actuator, range, target, or a STOP with a nonzero delta. | */
   CORRELATION_RESULT_STALE=4, /* The request expired or arrived after a session or boot change. | */
   CORRELATION_RESULT_SUPERSEDED=5, /* A newer request replaced this one before it completed. | */
   CORRELATION_RESULT_ENUM_END=6, /*  | */
} CORRELATION_RESULT;
#endif

/** @brief Why the reporting node last restarted. Report UNKNOWN rather than guessing. */
#ifndef HAVE_ENUM_RESET_REASON
#define HAVE_ENUM_RESET_REASON
typedef enum RESET_REASON
{
   RESET_REASON_UNKNOWN=0, /* The reset reason is not known or not measured. | */
   RESET_REASON_POWER_ON=1, /* Power was applied or the node was manually reset. | */
   RESET_REASON_WATCHDOG=2, /* A watchdog or missed-deadline reset occurred. | */
   RESET_REASON_SOFTWARE=3, /* The node restarted itself deliberately. | */
   RESET_REASON_BROWNOUT=4, /* Supply voltage fell below the reset threshold. | */
   RESET_REASON_ENUM_END=5, /*  | */
} RESET_REASON;
#endif

/** @brief Measurements currently valid at the reporting node. Used to mark unmeasured fields, such as SCALED_IMU magnetometer and temperature, invalid instead of reporting zero as a real measurement. */
#ifndef HAVE_ENUM_SENSOR_VALID_FIELDS
#define HAVE_ENUM_SENSOR_VALID_FIELDS
typedef enum SENSOR_VALID_FIELDS
{
   SENSOR_VALID_FIELDS_ACCELEROMETER=1, /* Accelerometer samples are valid. | */
   SENSOR_VALID_FIELDS_GYROSCOPE=2, /* Gyroscope samples are valid. | */
   SENSOR_VALID_FIELDS_MAGNETOMETER=4, /* Magnetometer samples are valid. | */
   SENSOR_VALID_FIELDS_TEMPERATURE=8, /* Temperature samples are valid. | */
   SENSOR_VALID_FIELDS_ENUM_END=9, /*  | */
} SENSOR_VALID_FIELDS;
#endif

/** @brief Classes of queued traffic that a bus client may transmit during a grant. */
#ifndef HAVE_ENUM_BUS_STREAM_FLAGS
#define HAVE_ENUM_BUS_STREAM_FLAGS
typedef enum BUS_STREAM_FLAGS
{
   BUS_STREAM_FLAGS_CRITICAL=1, /* Critical fault, control, and acknowledgement traffic. | */
   BUS_STREAM_FLAGS_STATUS=2, /* Component, actuator, power, and link status. | */
   BUS_STREAM_FLAGS_FAST_SENSOR=4, /* High-rate navigation and control sensor samples. | */
   BUS_STREAM_FLAGS_SLOW_SENSOR=8, /* Low-rate science and environmental samples. | */
   BUS_STREAM_FLAGS_DIAGNOSTIC=16, /* Debug and diagnostic traffic. | */
   BUS_STREAM_FLAGS_ENUM_END=17, /*  | */
} BUS_STREAM_FLAGS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_IMU_PATH
#define HAVE_ENUM_IMU_PATH
typedef enum IMU_PATH
{
   IMU_PATH_DIRECT=0, /* Pi reads the IMU directly. | */
   IMU_PATH_SENSOR_PICO=1, /* Pi supplies IMU readings to the sensor Pico mailbox for validation and reporting. | */
   IMU_PATH_ENUM_END=2, /*  | */
} IMU_PATH;
#endif

/** @brief  */
#ifndef HAVE_ENUM_IMU_STATUS
#define HAVE_ENUM_IMU_STATUS
typedef enum IMU_STATUS
{
   IMU_STATUS_OK=1, /*  | */
   IMU_STATUS_NOT_READY=2, /*  | */
   IMU_STATUS_UNAVAILABLE=3, /*  | */
   IMU_STATUS_TIMEOUT=4, /*  | */
   IMU_STATUS_REJECTED=5, /*  | */
   IMU_STATUS_ENUM_END=6, /*  | */
} IMU_STATUS;
#endif

/** @brief Micro air vehicle / autopilot classes. This identifies the individual model. */
#ifndef HAVE_ENUM_MAV_AUTOPILOT
#define HAVE_ENUM_MAV_AUTOPILOT
typedef enum MAV_AUTOPILOT
{
   MAV_AUTOPILOT_GENERIC=0, /* Generic autopilot, full support for everything | */
   MAV_AUTOPILOT_RESERVED=1, /* Reserved for future use. | */
   MAV_AUTOPILOT_SLUGS=2, /* SLUGS autopilot, http://slugsuav.soe.ucsc.edu | */
   MAV_AUTOPILOT_ARDUPILOTMEGA=3, /* ArduPilot - Plane/Copter/Rover/Sub/Tracker, https://ardupilot.org | */
   MAV_AUTOPILOT_OPENPILOT=4, /* OpenPilot, http://openpilot.org | */
   MAV_AUTOPILOT_GENERIC_WAYPOINTS_ONLY=5, /* Generic autopilot only supporting simple waypoints | */
   MAV_AUTOPILOT_GENERIC_WAYPOINTS_AND_SIMPLE_NAVIGATION_ONLY=6, /* Generic autopilot supporting waypoints and other simple navigation commands | */
   MAV_AUTOPILOT_GENERIC_MISSION_FULL=7, /* Generic autopilot supporting the full mission command set | */
   MAV_AUTOPILOT_INVALID=8, /* No valid autopilot, e.g. a GCS or other MAVLink component | */
   MAV_AUTOPILOT_PPZ=9, /* PPZ UAV - http://nongnu.org/paparazzi | */
   MAV_AUTOPILOT_UDB=10, /* UAV Dev Board | */
   MAV_AUTOPILOT_FP=11, /* FlexiPilot | */
   MAV_AUTOPILOT_PX4=12, /* PX4 Autopilot - http://px4.io/ | */
   MAV_AUTOPILOT_SMACCMPILOT=13, /* SMACCMPilot - http://smaccmpilot.org | */
   MAV_AUTOPILOT_AUTOQUAD=14, /* AutoQuad -- http://autoquad.org | */
   MAV_AUTOPILOT_ARMAZILA=15, /* Armazila -- http://armazila.com | */
   MAV_AUTOPILOT_AEROB=16, /* Aerob -- http://aerob.ru | */
   MAV_AUTOPILOT_ASLUAV=17, /* ASLUAV autopilot -- http://www.asl.ethz.ch | */
   MAV_AUTOPILOT_SMARTAP=18, /* SmartAP Autopilot - http://sky-drones.com | */
   MAV_AUTOPILOT_AIRRAILS=19, /* AirRails - http://uaventure.com | */
   MAV_AUTOPILOT_REFLEX=20, /* Fusion Reflex - https://fusion.engineering | */
   MAV_AUTOPILOT_FLIX=21, /* Flix quadcopter - https://quadcopter.dev | */
   MAV_AUTOPILOT_ENUM_END=22, /*  | */
} MAV_AUTOPILOT;
#endif

/** @brief These flags encode the MAV mode, see MAV_MODE enum for useful combinations. */
#ifndef HAVE_ENUM_MAV_MODE_FLAG
#define HAVE_ENUM_MAV_MODE_FLAG
typedef enum MAV_MODE_FLAG
{
   MAV_MODE_FLAG_CUSTOM_MODE_ENABLED=1, /* 0b00000001 system-specific custom mode is enabled. When using this flag to enable a custom mode all other flags should be ignored. | */
   MAV_MODE_FLAG_TEST_ENABLED=2, /* 0b00000010 system has a test mode enabled. This flag is intended for temporary system tests and should not be used for stable implementations. | */
   MAV_MODE_FLAG_AUTO_ENABLED=4, /* 0b00000100 autonomous mode enabled, system finds its own goal positions. Guided flag can be set or not, depends on the actual implementation. | */
   MAV_MODE_FLAG_GUIDED_ENABLED=8, /* 0b00001000 guided mode enabled, system flies waypoints / mission items. | */
   MAV_MODE_FLAG_STABILIZE_ENABLED=16, /* 0b00010000 system stabilizes electronically its attitude (and optionally position). It needs however further control inputs to move around. | */
   MAV_MODE_FLAG_HIL_ENABLED=32, /* 0b00100000 hardware in the loop simulation. All motors / actuators are blocked, but internal software is full operational. | */
   MAV_MODE_FLAG_MANUAL_INPUT_ENABLED=64, /* 0b01000000 remote control input is enabled. | */
   MAV_MODE_FLAG_SAFETY_ARMED=128, /* 0b10000000 MAV safety set to armed. Motors are enabled / running / can start. Ready to fly. Additional note: this flag is to be ignored when sent in the command MAV_CMD_DO_SET_MODE and MAV_CMD_COMPONENT_ARM_DISARM shall be used instead. The flag can still be used to report the armed state. | */
   MAV_MODE_FLAG_ENUM_END=129, /*  | */
} MAV_MODE_FLAG;
#endif

/** @brief  */
#ifndef HAVE_ENUM_MAV_STATE
#define HAVE_ENUM_MAV_STATE
typedef enum MAV_STATE
{
   MAV_STATE_UNINIT=0, /* Uninitialized system, state is unknown. | */
   MAV_STATE_BOOT=1, /* System is booting up. | */
   MAV_STATE_CALIBRATING=2, /* System is calibrating and not flight-ready. | */
   MAV_STATE_STANDBY=3, /* System is grounded and on standby. It can be launched any time. | */
   MAV_STATE_ACTIVE=4, /* System is active and might be already airborne. Motors are engaged. | */
   MAV_STATE_CRITICAL=5, /* System is in a non-normal flight mode (failsafe). It can however still navigate. | */
   MAV_STATE_EMERGENCY=6, /* System is in a non-normal flight mode (failsafe). It lost control over parts or over the whole airframe. It is in mayday and going down. | */
   MAV_STATE_POWEROFF=7, /* System just initialized its power-down sequence, will shut down now. | */
   MAV_STATE_FLIGHT_TERMINATION=8, /* System is terminating itself (failsafe or commanded). | */
   MAV_STATE_ENUM_END=9, /*  | */
} MAV_STATE;
#endif

/** @brief MAVLINK component type reported in HEARTBEAT message. Flight controllers must report the type of the vehicle on which they are mounted (e.g. MAV_TYPE_OCTOROTOR). All other components must report a value appropriate for their type (e.g. a camera must use MAV_TYPE_CAMERA). */
#ifndef HAVE_ENUM_MAV_TYPE
#define HAVE_ENUM_MAV_TYPE
typedef enum MAV_TYPE
{
   MAV_TYPE_GENERIC=0, /* Generic micro air vehicle | */
   MAV_TYPE_FIXED_WING=1, /* Fixed wing aircraft. | */
   MAV_TYPE_QUADROTOR=2, /* Quadrotor | */
   MAV_TYPE_COAXIAL=3, /* Coaxial helicopter | */
   MAV_TYPE_HELICOPTER=4, /* Normal helicopter with tail rotor. | */
   MAV_TYPE_ANTENNA_TRACKER=5, /* Ground installation | */
   MAV_TYPE_GCS=6, /* Operator control unit / ground control station | */
   MAV_TYPE_AIRSHIP=7, /* Airship, controlled | */
   MAV_TYPE_FREE_BALLOON=8, /* Free balloon, uncontrolled | */
   MAV_TYPE_ROCKET=9, /* Rocket | */
   MAV_TYPE_GROUND_ROVER=10, /* Ground rover | */
   MAV_TYPE_SURFACE_BOAT=11, /* Surface vessel, boat, ship | */
   MAV_TYPE_SUBMARINE=12, /* Submarine | */
   MAV_TYPE_HEXAROTOR=13, /* Hexarotor | */
   MAV_TYPE_OCTOROTOR=14, /* Octorotor | */
   MAV_TYPE_TRICOPTER=15, /* Tricopter | */
   MAV_TYPE_FLAPPING_WING=16, /* Flapping wing | */
   MAV_TYPE_KITE=17, /* Kite | */
   MAV_TYPE_ONBOARD_CONTROLLER=18, /* Onboard companion controller | */
   MAV_TYPE_VTOL_TAILSITTER_DUOROTOR=19, /* Two-rotor Tailsitter VTOL that additionally uses control surfaces in vertical operation. Note, value previously named MAV_TYPE_VTOL_DUOROTOR. | */
   MAV_TYPE_VTOL_TAILSITTER_QUADROTOR=20, /* Quad-rotor Tailsitter VTOL using a V-shaped quad config in vertical operation. Note: value previously named MAV_TYPE_VTOL_QUADROTOR. | */
   MAV_TYPE_VTOL_TILTROTOR=21, /* Tiltrotor VTOL. Fuselage and wings stay (nominally) horizontal in all flight phases. It able to tilt (some) rotors to provide thrust in cruise flight. | */
   MAV_TYPE_VTOL_FIXEDROTOR=22, /* VTOL with separate fixed rotors for hover and cruise flight. Fuselage and wings stay (nominally) horizontal in all flight phases. | */
   MAV_TYPE_VTOL_TAILSITTER=23, /* Tailsitter VTOL. Fuselage and wings orientation changes depending on flight phase: vertical for hover, horizontal for cruise. Use more specific VTOL MAV_TYPE_VTOL_TAILSITTER_DUOROTOR or MAV_TYPE_VTOL_TAILSITTER_QUADROTOR if appropriate. | */
   MAV_TYPE_VTOL_TILTWING=24, /* Tiltwing VTOL. Fuselage stays horizontal in all flight phases. The whole wing, along with any attached engine, can tilt between vertical and horizontal mode. | */
   MAV_TYPE_VTOL_RESERVED5=25, /* VTOL reserved 5 | */
   MAV_TYPE_GIMBAL=26, /* Gimbal | */
   MAV_TYPE_ADSB=27, /* ADSB system | */
   MAV_TYPE_PARAFOIL=28, /* Steerable, nonrigid airfoil | */
   MAV_TYPE_DODECAROTOR=29, /* Dodecarotor | */
   MAV_TYPE_CAMERA=30, /* Camera | */
   MAV_TYPE_CHARGING_STATION=31, /* Charging station | */
   MAV_TYPE_FLARM=32, /* FLARM collision avoidance system | */
   MAV_TYPE_SERVO=33, /* Servo | */
   MAV_TYPE_ODID=34, /* Open Drone ID. See https://mavlink.io/en/services/opendroneid.html. | */
   MAV_TYPE_DECAROTOR=35, /* Decarotor | */
   MAV_TYPE_BATTERY=36, /* Battery | */
   MAV_TYPE_PARACHUTE=37, /* Parachute | */
   MAV_TYPE_LOG=38, /* Log | */
   MAV_TYPE_OSD=39, /* OSD | */
   MAV_TYPE_IMU=40, /* IMU | */
   MAV_TYPE_GPS=41, /* GPS | */
   MAV_TYPE_WINCH=42, /* Winch | */
   MAV_TYPE_GENERIC_MULTIROTOR=43, /* Generic multirotor that does not fit into a specific type or whose type is unknown | */
   MAV_TYPE_ILLUMINATOR=44, /* Illuminator. An illuminator is a light source that is used for lighting up dark areas external to the system: e.g. a torch or searchlight (as opposed to a light source for illuminating the system itself, e.g. an indicator light). | */
   MAV_TYPE_SPACECRAFT_ORBITER=45, /* Orbiter spacecraft. Includes satellites orbiting terrestrial and extra-terrestrial bodies. Follows NASA Spacecraft Classification. | */
   MAV_TYPE_GROUND_QUADRUPED=46, /* A generic four-legged ground vehicle (e.g., a robot dog). | */
   MAV_TYPE_VTOL_GYRODYNE=47, /* VTOL hybrid of helicopter and autogyro. It has a main rotor for lift and separate propellers for forward flight. The rotor must be powered for hover but can autorotate in cruise flight. See: https://en.wikipedia.org/wiki/Gyrodyne | */
   MAV_TYPE_GRIPPER=48, /* Gripper | */
   MAV_TYPE_RADIO=49, /* Radio | */
   MAV_TYPE_ENUM_END=50, /*  | */
} MAV_TYPE;
#endif

// MAVLINK VERSION

#ifndef MAVLINK_VERSION
#define MAVLINK_VERSION 3
#endif

#if (MAVLINK_VERSION == 0)
#undef MAVLINK_VERSION
#define MAVLINK_VERSION 3
#endif

// MESSAGE DEFINITIONS
#include "./mavlink_msg_node_info.h"
#include "./mavlink_msg_actuator_setpoint.h"
#include "./mavlink_msg_actuator_status.h"
#include "./mavlink_msg_control_authority.h"
#include "./mavlink_msg_environmental_sample.h"
#include "./mavlink_msg_power_state.h"
#include "./mavlink_msg_link_status.h"
#include "./mavlink_msg_bus_grant.h"
#include "./mavlink_msg_actuator_request.h"
#include "./mavlink_msg_node_health.h"
#include "./mavlink_msg_request_result.h"
#include "./mavlink_msg_imu_request.h"
#include "./mavlink_msg_imu_sample.h"
#include "./mavlink_msg_heartbeat.h"
#include "./mavlink_msg_ping.h"
#include "./mavlink_msg_scaled_imu.h"

// base include



#if MAVLINK_ARSEM_STANDALONE_XML_HASH == MAVLINK_PRIMARY_XML_HASH
# define MAVLINK_MESSAGE_INFO {MAVLINK_MESSAGE_INFO_HEARTBEAT, MAVLINK_MESSAGE_INFO_PING, MAVLINK_MESSAGE_INFO_SCALED_IMU, MAVLINK_MESSAGE_INFO_NODE_INFO, MAVLINK_MESSAGE_INFO_ACTUATOR_SETPOINT, MAVLINK_MESSAGE_INFO_ACTUATOR_STATUS, MAVLINK_MESSAGE_INFO_CONTROL_AUTHORITY, MAVLINK_MESSAGE_INFO_ENVIRONMENTAL_SAMPLE, MAVLINK_MESSAGE_INFO_POWER_STATE, MAVLINK_MESSAGE_INFO_LINK_STATUS, MAVLINK_MESSAGE_INFO_BUS_GRANT, MAVLINK_MESSAGE_INFO_ACTUATOR_REQUEST, MAVLINK_MESSAGE_INFO_NODE_HEALTH, MAVLINK_MESSAGE_INFO_REQUEST_RESULT, MAVLINK_MESSAGE_INFO_IMU_REQUEST, MAVLINK_MESSAGE_INFO_IMU_SAMPLE}
# define MAVLINK_MESSAGE_NAMES {{ "ACTUATOR_REQUEST", 200008 }, { "ACTUATOR_SETPOINT", 200001 }, { "ACTUATOR_STATUS", 200002 }, { "BUS_GRANT", 200007 }, { "CONTROL_AUTHORITY", 200003 }, { "ENVIRONMENTAL_SAMPLE", 200004 }, { "HEARTBEAT", 0 }, { "IMU_REQUEST", 200011 }, { "IMU_SAMPLE", 200012 }, { "LINK_STATUS", 200006 }, { "NODE_HEALTH", 200009 }, { "NODE_INFO", 200000 }, { "PING", 4 }, { "POWER_STATE", 200005 }, { "REQUEST_RESULT", 200010 }, { "SCALED_IMU", 26 }}
# if MAVLINK_COMMAND_24BIT
#  include "../mavlink_get_info.h"
# endif
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // MAVLINK_ARSEM_STANDALONE_H
