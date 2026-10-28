#include "g2di_BitReader.h"
#include "nitro/gx.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_CharCanvas.c: drawing glyphs into, and clearing, a canvas of characters laid out as a BG's or as
// the OBJs of a 1D OBJ area, and laying out the screen or the OBJs that show a canvas. A 1D OBJ canvas is covered with
// the largest OBJs that fit, then the rest of it to the right, below and at the bottom right likewise. The game keeps
// the 2D OBJ canvas's functions table but not the function that uses it. The names are NitroSystem's, as ntrtwl's
// reconstruction of NitroSystem has them, except ObjSizeShiftOf, sMaxObjSize and sOamShape, which are ours: the two
// tables are statics in NitroSystem's functions

// The log2 sizes in characters of an OBJ
typedef struct {
    u8 widthShift;
    u8 heightShift;
} ObjectSize;

// A 1D OBJ canvas's param: the size of its largest OBJ
typedef union {
    u32 packed;
    struct {
        u32 objWidthShift : 8;
        u32 objHeightShift : 8;
        u32 : 16;
    };
} OBJ1DParam;

// A glyph drawn into one character: x and y place the glyph's top left in the character, and may be negative
typedef struct {
    u32 *dst;
    const u8 *src;
    int ofs_x;
    int ofs_y;
    int width;
    int height;
    int dsrc;
    int srcBpp;
    int dstBpp;
    int cl;
} LC_INFO;

static void DrawGlyphLine(const NNSG2dCharCanvas *pCC, const NNSG2dFont *pFont, int x, int y, int cl,
                          const NNSG2dGlyph *pGlyph);
static void DrawGlyph1D(const NNSG2dCharCanvas *pCC, const NNSG2dFont *pFont, int x, int y, int cl,
                        const NNSG2dGlyph *pGlyph);
static void ClearContinuous(const NNSG2dCharCanvas *pCC, int cl);
static void ClearLine(const NNSG2dCharCanvas *pCC, int cl);
static void ClearAreaLine(const NNSG2dCharCanvas *pCC, int cl, int x, int y, int w, int h);
static void ClearArea1D(const NNSG2dCharCanvas *pCC, int cl, int x, int y, int w, int h);
static void NNS_G2dMapScrToChar256x16Pltt(u16 *pScr, int areaWidth, int areaHeight, int scnWidth, int charNo, int cplt);

// The 2D OBJ canvas's functions. NitroSystem's table is static, but this one is not, since nothing the game keeps uses
// it and MWCC would leave it out
// MWCC emits these in this order: the 2D OBJ's table, the BG's, the 1D OBJ's, which is the ROM's
static const NNSiG2dCharCanvasVTable VTABLE_BG = { DrawGlyphLine, ClearContinuous, ClearAreaLine };
static const NNSiG2dCharCanvasVTable VTABLE_OBJ1D = { DrawGlyph1D, ClearContinuous, ClearArea1D };
const NNSiG2dCharCanvasVTable VTABLE_OBJ2DRECT = { DrawGlyphLine, ClearLine, ClearAreaLine };

// The largest OBJ that fits in an area, by the log2 sizes of the area, at most 8 characters: [height][width]
static const ObjectSize sMaxObjSize[4][4] = {
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

static inline int GetCharacterSize(int bpp) {
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
        const u32 fullbits = (u32)(~0);
        const u32 maskW = fullbits << objWidthShift;
        const u32 maskH = fullbits << objHeightShift;
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
            const ObjectSize *pSize = &sMaxObjSize[ObjSizeShiftOf(areaHeight)][ObjSizeShiftOf(areaWidth)];

            objWidthShift = pSize->widthShift;
            objHeightShift = pSize->heightShift;
        }
    }
}

static u32 OBJSizeToShape(const ObjectSize *pSize) {
    return sOamShape[pSize->heightShift][pSize->widthShift];
}

// Fills the w by h pixels at x, y of a character
static void ClearChar(u32 *pChar, int x, int y, int w, int h, u32 cl, int bpp) {
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
        const u32 fullbits = (u32)(~0);
        const u32 left = x * 8;
        const u32 right = 64 - (left + w * 8);
        u32 mask0 = fullbits >> left;
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
        mask1 = fullbits << right;
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

static void LetterChar(const LC_INFO *p) {
    u32 sx = (p->ofs_x >= 0) ? p->ofs_x : 0;
    const int sy = (p->ofs_y >= 0) ? p->ofs_y : 0;
    u32 ex = (p->ofs_x + p->width < 8) ? p->ofs_x + p->width : 8;
    const int ey = (p->ofs_y + p->height < 8) ? p->ofs_y + p->height : 8;
    const int offsetY = -MATH_MIN(p->ofs_y, 0);
    const int offsetX = -MATH_MIN(p->ofs_x, 0);
    const u8 *pSrc;
    int srcOffset;
    int srcLineBits;
    int srcBpp;
    int dstBpp;

    srcLineBits = p->dsrc;
    srcBpp = p->srcBpp;
    srcOffset = offsetY * srcLineBits + srcBpp * offsetX;
    pSrc = p->src;
    dstBpp = p->dstBpp;

    // From pixels to bit positions in a line of the character
    sx *= dstBpp;
    ex *= dstBpp;

    if (dstBpp == 4) {
        u32 *pLine = p->dst + sy;
        u32 *pEnd = p->dst + ey;
        const u32 colorOffset = p->cl;

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
        u32 *pLine = p->dst + sy * 2;
        u32 *pEnd = p->dst + ey * 2;
        const u32 colorOffset = p->cl;

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
static void DrawGlyphLine(const NNSG2dCharCanvas *pCC, const NNSG2dFont *pFont, int x, int y, int cl,
                          const NNSG2dGlyph *pGlyph) {
    int cx;
    int cy;
    int xBase;
    int yStart;
    int xEnd;
    int yEnd;
    int lineSkip;
    u8 *pChar;
    u8 glyphWidth;
    u8 glyphHeight;
    const int charSize = GetCharacterSize(pCC->dstBpp);
    LC_INFO param;

    {
        int numX;
        int numY;
        const u32 areaWidth = pCC->areaWidth;
        const u32 areaHeight = pCC->areaHeight;
        u8 *const charBase = pCC->charBase;
        u32 left, top, right, bottom;

        glyphWidth = pGlyph->pWidths->glyphWidth;
        glyphHeight = pFont->pRes->pGlyph->cellHeight;

        if (glyphWidth <= 0) {
            return;
        }
        if (x + glyphWidth < 0 || y + glyphHeight < 0) {
            return;
        }

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

        pChar = charBase + (pCC->param * top + left) * charSize;
        lineSkip = (pCC->param - numX) * charSize;
        xBase = (x < 0) ? x : x & 7;
        yStart = (y < 0) ? y : y & 7;
        xEnd = xBase - 8 * numX;
        yEnd = yStart - 8 * numY;
    }

    param.src = pGlyph->image;
    param.width = glyphWidth;
    param.height = glyphHeight;
    param.cl = cl - 1;
    param.srcBpp = pFont->pRes->pGlyph->bpp;
    param.dstBpp = pCC->dstBpp;
    param.dsrc = param.srcBpp * pFont->pRes->pGlyph->cellWidth;

    for (cy = yStart; cy > yEnd; cy -= 8) {
        param.ofs_y = cy;
        for (cx = xBase; cx > xEnd; cx -= 8) {
            param.dst = (u32 *)pChar;
            param.ofs_x = cx;
            LetterChar(&param);
            pChar += charSize;
        }
        pChar += lineSkip;
    }
}

static void DrawGlyph1D(const NNSG2dCharCanvas *pCC, const NNSG2dFont *pFont, int x, int y, int cl,
                        const NNSG2dGlyph *pGlyph) {
    const int charSize = GetCharacterSize(pCC->dstBpp);
    const u8 glyphWidth = pGlyph->pWidths->glyphWidth;
    const int glyphHeight = pFont->pRes->pGlyph->cellHeight;
    const int areaW = pCC->areaWidth;
    const int areaH = pCC->areaHeight;
    u32 left, top, right, bottom;
    int numX, numY;
    int xEnd, yEnd;
    int cy;
    u8 *charBase;
    OBJ1DParam objParam;
    LC_INFO param;

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

    param.src = pGlyph->image;
    param.height = glyphHeight;
    param.width = glyphWidth;
    param.cl = cl - 1;
    param.srcBpp = pFont->pRes->pGlyph->bpp;
    param.dstBpp = pCC->dstBpp;
    param.dsrc = param.srcBpp * pFont->pRes->pGlyph->cellWidth;

    objParam.packed = pCC->param;
    const int areaWidth = pCC->areaWidth;
    const int areaHeight = pCC->areaHeight;
    const int objWidthShift = objParam.objWidthShift;
    const int objHeightShift = objParam.objHeightShift;
    for (cy = top; y > yEnd; cy++, y -= 8) {
        int px, cx;

        param.ofs_y = y;
        for (px = x, cx = left; px > xEnd; px -= 8, cx++) {
            const u32 charNo = GetCharIndex1D(cx, cy, areaWidth, areaHeight, objWidthShift, objHeightShift);

            param.ofs_x = px;
            param.dst = (u32 *)(charBase + charNo * charSize);
            LetterChar(&param);
        }
    }
}

static inline u32 SpreadColor32(u32 cl, int bpp) {
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
    const u32 pattern = SpreadColor32(cl, pCC->dstBpp);

    MI_CpuFillFast(pCC->charBase, pattern, GetCharacterSize(pCC->dstBpp) * (pCC->areaWidth * pCC->areaHeight));
}

static void ClearLine(const NNSG2dCharCanvas *pCC, int cl) {
    const u32 pattern = SpreadColor32(cl, pCC->dstBpp);
    const int charSize = GetCharacterSize(pCC->dstBpp);
    const int lineSize = charSize * pCC->param;
    const int areaSize = charSize * pCC->areaWidth;
    int i;
    u8 *p = pCC->charBase;

    for (i = 0; i < pCC->areaHeight; i++) {
        MI_CpuFillFast(p, pattern, areaSize);
        p += lineSize;
    }
}

static void ClearAreaLine(const NNSG2dCharCanvas *pCC, int cl, int x, int y, int w, int h) {
    int cy;
    int height;
    const int right = x + w;
    const int bottom = y + h;
    const int bpp = pCC->dstBpp;
    const u32 pattern = SpreadColor32(cl, bpp);
    const int x0 = x & ~7;
    cy = y & ~7;
    const int x1 = (right + 7) & ~7;
    const int y1 = (bottom + 7) & ~7;
    const int charSize = GetCharacterSize(bpp);
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

            ClearChar((u32 *)p, left, top, width, height, pattern, bpp);
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
    const u32 pattern = SpreadColor32(cl, bpp);
    OBJ1DParam objParam;

    objParam.packed = pCC->param;
    const int x0 = x & ~7;
    cy = y & ~7;
    const int x1 = (right + 7) & ~7;
    const int y1 = (bottom + 7) & ~7;
    const int areaWidth = pCC->areaWidth;
    const int areaHeight = pCC->areaHeight;
    const int objWidthShift = objParam.objWidthShift;
    const int objHeightShift = objParam.objHeightShift;
    const int charSize = GetCharacterSize(bpp);
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

            ClearChar((u32 *)(charBase + charNo * charSize), left, top, width, height, pattern, bpp);
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
    const ObjectSize *pSize = &sMaxObjSize[ObjSizeShiftOf(areaHeight)][ObjSizeShiftOf(areaWidth)];
    OBJ1DParam param;

    param.objWidthShift = pSize->widthShift;
    param.objHeightShift = pSize->heightShift;
    InitCharCanvas(pCC, charBase, areaWidth, areaHeight, colorMode, &VTABLE_OBJ1D, param.packed);
}

// The 2D OBJ canvas init, which the original link dropped though its functions table stays in the ROM; the game
// keeps the table because this function references it. Reconstructed from ntrtwl's NitroSystem
void NNS_G2dCharCanvasInitForOBJ2DRect(NNSG2dCharCanvas *pCC, void *charBase, int areaWidth, int areaHeight,
                                       NNSG2dCharaColorMode colorMode) {
    int lineWidth = (colorMode == NNS_G2D_CHARA_COLORMODE_16) ? 32 : 16;

    InitCharCanvas(pCC, charBase, areaWidth, areaHeight, colorMode, &VTABLE_OBJ2DRECT, lineWidth);
}

// Maps an area of a text BG's screen to the characters of a canvas, in order. A screen wider than 32 characters is
// made of 32 by 32 blocks
void NNS_G2dMapScrToCharText(void *scnBase, int areaWidth, int areaHeight, int areaLeft, int areaTop,
                             NNSG2dTextBGWidth scnWidth, int charNo, int cplt) {
    if (scnWidth <= 32) {
        NNS_G2dMapScrToChar256x16Pltt((u16 *)scnBase + (scnWidth * areaTop + areaLeft), areaWidth, areaHeight, scnWidth,
                                      charNo, cplt);
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

static void NNS_G2dMapScrToChar256x16Pltt(u16 *pScr, int areaWidth, int areaHeight, int scnWidth, int charNo,
                                          int cplt) {
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
int NNSi_G2dCalcRequiredOBJ(u32 areaWidth, u32 areaHeight) {
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
int NNS_G2dArrangeOBJ1D(GXOamAttr *oam, int areaWidth, int areaHeight, int x, int y, GXOamColorMode color, int charName,
                        NNSG2dOBJVramMode vramMode) {
    const ObjectSize *pSize = &sMaxObjSize[ObjSizeShiftOf(areaHeight)][ObjSizeShiftOf(areaWidth)];
    const int objWidthShift = pSize->widthShift;
    const int objHeightShift = pSize->heightShift;
    const u32 fullbits = (u32)(~0);
    const u32 bodyWidth = areaWidth & (fullbits << objWidthShift);
    const u32 bodyHeight = areaHeight & (fullbits << objHeightShift);
    const int charUnit = (color == GX_OAM_COLORMODE_16) ? 1 : 2;
    int numObj = 0;
    const u32 shape = OBJSizeToShape(pSize);
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
