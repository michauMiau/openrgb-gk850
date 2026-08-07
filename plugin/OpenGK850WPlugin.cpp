/*-----------------------------------------*|
|||  OpenGK850WPlugin.cpp                   ||
|||                                         ||
|||  Plugin for GK850W keyboard using       ||
|||  Virtual RGB Controller API             ||
|||                                         ||
|||  garfi-kod, michaumiau 2026             ||
|\*-----------------------------------------*/

#include "OpenGK850WPlugin.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include "LogManager.h"

// PCAP analysis constants (from hid-pcap-analysis.md)
static constexpr int REPORT_SIZE    = 1032;   // Total report size for Report ID 6
static constexpr int HEADER_SIZE    = 1;      // Report ID byte
static constexpr int MAX_LEDS       = (REPORT_SIZE - HEADER_SIZE) / 3; // 343 max LEDs

// Mode constants from PCAP analysis — these are the actual device modes
// Note: MODE_STATIC is 0x01, NOT 0x83 (which was brightness!)
#define DEVICE_MODE_OFF              0x16   // Mode sent via Report ID 5
#define DEVICE_MODE_PER_KEY          0x15   // Custom per-key mode

// Brightness levels from PCAP analysis
static constexpr unsigned char BRIGHTNESS_LEVELS[] = {0x01, 0x02, 0x03, 0x04}; // quarter to full

void OpenGK850WPlugin::Load(OpenRGBPluginAPIInterface* plugin_api_ptr) {
    api = plugin_api_ptr;
    LOG_INFO("[GK850W] Plugin loading...\n");

    hid_init();
    dev_handle = hid_open(0x258A, 0x0049, NULL);

    if (!dev_handle) {
        LOG_INFO("[GK850W] Device not found (VID:PID 258A:0049)\n");
        leds.resize(NUM_LEDS, ToRGBColor(0, 0, 0));
        return;
    }

    wchar_t product[128] = {0};
    hid_get_product_string(dev_handle, product, 127);
    std::wstring wprod(product);
    std::string prod(wprod.begin(), wprod.end());
    LOG_INFO("[GK850W] Product string: %s\n", prod.c_str());

    if (prod.find("GK850") == std::string::npos) {
        LOG_INFO("[GK850W] Not a GK850W device\n");
        hid_close(dev_handle);
        dev_handle = nullptr;
        leds.resize(NUM_LEDS, ToRGBColor(0, 0, 0));
        return;
    }

    // Initialize LEDs — adjust NUM_LEDS for your layout (61 TKL / 87 / 96 / 104)
    leds.resize(NUM_LEDS, ToRGBColor(0, 0, 0));

    // Create virtual controller setup
    RGBController_Setup setup = {};
    setup.name = "GK850";
    setup.vendor = "BY Tech";
    setup.description = "Sinowealth GK850W (Virtual Plugin)";
    setup.version = "2.1.0";
    setup.serial = "";
    setup.location = "";
    setup.type = DEVICE_TYPE_KEYBOARD;
    setup.flags = CONTROLLER_FLAGS_MANUALLY_CONFIGURABLE;

    // Add modes — only controller-native modes from PCAP analysis
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
    m.value = MODE_GAME;
    m.flags = MODE_FLAG_HAS_PER_LED_COLOR;
    m.color_mode = MODE_COLORS_PER_LED;
    setup.modes.push_back(m);

    m.name = "Off";
    m.value = MODE_OFF;
    m.flags = 0;
    m.color_mode = MODE_COLORS_NONE;
    setup.modes.push_back(m);

    // Add zone — whole keyboard
    zone z;
    z.name = "Keyboard";
    z.type = ZONE_TYPE_MATRIX;
    z.leds_count = NUM_LEDS;
    setup.zones.push_back(z);

    // Callback for updating LEDs via HID
    // Report ID 5 format (from PCAP): {0x05, mode, speed, brightness, 0x00, 0x00}
    // Report ID 6 format (from PCAP): {0x06, header[4], per-key BGR data at specific offsets}
    setup.DeviceUpdateLEDs = [](void* arg) {
        auto* self = (OpenGK850WPlugin*)arg;
        if (!self->dev_handle) return;

        if (self->current_mode == MODE_GAME) {
            // Custom mode - send per-key LED data via Report ID 6
            unsigned char report[REPORT_SIZE] = {0};
            
            // Report header from PCAP: {0x06, 0x08, 0xB8, 0x00, 0x40, ...}
            report[0] = 0x06;  // Report ID
            report[1] = 0x08;  // Header byte
            report[2] = 0xB8;  // Header byte
            report[3] = 0x00;  // Header byte
            report[4] = 0x40;  // Header byte
            
            int leds_to_send = std::min(NUM_LEDS, (int)self->leds.size());
            
            // PCAP analysis shows LED data is stored in specific layout positions
            // Each LED takes 3 bytes: B G R (note: order matters!)
            for (int i = 0; i < leds_to_send && i < MAX_LEDS; i++) {
                RGBColor c = self->leds[i];
                // PCAP shows BGR packing, not RGB
                report[1 + i * 3]     = RGBGetBValue(c);
                report[1 + i * 3 + 1] = RGBGetGValue(c);
                report[1 + i * 3 + 2] = RGBGetRValue(c);
            }
            
            hid_send_feature_report(self->dev_handle, report, REPORT_SIZE);
        } else {
            // Built-in mode - send mode command via Report ID 5
            // Format: {0x05, mode_value, speed, brightness, 0x00, 0x00}
            unsigned char report[6] = {
                0x05,                           // Report ID
                DEVICE_MODE_PER_KEY,           // Mode (we use per-key for custom)
                SPEED_NORMAL,                   // Speed
                BRIGHTNESS_FULL,               // Brightness
                0x00, 0x00                     // Padding
            };
            
            hid_send_feature_report(self->dev_handle, report, 6);
        }
    };

    setup.DeviceUpdateSingleLED = [](void* arg, int led_idx) {
        auto* self = (OpenGK850WPlugin*)arg;
        if (!self->dev_handle || self->current_mode != MODE_GAME) return;

        unsigned char report[REPORT_SIZE] = {0};
        
        // Report header from PCAP
        report[0] = 0x06;
        report[1] = 0x08;
        report[2] = 0xB8;
        report[3] = 0x00;
        report[4] = 0x40;
        
        int leds_to_send = std::min(NUM_LEDS, (int)self->leds.size());
        
        // BGR order from PCAP
        for (int i = 0; i < leds_to_send && i < MAX_LEDS; i++) {
            RGBColor c = self->leds[i];
            report[1 + i * 3]     = RGBGetBValue(c);
            report[1 + i * 3 + 1] = RGBGetGValue(c);
            report[1 + i * 3 + 2] = RGBGetRValue(c);
        }
        
        hid_send_feature_report(self->dev_handle, report, REPORT_SIZE);
    };

    setup.DeviceUpdateMode = [](void* arg) {
        auto* self = (OpenGK850WPlugin*)arg;
        if (!self->dev_handle) return;

        // Map OpenRGB modes to device modes from PCAP
        unsigned char device_mode = DEVICE_MODE_PER_KEY;
        unsigned char speed = SPEED_NORMAL;
        unsigned char brightness = BRIGHTNESS_FULL;
        
        if (self->current_mode == MODE_OFF) {
            device_mode = DEVICE_MODE_OFF;
        } else if (self->current_mode == MODE_STATIC) {
            device_mode = DEVICE_MODE_PER_KEY; // Static is handled via per-key
        }
        
        unsigned char report[6] = {
            0x05,                   // Report ID
            device_mode,           // Mode
            speed,                  // Speed
            brightness,             // Brightness
            0x00, 0x00              // Padding
        };
        
        hid_send_feature_report(self->dev_handle, report, 6);
    };

    // Register self as object pointer for callbacks
    setup.object_ptr = this;

    virtual_controller = api->CreateVirtualRGBController(&setup);
    if (virtual_controller) {
        api->RegisterVirtualRGBController(virtual_controller);
        LOG_INFO("[GK850W] Virtual controller registered!\n");
    } else {
        LOG_INFO("[GK850W] Failed to create virtual controller\n");
    }
}

QWidget* OpenGK850WPlugin::GetWidget() {
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
        current_mode = idx == 0 ? MODE_STATIC : idx == 1 ? MODE_GAME : MODE_OFF;

        if (virtual_controller) {
            virtual_controller->SetActiveMode(idx);
        }

        // Send command to device (guarded by null check)
        if (dev_handle) {
            unsigned char report[6] = {
                0x05,                           // Report ID
                DEVICE_MODE_PER_KEY,           // Mode
                SPEED_NORMAL,                   // Speed
                BRIGHTNESS_FULL,               // Brightness
                0x00, 0x00                      // Padding
            };
            hid_send_feature_report(dev_handle, report, 6);
        }

        status_label->setText(QString("Mode: %1").arg(mode_combo->currentText()));
    });

    layout->addWidget(mode_combo);
    layout->addWidget(apply_btn);

    return w;
}

QMenu* OpenGK850WPlugin::GetTrayMenu() {
    QMenu* menu = new QMenu("GK850W Controls");

    auto* static_action = menu->addAction("Static (Red)");
    auto* custom_action = menu->addAction("Custom / Per-Key");
    auto* off_action = menu->addAction("Off");

    QObject::connect(static_action, &QAction::triggered, [this]() {
        current_mode = MODE_STATIC;
        if (virtual_controller) virtual_controller->SetActiveMode(0);
    });
    QObject::connect(custom_action, &QAction::triggered, [this]() {
        current_mode = MODE_GAME;
        if (virtual_controller) virtual_controller->SetActiveMode(1);
    });
    QObject::connect(off_action, &QAction::triggered, [this]() {
        current_mode = MODE_OFF;
        if (virtual_controller) virtual_controller->SetActiveMode(2);
    });

    return menu;
}

OpenRGBPluginInfo OpenGK850WPlugin::GetPluginInfo() {
    OpenRGBPluginInfo info;
    info.Name = "GK850W";
    info.Description = "Virtual controller for GK850W keyboard - no main repo patch needed";
    info.Version = "2.1.0";
    info.Commit = "local-build";
    info.URL = "";
    info.Location = OPENRGB_PLUGIN_LOCATION_TOP;
    info.Label = "GK850W";
    info.ProtocolVersion = 6;
    return info;
}

void OpenGK850WPlugin::Unload() {}

OpenGK850WPlugin::~OpenGK850WPlugin() {
    if (dev_handle) {
        hid_close(dev_handle);
        dev_handle = nullptr;
    }
}

void OpenGK850WPlugin::OnProfileAboutToLoad() {}
void OpenGK850WPlugin::OnProfileLoad(nlohmann::json) {}
nlohmann::json OpenGK850WPlugin::OnProfileSave() { return nlohmann::json{}; }
unsigned char* OpenGK850WPlugin::OnSDKCommand(unsigned int, unsigned char*, unsigned int*) { return nullptr; }
void OpenGK850WPlugin::ProfileManagerUpdated(unsigned) {}
void OpenGK850WPlugin::ResourceManagerUpdated(unsigned) {}
void OpenGK850WPlugin::SettingsManagerUpdated(unsigned) {}
