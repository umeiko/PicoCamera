# push_image_to_python

**[中文文档](readme_cn.md)**

Streams camera frames over USB serial to a PC, where a Python (tkinter) GUI displays them in real time.

## Protocol

Binary frames, four formats:

| Format | Frame layout |
|--------|--------------|
| RGB565 | `SRGB` + `width*height*2` raw bytes (big-endian RGB565) + `ERGB` |
| JPEG | `SJPG` + raw JPEG data (`FFD8...FFD9`) + `EJPG` |
| YUV422 | `SYUV` + `width*height*2` packed YUYV bytes + `EYUV` |
| GRAYSCALE | `SGRY` + `width*height` raw Y bytes + `EGRY` |

## Firmware (`push_image_to_python.ino`)

Select the pushed format with the macro at the top of the sketch before flashing:

```cpp
#define PUSH_FORMAT 1   // 0 = RGB565, 1 = JPEG, 2 = YUV422, 3 = GRAYSCALE
```

Default resolution is 320x240 (FRAMESIZE_QVGA). Once flashed, the board streams frames continuously; no interaction needed.

## PC viewer (`push_image_to_python.py`)

Dependencies (the only two external packages):

```
pip install pyserial Pillow
```

Run:

```
python push_image_to_python.py
```

Usage:

1. **Refresh** — rescan serial ports
2. Pick the Pico's port from the dropdown (e.g. `COM4`)
3. **Connect** — start receiving and rendering; click again to disconnect

The status bar shows the live frame rate.

<p align="center">
  <img src="../../docs/image_capture.png" width="360" alt="push_image_to_python viewer">
  <br>
  <i>The viewer rendering a live OV2640 JPEG stream at ~7 fps</i>
</p>

## Notes

- JPEG mode requires a sensor with a JPEG encoder (OV2640/OV3660); on OV7670 `pico_camera_init()` fails with `PICO_CAMERA_ERR_NOT_SUPPORTED` — use `PUSH_FORMAT 0` (RGB565) there
- RGB565 mode parses a fixed 320x240 frame; if you change `frame_size` in the sketch, update `WIDTH` / `HEIGHT` at the top of the .py file to match
- Validate the link with JPEG mode first (an order of magnitude less data, much higher frame rate); a raw RGB565 frame is ~150KB, so the frame rate over USB CDC is limited
- Frame rate is bounded by link throughput: capture and USB transmission run back-to-back in the sketch, and this USB CDC link (Arduino-Pico `Serial` + pyserial) measures ~320 KB/s. Rule of thumb: viewer fps ≈ 320 / frame size in KB — QVGA RGB565 (~150KB) lands at ~2 fps, QQVGA (~38KB) at ~8 fps, JPEG frames (~10-20KB) are no longer transport-bound. Lower `frame_size` for a smoother raw stream
- The viewer verifies the `FFD8` header on JPEG frames to avoid false `EJPG` trailer matches inside JPEG entropy data
