# QuecPython 基础与板载外设驱动指南

本文档提供在 EC600X-EVB (EC600M) 上运行 QuecPython 的板载外设驱动示例与配置说明。

---

## 1. QuecPython 运行环境与工具链

- **固件烧录与交互工具**: QPYcom（官方图形化串口/REPL与文件上传工具）
- **通信接口**:
  - USB CDC 串口：默认挂载 REPL 控制台与大容量存储传输。
  - J5 Pin 8 / 9 (TX2 / RX2)：Debug 串口输出底层日志。

---

## 2. 板载 AHT20 温湿度传感器驱动

板载 AHT20 挂载在 J5 的 I2C1 接口（SDA=Pin 56, SCL=Pin 57）。

### 2.1 驱动实现代码
```python
from machine import I2C
import utime

class AHT20:
    def __init__(self, i2c_bus=I2C.I2C1, mode=I2C.STANDARD_MODE):
        self.i2c = I2C(i2c_bus, mode)
        self.addr = 0x38
        utime.sleep_ms(100)
        self._init_sensor()

    def _init_sensor(self):
        # 发送校准初始化指令
        self.i2c.write(self.addr, bytearray([0xBE, 0x08, 0x00]))
        utime.sleep_ms(10)

    def read_data(self):
        # 触发测量
        self.i2c.write(self.addr, bytearray([0xAC, 0x33, 0x00]))
        utime.sleep_ms(80)
        
        # 读取 7 字节数据
        data = bytearray(7)
        self.i2c.read(self.addr, data, 7)
        
        # 检查测量状态
        if (data[0] & 0x80) != 0:
            raise RuntimeError("AHT20 busy")
            
        # 提取原始 20 位湿度和温度数据
        raw_humi = ((data[1] << 12) | (data[2] << 4) | (data[3] >> 4))
        raw_temp = (((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5])
        
        humidity = (raw_humi * 100.0) / (1 << 20)
        temperature = (raw_temp * 200.0) / (1 << 20) - 50.0
        
        return round(temperature, 2), round(humidity, 2)

# 使用示例
if __name__ == '__main__':
    sensor = AHT20()
    temp, humi = sensor.read_data()
    print("当前温度: {}°C, 湿度: {}%".format(temp, humi))
```

---

## 3. 板载 GT36528 光敏电阻采集 (ADC)

光敏电阻连接在 J6 Pin 4 的 ADC0 输入端（EC600M 引脚 19）。

### 3.1 驱动代码
```python
from misc import ADC
import utime

class LightSensor:
    def __init__(self, channel=ADC.ADC0):
        self.adc = ADC()
        self.channel = channel

    def get_voltage(self):
        # 返回采样得到的毫伏值 (mV)
        val = self.adc.read(self.channel)
        return val

if __name__ == '__main__':
    light = LightSensor()
    for _ in range(5):
        mv = light.get_voltage()
        print("光敏电阻采样电压: {} mV".format(mv))
        utime.sleep(1)
```

---

## 4. 板载音频功放控制与播放

板载 NS4160 功率放大器驱动外接扬声器，用于报警音或语音呼叫提示。

```python
import audio
from machine import Pin

# 播放系统预置或本地音频文件
def play_prompt(file_path):
    aud = audio.Audio(0)
    aud.setVolume(8)  # 音量范围 0~11
    aud.play(file_path)
```
