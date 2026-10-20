#include "types.h"
#include "app/zukan_detail.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/pokedex.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/palanm.h"

// What the Pokédex detail screen's pages share: the screen's systems and the list of Pokémon it pages through, the
// blending and palette fades of the pages, and the backgrounds that the pages load

static void ZukanDetailBlend_Apply(u32 screen, ZukanDetailBlend *blend);
static void ZukanDetailPalFade_Start(ZukanDetailPalFade *fade, s8 wait, u8 start, u8 end);

// The pages' backgrounds, in the Pokédex's graphics archive, for the main and sub screens
typedef struct {
    u32 nclr;
    u32 ncgr;
    u32 nscr;
} BackgroundFiles;

// The palettes of the backgrounds' files that their bottom three rows and the rest of them take, by screen and page
static const u8 sBottomPalettes[2][4] = {
    { 0, 3, 2, 4 },
    { 1, 1, 1, 1 },
};

static const u8 sTopPalettes[2][4] = {
    { 0, 3, 2, 4 },
    { 0, 3, 2, 4 },
};

static const BackgroundFiles sBackgroundFiles[2] = {
    { 1, 11, 42 },
    { 2, 12, 37 },
};

ZukanDetailCommon *ZukanDetailCommon_Create(HeapID heapId, GameData *gameData, ZukanDetailGraphic *graphic, Font *font,
                                            PrintQueue *printQueue, ZukanDetailTouchbar *touchbar,
                                            ZukanDetailHeadbar *headbar, const u16 *list, u16 count, u16 *index) {
    ZukanDetailCommon *common = GFL_HeapAllocate(heapId, sizeof(ZukanDetailCommon), TRUE, "zukan_detail_common.c", 96);
    common->gameData = gameData;
    common->graphic = graphic;
    common->font = font;
    common->printQueue = printQueue;
    common->touchbar = touchbar;
    common->headbar = headbar;
    common->list = list;
    common->count = count;
    common->index = index;
    return common;
}

void ZukanDetailCommon_Free(ZukanDetailCommon *common) {
    GFL_HeapFree(common);
}

GameData *ZukanDetailCommon_GetGameData(ZukanDetailCommon *common) {
    return common->gameData;
}

ZukanDetailGraphic *ZukanDetailCommon_GetGraphic(ZukanDetailCommon *common) {
    return common->graphic;
}

Font *ZukanDetailCommon_GetFont(ZukanDetailCommon *common) {
    return common->font;
}

PrintQueue *ZukanDetailCommon_GetPrintQueue(ZukanDetailCommon *common) {
    return common->printQueue;
}

ZukanDetailTouchbar *ZukanDetailCommon_GetTouchbar(ZukanDetailCommon *common) {
    return common->touchbar;
}

ZukanDetailHeadbar *ZukanDetailCommon_GetHeadbar(ZukanDetailCommon *common) {
    return common->headbar;
}

u16 ZukanDetailCommon_GetCount(ZukanDetailCommon *common) {
    return common->count;
}

u16 ZukanDetailCommon_GetSpecies(ZukanDetailCommon *common) {
    return common->list[*common->index];
}

// Moves to the next Pokémon of the list that has been seen, wrapping around
void ZukanDetailCommon_GoNext(ZukanDetailCommon *common) {
    u16 start = *common->index;
    PokeDexSave *pokedex = GameData_GetPokedex(common->gameData);

    do {
        (*common->index)++;
        if (*common->index >= common->count) {
            *common->index = 0;
        }
    } while (*common->index != start && !PokeDex_IsSeen(pokedex, common->list[*common->index]));
}

void ZukanDetailCommon_GoPrev(ZukanDetailCommon *common) {
    u16 start = *common->index;
    PokeDexSave *pokedex = GameData_GetPokedex(common->gameData);

    do {
        if (*common->index == 0) {
            *common->index = common->count - 1;
        } else {
            (*common->index)--;
        }
    } while (*common->index != start && !PokeDex_IsSeen(pokedex, common->list[*common->index]));
}

ZukanDetailBlend *ZukanDetailBlend_Create(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(ZukanDetailBlend), TRUE, "zukan_detail_common.c", 275);
}

void ZukanDetailBlend_Free(ZukanDetailBlend *blend) {
    GFL_HeapFree(blend);
}

void ZukanDetailBlend_Update(ZukanDetailBlend *main, ZukanDetailBlend *sub) {
    ZukanDetailBlend *blends[2];
    u8 screen;

    blends[0] = main;
    blends[1] = sub;
    for (screen = 0; screen < 2; screen++) {
        ZukanDetailBlend *blend = blends[screen];
        if (blend->step != 0) {
            if (blend->wait == 0) {
                blend->wait = 0;
                blend->ev += blend->step;
                if (blend->step > 0) {
                    if (blend->ev >= 16) {
                        blend->ev = 16;
                        blend->step = 0;
                    }
                } else {
                    if (blend->ev <= 0) {
                        blend->ev = 0;
                        blend->step = 0;
                    }
                }
                ZukanDetailBlend_Apply(screen, blend);
            } else {
                blend->wait--;
            }
        }
    }
}

BOOL ZukanDetailBlend_IsActive(ZukanDetailBlend *blend) {
    if (blend->step != 0) {
        return TRUE;
    }
    return FALSE;
}

void ZukanDetailBlend_StartIn(ZukanDetailBlend *blend) {
    blend->ev = 0;
    blend->step = 2;
    blend->wait = 0;
}

void ZukanDetailBlend_StartOut(ZukanDetailBlend *blend) {
    blend->ev = 16;
    blend->step = -2;
    blend->wait = 0;
}

void ZukanDetailBlend_SetIn(u32 screen, ZukanDetailBlend *blend) {
    blend->ev = 16;
    blend->step = 0;
    blend->wait = 0;
    ZukanDetailBlend_Apply(screen, blend);
}

void ZukanDetailBlend_SetOut(u32 screen, ZukanDetailBlend *blend) {
    blend->ev = 0;
    blend->step = 0;
    blend->wait = 0;
    ZukanDetailBlend_Apply(screen, blend);
}

void ZukanDetailBlend_InitPlanes(ZukanDetailBlend *blend) {
    blend->plane1 = GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BD;
    blend->plane2 = GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BD;
}

static void ZukanDetailBlend_Apply(u32 screen, ZukanDetailBlend *blend) {
    if (screen == 0) {
        G2x_SetBlendBrightnessExt_(REG_BLDCNT_ADDR, blend->plane1, blend->plane2, blend->ev, 0, blend->ev - 16);
    } else {
        G2x_SetBlendBrightnessExt_(REG_DB_BLDCNT_ADDR, blend->plane1, blend->plane2, blend->ev, 0, blend->ev - 16);
    }
}

ZukanDetailPalFade *ZukanDetailPalFade_CreateEx(HeapID heapId, u16 buffers) {
    ZukanDetailPalFade *fade = GFL_HeapAllocate(heapId, sizeof(ZukanDetailPalFade), TRUE, "zukan_detail_common.c", 485);
    fade->tcbBuffer = GFL_HeapAllocate(heapId, GFL_TCBMgrCalcAllocSize(2), TRUE, "zukan_detail_common.c", 488);
    fade->tcbMgr = GFL_TCBMgrCreate(2, fade->tcbBuffer);
    fade->palette = PaletteFade_Create(heapId);
    PaletteFade_SetTransferAll(fade->palette, 1);
    fade->buffers = buffers;
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_MAIN_BG) {
        PaletteFade_AllocBuffer(fade->palette, 0, 0x1a0, heapId);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_MAIN_OBJ) {
        PaletteFade_AllocBuffer(fade->palette, 2, 0x100, heapId);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_SUB_BG) {
        PaletteFade_AllocBuffer(fade->palette, 1, 0x1a0, heapId);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_SUB_OBJ) {
        PaletteFade_AllocBuffer(fade->palette, 3, 0x1c0, heapId);
    }
    fade->state = ZUKAN_DETAIL_PALFADE_SHOWN;
    return fade;
}

ZukanDetailPalFade *ZukanDetailPalFade_Create(HeapID heapId) {
    return ZukanDetailPalFade_CreateEx(heapId, ZUKAN_DETAIL_PALFADE_MAIN_BG | ZUKAN_DETAIL_PALFADE_SUB_BG |
                                                   ZUKAN_DETAIL_PALFADE_MAIN_OBJ | ZUKAN_DETAIL_PALFADE_SUB_OBJ);
}

void ZukanDetailPalFade_Free(ZukanDetailPalFade *fade) {
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_SUB_OBJ) {
        PaletteFade_FreeBuffer(fade->palette, 3);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_SUB_BG) {
        PaletteFade_FreeBuffer(fade->palette, 1);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_MAIN_OBJ) {
        PaletteFade_FreeBuffer(fade->palette, 2);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_MAIN_BG) {
        PaletteFade_FreeBuffer(fade->palette, 0);
    }
    PaletteFade_Free(fade->palette);
    GFL_TCBMgrDelete(fade->tcbMgr);
    GFL_HeapFree(fade->tcbBuffer);
    GFL_HeapFree(fade);
}

void ZukanDetailPalFade_Update(ZukanDetailPalFade *fade) {
    GFL_TCBMgrUpdate(fade->tcbMgr);
    switch (fade->state) {
    case ZUKAN_DETAIL_PALFADE_SHOWN:
    case ZUKAN_DETAIL_PALFADE_HIDDEN:
        break;
    case ZUKAN_DETAIL_PALFADE_FADING_IN:
        if (!PaletteFade_GetActiveMask(fade->palette)) {
            fade->state = ZUKAN_DETAIL_PALFADE_SHOWN;
        }
        break;
    case ZUKAN_DETAIL_PALFADE_FADING_OUT:
        if (!PaletteFade_GetActiveMask(fade->palette)) {
            fade->state = ZUKAN_DETAIL_PALFADE_HIDDEN;
        }
        break;
    }
}

void ZukanDetailPalFade_VBlank(ZukanDetailPalFade *fade) {
    PaletteFade_Transfer(fade->palette);
}

BOOL ZukanDetailPalFade_IsFading(ZukanDetailPalFade *fade) {
    if (fade->state == ZUKAN_DETAIL_PALFADE_FADING_IN || fade->state == ZUKAN_DETAIL_PALFADE_FADING_OUT) {
        return TRUE;
    }
    return FALSE;
}

void ZukanDetailPalFade_StartIn(ZukanDetailPalFade *fade) {
    if (!ZukanDetailPalFade_IsFading(fade)) {
        ZukanDetailPalFade_Start(fade, -1, 16, 0);
        fade->state = ZUKAN_DETAIL_PALFADE_FADING_IN;
    }
}

void ZukanDetailPalFade_StartOut(ZukanDetailPalFade *fade) {
    if (!ZukanDetailPalFade_IsFading(fade)) {
        ZukanDetailPalFade_Start(fade, -1, 0, 16);
        fade->state = ZUKAN_DETAIL_PALFADE_FADING_OUT;
    }
}

void ZukanDetailPalFade_SetHidden(ZukanDetailPalFade *fade) {
    if (!ZukanDetailPalFade_IsFading(fade)) {
        ZukanDetailPalFade_Start(fade, 0, 16, 16);
        fade->state = ZUKAN_DETAIL_PALFADE_HIDDEN;
    }
}

void ZukanDetailPalFade_LoadPalette(ZukanDetailPalFade *fade, ArcTool *arc, u32 fileId, HeapID heapId, u32 buffer,
                                    u32 size, u16 offset, u16 srcOffset) {
    PaletteFade_LoadArcNCLREx(fade->palette, arc, fileId, heapId, buffer, size, offset, srcOffset);
}

void ZukanDetailPalFade_ReadPalettes(ZukanDetailPalFade *fade) {
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_MAIN_BG) {
        PaletteFade_LoadFromVRAM(fade->palette, 0, 0, 0x1a0);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_MAIN_OBJ) {
        PaletteFade_LoadFromVRAM(fade->palette, 2, 0, 0x100);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_SUB_BG) {
        PaletteFade_LoadFromVRAM(fade->palette, 1, 0, 0x1a0);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_SUB_OBJ) {
        PaletteFade_LoadFromVRAM(fade->palette, 3, 0, 0x1c0);
    }
}

static void ZukanDetailPalFade_Start(ZukanDetailPalFade *fade, s8 wait, u8 start, u8 end) {
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_MAIN_BG) {
        PaletteFade_StartFade(fade->palette, ZUKAN_DETAIL_PALFADE_MAIN_BG, 0x1fff, wait, start, end, 0, fade->tcbMgr);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_MAIN_OBJ) {
        PaletteFade_StartFade(fade->palette, ZUKAN_DETAIL_PALFADE_MAIN_OBJ, 0xff, wait, start, end, 0, fade->tcbMgr);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_SUB_BG) {
        PaletteFade_StartFade(fade->palette, ZUKAN_DETAIL_PALFADE_SUB_BG, 0x1fff, wait, start, end, 0, fade->tcbMgr);
    }
    if (fade->buffers & ZUKAN_DETAIL_PALFADE_SUB_OBJ) {
        PaletteFade_StartFade(fade->palette, ZUKAN_DETAIL_PALFADE_SUB_OBJ, 0x3fff, wait, start, end, 0, fade->tcbMgr);
    }
}

// Loads a page's background on a BG, with its bottom rows' colors in bottomPalette and the rest's in topPalette
ZukanDetailBackground *ZukanDetailBackground_Create(HeapID heapId, u32 page, u8 bg, u8 bottomPalette, u8 topPalette) {
    ZukanDetailBackground *background =
        GFL_HeapAllocate(heapId, sizeof(ZukanDetailBackground), TRUE, "zukan_detail_common.c", 803);
    ArcTool *arc;
    u32 screen = 1;
    u32 palType;
    u32 nscr;

    if (bg < 4) {
        screen = 0;
        palType = PALTYPE_MAIN_BG;
    } else {
        palType = PALTYPE_SUB_BG;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, heapId);
    GFL_G2DIOLoadArcNCLR(arc, sBackgroundFiles[screen].nclr, palType, sBottomPalettes[screen][page] * 32,
                         bottomPalette * 32, 32, heapId);
    GFL_G2DIOLoadArcNCLR(arc, sBackgroundFiles[screen].nclr, palType, sTopPalettes[screen][page] * 32, topPalette * 32,
                         32, heapId);
    background->chars = GFL_BGSysLoadArcNCGRDynamic(arc, sBackgroundFiles[screen].ncgr, bg, 0, FALSE, heapId);
    nscr = sBackgroundFiles[screen].nscr;
    if (page == 3 && screen == 0) {
        nscr = 33;
    }
    GFL_G2DIOLoadNSCRSync(arc, nscr, bg, 0, CHAR_POS(background->chars), 0, FALSE, heapId);
    GFL_BGSysSetScrPaletteNo(bg, 0, 21, 32, 3, bottomPalette);
    GFL_BGSysSetScrPaletteNo(bg, 0, 0, 32, 21, topPalette);
    GFL_ArcToolFree(arc);
    GFL_BGSysQueueScrLoad(bg);
    background->bg = bg;
    background->wait = 4;
    return background;
}

void ZukanDetailBackground_Free(ZukanDetailBackground *background) {
    GFL_BGSysFreeCharMemory(background->bg, CHAR_POS(background->chars), CHAR_SIZE(background->chars));
    GFL_HeapFree(background);
}

// Scrolls the background by a pixel every few frames
void ZukanDetailBackground_Update(ZukanDetailBackground *background) {
    if (background->wait == 0) {
        GFL_BGSysMoveBGReq(background->bg, BG_MOVE_RIGHT, 1);
        background->wait = 4;
    } else {
        background->wait--;
    }
}

// Loads a BG's palette, characters and screen from an archive. The characters are only loaded, and returned, when
// loaded is FALSE; otherwise chars gives them
u32 ZukanDetail_LoadBG(BOOL loaded, HeapID heapId, u8 bg, u32 palettes, u8 palette, u8 srcPalette, u32 arcId, u32 nclr,
                       u32 ncgr, u32 nscr, u32 chars) {
    u32 palType = PALTYPE_MAIN_BG;
    ArcTool *arc;

    if (bg >= 4) {
        palType = PALTYPE_SUB_BG;
    }
    arc = GFL_ArcSysCreateFileHandle(arcId, heapId);
    if (!loaded) {
        GFL_G2DIOLoadArcNCLR(arc, nclr, palType, srcPalette * 32, palette * 32, palettes * 32, heapId);
    }
    if (!loaded) {
        chars = GFL_BGSysLoadArcNCGRDynamic(arc, ncgr, bg, 0, FALSE, heapId);
    }
    GFL_G2DIOLoadNSCRSync(arc, nscr, bg, 0, CHAR_POS(chars), 0, FALSE, heapId);
    GFL_BGSysSetScrPaletteNo(bg, 0, 0, 32, 24, palette);
    GFL_ArcToolFree(arc);
    GFL_BGSysQueueScrLoad(bg);
    return chars;
}

void ZukanDetail_FreeBG(u32 bg, u32 chars) {
    GFL_BGSysFreeCharMemory(bg, CHAR_POS(chars), CHAR_SIZE(chars));
}
