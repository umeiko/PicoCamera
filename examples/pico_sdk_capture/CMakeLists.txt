# Minimal Pico SDK project using PicoCamera without Arduino.
#
# Build (needs an ARM toolchain + the Pico SDK):
#   cd examples/pico_sdk_capture
#   mkdir build && cd build
#   PICO_SDK_PATH=/path/to/pico-sdk cmake ..
#   make -j
# or configure with -DPICO_SDK_FETCH_FROM_GIT=ON to download the SDK.

cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_sdk_capture C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# The PicoCamera repository root is two levels up from this example.
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. ${CMAKE_BINARY_DIR}/PicoCamera)

add_executable(pico_sdk_capture main.c)

# Camera logs and frame info go to USB serial (printf over stdio).
pico_enable_stdio_usb(pico_sdk_capture 1)
pico_enable_stdio_uart(pico_sdk_capture 0)

target_link_libraries(pico_sdk_capture pico_stdlib pico_camera)

pico_add_extra_outputs(pico_sdk_capture)
