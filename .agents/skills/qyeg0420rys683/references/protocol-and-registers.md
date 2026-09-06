# QYEG0420RYS683 驱动协议、寄存器映射与时序控制

## 1. 通信协议规范

- **通信接口**: 4-Wire SPI (8-bit 数据格式，MSB 首先发送)。
- **SPI 模式**: Mode 0 (CPOL = 0, CPHA = 0)。
  - SCL/SCK 空闲时保持低电平。
  - 数据在时钟上升沿被屏幕驱动 IC 采样锁存。
- **引脚配合**:
  - `CS#`: 传输前拉低，传输结束后拉高。
  - `D/C#`: 命令传输时拉低 (`0`)，参数或显示缓冲数据传输时拉高 (`1`)。
  - `BUSY`: 驱动 IC 执行内部高压升压或波形刷新时会拉低 (`0`)。在发送任何指令前，必须等待 BUSY 回到高电平 (`1`)。

---

## 2. 核心寄存器与指令集映射表

| 指令代码 (Hex) | 助记符 | 作用说明 | 参数字节数与典型值 |
| :---: | :---: | :--- | :--- |
| **`0x00`** | **PSR** (Panel Setting) | 面板配置与扫描方向设置 | 2 字节：`0x2F, 0x69` (或 FPC 下/左方向适配) |
| **`0x01`** | **PWR** (Power Setting) | 内部电源控制与 LDO 配置 | 内部电源参数 |
| **`0x02`** | **POF** (Power OFF) | 关闭内部 DC-DC 模拟高压驱动 | 1 字节：`0x00`。**发完后必须等待 BUSY 为高** |
| **`0x03`** | **PFS** (Power OFF Seq) | 掉电下电时序设定 | 内部配置 |
| **`0x04`** | **PON** (Power ON) | 开启内部升压电荷泵与高压电源 | 无参数。**发完后必须等待 BUSY 为高** |
| **`0x06`** | **BTST** (Booster Soft Start) | 升压回路软启动与电荷泵强度 | 4 字节：`0x0F, 0x8B, 0x9C, 0x96` |
| **`0x07`** | **DSLP** (Deep Sleep) | 进入极低功耗深度休眠模式 | 1 字节：`0xA5` (进入深度休眠后芯片不再响应 SPI，需硬复位唤醒) |
| **`0x10`** | **DTM** (Data Start Transmission) | 开始传输图像显存数据 | 连续发送 **30,000 字节** 像素数据 (400×300 / 4) |
| **`0x12`** | **DRF** (Display Refresh) | 启动全屏电泳微胶囊波形刷新 | 1 字节：`0x00`。**发完后必须等待 BUSY 为高 (约 15~20s)** |
| **`0x30`** | **PLL** (PLL Control) | 内部振荡器与帧频控制 | 1 字节：`0x08` (典型 50Hz/100Hz 刷新波形基准时钟) |
| **`0x41`** | **TSE** (Temp Sensor Enable) | 温度传感器使能 (根据内置表校正波形) | 1 字节：`0x00` |
| **`0x50`** | **CDI** (VCOM and DATA Interval) | VCOM 脉冲间隙与边框数据设置 | 1 字节：`0x37` |
| **`0x60`** | **TCON** (Timing Control) | 门极与源极非重叠时间配置 | 内部时序配置 |
| **`0x61`** | **TRES** (Resolution Setting) | 显存分辨率设置 | 4 字节：`0x01, 0x90, 0x01, 0x2C` (即 400 × 300) |
| **`0x62`** | - | 源极/门极预充电与内部通道配置 | 2 字节：`0x64, 0x53` |
| **`0x65`** | **GSST** (Gate/Source Start) | 源极和门极扫描起始偏置 | 4 字节：`0x00, 0x00, 0x00, 0x00` |
| **`0xE0`** | - | 扩展寄存器访问使能 | 1 字节：`0x02` |
| **`0xE3`** | **PWS** (Power Saving) | 省电寄存器 | 内部配置 |
| **`0xE6`** | - | 内部驱动频率与脉宽细调 | 1 字节：`0x5C` (十进制 92) |
| **`0xE9`** | - | 厂商私有初始化序列寄存器 1 | 1 字节：`0x01` |
| **`0xEF`** | - | 厂商私有初始化序列寄存器 2 | 1 字节：`0x01` (开启) / `0x00` (锁定) |
| **`0xF6`** | - | 厂商私有脉冲幅度偏置 | 1 字节：`0x15` |
| **`0xA5`** | - | 厂商内部校准执行确认 | 无参数。**执行后需等待 BUSY 回高** |

---

## 3. 标准完整初始化流程 (C/C++ 伪代码)

在对水墨屏做任何显示操作前，必须严格执行以下初始化时序：

```c
// 1. 硬件复位
void EPD_Hardware_Reset(void) {
    GPIO_Set(PIN_RST, 1);
    Delay_ms(200);
    GPIO_Set(PIN_RST, 0); // 拉低复位
    Delay_ms(200);
    GPIO_Set(PIN_RST, 1);
    Delay_ms(200);
    EPD_WaitUntilIdle();  // 等待内部稳压建立 (BUSY == 1)
}

// 2. 寄存器初始化配置
void EPD_Init(void) {
    EPD_Hardware_Reset();

    // 软启动配置
    EPD_SendCommand(0x06); // BTST
    EPD_SendData(0x0F);
    EPD_SendData(0x8B);
    EPD_SendData(0x9C);
    EPD_SendData(0x96);

    // 厂商私有波形优化序列
    EPD_SendCommand(0xE9);
    EPD_SendData(0x01);

    EPD_SendCommand(0xEF);
    EPD_SendData(0x01);

    EPD_SendCommand(0xF6);
    EPD_SendData(0x15);

    EPD_SendCommand(0xEF);
    EPD_SendData(0x00);

    // 面板尺寸与扫描方向
    EPD_SendCommand(0x00); // PSR
    EPD_SendData(0x2F);    // 0x2F (FPC向下排线) 或 0x27 (FPC向左排线)
    EPD_SendData(0x69);
    EPD_WaitUntilIdle();

    // PLL 与 帧频
    EPD_SendCommand(0x30); // PLL
    EPD_SendData(0x08);

    // 分辨率设置: 400 (0x0190) x 300 (0x012C)
    EPD_SendCommand(0x61); // TRES
    EPD_SendData(0x01);    // 400 >> 8
    EPD_SendData(0x90);    // 400 & 0xFF
    EPD_SendData(0x01);    // 300 >> 8
    EPD_SendData(0x2C);    // 300 & 0xFF

    // 内部时钟与门极微调
    EPD_SendCommand(0x62);
    EPD_SendData(0x64);
    EPD_SendData(0x53);

    EPD_SendCommand(0x65); // GSST
    EPD_SendData(0x00);
    EPD_SendData(0x00);
    EPD_SendData(0x00);
    EPD_SendData(0x00);

    // VCOM 与 数据间隔
    EPD_SendCommand(0x50); // CDI
    EPD_SendData(0x37);

    // 内部参数微调
    EPD_SendCommand(0xE0);
    EPD_SendData(0x02);

    EPD_SendCommand(0xE6);
    EPD_SendData(0x5C);

    // 执行内部自检
    EPD_SendCommand(0xA5);
    EPD_WaitUntilIdle();
}
```

---

## 4. 全屏刷新与关电休眠流程

数据写入显存后，需要给物理微胶囊通电进行高压震荡刷新，并必须在刷新完成后彻底休眠切断电荷：

```c
// 3. 全屏数据送入显存 (30000 字节)
void EPD_DisplayFrame(const uint8_t *image_buffer) {
    EPD_SendCommand(0x10); // DTM (Data Start Transmission)
    for (uint32_t i = 0; i < 30000; i++) {
        EPD_SendData(image_buffer[i]);
    }
}

// 4. 执行刷新与深度休眠
void EPD_Refresh_And_Sleep(void) {
    // 开启高压泵
    EPD_SendCommand(0x04); // PON (Power ON)
    EPD_WaitUntilIdle();

    // 触发微胶囊迁移刷新波形
    EPD_SendCommand(0x12); // DRF (Display Refresh)
    EPD_SendData(0x00);
    EPD_WaitUntilIdle();   // 屏幕开始闪烁刷新，此等待持续约 15~20 秒！

    // 关闭高压驱动
    EPD_SendCommand(0x02); // POF (Power OFF)
    EPD_SendData(0x00);
    EPD_WaitUntilIdle();

    // 进入深度休眠 (关闭内部 LDO 与振荡器)
    EPD_SendCommand(0x07); // DSLP (Deep Sleep)
    EPD_SendData(0xA5);    // 写入 0xA5 校验码确认休眠
    Delay_ms(200);
}
```
