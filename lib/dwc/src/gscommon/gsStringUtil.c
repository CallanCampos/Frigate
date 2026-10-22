#include "dwc/gsStringUtil.h"
#include "dwc/ext.h"

// GameSpy's texts (gsStringUtil.c). The ROM names neither the file nor its functions: the file and the conversions
// are the later SDK's (UTF8ToUCS2StringAllocLen after them), and gsiSafeStrcpyA is ours

static int _ReadUCS2CharFromUTF8String(const char *utf8, int length, gsi_u16 *ucs2);
static int _UTF8ToUCS2ConversionLengthOnly(const char *utf8, int length);

// Reads one character of a UTF-8 text of a length: returns how many bytes it took, or 0 when the text ends in it; a
// byte that starts no character becomes a question mark
static int _ReadUCS2CharFromUTF8String(const char *utf8, int length, gsi_u16 *ucs2) {
    if (length == 0) {
        *ucs2 = '?';
        return 0;
    }
    if ((gsi_u8)utf8[0] <= 0x7f) {
        *ucs2 = utf8[0];
        return 1;
    }
    if (((gsi_u8)utf8[0] & 0xe0) == 0xc0) {
        if (length < 2) {
            *ucs2 = '?';
            return 0;
        }
        if (((gsi_u8)utf8[1] & 0xc0) == 0x80) {
            *ucs2 = ((utf8[0] & 0x1f) << 6) + (utf8[1] & 0x3f);
            return 2;
        }
    } else if (((gsi_u8)utf8[0] & 0xf0) == 0xe0) {
        if (length < 3) {
            *ucs2 = '?';
            return 0;
        }
        if (((gsi_u8)utf8[1] & 0xc0) == 0x80 && ((gsi_u8)utf8[2] & 0xc0) == 0x80) {
            *ucs2 = ((utf8[0] & 0xf) << 12) + ((utf8[1] & 0x3f) << 6) + (utf8[2] & 0x3f);
            return 3;
        }
    }
    *ucs2 = '?';
    return 1;
}

int _UCS2CharToUTF8String(gsi_u16 c, char *utf8) {
    if (c <= 0x7f) {
        utf8[0] = c;
        return 1;
    }
    if (c <= 0x7ff) {
        utf8[0] = 0xc0 | c >> 6;
        utf8[1] = 0x80 | (c & 0x3f);
        return 2;
    }
    utf8[0] = 0xe0 | c >> 12;
    utf8[1] = 0x80 | (c >> 6 & 0x3f);
    utf8[2] = 0x80 | (c & 0x3f);
    return 3;
}

// Counts the characters of a UTF-8 text, as far as its end or its length
static int _UTF8ToUCS2ConversionLengthOnly(const char *utf8, int length) {
    int count = 0;

    if (utf8 == NULL) {
        return 0;
    }
    while (*utf8 != 0 && length > 0) {
        gsi_u8 c = *utf8;

        if ((c & 0xe0) == 0xc0 && length >= 2 && ((gsi_u8)utf8[1] & 0xc0) == 0x80) {
            utf8 += 2;
            length -= 2;
        } else if ((c & 0xf0) == 0xe0 && length >= 3 && ((gsi_u8)utf8[1] & 0xc0) == 0x80 &&
                   ((gsi_u8)utf8[2] & 0xc0) == 0x80) {
            utf8 += 3;
            length -= 3;
        } else {
            utf8++;
            length--;
        }
        count++;
    }
    return count;
}

gsi_u16 *UTF8ToUCS2StringAlloc(const char *utf8) {
    if (utf8 == NULL) {
        return NULL;
    }
    return UTF8ToUCS2StringAllocLen(utf8, strlen(utf8));
}

gsi_u16 *UTF8ToUCS2StringAllocLen(const char *utf8, int length) {
    int count;
    gsi_u16 *ucs2;

    if (utf8 == NULL) {
        return NULL;
    }
    count = _UTF8ToUCS2ConversionLengthOnly(utf8, length);
    ucs2 = gsimalloc((count + 1) * sizeof(gsi_u16));
    UTF8ToUCS2StringLen(utf8, length, ucs2, count);
    return ucs2;
}

int UTF8ToUCS2StringLen(const char *utf8, int length, gsi_u16 *ucs2, int count) {
    int converted = 0;
    int used = 0;
    gsi_u16 *write = ucs2;
    int size;

    if (utf8 == NULL) {
        *ucs2 = 0;
        return 1;
    }
    while (*(gsi_u8 *)utf8 != 0 && count + 1 > converted && length > used) {
        size = _ReadUCS2CharFromUTF8String(utf8, length - used, write);
        if (size == 0) {
            *ucs2 = 0;
            return 0;
        }
        used += size;
        utf8 += size;
        converted++;
        write++;
    }
    *write = 0;
    return converted + 1;
}

gsi_u32 gsiSafeStrcpyA(char *dest, const char *src, gsi_u32 size) {
    gsi_u32 length = strlen(src);

    if (length > size - 1) {
        length = size - 1;
    }
    memcpy(dest, src, length);
    dest[length] = '\0';
    return length;
}
