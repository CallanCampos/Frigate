// DWC's common helpers of the friends' match (TWL-DWC's dwc_common.c, overlay 11, Thumb): the key-value texts of the
// friends' statuses, the random numbers of the match and the length of a text of u16 characters. Its .data holds the
// key-value format and its .bss DWCi_GetMathRand32's generator.
// The ROM names none of DWC's files. Black 1's DWC, whose functions aletheia matched to TWL-DWC's library objects
// (layout/symbols_sdk.txt), has overlay 11's run of them in libdwcnonport's dwc_nonport.o, libdwcnas's dwc_nasfunc.o
// and dwc_nastime.o, and libdwcmatch's dwc_common.o, dwc_friend.o and dwc_login.o, in this order and with the same
// functions, and each file's .data and .bss follow in the same order.

#include <string.h>
// Before dwc/ext.h, as in ntrtwl's NitroDWC: only with <string.h>'s declarations does DWC_GetCommonValueString add
// the key's length to the pointer in the ROM's order
#include "dwc/common.h"
#include "dwc/ext.h"
#include "nitro/math.h"
#include "nitro/os.h"

int DWC_SetCommonKeyValueString(const char *key, const char *value, char *out, char sep) {
    OS_SNPrintf(out, 0x1000, "%c%s%c%s", sep, key, sep, value);
    return strlen(out);
}

int DWC_AddCommonKeyValueString(const char *key, const char *value, char *out, char sep) {
    DWC_SetCommonKeyValueString(key, value, strchr(out, 0), sep);
    return strlen(out);
}

int DWC_GetCommonValueString(const char *key, char *dest, const char *text, char sep) {
    char *pair = strchr(text, sep);

    if (pair == NULL) {
        return -1;
    }
    for (;;) {
        char *next;

        if (strncmp(pair + 1, key, strlen(key)) == 0 && sep == pair[strlen(key) + 1]) {
            break;
        }
        next = strchr(pair + 1, sep);
        if (next == NULL) {
            return -1;
        }
        pair = strchr(next + 1, sep);
        if (pair == NULL) {
            return -1;
        }
    }
    pair = strchr(pair + 1, sep);
    if (pair == NULL) {
        return -1;
    }
    {
        char *end = strchr(pair + 1, sep);
        int length;

        if (end != NULL) {
            length = end - (pair + 1);
        } else {
            length = strlen(pair + 1);
        }
        if (dest == NULL) {
            return length;
        }
        strncpy(dest, pair + 1, length);
        dest[length] = 0;
        return length;
    }
}

u32 DWCi_GetMathRand32(u32 max) {
    static MATHRandContext32 sRandom;

    if (sRandom.x == 0 && sRandom.mul == 0 && sRandom.add == 0) {
        u64 seed;

        OS_GetMacAddress((u8 *)&seed);
        seed = (seed >> 24 & 0xffffff) | OS_GetTick() << 24;
        MATH_InitRand32(&sRandom, seed);
    }
    return MATH_Rand32(&sRandom, max);
}

u32 DWCi_WStrLen(const u16 *str) {
    u32 length = 0;

    while (str[length] != 0) {
        length++;
    }
    return length;
}
