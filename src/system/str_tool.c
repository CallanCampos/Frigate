#include "types.h"
#include "gfl/str.h"
#include "system/str_tool.h"

// Numbers as strings, copies and comparisons of raw strings, accents and case, and compressed strings. The file's
// name is a guess: the ROM has no string for it

// The digits and the character for a digit out of range, in full width and in ASCII, for decimal and hexadecimal
// numbers, and the powers of 10 and 16 the digits are divided out by. Only GFL_WordSetFormatNumber's tables are used
// in the ROM; the others were left by the formatters below, which nothing calls
const u16 digitsMono[] = { 0xff10, 0xff11, 0xff12, 0xff13, 0xff14, 0xff15, 0xff16, 0xff17, 0xff18, 0xff19, 0xff1f };
const u16 digitsAscii[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '?' };
const u16 mono_0_9_Q[] = { 0xff10, 0xff11, 0xff12, 0xff13, 0xff14, 0xff15, 0xff16, 0xff17, 0xff18, 0xff19, 0xff1f };
const u16 ascii_0_9_Q[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '?' };
const u32 hexExponents[] = { 0x1, 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x10000000 };
const u16 hexDigitsMono[] = { 0xff10, 0xff11, 0xff12, 0xff13, 0xff14, 0xff15, 0xff16, 0xff17, 0xff18,
                              0xff19, 0xff21, 0xff22, 0xff23, 0xff24, 0xff25, 0xff26, 0xff1f };
const u16 hexDigitsAscii[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', '?' };
const u32 decimalExponents[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };
// Indexed by the count of digits: the value of the first digit, which for 0 digits is the largest a u32 has
const u32 tenExponents[] = {
    1000000000, 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000,
};

// The letter without its accent for each of 0xc0 to 0xff, or 0 to keep the character
// clang-format off
static const s8 Aaaaaa[] = {
    'A', 'A', 'A', 'A', 'A', 'A', 0,   'C', 'E', 'E', 'E', 'E', 'I', 'I', 'I', 'I',
    'D', 'N', 'O', 'O', 'O', 'O', 'O', 0,   'O', 'U', 'U', 'U', 'U', 'Y', 'Y', 's',
    'a', 'a', 'a', 'a', 'a', 'a', 0,   'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
    'd', 'n', 'o', 'o', 'o', 'o', 'o', 0,   'o', 'u', 'u', 'u', 'u', 'y', 'y', 'y',
};

// clang-format on

// The characters of codes 0x100 on in compressed strings
static const u16 SPECIAL_COMPRESSED_CHAR_TABLE[] = {
    0x0152, 0x0153, 0x015e, 0x015f, 0x2018, 0x201c, 0x201d, 0x201e, 0x2026, 0x2460, 0x2461, 0x2462, 0x2463,
    0x2464, 0x2465, 0x2466, 0x2467, 0x2468, 0x2469, 0x246a, 0x246b, 0x246c, 0x246d, 0x246e, 0x246f, 0x2470,
    0x2471, 0x2472, 0x2473, 0x2474, 0x2475, 0x2476, 0x2477, 0x2478, 0x2479, 0x247a, 0x247b, 0x247c, 0x247d,
    0x247e, 0x247f, 0x2480, 0x2481, 0x2482, 0x2483, 0x2484, 0x2485, 0x2486, 0x2487, 0xff65,
};

// Not in the ROM: MWCC leaves out static functions that nothing calls, but the global tables they read stay. These are
// reconstructed from those tables, as GFL_WordSetFormatNumber for unsigned decimal and for hexadecimal numbers
static void FormatDecimal(StrBuf *strbuf, u32 number, u32 digits, u32 pad, BOOL ascii) {
    const u16 *chars = ascii == FALSE ? digitsMono : digitsAscii;
    u32 div;

    GFL_StrBufClear(strbuf);
    for (div = decimalExponents[digits - 1]; div != 0; div /= 10) {
        u16 digit = number / div;

        number -= div * digit;
        if (digit >= 10) {
            digit = 10;
        }
        if (pad == NUM_PAD_ZERO || digit != 0 || div == 1) {
            pad = NUM_PAD_ZERO;
            GFL_StrBufAppend(strbuf, chars[digit]);
        } else if (pad == NUM_PAD_SPACE) {
            GFL_StrBufAppend(strbuf, ascii == FALSE ? 0x3000 : ' ');
        }
    }
}

static void FormatHex(StrBuf *strbuf, u32 number, u32 digits, u32 pad, BOOL ascii) {
    const u16 *chars = ascii == FALSE ? hexDigitsMono : hexDigitsAscii;
    u32 div;

    GFL_StrBufClear(strbuf);
    for (div = hexExponents[digits - 1]; div != 0; div /= 16) {
        u16 digit = number / div;

        number -= div * digit;
        if (digit >= 16) {
            digit = 16;
        }
        if (pad == NUM_PAD_ZERO || digit != 0 || div == 1) {
            pad = NUM_PAD_ZERO;
            GFL_StrBufAppend(strbuf, chars[digit]);
        } else if (pad == NUM_PAD_SPACE) {
            GFL_StrBufAppend(strbuf, ascii == FALSE ? 0x3000 : ' ');
        }
    }
}

#define COMPRESSED_STRING_MARK 0xf100
#define COMPRESSED_CHAR_BITS 9
#define COMPRESSED_CHAR_MASK ((1 << COMPRESSED_CHAR_BITS) - 1)
#define COMPRESSED_EOM COMPRESSED_CHAR_MASK
#define COMPRESSED_SPECIAL_START 0x100
#define COMPRESSED_SPECIAL_END (COMPRESSED_SPECIAL_START + NELEMS(SPECIAL_COMPRESSED_CHAR_TABLE))

void GFL_WordSetFormatNumber(StrBuf *strbuf, s32 number, u32 digits, u32 pad, BOOL ascii) {
    const u16 *chars = ascii == FALSE ? mono_0_9_Q : ascii_0_9_Q;
    u32 div;
    u16 negative;

    GFL_StrBufClear(strbuf);
    negative = number < 0 ? TRUE : FALSE;
    if (negative) {
        number *= -1;
        GFL_StrBufAppend(strbuf, ascii == FALSE ? 0x30fc : '-');
    }
    for (div = tenExponents[digits]; div != 0; div /= 10) {
        u16 digit = (u32)number / div;

        number -= div * digit;
        if (digit >= 10) {
            digit = 10;
        }
        if (pad == NUM_PAD_ZERO) {
            GFL_StrBufAppend(strbuf, chars[digit]);
        } else if (digit != 0 || div == 1) {
            // Digits after the first one that is not zero are all printed
            pad = NUM_PAD_ZERO;
            GFL_StrBufAppend(strbuf, chars[digit]);
        } else if (pad == NUM_PAD_SPACE) {
            GFL_StrBufAppend(strbuf, ascii == FALSE ? 0x3000 : ' ');
        }
    }
}

// Copies at most n characters, the last of which is always the terminator
void wcharsncpy(const u16 *src, u16 *dest, u32 n) {
    u16 eom = GFL_StrBufGetTerminator();
    u32 i;

    for (i = 0; i < n; i++) {
        dest[i] = src[i];
        if (eom == src[i]) {
            break;
        }
    }
    *(dest + n - 1) = eom;
}

BOOL wcharscmp(const u16 *a, const u16 *b) {
    u16 eom = GFL_StrBufGetTerminator();

    while (*a != eom) {
        if (*a != *b) {
            return FALSE;
        }
        a++;
        b++;
    }
    if (*b != eom) {
        return FALSE;
    }
    return TRUE;
}

u16 GFL_StrCharToUpperCase(u16 c) {
    if (c >= 'a' && c <= 'z') {
        c -= 'a' - 'A';
    }
    return c;
}

u16 GFL_StrCharRemoveAccents(u16 c) {
    if (c >= 0xc0 && c <= 0xff) {
        u32 result = c;
        s8 base = Aaaaaa[c - 0xc0];

        if (base != 0) {
            result = (u16)base;
        }
        c = result;
    }
    return c;
}

BOOL GFL_StrBufCmpIgnoreAccents(const StrBuf *a, const StrBuf *b) {
    u16 eom = GFL_StrBufGetTerminator();
    const u16 *strA = GFL_StrBufGetStringPtr(a);
    const u16 *strB = GFL_StrBufGetStringPtr(b);

    while (*strA != eom) {
        if (GFL_StrCharToUpperCase(GFL_StrCharRemoveAccents(*strA)) !=
            GFL_StrCharToUpperCase(GFL_StrCharRemoveAccents(*strB))) {
            return FALSE;
        }
        strA++;
        strB++;
    }
    if (*strB != eom) {
        return FALSE;
    }
    return TRUE;
}

BOOL GFL_StrBufIsCompressed(const StrBuf *strbuf) {
    if (GFL_StrBufGetCharCount(strbuf) != 0 && *GFL_StrBufGetStringPtr(strbuf) == COMPRESSED_STRING_MARK) {
        return TRUE;
    }
    return FALSE;
}

// A compressed string is its mark, then 9-bit characters, from the bottom bits of each character on
void GFL_StrBufUncompress(StrBuf *dest, const StrBuf *src) {
    if (GFL_StrBufIsCompressed(src)) {
        const u16 *str = GFL_StrBufGetStringPtr(src) + 1;
        int shift = 0;

        for (;;) {
            u16 c = (*str >> shift) & COMPRESSED_CHAR_MASK;

            shift += COMPRESSED_CHAR_BITS;
            if (shift >= 16) {
                str++;
                shift -= 16;
                if (shift != 0) {
                    c |= (u16)((*str << (COMPRESSED_CHAR_BITS - shift)) & COMPRESSED_CHAR_MASK);
                }
            }
            if (c == COMPRESSED_EOM) {
                break;
            }
            if (c >= COMPRESSED_SPECIAL_START && c < COMPRESSED_SPECIAL_END) {
                c = SPECIAL_COMPRESSED_CHAR_TABLE[c - COMPRESSED_SPECIAL_START];
            } else if (c >= COMPRESSED_SPECIAL_END) {
                c = '?';
            }
            GFL_StrBufAppend(dest, c);
        }
    } else {
        GFL_StrBufConcat(dest, src);
    }
}
