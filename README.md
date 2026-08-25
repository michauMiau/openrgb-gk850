# OpenRGB GK850W Plugin

OpenRGB plugin for the **BY Tech / Mad Dog GK850(W)** keyboard with full control over all 19 hardware effects, custom colors, and per-key painting. Ships as a standalone plugin using the Virtual Controller API.

## Features

- ✅ **All 19 hardware effects** with working custom color selection.
- 🎲 **Random Color mode** on every effect that supports it.
- 💡 Brightness and speed control (mapped to vendor nibbles)
- 🎨 **Custom / per-key mode** for painting patterns and artwork across the 61-key layout.
- 🔌 Plug-and-play: auto-detection via VID/PID + product string verification.
- 🏷️ Manual save button persists current state to keyboard flash.

### Known limitation

Per-key updates briefly flash black, this is a firmware behavior of the Sinowealth BY916 chip, not a plugin bug (the vendor application flickers identically). There is no realtime/direct mode in this keyboard; built in keyboard effects run at hardware speed instead.

## Device Identification

- **VID:PID**: `258a:0049` (Sinowealth BY916 chip)
- **Product string**: `GK850` — distinguishes it from the FL eSports F11, which shares the same PID

> ⚠️ **Brick risk note:** PID `0x0049` is shared with FL eSports F11 and some Redragon devices. The upstream OpenRGB detection was disabled over this. This plugin only talks to keyboards whose product string is `GK850`, and ships as a standalone plugin — nothing in the main OpenRGB repo is patched.

## Installation

### Linux

1. Install the udev rule so OpenRGB can talk to the keyboard without root:

```bash
sudo tee /etc/udev/rules.d/99-gk850w-plugin.rules << 'EOF'
SUBSYSTEMS=="usb|hidraw", ATTRS{idVendor}=="258a", ATTRS{idProduct}=="0049", TAG+="uaccess"
EOF
sudo udevadm control --reload-rules && sudo udevadm trigger
```

(or just run `./udev-setup.sh`, which does the same)

2. Download `OpenGK850WPlugin-linux64.zip` from the [latest release](https://github.com/michauMiau/openrgb-gk850/releases/latest) and copy `libOpenGK850WPlugin.so` into your OpenRGB plugins directory (`~/.config/OpenRGB/plugins/`).

### Windows

Download `OpenGK850WPlugin-win64.zip` from the [latest release](https://github.com/michauMiau/openrgb-gk850/releases/latest). Copy `OpenGK850WPlugin.dll` into `%APPDATA%\OpenRGB\plugins\`. The MinGW/Qt runtime DLLs go next to `OpenRGB.exe` if your build doesn't bundle them.

## Building From Source

### Prerequisites

- Qt 6.4+ development tools (`qtbase6-dev-tools`, `qmake6`)
- hidapi (`libhidapi-hidraw0-dev`)
- OpenRGB source headers

```bash
git clone https://gitlab.com/OpenRGBDevelopers/OpenRGB.git ~/OpenRGB
cd plugin/
qmake6 OpenGK850WPlugin.pro
make -j$(nproc)
cp libOpenGK850WPlugin.so ~/.config/OpenRGB/plugins/
```

CI builds on every version tag (`v*`) for both Linux and Windows via GitHub Actions.

## Usage

1. Start OpenRGB with the plugin installed — the keyboard appears as a normal device.
2. Pick one of the 19 hardware effects and set its color; toggle Random Colors for palette cycling.
3. Use **Custom** mode for per-key painting (patterns, artwork, indicators).
4. The debug log widget shows the last HID traffic for troubleshooting.

### Compatibility notes

- ✅ No main repo patching required
- ✅ Only detects the correct keyboard (product string check prevents sending data to other devices)
- ⚠️ Effects Plugin works through hardware effect emulation; rapid software-driven animation is limited by the firmware (~4 fps, see limitation above)

## Protocol Reference

Reverse-engineered from USB HID captures of the vendor software (see `hid-pcap-analysis.md`):

- **Report ID 5** (6 B): unlock/init commands (`05 83 b6`, `05 88 b8`, save unlock `05 84 d4`)
- **Report ID 6** (1032 B): LED data — sub-reports `06 08 b8` (effect color), `06 09 bc/c0` (per-key), commit `06 03 b6`
- Effect color lives at `C08[eid×21+8..10]` (R,G,B); commit carries effect ID at `[21]`, `(speed<<4)|brightness` at `[69]`, and per-effect flag/selector bytes

## Author

garfi-kod, michaumiau 2026
Based on PCAP analysis of the original vendor software, and some analysis from the SinowealthController in OpenRGB, no code made it's way directly though.
GPL-2 License (same as upstream)
