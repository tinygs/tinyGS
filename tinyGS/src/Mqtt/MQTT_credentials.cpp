#include "MQTT_credentials.h"
#include <esp_log.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <stdlib.h> // For random number generation
#include "../certs.h"
#include "../Logger/Logger.h"
#include <ArduinoJson.h>

#if TEST_API==1
const char* API_URL = "https://api.test.tinygs.com/credentials?otp=";
#else
const char* API_URL = "https://api.tinygs.com/credentials?otp=";
#endif

MQTTCredentials mqttCredentials;

void MQTTCredentials::generateOTPCode () {
    snprintf (otpCode, OTP_LENGTH, "%06ld", random (100000, 1000000));
    Log::console ("Generated OTP: %s", otpCode);
    url = String (API_URL) + otpCode;
}

char* MQTTCredentials::getOTPCode () {
    return otpCode;
}

String MQTTCredentials::fetchCredentials () {
    time_t now = millis ();
    if (now > httpLastChecked + CHECKEVERY) {
        httpLastChecked = now;
        Log::debug ("Fetching URL: %s", url.c_str ());
        Log::console("Trying to get MQTT credentials with %s", url.c_str ());

        HTTPClient http;
        http.setTimeout (3000);
        http.begin (url, ISRGroot_CA);
        int httpResponseCode = http.GET ();

        if (httpResponseCode == 200) {
            String payload = http.getString ();
            Log::debug ("Received 200 OK, payload: %s", payload.c_str ());
            http.end ();
            return payload;
        } else {
            Log::debug ("Received status code: %d", httpResponseCode);
            http.end ();
            return "";
        }
    }
    return "";
}
    
// bool getMqttData () {
// // Existing logic for getting MQTT data
//     ESP_LOGI (LOG_TAG, "Getting MQTT data");
//     xTaskCreate (generateAndFetchCredentialsTask, "generateAndFetchCredentialsTask", 4096, NULL, 5, NULL);
//     while (taskRunning) {
//         delay (1000);
//     }
//     Serial.println ("Task completed");
//     return true; // Placeholder
// }
