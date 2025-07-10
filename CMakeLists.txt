cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(rfid_security C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_definitions(-DPICO_ENTER_USB_BOOT_ON_EXIT)

pico_sdk_init()

add_subdirectory(modules/pico-rfid-2)

add_executable(rfid_security main.cpp)

pico_enable_stdio_usb(rfid_security 1)
pico_enable_stdio_uart(rfid_security 0)

pico_add_extra_outputs(rfid_security)

target_link_libraries(rfid_security pico_stdlib pico_rfid_2 hardware_pwm)
