# push_image_to_python

**[中文文档](readme_cn.md)**

Streams camera frames over USB serial to a PC, where a Python (tkinter) GUI displays them in real time.

## Protocol

Binary frames, two formats:

| Format | Frame layout |
|--------|--------------|
| RGB565 | `SRGB` + `width*height*2` raw bytes (big-endian RGB565) + `ERGB` |
| JPEG | `SJPG` + raw JPEG data (`FFD8...FFD9`) + `EJPG` |

## Firmware (`push_image_to_python.ino`)

Select the pushed format with the macro at the top of the sketch before flashing:

```cpp
#define PUSH_FORMAT 1   // 1 = JPEG (SJPG...EJPG), 0 = RGB565 (SRGB...ERGB)
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

- RGB565 mode parses a fixed 320x240 frame; if you change `frame_size` in the sketch, update `RGB_WIDTH` / `RGB_HEIGHT` at the top of the .py file to match
- Validate the link with JPEG mode first (an order of magnitude less data, much higher frame rate); a raw RGB565 frame is ~150KB, so the frame rate over USB CDC is limited
- The viewer verifies the `FFD8` header on JPEG frames to avoid false `EJPG` trailer matches inside JPEG entropy data
