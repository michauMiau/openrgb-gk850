# OpenRGB GK850W Controller + Virtual Plugin

## Overview

This repository contains a complete solution for supporting the BY Tech / Mad Dog GK850(W) keyboard in OpenRGB:

1. **GK850W Keyboard Controller** — New Sinowealth-based controller with product-string verification to distinguish from FL eSports F11
2. **Virtual Controller Plugin** — Standalone OpenRGB plugin using Virtual Controller API (no main repo patching required)

## VID/PID

- **VID:PID**: `258a:0049` (Sinowealth BY916 chip)
- **Product string**: "GK850" (distinguishes from FL eSports F11 which shares the same PID)
- **Protocol**: Report ID 5/6 (same as existing Sinowealth keyboards)

## Brick Risk Warning

PID `0x0049` is shared with FL eSports F11 and Redragon devices. The original OpenRGB code disabled detection due to brick risk on Redragon hardware. Our solution:

- Uses **product string "GK850"** as a safe distinguisher
- Virtual Controller plugin approach requires **no patching** of main repo, eliminating brick risk entirely

## Files

### Controller (Unmaintained)

Located in `Controllers/SinowealthController/SinowealthGK850WController/`:

- `SinowealthGK850WController.h/cpp` — Core controller implementation
- `RGBController_SinowealthGK850W.h/cpp` — OpenRGB RGBController wrapper

### Plugin (standalone, no patching required)

Located in `plugin/`:

- `OpenGK850WPlugin.h/cpp/.pro` — Source files
- `OpenGK850WPlugin.json` — Qt plugin metadata
- `README.md` — This file

## PCAP Analysis Reference

Protocol analysis from USB HID captures (`hid-pcap-analysis.md`):

### Report ID 5 (Command Packets) — 6 bytes

Format: `{0x05, mode, speed, brightness, 0x00, 0x00}`

| Mode | Value | Description |
| ------ | ------- | ------------- |
| Static | 0x01 | Single color static |
| Breathing | 0x02 | Breath effect |
| Rainbow/Transition | 0x03 | Rainbow transition |
| Flash Away | 0x04 | Flash outward |
| Raindrops | 0x05 | Raindrop pattern |
| Off | 0x16 | All LEDs off |
| Custom (per-key) | 0x15 | Addressable per-key mode |

Speed values: `SLOW=0x12`, `NORMAL=0x22`, `FASTER=0x32`, `FASTEST=0x42`
Brightness values: `OFF=0x00`, `QUARTER=0x01`, `HALF=0x02`, `THREE_QUARTERS=0x03`, `FULL=0x04`

### Report ID 6 (LED Data) — 1032 bytes

Header: `{0x06, 0x08, 0xB8, 0x00, 0x40, ...}`

- Per-key data at specific offsets (see `tkl_keys_per_key_index` in existing controller)
- **BGR byte order** (not RGB!) — blue, green, red channels

## Building the Plugin

### Prerequisites

- Qt 6.4+ development tools (`qtbase6-dev-tools`, `qmake6`)
- hidapi library (`libhidapi-hidraw0-dev`)
- OpenRGB source headers (clone from gitlab.com/OpenRGBDevelopers/OpenRGB)

### Build Steps

```bash
# Clone OpenRGB for headers
git clone https://gitlab.com/OpenRGBDevelopers/OpenRGB.git ~/OpenRGB

# Configure and build plugin
cd plugin/
qmake6 OpenGK850WPlugin.pro
make -j$(nproc)

# Install to OpenRGB plugins directory
cp libOpenGK850WPlugin.so ~/.config/OpenRGB/plugins/
```

### For AppImage Users (Qt 6.4.2)

The official OpenRGB pipeline AppImage bundles Qt 6.4.2. To avoid version mismatch:

```bash
# Extract AppImage Qt libs (if needed)
unsquashfs -d /tmp/appimg_extract OpenRGB-x86_64.AppImage

# Set RPATH to AppImage Qt directory
patchelf --set-rpath '/tmp/appimg_extract/usr/lib:$ORIGIN' libOpenGK850WPlugin.so
```

## Usage

1. Install Plugin using OpenRGB
2. Plugin will auto-detect GK850W keyboard via VID:PID + product string verification
3. Virtual controller appears in device list as normal

## Compatibility Notes

- ✅ OpenRGB Effects Plugin not compatible (Keyboard doesn't have a "Direct" mode, using effects may wear out the flash, or reset)
- ✅ No main repo patching required
- ✅ Only detects the right keyboard (product string check prevents sending data to other keyboards)
- ⚠️ Requires udev rules on Linux (script to install udev-setup.sh in repo)

## Author

garfi-kod, michaumiau 2026
Based on PCAP analysis of the original software.
GPL-2 License (same as upstream)
