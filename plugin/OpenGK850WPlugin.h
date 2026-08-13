#ifndef OPENGK850WPLUGIN_H
#define OPENGK850WPLUGIN_H

#include "OpenRGBPluginInterface.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <hidapi.h>

// Number of LEDs (86 for TKL layout - matches reference controller key map)
static constexpr int NUM_LEDS = 86;

// Mode values (from reference Sinowealth GK850W controller header).
// These are the ACTUAL mode values used in Report ID 5 commands.
// The previous values (0x01 static, 0x00 off) were incorrect and caused
// the device to not respond properly to mode changes.
#define MODE_OFF              0x16   // Turn off lights
#define MODE_STATIC           0x83   // Static color mode
#define MODE_PER_KEY          0x15   // Per-key / game mode

// Speed constants (from reference controller)
#define SPEED_SLOW            0x00
#define SPEED_NORMAL          0x40
#define SPEED_FAST            0x80
#define SPEED_FASTEST         0xC0

// Brightness constants (from reference controller)
#define BRIGHTNESS_OFF        0x00
#define BRIGHTNESS_MIN        0x01
#define BRIGHTNESS_MED        0x02
#define BRIGHTNESS_HI         0x03
#define BRIGHTNESS_FULL       0x04

// Report sizes
#define REPORT_SIZE_LED              1032   // Report ID 6 LED data
#define REPORT_SIZE_CMD              6      // Report ID 5 init command

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

    // Device management
    bool OpenDevice();
    void CloseDevice();
    void RescanDevice();

private:
    OpenRGBPluginAPIInterface* api = nullptr;
    hid_device* dev_handle = nullptr;

    std::vector<RGBColor> leds;
    unsigned int current_mode = MODE_OFF;
    RGBControllerInterface* virtual_controller = nullptr;

    // Protocol helpers
    void SendInitCommands();
    void SendModePacket(unsigned char mode, unsigned char speed, unsigned char brightness);
    void SendStaticColorPacket(RGBColor color);
    void SendPerKeyPacket();

    // Returns the currently active OpenRGB mode reading the controller
    unsigned int CurrentMode();
    void SyncModeFromController();
};

#endif // OPENGK850WPLUGIN_H
