/*
 * AX.100 Mode 5 decoder
 *
 * Frame layout (after sync word):
 *   [0..2] : 24-bit Golay codeword encoding frame_len
 *   [3..]  : CCSDS-randomised shortened RS(255,223) codeword
 *
 * Decode steps:
 *   1. Golay-decode bytes 0-2 to recover frame_len
 *   2. CCSDS de-randomise received RS bytes
 *   3. RS-decode, optionally recovering up to 3 missing
 *      trailing bytes as erasures (SX1262 255-byte limit)
 *   4. Extract payload (frame_len - 32 bytes)
 *
 * Based on gr-satellites by Daniel Estevez EA4GPZ.
 * Modified for tinyGS by Stefan/OE6ISP.
 */

#include "ax100_mode5.h"
#include "fixed.h"
#include "randomizer.h"
#include "golay24.h"

#include <stdint.h>
#include <string.h>

/*
 * In packet mode a 255-byte RS block plus the 3 Golay bytes is 258 bytes,
 * but the SX126x delivers at most 255, so up to 3 trailing bytes can be
 * missing. If all bytes are received (e.g. direct mode) nothing is erased.
 */
#define AX100_M5_MAX_ERASURES 3

int ax100_mode5_decode(const uint8_t *in, int in_len,
                       uint8_t *out, int *out_len,
                       ax100_mode5_info_t *info)
{
    uint8_t data[AX100_M5_HEADER_LEN + AX100_M5_RS_LEN];
    uint8_t scratch[AX100_M5_RS_LEN];
    char    seq[AX100_M5_RS_LEN];
    // decode_rs_8 writes back every corrected position (erasures + errors),
    // so this needs NROOTS entries, not just AX100_M5_MAX_ERASURES
    int     eras_pos[NROOTS];

    if (!in || !out || !out_len) return -1;

    *out_len = 0;

    if (in_len < AX100_M5_HEADER_LEN + 1) return -2;

    memset(data, 0, sizeof(data));
    memset(scratch, 0, sizeof(scratch));
    memset(eras_pos, 0, sizeof(eras_pos));

    if (in_len > (int)sizeof(data))
        in_len = (int)sizeof(data);

    memcpy(data, in, in_len);

    /* Step 1: Golay-decode the 24-bit header */
    uint32_t length_field =
        ((uint32_t)data[0] << 16) |
        ((uint32_t)data[1] <<  8) |
        ((uint32_t)data[2]);

    int golay_res = decode_golay24(&length_field);
    if (golay_res < 0)
        return -3;

    int frame_len = (int)(length_field & 0xff);

    if (frame_len <= 32 || frame_len > AX100_M5_RS_LEN)
        return -4;

    /* Bytes of the RS block actually received; the rest become erasures */
    int available = in_len - AX100_M5_HEADER_LEN;
    int missing   = frame_len - available;

    if (missing < 0)
        missing = 0;

    if (missing > AX100_M5_MAX_ERASURES)
        return -6;

    int copy_len = (available < frame_len) ? available : frame_len;

    /* Step 2: CCSDS de-randomise only the bytes actually received */
    ccsds_generate_sequence(seq, AX100_M5_RS_LEN);
    ccsds_xor_sequence(data + AX100_M5_HEADER_LEN, seq, copy_len);

    /* Step 3: RS decode, missing trailing bytes stay zero and are erasures */
    memcpy(scratch, data + AX100_M5_HEADER_LEN, copy_len);

    for (int i = 0; i < missing; i++)
        eras_pos[i] = copy_len + i;

    int pad    = AX100_M5_RS_LEN - frame_len;
    int rs_res = decode_rs_8(scratch, missing ? eras_pos : NULL, missing, pad);

    if (rs_res < 0)
        return -5;

    /* Step 4: extract payload */
    int payload_len = frame_len - 32;   /* 32 RS parity bytes */
    memcpy(out, scratch, payload_len);
    *out_len = payload_len;

    if (info) {
        info->golay_errors = golay_res;
        info->rs_errors    = rs_res;
        info->frame_len    = frame_len;
    }

    return 0;
}
