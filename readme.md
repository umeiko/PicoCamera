# PicoCamera

<p align="center">
  <img src="docs/image_project_logo.png" width="220" alt="PicoCamera logo">
</p>

**English** · [中文](readme_cn.md)

📖 **Documentation:** [English](https://umeiko.github.io/PicoCamera/) · [中文](https://umeiko.github.io/PicoCamera/zh/)

A camera library for RP2040 with an API aligned with [esp32-camera](https://github.com/espressif/esp32-camera) (`esp_` prefix becomes `pico_`), based on PIO + DMA capture.

- All pins (including VSYNC/HREF/PCLK/data) are configured at runtime in `camera_config_t` — no PIO source editing required
- No pioasm build step; works out of the box in Arduino IDE and PlatformIO
- Automatic sensor model detection (reads PID over SCCB)

## Supported sensors

Sensor list aligned with [esp32-camera](https://github.com/espressif/esp32-camera); the last column shows PicoCamera's current support status:

| model   | max resolution | color type | output format                                                | PicoCamera support |
| ------- | -------------- | ---------- | ------------------------------------------------------------ | ------------------ |
| OV2640  | 1600 x 1200    | color      | YUV(422/420)/YCbCr422<br>RGB565/555<br>8-bit compressed data<br>8/10-bit Raw RGB data |  RGB565<br>JPEG |
| OV3660  | 2048 x 1536    | color      | raw RGB data<br/>RGB565/555/444<br/>CCIR656<br/>YCbCr422<br/>compression | RGB565<br>JPEG |
| OV5640  | 2592 x 1944    | color      | RAW RGB<br/>RGB565/555/444<br/>CCIR656<br/>YUV422/420<br/>YCbCr422<br/>compression | ❌ Not supported yet |
| OV7670  | 640 x 480      | color      | Raw Bayer RGB<br/>Processed Bayer RGB<br>YUV/YCbCr422<br>GRB422<br>RGB565/555 |  RGB565  |
| OV7725  | 640 x 480      | color      | Raw RGB<br/>GRB 422<br/>RGB565/555/444<br/>YCbCr 422         | ❌ Not supported yet |
| NT99141 | 1280 x 720     | color      | YCbCr 422<br/>RGB565/555/444<br/>Raw<br/>CCIR656<br/>JPEG compression | ❌ Not supported yet |
| GC032A  | 640 x 480      | color      | YUV/YCbCr422<br/>RAW Bayer<br/>RGB565                        | ❌ Not supported yet |
| GC0308  | 640 x 480      | color      | YUV/YCbCr422<br/>RAW Bayer<br/>RGB565<br/>Grayscale          | ❌ Not supported yet |
| GC2145  | 1600 x 1200    | color      | YUV/YCbCr422<br/>RAW Bayer<br/>RGB565                        | RGB565 |
| BF3005  | 640 x 480      | color      | YUV/YCbCr422<br/>RAW Bayer<br/>RGB565                        | ❌ Not supported yet |
| BF20A6  | 640 x 480      | color      | YUV/YCbCr422<br/>RAW Bayer<br/>Only Y                        | ❌ Not supported yet |
| SC101IOT| 1280 x 720     | color      | YUV/YCbCr422<br/>Raw RGB                                     | ❌ Not supported yet |
| SC030IOT| 640 x 480      | color      | YUV/YCbCr422<br/>RAW Bayer                                   | ❌ Not supported yet |
| SC031GS | 640 x 480      | monochrome | RAW MONO<br/>Grayscale                                       | ❌ Not supported yet |
| HM0360  | 656 x 496      | monochrome | RAW MONO<br/>Grayscale                                       | ❌ Not supported yet |
| HM1055  | 1280 x 720     | color      | 8/10-bit Raw<br/>YUV/YCbCr422<br/>RGB565/555/444             | ❌ Not supported yet |

All supported sensors also expose `set_reg`/`get_reg`-style raw register access; more sensor controls are ported on demand.

**Requesting JPEG on a sensor without a JPEG encoder** (e.g. OV7670, GC2145) makes `pico_camera_init()` fail with `PICO_CAMERA_ERR_NOT_SUPPORTED` — same behavior as esp32-camera. A `frame_size` beyond the sensor's maximum is clamped to the maximum with a warning instead of failing.

## Installation

### Arduino IDE

PicoCamera is published in the Arduino Library Manager: open **Sketch → Include Library → Manage Libraries...**, search for **PicoCamera** and click **Install**.

<p align="center">
  <img src="docs/image_arduino_lib.png" width="560" alt="PicoCamera in the Arduino Library Manager">
</p>

Manual install also works: put this repository into `Arduino/libraries/`, or zip it and use "Add .ZIP Library".

### PlatformIO

PicoCamera is published in the [PlatformIO Registry](https://registry.platformio.org/libraries/umeiko/PicoCamera):

```ini
lib_deps =
    umeiko/PicoCamera@^0.4.1
```

A git URL (`lib_deps = https://github.com/umeiko/PicoCamera.git`) or a local `file://` path works too.

### Pico SDK (bare-metal CMake)

The library is pure Pico SDK code (no Arduino dependency), so bare CMake projects can consume it directly (SDK ≥ 1.5.0):

```cmake
add_subdirectory(path/to/PicoCamera)
target_link_libraries(my_app pico_stdlib pico_camera)
```

See [examples/pico_sdk_capture](https://github.com/umeiko/PicoCamera/tree/main/examples/pico_sdk_capture) for a complete project, and the [user guide](docs/user_guide.md) for details.

Both Arduino/PlatformIO require the earlephilhower [arduino-pico](https://github.com/earlephilhower/arduino-pico) core.

### If your core bundles PicoCamera

Newer arduino-pico releases may bundle a copy of PicoCamera. A copy you
install yourself (Library Manager, PlatformIO `lib_deps`, git) always takes
priority over the bundled one, so you can track the latest release without
waiting for a core release. To check which version your sketch compiled
against, use the macros from `pico_camera.h`:

```cpp
Serial.println(PICO_CAMERA_VERSION_STRING);        // e.g. "0.3.0"
#if PICO_CAMERA_VERSION_HEX >= 0x00030000          // feature guards
...
#endif
```

## Quick start

<p align="center">
  <img src="docs/image_pico_board.png" width="360" alt="RP2040 board with camera module">
  <br>
  <i>An RP2040 board with an OV2640/OV3660 camera module over FPC</i>
</p>

```cpp
#include <PicoCamera.h>

camera_config_t config = {
    .pin_pwdn = -1, .pin_reset = 29, .pin_xclk = 5,
    .pin_sccb_sda = 12, .pin_sccb_scl = 13,
    .pin_d0 = 14, .pin_d1 = 15, .pin_d2 = 16, .pin_d3 = 17,
    .pin_d4 = 18, .pin_d5 = 19, .pin_d6 = 20, .pin_d7 = 21,
    .pin_vsync = 2, .pin_href = 3, .pin_pclk = 4,
    .xclk_freq_hz = 10000000,
    .sccb_i2c_port = 0,
    .pixel_format = PIXFORMAT_RGB565,   // or PIXFORMAT_JPEG
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 10,
    .fb_count = 1,
    .fb_location = PICO_CAMERA_FB_AUTO,
};

void setup() {
    pico_camera_init(&config);
}

void loop() {
    camera_fb_t *fb = pico_camera_fb_get();
    if (fb) {
        // fb->buf / fb->len / fb->width / fb->height
        // In JPEG mode, buf is a complete JPEG file (FFD8...FFD9)
        pico_camera_fb_return(fb);
    }
}
```

Sensor control, esp32-camera style:

```cpp
sensor_t *s = pico_camera_sensor_get();
if (s) {
    if (s->set_vflip)   s->set_vflip(s, 1);
    if (s->set_quality) s->set_quality(s, 10);
}
```

## Differences from esp32-camera

- Data pins `pin_d0..pin_d7` must be **8 consecutive GPIOs** (a hardware limitation of the PIO `in pins` instruction); validated at init
- SCCB pins are hard-muxed (real I2C, not bit-banged like ESP32): SDA on even GPIOs, SCL on odd GPIOs, and the pair must route to `sccb_i2c_port` — SDA GP0/4/8...28 and SCL GP1/5/9...29 for I2C0, SDA GP2/6/10...26 and SCL GP3/7/11...27 for I2C1 (extended to GP47 on RP2350). Validated at init
- On RP2350 boards with PSRAM (enabled via the core's PSRAM menu), frame buffers can live in PSRAM — set `fb_location = PICO_CAMERA_FB_IN_PSRAM` to open up larger frame sizes. The default `PICO_CAMERA_FB_AUTO` uses PSRAM when available and **falls back to SRAM** otherwise (unlike esp32-camera, which fails init). RP2040 always uses SRAM (264KB): for RGB565 stay at or below QVGA in practice; the JPEG buffer is estimated as `width*height/4 + 8KB`
- SCCB can share an already initialized I2C bus: set `pin_sccb_sda = -1` and pick the bus with `sccb_i2c_port` (e.g. `1` to reuse `Wire1`, esp32-camera parity). You must `Wire1.begin()` (or `i2c_init()`) first; the library never touches or deinitializes a shared bus
- `grab_mode` is not supported: `pico_camera_fb_get()` captures one frame blocking (equivalent to `CAMERA_GRAB_WHEN_EMPTY`)

## Examples

- [CameraSerialInfo](https://github.com/umeiko/PicoCamera/tree/main/examples/CameraSerialInfo) — detect the sensor and print its info
- [CameraCaptureRGB565](https://github.com/umeiko/PicoCamera/tree/main/examples/CameraCaptureRGB565) — RGB565 capture
- [CameraCaptureJPEG](https://github.com/umeiko/PicoCamera/tree/main/examples/CameraCaptureJPEG) — JPEG capture (with SOI/EOI integrity check)
- [camera_render_to_tft](https://github.com/umeiko/PicoCamera/tree/main/examples/camera_render_to_tft) — live preview on a TFT display via TFT_eSPI
- [push_image_to_python](https://github.com/umeiko/PicoCamera/tree/main/examples/push_image_to_python) — stream frames over USB serial to a Python (tkinter) viewer

<p align="center">
  <img src="docs/image_capture.png" width="360" alt="push_image_to_python host viewer">
  <br>
  <i>push_image_to_python: a live OV2640 JPEG stream rendered by the Python viewer</i>
</p>

## Documentation

- [User guide](docs/user_guide.md) — configuration, capture workflow, `camera_fb_t` and `sensor_t` reference（[中文版](docs/user_guide_zh.md)）
- Full API reference: <https://umeiko.github.io/PicoCamera/>

## License

MIT
