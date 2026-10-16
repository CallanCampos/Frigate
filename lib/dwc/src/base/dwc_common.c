#include "dwc/common.h"
#include "nitro/mi.h"

// DWC's common code (TWL-DWC 5.3's dwc_common.c, a guessed name; the ROM names none of it, and the file is ARM code in
// ARM9 main like the rest of the library's base): its Base64 coding, with '.' and '-' as the last two characters and
// '*' to pad. The download of files (overlay 189) and overlay 11 call it.

static const char *sBase64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-";

// Encodes the bytes in groups of three, each as four characters
int func_020580b8(const u8 *src, u32 srcLen, char *dst, u32 dstLen) {
    u32 count;
    const u8 *end;
    char *out;
    u32 outLen = srcLen / 3 * 4 + (srcLen % 3 != 0 ? 4 : 0);

    if (dst == NULL) {
        return outLen;
    }
    if (dstLen < outLen) {
        return -1;
    }

    end = src + srcLen;
    out = dst;
    while (src != end) {
        u8 group[3];
        int size = end - src;
        u32 bits = size * 8;

        count = bits / 6 + (bits % 6 != 0 ? 1 : 0);
        size = size < 3 ? size : 3;
        sys_memset(group, 0, 3);
        sys_memcpy(src, group, size);

        out[0] = sBase64Chars[group[0] >> 2];
        out[1] = count >= 2 ? sBase64Chars[((group[0] << 4) & 0x3f) | (group[1] >> 4)] : '*';
        out[2] = count >= 3 ? sBase64Chars[((group[1] << 2) & 0x3f) | (group[2] >> 6)] : '*';
        out[3] = count >= 4 ? sBase64Chars[group[2] & 0x3f] : '*';
        src += size;
        out += 4;
    }
    return out - dst;
}

// Decodes the text in groups of four characters, each as three bytes
int func_02058234(const char *src, int srcLen, u8 *dst, u32 dstLen) {
    int bits;
    int i;
    int outLen;
    u8 *out;
    int size;

    if (srcLen & 3) {
        return -1;
    }

    bits = 0;
    for (i = 0; i < srcLen; i++) {
        if (src[i] != '*') {
            bits += 6;
        }
    }
    outLen = bits / 8;
    if (dst == NULL) {
        return outLen;
    }
    if (dstLen < outLen) {
        return -1;
    }
    if (srcLen == 0) {
        return 0;
    }

    out = dst;
    do {
        char group[4];

        for (i = 0; i < 4; i++) {
            char c = src[i];

            if (c >= 'A' && c <= 'Z') {
                group[i] = c - 'A';
            } else if (c >= 'a' && c <= 'z') {
                group[i] = c - 'a' + 26;
            } else if (c >= '0' && c <= '9') {
                group[i] = c - '0' + 52;
            } else if (c == '.') {
                group[i] = 62;
            } else if (c == '-') {
                group[i] = 63;
            } else {
                group[i] = 0;
            }
        }
        src += 4;

        out[0] = (group[0] << 2) | (group[1] >> 4);
        size = out + 1 - dst;
        if (size >= outLen) {
            break;
        }
        out[1] = (group[1] << 4) | (group[2] >> 2);
        size = out + 2 - dst;
        if (size >= outLen) {
            break;
        }
        out[2] = (group[2] << 6) | group[3];
        out += 3;
        size = out - dst;
    } while (size < outLen);
    return size;
}
