"""QuecPython cellLocator API used by the EC600M firmware."""

def getLocation(
    server_addr: str,
    port: int,
    token: str,
    timeout: int,
    profile_idx: int,
) -> tuple[float, float, int]: ...
