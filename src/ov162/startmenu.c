#include "types.h"
#include "app/dwc_utility.h"
#include "app/game_start.h"
#include "app/gsync.h"
#include "app/mb_parent.h"
#include "app/mic_test.h"
#include "app/mystery_gift.h"
#include "app/start_menu.h"
#include "app/title.h"
#include "app/unova_link.h"
#include "app/wifibattlematch.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "save/dream_world.h"
#include "save/event_work.h"
#include "save/join_avenue.h"
#include "save/key_info.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "save/save_outside.h"
#include "system/app_keycursor.h"
#include "system/bgwinfrm.h"
#include "system/blink_palanm.h"
#include "system/bmp_menu.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/dsi.h"
#include "system/game_beacon.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/startmenu_fieldmap_ctrl_hybrid.h"
#include "text/system/wifi_error.h"

// The menu after the title screen. Its items scroll on the main engine's BGs 1 and 2, and the sub engine shows the
// saved game. Picking an item ends the menu, and its exit starts what the item leads to

enum {
    STATE_SETUP,
    STATE_FREE,
    STATE_WAIT_WIPE,
    STATE_BLINK,
    STATE_WAIT_PRINT,
    STATE_SELECT,
    STATE_SCROLL,
    STATE_CONTINUE,
    STATE_NEW_GAME,
    STATE_KEY_NOTICE,
    STATE_WFC_SETTINGS,
    STATE_BROKEN_SAVE,
    STATE_LEAVE,
    STATE_END,
};

// The items, in the order of their names in the message file. The list of items ends with ITEM_NONE, which is also
// the selection that goes back to the title screen
enum {
    ITEM_CONTINUE,
    ITEM_NEW_GAME,
    ITEM_MYSTERY_GIFT,
    ITEM_BATTLE_COMPETITION,
    ITEM_GAME_SYNC_SETTINGS,
    ITEM_WFC_SETTINGS,
    ITEM_MIC_TEST,
    // Blank in this version, and never added
    ITEM_MB_PARENT,
    ITEM_UNOVA_LINK,
    ITEM_NONE,
    ITEM_COUNT = ITEM_NONE,
};

// The windows: one with the name of each item, then these
#define WINDOW_MESSAGE 9
#define WINDOW_SAVED_GAME 10
#define WINDOW_PLAYER_NAME 11
#define WINDOW_LOCATION 12
#define WINDOW_TIME 13
#define WINDOW_POKEDEX 14
#define WINDOW_BADGES 15
#define WINDOW_KEY_SYSTEM 16
// The keys that are set, one per window
#define WINDOW_KEYS 17
#define WINDOW_COUNT 20

// The cell actors: markers of new items, for the items that func_0200ca50 tells about, then these
#define ACTOR_WFC_SETTINGS 4
#define ACTOR_PLAYER 5
// Icons of the keys that are set
#define ACTOR_KEYS 6
#define ACTOR_COUNT 9

// The graphics of the actors
enum {
    OBJ_RES_PLAYER,
    OBJ_RES_NEW,
    OBJ_RES_WFC_SETTINGS,
    OBJ_RES_KEYS,
    OBJ_RES_COUNT,
};

// The width of an item in tiles
#define ITEM_WIDTH 26
// The screen row that the items start at, and the row that the list scrolls to keep the cursor within
#define FIRST_ITEM_ROW 2
#define LAST_ITEM_ROW 24
// Frames to scroll the list by one item
#define SCROLL_FRAMES 3
#define SCROLL_HEIGHT 512

// Palettes of the items' screens
#define ITEM_PALETTE_NORMAL 1
#define ITEM_PALETTE_SELECTED 2
#define ITEM_PALETTE_BLINK 3

typedef struct {
    u16 screenFile;
    u16 rows;
} ItemLayout;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} WindowLayout;

// The messages about save data that was erased, for the blocks that func_02007464 reports
typedef struct {
    u32 blocks;
    u32 message;
} BrokenSaveMessage;

typedef struct {
    SaveControl *save;
    PlayerInfo *playerInfo;
    TrainerGameInfoSave *gameInfo;
    void *unkC;
    EventWork *eventWork;
    KeyInfoSave *keyInfo;
    TCB *vblankTask;
    ClActUnit *clactUnit;
    ClActor *actors[ACTOR_COUNT];
    u32 chars[OBJ_RES_COUNT];
    u32 palettes[OBJ_RES_COUNT];
    u32 cellAnims[OBJ_RES_COUNT];
    Font *font;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    PrintQueue *printQueue;
    PrintStream *printStream;
    TCBExManager *tcbManager;
    KeyCursor *keyCursor;
    // Whether the message was already told to go on past its pause
    BOOL continued;
    BmpMenu *dialog;
    PrintWindow windows[WINDOW_COUNT];
    // A window over the menu, with a warning or a notice
    PrintWindow notice;
    u16 *itemScreens[ITEM_COUNT];
    // Makes a screen for each item from the window with its name, which BG 1 shows
    BGWinFrame *textScreens;
    void *unk16C;
    u8 items[ITEM_COUNT];
    u8 cursor;
    u8 selection;
    u8 seq;
    u8 wait;
    // Whether to continue with the C-Gear off, which clears bit 10 of the config
    u8 cgearOff;
    u8 blinkSeq;
    u8 blinkWait;
    // The screen row of the selected item
    s8 cursorRow;
    s32 scrollY;
    u32 scrollFrames;
    s32 scrollSpeed;
    u32 state;
    u32 blinkNextState;
    u32 wipeNextState;
    u32 brokenBlocks;
    u32 brokenIndex;
} StartMenuWork;

static BOOL StartMenu_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL StartMenu_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL StartMenu_Exit(GameProc *proc, u32 *state, void *param, void *work);
static u32 StartMenu_Setup(StartMenuWork *wk);
static u32 StartMenu_Free(StartMenuWork *wk);
static u32 StartMenu_WaitWipe(StartMenuWork *wk);
static u32 StartMenu_Blink(StartMenuWork *wk);
static u32 StartMenu_WaitPrint(StartMenuWork *wk);
static u32 StartMenu_Select(StartMenuWork *wk);
static u32 StartMenu_Scroll(StartMenuWork *wk);
static u32 StartMenu_Continue(StartMenuWork *wk);
static u32 StartMenu_NewGame(StartMenuWork *wk);
static u32 StartMenu_KeyNotice(StartMenuWork *wk);
static u32 StartMenu_WFCSettings(StartMenuWork *wk);
static u32 StartMenu_BrokenSave(StartMenuWork *wk);
static u32 StartMenu_Leave(StartMenuWork *wk);
static void StartMenu_VBlank(TCB *tcb, void *data);
static void StartMenu_AddVBlankTask(StartMenuWork *wk);
static void StartMenu_RemoveVBlankTask(StartMenuWork *wk);
static void StartMenu_InitVRAM(void);
static void StartMenu_InitBG(void);
static void StartMenu_FreeBG(void);
static void StartMenu_LoadBGGraphics(void);
static void StartMenu_InitUnk16C(StartMenuWork *wk);
static void StartMenu_FreeUnk16C(StartMenuWork *wk);
static void StartMenu_InitMsg(StartMenuWork *wk);
static void StartMenu_FreeMsg(StartMenuWork *wk);
static void StartMenu_InitWindows(StartMenuWork *wk);
static void StartMenu_FreeWindows(StartMenuWork *wk);
static void StartMenu_InitObj(StartMenuWork *wk);
static void StartMenu_FreeObj(StartMenuWork *wk);
static void StartMenu_InitTextScreens(StartMenuWork *wk);
static void StartMenu_FreeTextScreens(StartMenuWork *wk);
static void StartMenu_SetBlend(void);
static void StartMenu_InitItems(StartMenuWork *wk);
static void StartMenu_LoadItemScreens(StartMenuWork *wk);
static void StartMenu_FreeItemScreens(StartMenuWork *wk);
static void StartMenu_SetActorY(StartMenuWork *wk, u32 actor, s16 y);
static void StartMenu_DrawItems(StartMenuWork *wk);
static void StartMenu_DrawItem(StartMenuWork *wk, u32 item, u32 row);
static void StartMenu_SetItemPalette(StartMenuWork *wk, u32 item, s8 row, u8 palette);
static s8 StartMenu_GetScreenRow(StartMenuWork *wk, s8 row);
static void StartMenu_ShowSavedGame(StartMenuWork *wk);
static void StartMenu_HideSavedGame(StartMenuWork *wk);
static void StartMenu_MoveItemActors(StartMenuWork *wk, s32 dy);
static BOOL StartMenu_MoveCursor(StartMenuWork *wk, s32 dir);
static void StartMenu_SetItemActorsVisible(StartMenuWork *wk, BOOL visible);
static void StartMenu_DrawFrame(u8 x, u8 y, u8 width, u8 height, u8 bg);
static void StartMenu_ClearFrame(u8 x, u8 y, u8 width, u8 height, u8 bg);
static void StartMenu_OpenNewGameWarning(StartMenuWork *wk);
static void StartMenu_CloseNotice(StartMenuWork *wk, BOOL leaving);
static void StartMenu_OpenCGearWarning(StartMenuWork *wk);
static void StartMenu_CloseCGearWarning(StartMenuWork *wk);
static void StartMenu_OpenDSiNotice(StartMenuWork *wk);
static void StartMenu_Print(StartMenuWork *wk, u32 messageId);
static void StartMenu_ClearMessage(StartMenuWork *wk);
static BOOL StartMenu_UpdatePrint(StartMenuWork *wk);
static void StartMenu_OpenYesNo(StartMenuWork *wk);
static u32 StartMenu_WipeIn(StartMenuWork *wk, u32 next);
static u32 StartMenu_WipeOut(StartMenuWork *wk, u32 next);
static u32 StartMenu_StartBlink(StartMenuWork *wk, u32 next);
static BOOL StartMenu_CheckBrokenSave(StartMenuWork *wk);

// Declared in an order that gives the original layout of the data (tools/decomp/rodata_order.py)

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static u32 (*const sStateFuncs[])(StartMenuWork *wk) = {
    StartMenu_Setup,       StartMenu_Free,       StartMenu_WaitWipe, StartMenu_Blink,   StartMenu_WaitPrint,
    StartMenu_Select,      StartMenu_Scroll,     StartMenu_Continue, StartMenu_NewGame, StartMenu_KeyNotice,
    StartMenu_WFCSettings, StartMenu_BrokenSave, StartMenu_Leave,
};

static const WindowLayout sWindowLayouts[WINDOW_COUNT] = {
    { 1, 2, 0, 19, 3, 5 },  { 1, 2, 0, 19, 3, 5 },   { 1, 2, 0, 19, 3, 5 },   { 1, 2, 0, 19, 3, 5 },
    { 1, 2, 0, 22, 3, 5 },  { 1, 2, 0, 22, 3, 5 },   { 1, 2, 0, 19, 3, 5 },   { 1, 2, 0, 19, 3, 5 },
    { 1, 2, 0, 19, 3, 5 },  { 0, 1, 19, 30, 4, 15 }, { 4, 5, 2, 18, 3, 5 },   { 4, 10, 5, 8, 2, 5 },
    { 4, 10, 7, 15, 2, 5 }, { 4, 5, 12, 18, 2, 5 },  { 4, 15, 10, 13, 2, 5 }, { 4, 5, 10, 10, 2, 5 },
    { 4, 5, 15, 19, 3, 5 }, { 4, 7, 18, 19, 2, 5 },  { 4, 7, 20, 19, 2, 5 },  { 4, 7, 22, 19, 2, 5 },
};

static const ItemLayout sItemLayouts[ITEM_COUNT] = {
    { 7, 3 }, { 7, 3 }, { 7, 3 }, { 7, 3 }, { 7, 3 }, { 7, 3 }, { 7, 3 }, { 7, 3 }, { 7, 3 },
};

static const BrokenSaveMessage sBrokenSaveMessages[] = {
    { 0x2, 38 },   { 0x3c, 39 },    { 0x40, 40 },     { 0x80, 41 },     { 0x100, 42 },     { 0x200, 43 },
    { 0x400, 44 }, { 0x7f800, 46 }, { 0x380000, 48 }, { 0x800000, 45 }, { 0x1000000, 47 },
};

static const ClActorSetup sPlayerActorSetup = { 48, 52, 1, 0, 1 };

static const ClActorSetup sWFCSettingsActorSetup = { 208, 0, 1, 0, 1 };

static const ClActorSetup sNewActorSetup = { 208, 0, 0, 0, 1 };

static const ClActorSetup sKeyActorSetups[] = {
    { 48, 152, 0, 0, 0 },
    { 48, 168, 0, 0, 0 },
    { 48, 184, 0, 0, 0 },
};

const GameProcFunctions START_MENU_PROC_FUNCTIONS = { StartMenu_Init, StartMenu_Main, StartMenu_Exit };

static BOOL StartMenu_Init(GameProc *proc, u32 *state, void *param, void *work) {
    StartMenuWork *wk;
    void *ov331;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_STARTMENU, 0x80000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(StartMenuWork), HEAPID_STARTMENU);
    sys_memset(wk, 0, sizeof(StartMenuWork));
    wk->save = SaveControl_GetInstance();
    wk->playerInfo = SaveControl_GetPlayerInfo(wk->save);
    wk->gameInfo = getTrainerGameInfoAddress(wk->save);
    wk->unkC = func_02009918(wk->save);
    wk->eventWork = getConstDataBlock(wk->save);
    if (SaveControl_IsDataAlreadyPresent(wk->save)) {
        wk->keyInfo = getKeyInfoSaveBlk(wk->save);
    } else {
        GFL_OvlLoad(OVERLAY_ID(331));
        ov331 = SaveOutside_Load(HEAPID_STARTMENU);
        wk->keyInfo = func_0201046c(SaveOutside_GetKeyData(ov331));
        SaveOutside_Free(ov331);
        GFL_OvlUnload(OVERLAY_ID(331));
    }
    return TRUE;
}

static BOOL StartMenu_Main(GameProc *proc, u32 *state, void *param, void *work) {
    StartMenuWork *wk = work;

    wk->state = sStateFuncs[wk->state](wk);
    if (wk->state == STATE_END) {
        return TRUE;
    }
    GFL_TCBExMgrUpdate(wk->tcbManager);
    func_0204b794();
    return FALSE;
}

static BOOL StartMenu_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    StartMenuWork *wk = work;
    u8 selection = wk->selection;
    u8 cgearOff = wk->cgearOff;
    WifiBattleMatchParam *battleMatchParam;
    MBParentParam *mbParentParam;

    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_STARTMENU);
    switch (selection) {
    case ITEM_CONTINUE:
        func_0202d6a8();
        if (cgearOff == FALSE) {
            GameStart_ContinueCGearOn();
        } else {
            GameStart_ContinueCGearOff();
        }
        break;
    case ITEM_NEW_GAME:
        func_0202d6a8();
        GameStart_NewGame();
        break;
    case ITEM_MYSTERY_GIFT:
        GCTX_ProcMgrReplaceProc(OVERLAY_ID(197), &MYSTERY_GIFT_PROC_FUNCTIONS, NULL);
        break;
    case ITEM_BATTLE_COMPETITION:
        battleMatchParam = GFL_HeapAllocate(HEAPID_USER, sizeof(WifiBattleMatchParam), FALSE, "startmenu.c", 583);
        sys_memset(battleMatchParam, 0, sizeof(WifiBattleMatchParam));
        battleMatchParam->unk8 = 0;
        battleMatchParam->unk10 = 1;
        GCTX_ProcMgrReplaceProc(OVERLAY_ID(290), &WIFIBATTLEMATCH_PROC_FUNCTIONS, battleMatchParam);
        break;
    case ITEM_GAME_SYNC_SETTINGS:
        GCTX_ProcMgrReplaceProc(OVERLAY_NONE, &GAME_SYNC_SETTINGS_PROC_FUNCTIONS, NULL);
        break;
    case ITEM_WFC_SETTINGS:
        GCTX_ProcMgrReplaceProc(OVERLAY_ID(182), &DWC_UTILITY_RESET_PROC_FUNCTIONS, NULL);
        break;
    case ITEM_MIC_TEST:
        GCTX_ProcMgrReplaceProc(OVERLAY_ID(253), &MIC_TEST_PROC_FUNCTIONS, NULL);
        break;
    case ITEM_MB_PARENT:
        mbParentParam = GFL_HeapAllocate(HEAPID_USER, sizeof(MBParentParam), TRUE, "startmenu.c", 605);
        mbParentParam->startMenu = TRUE;
        GCTX_ProcMgrReplaceProc(OVERLAY_ID(181), &MB_PARENT_PROC_FUNCTIONS, mbParentParam);
        break;
    case ITEM_UNOVA_LINK:
        GCTX_ProcMgrReplaceProc(OVERLAY_ID(332), &UNOVA_LINK_PROC_FUNCTIONS, NULL);
        break;
    default:
        GCTX_ProcMgrReplaceProc(OVERLAY_ID(162), &TITLE_PROC_FUNCTIONS, NULL);
        break;
    }
    return TRUE;
}

static u32 StartMenu_Setup(StartMenuWork *wk) {
    if (func_020099f4(getDreamWorldStuffAddress(wk->save)) == TRUE) {
        func_0200ca38(wk->gameInfo, 2, 2);
    }
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetDisplayLayout(1);
    StartMenu_InitVRAM();
    StartMenu_InitBG();
    StartMenu_LoadBGGraphics();
    StartMenu_InitMsg(wk);
    StartMenu_InitItems(wk);
    StartMenu_LoadItemScreens(wk);
    StartMenu_InitWindows(wk);
    StartMenu_InitObj(wk);
    StartMenu_InitTextScreens(wk);
    StartMenu_DrawItems(wk);
    StartMenu_InitUnk16C(wk);
    StartMenu_SetBlend();
    StartMenu_AddVBlankTask(wk);
    if (SaveControl_IsDataAlreadyPresent(wk->save) == TRUE) {
        StartMenu_ShowSavedGame(wk);
    }
    return STATE_WAIT_PRINT;
}

static u32 StartMenu_Free(StartMenuWork *wk) {
    switch (wk->seq) {
    case 0:
        wk->seq++;
    case 1:
        if (func_02042ab8()) {
            StartMenu_RemoveVBlankTask(wk);
            StartMenu_FreeUnk16C(wk);
            StartMenu_FreeTextScreens(wk);
            StartMenu_FreeObj(wk);
            StartMenu_FreeWindows(wk);
            StartMenu_FreeItemScreens(wk);
            StartMenu_FreeMsg(wk);
            StartMenu_FreeBG();
            G2_BlendNone();
            G2S_BlendNone();
            GFL_BGSysSetEnabledBGsA(0);
            GFL_BGSysSetEnabledBGsB(0);
            return STATE_END;
        }
        break;
    }
    return STATE_FREE;
}

static u32 StartMenu_WaitWipe(StartMenuWork *wk) {
    if (GFL_WipeIsFinished() == TRUE) {
        return wk->wipeNextState;
    }
    return STATE_WAIT_WIPE;
}

// Blinks the selected item before going on
static u32 StartMenu_Blink(StartMenuWork *wk) {
    switch (wk->blinkSeq) {
    case 0:
    case 2:
        if (wk->blinkWait == 0) {
            StartMenu_SetItemPalette(wk, wk->items[wk->cursor], StartMenu_GetScreenRow(wk, wk->cursorRow),
                                     ITEM_PALETTE_BLINK);
            wk->blinkWait = 4;
            wk->blinkSeq++;
        } else {
            wk->blinkWait--;
        }
        break;
    case 1:
    case 3:
        if (wk->blinkWait == 0) {
            StartMenu_SetItemPalette(wk, wk->items[wk->cursor], StartMenu_GetScreenRow(wk, wk->cursorRow),
                                     ITEM_PALETTE_NORMAL);
            wk->blinkWait = 4;
            wk->blinkSeq++;
        } else {
            wk->blinkWait--;
        }
        break;
    case 4:
        return wk->blinkNextState;
    }
    return STATE_BLINK;
}

static u32 StartMenu_WaitPrint(StartMenuWork *wk) {
    u32 i;

    func_02021a3c(wk->printQueue);
    for (i = 0; i < WINDOW_COUNT; i++) {
        PrintWindow_Flush(&wk->windows[i], wk->printQueue);
    }
    if (func_02021c0c(wk->printQueue) == TRUE) {
        if (StartMenu_CheckBrokenSave(wk) == TRUE) {
            return STATE_BROKEN_SAVE;
        }
        if (func_0200ca74(wk->gameInfo) == TRUE) {
            func_0200ca78(wk->gameInfo, 0);
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2, FALSE);
            StartMenu_SetItemActorsVisible(wk, FALSE);
            return StartMenu_WipeIn(wk, STATE_KEY_NOTICE);
        }
        return StartMenu_WipeIn(wk, STATE_SELECT);
    }
    return STATE_WAIT_PRINT;
}

static u32 StartMenu_Select(StartMenuWork *wk) {
    BlinkPalAnm_Main(wk->unk16C);
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        switch (wk->items[wk->cursor]) {
        case ITEM_CONTINUE:
            wk->selection = wk->items[wk->cursor];
            return StartMenu_StartBlink(wk, STATE_CONTINUE);
        case ITEM_NEW_GAME:
            return StartMenu_StartBlink(wk, STATE_NEW_GAME);
        case ITEM_MYSTERY_GIFT:
            wk->selection = wk->items[wk->cursor];
            return StartMenu_StartBlink(wk, STATE_LEAVE);
        case ITEM_BATTLE_COMPETITION:
            func_0200ca38(wk->gameInfo, 1, 2);
            wk->selection = wk->items[wk->cursor];
            return StartMenu_StartBlink(wk, STATE_LEAVE);
        case ITEM_GAME_SYNC_SETTINGS:
            func_0200ca38(wk->gameInfo, 2, 2);
            wk->selection = wk->items[wk->cursor];
            return StartMenu_StartBlink(wk, STATE_LEAVE);
        case ITEM_MB_PARENT:
            func_0200ca38(wk->gameInfo, 3, 2);
            wk->selection = wk->items[wk->cursor];
            return StartMenu_StartBlink(wk, STATE_LEAVE);
        case ITEM_WFC_SETTINGS:
            return StartMenu_StartBlink(wk, STATE_WFC_SETTINGS);
        case ITEM_MIC_TEST:
            wk->selection = wk->items[wk->cursor];
            return StartMenu_StartBlink(wk, STATE_LEAVE);
        case ITEM_UNOVA_LINK:
            wk->selection = wk->items[wk->cursor];
            return StartMenu_StartBlink(wk, STATE_LEAVE);
        }
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        wk->selection = ITEM_NONE;
        return StartMenu_WipeOut(wk, STATE_FREE);
    }
    if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        if (StartMenu_MoveCursor(wk, -1) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return STATE_SCROLL;
        }
    }
    if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        if (StartMenu_MoveCursor(wk, 1) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return STATE_SCROLL;
        }
    }
    return STATE_SELECT;
}

static u32 StartMenu_Scroll(StartMenuWork *wk) {
    if (wk->scrollFrames == SCROLL_FRAMES) {
        wk->scrollFrames = 0;
        wk->scrollSpeed = 0;
        BlinkPalAnm_InitAnime(wk->unk16C);
        StartMenu_SetItemPalette(wk, wk->items[wk->cursor], StartMenu_GetScreenRow(wk, wk->cursorRow),
                                 ITEM_PALETTE_SELECTED);
        return STATE_SELECT;
    }
    wk->scrollY += wk->scrollSpeed;
    if (wk->scrollY < 0) {
        wk->scrollY += SCROLL_HEIGHT;
    } else if (wk->scrollY >= SCROLL_HEIGHT) {
        wk->scrollY -= SCROLL_HEIGHT;
    }
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_Y, wk->scrollY);
    GFL_BGSysMoveBGReq(2, BG_MOVE_SET_Y, wk->scrollY);
    StartMenu_MoveItemActors(wk, -wk->scrollSpeed);
    wk->scrollFrames++;
    return STATE_SCROLL;
}

// Asks whether to start the C-Gear, if it was on when the game was saved
static u32 StartMenu_Continue(StartMenuWork *wk) {
    switch (wk->seq) {
    case 0:
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2, FALSE);
        StartMenu_SetItemActorsVisible(wk, FALSE);
        if (func_020098c0(wk->unkC) == 0) {
            wk->cgearOff = TRUE;
            wk->seq = 0;
            return StartMenu_WipeOut(wk, STATE_FREE);
        }
        wk->wait = 8;
        wk->seq++;
        break;
    case 1:
        if (wk->wait != 0) {
            wk->wait--;
            break;
        }
        StartMenu_HideSavedGame(wk);
        StartMenu_Print(wk, 36);
        StartMenu_OpenCGearWarning(wk);
        wk->seq++;
        break;
    case 2:
        if (StartMenu_UpdatePrint(wk) == FALSE) {
            StartMenu_OpenYesNo(wk);
            wk->seq++;
        }
        break;
    case 3:
        switch (ConfirmDialog_Update(wk->dialog)) {
        case 0:
            if (isWirelessEnabled() == FALSE) {
                StartMenu_Print(wk, 27);
                wk->seq = 5;
            } else {
                wk->cgearOff = FALSE;
                wk->seq = 4;
            }
            break;
        case BMPMENU_CANCEL:
            StartMenu_Print(wk, 37);
            wk->seq = 7;
            break;
        }
        break;
    case 4:
        StartMenu_CloseCGearWarning(wk);
        StartMenu_ClearMessage(wk);
        wk->seq = 0;
        return StartMenu_WipeOut(wk, STATE_FREE);
    case 5:
        if (StartMenu_UpdatePrint(wk) == FALSE) {
            wk->seq++;
        }
        break;
    case 6:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            wk->cgearOff = TRUE;
            wk->seq = 4;
        }
        break;
    case 7:
        if (StartMenu_UpdatePrint(wk) == FALSE) {
            StartMenu_OpenYesNo(wk);
            wk->seq++;
        }
        break;
    case 8:
        switch (ConfirmDialog_Update(wk->dialog)) {
        case 0:
            wk->cgearOff = TRUE;
            wk->seq = 4;
            break;
        case BMPMENU_CANCEL:
            StartMenu_Print(wk, 36);
            wk->seq = 2;
            break;
        }
        break;
    }
    func_02021a3c(wk->printQueue);
    PrintWindow_Flush(&wk->notice, wk->printQueue);
    return STATE_CONTINUE;
}

// Warns that a new game cannot be saved over the saved one
static u32 StartMenu_NewGame(StartMenuWork *wk) {
    switch (wk->seq) {
    case 0:
        if (wk->items[0] == ITEM_NEW_GAME) {
            wk->selection = ITEM_NEW_GAME;
            wk->seq = 0;
            return StartMenu_WipeOut(wk, STATE_FREE);
        }
        StartMenu_OpenNewGameWarning(wk);
        wk->seq++;
        break;
    case 1:
        if (func_02021c0c(wk->printQueue) == TRUE) {
            wk->seq++;
        }
        break;
    case 2:
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            StartMenu_CloseNotice(wk, TRUE);
            wk->selection = ITEM_NEW_GAME;
            wk->seq = 0;
            return StartMenu_WipeOut(wk, STATE_FREE);
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            StartMenu_CloseNotice(wk, FALSE);
            StartMenu_SetItemPalette(wk, wk->items[wk->cursor], StartMenu_GetScreenRow(wk, wk->cursorRow),
                                     ITEM_PALETTE_SELECTED);
            wk->seq = 0;
            return STATE_SELECT;
        }
        break;
    }
    func_02021a3c(wk->printQueue);
    PrintWindow_Flush(&wk->notice, wk->printQueue);
    return STATE_NEW_GAME;
}

// Tells that Unova Link can now send the keys
static u32 StartMenu_KeyNotice(StartMenuWork *wk) {
    switch (wk->seq) {
    case 0:
        StartMenu_Print(wk, 29);
        wk->seq++;
        break;
    case 1:
        if (StartMenu_UpdatePrint(wk) == FALSE) {
            wk->seq++;
        }
        break;
    case 2:
        StartMenu_ClearMessage(wk);
        wk->seq++;
        break;
    case 3:
        func_0200ca38(wk->gameInfo, 0, 1);
        StartMenu_InitItems(wk);
        StartMenu_DrawItems(wk);
        GFL_BGSysMoveBGReq(1, BG_MOVE_SET_Y, wk->scrollY);
        GFL_BGSysMoveBGReq(2, BG_MOVE_SET_Y, wk->scrollY);
        BlinkPalAnm_InitAnime(wk->unk16C);
        wk->seq++;
        break;
    case 4:
        StartMenu_SetItemActorsVisible(wk, TRUE);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2, TRUE);
        wk->seq = 0;
        return STATE_SELECT;
    }
    return STATE_KEY_NOTICE;
}

// On a DSi, the connection settings are in the System Settings instead
static u32 StartMenu_WFCSettings(StartMenuWork *wk) {
    if (isRunningOnDSi() == TRUE) {
        switch (wk->seq) {
        case 0:
            StartMenu_OpenDSiNotice(wk);
            wk->seq++;
            break;
        case 1:
            if (func_02021c0c(wk->printQueue) == TRUE) {
                wk->seq++;
            }
            break;
        case 2:
            if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                StartMenu_CloseNotice(wk, FALSE);
                StartMenu_SetItemPalette(wk, wk->items[wk->cursor], StartMenu_GetScreenRow(wk, wk->cursorRow),
                                         ITEM_PALETTE_SELECTED);
                wk->seq = 0;
                return STATE_SELECT;
            }
            break;
        }
        func_02021a3c(wk->printQueue);
        PrintWindow_Flush(&wk->notice, wk->printQueue);
        return STATE_WFC_SETTINGS;
    }
    wk->selection = wk->items[wk->cursor];
    return STATE_LEAVE;
}

// Tells about each part of the save data that was erased
static u32 StartMenu_BrokenSave(StartMenuWork *wk) {
    switch (wk->seq) {
    case 0:
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ, FALSE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ, FALSE);
        GFL_BGSysResetStdPalette(0, GX_RGB(12, 12, 31));
        GFL_BGSysResetStdPalette(4, GX_RGB(12, 12, 31));
        wk->seq++;
        return StartMenu_WipeIn(wk, STATE_BROKEN_SAVE);
    case 1:
        while (TRUE) {
            if (wk->brokenIndex >= NELEMS(sBrokenSaveMessages)) {
                StartMenu_ClearMessage(wk);
                wk->seq = 3;
                return StartMenu_WipeOut(wk, STATE_BROKEN_SAVE);
            }
            if (sBrokenSaveMessages[wk->brokenIndex].blocks & wk->brokenBlocks) {
                break;
            }
            wk->brokenIndex++;
        }
        StartMenu_Print(wk, sBrokenSaveMessages[wk->brokenIndex].message);
        wk->brokenIndex++;
        wk->seq++;
        break;
    case 2:
        if (StartMenu_UpdatePrint(wk) == FALSE && (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            wk->seq = 1;
        }
        break;
    case 3:
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ, TRUE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ, TRUE);
        wk->seq = 0;
        return StartMenu_WipeIn(wk, STATE_SELECT);
    }
    return STATE_BROKEN_SAVE;
}

static u32 StartMenu_Leave(StartMenuWork *wk) {
    return StartMenu_WipeOut(wk, STATE_FREE);
}

static void StartMenu_VBlank(TCB *tcb, void *data) {
    GFL_BGSysUpdate();
    func_0204b7c8();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

static void StartMenu_AddVBlankTask(StartMenuWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(StartMenu_VBlank, wk, 0);
}

static void StartMenu_RemoveVBlankTask(StartMenuWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static void StartMenu_InitVRAM(void) {
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
}

// The main engine's BG 0 has the messages and notices, BGs 1 and 2 the items' names and screens, and BG 3 the
// background. The sub engine's BG 0 has the warning about the C-Gear, BG 1 the saved game and BG 2 its background
static void StartMenu_InitBG(void) {
    GFL_BGSysCreate(HEAPID_STARTMENU);
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
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x2000,
            0,
            BGRES_512x512,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xd000),
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
            0x2000,
            0,
            BGRES_512x512,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xb000),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
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
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
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
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
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
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
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
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2, TRUE);
}

static void StartMenu_FreeBG(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2, FALSE);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysFree();
}

static void StartMenu_LoadBGGraphics(void) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_STARTMENU, HEAPID_TAIL(HEAPID_STARTMENU));

    GFL_G2DIOLoadArcNCLRDefault(arc, 4, 0, 0, 0xc0, HEAPID_STARTMENU);
    GFL_BGSysLoadArcNCGRStatic(arc, 3, 2, 0, 0, TRUE, HEAPID_STARTMENU);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 2, 3, 0, 0, TRUE, HEAPID_STARTMENU);
    GFL_G2DIOLoadArcNCLRDefault(arc, 4, 4, 0, 0xc0, HEAPID_STARTMENU);
    GFL_BGSysLoadArcNCGRStatic(arc, 1, 5, 0, 0, TRUE, HEAPID_STARTMENU);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0, 6, 0, 0, TRUE, HEAPID_STARTMENU);
    GFL_ArcToolFree(arc);
    LoadSysMsgBox(0, 1, 14, 0, HEAPID_STARTMENU);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 15 * 0x20, 0x20, HEAPID_STARTMENU);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 15 * 0x20, 0x20, HEAPID_STARTMENU);
#ifdef WHITE2
    // White 2 draws BGs 3 and 6, one on each screen, in palette 4
    GFL_BGSysSetScrPaletteNo(3, 0, 0, 32, 24, 4);
    GFL_BGSysSetScrPaletteNo(6, 0, 0, 32, 24, 4);
    GFL_BGSysQueueScrLoad(3);
    GFL_BGSysQueueScrLoad(6);
#endif
}

static void StartMenu_InitUnk16C(StartMenuWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_STARTMENU, HEAPID_TAIL(HEAPID_STARTMENU));

    wk->unk16C = BlinkPalAnm_Create(0x20, 0x10, 2, HEAPID_STARTMENU);
    BlinkPalAnm_SetPalBufferArcTool(wk->unk16C, arc, 4, 0x20, 0x30);
    GFL_ArcToolFree(arc);
}

static void StartMenu_FreeUnk16C(StartMenuWork *wk) {
    BlinkPalAnm_Free(wk->unk16C);
}

static void StartMenu_InitMsg(StartMenuWork *wk) {
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_STARTMENU_FIELDMAP_CTRL_HYBRID, HEAPID_STARTMENU);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, HEAPID_STARTMENU);
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_STARTMENU);
    wk->printQueue = func_02021998(HEAPID_STARTMENU);
    wk->strbuf = GFL_StrBufCreate(1024, HEAPID_STARTMENU);
    wk->tcbManager = GFL_TCBExMgrCreate(HEAPID_STARTMENU, HEAPID_STARTMENU, 1, 4);
    wk->keyCursor = KeyCursor_Create(15, TRUE, FALSE, HEAPID_STARTMENU);
}

static void StartMenu_FreeMsg(StartMenuWork *wk) {
    KeyCursor_Free(wk->keyCursor);
    GFL_TCBExMgrFree(wk->tcbManager);
    GFL_StrBufFree(wk->strbuf);
    func_02021a18(wk->printQueue);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_FontFree(wk->font);
    GFL_MsgDataFree(wk->msgData);
}

static void StartMenu_InitWindows(StartMenuWork *wk) {
    u32 i;
    StrBuf *str;
    StrBuf *fmt;
    BOOL joinAvenue;
    ZoneSpawnInfo *spawn;
    SaveLocation location;
    JoinAvenueInfo *joinAvenueInfo;
    StrBuf *name;
    PlayTime *time;
    u32 row;
    u32 msg;
    u32 difficulty;

    BmpWin_InitAllocator(HEAPID_STARTMENU);
    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->windows[i].window =
            BmpWin_CreateDynamic(sWindowLayouts[i].bg, sWindowLayouts[i].x, sWindowLayouts[i].y,
                                 sWindowLayouts[i].width, sWindowLayouts[i].height, sWindowLayouts[i].palette, 1);
    }

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_Continue);
    PrintWindow_Print(&wk->windows[ITEM_CONTINUE], wk->printQueue, 0, 4, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_SavedGame);
    PrintWindow_Print(&wk->windows[WINDOW_SAVED_GAME], wk->printQueue, 0, 4, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);

    fmt = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_Empty_3);
    copyVarForText(wk->wordSet, 0, wk->playerInfo);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, fmt);
    if (getTrainerGender(wk->playerInfo) == 0) {
        PrintWindow_Print(&wk->windows[WINDOW_PLAYER_NAME], wk->printQueue, 0, 0, wk->strbuf, wk->font,
                          PRINT_COLOR(3, 4, 0));
    } else {
        PrintWindow_Print(&wk->windows[WINDOW_PLAYER_NAME], wk->printQueue, 0, 0, wk->strbuf, wk->font,
                          PRINT_COLOR(5, 6, 0));
    }
    GFL_StrBufFree(fmt);

    // Where the player saved, or where the player goes next after flag 0x965 is set
    fmt = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_Empty_7);
    joinAvenue = FALSE;
    InitZoneDataSystem(HEAPID_TAIL(HEAPID_STARTMENU));
    if (EventWork_FlagGet(wk->eventWork, 0x965) == TRUE) {
        spawn = PlayerSave_GetNextSpawnZone(SaveControl_GetPlayerSave(wk->save));
        if (IsZoneJoinAvenue(spawn->zoneId) || IsZoneJoinAvenueSubZone(spawn->zoneId)) {
            joinAvenue = TRUE;
        } else {
            loadLocationNameToStrbuf(wk->wordSet, 0, ZoneData_GetPlaceNameID(spawn->zoneId));
        }
    } else {
        func_02008fb8(wk->save, &location);
        if (IsZoneJoinAvenue(location.zoneId) || IsZoneJoinAvenueSubZone(location.zoneId)) {
            joinAvenue = TRUE;
        } else {
            loadLocationNameToStrbuf(wk->wordSet, 0, ZoneData_GetPlaceNameID(location.zoneId));
        }
    }
    FreeZoneDataSystem();
    // The Join Avenue is shown by the name the player gave it
    if (joinAvenue) {
        joinAvenueInfo = JoinAvenue_GetInfo(SaveControl_GetJoinAvenue(wk->save));
        name = GFL_StrBufCreate(64, HEAPID_STARTMENU);
        GFL_StrBufLoadString(name, (const u16 *)JoinAvenue_GetParam(joinAvenueInfo, 0, 0));
        func_0202437c(wk->wordSet, 0, name, 2, 1, 2);
        GFL_StrBufFree(name);
    }
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, fmt);
    PrintWindow_Print(&wk->windows[WINDOW_LOCATION], wk->printQueue, 0, 0, wk->strbuf, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(fmt);

    fmt = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_Time);
    time = func_02008de8(wk->save);
    WordSetNumber(wk->wordSet, 0, func_02008cec(time), 3, NUM_PAD_NONE, 1);
    WordSetNumber(wk->wordSet, 1, func_02008cf0(time), 2, NUM_PAD_ZERO, 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, fmt);
    PrintWindow_Print(&wk->windows[WINDOW_TIME], wk->printQueue, 0, 0, wk->strbuf, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(fmt);

    // The Pokédex after flag 0x962 is set
    if (EventWork_FlagGet(wk->eventWork, 0x962) == TRUE) {
        fmt = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_Pokedex);
        WordSetNumber(wk->wordSet, 0, countSeenDexPokes(getPokedexSaveAddress(wk->save), HEAPID_STARTMENU), 3,
                      NUM_PAD_NONE, 1);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, fmt);
        PrintWindow_Print(&wk->windows[WINDOW_POKEDEX], wk->printQueue, 0, 0, wk->strbuf, wk->font,
                          PRINT_COLOR(1, 2, 0));
        GFL_StrBufFree(fmt);
    }

    fmt = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_Badges);
    WordSetNumber(wk->wordSet, 0, getBadgeCount(getTrainerGameInfoAddress(wk->save)), 2, NUM_PAD_NONE, 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, fmt);
    PrintWindow_Print(&wk->windows[WINDOW_BADGES], wk->printQueue, 0, 0, wk->strbuf, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(fmt);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_KeySystemSettings);
    PrintWindow_Print(&wk->windows[WINDOW_KEY_SYSTEM], wk->printQueue, 0, 4, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);

    // The keys that are set, each in the next window
    row = WINDOW_KEYS;
    msg = 0;
    difficulty = GetGameDifficulty(wk->keyInfo);
    if (difficulty == GAME_DIFFICULTY_EASY) {
        msg = 20;
    } else if (difficulty == GAME_DIFFICULTY_CHALLENGE) {
        msg = 21;
    }
    if (msg != 0) {
        str = GFL_MsgDataLoadStrbufNew(wk->msgData, msg);
        PrintWindow_Print(&wk->windows[WINDOW_KEYS], wk->printQueue, 1, 0, str, wk->font, PRINT_COLOR(1, 2, 0));
        GFL_StrBufFree(str);
        row++;
    }
    {
        u32 cityKey = KeyInfo_GetCityKey(wk->keyInfo);
        u32 cityMsg = 0;

        if (cityKey != 0) {
#ifdef BLACK2
            cityMsg = 22;
#else
            cityMsg = 23;
#endif
        }
        if (cityMsg != 0) {
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, cityMsg);
            PrintWindow_Print(&wk->windows[row], wk->printQueue, 1, 0, str, wk->font, PRINT_COLOR(1, 2, 0));
            GFL_StrBufFree(str);
            row++;
        }
    }
    {
        u32 chamber = func_020105a0(wk->keyInfo);
        u32 chamberMsg = 0;

        if (chamber == 1) {
            chamberMsg = 25;
        } else if (chamber == 2) {
            chamberMsg = 26;
        }
        if (chamberMsg != 0) {
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, chamberMsg);
            PrintWindow_Print(&wk->windows[row], wk->printQueue, 1, 0, str, wk->font, PRINT_COLOR(1, 2, 0));
            GFL_StrBufFree(str);
        }
    }
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_NewGame);
    PrintWindow_Print(&wk->windows[ITEM_NEW_GAME], wk->printQueue, 0, 4, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_MysteryGift);
    PrintWindow_Print(&wk->windows[ITEM_MYSTERY_GIFT], wk->printQueue, 0, 4, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_BattleCompetition);
    PrintWindow_Print(&wk->windows[ITEM_BATTLE_COMPETITION], wk->printQueue, 0, 4, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_GameSyncSettings);
    PrintWindow_Print(&wk->windows[ITEM_GAME_SYNC_SETTINGS], wk->printQueue, 0, 4, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_NintendoWfcSettings);
    PrintWindow_Print(&wk->windows[ITEM_WFC_SETTINGS], wk->printQueue, 0, 4, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_MicTest);
    PrintWindow_Print(&wk->windows[ITEM_MIC_TEST], wk->printQueue, 0, 4, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_Empty);
    PrintWindow_Print(&wk->windows[ITEM_MB_PARENT], wk->printQueue, 0, 4, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_UnovaLink);
    PrintWindow_Print(&wk->windows[ITEM_UNOVA_LINK], wk->printQueue, 0, 4, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
}

static void StartMenu_FreeWindows(StartMenuWork *wk) {
    u32 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        BmpWin_Free(wk->windows[i].window);
    }
    BmpWin_FreeAllocator();
}

static void StartMenu_InitObj(StartMenuWork *wk) {
    ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 4, 4, 4, 0, 16, 16 };
    ArcTool *arc;
    u32 i;

    ClActSys_Create(&setup, &sVRAMConfig, HEAPID_STARTMENU);
    // Archive 30 has the player's icon for each gender
    arc = GFL_ArcSysCreateFileHandle(30, HEAPID_TAIL(HEAPID_STARTMENU));
    if (getTrainerGender(wk->playerInfo) == 0) {
        wk->chars[OBJ_RES_PLAYER] = func_0204b81c(arc, 5, 0, 1, HEAPID_STARTMENU);
        wk->palettes[OBJ_RES_PLAYER] = func_0204bba0(arc, 4, 1, 0, HEAPID_STARTMENU);
        wk->cellAnims[OBJ_RES_PLAYER] = func_0204bde0(arc, 0, 1, HEAPID_STARTMENU);
    } else {
        wk->chars[OBJ_RES_PLAYER] = func_0204b81c(arc, 7, 0, 1, HEAPID_STARTMENU);
        wk->palettes[OBJ_RES_PLAYER] = func_0204bba0(arc, 6, 1, 0, HEAPID_STARTMENU);
        wk->cellAnims[OBJ_RES_PLAYER] = func_0204bde0(arc, 2, 3, HEAPID_STARTMENU);
    }
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(ARCID_STARTMENU, HEAPID_TAIL(HEAPID_STARTMENU));
    wk->chars[OBJ_RES_NEW] = func_0204b81c(arc, 10, 1, 0, HEAPID_STARTMENU);
    wk->palettes[OBJ_RES_NEW] = func_0204bba0(arc, 11, 0, 0x20, HEAPID_STARTMENU);
    wk->cellAnims[OBJ_RES_NEW] = func_0204bde0(arc, 12, 13, HEAPID_STARTMENU);
    wk->chars[OBJ_RES_WFC_SETTINGS] = func_0204b81c(arc, 18, 1, 0, HEAPID_STARTMENU);
    wk->palettes[OBJ_RES_WFC_SETTINGS] = func_0204bba0(arc, 19, 0, 0x40, HEAPID_STARTMENU);
    wk->cellAnims[OBJ_RES_WFC_SETTINGS] = func_0204bde0(arc, 20, 21, HEAPID_STARTMENU);
    wk->chars[OBJ_RES_KEYS] = func_0204b81c(arc, 14, 1, 1, HEAPID_STARTMENU);
    wk->palettes[OBJ_RES_KEYS] = func_0204bba0(arc, 15, 1, 0x20, HEAPID_STARTMENU);
    wk->cellAnims[OBJ_RES_KEYS] = func_0204bde0(arc, 16, 17, HEAPID_STARTMENU);
    GFL_ArcToolFree(arc);
    wk->clactUnit = func_0204bf1c(ACTOR_COUNT, 0, HEAPID_STARTMENU);
    wk->actors[ACTOR_PLAYER] = func_0204c040(wk->clactUnit, wk->chars[OBJ_RES_PLAYER], wk->palettes[OBJ_RES_PLAYER],
                                             wk->cellAnims[OBJ_RES_PLAYER], &sPlayerActorSetup, 1, HEAPID_STARTMENU);
    func_0204c124(wk->actors[ACTOR_PLAYER], FALSE);
    for (i = 0; i <= 3; i++) {
        wk->actors[i] = func_0204c040(wk->clactUnit, wk->chars[OBJ_RES_NEW], wk->palettes[OBJ_RES_NEW],
                                      wk->cellAnims[OBJ_RES_NEW], &sNewActorSetup, 0, HEAPID_STARTMENU);
        func_0204c520(wk->actors[i], TRUE);
        func_0204c124(wk->actors[i], FALSE);
    }
    wk->actors[ACTOR_WFC_SETTINGS] =
        func_0204c040(wk->clactUnit, wk->chars[OBJ_RES_WFC_SETTINGS], wk->palettes[OBJ_RES_WFC_SETTINGS],
                      wk->cellAnims[OBJ_RES_WFC_SETTINGS], &sWFCSettingsActorSetup, 0, HEAPID_STARTMENU);
    for (i = ACTOR_KEYS; i <= ACTOR_KEYS + 2; i++) {
        wk->actors[i] =
            func_0204c040(wk->clactUnit, wk->chars[OBJ_RES_KEYS], wk->palettes[OBJ_RES_KEYS],
                          wk->cellAnims[OBJ_RES_KEYS], &sKeyActorSetups[i - ACTOR_KEYS], 1, HEAPID_STARTMENU);
        func_0204c124(wk->actors[i], FALSE);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void StartMenu_FreeObj(StartMenuWork *wk) {
    u32 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c108(wk->actors[i]);
    }
    func_0204bf98(wk->clactUnit);
    for (i = 0; i < OBJ_RES_COUNT; i++) {
        func_0204b98c(wk->chars[i]);
    }
    for (i = 0; i < OBJ_RES_COUNT; i++) {
        func_0204bcd0(wk->palettes[i]);
    }
    for (i = 0; i < OBJ_RES_COUNT; i++) {
        func_0204be64(wk->cellAnims[i]);
    }
    func_0204b758();
}

static void StartMenu_InitTextScreens(StartMenuWork *wk) {
    u32 i;

    wk->textScreens = BGWinFrame_Create(BGWINFRAME_TRANSFER_NONE, ITEM_COUNT, HEAPID_STARTMENU);
    for (i = 0; i < ITEM_COUNT; i++) {
        BGWinFrame_InitFrame(wk->textScreens, i, 1, ITEM_WIDTH, sItemLayouts[i].rows);
    }
    BGWinFrame_WriteBmpWin(wk->textScreens, ITEM_CONTINUE, wk->windows[ITEM_CONTINUE].window);
    BGWinFrame_WriteBmpWin(wk->textScreens, ITEM_NEW_GAME, wk->windows[ITEM_NEW_GAME].window);
    BGWinFrame_WriteBmpWin(wk->textScreens, ITEM_MYSTERY_GIFT, wk->windows[ITEM_MYSTERY_GIFT].window);
    BGWinFrame_WriteBmpWin(wk->textScreens, ITEM_BATTLE_COMPETITION, wk->windows[ITEM_BATTLE_COMPETITION].window);
    BGWinFrame_WriteBmpWin(wk->textScreens, ITEM_GAME_SYNC_SETTINGS, wk->windows[ITEM_GAME_SYNC_SETTINGS].window);
    BGWinFrame_WriteBmpWin(wk->textScreens, ITEM_WFC_SETTINGS, wk->windows[ITEM_WFC_SETTINGS].window);
    BGWinFrame_WriteBmpWin(wk->textScreens, ITEM_MIC_TEST, wk->windows[ITEM_MIC_TEST].window);
    BGWinFrame_WriteBmpWin(wk->textScreens, ITEM_MB_PARENT, wk->windows[ITEM_MB_PARENT].window);
    BGWinFrame_WriteBmpWin(wk->textScreens, ITEM_UNOVA_LINK, wk->windows[ITEM_UNOVA_LINK].window);
}

static void StartMenu_FreeTextScreens(StartMenuWork *wk) {
    BGWinFrame_Delete(wk->textScreens);
}

static void StartMenu_SetBlend(void) {
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 16, 3);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG1, GX_PLANEMASK_BG2, 16, 3);
}

// Lists the items that the menu has: continue with a saved game, then the items that are always there, then the ones
// that are added later
static void StartMenu_InitItems(StartMenuWork *wk) {
    u32 n = 0;

    if (SaveControl_IsDataAlreadyPresent(wk->save) == TRUE) {
        wk->items[n] = ITEM_CONTINUE;
        n++;
    }
    wk->items[n] = ITEM_NEW_GAME;
    *(wk->items + n + 1) = ITEM_MYSTERY_GIFT;
    *(wk->items + n + 2) = ITEM_UNOVA_LINK;
    n += 3;
    if (func_0200ca50(wk->gameInfo, 1)) {
        wk->items[n] = ITEM_BATTLE_COMPETITION;
        n++;
    }
    if (func_0200ca50(wk->gameInfo, 2)) {
        wk->items[n] = ITEM_GAME_SYNC_SETTINGS;
        n++;
    }
    wk->items[n] = ITEM_WFC_SETTINGS;
    *(wk->items + n + 1) = ITEM_MIC_TEST;
    n += 2;
    while (n < ITEM_COUNT) {
        wk->items[n++] = ITEM_NONE;
    }
}

static void StartMenu_LoadItemScreens(StartMenuWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_STARTMENU, HEAPID_TAIL(HEAPID_STARTMENU));
    u32 i;
    u32 size;
    void *file;
    NNSG2dScreenData *screen;

    for (i = 0; i < ITEM_COUNT; i++) {
        size = ITEM_WIDTH * sizeof(u16) * sItemLayouts[i].rows;
        wk->itemScreens[i] = GFL_HeapAllocate(HEAPID_STARTMENU, size, FALSE, "startmenu.c", 2240);
        file = GFL_G2DIOReadNSCRArc(arc, sItemLayouts[i].screenFile, TRUE, &screen, HEAPID_STARTMENU);
        sys_memcpy16(screen->rawData, wk->itemScreens[i], size);
        GFL_HeapFree(file);
    }
    GFL_ArcToolFree(arc);
}

static void StartMenu_FreeItemScreens(StartMenuWork *wk) {
    u32 i;

    for (i = 0; i < ITEM_COUNT; i++) {
        GFL_HeapFree(wk->itemScreens[i]);
    }
}

static void StartMenu_SetActorY(StartMenuWork *wk, u32 actor, s16 y) {
    ClActorPos pos;

    func_0204c178(wk->actors[actor], &pos, 0);
    pos.y = y;
    func_0204c140(wk->actors[actor], &pos, 0);
}

static void StartMenu_DrawItems(StartMenuWork *wk) {
    u32 i;
    s8 row = FIRST_ITEM_ROW;

    wk->cursor = 0;
    wk->scrollY = 0;
    wk->cursorRow = FIRST_ITEM_ROW;
    for (i = 0; i < ITEM_COUNT; i++) {
        if (wk->items[i] == ITEM_NONE) {
            break;
        }
        StartMenu_DrawItem(wk, wk->items[i], row);
        if (wk->items[i] != ITEM_CONTINUE) {
            if (wk->items[i] == ITEM_WFC_SETTINGS) {
                StartMenu_SetActorY(wk, ACTOR_WFC_SETTINGS, row * 8 + sItemLayouts[wk->items[i]].rows * 8 / 2);
                func_0204c124(wk->actors[ACTOR_WFC_SETTINGS], TRUE);
            } else if (wk->items[i] == ITEM_BATTLE_COMPETITION) {
                if (func_0200ca50(wk->gameInfo, 1) == TRUE) {
                    StartMenu_SetActorY(wk, 1, row * 8 + sItemLayouts[wk->items[i]].rows * 8 / 2);
                    func_0204c124(wk->actors[1], TRUE);
                }
            } else if (wk->items[i] == ITEM_GAME_SYNC_SETTINGS) {
                if (func_0200ca50(wk->gameInfo, 2) == TRUE) {
                    StartMenu_SetActorY(wk, 2, row * 8 + sItemLayouts[wk->items[i]].rows * 8 / 2);
                    func_0204c124(wk->actors[2], TRUE);
                }
            } else if (wk->items[i] == ITEM_MB_PARENT) {
                if (func_0200ca50(wk->gameInfo, 3) == TRUE) {
                    StartMenu_SetActorY(wk, 3, row * 8 + sItemLayouts[wk->items[i]].rows * 8 / 2);
                    func_0204c124(wk->actors[3], TRUE);
                }
            }
        }
        row += (s8)sItemLayouts[wk->items[i]].rows;
    }
    StartMenu_SetItemPalette(wk, wk->items[0], wk->cursorRow, ITEM_PALETTE_SELECTED);
}

// Draws an item's screen on BG 2 and its name on BG 1
static void StartMenu_DrawItem(StartMenuWork *wk, u32 item, u32 row) {
    u16 rows = sItemLayouts[item].rows;
    u8 y = row;

    GFL_BGSysLoadScrArea(2, 3, y, ITEM_WIDTH, rows, wk->itemScreens[item], 0, 0, ITEM_WIDTH, rows);
    GFL_BGSysLoadScrArea(1, 3, y, ITEM_WIDTH, rows, BGWinFrame_GetScreen(wk->textScreens, item), 0, 0, ITEM_WIDTH, rows);
    GFL_BGSysQueueScrLoad(1);
    GFL_BGSysQueueScrLoad(2);
}

static void StartMenu_SetItemPalette(StartMenuWork *wk, u32 item, s8 row, u8 palette) {
    GFL_BGSysSetScrPaletteNo(2, 3, row, ITEM_WIDTH, sItemLayouts[item].rows, palette);
    GFL_BGSysQueueScrLoad(2);
}

// The row of the BG's screen that is shown at a screen row
static s8 StartMenu_GetScreenRow(StartMenuWork *wk, s8 row) {
    return row + wk->scrollY / 8;
}

// Shows the saved game on the sub screen, with the keys that are set
static void StartMenu_ShowSavedGame(StartMenuWork *wk) {
    BOOL anyKey;
    u32 actor;
    u32 window;
    s32 i;

    func_0204c124(wk->actors[ACTOR_PLAYER], TRUE);
    anyKey = FALSE;
    actor = ACTOR_KEYS;
    window = WINDOW_KEYS;
    for (i = WINDOW_SAVED_GAME; i <= WINDOW_BADGES; i++) {
        BmpWin_FlushMap(wk->windows[i].window);
    }
    if (GetGameDifficulty(wk->keyInfo) != GAME_DIFFICULTY_NORMAL) {
        BmpWin_FlushMap(wk->windows[WINDOW_KEYS].window);
        func_0204c124(wk->actors[ACTOR_KEYS], TRUE);
        anyKey = TRUE;
        actor++;
        window++;
    }
    if (KeyInfo_GetCityKey(wk->keyInfo) == TRUE) {
        BmpWin_FlushMap(wk->windows[window].window);
        func_0204c124(wk->actors[actor], TRUE);
        anyKey = TRUE;
        actor++;
        window++;
    }
    if (func_020105a0(wk->keyInfo) != 0) {
        BmpWin_FlushMap(wk->windows[window].window);
        func_0204c124(wk->actors[actor], TRUE);
        anyKey = TRUE;
    }
    if (anyKey) {
        BmpWin_FlushMap(wk->windows[WINDOW_KEY_SYSTEM].window);
        loadBGScrToVramByNarcNoReserveNegAlign(ARCID_STARTMENU, 9, 5, 0, 0, TRUE, HEAPID_STARTMENU);
    } else {
        loadBGScrToVramByNarcNoReserveNegAlign(ARCID_STARTMENU, 8, 5, 0, 0, TRUE, HEAPID_STARTMENU);
    }
    GFL_BGSysQueueScrLoad(4);
    GFL_BGSysQueueScrLoad(5);
}

static void StartMenu_HideSavedGame(StartMenuWork *wk) {
    s32 i;

    for (i = WINDOW_SAVED_GAME; i <= WINDOW_KEYS + 2; i++) {
        BmpWin_ClearScreen(wk->windows[i].window);
    }
    func_0204c124(wk->actors[ACTOR_KEYS], FALSE);
    func_0204c124(wk->actors[ACTOR_KEYS + 1], FALSE);
    func_0204c124(wk->actors[ACTOR_KEYS + 2], FALSE);
    func_0204c124(wk->actors[ACTOR_PLAYER], FALSE);
    GFL_BGSysQueueScrLoad(4);
    GFL_BGSysFillScrAsync(5, 0);
}

// Moves the markers and the icon with the items as they scroll
static void StartMenu_MoveItemActors(StartMenuWork *wk, s32 dy) {
    u32 i;
    ClActorPos pos;

    for (i = 0; i < ACTOR_WFC_SETTINGS + 1; i++) {
        func_0204c178(wk->actors[i], &pos, 0);
        pos.y += (s16)dy;
        func_0204c140(wk->actors[i], &pos, 0);
    }
}

// Moves the cursor up or down an item. The list only scrolls when it has the saved game, and so more items than fit
static BOOL StartMenu_MoveCursor(StartMenuWork *wk, s32 dir) {
    u8 item = wk->items[wk->cursor];
    s8 row = 0;
    u16 rows;
    u8 nextItem;
    s8 next;

    wk->scrollFrames = 0;
    if (dir < 0) {
        if (wk->cursor == 0) {
            return FALSE;
        }
        next = wk->cursor + dir;
        rows = sItemLayouts[wk->items[next]].rows;
        row = wk->cursorRow - rows;
        StartMenu_SetItemPalette(wk, item, StartMenu_GetScreenRow(wk, wk->cursorRow), ITEM_PALETTE_NORMAL);
        if (wk->items[0] == ITEM_CONTINUE) {
            if (row < FIRST_ITEM_ROW) {
                if (next == 0) {
                    wk->scrollSpeed = -((rows - (wk->cursorRow - FIRST_ITEM_ROW)) / SCROLL_FRAMES * 8);
                    wk->cursorRow = FIRST_ITEM_ROW;
                } else {
                    wk->scrollSpeed = -(rows / SCROLL_FRAMES * 8);
                }
            } else {
                wk->cursorRow = row;
            }
        } else {
            wk->cursorRow = row;
        }
    } else {
        if (wk->cursor + dir >= ITEM_COUNT) {
            return FALSE;
        }
        if (wk->items[wk->cursor + dir] == ITEM_NONE) {
            return FALSE;
        }
        nextItem = wk->items[(s8)(wk->cursor + dir)];
        row = wk->cursorRow + sItemLayouts[item].rows;
        StartMenu_SetItemPalette(wk, item, StartMenu_GetScreenRow(wk, wk->cursorRow), ITEM_PALETTE_NORMAL);
        if (wk->items[0] == ITEM_CONTINUE) {
            if (row == LAST_ITEM_ROW) {
                wk->scrollSpeed = sItemLayouts[nextItem].rows / SCROLL_FRAMES * 8;
            } else if (row + sItemLayouts[nextItem].rows > LAST_ITEM_ROW) {
                wk->scrollSpeed = sItemLayouts[nextItem].rows / SCROLL_FRAMES * 8;
            } else {
                wk->cursorRow = row;
            }
        } else {
            wk->cursorRow = row;
        }
    }
    wk->cursor += dir;
    return TRUE;
}

static void StartMenu_SetItemActorsVisible(StartMenuWork *wk, BOOL visible) {
    u32 i;

    if (visible == FALSE) {
        for (i = 0; i <= ACTOR_WFC_SETTINGS; i++) {
            func_0204c124(wk->actors[i], visible);
        }
        return;
    }
    for (i = 0; i < ITEM_COUNT; i++) {
        if (wk->items[i] == ITEM_NONE) {
            return;
        }
        if (wk->items[i] == ITEM_BATTLE_COMPETITION) {
            if (func_0200ca50(wk->gameInfo, 1) == TRUE) {
                func_0204c124(wk->actors[1], TRUE);
            }
        } else if (wk->items[i] == ITEM_GAME_SYNC_SETTINGS) {
            if (func_0200ca50(wk->gameInfo, 2) == TRUE) {
                func_0204c124(wk->actors[2], TRUE);
            }
        } else if (wk->items[i] == ITEM_MB_PARENT) {
            if (func_0200ca50(wk->gameInfo, 3) == TRUE) {
                func_0204c124(wk->actors[3], TRUE);
            }
        } else if (wk->items[i] == ITEM_WFC_SETTINGS) {
            func_0204c124(wk->actors[ACTOR_WFC_SETTINGS], TRUE);
        }
    }
}

// Draws the frame of a notice with the characters of the frame loaded to the BG
static void StartMenu_DrawFrame(u8 x, u8 y, u8 width, u8 height, u8 bg) {
    GFL_BGSysFillScrArea(bg, 1, x, y, 1, 1, 1);
    GFL_BGSysFillScrArea(bg, 3, x + width - 1, y, 1, 1, 1);
    GFL_BGSysFillScrArea(bg, 7, x, y + height - 1, 1, 1, 1);
    GFL_BGSysFillScrArea(bg, 9, x + width - 1, y + height - 1, 1, 1, 1);
    GFL_BGSysFillScrArea(bg, 2, x + 1, y, width - 2, 1, 1);
    GFL_BGSysFillScrArea(bg, 8, x + 1, y + height - 1, width - 2, 1, 1);
    GFL_BGSysFillScrArea(bg, 4, x, y + 1, 1, height - 2, 1);
    GFL_BGSysFillScrArea(bg, 6, x + width - 1, y + 1, 1, height - 2, 1);
    GFL_BGSysFillScrArea(bg, 5, x + 1, y + 1, width - 2, height - 2, 1);
    GFL_BGSysQueueScrLoad(bg);
}

static void StartMenu_ClearFrame(u8 x, u8 y, u8 width, u8 height, u8 bg) {
    GFL_BGSysFillScrArea(bg, 0, x, y, width, height, 0);
    GFL_BGSysQueueScrLoad(bg);
}

// The notice goes on the main engine's BG 0 over a frame on the part of BGs 1 and 2 past the items, which they are
// scrolled to
static void StartMenu_OpenNewGameWarning(StartMenuWork *wk) {
    StrBuf *str;

    wk->notice.window = BmpWin_CreateDynamic(0, 2, 2, 30, 20, 5, 1);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_Warning_2);
    PrintWindow_Print(&wk->notice, wk->printQueue, 0, 0, str, wk->font, PRINT_COLOR(5, 6, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_TheresAlreadySavedGame);
    PrintWindow_Print(&wk->notice, wk->printQueue, 0, 24, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_ButtonBeginAdventure);
    PrintWindow_Print(&wk->notice, wk->printQueue, 0, 128, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_BButtonReturnMenu);
    PrintWindow_Print(&wk->notice, wk->printQueue, 0, 144, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    BmpWin_FlushMap(wk->notice.window);
    GFL_BGSysQueueScrLoad(0);
    StartMenu_DrawFrame(32, 1, 32, 22, 2);
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_X, 256);
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_Y, 0);
    GFL_BGSysMoveBGReq(2, BG_MOVE_SET_X, 256);
    GFL_BGSysMoveBGReq(2, BG_MOVE_SET_Y, 0);
    StartMenu_SetItemActorsVisible(wk, FALSE);
}

// Closes the notice, and unless the menu is being left, brings the items back
static void StartMenu_CloseNotice(StartMenuWork *wk, BOOL leaving) {
    BmpWin_Free(wk->notice.window);
    if (leaving == TRUE) {
        return;
    }
    StartMenu_ClearFrame(32, 1, 32, 22, 2);
    GFL_BGSysFillScrAsync(0, 0);
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_X, 0);
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_Y, wk->scrollY);
    GFL_BGSysMoveBGReq(2, BG_MOVE_SET_X, 0);
    GFL_BGSysMoveBGReq(2, BG_MOVE_SET_Y, wk->scrollY);
    if (wk->items[0] == ITEM_CONTINUE) {
        StartMenu_SetItemActorsVisible(wk, TRUE);
    }
}

// The warning about wireless communications on the sub screen, over the saved game
static void StartMenu_OpenCGearWarning(StartMenuWork *wk) {
    StrBuf *str;

    wk->notice.window = BmpWin_CreateDynamic(4, 2, 3, 30, 19, 5, 1);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_Warning);
    PrintWindow_Print(&wk->notice, wk->printQueue, 0, 0, str, wk->font, PRINT_COLOR(5, 6, 0));
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, StartmenuFieldmapCtrlHybrid_Text_CanEnjoyGameIts);
    PrintWindow_Print(&wk->notice, wk->printQueue, 0, 24, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    BmpWin_FlushMap(wk->notice.window);
    GFL_BGSysQueueScrLoad(4);
    StartMenu_DrawFrame(0, 1, 32, 22, 5);
}

static void StartMenu_CloseCGearWarning(StartMenuWork *wk) {
    BmpWin_Free(wk->notice.window);
}

static void StartMenu_OpenDSiNotice(StartMenuWork *wk) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_WIFI_ERROR, HEAPID_TAIL(HEAPID_STARTMENU));
    StrBuf *str;

    wk->notice.window = BmpWin_CreateDynamic(0, 2, 2, 30, 20, 5, 1);
    str = GFL_MsgDataLoadStrbufNew(msgData, WifiError_Text_PleaseConfigureConnectionSettings);
    PrintWindow_Print(&wk->notice, wk->printQueue, 0, 0, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
    BmpWin_FlushMap(wk->notice.window);
    GFL_BGSysQueueScrLoad(0);
    StartMenu_DrawFrame(32, 1, 32, 22, 2);
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_X, 256);
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_Y, 0);
    GFL_BGSysMoveBGReq(2, BG_MOVE_SET_X, 256);
    GFL_BGSysMoveBGReq(2, BG_MOVE_SET_Y, 0);
    StartMenu_SetItemActorsVisible(wk, FALSE);
}

static void StartMenu_Print(StartMenuWork *wk, u32 messageId) {
    BmpWin *window;

    GFL_MsgDataLoadStrbuf(wk->msgData, messageId, wk->strbuf);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_MESSAGE].window), 15);
    BmpWin_DrawFrame(wk->windows[WINDOW_MESSAGE].window, WINFRAME_TRANSFER_NONE, 1, 14);
    wk->printStream = func_02022268(wk->windows[WINDOW_MESSAGE].window, 0, 0, wk->strbuf, wk->font, func_02017bcc(),
                                    wk->tcbManager, 10, HEAPID_STARTMENU, 15);
    wk->continued = FALSE;
    window = wk->windows[WINDOW_MESSAGE].window;
    BmpWin_Transfer(window);
}

static void StartMenu_ClearMessage(StartMenuWork *wk) {
    BmpWin_ClearFrame(wk->windows[WINDOW_MESSAGE].window, WINFRAME_TRANSFER_VBLANK);
}

// Returns FALSE once the message has been printed and read, or when there is none
static BOOL StartMenu_UpdatePrint(StartMenuWork *wk) {
    if (wk->printStream == NULL) {
        return FALSE;
    }
    KeyCursor_Update(wk->keyCursor, wk->printStream, wk->windows[WINDOW_MESSAGE].window);
    switch (func_020223b4(wk->printStream)) {
    case PRINT_STREAM_RUNNING:
        if (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            func_020223e0(wk->printStream, 0);
        }
        wk->continued = FALSE;
        break;
    case PRINT_STREAM_PAUSED:
        if (wk->continued == FALSE && (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            func_020223bc(wk->printStream);
            wk->continued = TRUE;
        }
        break;
    case PRINT_STREAM_DONE:
        func_020223cc(wk->printStream);
        wk->printStream = NULL;
        wk->continued = FALSE;
        return FALSE;
    }
    return TRUE;
}

static void StartMenu_OpenYesNo(StartMenuWork *wk) {
    ConfirmDialogSetup setup;

    setup.bg = 0;
    setup.x = 24;
    setup.y = 13;
    setup.palette = 15;
    setup.unk4 = 0;
    wk->dialog = ShopUI_CreateConfirmDialog(&setup, 1, 14, 0, HEAPID_STARTMENU);
}

static u32 StartMenu_WipeIn(StartMenuWork *wk, u32 next) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, HEAPID_STARTMENU);
    wk->wipeNextState = next;
    return STATE_WAIT_WIPE;
}

static u32 StartMenu_WipeOut(StartMenuWork *wk, u32 next) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, HEAPID_STARTMENU);
    wk->wipeNextState = next;
    return STATE_WAIT_WIPE;
}

static u32 StartMenu_StartBlink(StartMenuWork *wk, u32 next) {
    wk->blinkWait = 0;
    wk->blinkSeq = 0;
    wk->blinkNextState = next;
    return STATE_BLINK;
}

static BOOL StartMenu_CheckBrokenSave(StartMenuWork *wk) {
    wk->brokenBlocks = func_02007464(wk->save);
    if (wk->brokenBlocks != 0) {
        return TRUE;
    }
    return FALSE;
}
