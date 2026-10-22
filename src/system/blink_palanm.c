#include "system/blink_palanm.h"
#include "types.h"
#include "gfl/arc_util.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"

// A palette animation for cursors, fading or blinking part of a palette between two sets of colors. Our names

#define BLINK_PALANM_MODE_FADE 0
#define BLINK_PALANM_MODE_BLINK 1

// How far the fade goes around its cycle each frame, so that a cycle takes 64 frames
#define FADE_SPEED 0x400
// The frames each color of a blink shows, and the frames of the whole blinking
#define BLINK_STEP_FRAMES 5
#define BLINK_FRAMES (BLINK_STEP_FRAMES * 4)

// The last BG frame of the main engine
#define BG_FRAME_MAIN_LAST 3

struct BlinkPalAnm {
    u16 *startColors;
    u16 *endColors;
    // The colors sent to VRAM
    u16 *colors;
    u16 offset;
    u16 count;
    // The point of the fade's cycle, or the frames since the blinking started
    u16 animeCount;
    u16 mode;
    u32 transferDest;
    HeapID heapId;
};

BlinkPalAnm *BlinkPalAnm_Create(u16 offset, u16 count, u16 palette, HeapID heapId) {
    BlinkPalAnm *anm = GFL_HeapAllocate(heapId, sizeof(BlinkPalAnm), FALSE, "blink_palanm.c", 62);

    anm->colors = GFL_HeapAllocate(heapId, count * sizeof(u16), FALSE, "blink_palanm.c", 63);
    anm->offset = offset;
    anm->count = count;
    anm->animeCount = 0;
    anm->mode = BLINK_PALANM_MODE_FADE;
    anm->heapId = heapId;
    if (palette == BLINK_PALANM_OBJ_MAIN) {
        anm->transferDest = NNS_GFD_DST_2D_OBJ_PLTT_MAIN;
    } else if (palette == BLINK_PALANM_OBJ_SUB) {
        anm->transferDest = NNS_GFD_DST_2D_OBJ_PLTT_SUB;
    } else if (palette <= BG_FRAME_MAIN_LAST) {
        anm->transferDest = NNS_GFD_DST_2D_BG_PLTT_MAIN;
    } else {
        anm->transferDest = NNS_GFD_DST_2D_BG_PLTT_SUB;
    }
    return anm;
}

void BlinkPalAnm_SetPalBufferArc(BlinkPalAnm *anm, u32 arcId, u32 fileId, u32 startPos, u32 endPos) {
    NNSG2dPaletteData *palette;
    void *file;
    u16 *colors;

    anm->startColors = GFL_HeapAllocate(anm->heapId, anm->count * sizeof(u16), FALSE, "blink_palanm.c", 102);
    anm->endColors = GFL_HeapAllocate(anm->heapId, anm->count * sizeof(u16), FALSE, "blink_palanm.c", 103);
    file = GFL_G2DIOReadNCLR(arcId, fileId, &palette, anm->heapId);
    colors = palette->pRawData;
    MI_CpuCopy8(&colors[startPos], anm->startColors, anm->count * sizeof(u16));
    MI_CpuCopy8(&colors[endPos], anm->endColors, anm->count * sizeof(u16));
    GFL_HeapFree(file);
}

void BlinkPalAnm_SetPalBufferArcTool(BlinkPalAnm *anm, ArcTool *arc, u32 fileId, u32 startPos, u32 endPos) {
    NNSG2dPaletteData *palette;
    void *file;
    u16 *colors;

    anm->startColors = GFL_HeapAllocate(anm->heapId, anm->count * sizeof(u16), FALSE, "blink_palanm.c", 133);
    anm->endColors = GFL_HeapAllocate(anm->heapId, anm->count * sizeof(u16), FALSE, "blink_palanm.c", 134);
    file = GFL_G2DIOReadNCLRArc(arc, fileId, &palette, anm->heapId);
    colors = palette->pRawData;
    MI_CpuCopy8(&colors[startPos], anm->startColors, anm->count * sizeof(u16));
    MI_CpuCopy8(&colors[endPos], anm->endColors, anm->count * sizeof(u16));
    GFL_HeapFree(file);
}

void BlinkPalAnm_Free(BlinkPalAnm *anm) {
    GFL_HeapFree(anm->endColors);
    GFL_HeapFree(anm->startColors);
    GFL_HeapFree(anm->colors);
    GFL_HeapFree(anm);
}

void BlinkPalAnm_Main(BlinkPalAnm *anm) {
    // Whether each step of the blinking shows the start colors
    BOOL blinkStart[] = { FALSE, TRUE, FALSE, TRUE, FALSE, TRUE };
    fx32 ratio;
    u32 i;

    switch (anm->mode) {
    case BLINK_PALANM_MODE_FADE:
        if (anm->animeCount + FADE_SPEED >= 0x10000) {
            anm->animeCount = anm->animeCount + FADE_SPEED - 0x10000;
        } else {
            anm->animeCount += FADE_SPEED;
        }
        // From the end colors to the start colors and back
        ratio = (FX_CosIdx(anm->animeCount) + FX32_ONE) / 2;
        for (i = 0; i < anm->count; i++) {
            u16 start = anm->startColors[i];
            u16 end = anm->endColors[i];
            u8 r = (start & 0x1f) + ((((end & 0x1f) - (start & 0x1f)) * ratio) >> FX32_SHIFT);
            u8 g = ((start >> 5) & 0x1f) + (((((end >> 5) & 0x1f) - ((start >> 5) & 0x1f)) * ratio) >> FX32_SHIFT);
            u8 b = ((start >> 10) & 0x1f) + (((((end >> 10) & 0x1f) - ((start >> 10) & 0x1f)) * ratio) >> FX32_SHIFT);

            anm->colors[i] = GX_RGB(r, g, b);
        }
        break;
    case BLINK_PALANM_MODE_BLINK:
        MI_CpuCopy8(blinkStart[anm->animeCount / BLINK_STEP_FRAMES] ? anm->startColors : anm->endColors, anm->colors,
                    anm->count * sizeof(u16));
        if (anm->animeCount < BLINK_FRAMES) {
            anm->animeCount++;
        }
        break;
    }
    NNS_GfdRegisterNewVramTransferTask(anm->transferDest, anm->offset * sizeof(u16), anm->colors,
                                       anm->count * sizeof(u16));
}

void BlinkPalAnm_InitAnime(BlinkPalAnm *anm) {
    anm->animeCount = 0;
    anm->mode = BLINK_PALANM_MODE_FADE;
}

void BlinkPalAnm_SetAnimeCount(BlinkPalAnm *anm, u16 count) {
    anm->animeCount = count;
    anm->mode = BLINK_PALANM_MODE_FADE;
}

void BlinkPalAnm_StartBlink(BlinkPalAnm *anm) {
    anm->animeCount = 0;
    anm->mode = BLINK_PALANM_MODE_BLINK;
}

BOOL BlinkPalAnm_IsBlinkEnd(BlinkPalAnm *anm) {
    if (anm->mode >= BLINK_PALANM_MODE_BLINK && anm->animeCount == BLINK_FRAMES) {
        return TRUE;
    }
    return FALSE;
}
