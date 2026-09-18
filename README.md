**this is a fork of [ZimengXiong/tinyTouch](https://github.com/ZimengXiong/tinyTouch)**, based on upstream 0.1.26.
See [changed from upstream](#changed-from-upstream) for what's different here, and [FORK.md](.claude/FORK.md) for how the fork is maintained.

# tinytouch-bzz
Authenticate, sudo, and log in with your fingerprint wire(less)ly without having to spend $149 and have it bzz on finger read with a haptic motor.

Build guide (upstream): https://www.youtube.com/watch?v=YsP1hRg28Gw

## Table of contents

- [red pill or blue pill?](#red-pill-or-blue-pill)
- [install](#install)
  - [flash](#flash)
  - [configure](#configure)
  - [update](#update)
  - [build from source](#build-from-source)
- [hardware](#hardware)
- [wiring](#wiring)
- [recovery](#recovery)
- [cli reference](#cli-reference)
- [notes](#notes)
- [changed from upstream](#changed-from-upstream)

## Red pill or blue pill?

There are two ways to use tinytouch-bzz on your computer: `HID` and `PIV/PAM` mode. Read about how they work in the sections below.

Each has its advantages, and we want to scare you a tiny bit so you actually do
your diligence and understand the security implications of such a device before
you decide whether you are willing to take on the risks:

| features | HID | PIV/PAM* |
| -- | -- | -- |
| keyboardless login | ✅ | ✅ |
| sudo prompts | ✅ | ✅ |
| apple TCC (privacy & security) | ✅ | ✅|
| general settings | ✅ | ❌ |
| keychain/apple passwords | ✅ | ❌ |
| everywhere your password is accepted (remote SSH sessions, etc) | ✅ | depends, but probably not |

| security | HID | PIV/PAM* |
| -- | -- | -- |
| fingerprint sensor <-> esp32| 🔴 (unauth'ed UART) | 🔴 (unauth'ed UART) |
| esp32<-> computer negotiation | 🟢 (shared-key mac/encryption) | 🔴 (plain usb ccid/apdu) |
| authentication | 🔴 (password typed over hid) | 🟢 (piv challenge/response) |

| attack | HID | PIV/PAM* |
| -- | -- | -- |
| sensor uart spoofing^ | yes | yes |
| wrong focused field | yes | no |
| malicious password field | yes | no |
| usb traffic sniffing | low impact (channel is encrypted/mac'ed) | can observe apdus, not piv private key |
| usb keylogger | can reveal password | cannot reveal key |
| usb command injection | reject bad macs/replays | device may receive apdus, but auth still needs fingerprint-gated key use |
| flash dumping (secure boot/flash encryption off) | shared-key exposable | piv key exposable |
| flash dumping (secure boot/flash encryption on) | shared-key non-exportable | piv key non-exportable |
| flash dumping (with secure element) | shared key non-exportable | piv key non-exportable |

*PIV/PAM always uses HID to deliver the mandatory PIV PIN, which we do not use.
authorization is still gated by your fingerprint. the PIV PIN is not your
password, and is not considered sensitive in our scenario.

^This is the major security issue with this device. since all authentication
happens inside the fingerprint sensor, and the sensor communicates with the esp32
over unauthenticated uart, it can be easily spoofed. Basic countermeasures
involve filling the insides of the device with black epoxy. A more proper fix
would be upgrading to a more secure fingerprint sensor.

### So... which pill, if any?
this depends on:

1. Your security tolerance
2. Your environment
3. Current/future criminal background
4. Family/roommate relations
5. Technical skill set of family members/roommates

Risks are low to begin with since every attack here requires *physical access* to
both the device and your Mac.

So ask yourself:
- Will your device ever leave your desk?
- Can your roommates perform a flash dump in half an hour?
- How about your family members?
- Do they have anything against you that would create a motive?
- Are you wanted by any government agency?
- Are you protecting sensitive or classified information?
- Are you using a company device or would you be personally implicated if you leaked company secrets?

If the answer is yes to any of the above questions, I think the Apple's Magic Keyboard presents an excellent value at $149 and is worth the added security.

If the answer is no, chances are you will be fine with a slightly insecure method
of authentication.

### HID mode

In HID mode, the esp32 acts like a usb keyboard:
- The Mac helper keeps your real password encrypted and stored on your Mac
- This way, an attacker cannot extract your password from the esp32 alone
- The esp32 keeps a shared pairing key
- After a fingerprint match, the esp32 sends a signed request to the helper, the helper checks it, encrypts the password for that one request, and sends it back
- The esp32 decrypts it in RAM, types it, then wipes it

This is why it works almost everywhere. It is also why it is scary: the final step is still your real password being typed into whatever has focus.

To make it less bad:
- The esp32 never stores the password
- Requests use a nonce and Mac so old requests cannot just be replayed, and the helper only sends back an encrypted one-time response
- The password only exists on the esp32 briefly in RAM.

### PIV mode

In piv mode, the esp32 acts like a USB Smart Card:
- macOS sends normal piv commands over CCID
- When macOS needs authentication, it asks the card to use the piv private key
- The esp32 only allows that key operation right after a fingerprint match.

macOS also expects a piv pin, so the firmware has a tiny HID side path that types
the dummy pin `111111`. That pin is not your macOS password. It is just there to
get through the macOS PIV prompt while the real authorization is the fingerprint
gate around the PIV key.

This avoids typing your real password, but only works where macOS accepts smart
cards, like login and `sudo` with pam.

## Install

tinytouch-bzz ships as one firmware image ([`firmware/tiny_touch_unified`](firmware/tiny_touch_unified))
that supports both HID and PIV mode. Flash it once, then choose - or later
switch - mode through the `tinytouch` CLI.

Everything runs from a clone of this repo. No ESP-IDF toolchain is needed:

```sh
git clone https://github.com/nicnic-cc/tinytouch-bzz.git
cd tinytouch-bzz
```

### Flash

1. Put the board in download mode: hold **BOOT** while plugging in USB, or
   hold **BOOT**, tap **RESET**, then release **BOOT**.
2. Flash the latest [release](https://github.com/nicnic-cc/tinytouch-bzz/releases/latest):

```sh
./tinytouch flash
```

It downloads the signed release image, checks it against the release
manifest, and writes it with the bundled `esptool`. The first run creates a
private `.venv/` in the repo for its Python dependencies.

Once flashing finishes, **unplug and reconnect USB once.** The
fingerprint sensor stays powered through an MCU reset, so the device won't
respond to setup until it sees a fresh USB reconnect.

### Configure

From the repo root:

```sh
./tinytouch setup
```

Follow the prompts. it walks you through:

- Choosing **PIV** or **HID** mode - see
  [red pill or blue pill?](#red-pill-or-blue-pill) above for the trade-offs
- PIV: creating the on-device PIV identity and pairing it with `sc_auth`
- HID: setting the Keychain password tinyTouch will type, and installing its
  background helper
- Enrolling your fingerprint (four touches, different views)

Switch modes later with `tinytouch mode piv` / `tinytouch mode hid`. See
[cli reference](#cli-reference) below for the full command list.

If setup reports a fingerprint already enrolled, or won't complete at all,
see [recovery](#recovery) below.

### Update

```sh
git pull
./tinytouch update
```

`git pull` updates the CLI and helper. `tinytouch update` then downloads the
latest release firmware and stages it over USB after a fingerprint touch.
Unplug and reconnect the device once to boot it.

### Build from source

Only needed if you want to change the firmware. Follow
[`firmware/README.md`](firmware/README.md) for the ESP-IDF 5.3.x setup, then:

```sh
./firmware/build-and-flash
```

**A self-built image is off the update track.** The device only accepts an
OTA image signed with the same key as the firmware it is running, and your
local build is signed with your own key. `tinytouch update` will be rejected
until you put the device back on a release image with `./tinytouch flash`.

## Hardware

| part | used here | notes |
| -- | -- | -- |
| microcontroller | seeed studio esp32-s3 | needs native usb and hardware uart. secure boot + flash encryption strongly recommended, but release images ship with both off |
| fingerprint sensor | zw101-style uart sensor | uses the common `0xef01` packet protocol |
| computer | macOS | hid mode needs the helper. piv/pam mode needs macOS smart card support |
| case | printed top/bottom stl | `hardware/case/case_top.stl` and `hardware/case/case_bottom.stl` |
| haptic motor (optional) | coin ERM vibration motor module with onboard driver | 3-pin GND/VCC/IN header, buzzes on match/non-match, see [wiring](#wiring) |
| wiring/solder/etc | misc | whatever your build needs |

- Other esp32-s3 boards should work if the usb and uart pins are available
- Other fingerprint sensors may work if they speak the same uart protocol
- Other microcontroller families can work, but are not currently supported.

## Wiring

The fingerprint sensor connects over uart: GPIO43 (tx) and GPIO44 (rx), which
are pins D6 and D7 on the XIAO. The touch interrupt is on GPIO2 (D1).

Use an esp32-s3 super mini or seeed studio xiao esp32-s3 with a zw101-style uart
fingerprint sensor. Use 3.3v power and logic.

### Wire the sensor

| sensor pin | signal | esp32-s3 | XIAO |
| -- | -- | -- | -- |
| 1 | VTouch | 3V3 | 3V3 |
| 2 | TouchOut | GPIO2 | D1 |
| 3 | VCC | 3V3 | 3V3 |
| 4 | TX | GPIO44 (RX) | D7 |
| 5 | RX | GPIO43 (TX) | D6 |
| 6 | GND | GND | GND |

- The UART pair is crossed: sensor TX goes to board RX. sensor RX goes to board TX
- Check continuity. confirm that 3V3 and GND are not shorted before connecting USB

### Wire the haptic motor (optional)

A small vibration motor module (onboard mosfet driver + flyback diode, 3-pin
gnd/vcc/in header) buzzes once on a fingerprint match and twice on a
non-match, including during enrollment and `tinytouch` approval touches.
It's optional - the firmware still works fine without one wired up.

| module pin | esp32-s3 | XIAO |
| -- | -- | -- |
| GND | GND | GND |
| VCC | 5V (if broken out), otherwise 3V3 | 5V, otherwise 3V3 |
| IN | GPIO4 | D3 |

5V gives the strongest, most reliable buzz; 3.3v will likely still work but
may feel weaker.

## Recovery

Recovery erases fingerprints, keys, pairings, settings, and firmware state.
Try this first:

```sh
tinytouch status --verbose
tinytouch factory-reset
```

If the device still can't be set up - it's unresponsive, or setup keeps
failing after a factory reset - erase it and reflash the release firmware:

1. Disconnect tinyTouch.
2. Put it in download mode: hold **BOOT** while reconnecting USB, or hold
   **BOOT**, tap **RESET**, then release **BOOT**.
3. Erase and reflash it:
   ```sh
   ./tinytouch flash --erase
   ```
4. Unplug and reconnect the device, wait 20 seconds, then run
   `tinytouch setup`.

Use `tinytouch update` for routine firmware updates - recovery is only for
when a factory reset isn't enough.

## CLI reference

Run `tinytouch <command> --help` for command help. Most commands accept
`--port /dev/cu.usbmodem...` to pick a device; the top-level `--verbose` flag
shows diagnostic output.

### Setup

```sh
tinytouch setup
tinytouch setup --mode piv
tinytouch setup --mode hid
tinytouch setup --skip-enroll
tinytouch mode piv
tinytouch mode hid
```

### Status

```sh
tinytouch status
tinytouch status --verbose
tinytouch test
tinytouch logs
```

### Fingerprints

```sh
tinytouch enroll 1
tinytouch delete 1
```

### PIV

```sh
tinytouch pair
tinytouch keys
```

### HID

```sh
tinytouch computers
tinytouch computers remove <host-id>
tinytouch config typing_delay_ms 12
tinytouch config submit_enter off
tinytouch config touch_cooldown_ms 1000
```

### Firmware

```sh
tinytouch update
tinytouch flash
tinytouch flash --erase
tinytouch flash --release-version 1.0.0-prod
tinytouch rom
tinytouch factory-reset
```

`tinytouch flash` writes a release image to a device in download mode.
`tinytouch rom` (alias `tinytouch bootloader`) just reminds you how to get
the device into download mode first.

## Notes

[cad](https://cad.onshape.com/documents/d0e6bb7977e6171d4e4a5086/w/1ded27ad6c634fd1fdaf26d0/e/aca67210e400490a08d0b29a?renderMode=0&uiState=6a4c1df32e292f12144a65fe). if you make changes, please make them open source as well.

## Changed from upstream

- No docs site, web flasher, or hosted API. Releases live on this repo's GitHub releases page, `tinytouch flash` replaces the web flasher, and `tinytouch update` run from a clone updates firmware only and leaves the CLI to `git pull`

- Fingerprint sensor LED is off during normal idle/waiting-for-touch and after match results, instead of staying lit (`firmware/tiny_touch_unified/main/fingerprint.c`, `set_aura_off`/`fingerprint_led_idle`)
  - On successful sensor init it flashes white once (`fingerprint_led_connect_flash`) to confirm the device came up correctly, then goes dark
  - Match/enroll attempts still flash green (success) or red (failure) before returning to off
  - Enrollment still lights white while waiting for a touch
- Normal PIV logins now allow the 9d (key management) slot to be used twice per touch instead of once (`firmware/tiny_touch_unified/main/piv.c`, `piv_note_user_presence`/`handle_general_authenticate`).
  - macOS unwraps the Login Keychain secret with a second, separate 9d decrypt beyond the initial PIV auth
  - The old one-operation-per-slot limit rejected that second call with SW 6982, so macOS silently fell back to prompting for the keychain password on every unlock even after a successful `sc_auth pair`
  - 9a (auth) still gets one operation per touch, and the pairing/configuration window is unchanged
- Added optional haptic feedback for a coin vibration motor module wired to GPIO4 (`firmware/tiny_touch_unified/main/haptic.c`):
  - One buzz on a fingerprint match, two short buzzes on a non-match
  - This also fixes a real gap - the everyday HID/PIV touch path (`touch_pin_hid.c`) previously gave no feedback at all on a failed read, silently waiting for the next touch 
  - The motor is optional, without one wired up the GPIO just toggles unobserved