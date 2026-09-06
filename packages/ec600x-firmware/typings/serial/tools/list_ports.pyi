"""Minimal pyserial port discovery surface used by the deployment CLI."""

class ListPortInfo:
    device: str
    description: str | None

def comports() -> list[ListPortInfo]: ...
