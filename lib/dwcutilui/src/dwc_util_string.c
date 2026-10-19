#include "dwc_util_string.h"

int func_ov182_021bcef8(const u8 *str, int max) {
    int i;

    for (i = 0; i < max; i++) {
        if (str[i] == 0) {
            break;
        }
    }
    return i;
}

int func_ov182_021bcf10(u16 *dst, const u8 *src, int size) {
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
