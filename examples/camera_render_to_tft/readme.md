# camera_render_to_tft

**[中文文档](readme_cn.md)**

Captures RGB565 frames with PicoCamera and shows a live preview on a TFT display via TFT_eSPI.

## Before you use it: configure TFT_eSPI

This sketch does **not** include any TFT_eSPI display configuration — it will fail to build or show a blank screen without it. Configure the TFT_eSPI library for your display first (a one-time, global setup):

1. Find the TFT_eSPI library directory (Arduino IDE: usually `Documents/Arduino/libraries/TFT_eSPI/`; PlatformIO: `.pio/libdeps/<env>/TFT_eSPI/` in your project)
2. Edit `User_Setup_Select.h` in that directory: comment out the default `#include <User_Setup.h>` and include your own setup file instead, e.g.:

   ```c
   #include <User_Setup_rp2040.h>   // your own RP2040 display setup
   ```

3. In that setup file, define your display parameters: driver chip (e.g. `ST7789_DRIVER`), resolution (`TFT_WIDTH`/`TFT_HEIGHT`), pins (`TFT_CS`/`TFT_DC`/`TFT_RST`/`TFT_MOSI`/`TFT_SCLK`), SPI frequency (`SPI_FREQUENCY`), etc. See the `User_Setups/Setup*` files shipped with TFT_eSPI for reference

Full documentation: https://github.com/Bodmer/TFT_eSPI

## Wiring

Camera (adjust `cam_config` at the top of the sketch):

| Signal | Default GPIO |
|--------|--------------|
| XCLK | GP5 |
| SIOD (SDA) | GP12 |
| SIOC (SCL) | GP13 |
| RESET | GP29 |
| VSYNC | GP2 |
| HREF | GP3 |
| PCLK | GP4 |
| D0..D7 (Y2..Y9) | GP14..GP21 (must be consecutive) |

Display wiring is defined by your own TFT_eSPI setup file.

## Notes

- The sketch captures 320x240 and center-crops to 280x240 for display; if your screen resolution differs, adjust `DISP_WIDTH` / `DISP_HEIGHT` / `CROP_X` in the sketch
- If colors look wrong or the image is garbled, check `TFT_RGB_ORDER` in your TFT_eSPI setup and whether `tft.setSwapBytes(true)` in the sketch is needed
