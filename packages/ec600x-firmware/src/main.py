"""
EC600X-EVB QuecPython 主应用程序
聚焦网络状态监控、SMS 接收处理与 VoLTE 语音服务
"""

import gc

from application import Application
from qpy_platform import QuecPlatform


def handle_incoming_sms(phone: str, content: str) -> None:
    print("[App] Received SMS from {}: {}".format(phone, content))
    # 可在此处理短信交互指令，如根据内容响应回复短信或触发任务


def handle_volte_event(event: int, phone: str, call_id: object) -> None:
    print("[App] VoLTE event: {}, phone: {}, call_id: {}".format(event, phone, call_id))


def run() -> None:
    print("========================================")
    print("     EC600X QuecPython Telephony        ")
    print("========================================")
    gc.collect()
    print("Free memory: {} bytes".format(gc.mem_free()))

    Application(
        QuecPlatform(), on_sms=handle_incoming_sms, on_call=handle_volte_event
    ).run_forever()


if __name__ == "__main__":
    run()
