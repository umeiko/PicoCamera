# OV7670 寄存器参考

> 本文档由 `src/sensors/ov7670_regs.h`(寄存器地址与位域宏,移植自 esp32-camera / OpenMV,MIT)和 `src/sensors/ov7670.cpp`(初始化与控制逻辑)归纳生成,仅供查阅。**权威定义以源码为准**。
>
> OV7670 与 OV2640/OV3660 的关键差异:**无 DSP、无 JPEG 编码器**,寄存器空间是扁平的 8 位地址(无 bank 切换),缩放/开窗直接由硬件 scaling 模块完成。

## 0. 芯片速查

| 项 | 值 |
|---|---|
| SCCB 地址(7 位) | `0x21`(写 0x42 / 读 0x43) |
| PID(0x0A) | `0x76` |
| VER(0x0B) | `0x73` |
| MIDH / MIDL(0x1C / 0x1D) | `0x7F` / `0xA2`(OmniVision) |
| 最大分辨率 | VGA 640x480 |
| 支持格式 | YUV422 / RGB565 / RGB555 / RGB444 / Bayer RAW(**无 JPEG**) |
| XCLK | 典型 24MHz;本库默认 10MHz,init 表按 12MHz 标称(见 §4) |

## 1. 关键寄存器总表

### 1.1 标识 / 复位 / 格式

| 宏名 | 地址 | 用途 |
|------|------|------|
| `GAIN` | 0x00 | AGC 增益(注意:地址为 0,所以 init 表用 `{0xFF,0xFF}` 作结束标记而不是 `{0,0}`) |
| `BLUE` / `RED` / `VREF` | 0x01 / 0x02 / 0x03 | AWB 蓝/红/绿通道增益 |
| `REG_PID` / `REG_VER` | 0x0A / 0x0B | 产品 ID,探测用 |
| `COM7` | 0x12 | **核心控制**:bit7 软复位;bit6 QVGA;bit[3:0] 输出格式(YUV=0x00,RGB565=0x04,RGB555=0x08,RGB444=0x0C,Bayer=0x01/0x03) |
| `COM15` | 0x40 | 配合 COM7:bit4 RGB565 使能;bit[7:6] 输出范围(`COM15_R00FF`=0xC0 全量程) |
| `RGB444` | 0x8C | RGB444 格式控制(本驱动恒写 0 关闭) |
| `TSLB` | 0x3A | 行缓冲选项,bit 控制输出字节序等(init 写 0x04) |

### 1.2 时钟

| 宏名 | 地址 | 用途 |
|------|------|------|
| `CLKRC` | 0x11 | 内部时钟预分频:bit6 使能分频,bit[5:0] 分频系数。0x00 = 直通 |
| `DBLV` | 0x6B | PLL 倍频:bit[7:6] 00=旁路 / 01=x4 / 10=x6 / 11=x8;init 写 0x4A(x4) |
| `COM4` | 0x0D | bit[7:6] PLL 控制(0x40=x4);bit[5:4] AEC 统计窗口 |
| `SCALING_PCLK_DIV` | 0x73 | PCLK 分频(VGA=0xF0 直通,QVGA=0xF1 ÷2,QQVGA=0xF2 ÷4),需 COM14 bit3 使能 |

时钟链:`XCLK → PLL(DBLV/COM4) → CLKRC 预分频 → 像素阵列/ISP → SCALING_PCLK_DIV → PCLK 引脚`。
init 表标称"12MHz XCLK 出 30fps":12 ×4(PLL) = 48MHz 内部,再按需分频。本库 XCLK=10MHz 时帧率约按比例降为 ~25fps。

### 1.3 时序极性(COM10,采集能否跑通的关键)

| 位 | 宏 | 说明 |
|---|---|---|
| bit5 | `COM10_PCLK_MASK` | 1=行消隐期屏蔽 PCLK;init 写 0(**PCLK 自由运行**,PIO 靠 HREF 门控,不受影响) |
| bit4 | `COM10_PCLK_REV` | PCLK 反相(数据有效沿翻转;若采到花屏/错位,先怀疑此位) |
| bit3 | `COM10_HREF_REV` | HREF 反相 |
| bit1 | `COM10_VSYNC_NEG` | VSYNC 反相。init 置位 → **VSYNC 高电平=有效帧数据**,与 OV2640 一致,是本库 PIO 捕获程序(等 VSYNC 上升沿、高电平期间采样)所要求的约定 |

init 写 `COM10 = VSYNC_NEG | PCLK_FREE(=0x00)`,即 0x02。

### 1.4 窗口 / 缩放

| 宏名 | 地址 | 用途 |
|------|------|------|
| `HSTART` / `HSTOP` | 0x17 / 0x18 | 水平窗口起止高 8 位(实际值 >>3) |
| `HREF` | 0x32 | 水平窗口低 3 位拼接:bit[5:3]=HSTOP 低位,bit[2:0]=HSTART 低位 |
| `VSTART` / `VSTOP` | 0x19 / 0x1A | 垂直窗口起止高 8 位(实际值 >>2) |
| `VREF` | 0x03 | 注意与 sensor bank 的 VREF 同名不同物!此处 bit[3:2]=VSTOP 低 2 位,bit[1:0]=VSTART 低 2 位(和 AWB 增益 VREF 共用地址,靠上下文区分——OV7670 资料的著名混乱点) |
| `COM3` | 0x0C | bit3 缩放使能,bit2 DCW(降采样)使能 |
| `COM14` | 0x3E | bit4 手动缩放使能,bit3 PCLK 分频使能,bit[2:0] PCLK 分频档 |
| `SCALING_XSC` / `SCALING_YSC` | 0x70 / 0x71 | 水平/垂直缩放系数;bit7 兼作彩条测试图案控制(XSC bit7 恒 0,YSC bit7 开关彩条) |
| `SCALING_DCWCTR` | 0x72 | DCW 降采样:VGA=0x11(不裁),QQVGA=0x22(横纵各 ÷4) |
| `SCALING_PCLK_DELAY` | 0xA2 | PCLK 延迟,恒 0x02 |

### 1.5 自动控制(COM8 / COM9 / MVFP)

| 宏名 | 地址 | 用途 |
|------|------|------|
| `COM8` | 0x13 | bit7 快速 AGC/AEC,bit6 不限步长,bit2 AGC 使能,bit1 AWB 使能,bit0 AEC 使能(`set_whitebal/gain_ctrl/exposure_ctrl` 即改此寄存器) |
| `COM9` | 0x14 | bit[6:4] AGC 增益上限(init 的 fmt 表写 0x6A=128x) |
| `COM11` | 0x3B | bit4 50/60Hz 自动检测,bit1 曝光控制扩展 |
| `MVFP` | 0x1E | bit5 镜像,bit4 垂直翻转,bit1 黑太阳校正(init 写 MVFP_SUN) |
| `MTX1..MTX6` / `MTXS` | 0x4F..0x54 / 0x58 | 颜色矩阵系数,RGB565 与 YUV 各有一组推荐值 |
| `BRIGHTNESS` / `CONTRAST` | 0x55 / 0x56 | 手动亮度/对比度(本驱动暂未实现 set_*,寄存器可用) |

## 2. 初始化序列(ov7670.cpp)

```
detect:  读 0x0A == 0x76?(带一次重试)
init:    写 ov7670_default_regs(VGA YUYV、VSYNC_NEG、PCLK 自由运行、AWB/AEC/AGC 全开)
config:  set_pixformat(RGB565) → ov7670_fmt_rgb565 → 重写 CLKRC(见 §3)
         set_framesize        → ov7670_vga/qvga/qqvga 缩放表 + frame_control() 开窗
```

`frame_control(hstart, hstop, vstart, vstop)` 把 Omnivision 推荐的窗口参数拆进 HSTART/HSTOP/HREF/VSTART/VSTOP/VREF 六个寄存器:

| 分辨率 | 参数 (hstart, hstop, vstart, vstop) |
|---|---|
| VGA | 158, 14, 10, 490 |
| QVGA | 158, 14, 10, 490(同 VGA,靠 DCW ÷2 + PCLK ÷2 得 320x240) |
| QQVGA | 158, 14, 12, 490(DCW ÷4 + PCLK ÷4) |

## 3. 移植注意事项(踩坑记录位)

- **CLKRC 重写怪癖**(沿用 esp32-camera 注释):RGB565 模式下必须在其他参数写完后**重写一次 CLKRC**,否则图像发灰;YUV 模式则不要重写。驱动里 `s_clkrc` 静态变量在 `set_framesize` 开头读回保存,`set_pixformat` 末尾写回,实现帧率跨配置保持。
- **GAIN 地址为 0x00**:寄存器表不能以 `{0,0}` 作结束标记,本驱动统一 `{0xFF,0xFF}`(`sccb_write_list8` 的约定)。
- **探测不要先写 0xFF=0x01**:esp32-camera 的 ov7670_detect 有这一句(从 OV2640 的 bank 切换抄来的),OV7670 无 bank 概念,写了无害但无意义,本库已省略。
- **SCCB 读必须 STOP 分段,不能 repeated-start**(实测踩坑):OV7670 是严格 SCCB 而非标准 I2C,寄存器地址写 phase 若用 repeated-start 紧接读 phase,芯片不应答(探测返回 -101)。本库 `sccb_read8` 已改为写地址→STOP→读数据的规范两段式,OV2640/OV3660 同样兼容。
- **无 JPEG**:`camera_sensor_info_t.support_jpeg = false`,`pico_camera_init` 遇到 `PIXFORMAT_JPEG` 直接返回 `PICO_CAMERA_ERR_NOT_SUPPORTED`(与 esp32-camera 行为一致);运行时再调 `set_pixformat(JPEG)` 返回 -1。
- **帧缓冲**:VGA RGB565 需 600KB,超 RP2040 SRAM,malloc 会失败返回 `PICO_CAMERA_ERR_NO_MEM`;实用上限 QVGA(150KB)。
