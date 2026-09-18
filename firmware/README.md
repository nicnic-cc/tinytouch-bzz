# Building the firmware on macOS

tinyTouch's firmware requires **ESP-IDF v5.3.x** specifically — other versions
are not supported (`build-and-flash` checks this and will refuse to build
otherwise).

## 1. Install required packages

```sh
brew install cmake ninja dfu-util
```

## 2. Clone ESP-IDF

```sh
mkdir -p ~/esp
cd ~/esp
git clone -b release/v5.3 --recursive https://github.com/espressif/esp-idf.git
```

## 3. Run the ESP-IDF install script

```sh
cd ~/esp/esp-idf
./install.sh esp32
```

## 4. Activate the environment

This only applies to the current shell session — you'll need to re-run it in
every new terminal/session you build from:

```sh
. $HOME/esp/esp-idf/export.sh
```

## 5. Generate a local app-signing key

The build signs app binaries (this is *not* hardware secure boot — that stays
off; `CONFIG_SECURE_BOOT` is unset). The key is gitignored and per-checkout,
so on a fresh clone you need to generate your own once:

```sh
espsecure.py generate_signing_key --version 2 \
  firmware/tiny_touch_unified/secure_boot_signing_key.pem
```

Skip this if you're only flashing pre-built binaries (run `./tinytouch flash`
for the latest release) rather than building from source.

**This key decides which updates the device accepts.** An OTA image is only
accepted if it is signed with the same key as the firmware already running.
A device flashed with your own build therefore rejects `tinytouch update`
from the public releases. Run `./tinytouch flash` to move it back onto the
release track.

## 6. Build (and optionally flash)

With the environment activated, from the repo root:

```sh
./firmware/build-and-flash --build-only
```

Drop `--build-only` to also flash a connected ESP32-S3 board.

For full setup details or troubleshooting, see Espressif's own docs:
https://docs.espressif.com/projects/esp-idf/en/release-v5.3/esp32/get-started/linux-macos-setup.html

---

This is based on one working build/flash cycle — if something above doesn't
match your experience, flag it so we can fix the doc.
