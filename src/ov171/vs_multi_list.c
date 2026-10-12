#include "types.h"
#include "battle/vs_multi_list.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "system/app_menu_common.h"
#include "system/bgwinfrm.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/egg_demo.h"

// vs_multi_list.c: the list of both teams' Pokémon that a multi battle shows before it starts. The Pokémon of the
// teams' first three slots show on two boards that slide in from the sides, with their icons, names, levels and HP

// The steps of the screen
enum {
    STATE_SETUP,
    STATE_TEARDOWN,
    STATE_WAIT_WIPE,
    STATE_WAIT_FRAMES,
    STATE_SLIDE,
    STATE_DONE,
};

// The steps of the slide
enum {
    SLIDE_START,
    SLIDE_MOVE,
    SLIDE_END,
};

#define POKEMON_COUNT 6
#define BOARD_POKEMON_COUNT 3
#define FRAME_COUNT 12
#define ACTOR_COUNT 24
#define CHAR_RES_COUNT 9
#define PLTT_RES_COUNT 4
#define CELL_RES_COUNT 4
#define BG_HEAP HEAPID_TAIL(HEAPID_VS_MULTI_LIST)

typedef struct {
    ClActorSetup setup;
    u32 charIndex;
    u32 palIndex;
    u32 cellIndex;
} VsMultiListActor;

typedef struct {
    VsMultiListParam *param;
    TCB *vblankTcb;
    BGWinFrame *frames;
    Font *font;
    Font *smallFont;
    MsgData *msgData;
    WordSet *wordSet;
    PrintQueue *printQueue;
    PrintWindow printWindows[POKEMON_COUNT];
    ClActUnit *clUnit;
    ClActor *actors[ACTOR_COUNT];
    u32 charRes[CHAR_RES_COUNT];
    u32 palRes[PLTT_RES_COUNT];
    u32 cellRes[CELL_RES_COUNT];
    PartyPkm *pkm[POKEMON_COUNT];
    int slideStep;
    int waitNext;
    int wipeNext;
    int wait;
} VsMultiListWork;

static BOOL VsMultiList_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL VsMultiList_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL VsMultiList_Exit(GameProc *proc, u32 *state, void *param, void *work);
static int VsMultiList_StateSetup(VsMultiListWork *wk);
static int VsMultiList_StateTeardown(VsMultiListWork *wk);
static int VsMultiList_StateWaitWipe(VsMultiListWork *wk);
static int VsMultiList_StateWaitFrames(VsMultiListWork *wk);
static int VsMultiList_StateSlide(VsMultiListWork *wk);
static int VsMultiList_StartWipeIn(VsMultiListWork *wk, int next);
static int VsMultiList_StartWipeOut(VsMultiListWork *wk, int next);
static int VsMultiList_StartWait(VsMultiListWork *wk, int frames, int next);
static void VsMultiList_InitVRAM(void);
static void VsMultiList_InitBG(void);
static void VsMultiList_FreeBG(void);
static void VsMultiList_LoadBG(VsMultiListWork *wk);
static void VsMultiList_InitText(VsMultiListWork *wk);
static void VsMultiList_FreeText(VsMultiListWork *wk);
static void VsMultiList_AddVBlankTask(VsMultiListWork *wk);
static void VsMultiList_RemoveVBlankTask(VsMultiListWork *wk);
static void VsMultiList_VBlank(TCB *tcb, void *data);
static void VsMultiList_InitParties(VsMultiListWork *wk);
static void VsMultiList_CreateWindows(VsMultiListWork *wk);
static void VsMultiList_FreeWindows(VsMultiListWork *wk);
static void VsMultiList_FlushText(VsMultiListWork *wk);
static void VsMultiList_DrawName(VsMultiListWork *wk, u32 index);
static void VsMultiList_DrawLevel(VsMultiListWork *wk, u32 index);
static void VsMultiList_DrawHP(VsMultiListWork *wk, u32 index);
static void VsMultiList_DrawHPBar(VsMultiListWork *wk, u32 index);
static void VsMultiList_CreateObjs(VsMultiListWork *wk);
static void VsMultiList_FreeObjs(VsMultiListWork *wk);
static void VsMultiList_InitClActSys(VsMultiListWork *wk);
static void VsMultiList_FreeClActSys(void);
static void VsMultiList_LoadObjRes(VsMultiListWork *wk);
static void VsMultiList_FreeObjRes(VsMultiListWork *wk);
static void VsMultiList_CreateActors(VsMultiListWork *wk);
static void VsMultiList_FreeActors(VsMultiListWork *wk);
static void VsMultiList_MoveActor(VsMultiListWork *wk, u32 index, s16 dx);
static void VsMultiList_MoveActors(VsMultiListWork *wk, u32 first, s16 dx);
static void VsMultiList_CreateFrames(VsMultiListWork *wk);
static void VsMultiList_FreeFrames(VsMultiListWork *wk);

// Declared in an order that gives the original layout of the data (tools/decomp/rodata_order.py)

const GameProcFunctions VS_MULTI_LIST_PROC_FUNCTIONS = {
    VsMultiList_Init,
    VsMultiList_Main,
    VsMultiList_Exit,
};

static int (*const sVsMultiListStateFuncs[])(VsMultiListWork *wk) = {
    VsMultiList_StateSetup,      VsMultiList_StateTeardown, VsMultiList_StateWaitWipe,
    VsMultiList_StateWaitFrames, VsMultiList_StateSlide,
};

static const BGSysVRAMConfig sVsMultiListVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// The 24 actors: for each of the six Pokémon its icon, its item's icon and two more
static const VsMultiListActor sVsMultiListActors[ACTOR_COUNT] = {
    { { 16, 16, 1, 1, 1 }, 0, 0, 0 },  { { 16, 64, 1, 1, 1 }, 1, 0, 0 },  { { 16, 112, 1, 1, 1 }, 2, 0, 0 },
    { { 144, 24, 1, 1, 1 }, 3, 0, 0 }, { { 144, 72, 1, 1, 1 }, 4, 0, 0 }, { { 144, 120, 1, 1, 1 }, 5, 0, 0 },
    { { 24, 24, 0, 0, 1 }, 6, 1, 1 },  { { 24, 72, 0, 0, 1 }, 6, 1, 1 },  { { 24, 120, 0, 0, 1 }, 6, 1, 1 },
    { { 152, 32, 0, 0, 1 }, 6, 1, 1 }, { { 152, 80, 0, 0, 1 }, 6, 1, 1 }, { { 152, 128, 0, 0, 1 }, 6, 1, 1 },
    { { 28, 40, 0, 2, 1 }, 7, 2, 2 },  { { 28, 88, 0, 2, 1 }, 7, 2, 2 },  { { 28, 136, 0, 2, 1 }, 7, 2, 2 },
    { { 156, 48, 0, 2, 1 }, 7, 2, 2 }, { { 156, 96, 0, 2, 1 }, 7, 2, 2 }, { { 156, 144, 0, 2, 1 }, 7, 2, 2 },
    { { 87, 28, 0, 2, 1 }, 8, 3, 3 },  { { 87, 76, 0, 2, 1 }, 8, 3, 3 },  { { 87, 124, 0, 2, 1 }, 8, 3, 3 },
    { { 215, 36, 0, 2, 1 }, 8, 3, 3 }, { { 215, 84, 0, 2, 1 }, 8, 3, 3 }, { { 215, 132, 0, 2, 1 }, 8, 3, 3 },
};

static BOOL VsMultiList_Init(GameProc *proc, u32 *state, void *param, void *work) {
    VsMultiListWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_VS_MULTI_LIST, 0x18000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(VsMultiListWork), HEAPID_VS_MULTI_LIST);
    sys_memset(wk, 0, sizeof(VsMultiListWork));
    wk->param = param;
    return TRUE;
}

static BOOL VsMultiList_Main(GameProc *proc, u32 *state, void *param, void *work) {
    VsMultiListWork *wk = work;

    *state = sVsMultiListStateFuncs[*state](wk);
    if (*state == STATE_DONE) {
        return TRUE;
    }
    BGWinFrame_UpdateMoves(wk->frames);
    VsMultiList_FlushText(wk);
    func_0204b794();
    return FALSE;
}

static BOOL VsMultiList_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_VS_MULTI_LIST);
    return TRUE;
}

static int VsMultiList_StateSetup(VsMultiListWork *wk) {
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    reg_G2_BLDCNT = 0;
    reg_G2S_DB_BLDCNT = 0;
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_MAIN_SUB);
    VsMultiList_InitParties(wk);
    VsMultiList_InitVRAM();
    VsMultiList_InitBG();
    VsMultiList_LoadBG(wk);
    VsMultiList_InitText(wk);
    VsMultiList_CreateWindows(wk);
    VsMultiList_CreateObjs(wk);
    VsMultiList_CreateFrames(wk);
    func_02042ba8(TRUE, HEAPID_VS_MULTI_LIST);
    GFL_BGSysSetBGEnabledA(0x17, TRUE);
    VsMultiList_AddVBlankTask(wk);
    return VsMultiList_StartWipeIn(wk, VsMultiList_StartWait(wk, 128, STATE_SLIDE));
}

static int VsMultiList_StateTeardown(VsMultiListWork *wk) {
    VsMultiList_RemoveVBlankTask(wk);
    GFL_BGSysSetBGEnabledA(0x17, FALSE);
    VsMultiList_FreeFrames(wk);
    VsMultiList_FreeObjs(wk);
    VsMultiList_FreeWindows(wk);
    VsMultiList_FreeText(wk);
    VsMultiList_FreeBG();
    return STATE_DONE;
}

static int VsMultiList_StateWaitWipe(VsMultiListWork *wk) {
    if (GFL_WipeIsFinished() == TRUE) {
        return wk->wipeNext;
    }
    return STATE_WAIT_WIPE;
}

static int VsMultiList_StateWaitFrames(VsMultiListWork *wk) {
    if (wk->wait == 0) {
        return wk->waitNext;
    }
    wk->wait--;
    return STATE_WAIT_FRAMES;
}

// Slides the boards in from the sides, waits, and slides the screen out
static int VsMultiList_StateSlide(VsMultiListWork *wk) {
    u32 i;

    switch (wk->slideStep) {
    case SLIDE_START:
        if (!func_02021c0c(wk->printQueue)) {
            break;
        }
        if (wk->param->flipped == 0) {
            for (i = 0; i < BOARD_POKEMON_COUNT; i++) {
                BGWinFrame_StartMove(wk->frames, i + 3, -1, 0, 16);
                BGWinFrame_StartMove(wk->frames, i + 9, -1, 0, 16);
            }
        } else {
            for (i = 0; i < BOARD_POKEMON_COUNT; i++) {
                BGWinFrame_StartMove(wk->frames, i, 1, 0, 16);
                BGWinFrame_StartMove(wk->frames, i + 6, 1, 0, 16);
            }
        }
        GFL_SndSEPlay(SEQ_SE_SYS_03);
        wk->slideStep++;
        // fall through
    case SLIDE_MOVE:
        if (wk->param->flipped == 0) {
            if (!BGWinFrame_IsMoving(wk->frames, 3)) {
                wk->slideStep++;
                GFL_SndSEPlay(SEQ_SE_ROTATION_B);
                return VsMultiList_StartWait(wk, 128, STATE_SLIDE);
            }
            VsMultiList_MoveActors(wk, 3, -8);
        } else {
            if (!BGWinFrame_IsMoving(wk->frames, 0)) {
                wk->slideStep++;
                GFL_SndSEPlay(SEQ_SE_ROTATION_B);
                return VsMultiList_StartWait(wk, 128, STATE_SLIDE);
            }
            VsMultiList_MoveActors(wk, 0, 8);
        }
        break;
    case SLIDE_END:
        return VsMultiList_StartWipeOut(wk, STATE_TEARDOWN);
    }
    return STATE_SLIDE;
}

static int VsMultiList_StartWipeIn(VsMultiListWork *wk, int next) {
    GFL_WipeSet(0, 1, 1, 0, 6, TRUE, HEAPID_VS_MULTI_LIST);
    wk->wipeNext = next;
    return STATE_WAIT_WIPE;
}

static int VsMultiList_StartWipeOut(VsMultiListWork *wk, int next) {
    GFL_WipeSet(0, 0, 0, 0, 6, TRUE, HEAPID_VS_MULTI_LIST);
    wk->wipeNext = next;
    return STATE_WAIT_WIPE;
}

static int VsMultiList_StartWait(VsMultiListWork *wk, int frames, int next) {
    wk->wait = frames;
    wk->waitNext = next;
    return STATE_WAIT_FRAMES;
}

static void VsMultiList_InitVRAM(void) {
    GFL_BGSysSetVRAMBanks(&sVsMultiListVRAMConfig);
}

static void VsMultiList_InitBG(void) {
    GFL_BGSysCreate(HEAPID_VS_MULTI_LIST);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&lcdConfig);
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
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(0);
        GFL_BGSysClearCharCore(0, BGSYS_TILE_SIZE_16, 0, HEAPID_VS_MULTI_LIST);
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
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
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
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
    }
}

static void VsMultiList_FreeBG(void) {
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysFree();
}

static void VsMultiList_LoadBG(VsMultiListWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(99, BG_HEAP);

    GFL_BGSysLoadArcNCGRStatic(arc, 18, 2, 0, 0, TRUE, BG_HEAP);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0, 2, 0, 0, TRUE, BG_HEAP);
    // The palette is file 20 of the archive in Black 2 and file 19 in White 2
#ifdef BLACK2
    GFL_G2DIOLoadArcNCLRDefault(arc, 20, 0, 0, 0, BG_HEAP);
#else
    GFL_G2DIOLoadArcNCLRDefault(arc, 19, 0, 0, 0, BG_HEAP);
#endif
    loadBGScrToVramByFileNoReserveNegAlign(arc, 16, 1, 0, 0x800, TRUE, BG_HEAP);
    GFL_ArcToolFree(arc);
    GFL_BGSysResetStdPalette(4, 0);
}

static void VsMultiList_InitText(VsMultiListWork *wk) {
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EGG_DEMO, HEAPID_VS_MULTI_LIST);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_VS_MULTI_LIST);
    wk->smallFont = GFL_FontCreate(ARCID_FONT, 3, 0, FALSE, HEAPID_VS_MULTI_LIST);
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_VS_MULTI_LIST);
    wk->printQueue = func_020219a8(0x800, HEAPID_VS_MULTI_LIST);
}

static void VsMultiList_FreeText(VsMultiListWork *wk) {
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->font);
    GFL_FontFree(wk->smallFont);
    GFL_WordSetSystemFree(wk->wordSet);
    func_02021a18(wk->printQueue);
}

static void VsMultiList_AddVBlankTask(VsMultiListWork *wk) {
    wk->vblankTcb = GFL_VBlankTCBAdd(VsMultiList_VBlank, wk, 0);
}

static void VsMultiList_RemoveVBlankTask(VsMultiListWork *wk) {
    GFL_TCBRemove(wk->vblankTcb);
}

static void VsMultiList_VBlank(TCB *tcb, void *data) {
    GFL_BGSysUpdate();
    func_0204b7c8();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

// Takes the first three Pokémon of each team, or none for a slot that the team has no Pokémon for
static void VsMultiList_InitParties(VsMultiListWork *wk) {
    PokeParty *party0;
    PokeParty *party1;
    u32 i;

    if (wk->param->flipped == 0) {
        party0 = wk->param->party0;
        party1 = wk->param->party1;
    } else {
        party0 = wk->param->party1;
        party1 = wk->param->party0;
    }
    for (i = 0; i < BOARD_POKEMON_COUNT; i++) {
        if (PokeParty_GetPkmCount(party0) > i) {
            wk->pkm[i] = PokeParty_GetPkm(party0, i);
        } else {
            wk->pkm[i] = NULL;
        }
        if (PokeParty_GetPkmCount(party1) > i) {
            wk->pkm[i + BOARD_POKEMON_COUNT] = PokeParty_GetPkm(party1, i);
        } else {
            wk->pkm[i + BOARD_POKEMON_COUNT] = NULL;
        }
    }
}

static void VsMultiList_CreateWindows(VsMultiListWork *wk) {
    u32 i;

    BmpWin_InitAllocator(HEAPID_VS_MULTI_LIST);
    for (i = 0; i < POKEMON_COUNT; i++) {
        wk->printWindows[i].window = BmpWin_CreateDynamic(0, 0, 0, 15, 5, 9, TRUE);
    }
    for (i = 0; i < POKEMON_COUNT; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(wk->printWindows[i].window), 0);
        if (wk->pkm[i] != NULL) {
            VsMultiList_DrawName(wk, i);
            VsMultiList_DrawLevel(wk, i);
            VsMultiList_DrawHP(wk, i);
            VsMultiList_DrawHPBar(wk, i);
        } else {
            BmpWin_FlushChar(wk->printWindows[i].window);
        }
    }
}

static void VsMultiList_FreeWindows(VsMultiListWork *wk) {
    u32 i;

    for (i = 0; i < POKEMON_COUNT; i++) {
        BmpWin_Free(wk->printWindows[i].window);
    }
    BmpWin_FreeAllocator();
}

static void VsMultiList_FlushText(VsMultiListWork *wk) {
    u32 i;

    func_02021a3c(wk->printQueue);
    for (i = 0; i < POKEMON_COUNT; i++) {
        PrintWindow_Flush(&wk->printWindows[i], wk->printQueue);
    }
}

// Prints the Pokémon's nickname and, unless it is an egg, its sex
static void VsMultiList_DrawName(VsMultiListWork *wk, u32 index) {
    BmpWin *window = wk->printWindows[index].window;
    StrBuf *name = GFL_StrBufCreate(12, BG_HEAP);
    StrBuf *template = GFL_MsgDataLoadStrbufNew(wk->msgData, EggDemo_Text_Empty);

    loadPokemonNicknameToStrbuf(wk->wordSet, 0, wk->pkm[index]);
    GFL_WordSetFormatStrbuf(wk->wordSet, name, template);
    PrintWindow_Print(&wk->printWindows[index], wk->printQueue, 32, 7, name, wk->font, PRINT_COLOR(15, 14, 0));
    GFL_StrBufFree(template);
    GFL_StrBufFree(name);
    if (PokeParty_GetParam(wk->pkm[index], PKM_PARAM_SHOW_SEX, NULL) == 1 &&
        PokeParty_GetParam(wk->pkm[index], PKM_PARAM_IS_EGG, NULL) == 0) {
        u32 sex = PokeParty_GetSex(wk->pkm[index]);

        if (sex == GENDER_MALE) {
            StrBuf *symbol = GFL_MsgDataLoadStrbufNew(wk->msgData, EggDemo_Text_Empty_16);

            PrintWindow_Print(&wk->printWindows[index], wk->printQueue,
                              BmpWin_GetSizeX(window) * 8 - GFL_FontGetBlockWidth(symbol, wk->font, 0), 7, symbol,
                              wk->font, PRINT_COLOR(10, 11, 0));
            GFL_StrBufFree(symbol);
        } else if (sex == GENDER_FEMALE) {
            StrBuf *symbol = GFL_MsgDataLoadStrbufNew(wk->msgData, EggDemo_Text_Empty_17);

            PrintWindow_Print(&wk->printWindows[index], wk->printQueue,
                              BmpWin_GetSizeX(window) * 8 - GFL_FontGetBlockWidth(symbol, wk->font, 0), 7, symbol,
                              wk->font, PRINT_COLOR(12, 13, 0));
            GFL_StrBufFree(symbol);
        }
    }
}

static void VsMultiList_DrawLevel(VsMultiListWork *wk, u32 index) {
    StrBuf *text = GFL_StrBufCreate(32, BG_HEAP);
    StrBuf *template = GFL_MsgDataLoadStrbufNew(wk->msgData, EggDemo_Text_L);

    WordSetNumber(wk->wordSet, 0, PokeParty_GetParam(wk->pkm[index], PKM_PARAM_LEVEL, NULL), 3, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, text, template);
    PrintWindow_Print(&wk->printWindows[index], wk->printQueue, 8, 32, text, wk->smallFont, PRINT_COLOR(15, 14, 0));
    GFL_StrBufFree(template);
    GFL_StrBufFree(text);
}

// Prints the Pokémon's HP, a slash and its maximum HP, around the middle of the window
static void VsMultiList_DrawHP(VsMultiListWork *wk, u32 index) {
    StrBuf *text = GFL_StrBufCreate(32, BG_HEAP);
    StrBuf *slash = GFL_MsgDataLoadStrbufNew(wk->msgData, EggDemo_Text_Empty_8);
    u8 slashWidth = GFL_FontGetBlockWidth(slash, wk->smallFont, 0);
    u8 x = 92 - slashWidth / 2;
    StrBuf *number;

    PrintWindow_Print(&wk->printWindows[index], wk->printQueue, x, 32, slash, wk->smallFont, PRINT_COLOR(15, 14, 0));
    GFL_StrBufFree(slash);
    number = GFL_MsgDataLoadStrbufNew(wk->msgData, EggDemo_Text_Empty_9);
    WordSetNumber(wk->wordSet, 0, PokeParty_GetParam(wk->pkm[index], PKM_PARAM_HP, NULL), 3, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, text, number);
    PrintWindow_Print(&wk->printWindows[index], wk->printQueue, x - GFL_FontGetBlockWidth(text, wk->smallFont, 0), 32,
                      text, wk->smallFont, PRINT_COLOR(15, 14, 0));
    GFL_StrBufFree(number);
    number = GFL_MsgDataLoadStrbufNew(wk->msgData, EggDemo_Text_Empty_9);
    WordSetNumber(wk->wordSet, 0, PokeParty_GetParam(wk->pkm[index], PKM_PARAM_MAX_HP, NULL), 3, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, text, number);
    PrintWindow_Print(&wk->printWindows[index], wk->printQueue, x + slashWidth, 32, text, wk->smallFont,
                      PRINT_COLOR(15, 14, 0));
    GFL_StrBufFree(number);
    GFL_StrBufFree(text);
}

// Draws the bar that the HP is shown in, two lines of pixels
static void VsMultiList_DrawHPBar(VsMultiListWork *wk, u32 index) {
    GFL_BitmapFillArea(BmpWin_GetBitmap(wk->printWindows[index].window), 64, 27, 48, 1, 1);
    GFL_BitmapFillArea(BmpWin_GetBitmap(wk->printWindows[index].window), 64, 28, 48, 1, 2);
}

static void VsMultiList_CreateObjs(VsMultiListWork *wk) {
    VsMultiList_InitClActSys(wk);
    VsMultiList_LoadObjRes(wk);
    VsMultiList_CreateActors(wk);
}

static void VsMultiList_FreeObjs(VsMultiListWork *wk) {
    VsMultiList_FreeActors(wk);
    VsMultiList_FreeObjRes(wk);
    VsMultiList_FreeClActSys();
}

static void VsMultiList_InitClActSys(VsMultiListWork *wk) {
    ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 9, 4, 4, 0, 16, 16 };

    ClActSys_Create(&setup, &sVsMultiListVRAMConfig, HEAPID_VS_MULTI_LIST);
}

static void VsMultiList_FreeClActSys(void) {
    func_0204b758();
}

static void VsMultiList_LoadObjRes(VsMultiListWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, BG_HEAP);
    ArcTool *uiArc;
    u32 *charRes = wk->charRes;
    u16 i;

    for (i = 0; i < POKEMON_COUNT; i++) {
        u16 icon;

        if (wk->pkm[i] != NULL) {
            icon = PokeIcon_GetFileOfPkm(func_0201d624(wk->pkm[i]));
        } else {
            icon = PokeParty_GetIconIndex(0, 0, 0, FALSE);
        }
        charRes[i] = func_0204b81c(arc, icon, FALSE, CLACT_VRAM_MAIN, HEAPID_VS_MULTI_LIST);
    }
    wk->palRes[0] = func_0204bc48(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_MAIN, 0, HEAPID_VS_MULTI_LIST);
    wk->cellRes[0] = func_0204bde0(arc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_SubEng(), HEAPID_VS_MULTI_LIST);
    GFL_ArcToolFree(arc);
    uiArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), BG_HEAP);
    wk->charRes[7] = func_0204b81c(uiArc, 12, FALSE, CLACT_VRAM_MAIN, HEAPID_VS_MULTI_LIST);
    wk->palRes[2] = func_0204bba0(uiArc, 11, CLACT_VRAM_MAIN, 96, HEAPID_VS_MULTI_LIST);
    wk->cellRes[2] = func_0204bde0(uiArc, 15, 18, HEAPID_VS_MULTI_LIST);
    wk->charRes[6] = func_0204b81c(uiArc, 65, FALSE, CLACT_VRAM_MAIN, HEAPID_VS_MULTI_LIST);
    wk->palRes[1] = func_0204bba0(uiArc, 66, CLACT_VRAM_MAIN, 128, HEAPID_VS_MULTI_LIST);
    wk->cellRes[1] = func_0204bde0(uiArc, 69, 72, HEAPID_VS_MULTI_LIST);
    wk->charRes[8] = func_0204b81c(uiArc, func_0202d958(2), FALSE, CLACT_VRAM_MAIN, HEAPID_VS_MULTI_LIST);
    wk->palRes[3] = func_0204bba0(uiArc, func_0202d954(), CLACT_VRAM_MAIN, 160, HEAPID_VS_MULTI_LIST);
    wk->cellRes[3] = func_0204bde0(uiArc, func_0202d95c(2), func_0202d960(2), HEAPID_VS_MULTI_LIST);
    GFL_ArcToolFree(uiArc);
}

static void VsMultiList_FreeObjRes(VsMultiListWork *wk) {
    u32 i;

    for (i = 0; i < CHAR_RES_COUNT; i++) {
        func_0204b98c(wk->charRes[i]);
    }
    for (i = 0; i < PLTT_RES_COUNT; i++) {
        func_0204bcd0(wk->palRes[i]);
    }
    for (i = 0; i < CELL_RES_COUNT; i++) {
        func_0204be64(wk->cellRes[i]);
    }
}

static void VsMultiList_CreateActors(VsMultiListWork *wk) {
    u32 i;

    wk->clUnit = func_0204bf1c(ACTOR_COUNT, 0, HEAPID_VS_MULTI_LIST);
    for (i = 0; i < ACTOR_COUNT; i++) {
        const VsMultiListActor *def = &sVsMultiListActors[i];

        wk->actors[i] = func_0204c040(wk->clUnit, wk->charRes[def->charIndex], wk->palRes[def->palIndex],
                                      wk->cellRes[def->cellIndex], &def->setup, 0, HEAPID_VS_MULTI_LIST);
    }
    for (i = 0; i < POKEMON_COUNT; i++) {
        if (wk->pkm[i] == NULL) {
            func_0204c124(wk->actors[i], FALSE);
            func_0204c124(wk->actors[i + 6], FALSE);
            func_0204c124(wk->actors[i + 18], FALSE);
        } else {
            u16 item;

            func_0204c520(wk->actors[i], TRUE);
            func_0204c378(wk->actors[i], PokeIcon_GetPaletteOfPkm(func_0201d624(wk->pkm[i])), TRUE);
            item = PokeParty_GetParam(wk->pkm[i], PKM_PARAM_ITEM, NULL);
            if (item == 0) {
                func_0204c124(wk->actors[i + 6], FALSE);
            } else if (PML_ItemIsMail(item) == TRUE) {
                func_0204c4d4(wk->actors[i + 6], 0);
                func_0204c488(wk->actors[i + 6], 1);
            } else {
                func_0204c4d4(wk->actors[i + 6], 0);
                func_0204c488(wk->actors[i + 6], 0);
            }
        }
        func_0204c124(wk->actors[i + 12], FALSE);
    }
    if (wk->param->flipped == 0) {
        for (i = 0; i < BOARD_POKEMON_COUNT; i++) {
            VsMultiList_MoveActor(wk, i + 3, 128);
            VsMultiList_MoveActor(wk, i + 9, 128);
            VsMultiList_MoveActor(wk, i + 15, 128);
            VsMultiList_MoveActor(wk, i + 21, 128);
        }
    } else {
        for (i = 0; i < BOARD_POKEMON_COUNT; i++) {
            VsMultiList_MoveActor(wk, i, -128);
            VsMultiList_MoveActor(wk, i + 6, -128);
            VsMultiList_MoveActor(wk, i + 12, -128);
            VsMultiList_MoveActor(wk, i + 18, -128);
        }
    }
}

static void VsMultiList_FreeActors(VsMultiListWork *wk) {
    u32 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c108(wk->actors[i]);
    }
    func_0204bf98(wk->clUnit);
}

static void VsMultiList_MoveActor(VsMultiListWork *wk, u32 index, s16 dx) {
    ClActorPos pos;

    func_0204c178(wk->actors[index], &pos, 0);
    pos.x += dx;
    func_0204c140(wk->actors[index], &pos, 0);
}

// Moves the three Pokémon's four actors each, starting at the one given
static void VsMultiList_MoveActors(VsMultiListWork *wk, u32 first, s16 dx) {
    u32 i;

    for (i = first; i < first + 3; i++) {
        VsMultiList_MoveActor(wk, i, dx);
        VsMultiList_MoveActor(wk, i + 6, dx);
        VsMultiList_MoveActor(wk, i + 12, dx);
        VsMultiList_MoveActor(wk, i + 18, dx);
    }
}

static void VsMultiList_CreateFrames(VsMultiListWork *wk) {
    u16 i;
    u16 *screen;
    u16 *buffer;

    wk->frames = BGWinFrame_Create(BGWINFRAME_TRANSFER_VBLANK, FRAME_COUNT, HEAPID_VS_MULTI_LIST);
    for (i = 0; i < POKEMON_COUNT; i++) {
        BGWinFrame_InitFrame(wk->frames, i, 1, 16, 6);
        BGWinFrame_InitFrame(wk->frames, i + 6, 0, 16, 6);
        BGWinFrame_WriteBmpWin(wk->frames, i + 6, wk->printWindows[i].window);
    }
    screen = GFL_BGSysIsScrHeapExists(1);
    buffer = GFL_HeapAllocate(BG_HEAP, 0xc0, FALSE, "vs_multi_list.c", 1552);
    for (i = 0; i < POKEMON_COUNT; i++) {
        sys_memcpy16(screen + i * 0x20, buffer + i * 0x10, 0x20);
    }
    for (i = 0; i < POKEMON_COUNT; i++) {
        if (wk->pkm[i] != NULL) {
            BGWinFrame_SetScreen(wk->frames, i, buffer);
        }
    }
    for (i = 0; i < POKEMON_COUNT; i++) {
        sys_memcpy16(screen + (i + 18) * 0x20, buffer + i * 0x10, 0x20);
    }
    for (i = 0; i < POKEMON_COUNT; i++) {
        if (wk->pkm[i] == NULL) {
            BGWinFrame_SetScreen(wk->frames, i, buffer);
        }
    }
    GFL_HeapFree(buffer);
    GFL_BGSysClearScr(1);
    if (wk->param->flipped == 0) {
        for (i = 3; i < POKEMON_COUNT; i++) {
            if (wk->pkm[i] != NULL) {
                BGWinFrame_SetPalette(wk->frames, i, 0, 0, 16, 6, 5);
            }
        }
    } else {
        for (i = 0; i < BOARD_POKEMON_COUNT; i++) {
            if (wk->pkm[i] != NULL) {
                BGWinFrame_SetPalette(wk->frames, i, 0, 0, 16, 6, 5);
            }
        }
    }
    if (wk->param->flipped == 0) {
        BGWinFrame_Put(wk->frames, 0, 0, 0);
        BGWinFrame_Put(wk->frames, 1, 0, 6);
        BGWinFrame_Put(wk->frames, 2, 0, 12);
        BGWinFrame_Put(wk->frames, 6, 0, 0);
        BGWinFrame_Put(wk->frames, 7, 0, 6);
        BGWinFrame_Put(wk->frames, 8, 0, 12);
        BGWinFrame_Put(wk->frames, 3, 32, 1);
        BGWinFrame_Put(wk->frames, 4, 32, 7);
        BGWinFrame_Put(wk->frames, 5, 32, 13);
        BGWinFrame_Put(wk->frames, 9, 32, 1);
        BGWinFrame_Put(wk->frames, 10, 32, 7);
        BGWinFrame_Put(wk->frames, 11, 32, 13);
    } else {
        BGWinFrame_Put(wk->frames, 0, -16, 0);
        BGWinFrame_Put(wk->frames, 1, -16, 6);
        BGWinFrame_Put(wk->frames, 2, -16, 12);
        BGWinFrame_Put(wk->frames, 6, -16, 0);
        BGWinFrame_Put(wk->frames, 7, -16, 6);
        BGWinFrame_Put(wk->frames, 8, -16, 12);
        BGWinFrame_Put(wk->frames, 3, 16, 1);
        BGWinFrame_Put(wk->frames, 4, 16, 7);
        BGWinFrame_Put(wk->frames, 5, 16, 13);
        BGWinFrame_Put(wk->frames, 9, 16, 1);
        BGWinFrame_Put(wk->frames, 10, 16, 7);
        BGWinFrame_Put(wk->frames, 11, 16, 13);
    }
}

static void VsMultiList_FreeFrames(VsMultiListWork *wk) {
    BGWinFrame_Delete(wk->frames);
}
