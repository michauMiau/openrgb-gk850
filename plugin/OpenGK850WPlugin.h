#ifndef OPENGK850WPLUGIN_H
#define OPENGK850WPLUGIN_H

#include "OpenRGBPluginInterface.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <hidapi.h>

// Number of LEDs (61 for TKL layout, adjust as needed)
static constexpr int NUM_LEDS = 61;

// Device mode constants from PCAP analysis
#define DEVICE_MODE_OFF              0x16   // Report ID 5: turn off lights
#define DEVICE_MODE_PER_KEY          0x15   // Report ID 5: enable per-key addressing
#define DEVICE_MODE_STATIC           0x01   // Report ID 5: static color mode

// Speed constants from PCAP analysis
#define SPEED_SLOW                   0x12
#define SPEED_NORMAL                 0x22
#define SPEED_FASTER                 0x32
#define SPEED_FASTEST                0x42

// Brightness levels (PCAP: 0x01-0x04 = quarter to full)
#define BRIGHTNESS_OFF               0x00
#define BRIGHTNESS_QUARTER           0x01
#define BRIGHTNESS_HALF              0x02
#define BRIGHTNESS_THREE_QUARTERS    0x03
#define BRIGHTNESS_FULL              0x04

class OpenGK850WPlugin : public QObject, public OpenRGBPluginInterface
{
    Q_OBJECT
    // Use FILE parameter with JSON that has TOP-LEVEL keys (like other plugins)
    Q_PLUGIN_METADATA(IID "org.openrgb.OpenRGBPluginInterface" FILE "OpenGK850WPlugin.json")
    Q_INTERFACES(OpenRGBPluginInterface)

public:
    ~OpenGK850WPlugin() override;

    void Load(OpenRGBPluginAPIInterface* api) override;
    void Unload() override;

    QWidget* GetWidget() override;
    QMenu* GetTrayMenu() override;

    OpenRGBPluginInfo GetPluginInfo() override;
    unsigned int GetPluginAPIVersion() override { return OPENRGB_PLUGIN_API_VERSION; }

    void OnProfileAboutToLoad() override;
    void OnProfileLoad(nlohmann::json profile_data) override;
    nlohmann::json OnProfileSave() override;
    unsigned char* OnSDKCommand(unsigned int pkt_id, unsigned char * pkt_data, unsigned int *pkt_size) override;
    void ProfileManagerUpdated(unsigned int update_reason) override;
    void ResourceManagerUpdated(unsigned int update_reason) override;
    void SettingsManagerUpdated(unsigned int update_reason) override;

private:
    OpenRGBPluginAPIInterface* api = nullptr;
    hid_device* dev_handle = nullptr;
    
    std::vector<RGBColor> leds;
    unsigned int current_mode = MODE_OFF;
    RGBControllerInterface* virtual_controller = nullptr;
};

#endif // OPENGK850WPLUGIN_H
