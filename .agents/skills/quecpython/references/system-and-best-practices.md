# QuecPython 系统架构、多线程与工程最佳实践指南

本文档全面梳理 QuecPython `syslib`、`stdlib`、`componentlib` 中的多任务并发模型（`_thread`、`Queue`、`sys_bus`、`uasyncio`）、文件系统（`uos`、`ql_fs`）、OTA 升级（`fota`、`app_fota`）以及鲁棒性异常防护机制。

---

## 1. 多任务并发模型

在资源受限的模组（如 EC600M 内存约几十 MB，部分轻量模块仅几 MB 堆空间）中，推荐选择合适的任务并发模式：

### 1.1 轻量多线程 (`_thread`) 与堆栈控制

QuecPython 支持标准的 `_thread` 模块，但必须严格指定每个子线程的堆栈大小，避免栈溢出（Stack Overflow）：

```python
import _thread
import utime

def sensor_worker(interval):
    print("传感器后台采集线程已启动")
    while True:
        # 执行采集
        utime.sleep(interval)

# 启动线程：必须传入 tuple 参数
# stack_size 建议在 8KB ~ 16KB (8192 ~ 16384 bytes)
_thread.stack_size(8192)
thread_id = _thread.start_new_thread(sensor_worker, (5,))
print(f"工作线程 ID: {thread_id}")
```

### 1.2 线程安全消息队列 (`queue.Queue`)

用于多线程间无锁/安全的数据解耦传递：

```python
from queue import Queue
import _thread
import utime

msg_queue = Queue(maxsize=20)

def producer():
    for i in range(10):
        msg_queue.put(f"Event-{i}")
        utime.sleep_ms(200)

def consumer():
    while True:
        item = msg_queue.get()
        print(f"消费消息: {item}")

_thread.start_new_thread(producer, ())
_thread.start_new_thread(consumer, ())
```

### 1.3 系统总线事件发布订阅 (`sys_bus`)

QuecPython 内置的轻量事件分发中心，实现全系统跨模块解耦：

```python
import sys_bus

# 1. 订阅特定 Topic
def topic_handler(topic, msg):
    print(f"[sys_bus] 收到事件 {topic}: {msg}")

sys_bus.subscribe("NET_STATUS_CHANGE", topic_handler)

# 2. 发布事件
sys_bus.publish("NET_STATUS_CHANGE", {"connected": True, "ip": "10.0.0.1"})
```

### 1.4 异步协作式协程 (`uasyncio`)

对于高并发 I/O 等待任务，使用协程可极大节约栈空间：

```python
import uasyncio as asyncio

async def blink():
    while True:
        # 翻转电平
        await asyncio.sleep(1)

async def network_poll():
    while True:
        # 异步查询
        await asyncio.sleep(5)

async def main():
    asyncio.create_task(blink())
    asyncio.create_task(network_poll())
    while True:
        await asyncio.sleep(10)

# asyncio.run(main())
```

---

## 2. 文件系统与文件存储分区 (`uos` & `ql_fs`)

QuecPython 文件系统根目录分为用户空间 `/usr` 独立分区：

```python
import uos

# 1. 查询磁盘剩余空间与总空间
# statvfs 返回: (f_bsize, f_frsize, f_blocks, f_bfree, f_bavail, ...)
stat = uos.statvfs("/usr")
free_kb = (stat[0] * stat[3]) // 1024
total_kb = (stat[0] * stat[2]) // 1024
print(f"用户存储区 /usr 剩余: {free_kb} KB / 总共: {total_kb} KB")

# 2. 目录遍历
files = uos.listdir("/usr")
print(f"/usr 下的文件列表: {files}")

# 3. 安全读写文件
with open("/usr/config.json", "w") as f:
    f.write('{"version": "1.0.0", "mode": "production"}')

with open("/usr/config.json", "r") as f:
    cfg = f.read()
    print(f"读取配置: {cfg}")
```

---

## 3. OTA 固件与应用升级 (`fota` & `app_fota`)

### 3.1 应用程序热更新 (`app_fota`)

无需下载庞大的系统固件，仅升级 Python 用户脚本 (`.py` / `.mpy`)，节省大量蜂窝网络流量：

```python
import app_fota
import checkNet

checkNet.wait_network_connected(30)

# 创建 App FOTA 升级实例
# 针对指定的应用升级包 (通常由 QPYcom 生成的打包 zip 或特定差分包)
fota_obj = app_fota.new()

# 从云端下载升级文件写入缓存区
# app_fota 校验通过后重启自动完成替换
# fota_obj.bulk_download(download_url)
```

### 3.2 完整系统固件差分升级 (`fota`)

```python
import fota
import utime

# 初始化 FOTA
fota_inst = fota()

# 写入固件包数据块
# fota_inst.write(firmware_chunk, len(firmware_chunk))

# 校验并触发重启进入 bootloader 升级流程
# ret = fota_inst.verify()
# if ret == 0:
#     Power.powerRestart()
```

---

## 4. 生产级主入口守护进程 (`_main.py` 架构模板)

```python
"""
QuecPython 工业级生产入口模板: /usr/_main.py
"""
import uos
import gc
import utime
import checkNet
from machine import WDT, Pin
import sys_bus

# 1. 硬件看门狗初始化 (30 秒)
wdt = WDT(30)

def main():
    print("========================================")
    print("      QuecPython 嵌入式应用启动")
    print("========================================")
    
    # 2. 垃圾回收与内存打印
    gc.collect()
    print(f"可用内存堆: {gc.mem_free()} bytes")

    # 3. 等待蜂窝注网
    print("等待蜂窝网络连接...")
    stage, state = checkNet.wait_network_connected(60)
    if stage == 3 and state == 1:
        print("网络就绪，启动核心业务线程...")
    else:
        print("警告: 网络连接超时，进入离线工作模式")

    # 4. 主循环喂狗与运行监控
    while True:
        wdt.feed()
        utime.sleep(5)
        # 定期主动回收小对象碎片
        gc.collect()

if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print(f"未捕获的关键异常: {e}")
        # 保存崩溃日志到 /usr/crash.log
        with open("/usr/crash.log", "a") as f:
            f.write(f"Crash at {utime.time()}: {e}\n")
        utime.sleep(2)
        # 由看门狗或系统重启兜底
```
