"""Application composition and long-running lifecycle."""

import gc
import utime
from machine import WDT

from services.network import NetworkService
from services.sms import SmsService
from services.volte import VolteService


class Application:
    def __init__(self, platform, on_sms=None, on_call=None):
        self._platform = platform
        self._on_sms = on_sms
        self._on_call = on_call
        self._network = None
        self._watchdog = None

    def start(self):
        self._network = NetworkService(self._platform)
        self._network.wait_connected(timeout_sec=45)
        SmsService(self._platform, on_message_cb=self._on_sms)
        VolteService(self._platform, event_cb=self._on_call)
        try:
            self._watchdog = WDT(30)
        except Exception as error:
            print("[App] WDT unavailable:", error)

    def run_forever(self):
        self.start()
        print("Telephony services running...")
        loop_count = 0
        while True:
            if self._watchdog:
                self._watchdog.feed()
            if loop_count % 30 == 0:
                csq = self._network.get_signal_csq()
                print("[Status] Signal CSQ: {}, Free RAM: {} B".format(csq, gc.mem_free()))
            loop_count += 1
            utime.sleep(1)
            gc.collect()
