/*-----------------------------------------*\
||  OpenGK850WPlugin.cpp                    ||
||                                          ||
||  Plugin for GK850W keyboard using        ||
||  Virtual RGB Controller API              ||
||                                          ||
||  garfi-kod, michaumiau 2026              ||
\*-----------------------------------------*/

#include "OpenGK850WPlugin.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QHBoxLayout>
#include <cstring>

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

static const unsigned char INIT_CMD1[6] = {0x05, 0x83, 0xB6, 0x00, 0x00, 0x00};
static const unsigned char INIT_CMD2[6] = {0x05, 0x88, 0xB8, 0x00, 0x00, 0x00};

// TKL per-key index map (86 keys) - from reference controller
//=============================================================================
static const unsigned char TKL_KEYS_PER_KEY_INDEX[86] = {
    0x08, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x1d, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x54, 0x5D, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x6A, 0x6B,
    0x71, 0x72, 0x73, 0x76, 0x79, 0x7A, 0x7B, 0x7F, 0x80, 0x81
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

    // The GK850W exposes multiple HID interfaces (lsusb -v shows 2: a boot
    // keyboard + a vendor RGB interface). The reference Sinowealth detector does
    // NOT trust usage_page alone - it probes each interface by attempting to read
    // the Report ID 6 feature report. We do the same: enumerate all matching
    // interfaces, log each one, and pick the interface that responds to the RGB
    // feature report (and whose product string matches "GK850").
    struct hid_device_info* devs = hid_enumerate(0x258A, 0x0049);
    struct hid_device_info* cur  = devs;
    bool                    saw_any = false;
    hid_device*             match = nullptr;
    hid_device*             last_resort = nullptr;   // product-matched, non-boot candidate

    while(cur)
    {
        saw_any = true;

        // Read product string for identification/logging
        std::string prod;
        {
            hid_device* tmp = hid_open_path(cur->path);
            if(tmp)
            {
                wchar_t product[128] = {0};
                if(hid_get_product_string(tmp, product, 127) == 0)
                {
                    std::wstring wprod(product);
                    prod.assign(wprod.begin(), wprod.end());
                }
                hid_close(tmp);
            }
        }

        GK_LOG_INFO("USB iface: path=%s usage_page=0x%04X usage=0x%04X iface=%d product='%s'\n",
                    cur->path ? cur->path : "(null)", cur->usage_page, cur->usage,
                    cur->interface_number, prod.c_str());

        // Skip the boot-keyboard interface (usage_page 0x01) - kernel-owned, never
        // opens and never carries RGB data. Also require product string match.
        bool product_ok = (prod.find("GK850") != std::string::npos ||
                           prod.find("gk850") != std::string::npos);

        if(cur->usage_page != 0x01 && product_ok)
        {
            // Open the candidate interface.
            hid_device* handle = hid_open_path(cur->path);
            if(handle)
            {
                // Probe: try to read Report ID 6 feature report.
                unsigned char buf[1032];
                memset(buf, 0, sizeof(buf));
                buf[0] = 0x06;   // Report ID 6

                int r = hid_get_feature_report(handle, buf, sizeof(buf));
                if(r > 0)
                {
                    GK_LOG_INFO("Interface responds to RID6 feature report (len=%d) - selected.\n", r);
                    if(last_resort) hid_close(last_resort);  // close unused candidate
                    match = handle;
                    break;
                }
                else
                {
                    GK_LOG_INFO("Interface does not respond to RID6, trying next.\n");
                    if(last_resort == nullptr)
                    {
                        last_resort = handle;   // keep open as fallback
                    }
                    else
                    {
                        hid_close(handle);      // already have a candidate
                    }
                }
            }
        }
        cur = cur->next;
    }

    hid_free_enumeration(devs);

    if(!saw_any)
    {
        GK_LOG_INFO("No HID interfaces found for VID:PID 258A:0049 (check udev rule / permissions)\n");
    }

    // If the probe didn't positively identify the interface, fall back to the
    // product-matched non-boot candidate (still safe: product string verified).
    if(!match && last_resort)
    {
        GK_LOG_INFO("Using fallback product-matched interface (no RID6 response)\n");
        match = last_resort;
    }

    if(!match)
    {
        GK_LOG_INFO("Device not found (VID:PID 258A:0049)\n");
        return false;
    }

    dev_handle = match;

    // Send init commands to unlock the device
    SendInitCommands();
    GK_LOG_INFO("Device opened successfully\n");
    return true;
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

void OpenGK850WPlugin::SendInitCommands()
{
    if(!dev_handle) return;
    // Send BOTH init commands - both are present in PCAP captures
    hid_send_feature_report(dev_handle, INIT_CMD1, sizeof(INIT_CMD1));
    hid_send_feature_report(dev_handle, INIT_CMD2, sizeof(INIT_CMD2));
    GK_LOG_INFO("SendInitCommands: done (both init commands sent)\n");
}

unsigned int OpenGK850WPlugin::CurrentMode()
{
    // Map the virtual controller's active mode index back to our MODE_* values.
    int idx = (virtual_controller) ? virtual_controller->GetActiveMode() : -1;
    switch(idx)
    {
        case 0:  return MODE_STATIC;    // Static
        case 1:  return MODE_PER_KEY;   // Custom
        case 2:  return MODE_OFF;       // Off
        default: return current_mode;   // fallback to cached value
    }
}

void OpenGK850WPlugin::SyncModeFromController()
{
    current_mode = CurrentMode();
}

void OpenGK850WPlugin::SendModePacket(unsigned char mode, unsigned char speed, unsigned char brightness)
{
    if(!dev_handle) return;

    // Report ID 5 (0x05): Mode command
    // Format: 05 MODE SPEED BRIGHTNESS 00 00
    // Mode value is used directly (no bitwise operations)
    unsigned char cmd[6] = {0x05, mode, speed, brightness, 0x00, 0x00};

    GK_LOG_INFO("SendModePacket: mode=0x%02X speed=0x%02X brightness=0x%02X\n", mode, speed, brightness);
    
    int ret = hid_send_feature_report(dev_handle, cmd, sizeof(cmd));
    if(ret < 0)
    {
        GK_LOG_INFO("SendModePacket: hid_send_feature_report failed (ret=%d)\n", ret);
    }
}

void OpenGK850WPlugin::SendStaticColorPacket(RGBColor color)
{
    if(!dev_handle) return;

    // PCAP verified: static color uses Report ID 6 with header 0608b80040
    // Color is repeated for each LED position as RGB triplets
    unsigned char buf[REPORT_SIZE_LED];
    memset(buf, 0x00, sizeof(buf));
    
    // Header from PCAP: 06 08 B8 00 40
    buf[0x00] = 0x06;
    buf[0x01] = 0x08;
    buf[0x02] = 0xB8;
    buf[0x03] = 0x00;
    buf[0x04] = 0x40;
    
    // Fill color data - RGB triplet per LED position
    // PCAP shows colors starting around offset 8, repeated pattern
    for(unsigned int i = 0; i < 300; i++)  // enough for all LEDs
    {
        unsigned int offset = 8 + i * 4;  // spacing from PCAP analysis
        if(offset + 2 < REPORT_SIZE_LED)
        {
            buf[offset]     = RGBGetRValue(color);
            buf[offset + 1] = RGBGetGValue(color);
            buf[offset + 2] = RGBGetBValue(color);
        }
    }
    
    GK_LOG_INFO("SendStaticColorPacket: R=%d G=%d B=%d\n", RGBGetRValue(color), RGBGetGValue(color), RGBGetBValue(color));
    
    int ret = hid_send_feature_report(dev_handle, buf, REPORT_SIZE_LED);
    if(ret < 0)
    {
        GK_LOG_INFO("SendStaticColorPacket: hid_send_feature_report failed (ret=%d)\n", ret);
    }
}

void OpenGK850WPlugin::SendPerKeyPacket()
{
    if(!dev_handle) return;

    // PCAP verified: per-key uses Report ID 6 with header 0609bc0040
    unsigned char buf[REPORT_SIZE_LED];
    memset(buf, 0x00, sizeof(buf));
    
    // Header from PCAP: 06 09 BC 00 40
    buf[0x00] = 0x06;
    buf[0x01] = 0x09;
    buf[0x02] = 0xBC;
    buf[0x03] = 0x00;
    buf[0x04] = 0x40;

    // Fill per-key color data using TKL key map
    // PCAP shows data starts after header, one RGB per key index
    unsigned int num_keys = sizeof(TKL_KEYS_PER_KEY_INDEX) / sizeof(unsigned char);
    unsigned int num_leds = std::min(num_keys, (unsigned int)leds.size());

    for(unsigned int i = 0; i < num_leds; i++)
    {
        unsigned int idx = TKL_KEYS_PER_KEY_INDEX[i];
        RGBColor c = leds[i];
        
        // Each key at position idx * 4 + 8 (based on PCAP spacing)
        unsigned int offset = 8 + idx * 4;
        if(offset + 2 < REPORT_SIZE_LED)
        {
            buf[offset]     = RGBGetRValue(c);
            buf[offset + 1] = RGBGetGValue(c);
            buf[offset + 2] = RGBGetBValue(c);
        }
    }

    GK_LOG_INFO("SendPerKeyPacket: %d keys\n", num_leds);
    
    int ret = hid_send_feature_report(dev_handle, buf, REPORT_SIZE_LED);
    if(ret < 0)
    {
        GK_LOG_INFO("SendPerKeyPacket: hid_send_feature_report failed (ret=%d)\n", ret);
    }
}

//=============================================================================
// Plugin lifecycle
//=============================================================================

void OpenGK850WPlugin::Load(OpenRGBPluginAPIInterface* plugin_api_ptr)
{
    api = plugin_api_ptr;
    GK_LOG_INFO("Plugin loading...\n");

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

    // Add modes
    mode m;

    m.name = "Static";
    m.value = MODE_STATIC;
    m.flags = MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
    m.color_mode = MODE_COLORS_MODE_SPECIFIC;
    m.colors_min = 1;
    m.colors_max = 1;
    m.colors.resize(1, ToRGBColor(0xFF, 0x00, 0x00)); // Red default
    setup.modes.push_back(m);

    m.name = "Custom";
    m.value = MODE_PER_KEY;
    m.flags = MODE_FLAG_HAS_PER_LED_COLOR;
    m.color_mode = MODE_COLORS_PER_LED;
    setup.modes.push_back(m);

    m.name = "Off";
    m.value = MODE_OFF;
    m.flags = 0;
    m.color_mode = MODE_COLORS_NONE;
    setup.modes.push_back(m);

    // Add zone - whole keyboard
    zone z;
    z.name = "Keyboard";
    z.type = ZONE_TYPE_MATRIX;
    z.leds_count = NUM_LEDS;
    setup.zones.push_back(z);

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
            RGBColor c = self->leds.empty() ? ToRGBColor(0,0,0) : self->leds[0];
            self->SendStaticColorPacket(c);
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
        unsigned int mode = self->CurrentMode();

        GK_LOG_INFO("DeviceUpdateMode: mode=0x%02X\n", mode);

        if(mode == MODE_STATIC)
        {
            RGBColor c = self->leds.empty() ? ToRGBColor(0xFF,0,0) : self->leds[0];
            self->SendStaticColorPacket(c);
        }
        else if(mode == MODE_PER_KEY)
        {
            self->SendPerKeyPacket();
        }
        else
        {
            // Off mode: send RID6 with all zeros (from keyboard_off.pcapng)
            self->SendStaticColorPacket(ToRGBColor(0,0,0));
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

QWidget* OpenGK850WPlugin::GetWidget()
{
    QWidget* w = new QWidget();
    auto* layout = new QVBoxLayout(w);

    // Device status
    auto* status_label = new QLabel(dev_handle ?
        "GK850W Connected\nProduct: BY Tech GK850" :
        "GK850W Virtual Controller\n(No device detected)");
    layout->addWidget(status_label);

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

    auto* btn_row = new QHBoxLayout();
    btn_row->addWidget(apply_btn);
    btn_row->addWidget(rescan_btn);

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
