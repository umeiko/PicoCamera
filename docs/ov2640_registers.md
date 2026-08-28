# OV2640 寄存器参考

> 本文档由 `ov2640/ov2640_regs.h`（寄存器地址与位域宏）和 `ov2640/ov2640_init.h`（初始化序列表）归纳生成，仅供查阅。
> **权威定义以源码为准**；部分用途说明依据宏名、位域宏及 OV2640 通用知识推断，个别保留寄存器（裸十六进制地址）仅按常见用法标注。

## 1. 寄存器总表

OV2640 的寄存器空间通过 `BANK_SEL`（0xFF）切换：

- **DSP bank**（`BANK_SEL = 0x00`）：图像信号处理 / DVP 输出 / 内部 MCU 控制寄存器。
- **Sensor bank**（`BANK_SEL = 0x01`）：Sensor 阵列、模拟前端、时序与自动曝光/增益控制寄存器。

### 1.1 DSP bank（BANK_SEL = 0x00）

| 宏名 | 地址 | 用途 |
|------|------|------|
| `R_BYPASS` | 0x05 | DSP 旁路选择：启用 DSP（0x00）或旁路 DSP 直接输出（0x01） |
| `QS` | 0x44 | JPEG 量化因子 / 缩放系数（Quantization Scale） |
| `CTRLI` | 0x50 | IPU（图像预处理单元）控制：行/像素加倍（LP_DP）、四舍五入（ROUND） |
| `HSIZE` | 0x51 | 输出图像宽度低 8 位（窗口宽度 / 4） |
| `VSIZE` | 0x52 | 输出图像高度低 8 位（窗口高度 / 4） |
| `XOFFL` | 0x53 | 图像窗口水平偏移量低 8 位 |
| `YOFFL` | 0x54 | 图像窗口垂直偏移量低 8 位 |
| `VHYX` | 0x55 | 高/宽/偏移量高位拼接寄存器（HSIZE/VSIZE/XOFFL/YOFFL 的第 8 位及 TEST 高位） |
| `DPRP` | 0x56 | DP 相关控制（数据通路控制） |
| `TEST` | 0x57 | 窗口尺寸最高位（HSIZE/VSIZE 的 bit[9:8]） |
| `ZMOW` | 0x5A | Zoom 模式输出宽度（OUTW[7:2]，单位 4 像素） |
| `ZMOH` | 0x5B | Zoom 模式输出高度（OUTH[7:2]） |
| `ZMHH` | 0x5C | Zoom 输出尺寸高位及 zoom 速度控制 |
| `BPADDR` | 0x7C | DSP 间接寻址：地址寄存器（配合 BPDATA 访问内部微码参数区） |
| `BPDATA` | 0x7D | DSP 间接寻址：数据寄存器 |
| `CTRL2` | 0x86 | DSP 使能 2：DCW（缩放）、SDE、UV_ADJ、UV_AVG、CMX（颜色矩阵）使能 |
| `CTRL3` | 0x87 | DSP 使能 3：BPC（黑点校正）、WPC（白点校正）使能 |
| `SIZEL` | 0x8C | 输出尺寸低位补充（HSIZE/VSIZE 低 2 位、offset 高位拼接） |
| `HSIZE8` | 0xC0 | 输出宽度（8 位，单位 8 像素），分辨率模式设置用 |
| `VSIZE8` | 0xC1 | 输出高度（8 位，单位 8 像素） |
| `CTRL0` | 0xC2 | DSP 使能 0：AEC 使能/选择、统计选择、场序、YUV/RGB/RAW 使能 |
| `CTRL1` | 0xC3 | DSP 使能 1：AWB、去噪、镜头校正等模块使能（init 中常写 0xFD） |
| `R_DVP_SP` | 0xD3 | DVP 输出时钟分频 / PCLK 速度设置，支持自动模式 |
| `IMAGE_MODE` | 0xDA | 输出图像格式：YUV422 / RGB565 / RAW10 / JPEG、字节序、HREF/VSYNC 模式 |
| `RESET` | 0xE0 | 模块复位：MCU / SCCB / JPEG / DVP / IPU / CIF |
| `MS_SP` | 0xF0 | SCCB 主设备速度（SCCB Master Speed） |
| `SS_ID` | 0xF7 | SCCB 从设备 ID（与 SS_CTRL 同地址，读写方向不同） |
| `SS_CTRL` | 0xF7 | SCCB 从设备控制（与 SS_ID 同地址） |
| `MC_BIST` | 0xF9 | MCU BIST / 启动控制：复位、Boot ROM / 12KB / 512KB 程序存储选择、启动 |
| `MC_AL` | 0xFA | MCU 程序地址低字节 |
| `MC_AH` | 0xFB | MCU 程序地址高字节 |
| `MC_D` | 0xFC | MCU 程序/数据口（下载微码用） |
| `P_CMD` | 0xFD | 协议命令寄存器 |
| `P_STATUS` | 0xFE | 协议状态寄存器 |
| `BANK_SEL` | 0xFF | Bank 选择：0x00 = DSP，0x01 = Sensor |

### 1.2 Sensor bank（BANK_SEL = 0x01）

| 宏名 | 地址 | 用途 |
|------|------|------|
| `GAIN` | 0x00 | AGC 增益值（手动增益 / 读出自动增益） |
| `COM1` | 0x03 | 公共控制 1：AEC 低 2 位、帧率/窗口模式相关位 |
| `REG04` | 0x04 | 镜像/翻转与 HREF/VREF 使能（默认 0x28） |
| `REG08` | 0x08 | 曝光/增益微调寄存器 |
| `COM2` | 0x09 | 公共控制 2：待机、输出驱动能力（1x–4x） |
| `REG_PID` | 0x0A | 产品 ID 高字节（OV2640 读出 0x26） |
| `REG_VER` | 0x0B | 产品 ID 低字节 / 版本（读出 0x42） |
| `COM3` | 0x0C | 公共控制 3：帧带（banding）50/60Hz/自动等（默认 0x38） |
| `COM4` | 0x0D | 公共控制 4：时钟/曝光相关控制 |
| `AEC` | 0x10 | 手动曝光值（AEC[15:8]，与 COM1/REG45 拼成完整曝光值） |
| `CLKRC` | 0x11 | 内部时钟分频与倍频（2X）控制 |
| `COM7` | 0x12 | 公共控制 7：系统复位（SRST）、分辨率模式（UXGA/SVGA/CIF）、Zoom、彩条测试 |
| `COM8` | 0x13 | 公共控制 8：AEC/AGC/带通滤波等自动控制开关 |
| `COM9` | 0x14 | AGC 增益上限（2x–128x） |
| `COM10` | 0x15 | HREF/HSYNC/PCLK/VSYNC 极性与输出模式 |
| `HSTART` | 0x17 | 水平窗口起始位置高 8 位 |
| `HSTOP` | 0x18 | 水平窗口结束位置高 8 位 |
| `VSTART` | 0x19 | 垂直窗口起始位置高 8 位 |
| `VSTOP` | 0x1A | 垂直窗口结束位置高 8 位 |
| `REG_MIDH` | 0x1C | 厂商 ID 高字节（读出 0x7F） |
| `REG_MIDL` | 0x1D | 厂商 ID 低字节（读出 0xA2） |
| `AEW` | 0x24 | AGC/AEC 稳定区上限（AEC 目标窗口上限） |
| `AEB` | 0x25 | AGC/AEC 稳定区下限 |
| `VV` | 0x26 | AGC 模式快速/慢速阈值（高 4 位高阈、低 4 位低阈） |
| `REG2A` | 0x2A | 行频/帧频高位（行时间高 2 位等） |
| `FRARL` | 0x2B | 帧率调整低位（50/60Hz 帧带微调） |
| `ADDVSL` | 0x2D | 附加垂直同步行数低 8 位（降帧率用） |
| `ADDVSH` | 0x2E | 附加垂直同步行数高 8 位 |
| `YAVG` | 0x2F | 平均亮度输出（Y average） |
| `HSDY` | 0x30 | 模拟行起始延迟（HREF 起点微调） |
| `HEDY` | 0x31 | 模拟行结束延迟 |
| `REG32` | 0x32 | 像素时钟/分辨率模式相关（UXGA 0x36 / SVGA 0x09 / CIF 0x89） |
| `ARCOM2` | 0x34 | 模拟参考控制（Zoom 模式相关） |
| `REG45` | 0x45 | AEC 最高 6 位（曝光值高位拼接） |
| `FLL` | 0x46 | 帧长调整低 8 位（Frame Length LSB） |
| `FLH` | 0x47 | 帧长调整高位 |
| `COM19` | 0x48 | 公共控制 19（Zoom 模式行频相关） |
| `ZOOMS` | 0x49 | Zoom 模式窗口垂直起始 |
| `COM22` | 0x4B | 公共控制 22（闪光灯/频闪控制等） |
| `COM25` | 0x4E | 公共控制 25（50/60Hz 检测相关） |
| `BD50` | 0x4F | 50Hz 帧带（banding）值 |
| `BD60` | 0x50 | 60Hz 帧带（banding）值 |
| `REG5D` | 0x5D | AVDD / 模拟电源相关控制 |
| `REG5E` | 0x5E | 模拟控制 |
| `REG5F` | 0x5F | 模拟控制 |
| `REG60` | 0x60 | 模拟控制 |
| `HISTO_LOW` | 0x61 | 直方图算法低阈值 |
| `HISTO_HIGH` | 0x62 | 直方图算法高阈值 |

## 2. 关键位域

### 2.1 `COM7`（Sensor bank 0x12）——系统控制/分辨率

| 宏 | 值 | 含义 |
|----|-----|------|
| `COM7_SRST` | 0x80 | SCCB 软复位（写 1 后所有寄存器恢复默认） |
| `COM7_RES_UXGA` | 0x00 | 分辨率模式 UXGA（1600×1200 全分辨率） |
| `COM7_RES_SVGA` | 0x40 | 分辨率模式 SVGA（800×600，2×2 子采样） |
| `COM7_RES_CIF` | 0x20 | 分辨率模式 CIF（352×288，4×4 子采样） |
| `COM7_ZOOM_EN` | 0x04 | 使能 Zoom 模式 |
| `COM7_COLOR_BAR` | 0x02 | 使能彩条测试图输出 |

### 2.2 `REG04`（0x04）——镜像翻转

| 宏 | 值 | 含义 |
|----|-----|------|
| `REG04_DEFAULT` | 0x28 | 默认值（保留位必须保留） |
| `REG04_HFLIP_IMG` | 0x80 | 水平镜像 |
| `REG04_VFLIP_IMG` | 0x40 | 垂直翻转 |
| `REG04_VREF_EN` | 0x10 | VREF 使能（VSYNC 输出控制） |
| `REG04_HREF_EN` | 0x08 | HREF 使能 |
| `REG04_SET(x)` | — | 组合宏：默认值 | 选项位 |

### 2.3 `CLKRC`（0x11）——内部时钟

| 宏 | 值 | 含义 |
|----|-----|------|
| `CLKRC_DIV(x)` | ((x)-1)&0x1F | 内部时钟分频系数（bit[5:0]），外部输入 / x |
| `CLKRC_2X` | 0x80 | 倍频使能（内部时钟 ×2） |
| `CLKRC_2X_UXGA` | 0x8B | UXGA 预设：2X + DIV(12) |
| `CLKRC_2X_SVGA` | 0x87 | SVGA 预设：2X + DIV(8) |
| `CLKRC_2X_CIF` | 0x80 | CIF 预设：2X + DIV(0) |

### 2.4 `CTRL0` / `CTRL1` / `CTRL2` / `CTRL3`（DSP 模块使能）

`CTRL0`（0xC2）：

| 宏 | 值 | 含义 |
|----|-----|------|
| `CTRL0_AEC_EN` | 0x80 | DSP 侧 AEC 使能 |
| `CTRL0_AEC_SEL` | 0x40 | AEC 来源选择 |
| `CTRL0_STAT_SEL` | 0x20 | 统计模块选择 |
| `CTRL0_VFIRST` | 0x10 | 帧序：垂直场优先 |
| `CTRL0_YUV422` | 0x08 | YUV422 输出格式选择 |
| `CTRL0_YUV_EN` | 0x04 | YUV 通路使能 |
| `CTRL0_RGB_EN` | 0x02 | RGB 通路使能 |
| `CTRL0_RAW_EN` | 0x01 | RAW 通路使能 |

`CTRL1`（0xC3）：定义了 `CTRL1_AWB`（0x08）自动白平衡使能；其余位在 init 表中以整体值（如 0xFD）写入。

`CTRL2`（0x86）：

| 宏 | 值 | 含义 |
|----|-----|------|
| `CTRL2_DCW_EN` | 0x20 | DCW（数字缩放 / down-sample）使能 |
| `CTRL2_SDE_EN` | 0x10 | SDE（特殊数字效果：对比度/饱和度/色相）使能 |
| `CTRL2_UV_ADJ_EN` | 0x08 | UV 调整使能 |
| `CTRL2_UV_AVG_EN` | 0x04 | UV 平均使能 |
| `CTRL2_CMX_EN` | 0x01 | 颜色矩阵（Color Matrix）使能 |

`CTRL3`（0x87）：`CTRL3_BPC_EN`（0x80）黑点校正、`CTRL3_WPC_EN`（0x40）白点校正。

### 2.5 `R_BYPASS` / `R_DVP_SP` / `IMAGE_MODE` / `RESET`

| 寄存器 | 宏 | 含义 |
|--------|----|------|
| `R_BYPASS` (0x05) | `R_BYPASS_DSP_EN` (0x00) / `R_BYPASS_DSP_BYPAS` (0x01) | 启用 DSP 处理 / 旁路 DSP |
| `R_DVP_SP` (0xD3) | `R_DVP_SP_DIV_YUV(x)` / `R_DVP_SP_DIV_RAW(x)` | YUV/RAW 模式下 PCLK 分频（bit[6:0]） |
| | `R_DVP_SP_AUTO_MODE` (0x80) | PCLK 自动模式（随分辨率自动选择速度） |
| `IMAGE_MODE` (0xDA) | `IMAGE_MODE_YUV422` (0x00) / `IMAGE_MODE_RAW10` (0x04) / `IMAGE_MODE_RGB565` (0x08) | 输出格式选择（bit[4:2]） |
| | `IMAGE_MODE_JPEG_EN` (0x10) | JPEG 压缩输出使能 |
| | `IMAGE_MODE_Y8_DVP_EN` (0x40) | Y8 灰度 + DVP 输出 |
| | `IMAGE_MODE_HREF_VSYNC` (0x02) | HREF 引脚输出 VSYNC |
| | `IMAGE_MODE_LBYTE_FIRST` (0x01) | 低字节先行（RGB565 字节序） |
| `RESET` (0xE0) | `RESET_MICROC` 0x40 / `RESET_SCCB` 0x20 / `RESET_JPEG` 0x10 / `RESET_DVP` 0x04 / `RESET_IPU` 0x02 / `RESET_CIF` 0x01 | 各子模块复位（写 1 复位，写 0 释放） |

### 2.6 其他常用组合宏

| 寄存器 | 宏 | 含义 |
|--------|----|------|
| `COM2` (0x09) | `COM2_STDBY` 0x10；`COM2_OUT_DRIVE_1x..4x` 0x00–0x03 | 待机；输出驱动能力选择 |
| `COM3` (0x0C) | `COM3_DEFAULT` 0x38；`COM3_BAND_50Hz/60Hz/AUTO`；`COM3_BAND_SET(x)` | 帧带频率设置 |
| `COM8` (0x13) | `COM8_DEFAULT` 0xC0；`COM8_BNDF_EN` 0x20；`COM8_AGC_EN` 0x04；`COM8_AEC_EN` 0x01；`COM8_SET(x)` | AGC/AEC/带通滤波开关 |
| `COM9` (0x14) | `COM9_DEFAULT` 0x08；`COM9_AGC_GAIN_2x..128x` 0x00–0x06；`COM9_AGC_SET(x)` | AGC 增益上限（写入 bit[7:5]） |
| `COM10` (0x15) | `COM10_HREF_EN`/`COM10_HSYNC_EN`/`COM10_PCLK_FREE`/`COM10_PCLK_EDGE`/`COM10_HREF_NEG`/`COM10_VSYNC_NEG`/`COM10_HSYNC_NEG` | 同步信号模式与极性 |
| `VV` (0x26) | `VV_AGC_TH_SET(h,l)` | AGC 高/低阈值（各 4 位） |
| `REG32` (0x32) | `REG32_UXGA` 0x36 / `REG32_SVGA` 0x09 / `REG32_CIF` 0x89 | 分辨率模式对应值 |
| `MC_BIST` (0xF9) | `MC_BIST_RESET` 0x80；`MC_BIST_BOOT_ROM_SEL` 0x40；`MC_BIST_12KB_SEL` 0x20；`MC_BIST_512KB_SEL` 0x08；`MC_BIST_LAUNCH` 0x01 等 | MCU 复位/程序存储选择/启动 |

## 3. 初始化序列说明

所有表均为 `const uint8_t xxx[][2]` 的 `{寄存器, 值}` 对，以 `ENDMARKER`（`{0xff, 0xff}`）结束；条目数统计均**不含**结束标记。驱动按顺序逐条 SCCB 写入即可。

### 3.1 `ov2640_settings_cif`（162 条）——上电全量初始化

OV2640 上电后的完整初始化（以 CIF 模式收尾）。大致阶段：

1. **DSP 预配置**：切 DSP bank，写 0x2C/0x2E 等保留寄存器。
2. **Sensor 基础时钟与输出**：切 Sensor bank，`CLKRC = CLKRC_2X|CLKRC_DIV(1)`（2 倍频不分频，内部 12MHz）；`COM2` 设 3x 输出驱动；`COM3` 自动帧带检测；`REG04` 默认值。
3. **AGC/AEC 自动曝光默认**：`COM8` 开 AGC/AEC/带通滤波，`COM9` 设 AGC 上限 8x，`AEW`/`AEB`/`VV` 设稳定区与阈值，`HISTO_LOW`/`HISTO_HIGH` 直方图参数，`BD50`/`BD60` 帧带值，并写入大量厂商未文档化的模拟调优寄存器（0x16/0x22/0x35/0x39 等）。
4. **CIF 窗口**：`COM7 = COM7_RES_CIF`，`HSTART/HSTOP/VSTART/VSTOP` 输出窗口，`REG32 = REG32_CIF`，并覆写一组随分辨率变化的寄存器（BD50/BD60、0x6D、0x3D 等）。
5. **DSP/MCU 复位与启动**：切回 DSP bank，`MC_BIST = RESET|BOOT_ROM_SEL` 复位 MCU 并选 Boot ROM，`RESET = JPEG|DVP` 复位 JPEG/DVP 模块。
6. **DSP 参数区下载**：通过 `BPADDR`/`BPDATA` 间接寻址写内部微码参数；0x90–0x97 区间写入大量量化/色彩查找表类数据。
7. **DSP 模块使能收尾**：`CTRL3 = WPC_EN|0x10`、`CTRL1 = 0xFD`（开 AWB 等）、`R_DVP_SP = AUTO_MODE`、`IMAGE_MODE = 0x00`（YUV422）；最后 `RESET = 0x00` 释放复位、`R_BYPASS = DSP_EN` 启用 DSP。

### 3.2 `ov2640_settings_to_cif`（47 条）——切换到 CIF 分辨率

1. 切 Sensor bank，`COM7 = COM7_RES_CIF`。
2. **Sensor 输出窗口**：`COM1`、`REG32_CIF`、`HSTART/HSTOP/VSTART/VSTOP`（CIF 窗口 0x11–0x43 / 0x00–0x25）。
3. **配套寄存器组**：BD50/BD60、ARCOM2、COM4 等随分辨率调整的一整组寄存器。
4. 切 DSP bank，`RESET = RESET_DVP` 复位 DVP。
5. **DSP 输出尺寸**：`HSIZE8 = 0x32`、`VSIZE8 = 0x25`、`SIZEL`（即 400×296 → CIF 352×288 档位）；随后 `HSIZE/VSIZE/XOFFL/YOFFL/VHYX/TEST` 设置图像窗口（≥ 输出尺寸）。
6. **使能缩放通路**：`CTRL2 = DCW_EN|0x1D`、`CTRLI = LP_DP`（行/像素加倍）。

### 3.3 `ov2640_settings_to_svga`（48 条）——切换到 SVGA 分辨率

流程与 to_cif 相同，差异在数值：

- `COM7 = COM7_RES_SVGA`，`REG32 = REG32_SVGA`；Sensor 窗口 `VSTOP = 0x4B`（CIF 为 0x25）。
- DSP 尺寸 `HSIZE8 = 0x64`、`VSIZE8 = 0x4B`（800×600）；图像窗口 `HSIZE = 0xC8`、`VSIZE = 0x96`。
- 寄存器组中多了 `{0x42, 0x03}`；`CTRL2`/`CTRLI` 收尾与 CIF 一致。

### 3.4 `ov2640_settings_to_uxga`（46 条）——切换到 UXGA 分辨率

- `COM7 = COM7_RES_UXGA`（0x00），`REG32 = REG32_UXGA`，`COM1 = 0x0F`；Sensor 窗口更大：`HSTOP = 0x75`、`VSTART = 0x01`、`VSTOP = 0x97`。
- 配套寄存器组与低分辨率差异较大（如 `COM4 = 0xB7`、`ARCOM2 = 0xA0`、`{0x42, 0x83}`、`BD50 = 0xBB`、`BD60 = 0x9C`），因 UXGA 为全分辨率无子采样。
- DSP 尺寸 `HSIZE8 = 0xC8`、`VSIZE8 = 0x96`（1600×1200）；图像窗口 `HSIZE = 0x90`、`VSIZE = 0x2C`，且 `VHYX = 0x88`——H/V 尺寸高位在 `VHYX` 中置位（UXGA 超过 8 位可表示范围）。
- 收尾 `CTRL2 = DCW_EN|0x1D`，`CTRLI = 0x00`（UXGA 不需要 LP_DP 加倍）。

### 3.5 `ov2640_settings_rgb565`（6 条）——切换 RGB565 输出

切 DSP bank → `RESET = RESET_DVP` → `IMAGE_MODE = IMAGE_MODE_RGB565`（RGB565 格式、高字节先行，与 esp32-camera 线上字节序一致）→ 两个保留寄存器（0xD7/0xE1）→ `RESET = 0x00` 释放复位。

### 3.6 `ov2640_settings_jpeg`（13 条）——切换 JPEG 输出

切 DSP bank → `RESET = RESET_JPEG|RESET_DVP` → `IMAGE_MODE = JPEG_EN | HREF_VSYNC`（JPEG 使能，HREF 引脚改为 VSYNC）→ 一组 JPEG 通路保留寄存器（0xD7/0xE1/0xE5/0xD9/0xDF/0x33/0x3C/0xEB/0xDD）→ `RESET = 0x00` 释放复位。

### 3.7 `brightness_regs`（6 行 × 5 列）——亮度调节表

非 `{reg,value}` 流，而是**查表结构**：第 0 行是寄存器序列模板 `{BPADDR, BPDATA, BPADDR, BPDATA, BPDATA}`（即通过 DSP 间接寻址口写 3 个值），第 1–5 行是 5 档亮度（-2/-1/0/+1/+2）对应的数据。5 档数据只有第 4 列（写入 `BPADDR=0x09` 处的 BPDATA）从 0x00 递增到 0x40，其余不变。`NUM_BRIGHTNESS_LEVELS = 5`。使用时按亮度档位取一行，与第 0 行模板交叉配对后依次写入。

## 4. 分辨率设置流程

结合 `ov2640_settings_to_cif/svga/uxga` 三张表，OV2640 设置分辨率的通用步骤为：

1. **选择分辨率模式（Sensor bank）**：写 `COM7` 的 `COM7_RES_UXGA/SVGA/CIF` 位（bit[6:5] 及 bit[3] 组合）。该位决定 Sensor 阵列的读出方式：UXGA 全分辨率、SVGA 2×2 子采样、CIF 4×4 子采样。
2. **设置 Sensor 输出窗口**：`COM1`、`REG32`（用 `REG32_UXGA/SVGA/CIF` 预设值）、`HSTART`/`HSTOP`/`VSTART`/`VSTOP` 圈定阵列读出区域；并写入随分辨率联动的整组寄存器（`BD50`/`BD60` 帧带、`ARCOM2`、`COM4` 等）。
3. **复位 DVP（DSP bank）**：`RESET = RESET_DVP`，使 DVP 输出逻辑与新配置对齐。
4. **设置 DSP 输出尺寸**：
   - `HSIZE8`/`VSIZE8`：输出宽/高（单位 8 像素），如 SVGA 800×600 → `0x64/0x4B`；
   - `SIZEL`：尺寸低位/拼接位；
   - `HSIZE`/`VSIZE`：DSP 图像窗口宽/高（单位 4 像素），须 **≥ 输出尺寸**；
   - `XOFFL`/`YOFFL`：窗口在传感器画面中的偏移（用于居中裁剪）；
   - `VHYX`/`TEST`：上述各尺寸/偏移的高位拼接（UXGA 尺寸超 8 位时，`VHYX = 0x88` 携带 H/V 高位）。
   - 如需 Zoom 模式，则另写 `ZMOW`/`ZMOH`/`ZMHH` 定义缩放后输出尺寸（本库三张表未使用 Zoom，保持 `COM7_ZOOM_EN` 清零）。
5. **使能缩放通路**：`CTRL2 |= CTRL2_DCW_EN`（DCW 缩放使能）；低分辨率档位写 `CTRLI = CTRLI_LP_DP` 启用行/像素加倍（UXGA 全分辨率写 0x00）。
6. **时钟配合**：
   - Sensor 侧 `CLKRC`：用 `CLKRC_2X_UXGA/SVGA/CIF` 预设组合（`CLKRC_2X` 倍频 + `CLKRC_DIV(x)` 分频），决定内部像素时钟；
   - DSP 侧 `R_DVP_SP`：PCLK 输出分频。YUV/RGB 用 `R_DVP_SP_DIV_YUV(x)`，RAW 用 `R_DVP_SP_DIV_RAW(x)`（数据率减半）；或写 `R_DVP_SP_AUTO_MODE`（0x80）让芯片按分辨率自动选择（本库全量初始化表采用自动模式，to_xxx 表中被注释掉的分频值可作为手动覆盖参考）。

典型调用顺序（以本库为例）：`ov2640_settings_cif`（上电全量初始化）→ `ov2640_settings_rgb565` 或 `ov2640_settings_jpeg`（选输出格式）→ `ov2640_settings_to_cif/svga/uxga`（选分辨率）→ 如需则写 `brightness_regs` 调亮度。

## 5. 备注

- `SS_ID` 与 `SS_CTRL` 同址（0xF7），读写语义不同，源自头文件原文，非笔误。
- init 表中大量裸十六进制地址（如 0x2C、0x33、0x5A、0x90–0x97）属于 OV2640 厂商未公开/保留寄存器，文档仅标注其在流程中的阶段归属，具体含义以 OmniVision 官方应用笔记为准。
- `ENDMARKER`（`{0xff, 0xff}`）为表结束标记；注意 `BANK_SEL` 的地址也是 0xFF，但因值恒为 0x00/0x01，不会与结束标记混淆。
