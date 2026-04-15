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

project(Remote-Station-LoRA C CXX ASM)

pico_sdk_init()

add_executable(Remote-Station-LoRA
        main.c
        uart.c
        helpers.c
        lora.c
        cjson.c
)

pico_set_program_name(Remote-Station-LoRA "Remote-Station-LoRA")
pico_set_program_version(Remote-Station-LoRA "0.1")

pico_enable_stdio_uart(Remote-Station-LoRA 0)
pico_enable_stdio_usb(Remote-Station-LoRA 1)

target_link_libraries(Remote-Station-LoRA
        pico_stdlib
        pico_time
)

target_include_directories(Remote-Station-LoRA PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(Remote-Station-LoRA
        hardware_uart
        hardware_irq
)

pico_add_extra_outputs(Remote-Station-LoRA)