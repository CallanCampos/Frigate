#include "g2di_BitReader.h"
#include "nitro/gx.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_CharCanvas.c: drawing glyphs into, and clearing, a canvas of characters laid out as a BG's or as
// the OBJs of a 1D OBJ area, and laying out the screen or the OBJs that show a canvas. A 1D OBJ canvas is covered with
// the largest OBJs that fit, then the rest of it to the right, below and at the bottom right likewise. The game keeps
// the 2D OBJ canvas's functions table but not the function that uses it

// The log2 sizes in characters of an OBJ
typedef struct {
    u8 widthShift;
    u8 heightShift;
} ObjSizeShift;

// A 1D OBJ canvas's param: the size of its largest OBJ
typedef union {
    u32 param;
    struct {
        u32 objWidthShift : 8;
        u32 objHeightShift : 8;
        u32 : 16;
    };
} Obj1DParam;

// A glyph drawn into one character: x and y place the glyph's top left in the character, and may be negative
typedef struct {
    u32 *pChar;
    const u8 *pSrc;
    int x;
    int y;
    int width;
    int height;
    int srcLineBits;
    int srcBpp;
    int dstBpp;
    int colorOffset;
} DrawGlyphParam;

static void DrawGlyphLinear(const NNSG2dCharCanvas *pCC, const NNSG2dFont *pFont, int x, int y, int cl,
                            const NNSG2dGlyph *pGlyph);
static void DrawGlyph1D(const NNSG2dCharCanvas *pCC, const NNSG2dFont *pFont, int x, int y, int cl,
                        const NNSG2dGlyph *pGlyph);
static void ClearContinuous(const NNSG2dCharCanvas *pCC, int cl);
static void ClearLinear(const NNSG2dCharCanvas *pCC, int cl);
static void ClearAreaLinear(const NNSG2dCharCanvas *pCC, int cl, int x, int y, int w, int h);
static void ClearArea1D(const NNSG2dCharCanvas *pCC, int cl, int x, int y, int w, int h);
static void MapScrToCharLinear(u16 *pScr, int areaWidth, int areaHeight, int scnWidth, int charNo, int cplt);

// The 2D OBJ canvas's functions. Not static, since nothing the game keeps uses it and MWCC would leave it out; the name
// is ours
const NNSiG2dCharCanvasVTable NNSi_G2dCharCanvasVTableOBJ2DRect = { DrawGlyphLinear, ClearLinear, ClearAreaLinear };
// MWCC emits these two in the reverse of their order here, which puts the BG's first as in the ROM
static const NNSiG2dCharCanvasVTable VTABLE_OBJ1D = { DrawGlyph1D, ClearContinuous, ClearArea1D };
static const NNSiG2dCharCanvasVTable VTABLE_BG = { DrawGlyphLinear, ClearContinuous, ClearAreaLinear };

// The largest OBJ that fits in an area, by the log2 sizes of the area, at most 8 characters: [height][width]
static const ObjSizeShift sMaxObjSize[4][4] = {
    { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 2, 0 } },
    { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 2, 1 } },
    { { 0, 2 }, { 1, 2 }, { 2, 2 }, { 3, 2 } },
    { { 0, 2 }, { 1, 2 }, { 2, 3 }, { 3, 3 } },
};

// The shape of an OBJ by its log2 sizes: [heightShift][widthShift]
static const u32 sOamShape[4][4] = {
    { GX_OAM_SHAPE_8x8, GX_OAM_SHAPE_16x8, GX_OAM_SHAPE_32x8, 0 },
    { GX_OAM_SHAPE_8x16, GX_OAM_SHAPE_16x16, GX_OAM_SHAPE_32x16, 0 },
    { GX_OAM_SHAPE_8x32, GX_OAM_SHAPE_16x32, GX_OAM_SHAPE_32x32, GX_OAM_SHAPE_64x32 },
    { 0, 0, GX_OAM_SHAPE_32x64, GX_OAM_SHAPE_64x64 },
};

static inline int CharSize(int bpp) {
    return bpp * 8 * 8 / 8;
}

// log2 of a size in characters, at most 3
static inline int ObjSizeShiftOf(int n) {
    return (n >= 8) ? 3 : 31 - (int)MATH_CountLeadingZeros((u32)n);
}

// A color repeated over a word
// The index of the character at x, y in a 1D OBJ canvas
static u32 GetCharIndex1D(u32 x, u32 y, int areaWidth, int areaHeight, int objWidthShift, int objHeightShift) {
    u32 charNo = 0;

    while (TRUE) {
        const u32 maskW = 0xffffffff << objWidthShift;
        const u32 maskH = 0xffffffff << objHeightShift;
        const u32 bodyWidth = areaWidth & maskW;
        const u32 bodyHeight = areaHeight & maskH;

        if (bodyHeight <= y) {
            charNo += areaWidth * bodyHeight;
            if (bodyWidth <= x) {
                charNo += bodyWidth * (areaHeight - bodyHeight);
                x -= bodyWidth;
                y -= bodyHeight;
                areaWidth -= bodyWidth;
                areaHeight -= bodyHeight;
            } else {
                y -= bodyHeight;
                areaWidth = bodyWidth;
                areaHeight -= bodyHeight;
            }
        } else {
            const u32 lowH = ~maskH;

            if (bodyWidth <= x) {
                charNo += bodyWidth * bodyHeight;
                x -= bodyWidth;
                areaWidth -= bodyWidth;
                areaHeight = bodyHeight;
            } else {
                charNo += (y & maskH) * bodyWidth;
                charNo += (x & maskW) << objHeightShift;
                charNo += (y & lowH) << objWidthShift;
                charNo += x & ~maskW;
                return charNo;
            }
        }

        {
            const ObjSizeShift *pSize = &sMaxObjSize[ObjSizeShiftOf(areaHeight)][ObjSizeShiftOf(areaWidth)];

            objWidthShift = pSize->widthShift;
            objHeightShift = pSize->heightShift;
        }
    }
}

static u32 GetOamShape(const ObjSizeShift *pSize) {
    return sOamShape[pSize->heightShift][pSize->widthShift];
}

// Fills the w by h pixels at x, y of a character
static void FillChar(u32 *pChar, int x, int y, int w, int h, u32 cl, int bpp) {
    if (w == 8 && h == 8) {
        MI_CpuFillFast(pChar, cl, (u32)bpp * 8);
        return;
    }

    if (bpp == 4) {
        const u32 left = x * 4;
        const u32 right = 32 - (left + w * 4);
        const u32 mask = ((0xffffffff >> left) << (left + right)) >> right;
        u32 data;
        u32 *p;
        u32 keep;
        u32 *pEnd;

        data = cl & mask;
        keep = ~mask;
        p = pChar + y;
        pEnd = p + h;
        while (p < pEnd) {
            *p = (*p & keep) | data;
            p++;
        }
    } else {
        // Two words a line, with the masks of both
        const u32 left = x * 8;
        const u32 right = 64 - (left + w * 8);
        u32 mask0 = 0xffffffff >> left;
        u32 mask1;
        u32 data0;
        u32 data1;
        u32 keep0;
        u32 keep1;
        u32 *pEnd;

        if (right >= 32) {
            mask0 = (mask0 << (left + (right - 32))) >> (right - 32);
        } else {
            mask0 <<= left;
        }
        mask1 = 0xffffffff << right;
        if (left >= 32) {
            mask1 = (mask1 >> ((left - 32) + right)) << (left - 32);
        } else {
            mask1 >>= right;
        }

        data0 = cl & mask0;
        data1 = cl & mask1;
        keep0 = ~mask0;
        keep1 = ~mask1;
        pChar += y * 2;
        pEnd = pChar + h * 2;
        while (pChar < pEnd) {
            pChar[0] = (pChar[0] & keep0) | data0;
            pChar[1] = (pChar[1] & keep1) | data1;
            pChar += 2;
        }
    }
}

static void DrawGlyphChar(const DrawGlyphParam *p) {
    u32 sx = (p->x >= 0) ? p->x : 0;
    const int sy = (p->y >= 0) ? p->y : 0;
    u32 ex = (p->x + p->width < 8) ? p->x + p->width : 8;
    const int ey = (p->y + p->height < 8) ? p->y + p->height : 8;
    const int offsetY = -MATH_MIN(p->y, 0);
    const int offsetX = -MATH_MIN(p->x, 0);
    const u8 *pSrc;
    int srcOffset;
    int srcLineBits;
    int srcBpp;
    int dstBpp;

    srcLineBits = p->srcLineBits;
    srcBpp = p->srcBpp;
    srcOffset = offsetY * srcLineBits + srcBpp * offsetX;
    pSrc = p->pSrc;
    dstBpp = p->dstBpp;

    // From pixels to bit positions in a line of the character
    sx *= dstBpp;
    ex *= dstBpp;

    if (dstBpp == 4) {
        u32 *pLine = p->pChar + sy;
        u32 *pEnd = p->pChar + ey;
        const u32 colorOffset = p->colorOffset;

        for (; pLine < pEnd; pLine++) {
            u32 line = *pLine;
            NNSiG2dBitReader reader;
            u32 bx;

            NNSi_G2dBitReaderInit(&reader, pSrc + (u32)srcOffset / 8);
            NNSi_G2dBitReaderRead(&reader, srcOffset % 8);
            for (bx = sx; bx < ex; bx += 4) {
                const u32 c = NNSi_G2dBitReaderRead(&reader, srcBpp);

                if (c != 0) {
                    line = (line & ~(0xf << bx)) | ((colorOffset + c) << bx);
                }
            }
            *pLine = line;
            srcOffset += srcLineBits;
        }
    } else {
        u32 *pLine = p->pChar + sy * 2;
        u32 *pEnd = p->pChar + ey * 2;
        const u32 colorOffset = p->colorOffset;

        for (; pLine < pEnd; pLine += 2) {
            u32 line0 = pLine[0];
            u32 line1 = pLine[1];
            NNSiG2dBitReader reader;
            u32 bx;

            NNSi_G2dBitReaderInit(&reader, pSrc + (u32)srcOffset / 8);
            NNSi_G2dBitReaderRead(&reader, srcOffset % 8);
            for (bx = sx; bx < ex; bx += 8) {
                const u32 c = NNSi_G2dBitReaderRead(&reader, srcBpp);

                if (c != 0) {
                    if (bx < 32) {
                        line0 = (line0 & ~(0xff << bx)) | ((colorOffset + c) << bx);
                    } else {
                        const u32 shift = bx - 32;

                        line1 = (line1 & ~(0xff << shift)) | ((colorOffset + c) << shift);
                    }
                }
            }
            pLine[0] = line0;
            pLine[1] = line1;
            srcOffset += srcLineBits;
        }
    }
}

// Draws a glyph into a canvas whose characters are in rows of param characters: a BG's, or a 2D OBJ area's
static void DrawGlyphLinear(const NNSG2dCharCanvas *pCC, const NNSG2dFont *pFont, int x, int y, int cl,
                            const NNSG2dGlyph *pGlyph) {
    u32 left, top, right, bottom;
    int numX, numY;
    DrawGlyphParam param;
    int yEnd;
    int lineSkip;
    int xEnd;
    int cx;
    u8 *pChar;
    const int charSize = CharSize(pCC->dstBpp);
    const int areaWidth = pCC->areaWidth;
    const int areaHeight = pCC->areaHeight;
    u8 *charBase = pCC->charBase;
    const int glyphWidth = pGlyph->pWidths->glyphWidth;
    const int glyphHeight = pFont->pRes->pGlyph->cellHeight;

    if (glyphWidth == 0 || x + glyphWidth < 0 || y + glyphHeight < 0) {
        return;
    }

    // The characters the glyph covers
    left = (x <= 0) ? 0 : (u32)x / 8;
    top = (y <= 0) ? 0 : (u32)y / 8;
    right = (u32)(x + glyphWidth + 7) / 8;
    if (right >= areaWidth) {
        right = areaWidth;
    }
    bottom = (u32)(y + glyphHeight + 7) / 8;
    if (bottom >= areaHeight) {
        bottom = areaHeight;
    }
    numX = right - left;
    numY = bottom - top;
    if (numX < 0 || numY < 0) {
        return;
    }

    pChar = charBase + charSize * (left + pCC->param * top);
    lineSkip = (pCC->param - numX) * charSize;
    if (x >= 0) {
        x &= 7;
    }
    if (y >= 0) {
        y &= 7;
    }
    xEnd = x - numX * 8;
    yEnd = y - numY * 8;

    param.pSrc = pGlyph->image;
    param.width = glyphWidth;
    param.height = glyphHeight;
    param.colorOffset = cl - 1;
    param.srcBpp = pFont->pRes->pGlyph->bpp;
    param.dstBpp = pCC->dstBpp;
    param.srcLineBits = param.srcBpp * pFont->pRes->pGlyph->cellWidth;

    for (; y > yEnd; y -= 8) {
        param.y = y;
        for (cx = x; cx > xEnd; cx -= 8) {
            param.pChar = (u32 *)pChar;
            param.x = cx;
            DrawGlyphChar(&param);
            pChar += charSize;
        }
        pChar += lineSkip;
    }
}

static void DrawGlyph1D(const NNSG2dCharCanvas *pCC, const NNSG2dFont *pFont, int x, int y, int cl,
                        const NNSG2dGlyph *pGlyph) {
    const int charSize = CharSize(pCC->dstBpp);
    const u8 glyphWidth = pGlyph->pWidths->glyphWidth;
    const int glyphHeight = pFont->pRes->pGlyph->cellHeight;
    const int areaW = pCC->areaWidth;
    const int areaH = pCC->areaHeight;
    u32 left, top, right, bottom;
    int numX, numY;
    int xEnd, yEnd;
    int cy;
    u8 *charBase;
    Obj1DParam objParam;
    DrawGlyphParam param;

    if (glyphWidth == 0 || x + glyphWidth < 0 || y + glyphHeight < 0) {
        return;
    }

    left = (x <= 0) ? 0 : (u32)x / 8;
    top = (y <= 0) ? 0 : (u32)y / 8;
    right = (u32)(x + glyphWidth + 7) / 8;
    if (right >= areaW) {
        right = areaW;
    }
    bottom = (u32)(y + glyphHeight + 7) / 8;
    if (bottom >= areaH) {
        bottom = areaH;
    }
    numX = right - left;
    numY = bottom - top;
    if (numX < 0 || numY < 0) {
        return;
    }

    if (x >= 0) {
        x &= 7;
    }
    if (y >= 0) {
        y &= 7;
    }
    xEnd = x - numX * 8;
    yEnd = y - numY * 8;
    charBase = pCC->charBase;

    param.pSrc = pGlyph->image;
    param.height = glyphHeight;
    param.width = glyphWidth;
    param.colorOffset = cl - 1;
    param.srcBpp = pFont->pRes->pGlyph->bpp;
    param.dstBpp = pCC->dstBpp;
    param.srcLineBits = param.srcBpp * pFont->pRes->pGlyph->cellWidth;

    objParam.param = pCC->param;
    const int areaWidth = pCC->areaWidth;
    const int areaHeight = pCC->areaHeight;
    const int objWidthShift = objParam.objWidthShift;
    const int objHeightShift = objParam.objHeightShift;
    for (cy = top; y > yEnd; cy++, y -= 8) {
        int px, cx;

        param.y = y;
        for (px = x, cx = left; px > xEnd; px -= 8, cx++) {
            const u32 charNo = GetCharIndex1D(cx, cy, areaWidth, areaHeight, objWidthShift, objHeightShift);

            param.x = px;
            param.pChar = (u32 *)(charBase + charNo * charSize);
            DrawGlyphChar(&param);
        }
    }
}

static inline u32 FillPattern(u32 cl, int bpp) {
    if (bpp == 4) {
        cl |= cl << 4;
        cl |= cl << 8;
        cl |= cl << 16;
    } else {
        cl |= cl << 8;
        cl |= cl << 16;
    }
    return cl;
}

static void ClearContinuous(const NNSG2dCharCanvas *pCC, int cl) {
    const u32 pattern = FillPattern(cl, pCC->dstBpp);

    MI_CpuFillFast(pCC->charBase, pattern, CharSize(pCC->dstBpp) * (pCC->areaWidth * pCC->areaHeight));
}

static void ClearLinear(const NNSG2dCharCanvas *pCC, int cl) {
    const u32 pattern = FillPattern(cl, pCC->dstBpp);
    const int charSize = CharSize(pCC->dstBpp);
    const int lineSize = charSize * pCC->param;
    const int areaSize = charSize * pCC->areaWidth;
    int i;
    u8 *p = pCC->charBase;

    for (i = 0; i < pCC->areaHeight; i++) {
        MI_CpuFillFast(p, pattern, areaSize);
        p += lineSize;
    }
}

static void ClearAreaLinear(const NNSG2dCharCanvas *pCC, int cl, int x, int y, int w, int h) {
    int cy;
    int height;
    const int right = x + w;
    const int bottom = y + h;
    const int bpp = pCC->dstBpp;
    const u32 pattern = FillPattern(cl, bpp);
    const int x0 = x & ~7;
    cy = y & ~7;
    const int x1 = (right + 7) & ~7;
    const int y1 = (bottom + 7) & ~7;
    const int charSize = CharSize(bpp);
    const int lineSize = pCC->param * charSize;
    u8 *pLine = pCC->charBase + (x0 / 8 + (cy / 8) * pCC->param) * charSize;

    for (; cy < y1; cy += 8) {
        const int top = (cy < y) ? y - cy : 0;
        int cx;
        u8 *p;

        height = MATH_MIN(bottom - cy, 8) - top;
        for (cx = x0, p = pLine; cx < x1; cx += 8) {
            const int left = (cx < x) ? x - cx : 0;
            const int width = MATH_MIN(right - cx, 8) - left;

            FillChar((u32 *)p, left, top, width, height, pattern, bpp);
            p += charSize;
        }
        pLine += lineSize;
    }
}

static void ClearArea1D(const NNSG2dCharCanvas *pCC, int cl, int x, int y, int w, int h) {
    int cy;
    int height;
    const int right = x + w;
    const int bottom = y + h;
    const int bpp = pCC->dstBpp;
    const u32 pattern = FillPattern(cl, bpp);
    Obj1DParam objParam;

    objParam.param = pCC->param;
    const int x0 = x & ~7;
    cy = y & ~7;
    const int x1 = (right + 7) & ~7;
    const int y1 = (bottom + 7) & ~7;
    const int areaWidth = pCC->areaWidth;
    const int areaHeight = pCC->areaHeight;
    const int objWidthShift = objParam.objWidthShift;
    const int objHeightShift = objParam.objHeightShift;
    const int charSize = CharSize(bpp);
    const int charX0 = x0 / 8;
    u8 *charBase = pCC->charBase;
    int charY = cy / 8;

    for (; cy < y1; charY++, cy += 8) {
        const int top = (cy < y) ? y - cy : 0;
        int cx;
        int charX;

        height = MATH_MIN(bottom - cy, 8) - top;
        for (charX = charX0, cx = x0; cx < x1; charX++, cx += 8) {
            const u32 charNo = GetCharIndex1D(charX, charY, areaWidth, areaHeight, objWidthShift, objHeightShift);
            const int left = (cx < x) ? x - cx : 0;
            const int width = MATH_MIN(right - cx, 8) - left;

            FillChar((u32 *)(charBase + charNo * charSize), left, top, width, height, pattern, bpp);
        }
    }
}

static void InitCharCanvas(NNSG2dCharCanvas *pCC, void *charBase, int areaWidth, int areaHeight, int dstBpp,
                           const NNSiG2dCharCanvasVTable *vtable, u32 param) {
    pCC->areaWidth = areaWidth;
    pCC->areaHeight = areaHeight;
    pCC->dstBpp = (u8)dstBpp;
    pCC->charBase = charBase;
    pCC->vtable = vtable;
    pCC->param = param;
}

int NNS_G2dCharCanvasDrawChar(const NNSG2dCharCanvas *pCC, const NNSG2dFont *pFont, int x, int y, int cl, u16 ccode) {
    NNSG2dGlyph glyph;
    u16 idx = NNS_G2dFontFindGlyphIndex(pFont, ccode);

    if (idx == NNS_G2D_GLYPH_INDEX_NOT_FOUND) {
        idx = pFont->pRes->alterCharIndex;
    }
    glyph.pWidths = NNS_G2dFontGetCharWidthsFromIndex(pFont, idx);
    glyph.image = pFont->pRes->pGlyph->glyphTable + idx * pFont->pRes->pGlyph->cellSize;

    switch (pFont->pRes->pGlyph->flags) {
    case 0:
    case 7:
        x += glyph.pWidths->left;
        break;
    case 1:
    case 2:
        x -= pFont->pRes->pGlyph->cellWidth;
        y += glyph.pWidths->left;
        break;
    case 3:
    case 4:
        x -= glyph.pWidths->left + glyph.pWidths->glyphWidth;
        y -= pFont->pRes->pGlyph->cellHeight;
        break;
    case 5:
    case 6:
        y -= glyph.pWidths->left + pFont->pRes->pGlyph->cellHeight;
        break;
    }

    pCC->vtable->pDrawGlyph(pCC, pFont, x, y, cl, &glyph);
    return glyph.pWidths->charWidth;
}

void NNS_G2dCharCanvasInitForBG(NNSG2dCharCanvas *pCC, void *charBase, int areaWidth, int areaHeight,
                                NNSG2dCharaColorMode colorMode) {
    InitCharCanvas(pCC, charBase, areaWidth, areaHeight, colorMode, &VTABLE_BG, areaWidth);
}

void NNS_G2dCharCanvasInitForOBJ1D(NNSG2dCharCanvas *pCC, void *charBase, int areaWidth, int areaHeight,
                                   NNSG2dCharaColorMode colorMode) {
    const ObjSizeShift *pSize = &sMaxObjSize[ObjSizeShiftOf(areaHeight)][ObjSizeShiftOf(areaWidth)];
    Obj1DParam param;

    param.objWidthShift = pSize->widthShift;
    param.objHeightShift = pSize->heightShift;
    InitCharCanvas(pCC, charBase, areaWidth, areaHeight, colorMode, &VTABLE_OBJ1D, param.param);
}

// Maps an area of a text BG's screen to the characters of a canvas, in order. A screen wider than 32 characters is
// made of 32 by 32 blocks
void NNS_G2dMapScrToCharText(void *scnBase, int areaWidth, int areaHeight, int areaLeft, int areaTop, int scnWidth,
                             int charNo, int cplt) {
    if (scnWidth <= 32) {
        MapScrToCharLinear((u16 *)scnBase + (scnWidth * areaTop + areaLeft), areaWidth, areaHeight, scnWidth, charNo,
                           cplt);
    } else {
        const int right = areaLeft + areaWidth;
        const int bottom = areaTop + areaHeight;
        const u16 palette = (u16)(cplt << 12);
        int x, y;

        for (y = areaTop; y < bottom; y++) {
            const int line = (y < 32) ? y : y + 32;
            u16 *pLine = (u16 *)scnBase + line * 32;

            for (x = areaLeft; x < right; x++) {
                const int pos = (x < 32) ? x : x + (32 * 32 - 32);

                pLine[pos] = (u16)(palette | charNo++);
            }
        }
    }
}

static void MapScrToCharLinear(u16 *pScr, int areaWidth, int areaHeight, int scnWidth, int charNo, int cplt) {
    const u16 palette = (u16)(cplt << 12);
    int x, y;

    for (y = 0; y < areaHeight; y++) {
        u16 *p = pScr;

        for (x = 0; x < areaWidth; x++) {
            *p++ = (u16)(palette | charNo++);
        }
        pScr += scnWidth;
    }
}

// The number of OBJs that NNS_G2dArrangeOBJ1D lays out for an area
int NNS_G2dCalcRequireOBJ1D(u32 areaWidth, u32 areaHeight) {
    const u32 numX8 = areaWidth / 8;
    const u32 numY8 = areaHeight / 8;
    const u32 numX4 = (areaWidth & 4) / 4;
    const u32 numY4 = (areaHeight & 4) / 4;
    const u32 numX21 = (areaWidth & 2) / 2 + (areaWidth & 1);
    const u32 numY21 = (areaHeight & 2) / 2 + (areaHeight & 1);
    int numObj = 0;

    numObj += numX8 * numY8;
    numObj += (numX4 + numX21 * 2) * numY8;
    numObj += (numY4 + numY21 * 2) * numX8;
    numObj += (numX21 + numX4) * (numY21 + numY4);
    return numObj;
}

// Lays out the OBJs that show a 1D OBJ canvas at x, y, and returns their number
int NNS_G2dArrangeOBJ1D(GXOamAttr *oam, int areaWidth, int areaHeight, int x, int y, int color, int charName,
                        int vramMode) {
    const ObjSizeShift *pSize = &sMaxObjSize[ObjSizeShiftOf(areaHeight)][ObjSizeShiftOf(areaWidth)];
    const int objWidthShift = pSize->widthShift;
    const int objHeightShift = pSize->heightShift;
    const u32 bodyWidth = areaWidth & (0xffffffff << objWidthShift);
    const u32 bodyHeight = areaHeight & (0xffffffff << objHeightShift);
    const int charUnit = (color == GX_OAM_COLORMODE_16) ? 1 : 2;
    int numObj = 0;
    const u32 shape = GetOamShape(pSize);
    const int numX = areaWidth >> objWidthShift;
    const int numY = areaHeight >> objHeightShift;
    const int charNameStep = ((charUnit << objWidthShift) << objHeightShift) >> vramMode;
    int i, j;

    for (j = 0; j < numY; j++) {
        const int objY = y + ((j << objHeightShift) * 8);

        for (i = 0; i < numX; i++) {
            const int objX = x + ((i << objWidthShift) * 8);

            // G2_SetOBJPosition with its terms the other way round, y before x
            oam->attr01 = (oam->attr01 & ~(GX_OAM_ATTR01_X_MASK | GX_OAM_ATTR01_Y_MASK)) | (objY & 0xff) |
                          ((objX & 0x1ff) << GX_OAM_ATTR01_X_SHIFT);
            G2_SetOBJShape(oam, shape);
            G2_SetOBJCharName(oam, charName);
            G2_SetOBJColorMode(oam, color);
            oam++;
            charName += charNameStep;
        }
    }
    numObj += numX * numY;

    if (bodyWidth < areaWidth) {
        const int restWidth = areaWidth - bodyWidth;
        const int n = NNS_G2dArrangeOBJ1D(oam, restWidth, bodyHeight, x + bodyWidth * 8, y, color, charName, vramMode);

        oam += n;
        numObj += n;
        charName += (charUnit * restWidth * bodyHeight) >> vramMode;
    }
    if (bodyHeight < areaHeight) {
        const int restHeight = areaHeight - bodyHeight;
        const int n = NNS_G2dArrangeOBJ1D(oam, bodyWidth, restHeight, x, y + bodyHeight * 8, color, charName, vramMode);

        oam += n;
        numObj += n;
        charName += (charUnit * bodyWidth * restHeight) >> vramMode;
    }
    if (bodyWidth < areaWidth && bodyHeight < areaHeight) {
        numObj += NNS_G2dArrangeOBJ1D(oam, areaWidth - bodyWidth, areaHeight - bodyHeight, x + bodyWidth * 8,
                                      y + bodyHeight * 8, color, charName, vramMode);
    }
    return numObj;
}
