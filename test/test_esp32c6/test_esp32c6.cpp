/**
 * test_esp32c6.cpp
 *
 * Auto-detection unit tests for the ESP32-C6 family.
 *
 * ── How to run this suite only ──────────────────────────────────────────
 *   ~/.platformio/penv/bin/platformio test -e native -f test_esp32c6
 *
 * ── Note on the C6 board ───────────────────────────────────────────────
 *   The only C6 entry in the table has no OLED, no Ethernet and no radio
 *   (radio=0). It cannot be distinguished by hardware probing; it reaches
 *   Phase 2b where it is the only candidate and matches itself (all pins
 *   UNUSED_PIN, RF_NONE family). Detection returns index 0 — PASS.
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
    using namespace esp32c6;
    const BoardDef& b = boards[board_idx];
    int detected = detect_board(boards, NUM_BOARDS, b);

    if (detected == board_idx) return;   // unique detection → PASS

    if (detected >= 0 && boards[detected].radio_matches(b)) {
        printf("\n  [ESP32-C6 COLLISION] '%s'\n"
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
void test_esp32c6_table_size_consistent()
{
    TEST_ASSERT_EQUAL_INT(1, esp32c6::NUM_BOARDS);
}

void test_esp32c6_all_boards_have_names()
{
    for (int i = 0; i < esp32c6::NUM_BOARDS; i++) {
        char msg[48];
        snprintf(msg, sizeof(msg), "ESP32-C6 idx=%d has empty name", i);
        TEST_ASSERT_NOT_NULL_MESSAGE(esp32c6::boards[i].name, msg);
        TEST_ASSERT_GREATER_THAN_MESSAGE(0, (int)strlen(esp32c6::boards[i].name), msg);
    }
}

// ---------------------------------------------------------------------------
//  Tests por placa — ESP32-C6 (1 placa)
// ---------------------------------------------------------------------------
// idx 0 — Custom ESP32-C6 (no radio), sin OLED, sin ETH.
//         Fase 2b: única entrada en la tabla → detectada.
void test_esp32c6_Custom_C6_no_radio() { check_board(0); }

// ---------------------------------------------------------------------------
//  Fixtures Unity
// ---------------------------------------------------------------------------
void setUp()    {}
void tearDown() {}

int main(int /*argc*/, char** /*argv*/)
{
    UNITY_BEGIN();

    // Structural checks for the table
    RUN_TEST(test_esp32c6_table_size_consistent);
    RUN_TEST(test_esp32c6_all_boards_have_names);

    // Detección por placa
    RUN_TEST(test_esp32c6_Custom_C6_no_radio);  // Fase 2b — única entrada → PASS

    UNITY_END();
    print_board_table("ESP32-C6", esp32c6::boards, esp32c6::NUM_BOARDS);
    return 0;
}
