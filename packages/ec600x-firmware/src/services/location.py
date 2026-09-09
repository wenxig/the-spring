"""
基站定位服务
通过 cellLocator 基站定位 API 获取当前经纬度坐标
"""

try:
    import typing
except ImportError:
    _type_checking = False
else:
    _type_checking = typing.TYPE_CHECKING

if _type_checking:
    from type_contracts import CellLocatorModule


class LocationService:
    def __init__(self, cell_locator: "CellLocatorModule") -> None:
        self._locator = cell_locator

    def get_position(
        self,
        server: str = "www.queclocator.com",
        port: int = 80,
        token: str = "",
        timeout: int = 10,
        profile_idx: int = 1,
    ) -> "tuple[float, float, int] | None":
        """
        获取基站定位坐标
        返回: (纬度, 经度, 精度米) 或 None 表示定位失败
        """
        try:
            lat, lng, accuracy = self._locator.getLocation(
                server, port, token, timeout, profile_idx
            )
            print(
                "[Location] Position: lat={:.6f}, lng={:.6f}, accuracy={}m".format(
                    lat, lng, accuracy
                )
            )
            return (lat, lng, accuracy)
        except Exception as e:
            print("[Location] Failed to get position:", e)
            return None
