/* General purpose Reed-Solomon decoder for 8-bit symbols
 * Copyright 2003 Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */

#include <string.h>
#include "fixed.h"

int decode_rs_8(data_t *data, int *eras_pos, int no_eras, int pad)
{
    int retval;

    if (pad < 0 || pad > 222 || no_eras < 0 || no_eras > NROOTS) {
        return -1;
    }

#include "decode_rs.h"

    return retval;
}
