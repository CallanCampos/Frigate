#ifndef POKEBW2_GFL_TEXTPRINT_H
#define POKEBW2_GFL_TEXTPRINT_H

#include "types.h"
#include "struct_decls.h"

// Debug text in the system font (textprint.c), from ASCII and Shift JIS strings. Strings are converted to glyph
// indices, which are glyphs of the font offset by TEXTPRINT_GLYPH_BASE, after the control indices

enum {
    TEXTPRINT_NEWLINE = 1,
    TEXTPRINT_TAB,
    TEXTPRINT_HALF_TAB,
    TEXTPRINT_GLYPH_BASE,
};

// An index for characters that are left out
#define TEXTPRINT_SKIP 0xfffe
// The index for characters the font does not have
#define TEXTPRINT_UNKNOWN 0x101

// Where to draw text. Drawing moves x and y to the end of the text
struct TextPrintParam {
    GFLBitmap *dest;
    u16 x;
    u16 y;
    u8 letterSpacing;
    u8 lineSpacing;
    u8 fgColor : 4;
    u8 bgColor : 4;
    u8 pad;
};

// Loads the font, from a file or the built-in one when path is NULL
void GFL_TextPrintInit(const char *path);
// Draws indices up to GFL_StrBufGetTerminator()
void GFL_TextPrintDrawIndices(const u16 *indices, TextPrintParam *param);
void GFL_TextPrintDrawString(const char *str, TextPrintParam *param);
// Converts a string to at most max - 1 indices and a terminator
void GFL_TextPrintStringToIndices(const char *str, u16 *dest, int max);
u16 GFL_TextPrintASCIIToIndex(u16 c);
u16 GFL_TextPrintSJISToIndex(u16 c);

#endif // POKEBW2_GFL_TEXTPRINT_H
