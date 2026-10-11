#include "types.h"
#include "app/comm_tvt.h"
#include "app/pokemon_trade.h"
#include "app/ui/ui_scene.h"
#include "app/wifi_login.h"
#include "app/wificlub.h"
#include "battle/battle_proc.h"
#include "battle/battle_select.h"
#include "battle/btl_net.h"
#include "battle/btl_setup.h"
#include "battle/regulation.h"
#include "constants/sound.h"
#include "demo/shinka_demo.h"
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

static WifiClubMode sWifiClubModes[16] = {
    { 0, 25 }, { 0, 9 },  { 0, 25 },  { 0, 25 },  { 0, 25 },  { 0, 23 },  { 0, 18 },  { 7, 13 },
    { 8, 13 }, { 9, 13 }, { 10, 13 }, { 11, 13 }, { 12, 13 }, { 13, 13 }, { 14, 13 }, { 0, 25 },
};

void EventWifiClub_SetupBattle(EventWifiClub *wk, u32 mode) {
    GameData *gameData = GSYS_GetGameData(wk->gsys);
    u8 unk48 = wk->club->unk48;
    u32 rule;

    if (unk48) {
        Regulation_SetParam(wk->club->regulation, 13, TRUE);
    } else {
        Regulation_SetParam(wk->club->regulation, 13, FALSE);
    }

    switch (mode) {
    case 8:
        rule = 1;
        if (unk48) {
            rule = 3;
        }
        break;
    case 7:
        rule = 0;
        if (unk48) {
            rule = 2;
        }
        break;
    case 10:
        rule = 5;
        if (unk48) {
            rule = 7;
        }
        break;
    case 9:
        rule = 4;
        if (unk48) {
            rule = 6;
        }
        break;
    case 12:
        rule = 9;
        if (unk48) {
            rule = 11;
        }
        break;
    case 11:
        rule = 8;
        if (unk48) {
            rule = 10;
        }
        break;
    case 14:
        rule = 13;
        if (unk48) {
            rule = 15;
        }
        break;
    case 13:
        rule = 12;
        if (unk48) {
            rule = 14;
        }
        break;
    }
    wk->battle.rule = rule;
    wk->battle.unk10 = 0;

    switch (mode) {
    case 7:
    case 8:
        BtlSetup_SetNet1v1Single(wk->btlSetup, gameData, func_02040440(), 1, HEAPID_GAMEEVENT);
        break;
    case 9:
    case 10:
        BtlSetup_SetNet1v1Double(wk->btlSetup, gameData, func_02040440(), 1, HEAPID_GAMEEVENT);
        break;
    case 11:
    case 12:
        BtlSetup_SetNetTriple(wk->btlSetup, gameData, func_02040440(), 1, HEAPID_GAMEEVENT);
        break;
    case 13:
    case 14:
        BtlSetup_SetNetRotation(wk->btlSetup, gameData, func_02040440(), 1, HEAPID_GAMEEVENT);
        break;
    }

    func_020186b0(wk->btlSetup, 1);
    func_02017d30(wk->btlSetup, wk->club->regulation, HEAPID_GAMEEVENT);
    func_0201f63c(wk->club->regulation, wk->club->parties[0]);
    func_0201f63c(wk->club->regulation, wk->club->parties[1]);
    wk->btlSetup->records = GameData_GetRecords(GSYS_GetGameData(wk->gsys));
}

void EventWifiClub_SetupBattleSelect(EventWifiClub *wk, GameData *gameData, u32 unused) {
    u32 netId = func_02042a6c(func_02040440());
    u32 otherNetId = 1 - netId;
    PlayerInfo *other = func_02017378(gameData, otherNetId);
    BattleSelectParam *select = &wk->select;

    select->regulation = wk->club->regulation;
    select->party = wk->club->parties[netId];
    select->otherName = GetPlayerName(other);
    select->otherGender = getTrainerGender(other);
    select->otherParty = wk->club->parties[otherNetId];
    select->gameData = gameData;
    select->unk18 = 0;
    PokeParty_Init(wk->party);
    select->unk1C = wk->party;
    select->parties[0] = wk->club->parties[0];
    select->parties[1] = wk->club->parties[1];
}

void EventWifiClub_SetBattleParty(EventWifiClub *wk, GameData *gameData, u32 unused) {
    func_02017cfc(wk->btlSetup, wk->party, 0);
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
    case 0:
        wk->bgm = GFL_SndBGMGetID();
        GFL_SndBGMFadeOut(6);
        (*state)++;
        break;
    case 1:
        // Wait for the comm system to shut down
        if (!GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
            wk->unk10 = 0;
            GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, wk->field));
            (*state)++;
        }
        break;
    case 2:
        GFL_SndSetVolumeControlCallbacks();
        *state = 8;
        break;
    case 4:
        if (func_020427a4()) {
            (*state)++;
            EventWifiClub_Free(wk);
            PokeVoice_ResetMasterVolume();
            GFL_SndBGMPlay(wk->bgm, SND_CHANNEL_MASK_ALL);
            GFL_SndBGMFadeIn(60);
        }
        break;
    case 5:
        RingtoneSys_RestoreLidCallbacks();
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case 6:
        if (wk->useTransitions) {
            GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, wk->field, 0, 0, 1, 0, 0));
        }
        (*state)++;
        break;
    case 7:
        return GAMEEVENT_DONE;
    case 8:
        sys_memset(&wk->login, 0, sizeof(WifiLoginParam));
        *state = 9;
        wk->login.unk14 = 0;
        break;
    case 9:
        wk->login.gameData = GSYS_GetGameData(gsys);
        wk->login.unk8 = 1;
        wk->login.unk4 = 0;
        wk->login.unkC = 10;
        wk->login.unk18 = 1;
        GFL_SndBGMPlay(SEQ_BGM_WIFI_ACCESS, SND_CHANNEL_MASK_ALL);
        wk->procManager = CreateGameProcManager(HEAPID_GAMEEVENT);
        QueueGameProc(wk->procManager, OVERLAY_WIFILOGIN, &WIFILOGIN_PROC_FUNCTIONS, &wk->login);
        wk->club->mode = 0;
        *state = 10;
        break;
    case 10:
        if (GFL_ProcMgrUpdate(wk->procManager)) {
            GFL_NetErrShow(0);
            return GAMEEVENT_CONTINUE;
        }
        FreeGameProcManager(wk->procManager);
        if (wk->login.result == 1) {
            GFL_SndBGMFadeOut(6);
            *state = 25;
        } else if (!func_0200a150(GameData_GetWifiList(wk->gameData))) {
            *state = 30;
        } else if (wk->login.result == 0) {
            *state = 11;
            GFL_SndBGMFadeOut(6);
        } else {
            GFL_SndBGMFadeOut(6);
            *state = 25;
        }
        break;
    case 30:
        wk->logout.gameData = GSYS_GetGameData(gsys);
        wk->logout.unk8 = 1;
        wk->logout.unk4 = 0;
        wk->logout.unkC = 1;
        wk->logout.unk10 = 0;
        wk->logout.unk14 = 0;
        wk->logout.unk18 = 0;
        wk->procManager = CreateGameProcManager(HEAPID_GAMEEVENT);
        QueueGameProc(wk->procManager, OVERLAY_WIFILOGIN, &WIFILOGOUT_PROC_FUNCTIONS, &wk->logout);
        (*state)++;
        break;
    case 31:
        if (GFL_ProcMgrUpdate(wk->procManager)) {
            GFL_NetErrShow(0);
            return GAMEEVENT_CONTINUE;
        }
        FreeGameProcManager(wk->procManager);
        GFL_SndBGMFadeOut(6);
        *state = 4;
        break;
    case 11:
        GFL_OvlLoad(OVERLAY_WIFICLUB_MAIN);
        GFL_OvlLoad(OVERLAY_APP_UI);
        func_ov173_021a6240(wk->club->buffer);
        if (wk->btlSetup != NULL) {
            BtlSetup_Free(wk->btlSetup);
            wk->btlSetup = NULL;
        }
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_WIFICLUB, &WIFICLUB_PROC_FUNCTIONS, wk->club);
        (*state)++;
        break;
    case 12:
        *state = sWifiClubModes[wk->club->mode].nextState;
        if (wk->club->mode == 1) {
            wk->login.unk14 = 1;
            EventWifiClub_ResetForLogin(wk);
        }
        wk->battleMode = sWifiClubModes[wk->club->mode].battleMode;
        GFL_OvlUnload(OVERLAY_APP_UI);
        GFL_OvlUnload(OVERLAY_WIFICLUB_MAIN);
        break;
    case 13:
        EventWifiClub_SetupBattleSelect(wk, GSYS_GetGameData(gsys), seq);
        wk->btlSetup = BtlSetup_Create(HEAPID_GAMEEVENT);
        EventWifiClub_SetupBattle(wk, wk->club->mode);
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_BATTLE_SELECT, &BATTLE_SELECT_PROC_FUNCTIONS, &wk->select);
        (*state)++;
        break;
    case 14:
        if (wk->select.result == 1) {
            if (func_02042788()) {
                wk->club->unk49 = 0;
                *state = 11;
            } else {
                EventWifiClub_ResetForLogin(wk);
                *state = 9;
            }
        } else {
            EventWifiClub_SetBattleParty(wk, GSYS_GetGameData(gsys), seq);
            GFL_OvlLoad(OVERLAY_BATTLE_MAIN);
            func_02040c20(0x100, data_ov167_021d7448, 9, 0);
            func_02040624(func_02040440(), 100, 10);
            (*state)++;
        }
        break;
    case 15:
        if (!func_02042788() || GFL_NetErrCheck()) {
            func_02012154();
            func_02011de0();
            EventWifiClub_ResetForLogin(wk);
            *state = 9;
        } else if (func_02040664(func_02040440(), 100, 10)) {
            (*state)++;
        }
        break;
    case 16: {
        int i;

        GFL_SndBGMPlay(SEQ_BGM_VS_TRAINER_WIFI, SND_CHANNEL_MASK_ALL);
        wk->players.unk44 = 1;
        if (func_02042a6c(func_02040440()) == 0) {
            for (i = 0; i < 2; i++) {
                wk->players.players[i].info = func_02017378(GSYS_GetGameData(gsys), i);
                wk->players.players[i].party = wk->club->parties[i];
            }
        } else {
            u8 order[2] = { 1, 0 };

            for (i = 0; i < 2; i++) {
                wk->players.players[order[i]].info = func_02017378(GSYS_GetGameData(gsys), i);
                wk->players.players[order[i]].party = wk->club->parties[i];
            }
        }
        wk->battle.gameData = GSYS_GetGameData(wk->gsys);
        wk->battle.setup = wk->btlSetup;
        wk->battle.players = &wk->players;
        wk->battle.unk14 = 1;
        wk->records = GameData_GetRecords(GSYS_GetGameData(wk->gsys));
        GFL_OvlUnload(OVERLAY_BATTLE_MAIN);
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_BATTLE, &data_ov010_0215039c, &wk->battle);
        (*state)++;
        break;
    }
    case 17:
        if (!func_02042788()) {
            EventWifiClub_ResetForLogin(wk);
            *state = 9;
        } else {
            if (wk->players.result == 0) {
                func_0200a2d4(GameData_GetWifiList(GSYS_GetGameData(wk->gsys)), wk->club->friendIndex - 1, 1, 0, 0);
            } else if (wk->players.result == 1) {
                func_0200a2d4(GameData_GetWifiList(GSYS_GetGameData(wk->gsys)), wk->club->friendIndex - 1, 0, 1, 0);
            }
            func_02040c64(0x100);
            *state = 11;
        }
        break;
    case 18:
        wk->trade.next = 0;
        wk->trade.friendIndex = wk->club->friendIndex;
        (*state)++;
        break;
    case 19:
        wk->trade.gameData = GSYS_GetGameData(gsys);
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_POKEMONTRADE, &POKEMONTRADE_WIFICLUB_PROC_FUNCTIONS, &wk->trade);
        (*state)++;
        break;
    case 20:
        switch (wk->trade.next) {
        case 1:
            *state = 21;
            break;
        case 2:
            wk->club->mode = 1;
            *state = 11;
            break;
        default:
            *state = 11;
            break;
        }
        if (!func_02042788()) {
            EventWifiClub_ResetForLogin(wk);
            *state = 9;
        }
        break;
    case 21: {
        ShinkaDemoParam *evolution =
            GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(ShinkaDemoParam), FALSE, "event_wificlub.c", 582);

        evolution->gameData = GSYS_GetGameData(wk->gsys);
        evolution->party = wk->trade.party;
        evolution->species = wk->trade.evolveSpecies;
        evolution->partyIndex = 0;
        evolution->method = wk->trade.evolveMethod;
        evolution->unkC = 1;
        evolution->canCancel = FALSE;
        wk->trade.evolution = evolution;
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_SHINKA_DEMO, &SHINKA_DEMO_PROC_FUNCTIONS, evolution);
        *state = 22;
        break;
    }
    case 22:
        GFL_HeapFree(wk->trade.evolution);
        wk->trade.next = 1;
        *state = 19;
        if (!func_02042788()) {
            EventWifiClub_ResetForLogin(wk);
            *state = 9;
        }
        break;
    case 23:
        wk->tvt.gameData = GSYS_GetGameData(gsys);
        wk->tvt.unk4 = 3;
        GSYS_QueueProcAsEvent(wk->event, OVERLAY_COMM_TVT, &COMM_TVT_PROC_FUNCTIONS, &wk->tvt);
        (*state)++;
        break;
    case 24:
        if (!func_02042788()) {
            EventWifiClub_ResetForLogin(wk);
            *state = 9;
        } else {
            *state = 11;
        }
        break;
    case 28:
        func_02042860(0);
        *state = 29;
        break;
    case 29:
        if (func_02042ab8()) {
            *state = 4;
        }
        break;
    case 25:
    case 26:
    case 27:
        *state = 4;
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
