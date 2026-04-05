/**
 * test_board_detection.cpp
 *
 * Unit tests for TinyGS board auto-detection logic.
 *
 * ── Detection algorithm (ConfigStore::boardDetection()) ─────────────────
 *
 *  Phase 1 — OLED probe (unique buses)  (all chip families)
 *    For each unique (SDA, SCL) I2C bus: probe OLED ACK → if found, collect
 *    all boards sharing that bus → radio probe among candidates → first match
 *    selected (fallback: first OLED candidate when radio inconclusive).
 *
 *  Phase 2 — Ethernet probe  (no OLED found)
 *    ETH probe simulated by target.eth_en==true → first ETH board in table.
 *
 *  Phase 2b — Radio-only boards  (no OLED, no ETH)
 *    Boards like Heltec WSL V3 detected purely by radio SPI probe.
 *
 * ── What the tests check ─────────────────────────────────────────────────
 *
 *  • For every board in the table a test function is generated.
 *  • OLED boards: simulate hardware = board's own config (OLED + SPI pins)
 *    and assert detect_board() returns that board's index.
 *    A collision occurs if two boards share identical (addr, SDA, SCL) AND
 *    identical (radio family, NSS, MISO, MOSI, SCK) — in that case the
 *    earlier board in the table wins and the test FAILS, documenting the
 *    known limitation.
 *    Key ambiguity: SX1276 and SX1278 are the same silicon (RF_SX127X);
 *    they respond identically to the radio SPI probe.
 *  • Ethernet boards: Phase 2 — target.eth_en=true → first ETH board wins.
 *  • Radio-only boards: Phase 2b — detected by radio SPI probe alone.
 *  • Sanity checks: table sizes, non-empty names, SDA ≠ SCL.
 *
 * Run on PC (no hardware needed):
 *   /path/to/pio test -e native --filter test_board_detection
 *   # or run the built binary directly:
 *   .pio/build/native/program
 *
 * All three chip families (ESP32, ESP32-S3, ESP32-C3) are tested in one binary.
 */

#include <unity.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "board_table.h"

// detect_board() is defined in board_table.h — simulates the 3-phase
// algorithm of ConfigStore::boardDetection() (OLED probe → ETH probe → radio-only).

// ---------------------------------------------------------------------------
//  Common check helper — called by every per-board test function
// ---------------------------------------------------------------------------

static void check_board(const char* chip,
                        const BoardDef* boards, int n, int board_idx) {
    const BoardDef& b = boards[board_idx];

    // Use the board's own config as the "hardware" being probed.
    // detect_board() handles all cases: OLED (Phase 1), ETH (Phase 2), radio-only (Phase 2b).
    int detected_idx = detect_board(boards, n, b);

    if (detected_idx == board_idx) {
        // Unique detection — test passes.
        return;
    }

    // Collision: an earlier board shadows this one (same OLED bus AND same
    // radio SPI family+pins, or radio probe fell back to OLED-only match).
    char msg[256];
    snprintf(msg, sizeof(msg),
             "[%s] idx=%d shadowed by idx=%d "
             "(OLED 0x%02X SDA=%u SCL=%u, radio type=%u NSS=%u). "
             "Shadowed: '%s'. Winner: '%s'.",
             chip, board_idx, detected_idx,
             b.oled_addr, b.oled_sda, b.oled_scl,
             b.radio_type, b.radio_nss,
             b.name, boards[detected_idx].name);
    TEST_FAIL_MESSAGE(msg);
}

// Legacy per-board test functions removed. They were previously
// the auto-generated `legacy_*` tests; these were deleted on
// user request. If archival is needed, move originals to
// test/legacy/ before reintroducing.

// ---------------------------------------------------------------------------
//  Additional sanity tests
// ---------------------------------------------------------------------------

/** Board table must not have more entries than NUM_BOARDS accounts for. */
void test_esp32_table_size_consistent() {
    TEST_ASSERT_EQUAL_INT(24, esp32::NUM_BOARDS);
}
void test_esp32s3_table_size_consistent() {
    TEST_ASSERT_EQUAL_INT(7, esp32s3::NUM_BOARDS);
}
void test_esp32c3_table_size_consistent() {
    TEST_ASSERT_EQUAL_INT(2, esp32c3::NUM_BOARDS);
}

/** Every OLED board must have a non-empty name. */
static void check_all_names(const char* chip, const BoardDef* boards, int n) {
    for (int i = 0; i < n; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "%s idx=%d has empty name", chip, i);
        TEST_ASSERT_NOT_NULL_MESSAGE(boards[i].name, msg);
        TEST_ASSERT_GREATER_THAN_MESSAGE(0, (int)strlen(boards[i].name), msg);
    }
}
void test_esp32_all_boards_have_names()   { check_all_names("ESP32",   esp32::boards,   esp32::NUM_BOARDS);   }
void test_esp32s3_all_boards_have_names() { check_all_names("ESP32-S3",esp32s3::boards, esp32s3::NUM_BOARDS); }
void test_esp32c3_all_boards_have_names() { check_all_names("ESP32-C3",esp32c3::boards, esp32c3::NUM_BOARDS); }

/** OLED SDA and SCL pins must differ on every OLED-capable board. */
static void check_sda_scl_differ(const char* chip, const BoardDef* boards, int n) {
    for (int i = 0; i < n; i++) {
        if (!boards[i].has_oled()) continue;
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "%s idx=%d '%s' has SDA==SCL==%u",
                 chip, i, boards[i].name, boards[i].oled_sda);
        TEST_ASSERT_NOT_EQUAL_MESSAGE(boards[i].oled_sda, boards[i].oled_scl, msg);
    }
}
void test_esp32_sda_scl_differ()   { check_sda_scl_differ("ESP32",   esp32::boards,   esp32::NUM_BOARDS);   }
void test_esp32s3_sda_scl_differ() { check_sda_scl_differ("ESP32-S3",esp32s3::boards, esp32s3::NUM_BOARDS); }
void test_esp32c3_sda_scl_differ() { check_sda_scl_differ("ESP32-C3",esp32c3::boards, esp32c3::NUM_BOARDS); }

// ---------------------------------------------------------------------------
//  This test set has been reorganized into per-SoC directories.
//  Run the new suites with:
//    pio test -e native -f test_esp32s3   (ESP32-S3)
//    pio test -e native -f test_esp32c3   (ESP32-C3)
//    pio test -e native -f test_esp32     (classic ESP32)
//    pio test -e native                   (all suites)
//
//  The file board_table.h remains here and is included by the three
//  new suites via "../test_board_detection/board_table.h".
// ---------------------------------------------------------------------------

void setUp()    {}
void tearDown() {}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    // No tests registered here — see test_esp32/, test_esp32s3/, test_esp32c3/

    UNITY_END();
    return 0;
}
