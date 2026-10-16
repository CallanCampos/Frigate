#include "types.h"
#include "gfl/heap.h"
#include "gfl/systemfont.h"
#include "nitro/fs.h"
#include "nitro/os.h"

// The font for debug text, of 8x8 characters with 1 bit per pixel, drawn as 16 color tiles

// A font file starts with the offsets of the glyphs and of the width of each character
typedef struct {
    u32 glyphOffset;
    u32 widthOffset;
    u32 charCount;
    u8 width;
    u8 height;
} SystemFontHeader;

typedef struct {
    const u8 *data;
    // The foreground and background color of each pixel of a row of a 16 color tile
    u32 fgMask;
    u32 bgMask;
    BOOL builtIn;
} SystemFont;

static u32 GFL_SystemFontUnpackBits(u8 bits);

static SystemFont *sSystemFont;

#include "systemfont_data.h"

void GFL_SystemFontInit(const char *path) {
    if (sSystemFont == NULL) {
        sSystemFont = GFL_HeapAllocate(HEAPID_SYSTEM, sizeof(SystemFont), FALSE, "systemfont.c", 68);
        if (path != NULL) {
            FSFile file;

            FS_InitFile(&file);
            if (FS_OpenFile(&file, path) == TRUE) {
                u32 size = FS_GetLength(&file);

                sSystemFont->data = GFL_HeapAllocate(HEAPID_SYSTEM, size, FALSE, "systemfont.c", 77);
                FS_ReadFile(&file, (void *)sSystemFont->data, size);
                FS_CloseFile(&file);
            } else {
                OS_Terminate();
            }
            sSystemFont->builtIn = FALSE;
        } else {
            sSystemFont->data = sDefaultFont;
            sSystemFont->builtIn = TRUE;
            GFL_SystemFontSetColorIndices(1, 15);
        }
    }
}

void GFL_SystemFontGetGlyph(int c, u8 *dest) {
    const SystemFontHeader *header = (const SystemFontHeader *)sSystemFont->data;
    const u8 *glyph = sSystemFont->data + header->glyphOffset + c * 0x20 / 4;
    u32 fgMask = sSystemFont->fgMask;
    u32 bgMask = sSystemFont->bgMask;
    u32 *row = (u32 *)dest;
    int i;

    for (i = 0; i < 8; i++) {
        u32 bits = GFL_SystemFontUnpackBits(*glyph++);

        *row++ = (fgMask & bits) | (bgMask & (bits ^ 0xffffffff));
    }
    dest += 0x20;
    *dest = (sSystemFont->data + ((const SystemFontHeader *)sSystemFont->data)->widthOffset)[c];
}

// Turns a row of 8 pixels, 1 bit each, into 4 bits each
static u32 GFL_SystemFontUnpackBits(u8 bits) {
    int i;
    u32 result = 0;

    for (i = 0; i < 8; i++) {
        if ((1 << i) & bits) {
            result |= 0xf0000000 >> (i * 4);
        }
    }
    return result;
}

u8 GFL_SystemFontGetHeight(void) {
    return ((const SystemFontHeader *)sSystemFont->data)->height;
}

u16 GFL_SystemFontGetTabSize(void) {
    return 7;
}

void GFL_SystemFontSetColorIndices(u32 fg, u32 bg) {
    sSystemFont->fgMask = (fg << 28) | (fg << 24) | (fg << 20) | (fg << 16) | (fg << 12) | (fg << 8) | (fg << 4) | fg;
    sSystemFont->bgMask = (bg << 28) | (bg << 24) | (bg << 20) | (bg << 16) | (bg << 12) | (bg << 8) | (bg << 4) | bg;
}
