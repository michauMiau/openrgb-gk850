/*-----------------------------------------*\
||  OpenGK850WPlugin.cpp                    ||
||                                          ||
||  Plugin for GK850W keyboard using        ||
||  Virtual RGB Controller API              ||
||                                          ||
||  garfi-kod, michaumiau 2026              ||
\*-----------------------------------------*/

#include "OpenGK850WPlugin.h"
#include "gk850_reports.h"
#include "gk850_effect_colors.h"
#include "gk850_frames.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <cstring>
#include <thread>
#include <chrono>

// Use fprintf for logging since LogManager is part of main OpenRGB, not a plugin library
#define GK_LOG_INFO(...) fprintf(stderr, "[GK850W] " __VA_ARGS__)

//=============================================================================
// Protocol constants (verified against PCAP captures)
// Key patterns from PCAP:
// - 0583b6000000: INIT command (present in EVERY capture)
// - 0588b8000000: REQUIRED second init (present in almost every capture!)
// - 050301000600: Status/keepalive (very frequent)
// - Effects use: 05 XX YY ZZ WW VV format
//=============================================================================

// All protocol reports are byte-accurate extractions from the vendor app's
// USB traffic (see gk850_reports.h). Each static/per-key sequence is:
//   init(s) -> color-data report(s) -> mode-commit report
// The mode-commit report (RID6, header 06 03 b6) is what actually applies the
// change; without it the keyboard blinks and reverts to its previous state.
// The commit differs per mode (on/off/game), so each has its own template.

// Per-key color region: PCAP diff of the game-mode captures shows exactly 61
// single-byte key slots (one byte per key, matching the 60% layout). The app
// writes each key's value into these fixed offsets. We map OpenRGB LED index i
// to GK_PERKEY_OFFSETS[i].
static const unsigned int GK_PERKEY_OFFSETS[61] = {
    281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294,
    302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315,
    323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 336,
    344, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 357,
    365, 366, 367, 370, 373, 374, 375, 378
};

//=============================================================================
// Device management
//=============================================================================

bool OpenGK850WPlugin::OpenDevice()
{
    if(dev_handle)
    {
        return true;
    }

    hid_init();

    // The GK850W exposes multiple HID interfaces. PCAP analysis shows RGB
    // feature reports go to interface 1 (wIndex=0001). We prefer iface 1,
    // but fall back to probing if needed.
    struct hid_device_info* devs = hid_enumerate(0x258A, 0x0049);
    struct hid_device_info* cur  = devs;
    bool                    saw_any = false;
    hid_device*             iface1_match = nullptr;  // Preferred: interface_number == 1
    hid_device*             last_resort = nullptr;   // Fallback

    while(cur)
    {
        saw_any = true;

        // Log each interface
        GK_LOG_INFO("USB iface: path=%s usage_page=0x%04X usage=0x%04X iface=%d\n",
                    cur->path ? cur->path : "(null)", cur->usage_page, cur->usage,
                    cur->interface_number);

        // Prefer interface 1 (where RGB reports go according to PCAP)
        if(cur->interface_number == 1 && !iface1_match)
        {
            hid_device* handle = hid_open_path(cur->path);
            if(handle)
            {
                GK_LOG_INFO("Found interface 1: %s - selected for RGB control\n", cur->path);
                iface1_match = handle;
                break;  // We have what we want
            }
        }

        // Keep track of any other valid interface as fallback
        if(!last_resort && cur->usage_page != 0x01)
        {
            hid_device* handle = hid_open_path(cur->path);
            if(handle)
            {
                last_resort = handle;
            }
        }

        cur = cur->next;
    }

    hid_free_enumeration(devs);

    if(!saw_any)
    {
        GK_LOG_INFO("No HID interfaces found for VID:PID 258A:0049\n");
        return false;
    }

    // Use interface 1 if found, otherwise fall back
    hid_device* match = iface1_match ? iface1_match : last_resort;

    if(match)
    {
        if(iface1_match)
            GK_LOG_INFO("Using interface 1 (RGB vendor interface)\n");
        else
            GK_LOG_INFO("Using fallback interface (no iface 1 found)\n");

        dev_handle = match;
        SendInitCommands(true);
        GK_LOG_INFO("Device opened successfully\n");
        return true;
    }

    GK_LOG_INFO("Failed to open any HID interface\n");
    return false;
}

void OpenGK850WPlugin::CloseDevice()
{
    if(dev_handle)
    {
        hid_close(dev_handle);
        dev_handle = nullptr;
        GK_LOG_INFO("Device closed\n");
    }
}

void OpenGK850WPlugin::RescanDevice()
{
    GK_LOG_INFO("Rescanning device...\n");
    CloseDevice();
    OpenDevice();
}

//=============================================================================
// Protocol helpers
//=============================================================================

void OpenGK850WPlugin::SendInitCommands(bool full)
{
    if(!dev_handle) return;
    // PCAP-verified: static/off sequences start with BOTH init commands
    // (05 83 b6 + 05 88 b8); the per-key/game sequence starts with only the
    // first one (05 83 b6). `full` selects which to send.
    hid_send_feature_report(dev_handle, GK_INIT_1, sizeof(GK_INIT_1));
    if(full)
    {
        hid_send_feature_report(dev_handle, GK_INIT_2, sizeof(GK_INIT_2));
    }
    GK_LOG_INFO("SendInitCommands: sent %s\n", full ? "init1+init2" : "init1 only");
}

unsigned int OpenGK850WPlugin::CurrentMode()
{
    // Resolve the active mode's value through the interface API (the
    // RGBControllerInterface does not expose the modes vector directly).
    if(virtual_controller)
    {
        int idx = virtual_controller->GetActiveMode();
        std::string name = virtual_controller->GetModeName((unsigned int)idx);

        // Match by mode name - stable regardless of list order.
        if(name == "Custom")        return MODE_PER_KEY;
        if(name == "Static")        return MODE_STATIC;
        if(name == "Off")           return MODE_OFF;

        // Hardware effects: name -> effect ID lookup.
        static const struct { const char* name; unsigned char id; } EFFECT_IDS[] = {
            {"Breathing", 0x02}, {"Spectrum Cycle", 0x03}, {"Rainbow Wave", 0x04},
            {"Rain", 0x05}, {"Double Spectrum", 0x06}, {"Water Drop", 0x07},
            {"Twinkling Stars", 0x08}, {"Shadow", 0x09}, {"Snake", 0x0A},
            {"Neon Wave", 0x0B}, {"Trail", 0x0C}, {"Sine Wave", 0x0D},
            {"Scan", 0x0E}, {"Carousel", 0x0F}, {"Waterfall", 0x10},
            {"Pulsing", 0x11}, {"Explosion", 0x12}, {"Collision", 0x13},
            {"Flashing", 0x14},
        };
        for(const auto& e : EFFECT_IDS)
        {
            if(name == e.name) return e.id;
        }
    }
    return current_mode;   // fallback to cached value
}

void OpenGK850WPlugin::SyncModeFromController()
{
    current_mode = CurrentMode();
}

void OpenGK850WPlugin::SendModeCommit(CommitType type)
{
    if(!dev_handle) return;
    // PCAP-verified: after the color-data report the working app sends a
    // 1032-byte "commit" report (06 03 b6). This is what actually applies the
    // change. Without it the keyboard blinks and reverts to its previous state.
    // The commit encodes the mode in byte [21]: off=0x00, static=0x01,
    // per-key/game=0x15 (see all_modes.pcapng: 22 unique commits).
    const unsigned char* commit = nullptr;
    switch(type)
    {
        case COMMIT_ON:   commit = GK_MODE_COMMIT_ON;   break;
        case COMMIT_OFF:  commit = GK_MODE_COMMIT_OFF;  break;
        case COMMIT_GAME: commit = GK_MODE_COMMIT_GAME; break;
    }
    int ret = hid_send_feature_report(dev_handle, commit, REPORT_SIZE_LED);
    if(ret < 0)
    {
        GK_LOG_INFO("SendModeCommit(%d): hid_send_feature_report failed (ret=%d)\n", (int)type, ret);
    }
}

void OpenGK850WPlugin::SendEffectPacket(unsigned char effect_id, RGBColor color)
{
    if(!dev_handle) return;

    /* DEFINITIVE protocol (USB setup-packet decode of every vendor
     * capture): an effect/color change is exactly FOUR feature writes:
     *   1. SET_REPORT ID5 "05 83 b6"        (init1)
     *   2. SET_REPORT ID5 "05 88 b8"        (init2)
     *   3. SET_REPORT ID6 "06 08 b8" + color @ [29:31]
     *   4. SET_REPORT ID6 "06 03 b6" commit with effect id @ [21],
     *      custom-color flags [40]=[46]=[58]=07 / [76]=00 and slider
     *      state at [39]/[59]/[69].
     * The "06 83 b6 frame" and "06 88 xx" records in pcaps are GET_REPORT
     * readbacks (bmRequestType=A1), never written by the vendor app. */
    hid_send_feature_report(dev_handle, GK_INIT_1, sizeof(GK_INIT_1));
    hid_send_feature_report(dev_handle, GK_INIT_2, sizeof(GK_INIT_2));

    unsigned char buf[REPORT_SIZE_LED];
    memcpy(buf, GK_COLOR_DATA_ON, sizeof(GK_COLOR_DATA_ON));
    buf[29] = RGBGetRValue(color);
    buf[30] = RGBGetGValue(color);
    buf[31] = RGBGetBValue(color);
    int r1 = hid_send_feature_report(dev_handle, buf, REPORT_SIZE_LED);
    GK_LOG_INFO("SendEffectPacket(%02X): color report ret=%d\n", effect_id, r1);
    AddDebug(QString("Eff %1: color ret=%2").arg(effect_id, 2, 16).arg(r1));

    const unsigned char* tmpl = nullptr;
    for(unsigned char k = 0; k < GK_EFFECT_COMMIT_COUNT; k++)
    {
        unsigned char map_id[] = { 0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
            0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14 };
        if(map_id[k] == effect_id) { tmpl = GK_EFFECT_COMMITS[k]; break; }
    }
    if(!tmpl) tmpl = GK_EFFECT_COMMITS[0];
    unsigned char commit[REPORT_SIZE_LED];
    memcpy(commit, tmpl, REPORT_SIZE_LED);
    commit[21] = effect_id;
    commit[40] = 0x07;
    commit[46] = 0x07;
    commit[58] = 0x07;
    commit[76] = 0x00;
    {
        unsigned int lvl3 = current_brightness & 0x0F; if(lvl3 < 1) lvl3 = 1; if(lvl3 > 4) lvl3 = 4;
        static const unsigned char sscale[5] = {0, 0x34, 0x33, 0x32, 0x31};
        commit[39] = sscale[lvl3];
        commit[59] = current_speed;
        unsigned int sn3 = (current_speed >> 4) & 0x0F; if(sn3 < 1) sn3 = 1; if(sn3 > 4) sn3 = 4;
        commit[69] = (unsigned char)((sn3 << 4) | lvl3);
    }
    int r2 = hid_send_feature_report(dev_handle, commit, REPORT_SIZE_LED);
    GK_LOG_INFO("SendEffectPacket(%02X): commit ret=%d flags=%02x%02x%02x%02x c=%02x%02x%02x\n",
                effect_id, r2, commit[40], commit[46], commit[58], commit[76],
                commit[29], commit[30], commit[31]);
    AddDebug(QString("Eff %1: commit ret=%2 fl=%3%4%5%6 c=%7,%8,%9")
        .arg(effect_id, 2, 16).arg(r2)
        .arg(commit[40], 2, 16, QChar('0')).arg(commit[46], 2, 16, QChar('0'))
        .arg(commit[58], 2, 16, QChar('0')).arg(commit[76], 2, 16, QChar('0'))
        .arg(RGBGetRValue(color)).arg(RGBGetGValue(color)).arg(RGBGetBValue(color)));
}


void OpenGK850WPlugin::SendStaticColorPacket(RGBColor color)
{
    if(!dev_handle) return;

    // PCAP-verified static sequence (Set_whole_keyboard_to_red.pcapng):
    //   [0] 05 83 b6 00 00 00   (init1)
    //   [1] 05 88 b8 00 00 00   (init2)
    //   [2] color-data report  (06 08 b8, R/G/B at offsets 29/30/31)
    //   [3] mode-commit report (06 03 b6)  <-- applies the change
    SendInitCommands(true);

    unsigned char buf[REPORT_SIZE_LED];
    memcpy(buf, GK_COLOR_DATA_ON, sizeof(GK_COLOR_DATA_ON));
    buf[29] = RGBGetRValue(color);
    buf[30] = RGBGetGValue(color);
    buf[31] = RGBGetBValue(color);

    /* THE MISSING REPORT: vendor sends the color TWICE in effect sessions -
     * once as [06 88 00 ...] (effects read THIS one) and once as
     * [06 08 b8 ...]. Byte-identical apart from the 3-byte header. */
    unsigned char buf88[REPORT_SIZE_LED];
    memcpy(buf88, buf, REPORT_SIZE_LED);
    buf88[0] = 0x06; buf88[1] = 0x88; buf88[2] = 0x00;
    hid_send_feature_report(dev_handle, buf88, REPORT_SIZE_LED);

    GK_LOG_INFO("SendStaticColorPacket: R=%d G=%d B=%d\n", RGBGetRValue(color), RGBGetGValue(color), RGBGetBValue(color));
    AddDebug(QString("Static: R=%1 G=%2 B=%3").arg(RGBGetRValue(color)).arg(RGBGetGValue(color)).arg(RGBGetBValue(color)));

    int ret = hid_send_feature_report(dev_handle, buf, REPORT_SIZE_LED);
    if(ret < 0)
    {
        GK_LOG_INFO("SendStaticColorPacket: hid_send_feature_report failed (ret=%d)\n", ret);
        AddDebug(QString("ERROR: color report failed (ret=%1)").arg(ret));
    }
    else
    {
        AddDebug(QString("Color data sent OK (%1 bytes)").arg(REPORT_SIZE_LED));
    }

    // Commit with static-on mode code AND current brightness (byte [39]).
    // STATIC uses its own scale: PCAP-verified 0x34=highest, 0x31=lowest
    // (vendor static_blue captures). The effect-scale [69] nibbles don't
    // apply here - map from the effect value to keep the slider natural.
    unsigned char commit[REPORT_SIZE_LED];
    memcpy(commit, GK_MODE_COMMIT_ON, REPORT_SIZE_LED);
    {
        /* Static scale is INVERTED (user + PCAP verified): 0x34 = dimmest,
         * 0x31 = brightest. Map slider nibble (1=dim..4=bright) accordingly. */
        unsigned int lvl = current_brightness & 0x0F; if(lvl < 1) lvl = 1; if(lvl > 4) lvl = 4;
        static const unsigned char static_scale[5] = {0, 0x34, 0x33, 0x32, 0x31};
        commit[39] = static_scale[lvl];
    }

    ret = hid_send_feature_report(dev_handle, commit, REPORT_SIZE_LED);
    if(ret < 0)
    {
        GK_LOG_INFO("SendStaticColorPacket: commit failed (ret=%d)\n", ret);
    }
}

void OpenGK850WPlugin::SendPerKeyPacket()
{
    if(!dev_handle) return;

    // PCAP-verified per-key sequence (game_mode_adressable_.pcapng):
    //   [0] 05 83 b6 00 00 00   (init1 only - NO init2 for game mode)
    //   [1] 06 09 bc ...        (key color data, RGB blocks 126B apart)
    //   [2] 06 09 c0 ...        (second per-key report)
    //   [3] 06 03 b6 ...        (mode commit, mode=0x15)
    //
    // Real-time optimization: the vendor app sends init1 only ONCE when
    // entering game mode, then streams bc/c0 reports WITHOUT re-committing.
    // Re-sending init + commit on every LED update makes the keyboard blink
    // black (each commit restarts the lighting engine). So:
    //   - inits are sent only on mode ENTRY (perkey_inited flag),
    //   - the commit is sent only on mode entry (or explicit force).
    if(!perkey_inited)
    {
        if(!skip_init_reports)
        {
            SendInitCommands(false);
        }
        perkey_inited = true;
        perkey_needs_commit = true;
    }

    unsigned char buf[REPORT_SIZE_LED];
    memcpy(buf, GK_PERKEY_DATA_1, sizeof(GK_PERKEY_DATA_1));

    unsigned char buf2[REPORT_SIZE_LED];
    memcpy(buf2, GK_PERKEY_DATA_2, sizeof(GK_PERKEY_DATA_2));

    // All three channels live in the SAME bc report. Channel blocks are 126
    // bytes apart: R = GK_PERKEY_OFFSETS, G = offsets-126, B = offsets-252.
    // ESC (key 0): R=281, G=155, B=29. Verified against esc_green/esc_blue.
    unsigned int num_leds = NUM_LEDS;
    if(virtual_controller) {
        zone z = virtual_controller->GetZone(0);
        if(z.colors && z.leds_count > 0) {
            num_leds = std::min((unsigned int)NUM_LEDS, z.leds_count);
            for(unsigned int i = 0; i < num_leds; i++)
            {
                buf[GK_PERKEY_OFFSETS[i]]       = RGBGetRValue(z.colors[i]);
                buf[GK_PERKEY_OFFSETS[i] - 126] = RGBGetGValue(z.colors[i]);
                buf[GK_PERKEY_OFFSETS[i] - 252] = RGBGetBValue(z.colors[i]);
            }
        }
    }

    int ret = hid_send_feature_report(dev_handle, buf, REPORT_SIZE_LED);
    if(ret < 0)
    {
        GK_LOG_INFO("SendPerKeyPacket[1]: hid_send_feature_report failed (ret=%d)\n", ret);
    }

    // Part 2: second per-key report (template from PCAP).
    ret = hid_send_feature_report(dev_handle, buf2, REPORT_SIZE_LED);
    if(ret < 0)
    {
        GK_LOG_INFO("SendPerKeyPacket[2]: hid_send_feature_report failed (ret=%d)\n", ret);
    }

    // Commit only on mode entry; streaming updates skip it so the keyboard
    // doesn't blink black on every frame.
    if(perkey_needs_commit && !skip_init_reports)
    {
        SendModeCommit(COMMIT_GAME);
        perkey_needs_commit = false;
    }
}

void OpenGK850WPlugin::SendOffPacket(RGBColor last_color)
{
    if(!dev_handle) return;
    (void)last_color;

    // PCAP-verified off sequence (keyboard_off.pcapng):
    //   [0] 05 83 b6 00 00 00   (init1)
    //   [1] 05 88 b8 00 00 00   (init2)
    //   [2] color-data report  (06 08 b8, template from PCAP)
    //   [3] mode-commit report (06 03 b6, off variant, byte [21]=0x00)
    SendInitCommands(true);

    int ret = hid_send_feature_report(dev_handle, GK_COLOR_DATA_OFF, REPORT_SIZE_LED);
    if(ret < 0)
    {
        GK_LOG_INFO("SendOffPacket: color-data failed (ret=%d)\n", ret);
    }

    // Commit with the OFF mode code.
    SendModeCommit(COMMIT_OFF);
}

//=============================================================================
// Plugin lifecycle
//=============================================================================

void OpenGK850WPlugin::Load(OpenRGBPluginAPIInterface* plugin_api_ptr)
{
    api = plugin_api_ptr;
    GK_LOG_INFO("Plugin loading...\n");

    // Delay before device enumeration to avoid race condition with OpenRGB startup.
    // The keyboard may not be fully enumerated yet when the plugin loads.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    GK_LOG_INFO("Load: waited 1000ms before opening device\n");

    OpenDevice();

    // Initialize LEDs
    leds.resize(NUM_LEDS, ToRGBColor(0, 0, 0));

    // Create virtual controller setup
    RGBController_Setup setup = {};
    setup.name = "GK850";
    setup.vendor = "BY Tech";
    setup.description = "Sinowealth GK850W (Virtual Plugin)";
    setup.version = "3.0.0";
    setup.serial = "";
    setup.location = "";
    setup.type = DEVICE_TYPE_KEYBOARD;
    setup.flags = CONTROLLER_FLAGS_MANUALLY_CONFIGURABLE;

    // Add modes - names follow the OpenRGB "Common Modes" convention:
    // https://openrgb.org/wiki/doku.php?id=common_modes
    mode m;

    // Custom: per-LED colors. Not "Direct" because the keyboard firmware
    // blinks on every update (does not meet the no-flicker Direct criteria).
    m.name = "Custom";
    m.value = MODE_PER_KEY;
    m.flags = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    m.color_mode = MODE_COLORS_PER_LED;
    m.brightness_min = 0;
    m.brightness_max = 4;
    m.brightness = 4;
    setup.modes.push_back(m);

    // Static: whole device set to one static color (may flicker/save).
    m.name = "Static";
    m.value = MODE_STATIC;
    m.flags = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    m.color_mode = MODE_COLORS_MODE_SPECIFIC;
    m.colors_min = 1;
    m.colors_max = 1;
    m.colors.resize(1, ToRGBColor(0xFF, 0x00, 0x00));
    m.brightness_min = 0;
    m.brightness_max = 4;
    m.brightness = 4;
    setup.modes.push_back(m);

    // Built-in hardware effects. PCAP-verified (all_modes.pcapng): the commit's
    // mode byte selects the effect; UI order in the vendor app maps 1:1 to HW
    // effect IDs 0x01..0x14. Names mapped to Common OpenRGB Modes where they
    // exist, vendor names kept otherwise.
    static const struct { const char* name; unsigned char id; bool has_color; bool has_speed; } EFFECTS[] = {
        {"Breathing",         0x02, true,  true},   /* vendor: Oddech          */
        {"Spectrum Cycle",    0x03, false, true},   /* vendor: Przejście       */
        {"Rainbow Wave",      0x04, true,  true},   /* vendor: Podświetl linię */
        {"Rain",              0x05, true,  true},
        {"Double Spectrum",   0x06, false, true},   /* vendor: Podwójne przejście */
        {"Water Drop",        0x07, true,  true},
        {"Twinkling Stars",   0x08, true,  true},
        {"Shadow",            0x09, true,  true},
        {"Snake",             0x0A, true,  true},
        {"Neon Wave",         0x0B, true,  true},
        {"Trail",             0x0C, true,  true},
        {"Sine Wave",         0x0D, true,  true},
        {"Scan",              0x0E, true,  true},
        {"Carousel",          0x0F, true,  true},
        {"Waterfall",         0x10, true,  true},
        {"Pulsing",           0x11, true,  true},
        {"Explosion",         0x12, true,  true},
        {"Collision",         0x13, true,  true},
        {"Flashing",          0x14, true,  true},   /* vendor: Błysk          */
    };
    for(const auto& e : EFFECTS)
    {
        mode em;
        em.name  = e.name;
        em.value = e.id;
        em.flags = 0;
        if(e.has_color)
        {
            em.flags |= MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
            em.color_mode = MODE_COLORS_MODE_SPECIFIC;
            em.colors_min = 1;
            em.colors_max = 1;
            em.colors.resize(1, ToRGBColor(0xFF, 0x00, 0x00));
        }
        else
        {
            em.color_mode = MODE_COLORS_NONE;
        }
        if(e.has_speed)
        {
            em.flags |= MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_BRIGHTNESS;
            em.speed_min = 0;
            em.speed_max = 3;
            em.speed = 1;
            em.brightness_min = 0;
            em.brightness_max = 4;
            em.brightness = 4;
        }
        setup.modes.push_back(em);
    }

    m.name = "Off";
    m.value = MODE_OFF;
    m.flags = 0;
    m.color_mode = MODE_COLORS_NONE;
    setup.modes.push_back(m);

    // Add zone - whole keyboard (with named LEDs + matrix map so the device
    // section in OpenRGB shows an editable keyboard layout like native
    // controllers).
    zone z;
    z.name = "Keyboard";
    z.type = ZONE_TYPE_MATRIX;
    z.leds_count = NUM_LEDS;
    setup.zones.push_back(z);

    SetupKeyboardLayout(setup);

    // Callback for updating LEDs via HID
    setup.DeviceUpdateLEDs = [](void* arg) {
        auto* self = (OpenGK850WPlugin*)arg;
        if(!self->dev_handle) return;

        unsigned int mode = self->CurrentMode();
        
        if(mode == MODE_PER_KEY)
        {
            self->SendPerKeyPacket();
        }
        else if(mode == MODE_STATIC)
        {
            // Get color from the active mode using OpenRGB API
            RGBColor c = ToRGBColor(255, 0, 0); // Default red
            
            if(self->virtual_controller) {
                int active_mode_idx = self->virtual_controller->GetActiveMode();
                if(active_mode_idx >= 0) {
                    c = self->virtual_controller->GetModeColor(active_mode_idx, 0);
                }
            }
            
            self->SendStaticColorPacket(c);
        }
        else if(mode >= 0x02 && mode <= 0x14)
        {
            /* Color change while a hardware effect is active: OpenRGB calls
             * UpdateLEDs (not UpdateMode) for color-only edits. Without this
             * branch the keyboard keeps the template's baked color - that's
             * why Snake stayed green and Explosion blue regardless of pick. */
            RGBColor c = ToRGBColor(0xFF, 0x00, 0x00);
            if(self->virtual_controller) {
                int idx = self->virtual_controller->GetActiveMode();
                if(idx >= 0) {
                    c = self->virtual_controller->GetModeColor(idx, 0);
                    if(c == ToRGBColor(0xFF, 0x00, 0x00))
                    {
                        c = self->virtual_controller->GetZoneColor(0, 0);
                    }
                }
            }
            self->SendEffectPacket(mode, c);
        }
    };

    setup.DeviceUpdateSingleLED = [](void* arg, int /*led_idx*/) {
        auto* self = (OpenGK850WPlugin*)arg;
        if(!self->dev_handle) return;
        if(self->CurrentMode() == MODE_PER_KEY)
        {
            self->SendPerKeyPacket();
        }
    };

    setup.DeviceUpdateMode = [](void* arg) {
        auto* self = (OpenGK850WPlugin*)arg;
        if(!self->dev_handle) return;

        self->SyncModeFromController();
        // Mode change: force fresh init + commit for the new mode.
        self->perkey_inited = false;
        unsigned int mode = self->CurrentMode();

        // Read brightness/speed set by the OpenRGB UI (slider values).
        if(self->virtual_controller) {
            int idx = self->virtual_controller->GetActiveMode();
            if(idx >= 0) {
                unsigned int b = self->virtual_controller->GetModeBrightness((unsigned int)idx);
                unsigned int s = self->virtual_controller->GetModeSpeed((unsigned int)idx);
                // NEW PCAPs (effect brightness/speed sweeps, effect 0x10):
                // for EFFECTS both params live in commit byte [69]:
                //   brightness: 0x43=highest, 0x42, 0x41=lowest (4 lvls)
                //   speed:      0x34, 0x24, 0x14 (slowest) - 0x44 fastest
                static const unsigned char bright_table[5] = {0x44, 0x44, 0x43, 0x42, 0x41};
                if(b > 4) b = 4;
                self->current_brightness = bright_table[b];
                // Speed 0..3 -> commit byte[59] 0x14/0x24/0x34/0x44
                // (PCAP-verified: neon slowest=0x14, fastest=0x44)
                static const unsigned char speed_table[4] = {0x14, 0x24, 0x34, 0x44};
                if(s > 3) s = 3;
                self->current_speed = speed_table[s];
            }
        }

        GK_LOG_INFO("DeviceUpdateMode: mode=0x%02X\n", mode);

        if(mode == MODE_STATIC)
        {
            // Get color from OpenRGB using proper API
            RGBColor c = ToRGBColor(0, 0, 0); // No forced default color
            
            if(self->virtual_controller) {
                int active_mode_idx = self->virtual_controller->GetActiveMode();
                
                if(active_mode_idx >= 0) {
                    // Get the color for this mode
                    c = self->virtual_controller->GetModeColor(active_mode_idx, 0);
                }
            }

            self->AddDebug(QString("Static color: R=%1 G=%2 B=%3")
                .arg(RGBGetRValue(c))
                .arg(RGBGetGValue(c))
                .arg(RGBGetBValue(c)));

            self->SendStaticColorPacket(c);
        }
        else if(mode == MODE_PER_KEY)
        {
            self->SendPerKeyPacket();
        }
        else if(mode >= 0x02 && mode <= 0x14)
        {
            // Built-in hardware effect: color-data + commit with the effect
            // ID as mode byte. Sequence verified in speeds_neon_wave_red.pcapng.
            RGBColor c = ToRGBColor(0xFF, 0x00, 0x00);
            if(self->virtual_controller) {
                int idx = self->virtual_controller->GetActiveMode();
                if(idx >= 0) {
                    c = self->virtual_controller->GetModeColor(idx, 0);
                    /* Fallback: if mode color still at init default (red),
                     * use the first LED's color - the UI may paint LEDs
                     * instead of mode colors depending on which wheel the
                     * user picked from. */
                    if(c == ToRGBColor(0xFF, 0x00, 0x00))
                    {
                        c = self->virtual_controller->GetZoneColor(0, 0);
                    }
                }
            }
            GK_LOG_INFO("DeviceUpdateMode: effect=0x%02X color R=%d G=%d B=%d bright=0x%02X speed=0x%02X\n",
                        mode, RGBGetRValue(c), RGBGetGValue(c), RGBGetBValue(c),
                        self->current_brightness, self->current_speed);
            self->AddDebug(QString("Effect %1 (0x%2): R=%3 G=%4 B=%5, bright=%6, speed=%7")
                .arg(self->virtual_controller ? QString::fromStdString(
                    self->virtual_controller->GetModeName((unsigned int)
                        self->virtual_controller->GetActiveMode())) : "?")
                .arg(mode, 2, 16)
                .arg(RGBGetRValue(c))
                .arg(RGBGetGValue(c))
                .arg(RGBGetBValue(c))
                .arg((self->current_brightness & 0x0F))
                .arg(((self->current_speed >> 4) & 0x07)));
            self->SendEffectPacket(mode, c);
        }
        else
        {
            // Off mode: send the PCAP-verified off sequence (color-data + commit_off)
            self->SendOffPacket(ToRGBColor(0,0,0));
        }
    };

    // Register self as object pointer for callbacks
    setup.object_ptr = this;

    virtual_controller = api->CreateVirtualRGBController(&setup);
        if(virtual_controller)
        {
            // Register in a background thread to avoid deadlock with the Effects
            // plugin (BlockingQueuedConnection). RegisterVirtualRGBController
            // calls UpdateDeviceList() which can deadlock if called from the UI thread.
            api->RegisterVirtualRGBControllerInThread(virtual_controller);
            GK_LOG_INFO("Virtual controller registered!\n");
        }
        else
        {
            GK_LOG_INFO("Failed to create virtual controller\n");
        }
    }

void OpenGK850WPlugin::SetupKeyboardLayout(RGBController_Setup& setup)
{
    // 61 keys, GK850W 60% layout. 5 rows x 15 cols matrix map.
    // Row layouts (LED index per column, -1 = no key):
    static const int ROWS[5][15] = {
        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, -1, 13},  /* Esc..Backspace */
        {14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, -1, 27},  /* Tab..Backslash */
        {28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, -1, -1, 40},  /* Caps..Enter    */
        {41, -1, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, 52},  /* Shift..RShift  */
        {53, 54, 55, 56, 57, -1, 58, -1, -1, 59, -1, -1, -1, 60, -1}   /* Ctrl..RCtrl    */
    };

    static const char* LED_NAMES[61] = {
        "Esc","1","2","3","4","5","6","7","8","9","0","-","=","Backspace",
        "Tab","Q","W","E","R","T","Y","U","I","O","P","[","]","\\",
        "Caps Lock","A","S","D","F","G","H","J","K","L",";","'","Enter",
        "L Shift","Z","X","C","V","B","N","M",",",".","/","R Shift",
        "L Ctrl","L Win","L Alt","Space","R Alt","Fn","Menu","R Ctrl"
    };

    setup.leds.clear();
    for(unsigned int i = 0; i < NUM_LEDS; i++)
    {
        led l;
        l.name  = (i < 61 && LED_NAMES[i]) ? LED_NAMES[i]
                  : ("Key " + std::to_string(i));
        l.value = i;
        setup.leds.push_back(l);
    }

    if(!setup.zones.empty())
    {
        zone& kz = setup.zones.back();

        unsigned int map[5 * 15];
        unsigned int col_count = 0;
        for(unsigned int r = 0; r < 5; r++)
        {
            unsigned int row_cols = 0;
            for(unsigned int c = 0; c < 15; c++)
            {
                int idx = ROWS[r][c];
                map[r * 15 + c] = (idx >= 0) ? (unsigned int)idx : 0xFFFFFFFF;
                row_cols++;
            }
            if(row_cols > col_count) col_count = row_cols;
        }

        kz.matrix_map.Set(5, 15, map);
    }
}

QWidget* OpenGK850WPlugin::GetWidget()
{
    QWidget* w = new QWidget();
    auto* layout = new QVBoxLayout(w);

    // Device status
    auto* status_label = new QLabel(dev_handle ?
        "GK850W Connected\nProduct: BY Tech GK850" :
        "GK850W Virtual Controller\n(No device detected)");
    layout->addWidget(status_label);

    // Debug log (scrollable, keeps last 50 lines, auto-scrolls to newest)
    auto* debug_log_widget = new QPlainTextEdit();
    debug_log_widget->setReadOnly(true);
    debug_log_widget->setMaximumHeight(150);
    debug_log_widget->setPlainText("Debug log:\n(device not opened yet)");
    layout->addWidget(debug_log_widget);

    // Store reference so AddDebug() can update it live
    widget_debug_log = debug_log_widget;

    // Mode selector
    auto* mode_combo = new QComboBox();
    mode_combo->addItem("Static (Red)");
    mode_combo->addItem("Custom / Per-Key");
    mode_combo->addItem("Off");

    auto* apply_btn = new QPushButton("Apply Mode");
    QObject::connect(apply_btn, &QPushButton::clicked, [this, mode_combo, status_label]() {
        int idx = mode_combo->currentIndex();
        current_mode = idx == 0 ? MODE_STATIC : idx == 1 ? MODE_PER_KEY : MODE_OFF;

        if(virtual_controller)
        {
            virtual_controller->SetActiveMode(idx);
        }

        status_label->setText(QString("Mode: %1").arg(mode_combo->currentText()));
    });

    // Rescan button
    auto* rescan_btn = new QPushButton("Rescan Device");
    QObject::connect(rescan_btn, &QPushButton::clicked, [this, status_label]() {
        RescanDevice();
        status_label->setText(dev_handle ?
            "GK850W Connected\nProduct: BY Tech GK850" :
            "GK850W Virtual Controller\n(No device detected)");
    });

    // Real-time test: skip ALL init reports and mode commits, send only the
    // raw color data reports. Use if the keyboard still blinks on updates.
    auto* skip_init_check = new QCheckBox("Skip init/commit (real-time test)");
    skip_init_check->setChecked(skip_init_reports);
    QObject::connect(skip_init_check, &QCheckBox::toggled, [this](bool checked) {
        skip_init_reports = checked;
        perkey_inited = false;  // re-init when unchecked
        AddDebug(QString("Skip init/commit: %1").arg(checked ? "ON" : "OFF"));
    });

    auto* btn_row = new QHBoxLayout();
    btn_row->addWidget(apply_btn);
    btn_row->addWidget(rescan_btn);

    layout->addWidget(skip_init_check);

    layout->addWidget(mode_combo);
    layout->addLayout(btn_row);

    return w;
}

QMenu* OpenGK850WPlugin::GetTrayMenu()
{
    QMenu* menu = new QMenu("GK850W Controls");

    auto* static_action = menu->addAction("Static (Red)");
    auto* custom_action = menu->addAction("Custom / Per-Key");
    auto* off_action = menu->addAction("Off");
    menu->addSeparator();
    auto* rescan_action = menu->addAction("Rescan Device");

    QObject::connect(static_action, &QAction::triggered, [this]() {
        current_mode = MODE_STATIC;
        if(virtual_controller) virtual_controller->SetActiveMode(0);
    });
    QObject::connect(custom_action, &QAction::triggered, [this]() {
        current_mode = MODE_PER_KEY;
        if(virtual_controller) virtual_controller->SetActiveMode(1);
    });
    QObject::connect(off_action, &QAction::triggered, [this]() {
        current_mode = MODE_OFF;
        if(virtual_controller) virtual_controller->SetActiveMode(2);
    });
    QObject::connect(rescan_action, &QAction::triggered, [this]() {
        RescanDevice();
    });

    return menu;
}

OpenRGBPluginInfo OpenGK850WPlugin::GetPluginInfo()
{
    OpenRGBPluginInfo info;
    info.Name = "GK850W";
    info.Description = "Virtual controller for GK850W keyboard - no main repo patch needed";
    info.Version = "3.0.0";
    info.Commit = "local-build";
    info.URL = "";
    info.Location = OPENRGB_PLUGIN_LOCATION_TOP;
    info.Label = "GK850W";
    info.ProtocolVersion = 6;
    return info;
}

void OpenGK850WPlugin::Unload() {}

OpenGK850WPlugin::~OpenGK850WPlugin()
{
    CloseDevice();
}

void OpenGK850WPlugin::OnProfileAboutToLoad() {}
void OpenGK850WPlugin::OnProfileLoad(nlohmann::json) {}
nlohmann::json OpenGK850WPlugin::OnProfileSave() { return nlohmann::json{}; }
unsigned char* OpenGK850WPlugin::OnSDKCommand(unsigned int, unsigned char*, unsigned int*) { return nullptr; }
void OpenGK850WPlugin::ProfileManagerUpdated(unsigned) {}
void OpenGK850WPlugin::ResourceManagerUpdated(unsigned) {}
void OpenGK850WPlugin::SettingsManagerUpdated(unsigned) {}
