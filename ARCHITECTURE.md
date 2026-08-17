# PicoCamera 架构设计

> 状态：阶段 0-4 已完成并硬件验证通过（OV2640/OV3660 × RGB565/JPEG）；库已发布至 GitHub。OV7670 已完成（RGB565,无 JPEG）并硬件验证通过。

## 0. 目标与已确认决策

- 发布形态：Arduino IDE 库管理器 + PlatformIO 均可直接安装（Arduino Library 1.5 布局，`src/` 结构）
- API 对齐 esp32-camera，前缀 `esp_` → `pico_`，其余命名/结构体尽量一致
- 库名：**PicoCamera**
- 引脚配置：**全运行时动态配置**，消除"必须改 .pio 源码"的短板（方案见 §3.1，已用现有编译产物验证可行性）
- 多传感器可扩展：sensor_t 函数指针 + 传感器探测表，新增型号只动 `src/sensors/`
- JPEG：利用 sensor 原生 JPEG 输出 + DMA 变长捕获

## 1. 仓库布局

```
PicoCamera/                        # 仓库根
├── library.properties             # Arduino IDE 元数据 (architectures=rp2040)
├── library.json                   # PlatformIO 元数据
├── keywords.txt                   # Arduino IDE 语法高亮
├── README.md
├── LICENSE
├── src/
│   ├── PicoCamera.h               # 对外唯一入口头文件，聚合下列全部
│   ├── pico_camera.h / .cpp       # pico_camera_init/deinit/fb_get/fb_return/sensor_get
│   ├── sensor.h                   # 公共类型（对齐 esp32-camera 的 sensor.h 子集）
│   ├── driver/                    # 硬件交互层（与具体 sensor 型号无关）
│   │   ├── sccb.h / .cpp          # I2C/SCCB 读写，抽象 8 位/16 位寄存器地址
│   │   ├── xclk.h / .cpp          # PWM 产生 XCLK，频率可配
│   │   ├── pio_capture.h / .cpp   # 运行时构建 PIO 捕获程序 + SM/DMA 捕获引擎
│   └── sensors/                   # 每型号一个模块，只做寄存器逻辑
│       ├── ov2640.h / .cpp
│       ├── ov2640_regs.h          # 现有寄存器定义迁移
│       ├── ov2640_settings.h      # 现有初始化序列表迁移
│       ├── ov3660.h / .cpp + regs/settings   # 阶段4 补充
│       └── ov7670.h / .cpp + regs            # 无 JPEG 型号,验证 support_jpeg 报错路径
├── scripts/
│   ├── check_style.sh           # astyle 格式化（沿用 arduino-pico 配置，去掉 remove-comment-prefix）
│   ├── ci_build.sh              # 全示例 × rp2040/rp2350/rp2350-riscv 严格编译检查
│   └── ci/                      # PlatformIO CI 工程（platformio.ini + extra_script.py）
└── examples/
    ├── CameraSerialInfo/          # 初始化 + 打印检测到的 sensor 信息
    ├── CameraCaptureRGB565/       # 抓帧并通过串口/自定义回调消费
    └── CameraCaptureJPEG/         # JPEG 抓帧示例（阶段3后）
```

迁移自旧结构的处置：

- `ov2640/`、`ov3660/`、`camera.hpp`、`camera_conf.h` 的内容迁移进上述结构后删除
- `camera.hpp` 中耦合业务的部分（`tft`、`lv_obj_*`、`board_type`）不进库，留在用户工程
- `examples/camera_render_to_tft` 保留为完整应用示例（清理 `.pio/`、`.vscode/` 后）

## 2. 公共 API 设计（对齐 esp32-camera）

`src/sensor.h`（以 esp32-camera 的 `driver/include/sensor.h` 为蓝本裁剪）：

- `pixformat_t`：`PIXFORMAT_RGB565`、`PIXFORMAT_JPEG`（后续可加 YUV422/GRAYSCALE）
- `framesize_t`：沿用现有枚举 + `resolution[]` 表（与 esp32 一致）
- `gainceiling_t`、`aspect_ratio_t`、`sensor_id_t`、`camera_sensor_info_t`：照搬 esp32 定义
- `sensor_t`：函数指针结构，照搬 esp32 的字段名。分阶段实现，未实现的指针置 `NULL`，调用返回 `PICO_ERR_NOT_SUPPORTED`。核心子集（阶段2 完成）：
  `reset / set_pixformat / set_framesize / set_brightness / set_contrast / set_saturation / set_sharpness / set_gainceiling / set_quality / set_colorbar / set_whitebal / set_gain_ctrl / set_exposure_ctrl / set_hmirror / set_vflip / set_special_effect / set_wb_mode / set_ae_level / set_aec_value / set_reg / get_reg / set_xclk`

`src/pico_camera.h`：

```c
typedef struct {
    int pin_pwdn, pin_reset, pin_xclk;
    int pin_sccb_sda, pin_sccb_scl;
    int pin_d0 ... pin_d7;          // 保持 esp32 字段名；要求物理连续，校验 pin_dN == pin_d0 + N
    int pin_vsync, pin_href, pin_pclk;
    int xclk_freq_hz;
    int sccb_i2c_port;              // 0/1 → i2c0/i2c1
    pixformat_t pixel_format;
    framesize_t frame_size;
    int jpeg_quality;               // 0-63，仅 JPEG 有效
    size_t fb_count;                // 帧缓冲数量
} camera_config_t;

typedef struct {
    uint8_t *buf;  size_t len;
    size_t width, height;
    pixformat_t format;
    struct timeval timestamp;
} camera_fb_t;

pico_camera_err_t pico_camera_init(const camera_config_t *config);
pico_camera_err_t pico_camera_deinit(void);
camera_fb_t *pico_camera_fb_get(void);
void pico_camera_fb_return(camera_fb_t *fb);
sensor_t *pico_camera_sensor_get(void);
```

错误码：`PICO_OK / PICO_FAIL / PICO_ERR_CAMERA_NOT_DETECTED / ...`，数值风格对齐 esp32 但不引入 ESP-IDF 依赖。

传感器探测：仿照 esp32 的 `camera_sensor[]` 表，每个型号注册 `{sccb_addr, pid, 8/16位地址模式, 检测+初始化入口}`。`pico_camera_init` 流程：XCLK → 复位 → 遍历探测表读 PID → 命中后调用该 sensor 的 `init(sensor_t*)` 填充函数指针 → `set_pixformat` / `set_framesize` 应用 config。OV2640（8 位寄存器地址）与 OV3660（16 位地址）的差异由 sccb 层吸收。

**能力检查（esp32-camera 对齐，随 OV7670 引入）**：探测命中后、分配缓冲前，按 `camera_sensor_info_t` 校验请求是否超出传感器能力——`pixel_format == JPEG && !support_jpeg` 时打日志并返回 `PICO_CAMERA_ERR_NOT_SUPPORTED`(esp32 对应 `ESP_ERR_NOT_SUPPORTED`);`frame_size > max_size` 时告警并钳位到最大值（不报错）。传感器自身的 `set_pixformat` 对不支持的格式仍返回 -1，作为第二道防线。

## 3. 关键技术方案

### 3.1 运行时 PIO 引脚（消除短板）——不用 pioasm，手工编码指令

验证依据：旧 `ov2640/image.pio.h` 的编译产物显示 `wait 0 gpio,2 = 0x2002`、`wait 1 gpio,2 = 0x2082`、`wait 1 gpio,3 = 0x2083`——引脚号在 bits 4:0，极性在 bit 7。pico-sdk 的 `hardware/pio_instructions.h` 提供 `pio_encode_wait_gpio(polarity, pin)` 等函数，参数直接是运行时变量。

方案：`pio_capture.cpp` 在运行时用 `pio_encode_*` 把原三个 PIO 程序（capture_sized 10 条、capture_single 6 条、frame_sync 3 条，指令序列照搬原 `image.pio`）组装成 `uint16_t` 数组，构造 `pio_program` 后 `pio_add_program`。数据引脚基址仍用 `sm_config_set_in_pins(pin_d0)`（本来就是运行时 API）。

收益：

- 全部引脚（含 VSYNC/HREF/PCLK）由 `camera_config_t` 决定，与 esp32 体验一致
- **库不再依赖 pioasm 构建步骤**——Arduino IDE 编译 .pio 需要额外工具链，手工编码后分发零障碍
- 旧 `image.pio` / `image.pio.h` / `pio_init.cpp` 删除

注意：RP2040 的 PIO 指令内存是只写的，"加载后再补丁"不可行，必须加载前构建好指令数组——本方案天然满足。SM 通过 `pio_claim_unused_sm` 自动分配，GPIO 功能号由 pio 实例决定（不再硬编码 `GPIO_FUNC_PIO1`）。

**实施期踩过的坑（已修复，勿回退）**：PIO 程序入口的 `wait 0 VSYNC / wait 1 VSYNC` 等待对只有在进入时 VSYNC 为低电平才等价于"等上升沿"；若在帧传输中途（VSYNC 高）武装捕获，两条 wait 会穿透，导致从帧中间开始截（JPEG 表现为丢帧头、只剩 FFD9 结尾的尾巴）。`pio_capture.cpp` 的 `wait_vsync_low()` 在武装前先把 VSYNC 等到低电平，两条捕获路径（定长/变长）都依赖它。同理每次捕获前必须 `pio_sm_restart()`，否则 SM 停在上次的捕获循环中间。

### 3.2 driver 层

- `sccb.cpp`：`sccb_init(port, sda, scl, freq)`、`sccb_write8/read8`、`sccb_write16/read16`、`sccb_write_list(...)`。sensor 只管表和逻辑
- `xclk.cpp`：`xclk_start(pin, freq_hz)` 用 PWM 分频计算 wrap/level（原 10.4MHz 逻辑泛化）
- `pio_capture.cpp`：§3.1 的程序构建 + SM 初始化 + DMA 捕获：
  - RGB565：长度已知，`transfer_count = w*h*2`，DMA 精确捕获（原有逻辑平移）
  - JPEG：长度未知，按上限缓冲捕获一个完整 VSYNC 帧窗口，完成后从尾部扫描 `0xFFD9` EOI 标记确定 `fb->len`
  - DMA 通道 `dma_claim_unused_channel`（沿用现状）

### 3.3 帧缓冲模型

- 启动按 `fb_count` malloc（32 字节对齐，沿用现有对齐逻辑），组成空闲队列
- 初版实现简单语义：`pico_camera_fb_get()` 取空闲 fb、触发一次阻塞捕获、返回；`fb_return` 归还。ISR 异步填充 + GRAB_LATEST 语义留到后续迭代（API 不变）

### 3.4 OV2640 驱动重构

- `ov2640_regs.h` / `ov2640_init.h`（设置表）原样迁入 `src/sensors/`
- `ov2640.cpp` 重写为 sensor_t 接口实现：`ov2640_detect()`、`ov2640_init_sensor(sensor_t*)`、各 `set_*` 函数；现有 vflip/hflip/brightness/framesize/output_size 逻辑平移，寄存器读写改走 sccb 层
- 颜色修正逻辑（`swap_red_green`、字节序）属于"数据管线"而非 sensor，放 driver 层或文档注明（先不进库核心，示例中演示）

### 3.5 JPEG 支持

- `PIXFORMAT_JPEG` + OV2640 JPEG 寄存器序列（现有 `ov2640_capture_jpeg` 雏形 + esp32-camera 的 `ov2640_settings.h` 参照）
- `jpeg_quality` → `set_quality`
- 变长捕获按 §3.2 扫描 EOI；缓冲上限按 `resolution[frame_size]` 估算

## 4. 分阶段实施

| 阶段 | 内容 | 验收 |
|---|---|---|
| 0 | 仓库骨架：目录结构、library.properties/library.json/keywords.txt/README/LICENSE | PlatformIO/Arduino 能识别为库 |
| 1 | driver 层（sccb/xclk/pio_capture）+ sensor.h/pico_camera 核心 + OV2640 RGB565 最小跑通 | 示例抓到一帧，尺寸/颜色正确 |
| 2 | sensor_t 完整化（vflip/hmirror/brightness 等），OV2640 寄存器控制全量平移 | 各 set_* 生效 |
| 3 | JPEG 捕获 | PIXFORMAT_JPEG 抓帧，FFD8...FFD9 完整 |
| 4 | OV3660（寄存器表移植自 esp32-camera 的 `ov3660_*`） | 探测 + 抓帧 |
| 5 | 三个 examples 完善、README（接线图/API 说明/与 esp32-camera 差异）、发布准备 | 全新环境 clone 可用 |
| 6 | OV7670（寄存器表移植自 esp32-camera 的 `ov7670_*`；无 JPEG，引入 support_jpeg 能力检查） | 探测 + RGB565 抓帧（已硬件验证） |

## 5. 明确不做（本期）

- 不做 ISP 级格式转换（YUV→RGB 等），后续视需求加 `conversions/`
- 不做双缓冲异步流水线（GRAB_LATEST），API 预留 `fb_count` 即可
- 不改写 esp32-camera 的传感器寄存器表，直接移植，避免引入新 bug
