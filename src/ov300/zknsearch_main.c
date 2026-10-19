#include "types.h"
#include "app/zknsearch_bmp.h"
#include "app/zknsearch_main.h"
#include "app/zknsearch_obj.h"
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

static void ZknSearchMain_VBlankTask(TCB *tcb, void *data);
static void ZknSearchMain_HBlankTask(TCB *tcb, void *data);

static const BGSysVRAMConfig sZknSearchVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// The messages of the types, and of none
static const u16 sZknSearchTypeMsgs[ZKNSEARCH_TYPE_COUNT + 1] = {
    ZknsearchMain_Text_Normal,   ZknsearchMain_Text_Fighting, ZknsearchMain_Text_Flying, ZknsearchMain_Text_Poison,
    ZknsearchMain_Text_Ground,   ZknsearchMain_Text_Rock,     ZknsearchMain_Text_Bug,    ZknsearchMain_Text_Ghost,
    ZknsearchMain_Text_Steel,    ZknsearchMain_Text_Fire,     ZknsearchMain_Text_Water,  ZknsearchMain_Text_Grass,
    ZknsearchMain_Text_Electric, ZknsearchMain_Text_Psychic,  ZknsearchMain_Text_Ice,    ZknsearchMain_Text_Dragon,
    ZknsearchMain_Text_Dark,     ZknsearchMain_Text_Empty_9,
};

// The shapes in the order of the list
static const u8 sZknSearchShapeOrder[ZKNSEARCH_SHAPE_COUNT] = { 0, 13, 1, 7, 5, 12, 8, 4, 6, 3, 2, 9, 11, 10 };

void ZknSearchMain_InitVBlank(ZknSearchWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(ZknSearchMain_VBlankTask, wk, 0);
}

void ZknSearchMain_ExitVBlank(ZknSearchWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static void ZknSearchMain_VBlankTask(TCB *tcb, void *data) {
    ZknSearchWork *wk = data;

    GFL_BGSysUpdate();
    func_0204b7c8();
    PaletteFade_Transfer(wk->paletteFade);
    if (wk->bgRequest == ZKNSEARCH_BG_REQUEST_ON) {
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG3, TRUE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, TRUE);
        wk->bgRequest = 0;
    } else if (wk->bgRequest == ZKNSEARCH_BG_REQUEST_OFF) {
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG3, FALSE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, FALSE);
        wk->bgRequest = 0;
    }
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

void ZknSearchMain_InitHBlank(ZknSearchWork *wk) {
    wk->hblankTask = GFL_HBlankTCBAdd(ZknSearchMain_HBlankTask, wk, 0);
}

void ZknSearchMain_ExitHBlank(ZknSearchWork *wk) {
    GFL_TCBRemove(wk->hblankTask);
}

// The blend of the top screen's list changes down the screen, by the height of the list's rows
static void ZknSearchMain_HBlankTask(TCB *tcb, void *data) {
    ZknSearchWork *wk = data;
    s32 vcount = GX_GetVCount();

    if (wk->page == ZKNSEARCH_PAGE_SHAPE) {
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

void ZknSearchMain_InitVRAM(void) {
    GFL_BGSysInitVRAM(0);
    GFL_BGSysSetVRAMBanks(&sZknSearchVRAMConfig);
}

const BGSysVRAMConfig *ZknSearchMain_GetVRAMConfig(void) {
    return &sZknSearchVRAMConfig;
}

void ZknSearchMain_InitBG(void) {
    GFL_BGSysCreate(HEAPID_ZUKAN_SEARCH);
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

void ZknSearchMain_ExitBG(void) {
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

void ZknSearchMain_SetMainBGPriority(void) {
    GFL_BGSysSetBGPriority(0, 1);
    GFL_BGSysSetBGPriority(1, 0);
}

void ZknSearchMain_SetListBGPriority(void) {
    GFL_BGSysSetBGPriority(0, 0);
    GFL_BGSysSetBGPriority(1, 1);
}

void ZknSearchMain_LoadGraphics(void) {
    ArcTool *arc;
    u32 chars;
    u16 *screen;
    u32 i;

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
    GFL_G2DIOLoadArcNCLRDefault(arc, 75, PALTYPE_MAIN_BG, 0, 0xc0, HEAPID_ZUKAN_SEARCH);
    GFL_G2DIOLoadArcNCLRDefault(arc, 76, PALTYPE_SUB_BG, 0, 0xa0, HEAPID_ZUKAN_SEARCH);
    GFL_BGSysLoadArcNCGRDynamic(arc, 83, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 83, 1, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 83, 3, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 81, 2, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 83, 5, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 82, 6, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_BGSysLoadArcNCGRStatic(arc, 82, 4, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 97, 2, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 96, 4, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 98, 6, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_ArcToolFree(arc);

    // The touch bar, at the bottom of BG 0
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
    chars = GFL_BGSysLoadArcNCGRDynamic(arc, AppMenuCommon_GetBarBgCharFile(), 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_G2DIOLoadArcNCLRDefault(arc, AppMenuCommon_GetBarBgPaletteFile(), PALTYPE_MAIN_BG, 14 * 0x20, 0x20,
                                HEAPID_ZUKAN_SEARCH);
    loadBGScrToVramByFileNoReserveNegAlign(arc, AppMenuCommon_GetBarScreenFile(), 0, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_BGSysSetScrPaletteNo(0, 0, TOUCHBAR_Y, 32, TOUCHBAR_HEIGHT, 14);
    GFL_BGSysQueueScrLoad(0);
    GFL_ArcToolFree(arc);
    screen = GFL_BGSysIsScrHeapExists(0);
    for (i = 0; i < 32 * TOUCHBAR_HEIGHT; i++) {
        screen[TOUCHBAR_Y * 32 + i] += chars;
    }
    GFL_BGSysQueueScrLoad(0);

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 15 * 0x20, 0x20, HEAPID_ZUKAN_SEARCH);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 15 * 0x20, 0x20, HEAPID_ZUKAN_SEARCH);
}

void ZknSearchMain_InitPaletteFade(ZknSearchWork *wk) {
    wk->paletteFade = PaletteFade_Create(HEAPID_ZUKAN_SEARCH);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_BG, 0x200, HEAPID_ZUKAN_SEARCH);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_BG, 0x200, HEAPID_ZUKAN_SEARCH);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0x200, HEAPID_ZUKAN_SEARCH);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0x200, HEAPID_ZUKAN_SEARCH);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_MAIN_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_SUB_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0, 0x200);
}

void ZknSearchMain_ExitPaletteFade(ZknSearchWork *wk) {
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_BG);
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_BG);
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ);
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ);
    PaletteFade_Free(wk->paletteFade);
}

// Fades everything but the touch bar and its buttons, which slide instead
void ZknSearchMain_StartFade(ZknSearchWork *wk, u8 start, u8 end) {
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_BG, 0xbfff, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_SUB_BG, 0x7fef, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 0xfe1f, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_SUB_OBJ, 0xbfff, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
}

void ZknSearchMain_SetBlend(BOOL enabled) {
    if (enabled == TRUE) {
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG3, GX_BLEND_PLANEMASK_BG2, 16, 6);
        G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2,
                           6, 10);
    } else {
        G2S_BlendNone();
    }
}

// The marks' windows on the top screen show the row under them unblended
void ZknSearchMain_InitWindow(void) {
    G2S_SetWndOBJInsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ,
                             TRUE);
    G2S_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 |
                               GX_WND_PLANEMASK_OBJ,
                           TRUE);
    GXS_SetVisibleWnd(GX_WNDMASK_OW);
}

void ZknSearchMain_ExitWindow(void) {
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

void ZknSearchMain_InitMsg(ZknSearchWork *wk) {
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ZKNSEARCH_MAIN, HEAPID_ZUKAN_SEARCH);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_ZUKAN_SEARCH);
    wk->printQueue = PrintQueue_CreateDefault(HEAPID_ZUKAN_SEARCH);
    wk->strbuf = GFL_StrBufCreate(0x80, HEAPID_ZUKAN_SEARCH);
}

void ZknSearchMain_ExitMsg(ZknSearchWork *wk) {
    GFL_StrBufFree(wk->strbuf);
    PrintQueue_Free(wk->printQueue);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_FontFree(wk->font);
    GFL_MsgDataFree(wk->msgData);
}

void ZknSearchMain_InitBlinkAnm(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);

    wk->blinkAnm = BlinkPalAnm_Create(3 * 0x10, 0x10, 1, HEAPID_ZUKAN_SEARCH);
    BlinkPalAnm_SetPalBufferArcTool(wk->blinkAnm, arc, 75, 2 * 0x10, 3 * 0x10);
    GFL_ArcToolFree(arc);
}

void ZknSearchMain_ExitBlinkAnm(ZknSearchWork *wk) {
    BlinkPalAnm_Free(wk->blinkAnm);
}

void ZknSearchMain_InitBGWinFrame(ZknSearchWork *wk) {
    u16 *screen = GFL_BGSysIsScrHeapExists(0);

    wk->bgWinFrame = BGWinFrame_Create(2, 1, HEAPID_ZUKAN_SEARCH);
    BGWinFrame_InitFrame(wk->bgWinFrame, 0, 0, 32, TOUCHBAR_HEIGHT);
    BGWinFrame_SetScreen(wk->bgWinFrame, 0, &screen[TOUCHBAR_Y * 32]);
    GFL_BGSysFillScrArea(0, 0, 0, TOUCHBAR_Y, 32, TOUCHBAR_HEIGHT, 0);
    GFL_BGSysQueueScrLoad(0);
}

void ZknSearchMain_ExitBGWinFrame(ZknSearchWork *wk) {
    BGWinFrame_Delete(wk->bgWinFrame);
}

void ZknSearchMain_LoadMainScreen(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 95, 0, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 94, 3, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_ArcToolFree(arc);
}

void ZknSearchMain_LoadOrderScreen(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 91, 3, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_ArcToolFree(arc);
}

void ZknSearchMain_LoadInitialScreen(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 90, 3, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_ArcToolFree(arc);
}

void ZknSearchMain_LoadTypeScreen(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 92, 3, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_ArcToolFree(arc);
}

void ZknSearchMain_LoadColorScreen(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 88, 3, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_ArcToolFree(arc);
}

void ZknSearchMain_LoadShapeScreen(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);

    loadBGScrToVramByFileNoReserveNegAlign(arc, 89, 3, 0, 0, FALSE, HEAPID_ZUKAN_SEARCH);
    GFL_ArcToolFree(arc);
}

void ZknSearchMain_LoadLoadingScreen(ZknSearchWork *wk) {
    ArcTool *arc;
    NNSG2dScreenData *screen;
    void *file;

    wk->loadingScreen =
        GFL_HeapAllocate(HEAPID_ZUKAN_SEARCH, 32 * LOADING_HEIGHT * sizeof(u16), FALSE, "zknsearch_main.c", 944);
    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
    file = GFL_G2DIOReadNSCRArc(arc, 93, FALSE, &screen, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
    MIi_CpuCopy16(screen->rawData, wk->loadingScreen, 32 * LOADING_HEIGHT * sizeof(u16));
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
}

void ZknSearchMain_FreeLoadingScreen(ZknSearchWork *wk) {
    GFL_HeapFree(wk->loadingScreen);
}

void ZknSearchMain_ShowLoading(ZknSearchWork *wk) {
    GFL_BGSysLoadScrAreaAll(1, wk->loadingScreen, 0, LOADING_Y, 32, LOADING_HEIGHT);
    GFL_BGSysQueueScrLoad(1);
    ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 15);
    ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_EXIT, 14);
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH) == TRUE) {
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_SHORTCUT, 22);
    } else {
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_SHORTCUT, 21);
    }
    ZknSearchObj_SetBGPriority(wk, ZKNSEARCH_OBJ_RETURN, 1);
    ZknSearchObj_SetBGPriority(wk, ZKNSEARCH_OBJ_EXIT, 1);
    ZknSearchObj_SetBGPriority(wk, ZKNSEARCH_OBJ_SHORTCUT, 1);
}

void ZknSearchMain_HideLoading(ZknSearchWork *wk) {
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_LOADING_BAR, FALSE);
    GFL_BGSysFillScrArea(1, 0, 0, LOADING_Y, 32, LOADING_HEIGHT, 0);
    GFL_BGSysQueueScrLoad(1);
    ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_RETURN, 1);
    ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_EXIT, 0);
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH) == TRUE) {
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_SHORTCUT, 7);
    } else {
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_SHORTCUT, 6);
    }
    ZknSearchObj_SetBGPriority(wk, ZKNSEARCH_OBJ_RETURN, 0);
    ZknSearchObj_SetBGPriority(wk, ZKNSEARCH_OBJ_EXIT, 0);
    ZknSearchObj_SetBGPriority(wk, ZKNSEARCH_OBJ_SHORTCUT, 0);
    ZknSearchBmp_PutStart(wk);
}

void ZknSearchMain_RequestBGOn(ZknSearchWork *wk) {
    wk->bgRequest = ZKNSEARCH_BG_REQUEST_ON;
}

void ZknSearchMain_ClearPage(ZknSearchWork *wk) {
    wk->bgRequest = ZKNSEARCH_BG_REQUEST_OFF;
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

void ZknSearchMain_InitFramePos(ZknSearchWork *wk) {
    u32 i;
    s16 x, y;

    GFL_BGSysMoveBGReq(4, BG_MOVE_SET_Y, TOUCHBAR_SLIDE);
    GFL_BGSysMoveBGReq(7, BG_MOVE_SET_Y, TOUCHBAR_SLIDE);
    BGWinFrame_Put(wk->bgWinFrame, 0, 0, TOUCHBAR_Y + TOUCHBAR_SLIDE / 8);
    BGWinFrame_UpdateMoves(wk->bgWinFrame);
    for (i = ZKNSEARCH_OBJ_RETURN; i <= ZKNSEARCH_OBJ_SHORTCUT; i++) {
        ZknSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
        y += TOUCHBAR_SLIDE;
        ZknSearchObj_SetPos(wk, i, x, y, CLACT_SURFACE_MAIN);
    }
}

void ZknSearchMain_StartFrameMove(ZknSearchWork *wk, s16 speed) {
    wk->frameMoveLeft = TOUCHBAR_SLIDE;
    wk->frameMoveSpeed = speed;
    BGWinFrame_StartMove(wk->bgWinFrame, 0, 0, speed / MATH_ABS(speed), TOUCHBAR_SLIDE / MATH_ABS(speed));
}

BOOL ZknSearchMain_UpdateFrameMove(ZknSearchWork *wk) {
    u32 i;
    s16 x, y;

    if (wk->frameMoveLeft == 0) {
        return FALSE;
    }
    BGWinFrame_UpdateMoves(wk->bgWinFrame);
    GFL_BGSysMoveBGReq(4, BG_MOVE_DOWN, wk->frameMoveSpeed);
    GFL_BGSysMoveBGReq(7, BG_MOVE_DOWN, wk->frameMoveSpeed);
    for (i = ZKNSEARCH_OBJ_RETURN; i <= ZKNSEARCH_OBJ_SHORTCUT; i++) {
        ZknSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
        y += wk->frameMoveSpeed;
        ZknSearchObj_SetPos(wk, i, x, y, CLACT_SURFACE_MAIN);
    }
    wk->frameMoveLeft -= MATH_ABS(wk->frameMoveSpeed);
    return TRUE;
}

u32 ZknSearchMain_ListPosToType(ZknSearchWork *wk, u32 pos) {
    return pos;
}

u32 ZknSearchMain_TypeToListPos(ZknSearchWork *wk, u32 type) {
    return type;
}

u32 ZknSearchMain_GetListPosTypeMsg(ZknSearchWork *wk, u32 pos) {
    return sZknSearchTypeMsgs[pos];
}

u32 ZknSearchMain_GetTypeMsg(ZknSearchWork *wk, u32 type) {
    return sZknSearchTypeMsgs[type];
}

void ZknSearchMain_ShapeToListPos(ZknSearchWork *wk) {
    u32 i;

    if (wk->param->criteria->shape != ZKNSEARCH_NONE) {
        for (i = 0; i < ZKNSEARCH_SHAPE_COUNT; i++) {
            if (wk->param->criteria->shape == sZknSearchShapeOrder[i]) {
                wk->param->criteria->shape = i;
                return;
            }
        }
    }
}

void ZknSearchMain_ListPosToShape(ZknSearchWork *wk) {
    ZukanSearchCriteria *criteria = wk->param->criteria;

    if (criteria->shape != ZKNSEARCH_NONE) {
        criteria->shape = sZknSearchShapeOrder[criteria->shape];
    }
}
