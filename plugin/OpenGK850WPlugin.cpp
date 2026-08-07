#include "OpenGK850WPlugin.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include "LogManager.h"

void OpenGK850WPlugin::Load(OpenRGBPluginAPIInterface* api) {
    this->api = api;
    LOG_INFO("[GK850W] Loading...\n");
    
    hid_init();
    dev_handle = hid_open(0x258A, 0x0049, NULL);
    
    leds.resize(NUM_LEDS, ToRGBColor(0, 0, 0));
    
    RGBController_Setup setup = {};
    setup.name = "GK850";
    setup.vendor = "BY Tech";
    setup.description = "Sinowealth GK850W (Virtual Plugin)";
    setup.version = "2.1.0";
    setup.type = DEVICE_TYPE_KEYBOARD;
    
    mode m;
    m.name = "Static";
    m.value = MODE_STATIC;
    m.color_mode = MODE_COLORS_MODE_SPECIFIC;
    m.colors.resize(1, ToRGBColor(0xFF, 0x00, 0x00));
    setup.modes.push_back(m);
    
    m.name = "Custom";
    m.value = MODE_GAME;
    m.color_mode = MODE_COLORS_PER_LED;
    setup.modes.push_back(m);
    
    m.name = "Off";
    m.value = MODE_OFF;
    m.color_mode = MODE_COLORS_NONE;
    setup.modes.push_back(m);
    
    zone z;
    z.name = "Keyboard";
    z.type = ZONE_TYPE_MATRIX;
    z.leds_count = NUM_LEDS;
    setup.zones.push_back(z);
    
    setup.DeviceUpdateLEDs = [](void* arg) {
        auto* self = (OpenGK850WPlugin*)arg;
        if (!self->dev_handle) return;
        
        unsigned char report[1032] = {0x06};
        int count = std::min(NUM_LEDS, (int)self->leds.size());
        for (int i = 0; i < count; i++) {
            RGBColor c = self->leds[i];
            report[1 + i*3]     = RGBGetBValue(c);
            report[1 + i*3 + 1] = RGBGetGValue(c);
            report[1 + i*3 + 2] = RGBGetRValue(c);
        }
        hid_send_feature_report(self->dev_handle, report, 1032);
    };
    
    setup.DeviceUpdateMode = [](void* arg) {
        auto* self = (OpenGK850WPlugin*)arg;
        if (!self->dev_handle) return;
        
        unsigned char report[6] = {0x05, 0x15, 0x22, 0x04, 0x00, 0x00};
        hid_send_feature_report(self->dev_handle, report, 6);
    };
    
    setup.object_ptr = this;
    virtual_controller = api->CreateVirtualRGBController(&setup);
    if (virtual_controller) {
        api->RegisterVirtualRGBController(virtual_controller);
        LOG_INFO("[GK850W] Registered!\n");
    }
}

QWidget* OpenGK850WPlugin::GetWidget() {
    QWidget* w = new QWidget();
    auto* layout = new QVBoxLayout(w);
    layout->addWidget(new QLabel(dev_handle ? "GK850W Connected" : "GK850W Virtual"));
    return w;
}

QMenu* OpenGK850WPlugin::GetTrayMenu() {
    return new QMenu("GK850W");
}

OpenRGBPluginInfo OpenGK850WPlugin::GetPluginInfo() {
    OpenRGBPluginInfo info;
    info.Name = "GK850W";
    info.Description = "Virtual controller";
    info.Version = "2.1.0";
    info.Location = OPENRGB_PLUGIN_LOCATION_TOP;
    info.ProtocolVersion = 6;
    return info;
}

void OpenGK850WPlugin::Unload() {}
OpenGK850WPlugin::~OpenGK850WPlugin() {
    if (dev_handle) hid_close(dev_handle);
}
void OpenGK850WPlugin::OnProfileAboutToLoad() {}
void OpenGK850WPlugin::OnProfileLoad(nlohmann::json) {}
nlohmann::json OpenGK850WPlugin::OnProfileSave() { return nlohmann::json{}; }
unsigned char* OpenGK850WPlugin::OnSDKCommand(unsigned int, unsigned char*, unsigned int*) { return nullptr; }
void OpenGK850WPlugin::ProfileManagerUpdated(unsigned) {}
void OpenGK850WPlugin::ResourceManagerUpdated(unsigned) {}
void OpenGK850WPlugin::SettingsManagerUpdated(unsigned) {}
