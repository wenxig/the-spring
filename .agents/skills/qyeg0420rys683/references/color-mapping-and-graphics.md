# QYEG0420RYS683 四色显存映射与图像取模算法

## 1. 像素编码与颜色定义

该水墨屏采用 **2 位（2-bit）表示 1 个像素**。每个字节包含 4 个像素，遵循 **从高位到低位（MSB to LSB）** 的像素排列顺序：

| 色彩名称 | 2-bit 二进制码 | 对应十六进制值 | 对应 RGB 参考色 |
| :---: | :---: | :---: | :--- |
| **黑色 (Black)** | `0b00` | `0x0` | `#000000` |
| **白色 (White)** | `0b01` | `0x1` | `#FFFFFF` |
| **黄色 (Yellow)**| `0b10` | `0x2` | `#FFD700` 或 `#FFFF00` |
| **红色 (Red)**   | `0b11` | `0x3` | `#FF0000` |

---

## 2. 字节内像素排布与显存计算

### 2.1 字节级排布公式
一个字节（8-bit）包含 4 个连续水平像素 $[P_0, P_1, P_2, P_3]$：

$$\text{Byte} = (P_0 \ll 6) \mid (P_1 \ll 4) \mid (P_2 \ll 2) \mid P_3$$

- **Bit 7 ~ 6**: 像素 0 ($P_0$)
- **Bit 5 ~ 4**: 像素 1 ($P_1$)
- **Bit 3 ~ 2**: 像素 2 ($P_2$)
- **Bit 1 ~ 0**: 像素 3 ($P_3$)

### 2.2 常见纯色全屏填充字节

当整屏填充单一颜色时，每个字节由 4 个相同的 2-bit 色码组合而成：

| 填充颜色 | 2-bit 色码 | 字节组合 (二进制) | 字节 Hex 值 | 全屏 30,000 字节填充说明 |
| :---: | :---: | :---: | :---: | :--- |
| **纯黑 (All Black)** | `00` | `00 00 00 00` | **`0x00`** | `memset(buf, 0x00, 30000);` |
| **纯白 (All White)** | `01` | `01 01 01 01` | **`0x55`** | `memset(buf, 0x55, 30000);` 清屏使用 |
| **纯黄 (All Yellow)**| `10` | `10 10 10 10` | **`0xAA`** | `memset(buf, 0xAA, 30000);` |
| **纯红 (All Red)**   | `11` | `11 11 11 11` | **`0xFF`** | `memset(buf, 0xFF, 30000);` |

> **⚠️ 特别注意**:
> 普通黑白单色屏清屏通常用 `0xFF` (全白)，**但本四色屏全白是 `0x55`！** 若使用 `0xFF` 写入显存，整屏刷新后将全部显示为**鲜红色**！

### 2.3 显存大小计算
- **水平分辨率**: 400 像素
- **垂直分辨率**: 300 像素
- **水平每行字节数**: $400 / 4 = 100$ 字节
- **全屏总显存**: $100 \times 300 = 30,000$ 字节 ($\approx 29.3\ \text{KB}$)

---

## 3. 帧缓冲坐标计算与画点 (SetPixel) 算法

在绘图库 (GUI/LVGL/自研图形库) 中，给定坐标 $(X, Y)$ 和颜色代码 $C \in \{0, 1, 2, 3\}$：

```c
#define EPD_WIDTH       400
#define EPD_HEIGHT      300
#define EPD_BUFFER_SIZE (EPD_WIDTH * EPD_HEIGHT / 4) // 30,000 字节

// 颜色定义
typedef enum {
    EPD_COLOR_BLACK  = 0x00, // 00
    EPD_COLOR_WHITE  = 0x01, // 01
    EPD_COLOR_YELLOW = 0x02, // 10
    EPD_COLOR_RED    = 0x03  // 11
} epd_color_t;

// 画点函数
void EPD_DrawPixel(uint8_t *buffer, int x, int y, epd_color_t color) {
    if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) {
        return;
    }

    // 计算目标字节在缓冲区中的偏移
    // 每行 100 个字节，每个字节容纳 4 个水平点
    uint32_t byte_index = y * (EPD_WIDTH / 4) + (x / 4);
    
    // 计算点在当前字节中的位置 (0~3)，0 为最左侧最高位
    uint8_t pixel_offset = x % 4;
    uint8_t shift = (3 - pixel_offset) * 2; // 0->6, 1->4, 2->2, 3->0

    // 清除该像素原有的 2 个 bit
    buffer[byte_index] &= ~(0x03 << shift);
    
    // 写入新颜色的 2 个 bit
    buffer[byte_index] |= ((color & 0x03) << shift);
}
```

---

## 4. 图像转换与取模软件 (Image2Lcd) 详细配置

如需预置静态位图或通过 PC 端工具取模，必须按照以下参数配置：

1. **原图尺寸处理**:
   - 制作或裁剪图像尺寸为 **400 × 300 像素**。
   - 建议在 Photoshop / GIMP 中先将图像色板缩减为 4 色（或通过 Floyd-Steinberg 误差扩散抖动算法分色）。
2. **取模软件设置**:
   - **输出数据类型**: C 语言数组 (`*.c`)。
   - **扫描方式**: **水平扫描 (Horizontal Scan)**。
   - **输出灰度**: **4 灰度 (4 Gray)** (即 2-bit/pixel)。
   - **扫描方向**: **从左到右，从上到下**。
   - **字节内位序**: **高位在前 (MSB First)**。
   - **包含图像头数据**: **取消勾选** (不要输出 6/8 字节的文件头)。
   - **颜色反转**: 根据取模软件默认定义核对，确保纯白输出为 `0x55`，纯黑为 `0x00`。

---

## 5. Python 自动化图片转 4 色点阵脚本

在服务端或上位机动态将任意 JPG/PNG 转换为 QYEG0420 显存字节流的参考脚本：

```python
from PIL import Image
import numpy as np

def convert_to_epd4color(image_path, output_bin_path=None):
    # 1. 打开并缩放图像至 400x300
    img = Image.open(image_path).convert('RGB')
    img = img.resize((400, 300), Image.Resampling.LANCZOS)
    
    # 2. 定义 4 色调色板: 黑(0,0,0), 白(255,255,255), 黄(255,255,0), 红(255,0,0)
    palette_data = [
        0,   0,   0,     # Index 0: 黑 (00)
        255, 255, 255,   # Index 1: 白 (01)
        255, 255, 0,     # Index 2: 黄 (10)
        255, 0,   0      # Index 3: 红 (11)
    ]
    # 填充到 256 色调色板
    palette_data += [0] * (768 - len(palette_data))
    
    pal_img = Image.new('P', (1, 1))
    pal_img.putpalette(palette_data)
    
    # 3. 使用扩散抖动量化到 4 色调色板
    quantized = img.quantize(palette=pal_img, dither=Image.Dither.FLOYDSTEINBERG)
    pixels = np.array(quantized, dtype=np.uint8) # 形状 (300, 400)，值域 0~3
    
    # 4. 4 个像素打包为一个 byte
    buffer = bytearray(30000)
    idx = 0
    for y in range(300):
        for x in range(0, 400, 4):
            p0 = pixels[y, x]
            p1 = pixels[y, x+1]
            p2 = pixels[y, x+2]
            p3 = pixels[y, x+3]
            byte_val = (p0 << 6) | (p1 << 4) | (p2 << 2) | p3
            buffer[idx] = byte_val
            idx += 1
            
    if output_bin_path:
        with open(output_bin_path, 'wb') as f:
            f.write(buffer)
            
    return buffer
```
