#include "app/wifibattlematch_net.h"
#include "types.h"
#include "app/livebattlematch_flow.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "dwc/dwc.h"
#include "dwc/gdb.h"
#include "dwc/nd.h"
#include "gfl/dwc_rap.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_state.h"
#include "gfl/net_system.h"
#include "gfl/nhttp_rap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/join_avenue.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "system/game_data.h"
#include "text/system/ability_handlers_btl_main.h"

// wifibattlematch_net.c, named by its embedded string, in overlay 260: the net part of the Wi-Fi battle competitions.
// It matches two players through the Wi-Fi Connection, keeps their stats and teams in the Sake database through the
// Gdb client that follows it in the overlay (dwc/gdb.h), sends the teams from one to the other and downloads the
// competitions' files. The work is called p_wk, as its assertions' texts show

// The most fields that a request on a table can have
#define WBM_GDB_FIELD_TABLE_MAX 0x26

// The table that the players' stats are in
#define WBM_GDB_TABLE_NAME "PlayerStats_v1"

// The kinds of the error that the work keeps
enum {
    WBM_ERROR_NONE,
    WBM_ERROR_DWC,
    WBM_ERROR_GDB,
    WBM_ERROR_UNUSED_3,
    WBM_ERROR_HTTP,
    WBM_ERROR_TIMEOUT,
};

// How an error ends for the player: nothing, the Wi-Fi connection lost, the competition failed or the game's network
// error. They are what DWCRapCommon_CheckError and the error converters answer
enum {
    WBM_RESULT_OK,
    WBM_RESULT_DISCONNECTED,
    WBM_RESULT_FAILED,
    WBM_RESULT_NET_ERROR,
};

// The error that was found, by its kind
typedef struct {
    s32 dwcCode;
    s32 gdbCode;
    s32 gdbSubCode1;
    s32 gdbSubCode2;
    u32 unk10;
    s32 httpCode;
    s32 timeoutCode;
    u32 kind;
} WifiBattleMatchNetError;

// A key of the matching query: its value, the ID the DWC library gave it and its three letters
typedef struct {
    s32 value;
    u8 keyId;
    u8 unk5;
    char name[6];
} WifiBattleMatchNetKey;

// What a player offers in a match, which is sent to the other player whole
typedef struct {
    const AuthCertificate *certificate;
    ScReport *connection;
    u8 unk8[0x28];
    u8 unk30[0x28];
    u8 unk58[0x10];
} WifiBattleMatchNetInfo;

// What overlay 290 shares with this file: the player's ID in the Sake database, the player's side and the other
// player's
typedef struct {
    s32 sake_recordID;
    WifiBattleMatchNetInfo mine;
    WifiBattleMatchNetInfo theirs;
} WifiBattleMatchNetData;

// The kind of a match (0 to 4), how it ended (0 to 6) and the two sides' counts
typedef struct {
    u32 format;
    u32 outcome;
    u32 mine;
    u32 theirs;
} WifiBattleMatchNetParam;

// Sets a battle's fields in the exchange with the other player
typedef int (*WifiBattleMatchNetResultFunc)(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param, int isMine,
                                            int a3);

// Two values that the server answers with, and that are sent back
typedef struct {
    u32 value1;
    u32 value2;
} WifiBattleMatchNetValues;

// What an NHTTP answer starts with: its result, and the two values at its end
typedef struct {
    u32 result;
    u8 unk4[0x7c];
    WifiBattleMatchNetValues values;
} WifiBattleMatchNetAnswer;

// What the download's callback sets, which is in the .bss and not in the work
typedef struct {
    u8 done;
    u8 done2;
    u32 code;
} WifiBattleMatchNetDownload;

// The name that the bad word check reads and its result
typedef struct {
    u16 name[12];
    const u16 *words;
    u8 result;
    int badWordCount;
    u16 unk24;
} WifiBattleMatchNetNameCheck;

// The value of the matching key "ver", which only players of the same version match with
#ifdef BLACK2
#define WBM_MATCH_VERSION 0x20017
#else
#define WBM_MATCH_VERSION 0x20016
#endif

// The name that a player gets when the check refuses theirs, by the version
#ifdef BLACK2
#define WBM_DEFAULT_NAME AbilityHandlersBtlMain_Text_Kuro
#else
#define WBM_DEFAULT_NAME AbilityHandlersBtlMain_Text_Shiro
#endif

struct WifiBattleMatchNet {
    void *unk0;
    WifiList *wifiList;
    GameData *gameData;
    s32 profileId;
    void *loginBuffer;
    u32 syncId;
    WifiBattleMatchNetError error;
    u8 loginDate[6];
    u32 matchState;
    char matchQuery[0x80];
    WifiBattleMatchNetKey matchKeys[8];
    HeapID msgHeapId;
    u32 phase;
    u32 state;
    u32 resumeState;
    u32 counter;
    u32 flagInfoReceived;
    u32 flagPlayerInfoReceived;
    u32 flag140;
    u32 flagPartyReceived;
    u32 flag148;
    u32 flagValueReceived;
    u32 flagPersonReceived;
    u32 receivedValue;
    u32 phaseB;
    WifiBattleMatchNetResultFunc resultFunc;
    s32 errorCode;
    u32 flag164;
    u32 requestSent;
    u32 flag16C;
    u32 flag170;
    u32 gdbOpen;
    u32 unk178;
    void *unk17c;
    GdbRecordsFunc recordsFunc;
    WifiBattleMatchNetParam param;
    GdbField *record;
    const char **tableNames;
    s32 table_name_num;
    u32 unk1a0;
    u32 timer;
    u32 matchTimer;
    u32 unk1ac;
    u32 mode;
    s32 recordIdArg;
    u8 unk1b8[0x10];
    WifiBattleMatchNetRecordData recordData;
    u8 recvBuffer[0x1000];
    DWCEvalFunc evalFunc;
    NHttpRap *p_nhttp;
    WifiBattleMatchNetValues httpValues;
    WifiBattleMatchNetValues postData;
    u8 downloadBuffer[0x160];
    DWCNdFileInfo downloadFile;
    u32 downloadPercent;
    u32 downloadReceived;
    u32 downloadSize;
    char downloadAttr[0xc];
    u32 downloading;
    u32 unk1460;
    u32 unk1464;
    WifiBattleMatchNetData *p_data;
    WifiBattleMatchNetNameCheck nameCheck;
    PlayerInfo *namePlayerInfo;
    u32 unk1498;
    u32 unk149c;
    JoinAvenuePerson person;
    u32 unk1564;
};

static void WifiBattleMatchNet_GetLoginDate(u8 *dest);
static void WifiBattleMatchNet_SetMatchKey(WifiBattleMatchNet *p_wk, u32 index, s32 value);
static int WifiBattleMatchNet_EvalAlias(int index, void *work);
static int WifiBattleMatchNet_EvalAny(int index, void *work);
static int WifiBattleMatchNet_Eval(int index, void *work);
static void WifiBattleMatchNet_OnRequestDone(int error, void *work);
static void WifiBattleMatchNet_OnRequestDoneWithData(int error, void *work);
static void WifiBattleMatchNet_OnRequestDone2(int error, void *work);
static void WifiBattleMatchNet_EndRequests(WifiBattleMatchNet *p_wk);
static int WifiBattleMatchNet_SendBattleResult(WifiBattleMatchNetInfo *info, WifiBattleMatchNetInfo *theirs,
                                               WifiBattleMatchNetParam *param, int a3,
                                               WifiBattleMatchNetResultFunc func, int a5, int mode, int a7);
static int WifiBattleMatchNet_SetBattleResult(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param, int isMine,
                                              int a3);
static int WifiBattleMatchNet_SetCupBattleResult(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param,
                                                 int isMine, int a3);
static int WifiBattleMatchNet_SetFlag1A(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param, int isMine,
                                        int a3);
static int WifiBattleMatchNet_SetFlag19(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param, int isMine,
                                        int a3);
static int WifiBattleMatchNet_SetNoRating1B(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param, int isMine,
                                            int a3);
static int WifiBattleMatchNet_SetNoRating1C(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param, int isMine,
                                            int a3);
static void WifiBattleMatchNet_OnInfoReceived(int netId, int size, const void *data, void *work, NetHandle *handle);
static void WifiBattleMatchNet_OnPlayerInfoReceived(int netId, int size, const void *data, void *work,
                                                    NetHandle *handle);
static void *WifiBattleMatchNet_GetRecvBuffer(int netId, void *work, int size);
static void WifiBattleMatchNet_OnCmd2402(int netId, int size, const void *data, void *work, NetHandle *handle);
static void WifiBattleMatchNet_OnPartyReceived(int netId, int size, const void *data, void *work, NetHandle *handle);
static void WifiBattleMatchNet_OnCmd2404(int netId, int size, const void *data, void *work, NetHandle *handle);
static void WifiBattleMatchNet_OnValueReceived(int netId, int size, const void *data, void *work, NetHandle *handle);
static void WifiBattleMatchNet_OnPersonReceived(int netId, int size, const void *data, void *work, NetHandle *handle);
static void WifiBattleMatchNet_EndGdb(WifiBattleMatchNet *p_wk);
static void WifiBattleMatchNet_BuildProfileRecord(WifiBattleMatchNet *p_wk);
static void WifiBattleMatchNet_OnRecords(int recordNum, GdbField **records, void *work, int fieldNum);
static void WifiBattleMatchNet_ReadArena(int recordNum, GdbField **records, void *work, int fieldNum);
static void WifiBattleMatchNet_ReadWifiCup(int recordNum, GdbField **records, void *work, int fieldNum);
static void WifiBattleMatchNet_ReadRecordId(int recordNum, GdbField **records, void *work, int fieldNum);
static void WifiBattleMatchNet_ReadWifiCupParty(int recordNum, GdbField **records, void *work, int fieldNum);
static void WifiBattleMatchNet_ReadLastLogin(int recordNum, GdbField **records, void *work, int fieldNum);
static BOOL WifiBattleMatchNet_IsAllZero(const u32 *data, u32 size);
static void WifiBattleMatchNet_OnDwcError(void *work, int code, int type, int error);
static u32 WifiBattleMatchNet_OnDwcEvent(void *work, int a1, int event, int a3);
static u32 WifiBattleMatchNet_GetHttpResultMessage(u32 result);
static void WifiBattleMatchNet_StartDownloadWait(WifiBattleMatchNet *p_wk, u32 nextState);
static void WifiBattleMatchNet_StartDownloadEnd(WifiBattleMatchNet *p_wk, u32 nextState);
static void WifiBattleMatchNet_StartDownloadCancel(WifiBattleMatchNet *p_wk, u32 nextState);
static void WifiBattleMatchNet_OnDownload(u32 reason, u32 error, int serverError);
static u32 WifiBattleMatchNet_GetDwcResult(u32 code);
static u32 WifiBattleMatchNet_GetGdbResult(u32 code, u32 subCode1, u32 subCode2);
static u32 WifiBattleMatchNet_GetTimeoutResult(u32 code);
static void WifiBattleMatchNet_ClearError(WifiBattleMatchNetError *error);
static void WifiBattleMatchNet_SetTimeoutError(WifiBattleMatchNetError *error, int code, int line);
static void WifiBattleMatchNet_SetDwcError(WifiBattleMatchNetError *error, int code, int line);
static void WifiBattleMatchNet_SetGdbError(WifiBattleMatchNetError *error, int code, int line);
static void WifiBattleMatchNet_SetGdbSubError1(WifiBattleMatchNetError *error, int code, int line);
static void WifiBattleMatchNet_SetGdbSubError2(WifiBattleMatchNetError *error, int code, int line);
static void WifiBattleMatchNet_SetHttpError(WifiBattleMatchNetError *error, int code, int line);

// The commands that the two players send each other
static const u32 sCmd2402Data = 0;

static const NetCommand sNetCommands[] = {
    { WifiBattleMatchNet_OnInfoReceived, NULL },
    { WifiBattleMatchNet_OnPlayerInfoReceived, WifiBattleMatchNet_GetRecvBuffer },
    { WifiBattleMatchNet_OnCmd2402, NULL },
    { WifiBattleMatchNet_OnPartyReceived, WifiBattleMatchNet_GetRecvBuffer },
    { WifiBattleMatchNet_OnCmd2404, WifiBattleMatchNet_GetRecvBuffer },
    { WifiBattleMatchNet_OnValueReceived, NULL },
    { WifiBattleMatchNet_OnPersonReceived, WifiBattleMatchNet_GetRecvBuffer },
};

static char sVer[] = "ver";
static char sDis[] = "dis";
static const char *sWifiCupPartyTable = "WIFICUP_POKEMON_PARTY";
static const char *sLastLoginTable = "LAST_LOGIN_DATETIME";
static const char *sRecordIdTable = "recordid";
static const char *sRecordDataTables[6] = {
    "RECORD_DATA_01", "RECORD_DATA_02", "RECORD_DATA_03", "RECORD_DATA_04", "RECORD_DATA_05", "RECORD_DATA_06",
};
// The names of the matching keys, by the number the work gives them
static const char *sMatchKeyNames[8] = { "mod", "rul", "deb", "rat", "cup", sDis, "cnt", sVer };

static WifiBattleMatchNetDownload sDownload;

WifiBattleMatchNet *WifiBattleMatchNet_Create(void *a0, GameData *gameData, void *loginBuffer, HeapID heapId) {
    WifiBattleMatchNet *p_wk;

    p_wk = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchNet), FALSE, "wifibattlematch_net.c", 700);
    sys_memset(p_wk, 0, sizeof(WifiBattleMatchNet));
    p_wk->wifiList = GameData_GetWifiList(gameData);
    p_wk->unk0 = func_02009f7c(p_wk->wifiList);
    p_wk->profileId = func_02008bdc(GetGameDataPlayerInfo(gameData));
    p_wk->gameData = gameData;
    p_wk->loginBuffer = loginBuffer;
    p_wk->p_data = a0;
    p_wk->unk1a0 = 0x573;
    p_wk->record =
        GFL_HeapAllocate(heapId, sizeof(GdbField) * WBM_GDB_FIELD_TABLE_MAX, FALSE, "wifibattlematch_net.c", 716);
    sys_memset(p_wk->record, 0, sizeof(GdbField) * WBM_GDB_FIELD_TABLE_MAX);
    if (!func_02042788()) {
        GFL_ASSERT(0);
    }
    func_02040c20(0x2400, sNetCommands, 7, p_wk);
    func_02042d8c(0);
    DWCRap_SetErrorFunc(WifiBattleMatchNet_OnDwcError, p_wk);
    DWCRap_SetEventFunc(WifiBattleMatchNet_OnDwcEvent, p_wk);
    return p_wk;
}

void WifiBattleMatchNet_Delete(WifiBattleMatchNet *p_wk) {
    DWCRap_SetEventFunc(NULL, NULL);
    DWCRap_SetErrorFunc(NULL, NULL);
    func_02040c64(0x2400);
    GFL_HeapFree(p_wk->record);
    GFL_HeapFree(p_wk);
}

void WifiBattleMatchNet_Nop(WifiBattleMatchNet *p_wk) {
}

void WifiBattleMatchNet_StartSync(WifiBattleMatchNet *p_wk, u32 syncId) {
    p_wk->syncId = syncId;
    func_02040624(func_02040440(), (u8)p_wk->syncId, 0x24);
}

BOOL WifiBattleMatchNet_CheckSync(WifiBattleMatchNet *p_wk) {
    return func_02040664(func_02040440(), (u8)p_wk->syncId, 0x24);
}

u32 WifiBattleMatchNet_CheckError(WifiBattleMatchNet *p_wk, u32 a1, u32 a2) {
    WifiBattleMatchNetError *error = &p_wk->error;
    u32 result = 0;

    if (func_02042788()) {
        if (GFL_NetErrCheck()) {
            func_02042e94(0);
            func_02042e9c(0);
            GFL_SndStop();
            if (GameData_CheckEventsPaused(p_wk->gameData)) {
                func_02017884(p_wk->gameData);
                func_02011d04(0x29);
            }
        }
        switch (DWCRapCommon_CheckError(a1, a2)) {
        case 0:
            result = WBM_RESULT_OK;
            break;
        case 1:
            result = WBM_RESULT_DISCONNECTED;
            break;
        case 2:
            result = WBM_RESULT_FAILED;
            break;
        case 3:
            result = WBM_RESULT_NET_ERROR;
            break;
        case 4:
            result = 4;
            break;
        }
        return result;
    }
    if (result == 0) {
        switch (error->kind) {
        case WBM_ERROR_NONE:
            break;
        case WBM_ERROR_DWC:
            result = WifiBattleMatchNet_GetDwcResult(error->dwcCode);
            break;
        case WBM_ERROR_GDB:
            result = WifiBattleMatchNet_GetGdbResult(error->gdbCode, error->gdbSubCode1, error->gdbSubCode2);
            break;
        case WBM_ERROR_UNUSED_3:
        case WBM_ERROR_HTTP:
            break;
        case WBM_ERROR_TIMEOUT:
            result = WifiBattleMatchNet_GetTimeoutResult(error->timeoutCode);
            break;
        default:
            result = 0;
            break;
        }
        if (result != 0) {
            func_02042e94(0);
            func_02042e9c(0);
            GFL_SndStop();
            if (GameData_CheckEventsPaused(p_wk->gameData)) {
                func_02017884(p_wk->gameData);
                func_02011d04(0x29);
            }
        }
        switch (result) {
        case WBM_RESULT_DISCONNECTED:
            func_02042478();
            func_020424e4();
            func_02011de0();
            func_02012144();
            break;
        case WBM_RESULT_FAILED:
            if (!GFL_NetErrCheck()) {
                func_020424ac(0, 0, 0, 0x3f7);
                func_02012050();
            }
            GFL_NetErrMarkShown();
            break;
        case WBM_RESULT_NET_ERROR:
            GFL_NetErrAbort();
            break;
        }
        if (result != 0) {
            WifiBattleMatchNet_ClearError(error);
        }
    }
    return result;
}

void WifiBattleMatchNet_ResetLogin(WifiBattleMatchNet *p_wk) {
    p_wk->state = 0;
    p_wk->phase = 0;
}

u32 WifiBattleMatchNet_LoginMain(WifiBattleMatchNet *p_wk) {
    switch (p_wk->phase) {
    case 0:
        WifiBattleMatchNet_StartRead(p_wk, -1, WBM_READ_RECORD_ID, p_wk->p_data);
        p_wk->phase = 1;
        break;
    case 1: {
        u32 result = WifiBattleMatchNet_ReadMain(p_wk);

        if (result == 1) {
            if (p_wk->p_data->sake_recordID != 0) {
                p_wk->phase = 4;
            } else {
                p_wk->phase = 2;
            }
        } else if (result == 2) {
            return 2;
        }
        break;
    }
    case 2:
        WifiBattleMatchNet_StartCreateRecord(p_wk);
        p_wk->phase = 3;
        break;
    case 3: {
        u32 result = WifiBattleMatchNet_CreateRecordMain(p_wk);

        if (result == 1) {
            p_wk->phase = 4;
        } else if (result == 2) {
            return 2;
        }
        break;
    }
    case 4:
        return 1;
    }
    return 0;
}

void WifiBattleMatchNet_StartMatch(WifiBattleMatchNet *p_wk, u32 mode, u32 a2, u32 a3, const WifiBattleMatchNetKeys *keys) {
    u32 modValue = mode;

    if (mode == 1) {
        modValue += a2;
    }
    if (!func_02042788()) {
        GFL_NetErrAbort();
    }
    func_ov011_02160170();
    WifiBattleMatchNet_SetMatchKey(p_wk, 0, modValue);
    WifiBattleMatchNet_SetMatchKey(p_wk, 1, a3);
    WifiBattleMatchNet_SetMatchKey(p_wk, 2, 1);
    WifiBattleMatchNet_SetMatchKey(p_wk, 3, keys->rating);
    WifiBattleMatchNet_SetMatchKey(p_wk, 5, keys->disconnects);
    WifiBattleMatchNet_SetMatchKey(p_wk, 4, keys->cup);
    WifiBattleMatchNet_SetMatchKey(p_wk, 6, keys->matches);
    WifiBattleMatchNet_SetMatchKey(p_wk, 7, WBM_MATCH_VERSION);
    func_020800e8(p_wk->matchQuery, "mod=%d And rul=%d And deb=%d", modValue, a3, 1);
    p_wk->matchState = 0;
    p_wk->timer = 0;
    p_wk->matchTimer = 0;
    sys_memset(&p_wk->flagInfoReceived, 0, 7 * sizeof(u32));
    switch (mode) {
    case 0:
        p_wk->evalFunc = WifiBattleMatchNet_Eval;
        break;
    case 1:
        GFL_ASSERT(0);
        break;
    case 2:
        p_wk->evalFunc = WifiBattleMatchNet_EvalAlias;
        break;
    case 3:
        p_wk->evalFunc = WifiBattleMatchNet_EvalAny;
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
}

u32 WifiBattleMatchNet_MatchMain(WifiBattleMatchNet *p_wk) {
    switch (p_wk->matchState) {
    case 0:
        if (DWCRap_StartMatchQuery(p_wk->matchQuery, 2, p_wk->evalFunc, p_wk)) {
            func_0204200c();
            func_ov011_02151e40(0);
            p_wk->matchState = 1;
        } else {
            func_020421ac(TRUE);
            return 2;
        }
        break;
    case 1:
        p_wk->matchState = 2;
        break;
    case 2:
        switch (func_020420b4()) {
        case 0: {
            GFLNetErrorInfo *info = func_02042540();

            if ((u32)(info->type - 0x3f0) <= 1) {
                WifiBattleMatchNet_ResetEnd(p_wk);
                p_wk->matchState = 7;
            }
            break;
        }
        case 1:
            p_wk->matchTimer = 0;
            p_wk->matchState = 3;
            func_02042e94(1);
            func_02042e9c(1);
            break;
        default:
            WifiBattleMatchNet_ResetEnd(p_wk);
            p_wk->matchState = 7;
            break;
        }
        break;
    case 3:
        if (func_0203ffc4()) {
            if (func_02040504()) {
                p_wk->matchState = 4;
            }
            if (p_wk->matchTimer++ > 3600) {
                WifiBattleMatchNet_ResetEnd(p_wk);
                p_wk->matchState = 7;
            }
        } else {
            p_wk->matchTimer = 0;
            p_wk->matchState = 5;
        }
        break;
    case 4:
        func_02040624(func_02040440(), 0xf, 0x24);
        p_wk->matchTimer = 0;
        p_wk->matchState = 6;
        break;
    case 5:
        if (p_wk->matchTimer++ > 3600) {
            WifiBattleMatchNet_ResetEnd(p_wk);
            p_wk->matchState = 7;
        } else if (func_02040474() && func_02040504()) {
            func_02040624(func_02040440(), 0xf, 0x24);
            p_wk->matchTimer = 0;
            p_wk->matchState = 6;
        }
        break;
    case 6:
        if (p_wk->matchTimer++ > 3600) {
            WifiBattleMatchNet_ResetEnd(p_wk);
            p_wk->matchState = 7;
        } else if (func_02040664(func_02040440(), 0xf, 0x24)) {
            p_wk->matchState = 8;
        }
        break;
    case 7:
        if (WifiBattleMatchNet_EndMain(p_wk)) {
            GFLNetErrorInfo *info = func_02042540();

            if ((u32)(info->type - 0x3f0) <= 1) {
                func_020424e4();
                func_02042478();
                func_02012144();
                func_020421ac(TRUE);
            }
            return 2;
        }
        break;
    case 8:
        func_02042bc4();
        return 1;
    }
    return 0;
}

void WifiBattleMatchNet_CancelMatch(WifiBattleMatchNet *p_wk) {
    func_02042e94(0);
    func_02042e9c(0);
    func_020421ac(TRUE);
}

void WifiBattleMatchNet_ResetEnd(WifiBattleMatchNet *p_wk) {
    p_wk->phaseB = 0;
}

BOOL WifiBattleMatchNet_EndMain(WifiBattleMatchNet *p_wk) {
    switch (p_wk->phaseB) {
    case 0:
        WifiBattleMatchNet_StartSync(p_wk, 0x18);
        p_wk->timer = 0;
        p_wk->phaseB++;
        break;
    case 1:
        if (WifiBattleMatchNet_CheckSync(p_wk)) {
            p_wk->phaseB++;
        } else if (p_wk->timer++ > 600) {
            p_wk->phaseB++;
        }
        break;
    case 2:
        func_02042e94(0);
        func_02042e9c(0);
        p_wk->counter = 0;
        p_wk->phaseB++;
        break;
    case 3:
        if (p_wk->counter++ > 30) {
            p_wk->phaseB++;
        }
        break;
    case 4:
        func_020421ac(TRUE);
        p_wk->phaseB++;
        break;
    case 5:
        return TRUE;
    }
    return FALSE;
}

BOOL WifiBattleMatchNet_SendCmd2402(WifiBattleMatchNet *p_wk) {
    if (DWCRap_IsConnected()) {
        return func_02042be8(func_02040440(), 0x2402, 4, &sCmd2402Data);
    }
    return TRUE;
}

BOOL WifiBattleMatchNet_TakeCmd2402(WifiBattleMatchNet *p_wk) {
    u32 flag = p_wk->flag140;

    if (flag != 0) {
        p_wk->flag140 = 0;
    }
    return flag;
}

BOOL WifiBattleMatchNet_SendPerson(WifiBattleMatchNet *p_wk) {
    BOOL toServer = func_02042a6c(func_02040440()) == 0;

    func_02036dc0(&p_wk->person, p_wk->gameData);
    return func_02042c18(func_02040440(), toServer, 0x2406, 0xc4, &p_wk->person, TRUE, FALSE, TRUE);
}

BOOL WifiBattleMatchNet_TakePerson(WifiBattleMatchNet *p_wk) {
    u32 flag = p_wk->flagPersonReceived;

    if (flag != 0) {
        p_wk->flagPersonReceived = 0;
    }
    return flag;
}

void WifiBattleMatchNet_SetGuest(WifiBattleMatchNet *p_wk, u32 value) {
    p_wk->unk1564 = value;
}

static void WifiBattleMatchNet_GetLoginDate(u8 *dest) {
    RTCDate date;
    RTCTime time;

    if (!func_02042788()) {
        GFL_NetErrAbort();
    }
    if (!func_ov011_0215dda8(&date, &time)) {
        GFL_NetErrAbort();
    }
    sys_memset(dest, 0, 6);
    dest[0] = date.year;
    dest[1] = date.month;
    dest[2] = date.day;
    dest[3] = time.hour;
    dest[4] = time.minute;
    dest[5] = time.second;
}

static void WifiBattleMatchNet_SetMatchKey(WifiBattleMatchNet *p_wk, u32 index, s32 value) {
    WifiBattleMatchNetKey *key;

    p_wk->matchKeys[index].value = value;
    key = &p_wk->matchKeys[index];
    sys_memcpy(sMatchKeyNames[index], key->name, 3);
    key->name[3] = '\0';
    key->keyId = func_ov011_02160e10(key->keyId, key->name, &key->value);
}

static int WifiBattleMatchNet_EvalAlias(int index, void *work) {
    return WifiBattleMatchNet_Eval(index, work);
}

static int WifiBattleMatchNet_EvalAny(int index, void *work) {
    return 1000;
}

static int WifiBattleMatchNet_Eval(int index, void *work) {
    WifiBattleMatchNet *p_wk = work;
    int disconnects;
    int rating;
    int matches;
    int cup;
    int ratio;
    int myRatio;
    int ratingGap;
    int ratioGap;
    int score;

    rating = func_ov011_02160f98(index, p_wk->matchKeys[3].name, -1);
    disconnects = func_ov011_02160f98(index, p_wk->matchKeys[5].name, -1);
    matches = func_ov011_02160f98(index, p_wk->matchKeys[6].name, -1);
    cup = func_ov011_02160f98(index, p_wk->matchKeys[4].name, -1);

    if (rating == -1 && disconnects == -1 && cup == -1 && matches == -1) {
        return 0;
    }
    if (cup == p_wk->matchKeys[4].value) {
        ratio = disconnects * 50 / (matches + 5);
        myRatio = p_wk->matchKeys[5].value * 50 / (p_wk->matchKeys[6].value + 5);
        ratingGap = MATH_ABS(rating - p_wk->matchKeys[3].value);
        ratioGap = MATH_ABS(ratio - myRatio);
        score = 1000 - ratingGap - ratioGap * 32;
        if (score < 0) {
            return 0;
        }
        return score;
    }
    return 0;
}

void WifiBattleMatchNet_StartReport(WifiBattleMatchNet *p_wk, u32 mode, u32 kind, u32 a3, const u32 *a4, u32 a5) {
    p_wk->unk1498 = 0;
    p_wk->state = 0;
    p_wk->unk1ac = 1;
    p_wk->flagValueReceived = 0;
    p_wk->flagInfoReceived = 0;
    p_wk->mode = mode;
    p_wk->flag164 = a5;
    p_wk->requestSent = 0;
    p_wk->flag16C = 0;
    p_wk->flag170 = 0;
    if (mode == 0) {
        switch (kind) {
        case 0:
            p_wk->resultFunc = WifiBattleMatchNet_SetFlag19;
            break;
        case 2:
            p_wk->resultFunc = WifiBattleMatchNet_SetFlag1A;
            break;
        default:
            GFL_ASSERT(0);
            break;
        }
    } else if (mode == 1) {
        if (a3 != 0) {
            switch (kind) {
            case 0:
                p_wk->resultFunc = WifiBattleMatchNet_SetNoRating1C;
                break;
            case 2:
                p_wk->resultFunc = WifiBattleMatchNet_SetNoRating1B;
                break;
            default:
                GFL_ASSERT(0);
                break;
            }
        } else {
            func_02042bc4();
            p_wk->param.format = a3;
            p_wk->param.outcome = a4[0];
            p_wk->param.mine = a4[4];
            switch (kind) {
            case 0:
                p_wk->resultFunc = WifiBattleMatchNet_SetCupBattleResult;
                break;
            case 2:
                p_wk->resultFunc = WifiBattleMatchNet_SetBattleResult;
                break;
            default:
                GFL_ASSERT(0);
                break;
            }
        }
    }
}

u32 WifiBattleMatchNet_ReportMain(WifiBattleMatchNet *p_wk, u32 *out) {
    GFLNetErrorInfo *netError;
    int err;

    if (out != NULL) {
        *out = p_wk->flag170;
    }
    netError = func_02042540();
    if ((u32)(netError->type - 0x3f0) <= 1) {
        p_wk->flag164 = TRUE;
    }
    if (GFL_NetErrCheck() == 1) {
        if (netError->unk4 == 6 || netError->unk4 == 3) {
            WifiBattleMatchNet_EndRequests(p_wk);
            return 2;
        }
    } else if (p_wk->flag164 == 0 && GFL_NetErrCheck() == 2) {
        WifiBattleMatchNet_EndRequests(p_wk);
        return 2;
    }
    if (p_wk->flag164 != 0 && p_wk->mode != 0 && p_wk->requestSent == 0 && p_wk->state > 7 && p_wk->state < 10) {
        p_wk->flag16C = 0;
        p_wk->state = 6;
    }
    switch (p_wk->state) {
    case 0:
        if (p_wk->flag164 != 0) {
            p_wk->state = 4;
        } else {
            func_02040624(func_02040440(), 4, 0x24);
            p_wk->state = 1;
        }
        break;
    case 1:
        if (p_wk->flag164 != 0) {
            p_wk->state = 4;
        } else if (func_02040664(func_02040440(), 4, 0x24) || p_wk->flag164 != 0) {
            p_wk->state = 2;
        }
        break;
    case 2:
        if (p_wk->flag164 != 0) {
            p_wk->state = 4;
        } else if (WifiBattleMatchNet_SendReportValue(p_wk, &p_wk->param.mine) || p_wk->flag164 != 0) {
            p_wk->state = 3;
        }
        break;
    case 3:
        if (p_wk->flag164 != 0) {
            p_wk->state = 4;
        } else if (WifiBattleMatchNet_TakeReportValue(p_wk, &p_wk->param.theirs) || p_wk->flag164 != 0) {
            p_wk->state = 4;
        }
        break;
    case 4:
        p_wk->counter = 0;
        p_wk->gdbOpen = TRUE;
        err = Sc_Startup(0xb5f, 0);
        if (err != 0) {
            WifiBattleMatchNet_SetDwcError(&p_wk->error, err, 1937);
            WifiBattleMatchNet_EndRequests(p_wk);
            return 2;
        }
        p_wk->state = 5;
        break;
    case 5:
        err = Sc_GetCertificate(&p_wk->p_data->mine.certificate);
        if (err != 0) {
            WifiBattleMatchNet_SetDwcError(&p_wk->error, err, 1953);
            WifiBattleMatchNet_EndRequests(p_wk);
            return 2;
        }
        p_wk->state = 6;
        break;
    case 6:
        if (func_02042bc4() || p_wk->flag164 != 0) {
            p_wk->counter++;
            err = Sc_CreateSession(WifiBattleMatchNet_OnRequestDone, 100, p_wk);
            p_wk->requestSent = TRUE;
            if (err != 0) {
                WifiBattleMatchNet_SetDwcError(&p_wk->error, err, 1971);
                WifiBattleMatchNet_EndRequests(p_wk);
                return 2;
            }
            p_wk->timer = 0;
            p_wk->state = 7;
        } else {
            p_wk->state = 8;
        }
        break;
    case 7:
        if (p_wk->counter == 0) {
            sys_memcpy(Sc_GetSessionId(), p_wk->p_data->mine.unk8, 0x28);
            p_wk->state = 8;
        } else {
            if (p_wk->timer++ > 3600) {
                WifiBattleMatchNet_SetTimeoutError(&p_wk->error, 1, 1999);
                WifiBattleMatchNet_EndRequests(p_wk);
                return 2;
            }
            Sc_Update();
            if (p_wk->errorCode != 0) {
                WifiBattleMatchNet_SetDwcError(&p_wk->error, p_wk->errorCode, 2007);
                WifiBattleMatchNet_EndRequests(p_wk);
                return 2;
            }
        }
        break;
    case 8:
        if (p_wk->flag164 != 0) {
            p_wk->state = 11;
        } else {
            sys_memcpy(Sc_GetConnectionId(), p_wk->p_data->mine.unk30, 0x28);
            func_02040624(func_02040440(), 1, 0x24);
            p_wk->state = 9;
        }
        break;
    case 9:
        if (func_02040664(func_02040440(), 1, 0x24) || p_wk->flag164 != 0) {
            p_wk->state = 10;
        }
        break;
    case 10:
        if (func_02042bc4() || p_wk->flag164 != 0) {
            if (func_02042be8(func_02040440(), 0x2400, 0x68, &p_wk->p_data->mine) || p_wk->flag164 != 0) {
                p_wk->state = 11;
            }
        } else if (p_wk->flagInfoReceived != 0 || p_wk->flag164 != 0) {
            p_wk->flagInfoReceived = 0;
            err = Sc_SetSessionId(p_wk->p_data->theirs.unk8);
            sys_memcpy(Sc_GetSessionId(), p_wk->p_data->mine.unk8, 0x28);
            if (err != 0) {
                WifiBattleMatchNet_SetDwcError(&p_wk->error, err, 2061);
                WifiBattleMatchNet_EndRequests(p_wk);
                return 2;
            }
            p_wk->state = 11;
        }
        break;
    case 11:
        p_wk->counter++;
        err = Sc_SetReportIntention(p_wk->unk1ac, WifiBattleMatchNet_OnRequestDoneWithData, 100, p_wk);
        if (err != 0) {
            WifiBattleMatchNet_SetDwcError(&p_wk->error, err, 2077);
            WifiBattleMatchNet_EndRequests(p_wk);
            return 2;
        }
        p_wk->timer = 0;
        p_wk->state = 12;
        break;
    case 12:
        if (p_wk->counter == 0) {
            p_wk->state = 13;
        } else {
            if (p_wk->timer++ > 3600) {
                WifiBattleMatchNet_SetTimeoutError(&p_wk->error, 1, 2098);
                WifiBattleMatchNet_EndRequests(p_wk);
                return 2;
            }
            Sc_Update();
            if (p_wk->errorCode != 0) {
                WifiBattleMatchNet_SetDwcError(&p_wk->error, p_wk->errorCode, 2106);
                WifiBattleMatchNet_EndRequests(p_wk);
                return 2;
            }
        }
        break;
    case 13:
        if (p_wk->flag164 != 0) {
            p_wk->state = 19;
        } else {
            func_02040624(func_02040440(), 2, 0x24);
            p_wk->state = 14;
        }
        break;
    case 14:
        if (func_02040664(func_02040440(), 2, 0x24) || p_wk->flag164 != 0) {
            p_wk->state = 15;
        }
        break;
    case 15:
        if (func_02042bc4()) {
            if (func_02042be8(func_02040440(), 0x2400, 0x68, &p_wk->p_data->mine) || p_wk->flag164 != 0) {
                p_wk->state = 16;
            }
        } else if (p_wk->flagInfoReceived != 0 || p_wk->flag164 != 0) {
            if (p_wk->flag164 == 0) {
                p_wk->flag16C = TRUE;
            }
            p_wk->flagInfoReceived = 0;
            p_wk->state = 16;
        }
        break;
    case 16:
        if (!func_02042bc4()) {
            if (func_02042be8(func_02040440(), 0x2400, 0x68, &p_wk->p_data->mine) || p_wk->flag164 != 0) {
                p_wk->state = 17;
            }
        } else if (p_wk->flagInfoReceived != 0 || p_wk->flag164 != 0) {
            if (p_wk->flag164 == 0) {
                p_wk->flag16C = TRUE;
            }
            p_wk->flagInfoReceived = 0;
            p_wk->state = 17;
        }
        break;
    case 17:
        func_02040624(func_02040440(), 3, 0x24);
        p_wk->state = 18;
        break;
    case 18:
        if (func_02040664(func_02040440(), 3, 0x24) || p_wk->flag164 != 0) {
            p_wk->state = 19;
        }
        break;
    case 19:
        err =
            WifiBattleMatchNet_SendBattleResult(&p_wk->p_data->mine, &p_wk->p_data->theirs, &p_wk->param, p_wk->unk1ac,
                                                p_wk->resultFunc, p_wk->flag164, p_wk->mode, p_wk->flag16C);
        if (err != 0) {
            WifiBattleMatchNet_SetDwcError(&p_wk->error, err, 2221);
            WifiBattleMatchNet_EndRequests(p_wk);
            return 2;
        }
        p_wk->state = 20;
        break;
    case 20:
        p_wk->counter++;
        err = Sc_SubmitReport(p_wk->p_data->mine.connection, p_wk->unk1ac, WifiBattleMatchNet_OnRequestDone2, 100,
                                  p_wk);
        if (err != 0) {
            WifiBattleMatchNet_SetDwcError(&p_wk->error, err, 2236);
            WifiBattleMatchNet_EndRequests(p_wk);
            return 2;
        }
        p_wk->timer = 0;
        p_wk->state = 21;
        break;
    case 21:
        if (p_wk->counter == 0) {
            p_wk->state = 22;
        } else {
            if (p_wk->timer++ > 3600) {
                WifiBattleMatchNet_SetTimeoutError(&p_wk->error, 1, 2256);
                WifiBattleMatchNet_EndRequests(p_wk);
                return 2;
            }
            Sc_Update();
            if (p_wk->errorCode != 0) {
                WifiBattleMatchNet_SetDwcError(&p_wk->error, p_wk->errorCode, 2264);
                WifiBattleMatchNet_EndRequests(p_wk);
                return 2;
            }
        }
        break;
    case 22:
        p_wk->flag170 = TRUE;
        WifiBattleMatchNet_EndRequests(p_wk);
        p_wk->state = 23;
        break;
    case 23:
        func_02040624(func_02040440(), 5, 0x24);
        p_wk->state = 24;
        break;
    case 24:
        if (func_02040664(func_02040440(), 5, 0x24) || p_wk->flag164 != 0) {
            p_wk->state = 25;
        }
        break;
    case 25:
        return 1;
    }
    return 0;
}

u32 WifiBattleMatchNet_GetReceivedValue(WifiBattleMatchNet *p_wk) {
    return p_wk->receivedValue;
}

static void WifiBattleMatchNet_OnRequestDone(int error, void *work) {
    WifiBattleMatchNet *p_wk = work;

    if (error != 0) {
        p_wk->errorCode = error;
    }
    p_wk->counter--;
}

static void WifiBattleMatchNet_OnRequestDoneWithData(int error, void *work) {
    WifiBattleMatchNet *p_wk = work;
    WifiBattleMatchNetData *p_data = p_wk->p_data;

    if (error == 0) {
        sys_memcpy(Sc_GetConnectionId(), p_data->mine.unk30, 0x28);
    } else {
        p_wk->errorCode = error;
    }
    p_wk->counter--;
}

static void WifiBattleMatchNet_OnRequestDone2(int error, void *work) {
    WifiBattleMatchNet *p_wk = work;

    if (error != 0) {
        p_wk->errorCode = error;
    }
    p_wk->counter--;
}

static void WifiBattleMatchNet_EndRequests(WifiBattleMatchNet *p_wk) {
    if (p_wk->gdbOpen == 1) {
        if (p_wk->state > 0x13) {
            Sc_FreeReport(p_wk->p_data->mine.connection);
        }
        if (p_wk->state >= 4) {
            Sc_Shutdown();
        }
        p_wk->gdbOpen = 0;
    }
}

static int WifiBattleMatchNet_SendBattleResult(WifiBattleMatchNetInfo *info, WifiBattleMatchNetInfo *theirs,
                                               WifiBattleMatchNetParam *param, int a3,
                                               WifiBattleMatchNetResultFunc func, int a5, int mode, int a7) {
    int err;
    int resultMode;

    err = Sc_CreateReport(1, 2, 0, &info->connection);
    if (err != 0) {
        return err;
    }
    err = Sc_BeginGame(info->connection);
    if (err != 0) {
        return err;
    }
    err = Sc_BeginPlayers(info->connection);
    if (err != 0) {
        return err;
    }
    err = Sc_BeginGroup(info->connection);
    if (err != 0) {
        return err;
    }
    if (a5 != 0 || mode == 0) {
        resultMode = 5;
    } else {
        switch (param->outcome) {
        case 1:
        case 4:
            resultMode = 0;
            break;
        case 0:
        case 3:
            resultMode = 1;
            break;
        case 2:
            resultMode = 2;
            break;
        default:
            GFL_ASSERT_MSG(0, "\x83\x6f\x83\x67\x83\x8b\x8c\x8b\x89\xca\x95\x73\x90\xb3\x92\x6c %d\n");
            break;
        }
    }
    err = Sc_SetPlayer(info->connection, 0, info->unk30, 0, resultMode, info->certificate, info->unk58);
    if (err != 0) {
        return err;
    }
    err = func(info, param, 1, a5);
    if (err != 0) {
        return err;
    }
    err = Sc_BeginGroup(info->connection);
    if (err != 0) {
        return err;
    }
    if (a7 == 1) {
        int theirsMode;

        if (mode == 0) {
            theirsMode = 5;
        } else if (a5 != 0) {
            theirsMode = 3;
        } else {
            switch (param->outcome) {
            case 1:
            case 4:
                theirsMode = 1;
                break;
            case 0:
            case 3:
                theirsMode = 0;
                break;
            case 2:
                theirsMode = 2;
                break;
            default:
                GFL_ASSERT_MSG(0, "\x83\x6f\x83\x67\x83\x8b\x8c\x8b\x89\xca\x95\x73\x90\xb3\x92\x6c %d\n");
                break;
            }
        }
        err = Sc_SetPlayer(info->connection, 1, theirs->unk30, 0, theirsMode, theirs->certificate, theirs->unk58);
        if (err != 0) {
            return err;
        }
        err = func(info, param, 0, a5);
        if (err != 0) {
            return err;
        }
    }
    err = Sc_BeginTeams(info->connection);
    if (err != 0) {
        return err;
    }
    if (a5 != 0) {
        return Sc_FinishReport(info->connection, a3, 1);
    }
    return Sc_FinishReport(info->connection, a3, 0);
}

static int WifiBattleMatchNet_SetBattleResult(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param, int isMine,
                                              int a3) {
    int win;
    int lose;
    int skip;
    int draw;
    int decided;
    int err;

    decided = 0;
    skip = 0;
    draw = 1;
    if (param->mine == 0 && param->theirs == 0) {
        draw = 0;
    }
    switch (param->outcome) {
    case 0:
    case 3:
        win = 0;
        lose = 1;
        decided = 1;
        break;
    case 1:
    case 4:
        win = 1;
        lose = 0;
        decided = 1;
        break;
    case 2:
        win = 0;
        lose = 0;
        skip = 1;
        break;
    case 5:
    case 6:
    default:
        win = 0;
        lose = 0;
        break;
    }
    if (isMine == 0 && decided) {
        win ^= 1;
        lose ^= 1;
    }
    switch (param->format) {
    case 0:
        if (skip == 0) {
            err = Sc_AddByte(info->connection, 0xf, 1);
            if (err != 0) {
                return err;
            }
        }
        err = Sc_AddInt(info->connection, 5, win);
        if (err != 0) {
            return err;
        }
        err = Sc_AddInt(info->connection, 6, lose);
        if (err != 0) {
            return err;
        }
        break;
    case 1:
        if (skip == 0) {
            err = Sc_AddByte(info->connection, 0x10, 1);
            if (err != 0) {
                return err;
            }
        }
        err = Sc_AddInt(info->connection, 8, win);
        if (err != 0) {
            return err;
        }
        err = Sc_AddInt(info->connection, 7, lose);
        if (err != 0) {
            return err;
        }
        break;
    case 2:
        if (skip == 0) {
            err = Sc_AddByte(info->connection, 0x12, 1);
            if (err != 0) {
                return err;
            }
        }
        err = Sc_AddInt(info->connection, 0xb, win);
        if (err != 0) {
            return err;
        }
        err = Sc_AddInt(info->connection, 0xc, lose);
        if (err != 0) {
            return err;
        }
        break;
    case 3:
        if (skip == 0) {
            err = Sc_AddByte(info->connection, 0x11, 1);
            if (err != 0) {
                return err;
            }
        }
        err = Sc_AddInt(info->connection, 9, win);
        if (err != 0) {
            return err;
        }
        err = Sc_AddInt(info->connection, 0xa, lose);
        if (err != 0) {
            return err;
        }
        break;
    case 4:
        if (skip == 0) {
            err = Sc_AddByte(info->connection, 0x13, 1);
            if (err != 0) {
                return err;
            }
        }
        err = Sc_AddInt(info->connection, 0xd, win);
        if (err != 0) {
            return err;
        }
        err = Sc_AddInt(info->connection, 0xe, lose);
        if (err != 0) {
            return err;
        }
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
    err = Sc_AddInt(info->connection, 0x14, draw != 0);
    if (err != 0) {
        return err;
    }
    return Sc_AddInt(info->connection, 0x1b, -1);
}

static int WifiBattleMatchNet_SetCupBattleResult(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param,
                                                 int isMine, int a3) {
    int win;
    int lose;
    int draw;
    int decided;
    int skip;
    int err;

    decided = 0;
    skip = 0;
    draw = 1;
    if (param->mine == 0 && param->theirs == 0) {
        draw = 0;
    }
    switch (param->outcome) {
    case 0:
    case 3:
        win = 0;
        lose = 1;
        decided = 1;
        break;
    case 1:
    case 4:
        win = 1;
        lose = 0;
        decided = 1;
        break;
    case 2:
        win = 0;
        lose = 0;
        skip = 1;
        break;
    case 5:
    case 6:
    default:
        win = 0;
        lose = 0;
        break;
    }
    if (isMine == 0 && decided) {
        win ^= 1;
        lose ^= 1;
    }
    if (skip == 0) {
        err = Sc_AddByte(info->connection, 0x15, 1);
        if (err != 0) {
            return err;
        }
    }
    err = Sc_AddInt(info->connection, 0x16, win);
    if (err != 0) {
        return err;
    }
    err = Sc_AddInt(info->connection, 0x17, lose);
    if (err != 0) {
        return err;
    }
    err = Sc_AddInt(info->connection, 0x18, draw != 0);
    if (err != 0) {
        return err;
    }
    return Sc_AddInt(info->connection, 0x1c, -1);
}

static int WifiBattleMatchNet_SetFlag1A(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param, int isMine,
                                        int a3) {
    return Sc_AddInt(info->connection, 0x1a, 1);
}

static int WifiBattleMatchNet_SetFlag19(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param, int isMine,
                                        int a3) {
    return Sc_AddInt(info->connection, 0x19, 1);
}

static int WifiBattleMatchNet_SetNoRating1B(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param, int isMine,
                                            int a3) {
    return Sc_AddInt(info->connection, 0x1b, -1);
}

static int WifiBattleMatchNet_SetNoRating1C(WifiBattleMatchNetInfo *info, WifiBattleMatchNetParam *param, int isMine,
                                            int a3) {
    return Sc_AddInt(info->connection, 0x1c, -1);
}

static void WifiBattleMatchNet_OnInfoReceived(int netId, int size, const void *data, void *work, NetHandle *handle) {
    WifiBattleMatchNet *p_wk = work;

    if (handle == func_02040440() && netId != func_02042a6c(func_02040440())) {
        sys_memcpy(data, &p_wk->p_data->theirs, 0x68);
        p_wk->flagInfoReceived = 1;
    }
}

static void WifiBattleMatchNet_OnPlayerInfoReceived(int netId, int size, const void *data, void *work,
                                                    NetHandle *handle) {
    WifiBattleMatchNet *p_wk = work;
    PlayerInfo *info = func_02017378(p_wk->gameData, netId);

    sys_memcpy(data, info, PlayerInfo_GetSize());
    if (handle == func_02040440() && netId != func_02042a6c(func_02040440())) {
        p_wk->flagPlayerInfoReceived = 1;
    }
}

static void *WifiBattleMatchNet_GetRecvBuffer(int netId, void *work, int size) {
    WifiBattleMatchNet *p_wk = work;

    if (netId == func_02042a6c(func_02040440())) {
        GFL_ASSERT(0);
        return NULL;
    }
    return p_wk->recvBuffer;
}

static void WifiBattleMatchNet_OnCmd2402(int netId, int size, const void *data, void *work, NetHandle *handle) {
    WifiBattleMatchNet *p_wk = work;

    if (handle == func_02040440() && netId != func_02042a6c(func_02040440())) {
        p_wk->flag140 = 1;
    }
}

static void WifiBattleMatchNet_OnPartyReceived(int netId, int size, const void *data, void *work, NetHandle *handle) {
    WifiBattleMatchNet *p_wk = work;

    if (handle == func_02040440() && netId != func_02042a6c(func_02040440())) {
        p_wk->flagPartyReceived = 1;
    }
}

static void WifiBattleMatchNet_OnCmd2404(int netId, int size, const void *data, void *work, NetHandle *handle) {
    WifiBattleMatchNet *p_wk = work;

    if (handle == func_02040440() && netId != func_02042a6c(func_02040440())) {
        p_wk->flag148 = 1;
    }
}

static void WifiBattleMatchNet_OnValueReceived(int netId, int size, const void *data, void *work, NetHandle *handle) {
    WifiBattleMatchNet *p_wk = work;
    u32 value = *(const u32 *)data;

    if (handle == func_02040440() && netId != func_02042a6c(func_02040440())) {
        p_wk->receivedValue = value;
        p_wk->flagValueReceived = 1;
    }
}

static void WifiBattleMatchNet_OnPersonReceived(int netId, int size, const void *data, void *work, NetHandle *handle) {
    WifiBattleMatchNet *p_wk = work;
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(p_wk->gameData));

    if (handle == func_02040440() && netId != func_02042a6c(func_02040440())) {
        JoinAvenuePerson *person = (JoinAvenuePerson *)data;

        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_UNK_26, 1);
        if (p_wk->unk1564 != 0) {
            MsgData *msgData =
                GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN, p_wk->msgHeapId);
            StrBuf *name = GFL_MsgDataLoadStrbufNew(msgData, WBM_DEFAULT_NAME);

            GFL_MsgDataFree(msgData);
            JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_NAME, (u32)GFL_StrBufGetStringPtr(name));
            GFL_StrBufFree(name);
        }
        func_02010078(joinAvenue, p_wk->gameData, person, 1);
        p_wk->flagPersonReceived = 1;
    }
}

void WifiBattleMatchNet_StartRead(WifiBattleMatchNet *p_wk, int recordId, int kind, void *out) {
    p_wk->state = 0;
    p_wk->unk17c = out;
    p_wk->recordIdArg = recordId;
    switch (kind) {
    case WBM_READ_ARENA:
        p_wk->tableNames = WifiBattleMatchNet_GetArenaFieldNames();
        p_wk->table_name_num = WifiBattleMatchNet_GetArenaFieldCount();
        p_wk->recordsFunc = WifiBattleMatchNet_ReadArena;
        break;
    case WBM_READ_WIFICUP:
        p_wk->tableNames = WifiBattleMatchNet_GetWifiCupFieldNames();
        p_wk->table_name_num = WifiBattleMatchNet_GetWifiCupFieldCount();
        p_wk->recordsFunc = WifiBattleMatchNet_ReadWifiCup;
        break;
    case WBM_READ_RECORD_ID:
        p_wk->tableNames = &sRecordIdTable;
        p_wk->table_name_num = 1;
        p_wk->recordsFunc = WifiBattleMatchNet_ReadRecordId;
        break;
    case WBM_READ_WIFICUP_PARTY:
        p_wk->tableNames = &sWifiCupPartyTable;
        p_wk->table_name_num = 1;
        p_wk->recordsFunc = WifiBattleMatchNet_ReadWifiCupParty;
        break;
    case WBM_READ_LAST_LOGIN:
        p_wk->tableNames = &sLastLoginTable;
        p_wk->table_name_num = 1;
        p_wk->recordsFunc = WifiBattleMatchNet_ReadLastLogin;
        break;
    }
}

u32 WifiBattleMatchNet_ReadMain(WifiBattleMatchNet *p_wk) {
    int err;

    switch (p_wk->state) {
    case 0:
        err = Gdb_Startup(0xb5f, p_wk->unk0, 0);
        if (err != 0) {
            WifiBattleMatchNet_SetGdbError(&p_wk->error, err, 3674);
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        }
        p_wk->state = 1;
        break;
    case 1:
        GFL_ASSERT(p_wk->table_name_num < WBM_GDB_FIELD_TABLE_MAX);
        if (p_wk->recordIdArg == -1) {
            err = Gdb_GetMyRecords(WBM_GDB_TABLE_NAME, p_wk->tableNames, p_wk->table_name_num,
                                      WifiBattleMatchNet_OnRecords, p_wk);
        } else {
            err = Gdb_GetSpecificRecords(WBM_GDB_TABLE_NAME, &p_wk->recordIdArg, 1, p_wk->tableNames, p_wk->table_name_num,
                                      WifiBattleMatchNet_OnRecords, p_wk);
        }
        if (err != 0) {
            WifiBattleMatchNet_SetGdbError(&p_wk->error, err, 3701);
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        }
        p_wk->state = 2;
        p_wk->timer = 0;
        break;
    case 2:
        if (p_wk->timer++ > 3600) {
            WifiBattleMatchNet_SetTimeoutError(&p_wk->error, 1, 3714);
            p_wk->state = 6;
        } else {
            Gdb_Update();
            err = Gdb_GetState();
            if (err == 0 || err == 4) {
                WifiBattleMatchNet_SetGdbSubError1(&p_wk->error, err, 3729);
                p_wk->state = 6;
            } else if (err == 1) {
                p_wk->state = 3;
            }
        }
        break;
    case 3:
        err = Gdb_GetResult();
        if (err == 0) {
            WifiBattleMatchNet_SetGdbSubError2(&p_wk->error, err, 3748);
            p_wk->state = 6;
        } else if (err != 1) {
            WifiBattleMatchNet_SetGdbSubError2(&p_wk->error, err, 3754);
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        } else {
            p_wk->state = 4;
        }
        break;
    case 4:
        WifiBattleMatchNet_EndGdb(p_wk);
        p_wk->state = 5;
        break;
    case 5:
        return 1;
    case 6:
        err = Gdb_Cancel();
        if (err == 0) {
            p_wk->timer = 0;
            p_wk->state = 7;
        } else {
            WifiBattleMatchNet_SetGdbError(&p_wk->error, err, 3785);
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        }
        break;
    case 7:
        if (p_wk->timer++ > 3600) {
            WifiBattleMatchNet_SetTimeoutError(&p_wk->error, 1, 3797);
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        }
        Gdb_Update();
        err = Gdb_GetState();
        if (err == 0 || err == 4) {
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        } else if (err == 1) {
            p_wk->state = 8;
        }
        break;
    case 8:
        WifiBattleMatchNet_EndGdb(p_wk);
        return 2;
    }
    return 0;
}

static void WifiBattleMatchNet_EndGdb(WifiBattleMatchNet *p_wk) {
    Gdb_Shutdown();
}

static void WifiBattleMatchNet_BuildProfileRecord(WifiBattleMatchNet *p_wk) {
    PlayerInfo *info = GetGameDataPlayerInfo(p_wk->gameData);

    p_wk->record[0].name = "MYSTATUS";
    p_wk->record[0].type = GDB_FIELD_BINARY;
    p_wk->record[0].value.data = info;
    p_wk->record[0].size = 0x20;
    WifiBattleMatchNet_GetLoginDate(p_wk->loginDate);
    p_wk->record[1].name = "LAST_LOGIN_DATETIME";
    p_wk->record[1].type = GDB_FIELD_BINARY;
    p_wk->record[1].value.data = p_wk->loginDate;
    p_wk->record[1].size = 6;
    p_wk->record[2].name = "INITIAL_PROFILE_ID";
    p_wk->record[2].type = GDB_FIELD_INT;
    p_wk->record[2].value.intValue = p_wk->profileId;
    p_wk->record[3].name = "NOW_PROFILE_ID";
    p_wk->record[3].type = GDB_FIELD_INT;
    p_wk->record[3].value.intValue = WifiBattleMatchNet_GetGSID(p_wk);
    p_wk->table_name_num = 4;
}

static void WifiBattleMatchNet_OnRecords(int recordNum, GdbField **records, void *work, int fieldNum) {
    WifiBattleMatchNet *p_wk = work;
    int i;
    int j;

    p_wk->recordsFunc(recordNum, records, p_wk->unk17c, p_wk->table_name_num);
    if (p_wk->recordIdArg == -1) {
        for (i = 0; i < recordNum; i++) {
            for (j = 0; j < p_wk->table_name_num; j++) {
                GdbField *field = &records[i][j];

                if (_STD_CompareNString(field->name, "recordid") == 0) {
                    p_wk->p_data->sake_recordID = field->value.intValue;
                }
            }
        }
    }
}

static void WifiBattleMatchNet_ReadArena(int recordNum, GdbField **records, void *work, int fieldNum) {
    WifiBattleMatchNetStats *stats = work;
    int i;
    int j;

    for (i = 0; i < recordNum; i++) {
        for (j = 0; j < fieldNum; j++) {
            GdbField *field = &records[i][j];

            if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_SINGLE_WIN))) {
                stats->numSingleWin = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_SINGLE_LOSE))) {
                stats->numSingleLose = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_DOUBLE_WIN))) {
                stats->numDoubleWin = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_DOUBLE_LOSE))) {
                stats->numDoubleLose = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_ROTATE_WIN))) {
                stats->numRotateWin = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_ROTATE_LOSE))) {
                stats->numRotateLose = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_TRIPLE_WIN))) {
                stats->numTripleWin = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_TRIPLE_LOSE))) {
                stats->numTripleLose = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_SHOOTER_WIN))) {
                stats->numShooterWin = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_SHOOTER_LOSE))) {
                stats->numShooterLose = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_ELO_RATING_1V1_SINGLE))) {
                stats->eloRatingSingle = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_ELO_RATING_1V1_DOUBLE))) {
                stats->eloRatingDouble = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_ELO_RATING_1V1_ROTATE))) {
                stats->eloRatingRotate = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_ELO_RATING_1V1_TRIPLE))) {
                stats->eloRatingTriple = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_ELO_RATING_1V1_SHOOTER))) {
                stats->eloRatingShooter = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_DISCONNECTS_COUNTER))) {
                stats->disconnectsCounter = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_CHEATS_COUNTER))) {
                stats->cheatsCounter = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, "INITIAL_PROFILE_ID")) {
                stats->initialProfileId = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, "NOW_PROFILE_ID")) {
                stats->nowProfileId = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, "RECORD_SAVE_IDX")) {
                stats->recordSaveIdx = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, "recordid")) {
                stats->recordId = field->value.intValue;
            }
        }
    }
}

static void WifiBattleMatchNet_ReadWifiCup(int recordNum, GdbField **records, void *work, int fieldNum) {
    WifiBattleMatchNetCupStats *stats = work;
    int i;
    int j;

    for (i = 0; i < recordNum; i++) {
        for (j = 0; j < fieldNum; j++) {
            GdbField *field = &records[i][j];

            if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_WIFICUP_WIN))) {
                stats->numWin = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_WIFICUP_LOSE))) {
                stats->numLose = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_ELO_RATING_1V1_WIFICUP))) {
                stats->eloRating = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_DISCONNECTS_WIFICUP_COUNTER))) {
                stats->disconnectsCounter = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, WifiBattleMatchNet_GetArenaFieldName(ARENA_CHEATS_WIFICUP_COUNTER))) {
                stats->cheatsCounter = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, "RECORD_SAVE_IDX")) {
                stats->recordSaveIdx = field->value.intValue;
            } else if (!_STD_CompareNString(field->name, "WIFICUP_POKEMON_PARTY")) {
                sys_memcpy(field->value.data, stats->party, field->size);
            }
        }
    }
}

static void WifiBattleMatchNet_ReadRecordId(int recordNum, GdbField **records, void *work, int fieldNum) {
    int i;
    int j;

    for (i = 0; i < recordNum; i++) {
        for (j = 0; j < fieldNum; j++) {
            GdbField *field = &records[i][j];

            if (_STD_CompareNString(field->name, "recordid") == 0) {
                *(s32 *)work = field->value.intValue;
            }
        }
    }
}

static void WifiBattleMatchNet_ReadWifiCupParty(int recordNum, GdbField **records, void *work, int fieldNum) {
    int i;
    int j;

    for (i = 0; i < recordNum; i++) {
        for (j = 0; j < fieldNum; j++) {
            GdbField *field = &records[i][j];

            if (_STD_CompareNString(field->name, "WIFICUP_POKEMON_PARTY") == 0) {
                sys_memcpy(field->value.data, work, field->size);
            }
        }
    }
}

static void WifiBattleMatchNet_ReadLastLogin(int recordNum, GdbField **records, void *work, int fieldNum) {
    int i;
    int j;

    for (i = 0; i < recordNum; i++) {
        for (j = 0; j < fieldNum; j++) {
            GdbField *field = &records[i][j];

            if (_STD_CompareNString(field->name, "LAST_LOGIN_DATETIME") == 0) {
                sys_memcpy(field->value.data, work, field->size);
            }
        }
    }
}

void WifiBattleMatchNet_StartWrite(WifiBattleMatchNet *p_wk, int kind, void *data) {
    p_wk->state = 0;
    GFL_ASSERT(p_wk->p_data->sake_recordID != 0);
    switch (kind) {
    case WBM_WRITE_WIFICUP:
        p_wk->record[0].name = WifiBattleMatchNet_GetArenaFieldName(ARENA_ELO_RATING_1V1_WIFICUP);
        p_wk->record[0].type = GDB_FIELD_INT;
        p_wk->record[0].value.intValue = 1500;
        p_wk->record[1].name = WifiBattleMatchNet_GetArenaFieldName(ARENA_CHEATS_WIFICUP_COUNTER);
        p_wk->record[1].type = GDB_FIELD_INT;
        p_wk->record[1].value.intValue = 0;
        p_wk->record[2].name = WifiBattleMatchNet_GetArenaFieldName(ARENA_DISCONNECTS_WIFICUP_COUNTER);
        p_wk->record[2].type = GDB_FIELD_INT;
        p_wk->record[2].value.intValue = 0;
        p_wk->record[3].name = WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_WIFICUP_LOSE);
        p_wk->record[3].type = GDB_FIELD_INT;
        p_wk->record[3].value.intValue = 0;
        p_wk->record[4].name = WifiBattleMatchNet_GetArenaFieldName(ARENA_NUM_WIFICUP_WIN);
        p_wk->record[4].type = GDB_FIELD_INT;
        p_wk->record[4].value.intValue = 0;
        p_wk->record[5].name = "WIFICUP_POKEMON_PARTY";
        p_wk->record[5].type = GDB_FIELD_BINARY;
        p_wk->record[5].value.data = data;
        p_wk->record[5].size = 1332;
        p_wk->table_name_num = 6;
        break;
    case WBM_WRITE_LAST_LOGIN:
        WifiBattleMatchNet_GetLoginDate(p_wk->loginDate);
        p_wk->table_name_num = 1;
        p_wk->record[0].name = "LAST_LOGIN_DATETIME";
        p_wk->record[0].type = GDB_FIELD_BINARY;
        p_wk->record[0].value.data = p_wk->loginDate;
        p_wk->record[0].size = 6;
        break;
    case WBM_WRITE_PROFILE:
        WifiBattleMatchNet_BuildProfileRecord(p_wk);
        break;
    case WBM_WRITE_RECORD_DATA:
        p_wk->recordData = *(WifiBattleMatchNetRecordData *)data;
        p_wk->table_name_num = 2;
        p_wk->record[0].name = sRecordDataTables[p_wk->recordData.slot];
        p_wk->record[0].type = GDB_FIELD_BINARY;
        p_wk->record[0].value.data = p_wk->recordData.data;
        p_wk->record[0].size = 0x50;
        p_wk->record[1].name = "RECORD_SAVE_IDX";
        p_wk->record[1].type = GDB_FIELD_BYTE;
        p_wk->record[1].value.byteValue = (p_wk->recordData.slot + 1) % 6;
        break;
    }
}

u32 WifiBattleMatchNet_WriteMain(WifiBattleMatchNet *p_wk) {
    int err;

    switch (p_wk->state) {
    case 0:
        err = Gdb_Startup(0xb5f, p_wk->unk0, 0);
        if (err != 0) {
            WifiBattleMatchNet_SetGdbError(&p_wk->error, err, 4556);
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        }
        p_wk->state = 1;
        break;
    case 1:
        GFL_ASSERT(p_wk->table_name_num < WBM_GDB_FIELD_TABLE_MAX);
        err = Gdb_UpdateRecord(WBM_GDB_TABLE_NAME, p_wk->p_data->sake_recordID, p_wk->record, p_wk->table_name_num);
        if (err != 0) {
            WifiBattleMatchNet_SetGdbError(&p_wk->error, err, 4569);
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        }
        p_wk->timer = 0;
        p_wk->state = 2;
        break;
    case 2:
        if (p_wk->timer++ > 3600) {
            WifiBattleMatchNet_SetTimeoutError(&p_wk->error, 1, 4582);
            p_wk->state = 6;
        } else {
            Gdb_Update();
            err = Gdb_GetState();
            if (err == 0 || err == 4) {
                WifiBattleMatchNet_SetGdbSubError1(&p_wk->error, err, 4593);
                p_wk->state = 6;
            } else if (err == 1) {
                p_wk->state = 3;
            }
        }
        break;
    case 3:
        err = Gdb_GetResult();
        if (err == 0) {
            WifiBattleMatchNet_SetGdbSubError2(&p_wk->error, err, 4612);
            p_wk->state = 6;
        } else if (err != 1) {
            WifiBattleMatchNet_EndGdb(p_wk);
            WifiBattleMatchNet_SetGdbSubError2(&p_wk->error, err, 4619);
            return 2;
        } else {
            p_wk->state = 4;
        }
        break;
    case 4:
        WifiBattleMatchNet_EndGdb(p_wk);
        p_wk->state = 5;
        // fall through
    case 5:
        return 1;
    case 6:
        err = Gdb_Cancel();
        if (err == 0) {
            p_wk->timer = 0;
            p_wk->state = 7;
        } else {
            WifiBattleMatchNet_SetGdbError(&p_wk->error, err, 4649);
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        }
        break;
    case 7:
        if (p_wk->timer++ > 3600) {
            WifiBattleMatchNet_SetTimeoutError(&p_wk->error, 1, 4661);
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        }
        Gdb_Update();
        err = Gdb_GetState();
        if (err == 0 || err == 4) {
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        } else if (err == 1) {
            p_wk->state = 8;
        }
        break;
    case 8:
        WifiBattleMatchNet_EndGdb(p_wk);
        return 2;
    }
    return 0;
}

void WifiBattleMatchNet_StartCreateRecord(WifiBattleMatchNet *p_wk) {
    p_wk->state = 0;
    WifiBattleMatchNet_BuildProfileRecord(p_wk);
}

u32 WifiBattleMatchNet_CreateRecordMain(WifiBattleMatchNet *p_wk) {
    int err;

    switch (p_wk->state) {
    case 0:
        err = Gdb_Startup(0xb5f, p_wk->unk0, 0);
        if (err != 0) {
            WifiBattleMatchNet_SetGdbError(&p_wk->error, err, 4736);
            return 2;
        }
        p_wk->state = 1;
        break;
    case 1:
        err = Gdb_CreateRecord(WBM_GDB_TABLE_NAME, p_wk->record, p_wk->table_name_num,
                                   &p_wk->p_data->sake_recordID);
        if (err != 0) {
            WifiBattleMatchNet_SetGdbError(&p_wk->error, err, 4750);
            return 2;
        }
        p_wk->timer = 0;
        p_wk->state = 2;
        break;
    case 2:
        if (p_wk->timer++ > 3600) {
            WifiBattleMatchNet_SetTimeoutError(&p_wk->error, 1, 4762);
            p_wk->state = 4;
        } else {
            Gdb_Update();
            err = Gdb_GetState();
            if (err == 0 || err == 4) {
                WifiBattleMatchNet_SetGdbSubError1(&p_wk->error, err, 4774);
                p_wk->state = 4;
            } else if (err == 1) {
                p_wk->state = 3;
            }
        }
        break;
    case 3:
        err = Gdb_GetResult();
        if (err == 0) {
            WifiBattleMatchNet_SetGdbSubError2(&p_wk->error, err, 4794);
            p_wk->state = 4;
        } else if (err != 1) {
            WifiBattleMatchNet_EndGdb(p_wk);
            WifiBattleMatchNet_SetGdbSubError2(&p_wk->error, err, 4801);
            return 2;
        } else {
            if (p_wk->p_data->sake_recordID == 0) {
                WifiBattleMatchNet_SetTimeoutError(&p_wk->error, 2, 4807);
            }
            WifiBattleMatchNet_EndGdb(p_wk);
            return 1;
        }
        break;
    case 4:
        err = Gdb_Cancel();
        if (err == 0) {
            p_wk->timer = 0;
            p_wk->state = 5;
        } else {
            WifiBattleMatchNet_SetGdbError(&p_wk->error, err, 4826);
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        }
        break;
    case 5:
        if (p_wk->timer++ > 3600) {
            WifiBattleMatchNet_SetTimeoutError(&p_wk->error, 1, 4838);
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        }
        Gdb_Update();
        err = Gdb_GetState();
        if (err == 0 || err == 4) {
            WifiBattleMatchNet_EndGdb(p_wk);
            return 2;
        } else if (err == 1) {
            p_wk->state = 6;
        }
        break;
    case 6:
        WifiBattleMatchNet_EndGdb(p_wk);
        return 2;
    }
    return 0;
}

u32 WifiBattleMatchNet_GetRecordId(WifiBattleMatchNet *p_wk) {
    return p_wk->p_data->sake_recordID;
}

BOOL WifiBattleMatchNet_SendPlayerInfo(WifiBattleMatchNet *p_wk, const void *data) {
    BOOL toServer = func_02042a6c(func_02040440()) == 0;
    NetHandle *handle = func_02040440();

    return func_02042c18(handle, toServer, 0x2401, PokeParty_GetSaveDataSize() + 0xd4, data, TRUE, TRUE, TRUE);
}

BOOL WifiBattleMatchNet_TakePlayerInfo(WifiBattleMatchNet *p_wk, void **out) {
    u32 flag = p_wk->flagPlayerInfoReceived;

    if (flag != 0) {
        p_wk->flagPlayerInfoReceived = 0;
        *out = p_wk->recvBuffer;
    }
    return flag;
}

BOOL WifiBattleMatchNet_SendParty(WifiBattleMatchNet *p_wk, PokeParty *party) {
    BOOL toServer = func_02042a6c(func_02040440()) == 0;
    NetHandle *handle = func_02040440();

    return func_02042c18(handle, toServer, 0x2403, PokeParty_GetSaveDataSize(), party, TRUE, TRUE, TRUE);
}

BOOL WifiBattleMatchNet_TakeParty(WifiBattleMatchNet *p_wk, PokeParty *party) {
    u32 flag = p_wk->flagPartyReceived;

    if (flag != 0) {
        p_wk->flagPartyReceived = 0;
        sys_memcpy(p_wk->recvBuffer, party, PokeParty_GetSaveDataSize());
    }
    return flag;
}

BOOL WifiBattleMatchNet_SendValue4(WifiBattleMatchNet *p_wk, const void *data) {
    BOOL toServer = func_02042a6c(func_02040440()) == 0;

    return func_02042c18(func_02040440(), toServer, 0x2404, 4, data, TRUE, TRUE, TRUE);
}

BOOL WifiBattleMatchNet_TakeValue4(WifiBattleMatchNet *p_wk, void *out) {
    u32 flag = p_wk->flag148;

    if (flag != 0) {
        p_wk->flag148 = 0;
        sys_memcpy(p_wk->recvBuffer, out, 4);
    }
    return flag;
}

BOOL WifiBattleMatchNet_SendReportValue(WifiBattleMatchNet *p_wk, const void *data) {
    return func_02042be8(func_02040440(), 0x2405, 4, data);
}

BOOL WifiBattleMatchNet_TakeReportValue(WifiBattleMatchNet *p_wk, u32 *out) {
    u32 flag = p_wk->flagValueReceived;

    if (flag != 0) {
        p_wk->flagValueReceived = 0;
        *out = p_wk->receivedValue;
    }
    return flag;
}

void WifiBattleMatchNet_InitDownload(WifiBattleMatchNet *p_wk, s32 profileId) {
    p_wk->state = 0;
    p_wk->downloading = 0;
    sys_memset(p_wk->downloadAttr, 0, 10);
    sys_memset(&p_wk->downloadFile, 0, sizeof(DWCNdFileInfo));
    func_020800e8(p_wk->downloadAttr, "%d", profileId);
}

u32 WifiBattleMatchNet_DownloadMain(WifiBattleMatchNet *p_wk) {
    switch (p_wk->state) {
    case 0:
        if (!DWC_NdInitAsync(WifiBattleMatchNet_OnDownload, "IRAO", "WX9x7Zh6J3aBC4zQ")) {
            return 3;
        }
        p_wk->downloading = TRUE;
        WifiBattleMatchNet_StartDownloadWait(p_wk, 1);
        break;
    case 1:
        if (!DWC_NdSetAttr("REGCARD_E", p_wk->downloadAttr, "")) {
            WifiBattleMatchNet_StartDownloadEnd(p_wk, 8);
        } else {
            p_wk->state = 2;
        }
        break;
    case 2:
        if (!DWC_NdGetFileListAsync(&p_wk->downloadFile, 0, 1)) {
            WifiBattleMatchNet_StartDownloadEnd(p_wk, 8);
        } else {
            WifiBattleMatchNet_StartDownloadWait(p_wk, 3);
        }
        break;
    case 3:
        if (!WifiBattleMatchNet_IsAllZero((const u32 *)&p_wk->downloadFile, sizeof(DWCNdFileInfo))) {
            if (!DWC_NdGetFileAsync(&p_wk->downloadFile, p_wk->downloadBuffer, sizeof(p_wk->downloadBuffer))) {
                WifiBattleMatchNet_StartDownloadEnd(p_wk, 8);
            } else {
                sDownload.done = 0;
                sDownload.code = 0;
                p_wk->state = 4;
            }
        } else if (!DWC_NdCleanupAsync()) {
            p_wk->downloading = FALSE;
            return 2;
        } else {
            WifiBattleMatchNet_StartDownloadWait(p_wk, 7);
        }
        break;
    case 4:
        NdReportResults();
        if (p_wk->timer++ > 7200) {
            func_020424ac(0, 0, 0, 0x3f7);
            WifiBattleMatchNet_StartDownloadCancel(p_wk, 8);
        }
        if (sDownload.done == 0) {
            if (DWC_NdGetProgress(&p_wk->downloadReceived, &p_wk->downloadSize) == 1) {
                u32 percent = p_wk->downloadReceived * 100 / p_wk->downloadSize;

                if (p_wk->downloadPercent != percent) {
                    p_wk->downloadPercent = percent;
                }
            }
        } else if (sDownload.code != 0) {
            if (sDownload.code == ND_ERROR_FATAL) {
                GFL_NetErrAbort();
            } else {
                WifiBattleMatchNet_StartDownloadEnd(p_wk, 8);
            }
        } else {
            if (!DWC_NdCleanupAsync()) {
                p_wk->downloading = FALSE;
                return 3;
            }
            WifiBattleMatchNet_StartDownloadWait(p_wk, 5);
        }
        break;
    case 5:
        p_wk->state = 6;
        break;
    case 6:
        if (TournamentEntry_CheckCRC(p_wk->downloadBuffer)) {
            p_wk->downloading = FALSE;
            return 1;
        }
        p_wk->downloading = FALSE;
        return 2;
    case 7:
        p_wk->downloading = FALSE;
        return 2;
    case 8:
        p_wk->downloading = FALSE;
        return 3;
    case 100:
        NdReportResults();
        if (p_wk->timer++ > 7200) {
            func_020424ac(0, 0, 0, 0x3f7);
            WifiBattleMatchNet_StartDownloadCancel(p_wk, 8);
        }
        if (sDownload.code != 0) {
            if (sDownload.code == ND_ERROR_FATAL) {
                GFL_NetErrAbort();
            } else {
                WifiBattleMatchNet_StartDownloadEnd(p_wk, 8);
            }
        }
        if (sDownload.done != 0) {
            sDownload.done = 0;
            if (sDownload.code == 0) {
                p_wk->state = p_wk->resumeState;
            }
        }
        break;
    case 200:
        NdReportResults();
        if (sDownload.done2 != 0) {
            sDownload.done2 = 0;
            p_wk->state = p_wk->resumeState;
        }
        break;
    case 300:
        NdReportResults();
        if (sDownload.done != 0) {
            sDownload.done = 0;
            WifiBattleMatchNet_StartDownloadEnd(p_wk, p_wk->resumeState);
        }
        break;
    }
    return 0;
}

static BOOL WifiBattleMatchNet_IsAllZero(const u32 *data, u32 size) {
    u32 i;

    for (i = 0; i < size / 4; i++) {
        if (data[i] != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

static void WifiBattleMatchNet_OnDwcError(void *work, int code, int type, int error) {
    WifiBattleMatchNet *p_wk = work;

    if (type == 3 || type == 6) {
        if (p_wk->p_nhttp != NULL) {
            NHttpRap_EndRequest(p_wk->p_nhttp);
            NHttpRap_Destroy(p_wk->p_nhttp);
            p_wk->p_nhttp = NULL;
        }
        WifiBattleMatchNet_EndRequests(p_wk);
    }
}

static u32 WifiBattleMatchNet_OnDwcEvent(void *work, int a1, int event, int a3) {
    WifiBattleMatchNet *p_wk = work;

    if (event == 3 || event == 6) {
        return p_wk->downloading;
    }
    return 0;
}

static u32 WifiBattleMatchNet_GetHttpResultMessage(u32 result) {
    switch (result) {
    case 0:
        GFL_ASSERT(0);
        return 0;
    case 1:
        return 0x34;
    case 2:
        return 0x35;
    case 3:
        return 0x36;
    case 4:
        return 0x37;
    case 5:
        return 0x38;
    case 6:
        return 0x39;
    case 7:
        return 0x3a;
    case 8:
        return 0x3b;
    case 9:
    default:
        return 0x3c;
    }
}

void WifiBattleMatchNet_GetDownload(WifiBattleMatchNet *p_wk, void *dest) {
    sys_memcpy(p_wk->downloadBuffer, dest, sizeof(p_wk->downloadBuffer));
}

static void WifiBattleMatchNet_StartDownloadWait(WifiBattleMatchNet *p_wk, u32 nextState) {
    sDownload.done = 0;
    sDownload.code = 0;
    p_wk->resumeState = nextState;
    p_wk->state = 100;
    p_wk->timer = 0;
}

static void WifiBattleMatchNet_StartDownloadEnd(WifiBattleMatchNet *p_wk, u32 nextState) {
    sDownload.done = 0;
    sDownload.code = 0;
    p_wk->resumeState = nextState;
    p_wk->state = 200;
    if (!DWC_NdCleanupAsync()) {
        p_wk->resumeState = nextState;
    }
}

static void WifiBattleMatchNet_StartDownloadCancel(WifiBattleMatchNet *p_wk, u32 nextState) {
    sDownload.done = 0;
    sDownload.code = 0;
    p_wk->resumeState = nextState;
    p_wk->state = 300;
    if (!DWC_NdCancelAsync()) {
        p_wk->resumeState = nextState;
    }
}

static void WifiBattleMatchNet_OnDownload(u32 reason, u32 error, int serverError) {
    switch (error) {
    case DWC_ND_ERROR_NONE:
    case DWC_ND_ERROR_ALLOC:
    case DWC_ND_ERROR_BUSY:
    case DWC_ND_ERROR_HTTP:
    case DWC_ND_ERROR_BUFFULL:
    case ND_ERROR_BAD_PARAMETER:
    case DWC_ND_ERROR_DLSERVER:
        break;
    case DWC_ND_ERROR_CANCELED:
    case ND_ERROR_FATAL:
        GFL_NetErrAbort();
        break;
    }
    sDownload.done = 1;
    sDownload.code = error;
    sDownload.done2 = 1;
}

void WifiBattleMatchNet_StartAccountRequest(WifiBattleMatchNet *p_wk, HeapID heapId) {
    GFL_ASSERT(p_wk->p_nhttp == NULL);
    p_wk->state = 0;
    p_wk->p_nhttp = NHttpRap_Create(heapId, func_02008bdc(GetGameDataPlayerInfo(p_wk->gameData)), p_wk->loginBuffer);
}

u32 WifiBattleMatchNet_AccountRequestMain(WifiBattleMatchNet *p_wk) {
    NHttpRap *p_nhttp = p_wk->p_nhttp;

    if (p_nhttp == NULL) {
        return 2;
    }
    switch (p_wk->state) {
    case 0:
        if (NHttpRap_SendRequest(5, p_nhttp)) {
            p_wk->state = 1;
        }
        break;
    case 1:
        if (NHttpRap_StartRequest(p_nhttp) == 0) {
            p_wk->state = 2;
        } else {
            if (p_wk->p_nhttp != NULL) {
                NHttpRap_Destroy(p_wk->p_nhttp);
                p_wk->p_nhttp = NULL;
            }
            return 2;
        }
        break;
    case 2: {
        int status = NHttpRap_GetStatus(p_nhttp);
        int result = NHttpRap_Poll(p_wk->p_nhttp);

        if (result == 0) {
            WifiBattleMatchNetAnswer *answer = NHttpRap_GetAnswer(p_wk->p_nhttp);

            switch (status) {
            case 503:
                p_wk->state = 4;
                func_020120f0(0x3e);
                return 0;
            case 502:
                p_wk->state = 4;
                func_020120f0(0x3f);
                return 0;
            }
            p_wk->httpValues = answer->values;
            if (answer->result == 0) {
                p_wk->state = 3;
            } else {
                func_020120f0(WifiBattleMatchNet_GetHttpResultMessage(answer->result));
                p_wk->state = 4;
            }
        } else if (result != 15) {
            if (p_wk->p_nhttp != NULL) {
                NHttpRap_Destroy(p_wk->p_nhttp);
                p_wk->p_nhttp = NULL;
            }
            return 2;
        }
        break;
    }
    case 4:
        sys_memset(&p_wk->httpValues, 0, sizeof(p_wk->httpValues));
        if (p_wk->p_nhttp != NULL) {
            NHttpRap_EndRequest(p_wk->p_nhttp);
            NHttpRap_Destroy(p_wk->p_nhttp);
            p_wk->p_nhttp = NULL;
        }
        return 3;
    case 3:
        if (p_nhttp != NULL) {
            NHttpRap_Destroy(p_nhttp);
            p_wk->p_nhttp = NULL;
        }
        return 1;
    }
    return 0;
}

void WifiBattleMatchNet_GetAccountValues(WifiBattleMatchNet *p_wk, void *out) {
    WifiBattleMatchNetValues *values = out;

    *values = p_wk->httpValues;
}

void WifiBattleMatchNet_StartPostRequest(WifiBattleMatchNet *p_wk, const void *in, HeapID heapId) {
    const WifiBattleMatchNetValues *values = in;

    GFL_ASSERT(p_wk->p_nhttp == NULL);
    p_wk->p_nhttp = NHttpRap_Create(heapId, func_02008bdc(GetGameDataPlayerInfo(p_wk->gameData)), p_wk->loginBuffer);
    p_wk->postData = *values;
    p_wk->state = 0;
}

u32 WifiBattleMatchNet_PostRequestMain(WifiBattleMatchNet *p_wk) {
    NHttpRap *p_nhttp = p_wk->p_nhttp;

    if (p_nhttp == NULL) {
        return 2;
    }
    switch (p_wk->state) {
    case 0:
        if (NHttpRap_SendRequest(6, p_nhttp)) {
            NHTTPSetPostDataRaw(NHttpRap_GetConnection(p_wk->p_nhttp), &p_wk->postData, sizeof(p_wk->postData));
            p_wk->state = 1;
        }
        break;
    case 1:
        if (NHttpRap_StartRequest(p_nhttp) == 0) {
            p_wk->state = 2;
        } else {
            if (p_wk->p_nhttp != NULL) {
                NHttpRap_Destroy(p_wk->p_nhttp);
                p_wk->p_nhttp = NULL;
            }
            return 2;
        }
        break;
    case 2: {
        int status = NHttpRap_GetStatus(p_nhttp);
        int result = NHttpRap_Poll(p_wk->p_nhttp);

        if (result == 0) {
            WifiBattleMatchNetAnswer *answer = NHttpRap_GetAnswer(p_wk->p_nhttp);

            switch (status) {
            case 503:
                p_wk->state = 3;
                func_020120f0(0x3e);
                return 0;
            case 502:
                p_wk->state = 3;
                func_020120f0(0x3f);
                return 0;
            }
            if (answer->result == 0) {
                p_wk->state = 4;
            } else {
                func_020120f0(WifiBattleMatchNet_GetHttpResultMessage(answer->result));
                p_wk->state = 3;
            }
        } else if (result != 15) {
            WifiBattleMatchNet_SetHttpError(&p_wk->error, result, 5899);
            if (p_wk->p_nhttp != NULL) {
                NHttpRap_Destroy(p_wk->p_nhttp);
                p_wk->p_nhttp = NULL;
            }
            return 2;
        }
        break;
    }
    case 3:
        if (p_nhttp != NULL) {
            NHttpRap_EndRequest(p_nhttp);
            NHttpRap_Destroy(p_wk->p_nhttp);
            p_wk->p_nhttp = NULL;
        }
        return 3;
    case 4:
        if (p_nhttp != NULL) {
            NHttpRap_Destroy(p_nhttp);
            p_wk->p_nhttp = NULL;
        }
        return 1;
    }
    return 0;
}

void WifiBattleMatchNet_StartNameCheck(WifiBattleMatchNet *p_wk, PlayerInfo *info, u16 unk24) {
    StrBuf *name;
    const u16 *chars;
    u32 i;

    p_wk->namePlayerInfo = info;
    name = copyTrainerNameToNewStrbuf(info->name, p_wk->msgHeapId);
    sys_memset(&p_wk->nameCheck, 0, sizeof(p_wk->nameCheck));
    p_wk->nameCheck.unk24 = unk24;
    chars = GFL_StrBufGetStringPtr(name);
    for (i = 0; i < GFL_StrBufGetCharCount(name) + 1; i++) {
        if (chars[i] == GFL_StrBufGetTerminator()) {
            p_wk->nameCheck.name[i] = 0;
        } else {
            p_wk->nameCheck.name[i] = chars[i];
        }
    }
    p_wk->nameCheck.words = p_wk->nameCheck.name;
    p_wk->nameCheck.badWordCount = 0;
    func_ov011_0216bea4(&p_wk->nameCheck.words, 1, NULL, 0, &p_wk->nameCheck.result, &p_wk->nameCheck.badWordCount,
                        0x80);
    GFL_StrBufFree(name);
}

BOOL WifiBattleMatchNet_NameCheckMain(WifiBattleMatchNet *p_wk, u32 *out) {
    BOOL done = func_ov011_0216bed4() == 2;

    if (done) {
        *out = p_wk->nameCheck.badWordCount > 0;
    }
    if (done == 1 && *out == 1) {
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN, p_wk->msgHeapId);
        StrBuf *name = GFL_MsgDataLoadStrbufNew(msgData, WBM_DEFAULT_NAME);

        GFL_MsgDataFree(msgData);
        copyTrainerNameFromStrbuf(p_wk->namePlayerInfo, name);
        GFL_StrBufFree(name);
    }
    return done;
}

void WifiBattleMatchNet_StartPokemonCheck(WifiBattleMatchNet *p_wk, void *data, int kind) {
    p_wk->state = 0;
    p_wk->p_nhttp =
        NHttpRap_Create(p_wk->msgHeapId, func_02008bdc(GetGameDataPlayerInfo(p_wk->gameData)), p_wk->loginBuffer);
    switch (kind) {
    case 0:
        NHttpRap_BeginPost(p_wk->p_nhttp, p_wk->msgHeapId, PokeParty_GetPkmRawSize(), 0);
        NHttpRap_AddPostData(p_wk->p_nhttp, data, PokeParty_GetPkmRawSize());
        p_wk->unk178 = 1;
        break;
    case 1: {
        PokeParty *party = data;
        u32 size = PokeParty_GetPkmRawSize();
        int i;

        NHttpRap_BeginPost(p_wk->p_nhttp, p_wk->msgHeapId, PokeParty_GetPkmCount(party) * size, 0);
        for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
            PartyPkm *pkm = PokeParty_GetPkm(party, i);

            NHttpRap_AddPostData(p_wk->p_nhttp, pkm, PokeParty_GetPkmRawSize());
        }
        p_wk->unk178 = PokeParty_GetPkmCount(party);
        break;
    }
    }
}

u32 WifiBattleMatchNet_PokemonCheckMain(WifiBattleMatchNet *p_wk, WifiBattleMatchNetCheck *out) {
    switch (p_wk->state) {
    case 0:
        if (NHttpRap_SendValidate(p_wk->p_nhttp)) {
            if (NHttpRap_StartRequest(p_wk->p_nhttp) == 0) {
                p_wk->state++;
            } else {
                if (p_wk->p_nhttp != NULL) {
                    NHttpRap_Destroy(p_wk->p_nhttp);
                    p_wk->p_nhttp = NULL;
                    p_wk->state = 0;
                }
                return 2;
            }
        } else {
            if (p_wk->p_nhttp != NULL) {
                NHttpRap_Destroy(p_wk->p_nhttp);
                p_wk->p_nhttp = NULL;
                p_wk->state = 0;
            }
            return 2;
        }
        break;
    case 1: {
        int status = NHttpRap_GetStatus(p_wk->p_nhttp);
        int result = NHttpRap_Poll(p_wk->p_nhttp);

        if (result == 0) {
            switch (status) {
            case 400:
                func_020120f0(0x41);
                if (p_wk->p_nhttp != NULL) {
                    NHttpRap_FreePostData(p_wk->p_nhttp);
                    NHttpRap_Destroy(p_wk->p_nhttp);
                    NHttpRap_EndRequest(p_wk->p_nhttp);
                    p_wk->p_nhttp = NULL;
                    p_wk->state = 0;
                }
                return 3;
            case 401:
                func_020120f0(0x42);
                if (p_wk->p_nhttp != NULL) {
                    NHttpRap_FreePostData(p_wk->p_nhttp);
                    NHttpRap_Destroy(p_wk->p_nhttp);
                    NHttpRap_EndRequest(p_wk->p_nhttp);
                    p_wk->p_nhttp = NULL;
                    p_wk->state = 0;
                }
                return 3;
            case 408:
                func_020120f0(0x44);
                if (p_wk->p_nhttp != NULL) {
                    NHttpRap_FreePostData(p_wk->p_nhttp);
                    NHttpRap_Destroy(p_wk->p_nhttp);
                    NHttpRap_EndRequest(p_wk->p_nhttp);
                    p_wk->p_nhttp = NULL;
                    p_wk->state = 0;
                }
                return 3;
            default:
                func_020120f0(0x43);
                if (p_wk->p_nhttp != NULL) {
                    NHttpRap_FreePostData(p_wk->p_nhttp);
                    NHttpRap_Destroy(p_wk->p_nhttp);
                    NHttpRap_EndRequest(p_wk->p_nhttp);
                    p_wk->p_nhttp = NULL;
                    p_wk->state = 0;
                }
                return 3;
            case 200:
                p_wk->state++;
                break;
            }
        } else if (result != 15) {
            WifiBattleMatchNet_SetHttpError(&p_wk->error, result, 6159);
            NHttpRap_FreePostData(p_wk->p_nhttp);
            if (p_wk->p_nhttp != NULL) {
                NHttpRap_Destroy(p_wk->p_nhttp);
                p_wk->p_nhttp = NULL;
                p_wk->state = 0;
            }
            return 2;
        }
        break;
    }
    case 2: {
        void *answer = NHttpRap_GetAnswer(p_wk->p_nhttp);
        u32 i;

        out->status = NHttpRap_GetCheckStatus(answer);
        for (i = 0; i < p_wk->unk178; i++) {
            out->result[i] = NHttpRap_GetCheckResult(answer, i);
        }
        if (out->status == 0) {
            sys_memcpy(NHttpRap_GetCheckSignature(answer, p_wk->unk178), out->signature, 0x80);
        }
        NHttpRap_FreePostData(p_wk->p_nhttp);
        if (p_wk->p_nhttp != NULL) {
            NHttpRap_Destroy(p_wk->p_nhttp);
            p_wk->p_nhttp = NULL;
        }
        return 1;
    }
    }
    return 0;
}

static void WifiBattleMatchNet_SetTimeoutError(WifiBattleMatchNetError *error, int code, int line) {
    error->timeoutCode = code;
    error->kind = WBM_ERROR_TIMEOUT;
    func_020424ac(0, 0, 0, 0x3f5);
}

static void WifiBattleMatchNet_SetDwcError(WifiBattleMatchNetError *error, int code, int line) {
    error->dwcCode = code;
    error->kind = WBM_ERROR_DWC;
    func_020424ac(0, 0, 0, 0x3f7);
}

static void WifiBattleMatchNet_SetGdbError(WifiBattleMatchNetError *error, int code, int line) {
    error->gdbCode = code;
    error->kind = WBM_ERROR_GDB;
}

static void WifiBattleMatchNet_SetGdbSubError1(WifiBattleMatchNetError *error, int code, int line) {
    error->gdbSubCode1 = code;
    error->kind = WBM_ERROR_GDB;
}

static void WifiBattleMatchNet_SetGdbSubError2(WifiBattleMatchNetError *error, int code, int line) {
    error->gdbSubCode2 = code;
    error->kind = WBM_ERROR_GDB;
}

static void WifiBattleMatchNet_SetHttpError(WifiBattleMatchNetError *error, int code, int line) {
    error->httpCode = code;
    error->kind = WBM_ERROR_HTTP;
}

static u32 WifiBattleMatchNet_GetDwcResult(u32 code) {
    switch (code) {
    case 0:
        return WBM_RESULT_OK;
    case 1:
        return WBM_RESULT_FAILED;
    case 2:
        return WBM_RESULT_FAILED;
    case 3:
        return WBM_RESULT_FAILED;
    case 4:
        return WBM_RESULT_FAILED;
    case 5:
        return WBM_RESULT_FAILED;
    case 6:
        return WBM_RESULT_FAILED;
    case 7:
        return WBM_RESULT_FAILED;
    case 8:
        return WBM_RESULT_FAILED;
    case 9:
        return WBM_RESULT_FAILED;
    case 10:
        return WBM_RESULT_FAILED;
    case 11:
        return WBM_RESULT_FAILED;
    case 12:
        return WBM_RESULT_FAILED;
    case 13:
        return WBM_RESULT_FAILED;
    case 14:
        return WBM_RESULT_FAILED;
    default:
        GFL_ASSERT(0);
        return WBM_RESULT_OK;
    }
}

static u32 WifiBattleMatchNet_GetGdbResult(u32 code, u32 subCode1, u32 subCode2) {
    switch (code) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        return WBM_RESULT_FAILED;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        return WBM_RESULT_DISCONNECTED;
    case 17:
    case 18:
        return WBM_RESULT_FAILED;
    case 19:
        return WBM_RESULT_DISCONNECTED;
    case 21:
    case 22:
        return WBM_RESULT_FAILED;
    }
    switch (subCode1) {
    case 0:
        return WBM_RESULT_DISCONNECTED;
    case 1:
    case 2:
    case 3:
        break;
    case 4:
        return WBM_RESULT_FAILED;
    }
    switch (subCode2) {
    case 2:
        return WBM_RESULT_FAILED;
    case 3:
        return WBM_RESULT_FAILED;
    case 4:
        return WBM_RESULT_DISCONNECTED;
    case 5:
        return WBM_RESULT_FAILED;
    case 6:
        return WBM_RESULT_FAILED;
    case 7:
        return WBM_RESULT_FAILED;
    case 8:
    case 9:
    case 10:
        return WBM_RESULT_FAILED;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        return WBM_RESULT_DISCONNECTED;
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
        return WBM_RESULT_DISCONNECTED;
    case 22:
        return WBM_RESULT_FAILED;
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
        return WBM_RESULT_DISCONNECTED;
    case 28:
    case 29:
        return WBM_RESULT_FAILED;
    case 30:
    case 31:
        return WBM_RESULT_FAILED;
    }
    return WBM_RESULT_OK;
}

static u32 WifiBattleMatchNet_GetTimeoutResult(u32 code) {
    switch (code) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    }
    return 0;
}

static void WifiBattleMatchNet_ClearError(WifiBattleMatchNetError *error) {
    sys_memset(error, 0, sizeof(WifiBattleMatchNetError));
}

void WifiBattleMatchNet_ShowIcon(WifiBattleMatchNet *p_wk, int a1) {
    if (a1 != 0) {
        func_02042ba8(TRUE, p_wk->msgHeapId);
    }
}

s32 WifiBattleMatchNet_GetGSID(WifiBattleMatchNet *p_wk) {
    return WifiList_GetMyGSID(p_wk->wifiList);
}
