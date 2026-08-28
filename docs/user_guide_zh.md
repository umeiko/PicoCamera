# PicoCamera 使用指南

本指南介绍如何配置、拉起摄像头并获取帧数据。完整的符号定义见
`pico_camera.h` 和 `sensor.h` 的 API 页面；可运行的示例见
[examples](https://github.com/umeiko/PicoCamera/tree/main/examples)。

## 1. 配置 camera_config_t

驱动所需的全部信息在初始化时通过 `camera_config_t` 一次性传入，
没有任何板级硬编码。

| 字段 | 含义 | 说明 |
|------|------|------|
| `pin_pwdn` | 摄像头电源控制 GPIO | 未接填 `-1` |
| `pin_reset` | 摄像头复位 GPIO | 未接填 `-1` |
| `pin_xclk` | 主时钟（XCLK）输出 GPIO | 由 PWM 切片产生 |
| `pin_sccb_sda` / `pin_sccb_scl` | SCCB（I2C）数据/时钟 GPIO | 用于探测和配置传感器。引脚硬复用：SDA 偶数脚、SCL 奇数脚，且两脚必须路由到 `sccb_i2c_port`（`(pin / 2) % 2 == 端口号`）。`pin_sccb_sda = -1` 可复用已初始化的 I2C 总线（与 esp32-camera 一致，此时忽略 `pin_sccb_scl`）——见下方共享总线说明 |
| `pin_d0` .. `pin_d7` | 8 位并口数据线 | **必须是 8 个连续 GPIO**（`pin_dN == pin_d0 + N`，PIO 硬件限制，初始化时校验） |
| `pin_vsync` / `pin_href` / `pin_pclk` | 帧同步 / 行同步 / 像素时钟输入 | GPIO 任选 |
| `xclk_freq_hz` | XCLK 频率（Hz） | `0` 表示默认 10 MHz；传感器一般接受 10–24 MHz |
| `sccb_i2c_port` | SCCB 使用的 RP2040 I2C 外设号 | `0` 或 `1`；共享总线模式（`pin_sccb_sda = -1`）下选择复用哪条已初始化的总线 |
| `pixel_format` | `PIXFORMAT_RGB565` 或 `PIXFORMAT_JPEG` | JPEG 要求传感器自带编码器（OV2640/OV3660 支持，OV7670/GC2145 不支持） |
| `frame_size` | `FRAMESIZE_*` 枚举 | 超过传感器上限时不报错，告警并钳位到最大值 |
| `jpeg_quality` | 0–63，**越小画质越高** | 仅 JPEG 模式有效 |
| `fb_count` | 帧缓冲数量 | 默认缓冲位于 SRAM（共 264 KB），也可用 PSRAM，见下方内存估算 |
| `fb_location` | 帧缓冲存放位置 | `PICO_CAMERA_FB_AUTO`（默认）：有 PSRAM 用 PSRAM，否则 SRAM；`PICO_CAMERA_FB_IN_PSRAM`：只用 PSRAM，不可用则 init 失败（与 esp32-camera 一致）；`PICO_CAMERA_FB_IN_SRAM`：只用片上 SRAM |

可选分辨率（见 `framesize_t`）：96x96、QQVGA 160x120、QCIF 176x144、
HQVGA 240x176、240x240、QVGA 320x240、CIF 400x296、HVGA 480x320、
VGA 640x480、SVGA 800x600、XGA 1024x768、HD 1280x720、SXGA 1280x1024、
UXGA 1600x1200。

内存估算（RP2040 只有 264 KB SRAM，无 PSRAM；带 PSRAM 芯片的 RP2350 板
在 core 启用 PSRAM 支持后可把缓冲放进 PSRAM——见上方 `fb_location`）：

- RGB565 每缓冲占 `宽 × 高 × 2` 字节——SRAM 下实际建议不超过 QVGA
  （153 KB）；VGA（600 KB）放不下，但在 RP2350 上可从 PSRAM 分配。
- JPEG 缓冲按 `宽 × 高 / 4 + 8 KB` 分配，可覆盖常规画面；极端噪点
  画面可能超出。

共享 SCCB 总线（与 esp32-camera 一致）：若想把传感器挂在你已在用的
I2C 总线上，先自行初始化该总线（Arduino 下 `Wire.begin()` /
`Wire1.begin()`，裸 Pico SDK 下 `i2c_init()`），然后设
`config.pin_sccb_sda = -1`、`config.sccb_i2c_port = 0 或 1`。库会跳过
所有引脚/总线初始化，且 `pico_camera_deinit()` 不会动共享总线。

## 2. 拉起摄像头与取帧流程

生命周期与 esp32-camera 保持一致：

1. **填好 `camera_config_t`**，调用 `pico_camera_init(&config)`。
   该函数会通过 SCCB 探测传感器（无应答返回
   `PICO_CAMERA_ERR_NOT_DETECTED`）、校验引脚布局、对无编码器的
   传感器拒绝 JPEG（`PICO_CAMERA_ERR_NOT_SUPPORTED`）、分配帧缓冲并
   启动 PIO + DMA 采集引擎。只能初始化一次；要换配置需先
   `pico_camera_deinit()`。
2. **取帧**：`camera_fb_t *fb = pico_camera_fb_get();`——阻塞直到
   一帧完整 DMA 进缓冲；出错或没有空闲缓冲时返回 `NULL`。
3. **使用** `fb->buf` / `fb->len` / `fb->width` / `fb->height`
   （见下节），用完**必须** `pico_camera_fb_return(fb)` 归还缓冲。
   缓冲是复用的，不归还会耗尽全部 `fb_count` 个缓冲，驱动将无缓冲可用。
4. **关闭**（可选）：`pico_camera_deinit()` 停止 PIO/DMA、释放
   SCCB 总线并释放所有缓冲。

取到帧之后怎么用由应用决定：推上 TFT 屏（`camera_render_to_tft`
示例）、经 USB 串口发给上位机（`push_image_to_python` 示例）、或者
交给自己的处理流水线（如视觉/推理任务）。取帧 → 处理 → 归还缓冲，
驱动要求的循环就这么多。

`pico_camera_init()` / `pico_camera_deinit()` 的错误码：

| 错误码 | 含义 |
|--------|------|
| `PICO_CAMERA_OK` (0) | 成功 |
| `PICO_CAMERA_ERR_NOT_DETECTED` | SCCB 上无传感器应答 |
| `PICO_CAMERA_ERR_NOT_SUPPORTED` | 如对 OV7670 请求 JPEG |
| `PICO_CAMERA_ERR_INVALID_ARG` | 配置非法（如数据引脚不连续） |
| `PICO_CAMERA_ERR_INVALID_STATE` | 重复初始化，或未初始化就 deinit |
| `PICO_CAMERA_ERR_NO_MEM` | 帧缓冲分配失败 |
| `PICO_CAMERA_ERR_TIMEOUT` | 采集超时 |
| `PICO_CAMERA_ERR_FAILED_TO_SET_FRAME_SIZE` / `..._SET_OUT_FORMAT` | 传感器拒绝该尺寸/格式 |

## 3. camera_fb_t：帧缓冲

```cpp
typedef struct {
    uint8_t *buf;        // 像素数据
    size_t   len;        // buf 中有效字节数
    size_t   width;      // 宽（像素）
    size_t   height;     // 高（像素）
    pixformat_t format;  // PIXFORMAT_RGB565 或 PIXFORMAT_JPEG
    struct timeval timestamp;  // 采集时刻（距开机）
} camera_fb_t;
```

- **RGB565**：`len == 宽 × 高 × 2`，每 2 字节一个 16 位像素。
- **JPEG**：`buf` 是一个完整独立的 JPEG 文件——以 `0xFF 0xD8`
  （SOI）开头、`0xFF 0xD9`（EOI）结尾，`len` 逐帧变化。可直接写
  文件或发网络。

操作帧缓冲的函数：

| 函数 | 作用 |
|------|------|
| `pico_camera_fb_get()` | 借出一个装有新帧的缓冲（阻塞） |
| `pico_camera_fb_return(fb)` | 归还缓冲以便复用——必须成对调用 |

`fb_count > 1` 时，DMA 引擎可以在你处理上一帧的同时填充下一个缓冲；
`fb_count == 1` 时，每次 `fb_get()` 都要等上一帧归还后再采下一帧。

## 4. sensor_t：运行时传感器控制

`pico_camera_sensor_get()` 返回已探测传感器的控制结构（初始化前返回
`NULL`）。所有控制项都是函数指针；**每种传感器只实现其中一个子集，
调用前务必判空**：

```cpp
sensor_t *s = pico_camera_sensor_get();
if (s && s->set_vflip) s->set_vflip(s, 1);
```

可用操作一览：

| 函数 | 作用 |
|------|------|
| `set_pixformat(fmt)` | 运行时切换 RGB565 / JPEG |
| `set_framesize(size)` | 运行时改分辨率 |
| `set_brightness(level)` / `set_contrast(level)` / `set_saturation(level)` / `set_sharpness(level)` | 画质调节，小整数档位（OV2640 上亮度/对比度/饱和度取 −2…2） |
| `set_gainceiling(gc)` | 自动增益上限，`GAINCEILING_2X` … `GAINCEILING_128X` |
| `set_quality(q)` | JPEG 质量 0–63，越小越好 |
| `set_colorbar(on)` | 彩条测试图 |
| `set_whitebal(on)` / `set_wb_mode(mode)` | 自动白平衡开关 / 白平衡预设模式 |
| `set_gain_ctrl(on)` / `set_exposure_ctrl(on)` | 自动增益 / 自动曝光开关 |
| `set_ae_level(level)` / `set_aec_value(value)` | 自动曝光目标档 / 手动曝光值 |
| `set_hmirror(on)` / `set_vflip(on)` | 水平镜像 / 垂直翻转 |
| `set_special_effect(effect)` | 特效序号（灰度、负片等） |
| `set_reg(reg, mask, value)` / `get_reg(reg, mask)` | 原始寄存器读写，`mask` 选择要操作的位。上面没封装的功能用它兜底 |
| `set_xclk(timer, xclk)` | 运行时改 XCLK 频率（RP2040 上 `timer` 参数不用） |
| `reset()` | 传感器软复位 |

各传感器支持情况（截至 v0.4.0）：

| 操作 | OV2640 | OV3660 | OV7670 | GC2145 |
|------|:------:|:------:|:------:|:------:|
| pixformat / framesize | ✅ | ✅ | ✅ | ✅（仅 RGB565） |
| hmirror / vflip | ✅ | ✅ | ✅ | ✅ |
| brightness / contrast / saturation / sharpness | ✅ | — | — | — |
| gainceiling / quality | ✅ | — | —（无 JPEG） | —（无 JPEG） |
| colorbar | ✅ | — | ✅ | — |
| whitebal / wb_mode | ✅ | — | 仅 whitebal | — |
| gain_ctrl / exposure_ctrl | ✅ | — | ✅ | — |
| ae_level / aec_value / special_effect | ✅ | — | — | — |
| set_reg / get_reg | ✅ | ✅ | ✅ | ✅ |

## 5. 传感器信息

`pico_camera_sensor_info_get()` 返回已探测型号的静态信息：`name`、
`sccb_addr`、`pid`、`max_size`（支持的最大 `framesize_t`）和
`support_jpeg`。可用于在初始化前后按传感器能力动态调整配置。

## 6. 在 Pico SDK 工程中使用（无 Arduino）

本库是纯 Pico SDK 代码——`src/` 里没有任何 Arduino 依赖——裸 CMake
工程可以直接使用。要求 **Pico SDK ≥ 1.5.0**（用到 `pio_encode_*`
API）。

在你的 `CMakeLists.txt` 中：

```cmake
include(pico_sdk_import.cmake)   # SDK 标准样板
project(my_app C CXX ASM)
pico_sdk_init()

add_subdirectory(path/to/PicoCamera)   # 本仓库

add_executable(my_app main.c)
target_link_libraries(my_app pico_stdlib pico_camera)

pico_enable_stdio_usb(my_app 1)        # 驱动日志走 printf
pico_add_extra_outputs(my_app)
```

或者用 FetchContent：

```cmake
include(FetchContent)
FetchContent_Declare(PicoCamera
    GIT_REPOSITORY https://github.com/umeiko/PicoCamera.git
    GIT_TAG main)
FetchContent_MakeAvailable(PicoCamera)
target_link_libraries(my_app pico_stdlib pico_camera)
```

代码里 `#include "PicoCamera.h"`，API 用法与第 1–5 节完全一致——和
Arduino sketch 唯一的区别是你需要自己调用 `stdio_init_all()`。驱动
日志通过 `printf` 输出，想看日志请启用 USB 或 UART stdio。

完整可编译的工程见
[examples/pico_sdk_capture](https://github.com/umeiko/PicoCamera/tree/main/examples/pico_sdk_capture)；
每次 `src/` 有改动，CI 都会用最新 Pico SDK 编译该示例，保证你看到的
代码一定能通过编译。
