#ifndef OPENGK850WPLUGIN_H
#define OPENGK850WPLUGIN_H

#include "OpenRGBPluginInterface.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <hidapi.h>

// Number of LEDs (61 for 60% layout - GK850W is a 60% keyboard)
static constexpr int NUM_LEDS = 61;

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
    QString debug_log = "no reports sent yet";
    QPlainTextEdit* widget_debug_log = nullptr;  // Reference to UI debug log

    void AddDebug(const QString& msg) {
        debug_log += msg + "\n";

        // Update UI if available: append and trim to the last 50 lines so the
        // widget shows the newest entries (oldest are removed).
        if(widget_debug_log) {
            widget_debug_log->setPlainText(debug_log);
            auto lines = debug_log.split('\n');
            if(lines.size() > 50) {
                lines = lines.mid(lines.size() - 50);
                debug_log = lines.join('\n');
                widget_debug_log->setPlainText(debug_log);
            }
            widget_debug_log->verticalScrollBar()->setValue(
                widget_debug_log->verticalScrollBar()->maximum());
        }
    }

    // Protocol helpers
    // PCAP-verified sequences:
    //   static: init1 + init2 -> color-data (RGB @ 29/30/31) -> commit_on
    //   off:    init1 + init2 -> color-data                -> commit_off
    //   perkey: init1         -> bc-data + c0-data         -> commit_game
    enum CommitType { COMMIT_ON, COMMIT_OFF, COMMIT_GAME };
    void SendInitCommands(bool full);
    void SendModeCommit(CommitType type);
    void SendStaticColorPacket(RGBColor color);
    void SendPerKeyPacket();
    void SendOffPacket(RGBColor last_color);

    void SetupKeyboardLayout(RGBController_Setup& setup);

    // Real-time streaming state: inits/commit only on mode entry.
    bool perkey_inited = false;
    bool perkey_needs_commit = false;
    // UI checkbox: skip ALL init reports and commits (pure data stream test).
    bool skip_init_reports = false;

    // Returns the currently active OpenRGB mode reading the controller
    unsigned int CurrentMode();
    void SyncModeFromController();
};

#endif // OPENGK850WPLUGIN_H
