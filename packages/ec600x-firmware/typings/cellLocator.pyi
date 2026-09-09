"""QuecPython cellLocator API used by the EC600M firmware."""

from typing import Protocol

class _CellLocatorModule(Protocol):
    def getLocation(
        self,
        server_addr: str,
        port: int,
        token: str,
        timeout: int,
        profile_idx: int,
    ) -> tuple[float, float, int]: ...

def getLocation(
    server_addr: str,
    port: int,
    token: str,
    timeout: int,
    profile_idx: int,
) -> tuple[float, float, int]: ...
