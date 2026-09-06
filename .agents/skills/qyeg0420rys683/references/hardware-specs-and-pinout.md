# QYEG0420RYS683 硬件电气规格与引脚定义

## 1. 显示屏基础电气与机械规格

- **型号**: `QYEG0420RYS683F38`
- **制造商/方案**: 齐运微电 (Qiyun Microelectronics) / 佳显电子 (Good Display) 4.2 寸 4 色电子纸
- **显示尺寸**: 4.2 英寸对角线 (Active Area: 84.80 mm × 63.60 mm)
- **物理外形尺寸**: 91.00 mm (H) × 77.00 mm (V) × 1.00 mm (D)
- **点阵分辨率**: 400 (水平) × 300 (垂直)
- **像素间距 (Pixel Pitch)**: 0.212 mm × 0.212 mm (约 119 DPI)
- **色彩模式**: 4 色 (黑 / 白 / 黄 / 红，2-bit/pixel)
- **工作电压**: VCI / VDDIO = 2.3V ~ 3.6V (典型工作电压 3.3V)
- **逻辑电平**:
  - 输入高电平 $V_{IH} \ge 0.8 \times V_{DDIO}$
  - 输入低电平 $V_{IL} \le 0.2 \times V_{DDIO}$
  - 输出高电平 $V_{OH} \ge 0.9 \times V_{DDIO}$
  - 输出低电平 $V_{OL} \le 0.1 \times V_{DDIO}$
- **功耗特性**:
  - 刷新功耗 (Refresh Power): 典型 40 mW ~ 50 mW
  - 静态待机功耗 (Standby): 极低 (微安级)
  - 深度休眠功耗 (Deep Sleep): $\le 5\ \mu\text{A}$ (断开 DC-DC 与逻辑时钟)
- **温度特性**:
  - 工作环境温度: 0°C ~ +40°C
  - 存储环境温度: -25°C ~ +70°C (最佳存储条件: 23±3°C, 55±10% RH)
- **全屏刷新时间**: 约 15 ~ 20 秒 (四色电泳颗粒需分阶段多次极性迁移翻转，不支持局部微秒级局刷)

---

## 2. 24-Pin FPC 排线引脚定义 (0.5mm 间距 下接/正向)

水墨屏出厂自带 24-pin FPC 软排线，与控制板或转接板接口引脚对应关系如下：

| Pin | 名称 | 类型 | 详细描述与接线建议 |
| :---: | :---: | :---: | :--- |
| **1** | NC | - | 空脚，悬空不接 (Keep Open) |
| **2** | **GDR** | O | N-MOSFET 栅极驱动输出，用于外部 DC-DC 升压回路开关控制 |
| **3** | **RESE** | I | 电流检测输入 (Current Sense)，外接采样电阻到地用于控制环路 |
| **4** | NC | - | 空脚，悬空不接 |
| **5** | **VSH2** | P | 正向高压电源 2 (用于红色彩色粒子驱动)，外接滤波电容 (1uF~4.7uF) 到 VSS |
| **6** | NC | - | 空脚，悬空不接 |
| **7** | NC | - | 空脚，悬空不接 |
| **8** | **BS1** | I | 总线模式选择：<br>• **LOW (GND)**: 4 线 SPI (8-bit 传输，由 D/C# 引脚区分命令/数据) **[推荐模式]**<br>• **HIGH (VDD)**: 3 线 SPI (9-bit 传输，首 bit 为 D/C 标志) |
| **9** | **BUSY** | O | **忙状态输出引脚** (高有效/低有效判定注意)：<br>• **低电平 (LOW) = 屏处于 BUSY 状态** (内部正在执行刷新或上电，此时严禁发送指令/数据！)<br>• **高电平 (HIGH) = 空闲 IDLE**，可以接收 SPI 通信 |
| **10** | **RES#** | I | **硬件复位引脚** (低电平有效，Active LOW)。拉低至少 200ms 后拉高复位芯片内部寄存器 |
| **11** | **D/C#** | I | **数据/命令控制引脚**：<br>• LOW: 写入指令 (Command/Register Address)<br>• HIGH: 写入数据 (Data / Pixel Buffer) |
| **12** | **CS#** | I | **SPI 片选引脚** (低电平有效，Active LOW)。拉低使能 SPI 通信，通信完成后拉高 |
| **13** | **SCL (SCK)**| I | **SPI 时钟引脚**。SPI Mode 0 (CPOL=0, CPHA=0)，时钟上升沿锁存采样数据 |
| **14** | **SDA (MOSI)**| I | **SPI 主出从入数据引脚**。MSB 优先传输 |
| **15** | **VDDIO** | P | 数字接口 I/O 电源引脚，通常接 **3.3V** (需接 1uF 去耦电容) |
| **16** | **VCI** | P | 模拟与芯片主电源引脚，接 **3.3V** (需接 1uF ~ 4.7uF 旁路去耦电容) |
| **17** | **VSS** | P | 系统参考地 **GND** |
| **18** | **VDD** | P | 内部数字逻辑稳压核心引脚 (约 1.8V)，外接 1uF 旁路电容到 VSS |
| **19** | **VPP** | - | OTP 编程高压供电，正常工作保持悬空或接 0.1uF 电容到 VSS |
| **20** | **VSH1** | P | 正源极高压电源 (Positive Source Driving Voltage)，外接 1uF~2.2uF 耐压高压电容到 VSS |
| **21** | **VGH** | P | 正栅极高压电源 (Positive Gate Driving Voltage, ~15V~22V)，外接高压电容到 VSS |
| **22** | **VSL** | P | 负源极高压电源 (Negative Source Driving Voltage)，外接高压电容到 VSS |
| **23** | **VGL** | P | 负栅极高压电源 (Negative Gate Driving Voltage, ~-15V~-20V)，外接高压电容到 VSS |
| **24** | **VCOM** | P | 公共电极驱动电压 (VCOM Driving Voltage)，外接高压电容到 VSS |

---

## 3. 标准 8-Pin 转接板 / 开发板引脚对接 (DESPI-C02)

如果使用官方转接板 (如 DESPI-C01 / DESPI-C02 等带有外围升压 DC-DC 电路的成品板)，转接板通常只引出标准的 8 个排针接口：

| 转接板排针 | 信号定义 | 微控制器 / ESP32-P4 连接建议 | 功能说明 |
| :---: | :---: | :---: | :--- |
| **VCC** | 3.3V | 3.3V 供电轨 | 需保证瞬态供电电流 > 50mA (升压瞬间有尖峰) |
| **GND** | GND | 系统地 GND | 供电地 |
| **DIN** | MOSI | ESP32-P4 SPI MOSI | SPI 数据主发线 |
| **CLK** | SCK | ESP32-P4 SPI SCLK | SPI 时钟线 (推荐频率 2MHz ~ 10MHz) |
| **CS** | CS# | ESP32-P4 GPIO (CS) | 片选，低电平使能 |
| **DC** | D/C# | ESP32-P4 GPIO (D/C) | 数据/命令切换 (High=Data, Low=Cmd) |
| **RST** | RES# | ESP32-P4 GPIO (RST) | 硬件复位引脚 (Low 脉冲复位) |
| **BUSY**| BUSY | ESP32-P4 GPIO (Input)| 状态检测引脚 (Low=忙碌, High=就绪) |

---

## 4. 关键硬件设计避坑点

1. **BUSY 引脚状态极性注意**:
   - 绝大多数标准 Eink (如 SSD1680) 是 High=Busy, Low=Ready。
   - **但是该屏幕 QYEG0420RYS683 驱动芯片为 LOW = BUSY, HIGH = IDLE!**
   - 轮询等待忙状态的循环条件**必须**写成：
     ```c
     while (gpio_get_level(PIN_BUSY) == 0) {
         vTaskDelay(pdMS_TO_TICKS(10));
     }
     ```
   - 任何误将 `HIGH` 当作 BUSY 的代码都会导致程序跳过等待或死锁。
2. **休眠保护与防烧屏 (Deep Sleep)**:
   - 水墨屏内部电极在长时间维持静态高压时极易老化或损坏物理微胶囊。
   - **一旦完成全屏刷新，必须依次发送 Power Off (`0x02`) 和 Deep Sleep (`0x07`, `0xA5`) 命令**。
   - 若设备休眠，必须释放或拉低 SPI 引脚，切断背部漏电路径。
3. **SPI 速率与工作模式**:
   - 必须使用 **SPI Mode 0** (CPOL=0, CPHA=0)。
   - 电子纸芯片内部移位寄存器速度限制，SPI 时钟建议设定在 **4MHz ~ 10MHz**，不可超过 12MHz。
