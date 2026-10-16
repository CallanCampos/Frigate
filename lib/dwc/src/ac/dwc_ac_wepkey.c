// DWC's auto-connect, the keys of Nintendo's own access points (TWL-DWC 5.3, ac). The file name is a guess: its
// functions recognize the SSIDs of the NWCUSBAP and NDWCSHAP access points and derive their WEP keys from the SSIDs
// alone. An NWCUSBAP's SSID is "NWCUSBAP", 4 bytes and 20 bytes that its key is folded from; an NDWCSHAP's is the
// Base64 text of 24 bytes, masked and shuffled, that start with "NDWCSHAP" and hold the 10 bytes of its spot
// information, and its key is the MD5 of the 24.
//
// The functions are still named after their addresses; the comment above each has the name we would give it.

#include "dwc/ac.h"
#include "dwc/ext.h"
#include "nitro/math.h"
#include "nitro/mi.h"

// The masks that an NDWCSHAP's decoded SSID is XORed with, the first right after the decoding and the last at the end
static char *sSsidMaskFirst = "952uybjnpmu903bia@bk5m[-";
static char *sSsidMaskLast = "38g6zxjk20gvmv]6^=j&%vY1";

// Where each byte of an NWCUSBAP's key goes, and what each half byte turns into
static const u8 sKeyOrder[13] = { 5, 1, 12, 4, 2, 3, 10, 0, 11, 7, 9, 8, 6 };
static const u8 sNibbleMap[16] = { 10, 13, 14, 8, 9, 3, 6, 0, 12, 5, 2, 7, 11, 1, 15, 4 };

// The masks that an NWCUSBAP's key is XORed with, before it is shuffled and after
static char *sKeyMaskFirst = "gwi'6&fs=0Nf~";
static char *sKeyMaskLast = "%(egEr)ag(s&m";

static void DWCAc_DecodeNdwcShapSsid(const u8 *ssid, u8 *out);
static void DWCAc_HashNdwcShapKey(const u8 *in, u8 *key);
static void DWCAc_ScrambleNwcUsbApKey(const u8 *in, u8 *key);
static int DWCAc_Base64Value(u32 c);
static int DWCAc_DecodeBase64(const u8 *src, u8 *dest, u32 length, u32 capacity);

// Whether the SSID, decoded, starts with "NDWCSHAP"
BOOL DWCAc_IsNdwcShapSsid(const u8 *ssid) {
    u8 buf[0x18];

    DWCAc_DecodeNdwcShapSsid(ssid, buf);
    return memcmp(buf, "NDWCSHAP", 8) == 0;
}

// Makes the key of an NDWCSHAP from its SSID, the 13 bytes of the MD5 of the 24 that it holds
void DWCAc_MakeNdwcShapKey(const u8 *ssid, u8 *key) {
    u8 buf[0x18];

    DWCAc_DecodeNdwcShapSsid(ssid, buf);
    DWCAc_HashNdwcShapKey(buf, key);
}

// DWCAc_GetNdwcShapSpotInfo: decodes an NDWCSHAP's SSID into its 10 bytes of spot information; TRUE if it holds the 8
// bytes "NDWCSHAP" and the 10 are printable
BOOL DWCAc_GetNdwcShapSpotInfo(const u8 *ssid, u8 *spotInfo) {
    u8 buf[0x18];
    int i;

    DWCAc_DecodeNdwcShapSsid(ssid, buf);
    if (memcmp(buf, "NDWCSHAP", 8) == 0) {
        MI_CpuCopy8(buf + 8, spotInfo, 10);
        for (i = 0; i < 10; i++) {
            if (spotInfo[i] < 0x20 || spotInfo[i] > 0x7e) {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

// Whether the SSID starts with "NWCUSBAP"
BOOL DWCAc_IsNwcUsbApSsid(const u8 *ssid) {
    return memcmp(ssid, "NWCUSBAP", 8) == 0;
}

// Makes the key of an NWCUSBAP from its SSID, from the 20 bytes behind "NWCUSBAP" and 4 more
void DWCAc_MakeNwcUsbApKey(const u8 *ssid, u8 *key) {
    DWCAc_ScrambleNwcUsbApKey(ssid + 12, key);
}

// DWCAc_DecodeNdwcShapSsid: turns the 32 characters of an NDWCSHAP's SSID into the 24 bytes that they stand for:
// decodes the Base64, masks the bytes, puts them in their places (each cycle of the permutation in `order` is walked
// once, the places that are done being marked with 0xff) and masks them again
static void DWCAc_DecodeNdwcShapSsid(const u8 *ssid, u8 *out) {
    u8 order[0x18] = { 0x17, 0x14, 0x11, 0x0d, 0x0b, 0x06, 0x0f, 0x0e, 0x09, 0x15, 0x0c, 0x04,
                       0x02, 0x01, 0x12, 0x10, 0x05, 0x03, 0x13, 0x0a, 0x07, 0x08, 0x00, 0x16 };
    int i;

    DWCAc_DecodeBase64(ssid, out, 0x20, 0x18);
    for (i = 0; i < 0x18; i++) {
        out[i] = out[i] ^ sSsidMaskFirst[i];
    }
    for (i = 0; i < 0x18; i++) {
        // Two cursors that always hold the same index: the original marks the place it came from with one and puts the
        // byte it carries where the other says. A single cursor compiles 12 bytes shorter
        u8 slot = i;
        u8 from = i;
        u8 next;
        u8 old;
        u8 value = out[i];

        while (order[from] != 0xff) {
            next = order[from];
            old = out[next];
            out[order[slot]] = value;
            order[from] = 0xff;
            slot = next;
            from = next;
            value = old;
        }
    }
    for (i = 0; i < 0x18; i++) {
        out[i] = out[i] ^ sSsidMaskLast[i];
    }
}

// The key of an NDWCSHAP, bytes 3 to 15 of the MD5 of the 24 bytes of its decoded SSID
static void DWCAc_HashNdwcShapKey(const u8 *in, u8 *key) {
    MATHMD5Context context;
    u8 digest[20]; // 4 bytes more than an MD5 digest takes, as the frame has them

    MATH_MD5Init(&context);
    MATH_MD5Update(&context, in, 0x18);
    MATH_MD5GetHash(&context, digest);
    MI_CpuCopy8(digest + 3, key, 13);
}

// DWCAc_ScrambleNwcUsbApKey: the key of an NWCUSBAP, the 13 bytes of a 104-bit WEP key, from the 20 bytes of its SSID
// that follow "NWCUSBAP" and 4 more: they are folded to 13, masked, put in their places, masked again, their half
// bytes are mapped and the bytes are mixed
static void DWCAc_ScrambleNwcUsbApKey(const u8 *in, u8 *key) {
    u8 copy[13];
    int i;

    for (i = 0; i < 13; i++) {
        key[i] = in[i] ^ in[13 + i % 7];
    }
    for (i = 0; i < 7; i++) {
        key[3 + i] ^= in[13 + i];
    }
    for (i = 0; i < 13; i++) {
        key[i] = key[i] ^ sKeyMaskFirst[i];
    }
    MI_CpuCopy8(key, copy, 13);
    for (i = 0; i < 13; i++) {
        key[sKeyOrder[i]] = copy[i];
    }
    for (i = 0; i < 13; i++) {
        key[i] = key[i] ^ sKeyMaskLast[i];
    }
    for (i = 0; i < 13; i++) {
        key[i] = (sNibbleMap[(key[i] >> 4) & 0xf] << 4) | sNibbleMap[key[i] & 0xf];
    }
    for (i = 0; i < 3; i++) {
        key[i] ^= key[i + 6];
        key[i + 3] ^= key[i + 9];
        key[i + 6] ^= key[i + 3];
        key[i + 9] ^= key[i];
        key[12] ^= key[i];
    }
}

// DWCAc_Base64Value: the value of a Base64 character ('=' is 0), or -1 if it is none. The offset into a range is kept
// in a local, as MWCC folds `c - 'a' + 26` into one subtract where the original has two steps
static int DWCAc_Base64Value(u32 c) {
    int v;

    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        v = c - 'a';
        return v + 26;
    }
    if (c >= '0' && c <= '9') {
        v = c - '0';
        return v + 52;
    }
    if (c == '+') {
        return 62;
    }
    if (c == '/') {
        return 63;
    }
    if (c == '=') {
        return 0;
    }
    return -1;
}

// DWCAc_DecodeBase64: decodes Base64 text of `length` characters into `capacity` bytes at most, a partial group of
// characters last. Returns the number of bytes it makes, or -1 if the destination is too small
static int DWCAc_DecodeBase64(const u8 *src, u8 *dest, u32 length, u32 capacity) {
    int size = length * 3 / 4;
    u32 bits;
    int rest;
    int whole;
    int i;
    int j;
    int k;
    union {
        u32 word;
        u8 bytes[4];
    } group;

    if (capacity >= size) {
        rest = length & 3;
        whole = length - rest;
    } else {
        return -1;
    }
    for (i = 0; i < whole; i += 4) {
        bits = 0;
        for (j = 0; j < 4; j++) {
            bits |= DWCAc_Base64Value(src[i + j]) << (3 - j) * 6;
        }
        group.word = bits;
        for (k = 0; k < 3; k++) {
            dest[i / 4 * 3 + k] = group.bytes[2 - k];
        }
    }
    if (rest != 0) {
        bits = 0;
        group.word = 0;
        for (j = 0; j < rest; j++) {
            bits |= DWCAc_Base64Value(src[whole + j]) << (3 - j) * 6;
            group.word |= bits;
        }
        for (k = 0; k < rest; k++) {
            dest[whole * 3 / 4 + k] = group.bytes[2 - k];
        }
    }
    return size;
}
