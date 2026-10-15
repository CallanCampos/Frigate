#include "nnsys/g2d.h"

// NitroSystem's g2d_TextCanvas.c: drawing strings and text with a font into a character canvas, placed and aligned by
// flags, in any of the font's directions

void NNSi_G2dTextCanvasDrawString(const NNSG2dTextCanvas *pTxn, int x, int y, int cl, const void *str,
                                  const void **ppEnd, NNSG2dTextDirection dir) {
    const int hSpace = pTxn->hSpace;
    const NNSG2dFont *pFont = pTxn->pFont;
    const void *pos = str;
    NNSiG2dSplitCharCallback getNextChar = pFont->cbCharSpliter;
    u16 c;

    while ((c = getNextChar(&pos)) != 0) {
        int width;

        if (c == '\n') {
            break;
        }
        width = NNS_G2dCharCanvasDrawChar(pTxn->pCanvas, pFont, x, y, cl, c) + hSpace;
        x += width * dir.x;
        y += width * dir.y;
    }

    if (ppEnd != NULL) {
        *ppEnd = (c == '\n') ? pos : NULL;
    }
}

// Draws the lines of a text, each aligned in a box areaWidth wide
static void DrawTextAligned(const NNSG2dTextCanvas *pTxn, int x, int y, int areaWidth, int cl, u32 flags,
                            const void *txt, NNSG2dTextDirection dir) {
    int lineX, lineY;
    const int linefeed = NNS_G2dFontGetLineFeed(pTxn->pFont) + pTxn->vSpace;
    const void *pos = txt;
    const int lfx = linefeed * -dir.y;
    const int lfy = linefeed * dir.x;
    int line = 0;

    while (pos != NULL) {
        lineX = x + lfx * line;
        lineY = y + lfy * line;

        if (flags & NNS_G2D_HORIZONTALALIGN_RIGHT) {
            const int shift = areaWidth - NNSi_G2dFontGetStringWidth(pTxn->pFont, pTxn->hSpace, pos, NULL);

            lineX += shift * dir.x;
            lineY += shift * dir.y;
        } else if (flags & NNS_G2D_HORIZONTALALIGN_CENTER) {
            const int width = NNSi_G2dFontGetStringWidth(pTxn->pFont, pTxn->hSpace, pos, NULL);
            const int shift = (areaWidth + 1) / 2 - (width + 1) / 2;

            lineX += shift * dir.x;
            lineY += shift * dir.y;
        }

        NNSi_G2dTextCanvasDrawString(pTxn, lineX, lineY, cl, pos, &pos, dir);
        line++;
    }
}

void NNSi_G2dTextCanvasDrawText(const NNSG2dTextCanvas *pTxn, int x, int y, int cl, u32 flags, const void *txt,
                                NNSG2dTextDirection dir) {
    const NNSG2dTextRect rect = NNS_G2dFontGetTextRect(pTxn->pFont, pTxn->hSpace, pTxn->vSpace, txt);

    if (flags & NNS_G2D_HORIZONTALORIGIN_CENTER) {
        const int shift = -(rect.width + 1) / 2;

        x += shift * dir.x;
        y += shift * dir.y;
    } else if (flags & NNS_G2D_HORIZONTALORIGIN_RIGHT) {
        const int shift = -rect.width;

        x += shift * dir.x;
        y += shift * dir.y;
    }

    if (flags & NNS_G2D_VERTICALORIGIN_MIDDLE) {
        const int shift = -(rect.height + 1) / 2;

        x += shift * -dir.y;
        y += shift * dir.x;
    } else if (flags & NNS_G2D_VERTICALORIGIN_BOTTOM) {
        const int shift = -rect.height;

        x += shift * -dir.y;
        y += shift * dir.x;
    }

    DrawTextAligned(pTxn, x, y, rect.width, cl, flags, txt, dir);
}

void NNSi_G2dTextCanvasDrawTextRect(const NNSG2dTextCanvas *pTxn, int x, int y, int w, int h, int cl, u32 flags,
                                    const void *txt, NNSG2dTextDirection dir) {
    if (flags & NNS_G2D_VERTICALALIGN_BOTTOM) {
        const int shift = h - NNSi_G2dFontGetTextHeight(pTxn->pFont, pTxn->vSpace, txt);

        x += shift * -dir.y;
        y += shift * dir.x;
    } else if (flags & NNS_G2D_VERTICALALIGN_MIDDLE) {
        const int height = NNSi_G2dFontGetTextHeight(pTxn->pFont, pTxn->vSpace, txt);
        const int shift = (h + 1) / 2 - (height + 1) / 2;

        x += shift * -dir.y;
        y += shift * dir.x;
    }

    DrawTextAligned(pTxn, x, y, w, cl, flags, txt, dir);
}
