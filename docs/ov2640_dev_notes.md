# OV2640 驱动修改记录

## 硬件连接
```
Y2-Y9:  GPIO14-GPIO21 (连续数据引脚)
PCLK:   GPIO4 (像素时钟)
HREF:   GPIO3 (行同步)
VSYNC:  GPIO2 (帧同步)
SIOC:   GPIO13 (I2C SCL)
SIOD:   GPIO12 (I2C SDA)
XCLK:   GPIO5 (PWM时钟输出)
RESET:  GPIO29 (复位)
```

## 问题与修复

### 1. PIO 指令编码错误 ❌ → 修复 ✅
**问题：** GPIO 号在 PIO 指令中的位置错误

**错误代码：**
```c
// 错误：GPIO 号放在 bits [9:5]
((gpio & 0x1F) << 5)
```

**正确编码：**
```c
// 正确：GPIO 号放在 bits [10:6]
((gpio & 0x1F) << 6)
```

**PIO wait gpio 指令格式（16-bit）：**
- Bits 15:13 = 001 (wait)
- Bit 12 = polarity (1=wait for 1)
- Bit 11 = 0 (gpio mode)
- **Bits 10:6 = GPIO 编号**
- Bits 5:0 = delay

**解决方案：** 将 GPIO 号直接硬编码在 `image.pio` 中，让 pioasm 自动生成正确编码：
```asm
.define PUBLIC VSYNC_PIN  2
.define PUBLIC HREF_PIN   3
.define PUBLIC PCLK_PIN   4

wait 0 gpio VSYNC_PIN
wait 1 gpio VSYNC_PIN
```

### 2. XCLK 时钟频率
**原始：** 20.83MHz
**修改：** 10.4MHz (125MHz/12)

**原因：** OV2640 需要 10-24MHz，降低频率提高稳定性
```c
pwm_set_wrap(slice_num, 11);      // 12 cycles
pwm_set_gpio_level(config->pin_xclk, 6);  // 50% duty
```

### 3. GPIO 功能配置
必须将 GPIO 设置为 PIO 功能：
```c
gpio_set_function(VSYNC_PIN, GPIO_FUNC_PIO1);
gpio_set_function(HREF_PIN, GPIO_FUNC_PIO1);
gpio_set_function(PCLK_PIN, GPIO_FUNC_PIO1);
for (i=0; i<8; i++) {
    gpio_set_function(Y2_BASE + i, GPIO_FUNC_PIO1);
}
```

### 4. 初始化延时
增加延时确保 XCLK 稳定：
```c
pwm_set_enabled(slice_num, true);
sleep_ms(100);  // 等待时钟稳定
```

## 显示相关

### 字节序转换（毒蘑菇）
OV2640 输出大端 RGB565，需要转换为小端：
```c
line_buffer[x] = __builtin_bswap16(src_row[x]);
```

### 分辨率适配
- 摄像头：QVGA (320x240)
- 屏幕：280x240
- 显示：水平居中裁剪 280 列 (crop_x=20)

## 关键调试信息

### PIO 程序计数器 (PC) 位置
- `offset + 0` = pull
- `offset + 1` = mov x, osr
- `offset + 2` = wait 0 gpio VSYNC
- `offset + 3` = wait 1 gpio VSYNC
- `offset + 4` = wait 0 gpio HREF
- `offset + 5` = wait 1 gpio HREF (capture_loop)
- `offset + 6` = wait 1 gpio PCLK
- `offset + 7` = in pins, 8
- `offset + 8` = wait 0 gpio PCLK
- `offset + 9` = jmp x-- capture_loop

### 超时故障排查
如果 PC 停在 `offset + 2`：
- VSYNC 一直是高电平，摄像头未输出信号
- 检查 XCLK 时钟是否正常
- 检查 I2C 配置是否正确
- 增加初始化延时

## 文件修改列表
- `src/ov2640/image.pio` - PIO 汇编程序（硬编码 GPIO 号）
- `src/ov2640/pio_init.cpp` - PIO 初始化函数
- `src/ov2640/ov2640.cpp` - 驱动实现（XCLK、延时）
- `src/main.cpp` - 显示函数（字节序转换）
