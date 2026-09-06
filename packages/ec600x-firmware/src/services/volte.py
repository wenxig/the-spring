"""
VoLTE 语音通话服务 (voiceCall)
管理 VoLTE 呼入接听、呼出拨号、挂断与事件回调状态机
"""

import voiceCall

# QuecPython voiceCall 状态码常量
CALL_STATE_INCOMING = 10     # 来电通知（振铃）
CALL_STATE_CONNECTED = 11    # 通话已接通
CALL_STATE_DISCONNECTED = 12 # 通话已挂断
CALL_STATE_WAITING = 13      # 呼叫等待
CALL_STATE_DIALING = 14      # 呼出中
CALL_STATE_ALERTING = 15     # 对方振铃中
CALL_STATE_HOLDING = 16      # 呼叫保持

class VolteService:
    def __init__(self, auto_answer=False, event_cb=None):
        self._auto_answer = auto_answer
        self._event_cb = event_cb
        self._current_call_id = None
        self._current_phone = None
        self._state = None
        self._register_callback()

    def _register_callback(self):
        def _cb(args):
            if not isinstance(args, (tuple, list)):
                return
            event = args[0]
            if event < 10:
                return

            phone = args[6] if len(args) > 6 else ""
            call_id = args[1] if len(args) > 1 else None

            self._state = event
            self._current_phone = phone

            if event == CALL_STATE_INCOMING:
                self._current_call_id = call_id
                print("[VoLTE] Incoming call from: {} (call_id: {})".format(phone, call_id))
                if self._auto_answer:
                    print("[VoLTE] Auto-answering call...")
                    self.answer()

            elif event == CALL_STATE_CONNECTED:
                print("[VoLTE] Call connected with {}".format(phone))

            elif event == CALL_STATE_DISCONNECTED:
                print("[VoLTE] Call disconnected with {}".format(phone))
                self._current_call_id = None
                self._current_phone = None

            elif event == CALL_STATE_DIALING:
                print("[VoLTE] Dialing {}".format(phone))

            elif event == CALL_STATE_ALERTING:
                print("[VoLTE] Remote alerting: {}".format(phone))

            if self._event_cb:
                self._event_cb(event, phone, call_id)

        try:
            voiceCall.setCallback(_cb)
            print("[VoLTE] Callback registered.")
        except Exception as e:
            print("[VoLTE] Failed to register callback:", e)

    def call(self, phone_number):
        """发起拨号"""
        try:
            print("[VoLTE] Calling {}...".format(phone_number))
            ret = voiceCall.callStart(phone_number)
            return ret == 0
        except Exception as e:
            print("[VoLTE] Call failed:", e)
            return False

    def answer(self):
        """接听当前来电"""
        try:
            ret = voiceCall.callAnswer()
            return ret == 0
        except Exception as e:
            print("[VoLTE] Answer failed:", e)
            return False

    def hangup(self):
        """挂断电话"""
        try:
            ret = voiceCall.callEnd()
            return ret == 0
        except Exception as e:
            print("[VoLTE] Hangup failed:", e)
            return False

    def set_volume(self, volume):
        """设置通话音量 (0~11)"""
        try:
            voiceCall.setVolume(volume)
        except Exception as e:
            print("[VoLTE] Set volume error:", e)

    def set_channel(self, channel=0):
        """切换音频通道 (0: 听筒/喇叭, 1: 耳机)"""
        try:
            voiceCall.setChannel(channel)
        except Exception as e:
            print("[VoLTE] Set channel error:", e)
