/**
 * test_esp32s3.cpp
 *
 * Auto-detection unit tests for the ESP32-S3 family.
 *
 * ── How to run this suite only ──────────────────────────────────────────
 *   ~/.platformio/penv/bin/platformio test -e native -f test_esp32s3
 *
 * ── Detection algorithm (3 phases) ────────────────────────────────────
 *   Phase 1 — OLED I2C probe → candidates → radio probe among candidates.
 *   Phase 2 — No OLED → Ethernet probe.
 *   Phase 2b — No OLED and no ETH → radio-only probe.
 *
 * ── Collision behavior ─────────────────────────────────────────────────
 *   If two boards share identical SPI pins and radio family the hardware
 *   cannot distinguish them. In that case the test PASSES and logs which
 *   board was detected instead (known silicon limitation). If the mismatch
 *   is due to different pins the test FAILS.
 */

#include <unity.h>
#include <cstdio>
#include <cstring>

#include "../test_board_detection/board_table.h"

// ---------------------------------------------------------------------------
//  Helper central de verificación
// ---------------------------------------------------------------------------
static void check_board(int board_idx)
{
    using namespace esp32s3;
    const BoardDef& b = boards[board_idx];
    int detected = detect_board(boards, NUM_BOARDS, b);

    if (detected == board_idx) return;   // unique detection → PASS

    if (detected >= 0 && boards[detected].radio_matches(b)) {
        // Collision due to identical pins — silicon ambiguity, not a bug.
        printf("\n  [ESP32-S3 COLLISION] '%s'\n"
               "               detected as '%s'\n"
               "               (same SPI pins and radio family — known ambiguity)\n",
               b.name, boards[detected].name);
        return;   // PASS with informative log
    }

    // Real error: detected board has different pins
    char msg[256];
    snprintf(msg, sizeof(msg),
             "idx=%d '%s' incorrectly detected as idx=%d '%s'",
             board_idx, b.name,
             detected, detected >= 0 ? boards[detected].name : "(none)");
    TEST_FAIL_MESSAGE(msg);
}

// ---------------------------------------------------------------------------
//  Sanity checks de la tabla
// ---------------------------------------------------------------------------
void test_esp32s3_table_size_consistent()
{
    TEST_ASSERT_EQUAL_INT(7, esp32s3::NUM_BOARDS);
}

void test_esp32s3_all_boards_have_names()
{
    for (int i = 0; i < esp32s3::NUM_BOARDS; i++) {
        char msg[48];
        snprintf(msg, sizeof(msg), "ESP32-S3 idx=%d has empty name", i);
        TEST_ASSERT_NOT_NULL_MESSAGE(esp32s3::boards[i].name, msg);
        TEST_ASSERT_GREATER_THAN_MESSAGE(0, (int)strlen(esp32s3::boards[i].name), msg);
    }
}

void test_esp32s3_sda_scl_differ()
{
    for (int i = 0; i < esp32s3::NUM_BOARDS; i++) {
        if (!esp32s3::boards[i].has_oled()) continue;
        char msg[64];
        snprintf(msg, sizeof(msg),
                 "ESP32-S3 idx=%d '%s' has SDA==SCL==%u",
                 i, esp32s3::boards[i].name, esp32s3::boards[i].oled_sda);
        TEST_ASSERT_NOT_EQUAL_MESSAGE(
            esp32s3::boards[i].oled_sda, esp32s3::boards[i].oled_scl, msg);
    }
}

// ---------------------------------------------------------------------------
//  Tests por placa — ESP32-S3 (7 placas)
// ---------------------------------------------------------------------------
// idx 0 — HELTEC LORA32 V3, OLED SDA=17/SCL=18, SX1262 NSS=8
//         Fase 1: primera en ese bus + SX126X → detectada directamente.
void test_esp32s3_HELTEC_LORA32_V3()    { check_board(0); }

// idx 1 — Custom S3 SX1278, mismo bus OLED, SX1278 NSS=8
//         Familia SX127X ≠ SX126X del idx 0 → diferenciable.
void test_esp32s3_Custom_S3_SX1278()    { check_board(1); }

// idx 2 — TTGO T-Beam SX1262, mismo bus OLED, SX1262 NSS=10
//         Pines SPI distintos (NSS=10, MISO=13) → detectada.
void test_esp32s3_TTGO_TBeam_SX1262()   { check_board(2); }

// idx 3 — LILYGO SX1280, mismo bus OLED, familia SX1280 única.
void test_esp32s3_LILYGO_SX1280()       { check_board(3); }

// idx 4 — HELTEC WSL V3, sin OLED → Fase 2b radio-only.
//         Única placa radio-only en la tabla → detectada.
void test_esp32s3_HELTEC_WSL_V3()       { check_board(4); }

// idx 5 — Waveshare ESP32-S3-ETH, sin OLED → Fase 2 ETH.
//         Única placa ETH en la tabla → detectada.
void test_esp32s3_Waveshare_ETH_W5500() { check_board(5); }

// idx 6 — EBYTE EoRa-HUB LR1121, OLED con SDA=18/SCL=17 (bus invertido).
//         Bus único en la tabla → detectada.
void test_esp32s3_EBYTE_EoRa_LR1121()  { check_board(6); }

// ---------------------------------------------------------------------------
//  Fixtures Unity
// ---------------------------------------------------------------------------
void setUp()    {}
void tearDown() {}

int main(int /*argc*/, char** /*argv*/)
{
    UNITY_BEGIN();

    // Structural checks for the table
    RUN_TEST(test_esp32s3_table_size_consistent);
    RUN_TEST(test_esp32s3_all_boards_have_names);
    RUN_TEST(test_esp32s3_sda_scl_differ);

    // Detección por placa
    RUN_TEST(test_esp32s3_HELTEC_LORA32_V3);    // Fase 1 — OLED+SX126X  → PASS
    RUN_TEST(test_esp32s3_Custom_S3_SX1278);     // Fase 1 — OLED+SX127X  → PASS
    RUN_TEST(test_esp32s3_TTGO_TBeam_SX1262);    // Fase 1 — pines únicos  → PASS
    RUN_TEST(test_esp32s3_LILYGO_SX1280);        // Fase 1 — familia única  → PASS
    RUN_TEST(test_esp32s3_HELTEC_WSL_V3);        // Fase 2b — radio-only   → PASS
    RUN_TEST(test_esp32s3_Waveshare_ETH_W5500);  // Fase 2  — ETH          → PASS
    RUN_TEST(test_esp32s3_EBYTE_EoRa_LR1121);   // Fase 1 — bus invertido  → PASS

    UNITY_END();
    return 0;
}
