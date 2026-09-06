"""
EC600X-EVB QuecPython 主应用程序
聚焦网络状态监控、SMS 接收处理与 VoLTE 语音服务
"""

import gc
import utime
from machine import WDT

from services.network import NetworkService
from services.sms import SmsService
from services.volte import VolteService

def handle_incoming_sms(phone, content):
    print("[App] Received SMS from {}: {}".format(phone, content))
    # 可在此处理短信交互指令，如根据内容响应回复短信或触发任务

def handle_volte_event(event, phone, call_id):
    print("[App] VoLTE event: {}, phone: {}, call_id: {}".format(event, phone, call_id))

def run():
    print("========================================")
    print("     EC600X QuecPython Telephony        ")
    print("========================================")
    gc.collect()
    print("Free memory: {} bytes".format(gc.mem_free()))

    # 1. 蜂窝网络服务
    net_service = NetworkService()
    net_service.wait_connected(timeout_sec=45)

    # 2. 短信服务
    sms_service = SmsService(on_message_cb=handle_incoming_sms)

    # 3. VoLTE 通话服务
    volte_service = VolteService(auto_answer=False, event_cb=handle_volte_event)

    # 4. 看门狗
    wdt = None
    try:
        wdt = WDT(30)
    except Exception:
        pass

    # 5. 主事件循环
    print("Telephony services running...")
    loop_count = 0
    while True:
        if wdt:
            wdt.feed()

        # 每 30 秒轮询一次信号质量 (CSQ)
        if loop_count % 30 == 0:
            csq = net_service.get_signal_csq()
            print("[Status] Signal CSQ: {}, Free RAM: {} B".format(csq, gc.mem_free()))

        loop_count += 1
        utime.sleep(1)
        gc.collect()

if __name__ == '__main__':
    run()
