/**
 * board_table.h
 *
 * Pure-C++ mirror of the board table defined in ConfigStore.cpp.
 * No Arduino dependencies – usable in native PC unit tests.
 *
 * All three chip families (ESP32, ESP32-S3, ESP32-C3) are defined here
 * so a single native binary can test all of them at once.
 *
 * Detection algorithm (mirrors ConfigStore::boardDetection()):
 *   Phase 1 — OLED probe (unique buses only)
 *     For each unique (SDA, SCL) bus: simulate I2C ACK → first bus that matches
 *     target (addr, SDA, SCL) wins → collect OLED-matching candidates →
 *     radio probe among them → return first confirm (or first candidate fallback).
 *   Phase 2 — Ethernet probe (no OLED found)
 *     If target.eth_en: return first ETH board in table.
 *   Phase 2b — Radio-only boards (no OLED, no ETH)
 *     Among boards where oled_addr==0 && !eth_en: first radio_matches → return.
 *
 * Note on radio family: SX1276 and SX1278 are the same silicon;
 * they cannot be distinguished by SPI probe. Boards sharing identical
 * SPI pins AND the same radio family remain ambiguous — the test for the
 * shadowed board FAILs, documenting the known limitation.
 */

#pragma once
#include <cstdint>
#include <cstring>

static constexpr uint8_t UNUSED_PIN = 255;

// Radio type constants (mirror ConfigStore.h RadioModelNum_t)
static constexpr uint8_t RT_SX1278 =  1;
static constexpr uint8_t RT_SX1276 =  2;
static constexpr uint8_t RT_SX1268 =  5;
static constexpr uint8_t RT_SX1262 =  6;
static constexpr uint8_t RT_SX1280 =  8;
static constexpr uint8_t RT_LR1121 = 10;
static constexpr uint8_t RT_LR2021 = 20;
static constexpr uint8_t RT_NONE   =  0;  // ethernet-only boards

// Radio families (for probe disambiguation — matches firmware probeRadioSpi logic)
enum RadioFamily : uint8_t {
    RF_NONE   = 0,
    RF_SX127X = 1,  // SX1276, SX1278 (same silicon — indistinguishable by SPI)
    RF_SX126X = 2,  // SX1262, SX1268
    RF_SX1280 = 3,
    RF_LR11XX = 4,  // LR1121, LR2021
};

/** Converts a radio type enum to its probe family. */
static RadioFamily radioFamily(uint8_t rt) {
    switch (rt) {
        case RT_SX1278: case RT_SX1276: return RF_SX127X;
        case RT_SX1268: case RT_SX1262: return RF_SX126X;
        case RT_SX1280:                 return RF_SX1280;
        case RT_LR1121: case RT_LR2021: return RF_LR11XX;
        default:                        return RF_NONE;
    }
}

/**
 * Minimal board descriptor — contains only the fields used for auto-detection.
 * Fields map 1:1 to ConfigStore board_t members.
 */
struct BoardDef {
    // OLED I2C
    uint8_t     oled_addr;   ///< 0 = no OLED
    uint8_t     oled_sda;
    uint8_t     oled_scl;
    uint8_t     oled_rst;    ///< UNUSED_PIN when absent

    // Ethernet flag (no radio / no OLED)
    bool        eth_en;

    // Radio SPI
    uint8_t     radio_type;  ///< RT_* constant above; RT_NONE for eth-only boards
    uint8_t     radio_nss;
    uint8_t     radio_miso;
    uint8_t     radio_mosi;
    uint8_t     radio_sck;

    const char* name;

    // Helpers -----------------------------------------------------------

    /** True if this board uses OLED I2C probe (not ethernet-only). */
    bool has_oled() const { return oled_addr != 0 && !eth_en; }

    /**
     * True if @p other's (NSS, MISO, MOSI, SCK, radio family) matches ours.
     * This is the radio SPI probe check: same physical wiring AND same chip
     * family respond identically on the SPI bus.
     */
    bool radio_matches(const BoardDef& other) const {
        return radio_nss    == other.radio_nss   &&
               radio_miso   == other.radio_miso  &&
               radio_mosi   == other.radio_mosi  &&
               radio_sck    == other.radio_sck   &&
               radioFamily(radio_type) == radioFamily(other.radio_type);
    }
};

// ---------------------------------------------------------------------------
//  Detection simulation (mirrors ConfigStore::boardDetection 3-phase algorithm)
// ---------------------------------------------------------------------------

/**
 * Simulates boardDetection() for unit testing.
 *
 * "Hardware" is described by @p target — the board physically present.
 * We scan @p boards exactly as the firmware does:
 *
 * Phase 1 — OLED probe
 *   For each unique (SDA, SCL) bus in the table (OLED boards only):
 *     Simulate I2C ACK: bus (addr, SDA, SCL) matches target.
 *     First bus to ACK → record (oledAddr, oledSda, oledScl); break.
 *
 *   If OLED found → Phase 3a: radio probe among OLED-matching candidates.
 *     Candidates = all boards with same (addr, SDA, SCL) as detected bus.
 *     First candidate where radio_matches(target) → return its index.
 *     Fallback (radio inconclusive) → first candidate.
 *
 * Phase 2 — No OLED → Ethernet probe (simulated: target.eth_en == true).
 *   ETH probe always succeeds for the target board; return the first ETH
 *   board in the table (probe order = table order, same as firmware).
 *
 * Phase 2b — No OLED, no ETH → radio-only boards.
 *   Candidates = boards where oled_addr==0 AND !eth_en.
 *   First candidate where radio_matches(target) → return its index.
 *
 * Returns -1 if no board was detected.
 */
static int detect_board(const BoardDef* boards, int n, const BoardDef& target) {

    // ── Phase 1: OLED probe ────────────────────────────────────────────────
    uint8_t oled_addr = 0, oled_sda = UNUSED_PIN, oled_scl = UNUSED_PIN;

    {
        uint8_t tried_sda[8], tried_scl[8];
        int tried_count = 0;

        for (int i = 0; i < n && oled_addr == 0; i++) {
            if (boards[i].oled_addr == 0) continue;

            uint8_t sda = boards[i].oled_sda;
            uint8_t scl = boards[i].oled_scl;

            // Skip buses already probed
            bool already = false;
            for (int t = 0; t < tried_count; t++) {
                if (tried_sda[t] == sda && tried_scl[t] == scl) { already = true; break; }
            }
            if (already) continue;
            if (tried_count < 8) { tried_sda[tried_count] = sda; tried_scl[tried_count++] = scl; }

            // Simulate I2C ACK: target must have the same (addr, SDA, SCL)
            if (boards[i].oled_addr == target.oled_addr &&
                sda                 == target.oled_sda  &&
                scl                 == target.oled_scl) {
                oled_addr = boards[i].oled_addr;
                oled_sda  = sda;
                oled_scl  = scl;
            }
        }
    }

    if (oled_addr != 0) {
        // ── Phase 3a: radio probe among OLED-matching candidates ──────────
        int first_candidate = -1;
        for (int i = 0; i < n; i++) {
            if (boards[i].oled_addr != oled_addr) continue;
            if (boards[i].oled_sda  != oled_sda)  continue;
            if (boards[i].oled_scl  != oled_scl)  continue;
            if (first_candidate < 0) first_candidate = i;
            if (boards[i].radio_matches(target)) return i;
        }
        return first_candidate;  // fallback: first OLED-matching board
    }

    // ── Phase 2: No OLED → Ethernet probe ─────────────────────────────────
    // ETH probe success is simulated by target.eth_en == true.
    if (target.eth_en) {
        for (int i = 0; i < n; i++) {
            if (boards[i].eth_en) return i;  // first ETH board in table wins
        }
    }

    // ── Phase 2b: No OLED, no ETH → radio-only boards ─────────────────────
    for (int i = 0; i < n; i++) {
        if (boards[i].oled_addr != 0) continue;  // skip OLED boards
        if (boards[i].eth_en)         continue;  // skip ETH boards
        if (boards[i].radio_matches(target)) return i;
    }

    return -1;  // no board detected
}

// ===========================================================================
//  Board tables — one namespace per chip family
// ===========================================================================

// Struct field order: oled_addr, oled_sda, oled_scl, oled_rst, eth_en,
//                    radio_type, radio_nss, radio_miso, radio_mosi, radio_sck,
//                    name

// ---------------------------------------------------------------------------
//  ESP32 classic
// ---------------------------------------------------------------------------
namespace esp32 {

static const BoardDef boards[] = {
    // ── OLED bus (addr=0x3C, SDA=4, SCL=15) ────────────────────────────
    // idx  0  HELTEC_V1_LF    SX1278  NSS=18 MISO=19 MOSI=27 SCK=5
    { 0x3c, 4, 15, 16, false, RT_SX1278, 18, 19, 27, 5,
      "433MHz HELTEC WiFi LoRA 32 V1"                                      },
    // idx  1  HELTEC_V1_HF    SX1276  NSS=18 MISO=19 MOSI=27 SCK=5  ← same family/pins as 0
    { 0x3c, 4, 15, 16, false, RT_SX1276, 18, 19, 27, 5,
      "863-928MHz HELTEC WiFi LoRA 32 V1"                                  },
    // idx  2  HELTEC_V2_LF    SX1278  same SPI as 0
    { 0x3c, 4, 15, 16, false, RT_SX1278, 18, 19, 27, 5,
      "433MHz HELTEC WiFi LoRA 32 V2"                                      },
    // idx  3  HELTEC_V2_HF    SX1276  same SPI as 0
    { 0x3c, 4, 15, 16, false, RT_SX1276, 18, 19, 27, 5,
      "863-928MHz HELTEC WiFi LoRA 32 V2"                                  },
    // idx  4  TTGO_V1_LF      SX1278  same SPI as 0
    { 0x3c, 4, 15, 16, false, RT_SX1278, 18, 19, 27, 5,
      "433MHz TTGO LoRa 32 V1"                                             },
    // idx  5  TTGO_V1_HF      SX1276  same SPI as 0
    { 0x3c, 4, 15, 16, false, RT_SX1276, 18, 19, 27, 5,
      "868-915MHz TTGO LoRa 32 V1"                                         },

    // ── OLED bus (addr=0x3C, SDA=21, SCL=22) ───────────────────────────
    // idx  6  TTGO_V2_LF      SX1278  NSS=18 MISO=19 MOSI=27 SCK=5
    { 0x3c, 21, 22, UNUSED_PIN, false, RT_SX1278, 18, 19, 27, 5,
      "433MHz TTGO LoRA 32 V2"                                             },
    // idx  7  TTGO_V2_HF      SX1276  same SPI as 6  ← same family/pins
    { 0x3c, 21, 22, 16,        false, RT_SX1276, 18, 19, 27, 5,
      "868-915MHz TTGO LoRA 32 V2"                                         },
    // idx  8  TBEAM_OLED_LF   SX1278  same SPI as 6
    { 0x3c, 21, 22, 16,        false, RT_SX1278, 18, 19, 27, 5,
      "433MHz T-BEAM + OLED"                                               },
    // idx  9  TBEAM_OLED_HF   SX1276  same SPI as 6
    { 0x3c, 21, 22, 16,        false, RT_SX1276, 18, 19, 27, 5,
      "868-915MHz T-BEAM + OLED"                                           },
    // idx 10  ESP32_SX126X_XTAL  SX1268  NSS=5 MISO=19 MOSI=23 SCK=18  ← unique
    { 0x3c, 21, 22, 16,        false, RT_SX1268,  5, 19, 23, 18,
      "Custom ESP32 Wroom + SX126x (Crystal)"                              },
    // idx 11  TTGO_V2_SX126X_XTAL  SX1268  NSS=18 MISO=19 MOSI=27 SCK=5 ← SX126x on same pins as 6
    { 0x3c, 21, 22, UNUSED_PIN, false, RT_SX1268, 18, 19, 27, 5,
      "TTGO LoRa 32 V2 Modified with SX126x (crystal)"                     },
    // idx 12  DRF1268T (5,2,26,13)  SX1268  NSS=5 MISO=19 MOSI=23 SCK=18 ← same SPI as 10
    { 0x3c, 21, 22, 16,        false, RT_SX1268,  5, 19, 23, 18,
      "Custom ESP32 Wroom + SX126x DRF1268T TCX0 (5,2,26,13)"             },
    // idx 13  DRF1268T (5,26,14,12)  SX1268  same SPI as 10
    { 0x3c, 21, 22, 16,        false, RT_SX1268,  5, 19, 23, 18,
      "Custom ESP32 Wroom + SX126x DRF1268T TCX0 (5,26,14,12)"            },
    // idx 14  TBEAM_V1.0       SX1278  same SPI as 6
    { 0x3c, 21, 22, UNUSED_PIN, false, RT_SX1278, 18, 19, 27, 5,
      "433MHz T-BEAM V1.0 + OLED"                                          },
    // idx 15  FOSSA_LF         SX1268  NSS=5 MISO=19 MOSI=27 SCK=18  ← unique
    { 0x3c, 21, 22, 16,        false, RT_SX1268,  5, 19, 27, 18,
      "433MHz FOSSA 1W Ground Station"                                      },
    // idx 16  FOSSA_HF         SX1276  NSS=5 MISO=19 MOSI=27 SCK=18  ← SX127x on (NSS=5,SCK=18)
    { 0x3c, 21, 22, 16,        false, RT_SX1276,  5, 19, 27, 18,
      "868-915MHz FOSSA 1W Ground Station"                                  },
    // idx 17  ESP32_SX1280     SX1280  NSS=5 MISO=19 MOSI=27 SCK=18  ← unique (SX1280 family)
    { 0x3c, 21, 22, UNUSED_PIN, false, RT_SX1280,  5, 19, 27, 18,
      "2.4GHz ESP32 + SX1280"                                              },
    // idx 18  TBEAM_V1.0_HF    SX1276  same SPI as 6
    { 0x3c, 21, 22, UNUSED_PIN, false, RT_SX1276, 18, 19, 27, 5,
      "868-915MHz T-BEAM V1.0 + OLED"                                      },
    // idx 19  LILYGO_T3_LF     SX1278  same SPI as 6
    { 0x3c, 21, 22, UNUSED_PIN, false, RT_SX1278, 18, 19, 27, 5,
      "433MHz LILYGO T3_V1.6.1"                                            },
    // idx 20  LILYGO_T3_HF     SX1276  same SPI as 6
    { 0x3c, 21, 22, UNUSED_PIN, false, RT_SX1276, 18, 19, 27, 5,
      "868-915MHz LILYGO T3_V1.6.1"                                        },
    // idx 21  LILYGO_T3_TCXO   SX1276  same SPI as 6
    { 0x3c, 21, 22, UNUSED_PIN, false, RT_SX1276, 18, 19, 27, 5,
      "868-915MHz LILYGO T3_V1.6.1 TCXO"                                  },
    // idx 22  TBEAM_SX1268     SX1268  NSS=18 MISO=19 MOSI=27 SCK=5 ← SX126x, same SPI as 11
    { 0x3c, 21, 22, UNUSED_PIN, false, RT_SX1268, 18, 19, 27, 5,
      "433MHz T-Beam SX1268 V1.0"                                          },

    // ── Ethernet-only (no OLED) ─────────────────────────────────────────
    // idx 23  WT32_ETH01
    { 0, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, true,
      RT_NONE, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN,
      "WT32-ETH01/02 (LAN8720)"                                            },
};

static constexpr int NUM_BOARDS = static_cast<int>(sizeof(boards) / sizeof(boards[0]));

} // namespace esp32

// ---------------------------------------------------------------------------
//  ESP32-S3
// ---------------------------------------------------------------------------
namespace esp32s3 {

static const BoardDef boards[] = {
    // ── OLED bus (addr=0x3C, SDA=17, SCL=18) ───────────────────────────
    // idx 0  HELTEC_LORA32_V3  SX1262  NSS=8 MISO=11 MOSI=10 SCK=9
    { 0x3c, 17, 18, 21,         false, RT_SX1262,  8, 11, 10, 9,
      "150-960MHz HELTEC LORA32 V3 SX1262"                                 },
    // idx 1  Custom S3 SX1278   SX1278  NSS=8 MISO=11 MOSI=10 SCK=9  ← SX127x on same SPI as 0
    { 0x3c, 17, 18, UNUSED_PIN, false, RT_SX1278,  8, 11, 10, 9,
      "Custom ESP32-S3 433MHz SX1278"                                      },
    // idx 2  TTGO T-Beam SX1262  SX1262  NSS=10 MISO=13 MOSI=11 SCK=12  ← unique SPI
    { 0x3c, 17, 18, UNUSED_PIN, false, RT_SX1262, 10, 13, 11, 12,
      "433MHz TTGO T-Beam Sup SX1262 V1.0"                                 },
    // idx 3  LILYGO SX1280       SX1280  NSS=7 MISO=3 MOSI=6 SCK=5  ← unique
    { 0x3c, 17, 18, UNUSED_PIN, false, RT_SX1280,  7,  3,  6, 5,
      "2.4GHz LILYGO SX1280"                                               },

    // ── No OLED, no ETH — radio-only boards (cannot be auto-detected) ───
    // idx 4  Heltec WSL V3  SX1262  same radio SPI as idx 0, no OLED
    { 0,    UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, false, RT_SX1262,  8, 11, 10, 9,
      "150-960Mhz - HELTEC WSL V3 SX1262"                                  },

    // ── Ethernet-only (no OLED) ─────────────────────────────────────────
    // idx 5  Waveshare ETH
    { 0,    UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, true,
      RT_NONE, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN,
      "Waveshare ESP32-S3-ETH (W5500)"                                     },

    // ── OLED bus (addr=0x3C, SDA=18, SCL=17) ← SDA/SCL swapped vs idx 0-3
    // idx 6  EoRa-HUB LR1121   LR1121  NSS=8 MISO=11 MOSI=10 SCK=9  ← unique LR11xx family
    { 0x3c, 18, 17, 21,         false, RT_LR1121,  8, 11, 10, 9,
      "EBYTE EoRa-HUB ESP32S3 + LR1121"                                    },
};

static constexpr int NUM_BOARDS = static_cast<int>(sizeof(boards) / sizeof(boards[0]));

} // namespace esp32s3

// ---------------------------------------------------------------------------
//  ESP32-C3
// ---------------------------------------------------------------------------
namespace esp32c3 {

static const BoardDef boards[] = {
    // ── OLED bus (addr=0x3C, SDA=0, SCL=1) ─────────────────────────────
    // idx 0  HELTEC HT-CT62  SX1262  NSS=8 MISO=6 MOSI=7 SCK=10
    { 0x3c, 0, 1, UNUSED_PIN, false, RT_SX1262,  8, 6, 7, 10,
      "433MHz HELTEC LORA32 HT-CT62 SX1262"                                },
    // idx 1  Custom C3 SX1278  SX1278  NSS=8 MISO=6 MOSI=7 SCK=10  ← SX127x on same SPI
    { 0x3c, 0, 1, UNUSED_PIN, false, RT_SX1278,  8, 6, 7, 10,
      "Custom ESP32-C3 433MHz SX1278"                                       },
};

static constexpr int NUM_BOARDS = static_cast<int>(sizeof(boards) / sizeof(boards[0]));

} // namespace esp32c3
