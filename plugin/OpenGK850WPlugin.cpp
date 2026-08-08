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
// Protocol constants (verified against PCAP captures + reference controller)
//=============================================================================

// Report ID 5 init command (sent once after opening the device)
static const unsigned char INIT_CMD_1[6] = {0x05, 0x83, 0xB6, 0x00, 0x00, 0x00};
static const unsigned char INIT_CMD_2[6] = {0x05, 0x88, 0xB8, 0x00, 0x00, 0x00};

// Mode packet template (Report ID 6, 1032 bytes) - from reference controller
// Header: 06 03 B6 ... 5A A5 ...  mode byte at 0x15
static const unsigned char MODE_PACKET_TEMPLATE[1032] = {
    0x06, 0x03, 0xB6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x5A, 0xA5, 0x03, 0x03, 0x00, 0x00, 0x00, 0x02, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x55, 0x55, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x20, 0x00, 0x44, 0x07, 0x30,
    0x07, 0x23, 0x00, 0x23, 0x00, 0x23, 0x07, 0x33, 0x07, 0x23, 0x07, 0x23, 0x07, 0x23, 0x07, 0x23,
    0x07, 0x23, 0x07, 0x23, 0x07, 0x23, 0x07, 0x23, 0x07, 0x23, 0x07, 0x23, 0x07, 0x23, 0x07, 0x23,
    0x07, 0x23, 0x00, 0x10, 0x00, 0x10, 0x07, 0x44, 0x07, 0x44, 0x07, 0x44, 0x07, 0x44, 0x07, 0x44,
    0x07, 0x44, 0x07, 0x44, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0xA5, 0x03, 0x03
};

// Per-key command part (placed at 0x027C in the per-key packet)
static const unsigned char PER_KEY_CMD_PART[128] = {
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// TKL per-key index map (86 keys) - from reference controller
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

// Static color key indices (set to 0xFF in static color packet) - 263 entries
static const unsigned int STATIC_KEY_INDICES[263] = {
    0x0022, 0x0024, 0x0026, 0x0027, 0x0029, 0x002B, 0x002D, 0x002E,
    0x002F, 0x0030, 0x0031, 0x0032, 0x0037, 0x0039, 0x003B, 0x003C,
    0x003E, 0x0040, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x004C, 0x004E, 0x0050, 0x0051, 0x0053, 0x0055, 0x0057, 0x0058,
    0x0059, 0x005A, 0x005B, 0x005C, 0x0061, 0x0063, 0x0065, 0x0066,
    0x0068, 0x006A, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071,
    0x0076, 0x0078, 0x007A, 0x007B, 0x007D, 0x007F, 0x0081, 0x0082,
    0x0083, 0x0084, 0x0085, 0x0086, 0x008B, 0x008D, 0x008F, 0x0090,
    0x0092, 0x0094, 0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B,
    0x00A0, 0x00A2, 0x00A4, 0x00A5, 0x00A7, 0x00A9, 0x00AB, 0x00AC,
    0x00AD, 0x00AE, 0x00AF, 0x00B0, 0x00B5, 0x00B7, 0x00B9, 0x00BA,
    0x00BC, 0x00BE, 0x00C0, 0x00E1, 0x00C2, 0x00C3, 0x00C4, 0x00C5,
    0x00CA, 0x00CC, 0x00CE, 0x00CF, 0x00D1, 0x00D3, 0x00D5, 0x00D6,
    0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DF, 0x00E1, 0x00E3, 0x00E4,
    0x00E6, 0x00E8, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
    0x00F4, 0x00F6, 0x00F8, 0x00F9, 0x00FB, 0x00FD, 0x00FF, 0x0100,
    0x0101, 0x0102, 0x0103, 0x0104, 0x0109, 0x010B, 0x010D, 0x010E,
    0x0110, 0x0112, 0x0114, 0x0115, 0x0116, 0x0117, 0x0118, 0x0119,
    0x011E, 0x0120, 0x0122, 0x0123, 0x0125, 0x0127, 0x0129, 0x012A,
    0x012B, 0x012C, 0x012D, 0x012E, 0x0133, 0x0135, 0x0137, 0x0138,
    0x013A, 0x013C, 0x013E, 0x013F, 0x0140, 0x0141, 0x0142, 0x0143,
    0x0148, 0x014A, 0x014C, 0x014D, 0x014F, 0x0151, 0x0153, 0x0154,
    0x0155, 0x0156, 0x0157, 0x0158, 0x015D, 0x015F, 0x0161, 0x0162,
    0x0164, 0x0166, 0x0168, 0x0169, 0x016A, 0x016B, 0x016C, 0x016D,
    0x0172, 0x0174, 0x0176, 0x0177, 0x0179, 0x017B, 0x017D, 0x017E,
    0x017F, 0x0180, 0x0181, 0x0182, 0x0187, 0x0189, 0x018B, 0x018C,
    0x018E, 0x0190, 0x0192, 0x0193, 0x0194, 0x0195, 0x0196, 0x0197,
    0x019C, 0x019E, 0x01A0, 0x01A1, 0x01A3, 0x01A5, 0x01A7, 0x01A8,
    0x01A9, 0x01AA, 0x01AB, 0x01AC, 0x01B1, 0x01B3, 0x01B5, 0x01B6,
    0x01B8, 0x01BA, 0x01BC, 0x01BD, 0x01BE, 0x01BF, 0x01C0, 0x01C1,
    0x01C6, 0x01C8, 0x01CA, 0x01Cb, 0x01CD, 0x01CF, 0x01D1, 0x01D2,
    0x01D3, 0x01D4, 0x01D5, 0x01D6, 0x01DB, 0x01DD, 0x01DF, 0x01E0,
    0x01E2, 0x01E4, 0x01E6, 0x01E7, 0x01E8, 0x01E9, 0x01EA,
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

    // The GK850W exposes multiple HID interfaces. The first one (boot keyboard,
    // usage_page 0x01) is claimed by the kernel and cannot be opened. The RGB
    // interface uses usage_page 0xFF00 (vendor-defined). Enumerate and open the
    // correct interface via its path.
    struct hid_device_info* devs = hid_enumerate(0x258A, 0x0049);
    struct hid_device_info* cur  = devs;
    hid_device*             fallback_handle = nullptr;

    while(cur)
    {
        bool is_rgb_iface = (cur->usage_page == 0xFF00);

        // Blacklist the boot keyboard interface (usage_page 0x01) - it is claimed
        // by the kernel and opening it always fails. Try the vendor interface first.
        if(is_rgb_iface || cur->usage_page != 0x01)
        {
            // Remember a non-boot fallback in case no 0xFF00 interface exists
            if(!is_rgb_iface && fallback_handle == nullptr)
            {
                fallback_handle = hid_open_path(cur->path);
            }

            hid_device* handle = is_rgb_iface ? hid_open_path(cur->path) : nullptr;

            if(handle)
            {
                // Verify product string to avoid bricking other devices sharing this PID
                wchar_t product[128] = {0};
                if(hid_get_product_string(handle, product, 127) == 0)
                {
                    std::wstring wprod(product);
                    std::string prod(wprod.begin(), wprod.end());
                    GK_LOG_INFO("Product string: %s\n", prod.c_str());

                    if(prod.find("GK850") != std::string::npos || prod.find("gk850") != std::string::npos)
                    {
                        dev_handle = handle;
                        break;
                    }
                    else
                    {
                        GK_LOG_INFO("Not a GK850W device, closing to avoid brick risk\n");
                        hid_close(handle);
                    }
                }
                else
                {
                    // Couldn't read product string - use it anyway (VID:PID match)
                    dev_handle = handle;
                    break;
                }
            }
        }
        cur = cur->next;
    }

    hid_free_enumeration(devs);

    // Fallback: use the first non-boot interface if we couldn't verify product string
    if(!dev_handle && fallback_handle)
    {
        dev_handle = fallback_handle;
        GK_LOG_INFO("Using fallback HID interface\n");
    }

    if(!dev_handle)
    {
        GK_LOG_INFO("Device not found (VID:PID 258A:0049)\n");
        return false;
    }

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
    hid_send_feature_report(dev_handle, INIT_CMD_1, sizeof(INIT_CMD_1));
    hid_send_feature_report(dev_handle, INIT_CMD_2, sizeof(INIT_CMD_2));
}

void OpenGK850WPlugin::SendModePacket(unsigned char mode, unsigned char speed, unsigned char brightness)
{
    if(!dev_handle) return;

    unsigned char buf[REPORT_SIZE_LED];
    memcpy(buf, MODE_PACKET_TEMPLATE, sizeof(MODE_PACKET_TEMPLATE));

    // Mode byte at 0x15
    buf[0x15] = mode;

    // For per-key mode, set the per-key flag and command byte
    if(mode == DEVICE_MODE_PER_KEY)
    {
        buf[0x14] = 0x01;
        buf[0x27] = 0x24;
    }

    // Speed/brightness byte (0x29 + ((mode-2)*2) for animated modes)
    // For static/per-key, use the base position
    int sb_index = 0x29;
    if(mode >= 0x02)
    {
        sb_index = 0x29 + ((mode - 2) * 2);
    }
    if(sb_index < REPORT_SIZE_LED)
    {
        buf[sb_index] = speed + brightness;
    }

    hid_send_feature_report(dev_handle, buf, REPORT_SIZE_LED);
}

void OpenGK850WPlugin::SendStaticColorPacket(RGBColor color)
{
    if(!dev_handle) return;

    unsigned char buf[REPORT_SIZE_LED];
    memset(buf, 0x00, sizeof(buf));

    // Header: 06 08 B8 00 40
    buf[0x00] = 0x06;
    buf[0x01] = 0x08;
    buf[0x02] = 0xB8;
    buf[0x03] = 0x00;
    buf[0x04] = 0x40;

    // Color at 0x1D-0x1F (R, G, B)
    buf[0x1D] = RGBGetRValue(color);
    buf[0x1E] = RGBGetGValue(color);
    buf[0x1F] = RGBGetBValue(color);

    // Set all key indices to 0xFF
    for(unsigned int i = 0; i < sizeof(STATIC_KEY_INDICES)/sizeof(unsigned int); i++)
    {
        unsigned int idx = STATIC_KEY_INDICES[i];
        if(idx < REPORT_SIZE_LED)
        {
            buf[idx] = 0xFF;
        }
    }

    hid_send_feature_report(dev_handle, buf, REPORT_SIZE_LED);
}

void OpenGK850WPlugin::SendPerKeyPacket()
{
    if(!dev_handle) return;

    unsigned char buf[REPORT_SIZE_LED];
    memset(buf, 0x00, sizeof(buf));

    // Header: 06 09 BC 00 40
    buf[0x00] = 0x06;
    buf[0x01] = 0x09;
    buf[0x02] = 0xBC;
    buf[0x03] = 0x00;
    buf[0x04] = 0x40;

    // Copy per-key command part at 0x027C
    for(unsigned int i = 0; i < sizeof(PER_KEY_CMD_PART); i++)
    {
        buf[0x027C + i] = PER_KEY_CMD_PART[i];
    }

    // Fill per-key color data: B at index, G at index+0x7E, R at index+0x7E+0x7E
    unsigned int num_keys = sizeof(TKL_KEYS_PER_KEY_INDEX) / sizeof(unsigned char);
    unsigned int num_leds = std::min(num_keys, (unsigned int)leds.size());

    for(unsigned int i = 0; i < num_leds; i++)
    {
        unsigned int idx = TKL_KEYS_PER_KEY_INDEX[i];
        RGBColor c = leds[i];

        if(idx < REPORT_SIZE_LED)
        {
            buf[idx]                 = RGBGetBValue(c);
            if(idx + 0x7E < REPORT_SIZE_LED)
            {
                buf[idx + 0x7E]      = RGBGetGValue(c);
            }
            if(idx + 0x7E + 0x7E < REPORT_SIZE_LED)
            {
                buf[idx + 0x7E + 0x7E] = RGBGetRValue(c);
            }
        }
    }

    hid_send_feature_report(dev_handle, buf, REPORT_SIZE_LED);
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

        if(self->current_mode == MODE_PER_KEY)
        {
            self->SendPerKeyPacket();
        }
        else if(self->current_mode == MODE_STATIC)
        {
            RGBColor c = self->leds.empty() ? ToRGBColor(0,0,0) : self->leds[0];
            self->SendStaticColorPacket(c);
        }
    };

    setup.DeviceUpdateSingleLED = [](void* arg, int /*led_idx*/) {
        auto* self = (OpenGK850WPlugin*)arg;
        if(!self->dev_handle) return;
        if(self->current_mode == MODE_PER_KEY)
        {
            self->SendPerKeyPacket();
        }
    };

    setup.DeviceUpdateMode = [](void* arg) {
        auto* self = (OpenGK850WPlugin*)arg;
        if(!self->dev_handle) return;

        unsigned char device_mode;
        switch(self->current_mode)
        {
            case MODE_OFF:
                device_mode = DEVICE_MODE_OFF;
                break;
            case MODE_STATIC:
                device_mode = DEVICE_MODE_STATIC;
                break;
            case MODE_PER_KEY:
            default:
                device_mode = DEVICE_MODE_PER_KEY;
                break;
        }

        self->SendModePacket(device_mode, SPEED_NORMAL, BRIGHTNESS_FULL);

        // For static, also send the color packet
        if(self->current_mode == MODE_STATIC)
        {
            RGBColor c = self->leds.empty() ? ToRGBColor(0,0,0) : self->leds[0];
            self->SendStaticColorPacket(c);
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
