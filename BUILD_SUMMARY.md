# OpenRGB GK850W Plugin - Build Summary & Technical Reference

## Repository
- **URL**: https://github.com/michauMiau/openrgb-gk850.git
- **Working commit (v2)**: `4a0ffb5` — "Fix LogManager symbol resolution and Qt 6.4.2 compatibility"
- **Debug commit (v3, non-working on user's PC)**: `b80e73e` — "Add debug enumeration and path-based device opening"

## Build Environment

### Qt Installation
- **Location**: `~/Qt/6.4.2/gcc_64/bin/qmake`
- **Version**: Qt 6.4.2 (must use this, NOT system Qt 5.15 or Qt 6.8)
- **Library path**: `/home/truenas_admin/Qt/6.4.2/gcc_64/lib`
- **Installation method**: aqtinstall in `~/qt-venv`

### Build Commands
```bash
cd ~/openrgb-gk850/plugin
~/Qt/6.4.2/gcc_64/bin/qmake OpenGK850WPlugin.pro
make -j1  # Use single job to avoid OOM kills
```

### Build Output
- **Plugin location**: `~/openrgb-gk850/libOpenGK850WPlugin.so` (61KB)
- **Metadata JSON**: `~/openrgb-gk850/plugin/OpenGK850WPlugin.json`
- **RUNPATH**: `/home/truenas_admin/Qt/6.4.2/gcc_64/lib`

## Critical Technical Details

### Plugin Metadata (OpenGK850WPlugin.json)
```json
{
    "Id": "OpenGK850WPlugin",
    "Name": "GK850(W) Keyboard Controller Plugin",
    "VersionStr": "3.0",
    "Url": "https://github.com/michauMiau/openrgb-gk850",
    "Description": "Plugin implementing support for the BY Tech/Mad Dog GK850(W)",
    "OpenRGBPluginAPIVersion": 5,
    "Commit": ""
}
```

### Dependencies (nm -D output)
- **HIDAPI symbols**: `hid_init`, `hid_open`, `hid_close`, `hid_enumerate`, `hid_free_enumeration`, `hid_get_product_string`, `hid_send_feature_report`, `hid_open_path`, `hid_error`
- **Qt symbols**: `qt_plugin_instance`, `qt_plugin_query_metadata_v2`, all Qt_6_* private API symbols
- **JSON ABI**: `nlohmann::json_abi_v3_11_3` — matches OpenRGB's nlohmann version

### Include Paths (from .pro)
```
INCLUDEPATH += /home/truenas_admin/OpenRGB
INCLUDEPATH += /home/truenas_admin/OpenRGB/RGBController
INCLUDEPATH += /home/truenas_admin/OpenRGB/dependencies/json
INCLUDEPATH += /usr/include/hidapi
```

### Build Flags (from Makefile)
- `-DOPENRGB_PLUGIN`
- `-DQT_NO_DEBUG`
- `-DQT_PLUGIN`
- `-DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB`
- `-fPIC`
- `-O2 -Wall -Wextra`

## Known Issues & Fixes Applied

### 1. LogManager Symbol Resolution
**Problem**: `undefined symbol: _ZN10LogManager3getEv` — LogManager is part of main OpenRGB binary, not available in plugin shared library context.

**Fix**: Replaced `LOG_INFO(...)` with custom macro using `fprintf(stderr, ...)`:
```cpp
#define GK_LOG_INFO(...) fprintf(stderr, "[GK850W] " __VA_ARGS__)
```

### 2. Double lib Prefix Bug (earlier fix)
**Problem**: TARGET was set to `libOpenGK850WPlugin`, qmake adds another `lib` prefix producing non-loadable plugin.

**Fix**: Changed TARGET to just `OpenGK850WPlugin`.

### 3. Missing Mode Constants
**Problem**: `MODE_OFF`, `MODE_STATIC`, `MODE_GAME` not defined in plugin header but used in code.

**Fix**: Added defines:
```cpp
#define DEVICE_MODE_OFF              0x16   // From PCAP analysis
#define DEVICE_MODE_PER_KEY          0x15   // Custom per-key mode
#define BRIGHTNESS_LEVELS[] = {0x01, 0x02, 0x03, 0x04}
```

## Current Problem: Plugin Loading Failure on User's PC

### Symptoms
- **v2 (commit 4a0ffb5)**: Works correctly — device detected, logs show "Device not found" only if keyboard disconnected
- **v3/v4/v5**: Fails with "Unknown error" when loading in OpenRGB
- **Error message**: `[Warning][PluginManager] Plugin /path/libOpenGK850WPlugin.so cannot be loaded: Unknown error`

## v3.0 Fixes (verified against PCAP captures)

### 1. Correct HID Interface Selection
**Problem**: `hid_open(0x258A, 0x0049, NULL)` opened the boot-keyboard interface
(usage_page 0x01), which is claimed by the kernel and always fails. Log showed
"Device not found" even though the keyboard was connected.

**Fix**: Enumerate HID devices and open the vendor-defined RGB interface
(usage_page 0xFF00) via `hid_open_path()`. Falls back to the first non-boot
interface if needed. Product string "GK850" is verified before sending commands
to avoid bricking other devices sharing PID 0x0049.

### 2. Virtual Controller Registration Deadlock
**Problem**: `QMetaMethod::invoke: Dead lock detected in BlockingQueuedConnection:
Receiver is OpenRGBEffectsPlugin`. `RegisterVirtualRGBController()` calls
`UpdateDeviceList()` from the UI thread, which deadlocks with the Effects plugin.

**Fix**: Use `RegisterVirtualRGBControllerInThread()` which registers the virtual
controller in a background thread (exactly what the API provides this for).

### 3. Corrected Line Color Protocol
**Problem**: The plugin sent query commands over Report ID 5 for modes, but the
device expects all state in Report ID 6 packets.

**Correct protocol (from PCAP captures + reference controller):**
| Purpose | Report | Structure |
|---------|--------|-----------|
| Init commands | RID5 (6B) | `05 83 B6 00 00 00` / `05 88 B8 00 00 00` |
| Set mode | RID6 (1032B) | `06 03 B6 ... 5A A5`, mode byte at 0x15 |
| Static color | RID6 (1032B) | `06 08 B8 00 40`, color at 0x1D-0x1F |
| Per-key | RID6 (1032B) | `06 09 BC 00 40`, BGR at key index offsets |

Mode byte values: `OFF=0x16`, `STATIC=0x83`, `PER_KEY=0x15`.

### Debugging Information (from v3 logs on user's PC)
When using debug version, device IS visible but opening fails:
```
[GK850W] Device not found via VID:PID, enumerating all devices...
[GK850W] Found device path: /dev/hidraw5
[GK850W] Product string: 'GK850'
[GK850W] Failed to open via path: /dev/hidraw5
[GK850W] Found device path: /dev/hidraw7
[GK850W] Product string: 'GK850'
[GK850W] Failed to open via path: /dev/hidraw7
```

### Key Observations
1. **Device IS visible** — `hid_enumerate()` finds `/dev/hidraw5` and `/dev/hidraw7` with product string "GK850"
2. **VID:PID matches** — 0x258A:0x0049 (BY Tech GK850)
3. **hid_open_path() fails** for both paths — possible causes: permissions, exclusive access, or kernel-level HID filtering

### Potential Causes of "Unknown Error" on User's PC
1. **Qt version mismatch** between AppImage and system libraries
2. **nlohmann::json ABI mismatch** — plugin compiled with `json_abi_v3_11_3`, OpenRGB may expect different ABI tag
3. **Platform plugin missing** — Qt requires platform plugin (e.g., `libqxcb.so`) which may not be in AppImage's Qt6 directory
4. **D-Bus or other system dependency conflict**

## Testing & Verification Commands

### Verify Plugin Symbols
```bash
nm -D ~/openrgb-gk850/libOpenGK850WPlugin.so | grep -E "qt_plugin|plugin_instance"
# Should output: qt_plugin_instance and qt_plugin_query_metadata_v2

nm -D ~/openrgb-gk850/libOpenGK850WPlugin.so | grep " U hid_"
# Should show all hid_* symbols as undefined (external dependencies)
```

### Check Library Dependencies
```bash
ldd ~/openrgb-gk850/libOpenGK850WPlugin.so
readelf -d ~/openrgb-gk850/libOpenGK850WPlugin.so | grep RUNPATH
```

### Test Device Access
```bash
# Check if device is accessible
ls -la /dev/hidraw*
lsusb | grep 258a
udevadm info /dev/hidrawX  # Replace X with actual number
```

## User's Environment (Testing PC)
- **OS**: Linux (likely Ubuntu/Debian-based)
- **User**: `mcholewinski`
- **OpenRGB location**: System-installed or AppImage
- **Plugin install path**: `/home/mcholewinski/.config/OpenRGB/plugins/libOpenGK850WPlugin.so`
- **Debug output**: Use `openrgb --very-verbose` to see plugin loading details

## Archive Locations
- v2 (working): `~/GK850W_Plugin_Qt642_v2.zip`
- v4 (current): `~/GK850W_Plugin_Qt642_v4.zip`
- v5 (with error logging): `~/GK850W_Plugin_Qt642_v5.zip`

## Next Steps for Investigation
1. Compare plugin loading behavior between v2 and v3+ on user's PC
2. Check if OpenRGB AppImage has all required Qt platform plugins
3. Investigate why `hid_open_path()` fails despite device being visible
4. Consider adding fallback to direct `/dev/hidrawX` file access with read/write permissions
