// wifibattlematch_sys.c, in overlay 290 (named by its embedded string): the Battle Competition. It runs a chain of the
// overlays that make it up, each with a function that makes its parameters and one that reads what it left

#include "types.h"
#include "app/battle_championship_flow.h"
#include "app/livebattlematch_irc.h"
#include "app/ov306.h"
#include "app/wifi_login.h"
#include "app/wifibattlematch.h"
#include "app/wifibattlematch_link.h"
#include "app/wifibattlematch_subproc.h"
#include "battle/battle_proc.h"
#include "battle/battle_rec_tool.h"
#include "battle/btl_net.h"
#include "battle/btl_setup.h"
#include "battle/regulation.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "dwc/dwc.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/tournament_block.h"
#include "system/game_data.h"

#define HEAPID_WIFIBATTLEMATCH 0x56
// The heap of the proc that runs the battle
#define HEAPID_WIFIBATTLEMATCH_BATTLE 0x58

// GFLNetInitData.bNetType of an infrared connection
#define NET_TYPE_IRC 3

// What the chain runs, by the number that WifiBattleMatchSys_ChainSetNext gets
enum {
    STEP_CHAMPIONSHIP,
    STEP_NET,
    STEP_TEAM_LIST,
    STEP_BATTLE,
    STEP_LOGIN,
    STEP_TEAM_EXCHANGE,
    STEP_RESULT,
    STEP_LOGOUT,
    STEP_REPLAY,
    STEP_COUNT,
};

typedef struct WifiBattleMatchSys WifiBattleMatchSys;
typedef struct WifiBattleMatchChain WifiBattleMatchChain;

// Makes the parameters of a step, which the chain frees or the step's finish function does
typedef void *(*WifiBattleMatchCreateFunc)(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys);
// Reads what the step left, and returns TRUE once it is done
typedef BOOL (*WifiBattleMatchFinishFunc)(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys);

typedef struct {
    s32 overlayId;
    const GameProcFunctions *functions;
    WifiBattleMatchCreateFunc create;
    WifiBattleMatchFinishFunc finish;
} WifiBattleMatchStep;

struct WifiBattleMatchChain {
    GameProcManager *procManager;
    void *p_param;
    HeapID heapId;
    WifiBattleMatchSys *sys;
    const WifiBattleMatchStep *steps;
    u32 stepCount;
    // The step that ran last, the one to run next and the one running
    u8 previous;
    u8 next;
    u8 current;
    u8 state;
    u32 procResult;
};

// A player as the two machines exchange it: the info and the team picked
typedef struct {
    PlayerInfo info;
    u8 unk20[0x24];
    u32 unk44;
    u32 unk48;
    u8 unk4C[0xd4 - 0x4c];
    u8 partyData[];
} WifiBattleMatchPlayer;

// What the battle left, for the steps that follow it
typedef struct {
    u32 result;
    u32 battleStyle;
    u8 unk8;
    u8 unk9;
    u8 unkA;
    // Set when the other player left
    u32 unkC;
    u32 unk10;
    u32 unk14;
} WifiBattleMatchBattleResult;

// What a battle sends to the ranking: the other player's team, then the player's own
typedef struct {
    s32 profileId;
    u32 unk4;
    u16 species[6];
    u8 forms[6];
    u8 levels[6];
    u8 genders[6];
    u16 ownSpecies[6];
    u8 ownForms[6];
    u8 ownLevels[6];
    u8 ownGenders[6];
    u32 year : 7;
    u32 month : 4;
    u32 day : 5;
    u32 hour : 5;
    u32 minute : 6;
    u32 unk : 4;
    u32 unkFlag : 1;
    u32 unk48;
    u8 result;
    u8 battleType;
    u8 aliveOwn : 4;
    u8 aliveOther : 4;
} WifiBattleMatchReport;

struct WifiBattleMatchSys {
    BOOL ownsGameData;
    WifiBattleMatchParam param;
    WifiBattleMatchChain *chain;
    // 0 and 1 are the competitions that the first screen starts, 2 and 3 the live ones
    u32 mode;
    u32 result;
    BOOL unk24;
    WifiBattleMatchPlayer *player;
    WifiBattleMatchPlayer *otherPlayer;
    // The team picked, the other player's team, and the parties they pick from
    PokeParty *party;
    PokeParty *otherParty;
    PokeParty *pickParty;
    PokeParty *otherPickParty;
    u8 unk40[0x174];
    WifiBattleMatchBattleResult battleResult;
    u32 unk1CC;
    WifiBattleMatchReport report;
    BOOL relogin;
    u8 unk224[8];
    u8 unk22C[0x54c];
    u8 unk778[0x5c];
    u8 unk7D4[0xd4];
    u32 unk8A8;
    BOOL unk8AC;
};

// What the infrared steps get
typedef struct {
    GameData *gameData;
    u32 previous;
    u32 result;
    WifiBattleMatchPlayer *player;
    WifiBattleMatchPlayer *otherPlayer;
    WifiBattleMatchBattleResult *battleResult;
} WifiBattleMatchChampionshipParam;

// What the Wi-Fi link gets
typedef struct {
    u32 mode;
    // What the link leaves for the chain to do next
    u32 action;
    WifiBattleMatchParam *param;
    u32 result;
    void *record;
    WifiBattleMatchPlayer *player;
    WifiBattleMatchPlayer *otherPlayer;
    void *unk1C;
    u32 *unk20;
    WifiBattleMatchReport *report;
    void *unk28;
    WifiBattleMatchBattleResult *battleResult;
    void *unk30;
    void *unk34;
    void *unk38;
    void *unk3C;
    BOOL *unk40;
    u32 unk44;
} WifiBattleMatchNetParam;

// The players of a battle, with what the ranking needs
typedef struct {
    BattlePlayers base;
    u32 unk50;
    GameRecords *records;
} WifiBattleMatchPlayers;

// What the battle step gets
typedef struct {
    GameData *gameData;
    BtlSetup *setup;
    WifiBattleMatchPlayers *players;
    u32 result;
} WifiBattleMatchBattleParam;

// The battle proc's work
typedef struct {
    GameProcManager *manager;
    u32 timer;
    BOOL ov167Loaded;
    BOOL commandsSet;
} WifiBattleMatchBattleWork;

static BOOL WifiBattleMatchSys_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchSys_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchSys_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void *WifiBattleMatchSys_CreateChampionshipParam(WifiBattleMatchChain *chain, HeapID heapId,
                                                        WifiBattleMatchSys *sys);
static BOOL WifiBattleMatchSys_FinishChampionship(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys);
static void *WifiBattleMatchSys_CreateNetParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys);
static BOOL WifiBattleMatchSys_FinishNet(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys);
static void *WifiBattleMatchSys_CreateListParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys);
static BOOL WifiBattleMatchSys_FinishList(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys);
static void *WifiBattleMatchSys_CreateBattleParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys);
static BOOL WifiBattleMatchSys_FinishBattle(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys);
static void *WifiBattleMatchSys_CreateLoginParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys);
static BOOL WifiBattleMatchSys_FinishLogin(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys);
static void *WifiBattleMatchSys_CreateExchangeParam(WifiBattleMatchChain *chain, HeapID heapId,
                                                    WifiBattleMatchSys *sys);
static BOOL WifiBattleMatchSys_FinishExchange(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys);
static void *WifiBattleMatchSys_CreateResultParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys);
static BOOL WifiBattleMatchSys_FinishResult(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys);
static void *WifiBattleMatchSys_CreateLogoutParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys);
static BOOL WifiBattleMatchSys_FinishLogout(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys);
static void *WifiBattleMatchSys_CreateReplayParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys);
static BOOL WifiBattleMatchSys_FinishReplay(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys);
static void WifiBattleMatchSys_AllocPlayers(WifiBattleMatchSys *sys, HeapID heapId);
static void WifiBattleMatchSys_FreePlayers(WifiBattleMatchSys *sys);
static void WifiBattleMatchSys_BuildReport(WifiBattleMatchReport *report, PokeParty *party, PokeParty *otherParty,
                                           WifiBattleMatchPlayer *otherPlayer, Regulation *regulation, u32 unk14,
                                           BtlSetup *setup, WifiBattleMatchBattleResult *battleResult, u32 unk10);
static void WifiBattleMatchSys_CountAlive(BtlSetup *setup, u8 *own, u8 *other);
static void WifiBattleMatchSys_GetHpPercents(BtlSetup *setup, u8 *own, u8 *other);
static WifiBattleMatchChain *WifiBattleMatchSys_ChainCreate(const WifiBattleMatchStep *steps, u32 stepCount,
                                                            WifiBattleMatchSys *sys, HeapID heapId);
static void WifiBattleMatchSys_ChainFree(WifiBattleMatchChain *p_wk);
static BOOL WifiBattleMatchSys_ChainUpdate(WifiBattleMatchChain *chain);
static u32 WifiBattleMatchSys_ChainGetResult(WifiBattleMatchChain *chain);
static u8 WifiBattleMatchSys_ChainGetPrevious(WifiBattleMatchChain *chain);
static void WifiBattleMatchSys_ChainSetNext(WifiBattleMatchChain *chain, u8 step);
static void WifiBattleMatchSys_ChainEnd(WifiBattleMatchChain *chain);
static BOOL WifiBattleMatchSys_BattleProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchSys_BattleProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchSys_BattleProcMain(GameProc *proc, u32 *state, void *param, void *work);

static const GameProcFunctions sWifiBattleMatchSysBattleProcFunctions;

const GameProcFunctions WIFIBATTLEMATCH_PROC_FUNCTIONS = {
    WifiBattleMatchSys_Init,
    WifiBattleMatchSys_Main,
    WifiBattleMatchSys_Exit,
};

static const WifiBattleMatchStep sWifiBattleMatchSysSteps[STEP_COUNT] = {
    { OVERLAY_ID(261), &BATTLE_CHAMPIONSHIP_FLOW_PROC_FUNCTIONS, WifiBattleMatchSys_CreateChampionshipParam,
      WifiBattleMatchSys_FinishChampionship },
    { OVERLAY_ID(260), &WIFIBATTLEMATCH_LINK_PROC_FUNCTIONS, WifiBattleMatchSys_CreateNetParam, WifiBattleMatchSys_FinishNet },
    { OVERLAY_ID(263), &WIFIBATTLEMATCH_LIST_PROC_FUNCTIONS, WifiBattleMatchSys_CreateListParam,
      WifiBattleMatchSys_FinishList },
    { OVERLAY_NONE, &sWifiBattleMatchSysBattleProcFunctions, WifiBattleMatchSys_CreateBattleParam, WifiBattleMatchSys_FinishBattle },
    { OVERLAY_ID(190), &WIFILOGIN_PROC_FUNCTIONS, WifiBattleMatchSys_CreateLoginParam, WifiBattleMatchSys_FinishLogin },
    { OVERLAY_ID(263), &WIFIBATTLEMATCH_EXCHANGE_PROC_FUNCTIONS, WifiBattleMatchSys_CreateExchangeParam,
      WifiBattleMatchSys_FinishExchange },
    { OVERLAY_ID(306), &BtlRecSelProcFunctions, WifiBattleMatchSys_CreateResultParam, WifiBattleMatchSys_FinishResult },
    { OVERLAY_ID(190), &WIFILOGOUT_PROC_FUNCTIONS, WifiBattleMatchSys_CreateLogoutParam,
      WifiBattleMatchSys_FinishLogout },
    { OVERLAY_NONE, &data_ov167_021d6ce0, WifiBattleMatchSys_CreateReplayParam, WifiBattleMatchSys_FinishReplay },
};

static const GameProcFunctions sWifiBattleMatchSysBattleProcFunctions = {
    WifiBattleMatchSys_BattleProcInit,
    WifiBattleMatchSys_BattleProcMain,
    WifiBattleMatchSys_BattleProcExit,
};

static BOOL WifiBattleMatchSys_Init(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchParam *matchParam = param;
    WifiBattleMatchSys *sys;
    WifiBattleMatchPlayer *player;
    PokeParty *party;
    HeapID parentHeapId;

    switch (matchParam->unk8) {
    case 1:
        parentHeapId = HEAPID_USER;
        break;
    case 0:
        parentHeapId = HEAPID_USER;
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
    GFL_HeapCreateChild(parentHeapId, HEAPID_WIFIBATTLEMATCH, 0x8000);
    sys = GFL_ProcInitSubsystem(proc, sizeof(WifiBattleMatchSys), HEAPID_WIFIBATTLEMATCH);
    sys_memset(sys, 0, sizeof(WifiBattleMatchSys));
    sys_memcpy(matchParam, &sys->param, sizeof(WifiBattleMatchParam));
    sys->result = 0;
    if (sys->param.gameData == NULL) {
        sys->param.gameData = GameData_Create(HEAPID_USER);
        sys->ownsGameData = TRUE;
    }
    sys->party = PokeParty_Create(HEAPID_WIFIBATTLEMATCH);
    sys->otherParty = PokeParty_Create(HEAPID_WIFIBATTLEMATCH);
    sys->pickParty = PokeParty_Create(HEAPID_WIFIBATTLEMATCH);
    sys->otherPickParty = PokeParty_Create(HEAPID_WIFIBATTLEMATCH);
    func_0200bb24(HEAPID_WIFIBATTLEMATCH);
    WifiBattleMatchSys_AllocPlayers(sys, HEAPID_WIFIBATTLEMATCH);
    sys->chain = WifiBattleMatchSys_ChainCreate(sWifiBattleMatchSysSteps, STEP_COUNT, sys, HEAPID_WIFIBATTLEMATCH);
    switch (sys->param.unk8) {
    case 0:
        sys->mode = 0;
        WifiBattleMatchSys_ChainSetNext(sys->chain, STEP_CHAMPIONSHIP);
        break;
    case 1:
        sys->mode = 2;
        WifiBattleMatchSys_ChainSetNext(sys->chain, STEP_LOGIN);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
    player = sys->player;
    switch (sys->param.unkC) {
    case 0:
        party = GameData_GetParty(sys->param.gameData);
        sys_memcpy(party, player->partyData, PokeParty_GetSaveDataSize());
        break;
    case 1:
        party = convertBoxedPokeSetToParty(getBattleBox(GameData_GetSaveControl(sys->param.gameData)),
                                           HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH));
        sys_memcpy(party, player->partyData, PokeParty_GetSaveDataSize());
        GFL_HeapFree(party);
        break;
    }
    return TRUE;
}

static BOOL WifiBattleMatchSys_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchSys *sys = work;

    WifiBattleMatchSys_ChainFree(sys->chain);
    WifiBattleMatchSys_FreePlayers(sys);
    freeVSPlayerBlkClearPtr();
    GFL_HeapFree(sys->otherPickParty);
    GFL_HeapFree(sys->pickParty);
    GFL_HeapFree(sys->otherParty);
    GFL_HeapFree(sys->party);
    if (sys->ownsGameData) {
        GameData_Free(sys->param.gameData);
    }
    if (sys->param.unk8 != 1) {
        sys_reset(0);
    }
    if (sys->param.unk10 != 0) {
        GFL_HeapFree(param);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFIBATTLEMATCH);
    func_02005d8c();
    return TRUE;
}

static BOOL WifiBattleMatchSys_Main(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchSys *sys = work;
    u32 result;

    switch (*state) {
    case 0:
        if (WifiBattleMatchSys_ChainUpdate(sys->chain)) {
            *state = 1;
        }
        if (sys->unk1CC != 0) {
            sys->unk1CC--;
        }
        result = WifiBattleMatchSys_ChainGetResult(sys->chain);
        if (result == 2 || result == 0) {
            GFL_NetErrShow(0);
        }
        break;
    case 1:
        return TRUE;
    }
    return FALSE;
}

static void *WifiBattleMatchSys_CreateChampionshipParam(WifiBattleMatchChain *chain, HeapID heapId,
                                                        WifiBattleMatchSys *sys) {
    WifiBattleMatchChampionshipParam *param =
        GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchChampionshipParam), FALSE, "wifibattlematch_sys.c", 591);

    sys_memset(param, 0, sizeof(WifiBattleMatchChampionshipParam));
    param->gameData = sys->param.gameData;
    param->player = sys->player;
    param->otherPlayer = sys->otherPlayer;
    param->battleResult = &sys->battleResult;
    switch (WifiBattleMatchSys_ChainGetPrevious(chain)) {
    case STEP_LOGIN:
    case STEP_LOGOUT:
        param->previous = 1;
        break;
    case STEP_REPLAY:
        param->previous = 6;
        break;
    case STEP_BATTLE:
        param->previous = 4;
        break;
    case STEP_RESULT:
        param->previous = 5;
        break;
    case STEP_CHAMPIONSHIP:
        param->previous = 0;
        break;
    default:
        if (sys->battleResult.unkC != 0) {
            param->previous = 6;
        } else {
            GFL_ASSERT(0);
        }
        break;
    }
    if (GFL_SndBGMGetID() != SEQ_BGM_WCS) {
        GFL_SndBGMPlay(SEQ_BGM_WCS, SND_CHANNEL_MASK_ALL);
    }
    return param;
}

static BOOL WifiBattleMatchSys_FinishChampionship(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys) {
    WifiBattleMatchChampionshipParam *championship = param;

    switch (championship->result) {
    case 0:
        WifiBattleMatchSys_ChainEnd(chain);
        break;
    case 1:
        sys->mode = 0;
        WifiBattleMatchSys_ChainSetNext(chain, STEP_LOGIN);
        break;
    case 2:
        sys->mode = 1;
        WifiBattleMatchSys_ChainSetNext(chain, STEP_TEAM_LIST);
        break;
    case 3:
        sys->mode = 1;
        WifiBattleMatchSys_ChainSetNext(chain, STEP_RESULT);
        break;
    case 4:
        sys->mode = 1;
        WifiBattleMatchSys_ChainSetNext(chain, STEP_REPLAY);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
    GFL_HeapFree(championship);
    return TRUE;
}

static void *WifiBattleMatchSys_CreateNetParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys) {
    WifiBattleMatchNetParam *param =
        GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchNetParam), FALSE, "wifibattlematch_sys.c", 704);

    sys_memset(param, 0, sizeof(WifiBattleMatchNetParam));
    param->param = &sys->param;
    param->result = sys->result;
    param->mode = sys->mode - 2;
    param->player = sys->player;
    param->otherPlayer = sys->otherPlayer;
    param->unk1C = sys->unk40;
    param->unk20 = &sys->unk1CC;
    param->report = &sys->report;
    param->unk3C = sys->unk7D4;
    param->unk28 = &sys->unk8A8;
    param->battleResult = &sys->battleResult;
    param->unk30 = sys->unk224;
    param->unk34 = sys->unk22C;
    param->unk38 = sys->unk778;
    param->unk40 = &sys->unk8AC;
    param->record = func_0200f2d4(getRecordBlkAddress(GameData_GetSaveControl(sys->param.gameData)));
    if (GFL_SndBGMGetID() != SEQ_BGM_WCS) {
        GFL_SndBGMPlay(SEQ_BGM_WCS, SND_CHANNEL_MASK_ALL);
    }
    return param;
}

static BOOL WifiBattleMatchSys_FinishNet(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys) {
    WifiBattleMatchNetParam *net = param;

    switch (net->mode) {
    case 0:
        sys->mode = 2;
        break;
    case 1:
        sys->mode = 3;
        break;
    case 2:
        break;
    case 3:
        sys->mode = 3;
        break;
    }
    switch (net->action) {
    case 1:
        WifiBattleMatchSys_ChainSetNext(chain, STEP_TEAM_LIST);
        break;
    case 2:
        WifiBattleMatchSys_ChainSetNext(chain, STEP_RESULT);
        break;
    case 3:
        sys->relogin = TRUE;
        WifiBattleMatchSys_ChainSetNext(chain, STEP_LOGIN);
        break;
    case 4:
        sys->unk24 = TRUE;
        // fall through
    case 0:
        WifiBattleMatchSys_ChainSetNext(chain, STEP_LOGOUT);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
    GFL_HeapFree(net);
    return TRUE;
}

static void *WifiBattleMatchSys_CreateListParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys) {
    WifiBattleMatchListParam *param;
    WifiBattleMatchPlayer *otherPlayer;
    u32 fileId;

    PokeParty_Init(sys->party);
    PokeParty_Init(sys->otherParty);
    param = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchListParam), FALSE, "wifibattlematch_sys.c", 815);
    sys_memset(param, 0, sizeof(WifiBattleMatchListParam));
    if (sys->mode - 2 <= 1) {
        switch (sys->param.unk4) {
        case 0:
            fileId = 0xf;
            break;
        case 1:
            fileId = 0x10;
            break;
        case 2:
            fileId = 0x11;
            break;
        case 3:
            fileId = 0x12;
            break;
        case 4:
            fileId = 0x13;
            break;
        default:
            GFL_ASSERT(0);
            break;
        }
        param->regulation = PokeRegulation_LoadNew(fileId, heapId);
    } else if (sys->mode == 0) {
        param->regulation =
            func_0200b530(TournamentBlock_GetEntry(getTournamentBlockAddress(GameData_GetSaveControl(sys->param.gameData)), 0));
    } else if (sys->mode == 1) {
        param->regulation =
            func_0200b530(TournamentBlock_GetEntry(getTournamentBlockAddress(GameData_GetSaveControl(sys->param.gameData)), 1));
    } else {
        GFL_ASSERT(0);
    }
    param->selected = sys->party;
    param->gameData = sys->param.gameData;
    PokeParty_Copy((PokeParty *)sys->player->partyData, sys->pickParty);
    PokeRegulation_ApplyLevelToParty(param->regulation, sys->pickParty);
    param->party = sys->pickParty;
    otherPlayer = sys->otherPlayer;
    PokeParty_Copy((PokeParty *)otherPlayer->partyData, sys->otherPickParty);
    PokeRegulation_ApplyLevelToParty(param->regulation, sys->otherPickParty);
    param->otherName = GetPlayerName(&otherPlayer->info);
    param->otherParty = sys->otherPickParty;
    param->otherGender = getTrainerGender(&otherPlayer->info);
    return param;
}

static BOOL WifiBattleMatchSys_FinishList(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys) {
    WifiBattleMatchListParam *list = param;

    if (sys->mode == 1) {
        switch (list->result) {
        case 0:
            WifiBattleMatchSys_ChainSetNext(chain, STEP_TEAM_EXCHANGE);
            break;
        case 2:
            sys->battleResult.unkC = TRUE;
            WifiBattleMatchSys_ChainSetNext(chain, STEP_CHAMPIONSHIP);
            break;
        }
    } else {
        switch (list->result) {
        case 0:
            WifiBattleMatchSys_ChainSetNext(chain, STEP_TEAM_EXCHANGE);
            break;
        case 1:
            sys->relogin = TRUE;
            WifiBattleMatchSys_ChainSetNext(chain, STEP_LOGIN);
            break;
        case 3:
            sys->result = 2;
            WifiBattleMatchSys_ChainSetNext(chain, STEP_NET);
            break;
        }
    }
    if (sys->mode - 2 <= 1) {
        GFL_HeapFree(list->regulation);
    }
    GFL_HeapFree(list);
    return TRUE;
}

static void *WifiBattleMatchSys_CreateBattleParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys) {
    WifiBattleMatchBattleParam *param;
    Regulation *regulation;
    void *tournament;
    void *entry;
    BOOL ownsRegulation = FALSE;
    u32 fileId;
    u32 battleType;
    int param0D;
    u32 value;
    u32 param12;
    BattlePlayer *player;

    tournament = getTournamentBlockAddress(GameData_GetSaveControl(sys->param.gameData));
    fileId = 0x10;
    param = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchBattleParam), FALSE, "wifibattlematch_sys.c", 992);
    sys_memset(param, 0, sizeof(WifiBattleMatchBattleParam));
    param->gameData = sys->param.gameData;
    clearVSPlayer();
    sys_memset(&sys->battleResult, 0, sizeof(WifiBattleMatchBattleResult));
    param->players = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchPlayers), FALSE, "wifibattlematch_sys.c", 1003);
    sys_memset(param->players, 0, sizeof(WifiBattleMatchPlayers));
    param->players->records = GameData_GetRecords(sys->param.gameData);
    param->players->unk50 = 0;
    player = &param->players->base.players[0];
    player->info = &sys->player->info;
    player->party = sys->party;
    player->unk08[1] = sys->player->unk44;
    player = &param->players->base.players[1];
    player->info = &sys->otherPlayer->info;
    player->party = sys->otherParty;
    player->unk08[1] = sys->otherPlayer->unk44;
    param->setup = BtlSetup_Create(heapId);
    GFL_OvlLoad(OVERLAY_ID(167));
    if (sys->mode - 2 <= 1) {
        switch (sys->param.unk4) {
        default:
        case 0:
            fileId = 0xf;
            BtlSetup_SetNet1v1Single(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        case 1:
            BtlSetup_SetNet1v1Double(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        case 2:
            fileId = 0x11;
            BtlSetup_SetNetTriple(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        case 3:
            fileId = 0x12;
            BtlSetup_SetNetRotation(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        case 4:
            fileId = 0x13;
            BtlSetup_SetNetTriple(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        }
        regulation = PokeRegulation_LoadNew(fileId, heapId);
        ownsRegulation = TRUE;
    } else if (sys->mode == 0) {
        regulation = func_0200b530(TournamentBlock_GetEntry(tournament, 0));
        switch (Regulation_GetParam(regulation, 0x15)) {
        case 0:
            BtlSetup_SetNet1v1Single(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        case 1:
            BtlSetup_SetNet1v1Double(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        case 2:
            BtlSetup_SetNetTriple(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        case 3:
            BtlSetup_SetNetRotation(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        default:
            BtlSetup_SetNet1v1Single(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        }
    } else if (sys->mode == 1) {
        regulation = func_0200b530(TournamentBlock_GetEntry(tournament, 1));
        switch (Regulation_GetParam(regulation, 0x15)) {
        case 0:
            BtlSetup_SetNet1v1Single(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        case 1:
            BtlSetup_SetNet1v1Double(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        case 2:
            BtlSetup_SetNetTriple(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        case 3:
            BtlSetup_SetNetRotation(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        default:
            BtlSetup_SetNet1v1Single(param->setup, sys->param.gameData, func_02040440(), 2, heapId);
            break;
        }
    } else {
        GFL_ASSERT(0);
    }
    if (sys->mode == 3) {
        battleType = sys->param.unk4 + 0x17;
    } else if (sys->mode == 2) {
        battleType = sys->param.unk4 + 0x1c;
    } else {
        param0D = Regulation_GetParam(regulation, 0xd);
        value = Regulation_GetParam(regulation, 0x15);
        if (param0D >= 2) {
            param0D = 1;
        }
        battleType = value * 2 + 0x21 + param0D;
    }
    param->players->base.rule = battleType;
    if (sys->mode - 2 <= 1) {
        param->setup->fieldSituation.bgm = SEQ_BGM_VS_TRAINER_WIFI;
        param->setup->fieldSituation.unk12 = SEQ_BGM_WIN2;
    } else if (sys->mode == 0) {
        entry = TournamentBlock_GetEntry(tournament, 0);
        value = TournamentEntry_GetField(entry, 0xb);
        param12 = Regulation_GetParam(func_0200b530(entry), 0x12);
        if (value == 1) {
            param->setup->fieldSituation.bgm = SEQ_BGM_VS_WCS;
            param->setup->fieldSituation.unk12 = SEQ_BGM_WIN5;
        } else {
            param->setup->fieldSituation.bgm = SEQ_BGM_VS_SUBWAY_TRAINER;
            param->setup->fieldSituation.unk12 = SEQ_BGM_WIN2;
        }
        if (param12 - 2 <= 1) {
            param->setup->fieldSituation.env.terrain = 0xe;
            param->setup->fieldSituation.env.bgType = 0x12;
            param->players->unk50 = 1;
        }
    } else if (sys->mode == 1) {
        entry = TournamentBlock_GetEntry(tournament, 1);
        value = TournamentEntry_GetField(entry, 0xb);
        param12 = Regulation_GetParam(func_0200b530(entry), 0x12);
        if (value == 1) {
            param->setup->fieldSituation.bgm = SEQ_BGM_VS_WCS;
            param->setup->fieldSituation.unk12 = SEQ_BGM_WIN5;
        } else {
            param->setup->fieldSituation.bgm = SEQ_BGM_VS_SUBWAY_TRAINER;
            param->setup->fieldSituation.unk12 = SEQ_BGM_WIN2;
        }
        if (param12 - 2 <= 1) {
            param->setup->fieldSituation.env.terrain = 0xe;
            param->setup->fieldSituation.env.bgType = 0x12;
            param->players->unk50 = 1;
        }
    }
    BtlSetup_SetParty(param->setup, sys->party, 0);
    BtlSetup_ApplyRegulation(param->setup, regulation, HEAPID_TAIL(heapId));
    if (sys->unk8AC) {
        param->setup->unkDD_2 = 1;
    }
    BtlSetup_AllocRecBuffer(param->setup, heapId);
    GFL_OvlUnload(OVERLAY_ID(167));
    if (ownsRegulation) {
        GFL_HeapFree(regulation);
    }
    return param;
}

static BOOL WifiBattleMatchSys_FinishBattle(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys) {
    WifiBattleMatchBattleParam *battle = param;
    BtlSetup *setup = battle->setup;
    u32 result = battle->result;
    Regulation *regulation;
    void *entry;
    u32 value;
    u8 aliveOther;
    u8 aliveOwn;
    u8 hpPercent;

    sys->battleResult.battleStyle = setup->battleStyle;
    sys->battleResult.result = setup->unkA8;
    sys->battleResult.unk10 = setup->unkDD_0;
    sys->battleResult.result = setup->unkA8;
    WifiBattleMatchSys_CountAlive(battle->setup, &aliveOwn, &aliveOther);
    sys->battleResult.unkA = aliveOwn;
    sys->battleResult.unk8 = aliveOther;
    WifiBattleMatchSys_GetHpPercents(battle->setup, &hpPercent, &sys->battleResult.unk9);
    func_0200c1f0();
    BattleRecTool_StoreSetup(battle->setup);
    func_0200c200();
    if (sys->mode == 0) {
        entry = TournamentBlock_GetEntry(getTournamentBlockAddress(GameData_GetSaveControl(sys->param.gameData)), 0);
        regulation = func_0200b530(entry);
        value = TournamentEntry_GetField(entry, 2);
        if (result == 0) {
            WifiBattleMatchSys_BuildReport(&sys->report, sys->party, sys->otherParty, sys->otherPlayer, regulation,
                                           value, setup, &sys->battleResult, 0);
        }
    } else if (sys->mode == 2) {
        regulation = PokeRegulation_LoadNew(sys->param.unk4 + 0xf, HEAPID_WIFIBATTLEMATCH);
        if (result == 0) {
            WifiBattleMatchSys_BuildReport(&sys->report, sys->party, sys->otherParty, sys->otherPlayer, regulation, 0,
                                           setup, &sys->battleResult, 2);
        }
        GFL_HeapFree(regulation);
    }
    BtlSetup_Free(battle->setup);
    GFL_HeapFree(battle->players);
    GFL_HeapFree(battle);
    if (sys->mode == 1) {
        switch (result) {
        case 0:
            WifiBattleMatchSys_ChainSetNext(chain, STEP_CHAMPIONSHIP);
            break;
        case 1:
            sys->battleResult.unkC = TRUE;
            WifiBattleMatchSys_ChainSetNext(chain, STEP_CHAMPIONSHIP);
            break;
        }
    } else {
        switch (result) {
        case 0:
            sys->result = 1;
            WifiBattleMatchSys_ChainSetNext(chain, STEP_NET);
            break;
        case 2:
            sys->relogin = TRUE;
            WifiBattleMatchSys_ChainSetNext(chain, STEP_LOGIN);
            break;
        case 3:
            sys->result = 2;
            WifiBattleMatchSys_ChainSetNext(chain, STEP_NET);
            break;
        }
    }
    return TRUE;
}

static void *WifiBattleMatchSys_CreateLoginParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys) {
    WifiLoginParam *param = GFL_HeapAllocate(heapId, sizeof(WifiLoginParam), FALSE, "wifibattlematch_sys.c", 1415);
    u32 serviceId;

    sys_memset(param, 0, sizeof(WifiLoginParam));
    param->gameData = sys->param.gameData;
    param->unk4 = 0;
    param->unk8 = 1;
    param->buffer = sys->unk40;
    if (sys->param.unk8 == 0) {
        serviceId = 0x24;
    } else {
        serviceId = 0x34;
    }
    param->unkC = serviceId;
    if (sys->relogin) {
        sys->relogin = FALSE;
        param->unk14 = TRUE;
    } else {
        param->unk14 = FALSE;
    }
    return param;
}

static BOOL WifiBattleMatchSys_FinishLogin(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys) {
    WifiLoginParam *login = param;
    u32 result = login->result;

    GFL_HeapFree(login);
    switch (result) {
    case 0:
        sys->result = 0;
        WifiBattleMatchSys_ChainSetNext(chain, STEP_NET);
        break;
    case 1:
        if (sys->param.unk8 == 0) {
            WifiBattleMatchSys_ChainSetNext(chain, STEP_CHAMPIONSHIP);
        } else {
            WifiBattleMatchSys_ChainEnd(chain);
        }
        break;
    }
    return TRUE;
}

static void *WifiBattleMatchSys_CreateExchangeParam(WifiBattleMatchChain *chain, HeapID heapId,
                                                    WifiBattleMatchSys *sys) {
    WifiBattleMatchExchangeParam *param =
        GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchExchangeParam), FALSE, "wifibattlematch_sys.c", 1499);

    sys_memset(param, 0, sizeof(WifiBattleMatchExchangeParam));
    param->gameData = &sys->param.gameData;
    param->party = sys->party;
    param->otherParty = sys->otherParty;
    param->unk14 = sys->unk7D4;
    param->unk18 = &sys->unk8A8;
    if (sys->mode == 1) {
        param->mode = WIFIBATTLEMATCH_EXCHANGE_IRC;
    } else {
        param->mode = WIFIBATTLEMATCH_EXCHANGE_WIFI;
    }
    return param;
}

static BOOL WifiBattleMatchSys_FinishExchange(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys) {
    WifiBattleMatchExchangeParam *exchange = param;
    u32 result = exchange->result;

    GFL_HeapFree(exchange);
    if (sys->mode == 1) {
        switch (result) {
        case 0:
            WifiBattleMatchSys_ChainSetNext(chain, STEP_BATTLE);
            break;
        case 2:
            sys->battleResult.unkC = TRUE;
            WifiBattleMatchSys_ChainSetNext(chain, STEP_CHAMPIONSHIP);
            break;
        }
    } else {
        switch (result) {
        case 0:
            WifiBattleMatchSys_ChainSetNext(chain, STEP_BATTLE);
            break;
        case 1:
            sys->relogin = TRUE;
            WifiBattleMatchSys_ChainSetNext(chain, STEP_LOGIN);
            break;
        case 3:
            sys->result = 2;
            WifiBattleMatchSys_ChainSetNext(chain, STEP_NET);
            break;
        }
    }
    return TRUE;
}

static void *WifiBattleMatchSys_CreateResultParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys) {
    Ov306Param *param = GFL_HeapAllocate(heapId, sizeof(Ov306Param), FALSE, "wifibattlematch_sys.c", 1590);
    u32 battleType;
    u32 index;
    u32 value;
    int param0D;
    BOOL flag = TRUE;
    Regulation *regulation;

    sys_memset(param, 0, sizeof(Ov306Param));
    if (sys->mode == 3) {
        battleType = sys->param.unk4 + 0x17;
    } else if (sys->mode == 2) {
        battleType = sys->param.unk4 + 0x1c;
    } else {
        if (sys->mode == 0) {
            index = 0;
        } else if (sys->mode == 1) {
            index = 1;
        } else {
            GFL_ASSERT(0);
        }
        regulation = func_0200b530(
            TournamentBlock_GetEntry(getTournamentBlockAddress(GameData_GetSaveControl(sys->param.gameData)), index));
        param0D = Regulation_GetParam(regulation, 0xd);
        value = Regulation_GetParam(regulation, 0x15);
        if (param0D >= 2) {
            param0D = 1;
        }
        battleType = value * 2 + 0x21 + param0D;
    }
    param->gameData = sys->param.gameData;
    param->unk4 = flag;
    param->unk8 = FALSE;
    param->unk10 = battleType;
    param->unk14 = 0;
    if (sys->mode != 1) {
        if (sys->battleResult.unk10 != 0 || sys->battleResult.unk14 != 0) {
            flag = FALSE;
        }
    }
    param->unkC = flag;
    if (func_0200c1d0(sys->otherPlayer->unk44)) {
        param->unk4 = TRUE;
    } else {
        param->unk4 = FALSE;
    }
    return param;
}

static BOOL WifiBattleMatchSys_FinishResult(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys) {
    GFL_HeapFree(param);
    if (sys->mode == 1) {
        WifiBattleMatchSys_ChainSetNext(chain, STEP_CHAMPIONSHIP);
    } else {
        sys->result = 3;
        WifiBattleMatchSys_ChainSetNext(chain, STEP_NET);
    }
    return TRUE;
}

static void *WifiBattleMatchSys_CreateLogoutParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys) {
    WifiLogoutParam *param = GFL_HeapAllocate(heapId, sizeof(WifiLogoutParam), FALSE, "wifibattlematch_sys.c", 1713);

    sys_memset(param, 0, sizeof(WifiLogoutParam));
    param->gameData = sys->param.gameData;
    param->unk4 = 0;
    param->unk8 = 1;
    if (sys->unk24) {
        param->unk10 = 0x12;
    }
    return param;
}

static BOOL WifiBattleMatchSys_FinishLogout(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys) {
    switch (sys->mode) {
    case 0:
        if (sys->unk24 != 0) {
            WifiBattleMatchSys_ChainEnd(chain);
        } else {
            WifiBattleMatchSys_ChainSetNext(chain, STEP_CHAMPIONSHIP);
        }
        break;
    case 1:
        GFL_ASSERT(0);
        break;
    case 2:
    case 3:
        WifiBattleMatchSys_ChainEnd(chain);
        break;
    }
    GFL_HeapFree(param);
    return TRUE;
}

static void *WifiBattleMatchSys_CreateReplayParam(WifiBattleMatchChain *chain, HeapID heapId, WifiBattleMatchSys *sys) {
    BtlSetup *setup = GFL_HeapAllocate(heapId, sizeof(BtlSetup), FALSE, "wifibattlematch_sys.c", 1779);

    sys_memset(setup, 0, sizeof(BtlSetup));
    GFL_OvlLoad(OVERLAY_ID(167));
    BtlSetup_Reset(setup);
    BtlSetup_InitForRecord(setup, sys->param.gameData, heapId);
    GFL_OvlUnload(OVERLAY_ID(167));
    func_0200c1f0();
    BattleRecTool_LoadSetup(setup, 0, heapId);
    func_0200c200();
    GFL_OvlLoad(OVERLAY_ID(167));
    func_02005d8c();
    GFL_SndBGMPlay(setup->fieldSituation.bgm, SND_CHANNEL_MASK_ALL);
    return setup;
}

static BOOL WifiBattleMatchSys_FinishReplay(WifiBattleMatchChain *chain, void *param, WifiBattleMatchSys *sys) {
    BtlSetup *setup = param;

    func_02005d8c();
    GFL_HeapFree(setup->unk34[0]);
    BtlSetup_FreeForRecord(setup);
    GFL_HeapFree(setup);
    GFL_OvlUnload(OVERLAY_ID(167));
    WifiBattleMatchSys_ChainSetNext(chain, STEP_CHAMPIONSHIP);
    return TRUE;
}

static void WifiBattleMatchSys_AllocPlayers(WifiBattleMatchSys *sys, HeapID heapId) {
    sys->player = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchPlayer) + PokeParty_GetSaveDataSize(), FALSE,
                                   "wifibattlematch_sys.c", 1836);
    sys_memset(sys->player, 0, sizeof(WifiBattleMatchPlayer) + PokeParty_GetSaveDataSize());
    sys->otherPlayer = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchPlayer) + PokeParty_GetSaveDataSize(), FALSE,
                                        "wifibattlematch_sys.c", 1838);
    sys_memset(sys->otherPlayer, 0, sizeof(WifiBattleMatchPlayer) + PokeParty_GetSaveDataSize());
}

static void WifiBattleMatchSys_FreePlayers(WifiBattleMatchSys *sys) {
    GFL_HeapFree(sys->player);
    GFL_HeapFree(sys->otherPlayer);
}

static void WifiBattleMatchSys_BuildReport(WifiBattleMatchReport *report, PokeParty *party, PokeParty *otherParty,
                                           WifiBattleMatchPlayer *otherPlayer, Regulation *regulation, u32 unk14,
                                           BtlSetup *setup, WifiBattleMatchBattleResult *battleResult, u32 unk10) {
    RTCDate date;
    RTCTime time;
    u8 alive[2];
    PartyPkm *pkm;
    BOOL ret;
    int i;

    sys_memset(report, 0, sizeof(WifiBattleMatchReport));
    report->profileId = func_02008bdc(&otherPlayer->info);
    report->unk4 = otherPlayer->unk48;
    for (i = 0; i < PokeParty_GetPkmCount(otherParty); i++) {
        pkm = PokeParty_GetPkm(otherParty, i);
        if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            report->species[i] = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
            report->forms[i] = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
            report->levels[i] = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
            report->genders[i] = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
        }
    }
    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        pkm = PokeParty_GetPkm(party, i);
        if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            report->ownSpecies[i] = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
            report->ownForms[i] = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
            report->ownLevels[i] = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
            report->ownGenders[i] = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
        }
    }
    ret = func_ov011_0215dda8(&date, &time);
    GFL_ASSERT(ret);
    report->year = date.year;
    report->month = date.month;
    report->day = date.day;
    report->hour = time.hour;
    report->minute = time.minute;
    report->unk48 = unk14;
    report->result = setup->unkA8;
    WifiBattleMatchSys_CountAlive(setup, &alive[0], &alive[1]);
    report->aliveOwn = alive[0];
    report->aliveOther = alive[1];
    report->battleType = Regulation_GetParam(regulation, 0x15);
    report->unkFlag = Regulation_GetParam(regulation, 0xd);
}

static void WifiBattleMatchSys_CountAlive(BtlSetup *setup, u8 *own, u8 *other) {
    int i;
    u32 client = setup->fieldSituation.unk19 & 1;
    u32 index;
    int j;
    int count;

    *own = 0;
    *other = 0;
    for (i = 0; i <= 1; i++) {
        if ((i & 1) == client) {
            if (client) {
                index = (i & 2) + 1;
            } else {
                index = i & 2;
            }
            count = PokeParty_GetPkmCount(setup->party[index]);
            for (j = 0; j < count; j++) {
                if (setup->unkE7[index][j] != 2) {
                    (*own)++;
                }
            }
        } else {
            if (client) {
                index = i & 2;
            } else {
                index = (i & 2) + 1;
            }
            count = PokeParty_GetPkmCount(setup->party[index]);
            for (j = 0; j < count; j++) {
                if (setup->unkE7[index][j] != 2) {
                    (*other)++;
                }
            }
        }
    }
}

static void WifiBattleMatchSys_GetHpPercents(BtlSetup *setup, u8 *own, u8 *other) {
    int i;
    u32 client = setup->fieldSituation.unk19 & 1;
    u32 index;

    *own = 0;
    *other = 0;
    for (i = 0; i <= 1; i++) {
        if ((i & 1) == client) {
            if (client) {
                index = (i & 2) + 1;
            } else {
                index = i & 2;
            }
            *own = setup->unk100[index];
        } else {
            if (client) {
                index = i & 2;
            } else {
                index = (i & 2) + 1;
            }
            *other = setup->unk100[index];
        }
    }
}

static WifiBattleMatchChain *WifiBattleMatchSys_ChainCreate(const WifiBattleMatchStep *steps, u32 stepCount,
                                                            WifiBattleMatchSys *sys, HeapID heapId) {
    WifiBattleMatchChain *chain =
        GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchChain), FALSE, "wifibattlematch_sys.c", 2117);

    sys_memset(chain, 0, sizeof(WifiBattleMatchChain));
    chain->procManager = CreateGameProcManager(heapId);
    chain->sys = sys;
    chain->steps = steps;
    chain->stepCount = stepCount;
    chain->heapId = heapId;
    return chain;
}

static void WifiBattleMatchSys_ChainFree(WifiBattleMatchChain *p_wk) {
    GFL_ASSERT(p_wk->p_param == NULL);
    FreeGameProcManager(p_wk->procManager);
    GFL_HeapFree(p_wk);
}

static BOOL WifiBattleMatchSys_ChainUpdate(WifiBattleMatchChain *chain) {
    const WifiBattleMatchStep *step;

    switch (chain->state) {
    case 0:
        chain->previous = chain->current;
        chain->current = chain->next;
        chain->state = 1;
        break;
    case 1:
        step = &chain->steps[chain->current];
        if (step->create != NULL) {
            chain->p_param = step->create(chain, chain->heapId, chain->sys);
        } else {
            chain->p_param = NULL;
        }
        QueueGameProc(chain->procManager, chain->steps[chain->current].overlayId,
                      chain->steps[chain->current].functions, chain->p_param);
        chain->state = 2;
        break;
    case 2:
        chain->procResult = GFL_ProcMgrUpdate(chain->procManager);
        if (chain->procResult == 0) {
            chain->state = 3;
        }
        break;
    case 3:
        step = &chain->steps[chain->current];
        if (step->finish != NULL) {
            if (!step->finish(chain, chain->p_param, chain->sys)) {
                break;
            }
            chain->p_param = NULL;
            chain->state = 4;
        } else {
            chain->state = 4;
        }
        break;
    case 4:
        if (chain->next == 0xff) {
            chain->state = 5;
        } else {
            chain->state = 0;
        }
        break;
    case 5:
        return TRUE;
    }
    return FALSE;
}

static u32 WifiBattleMatchSys_ChainGetResult(WifiBattleMatchChain *chain) {
    return chain->procResult;
}

static u8 WifiBattleMatchSys_ChainGetPrevious(WifiBattleMatchChain *chain) {
    return chain->previous;
}

static void WifiBattleMatchSys_ChainSetNext(WifiBattleMatchChain *chain, u8 step) {
    chain->next = step;
}

static void WifiBattleMatchSys_ChainEnd(WifiBattleMatchChain *chain) {
    chain->next = 0xff;
}

static BOOL WifiBattleMatchSys_BattleProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchBattleParam *battle = param;
    WifiBattleMatchBattleWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_BATTLE, 0x150);
    wk = GFL_ProcInitSubsystem(proc, sizeof(WifiBattleMatchBattleWork), HEAPID_WIFIBATTLEMATCH_BATTLE);
    sys_memset(wk, 0, sizeof(WifiBattleMatchBattleWork));
    battle->result = 0;
    wk->manager = CreateGameProcManager(HEAPID_WIFIBATTLEMATCH_BATTLE);
    GFL_SndBGMFadeOut(0x10);
    return TRUE;
}

static BOOL WifiBattleMatchSys_BattleProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchBattleWork *wk = work;

    if (wk->ov167Loaded) {
        GFL_OvlUnload(OVERLAY_ID(167));
        wk->ov167Loaded = FALSE;
    }
    if (wk->commandsSet) {
        func_02040c64(0x100);
        wk->commandsSet = FALSE;
    }
    FreeGameProcManager(wk->manager);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFIBATTLEMATCH_BATTLE);
    return TRUE;
}

static BOOL WifiBattleMatchSys_BattleProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchBattleParam *battle = param;
    WifiBattleMatchBattleWork *wk = work;
    int running = GFL_ProcMgrUpdate(wk->manager);
    BOOL queue;
    int last;
    u32 first;
    u32 second;
    int i;
    int j;
    u32 client;
    u32 source;

    switch (*state) {
    case 0:
        GFL_SndBGMPlay(battle->setup->fieldSituation.bgm, SND_CHANNEL_MASK_ALL);
        battle->players->base.unk44 = 1;
        QueueGameProc(wk->manager, OVERLAY_ID(305), &CommBtlDemoProc, battle->players);
        running = 2;
        *state = 1;
        break;
    case 1:
        if (running == TRUE) {
            break;
        }
        *state = 2;
        break;
    case 2:
        GFL_OvlLoad(OVERLAY_ID(167));
        wk->ov167Loaded = TRUE;
        func_02040c20(0x100, data_ov167_021d7448, 9, NULL);
        wk->commandsSet = TRUE;
        func_02040624(func_02040440(), 0xc8, 0x24);
        *state = 3;
        break;
    case 3:
        if (func_02040664(func_02040440(), 0xc8, 0x24)) {
            *state = 4;
        }
        break;
    case 4:
        QueueGameProc(wk->manager, OVERLAY_NONE, &data_ov167_021d6ce0, battle->setup);
        running = 2;
        *state = 5;
        break;
    case 5:
        if (running == TRUE) {
            break;
        }
        *state = 6;
        break;
    case 6:
        *state = 7;
        break;
    case 7:
        func_02040624(func_02040440(), 0xd2, 0x24);
        wk->timer = 0;
        *state = 8;
        break;
    case 8:
        if (func_02040664(func_02040440(), 0xd2, 0x24)) {
            func_02040c64(0x100);
            wk->commandsSet = FALSE;
            GFL_OvlUnload(OVERLAY_ID(167));
            wk->ov167Loaded = FALSE;
            *state = 9;
        }
        if (func_02042788()) {
            if (func_02042e84()->bNetType == NET_TYPE_IRC) {
                if (wk->timer++ > 600) {
                    func_02040c64(0x100);
                    wk->commandsSet = FALSE;
                    GFL_OvlUnload(OVERLAY_ID(167));
                    wk->ov167Loaded = FALSE;
                    *state = 9;
                    wk->timer = 0;
                }
            }
        }
        break;
    case 9:
        func_02040624(func_02040440(), 0xd3, 0x24);
        *state = 10;
        wk->timer = 0;
        break;
    case 10:
        if (func_02040664(func_02040440(), 0xd3, 0x24)) {
            *state = 11;
        }
        if (func_02042788()) {
            if (func_02042e84()->bNetType == NET_TYPE_IRC) {
                if (wk->timer++ > 600) {
                    *state = 11;
                    wk->timer = 0;
                }
            }
        }
        break;
    case 11:
        queue = TRUE;
        if (battle->setup->fieldSituation.unk1a == 0) {
            battle->players->base.unk44 = 2;
        } else {
            battle->players->base.unk44 = 4;
        }
        switch (battle->setup->unkA8) {
        case 1:
        case 4:
            battle->players->base.result = 0;
            break;
        case 0:
        case 3:
            battle->players->base.result = 1;
            break;
        case 2:
            battle->players->base.result = 2;
            break;
        case 6:
            wk->timer = 0;
            queue = FALSE;
            *state = 15;
            break;
        default:
            battle->players->base.result = 2;
            break;
        }
        if (battle->setup->fieldSituation.unk1a == 0) {
            last = 1;
            first = 0;
            second = 1;
        } else {
            last = 3;
            first = 0;
            second = 2;
        }
        client = battle->setup->fieldSituation.unk19;
        for (i = 0; i <= last; i++) {
            if ((i & 1) == (client & 1)) {
                if (client & 1) {
                    source = (i & 2) + 1;
                } else {
                    source = i & 2;
                }
                for (j = 0; j < 6; j++) {
                    battle->players->base.players[first + (i >> 1)].order[j] = battle->setup->unkE7[source][j];
                }
            } else {
                if (client & 1) {
                    source = i & 2;
                } else {
                    source = (i & 2) + 1;
                }
                for (j = 0; j < 6; j++) {
                    battle->players->base.players[second + (i >> 1)].order[j] = battle->setup->unkE7[source][j];
                }
            }
        }
        if (queue) {
            QueueGameProc(wk->manager, OVERLAY_ID(305), &CommBtlDemoProc, battle->players);
            running = 2;
            *state = 12;
        }
        break;
    case 12:
        if (running == TRUE) {
            break;
        }
        *state = 14;
        break;
    case 13:
        break;
    case 14:
        return TRUE;
    case 15:
        if (wk->timer++ > 1800) {
            GFL_NetErrMarkShown();
            battle->result = 2;
            return TRUE;
        }
        break;
    }
    if (func_02042788() && running != TRUE) {
        if (func_02042e84()->bNetType == NET_TYPE_IRC) {
            if (GFL_NetErrCheck()) {
                GFL_NetErrMarkShown();
                battle->result = 1;
                return TRUE;
            }
        } else {
            switch (DWCRapCommon_CheckError(TRUE, TRUE)) {
            case 0:
                break;
            case 1:
            case 2:
            case 3:
            default:
                battle->result = 2;
                return TRUE;
            case 4:
                battle->result = 3;
                return TRUE;
            }
        }
    }
    return FALSE;
}
