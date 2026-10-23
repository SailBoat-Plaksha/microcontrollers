# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

set(PICO_PLATFORM rp2350)
set(PICO_BOARD pico2_w)  

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Servo-Chip C CXX ASM)

option(BOAT_MIDSEM_DEMO "Build the I2C/MAVLink motor-node demo without startup motion" ON)
set(MOTOR_I2C_SDA_PIN 0 CACHE STRING "I2C0 SDA GPIO for the motor Pico" )
set(MOTOR_I2C_SCL_PIN 1 CACHE STRING "I2C0 SCL GPIO for the motor Pico" )
set(MOTOR_STANDBY_PIN 9 CACHE STRING "Motor driver standby GPIO; verify physical wiring" )
set(MOTOR_M1_COUNTS_PER_REV 830 CACHE STRING "Measured sail encoder counts per output revolution" )
set(MOTOR_M2_COUNTS_PER_REV 830 CACHE STRING "Measured rudder encoder counts per output revolution" )
set(MOTOR_M1_ZERO_COUNT 0 CACHE STRING "Measured encoder count at sail zero angle" )
set(MOTOR_M2_ZERO_COUNT 0 CACHE STRING "Measured encoder count at rudder zero angle" )
set(MOTOR_M1_MAX_TRAVEL_CDEG 18000 CACHE STRING "Sail absolute travel limit in centidegrees" )
set(MOTOR_M2_MAX_TRAVEL_CDEG 18000 CACHE STRING "Rudder absolute travel limit in centidegrees" )
set(MOTOR_M1_MAX_RATE_CDEG_S 3000 CACHE STRING "Sail rate limit in centidegrees per second" )
set(MOTOR_M2_MAX_RATE_CDEG_S 3000 CACHE STRING "Rudder rate limit in centidegrees per second" )
option(MOTOR_M1_REFERENCE_VALID "Enable sail angle moves after reference calibration" OFF)
option(MOTOR_M2_REFERENCE_VALID "Enable rudder angle moves after reference calibration" OFF)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Servo-Chip
        main.c
        uart.c
        helpers.c
        servo.c
        ../protocol/mavlink_min.c
        ../protocol/actuator_command.c
        rc_control.c
        operating_loops.c
        ../motor-mcu/adapter.c
        ../motor-mcu/pico_mailbox.c
)

pico_set_program_name(Servo-Chip "Servo-Chip")
pico_set_program_version(Servo-Chip "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Servo-Chip 0)
pico_enable_stdio_usb(Servo-Chip 1)

# Add the standard library to the build
target_link_libraries(Servo-Chip
        pico_stdlib
        pico_time
        )

# Add the standard include files to the build
target_include_directories(Servo-Chip PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/../protocol
        ${CMAKE_CURRENT_LIST_DIR}/../protocol/gen/C
        ${CMAKE_CURRENT_LIST_DIR}/../protocol/gen/C/arsem_standalone
        ${CMAKE_CURRENT_LIST_DIR}/../motor-mcu
)

# Add any user requested libraries
target_link_libraries(Servo-Chip
        hardware_uart 
        hardware_pwm
        hardware_i2c
        pico_i2c_slave
        pico_rand
        #hardware_interp
        #hardware_timer
        #hardware_watchdog
        #hardware_clocks
        )

if(BOAT_MIDSEM_DEMO)
    target_compile_definitions(Servo-Chip PRIVATE
        BOAT_MIDSEM_DEMO=1
        MOTOR_I2C_SDA_PIN=${MOTOR_I2C_SDA_PIN}
        MOTOR_I2C_SCL_PIN=${MOTOR_I2C_SCL_PIN}
        MOTOR_STANDBY_PIN=${MOTOR_STANDBY_PIN}
        MOTOR_M1_COUNTS_PER_REV=${MOTOR_M1_COUNTS_PER_REV}
        MOTOR_M2_COUNTS_PER_REV=${MOTOR_M2_COUNTS_PER_REV}
        MOTOR_M1_ZERO_COUNT=${MOTOR_M1_ZERO_COUNT}
        MOTOR_M2_ZERO_COUNT=${MOTOR_M2_ZERO_COUNT}
        MOTOR_M1_MAX_TRAVEL_CDEG=${MOTOR_M1_MAX_TRAVEL_CDEG}
        MOTOR_M2_MAX_TRAVEL_CDEG=${MOTOR_M2_MAX_TRAVEL_CDEG}
        MOTOR_M1_MAX_RATE_CDEG_S=${MOTOR_M1_MAX_RATE_CDEG_S}
        MOTOR_M2_MAX_RATE_CDEG_S=${MOTOR_M2_MAX_RATE_CDEG_S}
        MOTOR_M1_REFERENCE_VALID=$<BOOL:${MOTOR_M1_REFERENCE_VALID}>
        MOTOR_M2_REFERENCE_VALID=$<BOOL:${MOTOR_M2_REFERENCE_VALID}>
    )
endif()

pico_add_extra_outputs(Servo-Chip)
