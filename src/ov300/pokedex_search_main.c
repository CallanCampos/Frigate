// The ROM names this file zknsearch_main.c.
#include "types.h"
#include "app/pokedex_search_bmp.h"
#include "app/pokedex_search_main.h"
#include "app/pokedex_search_obj.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "system/app_menu_common.h"
#include "system/bgwinfrm.h"
#include "system/blink_palanm.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/zknsearch_main.h"

// The Pokédex's search screen: its BGs and graphics, its fades and the touch bar that slides in

// The frame of the touch bar: where it is in BG 0's screen and how far it slides
#define TOUCHBAR_Y 21
#define TOUCHBAR_HEIGHT 3
#define TOUCHBAR_SLIDE 48

// The screen of the search's progress bar
#define LOADING_Y 17
#define LOADING_HEIGHT 6

static void PokedexSearchMain_VBlankTask(TCB *tcb, void *data);
static void PokedexSearchMain_HBlankTask(TCB *tcb, void *data);

static const BGSysVRAMConfig sPokedexSearchVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// The messages of the types, and of none
static const u16 sPokedexSearchTypeMsgs[POKEDEX_SEARCH_TYPE_COUNT + 1] = {
    ZknsearchMain_Text_Normal,   ZknsearchMain_Text_Fighting, ZknsearchMain_Text_Flying, ZknsearchMain_Text_Poison,
    ZknsearchMain_Text_Ground,   ZknsearchMain_Text_Rock,     ZknsearchMain_Text_Bug,    ZknsearchMain_Text_Ghost,
    ZknsearchMain_Text_Steel,    ZknsearchMain_Text_Fire,     ZknsearchMain_Text_Water,  ZknsearchMain_Text_Grass,
    ZknsearchMain_Text_Electric, ZknsearchMain_Text_Psychic,  ZknsearchMain_Text_Ice,    ZknsearchMain_Text_Dragon,
    ZknsearchMain_Text_Dark,     ZknsearchMain_Text_Empty_9,
};

// The shapes in the order of the list
static const u8 sPokedexSearchShapeOrder[POKEDEX_SEARCH_SHAPE_COUNT] = { 0, 13, 1, 7, 5, 12, 8, 4, 6, 3, 2, 9, 11, 10 };

void PokedexSearchMain_InitVBlank(PokedexSearchWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(PokedexSearchMain_VBlankTask, wk, 0);
}

void PokedexSearchMain_ExitVBlank(PokedexSearchWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static void PokedexSearchMain_VBlankTask(TCB *tcb, void *data) {
    PokedexSearchWork *wk = data;

    GFL_BGSysUpdate();
    ClActSys_VBlank();
    PaletteFade_Transfer(wk->paletteFade);
    if (wk->bgRequest == POKEDEX_SEARCH_BG_REQUEST_ON) {
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG3, TRUE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, TRUE);
        wk->bgRequest = 0;
    } else if (wk->bgRequest == POKEDEX_SEARCH_BG_REQUEST_OFF) {
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG3, FALSE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, FALSE);
        wk->bgRequest = 0;
    }
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

void PokedexSearchMain_InitHBlank(PokedexSearchWork *wk) {
    wk->hblankTask = GFL_HBlankTCBAdd(PokedexSearchMain_HBlankTask, wk, 0);
}

void PokedexSearchMain_ExitHBlank(PokedexSearchWork *wk) {
    GFL_TCBRemove(wk->hblankTask);
}

// The blend of the top screen's list changes down the screen, by the height of the list's rows
static void PokedexSearchMain_HBlankTask(TCB *tcb, void *data) {
    PokedexSearchWork *wk = data;
    s32 vcount = GX_GetVCount();

    if (wk->page == POKEDEX_SEARCH_PAGE_SHAPE) {
        if (vcount >= 152) {
            G2S_ChangeBlendAlpha(11, 5);
        } else if (vcount >= 112) {
            G2S_ChangeBlendAlpha(10, 6);
        } else if (vcount >= 72) {
            G2S_ChangeBlendAlpha(9, 7);
        } else if (vcount >= 32) {
            G2S_ChangeBlendAlpha(8, 8);
        } else {
            G2S_ChangeBlendAlpha(6, 10);
        }
    } else {
        if (vcount >= 168) {
            G2S_ChangeBlendAlpha(11, 5);
        } else if (vcount >= 144) {
            G2S_ChangeBlendAlpha(10, 6);
        } else if (vcount >= 120) {
            G2S_ChangeBlendAlpha(9, 7);
        } else if (vcount >= 96) {
            G2S_ChangeBlendAlpha(8, 8);
        } else if (vcount >= 72) {
            G2S_ChangeBlendAlpha(6, 10);
        } else {
            G2S_ChangeBlendAlpha(4, 12);
        }
    }
}

void PokedexSearchMain_InitVRAM(void) {
    GFL_BGSysInitVRAM(0);
    GFL_BGSysSetVRAMBanks(&sPokedexSearchVRAMConfig);
}

const BGSysVRAMConfig *PokedexSearchMain_GetVRAMConfig(void) {
    return &sPokedexSearchVRAMConfig;
}

void PokedexSearchMain_InitBG(void) {
    GFL_BGSysCreate(HEAPID_POKEDEX_SEARCH);
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&config);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            0,
            0,
        };

        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            0,
            0,
        };

        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(1);
        GFL_BGSysQueueScrLoad(1);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x08000),
            0x4000,
            GX_BG_EXTPLTT_01,
            3,
            0,
            0,
        };

        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x18000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            0,
            0,
        };

        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(3);
        GFL_BGSysQueueScrLoad(3);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x18000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            0,
            0,
        };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            0,
            0,
        };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(5);
        GFL_BGSysQueueScrLoad(5);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            0,
            0,
        };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            0,
            0,
        };

        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(7);
        GFL_BGSysQueueScrLoad(7);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
}

void PokedexSearchMain_ExitBG(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysFree();
}

void PokedexSearchMain_SetMainBGPriority(void) {
    GFL_BGSysSetBGPriority(0, 1);
    GFL_BGSysSetBGPriority(1, 0);
}

void PokedexSearchMain_SetListBGPriority(void) {
    GFL_BGSysSetBGPriority(0, 0);
    GFL_BGSysSetBGPriority(1, 1);
}

void PokedexSearchMain_LoadGraphics(void) {
    ArcTool *arc;
    u32 chars;
    u16 *screen;
    u32 i;

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
    GFL_G2DIOLoadArcNCLRDefault(arc, 75, PALTYPE_MAIN_BG, 0, 0xc0, HEAPID_POKEDEX_SEARCH);
    GFL_G2DIOLoadArcNCLRDefault(arc, 76, PALTYPE_SUB_BG, 0, 0xa0, HEAPID_POKEDEX_SEARCH);
    GFL_BGSysLoadArcNCGRDynamic(arc, 83, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 83, 1, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 83, 3, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 81, 2, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 83, 5, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 82, 6, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 82, 4, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 97, 2, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 96, 4, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 98, 6, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_ArcToolFree(arc);

    // The touch bar, at the bottom of BG 0
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
    chars = GFL_BGSysLoadArcNCGRDynamic(arc, AppMenuCommon_GetBarBgCharFile(), 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_G2DIOLoadArcNCLRDefault(arc, AppMenuCommon_GetBarBgPaletteFile(), PALTYPE_MAIN_BG, 14 * 0x20, 0x20,
                                HEAPID_POKEDEX_SEARCH);
    loadBGScrToVramByFileNoReserveNegAlign(arc, AppMenuCommon_GetBarScreenFile(), 0, 0, 0, FALSE,
                                           HEAPID_POKEDEX_SEARCH);
    GFL_BGSysSetScrPaletteNo(0, 0, TOUCHBAR_Y, 32, TOUCHBAR_HEIGHT, 14);
    GFL_BGSysQueueScrLoad(0);
    GFL_ArcToolFree(arc);
    screen = GFL_BGSysIsScrHeapExists(0);
    for (i = 0; i < 32 * TOUCHBAR_HEIGHT; i++) {
        screen[TOUCHBAR_Y * 32 + i] += chars;
    }
    GFL_BGSysQueueScrLoad(0);

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 15 * 0x20, 0x20, HEAPID_POKEDEX_SEARCH);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 15 * 0x20, 0x20, HEAPID_POKEDEX_SEARCH);
}

void PokedexSearchMain_InitPaletteFade(PokedexSearchWork *wk) {
    wk->paletteFade = PaletteFade_Create(HEAPID_POKEDEX_SEARCH);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_BG, 0x200, HEAPID_POKEDEX_SEARCH);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_BG, 0x200, HEAPID_POKEDEX_SEARCH);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0x200, HEAPID_POKEDEX_SEARCH);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0x200, HEAPID_POKEDEX_SEARCH);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_MAIN_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_SUB_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0, 0x200);
}

void PokedexSearchMain_ExitPaletteFade(PokedexSearchWork *wk) {
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_BG);
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_BG);
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ);
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ);
    PaletteFade_Free(wk->paletteFade);
}

// Fades everything but the touch bar and its buttons, which slide instead
void PokedexSearchMain_StartFade(PokedexSearchWork *wk, u8 start, u8 end) {
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_BG, 0xbfff, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_SUB_BG, 0x7fef, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 0xfe1f, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_SUB_OBJ, 0xbfff, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
}

void PokedexSearchMain_SetBlend(BOOL enabled) {
    if (enabled == TRUE) {
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG3, GX_BLEND_PLANEMASK_BG2, 16, 6);
        G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2,
                           6, 10);
    } else {
        G2S_BlendNone();
    }
}

// The marks' windows on the top screen show the row under them unblended
void PokedexSearchMain_InitWindow(void) {
    G2S_SetWndOBJInsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ,
                             TRUE);
    G2S_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 |
                               GX_WND_PLANEMASK_OBJ,
                           TRUE);
    GXS_SetVisibleWnd(GX_WNDMASK_OW);
}

void PokedexSearchMain_ExitWindow(void) {
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

void PokedexSearchMain_InitMsg(PokedexSearchWork *wk) {
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ZKNSEARCH_MAIN, HEAPID_POKEDEX_SEARCH);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_POKEDEX_SEARCH);
    wk->printQueue = PrintQueue_CreateDefault(HEAPID_POKEDEX_SEARCH);
    wk->strbuf = GFL_StrBufCreate(0x80, HEAPID_POKEDEX_SEARCH);
}

void PokedexSearchMain_ExitMsg(PokedexSearchWork *wk) {
    GFL_StrBufFree(wk->strbuf);
    PrintQueue_Free(wk->printQueue);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_FontFree(wk->font);
    GFL_MsgDataFree(wk->msgData);
}

void PokedexSearchMain_InitBlinkAnm(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);

    wk->blinkAnm = BlinkPalAnm_Create(3 * 0x10, 0x10, 1, HEAPID_POKEDEX_SEARCH);
    BlinkPalAnm_SetPalBufferArcTool(wk->blinkAnm, arc, 75, 2 * 0x10, 3 * 0x10);
    GFL_ArcToolFree(arc);
}

void PokedexSearchMain_ExitBlinkAnm(PokedexSearchWork *wk) {
    BlinkPalAnm_Free(wk->blinkAnm);
}

void PokedexSearchMain_InitBGWinFrame(PokedexSearchWork *wk) {
    u16 *screen = GFL_BGSysIsScrHeapExists(0);

    wk->bgWinFrame = BGWinFrame_Create(2, 1, HEAPID_POKEDEX_SEARCH);
    BGWinFrame_InitFrame(wk->bgWinFrame, 0, 0, 32, TOUCHBAR_HEIGHT);
    BGWinFrame_SetScreen(wk->bgWinFrame, 0, &screen[TOUCHBAR_Y * 32]);
    GFL_BGSysFillScrArea(0, 0, 0, TOUCHBAR_Y, 32, TOUCHBAR_HEIGHT, 0);
    GFL_BGSysQueueScrLoad(0);
}

void PokedexSearchMain_ExitBGWinFrame(PokedexSearchWork *wk) {
    BGWinFrame_Delete(wk->bgWinFrame);
}

void PokedexSearchMain_LoadMainScreen(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 95, 0, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 94, 3, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_ArcToolFree(arc);
}

void PokedexSearchMain_LoadOrderScreen(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 91, 3, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_ArcToolFree(arc);
}

void PokedexSearchMain_LoadInitialScreen(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 90, 3, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_ArcToolFree(arc);
}

void PokedexSearchMain_LoadTypeScreen(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 92, 3, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_ArcToolFree(arc);
}

void PokedexSearchMain_LoadColorScreen(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 88, 3, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_ArcToolFree(arc);
}

void PokedexSearchMain_LoadShapeScreen(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 89, 3, 0, 0, FALSE, HEAPID_POKEDEX_SEARCH);
    GFL_ArcToolFree(arc);
}

void PokedexSearchMain_LoadLoadingScreen(PokedexSearchWork *wk) {
    ArcTool *arc;
    NNSG2dScreenData *screen;
    void *file;

    wk->loadingScreen =
        GFL_HeapAllocate(HEAPID_POKEDEX_SEARCH, 32 * LOADING_HEIGHT * sizeof(u16), FALSE, "zknsearch_main.c", 944);
    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
    file = GFL_G2DIOReadNSCRArc(arc, 93, FALSE, &screen, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
    MIi_CpuCopy16(screen->rawData, wk->loadingScreen, 32 * LOADING_HEIGHT * sizeof(u16));
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
}

void PokedexSearchMain_FreeLoadingScreen(PokedexSearchWork *wk) {
    GFL_HeapFree(wk->loadingScreen);
}

void PokedexSearchMain_ShowLoading(PokedexSearchWork *wk) {
    GFL_BGSysLoadScrAreaAll(1, wk->loadingScreen, 0, LOADING_Y, 32, LOADING_HEIGHT);
    GFL_BGSysQueueScrLoad(1);
    PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 15);
    PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_EXIT, 14);
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH) == TRUE) {
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, 22);
    } else {
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, 21);
    }
    PokedexSearchObj_SetBGPriority(wk, POKEDEX_SEARCH_OBJ_RETURN, 1);
    PokedexSearchObj_SetBGPriority(wk, POKEDEX_SEARCH_OBJ_EXIT, 1);
    PokedexSearchObj_SetBGPriority(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, 1);
}

void PokedexSearchMain_HideLoading(PokedexSearchWork *wk) {
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_LOADING_BAR, FALSE);
    GFL_BGSysFillScrArea(1, 0, 0, LOADING_Y, 32, LOADING_HEIGHT, 0);
    GFL_BGSysQueueScrLoad(1);
    PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 1);
    PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_EXIT, 0);
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH) == TRUE) {
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, 7);
    } else {
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, 6);
    }
    PokedexSearchObj_SetBGPriority(wk, POKEDEX_SEARCH_OBJ_RETURN, 0);
    PokedexSearchObj_SetBGPriority(wk, POKEDEX_SEARCH_OBJ_EXIT, 0);
    PokedexSearchObj_SetBGPriority(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, 0);
    PokedexSearchBmp_PutStart(wk);
}

void PokedexSearchMain_RequestBGOn(PokedexSearchWork *wk) {
    wk->bgRequest = POKEDEX_SEARCH_BG_REQUEST_ON;
}

void PokedexSearchMain_ClearPage(PokedexSearchWork *wk) {
    wk->bgRequest = POKEDEX_SEARCH_BG_REQUEST_OFF;
    GFL_BGSysClearScr(1);
    GFL_BGSysClearScr(3);
    GFL_BGSysClearScr(5);
    GFL_BGSysQueueScrLoad(1);
    GFL_BGSysQueueScrLoad(3);
    GFL_BGSysQueueScrLoad(5);
    GFL_BGSysFillScrArea(0, 0, 0, 0, 32, TOUCHBAR_Y, 0);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_Y, 0);
    GFL_BGSysMoveBGReq(5, BG_MOVE_SET_Y, 0);
}

void PokedexSearchMain_InitFramePos(PokedexSearchWork *wk) {
    u32 i;
    s16 x, y;

    GFL_BGSysMoveBGReq(4, BG_MOVE_SET_Y, TOUCHBAR_SLIDE);
    GFL_BGSysMoveBGReq(7, BG_MOVE_SET_Y, TOUCHBAR_SLIDE);
    BGWinFrame_Put(wk->bgWinFrame, 0, 0, TOUCHBAR_Y + TOUCHBAR_SLIDE / 8);
    BGWinFrame_UpdateMoves(wk->bgWinFrame);
    for (i = POKEDEX_SEARCH_OBJ_RETURN; i <= POKEDEX_SEARCH_OBJ_SHORTCUT; i++) {
        PokedexSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
        y += TOUCHBAR_SLIDE;
        PokedexSearchObj_SetPos(wk, i, x, y, CLACT_SURFACE_MAIN);
    }
}

void PokedexSearchMain_StartFrameMove(PokedexSearchWork *wk, s16 speed) {
    wk->frameMoveLeft = TOUCHBAR_SLIDE;
    wk->frameMoveSpeed = speed;
    BGWinFrame_StartMove(wk->bgWinFrame, 0, 0, speed / MATH_ABS(speed), TOUCHBAR_SLIDE / MATH_ABS(speed));
}

BOOL PokedexSearchMain_UpdateFrameMove(PokedexSearchWork *wk) {
    u32 i;
    s16 x, y;

    if (wk->frameMoveLeft == 0) {
        return FALSE;
    }
    BGWinFrame_UpdateMoves(wk->bgWinFrame);
    GFL_BGSysMoveBGReq(4, BG_MOVE_DOWN, wk->frameMoveSpeed);
    GFL_BGSysMoveBGReq(7, BG_MOVE_DOWN, wk->frameMoveSpeed);
    for (i = POKEDEX_SEARCH_OBJ_RETURN; i <= POKEDEX_SEARCH_OBJ_SHORTCUT; i++) {
        PokedexSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
        y += wk->frameMoveSpeed;
        PokedexSearchObj_SetPos(wk, i, x, y, CLACT_SURFACE_MAIN);
    }
    wk->frameMoveLeft -= MATH_ABS(wk->frameMoveSpeed);
    return TRUE;
}

u32 PokedexSearchMain_ListPosToType(PokedexSearchWork *wk, u32 pos) {
    return pos;
}

u32 PokedexSearchMain_TypeToListPos(PokedexSearchWork *wk, u32 type) {
    return type;
}

u32 PokedexSearchMain_GetListPosTypeMsg(PokedexSearchWork *wk, u32 pos) {
    return sPokedexSearchTypeMsgs[pos];
}

u32 PokedexSearchMain_GetTypeMsg(PokedexSearchWork *wk, u32 type) {
    return sPokedexSearchTypeMsgs[type];
}

void PokedexSearchMain_ShapeToListPos(PokedexSearchWork *wk) {
    u32 i;

    if (wk->param->criteria->shape != POKEDEX_SEARCH_NONE) {
        for (i = 0; i < POKEDEX_SEARCH_SHAPE_COUNT; i++) {
            if (wk->param->criteria->shape == sPokedexSearchShapeOrder[i]) {
                wk->param->criteria->shape = i;
                return;
            }
        }
    }
}

void PokedexSearchMain_ListPosToShape(PokedexSearchWork *wk) {
    PokedexSearchCriteria *criteria = wk->param->criteria;

    if (criteria->shape != POKEDEX_SEARCH_NONE) {
        criteria->shape = sPokedexSearchShapeOrder[criteria->shape];
    }
}
