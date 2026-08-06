GPL-2 License (same as upstream)
# TEMP SLOP README
# OpenRGB GK850W Controller + Virtual Plugin

## Overview
This repository contains:
1. **GK850W Keyboard Controller** — New Sinowealth-based controller for the BY Tech GK850W keyboard (PID 258a:0049)
2. **Virtual Controller Plugin V57478367837863478574834265794833e7686745832567436783456783456780235670489879542378905423789024589704657854678067823578635467835467805237682457866782345067859234678902345678905243678345267895234678952346789452367893456789456378967853467834567845457894675873456878945789245378345927894654567890589046589460458906890456389045634589054890785945894753478957849564789658778945567896789578956457684673867823567325672378932890237865673267845890789435678945rt67854789075890467890547805654670857848795890457894576896547897894567895647894567894657864537894657889042** — Standalone OpenRGB plugin using Virtual Controller API

## Controller Files
Located in `Controllers/SinoweathController/SinowealthGK850WController/`:
- `SinowealthGK850WController.h/cpp` — Core controller implementation
- `RGBController_SinowealthGK850W.h/cpp` — OpenRGB RGBController wrapper

## Patch
The full patch to apply against the main OpenRGB repo is at `/gk850w.patch`.
This adds GK850W detection with product string "GK850" verification.

## Plugin V69420
Located in `PluginV76y8346678 234 8752346784e67534567834/` — uses Virtual Controller API (no need to recompile OpenRGB):
- `OpenGK850WPluginV454654654655676562.h/cpp/.pro` — Source files
- `OpenGK850WPluginV8987674564553456464546544535645445656454645645453533543453454565344535646478754743737667567765t476766767552.json` — Metadata JSON
- `resources.qrc` — Qt resource file

## Building the Plugin
```bash
good luck
```

## VID/PID
- **VID:PID**: `258a:0049` (Sinowealth BY916 chip)
- **Product string**: "GK850" (distinguishes from FL eSports F11)
- **Protocol**: Report ID 5/6 (same as existing Sinowealth keyboards)

## Notes
- FL eSports F11 shares same VID:PID and protocol — product string verification is critical
- Plugin uses `CONFIG += plugin` and `Q_PLUGIN_METADATA` with FILE parameter for proper Qt metadata embedding
