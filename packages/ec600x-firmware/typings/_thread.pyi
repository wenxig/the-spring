"""QuecPython _thread API used by the EC600M firmware."""

from typing import Callable

def start_new_thread(function: Callable[..., object], args: tuple[object, ...]) -> int: ...
