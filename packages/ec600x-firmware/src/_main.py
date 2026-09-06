"""
QuecPython 入口守护进程: _main.py
开机自动执行，负责看门狗保护、全局异常兜底与启动 main.py
"""

import gc
import uos
import utime
from machine import WDT

def run_app():
    import main
    main.run()

if __name__ == '__main__':
    # 启用硬件看门狗 (30秒超时)
    wdt = None
    try:
        wdt = WDT(30)
    except Exception as e:
        print("[_main] WDT not available:", e)

    try:
        gc.collect()
        run_app()
    except Exception as e:
        print("[_main] Fatal unhandled exception:", e)
        try:
            with open("/usr/crash.log", "a") as f:
                f.write("Crash at {}: {}\n".format(utime.time(), e))
        except Exception:
            pass
        utime.sleep(2)
        raise e
