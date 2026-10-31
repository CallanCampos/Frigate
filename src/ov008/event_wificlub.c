#include "types.h"
#include "app/comm_tvt.h"
#include "app/pokemon_trade.h"
#include "app/ui/ui_scene.h"
#include "app/wifi_login.h"
#include "app/wifi_status.h"
#include "app/wificlub.h"
#include "battle/battle_proc.h"
#include "battle/battle_select.h"
#include "battle/btl_net.h"
#include "battle/btl_setup.h"
#include "battle/regulation.h"
#include "constants/sound.h"
#include "demo/evolution_demo.h"
#include "field/event_wificlub.h"
#include "field/field_event.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "save/wifi_list.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/ringtone_sys.h"

enum {
    STEP_SAVE_BGM = 0,
    STEP_CLOSE_FIELD = 1,
    STEP_START = 2,
    STEP_NET_END_WAIT = 4,
    STEP_OPEN_FIELD = 5,
    STEP_ENTRANCE_TRANSITION = 6,
    STEP_DONE = 7,
    STEP_CLEAR_LOGIN = 8,
    STEP_LOGIN = 9,
    STEP_LOGIN_WAIT = 10,
    STEP_LOGOUT = 30,
    STEP_LOGOUT_WAIT = 31,
    STEP_CLUB = 11,
    STEP_CLUB_RESULT = 12,
    STEP_BATTLE_SELECT = 13,
    STEP_BATTLE_SELECT_RESULT = 14,
    STEP_BATTLE_SYNC = 15,
    STEP_BATTLE_START = 16,
    STEP_BATTLE_END = 17,
    STEP_TRADE_INIT = 18,
    STEP_TRADE = 19,
    STEP_TRADE_RESULT = 20,
    STEP_EVOLVE = 21,
    STEP_EVOLVE_END = 22,
    STEP_TVT = 23,
    STEP_TVT_END = 24,
    STEP_NET_EXIT = 28,
    STEP_NET_EXIT_WAIT = 29,
    STEP_LEAVE = 25,
    STEP_LEAVE_2 = 26,
    STEP_LEAVE_3 = 27,
};

typedef struct {
    u8 battleMode;
    u16 nextState;
} WifiClubMode;

typedef struct {
    GameEvent *event;
    GameSystem *gsys;
    Field *field;
    void *self;
    u32 unk10;
    BOOL useTransitions;
    u32 unk18;
    GameProcManager *procManager;
    BattleSelectParam select;
    CommTvtParam tvt;
    WifiClubData *club;
    PokemonTradeParam trade;
    GameData *gameData;
    WifiList *wifiList;
    WifiLoginParam login;
    WifiLogoutParam logout;
    BtlSetup *btlSetup;
    BattlePlayers players;
    u8 unk130[4];
    GameRecords *records;
    BattleParam battle;
    PokeParty *party;
    u32 unk16C;
    u16 bgm;
    u8 battleMode;
} EventWifiClub;

void EventWifiClub_Free(EventWifiClub *wk);
GameEventReturnCode EventWifiClub_Callback(GameEvent *event, u32 *state, void *data);
void EventWifiClub_Init(GameEvent *event, GameSystem *gsys, Field *field, BOOL useTransitions);

void EventWifiClub_SetupBattle(EventWifiClub *wk, u32 mode);
void EventWifiClub_SetupBattleSelect(EventWifiClub *wk, GameData *gameData, u32 unused);
void EventWifiClub_SetBattleParty(EventWifiClub *wk, GameData *gameData, u32 unused);
void EventWifiClub_ResetForLogin(EventWifiClub *wk);

enum {
    WIFICLUB_BATTLE_SINGLE = 7,
    WIFICLUB_BATTLE_SINGLE_FLAT,
    WIFICLUB_BATTLE_DOUBLE,
    WIFICLUB_BATTLE_DOUBLE_FLAT,
    WIFICLUB_BATTLE_TRIPLE,
    WIFICLUB_BATTLE_TRIPLE_FLAT,
    WIFICLUB_BATTLE_ROTATION,
    WIFICLUB_BATTLE_ROTATION_FLAT,
};

static WifiClubMode sWifiClubModes[16] = {
    { 0, STEP_LEAVE },          { 0, STEP_LOGIN },          { 0, STEP_LEAVE },
    { 0, STEP_LEAVE },          { 0, STEP_LEAVE },          { 0, STEP_TVT },
    { 0, STEP_TRADE_INIT },     { 7, STEP_BATTLE_SELECT },  { 8, STEP_BATTLE_SELECT },
    { 9, STEP_BATTLE_SELECT },  { 10, STEP_BATTLE_SELECT }, { 11, STEP_BATTLE_SELECT },
    { 12, STEP_BATTLE_SELECT }, { 13, STEP_BATTLE_SELECT }, { 14, STEP_BATTLE_SELECT },
    { 0, STEP_LEAVE },
};

void EventWifiClub_SetupBattle(EventWifiClub *wk, u32 mode) {
    GameData *gameData = GSYS_GetGameData(wk->gsys);
    u8 launcher = wk->club->launcher;
    u32 rule;

    if (launcher) {
        Regulation_SetParam(wk->club->regulation, REGULATION_PARAM_LAUNCHER, TRUE);
    } else {
        Regulation_SetParam(wk->club->regulation, REGULATION_PARAM_LAUNCHER, FALSE);
    }

    switch (mode) {
    case WIFICLUB_BATTLE_SINGLE_FLAT:
        rule = 1;
        if (launcher) {
            rule = 3;
        }
        break;
    case WIFICLUB_BATTLE_SINGLE:
        rule = 0;
        if (launcher) {
            rule = 2;
        }
        break;
    case WIFICLUB_BATTLE_DOUBLE_FLAT:
        rule = 5;
        if (launcher) {
            rule = 7;
        }
        break;
    case WIFICLUB_BATTLE_DOUBLE:
        rule = 4;
        if (launcher) {
            rule = 6;
        }
        break;
    case WIFICLUB_BATTLE_TRIPLE_FLAT:
        rule = 9;
        if (launcher) {
            rule = 11;
        }
        break;
    case WIFICLUB_BATTLE_TRIPLE:
        rule = 8;
        if (launcher) {
            rule = 10;
        }
        break;
    case WIFICLUB_BATTLE_ROTATION_FLAT:
        rule = 13;
        if (launcher) {
            rule = 15;
        }
        break;
    case WIFICLUB_BATTLE_ROTATION:
        rule = 12;
        if (launcher) {
            rule = 14;
        }
        break;
    }
    wk->battle.rule = rule;
    wk->battle.unk10 = 0;

    switch (mode) {
    case WIFICLUB_BATTLE_SINGLE:
    case WIFICLUB_BATTLE_SINGLE_FLAT:
        BtlSetup_SetNet1v1Single(wk->btlSetup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    case WIFICLUB_BATTLE_DOUBLE:
    case WIFICLUB_BATTLE_DOUBLE_FLAT:
        BtlSetup_SetNet1v1Double(wk->btlSetup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    case WIFICLUB_BATTLE_TRIPLE:
    case WIFICLUB_BATTLE_TRIPLE_FLAT:
        BtlSetup_SetNetTriple(wk->btlSetup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    case WIFICLUB_BATTLE_ROTATION:
    case WIFICLUB_BATTLE_ROTATION_FLAT:
        BtlSetup_SetNetRotation(wk->btlSetup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    }

    BtlSetup_AllocRecBuffer(wk->btlSetup, 1);
    BtlSetup_ApplyRegulation(wk->btlSetup, wk->club->regulation, HEAPID_GAMEEVENT);
    PokeRegulation_ApplyLevelToParty(wk->club->regulation, wk->club->parties[0]);
    PokeRegulation_ApplyLevelToParty(wk->club->regulation, wk->club->parties[1]);
    wk->btlSetup->records = GameData_GetRecords(GSYS_GetGameData(wk->gsys));
}

void EventWifiClub_SetupBattleSelect(EventWifiClub *wk, GameData *gameData, u32 unused) {
    u32 netId = GFL_NetGetHandleNetId(NetHandle_GetOwn());
    u32 otherNetId = 1 - netId;
    PlayerInfo *other = GameData_GetPlayerInfoByNetID(gameData, otherNetId);
    BattleSelectParam *select = &wk->select;

    select->regulation = wk->club->regulation;
    select->party = wk->club->parties[netId];
    select->otherName = GetPlayerName(other);
    select->otherGender = getTrainerGender(other);
    select->otherParty = wk->club->parties[otherNetId];
    select->gameData = gameData;
    select->unk18 = 0;
    PokeParty_Init(wk->party);
    select->pickedParty = wk->party;
    select->parties[0] = wk->club->parties[0];
    select->parties[1] = wk->club->parties[1];
}

void EventWifiClub_SetBattleParty(EventWifiClub *wk, GameData *gameData, u32 unused) {
    BtlSetup_SetParty(wk->btlSetup, wk->party, 0);
}

// Resets the volume before logging in again
void EventWifiClub_ResetForLogin(EventWifiClub *wk) {
    GFL_SndPlayerSetVolumeEx(SND_VOLUME_MAX, SND_PLAYER_MASK_ALL);
    PokeVoice_SetMasterVolume(SND_VOLUME_MAX);
    wk->login.unk14 = 1;
}

GameEventReturnCode EventWifiClub_Callback(GameEvent *event, u32 *state, void *data) {
    EventWifiClub *wk = data;
    GameSystem *gsys = wk->gsys;
    u32 seq = *state;

    switch (seq) {
    case STEP_SAVE_BGM:
        wk->bgm = GFL_SndBGMGetID();
        GFL_SndBGMFadeOut(6);
        (*state)++;
        break;
    case STEP_CLOSE_FIELD:
        // Wait for the comm system to shut down
        if (!GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
            wk->unk10 = 0;
            GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, wk->field));
            (*state)++;
        }
        break;
    case STEP_START:
        GFL_SndSetVolumeControlCallbacks();
        *state = STEP_CLEAR_LOGIN;
        break;
    case STEP_NET_END_WAIT:
        if (GFL_NetIsEnded()) {
            (*state)++;
            EventWifiClub_Free(wk);
            PokeVoice_ResetMasterVolume();
            GFL_SndBGMPlay(wk->bgm, SND_CHANNEL_MASK_ALL);
            GFL_SndBGMFadeIn(60);
        }
        break;
    case STEP_OPEN_FIELD:
        RingtoneSys_RestoreLidCallbacks();
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case STEP_ENTRANCE_TRANSITION:
        if (wk->useTransitions) {
            GameEvent_ChainNext(
                event, CallFieldMapEntranceInTransition(gsys, wk->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        }
        (*state)++;
        break;
    case STEP_DONE:
        return GAMEEVENT_DONE;
    case STEP_CLEAR_LOGIN:
        MI_CpuFill8(&wk->login, 0, sizeof(WifiLoginParam));
        *state = STEP_LOGIN;
        wk->login.unk14 = 0;
        break;
    case STEP_LOGIN:
        wk->login.gameData = GSYS_GetGameData(gsys);
        wk->login.unk8 = 1;
        wk->login.unk4 = 0;
        wk->login.unkC = 10;
        wk->login.keepBgm = 1;
        GFL_SndBGMPlay(SEQ_BGM_WIFI_ACCESS, SND_CHANNEL_MASK_ALL);
        wk->procManager = CreateGameProcManager(HEAPID_GAMEEVENT);
        QueueGameProc(wk->procManager, OVERLAY_WIFILOGIN, &WIFILOGIN_PROC_FUNCTIONS, &wk->login);
        wk->club->mode = 0;
        *state = STEP_LOGIN_WAIT;
        break;
    case STEP_LOGIN_WAIT:
        if (GFL_ProcMgrUpdate(wk->procManager)) {
            GFL_NetErrShow(0);
            return GAMEEVENT_CONTINUE;
        }
        FreeGameProcManager(wk->procManager);
        if (wk->login.result == 1) {
            GFL_SndBGMFadeOut(6);
            *state = STEP_LEAVE;
        } else if (!WifiList_CountFriends(GameData_GetWifiList(wk->gameData))) {
            *state = STEP_LOGOUT;
        } else if (wk->login.result == 0) {
            *state = STEP_CLUB;
            GFL_SndBGMFadeOut(6);
        } else {
            GFL_SndBGMFadeOut(6);
            *state = STEP_LEAVE;
        }
        break;
    case STEP_LOGOUT:
        wk->logout.gameData = GSYS_GetGameData(gsys);
        wk->logout.unk8 = 1;
        wk->logout.unk4 = 0;
        wk->logout.keepBgm = 1;
        wk->logout.wipeFlags = 0;
        wk->logout.unk14 = 0;
        wk->logout.unk18 = 0;
        wk->procManager = CreateGameProcManager(HEAPID_GAMEEVENT);
        QueueGameProc(wk->procManager, OVERLAY_WIFILOGIN, &WIFILOGOUT_PROC_FUNCTIONS, &wk->logout);
        (*state)++;
        break;
    case STEP_LOGOUT_WAIT:
        if (GFL_ProcMgrUpdate(wk->procManager)) {
            GFL_NetErrShow(0);
            return GAMEEVENT_CONTINUE;
        }
        FreeGameProcManager(wk->procManager);
        GFL_SndBGMFadeOut(6);
        *state = STEP_NET_END_WAIT;
        break;
    case STEP_CLUB:
        GFL_OvlLoad(OVERLAY_WIFICLUB_MAIN);
        GFL_OvlLoad(OVERLAY_APP_UI);
        WIFI_STATUS_SetMyMac(wk->club->buffer);
        if (wk->btlSetup != NULL) {
            BtlSetup_Free(wk->btlSetup);
            wk->btlSetup = NULL;
        }
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_WIFICLUB, &WIFICLUB_PROC_FUNCTIONS, wk->club);
        (*state)++;
        break;
    case STEP_CLUB_RESULT:
        *state = sWifiClubModes[wk->club->mode].nextState;
        if (wk->club->mode == 1) {
            wk->login.unk14 = 1;
            EventWifiClub_ResetForLogin(wk);
        }
        wk->battleMode = sWifiClubModes[wk->club->mode].battleMode;
        GFL_OvlUnload(OVERLAY_APP_UI);
        GFL_OvlUnload(OVERLAY_WIFICLUB_MAIN);
        break;
    case STEP_BATTLE_SELECT:
        EventWifiClub_SetupBattleSelect(wk, GSYS_GetGameData(gsys), seq);
        wk->btlSetup = BtlSetup_Create(HEAPID_GAMEEVENT);
        EventWifiClub_SetupBattle(wk, wk->club->mode);
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_BATTLE_SELECT, &BATTLE_SELECT_PROC_FUNCTIONS, &wk->select);
        (*state)++;
        break;
    case STEP_BATTLE_SELECT_RESULT:
        if (wk->select.result == 1) {
            if (GFL_NetIsInitialized()) {
                wk->club->unk49 = 0;
                *state = STEP_CLUB;
            } else {
                EventWifiClub_ResetForLogin(wk);
                *state = STEP_LOGIN;
            }
        } else {
            EventWifiClub_SetBattleParty(wk, GSYS_GetGameData(gsys), seq);
            GFL_OvlLoad(OVERLAY_BATTLE_MAIN);
            NetCommand_AddTable(0x100, BTL_NET_COMMANDS, 9, 0);
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 100, 10);
            (*state)++;
        }
        break;
    case STEP_BATTLE_SYNC:
        if (!GFL_NetIsInitialized() || GFL_NetErrCheck()) {
            GFL_NetErrShutdown();
            GFL_NetErrShowScreen();
            EventWifiClub_ResetForLogin(wk);
            *state = STEP_LOGIN;
        } else if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 100, 10)) {
            (*state)++;
        }
        break;
    case STEP_BATTLE_START: {
        int i;

        GFL_SndBGMPlay(SEQ_BGM_VS_TRAINER_WIFI, SND_CHANNEL_MASK_ALL);
        wk->players.demoType = 1;
        if (GFL_NetGetHandleNetId(NetHandle_GetOwn()) == 0) {
            for (i = 0; i < 2; i++) {
                wk->players.players[i].info = GameData_GetPlayerInfoByNetID(GSYS_GetGameData(gsys), i);
                wk->players.players[i].party = wk->club->parties[i];
            }
        } else {
            u8 order[2] = { 1, 0 };

            for (i = 0; i < 2; i++) {
                wk->players.players[order[i]].info = GameData_GetPlayerInfoByNetID(GSYS_GetGameData(gsys), i);
                wk->players.players[order[i]].party = wk->club->parties[i];
            }
        }
        wk->battle.gameData = GSYS_GetGameData(wk->gsys);
        wk->battle.setup = wk->btlSetup;
        wk->battle.players = &wk->players;
        wk->battle.markNetErrShown = TRUE;
        wk->records = GameData_GetRecords(GSYS_GetGameData(wk->gsys));
        GFL_OvlUnload(OVERLAY_BATTLE_MAIN);
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_BATTLE, &data_ov010_0215039c, &wk->battle);
        (*state)++;
        break;
    }
    case STEP_BATTLE_END:
        if (!GFL_NetIsInitialized()) {
            EventWifiClub_ResetForLogin(wk);
            *state = STEP_LOGIN;
        } else {
            if (wk->players.result == 0) {
                WifiList_AddFriendRecord(GameData_GetWifiList(GSYS_GetGameData(wk->gsys)), wk->club->friendIndex - 1, 1,
                                         0, 0);
            } else if (wk->players.result == 1) {
                WifiList_AddFriendRecord(GameData_GetWifiList(GSYS_GetGameData(wk->gsys)), wk->club->friendIndex - 1, 0,
                                         1, 0);
            }
            NetCommand_RemoveTable(0x100);
            *state = STEP_CLUB;
        }
        break;
    case STEP_TRADE_INIT:
        wk->trade.next = 0;
        wk->trade.friendIndex = wk->club->friendIndex;
        (*state)++;
        break;
    case STEP_TRADE:
        wk->trade.gameData = GSYS_GetGameData(gsys);
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_POKEMONTRADE, &POKEMONTRADE_WIFICLUB_PROC_FUNCTIONS, &wk->trade);
        (*state)++;
        break;
    case STEP_TRADE_RESULT:
        switch (wk->trade.next) {
        case TRADE_NEXT_EVOLVE:
            *state = STEP_EVOLVE;
            break;
        case TRADE_NEXT_NEGOTIATE:
            wk->club->mode = 1;
            *state = STEP_CLUB;
            break;
        default:
            *state = STEP_CLUB;
            break;
        }
        if (!GFL_NetIsInitialized()) {
            EventWifiClub_ResetForLogin(wk);
            *state = STEP_LOGIN;
        }
        break;
    case STEP_EVOLVE: {
        EvolutionDemoParam *evolution =
            GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(EvolutionDemoParam), FALSE, "event_wificlub.c", 582);

        evolution->gameData = GSYS_GetGameData(wk->gsys);
        evolution->party = wk->trade.party;
        evolution->species = wk->trade.evolveSpecies;
        evolution->partyIndex = 0;
        evolution->method = wk->trade.evolveMethod;
        evolution->unkC = 1;
        evolution->canCancel = FALSE;
        wk->trade.evolution = evolution;
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_EVOLUTION_DEMO, &EVOLUTION_DEMO_PROC_FUNCTIONS, evolution);
        *state = STEP_EVOLVE_END;
        break;
    }
    case STEP_EVOLVE_END:
        GFL_HeapFree(wk->trade.evolution);
        wk->trade.next = TRADE_NEXT_EVOLVE;
        *state = STEP_TRADE;
        if (!GFL_NetIsInitialized()) {
            EventWifiClub_ResetForLogin(wk);
            *state = STEP_LOGIN;
        }
        break;
    case STEP_TVT:
        wk->tvt.gameData = GSYS_GetGameData(gsys);
        wk->tvt.startMode = COMM_TVT_START_EXISTING;
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_COMM_TVT, &COMM_TVT_PROC_FUNCTIONS, &wk->tvt);
        (*state)++;
        break;
    case STEP_TVT_END:
        if (!GFL_NetIsInitialized()) {
            EventWifiClub_ResetForLogin(wk);
            *state = STEP_LOGIN;
        } else {
            *state = STEP_CLUB;
        }
        break;
    case STEP_NET_EXIT:
        GFL_NetExit(0);
        *state = STEP_NET_EXIT_WAIT;
        break;
    case STEP_NET_EXIT_WAIT:
        if (GFL_NetIsNotInitialized()) {
            *state = STEP_NET_END_WAIT;
        }
        break;
    case STEP_LEAVE:
    case STEP_LEAVE_2:
    case STEP_LEAVE_3:
        *state = STEP_NET_END_WAIT;
        break;
    }
    return GAMEEVENT_CONTINUE;
}

void EventWifiClub_Free(EventWifiClub *wk) {
    if (wk->btlSetup != NULL) {
        BtlSetup_Free(wk->btlSetup);
        wk->btlSetup = NULL;
    }
    GFL_HeapFree(wk->club->parties[0]);
    GFL_HeapFree(wk->club->parties[1]);
    GFL_HeapFree(wk->club->regulation);
    GFL_HeapFree(wk->party);
    GFL_HeapFree(wk->club->buffer);
    GFL_HeapFree(wk->club);
}

void EventWifiClub_Init(GameEvent *event, GameSystem *gsys, Field *field, BOOL useTransitions) {
    EventWifiClub *wk;

    if (GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
        GameCommSys_ExitReq(GSYS_GetGameCommSystem(gsys));
    }

    wk = GameEvent_GetData(event);
    wk->gsys = gsys;
    wk->field = field;
    wk->event = event;
    wk->useTransitions = useTransitions;
    wk->club = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_GAMEEVENT), sizeof(WifiClubData), TRUE, "event_wificlub.c", 683);
    wk->club->buffer = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_GAMEEVENT), 0x20, TRUE, "event_wificlub.c", 684);
    wk->club->gameData = GSYS_GetGameData(wk->gsys);
    wk->club->save = GameData_GetSaveControl(wk->club->gameData);
    wk->club->unk46 = 1;
    wk->club->unk49 = 0;
    wk->gameData = wk->club->gameData;
    wk->wifiList = GameData_GetWifiList(wk->gameData);
    wk->club->mode = 0;
    wk->party = PokeParty_Create(HEAPID_TAIL(HEAPID_GAMEEVENT));
    wk->trade.party = wk->party;
    wk->self = wk;
    wk->bgm = GFL_SndBGMGetID();
    wk->club->parties[0] = PokeParty_Create(HEAPID_TAIL(HEAPID_USER));
    wk->club->parties[1] = PokeParty_Create(HEAPID_TAIL(HEAPID_USER));
    wk->club->regulation = Regulation_Create(HEAPID_TAIL(HEAPID_USER));
    wk->club->unk20 = 1;
}

GameEvent *EventWifiClub_Create(GameSystem *gsys, Field *field, BOOL useTransitions) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWifiClub_Callback, sizeof(EventWifiClub));

    EventWifiClub_Init(event, gsys, field, useTransitions);
    return event;
}

// Called through GameEvent_CreateOverlayDelegate, by the NetConnectWiFiClub script command
GameEvent *EventWifiClub_CreateFromArgs(GameSystem *gsys, void *data) {
    EventWifiClubArgs *args = data;

    return EventWifiClub_Create(gsys, args->field, args->useTransitions);
}
