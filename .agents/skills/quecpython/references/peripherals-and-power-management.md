# QuecPython 硬件外设与电源管理开发指南

本文档全面梳理 QuecPython `machine`、`misc`、`pm` 等模块对于 GPIO、UART、I2C、SPI、ADC、PWM、RTC、硬件看门狗 (WDT) 以及低功耗休眠唤醒模式的使用规范。

---

## 1. GPIO 控制与外部中断 (`machine.Pin` & `machine.ExtInt`)

### 1.1 Pin 基本输入输出

```python
from machine import Pin

# 创建输出引脚 (Pin.OUT, Pin.PULL_DISABLE, 初始电平 0)
led = Pin(Pin.GPIO1, Pin.OUT, Pin.PULL_DISABLE, 0)
led.write(1)  # 输出高电平
led.write(0)  # 输出低电平

# 创建输入引脚带上拉 (Pin.IN, Pin.PULL_PU)
btn = Pin(Pin.GPIO2, Pin.IN, Pin.PULL_PU, 0)
val = btn.read()
print(f"当前输入引脚电平: {val}")
```

### 1.2 ExtInt 外部中断

```python
from machine import ExtInt

def extint_cb(args):
    # args: 中断源引脚号
    print(f"GPIO 中断触发: pin={args}")

# 配置 GPIO2 为双边沿触发中断 (1: 上升沿, 2: 下降沿, 3: 双边沿)
ext = ExtInt(ExtInt.GPIO2, ExtInt.EDGE_BOTH, ExtInt.PULL_PU, extint_cb)
ext.enable()
# 不需要时可以 ext.disable()
```

---

## 2. 串口通信 (`machine.UART`)

QuecPython 模块通常具备多个 UART 控制器（例如 UART0/UART1/UART2/UART3）。

```python
from machine import UART

# 初始化 UART1: 波特率 115200, 数据位 8, 校验位 0, 停止位 1, 流控 0
uart1 = UART(UART.UART1, 115200, 8, 0, 1, 0)

# 设置读取异步回调函数
def uart_cb(args):
    # args: (UART 编号, 可读字节长度)
    port, length = args
    if length > 0:
        data = uart1.read(length)
        print(f"收到串口数据: {data}")
        # 原样回传
        uart1.write(b"Echo: " + data)

uart1.set_callback(uart_cb)

# 主动发送数据
uart1.write(b"Hello QuecPython UART\r\n")
```

---

## 3. I2C 与 SPI 总线外设

### 3.1 硬件 I2C 通信

```python
from machine import I2C

# 初始化 I2C1 (通道 1, 速率 400kHz 标准快速模式)
i2c = I2C(I2C.I2C1, I2C.FAST_MODE)

# 扫描 I2C 挂载设备地址
addrs = i2c.scan()
print(f"扫描到 I2C 设备列表: {[hex(a) for a in addrs]}")

# 读写指定设备 (以传感器 AHT20 地址 0x38 为例)
DEV_ADDR = 0x38
# 向寄存器写入控制命令
i2c.write(DEV_ADDR, bytearray([0xAC, 0x33, 0x00]))
# 从设备连续读取 6 字节数据
data = bytearray(6)
i2c.read(DEV_ADDR, data, 6)
print(f"I2C 读取数据: {list(data)}")
```

### 3.2 硬件 SPI 总线

```python
from machine import SPI

# 初始化 SPI0: 主机模式 0, CPOL=0, CPHA=0, 时钟分频 (频率可达 10MHz~20MHz)
spi = SPI(SPI.SPI0, 0, 0, 0, 0)

# 发送与接收
send_buf = bytearray([0x9F])  # 读 Flash JEDEC ID 命令
recv_buf = bytearray(3)
spi.write_read(send_buf, recv_buf, 3)
print(f"SPI 接收 JEDEC ID: {[hex(b) for b in recv_buf]}")
```

---

## 4. 模拟信号输入 (`misc.ADC`) 与脉宽调制 (`misc.PWM`)

### 4.1 模拟量采集 (ADC)

```python
from misc import ADC

# 初始化 ADC 通道 0
adc = ADC()
adc.open()

# 读取当前电压值 (毫伏 mV)
# ADC.ADC0 (根据模组硬件引脚映射)
mv = adc.read(ADC.ADC0)
print(f"ADC0 采样电压: {mv} mV")

adc.close()
```

### 4.2 蜂鸣器 / 调光控制 (PWM)

```python
from misc import PWM

# 初始化 PWM0: 频率 2000Hz, 占空比 50% (范围 0~1000)
# PWM.PWM0
pwm = PWM(PWM.PWM0, PWM.ABOVE_10US, 500, 1000)
pwm.open()

# 调节占空比 (改为 20%)
pwm.open(PWM.PWM0, PWM.ABOVE_10US, 200, 1000)

# 关闭 PWM 输出
pwm.close()
```

---

## 5. 硬件看门狗 (`machine.WDT`)

防止系统在极值状态或网络通信死锁时死机：

```python
from machine import WDT
import utime

# 初始化看门狗，喂狗超时时间 10 秒
wdt = WDT(10)

# 主循环中定期喂狗
while True:
    # 执行业务逻辑
    utime.sleep(1)
    wdt.feed()  # 重置看门狗计时器
```

---

## 6. 电源管理与低功耗休眠唤醒 (`pm` & `misc.Power`)

### 6.1 低功耗休眠模式 (`pm`)

蜂窝模组待机功耗主要取决于休眠模式。QuecPython 提供自动休眠管理：

```python
import pm

# 1. 设置自动休眠
# 模组在无通信任务时自动进入低功耗休眠 (Sleep 模式，功耗降低至毫安甚至微安级)
pm.autosleep(1)

# 2. 查询当前休眠锁计数 (若大于 0 说明有唤醒锁持有，无法休眠)
lock_count = pm.get_wakelock_num()
print(f"当前唤醒锁数量: {lock_count}")

# 3. 创建与释放业务唤醒锁
lock = pm.create_wakelock("my_business_lock", len("my_business_lock"))
lock.acquire()  # 阻止模组休眠，执行高功耗任务
# ... 执行耗时任务 ...
lock.release()  # 释放锁，允许系统重新入睡
```

### 6.2 设备重启与关机 (`misc.Power`)

```python
from misc import Power

# 重启设备
# Power.powerRestart()

# 安全关机
# Power.powerDown()
```
