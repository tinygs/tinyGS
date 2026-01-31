#ifndef GnssManager_h
#define GnssManager_h

#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "../ConfigManager/ConfigManager.h"
#include "../Status.h"
#include "../Power/Power.h"

class GnssManager {
public:
    static GnssManager& getInstance() {
        static GnssManager instance;
        return instance;
    }

    void begin();
    void loop();
    bool hasFix();
    uint32_t getSatellites();
    double getLatitude();
    double getLongitude();
    double getAltitude();
    void syncTime();
    bool isEnabled() { return enabled; }
    bool isSleeping();
    bool isLocationUpdated() { return locationUpdated; }
    void clearLocationUpdated() { locationUpdated = false; }

private:
    GnssManager();
    void checkPowerCycle();
    TinyGPSPlus gps;
    HardwareSerial* serial;
    bool enabled; // Configured and pins available
    bool powered; // Currently powered on
    bool locationUpdated;
    bool fixLoggedThisCycle;
    unsigned long lastSync;
    unsigned long lastLocationUpdate;
    unsigned long powerOnTime;
};

#endif