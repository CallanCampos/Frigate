#include "types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "system/gf_font.h"

// Fonts in Nitro's font format, read from an archive file: its font information, the glyph cells' sizes, the widths
// and the character maps are loaded, and the glyphs are either loaded too or read from the file one at a time. Glyphs
// have 2 bits a pixel, and are drawn as 4-bit tiles through a table of the current letter, shadow and background
// color indices

// The font information block (FINF), after its header
typedef struct {
    u8 fontType;
    u8 lineFeed;
    u16 alterCharIndex;
    u8 defaultLeft;
    u8 defaultGlyphWidth;
    u8 defaultCharWidth;
    u8 encoding;
    // Offsets in the file of the glyphs' block (CGLP), the widths' (CWDH) and the first character map's (CMAP), each
    // after its block's header
    u32 glyphOffset;
    u32 widthOffset;
    u32 mapOffset;
} FontInfo;

// The start of the glyphs' block
typedef struct {
    u8 cellWidth;
    u8 cellHeight;
    u16 cellSize;
    s8 baseline;
    u8 maxCharWidth;
    u8 bpp;
    u8 flags;
} FontGlyphInfo;

// A character map: the characters from first to last map to glyphs by an offset, a table or a sorted table of pairs
typedef struct {
    u16 first;
    u16 last;
    u16 method;
    u16 reserved;
    u32 next;
    u16 data[];
} FontMap;

enum {
    FONT_MAP_DIRECT,
    FONT_MAP_TABLE,
    FONT_MAP_SCAN,
};

typedef void (*FontGetGlyphProc)(const Font *font, u32 index, void *dest, GlyphInfo *info);
typedef void (*FontBitmapCopyProc)(const u8 *src, u16 lastBits, u16 *dest);
typedef u8 (*FontGetWidthProc)(Font *font, u32 index);
typedef void (*FontInitProc)(Font *font, u32 tilesWide, u32 tilesHigh, HeapID heapId);
typedef void (*FontFreeProc)(Font *font);

struct Font {
    u32 loadType;
    FontGetGlyphProc getGlyph;
    FontBitmapCopyProc copyBitmap;
    // All the glyphs, when they are loaded
    u8 *glyphs;
    u8 unk10[0x40];
    ArcTool *handle;
    u32 fileId;
    FontInfo info;
    FontGetWidthProc getWidth;
    // The glyph drawn for characters the font does not have
    u16 unknownIndex;
    u8 fixedWidth;
    FontGlyphInfo glyphInfo;
    // The bits of a glyph row's last read, from 1 to 8
    u16 lastBits;
    u8 tilesWide;
    u8 tilesHigh;
    // Where the file starts in the archive
    u32 fileOffset;
    u8 *widths;
    u8 *maps;
    // Where the glyphs start in the file
    u32 glyphsOffset;
    // A glyph's cell, as read
    u8 *cell;
};

// Reads bits from the top of each byte down
typedef struct {
    const u8 *src;
    u32 bitsLeft;
    u8 byte;
} BitReader;

static void GFL_FontLoad(Font *font, u32 fileId, BOOL fixedWidth, HeapID heapId);
static void func_02022f38(Font *font);
static void GFL_FontInit(Font *font, u32 loadType, HeapID heapId);
static void GFL_FontInitCached(Font *font, u32 tilesWide, u32 tilesHigh, HeapID heapId);
static void GFL_FontInitStreamed(Font *font, u32 tilesWide, u32 tilesHigh, HeapID heapId);
static void func_0202304c(Font *font);
static void func_02023060(Font *font);
static void func_02023070(Font *font);
static u32 GFL_FontGetCharIndex(const Font *font, u16 c);
static void GFL_FontGetGlyphCached(const Font *font, u32 index, void *dest, GlyphInfo *info);
static void GFL_FontGetGlyphStreamed(const Font *font, u32 index, void *dest, GlyphInfo *info);
static void GFL_FontGlyphCopy(const Font *font, const u8 *cell, u32 lastBits, void *dest, GlyphInfo *info);
static u8 GFL_FontGetCharWidthDynamic(Font *font, u32 index);
static u8 GFL_FontGetCharWidthFixed(Font *font, u32 index);
static void GFL_FontBitmapCopy_8x8(const u8 *src, u16 lastBits, u16 *dest);
static void GFL_BitReaderInit(BitReader *reader, const u8 *src);
static u8 GFL_BitReaderRead(BitReader *reader, u8 bits);
static void GFL_FontBitmapCopy_8x16(const u8 *src, u16 lastBits, u16 *dest);
static void GFL_FontBitmapCopy_16x8(const u8 *src, u16 lastBits, u16 *dest);
static void GFL_FontBitmapCopy_16x16(const u8 *src, u16 lastBits, u16 *dest);

// Indexed by the load type: the glyphs read when drawn, or all loaded
static const FontInitProc v_GFLFontInitSub[] = {
    GFL_FontInitStreamed,
    GFL_FontInitCached,
};

static const FontFreeProc v_GFLFontFreeSub[] = {
    func_02023070,
    func_02023060,
};

// The top n bits of a byte
static const u8 BIT_COUNT_TO_BIT_MASK[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};

static u8 g_LastTextColor2[3];
static BitReader g_GlyphBitReader;
// Four 2-bit pixels to four 4-bit pixels in the color indices
static u16 TEXT_COLOR_SHUFFLE_LUT[0x100];

Font *GFL_FontCreate(u32 arcId, u32 fileId, u32 loadType, BOOL fixedWidth, HeapID heapId) {
    Font *font = GFL_HeapAllocate(heapId, sizeof(Font), FALSE, "gf_font.c", 201);

    if (font != NULL) {
        font->handle = GFL_ArcSysCreateFileHandle(arcId, heapId);
        font->fileId = fileId;
        GFL_FontLoad(font, fileId, fixedWidth, heapId);
        GFL_FontInit(font, loadType, heapId);
    }
    return font;
}

void GFL_FontFree(Font *font) {
    func_0202304c(font);
    func_02022f38(font);
    GFL_HeapFree(font);
}

static void GFL_FontLoad(Font *font, u32 fileId, BOOL fixedWidth, HeapID heapId) {
    u32 size;

    if (font->handle == NULL) {
        return;
    }
    GFL_ArcToolReadRange(font->handle, fileId, 0x18, sizeof(FontInfo), &font->info);
    GFL_ArcToolCopyDataOfs(font->handle, fileId, &font->fileOffset);
    font->fixedWidth = fixedWidth;
    if (fixedWidth) {
        font->widths = NULL;
        font->getWidth = GFL_FontGetCharWidthFixed;
    } else {
        size = font->info.mapOffset - font->info.widthOffset;
        font->widths = GFL_HeapAllocate(heapId, size, FALSE, "gf_font.c", 257);
        font->getWidth = GFL_FontGetCharWidthDynamic;
        GFL_ArcToolReadRange(font->handle, fileId, font->info.widthOffset, size, font->widths);
    }
    GFL_ArcToolReadRange(font->handle, fileId, font->info.glyphOffset, sizeof(FontGlyphInfo), &font->glyphInfo);
    font->tilesWide = font->glyphInfo.cellWidth / 8 + (font->glyphInfo.cellWidth % 8 != 0 ? 1 : 0);
    font->tilesHigh = font->glyphInfo.cellHeight / 8 + (font->glyphInfo.cellHeight % 8 != 0 ? 1 : 0);
    font->lastBits = (font->glyphInfo.cellWidth * 2) % 8;
    if (font->lastBits == 0) {
        font->lastBits = 8;
    }
    font->glyphsOffset = font->info.glyphOffset + sizeof(FontGlyphInfo);
    font->cell = GFL_HeapAllocate(heapId, font->glyphInfo.cellSize, FALSE, "gf_font.c", 275);
    size = GFL_ArcToolGetDataLength(font->handle, fileId) - font->info.mapOffset;
    font->maps = GFL_HeapAllocate(heapId, size, FALSE, "gf_font.c", 280);
    GFL_ArcToolReadRange(font->handle, fileId, font->info.mapOffset, size, font->maps);
    font->unknownIndex = 0;
    font->unknownIndex = GFL_FontGetCharIndex(font, '?');
}

static void func_02022f38(Font *font) {
    if (font->cell != NULL) {
        GFL_HeapFree(font->cell);
    }
    if (font->widths != NULL) {
        GFL_HeapFree(font->widths);
    }
    if (font->maps != NULL) {
        GFL_HeapFree(font->maps);
    }
    if (font->handle != NULL) {
        GFL_ArcToolFree(font->handle);
    }
}

static void GFL_FontInit(Font *font, u32 loadType, HeapID heapId) {
    font->loadType = loadType;
    v_GFLFontInitSub[loadType](font, font->tilesWide, font->tilesHigh, heapId);
}

static void GFL_FontInitCached(Font *font, u32 tilesWide, u32 tilesHigh, HeapID heapId) {
    u32 size = GFL_ArcToolGetDataLength(font->handle, font->fileId) - font->glyphsOffset;

    font->glyphs = GFL_HeapAllocate(heapId, size, FALSE, "gf_font.c", 372);
    GFL_ArcToolReadRange(font->handle, font->fileId, font->glyphsOffset, size, font->glyphs);
    font->getGlyph = GFL_FontGetGlyphCached;
    if (tilesWide == 1) {
        font->copyBitmap = tilesHigh == 1 ? GFL_FontBitmapCopy_8x8 : GFL_FontBitmapCopy_8x16;
    } else {
        font->copyBitmap = tilesHigh == 1 ? GFL_FontBitmapCopy_16x8 : GFL_FontBitmapCopy_16x16;
    }
}

static void GFL_FontInitStreamed(Font *font, u32 tilesWide, u32 tilesHigh, HeapID heapId) {
    font->glyphs = NULL;
    font->getGlyph = GFL_FontGetGlyphStreamed;
    if (tilesWide == 1) {
        font->copyBitmap = tilesHigh == 1 ? GFL_FontBitmapCopy_8x8 : GFL_FontBitmapCopy_8x16;
    } else {
        font->copyBitmap = tilesHigh == 1 ? GFL_FontBitmapCopy_16x8 : GFL_FontBitmapCopy_16x16;
    }
}

static void func_0202304c(Font *font) {
    v_GFLFontFreeSub[font->loadType](font);
}

static void func_02023060(Font *font) {
    GFL_HeapFree(font->glyphs);
    font->glyphs = NULL;
}

static void func_02023070(Font *font) {
}

void GFL_FontGetGlyph(Font *font, u16 c, void *dest, GlyphInfo *info) {
    font->getGlyph(font, GFL_FontGetCharIndex(font, c), dest, info);
}

static u32 GFL_FontGetCharIndex(const Font *font, u16 c) {
    FontMap *map = (FontMap *)font->maps;

    for (;;) {
        if (map->first <= c && map->last >= c) {
            switch (map->method) {
            case FONT_MAP_DIRECT:
                return map->data[0] + (c - map->first);
            case FONT_MAP_TABLE:
                return map->data[(u16)(c - map->first)];
            case FONT_MAP_SCAN: {
                // Pairs of a character and its glyph, after their count
                int low = 0;
                int high = map->data[0] - 1;

                while (low <= high) {
                    int mid = low + (high - low) / 2;
                    int i = mid * 2 + 1;

                    if (map->data[i] < c) {
                        low = mid + 1;
                    } else if (c < map->data[i]) {
                        high = mid - 1;
                    } else {
                        return map->data[i + 1];
                    }
                }
                break;
            }
            }
            return font->unknownIndex;
        }
        if (map->next != 0) {
            map = (FontMap *)(font->maps + (map->next - font->info.mapOffset));
        } else {
            return 0;
        }
    }
}

static void GFL_FontGetGlyphCached(const Font *font, u32 index, void *dest, GlyphInfo *info) {
    sys_memcpy(font->glyphs + index * font->glyphInfo.cellSize, font->cell, font->glyphInfo.cellSize);
    GFL_FontGlyphCopy(font, font->cell, font->lastBits, dest, info);
}

static void GFL_FontGetGlyphStreamed(const Font *font, u32 index, void *dest, GlyphInfo *info) {
    GFL_ArcToolSeek(font->handle, font->fileOffset + (font->glyphsOffset + index * font->glyphInfo.cellSize));
    GFL_ArcToolReadRaw(font->handle, font->glyphInfo.cellSize, font->cell);
    GFL_FontGlyphCopy(font, font->cell, font->lastBits, dest, info);
}

// A cell is the glyph's left offset, width and advance, then its pixels
static void GFL_FontGlyphCopy(const Font *font, const u8 *cell, u32 lastBits, void *dest, GlyphInfo *info) {
    font->copyBitmap(cell + 3, lastBits, dest);
    info->x = cell[0];
    info->width = cell[1];
    info->advance = cell[2];
    info->height = font->info.lineFeed;
}

u32 GFL_FontGetCharWidth(Font *font, u16 c) {
    return font->getWidth(font, GFL_FontGetCharIndex(font, c));
}

u8 GFL_FontGetCharHeight(Font *font) {
    return font->info.lineFeed;
}

// The widths are three common widths, then offsets of: a 2-bit class for each glyph, the first three meaning those
// widths; a hash table of widths, negative for a bucket of exceptions; and the buckets, each a count and then
// big-endian characters and their widths
static u8 GFL_FontGetCharWidthDynamic(Font *font, u32 index) {
    u8 *widths = font->widths;
    u32 bit = index * 2;
    u8 class = ((widths + *(u32 *)(widths + 4))[bit >> 3] >> (6 - (bit & 7))) & 3;
    u16 hash;
    s8 width;

    if (class < 3) {
        return widths[class];
    }
    {
        u16 parity = ((index >> 12) ^ (index >> 11) ^ (index >> 10) ^ (index >> 9)) & 1;

        hash = ((index & 0x1ff) ^ (parity << 3)) & 0x1ff;
    }
    width = ((s8 *)(widths + *(u32 *)(widths + 8)))[hash];
    if (width >= 0) {
        return width;
    } else {
        u16 bucket = (u16)-width - 1;
        u8 *entry = widths + *(u32 *)(widths + 0xc);
        u8 count;
        u8 i;

        while (bucket != 0) {
            entry += entry[0] * 3 + 1;
            bucket--;
        }
        count = *entry++;
        for (i = 0; i < count; i++, entry += 3) {
            if (index == (u16)((entry[0] << 8) | entry[1])) {
                return entry[2];
            }
        }
    }
    return font->info.defaultCharWidth;
}

static u8 GFL_FontGetCharWidthFixed(Font *font, u32 index) {
    return font->info.defaultCharWidth;
}

void func_020232d0(void) {
    func_020232d8();
}

void func_020232d8(void) {
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
}

void GFL_TextRndUpdateColorIndexLUT(u8 letter, u8 shadow, u8 background) {
    int colors[4];
    int i, j, k, l;
    int n = 0;

    g_LastTextColor2[0] = letter;
    colors[1] = letter;
    g_LastTextColor2[1] = shadow;
    colors[0] = 0;
    colors[2] = shadow;
    colors[3] = background;
    g_LastTextColor2[2] = background;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 4; k++) {
                for (l = 0; l < 4; l++) {
                    TEXT_COLOR_SHUFFLE_LUT[n++] = (colors[l] << 12) | (colors[k] << 8) | (colors[j] << 4) | colors[i];
                }
            }
        }
    }
}

void GFL_TextRndGetGlobalColors(u8 *letter, u8 *shadow, u8 *background) {
    *letter = g_LastTextColor2[0];
    *shadow = g_LastTextColor2[1];
    *background = g_LastTextColor2[2];
}

BOOL GFL_TextRndCheckColorIndexChange(u8 letter, u8 shadow, u8 background) {
    if (letter != g_LastTextColor2[0] || shadow != g_LastTextColor2[1] || background != g_LastTextColor2[2]) {
        return TRUE;
    }
    return FALSE;
}

// GFL_BitReaderInit and GFL_BitReaderRead, which GFL_FontBitmapCopy_8x8 has inlined
static inline void BitReader_Init(BitReader *reader, const u8 *src) {
    reader->src = src;
    reader->byte = *src;
    reader->bitsLeft = 8;
}

static inline u8 BitReader_Read(BitReader *reader, u8 bits) {
    u32 high;
    u32 low;
    u32 shift;

    if (reader->bitsLeft < bits) {
        // The rest of this byte, above the start of the next
        high = reader->byte & BIT_COUNT_TO_BIT_MASK[reader->bitsLeft];
        shift = reader->bitsLeft;
        bits -= (u8)reader->bitsLeft;
        reader->byte = *++reader->src;
        reader->bitsLeft = 8;
    } else {
        high = 0;
        shift = 0;
    }
    low = reader->byte & BIT_COUNT_TO_BIT_MASK[bits];
    reader->bitsLeft -= bits;
    if (reader->bitsLeft == 0) {
        reader->byte = *++reader->src;
        reader->bitsLeft = 8;
    } else {
        reader->byte <<= bits;
    }
    return (low >> shift) | high;
}

static void GFL_FontBitmapCopy_8x8(const u8 *src, u16 lastBits, u16 *dest) {
    BitReader_Init(&g_GlyphBitReader, src);
    dest[0] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, 8)];
    dest[1] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, (u8)lastBits)];
    dest[2] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, 8)];
    dest[3] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, (u8)lastBits)];
    dest[4] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, 8)];
    dest[5] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, (u8)lastBits)];
    dest[6] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, 8)];
    dest[7] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, (u8)lastBits)];
    dest[8] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, 8)];
    dest[9] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, (u8)lastBits)];
    dest[10] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, 8)];
    dest[11] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, (u8)lastBits)];
    dest[12] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, 8)];
    dest[13] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, (u8)lastBits)];
    dest[14] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, 8)];
    dest[15] = TEXT_COLOR_SHUFFLE_LUT[BitReader_Read(&g_GlyphBitReader, (u8)lastBits)];
}

static void GFL_BitReaderInit(BitReader *reader, const u8 *src) {
    reader->src = src;
    reader->byte = *src;
    reader->bitsLeft = 8;
}

static u8 GFL_BitReaderRead(BitReader *reader, u8 bits) {
    u32 high;
    u32 low;
    u32 shift;

    if (reader->bitsLeft < bits) {
        // The rest of this byte, above the start of the next
        high = reader->byte & BIT_COUNT_TO_BIT_MASK[reader->bitsLeft];
        shift = reader->bitsLeft;
        bits -= (u8)reader->bitsLeft;
        reader->byte = *++reader->src;
        reader->bitsLeft = 8;
    } else {
        high = 0;
        shift = 0;
    }
    low = reader->byte & BIT_COUNT_TO_BIT_MASK[bits];
    reader->bitsLeft -= bits;
    if (reader->bitsLeft == 0) {
        reader->byte = *++reader->src;
        reader->bitsLeft = 8;
    } else {
        reader->byte <<= bits;
    }
    return (low >> shift) | high;
}

static void GFL_FontBitmapCopy_8x16(const u8 *src, u16 lastBits, u16 *dest) {
    GFL_BitReaderInit(&g_GlyphBitReader, src);
    dest[0] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[1] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[2] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[3] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[4] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[5] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[6] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[7] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[8] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[9] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[10] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[11] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[12] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[13] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[14] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[15] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[16] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[17] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[18] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[19] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[20] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[21] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[22] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[23] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[24] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[25] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[26] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[27] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[28] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[29] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[30] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[31] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
}

static void GFL_FontBitmapCopy_16x8(const u8 *src, u16 lastBits, u16 *dest) {
    u16 *right = dest + 16;

    GFL_BitReaderInit(&g_GlyphBitReader, src);
    dest[0] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[1] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[0] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[1] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[2] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[3] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[2] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[3] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[4] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[5] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[4] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[5] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[6] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[7] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[6] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[7] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[8] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[9] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[8] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[9] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[10] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[11] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[10] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[11] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[12] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[13] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[12] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[13] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[14] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[15] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[14] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[15] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
}

static void GFL_FontBitmapCopy_16x16(const u8 *src, u16 lastBits, u16 *dest) {
    u16 *right = dest + 16;

    GFL_BitReaderInit(&g_GlyphBitReader, src);
    dest[0] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[1] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[0] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[2] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[3] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[2] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[4] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[5] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[4] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[6] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[7] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[6] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[8] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[9] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[8] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[10] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[11] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[10] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[12] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[13] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[12] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[14] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[15] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[14] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[32] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[33] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[32] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[34] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[35] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[34] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[36] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[37] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[36] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[38] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[39] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[38] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[40] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[41] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[40] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[42] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[43] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[42] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[44] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[45] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[44] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
    dest[46] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    dest[47] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, 8)];
    right[46] = TEXT_COLOR_SHUFFLE_LUT[GFL_BitReaderRead(&g_GlyphBitReader, (u8)lastBits)];
}
