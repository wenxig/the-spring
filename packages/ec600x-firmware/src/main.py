"""
EC600X-EVB QuecPython 4G 注册与基站定位测试程序
"""

import gc

import utime

from qpy_platform import QuecPlatform
from services.location import LocationService
from services.network import NetworkService


def run() -> None:
    print("========================================")
    print("  EC600X 4G Registration & Location Test")
    print("========================================")
    gc.collect()
    print("Free memory: {} bytes".format(gc.mem_free()))

    platform = QuecPlatform()

    # 4G 网络注册测试
    print("\n[Test 1/2] Testing 4G network registration...")
    network = NetworkService(platform)
    if not network.wait_connected(timeout_sec=60):
        print("[FAIL] Network registration timeout")
        return

    csq = network.get_signal_csq()
    print("[OK] Signal CSQ: {}".format(csq))

    cell_info = network.get_cell_info()
    print("[OK] Cell info: {}".format(cell_info))

    # 基站定位测试
    print("\n[Test 2/2] Testing cell tower location...")
    location = LocationService(platform.cell_locator)
    position = location.get_position(timeout=15)

    if position is None:
        print("[FAIL] Location service unavailable")
    else:
        lat, lng, accuracy = position
        print("[OK] Position: lat={:.6f}, lng={:.6f}, accuracy={}m".format(lat, lng, accuracy))

    print("\n========================================")
    print("All tests completed. Module will stay alive for monitoring.")
    print("Press Ctrl+C to exit.")
    print("========================================")

    # 保持运行并周期性报告状态
    loop_count = 0
    while True:
        if loop_count % 60 == 0:
            csq = network.get_signal_csq()
            print("[Status] CSQ: {}, Free RAM: {} bytes".format(csq, gc.mem_free()))
        loop_count += 1
        utime.sleep(1)
        gc.collect()


if __name__ == "__main__":
    run()
