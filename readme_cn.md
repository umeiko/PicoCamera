# PicoCamera

<p align="center">
  <img src="docs/image_project_logo.png" width="220" alt="PicoCamera logo">
</p>

RP2040 摄像头库，API 与 [esp32-camera](https://github.com/espressif/esp32-camera) 对齐（`esp_` 前缀换成 `pico_`），基于 PIO + DMA 采集。

- 所有引脚（含 VSYNC/HREF/PCLK/数据线）都在 `camera_config_t` 里运行时配置，无需修改 PIO 源码
- 不依赖 pioasm 构建步骤，Arduino IDE / PlatformIO 直接可用
- 自动探测传感器型号（SCCB 读 PID）

## 支持的传感器

| 传感器 | RGB565 | JPEG | 状态 |
|--------|--------|------|------|
| OV2640 | ✅ | ✅ | 已硬件验证 |
| OV3660 | ✅ | ✅ | 已硬件验证 |

传感器控制（`sensor->set_vflip` 等）：OV2640 较完整（翻转/镜像/亮度/对比度/饱和度/白平衡/曝光/特效/质量……）；OV3660 目前实现了 vflip/hmirror/framesize/pixformat，更多按需补充。

## 安装

- **Arduino IDE**：把本仓库放入 `Arduino/libraries/`，或打包为 zip 通过「添加 .ZIP 库」安装
- **PlatformIO**：`lib_deps = https://github.com/umeiko/PicoCamera.git`（或本地 `file://` 路径）

需要 earlephilhower 的 [arduino-pico](https://github.com/earlephilhower/arduino-pico) 核心。

## 快速开始

<p align="center">
  <img src="docs/image_pico_board.png" width="360" alt="RP2040 开发板连接摄像头模组">
  <br>
  <i>RP2040 开发板通过 FPC 排线连接 OV2640/OV3660 摄像头模组</i>
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
    .pixel_format = PIXFORMAT_RGB565,   // 或 PIXFORMAT_JPEG
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
        // JPEG 模式下 buf 是完整 JPEG 文件（FFD8...FFD9）
        pico_camera_fb_return(fb);
    }
}
```

传感器控制：

```cpp
sensor_t *s = pico_camera_sensor_get();
if (s) {
    if (s->set_vflip)   s->set_vflip(s, 1);
    if (s->set_quality) s->set_quality(s, 10);
}
```

## 与 esp32-camera 的差异

- 数据引脚 `pin_d0..pin_d7` 必须是 **8 个连续 GPIO**（PIO `in pins` 指令的硬件限制），初始化时会校验
- 无 PSRAM 概念，帧缓冲在 SRAM（264KB）。RGB565 建议 ≤ VGA(640x480, 600KB 超 SRAM，实际 QVGA 较稳）；JPEG 缓冲按 `宽×高/4 + 8KB` 估算
- 暂不支持 `grab_mode`：`pico_camera_fb_get()` 是阻塞单帧语义（等价 `CAMERA_GRAB_WHEN_EMPTY`），每次调用同步等下一帧

## 示例

- `examples/CameraSerialInfo` — 探测传感器并打印信息
- `examples/CameraCaptureRGB565` — RGB565 抓帧
- `examples/CameraCaptureJPEG` — JPEG 抓帧（含 SOI/EOI 完整性检查）
- `examples/camera_render_to_tft` — 通过 TFT_eSPI 在 TFT 屏上实时预览（需先自行配置 TFT_eSPI）
- `examples/push_image_to_python` — 通过 USB 串口把帧推送给 Python(tkinter）上位机查看器

<p align="center">
  <img src="docs/image_capture.png" width="360" alt="push_image_to_python 上位机截图">
  <br>
  <i>push_image_to_python：上位机实时渲染 OV2640 JPEG 图像流</i>
</p>

## 文档

- 架构设计与路线图：`ARCHITECTURE.md`
- OV2640 寄存器速查：`docs/ov2640_registers.md`
- 早期驱动调试笔记：`docs/ov2640_dev_notes.md`

## 许可证

MIT
