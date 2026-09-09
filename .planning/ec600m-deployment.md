# EC600M Deployment Investigation

## Task And Commit Boundaries

1. Record the verified host and device diagnostics in one documentation commit.
2. Establish a supported base-firmware download path, install QuecPython, and verify the runtime.
3. Deploy the application and capture network and location test results; commit any required application fixes separately.

## Verified On 2026-09-09

- macOS enumerates USB VID `2c7c`, PID `6002`, product `Android`.
- Interface 0/1 is CDC ECM; interface 2 is diagnostics; interfaces 3/4 are vendor-specific AT interfaces.
- macOS network service `Android` remains disabled at the user's request.
- `192.168.1.1` routes through Wi-Fi `en0` and gateway `192.168.50.1`. Its web page does not establish a connection to this module.
- `/dev/cu.URT0` belongs to the Mac's onboard UART.
- libusb 1.0.30 can open this module and claim interfaces 2, 3 and 4 without a serial driver.
- Interface 3 bulk OUT `0x0f` / IN `0x86` and interface 4 bulk OUT `0x0a` / IN `0x81` respond to `AT` and `ATI`.
- `ATI` and `AT+QGMR` return `EC600M`, revision `EC600MCNLER06A08M08`.
- Raw REPL probing on interface 3 produced no response. A QuecPython runtime has not been verified.
- `AT+QCFG="usbnet"` returns `1`; the supported values reported by `AT+QCFG=?` are `1,3`.
- Attempting `AT+QCFG="usbnet",0` returned `+CME ERROR: 50`; subsequent query still returned `1`.
- `usbdesc`, `usbcfg` and `usbmode` queries returned `+CME ERROR: 4`.
- `AT+QCFG="usbifc"` returns `0,0`; `AT+QCFG="usbauto"` returns `0`. Their setting semantics remain unverified.
- No successful persistent device configuration change or application upload occurred.
- Temporary Alpine containers performed host-device discovery. No Quectel USB passthrough was established. Containers were removed on exit.

## Prepared Firmware

Official resource: <https://developer.quectel.com/resource-download/qpy_ocpu_ec600m_cnle_fw>

- Package: `QPY_OCPU_EC600M_CNLE_FW_V0006.zip`.
- Local cache: `.cache/ec600m-firmware/QPY_OCPU_EC600M_CNLE_FW_V0006.zip` (Git ignored).
- Size: 12,017,288 bytes.
- SHA-256: `0ba85f453da174db615863a698ae7538dfc314e075f709b2e2ab32716fff41cb`.
- ZIP integrity check passed. This locally calculated hash records the retrieved artifact; it is not a vendor signature.
- Contains `EC600MCNLER06A06M08_OCPU_QPY.zip` and release notes.
- Base-firmware compatibility and the download path must be verified before flashing.

## Remaining Work

- Establish USB access for a supported base-firmware downloader. Docker Desktop currently has no passthrough for this module.
- The public [QuecPython download tool](https://github.com/QuecPython/download_tool) invokes `adownload.exe` for EC600M/ASR. Official Linux QPYcom packages also exist; their EC600M flashing capability has not been verified.
- Confirm the meaning of `usbifc` from model-specific documentation before changing it.
- Verify QuecPython REPL after base-firmware installation, then rebuild, upload and observe the application network/location tests.

## Host Checks

- `uv run --with pytest pytest`: 4 tests passed.
- `uv run ruff check src tools tests`: passed (the package's configured lint scope).
- `uv run --with ruff ruff check .`: 39 existing findings in API stubs under `typings`.
- `vp check`: existing formatting findings in root/package manifests, `pyproject.toml` and `pnpm-workspace.yaml`.
- `vp test`: no files matched the root `script/**/*.test.ts` configuration; exit code 1.

## USB/IP runtime probe update (2026-09-09)

- Rebuilt the temporary USB/IP host with interface claim handling for modem interfaces 2-4 and shorter transfer timeouts.
- With the host server held open, Docker Linux attached `20-10-1`; `/dev/ttyUSB0..2` appeared.
- `ttyUSB1` and `ttyUSB2` both answered `AT` with `AT\r\r\nOK\r\n`; `ttyUSB0` had no response.
- After the USB/IP host process/session ended, the imported device detached and serial nodes disappeared.
- A Raw REPL probe was not completed before the session dropped. No application files were uploaded and no base firmware was flashed.
