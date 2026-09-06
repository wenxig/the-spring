"""
短信服务 (SMS)
支持文本/UCS2长短信发送、来信异步通知与短信指令解析
"""

import _thread

try:
    import typing
except ImportError:
    _type_checking = False
else:
    _type_checking = typing.TYPE_CHECKING

if _type_checking:
    from type_contracts import CallbackArgs, Platform, SmsCallback


class SmsService:
    def __init__(
        self,
        platform: "Platform",
        on_message_cb: "SmsCallback | None" = None,
    ) -> None:
        self._sms = platform.sms
        self._on_message_cb = on_message_cb
        self._init_storage()
        self._register_callback()

    def _init_storage(self) -> None:
        try:
            # 统一存储到 SIM 卡 ("SM")
            self._sms.setSaveLoc("SM", "SM", "SM")
            print("[SMS] Storage configured: SM")
        except Exception as e:
            print("[SMS] Failed to set storage loc:", e)

    def _register_callback(self) -> None:
        def _cb(args: "CallbackArgs") -> None:
            # args: (sim_id, index, storage)
            if len(args) < 2 or not isinstance(args[1], int):
                return
            sim_id = args[0]
            index = args[1]
            print("[SMS] Incoming SMS on SIM {}, index {}".format(sim_id, index))
            _thread.start_new_thread(self._handle_incoming, (index,))

        try:
            self._sms.setCallback(_cb)
        except Exception as e:
            print("[SMS] Failed to register SMS callback:", e)

    def _handle_incoming(self, index: int) -> None:
        try:
            res = self._sms.searchTextMsg(index)
            if isinstance(res, tuple):
                phone, content, length = res
                print("[SMS] From: {}, Length: {}, Content: {}".format(phone, length, content))
                if self._on_message_cb:
                    self._on_message_cb(phone, content)
            else:
                # 尝试以 PDU 解析
                pdu_hex = self._sms.searchPduMsg(index)
                if isinstance(pdu_hex, (str, bytes)):
                    pdu_len = self._sms.getPduLength(pdu_hex)
                    decoded = self._sms.decodePdu(pdu_hex, pdu_len)
                    phone, content = decoded[0], decoded[1]
                    print("[SMS PDU] From: {}, Content: {}".format(phone, content))
                    if self._on_message_cb:
                        self._on_message_cb(phone, content)
            # 处理完成后删除短信，防止存储满溢
            self._sms.deleteMsg(index, 0)
        except Exception as e:
            print("[SMS] Error processing message index {}: {}".format(index, e))

    def send_text(self, phone_number: str, message: str, encoding: str = "UCS2") -> bool:
        """发送短信，中文必须为 UCS2，英文数字可用 GSM"""
        try:
            ret = self._sms.sendTextMsg(phone_number, message, encoding)
            if ret == 0:
                print("[SMS] Sent message successfully to {}".format(phone_number))
                return True
            else:
                print("[SMS] Send message failed with code {}".format(ret))
                return False
        except Exception as e:
            print("[SMS] Send error:", e)
            return False
