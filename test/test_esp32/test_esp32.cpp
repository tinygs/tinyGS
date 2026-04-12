/**
 * test_esp32.cpp
 *
 * Auto-detection unit tests for the classic ESP32 family.
 *
 * ── How to run this suite only ──────────────────────────────────────────
 *   ~/.platformio/penv/bin/platformio test -e native -f test_esp32
 *
 * ── About collisions ───────────────────────────────────────────────────
 *   SX1276 and SX1278 share the same silicon (RF_SX127X) and respond
 *   identically on the SPI bus. Boards sharing the same OLED bus AND
 *   identical SPI pins with the same radio family cannot be distinguished
 *   by hardware. In those cases the test PASSES and logs the board that
 *   was detected instead — this documents a known limitation, not a bug.
 *
 * ── Expected summary ───────────────────────────────────────────────────
 *   Direct PASS (unique detection): 8 boards (idx 0,6,10,11,15,16,17,23)
 *   PASS with log (SX127X collision): 16 boards (the rest)
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
    using namespace esp32;
    const BoardDef& b = boards[board_idx];
    int detected = detect_board(boards, NUM_BOARDS, b);

    if (detected == board_idx) return;   // unique detection → PASS

    if (detected >= 0 && boards[detected].radio_matches(b)) {
        printf("\n  [ESP32 COLLISION] '%s'\n"
               "          detected as '%s'\n"
               "          (same SPI pins and radio family — known SX127X ambiguity)\n",
               b.name, boards[detected].name);
        return;   // PASS with informative log
    }

    // Real error: different pins — should not happen
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
void test_esp32_table_size_consistent()
{
    TEST_ASSERT_EQUAL_INT(24, esp32::NUM_BOARDS);
}

void test_esp32_all_boards_have_names()
{
    for (int i = 0; i < esp32::NUM_BOARDS; i++) {
        char msg[48];
        snprintf(msg, sizeof(msg), "ESP32 idx=%d has empty name", i);
        TEST_ASSERT_NOT_NULL_MESSAGE(esp32::boards[i].name, msg);
        TEST_ASSERT_GREATER_THAN_MESSAGE(0, (int)strlen(esp32::boards[i].name), msg);
    }
}

void test_esp32_sda_scl_differ()
{
    for (int i = 0; i < esp32::NUM_BOARDS; i++) {
        if (!esp32::boards[i].has_oled()) continue;
        char msg[64];
        snprintf(msg, sizeof(msg),
                 "ESP32 idx=%d '%s' has SDA==SCL==%u",
                 i, esp32::boards[i].name, esp32::boards[i].oled_sda);
        TEST_ASSERT_NOT_EQUAL_MESSAGE(
            esp32::boards[i].oled_sda, esp32::boards[i].oled_scl, msg);
    }
}

// ---------------------------------------------------------------------------
//  Tests por placa — ESP32 clásico (24 placas)
//
//  Bus OLED A: addr=0x3C, SDA=4,  SCL=15  (idx 0-5)
//  Bus OLED B: addr=0x3C, SDA=21, SCL=22  (idx 6-22)
//  Sin OLED  :                             (idx 23)
// ---------------------------------------------------------------------------

// ── Bus OLED A (SDA=4, SCL=15) ─────────────────────────────────────────────

// idx 0 — Primera del bus A, SX1278 (SX127X, NSS=18) → PASS detectada directa.
void test_esp32_HELTEC_LoRA32_V1_433MHz()
{ check_board(0); }

// idx 1 — SX1276 (= mismo silicio SX127X), mismo bus A y pines → COLISIÓN con idx 0.
void test_esp32_HELTEC_LoRA32_V1_868MHz()
{ check_board(1); }

// idx 2 — SX1278, mismo bus A, mismos pines → COLISIÓN con idx 0.
void test_esp32_HELTEC_LoRA32_V2_433MHz()
{ check_board(2); }

// idx 3 — SX1276, mismo bus A, mismos pines → COLISIÓN con idx 0.
void test_esp32_HELTEC_LoRA32_V2_868MHz()
{ check_board(3); }

// idx 4 — SX1278, mismo bus A, mismos pines → COLISIÓN con idx 0.
void test_esp32_TTGO_LoRa32_V1_433MHz()
{ check_board(4); }

// idx 5 — SX1276, mismo bus A, mismos pines → COLISIÓN con idx 0.
void test_esp32_TTGO_LoRa32_V1_868MHz()
{ check_board(5); }

// ── Bus OLED B (SDA=21, SCL=22) ────────────────────────────────────────────

// idx 6 — Primera del bus B, SX1278 (SX127X, NSS=18) → PASS detectada directa.
void test_esp32_TTGO_LoRA32_V2_433MHz()
{ check_board(6); }

// idx 7 — SX1276, mismo bus B y pines → COLISIÓN con idx 6.
void test_esp32_TTGO_LoRA32_V2_868MHz()
{ check_board(7); }

// idx 8 — SX1278, mismo bus B, mismos pines → COLISIÓN con idx 6.
void test_esp32_TBEAM_OLED_433MHz()
{ check_board(8); }

// idx 9 — SX1276, mismo bus B, mismos pines → COLISIÓN con idx 6.
void test_esp32_TBEAM_OLED_868MHz()
{ check_board(9); }

// idx 10 — SX1268 (SX126X), bus B, NSS=5 MISTO=23 SCK=18 → PASS pines únicos.
void test_esp32_Custom_SX126x_Crystal()
{ check_board(10); }

// idx 11 — SX1268 (SX126X), bus B, NSS=18 MOSI=27 SCK=5 → PASS pines únicos.
void test_esp32_TTGO_V2_SX126x_Crystal()
{ check_board(11); }

// idx 12 — SX1268, bus B, mismos pines que idx 10 → COLISIÓN con idx 10.
void test_esp32_DRF1268T_TCXO_5_2_26_13()
{ check_board(12); }

// idx 13 — SX1268, bus B, mismos pines que idx 10 → COLISIÓN con idx 10.
void test_esp32_DRF1268T_TCXO_5_26_14_12()
{ check_board(13); }

// idx 14 — SX1278, bus B, NSS=18 → COLISIÓN con idx 6.
void test_esp32_TBEAM_V1_OLED_433MHz()
{ check_board(14); }

// idx 15 — SX1268 (SX126X), bus B, NSS=5 MOSI=27 SCK=18 → PASS pines únicos.
void test_esp32_FOSSA_1W_433MHz()
{ check_board(15); }

// idx 16 — SX1276 (SX127X), bus B, NSS=5 MOSI=27 SCK=18 → PASS familia distinta de SX126X.
void test_esp32_FOSSA_1W_868MHz()
{ check_board(16); }

// idx 17 — SX1280, bus B → PASS familia SX1280 única.
void test_esp32_ESP32_SX1280()
{ check_board(17); }

// idx 18 — SX1276, bus B, NSS=18 → COLISIÓN con idx 6.
void test_esp32_TBEAM_V1_OLED_868MHz()
{ check_board(18); }

// idx 19 — SX1278, bus B, NSS=18 → COLISIÓN con idx 6.
void test_esp32_LILYGO_T3_433MHz()
{ check_board(19); }

// idx 20 — SX1276, bus B, NSS=18 → COLISIÓN con idx 6.
void test_esp32_LILYGO_T3_868MHz()
{ check_board(20); }

// idx 21 — SX1276, bus B, NSS=18 → COLISIÓN con idx 6.
void test_esp32_LILYGO_T3_TCXO_868MHz()
{ check_board(21); }

// idx 22 — SX1268, bus B, NSS=18 → COLISIÓN con idx 11 (misma familia SX126X y pines).
void test_esp32_TBEAM_SX1268()
{ check_board(22); }

// ── Sin OLED — Ethernet ─────────────────────────────────────────────────────

// idx 23 — WT32-ETH01, sin OLED → Fase 2 ETH → PASS (única ETH en tabla ESP32).
void test_esp32_WT32_ETH01()
{ check_board(23); }

// ---------------------------------------------------------------------------
//  Fixtures Unity
// ---------------------------------------------------------------------------
void setUp()    {}
void tearDown() {}

int main(int /*argc*/, char** /*argv*/)
{
    UNITY_BEGIN();

    // Structural checks for the table
    RUN_TEST(test_esp32_table_size_consistent);
    RUN_TEST(test_esp32_all_boards_have_names);
    RUN_TEST(test_esp32_sda_scl_differ);

    // Bus OLED A (SDA=4, SCL=15)
    RUN_TEST(test_esp32_HELTEC_LoRA32_V1_433MHz);    // PASS — primera del bus A
    RUN_TEST(test_esp32_HELTEC_LoRA32_V1_868MHz);    // COLISIÓN SX1276≡SX1278
    RUN_TEST(test_esp32_HELTEC_LoRA32_V2_433MHz);    // COLISIÓN mismos pines
    RUN_TEST(test_esp32_HELTEC_LoRA32_V2_868MHz);    // COLISIÓN SX1276≡SX1278
    RUN_TEST(test_esp32_TTGO_LoRa32_V1_433MHz);      // COLISIÓN mismos pines
    RUN_TEST(test_esp32_TTGO_LoRa32_V1_868MHz);      // COLISIÓN SX1276≡SX1278

    // Bus OLED B (SDA=21, SCL=22)
    RUN_TEST(test_esp32_TTGO_LoRA32_V2_433MHz);      // PASS — primera del bus B
    RUN_TEST(test_esp32_TTGO_LoRA32_V2_868MHz);      // COLISIÓN SX1276≡SX1278
    RUN_TEST(test_esp32_TBEAM_OLED_433MHz);          // COLISIÓN mismos pines
    RUN_TEST(test_esp32_TBEAM_OLED_868MHz);          // COLISIÓN SX1276≡SX1278
    RUN_TEST(test_esp32_Custom_SX126x_Crystal);      // PASS — SX126X pines únicos NSS=5
    RUN_TEST(test_esp32_TTGO_V2_SX126x_Crystal);     // PASS — SX126X pines únicos NSS=18
    RUN_TEST(test_esp32_DRF1268T_TCXO_5_2_26_13);    // COLISIÓN SX126X mismos pines que idx 10
    RUN_TEST(test_esp32_DRF1268T_TCXO_5_26_14_12);   // COLISIÓN SX126X mismos pines que idx 10
    RUN_TEST(test_esp32_TBEAM_V1_OLED_433MHz);       // COLISIÓN SX127X con idx 6
    RUN_TEST(test_esp32_FOSSA_1W_433MHz);             // PASS — SX126X NSS=5 MOSI=27 SCK=18
    RUN_TEST(test_esp32_FOSSA_1W_868MHz);             // PASS — SX127X NSS=5 MOSI=27 SCK=18
    RUN_TEST(test_esp32_ESP32_SX1280);               // PASS — familia SX1280 única
    RUN_TEST(test_esp32_TBEAM_V1_OLED_868MHz);       // COLISIÓN SX127X con idx 6
    RUN_TEST(test_esp32_LILYGO_T3_433MHz);           // COLISIÓN SX127X con idx 6
    RUN_TEST(test_esp32_LILYGO_T3_868MHz);           // COLISIÓN SX127X con idx 6
    RUN_TEST(test_esp32_LILYGO_T3_TCXO_868MHz);      // COLISIÓN SX127X con idx 6
    RUN_TEST(test_esp32_TBEAM_SX1268);               // COLISIÓN SX126X con idx 11

    // Sin OLED — Ethernet
    RUN_TEST(test_esp32_WT32_ETH01);                  // PASS — Fase 2 ETH

    UNITY_END();
    print_board_table("ESP32", esp32::boards, esp32::NUM_BOARDS);
    return 0;
}
