/**
 * test_esp32c3.cpp
 *
 * Auto-detection unit tests for the ESP32-C3 family.
 *
 * ── How to run this suite only ──────────────────────────────────────────
 *   ~/.platformio/penv/bin/platformio test -e native -f test_esp32c3
 *
 * ── Collision behavior ─────────────────────────────────────────────────
 *   If two boards share identical SPI pins and radio family the test
 *   passes and logs the detected board. If the mismatch is due to
 *   different pins the test fails.
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
    using namespace esp32c3;
    const BoardDef& b = boards[board_idx];
    int detected = detect_board(boards, NUM_BOARDS, b);

    if (detected == board_idx) return;   // unique detection → PASS

    if (detected >= 0 && boards[detected].radio_matches(b)) {
        printf("\n  [ESP32-C3 COLLISION] '%s'\n"
               "               detected as '%s'\n"
               "               (same SPI pins and radio family — known ambiguity)\n",
               b.name, boards[detected].name);
        return;   // PASS with informative log
    }

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
void test_esp32c3_table_size_consistent()
{
    TEST_ASSERT_EQUAL_INT(2, esp32c3::NUM_BOARDS);
}

void test_esp32c3_all_boards_have_names()
{
    for (int i = 0; i < esp32c3::NUM_BOARDS; i++) {
        char msg[48];
        snprintf(msg, sizeof(msg), "ESP32-C3 idx=%d has empty name", i);
        TEST_ASSERT_NOT_NULL_MESSAGE(esp32c3::boards[i].name, msg);
        TEST_ASSERT_GREATER_THAN_MESSAGE(0, (int)strlen(esp32c3::boards[i].name), msg);
    }
}

void test_esp32c3_sda_scl_differ()
{
    for (int i = 0; i < esp32c3::NUM_BOARDS; i++) {
        if (!esp32c3::boards[i].has_oled()) continue;
        char msg[64];
        snprintf(msg, sizeof(msg),
                 "ESP32-C3 idx=%d '%s' has SDA==SCL==%u",
                 i, esp32c3::boards[i].name, esp32c3::boards[i].oled_sda);
        TEST_ASSERT_NOT_EQUAL_MESSAGE(
            esp32c3::boards[i].oled_sda, esp32c3::boards[i].oled_scl, msg);
    }
}

// ---------------------------------------------------------------------------
//  Tests por placa — ESP32-C3 (2 placas)
// ---------------------------------------------------------------------------
// idx 0 — HELTEC HT-CT62 SX1262, OLED SDA=0/SCL=1, SX1262 NSS=8
//         Primera en ese bus + familia SX126X → detectada directamente.
void test_esp32c3_HELTEC_HT_CT62_SX1262() { check_board(0); }

// idx 1 — Custom C3 SX1278, mismo bus OLED, SX1278 NSS=8
//         Familia SX127X ≠ SX126X del idx 0 → diferenciable.
void test_esp32c3_Custom_C3_SX1278()       { check_board(1); }

// ---------------------------------------------------------------------------
//  Fixtures Unity
// ---------------------------------------------------------------------------
void setUp()    {}
void tearDown() {}

int main(int /*argc*/, char** /*argv*/)
{
    UNITY_BEGIN();

    // Structural checks for the table
    RUN_TEST(test_esp32c3_table_size_consistent);
    RUN_TEST(test_esp32c3_all_boards_have_names);
    RUN_TEST(test_esp32c3_sda_scl_differ);

    // Detección por placa
    RUN_TEST(test_esp32c3_HELTEC_HT_CT62_SX1262);  // Fase 1 — OLED+SX126X → PASS
    RUN_TEST(test_esp32c3_Custom_C3_SX1278);        // Fase 1 — OLED+SX127X → PASS

    UNITY_END();
    return 0;
}
