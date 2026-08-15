# push_image_to_python

摄像头画面通过 USB 串口实时推送到 PC,Python 上位机用 tkinter GUI 显示。

## 协议

二进制帧，两种格式：

| 格式 | 帧结构 |
|------|--------|
| RGB565 | `SRGB` + `宽×高×2` 字节原始数据（大端 RGB565)+ `ERGB` |
| JPEG | `SJPG` + 原始 JPEG 数据（`FFD8...FFD9`)+ `EJPG` |

## 下位机（`push_image_to_python.ino`)

烧录前用文件顶部的宏选择推送格式：

```cpp
#define PUSH_FORMAT 1   // 1 = JPEG (SJPG...EJPG), 0 = RGB565 (SRGB...ERGB)
```

默认 320x240(FRAMESIZE_QVGA)。烧录后下位机会循环向上位机发帧，无需任何交互。

## 上位机（`push_image_to_python.py`)

依赖（仅两个外部库）：

```
pip install pyserial Pillow
```

运行：

```
python push_image_to_python.py
```

界面操作：

1. **Refresh** — 重新扫描串口
2. 下拉框选择 Pico 的串口（如 `COM4`)
3. **Connect** — 开始接收并渲染画面；再点一次断开

状态栏显示实时帧率。

<p align="center">
  <img src="../../docs/image_capture.png" width="360" alt="push_image_to_python 上位机截图">
  <br>
  <i>上位机实时渲染 OV2640 JPEG 图像流（约 7 fps）</i>
</p>

## 说明

- RGB565 模式按固定尺寸 320x240 解析；如下位机改了 `frame_size`，请同步修改 py 文件顶部的 `RGB_WIDTH` / `RGB_HEIGHT`
- RGB565 推荐先用 JPEG 模式验证链路（数据量小一个数量级，帧率高得多）;RGB565 原始流约 150KB/帧，USB CDC 下帧率有限
- 上位机解析 JPEG 帧时以 `FFD8` 开头校验，避免因 JPEG 熵数据中偶现 `EJPG` 字节序列导致错帧
