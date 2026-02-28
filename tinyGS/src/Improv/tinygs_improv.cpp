#include "tinygs_improv.h"
#include <functional>

using namespace std;
using namespace placeholders;

// void TinyGSImprov::initImprovVersionInfo (uint32_t version) {
//     firmwareVersion = version;
// }

// void TinyGSImprov::onConnected (onConnected_t callback) {
//     onConnectedCb = callback;
// }

TinyGSImprov improvWiFi;

void TinyGSImprov::send_response (std::vector<uint8_t>& response) {
    std::vector<uint8_t> data = { 'I', 'M', 'P', 'R', 'O', 'V' };
    data.resize (9);
    data[6] = improv::IMPROV_SERIAL_VERSION;
    data[7] = improv::TYPE_RPC_RESPONSE;
    data[8] = response.size ();
    data.insert (data.end (), response.begin (), response.end ());

    uint8_t checksum = 0x00;
    for (uint8_t d : data)
        checksum += d;
    data.push_back (checksum);

    Serial.write (data.data (), data.size ());
}

void TinyGSImprov::set_state (improv::State state) {
    std::vector<uint8_t> data = { 'I', 'M', 'P', 'R', 'O', 'V' };
    data.resize (11);
    data[6] = improv::IMPROV_SERIAL_VERSION;
    data[7] = improv::TYPE_CURRENT_STATE;
    data[8] = 1;
    data[9] = state;

    uint8_t checksum = 0x00;
    for (uint8_t d : data)
        checksum += d;
    data[10] = checksum;

    Serial.write (data.data (), data.size ());
}

void TinyGSImprov::set_error (improv::Error error) {
    std::vector<uint8_t> data = { 'I', 'M', 'P', 'R', 'O', 'V' };
    data.resize (11);
    data[6] = improv::IMPROV_SERIAL_VERSION;
    data[7] = improv::TYPE_ERROR_STATE;
    data[8] = 1;
    data[9] = error;

    uint8_t checksum = 0x00;
    for (uint8_t d : data)
        checksum += d;
    data[10] = checksum;

    Serial.write (data.data (), data.size ());
}

std::vector<std::string> TinyGSImprov::getLocalUrl () {
    ConnectionManager& cm = ConnectionManager::getInstance();
    return {
      // URL where user can finish onboarding or use device
      // Recommended to use website hosted by device
      String ("http://admin:12345678@" + cm.getLocalIP().toString () + "/").c_str ()
    };
}

bool TinyGSImprov::connectWifi (std::string ssid, std::string password) {
    uint8_t count = 0;

    WiFi.enableAP (false);
    WiFi.mode (WIFI_STA);
    WiFi.disconnect ();
    WiFi.begin (ssid.c_str (), password.c_str ());

    while (WiFi.status () != WL_CONNECTED) {
        //blink_led (500, 1);
        delay (250);

        if (count > MAX_ATTEMPTS_WIFI_CONNECTION) {
            Serial.printf("Wifi status: %d\n", WiFi.status ());
            Serial.println("Failed to connect to wifi...");
            WiFi.disconnect ();
            return false;
        }
        count++;
    }
    set_state (improv::STATE_PROVISIONED);
    std::vector<uint8_t> data = improv::build_rpc_response (improv::WIFI_SETTINGS, getLocalUrl (), false);
    send_response (data);

    strncpy (globalConfigStore->wifiSSIDBuffer(), ssid.c_str (), globalConfigStore->wifiSSIDLen());
    strncpy (globalConfigStore->wifiPassBuffer(), password.c_str (), globalConfigStore->wifiPassLen());
    char* stationName = globalConfigStore->stationNameBuffer();
    int stationNameLength = globalConfigStore->stationNameLen();
    Serial.printf("Station name: %s\n", stationName);
    if (strncmp (stationName, "", stationNameLength) == 0) {
        strncpy (stationName, "TinyGS_Improv", stationNameLength);
        Serial.println("Station name was empty");
    }
    char* apPassword = globalConfigStore->apPasswordBuffer();
    int apPasswordLength = globalConfigStore->apPasswordLen();
    Serial.printf("AP password: %s\n", apPassword);
    if (strncmp (apPassword, "", apPasswordLength) == 0) {
        strncpy (apPassword, "12345678", apPasswordLength);
        Serial.println("AP password was empty");
    }
    globalConfigStore->save();
    delay (100);
    ESP.restart ();
    // Connection state is now managed by ConnectionManager
    // if (onConnectedCb != NULL) {
    //     onConnectedCb ();
    // }

    return true;
}

void TinyGSImprov::getAvailableWifiNetworks () {
    int networkNum = WiFi.scanNetworks ();

    for (int id = 0; id < networkNum; ++id) {
        std::vector<uint8_t> data = improv::build_rpc_response (
            improv::GET_WIFI_NETWORKS, { WiFi.SSID (id), String (WiFi.RSSI (id)), (WiFi.encryptionType (id) == WIFI_AUTH_OPEN ? "NO" : "YES") }, false);
        send_response (data);
        delay (1);
    }
    //final response
    std::vector<uint8_t> data =
        improv::build_rpc_response (improv::GET_WIFI_NETWORKS, std::vector<std::string>{}, false);
    send_response (data);
    WiFi.scanDelete ();
}

bool TinyGSImprov::onCommandCallback (improv::ImprovCommand cmd) {
    switch (cmd.command) {
    case improv::Command::GET_CURRENT_STATE:
    {
        if ((WiFi.status () == WL_CONNECTED)) {
            set_state (improv::State::STATE_PROVISIONED);
            std::vector<uint8_t> data = improv::build_rpc_response (improv::GET_CURRENT_STATE, getLocalUrl (), false);
            send_response (data);

        } else {
            set_state (improv::State::STATE_AUTHORIZED);
        }

        break;
    }
    case improv::Command::WIFI_SETTINGS:
    {
        if (cmd.ssid.length () == 0) {
            set_error (improv::Error::ERROR_INVALID_RPC);
            break;
        }

        set_state (improv::STATE_PROVISIONING);

        if (connectWifi (cmd.ssid, cmd.password)) {

            //TODO: Persist credentials here

            // set_state (improv::STATE_PROVISIONED);
            // std::vector<uint8_t> data = improv::build_rpc_response (improv::WIFI_SETTINGS, getLocalUrl (), false);
            // send_response (data);

        } else {
            set_state (improv::STATE_STOPPED);
            set_error (improv::Error::ERROR_UNABLE_TO_CONNECT);
        }

        break;
    }
    case improv::Command::GET_WIFI_NETWORKS:
    {
        getAvailableWifiNetworks ();
        break;
    }
    case improv::Command::GET_DEVICE_INFO: {
        std::string version = std::to_string (firmwareVersion);
        std::string platform = CONFIG_IDF_TARGET;
        std::transform(platform.begin(), platform.end(), platform.begin(), ::toupper);
        std::vector<std::string> infos = {
            // Firmware name
            "TinyGS",
            // Firmware version
            version,
            // Hardware chip/variant
            platform,
            // Device name
            "TinyGS"
            };
            std::vector<uint8_t> data = improv::build_rpc_response (improv::GET_DEVICE_INFO, infos, false);
            send_response (data);
            break;
    }
    default: {
        set_error (improv::ERROR_UNKNOWN_RPC);
        return false;
    }
    }

    return true;
}

void TinyGSImprov::onErrorCallback (improv::Error err) {}

void TinyGSImprov::handleImprovPacket () {
    while (Serial.available () > 0) {
        uint8_t b = Serial.read ();

        if (parse_improv_serial_byte (improvBufferPosition, b, improvBuffer,
                                      std::bind (&TinyGSImprov::onCommandCallback, this, _1),
                                      std::bind (&TinyGSImprov::onErrorCallback, this, _1)) &&
            (improvBufferPosition < IMPROV_BUFFER_SIZE)) { // Avoid buffer overflow
                improvBuffer[improvBufferPosition++] = b;
        } else {
            improvBufferPosition = 0;
        }
    }
}