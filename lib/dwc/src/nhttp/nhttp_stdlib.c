#include "nhttp_internal.h"

#include "nitro/mi.h"

// NHTTP_stdlib: the library's C library, over NitroSDK's, and its text conversions

static char sBase64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void NHTTPi_memcpy(void *dest, const void *src, u32 size) {
    sys_memcpy(src, dest, size);
}

int NHTTPi_strlen(const char *s) {
    return NNS_STD_StrLen(s);
}

int NHTTPi_strnlen(const char *s, int max) {
    return func_0207f900(s, max);
}

int NHTTPi_strcmp(const char *a, const char *b) {
    return STD_CompareString(a, b);
}

void NHTTPi_memclr(void *dest, u32 size) {
    sys_memset(dest, 0, size);
}

// Compares two strings up to a size without caring for the case; 0 if they are equal, else the size that was left
int NHTTPi_strnicmp(const char *a, const char *b, int size) {
    int cb;
    int ca;

    while (size > 0) {
        ca = *a++;
        cb = *b++;
        if ((ca == 0 || cb == 0) && ca == 0 && cb == 0) {
            size = 0;
            break;
        }
        if (NHTTPi_isUpper(cb)) {
            cb += 'a' - 'A';
        }
        if (NHTTPi_isUpper(ca)) {
            ca += 'a' - 'A';
        }
        if (ca != cb) {
            break;
        }
        size--;
    }
    return size;
}

// The size of a string once it is URL encoded: a letter, a digit or a space is one byte and the rest three
int NHTTPi_getUrlEncodedSize(const char *s) {
    int size = 0;
    char c = *s++;

    while (c != 0) {
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ' ') {
            size += 1;
        } else {
            size += 3;
        }
        c = *s++;
    }
    return size;
}

int NHTTPi_getUrlEncodedSize2(const char *s, int length) {
    int i = 0;
    int size = 0;
    char c = *s++;

    if (length > 0) {
        for (; i < length; i++) {
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ' ') {
                size += 1;
            } else {
                size += 3;
            }
            c = *s++;
        }
    }
    return size;
}

// Writes a character as it goes in a URL: a space is a plus, and what is not a letter or a digit is %XX. Returns the
// number of bytes written
int NHTTPi_encodeUrlChar(char *dest, char c) {
    int high;
    int low;

    if (c == ' ') {
        dest[0] = '+';
        return 1;
    }
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        dest[0] = c;
        return 1;
    }
    high = (c >> 4) & 0xf;
    low = c & 0xf;
    dest[0] = '%';
    if (high < 10) {
        high += '0';
    } else {
        high += 'A' - 10;
    }
    dest[1] = high;
    if (low < 10) {
        low += '0';
    } else {
        low += 'A' - 10;
    }
    dest[2] = low;
    return 3;
}

// Reads a number in hexadecimal, from at most 8 digits, with spaces before it and a space or the end after it, or -1
int NHTTPi_strToHex(const char *s, int size) {
    int value;
    int started;
    int i;

    if (size > 8) {
        return -1;
    }
    if (size == 8 && s[0] > '7') {
        return -1;
    }
    value = 0;
    started = 0;
    for (i = 0; i < size; i++) {
        char c = NHTTPi_toLower(s[i]);

        if (c >= '0' && c <= '9') {
            value = value * 16 + c - '0';
            started = 1;
        } else if (c >= 'a' && c <= 'f') {
            value = value * 16 + c - 'a' + 10;
            started = 1;
        } else if (started && (c == ' ' || c == 0)) {
            break;
        } else if (started || c != ' ') {
            return -1;
        }
    }
    return value;
}

// Reads a number in decimal, from at most 10 digits, with spaces before it and a space or the end after it, or -1
int NHTTPi_strToInt(const char *s, int size) {
    int value;
    int started;
    int i;

    if (size > 10) {
        return -1;
    }
    value = 0;
    started = 0;
    for (i = 0; i < size; i++) {
        char c = s[i];

        if (started && (c == ' ' || c == 0)) {
            break;
        }
        if (!started && c == ' ') {
            continue;
        }
        if (c < '0' || c > '9') {
            return -1;
        } else {
            int before = value;

            value = c + value * 10 - '0';
            started = 1;
            if (before > value) {
                return -1;
            }
        }
    }
    return value;
}

// Writes a number in decimal, and returns the number of digits
int NHTTPi_intToStr(char *dest, u32 value) {
    const u32 powers[9] = { 1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10 };
    int length = 0;
    int started = 0;
    int i;

    for (i = 0; i < 9; i++) {
        u32 power = powers[i];

        if (value >= power) {
            u32 digit = value / power;

            value -= digit * power;
            started = 1;
            dest[length] = digit + '0';
            length++;
        } else if (started) {
            dest[length] = '0';
            length++;
        }
    }
    dest[length] = value + '0';
    return length + 1;
}

// Compares a token, up to a space or the end of the first string, without caring for the case; 0 if they are equal
int NHTTPi_compareToken(const char *a, const char *b) {
    int i = 0;
    int j = 0;

    for (;;) {
        char cb = b[j];
        char ca = a[i];
        char lower;

        if (NHTTPi_isUpper(cb)) {
            cb += 'a' - 'A';
        }
        lower = ca;
        if (NHTTPi_isUpper(ca)) {
            lower = ca + ('a' - 'A');
        }
        if (lower != cb) {
            break;
        }
        if (ca == 0 || ca == ' ') {
            return 0;
        }
        a++;
        b++;
    }
    return -1;
}

// Reads the digits of a string, leaving out what is not a digit, or -1 if there are none or more than 9
int NHTTPi_strtonum(const char *s, int size) {
    int value = 0;
    int count = 0;

    while (size-- != 0) {
        char c = *s;

        if (c != ' ' && c >= '0' && c <= '9') {
            value = c + value * 10 - '0';
            count++;
            if (count > 9) {
                return -1;
            }
        }
        s++;
    }
    if (count == 0) {
        value = -1;
    }
    return value;
}

// Looks for a string in another one: 0 if it is there, -1 if not
int NHTTPi_memfind(const char *haystack, int haystackSize, const char *needle, int needleSize) {
    int i;
    int j;
    const char *p;

    if (haystackSize < needleSize) {
        return -1;
    }
    for (i = 0; i < haystackSize - needleSize + 1; i++) {
        if (haystack[i] == needle[0]) {
            p = haystack + i;
            for (j = 1; j < needleSize; j++) {
                if (p[j] != needle[j]) {
                    break;
                }
            }
            if (j == needleSize) {
                return 0;
            }
        }
    }
    return -1;
}

// Writes a string in base64, and returns the size of what it wrote
int NHTTPi_Base64Encode(char *dest, const char *src) {
    const char *in = src;
    char *out = dest;
    char *table = sBase64Alphabet;
    int length = NHTTPi_strlen(in);
    int i;

    for (i = 0; i < length; i += 3) {
        *out++ = table[in[0] >> 2];
        *out++ = table[((in[0] & 3) << 4) + (in[1] >> 4)];
        *out++ = table[((in[1] & 0xf) << 2) + (in[2] >> 6)];
        *out++ = table[in[2] & 0x3f];
        in += 3;
    }
    if (i == length + 1) {
        out[-1] = '=';
    } else if (i == length + 2) {
        out[-1] = out[-2] = '=';
    }
    *out = 0;
    return NHTTPi_strlen(dest);
}
