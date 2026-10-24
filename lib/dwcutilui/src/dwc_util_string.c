#include "dwc_util_string.h"

int DWCi_STRlNLen(const u8 *str, int max) {
    int i;

    for (i = 0; i < max; i++) {
        if (str[i] == 0) {
            break;
        }
    }
    return i;
}

int DWCi_STRlConvertStringAsciiToUnicode(u16 *dst, const u8 *src, int size) {
    int i;

    for (i = 0; i < size - 1; i++) {
        u8 c = src[i];

        if (c == 0) {
            break;
        }
        dst[i] = c;
    }
    dst[i] = 0;
    return i;
}
