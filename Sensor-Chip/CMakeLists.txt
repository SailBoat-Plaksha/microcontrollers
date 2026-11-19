# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

set(PICO_BOARD pico CACHE STRING "Board type")
set(PICO_PLATFORM rp2040)

include(pico_sdk_import.cmake)

project(Sensor-Chip C CXX ASM)

pico_sdk_init()

add_executable(Sensor-Chip
        main.c
        gps.c
)

pico_set_program_name(Sensor-Chip "Sensor-Chip")
pico_set_program_version(Sensor-Chip "0.1")

pico_enable_stdio_uart(Sensor-Chip 0)
pico_enable_stdio_usb(Sensor-Chip 1)

target_link_libraries(Sensor-Chip
        pico_stdlib
        hardware_uart
)

target_include_directories(Sensor-Chip PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

pico_add_extra_outputs(Sensor-Chip)
