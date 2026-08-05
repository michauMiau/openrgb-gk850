/*------------------------------------------*\
||  OpenGK850WPluginV2.cpp                 |
||                                            ||
||  Plugin for GK850W keyboard using         |
||  Virtual RGB Controller API               |
||                                            ||
||  garfi-kod 2026                            |
\*-----------------------------------------*/

#include "OpenGK850WPluginV2.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <iostream>

// Number of LEDs per zone — adjust to match your keyboard layout
// Common layouts: 87 (TKL), 96 (full compact), 104 (full-size)
constexpr int NUM_LEDS = 96; // Adjust for your layout (87/96/104)

void OpenGK850WPluginV2::Load(OpenRGBPluginAPIInterface* plugin_api_ptr) {
    api = plugin_api_ptr;
    printf("[GK850W V2] Plugin loading...\n");

    hid_init();
    dev_handle = hid_open(0x258A, 0x0049, NULL);

    if (!dev_handle) {
        printf("[GK850W V2] Device not found (VID:PID 258A:0049)\n");
        leds.resize(NUM_LEDS, ToRGBColor(0, 0, 0));
        return;
    }

    wchar_t product[128] = {0};
    hid_get_product_string(dev_handle, product, 127);
    std::wstring wprod(product);
    std::string prod(wprod.begin(), wprod.end());
    printf("[GK850W V2] Product string: %s\n", prod.c_str());

    if (prod.find("GK850") == std::string::npos) {
        printf("[GK850W V2] Not a GK850W device\n");
        hid_close(dev_handle);
        dev_handle = nullptr;
        leds.resize(NUM_LEDS, ToRGBColor(0, 0, 0));
        return;
    }

    // Initialize LEDs — adjust NUM_LEDS for your layout (87/96/104)
    leds.resize(NUM_LEDS, ToRGBColor(0, 0, 0));

    // Create virtual controller setup
    RGBController_Setup setup = {};
    setup.name = "GK850W Virtual";
    setup.vendor = "BY Tech";
    setup.description = "Sinowealth GK850W (Virtual Plugin)";
    setup.version = "2.0.0";
    setup.serial = "";
    setup.location = "";
    setup.type = DEVICE_TYPE_KEYBOARD;
    setup.flags = CONTROLLER_FLAGS_MANUALLY_CONFIGURABLE;

    // Add modes — only controller modes, no sound reactive (handled by Effects Plugin)
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
    setup.DeviceUpdateLEDs = [](void* arg) {
        auto* self = (OpenGK850WPluginV2*)arg;
        if (!self->dev_handle) return;

        if (self->current_mode == MODE_GAME || self->current_mode == 1) {
            // Custom mode - send per-key LED data (Report ID 6)
            unsigned char report[1032] = {0x06};
            for (int i = 0; i < NUM_LEDS && i < (int)self->leds.size(); i++) {
                RGBColor c = self->leds[i];
                report[i * 3 + 1] = c & 0xFF;
                report[i * 3 + 2] = (c >> 8) & 0xFF;
                report[i * 3 + 3] = (c >> 16) & 0xFF;
            }
            hid_send_feature_report(self->dev_handle, report, 1032);
        } else {
            // Built-in mode - send mode command (Report ID 5)
            unsigned char report[6] = {0x05, (unsigned char)self->current_mode, 0x83, 0x00, 0x00, 0x00};
            hid_send_feature_report(self->dev_handle, report, 6);
        }
    };

    setup.DeviceUpdateSingleLED = [](void* arg, int led_idx) {
        auto* self = (OpenGK850WPluginV2*)arg;
        if (!self->dev_handle || self->current_mode != MODE_GAME && self->current_mode != 1) return;

        unsigned char report[1032] = {0x06};
        for (int i = 0; i < NUM_LEDS && i < (int)self->leds.size(); i++) {
            RGBColor c = self->leds[i];
            report[i * 3 + 1] = c & 0xFF;
            report[i * 3 + 2] = (c >> 8) & 0xFF;
            report[i * 3 + 3] = (c >> 16) & 0xFF;
        }
        hid_send_feature_report(self->dev_handle, report, 1032);
    };

    setup.DeviceUpdateMode = [](void* arg) {
        auto* self = (OpenGK850WPluginV2*)arg;
        if (!self->dev_handle) return;

        unsigned char report[6] = {0x05, (unsigned char)self->current_mode, 0x83, 0x00, 0x00, 0x00};
        hid_send_feature_report(self->dev_handle, report, 6);
    };

    // Register self as object pointer for callbacks
    setup.object_ptr = this;

    virtual_controller = api->CreateVirtualRGBController(&setup);
    if (virtual_controller) {
        api->RegisterVirtualRGBController(virtual_controller);
        printf("[GK850W V2] Virtual controller registered!\n");
    } else {
        printf("[GK850W V2] Failed to create virtual controller\n");
    }
}

QWidget* OpenGK850WPluginV2::GetWidget() {
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

        // Send command to device
        unsigned char report[6] = {0x05, (unsigned char)current_mode, 0x83, 0x00, 0x00, 0x00};
        hid_send_feature_report(dev_handle, report, 6);

        status_label->setText(QString("Mode: %1").arg(mode_combo->currentText()));
    });

    layout->addWidget(mode_combo);
    layout->addWidget(apply_btn);

    return w;
}

QMenu* OpenGK850WPluginV2::GetTrayMenu() {
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

OpenRGBPluginInfo OpenGK850WPluginV2::GetPluginInfo() {
    OpenRGBPluginInfo info;
    info.Name = "GK850W V2";
    info.Description = "Virtual controller for GK850W keyboard - no main repo patch needed";
    info.Version = "2.0.0";
    info.Commit = "local-build";
    info.URL = "";
    info.Location = OPENRGB_PLUGIN_LOCATION_TOP;
    info.Label = "GK850W V2";
    info.ProtocolVersion = 6;
    return info;
}

void OpenGK850WPluginV2::Unload() {}

OpenGK850WPluginV2::~OpenGK850WPluginV2() {
    if (dev_handle) {
        hid_close(dev_handle);
        dev_handle = nullptr;
    }
}

void OpenGK850WPluginV2::OnProfileAboutToLoad() {}
void OpenGK850WPluginV2::OnProfileLoad(nlohmann::json) {}
nlohmann::json OpenGK850WPluginV2::OnProfileSave() { return nlohmann::json{}; }
unsigned char* OpenGK850WPluginV2::OnSDKCommand(unsigned int, unsigned char*, unsigned int*) { return nullptr; }
void OpenGK850WPluginV2::ProfileManagerUpdated(unsigned) {}
void OpenGK850WPluginV2::ResourceManagerUpdated(unsigned) {}
void OpenGK850WPluginV2::SettingsManagerUpdated(unsigned) {}
