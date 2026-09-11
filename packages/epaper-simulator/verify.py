#!/usr/bin/env python3
"""Run the host and no-panel acceptance checks for the embedded display stack."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "packages/epaper-simulator"


def run(*command):
    subprocess.run(command, cwd=ROOT, check=True)


def main():
    with tempfile.TemporaryDirectory(prefix="spring-epaper-") as directory:
        build = Path(directory) / "build"
        run("cmake", "-S", str(SOURCE), "-B", str(build), "-G", "Ninja")
        run("cmake", "--build", str(build))
        run("ctest", "--test-dir", str(build), "--output-on-failure")
        prefix = Path(directory) / "pattern_"
        run(str(build / "pattern_test"), str(prefix))
        expected = {"white.pbm", "black.pbm", "checkerboard.pbm", "corners_marked.pbm"}
        actual = {path.name for path in Path(directory).glob("pattern_*.pbm")}
        if actual != {f"pattern_{name}" for name in expected}:
            raise RuntimeError(f"calibration output mismatch: {sorted(actual)}")
        for path in Path(directory).glob("pattern_*.pbm"):
            if path.stat().st_size != 15_011:
                raise RuntimeError(f"invalid PBM size: {path}")
    sdkconfig = ROOT / "packages/esp32p4/sdkconfig"
    if "CONFIG_SPRING_DISPLAY_BUFFER_ONLY=y" not in sdkconfig.read_text():
        raise RuntimeError("buffer-only configuration is not enabled")
    firmware = ROOT / "packages/esp32p4/build/the_spring_esp32p4.bin"
    if not firmware.is_file() or firmware.stat().st_size == 0:
        raise RuntimeError("ESP32-P4 firmware binary is missing")
    print("embedded display acceptance checks passed")


if __name__ == "__main__":
    main()
