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

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Servo-Chip
        main.c
        uart.c
        helpers.c
        servo.c
)

pico_set_program_name(Servo-Chip "Servo-Chip")
pico_set_program_version(Servo-Chip "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Servo-Chip 1)
pico_enable_stdio_usb(Servo-Chip 0)

# Add the standard library to the build
target_link_libraries(Servo-Chip
        pico_stdlib
        pico_time
        )

# Add the standard include files to the build
target_include_directories(Servo-Chip PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# Add any user requested libraries
target_link_libraries(Servo-Chip
        hardware_uart 
        hardware_pwm
        #hardware_i2c
        #hardware_interp
        #hardware_timer
        #hardware_watchdog
        #hardware_clocks
        )

pico_add_extra_outputs(Servo-Chip)
pico_enable_stdio_uart(Servo-Chip 0)
pico_enable_stdio_usb(Servo-Chip 1)


