#include "types.h"
#include "app/wifi_bsubway.h"
#include "app/codein.h"
#include "app/wifi_bsubway_graphic.h"
#include "app/wifi_login.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "dpw/dpw_bt.h"
#include "dwc/dwc.h"
#include "field/bsubway_scr.h"
#include "field/unity_tower.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/dpw_profile.h"
#include "gfl/dwc_rap.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/nhttp_rap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "nnsys/g2d.h"
#include "pml/species_names.h"
#include "save/bsubway_save.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "system/bmp_menu.h"
#include "system/bmp_winframe.h"
#include "system/codein_param.h"
#include "system/country_region.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/pms.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/union_view.h"
#include "system/version.h"
#include "system/wordset.h"
#include "text/system/0413.h"
#include "text/system/ability_handlers_btl_main.h"

// The Wi-Fi Battle Subway's screen: its proc, the three things it does with the Battle Subway's server (sending the
// player's record, downloading a train's trainers and downloading a train's leaders), the requests behind them and the
// message window. The ROM names only wifi_bsubway_graphic.c, so the name of this file is a guess after it and after
// the callers' event_wifi_bsubway.c. All the names are ours

// Frames a request may take before it is given up, a minute
#define REQUEST_TIMEOUT (60 * 60)

// The ranks a Wi-Fi train has
#define RANK_MAX 10

// The name the player gets when the server finds a bad word in theirs
#ifdef BLACK2
#define DEFAULT_NAME_MSG AbilityHandlersBtlMain_Text_Kuro
#else
#define DEFAULT_NAME_MSG AbilityHandlersBtlMain_Text_Shiro
#endif

#define TEAM_POKEMON_COUNT 3
#define PLAYER_MESSAGE_COUNT 3
#define ROOM_PLAYER_COUNT 7
#define ROOM_LEADER_COUNT 30

#define PALETTE_COUNT 15
#define BLEND_STEPS 4

// The message window's BG, the yes/no menu's, and the frame's palette
#define MESSAGE_BG 1
#define YES_NO_BG 2
#define FRAME_PALETTE 13

// The proc's steps
enum {
    STATE_BGM_FADE_OUT,
    STATE_BGM_START,
    STATE_LOGIN,
    STATE_LOGIN_WAIT,
    STATE_VIEW_INIT,
    STATE_FADE_IN_WAIT,
    STATE_CONNECT,
    STATE_CONNECT_WAIT,
    STATE_CONNECT_PRINT_WAIT,
    STATE_CHECK_NAME,
    STATE_CHECK_NAME_WAIT,
    STATE_SET_PROFILE,
    STATE_SET_PROFILE_WAIT,
    STATE_MODE_MAIN,
    STATE_DISCONNECT,
    STATE_DISCONNECT_WAIT,
    STATE_LOGOUT,
    STATE_BGM_END,
    STATE_BGM_END_WAIT,
    STATE_END,
    STATE_ERROR,
    STATE_ERROR_WAIT,
};

// What a mode's function returns
enum {
    MODE_RESULT_CONTINUE,
    MODE_RESULT_END,
    MODE_RESULT_CANCEL,
};

// What the check of the team's Pokémon returns
enum {
    CHECK_RESULT_WAIT,
    CHECK_RESULT_OK,
    CHECK_RESULT_NG,
};

// What went wrong with the check of the team's Pokémon
enum {
    CHECK_ERROR_NONE,
    // A Pokémon failed the check a second time
    CHECK_ERROR_POKEMON,
    // The server refused the check
    CHECK_ERROR_STATUS,
    // The request failed, with an HTTP status
    CHECK_ERROR_HTTP,
};

// The server library's types, which are the DS Wi-Fi library's Dpw_Bt_Player, Dpw_Bt_Leader and Dpw_Bt_Room. The
// library's functions take them as untyped pointers. The Pokémon are BSubwayPokemon and the messages PMSData, kept as
// bytes: the types are aligned to 2 bytes
typedef struct {
    u8 pokemon[TEAM_POKEMON_COUNT][sizeof(BSubwayPokemon)];
    u16 name[8];
    u8 versionCode;
    u8 langCode;
    u8 countryCode;
    u8 localCode;
    u8 playerId[4];
    u8 leaderMessage[sizeof(PMSData)];
    u8 ngName : 1;
    u8 gender : 1;
    u8 : 6;
    u8 trainerType;
    u8 messages[PLAYER_MESSAGE_COUNT][sizeof(PMSData)];
    u16 result;
} Dpw_Bt_Player;

typedef struct {
    u16 name[8];
    u8 versionCode;
    u8 langCode;
    u8 countryCode;
    u8 localCode;
    u8 playerId[4];
    u8 leaderMessage[8];
    u8 flags;
    u8 padding;
} Dpw_Bt_Leader;

typedef struct {
    Dpw_Bt_Player players[ROOM_PLAYER_COUNT];
    Dpw_Bt_Leader leaders[ROOM_LEADER_COUNT];
} Dpw_Bt_Room;

// Palette 1's colors 4 to 10 cycle through blends of the file's palettes, a step every other frame, as on the Wi-Fi
// login's screen (wifilogin_disp.c)
typedef struct {
    TCB *task;
    BOOL active;
    u16 palettes[PALETTE_COUNT][16];
    u16 blended[PALETTE_COUNT * BLEND_STEPS][16];
    s16 frame;
    u8 unk96A;
    u8 skip;
    // The BG that scrolls with the cycle
    u8 scrollBg;
    u8 unk96D[3];
} WifiBSubwayPaletteCycle;

// The screen's display: the BGs and the message window
typedef struct {
    BOOL initialized;
    HeapID heapId;
    WifiBSubwayGraphic *graphic;
    WifiBSubwayPaletteCycle cycle;
    BmpWin *window;
    BmpMenu *yesNo;
    // The frames' characters, of the message window's BG and of the yes/no menu's
    u32 messageFrameChars;
    u32 yesNoFrameChars;
    WaitIcon *waitIcon;
    MsgData *msgData;
    WordSet *wordSet;
    Font *font;
    StrBuf *message;
    StrBuf *expanded;
    PrintStream *stream;
    TCBExManager *tcbManager;
} WifiBSubwayView;

// A train's download: the number of trains a rank has, then the trainers and leaders of one
typedef struct {
    BOOL hasCount;
    BOOL downloaded;
    u16 rank;
    u16 number;
    u16 count;
    Dpw_Bt_Room data;
} WifiBSubwayRoom;

// The check of the player's name for bad words
typedef struct {
    u16 name[12];
    const u16 *words;
    char result;
    int badWordCount;
    HeapID heapId;
} WifiBSubwayNameCheck;

// The player's profile and record, as the server takes them
typedef struct {
    BOOL profileSet;
    Dpw_Common_Profile profile;
    Dpw_Common_ProfileResult profileResult;
    Dpw_Bt_Player player;
    BSubwayWifiData *wifiData;
    BSubwayScoreData *score;
    // The check of the team's Pokémon, whose signature goes with the record
    NHttpRap *nhttp;
    u16 pokemonCount;
    u8 sign[0x80];
    WifiBSubwayNameCheck nameCheck;
    // A copy of the player's, whose name the check may replace
    PlayerInfo *playerInfo;
} WifiBSubwayUpload;

// What went wrong with a request
typedef struct {
    // The server library's error, when negative
    s32 dpwResult;
    // The server's state, which nothing sets
    u32 serverState;
    // Whether the request ran out of time
    u16 timeout;
    s16 timer;
    // A CHECK_ERROR_*, and the HTTP status of CHECK_ERROR_HTTP
    u32 checkError;
    s32 httpStatus;
} WifiBSubwayError;

typedef struct {
    WifiBSubwayParam *param;
    WifiBSubwayView view;
    WifiLoginParam login;
    WifiLogoutParam logout;
    u8 loginBuffer[0x174];
    // Whether the server library runs
    u16 dpwActive;
    u16 saving;
    // The mode's step
    u32 seq;
    // The number the player typed
    u32 inputNumber;
    WifiBSubwayRoom room;
    WifiBSubwayUpload upload;
    WifiBSubwayError error;
    CodeInParam *codeIn;
    GameData *gameData;
    AdventureSave *adventure;
    WifiList *wifiList;
    PlayerInfo *playerInfo;
    BSubwayWifiData *wifiData;
    BSubwayScoreData *score;
} WifiBSubwayWork;

typedef u32 (*WifiBSubwayModeFunc)(WifiBSubwayWork *wk, HeapID heapId);

static BOOL WifiBSubway_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBSubway_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBSubway_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBSubway_CanCheckError(WifiBSubwayWork *wk, int state);
static void WifiBSubwayRoom_Init(WifiBSubwayRoom *room);
static void WifiBSubwayRoom_RequestCount(WifiBSubwayRoom *room, WifiBSubwayError *error, u16 rank);
static BOOL WifiBSubwayRoom_WaitCount(WifiBSubwayRoom *room, WifiBSubwayError *error);
static void WifiBSubwayRoom_Download(WifiBSubwayRoom *room, WifiBSubwayError *error, u16 number);
static BOOL WifiBSubwayRoom_WaitDownload(WifiBSubwayRoom *room, WifiBSubwayError *error);
static u32 WifiBSubwayRoom_GetRank(const WifiBSubwayRoom *room);
static u32 WifiBSubwayRoom_GetNumber(const WifiBSubwayRoom *room);
static u32 WifiBSubwayRoom_GetCount(const WifiBSubwayRoom *room);
static u32 WifiBSubwayRoom_GetCountDigits(const WifiBSubwayRoom *room);
static void WifiBSubwayRoom_FixData(WifiBSubwayRoom *room);
static void WifiBSubwayRoom_SavePlayers(WifiBSubwayRoom *room, BSubwayWifiData *wifiData);
static void WifiBSubwayRoom_SaveLeaders(WifiBSubwayRoom *room, BSubwayWifiData *wifiData);
static void WifiBSubwayUpload_Init(WifiBSubwayUpload *upload, HeapID heapId);
static void WifiBSubwayUpload_Free(WifiBSubwayUpload *upload);
static void WifiBSubwayUpload_CheckName(WifiBSubwayUpload *upload, GameData *gameData, HeapID heapId);
static u32 WifiBSubwayUpload_WaitCheckName(WifiBSubwayUpload *upload, HeapID heapId);
static void WifiBSubwayUpload_SetProfile(WifiBSubwayUpload *upload, GameData *gameData, WifiBSubwayError *error);
static BOOL WifiBSubwayUpload_WaitProfile(WifiBSubwayUpload *upload, WifiBSubwayError *error);
static void WifiBSubwayUpload_FillPlayer(Dpw_Bt_Player *player, GameData *gameData, PlayerInfo *playerInfo);
static void WifiBSubwayUpload_OnError(void *work, int code, int type, int error);
static void WifiBSubwayUpload_CheckPokemon(WifiBSubwayUpload *upload, WifiBSubwayError *error, void *loginBuffer,
                                           HeapID heapId);
static u32 WifiBSubwayUpload_WaitCheckPokemon(WifiBSubwayUpload *upload, WifiBSubwayError *error, BOOL last);
static void WifiBSubwayUpload_ResetNickname(WifiBSubwayUpload *upload, int index);
static void WifiBSubwayUpload_CancelCheck(WifiBSubwayUpload *upload);
static void WifiBSubwayUpload_Send(WifiBSubwayUpload *upload, WifiBSubwayError *error);
static BOOL WifiBSubwayUpload_WaitSend(WifiBSubwayUpload *upload, WifiBSubwayError *error);
static void WifiBSubwayError_Init(WifiBSubwayError *error);
static void WifiBSubwayError_StartDpw(WifiBSubwayError *error);
static BOOL WifiBSubwayError_WaitDpw(WifiBSubwayError *error, s32 *result);
static BOOL WifiBSubwayError_Check(const WifiBSubwayError *error);
static u32 WifiBSubwayError_GetMessage(const WifiBSubwayError *error);
static BOOL WifiBSubwayError_ShowHttpError(const WifiBSubwayError *error);
static void WifiBSubwayError_SetCheckError(WifiBSubwayError *error, u32 checkError);
static void WifiBSubwayError_SetHttpError(WifiBSubwayError *error, s32 status);
static void WifiBSubwayError_StartHttp(WifiBSubwayError *error);
static void WifiBSubwayError_CountHttp(WifiBSubwayError *error);
static void WifiBSubway_CallRoomInput(WifiBSubwayWork *wk, HeapID heapId);
static void WifiBSubway_CallRankInput(WifiBSubwayWork *wk, HeapID heapId);
static void WifiBSubway_FreeCodeIn(WifiBSubwayWork *wk);
static u32 WifiBSubway_GetCodeInNumber(WifiBSubwayWork *wk);
static void WifiBSubway_CallLogin(WifiBSubwayWork *wk);
static void WifiBSubway_FreeLogin(WifiBSubwayWork *wk);
static u32 WifiBSubway_GetLoginResult(const WifiBSubwayWork *wk);
static void WifiBSubway_CallLogout(WifiBSubwayWork *wk);
static void WifiBSubway_FreeLogout(WifiBSubwayWork *wk);
static u32 WifiBSubway_RunMode(WifiBSubwayWork *wk, HeapID heapId);
static u32 WifiBSubway_UploadMain(WifiBSubwayWork *wk, HeapID heapId);
static u32 WifiBSubway_DownloadTrainMain(WifiBSubwayWork *wk, HeapID heapId);
static u32 WifiBSubway_DownloadLeadersMain(WifiBSubwayWork *wk, HeapID heapId);
static void WifiBSubway_Connect(WifiBSubwayWork *wk);
static BOOL WifiBSubway_WaitConnect(WifiBSubwayWork *wk);
static void WifiBSubway_Disconnect(WifiBSubwayWork *wk);
static BOOL WifiBSubway_WaitDisconnect(WifiBSubwayWork *wk);
static BOOL WifiBSubway_PrintError(WifiBSubwayWork *wk);
static BOOL WifiBSubway_WaitError(WifiBSubwayWork *wk);
static void WifiBSubwayView_Init(WifiBSubwayView *view, HeapID heapId);
static void WifiBSubwayView_Free(WifiBSubwayView *view);
static void WifiBSubwayView_Update(WifiBSubwayView *view);
static void WifiBSubwayView_LoadGraphics(WifiBSubwayView *view, HeapID heapId);
static void WifiBSubwayView_CyclePalette(TCB *task, void *work);
static void WifiBSubwayView_FreeGraphics(WifiBSubwayView *view);
static void WifiBSubwayView_Print(WifiBSubwayView *view, u32 message);
static BOOL WifiBSubwayView_WaitPrint(WifiBSubwayView *view);
static void WifiBSubwayView_SetNumber(WifiBSubwayView *view, u32 index, u32 number, u32 digits);
static void WifiBSubwayView_StartYesNo(WifiBSubwayView *view);
static u32 WifiBSubwayView_UpdateYesNo(WifiBSubwayView *view);
static void WifiBSubwayView_StartWaitIcon(WifiBSubwayView *view);
static void WifiBSubwayView_StopWaitIcon(WifiBSubwayView *view);
static void WifiBSubway_StartSave(WifiBSubwayWork *wk);
static BOOL WifiBSubway_WaitSave(WifiBSubwayWork *wk);

const GameProcFunctions WIFI_BSUBWAY_PROC_FUNCTIONS = {
    WifiBSubway_ProcInit,
    WifiBSubway_ProcMain,
    WifiBSubway_ProcExit,
};

static ConfirmDialogSetup sWifiBSubwayYesNoSetup = { YES_NO_BG, 24, 13, 12, 0 };
// The groups of digits of a train's number, whose length depends on how many trains there are, and of a rank
static int sWifiBSubwayRoomBlocks[CODEIN_BLOCK_COUNT] = { 3, 0, 0 };
static int sWifiBSubwayRankBlocks[CODEIN_BLOCK_COUNT] = { 2, 0, 0 };

static BOOL WifiBSubway_ProcInit(GameProc *proc, u32 *state, void *paramPtr, void *work) {
    WifiBSubwayParam *param = paramPtr;
    WifiBSubwayWork *wk;
    GameData *gameData;
    SaveControl *save;

    GFL_OvlLoad(OVERLAY_ID(189));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFI_BSUBWAY, 0x18000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(WifiBSubwayWork), HEAPID_WIFI_BSUBWAY);
    sys_memset(wk, 0, sizeof(WifiBSubwayWork));
    wk->param = param;
    param->result = WIFI_BSUBWAY_RESULT_ERROR;
    gameData = GSYS_GetGameData(wk->param->gsys);
    save = GameData_GetSaveControl(gameData);
    wk->gameData = gameData;
    wk->adventure = getSaveAdventureDataBlk(save);
    wk->wifiList = GameData_GetWifiList(gameData);
    wk->playerInfo = GetGameDataPlayerInfo(gameData);
    wk->wifiData = func_02017968(gameData);
    wk->score = func_0201795c(gameData);
    WifiBSubwayRoom_Init(&wk->room);
    WifiBSubwayUpload_Init(&wk->upload, HEAPID_WIFI_BSUBWAY);
    WifiBSubwayError_Init(&wk->error);
    return TRUE;
}

static BOOL WifiBSubway_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBSubwayWork *wk = work;

    if (WifiBSubway_CanCheckError(wk, *state)) {
        if (WifiBSubwayError_Check(&wk->error) || GFL_NetErrCheck()) {
            *state = STATE_ERROR;
        }
    }
    switch (*state) {
    case STATE_BGM_FADE_OUT:
        GFL_SndBGMFadeOut(6);
        (*state)++;
        break;
    case STATE_BGM_START:
        if (!GFL_SndBGMIsFading()) {
            GFL_SndBGMSetPaused(TRUE);
            GFL_SndBGMPush();
            GFL_SndBGMPlay(SEQ_BGM_WIFI_ACCESS, SND_CHANNEL_MASK_ALL);
            GFL_SndBGMFadeIn(6);
            (*state)++;
        }
        break;
    case STATE_LOGIN:
        WifiBSubway_CallLogin(wk);
        (*state)++;
        break;
    case STATE_LOGIN_WAIT:
        if (WifiBSubway_GetLoginResult(wk) == 0) {
            (*state)++;
        } else {
            wk->param->result = WIFI_BSUBWAY_RESULT_CANCEL;
            *state = STATE_BGM_END;
        }
        break;
    case STATE_VIEW_INIT:
        WifiBSubwayView_Init(&wk->view, HEAPID_WIFI_BSUBWAY);
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 1);
        (*state)++;
        break;
    case STATE_FADE_IN_WAIT:
        if (!GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case STATE_CONNECT:
        WifiBSubway_Connect(wk);
        WifiBSubwayView_Print(&wk->view, Bank0413_Text_CommunicatingPleaseStandBy);
        WifiBSubwayView_StartWaitIcon(&wk->view);
        (*state)++;
        break;
    case STATE_CONNECT_WAIT:
        if (WifiBSubway_WaitConnect(wk)) {
            (*state)++;
        }
        break;
    case STATE_CONNECT_PRINT_WAIT:
        if (WifiBSubwayView_WaitPrint(&wk->view)) {
            (*state)++;
        }
        break;
    case STATE_CHECK_NAME:
        WifiBSubwayUpload_CheckName(&wk->upload, wk->gameData, HEAPID_WIFI_BSUBWAY);
        (*state)++;
        break;
    case STATE_CHECK_NAME_WAIT:
        if (WifiBSubwayUpload_WaitCheckName(&wk->upload, HEAPID_WIFI_BSUBWAY)) {
            (*state)++;
        }
        break;
    case STATE_SET_PROFILE:
        WifiBSubwayUpload_SetProfile(&wk->upload, wk->gameData, &wk->error);
        (*state)++;
        break;
    case STATE_SET_PROFILE_WAIT:
        if (WifiBSubwayUpload_WaitProfile(&wk->upload, &wk->error)) {
            (*state)++;
        }
        break;
    case STATE_MODE_MAIN: {
        u32 result = WifiBSubway_RunMode(wk, HEAPID_WIFI_BSUBWAY);

        if (result == MODE_RESULT_END) {
            wk->param->result = WIFI_BSUBWAY_RESULT_OK;
            (*state)++;
        } else if (result == MODE_RESULT_CANCEL) {
            wk->param->result = WIFI_BSUBWAY_RESULT_CANCEL;
            (*state)++;
        }
        break;
    }
    case STATE_DISCONNECT:
        WifiBSubway_Disconnect(wk);
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 1);
        (*state)++;
        break;
    case STATE_DISCONNECT_WAIT:
        if (WifiBSubway_WaitDisconnect(wk) && !GFL_FadeIsRunning()) {
            WifiBSubwayView_Free(&wk->view);
            (*state)++;
        }
        break;
    case STATE_LOGOUT:
        if (!GFL_NetErrShow(FALSE)) {
            WifiBSubway_CallLogout(wk);
        }
        (*state)++;
        break;
    case STATE_BGM_END:
        GFL_SndBGMFadeOut(6);
        (*state)++;
        break;
    case STATE_BGM_END_WAIT:
        if (!GFL_SndBGMIsFading()) {
            (*state)++;
        }
        break;
    case STATE_END:
        return TRUE;
    case STATE_ERROR:
        if (WifiBSubway_PrintError(wk)) {
            *state = STATE_ERROR_WAIT;
        } else {
            *state = STATE_DISCONNECT;
        }
        WifiBSubwayUpload_CancelCheck(&wk->upload);
        break;
    case STATE_ERROR_WAIT:
        if (WifiBSubway_WaitError(wk)) {
            *state = STATE_DISCONNECT;
        }
        break;
    }
    if (wk->dpwActive) {
        DpwBt_Process();
    }
    WifiBSubwayView_Update(&wk->view);
    return FALSE;
}

static BOOL WifiBSubway_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBSubwayWork *wk = work;

    WifiBSubway_FreeCodeIn(wk);
    WifiBSubway_FreeLogin(wk);
    WifiBSubway_FreeLogout(wk);
    WifiBSubwayUpload_Free(&wk->upload);
    WifiBSubwayView_Free(&wk->view);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFI_BSUBWAY);
    GFL_OvlUnload(OVERLAY_ID(189));
    GFL_SndBGMPop();
    GFL_SndBGMSetPaused(FALSE);
    GFL_SndBGMFadeIn(60);
    return TRUE;
}

// Whether an error is acted on: up to the mode's steps, while the server library runs and nothing is being saved
static BOOL WifiBSubway_CanCheckError(WifiBSubwayWork *wk, int state) {
    if (state <= STATE_MODE_MAIN && !GFL_FadeIsRunning() && wk->dpwActive && !wk->saving) {
        return TRUE;
    }
    return FALSE;
}

static void WifiBSubwayRoom_Init(WifiBSubwayRoom *room) {
    sys_memset(room, 0, sizeof(WifiBSubwayRoom));
}

// Asks the server how many trains a rank has
static void WifiBSubwayRoom_RequestCount(WifiBSubwayRoom *room, WifiBSubwayError *error, u16 rank) {
    room->rank = rank;
    DpwBt_GetRoomNumber((u8)rank);
    WifiBSubwayError_StartDpw(error);
}

static BOOL WifiBSubwayRoom_WaitCount(WifiBSubwayRoom *room, WifiBSubwayError *error) {
    s32 result;

    if (WifiBSubwayError_WaitDpw(error, &result) && !WifiBSubwayError_Check(error)) {
        room->count = result;
        room->hasCount = TRUE;
        return TRUE;
    }
    return FALSE;
}

// Asks the server for a train of the rank
static void WifiBSubwayRoom_Download(WifiBSubwayRoom *room, WifiBSubwayError *error, u16 number) {
    room->number = number;
    DpwBt_Download((u8)room->rank, (u8)room->number, &room->data);
    WifiBSubwayError_StartDpw(error);
}

static BOOL WifiBSubwayRoom_WaitDownload(WifiBSubwayRoom *room, WifiBSubwayError *error) {
    s32 result;

    if (room->downloaded) {
        return TRUE;
    }
    if (WifiBSubwayError_WaitDpw(error, &result) && !WifiBSubwayError_Check(error)) {
        room->downloaded = TRUE;
        return TRUE;
    }
    return FALSE;
}

static u32 WifiBSubwayRoom_GetRank(const WifiBSubwayRoom *room) {
    return room->rank;
}

static u32 WifiBSubwayRoom_GetNumber(const WifiBSubwayRoom *room) {
    return room->number;
}

static u32 WifiBSubwayRoom_GetCount(const WifiBSubwayRoom *room) {
    return room->count;
}

// How many digits a train's number has
static u32 WifiBSubwayRoom_GetCountDigits(const WifiBSubwayRoom *room) {
    if (room->count < 100) {
        return 2;
    }
    return 3;
}

// Replaces the countries, regions and trainer types that this game doesn't know
static void WifiBSubwayRoom_FixData(WifiBSubwayRoom *room) {
    Dpw_Bt_Player *players = room->data.players;
    Dpw_Bt_Leader *leaders = room->data.leaders;
    int i;

    for (i = 0; i < ROOM_PLAYER_COUNT; i++) {
        players[i].countryCode =
            Country_GetValidCountry(players[i].countryCode, players[i].localCode, players[i].langCode);
        players[i].localCode = Country_GetValidRegion(players[i].countryCode, players[i].localCode, players[i].langCode);
        players[i].trainerType = func_0202b5e8(func_0202b630(players[i].trainerType));
    }
    for (i = 0; i < ROOM_LEADER_COUNT; i++) {
        leaders[i].countryCode =
            Country_GetValidCountry(leaders[i].countryCode, leaders[i].localCode, leaders[i].langCode);
        leaders[i].localCode = Country_GetValidRegion(leaders[i].countryCode, leaders[i].localCode, leaders[i].langCode);
    }
}

// Saves the train's trainers, and that the train was taken on the server's date
static void WifiBSubwayRoom_SavePlayers(WifiBSubwayRoom *room, BSubwayWifiData *wifiData) {
    RTCTime time;
    RTCDate date;

    func_ov011_0215dda8(&date, &time);
    BSubwayWifiData_SetPlayers(wifiData, room->data.players, room->rank, room->number);
    BSubwayWifiData_SetRoomFlag(wifiData, room->rank, room->number, &date);
}

static void WifiBSubwayRoom_SaveLeaders(WifiBSubwayRoom *room, BSubwayWifiData *wifiData) {
    BSubwayWifiData_SetLeaders(wifiData, room->data.leaders, room->rank, room->number);
}

static void WifiBSubwayUpload_Init(WifiBSubwayUpload *upload, HeapID heapId) {
    sys_memset(upload, 0, sizeof(WifiBSubwayUpload));
    upload->playerInfo = func_02008b0c(heapId);
}

static void WifiBSubwayUpload_Free(WifiBSubwayUpload *upload) {
    if (upload->playerInfo != NULL) {
        GFL_HeapFree(upload->playerInfo);
        upload->playerInfo = NULL;
    }
}

// Copies the player's profile and asks the server whether the name is a bad word
static void WifiBSubwayUpload_CheckName(WifiBSubwayUpload *upload, GameData *gameData, HeapID heapId) {
    StrBuf *name;
    const u16 *chars;
    u32 i;

    func_02008b34(GetGameDataPlayerInfo(gameData), upload->playerInfo);
    name = copyTrainerNameToNewStrbuf(upload->playerInfo->name, heapId);
    sys_memset(&upload->nameCheck, 0, sizeof(WifiBSubwayNameCheck));
    upload->nameCheck.heapId = heapId;
    chars = GFL_StrBufGetStringPtr(name);
    for (i = 0; i < GFL_StrBufGetCharCount(name) + 1; i++) {
        if (chars[i] == GFL_StrBufGetTerminator()) {
            upload->nameCheck.name[i] = 0;
        } else {
            upload->nameCheck.name[i] = chars[i];
        }
    }
    upload->nameCheck.words = upload->nameCheck.name;
    upload->nameCheck.badWordCount = 0;
    func_ov011_0216bea4(&upload->nameCheck.words, 1, NULL, 0, &upload->nameCheck.result,
                        &upload->nameCheck.badWordCount, sizeof(upload->sign));
    GFL_StrBufFree(name);
}

// Returns 0 until the check ends, 1 once it does, and 2 when the name was replaced
static u32 WifiBSubwayUpload_WaitCheckName(WifiBSubwayUpload *upload, HeapID heapId) {
    BOOL done = func_ov011_0216bed4() == 2;
    BOOL bad;

    if (done) {
        bad = upload->nameCheck.badWordCount > 0;
    }
    if (!done) {
        return 0;
    }
    if (done == TRUE && bad == TRUE) {
        MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN, heapId);
        StrBuf *name = GFL_MsgDataLoadStrbufNew(msgData, DEFAULT_NAME_MSG);

        GFL_MsgDataFree(msgData);
        copyTrainerNameFromStrbuf(upload->playerInfo, name);
        GFL_StrBufFree(name);
        return 2;
    }
    return 1;
}

// Sends the player's profile and fills the record
static void WifiBSubwayUpload_SetProfile(WifiBSubwayUpload *upload, GameData *gameData, WifiBSubwayError *error) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    BSubwayWifiData *wifiData = func_02017968(gameData);
    BSubwayScoreData *score = func_0201795c(gameData);
    PlayerInfo *playerInfo = upload->playerInfo;

    DpwProfile_Fill(&upload->profile, playerInfo);
    DpwBt_SetProfile(&upload->profile, &upload->profileResult);
    WifiBSubwayError_StartDpw(error);
    WifiBSubwayUpload_FillPlayer(&upload->player, gameData, playerInfo);
    upload->wifiData = wifiData;
    upload->score = score;
}

static BOOL WifiBSubwayUpload_WaitProfile(WifiBSubwayUpload *upload, WifiBSubwayError *error) {
    s32 result;

    if (WifiBSubwayError_WaitDpw(error, &result) && !WifiBSubwayError_Check(error)) {
        upload->profileSet = TRUE;
        return TRUE;
    }
    return FALSE;
}

static void WifiBSubwayUpload_FillPlayer(Dpw_Bt_Player *player, GameData *gameData, PlayerInfo *playerInfo) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    UnityTowerSurveySave *survey = getUnityTower_SurveySaveBlkAddrress(save);
    WifiList *wifiList = GameData_GetWifiList(gameData);
    BSubwayScoreData *score = func_0201795c(gameData);
    void *cgear = getCGearDataBlkAddress(save);
    int i;

    sys_memset(player, 0, sizeof(Dpw_Bt_Player));
    sys_memcpy(GetPlayerName(playerInfo), player->name, sizeof(player->name));
    *(u32 *)player->playerId = getIDAsUInt(playerInfo);
    player->versionCode = game_version;
    player->langCode = region;
    player->countryCode = UnityTowerVisitor_GetCountry(playerInfo);
    player->localCode = UnityTowerVisitor_GetProvince(playerInfo);
    player->gender = getTrainerGender(playerInfo);
    player->trainerType = func_02008bf4(playerInfo);
    player->trainerType = func_0202b5e8(player->trainerType);
    for (i = 0; i < PLAYER_MESSAGE_COUNT; i++) {
        CGearPhrases_Get(cgear, i + 1, (PMSData *)player->messages[i]);
    }
    CGearPhrases_Get(cgear, 4, (PMSData *)player->leaderMessage);
    player->result = BSubwayScoreData_GetWifiScore(score);
    BSubwayScoreData_GetTeam(score, 1, player->pokemon);
}

// Ends the check's request when the Wi-Fi library reports an error
static void WifiBSubwayUpload_OnError(void *work, int code, int type, int error) {
    WifiBSubwayUpload *upload = work;

    if (upload->nhttp != NULL) {
        NHttpRap_EndRequest(upload->nhttp);
    }
}

// Asks the server to check the team's Pokémon
static void WifiBSubwayUpload_CheckPokemon(WifiBSubwayUpload *upload, WifiBSubwayError *error, void *loginBuffer,
                                           HeapID heapId) {
    upload->nhttp = NHttpRap_Create(heapId, func_02008bdc(upload->playerInfo), loginBuffer);
    DWCRap_SetErrorFunc(WifiBSubwayUpload_OnError, upload);
    NHttpRap_BeginPost(upload->nhttp, heapId, sizeof(upload->player.pokemon), 4);
    NHttpRap_AddPostData(upload->nhttp, upload->player.pokemon, sizeof(upload->player.pokemon));
    upload->pokemonCount = TEAM_POKEMON_COUNT;
    NHttpRap_SendValidate(upload->nhttp);
    NHttpRap_StartRequest(upload->nhttp);
    WifiBSubwayError_StartHttp(error);
}

// Returns a CHECK_RESULT_*. A Pokémon that fails the check gets its species' name for a nickname, to be checked
// again; on the last check, the failure is an error instead
static u32 WifiBSubwayUpload_WaitCheckPokemon(WifiBSubwayUpload *upload, WifiBSubwayError *error, BOOL last) {
    u32 result = CHECK_RESULT_WAIT;
    int status = NHttpRap_GetStatus(upload->nhttp);
    int poll = NHttpRap_Poll(upload->nhttp);
    void *answer;
    int i;

    if (status != 200 && poll == 0) {
        WifiBSubwayError_SetHttpError(error, status);
        NHttpRap_EndRequest(upload->nhttp);
        NHttpRap_FreePostData(upload->nhttp);
        NHttpRap_Destroy(upload->nhttp);
        upload->nhttp = NULL;
        DWCRap_SetErrorFunc(NULL, NULL);
        return CHECK_RESULT_NG;
    }
    if (poll != 0) {
        WifiBSubwayError_CountHttp(error);
    } else {
        answer = NHttpRap_GetAnswer(upload->nhttp);
        if (NHttpRap_GetCheckStatus(answer) == 0) {
            BOOL ok = TRUE;

            sys_memcpy(NHttpRap_GetCheckSignature(answer, upload->pokemonCount), upload->sign, sizeof(upload->sign));
            for (i = 0; i < upload->pokemonCount; i++) {
                if (NHttpRap_GetCheckResult(answer, i)) {
                    ok = FALSE;
                    if (last) {
                        WifiBSubwayError_SetCheckError(error, CHECK_ERROR_POKEMON);
                    } else {
                        WifiBSubwayUpload_ResetNickname(upload, i);
                    }
                }
            }
            if (ok) {
                result = CHECK_RESULT_OK;
            } else {
                result = CHECK_RESULT_NG;
            }
        } else {
            if (last) {
                WifiBSubwayError_SetCheckError(error, CHECK_ERROR_STATUS);
            }
            result = CHECK_RESULT_NG;
            for (i = 0; i < upload->pokemonCount; i++) {
                if (NHttpRap_GetCheckResult(answer, i)) {
                    WifiBSubwayUpload_ResetNickname(upload, i);
                }
            }
        }
        NHttpRap_FreePostData(upload->nhttp);
        NHttpRap_Destroy(upload->nhttp);
        upload->nhttp = NULL;
        DWCRap_SetErrorFunc(NULL, NULL);
    }
    return result;
}

static void WifiBSubwayUpload_ResetNickname(WifiBSubwayUpload *upload, int index) {
    BSubwayPokemon *pokemon = (BSubwayPokemon *)upload->player.pokemon;

    GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, pokemon[index].species, pokemon[index].nickname,
                          NELEMS(pokemon[index].nickname));
}

static void WifiBSubwayUpload_CancelCheck(WifiBSubwayUpload *upload) {
    if (upload->nhttp != NULL) {
        DWCRap_SetErrorFunc(NULL, NULL);
        NHttpRap_FreePostData(upload->nhttp);
        NHttpRap_Destroy(upload->nhttp);
        upload->nhttp = NULL;
    }
}

// Sends the record, for the train the player's saved trainers are of
static void WifiBSubwayUpload_Send(WifiBSubwayUpload *upload, WifiBSubwayError *error) {
    u32 rank = BSubwayWifiData_GetPlayerRank(upload->wifiData);
    u32 number = BSubwayWifiData_GetPlayerRoom(upload->wifiData);
    u32 rounds = BSubwayScoreData_GetWifiScoreRounds(upload->score);

    DpwBt_Upload((u8)rank, (u8)number, (u8)rounds, &upload->player, upload->sign, sizeof(upload->sign));
    WifiBSubwayError_StartDpw(error);
}

static BOOL WifiBSubwayUpload_WaitSend(WifiBSubwayUpload *upload, WifiBSubwayError *error) {
    s32 result;

    if (WifiBSubwayError_WaitDpw(error, &result) && !WifiBSubwayError_Check(error)) {
        return TRUE;
    }
    return FALSE;
}

static void WifiBSubwayError_Init(WifiBSubwayError *error) {
    sys_memset(error, 0, sizeof(WifiBSubwayError));
}

// Starts the wait for a request of the server library
static void WifiBSubwayError_StartDpw(WifiBSubwayError *error) {
    error->timeout = FALSE;
    error->timer = 0;
}

// Returns whether the request ended, giving its result if it went well; one that takes too long is canceled
static BOOL WifiBSubwayError_WaitDpw(WifiBSubwayError *error, s32 *result) {
    BOOL done = FALSE;

    if (!DpwBt_IsFinished()) {
        error->timer++;
        if (error->timer == REQUEST_TIMEOUT) {
            DpwBt_Cancel();
            error->timeout = TRUE;
        }
    } else {
        s32 dpwResult = DpwBt_GetResult();

        done = TRUE;
        switch (dpwResult) {
        case -7:
        case -6:
        case -4:
        case -3:
        case -2:
        case -1:
            error->dpwResult = dpwResult;
            break;
        case -5:
            GFL_NetErrShow(TRUE);
            break;
        default:
            if (dpwResult >= 0) {
                *result = dpwResult;
            } else {
                GFL_NetErrShow(TRUE);
            }
            break;
        }
    }
    return done;
}

static BOOL WifiBSubwayError_Check(const WifiBSubwayError *error) {
    if (error->dpwResult < 0) {
        return TRUE;
    }
    if (error->serverState != 0) {
        return TRUE;
    }
    if (error->checkError != CHECK_ERROR_NONE) {
        return TRUE;
    }
    if (error->timeout) {
        return TRUE;
    }
    return FALSE;
}

// The message of the error
static u32 WifiBSubwayError_GetMessage(const WifiBSubwayError *error) {
    u32 message = 0;

    if (error->dpwResult < 0) {
        switch (error->dpwResult) {
        case -2:
            message = Bank0413_Text_UnableConnectWiFi;
            break;
        case -6:
            message = Bank0413_Text_UnableConnectWiFi_2;
            break;
        case -1:
            message = Bank0413_Text_WiFiTrainVery_2;
            break;
        case -7:
            message = Bank0413_Text_WereDisconnectedFromWi_2;
            break;
        case -4:
            message = Bank0413_Text_WereDisconnectedFromWi;
            break;
        case -3:
            message = Bank0413_Text_NumberCannotSpecifiedWi;
            break;
        default:
            message = Bank0413_Text_CommunicationErrorErrorCode;
            break;
        }
    } else if (error->serverState != 0) {
        switch (error->serverState) {
        case 1:
            message = Bank0413_Text_WiFiTrainCurrently;
            break;
        case 2:
            message = Bank0413_Text_WiFiTrainVery;
            break;
        }
    } else if (error->checkError != CHECK_ERROR_NONE) {
        switch (error->checkError) {
        case CHECK_ERROR_POKEMON:
            message = Bank0413_Text_ThereAppearsProblemPokemon;
            break;
        case CHECK_ERROR_STATUS:
            message = Bank0413_Text_ThereAppearsProblemPokemon;
            break;
        }
    } else if (error->timeout) {
        message = Bank0413_Text_UnableConnectWiFi;
    }
    return message;
}

// Records the Wi-Fi error of a failed check, for the error screen; returns whether there was one
static BOOL WifiBSubwayError_ShowHttpError(const WifiBSubwayError *error) {
    if (error->checkError == CHECK_ERROR_HTTP) {
        u32 message;

        switch (error->httpStatus) {
        case 200:
        case 400:
            message = 0x41;
            break;
        case 401:
            message = 0x42;
            break;
        case 408:
            message = 0x44;
            break;
        default:
            message = 0x43;
            break;
        }
        func_020120f0(message);
        return TRUE;
    }
    return FALSE;
}

static void WifiBSubwayError_SetCheckError(WifiBSubwayError *error, u32 checkError) {
    error->checkError = checkError;
}

static void WifiBSubwayError_SetHttpError(WifiBSubwayError *error, s32 status) {
    error->checkError = CHECK_ERROR_HTTP;
    error->httpStatus = status;
}

// Starts the wait for the check's request
static void WifiBSubwayError_StartHttp(WifiBSubwayError *error) {
    error->timeout = FALSE;
    error->timer = 0;
}

static void WifiBSubwayError_CountHttp(WifiBSubwayError *error) {
    error->timer++;
    if (error->timer == REQUEST_TIMEOUT) {
        error->timeout = TRUE;
    }
}

// Opens the code input screen for a train's number
static void WifiBSubway_CallRoomInput(WifiBSubwayWork *wk, HeapID heapId) {
    sWifiBSubwayRoomBlocks[0] = WifiBSubwayRoom_GetCountDigits(&wk->room);
    wk->codeIn = CodeInParam_Create(heapId, 1, WifiBSubwayRoom_GetCount(&wk->room), 3, sWifiBSubwayRoomBlocks);
    GSYS_QueueProc(wk->param->gsys, OVERLAY_CODEIN, &CODEIN_PROC_FUNCTIONS, wk->codeIn);
}

// Opens the code input screen for a rank
static void WifiBSubway_CallRankInput(WifiBSubwayWork *wk, HeapID heapId) {
    wk->codeIn = CodeInParam_Create(heapId, 2, RANK_MAX, 2, sWifiBSubwayRankBlocks);
    GSYS_QueueProc(wk->param->gsys, OVERLAY_CODEIN, &CODEIN_PROC_FUNCTIONS, wk->codeIn);
}

static void WifiBSubway_FreeCodeIn(WifiBSubwayWork *wk) {
    if (wk->codeIn != NULL) {
        CodeInParam_Free(wk->codeIn);
        wk->codeIn = NULL;
    }
}

// The number typed, from ASCII or full-width digits
static u32 WifiBSubway_GetCodeInNumber(WifiBSubwayWork *wk) {
    u16 terminator = GFL_StrBufGetTerminator();
    const u16 *chars = GFL_StrBufGetStringPtr(wk->codeIn->str);
    u32 number = 0;
    u16 i;

    for (i = 0; chars[i] != terminator; i++) {
        u16 digit = chars[i] - L'0';

        if (digit >= 10) {
            digit = chars[i] - 0xff10;
        }
        number *= 10;
        number += digit;
    }
    return number;
}

static void WifiBSubway_CallLogin(WifiBSubwayWork *wk) {
    wk->login.gameData = wk->gameData;
    wk->login.unk4 = 0;
    wk->login.unk18 = TRUE;
    wk->login.unk8 = 1;
    wk->login.unkC = 0x2c;
    wk->login.buffer = wk->loginBuffer;
    GSYS_QueueProc(wk->param->gsys, OVERLAY_WIFILOGIN, &WIFILOGIN_PROC_FUNCTIONS, &wk->login);
}

static void WifiBSubway_FreeLogin(WifiBSubwayWork *wk) {
}

static u32 WifiBSubway_GetLoginResult(const WifiBSubwayWork *wk) {
    return wk->login.result;
}

static void WifiBSubway_CallLogout(WifiBSubwayWork *wk) {
    wk->logout.gameData = wk->gameData;
    wk->logout.unk4 = 0;
    wk->logout.unk8 = 1;
    wk->logout.unkC = 1;
    GSYS_QueueProc(wk->param->gsys, OVERLAY_WIFILOGIN, &WIFILOGOUT_PROC_FUNCTIONS, &wk->logout);
}

static void WifiBSubway_FreeLogout(WifiBSubwayWork *wk) {
}

// Runs the mode's step, and returns a MODE_RESULT_*
static u32 WifiBSubway_RunMode(WifiBSubwayWork *wk, HeapID heapId) {
    WifiBSubwayModeFunc funcs[] = {
        WifiBSubway_UploadMain,
        WifiBSubway_DownloadTrainMain,
        WifiBSubway_DownloadLeadersMain,
    };

    return funcs[wk->param->mode](wk, heapId);
}

// Sends the record: the team's Pokémon are checked, a second time if one failed, and the record sent and marked sent
static u32 WifiBSubway_UploadMain(WifiBSubwayWork *wk, HeapID heapId) {
    switch (wk->seq) {
    case 0:
        WifiBSubwayView_Print(&wk->view, Bank0413_Text_SendingRecordDataDont);
        wk->seq++;
        break;
    case 1:
        WifiBSubwayUpload_CheckPokemon(&wk->upload, &wk->error, wk->loginBuffer, heapId);
        wk->seq++;
        break;
    case 2: {
        u32 result = WifiBSubwayUpload_WaitCheckPokemon(&wk->upload, &wk->error, FALSE);

        if (result == CHECK_RESULT_OK) {
            wk->seq = 5;
        } else if (result == CHECK_RESULT_NG) {
            wk->seq = 3;
        }
        break;
    }
    case 3:
        WifiBSubwayUpload_CheckPokemon(&wk->upload, &wk->error, wk->loginBuffer, heapId);
        wk->seq++;
        break;
    case 4:
        if (WifiBSubwayUpload_WaitCheckPokemon(&wk->upload, &wk->error, TRUE) == CHECK_RESULT_OK) {
            wk->seq = 5;
        }
        break;
    case 5:
        WifiBSubwayUpload_Send(&wk->upload, &wk->error);
        wk->seq++;
        break;
    case 6:
        if (WifiBSubwayUpload_WaitSend(&wk->upload, &wk->error) && WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq++;
        }
        break;
    case 7:
        BSubwayScoreData_SetFlag(wk->score, 1, BSUBWAY_FLAG_RESET);
        WifiBSubway_StartSave(wk);
        wk->seq++;
        break;
    case 8:
        if (WifiBSubway_WaitSave(wk)) {
            WifiBSubwayView_StopWaitIcon(&wk->view);
            WifiBSubwayView_Print(&wk->view, Bank0413_Text_DataHasBeenSent);
            wk->seq++;
        }
        break;
    case 9:
        if (WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq++;
        }
        break;
    case 10:
        return MODE_RESULT_END;
    }
    return MODE_RESULT_CONTINUE;
}

// Downloads a train of the player's rank: the player types its number, and a train already taken today is refused
static u32 WifiBSubway_DownloadTrainMain(WifiBSubwayWork *wk, HeapID heapId) {
    RTCTime time;
    RTCDate date;

    switch (wk->seq) {
    case 0:
        WifiBSubwayRoom_RequestCount(&wk->room, &wk->error, BSubwayScoreData_GetWifiRank(wk->score));
        wk->seq++;
        break;
    case 1:
        if (WifiBSubwayRoom_WaitCount(&wk->room, &wk->error)) {
            wk->seq++;
        }
        break;
    case 2:
        WifiBSubwayView_StopWaitIcon(&wk->view);
        WifiBSubwayView_SetNumber(&wk->view, 0, WifiBSubwayRoom_GetCount(&wk->room),
                                  WifiBSubwayRoom_GetCountDigits(&wk->room));
        WifiBSubwayView_Print(&wk->view, Bank0413_Text_EnterTrainNumber01);
        wk->seq++;
        break;
    case 3:
        if (WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq++;
        }
        break;
    case 4:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 1);
        wk->seq++;
        break;
    case 5:
        if (!GFL_FadeIsRunning()) {
            WifiBSubwayView_Free(&wk->view);
            wk->seq++;
        }
        break;
    case 6:
        WifiBSubway_CallRoomInput(wk, heapId);
        wk->seq++;
        break;
    case 7:
        wk->inputNumber = WifiBSubway_GetCodeInNumber(wk);
        WifiBSubway_FreeCodeIn(wk);
        WifiBSubwayView_Init(&wk->view, heapId);
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 1);
        wk->seq++;
        break;
    case 8:
        if (!GFL_FadeIsRunning()) {
            u32 rank = WifiBSubwayRoom_GetRank(&wk->room);
            u32 number = wk->inputNumber;
            u32 count = WifiBSubwayRoom_GetCount(&wk->room);

            func_ov011_0215dda8(&date, &time);
            if (number < 1 || number > count) {
                wk->seq = 16;
            } else if (BSubwayWifiData_GetRoomFlag(wk->wifiData, rank, number, &date)) {
                wk->seq = 14;
            } else {
                WifiBSubwayRoom_Download(&wk->room, &wk->error, wk->inputNumber);
                WifiBSubwayView_SetNumber(&wk->view, 0, WifiBSubwayRoom_GetNumber(&wk->room),
                                          WifiBSubwayRoom_GetCountDigits(&wk->room));
                WifiBSubwayView_SetNumber(&wk->view, 1, WifiBSubwayRoom_GetRank(&wk->room), 2);
                WifiBSubwayView_Print(&wk->view, Bank0413_Text_ReceivingDataRankTrain);
                WifiBSubwayView_StartWaitIcon(&wk->view);
                wk->seq++;
            }
        }
        break;
    case 9:
        if (WifiBSubwayRoom_WaitDownload(&wk->room, &wk->error) && WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq++;
        }
        break;
    case 10:
        WifiBSubwayRoom_FixData(&wk->room);
        WifiBSubwayRoom_SavePlayers(&wk->room, wk->wifiData);
        WifiBSubwayRoom_SaveLeaders(&wk->room, wk->wifiData);
        WifiBSubway_StartSave(wk);
        wk->seq++;
        break;
    case 11:
        if (WifiBSubway_WaitSave(wk)) {
            WifiBSubwayView_StopWaitIcon(&wk->view);
            WifiBSubwayView_Print(&wk->view, Bank0413_Text_DataHasBeenReceived);
            wk->seq++;
        }
        break;
    case 12:
        if (WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq++;
        }
        break;
    case 13:
        return MODE_RESULT_END;
    case 14:
        WifiBSubwayView_Print(&wk->view, Bank0413_Text_MayNotChallengeTrain);
        wk->seq++;
        break;
    case 15:
        if (WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq = 2;
        }
        break;
    case 16:
        WifiBSubwayView_Print(&wk->view, Bank0413_Text_CancelChallenge);
        wk->seq++;
        break;
    case 17:
        if (WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq++;
        }
        break;
    case 18:
        WifiBSubwayView_StartYesNo(&wk->view);
        wk->seq++;
        break;
    case 19: {
        u32 result = WifiBSubwayView_UpdateYesNo(&wk->view);

        if (result == 0) {
            return MODE_RESULT_CANCEL;
        } else if (result == BMPMENU_CANCEL) {
            wk->seq = 2;
        }
        break;
    }
    }
    return MODE_RESULT_CONTINUE;
}

// Downloads the leaders of a train: the player types its rank and its number
static u32 WifiBSubway_DownloadLeadersMain(WifiBSubwayWork *wk, HeapID heapId) {
    switch (wk->seq) {
    case 0:
        WifiBSubwayView_StopWaitIcon(&wk->view);
        WifiBSubwayView_Print(&wk->view, Bank0413_Text_EnterRankTrain01);
        wk->seq++;
        break;
    case 1:
        if (WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq++;
        }
        break;
    case 2:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 1);
        wk->seq++;
        break;
    case 3:
        if (!GFL_FadeIsRunning()) {
            WifiBSubwayView_Free(&wk->view);
            wk->seq++;
        }
        break;
    case 4:
        WifiBSubway_CallRankInput(wk, heapId);
        wk->seq++;
        break;
    case 5:
        wk->inputNumber = WifiBSubway_GetCodeInNumber(wk);
        WifiBSubway_FreeCodeIn(wk);
        WifiBSubwayView_Init(&wk->view, heapId);
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 1);
        wk->seq++;
        break;
    case 6:
        if (!GFL_FadeIsRunning()) {
            if (wk->inputNumber >= 1 && wk->inputNumber <= RANK_MAX) {
                WifiBSubwayRoom_RequestCount(&wk->room, &wk->error, wk->inputNumber);
                WifiBSubwayView_Print(&wk->view, Bank0413_Text_CommunicatingPleaseStandBy);
                WifiBSubwayView_StartWaitIcon(&wk->view);
                wk->seq++;
            } else {
                wk->seq = 19;
            }
        }
        break;
    case 7:
        if (WifiBSubwayRoom_WaitCount(&wk->room, &wk->error)) {
            WifiBSubwayView_StopWaitIcon(&wk->view);
            WifiBSubwayView_SetNumber(&wk->view, 0, WifiBSubwayRoom_GetCount(&wk->room),
                                      WifiBSubwayRoom_GetCountDigits(&wk->room));
            WifiBSubwayView_Print(&wk->view, Bank0413_Text_EnterTrainNumber01_2);
            wk->seq++;
        }
        break;
    case 8:
        if (WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq++;
        }
        break;
    case 9:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 1);
        wk->seq++;
        break;
    case 10:
        if (!GFL_FadeIsRunning()) {
            WifiBSubwayView_Free(&wk->view);
            wk->seq++;
        }
        break;
    case 11:
        WifiBSubway_CallRoomInput(wk, heapId);
        wk->seq++;
        break;
    case 12:
        wk->inputNumber = WifiBSubway_GetCodeInNumber(wk);
        WifiBSubway_FreeCodeIn(wk);
        WifiBSubwayView_Init(&wk->view, heapId);
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 1);
        wk->seq++;
        break;
    case 13:
        if (!GFL_FadeIsRunning()) {
            u32 count = WifiBSubwayRoom_GetCount(&wk->room);

            if (wk->inputNumber < 1 || wk->inputNumber > count) {
                wk->seq = 19;
            } else {
                WifiBSubwayRoom_Download(&wk->room, &wk->error, wk->inputNumber);
                WifiBSubwayView_SetNumber(&wk->view, 0, WifiBSubwayRoom_GetNumber(&wk->room),
                                          WifiBSubwayRoom_GetCountDigits(&wk->room));
                WifiBSubwayView_SetNumber(&wk->view, 1, WifiBSubwayRoom_GetRank(&wk->room), 2);
                WifiBSubwayView_Print(&wk->view, Bank0413_Text_ReceivingDataRankTrain);
                WifiBSubwayView_StartWaitIcon(&wk->view);
                wk->seq++;
            }
        }
        break;
    case 14:
        if (WifiBSubwayRoom_WaitDownload(&wk->room, &wk->error) && WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq++;
        }
        break;
    case 15:
        WifiBSubwayRoom_FixData(&wk->room);
        WifiBSubwayRoom_SaveLeaders(&wk->room, wk->wifiData);
        WifiBSubway_StartSave(wk);
        wk->seq++;
        break;
    case 16:
        if (WifiBSubway_WaitSave(wk)) {
            WifiBSubwayView_StopWaitIcon(&wk->view);
            WifiBSubwayView_Print(&wk->view, Bank0413_Text_DataHasBeenReceived);
            wk->seq++;
        }
        break;
    case 17:
        if (WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq++;
        }
        break;
    case 18:
        return MODE_RESULT_END;
    case 19:
        WifiBSubwayView_Print(&wk->view, Bank0413_Text_WouldLikeCancelData);
        wk->seq++;
        break;
    case 20:
        if (WifiBSubwayView_WaitPrint(&wk->view)) {
            wk->seq++;
        }
        break;
    case 21:
        WifiBSubwayView_StartYesNo(&wk->view);
        wk->seq++;
        break;
    case 22: {
        u32 result = WifiBSubwayView_UpdateYesNo(&wk->view);

        if (result == 0) {
            return MODE_RESULT_CANCEL;
        } else if (result == BMPMENU_CANCEL) {
            WifiBSubwayRoom_Init(&wk->room);
            wk->seq = 0;
        }
        break;
    }
    }
    return MODE_RESULT_CONTINUE;
}

// Starts the server library and asks for the server's state
static void WifiBSubway_Connect(WifiBSubwayWork *wk) {
    DWCUserData *userData = func_02009f7c(wk->wifiList);
    s32 profileId;

    func_02008be0(wk->playerInfo, WifiList_GetMyGSID(wk->wifiList));
    profileId = func_02008bdc(wk->playerInfo);
    DpwBt_Init(profileId, func_02057ec4(userData), 0);
    DpwBt_GetServerState();
    WifiBSubwayError_StartDpw(&wk->error);
    wk->dpwActive = TRUE;
}

static BOOL WifiBSubway_WaitConnect(WifiBSubwayWork *wk) {
    s32 result;

    return WifiBSubwayError_WaitDpw(&wk->error, &result);
}

// Cancels the request that runs, if any
static void WifiBSubway_Disconnect(WifiBSubwayWork *wk) {
    if (!DpwBt_IsFinished()) {
        DpwBt_Cancel();
        WifiBSubwayError_StartDpw(&wk->error);
    }
}

static BOOL WifiBSubway_WaitDisconnect(WifiBSubwayWork *wk) {
    s32 result;

    if (!wk->dpwActive) {
        return TRUE;
    }
    if (WifiBSubwayError_WaitDpw(&wk->error, &result)) {
        DpwBt_End();
        wk->dpwActive = FALSE;
        return TRUE;
    }
    return FALSE;
}

// Prints the message of the screen's own error and returns TRUE; an error of the Wi-Fi library or the network is left
// to the error screen
static BOOL WifiBSubway_PrintError(WifiBSubwayWork *wk) {
    if (DWCRapCommon_CheckError(TRUE, FALSE)) {
        return FALSE;
    }
    if (GFL_NetErrCheck()) {
        func_02042540();
        return FALSE;
    }
    if (WifiBSubwayError_Check(&wk->error)) {
        u32 message;

        if (WifiBSubwayError_ShowHttpError(&wk->error)) {
            return FALSE;
        }
        message = WifiBSubwayError_GetMessage(&wk->error);
        WifiBSubwayView_StopWaitIcon(&wk->view);
        WifiBSubwayView_Print(&wk->view, message);
        return TRUE;
    }
    return FALSE;
}

static BOOL WifiBSubway_WaitError(WifiBSubwayWork *wk) {
    return WifiBSubwayView_WaitPrint(&wk->view);
}

static void WifiBSubwayView_Init(WifiBSubwayView *view, HeapID heapId) {
    view->graphic = WifiBSubwayGraphic_Create(1, heapId);
    view->heapId = heapId;
    WifiBSubwayView_LoadGraphics(view, heapId);
    view->window = BmpWin_CreateDynamic(MESSAGE_BG, 1, 19, 30, 4, 12, TRUE);
    BmpWin_DrawFrame(view->window, 1, view->messageFrameChars, FRAME_PALETTE);
    GFL_BitmapFill(BmpWin_GetBitmap(view->window), 15);
    BmpWin_FlushMap(view->window);
    BmpWin_FlushChar(view->window);
    GFL_BGSysQueueScrLoad(MESSAGE_BG);
    view->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0413, heapId);
    view->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    view->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    view->message = GFL_StrBufCreate(0x80, heapId);
    view->expanded = GFL_StrBufCreate(0x80, heapId);
    view->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 1, 0);
    view->initialized = TRUE;
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    func_02042ba8(TRUE, heapId);
}

static void WifiBSubwayView_Free(WifiBSubwayView *view) {
    if (view->initialized) {
        if (view->stream != NULL) {
            func_020223cc(view->stream);
        }
        GFL_TCBExMgrFree(view->tcbManager);
        GFL_MsgDataFree(view->msgData);
        GFL_WordSetSystemFree(view->wordSet);
        GFL_FontFree(view->font);
        GFL_StrBufFree(view->message);
        GFL_StrBufFree(view->expanded);
        WifiBSubwayView_FreeGraphics(view);
        WifiBSubwayView_StopWaitIcon(view);
        if (view->yesNo != NULL) {
            ConfirmDialog_Free(view->yesNo);
            view->yesNo = NULL;
        }
        BmpWin_Free(view->window);
        WifiBSubwayGraphic_Free(view->graphic);
        func_020232d8();
        sys_memset(view, 0, sizeof(WifiBSubwayView));
    }
}

static void WifiBSubwayView_Update(WifiBSubwayView *view) {
    if (view->initialized) {
        GFL_TCBExMgrUpdate(view->tcbManager);
    }
}

// Loads the palettes the cycle blends between, makes the blends and starts the cycle
static inline void WifiBSubwayView_InitCycle(WifiBSubwayView *view, ArcTool *arc, u32 heapId) {
    int i;
    int count = 0;
    NNSG2dPaletteData *palette;
    void *file;

    sys_memset(&view->cycle, 0, sizeof(view->cycle));
    file = GFL_G2DIOReadNCLRArc(arc, 2, &palette, heapId);
    sys_memcpy(palette->rawData, view->cycle.palettes, sizeof(view->cycle.palettes));
    sys_memcpy(palette->rawData, view->cycle.blended, sizeof(view->cycle.palettes));
    GFL_HeapFree(file);

    // Colors 4 to 10 of each palette, blended into the next in 4 steps
    for (i = 0; i < PALETTE_COUNT; i++) {
        int next = i + 1;
        fx32 fraction;
        BOOL done;

        if (next >= PALETTE_COUNT) {
            next -= PALETTE_COUNT;
        }
        fraction = 0;
        done = FALSE;
        while (TRUE) {
            int j;

            for (j = 4; j < 11; j++) {
                BlendColors(&view->cycle.palettes[i][j], &view->cycle.blended[count][j], 1, fraction >> 8,
                            view->cycle.palettes[next][j]);
            }
            count++;
            if (done == TRUE) {
                break;
            }
            fraction += FX32_ONE * 6 / 16;
            if (fraction >= FX32_ONE) {
                fraction = FX32_ONE;
                done = TRUE;
            }
        }
    }
    cp15_flushDC(view->cycle.blended, sizeof(view->cycle.blended));
    view->cycle.active = TRUE;
    view->cycle.task = GFL_VBlankTCBAdd(WifiBSubwayView_CyclePalette, &view->cycle, 20);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0, GX_BLEND_PLANEMASK_BG3, 8, 16);
    view->cycle.scrollBg = 0;
}

// Loads the Wi-Fi login screen's BGs and the frames of the message window and the yes/no menu
static void WifiBSubwayView_LoadGraphics(WifiBSubwayView *view, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PDWACC, heapId);
    u32 chars;

    GFL_G2DIOLoadArcNCLRDefault(arc, 3, PALTYPE_SUB_BG, 0, 0, heapId);
    chars = GFL_BGSysLoadArcNCGRDynamic(arc, 4, 4, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 6, 4, 0, (u16)chars, 0, FALSE, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 3, PALTYPE_MAIN_BG, 0, 0, heapId);
    chars = GFL_BGSysLoadArcNCGRDynamic(arc, 4, 0, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 5, 0, 0, (u16)chars, 0, FALSE, heapId);
    chars = GFL_BGSysLoadArcNCGRDynamic(arc, 4, 3, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 7, 3, 0, (u16)chars, 0, FALSE, heapId);
    WifiBSubwayView_InitCycle(view, arc, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysAllocChar(MESSAGE_BG, BGSYS_TILE_SIZE_16, FALSE);
    GFL_BGSysAllocChar(YES_NO_BG, BGSYS_TILE_SIZE_16, FALSE);
    view->messageFrameChars = LoadCursorImageEndOfHeap(MESSAGE_BG, FRAME_PALETTE, 0, heapId);
    view->yesNoFrameChars = LoadCursorImageEndOfHeap(YES_NO_BG, FRAME_PALETTE, 0, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 12 * 0x20, 0x20, heapId);
}

static void WifiBSubwayView_CyclePalette(TCB *task, void *work) {
    WifiBSubwayPaletteCycle *cycle = work;

    if (cycle->active) {
        cycle->skip ^= 1;
        if (!(cycle->skip & 1)) {
            gfxUploadStdPaletteBGA(&cycle->blended[cycle->frame][4], 0x20 + 4 * 2, 7 * 2);
            gfxUploadStdPaletteBGB(&cycle->blended[cycle->frame][4], 0x20 + 4 * 2, 7 * 2);
            cycle->frame++;
            if (cycle->frame >= PALETTE_COUNT * BLEND_STEPS) {
                cycle->frame = 0;
            }
            GFL_BGSysMoveBG(cycle->scrollBg, BG_MOVE_LEFT, 7);
        }
    }
}

static void WifiBSubwayView_FreeGraphics(WifiBSubwayView *view) {
    GFL_BGSysFreeCharMemory(MESSAGE_BG, CHAR_POS(view->messageFrameChars), CHAR_SIZE(view->messageFrameChars));
    GFL_BGSysFreeCharMemory(YES_NO_BG, CHAR_POS(view->yesNoFrameChars), CHAR_SIZE(view->yesNoFrameChars));
    if (view->cycle.task != NULL) {
        GFL_TCBRemove(view->cycle.task);
        view->cycle.task = NULL;
        view->cycle.active = FALSE;
    }
    G2_BlendNone();
}

// Prints a message into the window, with the numbers that were set
static void WifiBSubwayView_Print(WifiBSubwayView *view, u32 message) {
    if (view->stream != NULL) {
        func_020223cc(view->stream);
        view->stream = NULL;
    }
    GFL_MsgDataLoadStrbuf(view->msgData, message, view->message);
    GFL_WordSetFormatStrbuf(view->wordSet, view->expanded, view->message);
    GFL_BitmapFill(BmpWin_GetBitmap(view->window), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    view->stream = func_02022268(view->window, 0, 0, view->expanded, view->font, func_02017bcc(), view->tcbManager, 2,
                                 view->heapId, 15);
    BmpWin_DrawFrame(view->window, 1, view->messageFrameChars, FRAME_PALETTE);
    BmpWin_FlushChar(view->window);
}

// Returns whether the message is printed, going on at the A Button or a touch where it waits
static BOOL WifiBSubwayView_WaitPrint(WifiBSubwayView *view) {
    u32 state;

    if (view->stream == NULL) {
        return TRUE;
    }
    state = func_020223b4(view->stream);
    if (state == PRINT_STREAM_DONE) {
        func_020223cc(view->stream);
        view->stream = NULL;
        return TRUE;
    }
    if (state == PRINT_STREAM_PAUSED) {
        if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A || func_0203da48()) {
            func_020223bc(view->stream);
        }
    }
    return FALSE;
}

static void WifiBSubwayView_SetNumber(WifiBSubwayView *view, u32 index, u32 number, u32 digits) {
    WordSetNumber(view->wordSet, index, number, digits, 2, TRUE);
}

static void WifiBSubwayView_StartYesNo(WifiBSubwayView *view) {
    sWifiBSubwayYesNoSetup.unk4 = view->yesNoFrameChars;
    view->yesNo = ShopUI_CreateConfirmDialog(&sWifiBSubwayYesNoSetup, view->yesNoFrameChars, FRAME_PALETTE, 0,
                                             view->heapId);
}

// Returns 0 for yes, BMPMENU_CANCEL for no, also when there is no menu, and BMPMENU_NULL until the player answers
static u32 WifiBSubwayView_UpdateYesNo(WifiBSubwayView *view) {
    u32 result = BMPMENU_CANCEL;

    if (view->yesNo != NULL) {
        result = ConfirmDialog_Update(view->yesNo);
        if (result != BMPMENU_NULL) {
            view->yesNo = NULL;
        }
    }
    return result;
}

static void WifiBSubwayView_StartWaitIcon(WifiBSubwayView *view) {
    view->waitIcon = WaitIcon_Create(GFL_VBlankGetTCBMgr(), view->window, 15, 16, view->heapId);
}

static void WifiBSubwayView_StopWaitIcon(WifiBSubwayView *view) {
    if (view->waitIcon != NULL) {
        WaitIcon_Free(view->waitIcon);
        view->waitIcon = NULL;
    }
}

static void WifiBSubway_StartSave(WifiBSubwayWork *wk) {
    func_0201782c(wk->gameData);
    wk->saving = TRUE;
}

static BOOL WifiBSubway_WaitSave(WifiBSubwayWork *wk) {
    if (!wk->saving) {
        return TRUE;
    }
    if (func_02017850(wk->gameData) == 2) {
        wk->saving = FALSE;
        return TRUE;
    }
    return FALSE;
}
