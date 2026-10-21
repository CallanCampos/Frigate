#include "dwc_util_text.h"
#include "dwc_util_cell.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_oam.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_shared.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/os.h"

typedef struct DWCUtilTextWork {
    NNSG2dFont fonts[2];
    // The OBJ canvases
    u8 objTexts[0x20][sizeof(DWCUtilText)];
    DWCUtilText bgTexts[2];
    DWCUtilStack *freeObjTexts;
    void *fontFiles[2];
    // Whether each screen's BG canvas waits for its upload
    u8 dirty[2];
} DWCUtilTextWork;

typedef struct DWCUtilTextBgSize {
    u16 width;
    u16 height;
} DWCUtilTextBgSize;

// The fonts of the utility's languages but 6, and of language 6
static const char *sFontPaths[2] = { "msg/lc_m.NFTR.l", "msg/lc_s.NFTR.l" };
static const char *sKcFontPaths[2] = { "msg/kc_m.NFTR.l", "msg/lc_s.NFTR.l" };

static const DWCUtilTextBgSize sBgSizes[2] = { { 32, 24 }, { 32, 12 } };
static const u16 sBgCharNames[2] = { 0, 0x180 };

static DWCUtilTextWork *sTextWork;

// The direction of the font's text, from how its glyphs are rotated
static inline NNSG2dTextDirection DWCUtilText_GetDirection(const NNSG2dFont *font) {
    NNSG2dTextDirection dir = { 0, 0 };

    switch (font->pRes->pGlyph->flags) {
    case 0:
    case 7:
        dir.x = 1;
        break;
    case 1:
    case 2:
        dir.y = 1;
        break;
    case 3:
    case 4:
        dir.x = -1;
        break;
    case 5:
    case 6:
        dir.y = -1;
        break;
    }
    return dir;
}

void DWCUtilText_Init(void) {
    int i;

    sTextWork = DWCUtilHeap_AllocClear(sizeof(DWCUtilTextWork), 4);
    sTextWork->freeObjTexts = DWCUtilStack_CreateFilled(0x20, sTextWork->objTexts[0], sizeof(DWCUtilText));
    if (DWCUtilMain_GetLanguage() == 6) {
        for (i = 0; i < 2; i++) {
            sTextWork->fontFiles[i] = DWCUtilRomResource_LoadFile(sKcFontPaths[i], NULL, 4);
            NNS_G2dFontInitUTF16(&sTextWork->fonts[i], sTextWork->fontFiles[i]);
        }
    } else {
        for (i = 0; i < 2; i++) {
            sTextWork->fontFiles[i] = DWCUtilRomResource_LoadFile(sFontPaths[i], NULL, 4);
            NNS_G2dFontInitUTF16(&sTextWork->fonts[i], sTextWork->fontFiles[i]);
        }
    }
}

void DWCUtilText_Exit(void) {
    int i;

    for (i = 0; i < 2; i++) {
        DWCUtilRomResource_FreeFile(sTextWork->fontFiles[i]);
    }
    DWCUtilStack_Destroy(sTextWork->freeObjTexts);
    DWCUtilHeap_FreeAndNull(&sTextWork);
}

DWCUtilText *DWCUtilText_CreateObj(int screen, int width, int height, s16 fromFront, int *objCount, int font) {
    u32 charName;
    DWCUtilText *text = DWCUtilStack_Pop(sTextWork->freeObjTexts);

    text->u.obj.width = width;
    text->u.obj.height = height;
    text->buffer = DWCUtilVramAlloc_Alloc(screen, width * height, fromFront, &charName);
    text->u.obj.charName = charName;
    *objCount = NNS_G2dCalcRequireOBJ1D(width, height);
    NNS_G2dCharCanvasInitForOBJ1D(&text->canvas,
                                  (void *)((screen == 1 ? HW_DB_OBJ_VRAM : HW_OBJ_VRAM) + charName * 0x80), width,
                                  height, NNS_G2D_CHARA_COLORMODE_16);
    text->canvas.vtable->pClear(&text->canvas, 0);
    NNS_G2dTextCanvasInit(&text->text, &text->canvas, &sTextWork->fonts[font], 1, 1);
    return text;
}

void DWCUtilText_DestroyObj(DWCUtilText *text) {
    DWCUtilVramAlloc_Free(text->buffer);
    DWCUtilStack_Push(sTextWork->freeObjTexts, text);
}

DWCUtilText *DWCUtilText_CreateBg(int screen, int font) {
    DWCUtilText *text = &sTextWork->bgTexts[screen];
    u16 height = sBgSizes[screen].height;
    u16 width = sBgSizes[screen].width;

    text->buffer = DWCUtilHeap_AllocClear(width * height * 0x20, 0x20);
    if (screen == 1) {
        G2S_BG0Mosaic(FALSE);
        G2S_SetBG0Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x6000, GX_BG_CHARBASE_0x00000,
                          GX_BG_EXTPLTT_01);
    } else {
        G2_BG0Mosaic(FALSE);
        G2_SetBG0Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x6000, GX_BG_CHARBASE_0x00000,
                         GX_BG_EXTPLTT_01);
    }
    NNS_G2dCharCanvasInitForBG(&text->canvas, text->buffer, width, height, NNS_G2D_CHARA_COLORMODE_16);
    NNS_G2dTextCanvasInit(&text->text, &text->canvas, &sTextWork->fonts[font], 1, 1);
    NNS_G2dMapScrToCharText(screen == 1 ? G2S_GetBG0ScrPtr() : G2_GetBG0ScrPtr(), width, height, 0, 0, 32,
                            sBgCharNames[screen], 15);
    DWCUtilText_Clear(text, 0);
    text->u.task = DWCUtilTask_Create(1, DWCUtilText_UploadTask, &sTextWork->dirty[screen], 200);
    return text;
}

void DWCUtilText_UploadTask(DWCUtilTask *task, void *arg) {
    u8 *dirty = arg;

    if (*dirty) {
        if (dirty == &sTextWork->dirty[0]) {
            DC_FlushRange(sTextWork->bgTexts[0].buffer, 0x6000);
            GX_LoadBG0Char(sTextWork->bgTexts[0].buffer, 0, 0x6000);
        } else {
            DC_FlushRange(sTextWork->bgTexts[1].buffer, 0x3000);
            GXS_LoadBG0Char(sTextWork->bgTexts[1].buffer, 0x3000, 0x3000);
        }
        *dirty = FALSE;
    }
}

void DWCUtilText_DestroyBg(DWCUtilText *text) {
    DWCUtilTask_Remove(1, text->u.task);
    if (text == &sTextWork->bgTexts[0]) {
        MI_CpuClear16(G2_GetBG0CharPtr(), 0x6000);
    } else {
        // BUG: This clears the main screen's characters, not the sub screen's, which the canvas has from 0x3000
#ifdef BUGFIX
        MI_CpuClear16((u8 *)G2S_GetBG0CharPtr() + 0x3000, 0x3000);
#else
        MI_CpuClear16(G2_GetBG0CharPtr(), 0x3000);
#endif
    }
    DWCUtilHeap_FreeAndNull(&text->buffer);
}

void DWCUtilText_DestroyScreenBg(int screen) {
    DWCUtilText_DestroyBg(&sTextWork->bgTexts[screen]);
}

void DWCUtilText_Draw(DWCUtilText *text, int x, int y, int cl, u32 flags, const void *str) {
    NNSi_G2dTextCanvasDrawText(&text->text, x, y, cl, flags, str, DWCUtilText_GetDirection(text->text.pFont));
}

void DWCUtilText_DrawChar(DWCUtilText *text, int x, int y, int cl, u16 ch, int font) {
    NNS_G2dCharCanvasDrawChar(&text->canvas, &sTextWork->fonts[font], x, y, cl, ch);
}

void DWCUtilText_DrawCentered(DWCUtilText *text, int x, int y, int cl, int cellWidth, const u16 *str, int font) {
    while (*str != 0) {
        const NNSG2dCharWidths *widths = NNS_G2dFontGetCharWidths(&sTextWork->fonts[font], *str);

        DWCUtilText_DrawChar(text, x + ((cellWidth - widths->charWidth) >> 1), y, cl, *str, font);
        str++;
        x += cellWidth;
    }
}

void DWCUtilText_DrawRect(DWCUtilText *text, int x, int y, int w, int h, int cl, u32 flags, const void *str) {
    NNSi_G2dTextCanvasDrawTextRect(&text->text, x, y, w, h, cl, flags, str, DWCUtilText_GetDirection(text->text.pFont));
}

void DWCUtilText_Clear(DWCUtilText *text, int cl) {
    text->canvas.vtable->pClear(&text->canvas, cl);
}

void DWCUtilText_ShowObj(DWCUtilText *text, int x, int y, DWCUtilOamNode *node, int priority) {
    GXOamAttr *oam = DWCUtilOam_GetEntry(node, 0);

    DWCUtilOam_SetEffect(node, -1, 0, 0);
    DWCUtilOam_SetMode(node, -1, 0, 15);
    DWCUtilOam_SetPriority(node, -1, priority);
    NNS_G2dArrangeOBJ1D(oam, text->u.obj.width, text->u.obj.height, x, y, 0, text->u.obj.charName, 2);
}

void DWCUtilText_MarkDirty(DWCUtilText *text) {
    if (text == &sTextWork->bgTexts[0]) {
        sTextWork->dirty[0] = TRUE;
    } else {
        sTextWork->dirty[1] = TRUE;
    }
}
