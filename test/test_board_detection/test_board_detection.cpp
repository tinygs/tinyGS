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

// ===========================================================================
//  Per-board test functions — ESP32 classic
// ===========================================================================

/* clang-format off */
void test_esp32_board_00() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS,  0); }
void test_esp32_board_01() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS,  1); }
void test_esp32_board_02() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS,  2); }
void test_esp32_board_03() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS,  3); }
void test_esp32_board_04() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS,  4); }
void test_esp32_board_05() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS,  5); }
void test_esp32_board_06() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS,  6); }
void test_esp32_board_07() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS,  7); }
void test_esp32_board_08() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS,  8); }
void test_esp32_board_09() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS,  9); }
void test_esp32_board_10() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 10); }
void test_esp32_board_11() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 11); }
void test_esp32_board_12() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 12); }
void test_esp32_board_13() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 13); }
void test_esp32_board_14() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 14); }
void test_esp32_board_15() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 15); }
void test_esp32_board_16() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 16); }
void test_esp32_board_17() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 17); }
void test_esp32_board_18() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 18); }
void test_esp32_board_19() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 19); }
void test_esp32_board_20() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 20); }
void test_esp32_board_21() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 21); }
void test_esp32_board_22() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 22); }
void test_esp32_board_23() { check_board("ESP32", esp32::boards, esp32::NUM_BOARDS, 23); }

// ===========================================================================
//  Per-board test functions — ESP32-S3
// ===========================================================================

void test_esp32s3_board_00() { check_board("ESP32-S3", esp32s3::boards, esp32s3::NUM_BOARDS, 0); }
void test_esp32s3_board_01() { check_board("ESP32-S3", esp32s3::boards, esp32s3::NUM_BOARDS, 1); }
void test_esp32s3_board_02() { check_board("ESP32-S3", esp32s3::boards, esp32s3::NUM_BOARDS, 2); }
void test_esp32s3_board_03() { check_board("ESP32-S3", esp32s3::boards, esp32s3::NUM_BOARDS, 3); }
void test_esp32s3_board_04() { check_board("ESP32-S3", esp32s3::boards, esp32s3::NUM_BOARDS, 4); }
void test_esp32s3_board_05() { check_board("ESP32-S3", esp32s3::boards, esp32s3::NUM_BOARDS, 5); }
void test_esp32s3_board_06() { check_board("ESP32-S3", esp32s3::boards, esp32s3::NUM_BOARDS, 6); }

// ===========================================================================
//  Per-board test functions — ESP32-C3
// ===========================================================================

void test_esp32c3_board_00() { check_board("ESP32-C3", esp32c3::boards, esp32c3::NUM_BOARDS, 0); }
void test_esp32c3_board_01() { check_board("ESP32-C3", esp32c3::boards, esp32c3::NUM_BOARDS, 1); }

/* clang-format on */

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
//  Entry point
// ---------------------------------------------------------------------------

void setUp()    { /* nothing */ }
void tearDown() { /* nothing */ }

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();

    // ── Sanity / structural tests ─────────────────────────────────────────
    RUN_TEST(test_esp32_table_size_consistent);
    RUN_TEST(test_esp32s3_table_size_consistent);
    RUN_TEST(test_esp32c3_table_size_consistent);

    RUN_TEST(test_esp32_all_boards_have_names);
    RUN_TEST(test_esp32s3_all_boards_have_names);
    RUN_TEST(test_esp32c3_all_boards_have_names);

    RUN_TEST(test_esp32_sda_scl_differ);
    RUN_TEST(test_esp32s3_sda_scl_differ);
    RUN_TEST(test_esp32c3_sda_scl_differ);

    // ── Per-board detection tests — ESP32 classic (24 boards) ────────────
    // PASS = board uniquely identified by OLED + radio SPI probe.
    // FAIL = collision: an earlier board shares the same (bus, radio family,
    //        SPI pins) and is selected instead.
    // Key residual ambiguity: SX1276 and SX1278 are identical silicon
    // (RF_SX127X) and cannot be distinguished by SPI probe.
    RUN_TEST(test_esp32_board_00);  // 433MHz HELTEC WiFi LoRA 32 V1       → PASS  unique (SDA=4,SCL=15, SX127X NSS=18)
    RUN_TEST(test_esp32_board_01);  // 863-928MHz HELTEC WiFi LoRA 32 V1   → FAIL  SX1276≡SX1278, same SPI as idx 0
    RUN_TEST(test_esp32_board_02);  // 433MHz HELTEC WiFi LoRA 32 V2       → FAIL  same SPI as idx 0
    RUN_TEST(test_esp32_board_03);  // 863-928MHz HELTEC WiFi LoRA 32 V2   → FAIL  SX1276≡SX1278, same SPI as idx 0
    RUN_TEST(test_esp32_board_04);  // 433MHz TTGO LoRa 32 V1              → FAIL  same SPI as idx 0
    RUN_TEST(test_esp32_board_05);  // 868-915MHz TTGO LoRa 32 V1          → FAIL  SX1276≡SX1278, same SPI as idx 0
    RUN_TEST(test_esp32_board_06);  // 433MHz TTGO LoRA 32 V2              → PASS  unique (SDA=21,SCL=22, SX127X NSS=18)
    RUN_TEST(test_esp32_board_07);  // 868-915MHz TTGO LoRA 32 V2          → FAIL  SX1276≡SX1278, same SPI as idx 6
    RUN_TEST(test_esp32_board_08);  // 433MHz T-BEAM + OLED                → FAIL  same SPI as idx 6
    RUN_TEST(test_esp32_board_09);  // 868-915MHz T-BEAM + OLED            → FAIL  SX1276≡SX1278, same SPI as idx 6
    RUN_TEST(test_esp32_board_10);  // Custom ESP32 + SX126x (Crystal)     → PASS  unique (SX126X, NSS=5, MOSI=23, SCK=18)
    RUN_TEST(test_esp32_board_11);  // TTGO V2 + SX126x (crystal)          → PASS  unique (SX126X, NSS=18, MOSI=27, SCK=5)
    RUN_TEST(test_esp32_board_12);  // Custom + DRF1268T TCX0 (5,2,26,13)  → FAIL  same SPI as idx 10
    RUN_TEST(test_esp32_board_13);  // Custom + DRF1268T TCX0 (5,26,14,12) → FAIL  same SPI as idx 10
    RUN_TEST(test_esp32_board_14);  // 433MHz T-BEAM V1.0 + OLED           → FAIL  SX127X same SPI as idx 6
    RUN_TEST(test_esp32_board_15);  // 433MHz FOSSA 1W Ground Station       → PASS  unique (SX126X, NSS=5, MOSI=27, SCK=18)
    RUN_TEST(test_esp32_board_16);  // 868-915MHz FOSSA 1W Ground Station   → PASS  unique (SX127X, NSS=5, MOSI=27, SCK=18)
    RUN_TEST(test_esp32_board_17);  // 2.4GHz ESP32 + SX1280               → PASS  unique (SX1280 family)
    RUN_TEST(test_esp32_board_18);  // 868-915MHz T-BEAM V1.0 + OLED       → FAIL  SX1276≡SX1278 same SPI as idx 6
    RUN_TEST(test_esp32_board_19);  // 433MHz LILYGO T3_V1.6.1             → FAIL  same SPI as idx 6
    RUN_TEST(test_esp32_board_20);  // 868-915MHz LILYGO T3_V1.6.1         → FAIL  SX1276≡SX1278 same SPI as idx 6
    RUN_TEST(test_esp32_board_21);  // 868-915MHz LILYGO T3_V1.6.1 TCXO   → FAIL  SX1276≡SX1278 same SPI as idx 6
    RUN_TEST(test_esp32_board_22);  // 433MHz T-Beam SX1268 V1.0           → FAIL  SX126X same SPI as idx 11
    RUN_TEST(test_esp32_board_23);  // WT32-ETH01 (LAN8720)                → PASS  Phase 2 ETH probe (only ETH board)

    // ── Per-board detection tests — ESP32-S3 (7 boards) ────────────────────
    // All OLED boards now uniquely identified thanks to different radio SPI
    // families or different SPI pins — no remaining collisions.
    RUN_TEST(test_esp32s3_board_00);  // HELTEC LORA32 V3         → PASS  first on (SDA=17,SCL=18), SX126X
    RUN_TEST(test_esp32s3_board_01);  // Custom S3 433MHz SX1278   → PASS  different radio family (SX127X) on same SPI
    RUN_TEST(test_esp32s3_board_02);  // TTGO T-Beam SX1262        → PASS  unique SPI (NSS=10, MISO=13)
    RUN_TEST(test_esp32s3_board_03);  // LILYGO SX1280             → PASS  unique SPI family (SX1280)
    RUN_TEST(test_esp32s3_board_04);  // Heltec WSL V3 (no OLED)   → PASS  Phase 2b radio-only probe (unique SX1262 SPI)
    RUN_TEST(test_esp32s3_board_05);  // Waveshare ETH (W5500)     → PASS  Phase 2 ETH probe (only ETH board)
    RUN_TEST(test_esp32s3_board_06);  // EoRa-HUB LR1121           → PASS  unique bus (SDA=18, SCL=17)

    // ── Per-board detection tests — ESP32-C3 (2 boards) ──────────────────
    // Both boards uniquely identified — different radio families on same SPI.
    RUN_TEST(test_esp32c3_board_00);  // HELTEC HT-CT62 SX1262    → PASS  first, SX126X
    RUN_TEST(test_esp32c3_board_01);  // Custom C3 433MHz SX1278   → PASS  different radio family (SX127X)

    return UNITY_END();
}
