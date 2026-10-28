#include "types.h"
#include "app/pokedex_list_bmp.h"
#include "app/pokedex_list_main.h"
#include "app/pokedex_list_obj.h"
#include "app/ui/frame_list.h"
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
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "pml/personal.h"
#include "pml/species_names.h"
#include "save/pokedex_savedata.h"
#include "system/app_menu_common.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/game_start_3.h"

// The Pokédex's list: its BGs and graphics, its fades, the touch bar that slides in and the list of the species. The
// ROM doesn't name this file; pokedex_list_main.c is a guess after the search screen's pokedex_search_main.c

// The frame of the touch bar: where it is in BG 1's screen and how far it slides
#define TOUCHBAR_Y 21
#define TOUCHBAR_HEIGHT 3
#define TOUCHBAR_SLIDE 48

static void PokedexListMain_VBlankTask(TCB *tcb, void *data);
static void PokedexListMain_HBlankTask(TCB *tcb, void *data);
static void PokedexListMain_ListPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void PokedexListMain_ListSelect(void *work, u32 index, BOOL moved);
static void PokedexListMain_ListScroll(void *work, s16 delta);

static const BGSysVRAMConfig sPokedexListVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const FrameListCallbacks sPokedexListCallbacks = {
    PokedexListMain_ListPrint,
    PokedexListMain_ListSelect,
    PokedexListMain_ListScroll,
};

// The rows, the scroll bar and the page buttons
static const FrameListTouch sPokedexListTouch[] = {
    { { 0, 23, 128, 231 }, FRAMELIST_TOUCH_ITEM },         { { 24, 47, 128, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 48, 71, 128, 231 }, FRAMELIST_TOUCH_ITEM },        { { 72, 95, 128, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 96, 119, 128, 231 }, FRAMELIST_TOUCH_ITEM },       { { 120, 143, 128, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 144, 167, 128, 231 }, FRAMELIST_TOUCH_ITEM },      { { 8, 160, 232, 255 }, FRAMELIST_TOUCH_BAR },
    { { 168, 191, 136, 159 }, FRAMELIST_TOUCH_PAGE_UP },   { { 168, 191, 160, 183 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END, 0, 0, 0 }, FRAMELIST_TOUCH_ITEM },
};

// The list's setup; its count and its work are set when it is created
static const FrameListSetup sPokedexListSetup = {
    { 2, 6, 8, 0, 24, 3, 11, 0, 11, 3, 1, { 24, 12, 8, 6, 4, 3 }, 3, 8, 0 },
    0,
    3,
    0,
    POKEDEX_LIST_ROW_COUNT,
    0,
    sPokedexListTouch,
    &sPokedexListCallbacks,
    NULL,
};

void PokedexListMain_InitVBlank(PokedexListWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(PokedexListMain_VBlankTask, wk, 0);
}

void PokedexListMain_ExitVBlank(PokedexListWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static void PokedexListMain_VBlankTask(TCB *tcb, void *data) {
    PokedexListWork *wk = data;

    GFL_BGSysUpdate();
    ClActSys_VBlank();
    PaletteFade_Transfer(wk->paletteFade);
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

void PokedexListMain_InitHBlank(PokedexListWork *wk) {
    wk->hblankTask = GFL_HBlankTCBAdd(PokedexListMain_HBlankTask, wk, 0);
}

void PokedexListMain_ExitHBlank(PokedexListWork *wk) {
    GFL_TCBRemove(wk->hblankTask);
}

// The blend of the top screen's list changes down the screen, by the height of the list's rows
static void PokedexListMain_HBlankTask(TCB *tcb, void *data) {
    s32 vcount = GX_GetVCount();

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

void PokedexListMain_InitVRAM(void) {
    GFL_BGSysSetVRAMBanks(&sPokedexListVRAMConfig);
}

const BGSysVRAMConfig *PokedexListMain_GetVRAMConfig(void) {
    return &sPokedexListVRAMConfig;
}

void PokedexListMain_InitBG(void) {
    GFL_BGSysCreate(HEAPID_POKEDEX_LIST);
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
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            0,
            0,
        };

        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(0);
        GFL_BGSysClearCharCore(0, 0x20, 0, HEAPID_POKEDEX_LIST);
        GFL_BGSysQueueScrLoad(0);
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
            GX_BG_CHARBASE(0x08000),
            0x4000,
            GX_BG_EXTPLTT_01,
            1,
            0,
            0,
        };

        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
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

        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(2);
        GFL_BGSysQueueScrLoad(2);
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
            GX_BG_CHARBASE(0x18000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            0,
            0,
        };

        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
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
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            0,
            0,
        };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(4);
        GFL_BGSysClearCharCore(4, 0x20, 0, HEAPID_POKEDEX_LIST);
        GFL_BGSysQueueScrLoad(4);
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

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
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

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(6);
        GFL_BGSysQueueScrLoad(6);
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
            GX_BG_CHARBASE(0x18000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            0,
            0,
        };

        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
}

void PokedexListMain_ExitBG(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysFree();
}

void PokedexListMain_LoadGraphics(void) {
    ArcTool *arc;

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_LIST);
    GFL_G2DIOLoadArcNCLRDefault(arc, 49, PALTYPE_MAIN_BG, 0, 0x80, HEAPID_POKEDEX_LIST);
    GFL_G2DIOLoadArcNCLRDefault(arc, 51, PALTYPE_SUB_BG, 0, 0xa0, HEAPID_POKEDEX_LIST);
    GFL_BGSysLoadArcNCGRStatic(arc, 60, 2, 0, 0, FALSE, HEAPID_POKEDEX_LIST);
    GFL_BGSysLoadArcNCGRStatic(arc, 58, 3, 0, 0, FALSE, HEAPID_POKEDEX_LIST);
    GFL_BGSysLoadArcNCGRStatic(arc, 61, 6, 0, 0, FALSE, HEAPID_POKEDEX_LIST);
    GFL_BGSysLoadArcNCGRStatic(arc, 59, 7, 0, 0, FALSE, HEAPID_POKEDEX_LIST);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 64, 3, 0, 0, FALSE, HEAPID_POKEDEX_LIST);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 71, 5, 0, 0, FALSE, HEAPID_POKEDEX_LIST);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 65, 7, 0, 0, FALSE, HEAPID_POKEDEX_LIST);
    GFL_ArcToolFree(arc);

    // The touch bar, at the bottom of BG 1
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_POKEDEX_LIST);
    GFL_BGSysLoadArcNCGRDynamic(arc, AppMenuCommon_GetBarBgCharFile(), 1, 0, FALSE, HEAPID_POKEDEX_LIST);
    GFL_G2DIOLoadArcNCLRDefault(arc, AppMenuCommon_GetBarBgPaletteFile(), PALTYPE_MAIN_BG, 14 * 0x20, 0x20,
                                HEAPID_POKEDEX_LIST);
    loadBGScrToVramByFileNoReserveNegAlign(arc, AppMenuCommon_GetBarScreenFile(), 1, 0, 0, FALSE, HEAPID_POKEDEX_LIST);
    GFL_BGSysSetScrPaletteNo(1, 0, TOUCHBAR_Y, 32, TOUCHBAR_HEIGHT, 14);
    GFL_BGSysQueueScrLoad(1);
    GFL_ArcToolFree(arc);

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 15 * 0x20, 0x20, HEAPID_POKEDEX_LIST);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 15 * 0x20, 0x20, HEAPID_POKEDEX_LIST);
}

void PokedexListMain_InitPaletteFade(PokedexListWork *wk) {
    wk->paletteFade = PaletteFade_Create(HEAPID_POKEDEX_LIST);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_BG, 0x200, HEAPID_POKEDEX_LIST);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_BG, 0x200, HEAPID_POKEDEX_LIST);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0x200, HEAPID_POKEDEX_LIST);
    PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0x200, HEAPID_POKEDEX_LIST);
}

void PokedexListMain_ExitPaletteFade(PokedexListWork *wk) {
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_BG);
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_BG);
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ);
    PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ);
    PaletteFade_Free(wk->paletteFade);
}

void PokedexListMain_StartFade(PokedexListWork *wk, u8 start, u8 end) {
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_BG, 0xbfff, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_SUB_BG, 0xffef, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 0xf87f, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(wk->paletteFade, 1 << PALFADE_BUFFER_SUB_OBJ, 0xbfff, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
}

void PokedexListMain_SetBlend(void) {
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3, 6, 10);
}

// The icons' windows on the top screen show the row under them unblended
void PokedexListMain_InitWindow(void) {
    G2S_SetWndOBJInsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ,
                             TRUE);
    G2S_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 |
                               GX_WND_PLANEMASK_OBJ,
                           TRUE);
    GXS_SetVisibleWnd(GX_WNDMASK_OW);
}

void PokedexListMain_ExitWindow(void) {
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

void PokedexListMain_InitMsg(PokedexListWork *wk) {
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_GAME_START_3, HEAPID_POKEDEX_LIST);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_POKEDEX_LIST);
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_POKEDEX_LIST);
    wk->printQueue = PrintQueue_CreateDefault(HEAPID_POKEDEX_LIST);
    wk->strbuf = GFL_StrBufCreate(0x80, HEAPID_POKEDEX_LIST);
}

void PokedexListMain_ExitMsg(PokedexListWork *wk) {
    GFL_StrBufFree(wk->strbuf);
    PrintQueue_Free(wk->printQueue);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_FontFree(wk->font);
    GFL_MsgDataFree(wk->msgData);
}

void PokedexListMain_LoadRegionalNumbers(PokedexListWork *wk) {
    wk->regionalNumbers = PML_PersonalLoadRegionalDexTable(HEAPID_POKEDEX_LIST, NULL);
}

void PokedexListMain_FreeRegionalNumbers(PokedexListWork *wk) {
    GFL_HeapFree(wk->regionalNumbers);
}

void PokedexListMain_InitFramePos(PokedexListWork *wk) {
    u32 i;
    s16 x, y;

    GFL_BGSysMoveBGReq(5, BG_MOVE_SET_Y, TOUCHBAR_SLIDE);
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_Y, -TOUCHBAR_SLIDE);
    for (i = POKEDEX_LIST_OBJ_RETURN; i <= POKEDEX_LIST_OBJ_START; i++) {
        PokedexListObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
        y += TOUCHBAR_SLIDE;
        PokedexListObj_SetPos(wk, i, x, y, CLACT_SURFACE_MAIN);
    }
}

void PokedexListMain_StartFrameMove(PokedexListWork *wk, s16 speed) {
    wk->frameMoveLeft = TOUCHBAR_SLIDE;
    wk->frameMoveSpeed = speed;
}

BOOL PokedexListMain_UpdateFrameMove(PokedexListWork *wk) {
    u32 i;
    s16 x, y;

    if (wk->frameMoveLeft == 0) {
        return FALSE;
    }
    GFL_BGSysMoveBGReq(5, BG_MOVE_DOWN, wk->frameMoveSpeed);
    GFL_BGSysMoveBGReq(1, BG_MOVE_UP, wk->frameMoveSpeed);
    for (i = POKEDEX_LIST_OBJ_RETURN; i <= POKEDEX_LIST_OBJ_START; i++) {
        PokedexListObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
        y += wk->frameMoveSpeed;
        PokedexListObj_SetPos(wk, i, x, y, CLACT_SURFACE_MAIN);
    }
    wk->frameMoveLeft -= MATH_ABS(wk->frameMoveSpeed);
    return TRUE;
}

void PokedexListMain_InitList(PokedexListWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
    u16 last = PokeDex_GetLastSpecies(wk->param->pokedex);
    FrameListSetup setup = sPokedexListSetup;
    u32 i;

    setup.count = POKEDEX_LIST_SPECIES_COUNT;
    setup.work = wk;
    // The cursor goes on the species that the Pokédex was last on
    for (i = 0; i < wk->param->count; i++) {
        if (last == wk->param->list[i]) {
            if (i > wk->param->count - POKEDEX_LIST_ROW_COUNT) {
                setup.cursorPos = i - (wk->param->count - POKEDEX_LIST_ROW_COUNT);
                setup.scroll = i - setup.cursorPos;
            } else if (i < POKEDEX_LIST_ROW_COUNT) {
                setup.cursorPos = i;
            } else {
                setup.scroll = i;
            }
            break;
        }
    }
    wk->frameList = FrameList_Create(&setup, HEAPID_POKEDEX_LIST);
    if (wk->param->count < POKEDEX_LIST_ROW_COUNT) {
        wk->listRows = wk->param->count;
    } else {
        wk->listRows = POKEDEX_LIST_ROW_COUNT;
    }
    FrameList_LoadScreen(wk->frameList, arc, 68, FALSE, 0);
    FrameList_LoadScreen(wk->frameList, arc, 69, FALSE, 1);
    FrameList_LoadScreen(wk->frameList, arc, 70, FALSE, 2);
    FrameList_LoadCursorPalette(wk->frameList, arc, 49, 2, 3);
    GFL_ArcToolFree(arc);

    for (i = 0; i < wk->param->count; i++) {
        if (PokeDex_IsCaught(wk->param->pokedex, wk->param->list[i]) == TRUE) {
            StrBuf *name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, wk->param->list[i]);

            wk->names[i] = GFL_StrBufClone(name, HEAPID_POKEDEX_LIST);
            GFL_StrBufFree(name);
            FrameList_AddItem(wk->frameList, 0, wk->param->list[i] | POKEDEX_LIST_ITEM_CAUGHT);
        } else if (PokeDex_IsSeen(wk->param->pokedex, wk->param->list[i]) == TRUE) {
            StrBuf *name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, wk->param->list[i]);

            wk->names[i] = GFL_StrBufClone(name, HEAPID_POKEDEX_LIST);
            GFL_StrBufFree(name);
            FrameList_AddItem(wk->frameList, 1, wk->param->list[i] | POKEDEX_LIST_ITEM_SEEN);
        } else {
            wk->names[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, GameStart3_Text_Empty_3);
            FrameList_AddItem(wk->frameList, 2, wk->param->list[i]);
        }
    }
}

void PokedexListMain_ExitList(PokedexListWork *wk) {
    u32 i;

    FrameList_Free(wk->frameList);
    for (i = 0; i < POKEDEX_LIST_SPECIES_COUNT; i++) {
        if (wk->names[i] != NULL) {
            GFL_StrBufFree(wk->names[i]);
        }
    }
}

static void PokedexListMain_ListPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    PokedexListWork *wk = work;
    u32 value = FrameList_GetValue(wk->frameList, index);
    u32 species = value & POKEDEX_LIST_ITEM_SPECIES_MASK;

    PokedexListBmp_PrintItem(wk, window, wk->names[index], species);
    if ((value & POKEDEX_LIST_ITEM_STATE_MASK) >> POKEDEX_LIST_ITEM_STATE_SHIFT != 0) {
        PokedexListObj_PutIcon(wk, species, y, firstBG);
    }
}

static void PokedexListMain_ListSelect(void *work, u32 index, BOOL moved) {
    PokedexListWork *wk = work;
    u32 value = FrameList_GetValue(wk->frameList, index);

    if ((value & POKEDEX_LIST_ITEM_STATE_MASK) >> POKEDEX_LIST_ITEM_STATE_SHIFT != 0) {
        PokedexListObj_PutMon(wk, value & POKEDEX_LIST_ITEM_SPECIES_MASK);
    } else {
        PokedexListObj_PutMon(wk, 0);
    }
    PokedexListObj_SelectIcon(wk, index - FrameList_GetScroll(wk->frameList));
}

static void PokedexListMain_ListScroll(void *work, s16 delta) {
    PokedexListWork *wk = work;

    PokedexListObj_ScrollIcons(wk, -delta);
    PokedexListObj_SelectIcon(wk, FrameList_GetCursor(wk->frameList));
    PokedexListObj_SetPageButtons(wk, TRUE);
}
