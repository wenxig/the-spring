"""Build a deterministic QuecPython upload archive on the host."""

import argparse
import hashlib
import shutil
import zipfile
from pathlib import Path


PACKAGE_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = PACKAGE_ROOT / "src"
DIST_ROOT = PACKAGE_ROOT / "dist"
STAGE_ROOT = DIST_ROOT / "ec600x-firmware"
ARCHIVE = DIST_ROOT / "ec600x-firmware.zip"
CHECKSUMS = DIST_ROOT / "SHA256SUMS"


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def build():
    if DIST_ROOT.exists():
        shutil.rmtree(DIST_ROOT)
    STAGE_ROOT.mkdir(parents=True)
    for source in SOURCE_ROOT.rglob("*"):
        if not source.is_file() or source.suffix == ".pyc":
            continue
        destination = STAGE_ROOT / source.relative_to(SOURCE_ROOT)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)

    with zipfile.ZipFile(ARCHIVE, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for source in sorted(STAGE_ROOT.rglob("*")):
            if source.is_file():
                archive.write(source, source.relative_to(DIST_ROOT).as_posix())

    CHECKSUMS.write_text("{}  {}\n".format(sha256(ARCHIVE), ARCHIVE.name), encoding="ascii")
    print("Built {}".format(ARCHIVE))
    print("SHA-256: {}".format(sha256(ARCHIVE)))


def verify():
    if not ARCHIVE.is_file() or not CHECKSUMS.is_file():
        raise SystemExit("Build output is missing; run `python tools/pack.py build` first")
    expected = CHECKSUMS.read_text(encoding="ascii").split()[0]
    actual = sha256(ARCHIVE)
    if expected != actual:
        raise SystemExit("Checksum mismatch: expected {}, got {}".format(expected, actual))
    with zipfile.ZipFile(ARCHIVE) as archive:
        broken = archive.testzip()
    if broken:
        raise SystemExit("Corrupt archive member: {}".format(broken))
    print("Verified {}".format(ARCHIVE))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("build", "verify"))
    args = parser.parse_args()
    {"build": build, "verify": verify}[args.command]()


if __name__ == "__main__":
    main()
