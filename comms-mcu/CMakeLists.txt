cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()
if(NOT PICO_SDK_PATH)
    message(FATAL_ERROR "Set PICO_SDK_PATH to build the Pico firmware")
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(boat_comms_mcu C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(comms_mcu main.c relay.c modem.c)
target_include_directories(comms_mcu PRIVATE
    include
    ../protocol
    ../protocol/gen/C
    ../protocol/gen/C/arsem_standalone
)
target_link_libraries(comms_mcu PRIVATE
    pico_stdlib
    hardware_i2c
    hardware_uart
    hardware_gpio
    pico_i2c_slave
)
pico_enable_stdio_usb(comms_mcu 0)
pico_enable_stdio_uart(comms_mcu 0)
pico_add_extra_outputs(comms_mcu)
