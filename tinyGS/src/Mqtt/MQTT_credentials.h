#ifndef MQTT_CREDENTIALS_H
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "../ConfigManager/ConfigManager.h"
#include "../Status.h"

class MQTTCredentials {
public:
    String fetchCredentials ();
    void generateOTPCode ();
    char* getOTPCode ();
private:
    const time_t CHECKEVERY = 15000;
    time_t httpLastChecked = 10000;
    static const size_t OTP_LENGTH = 7;
    char otpCode[OTP_LENGTH];
    String url;
    const char* LOG_TAG = "MQTT_CRED";

};

extern MQTTCredentials mqttCredentials;

#endif // MQTT_CREDENTIALS_H