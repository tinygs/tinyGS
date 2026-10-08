#ifndef TINYGS_IMPROV_H
#include <Arduino.h>
#include <improv.h>
#include <WiFi.h>
#include "../ConfigManager/ConfigManager.h"

//typedef std::function<void ()> onConnected_t;
const uint8_t IMPROV_BUFFER_SIZE = 128;

class TinyGSImprov {
public:
    void handleImprovPacket ();
    bool isParsing () const { return improvBufferPosition > 0; }
    void expirePartialFrame ();
    //void initImprovVersionInfo (uint32_t version);
    //void onConnected (onConnected_t callback);
    TinyGSImprov () : globalConfigManager (&ConfigManager::getInstance ()) {}
    void setVersion (uint32_t version) {
        firmwareVersion = version;
    }

private:

    const uint8_t MAX_ATTEMPTS_WIFI_CONNECTION = 20;
    const uint32_t PARTIAL_FRAME_TIMEOUT_MS = 100;

    uint8_t improvBuffer[IMPROV_BUFFER_SIZE];
    uint8_t improvBufferPosition = 0;
    uint32_t improvLastByteAt = 0;
    uint32_t firmwareVersion;
    //onConnected_t onConnectedCb = NULL;
    ConfigManager* globalConfigManager; // Reference to singleton instance

    //onConnected_t _callback;

    void send_response (std::vector<uint8_t>& response);
    void set_state (improv::State state);
    void set_error (improv::Error error);
    std::vector<std::string> getLocalUrl ();
    bool connectWifi (std::string ssid, std::string password);
    void getAvailableWifiNetworks ();
    bool onCommandCallback (improv::ImprovCommand cmd);
    void onErrorCallback (improv::Error err);
};

extern TinyGSImprov improvWiFi;

#endif // TINYGS_IMPROV_H