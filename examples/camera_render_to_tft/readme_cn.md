# camera_render_to_tft

用 PicoCamera 采集 RGB565 帧，经 TFT_eSPI 实时显示到 TFT 屏。

## 使用前必做：配置 TFT_eSPI

本例程**不包含** TFT_eSPI 的显示屏配置，直接使用会编译失败或白屏。请先按你的屏幕型号配置 TFT_eSPI 库（全局生效，只需配一次）：

1. 找到 TFT_eSPI 库目录（Arduino IDE 通常在 `文档/Arduino/libraries/TFT_eSPI/`,PlatformIO 在项目的 `.pio/libdeps/<env>/TFT_eSPI/`)
2. 编辑其中的 `User_Setup_Select.h`，注释掉默认的 `#include <User_Setup.h>`，改为包含你自己的配置文件，例如：

   ```c
   #include <User_Setup_rp2040.h>   // 你自己的 RP2040 屏幕配置
   ```

3. 在该配置文件里定义你的屏幕参数：驱动芯片（如 `ST7789_DRIVER`)、分辨率（`TFT_WIDTH`/`TFT_HEIGHT`)、引脚（`TFT_CS`/`TFT_DC`/`TFT_RST`/`TFT_MOSI`/`TFT_SCLK`)、SPI 频率（`SPI_FREQUENCY`）等。可参考 TFT_eSPI 自带的 `User_Setups/Setup*` 示例文件

详细说明见 TFT_eSPI 文档：https://github.com/Bodmer/TFT_eSPI

## 接线

摄像头（在例程顶部的 `cam_config` 里改）：

| 信号 | 默认 GPIO |
|------|-----------|
| XCLK | GP5 |
| SIOD (SDA) | GP12 |
| SIOC (SCL) | GP13 |
| RESET | GP29 |
| VSYNC | GP2 |
| HREF | GP3 |
| PCLK | GP4 |
| D0..D7 (Y2..Y9) | GP14..GP21（必须连续） |

TFT 屏接线由你自己的 TFT_eSPI 配置文件决定。

## 说明

- 默认采集 320x240，居中裁剪为 280x240 显示；屏幕分辨率不同请改例程里的 `DISP_WIDTH` / `DISP_HEIGHT` / `CROP_X`
- 屏幕花屏/颜色异常时，先检查 TFT_eSPI 配置里的 `TFT_RGB_ORDER`，以及例程中的 `tft.setSwapBytes(true)` 是否需要
