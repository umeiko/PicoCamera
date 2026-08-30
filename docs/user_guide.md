# PicoCamera User Guide

This guide walks through configuring, starting and using the camera.
For the full symbol reference see the API pages of `pico_camera.h` and
`sensor.h`; for runnable sketches see the
[examples](https://github.com/umeiko/PicoCamera/tree/main/examples).

## 1. Configuration: camera_config_t

Everything the driver needs is supplied once, at init time, through
`camera_config_t`. There is no board-level hardcoding.

| Field | Meaning | Notes |
|-------|---------|-------|
| `pin_pwdn` | Camera power-down GPIO | `-1` if not connected |
| `pin_reset` | Camera reset GPIO | `-1` if not connected |
| `pin_xclk` | Master clock (XCLK) output GPIO | Driven by a PWM slice |
| `pin_sccb_sda` / `pin_sccb_scl` | SCCB (I2C) data/clock GPIOs | Used to detect and configure the sensor. Pins are hard-muxed: SDA even, SCL odd, both routing to `sccb_i2c_port` (`(pin / 2) % 2 == port`). Set `pin_sccb_sda = -1` to reuse an already initialized I2C bus (esp32-camera parity; `pin_sccb_scl` is then ignored) — see the shared-bus note below |
| `pin_d0` .. `pin_d7` | 8 parallel data lines | **Must be 8 consecutive GPIOs** (`pin_dN == pin_d0 + N`, a PIO hardware constraint; validated at init) |
| `pin_vsync` / `pin_href` / `pin_pclk` | Frame sync / line sync / pixel clock inputs | Free choice of GPIO |
| `xclk_freq_hz` | XCLK frequency in Hz | `0` selects the default 10 MHz; 10–24 MHz is the typical sensor range |
| `sccb_i2c_port` | RP2040 I2C peripheral used for SCCB | `0` or `1`; in shared-bus mode (`pin_sccb_sda = -1`) selects which initialized bus to reuse |
| `pixel_format` | `PIXFORMAT_RGB565`, `PIXFORMAT_YUV422`, `PIXFORMAT_GRAYSCALE` or `PIXFORMAT_JPEG` | YUV422 streams packed YUYV (2 bytes/pixel, supported on OV2640/OV3660/OV7670/GC2145/GC0308). GRAYSCALE is Y-only (1 byte/pixel, GC0308). JPEG requires a sensor with an on-chip encoder (OV2640/OV3660 yes, OV7670/GC2145/GC0308/GC032A no) |
| `frame_size` | `FRAMESIZE_*` enum | Beyond-sensor sizes are clamped to the sensor maximum with a warning instead of failing |
| `jpeg_quality` | 0–63, **lower = higher quality** | JPEG mode only |
| `fb_count` | Number of frame buffers to allocate | Buffers live in SRAM (264 KB total) unless PSRAM is used; see the memory notes below |
| `fb_location` | Where frame buffers live | `PICO_CAMERA_FB_AUTO` (default): PSRAM when available, SRAM otherwise. `PICO_CAMERA_FB_IN_PSRAM`: PSRAM only, init fails if unavailable (esp32-camera parity). `PICO_CAMERA_FB_IN_SRAM`: on-chip SRAM only |

Available frame sizes (see `framesize_t`): 96x96, QQVGA 160x120, QCIF
176x144, HQVGA 240x176, 240x240, QVGA 320x240, CIF 400x296, HVGA 480x320,
VGA 640x480, SVGA 800x600, XGA 1024x768, HD 1280x720, SXGA 1280x1024,
UXGA 1600x1200.

Memory planning (RP2040 has 264 KB SRAM, no PSRAM; RP2350 boards with a
PSRAM chip can place buffers in PSRAM when the core's PSRAM support is
enabled — see `fb_location` above):

- RGB565 needs `width * height * 2` bytes per buffer — QVGA (153 KB) is
  the practical ceiling in SRAM; VGA (600 KB) does not fit but works from
  PSRAM on RP2350.
- JPEG buffers are allocated as `width * height / 4 + 8 KB`, which covers
  typical scenes; pathological noise can overflow it.

Shared SCCB bus (esp32-camera parity): to put the sensor on an I2C bus you
already use for other devices, initialize that bus yourself
(`Wire.begin()` / `Wire1.begin()` under Arduino, `i2c_init()` under the
bare Pico SDK), then set `config.pin_sccb_sda = -1` and
`config.sccb_i2c_port = 0 or 1`. The library skips all pin/bus setup and
never deinitializes a shared bus at `pico_camera_deinit()`.

## 2. Bringing the camera up and capturing frames

The lifecycle is intentionally the same shape as esp32-camera:

1. **Fill `camera_config_t`** and call `pico_camera_init(&config)`.
   This probes the sensor over SCCB (returns
   `PICO_CAMERA_ERR_NOT_DETECTED` if nothing answers), validates the pin
   layout, rejects `PIXFORMAT_JPEG` on sensors without an encoder
   (`PICO_CAMERA_ERR_NOT_SUPPORTED`), allocates the frame buffers, and
   arms the PIO + DMA capture engine. Call it once; call
   `pico_camera_deinit()` before re-initializing with a different config.
2. **Capture** with `camera_fb_t *fb = pico_camera_fb_get();` — it blocks
   until one full frame has been DMA'd into a buffer, and returns `NULL`
   on error or when no free buffer is available.
3. **Consume** the frame through `fb->buf` / `fb->len` / `fb->width` /
   `fb->height` (details in the next section), then **release** it with
   `pico_camera_fb_return(fb)`. Buffers are recycled; not returning them
   starves the driver once all `fb_count` buffers are checked out.
4. **Shutdown** (optional) with `pico_camera_deinit()`, which stops the
   PIO/DMA engine, releases the SCCB bus and frees all buffers.

What you do with each frame is up to the application: push it to a TFT
(`camera_render_to_tft` example), stream it over USB serial
(`push_image_to_python` example), or hand `fb->buf` to your own
processing pipeline (e.g. a vision/inference task). Grab the frame,
process, return the buffer — that loop is all the driver asks.

Error codes returned by `pico_camera_init()` / `pico_camera_deinit()`:

| Code | Meaning |
|------|---------|
| `PICO_CAMERA_OK` (0) | Success |
| `PICO_CAMERA_ERR_NOT_DETECTED` | No sensor answered on SCCB |
| `PICO_CAMERA_ERR_NOT_SUPPORTED` | e.g. JPEG requested on OV7670 |
| `PICO_CAMERA_ERR_INVALID_ARG` | Bad config (e.g. non-consecutive data pins) |
| `PICO_CAMERA_ERR_INVALID_STATE` | Init called twice, or deinit without init |
| `PICO_CAMERA_ERR_NO_MEM` | Frame buffer allocation failed |
| `PICO_CAMERA_ERR_TIMEOUT` | Capture timed out |
| `PICO_CAMERA_ERR_FAILED_TO_SET_FRAME_SIZE` / `..._SET_OUT_FORMAT` | Sensor rejected the format/size |

## 3. camera_fb_t: the frame buffer

```cpp
typedef struct {
    uint8_t *buf;        // pixel data
    size_t   len;        // used bytes in buf
    size_t   width;      // pixels
    size_t   height;     // pixels
    pixformat_t format;  // PIXFORMAT_RGB565 or PIXFORMAT_JPEG
    struct timeval timestamp;  // capture time since boot
} camera_fb_t;
```

- **RGB565**: `len == width * height * 2`, one 16-bit pixel per 2 bytes.
- **JPEG**: `buf` is a complete, standalone JPEG file — starts with
  `0xFF 0xD8` (SOI), ends with `0xFF 0xD9` (EOI); `len` varies per frame.
  You can write it straight to a file or socket.

Functions operating on frame buffers:

| Function | Purpose |
|----------|---------|
| `pico_camera_fb_get()` | Borrow a buffer with a freshly captured frame (blocking) |
| `pico_camera_fb_return(fb)` | Give the buffer back for reuse — always call this |

With `fb_count > 1` the DMA engine can fill the next buffer while your
code still processes the previous one; with `fb_count == 1` each
`fb_get()` waits for a new frame to be captured after the previous
buffer was returned.

## 4. sensor_t: runtime sensor control

`pico_camera_sensor_get()` returns the detected sensor's control
structure (or `NULL` before init). All controls are function pointers;
**a sensor only implements a subset — always NULL-check before calling**:

```cpp
sensor_t *s = pico_camera_sensor_get();
if (s && s->set_vflip) s->set_vflip(s, 1);
```

Available operations:

| Function | Purpose |
|----------|---------|
| `set_pixformat(fmt)` | Switch between RGB565 and JPEG at runtime |
| `set_framesize(size)` | Change resolution at runtime |
| `set_brightness(level)` / `set_contrast(level)` / `set_saturation(level)` / `set_sharpness(level)` | Image tuning, small-integer levels (on OV2640 brightness/contrast/saturation accept −2…2) |
| `set_gainceiling(gc)` | AGC ceiling, `GAINCEILING_2X` … `GAINCEILING_128X` |
| `set_quality(q)` | JPEG quality 0–63, lower = better |
| `set_colorbar(on)` | Test color-bar pattern |
| `set_whitebal(on)` / `set_wb_mode(mode)` | Auto white balance switch / WB preset mode |
| `set_gain_ctrl(on)` / `set_exposure_ctrl(on)` | AGC / AEC switch |
| `set_ae_level(level)` / `set_aec_value(value)` | AE target level / manual exposure value |
| `set_hmirror(on)` / `set_vflip(on)` | Horizontal mirror / vertical flip |
| `set_special_effect(effect)` | Effect index (grayscale, negative, …) |
| `set_reg(reg, mask, value)` / `get_reg(reg, mask)` | Raw register write/read; `mask` selects which bits to touch. Escape hatch for anything not wrapped above |
| `set_xclk(timer, xclk)` | Change XCLK at runtime (`timer` unused on RP2040) |
| `reset()` | Sensor soft reset |

Per-sensor support:

| Operation | OV2640 | OV3660 | OV7670 | GC2145 | GC0308 | GC032A |
|-----------|:------:|:------:|:------:|:------:|:------:|:------:|
| pixformat / framesize | ✅ | ✅ | ✅ | ✅ (RGB565 only) | ✅ (RGB565 only) | ✅ (RGB565 only) |
| hmirror / vflip | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| brightness / contrast / saturation / sharpness | ✅ | — | — | — | — | — |
| gainceiling / quality | ✅ | — | — (no JPEG) | — (no JPEG) | — (no JPEG) | — (no JPEG) |
| colorbar | ✅ | — | ✅ | — | — | ✅ |
| whitebal / wb_mode | ✅ | — | whitebal only | — | — | — |
| gain_ctrl / exposure_ctrl | ✅ | — | ✅ | — | — | — |
| ae_level / aec_value / special_effect | ✅ | — | — | — | — | — |
| set_reg / get_reg | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

## 5. Sensor identification

`pico_camera_sensor_info_get()` returns static information about the
detected model: `name`, `sccb_addr`, `pid`, `max_size` (largest
`framesize_t` supported) and `support_jpeg`. Useful for adapting your
config at runtime before or after init.

## 6. Using from a Pico SDK project (no Arduino)

The library is pure Pico SDK code — nothing in `src/` touches the
Arduino core — so a bare CMake project can use it directly. Requires
**Pico SDK ≥ 1.5.0** (the `pio_encode_*` API).

In your project's `CMakeLists.txt`:

```cmake
include(pico_sdk_import.cmake)   # standard SDK boilerplate
project(my_app C CXX ASM)
pico_sdk_init()

add_subdirectory(path/to/PicoCamera)   # this repository

add_executable(my_app main.c)
target_link_libraries(my_app pico_stdlib pico_camera)

pico_enable_stdio_usb(my_app 1)        # camera logs use printf
pico_add_extra_outputs(my_app)
```

Or with FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(PicoCamera
    GIT_REPOSITORY https://github.com/umeiko/PicoCamera.git
    GIT_TAG main)
FetchContent_MakeAvailable(PicoCamera)
target_link_libraries(my_app pico_stdlib pico_camera)
```

In your code, `#include "PicoCamera.h"` and use the API exactly as shown
in sections 1–5 — the only difference from Arduino sketches is that you
manage `stdio_init_all()` yourself. Driver log messages go through
`printf`, so enable USB or UART stdio if you want to see them.

A complete, buildable project lives in
[examples/pico_sdk_capture](https://github.com/umeiko/PicoCamera/tree/main/examples/pico_sdk_capture);
it is compiled against the latest Pico SDK in CI on every change to
`src/`, so what you see there is guaranteed to build.
