#include "GnssManager.h"
#include "../Logger/Logger.h"
#include <time.h>
#include <sys/time.h>

// Sync every 24h minus 61 seconds (prime) to avoid locking to daily cycles
#define GNSS_SYNC_INTERVAL ((24 * 3600 - 61) * 1000) 
#define GNSS_TIMEOUT (15 * 60 * 1000)         // 15 Minutes max on time without fix

GnssManager::GnssManager() : serial(nullptr), enabled(false), powered(false), locationUpdated(false), lastSync(0), lastLocationUpdate(0), powerOnTime(0) {}

void GnssManager::begin() {
    board_t board;
    uint8_t boardIdx = ConfigManager::getInstance().getBoard();
    if (!ConfigManager::getInstance().getBoardConfig(board)) {
        Log::console(PSTR("GNSS: Failed to get board config"));
        return;
    }

    if (board.GNSS_RX != UNUSED && board.GNSS_TX != UNUSED) {
        Log::console(PSTR("GNSS: Initializing on RX:%d TX:%d"), board.GNSS_RX, board.GNSS_TX);
        
        // Wake up GNSS if pin is defined
        if (board.GNSS_WAKEUP != UNUSED) {
            pinMode(board.GNSS_WAKEUP, OUTPUT);
            digitalWrite(board.GNSS_WAKEUP, HIGH);
            Log::console(PSTR("GNSS: Wakeup pin %d set HIGH"), board.GNSS_WAKEUP);
        }

        Serial1.begin(9600, SERIAL_8N1, board.GNSS_RX, board.GNSS_TX);
        serial = &Serial1;
        enabled = true;
        
        // Initial Power On
        Power::getInstance().setGnssPower(true);
        powered = true;
        powerOnTime = millis();
    } else {
        Log::console(PSTR("GNSS: No GNSS pins defined for this board"));
    }
}

void GnssManager::loop() {
    if (!enabled) return;

    checkPowerCycle();

    if (powered && serial) {
        while (serial->available() > 0) {
            gps.encode(serial->read());
        }

        static unsigned long lastStatusLog = 0;
        if (millis() - lastStatusLog > 30000) {
            Log::debug(PSTR("GNSS Status: %s, Sats: %d, HDOP: %d"), 
                hasFix() ? "FIX" : "NO FIX", 
                gps.satellites.value(), 
                gps.hdop.value());
            lastStatusLog = millis();
        }

        if (hasFix()) {
            syncTime();

            // Auto Location Logic
            if (ConfigManager::getInstance().getAutoLocation()) {
                unsigned long interval = ConfigManager::getInstance().getGnssInterval() * 1000;
                // If interval > 0, update periodically. If 0, update once (if never updated).
                bool shouldUpdate = (interval > 0 && millis() - lastLocationUpdate > interval) || 
                                    (interval == 0 && lastLocationUpdate == 0);

                if (shouldUpdate) {
                    ConfigManager::getInstance().setLatRuntime(getLatitude());
                    ConfigManager::getInstance().setLonRuntime(getLongitude());
                    lastLocationUpdate = millis();
                    locationUpdated = true;
                    Log::console(PSTR("GNSS: Auto-updated location: %.3f, %.3f"), getLatitude(), getLongitude());
                }
            } else {
                // Legacy behavior: Only update if 0.0/0.0
                static bool fixLogged = false;
                if (!fixLogged && ConfigManager::getInstance().getLatitude() == 0.0 && ConfigManager::getInstance().getLongitude() == 0.0) {
                    char latBuf[16], lonBuf[16];
                    snprintf(latBuf, sizeof(latBuf), "%.3f", getLatitude());
                    snprintf(lonBuf, sizeof(lonBuf), "%.3f", getLongitude());
                    ConfigManager::getInstance().setLat(latBuf);
                    ConfigManager::getInstance().setLon(lonBuf);
                    Log::console(PSTR("GNSS: Updated station location to current fix (Legacy)."));
                    fixLogged = true;
                }
            }
        }
    }
}

void GnssManager::checkPowerCycle() {
    unsigned long now = millis();
    bool autoLoc = ConfigManager::getInstance().getAutoLocation();
    unsigned long interval = ConfigManager::getInstance().getGnssInterval() * 1000;

    // Auto Location Power Management
    if (autoLoc) {
        if (interval > 0) {
            // Periodic Mode
            if (powered) {
                // Turn off if we updated location in this session
                if (lastLocationUpdate > powerOnTime) {
                    Log::console(PSTR("GNSS: Location updated, powering down."));
                    Power::getInstance().setGnssPower(false);
                    powered = false;
                } else if (now - powerOnTime > GNSS_TIMEOUT) {
                    Log::console(PSTR("GNSS: Timeout (no fix), powering down."));
                    Power::getInstance().setGnssPower(false);
                    powered = false;
                }
            } else {
                // Turn on if interval passed
                if (now - lastLocationUpdate > interval) {
                    Log::console(PSTR("GNSS: Waking for periodic update."));
                    Power::getInstance().setGnssPower(true);
                    powered = true;
                    powerOnTime = now;
                }
            }
            return; // Skip legacy logic
        } else {
            // Interval 0 = Always On (or Once). 
            // If we interpret 0 as "Once", we should sleep after update.
            // But usually 0 means "Disabled" or "Continuous". 
            // Let's assume 0 with AutoLoc ON means "Continuous / Once at boot then Manual control?".
            // Let's keep it powered if AutoLoc is ON and Interval is 0, to behave like "Always On".
            if (!powered) {
                 Power::getInstance().setGnssPower(true);
                 powered = true;
                 powerOnTime = now;
            }
            return;
        }
    }

    // Legacy Power Management (Time Sync Only)
    if (powered) {
        // Turn off if synced recently
        if (now - lastSync < 60000 && lastSync != 0) { 
             Log::console(PSTR("GNSS: Time synced, powering down GNSS to save energy."));
             Power::getInstance().setGnssPower(false);
             powered = false;
        }
        else if (now - powerOnTime > GNSS_TIMEOUT) {
            Log::console(PSTR("GNSS: Timeout (no fix), powering down."));
            Power::getInstance().setGnssPower(false);
            powered = false;
        }
    }
    else {
        // Turn on if needed
        if (now - lastSync > GNSS_SYNC_INTERVAL) {
            Log::console(PSTR("GNSS: Waking up for daily sync."));
            Power::getInstance().setGnssPower(true);
            powered = true;
            powerOnTime = now;
        }
    }
}

bool GnssManager::hasFix() {
    return gps.location.isValid() && gps.location.age() < 2000;
}

uint32_t GnssManager::getSatellites() {
    return gps.satellites.value();
}

double GnssManager::getLatitude() {
    return gps.location.lat();
}

double GnssManager::getLongitude() {
    return gps.location.lng();
}

double GnssManager::getAltitude() {
    return gps.altitude.meters();
}

void GnssManager::syncTime() {
    if (!gps.time.isValid() || !gps.date.isValid()) return;
    
    // Check if we already synced recently (debounce)
    if (millis() - lastSync < 60000 && lastSync != 0) return;

    struct tm t = {0};
    t.tm_year = gps.date.year() - 1900;
    t.tm_mon = gps.date.month() - 1;
    t.tm_mday = gps.date.day();
    t.tm_hour = gps.time.hour();
    t.tm_min = gps.time.minute();
    t.tm_sec = gps.time.second();
    
    // Manual timegm equivalent for UTC
    t.tm_isdst = 0;
    uint32_t year = t.tm_year + 1900;
    uint32_t month = t.tm_mon + 1;
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    time_t timeVal = (86400ULL * (365L * year + year / 4 - year / 100 + year / 400 + (306 * month + 5) / 10 + (t.tm_mday - 1) - 719468L) + 
            3600ULL * t.tm_hour + 60ULL * t.tm_min + t.tm_sec);

    struct timeval now = { .tv_sec = timeVal, .tv_usec = 0 };
    settimeofday(&now, NULL);
    
    Log::console(PSTR("GNSS: Time synced to %04d-%02d-%02d %02d:%02d:%02d UTC"), 
                 gps.date.year(), gps.date.month(), gps.date.day(), 
                 gps.time.hour(), gps.time.minute(), gps.time.second());
    lastSync = millis();
}

bool GnssManager::isSleeping() {
    return enabled && !powered;
}