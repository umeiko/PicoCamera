# PicoCamera

<p align="center">
  <img src="docs/image_project_logo.png" width="220" alt="PicoCamera logo">
</p>

**[中文文档](readme_cn.md)**

A camera library for RP2040 with an API aligned with [esp32-camera](https://github.com/espressif/esp32-camera) (`esp_` prefix becomes `pico_`), based on PIO + DMA capture.

- All pins (including VSYNC/HREF/PCLK/data) are configured at runtime in `camera_config_t` — no PIO source editing required
- No pioasm build step; works out of the box in Arduino IDE and PlatformIO
- Automatic sensor model detection (reads PID over SCCB)

## Supported sensors

| Sensor | RGB565 | JPEG | Status |
|--------|--------|------|--------|
| OV2640 | ✅ | ✅ | Verified on hardware |
| OV3660 | ✅ | ✅ | Verified on hardware |

For sensor controls (`sensor->set_vflip` etc.), OV2640 is fairly complete (vflip/hmirror/brightness/contrast/saturation/white balance/exposure/effects/quality...); OV3660 currently implements vflip/hmirror/framesize/pixformat, more on demand.

## Installation

- **Arduino IDE**: put this repository into `Arduino/libraries/`, or zip it and use "Add .ZIP Library"
- **PlatformIO**: `lib_deps = https://github.com/umeiko/PicoCamera.git` (or a local `file://` path)

Requires the earlephilhower [arduino-pico](https://github.com/earlephilhower/arduino-pico) core.

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
- No PSRAM; frame buffers live in SRAM (264KB). For RGB565 stay at or below QVGA in practice; the JPEG buffer is estimated as `width*height/4 + 8KB`
- `grab_mode` is not supported: `pico_camera_fb_get()` captures one frame blocking (equivalent to `CAMERA_GRAB_WHEN_EMPTY`)

## Examples

- `examples/CameraSerialInfo` — detect the sensor and print its info
- `examples/CameraCaptureRGB565` — RGB565 capture
- `examples/CameraCaptureJPEG` — JPEG capture (with SOI/EOI integrity check)
- `examples/camera_render_to_tft` — live preview on a TFT display via TFT_eSPI
- `examples/push_image_to_python` — stream frames over USB serial to a Python (tkinter) viewer

<p align="center">
  <img src="docs/image_capture.png" width="360" alt="push_image_to_python host viewer">
  <br>
  <i>push_image_to_python: a live OV2640 JPEG stream rendered by the Python viewer</i>
</p>

## Documentation

- Architecture and roadmap: `ARCHITECTURE.md` (Chinese)
- OV2640 register reference: `docs/ov2640_registers.md` (Chinese)
- Early driver debug notes: `docs/ov2640_dev_notes.md` (Chinese)

## License

MIT
