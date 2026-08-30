# Changelog

All notable changes to PicoCamera are documented here. The project follows
[Semantic Versioning](https://semver.org/).

## [0.4.2] - 2026-08-30

### Added

- **GC0308 sensor support** (RGB565, up to VGA; no on-chip JPEG encoder).
  Driver adapted from esp32-camera (Apache-2.0), hardware-verified on RP2040.
- **GC032A sensor support** (RGB565, up to VGA; no on-chip JPEG encoder).
  Driver adapted from esp32-camera, with host-specific tweaks for the RP2040
  capture path: PLL/DVP clock division and VSYNC polarity (P0_SYNC_MODE bit0
  cleared — with the esp32-camera default the frame capture never sees a
  VSYNC edge and times out silently). Detection, streaming and the colorbar
  test pattern verified on RP2040; full image quality still pending a
  replacement module (the first unit shipped with two dead data lines).
  Marked **untested** in the readme until then.
- **OV7725 sensor support** (RGB565, up to VGA; no on-chip JPEG encoder).
  Driver adapted from esp32-camera (OpenMV, MIT). Marked **untested** — no
  genuine OV7725 module at hand yet.
- **`PIXFORMAT_YUV422` output** (packed YUYV, 2 bytes/pixel) for
  OV2640/OV3660/OV7670/GC2145/GC0308, hardware-verified on RP2040. On the
  OV7670 this required `COM13` bit0 (UV swap): the sensor emits YVYU, which
  esp32-camera never notices because the ESP32 capture hardware swaps bytes.
- **`PIXFORMAT_GRAYSCALE` output** (Y only, 1 byte/pixel) for GC0308.
- `pico_camera_err_str()` — maps an error code to a human-readable string;
  all examples now print the failure reason instead of a bare number.
- push_image_to_python: the viewer decodes YUV422 and grayscale frames and
  shows the active protocol next to the fps counter.

## [0.4.1] - 2026-08-27

### Added

- **GC2145 sensor support** (RGB565, up to UXGA; no on-chip JPEG encoder).
  Driver adapted from esp32-camera (Apache-2.0), hardware-verified on RP2040.
  Ships with two host-specific tweaks baked in: VSYNC polarity for the PIO
  capture engine, and a PCLK divider tuned for reliable capture over
  breadboard wiring (~10.6 fps at QVGA with 10 MHz XCLK).
- `CameraCaptureJPEG` / `CameraCaptureRGB565` examples print the detected
  sensor model after init, plus a per-frame capture duration and a frame
  time / frame rate estimate.
- The push_image_to_python example documents the measured USB CDC link
  throughput (~320 KB/s) and the resulting frame-rate ceiling per frame
  size, in the sketch header and both readmes.

### Fixed

- **Capture frame alignment**: `pio_sm_restart()` does not reset the PIO
  program counter, so from the second frame on, captures could start at a
  random line instead of waiting for the VSYNC edge. In continuous
  streaming this showed up as the picture scrolling vertically by a varying
  offset each frame, with occasional torn/garbage frames. The capture
  engine now forces the PC back to the program start on every frame.
- **OV2640 RGB565 byte order**: the register table streamed the low byte
  first; colors came out scrambled. Now big-endian, matching esp32-camera
  and the PicoCamera python viewer.
- **Init without a hardware reset line**: `pico_camera_init()` now performs
  an SCCB software reset after detection, so the sensor restarts from its
  default register set even when `pin_reset = -1`. Previously the sensor
  kept state left behind by the previous firmware and init could fail with
  `PICO_CAMERA_ERR_FAILED_TO_SET_OUT_FORMAT`.

### Changed

- All examples default `pin_reset` to `-1` (no reset line required).
- Documentation: GC2145 added to the supported-sensor tables in the readme
  (EN/CN) and the user guide (EN/CN); ARCHITECTURE status updated.

## [0.4.0]

- PSRAM frame buffers (`fb_location`, esp32-camera parity)
- Shared SCCB bus mode (`pin_sccb_sda = -1` reuses an initialized I2C bus)

## [0.3.0]

- Version macros (`PICO_CAMERA_VERSION_*`) for feature guards when the
  Arduino core bundles a copy of the library
- Bare Pico SDK (CMake) project support
- Bilingual Doxygen documentation site

## [0.2.0]

- OV7670 support (RGB565, no JPEG) with per-sensor capability checks
  (`support_jpeg`, frame-size clamping)
