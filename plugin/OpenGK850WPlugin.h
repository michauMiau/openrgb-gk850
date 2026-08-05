#ifndef OPENGK850WPLUGIN_H
#define OPENGK850WPLUGIN_H

#include "OpenRGBPluginInterface.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <hidapi.h>

// Sinowealth mode constants
#define MODE_STATIC              0x83
#define MODE_BREATHING           0x84
#define MODE_TRANSITION          0x86
#define MODE_FLASHING_STARS      0x87
#define MODE_WATER_DROP          0x88
#define MODE_DOUBLE_TRANSITION   0x89
#define MODE_SHADOW              0x8a
#define MODE_SNAKE               0x8b
#define MODE_NEON_WAVE           0x8c
#define MODE_MARK                0x8d
#define MODE_SINE_WAVE           0x8e
#define MODE_SCANNING            0x8f
#define MODE_CAROUSEL            0x90
#define MODE_WATERFALL           0x91
#define MODE_ILLUMINATE_LINE     0x92
#define MODE_RAIN                0x93
#define MODE_PULSING             0x12
#define MODE_COLLISION           0x13
#define MODE_FLASH               0x14
#define MODE_GAME                0x15   // Addressable Mode for this keyboard
#define MODE_OFF                 0x16

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
    
    // State for sound reactive mode
    std::vector<RGBColor> leds;
    unsigned int current_mode = MODE_OFF;
    RGBControllerInterface* virtual_controller = nullptr;
};

#endif // OPENGK850WPLUGINV2_H
