// The ROM names this file zukan_detail_common.c.
#include "types.h"
#include "app/pokedex_detail.h"
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
#include "save/pokedex_savedata.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/palanm.h"

// What the Pokédex detail screen's pages share: the screen's systems and the list of Pokémon it pages through, the
// blending and palette fades of the pages, and the backgrounds that the pages load

static void PokedexDetailBlend_Apply(u32 screen, PokedexDetailBlend *blend);
static void PokedexDetailPalFade_Start(PokedexDetailPalFade *fade, s8 wait, u8 start, u8 end);

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

PokedexDetailCommon *PokedexDetailCommon_Create(HeapID heapId, GameData *gameData, PokedexDetailGraphic *graphic,
                                                Font *font, PrintQueue *printQueue, PokedexDetailTouchbar *touchbar,
                                                PokedexDetailHeadbar *headbar, const u16 *list, u16 count, u16 *index) {
    PokedexDetailCommon *common =
        GFL_HeapAllocate(heapId, sizeof(PokedexDetailCommon), TRUE, "zukan_detail_common.c", 96);
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

void PokedexDetailCommon_Free(PokedexDetailCommon *common) {
    GFL_HeapFree(common);
}

GameData *PokedexDetailCommon_GetGameData(PokedexDetailCommon *common) {
    return common->gameData;
}

PokedexDetailGraphic *PokedexDetailCommon_GetGraphic(PokedexDetailCommon *common) {
    return common->graphic;
}

Font *PokedexDetailCommon_GetFont(PokedexDetailCommon *common) {
    return common->font;
}

PrintQueue *PokedexDetailCommon_GetPrintQueue(PokedexDetailCommon *common) {
    return common->printQueue;
}

PokedexDetailTouchbar *PokedexDetailCommon_GetTouchbar(PokedexDetailCommon *common) {
    return common->touchbar;
}

PokedexDetailHeadbar *PokedexDetailCommon_GetHeadbar(PokedexDetailCommon *common) {
    return common->headbar;
}

u16 PokedexDetailCommon_GetCount(PokedexDetailCommon *common) {
    return common->count;
}

u16 PokedexDetailCommon_GetSpecies(PokedexDetailCommon *common) {
    return common->list[*common->index];
}

// Moves to the next Pokémon of the list that has been seen, wrapping around
void PokedexDetailCommon_GoNext(PokedexDetailCommon *common) {
    u16 start = *common->index;
    PokeDexSave *pokedex = GameData_GetPokedex(common->gameData);

    do {
        (*common->index)++;
        if (*common->index >= common->count) {
            *common->index = 0;
        }
    } while (*common->index != start && !PokeDex_IsSeen(pokedex, common->list[*common->index]));
}

void PokedexDetailCommon_GoPrev(PokedexDetailCommon *common) {
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

PokedexDetailBlend *PokedexDetailBlend_Create(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(PokedexDetailBlend), TRUE, "zukan_detail_common.c", 275);
}

void PokedexDetailBlend_Free(PokedexDetailBlend *blend) {
    GFL_HeapFree(blend);
}

void PokedexDetailBlend_Update(PokedexDetailBlend *main, PokedexDetailBlend *sub) {
    PokedexDetailBlend *blends[2];
    u8 screen;

    blends[0] = main;
    blends[1] = sub;
    for (screen = 0; screen < 2; screen++) {
        PokedexDetailBlend *blend = blends[screen];
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
                PokedexDetailBlend_Apply(screen, blend);
            } else {
                blend->wait--;
            }
        }
    }
}

BOOL PokedexDetailBlend_IsActive(PokedexDetailBlend *blend) {
    if (blend->step != 0) {
        return TRUE;
    }
    return FALSE;
}

void PokedexDetailBlend_StartIn(PokedexDetailBlend *blend) {
    blend->ev = 0;
    blend->step = 2;
    blend->wait = 0;
}

void PokedexDetailBlend_StartOut(PokedexDetailBlend *blend) {
    blend->ev = 16;
    blend->step = -2;
    blend->wait = 0;
}

void PokedexDetailBlend_SetIn(u32 screen, PokedexDetailBlend *blend) {
    blend->ev = 16;
    blend->step = 0;
    blend->wait = 0;
    PokedexDetailBlend_Apply(screen, blend);
}

void PokedexDetailBlend_SetOut(u32 screen, PokedexDetailBlend *blend) {
    blend->ev = 0;
    blend->step = 0;
    blend->wait = 0;
    PokedexDetailBlend_Apply(screen, blend);
}

void PokedexDetailBlend_InitPlanes(PokedexDetailBlend *blend) {
    blend->plane1 = GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BD;
    blend->plane2 = GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BD;
}

static void PokedexDetailBlend_Apply(u32 screen, PokedexDetailBlend *blend) {
    if (screen == 0) {
        G2x_SetBlendBrightnessExt_(REG_BLDCNT_ADDR, blend->plane1, blend->plane2, blend->ev, 0, blend->ev - 16);
    } else {
        G2x_SetBlendBrightnessExt_(REG_DB_BLDCNT_ADDR, blend->plane1, blend->plane2, blend->ev, 0, blend->ev - 16);
    }
}

PokedexDetailPalFade *PokedexDetailPalFade_CreateEx(HeapID heapId, u16 buffers) {
    PokedexDetailPalFade *fade =
        GFL_HeapAllocate(heapId, sizeof(PokedexDetailPalFade), TRUE, "zukan_detail_common.c", 485);
    fade->tcbBuffer = GFL_HeapAllocate(heapId, GFL_TCBMgrCalcAllocSize(2), TRUE, "zukan_detail_common.c", 488);
    fade->tcbMgr = GFL_TCBMgrCreate(2, fade->tcbBuffer);
    fade->palette = PaletteFade_Create(heapId);
    PaletteFade_SetTransferAll(fade->palette, 1);
    fade->buffers = buffers;
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_MAIN_BG) {
        PaletteFade_AllocBuffer(fade->palette, 0, 0x1a0, heapId);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_MAIN_OBJ) {
        PaletteFade_AllocBuffer(fade->palette, 2, 0x100, heapId);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_SUB_BG) {
        PaletteFade_AllocBuffer(fade->palette, 1, 0x1a0, heapId);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_SUB_OBJ) {
        PaletteFade_AllocBuffer(fade->palette, 3, 0x1c0, heapId);
    }
    fade->state = POKEDEX_DETAIL_PALFADE_SHOWN;
    return fade;
}

PokedexDetailPalFade *PokedexDetailPalFade_Create(HeapID heapId) {
    return PokedexDetailPalFade_CreateEx(heapId, POKEDEX_DETAIL_PALFADE_MAIN_BG | POKEDEX_DETAIL_PALFADE_SUB_BG |
                                                     POKEDEX_DETAIL_PALFADE_MAIN_OBJ | POKEDEX_DETAIL_PALFADE_SUB_OBJ);
}

void PokedexDetailPalFade_Free(PokedexDetailPalFade *fade) {
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_SUB_OBJ) {
        PaletteFade_FreeBuffer(fade->palette, 3);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_SUB_BG) {
        PaletteFade_FreeBuffer(fade->palette, 1);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_MAIN_OBJ) {
        PaletteFade_FreeBuffer(fade->palette, 2);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_MAIN_BG) {
        PaletteFade_FreeBuffer(fade->palette, 0);
    }
    PaletteFade_Free(fade->palette);
    GFL_TCBMgrDelete(fade->tcbMgr);
    GFL_HeapFree(fade->tcbBuffer);
    GFL_HeapFree(fade);
}

void PokedexDetailPalFade_Update(PokedexDetailPalFade *fade) {
    GFL_TCBMgrUpdate(fade->tcbMgr);
    switch (fade->state) {
    case POKEDEX_DETAIL_PALFADE_SHOWN:
    case POKEDEX_DETAIL_PALFADE_HIDDEN:
        break;
    case POKEDEX_DETAIL_PALFADE_FADING_IN:
        if (!PaletteFade_GetActiveMask(fade->palette)) {
            fade->state = POKEDEX_DETAIL_PALFADE_SHOWN;
        }
        break;
    case POKEDEX_DETAIL_PALFADE_FADING_OUT:
        if (!PaletteFade_GetActiveMask(fade->palette)) {
            fade->state = POKEDEX_DETAIL_PALFADE_HIDDEN;
        }
        break;
    }
}

void PokedexDetailPalFade_VBlank(PokedexDetailPalFade *fade) {
    PaletteFade_Transfer(fade->palette);
}

BOOL PokedexDetailPalFade_IsFading(PokedexDetailPalFade *fade) {
    if (fade->state == POKEDEX_DETAIL_PALFADE_FADING_IN || fade->state == POKEDEX_DETAIL_PALFADE_FADING_OUT) {
        return TRUE;
    }
    return FALSE;
}

void PokedexDetailPalFade_StartIn(PokedexDetailPalFade *fade) {
    if (!PokedexDetailPalFade_IsFading(fade)) {
        PokedexDetailPalFade_Start(fade, -1, 16, 0);
        fade->state = POKEDEX_DETAIL_PALFADE_FADING_IN;
    }
}

void PokedexDetailPalFade_StartOut(PokedexDetailPalFade *fade) {
    if (!PokedexDetailPalFade_IsFading(fade)) {
        PokedexDetailPalFade_Start(fade, -1, 0, 16);
        fade->state = POKEDEX_DETAIL_PALFADE_FADING_OUT;
    }
}

void PokedexDetailPalFade_SetHidden(PokedexDetailPalFade *fade) {
    if (!PokedexDetailPalFade_IsFading(fade)) {
        PokedexDetailPalFade_Start(fade, 0, 16, 16);
        fade->state = POKEDEX_DETAIL_PALFADE_HIDDEN;
    }
}

void PokedexDetailPalFade_LoadPalette(PokedexDetailPalFade *fade, ArcTool *arc, u32 fileId, HeapID heapId, u32 buffer,
                                      u32 size, u16 offset, u16 srcOffset) {
    PaletteFade_LoadArcNCLREx(fade->palette, arc, fileId, heapId, buffer, size, offset, srcOffset);
}

void PokedexDetailPalFade_ReadPalettes(PokedexDetailPalFade *fade) {
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_MAIN_BG) {
        PaletteFade_LoadFromVRAM(fade->palette, 0, 0, 0x1a0);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_MAIN_OBJ) {
        PaletteFade_LoadFromVRAM(fade->palette, 2, 0, 0x100);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_SUB_BG) {
        PaletteFade_LoadFromVRAM(fade->palette, 1, 0, 0x1a0);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_SUB_OBJ) {
        PaletteFade_LoadFromVRAM(fade->palette, 3, 0, 0x1c0);
    }
}

static void PokedexDetailPalFade_Start(PokedexDetailPalFade *fade, s8 wait, u8 start, u8 end) {
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_MAIN_BG) {
        PaletteFade_StartFade(fade->palette, POKEDEX_DETAIL_PALFADE_MAIN_BG, 0x1fff, wait, start, end, 0, fade->tcbMgr);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_MAIN_OBJ) {
        PaletteFade_StartFade(fade->palette, POKEDEX_DETAIL_PALFADE_MAIN_OBJ, 0xff, wait, start, end, 0, fade->tcbMgr);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_SUB_BG) {
        PaletteFade_StartFade(fade->palette, POKEDEX_DETAIL_PALFADE_SUB_BG, 0x1fff, wait, start, end, 0, fade->tcbMgr);
    }
    if (fade->buffers & POKEDEX_DETAIL_PALFADE_SUB_OBJ) {
        PaletteFade_StartFade(fade->palette, POKEDEX_DETAIL_PALFADE_SUB_OBJ, 0x3fff, wait, start, end, 0, fade->tcbMgr);
    }
}

// Loads a page's background on a BG, with its bottom rows' colors in bottomPalette and the rest's in topPalette
PokedexDetailBackground *PokedexDetailBackground_Create(HeapID heapId, u32 page, u8 bg, u8 bottomPalette,
                                                        u8 topPalette) {
    PokedexDetailBackground *background =
        GFL_HeapAllocate(heapId, sizeof(PokedexDetailBackground), TRUE, "zukan_detail_common.c", 803);
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
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, heapId);
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

void PokedexDetailBackground_Free(PokedexDetailBackground *background) {
    GFL_BGSysFreeCharMemory(background->bg, CHAR_POS(background->chars), CHAR_SIZE(background->chars));
    GFL_HeapFree(background);
}

// Scrolls the background by a pixel every few frames
void PokedexDetailBackground_Update(PokedexDetailBackground *background) {
    if (background->wait == 0) {
        GFL_BGSysMoveBGReq(background->bg, BG_MOVE_RIGHT, 1);
        background->wait = 4;
    } else {
        background->wait--;
    }
}

// Loads a BG's palette, characters and screen from an archive. The characters are only loaded, and returned, when
// loaded is FALSE; otherwise chars gives them
u32 PokedexDetail_LoadBG(BOOL loaded, HeapID heapId, u8 bg, u32 palettes, u8 palette, u8 srcPalette, u32 arcId,
                         u32 nclr, u32 ncgr, u32 nscr, u32 chars) {
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

void PokedexDetail_FreeBG(u32 bg, u32 chars) {
    GFL_BGSysFreeCharMemory(bg, CHAR_POS(chars), CHAR_SIZE(chars));
}
