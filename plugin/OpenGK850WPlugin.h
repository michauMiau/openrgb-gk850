#ifndef OPENGK850WPLUGIN_H
#define OPENGK850WPLUGIN_H

#include "OpenRGBPluginInterface.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <hidapi.h>

// Number of LEDs (61 for TKL layout, adjust as needed)
static constexpr int NUM_LEDS = 61;

// OpenRGB mode values (match RGBController.h)
#define MODE_OFF              0x00
#define MODE_STATIC           0x01
#define MODE_PER_KEY          0x15

// Device mode values used in the mode packet (Report ID 6, byte 0x15).
// NOTE: these come from the reference SinowealthKeyboardController (FL eSports
// F11, same chip) and match the PCAP captures. The GK850W patch in this repo
// used wrong values (0x83 static / 0x16 off) which send Bluetooth/off commands.
#define DEVICE_MODE_OFF              0x00   // Turn off lights
#define DEVICE_MODE_STATIC           0x01   // Static color (handled by SetStaticColor packet)
#define DEVICE_MODE_PER_KEY          0x15   // Per-key addressing

// Speed constants (from reference controller)
#define SPEED_SLOW                   0x12
#define SPEED_NORMAL                 0x22
#define SPEED_FAST                   0x32
#define SPEED_FASTEST                0x42

// Brightness constants
#define BRIGHTNESS_FULL              0x04

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
