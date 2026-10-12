#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "field/ev_time.h"
#include "field/game_clear_save.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/fade.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"
#include "save/hall_of_fame.h"
#include "save/playtime.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/app_printsys_common.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/rtc.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/version.h"
#include "system/wordset.h"
#include "text/system/0041.h"

// The records that the first clear sets
#define RECORD_CLEAR_PLAY_TIME 2
#define RECORD_HALL_OF_FAME_ENTRIES 0x47
// The Hall of Fame is extra save block 8
#define HALL_OF_FAME_BLOCK 8
#define MESSAGE_WAIT_FRAMES 120

enum {
    STATE_SETUP,
    STATE_START_FADE_IN,
    STATE_WAIT_FADE_IN,
    STATE_START_SAVE,
    STATE_REGISTER,
    STATE_FREE_BLOCKS,
    STATE_WAIT_SAVE,
    STATE_RESULT_MESSAGE,
    STATE_WAIT,
    STATE_START_FADE_OUT,
    STATE_WAIT_FADE_OUT,
    STATE_CLEANUP,
    STATE_END,
    STATE_WAIT_MESSAGE,
};

typedef struct {
    u16 heapId;
    u16 msgBank;
    GameData *gameData;
    PlayerInfo *playerInfo;
    StrBuf *strBuf;
    Font *font;
    MsgData *msgData;
    WordSet *wordSet;
    BmpWin *window;
    PrintStream *stream;
    TCBExManager *tcbManager;
    AppPrintsysCommon printsys;
    WaitIcon *waitIcon;
    // Whether the save succeeded
    BOOL saved;
    // Whether the team is in the Hall of Fame
    BOOL registered;
    // Whether the intermediate save blocks are freed
    BOOL blocksFreed;
    // Whether the game can't be saved
    BOOL cannotSave;
    // The state once the message is read
    u32 nextState;
    s32 waitFrames;
} GameClearSave;

static BOOL GameClearSave_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL GameClearSave_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL GameClearSave_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void GameClearSave_Setup(GameClearSave *save, GameClearSaveParam *param);
static void GameClearSave_Free(GameClearSave *save);
static void GameClearSave_InitBGs(HeapID heapId);
static void GameClearSave_FreeBGs(void);
static void GameClearSave_PrintMessage(GameClearSave *save, u32 messageId, s16 x, s16 y);
static void GameClearSave_SetClearPlayTime(GameClearSave *save);
static void GameClearSave_CountClear(GameClearSave *save);
static void GameClearSave_SetFirstClearTime(GameClearSave *save);
static void GameClearSave_RegisterTeam(GameClearSave *save);
static void GameClearSave_FreeBlocks(GameClearSave *save);

const GameProcFunctions GAME_CLEAR_SAVE_PROC_FUNCTIONS = {
    GameClearSave_Init,
    GameClearSave_Main,
    GameClearSave_Exit,
};

static BOOL GameClearSave_Init(GameProc *proc, u32 *state, void *param, void *work) {
    GameClearSaveParam *saveParam = param;
    GameSystem *gsys = saveParam->gsys;
    GameClearSave *save;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_GAME_CLEAR_SAVE, 0x40000);
    save = GFL_ProcInitSubsystem(proc, sizeof(GameClearSave), HEAPID_GAME_CLEAR_SAVE);
    sys_memset32(0, save, sizeof(GameClearSave));
    save->heapId = HEAPID_GAME_CLEAR_SAVE;
    save->msgBank = TEXT_BANK_0041;
    save->gameData = GSYS_GetGameData(gsys);
    save->playerInfo = GetGameDataPlayerInfo(save->gameData);
    if (func_0200746c(GameData_GetSaveControl(save->gameData))) {
        save->cannotSave = TRUE;
    }
    return TRUE;
}

static BOOL GameClearSave_Main(GameProc *proc, u32 *state, void *param, void *work) {
    GameClearSave *save = work;
    GameClearSaveParam *saveParam = param;
    BmpWin *window;
    u32 keys = GCTX_HIDGetPressedKeys();

    switch (*state) {
    case STATE_SETUP:
        GameClearSave_Setup(save, saveParam);
        (*state)++;
        break;
    case STATE_START_FADE_IN:
        reg_G2_BLDCNT = 0;
        GX_SetVisiblePlane(GX_PLANEMASK_BG0);
        if (getGameVersion() == VERSION_BLACK2) {
            u16 black[1] = { 0 };

            gfxUploadStdPaletteBGB(black, 0, 2);
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        } else {
            u16 white[1] = { 0xffff };

            gfxUploadStdPaletteBGB(white, 0, 2);
            GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 16, 0, 0);
        }
        (*state)++;
        break;
    case STATE_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            *state = STATE_START_SAVE;
        }
        break;
    case STATE_START_SAVE:
        if (save->cannotSave) {
            (*state)++;
            break;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        GameClearSave_PrintMessage(save, Bank0041_Text_SavingDontTurnOff, 0, 0);
        window = save->window;
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
        save->nextState = STATE_REGISTER;
        *state = STATE_WAIT_MESSAGE;
        func_0201782c(save->gameData);
        break;
    case STATE_REGISTER:
        if (!save->cannotSave) {
            if (saveParam->repeat == 0) {
                GameClearSave_SetFirstClearTime(save);
                GameClearSave_SetClearPlayTime(save);
            }
            GameClearSave_CountClear(save);
            GameClearSave_RegisterTeam(save);
        }
        save->waitIcon = WaitIcon_CreateTCBEx(save->tcbManager, save->window, 15, 16, save->heapId);
        (*state)++;
        break;
    case STATE_FREE_BLOCKS:
        if (!save->cannotSave && save->registered) {
            GameClearSave_FreeBlocks(save);
            if (!save->blocksFreed) {
                break;
            }
            (*state)++;
            break;
        }
        (*state)++;
        break;
    case STATE_WAIT_SAVE:
        if (!save->cannotSave) {
            switch (func_02017850(save->gameData)) {
            case 0:
            case 1:
                break;
            case 2:
                save->saved = TRUE;
                (*state)++;
                break;
            case 3:
                save->saved = FALSE;
                (*state)++;
                break;
            }
        } else {
            (*state)++;
        }
        break;
    case STATE_RESULT_MESSAGE:
        WaitIcon_Free(save->waitIcon);
        if (!save->cannotSave && save->saved) {
            GameClearSave_PrintMessage(save, Bank0041_Text_SavedGame, 0, 0);
            window = save->window;
            BmpWin_FlushChar(window);
            BmpWin_FlushMap(window);
            GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
            save->nextState = STATE_WAIT;
            *state = STATE_WAIT_MESSAGE;
            GFL_SndSEPlay(SEQ_SE_SAVE);
        } else {
            GameClearSave_PrintMessage(save, Bank0041_Text_ThereAlreadySavedGame, 0, 0);
            window = save->window;
            BmpWin_FlushChar(window);
            BmpWin_FlushMap(window);
            GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
            save->nextState = STATE_WAIT;
            *state = STATE_WAIT_MESSAGE;
        }
        break;
    case STATE_WAIT:
        if (save->waitFrames > MESSAGE_WAIT_FRAMES) {
            (*state)++;
        }
        save->waitFrames++;
        break;
    case STATE_START_FADE_OUT:
        if (getGameVersion() == VERSION_BLACK2) {
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        } else {
            GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 0, 16, 0);
        }
        (*state)++;
        break;
    case STATE_WAIT_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            GFL_BitmapFill(BmpWin_GetBitmap(save->window), 15);
            (*state)++;
        }
        break;
    case STATE_CLEANUP:
        GameClearSave_Free(save);
        (*state)++;
        break;
    case STATE_END:
        return TRUE;
    case STATE_WAIT_MESSAGE:
        if (AppPrintsysCommon_Update(&save->printsys, save->stream)) {
            *state = save->nextState;
        }
        break;
    }
    if (save->tcbManager != NULL) {
        GFL_TCBExMgrUpdate(save->tcbManager);
    }
    if (save->waitIcon != NULL) {
        WaitIcon_Main(save->waitIcon);
    }
    return FALSE;
}

static BOOL GameClearSave_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_GAME_CLEAR_SAVE);
    return TRUE;
}

static void GameClearSave_Setup(GameClearSave *save, GameClearSaveParam *param) {
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_MAIN_SUB);
    gfxSetLCDCBanks(GX_VRAM_LCDC_ALL);
    sys_memset32_fast(0, (void *)HW_LCDC_VRAM, HW_LCDC_VRAM_SIZE);
    gfxDisableLCDCBanks();
    GameClearSave_InitBGs(save->heapId);
    save->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, save->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1a0, 0x20, save->heapId);
    save->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, save->msgBank, save->heapId);
    save->wordSet = GFL_WordSetSystemCreateDefault(save->heapId);
    LoadSysMsgBox(0, 1, 12, 0, save->heapId);
    save->window = BmpWin_CreateDynamic(0, 2, 18, 27, 4, 13, 1);
    save->tcbManager = GFL_TCBExMgrCreate(save->heapId, save->heapId, 10, 32);
    save->strBuf = GFL_StrBufCreate(0x400, save->heapId);
}

static void GameClearSave_Free(GameClearSave *save) {
    GFL_TCBExMgrFreeTasks(save->tcbManager);
    GFL_TCBExMgrFree(save->tcbManager);
    save->tcbManager = NULL;
    BmpWin_ClearFrame(save->window, WINFRAME_TRANSFER_NOW);
    BmpWin_Free(save->window);
    GFL_StrBufFree(save->strBuf);
    GFL_WordSetSystemFree(save->wordSet);
    GFL_MsgDataFree(save->msgData);
    GFL_BGSysReleaseBG(0);
    GFL_FontFree(save->font);
    GameClearSave_FreeBGs();
}

static void GameClearSave_InitBGs(HeapID heapId) {
    static const BGSysVRAMConfig sVRAMConfig = {
        GX_VRAM_BG_128_B, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
        GX_VRAM_OBJ_64_E, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_0_A,  GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
    };
    static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
    static const BGSetup sBGSetup = {
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
        1,
        GX_BG_AREAOVER_XLU,
        FALSE,
    };

    GFL_BGSysCreate(heapId);
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    GFL_BGSysCreateBG(0, &sBGSetup, BGMODE_TEXT);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 7, 0, 0x1a0, 0x20, heapId);
    if (getGameVersion() == VERSION_WHITE2) {
        GFL_BGSysResetStdPalette(0, 0xffff);
    } else {
        GFL_BGSysResetStdPalette(0, 0);
    }
    BmpWin_InitAllocator(heapId);
}

static void GameClearSave_FreeBGs(void) {
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void GameClearSave_PrintMessage(GameClearSave *save, u32 messageId, s16 x, s16 y) {
    StrBuf *message = GFL_StrBufCreate(0x400, save->heapId);

    GFL_BitmapFill(BmpWin_GetBitmap(save->window), 15);
    GFL_MsgDataLoadStrbuf(save->msgData, messageId, message);
    copyVarForText(save->wordSet, 0, save->playerInfo);
    GFL_WordSetFormatStrbuf(save->wordSet, save->strBuf, message);
    save->stream = func_02022268(save->window, x, y, save->strBuf, save->font, func_02017bcc(), save->tcbManager, 0,
                                 save->heapId, 0xf0f);
    GFL_StrBufFree(message);
    BmpWin_DrawFrame(save->window, WINFRAME_TRANSFER_NONE, 1, 12);
}

// Sets the record of the play time of the first clear to hours, minutes and seconds in decimal
static void GameClearSave_SetClearPlayTime(GameClearSave *save) {
    GameRecords *records = GameData_GetRecords(save->gameData);
    PlayTime *playTime = func_02017a40(save->gameData);
    u16 hours = func_02008cec(playTime);
    u8 minutes = func_02008cf0(playTime);
    u8 seconds = func_02008cf4(playTime);

    func_020094cc(records, RECORD_CLEAR_PLAY_TIME, hours * 10000 + minutes * 100 + seconds);
}

static void GameClearSave_CountClear(GameClearSave *save) {
    SaveControl *saveControl = GameData_GetSaveControl(save->gameData);
    GameRecords *records = GameData_GetRecords(save->gameData);

    if (!func_0200746c(saveControl)) {
        RecordAdd(records, RECORD_HALL_OF_FAME_ENTRIES, 1);
    }
}

static void GameClearSave_SetFirstClearTime(GameClearSave *save) {
    GameRecords *records = GameData_GetRecords(save->gameData);

    if (RecordGet(records, RECORD_HALL_OF_FAME_ENTRIES) == 0) {
        setCurrentSeconds(save->gameData);
    }
}

static void GameClearSave_RegisterTeam(GameClearSave *save) {
    RTCDate date;
    PokeParty *party;
    SaveControl *saveControl;
    HallOfFameSave *hallOfFame;

    func_0207cc10(&date);
    party = GameData_GetParty(save->gameData);
    saveControl = GameData_GetSaveControl(save->gameData);
    func_020074ec(saveControl, HALL_OF_FAME_BLOCK, HEAPID_GAMEEVENT);
    hallOfFame = getAddressOfExtraSaveBlk(saveControl, HALL_OF_FAME_BLOCK, 0);
    if (!func_0200746c(saveControl)) {
        HallOfFameSave_AddTeam(hallOfFame, party, &date, HEAPID_GAMEEVENT);
        func_020178c4(save->gameData, HALL_OF_FAME_BLOCK);
        save->registered = TRUE;
    }
}

static void GameClearSave_FreeBlocks(GameClearSave *save) {
    SaveControl *saveControl = GameData_GetSaveControl(save->gameData);

    if (!func_0200746c(saveControl)) {
        u32 state = func_020178f4(save->gameData, HALL_OF_FAME_BLOCK);

        if (state == 2 || state == 3) {
            freeIntermediateSaveExtraBlksAfterLoad(saveControl, HALL_OF_FAME_BLOCK);
            save->blocksFreed = TRUE;
        }
    }
}
