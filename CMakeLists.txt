# PicoCamera — camera library for RP2040 with an esp32-camera compatible API.
#
# This CMakeLists.txt lets bare Pico SDK projects consume the library:
#
#     add_subdirectory(path/to/PicoCamera)
#     target_link_libraries(my_app pico_stdlib pico_camera)
#
# or via FetchContent:
#
#     FetchContent_Declare(PicoCamera
#         GIT_REPOSITORY https://github.com/umeiko/PicoCamera.git
#         GIT_TAG main)
#     FetchContent_MakeAvailable(PicoCamera)
#     target_link_libraries(my_app pico_stdlib pico_camera)
#
# Requires Pico SDK >= 1.5.0 (pio_encode_* API). Arduino IDE and
# PlatformIO builds ignore this file.

add_library(pico_camera STATIC
    src/pico_camera.cpp
    src/driver/pio_capture.cpp
    src/driver/sccb.cpp
    src/driver/xclk.cpp
    src/sensors/ov2640.cpp
    src/sensors/ov3660.cpp
    src/sensors/ov7670.cpp
    src/sensors/ov7725.cpp
    src/sensors/gc2145.cpp
    src/sensors/gc0308.cpp
    src/sensors/gc032a.cpp
)

target_include_directories(pico_camera PUBLIC src)

target_link_libraries(pico_camera PUBLIC
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_i2c
    hardware_pwm
    hardware_gpio
    hardware_clocks
)
