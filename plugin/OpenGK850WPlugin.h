#ifndef OPENGK850WPLUGIN_H
#define OPENGK850WPLUGIN_H

#include "OpenRGBPluginInterface.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <hidapi.h>

static constexpr int NUM_LEDS = 61;

// OpenRGB mode constants (from RGBController.h)
#ifndef MODE_STATIC
#define MODE_STATIC 1
#endif
#ifndef MODE_GAME
#define MODE_GAME 2  
#endif
#ifndef MODE_OFF
#define MODE_OFF 3
#endif

// Device mode constants from PCAP
#define DEVICE_MODE_OFF              0x16
#define DEVICE_MODE_PER_KEY          0x15
#define DEVICE_MODE_STATIC           0x01

class OpenGK850WPlugin : public QObject, public OpenRGBPluginInterface
{
    Q_OBJECT
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
    unsigned int current_mode = 0;
    RGBControllerInterface* virtual_controller = nullptr;
};

#endif
