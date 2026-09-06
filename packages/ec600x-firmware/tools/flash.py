"""CLI flashing / sync tool for QuecPython devices over Serial REPL."""

import argparse
import os
import time
from contextlib import suppress
from pathlib import Path
from typing import TypeAlias

import serial
import serial.tools.list_ports

PACKAGE_ROOT = Path(__file__).resolve().parents[1]
DIST_SRC = PACKAGE_ROOT / "dist" / "ec600x-firmware"
RAW_REPL_ENTER = b"\r\x03\x03\x01"  # Ctrl-C twice, then Ctrl-A (enter raw repl)
RAW_REPL_EXIT = b"\r\x02"  # Ctrl-B (exit raw repl)


FileUpload: TypeAlias = tuple[Path, str]


def find_default_port() -> str | None:
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        desc = (p.description or "").lower()
        dev = p.device.lower()
        if "quectel" in desc or "ec600" in desc or "usbmodem" in dev or "usbserial" in dev:
            return p.device
    return None


class QuecSerialClient:
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 3.0) -> None:
        self.ser = serial.Serial(port=port, baudrate=baudrate, timeout=timeout)

    def close(self) -> None:
        with suppress(Exception):
            self.ser.close()

    def enter_raw_repl(self) -> None:
        # Flush buffers
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()

        # Send Ctrl-C x2, Ctrl-A
        self.ser.write(RAW_REPL_ENTER)
        time.sleep(0.1)
        response = self.ser.read_until(b"raw REPL; CTRL-B to exit\r\n>")
        if b"raw REPL; CTRL-B to exit\r\n>" not in response:
            # Try once more
            self.ser.write(b"\r\x03\x01")
            time.sleep(0.2)
            response += self.ser.read(100)
            if b"raw REPL" not in response and b">" not in response:
                raise RuntimeError(
                    f"Failed to enter raw REPL on port {self.ser.port}. Response: {response!r}"
                )

    def exit_raw_repl(self) -> None:
        self.ser.write(RAW_REPL_EXIT)
        time.sleep(0.1)

    def exec_raw(self, code: str) -> str:
        # Send code followed by Ctrl-D
        code_bytes = code.encode("utf-8") + b"\x04"
        self.ser.write(code_bytes)

        # Read OK marker
        ok = self.ser.read(2)
        if ok != b"OK":
            raise RuntimeError(f"Device did not acknowledge execution start: {ok!r}")

        # Read until output ends with '\x04>'
        data = bytearray()
        while True:
            chunk = self.ser.read(1)
            if not chunk:
                break
            data.extend(chunk)
            if data.endswith(b"\x04>"):
                break

        res = bytes(data[:-2])
        parts = res.split(b"\x04", 1)
        out = parts[0].decode("utf-8", errors="replace")
        err = parts[1].decode("utf-8", errors="replace") if len(parts) > 1 else ""

        if err.strip():
            raise RuntimeError(f"Execution failed:\n{err}")
        return out

    def mkdir(self, remote_dir: str) -> None:
        py_code = f"""
try:
    import uos as os
except:
    import os
try:
    os.mkdir('{remote_dir}')
except:
    pass
"""
        self.exec_raw(py_code)

    def write_file(self, local_path: Path, remote_path: str) -> None:
        content = local_path.read_bytes()
        chunk_size = 256
        total = len(content)

        # Open remote file in write-binary mode
        init_code = f"__f = open('{remote_path}', 'wb')"
        self.exec_raw(init_code)

        try:
            for offset in range(0, total, chunk_size):
                chunk = content[offset : offset + chunk_size]
                chunk_repr = repr(chunk)
                write_code = f"__f.write({chunk_repr})"
                self.exec_raw(write_code)
        finally:
            self.exec_raw("__f.close(); del __f")


def flash(port: str, baudrate: int = 115200, target_prefix: str = "/usr") -> None:
    if not DIST_SRC.exists():
        raise SystemExit(
            f"Build directory {DIST_SRC} not found. Run `python tools/pack.py build` first."
        )

    print(f"Connecting to {port} @ {baudrate} baud...")
    client = QuecSerialClient(port=port, baudrate=baudrate)

    try:
        print("Entering Raw REPL...")
        client.enter_raw_repl()

        # Collect files to upload
        files_to_upload: list[FileUpload] = []
        dirs_to_create: set[str] = set()

        for root, _dirs, files in os.walk(DIST_SRC):
            rel_root = Path(root).relative_to(DIST_SRC)
            if str(rel_root) != ".":
                dirs_to_create.add(f"{target_prefix}/{rel_root.as_posix()}")

            for f in sorted(files):
                if f.endswith(".pyc") or f == ".DS_Store":
                    continue
                loc_path = Path(root) / f
                rel_path = loc_path.relative_to(DIST_SRC)
                rem_path = f"{target_prefix}/{rel_path.as_posix()}"
                files_to_upload.append((loc_path, rem_path))

        for d in sorted(dirs_to_create):
            print(f"Creating remote directory {d}...")
            client.mkdir(d)

        for loc_path, rem_path in files_to_upload:
            size = loc_path.stat().st_size
            print(f"Uploading {loc_path.name} -> {rem_path} ({size} bytes)...")
            client.write_file(loc_path, rem_path)

        print("\nAll files successfully uploaded!")
        print("Soft rebooting module...")
        with suppress(Exception):
            client.exec_raw("import machine; machine.reset()")
    finally:
        client.exit_raw_repl()
        client.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("flash", "ports", "run"))
    parser.add_argument("-p", "--port", help="Serial port (e.g. /dev/cu.usbmodem...)")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default 115200)")
    parser.add_argument(
        "--target-prefix", default="/usr", help="Module storage destination (default /usr)"
    )
    args = parser.parse_args()

    if args.command == "ports":
        ports = list(serial.tools.list_ports.comports())
        if not ports:
            print("No serial ports detected.")
            return
        for p in ports:
            print(f"- {p.device} ({p.description or 'No description'})")
        return

    port = args.port or find_default_port()
    if not port:
        raise SystemExit("No serial port specified and none auto-detected. Use -p <port>.")

    if args.command == "flash":
        flash(port, baudrate=args.baud, target_prefix=args.target_prefix)


if __name__ == "__main__":
    main()
