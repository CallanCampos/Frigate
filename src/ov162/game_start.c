#include "types.h"
#include "app/game_start.h"
#include "app/name_entry.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "demo/intro.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/config.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/save_control_intr.h"
#include "save/save_outside.h"
#include "system/bmp_winframe.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"

// Starting the game: a new game runs the intro and the name entries while it creates the save data, and a continue
// loads the save. Both then start the game system. A debug screen, whose questions are blank outside Japan, can set
// the text, the gender and whether the C-Gear is on first

// The most sound sequences that the intro and the name entry can load together
#define PRELOAD_SEQ_MAX 30

// The new game's steps
enum {
    NEW_GAME_INTRO,
    NEW_GAME_WAIT_INTRO,
    NEW_GAME_NAME_ENTRY,
    NEW_GAME_WAIT_NAME_ENTRY,
    NEW_GAME_INTRO_PLAYER_NAMED,
    NEW_GAME_AFTER_PLAYER_NAMED,
    NEW_GAME_WAIT_INTRO_PLAYER_NAMED,
    NEW_GAME_RIVAL_NAME_ENTRY,
    NEW_GAME_WAIT_RIVAL_NAME_ENTRY,
    NEW_GAME_INTRO_RIVAL_NAMED,
    NEW_GAME_AFTER_RIVAL_NAMED,
    NEW_GAME_WAIT_INTRO_RIVAL_NAMED,
    NEW_GAME_END,
};

// The continue's parameters: show the debug screen first, or start with the C-Gear off or on, as the start menu asks
enum {
    CONTINUE_DEBUG,
    CONTINUE_CGEAR_OFF,
    CONTINUE_CGEAR_ON,
};

// The debug screen's questions
enum {
    DEBUG_QUESTION_TEXT,
    DEBUG_QUESTION_GENDER,
    DEBUG_QUESTION_CGEAR,
    DEBUG_QUESTION_END,
};

// The work of the new game and the continue. The first fields are the debug screen's parameter
typedef struct {
    // FALSE asks every question of the debug screen, TRUE only the last
    BOOL onlyCGearQuestion;
    Config *config;
    PlayerInfo *playerInfo;
    TrainerGameInfoSave *gameInfo;
    IntroParam introParam;
    NameEntryParam *nameEntryParam;
    SaveControlIntr *saveTask;
    GameProcManager *procManager;
    u32 preloadedSeqs;
    u8 unk3C[0x18];
    u32 pokeVoice;
    // The new game's player and config, which are copied into the save at the end
    PlayerInfo *newPlayerInfo;
    Config *newConfig;
    SaveControl *save;
    u16 rivalName[8];
} GameStartWork;

typedef struct {
    TouchRect rects[3];
} TouchRectTable;

typedef struct {
    u8 cursor;
    u32 question;
    Font *font;
    BmpWin *questionWindow;
    BmpWin *answerWindows[2];
    u16 cursorColorAngle;
    GXRgb cursorColor;
} DebugGameStartWork;

static BOOL NewGame_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL NewGame_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL NewGame_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Continue_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Continue_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Continue_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void NewGame_SetUpPlayer(PlayerInfo *info, StrBuf *name, u32 gender);
static void func_ov162_021a04a8(SaveControl *save);
static void func_ov162_021a04e8(SaveControl *save);
static BOOL DebugGameStart_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL DebugGameStart_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL DebugGameStart_Main(GameProc *proc, u32 *state, void *param, void *work);
static void DebugGameStart_InitBG(DebugGameStartWork *wk);
static void DebugGameStart_CreateBG(const BGSetup *setup, u8 bg);
static void DebugGameStart_CreateWindows(DebugGameStartWork *wk);
static void DebugGameStart_FreeWindows(DebugGameStartWork *wk);
static void DebugGameStart_DrawCursor(DebugGameStartWork *wk);
static u32 DebugGameStart_UpdateMenu(DebugGameStartWork *wk);
static void DebugGameStart_CycleCursorColor(DebugGameStartWork *wk);

// The static data is declared in this order to keep the original layout (tools/decomp/rodata_order.py)
static const TouchRectTable sDebugTouchRects = { {
    { 96, 112, 48, 208 },
    { 128, 144, 48, 208 },
    { TOUCH_RECT_END, 0, 0, 0 },
} };

static const BGSysVRAMConfig sDebugVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,      GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const BGSysLCDConfig sDebugLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const BGSetup sDebugBG3Setup = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x0c000), 0x8000,
    GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE,
};

static const BGSetup sDebugBG1Setup = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
    GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
};

static const BGSetup sDebugBG5Setup = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
    GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
};

const GameProcFunctions CONTINUE_PROC_FUNCTIONS = { Continue_Init, Continue_Main, Continue_Exit };
const GameProcFunctions NEW_GAME_PROC_FUNCTIONS = { NewGame_Init, NewGame_Main, NewGame_Exit };
const GameProcFunctions DEBUG_GAME_START_PROC_FUNCTIONS = { DebugGameStart_Init, DebugGameStart_Main,
                                                            DebugGameStart_Exit };

void GameStart_NewGame(void) {
    GCTX_ProcMgrReplaceProc(OVERLAY_ID(162), &NEW_GAME_PROC_FUNCTIONS, NULL);
}

void GameStart_ContinueCGearOff(void) {
    GCTX_ProcMgrReplaceProc(OVERLAY_ID(162), &CONTINUE_PROC_FUNCTIONS, (void *)CONTINUE_CGEAR_OFF);
}

void GameStart_ContinueCGearOn(void) {
    GCTX_ProcMgrReplaceProc(OVERLAY_ID(162), &CONTINUE_PROC_FUNCTIONS, (void *)CONTINUE_CGEAR_ON);
}

static BOOL NewGame_Init(GameProc *proc, u32 *state, void *param, void *work) {
    SaveControl *save = SaveControl_GetInstance();
    GameStartWork *wk = GFL_ProcInitSubsystem(proc, sizeof(GameStartWork), HEAPID_USER);
    u32 seqs[PRELOAD_SEQ_MAX];
    u32 nameEntryCount;
    u32 introCount;
    u32 i;
    u32 pokeVoice;

    wk->saveTask = SaveControlIntr_Create(HEAPID_USER, save);
    func_ov162_021a04a8(save);
    func_0200749c(save);
    wk->save = save;
    wk->newPlayerInfo = func_02008b0c(HEAPID_USER);
    wk->newConfig = func_0200898c(HEAPID_USER);
    sys_memset(wk->rivalName, 0, sizeof(wk->rivalName));
    wk->onlyCGearQuestion = FALSE;
    wk->config = wk->newConfig;
    wk->playerInfo = wk->newPlayerInfo;
    wk->nameEntryParam = setupNameEntry(1, 0, 0, 0, 7, 0, 0);
    wk->procManager = CreateGameProcManager(HEAPID_USER);

    // The name entry's and the intro's sounds, loaded now so that they play at once
    i = 0;
    GFL_OvlLoad(OVERLAY_ID(280));
    nameEntryCount = NAME_ENTRY_SOUND_COUNT;
    for (; i < nameEntryCount; i++) {
        seqs[i] = NAME_ENTRY_SOUNDS[i];
    }
    GFL_OvlUnload(OVERLAY_ID(280));
    GFL_OvlLoad(OVERLAY_ID(294));
    introCount = INTRO_SOUND_COUNT;
    for (i = 0; i < introCount; i++) {
        seqs[nameEntryCount + i] = INTRO_SOUNDS[i];
    }
    GFL_OvlUnload(OVERLAY_ID(294));
    wk->preloadedSeqs = func_02005af4(seqs, nameEntryCount + introCount);

    // The intro's Pokémon
    pokeVoice = PokeVoice_Load(SPECIES_CINCCINO, 0, 64, 0, 0, 0, 0, 0);
    wk->pokeVoice = pokeVoice;
    wk->introParam.playerInfo = wk->newPlayerInfo;
    wk->introParam.config = wk->newConfig;
    wk->introParam.mode = INTRO_MODE_START;
    wk->introParam.saveTask = wk->saveTask;
    wk->introParam.pokeVoice = pokeVoice;
    wk->introParam.rivalName = wk->rivalName;
    return TRUE;
}

static BOOL NewGame_Main(GameProc *proc, u32 *state, void *param, void *work) {
    GameStartWork *wk = work;
    BOOL running;
    MsgData *msgData;
    StrBuf *name;

    SaveControlIntr_Update(wk->saveTask);
    running = GFL_ProcMgrUpdate(wk->procManager);
    switch (*state) {
    case NEW_GAME_INTRO:
        QueueGameProc(wk->procManager, OVERLAY_ID(294), &INTRO_PROC_FUNCTIONS, &wk->introParam);
        *state = NEW_GAME_WAIT_INTRO;
        break;
    case NEW_GAME_WAIT_INTRO:
        if (running == TRUE) {
            break;
        }
        *state = NEW_GAME_NAME_ENTRY;
        break;
    case NEW_GAME_NAME_ENTRY:
        wk->nameEntryParam->gender = getTrainerGender(wk->playerInfo);
        wk->nameEntryParam->saveTask = wk->saveTask;
        QueueGameProc(wk->procManager, OVERLAY_ID(280), &NAME_ENTRY_PROC_FUNCTIONS, wk->nameEntryParam);
        *state = NEW_GAME_WAIT_NAME_ENTRY;
        break;
    case NEW_GAME_WAIT_NAME_ENTRY:
        if (running == TRUE) {
            break;
        }
        *state = NEW_GAME_INTRO_PLAYER_NAMED;
        break;
    case NEW_GAME_INTRO_PLAYER_NAMED:
        NewGame_SetUpPlayer(wk->newPlayerInfo, wk->nameEntryParam->name, getTrainerGender(wk->newPlayerInfo));
        wk->introParam.mode = INTRO_MODE_PLAYER_NAMED;
        QueueGameProc(wk->procManager, OVERLAY_ID(294), &INTRO_PROC_FUNCTIONS, &wk->introParam);
        *state = NEW_GAME_WAIT_INTRO_PLAYER_NAMED;
        break;
    case NEW_GAME_WAIT_INTRO_PLAYER_NAMED:
        if (running == TRUE) {
            break;
        }
        *state = NEW_GAME_AFTER_PLAYER_NAMED;
        break;
    case NEW_GAME_AFTER_PLAYER_NAMED:
        if (wk->introParam.result == INTRO_RESULT_ENTER_RIVAL_NAME) {
            *state = NEW_GAME_RIVAL_NAME_ENTRY;
        } else {
            *state = NEW_GAME_NAME_ENTRY;
        }
        break;
    case NEW_GAME_RIVAL_NAME_ENTRY:
        // Hugh
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_GAME_START, HEAPID_USER);
        name = GFL_MsgDataLoadStrbufNew(msgData, 25);
        GFL_StrBufCopy(wk->nameEntryParam->name, name);
        GFL_StrBufFree(name);
        GFL_MsgDataFree(msgData);
        wk->nameEntryParam->mode = NAME_ENTRY_RIVAL;
        QueueGameProc(wk->procManager, OVERLAY_ID(280), &NAME_ENTRY_PROC_FUNCTIONS, wk->nameEntryParam);
        *state = NEW_GAME_WAIT_RIVAL_NAME_ENTRY;
        break;
    case NEW_GAME_WAIT_RIVAL_NAME_ENTRY:
        if (running == TRUE) {
            break;
        }
        *state = NEW_GAME_INTRO_RIVAL_NAMED;
        break;
    case NEW_GAME_INTRO_RIVAL_NAMED:
        GFL_StrBufStoreString(wk->nameEntryParam->name, wk->rivalName, NELEMS(wk->rivalName));
        wk->introParam.mode = INTRO_MODE_RIVAL_NAMED;
        QueueGameProc(wk->procManager, OVERLAY_ID(294), &INTRO_PROC_FUNCTIONS, &wk->introParam);
        *state = NEW_GAME_WAIT_INTRO_RIVAL_NAMED;
        break;
    case NEW_GAME_WAIT_INTRO_RIVAL_NAMED:
        if (running == TRUE) {
            break;
        }
        *state = NEW_GAME_AFTER_RIVAL_NAMED;
        break;
    case NEW_GAME_AFTER_RIVAL_NAMED:
        if (wk->introParam.result == INTRO_RESULT_DONE) {
            *state = NEW_GAME_END;
        } else {
            *state = NEW_GAME_RIVAL_NAME_ENTRY;
        }
        break;
    case NEW_GAME_END:
        return TRUE;
    }
    return FALSE;
}

// Puts the new player, config and rival name into the save, and starts the game system from the opening
static BOOL NewGame_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GameStartWork *wk = work;
    SaveControl *save = SaveControl_GetInstance();
    VecFx32 spawnPos = { 0, 0, 0 };
    GameSystemProcData *procData;

    func_02005b60(wk->preloadedSeqs);
    SaveControlIntr_Free(wk->saveTask);
    func_02008b34(wk->newPlayerInfo, SaveControl_GetPlayerInfo(save));
    initConfig(wk->newConfig, (Config *)getTrainerDataBlkAddress(save));
    copyRivalNameIntoHollowBlock(getHollow_RivalData(save), wk->rivalName);
    GFL_HeapFree(wk->newPlayerInfo);
    GFL_HeapFree(wk->newConfig);
    FreeGameProcManager(wk->procManager);
    func_ov162_021a04e8(save);
    procData = GameSystem_CreateProcData(GAME_ENTRYPOINT_OPENING, 0, &spawnPos, 0);
    func_ov012_02165ae8(wk->nameEntryParam);
    GFL_ProcReleaseSubsystem(proc);
    GCTX_ProcMgrReplaceProc(OVERLAY_NONE, &GAMESYSTEM_PROC_FUNCTIONS, procData);
    return TRUE;
}

static BOOL Continue_Init(GameProc *proc, u32 *state, void *param, void *work) {
    GameStartWork *wk = GFL_ProcInitSubsystem(proc, sizeof(GameStartWork), HEAPID_USER);

    wk->onlyCGearQuestion = TRUE;
    wk->config = (Config *)getTrainerDataBlkAddress(SaveControl_GetInstance());
    wk->gameInfo = getTrainerGameInfoAddress(SaveControl_GetInstance());
    wk->playerInfo = NULL;
    return TRUE;
}

static BOOL Continue_Main(GameProc *proc, u32 *state, void *param, void *work) {
    GameStartWork *wk = work;

    switch (*state) {
    case 0:
        if ((u32)param == CONTINUE_DEBUG) {
            GCTX_ProcMgrQueueProc(OVERLAY_NONE, &DEBUG_GAME_START_PROC_FUNCTIONS, wk);
        } else if ((u32)param == CONTINUE_CGEAR_OFF) {
            func_02008af0(wk->config, FALSE);
        } else if ((u32)param == CONTINUE_CGEAR_ON) {
            func_02008af0(wk->config, TRUE);
        }
        (*state)++;
        break;
    case 1:
        return TRUE;
    }
    return TRUE;
}

// Loads the save and starts the game system where the player saved. Whether the C-Gear is on and two values of the
// game info keep the values of this session across the load
static BOOL Continue_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GameStartWork *wk = work;
    SaveControl *save = SaveControl_GetInstance();
    u32 cgearOn = func_02008ae8(wk->config);
    u32 gameInfoValues[2];
    SaveLocation location;

    gameInfoValues[0] = func_0200ca64(wk->gameInfo);
    gameInfoValues[1] = func_0200ca74(wk->gameInfo);
    func_02007324(save);
    func_02008ab4((Config *)getTrainerDataBlkAddress(save));
    func_02008fb8(save, &location);
    GCTX_ProcMgrReplaceProc(OVERLAY_NONE, &GAMESYSTEM_PROC_FUNCTIONS,
                            GameSystem_CreateProcData(GAME_ENTRYPOINT_FIELD_CONTINUE, location.zoneId,
                                                      &location.pos, location.unk18));
    func_02008af0(wk->config, cgearOn);
    func_0200ca6c(wk->gameInfo, gameInfoValues[0]);
    func_0200ca78(wk->gameInfo, gameInfoValues[1]);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static void NewGame_SetUpPlayer(PlayerInfo *info, StrBuf *name, u32 gender) {
    copyTrainerNameFromStrbuf(info, name);
    setTrainerGender(info, gender);
    setIDAsUInt(info, GFL_RandomLCAlt(0xffffffff));
    func_02008bf8(info, GFL_RandomMTRange(8) + getTrainerGender(info) * 8);
}

static void func_ov162_021a04a8(SaveControl *save) {
    void *work;

    if (func_02007464(save) & 2) {
        GFL_OvlLoad(OVERLAY_ID(331));
        work = SaveOutside_Load(HEAPID_USER);
        if (SaveOutside_IsKeysLoaded(work)) {
            SaveOutside_EraseKeys(HEAPID_USER);
        }
        SaveOutside_Free(work);
        GFL_OvlUnload(OVERLAY_ID(331));
    }
}

static void func_ov162_021a04e8(SaveControl *save) {
    void *work;

    if (SaveControl_GetStatus(save) == 1 && !SaveControl_IsDataAlreadyPresent(save)) {
        GFL_OvlLoad(OVERLAY_ID(331));
        work = SaveOutside_Load(HEAPID_USER);
        if (SaveOutside_IsGiftsLoaded(work)) {
            SaveOutside_CopyGiftsToSave(work, save);
        }
        if (SaveOutside_IsKeysLoaded(work)) {
            SaveOutside_CopyKeysToSave(work, save);
        }
        SaveOutside_Free(work);
        GFL_OvlUnload(OVERLAY_ID(331));
    }
}

static BOOL DebugGameStart_Init(GameProc *proc, u32 *state, void *param, void *work) {
    GameStartWork *gameStart = param;
    DebugGameStartWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_DEBUG_GENDER_SELECT, 0x80000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(DebugGameStartWork), HEAPID_DEBUG_GENDER_SELECT);
    if (gameStart->onlyCGearQuestion == FALSE) {
        wk->question = DEBUG_QUESTION_TEXT;
    } else {
        wk->question = DEBUG_QUESTION_CGEAR;
    }
    wk->cursorColorAngle = 0;
    DebugGameStart_InitBG(wk);
    DebugGameStart_CreateWindows(wk);
    return TRUE;
}

static BOOL DebugGameStart_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    DebugGameStartWork *wk = work;

    DebugGameStart_FreeWindows(wk);
    GFL_FontFree(wk->font);
    BmpWin_FreeAllocator();
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysFree();
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_DEBUG_GENDER_SELECT);
    return TRUE;
}

// Answers come as 1 for the first option and 2 for the second
static BOOL DebugGameStart_Main(GameProc *proc, u32 *state, void *param, void *work) {
    GameStartWork *gameStart = param;
    DebugGameStartWork *wk = work;
    u32 answer;

    switch (wk->question) {
    case DEBUG_QUESTION_TEXT:
        answer = DebugGameStart_UpdateMenu(wk);
        if (answer == 0) {
            break;
        }
        if (answer == 1) {
            func_02008a8c(gameStart->config, 0);
        } else {
            func_02008a8c(gameStart->config, 1);
        }
        DebugGameStart_FreeWindows(wk);
        wk->question = DEBUG_QUESTION_GENDER;
        DebugGameStart_CreateWindows(wk);
        break;
    case DEBUG_QUESTION_GENDER:
        answer = DebugGameStart_UpdateMenu(wk);
        if (answer == 0) {
            break;
        }
        if (answer == 1) {
            setTrainerGender(gameStart->playerInfo, GENDER_MALE);
        } else {
            setTrainerGender(gameStart->playerInfo, GENDER_FEMALE);
        }
        DebugGameStart_FreeWindows(wk);
        wk->question = DEBUG_QUESTION_CGEAR;
        DebugGameStart_CreateWindows(wk);
        break;
    case DEBUG_QUESTION_CGEAR:
        answer = DebugGameStart_UpdateMenu(wk);
        if (answer == 0) {
            break;
        }
        if (answer == 1) {
            func_02008af0(gameStart->config, TRUE);
        } else {
            func_02008af0(gameStart->config, FALSE);
        }
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 12, 1,
                    HEAPID_DEBUG_GENDER_SELECT);
        wk->question = DEBUG_QUESTION_END;
        break;
    case DEBUG_QUESTION_END:
        if (GFL_WipeIsFinished() == TRUE) {
            return TRUE;
        }
        break;
    }
    DebugGameStart_CycleCursorColor(wk);
    return FALSE;
}

static void DebugGameStart_InitBG(DebugGameStartWork *wk) {
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_BGSysSetVRAMBanks(&sDebugVRAMConfig);
    GFL_BGSysCreate(HEAPID_DEBUG_GENDER_SELECT);
    GFL_BGSysSetLCDConfig(&sDebugLCDConfig);
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    DebugGameStart_CreateBG(&sDebugBG3Setup, 3);
    DebugGameStart_CreateBG(&sDebugBG1Setup, 1);
    DebugGameStart_CreateBG(&sDebugBG5Setup, 5);
    BmpWin_InitAllocator(HEAPID_DEBUG_GENDER_SELECT);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0, 0x20, HEAPID_DEBUG_GENDER_SELECT);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0, 0x20, HEAPID_DEBUG_GENDER_SELECT);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, HEAPID_DEBUG_GENDER_SELECT);
    LoadSysMsgBox(1, 1, 3, 0, HEAPID_DEBUG_GENDER_SELECT);
    LoadSysMsgBox(1, 31, 4, 0, HEAPID_DEBUG_GENDER_SELECT);
    LoadSysMsgBox(5, 1, 1, 0, HEAPID_DEBUG_GENDER_SELECT);
    *(vu16 *)HW_BG_PLTT = GX_RGB(12, 12, 31);
    *(vu16 *)HW_DB_BG_PLTT = GX_RGB(12, 12, 31);
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
}

static void DebugGameStart_CreateBG(const BGSetup *setup, u8 bg) {
    GFL_BGSysCreateBG(bg, setup, BGMODE_TEXT);
    GFL_BGSysSetBGEnabled(bg, TRUE);
    GFL_BGSysClearBG(bg);
    GFL_BGSysLoadScr(bg);
}

// The question on the sub screen, and the two answers on the main screen. The messages are those of system message
// file 363: the questions from 0, and their answers from 5, two to a question
static void DebugGameStart_CreateWindows(DebugGameStartWork *wk) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_GAME_START_2, HEAPID_DEBUG_GENDER_SELECT);
    StrBuf *strbuf;
    u8 i;

    wk->cursor = 0;
    wk->questionWindow = BmpWin_CreateDynamic(5, 1, 19, 29, 4, 0, 1);
    BmpWin_FlushMap(wk->questionWindow);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->questionWindow), 15);
    strbuf = GFL_MsgDataLoadStrbufNew(msgData, wk->question);
    BmpWin_DrawFrame(wk->questionWindow, WINFRAME_TRANSFER_NOW, 1, 1);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->questionWindow), 2, 2, strbuf, wk->font);
    GFL_StrBufFree(strbuf);
    BmpWin_FlushChar(wk->questionWindow);
    for (i = 0; i < 2; i++) {
        wk->answerWindows[i] = BmpWin_CreateDynamic(1, 6, 12 + i * 4, 20, 2, 0, 1);
        BmpWin_FlushMap(wk->answerWindows[i]);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->answerWindows[i]), 15);
        strbuf = GFL_MsgDataLoadStrbufNew(msgData, 5 + i + wk->question * 2);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->answerWindows[i]), 2, 2, strbuf, wk->font);
        GFL_StrBufFree(strbuf);
        BmpWin_FlushChar(wk->answerWindows[i]);
    }
    GFL_MsgDataFree(msgData);
    DebugGameStart_DrawCursor(wk);
}

static void DebugGameStart_FreeWindows(DebugGameStartWork *wk) {
    u8 i;

    BmpWin_Free(wk->questionWindow);
    for (i = 0; i < 2; i++) {
        BmpWin_Free(wk->answerWindows[i]);
    }
}

// The chosen answer gets a frame in palette 3, whose color cycles, and the other in palette 4
static void DebugGameStart_DrawCursor(DebugGameStartWork *wk) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (i == wk->cursor) {
            BmpWin_DrawFrame(wk->answerWindows[i], WINFRAME_TRANSFER_NOW, 1, 3);
        } else {
            BmpWin_DrawFrame(wk->answerWindows[i], WINFRAME_TRANSFER_NOW, 31, 4);
        }
    }
}

// Returns 1 or 2 for the answer that A or a touch chose, or 0
static u32 DebugGameStart_UpdateMenu(DebugGameStartWork *wk) {
    u32 answer;
    s32 touched;

    if ((GCTX_HIDGetPressedKeys() & PAD_KEY_UP) || (GCTX_HIDGetPressedKeys() & PAD_KEY_DOWN)) {
        wk->cursor = wk->cursor == 0 ? 1 : 0;
        DebugGameStart_DrawCursor(wk);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        answer = 1;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (wk->cursor != 0) {
            answer = 2;
        }
        return answer;
    }
    {
        TouchRectTable table = sDebugTouchRects;

        touched = func_0203da0c(table.rects);
    }
    if (touched != TOUCH_RECT_NONE) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (touched == 0) {
            return 1;
        }
        return 2;
    }
    return 0;
}

// Cycles the chosen answer's frame color, through palette 3 of the main screen
static void DebugGameStart_CycleCursorColor(DebugGameStartWork *wk) {
    fx16 t;
    u8 r;
    u8 g;
    u8 b;

    if (wk->cursorColorAngle + 0x400 >= 0x10000) {
        wk->cursorColorAngle = wk->cursorColorAngle + 0x400 - 0x10000;
    } else {
        wk->cursorColorAngle += 0x400;
    }
    t = (FX_CosIdx(wk->cursorColorAngle) + FX16_ONE) / 2;
    b = 29 + ((-8 * t) >> FX32_SHIFT);
    r = 25 + ((-20 * t) >> FX32_SHIFT);
    g = 30 + ((-15 * t) >> FX32_SHIFT);
    wk->cursorColor = GX_RGB(r, g, b);
    NNS_GfdRegisterNewVramTransferTask(15, 3 * 0x20 + 6 * 2, &wk->cursorColor, sizeof(GXRgb));
}
