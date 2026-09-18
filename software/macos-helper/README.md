# tinyTouch macOS helper

Background service that lets a tinyTouch device (in HID mode) type your
password for you. It talks to the ESP32 over USB serial, authenticates each
fingerprint-match event, and — only after that check passes — hands back an
encrypted, one-time password that gets typed via a virtual HID keyboard.

PIV mode does not use any of this; macOS smart-card support handles auth
directly. This helper only runs when a device is paired in HID mode.

## Files

| file | purpose |
| -- | -- |
| `tinytouch_helper.py` | entry point. Serial protocol, event auth, HID key injection, multi-device manager |
| `tinytouch_keychain.py` | thin `Security.framework` wrapper (via `ctypes`) for reading/writing Keychain items |
| `tinytouch_runtime.py` | shared low-level bits: atomic file writes, the single-instance lease, backoff policy, serial frame decoding, structured diagnostics |
| `requirements.txt` | runtime deps for running the helper from source (`pyserial`, `esptool`, `certifi`) |
| `requirements-bootstrap.txt` / `requirements-release.txt` | pinned, hash-locked deps used only by the release build ([../../packaging/build-standalone-macos.sh](../../packaging/build-standalone-macos.sh)) to produce the standalone `tinytouch` binary via PyInstaller |

## How it's installed (you shouldn't `pip install` this by hand)

Nobody is expected to run `pip install -r requirements.txt` against their
system Python. There are two supported paths, and both avoid touching global
site-packages:

- **Production / distributed CLI**: the release `tinytouch` binary is a
  PyInstaller bundle with `pyserial`, `esptool`, etc. already baked in. It
  runs the helper as `tinytouch _helper` — no separate interpreter or
  install step at all.
- **Running from source** (development, or `./tinytouch setup` before a
  release binary exists): `ensure_helper_environment()` in the root
  [`tinytouch`](../../tinytouch) script creates a private `.venv/` at the repo
  root the first time it's needed and installs `requirements.txt` into
  *that*, never into your system or shell Python. `install_helper()` then
  points the LaunchAgent's `ProgramArguments` at that venv's `python` and
  `tinytouch_helper.py` directly.

So in both cases the helper ends up running out of an isolated interpreter.
If you're hacking on this code directly (e.g. running `tinytouch_helper.py`
or the test suite without going through `./tinytouch setup`), do the same —
create your own venv rather than installing into system Python:

```sh
python3 -m venv .venv
.venv/bin/pip install -r software/macos-helper/requirements.txt
.venv/bin/python software/macos-helper/tinytouch_helper.py --self-test
```

`.venv/` at the repo root is already gitignored.

## What it does

1. Enumerates connected tinyTouch serial ports (`device_ports` /
   `device_endpoints`) and spawns one `Worker` thread per device, managed by
   `run_manager()` — so multiple paired devices can be plugged in at once.
2. Each worker calls `serve_port()`, which reads newline-delimited frames off
   serial (`SerialFrameDecoder` in `tinytouch_runtime.py`) and looks for
   fingerprint-match events (`EV ...` / `EV2 ...`).
3. `parse_event()` validates and HMAC-authenticates the event against the
   pairing key stored in Keychain, and `handle_event()` rejects anything
   with a bad MAC, an out-of-range field, or a replayed nonce
   (`state["seen_nonces"]`, capped at `MAX_SEEN_NONCES`).
4. On a valid event, the plaintext password for that fingerprint slot is
   pulled from Keychain, re-encoded for the device's configured keyboard
   layout (`translate_password`), and encrypted with AES-CTR
   (`aes_ctr_crypt`, via CommonCrypto through `ctypes` — no third-party
   crypto library needed for this part) under a session key derived from the
   pairing key and the event's nonce.
5. The encrypted reply (`PW ...` / `PW2 ...`, MAC'd the same way) is written
   back over serial; the firmware decrypts it locally and types it over USB
   HID. The plaintext password never crosses the wire unencrypted, and each
   reply is only valid for the nonce that requested it.

`tinytouch_runtime.py`'s `ForegroundLease`/`LeaseObserver` machinery exists
so only one helper instance drives the device at a time — e.g. the
foreground `tinytouch` CLI can ask the background LaunchAgent to stand down
(`helper-suspend`) while it does something like pairing or firmware updates,
then hand control back.

## Running standalone

```sh
.venv/bin/python tinytouch_helper.py [--port /dev/tty...] [--once] [--self-test] [--device-id ID]
```

Normally you don't invoke this directly — `./tinytouch setup` (HID mode)
installs it as a LaunchAgent that starts it automatically. See the root
[README](../../README.md) for the end-user setup flow.
