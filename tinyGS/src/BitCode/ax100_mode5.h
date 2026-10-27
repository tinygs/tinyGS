#pragma once
/*
 * AX.100 Mode 5 (Gomspace NanoCom AX100) frame decoder.
 * Frame format: 3-byte Golay-encoded header | RS(255,223) block
 *
 * Based on gr-satellites by Daniel Estevez EA4GPZ.
 */

#include <stdint.h>

/* AX.100 Mode 5 constants */
#define AX100_M5_HEADER_LEN  3    /* bytes: Golay-encoded length field */
#define AX100_M5_RS_LEN      255  /* RS block size (bytes) */
#define AX100_M5_MAX_OUT     223  /* max payload = RS_LEN - NROOTS */

typedef struct {
    int golay_errors; /**< bit errors corrected by Golay decoder */
    int rs_errors;    /**< byte errors corrected by RS decoder   */
    int frame_len;    /**< RS frame length extracted from header */
} ax100_mode5_info_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Decode one AX.100 Mode 5 frame.
 *
 * @param in      raw received bytes (at least AX100_M5_HEADER_LEN + 1)
 * @param in_len  number of bytes in @p in
 * @param out     output buffer, must hold at least AX100_M5_MAX_OUT bytes
 * @param out_len receives payload byte count on success
 * @param info    optional; receives diagnostic info (may be NULL)
 * @return 0 on success, negative on error:
 *   -1 NULL pointer
 *   -2 frame too short
 *   -3 Golay uncorrectable
 *   -4 frame_len out of range
 *   -5 RS uncorrectable
 *   -6 more than 3 trailing RS bytes missing from the input
 */
int ax100_mode5_decode(const uint8_t *in, int in_len,
                       uint8_t *out, int *out_len,
                       ax100_mode5_info_t *info);

#ifdef __cplusplus
}
#endif
