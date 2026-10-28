#include "types.h"
#include "app/irc_compatible.h"
#include "app/ircbattlematch.h"
#include "app/pokemon_trade.h"
#include "battle/battle_proc.h"
#include "battle/btl_net.h"
#include "battle/btl_setup.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "demo/evolution_demo.h"
#include "field/event_irc.h"
#include "field/event_make.h"
#include "field/field_event.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "nitro/gx.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/wifi_list.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

void setPartyLv50(PokeParty *party) {
    int i;

    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        setLevel(PokeParty_GetPkm(party, i), 50);
    }
}

void battleBoxToLv50Party(BOOL useBattleBox, EventIRCWork *work) {
    int i;
    BattleBoxSave *battleBox;
    PartyPkm *pkm;
    PokeParty *party;

    battleBox = getBattleBox(work->save);
    if (!useBattleBox) {
        party = GameData_GetParty(GSYS_GetGameData(work->gsys));
        for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
            pkm = PokeParty_GetPkm(party, i);
            if (!PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
                PokeParty_AddPkm(work->party, pkm);
            }
        }
    } else {
        party = convertBoxedPokeSetToParty(battleBox, HEAPID_GAMEEVENT);
        PokeParty_Copy(party, work->party);
        GFL_HeapFree(party);
    }
    PokeParty_RecoverAll(work->party);
    setPartyLv50(work->party);
}

void TrimPartyTo3Members(PokeParty *party) {
    s32 i;
    s32 count = PokeParty_GetPkmCount(party);

    for (i = 3; i < count; i++) {
        PokeParty_RemovePkm(party, 3);
    }
}

void TrimPartiesTo3Members(EventIRCWork *work) {
    s32 i;

    TrimPartyTo3Members(work->party);
    for (i = 0; i < 4; i++) {
        TrimPartyTo3Members(work->parties[i]);
    }
}

// Records the battle with each other player in the friend list
void EventIRC_RecordFriendBattles(EventIRCWork *work, GameData *gameData) {
    s32 partner = 0;
    s32 count;
    u32 place;
    u32 partnerPlace;
    s32 i;
    PlayerInfo *info;
    u32 friendIndex;

    if (work->btlSetup->fieldSituation.unk1a == 0) {
        partner = -1;
        count = 2;
    } else {
        place = work->ircMatch.order[GFL_NetGetHandleNetId(NetHandle_GetOwn())];
        if (place < 2) {
            partnerPlace = 1 - place;
        } else {
            partnerPlace = 3 - (place - 2);
        }
        for (i = 0; i < 4; i++) {
            if (partnerPlace == work->ircMatch.order[i]) {
                partner = i;
                break;
            }
        }
        count = 4;
    }
    for (i = 0; i < count; i++) {
        info = GameData_GetPlayerInfoByNetID(gameData, i);
        if (i != GFL_NetGetHandleNetId(NetHandle_GetOwn()) && info != NULL &&
            WifiList_FindFriendByPlayerInfo(GameData_GetWifiList(gameData), info, &friendIndex)) {
            if (i == partner) {
                WifiList_SetFriendLastDateToNow(GameData_GetWifiList(gameData), friendIndex);
            } else if (work->players.result == 0) {
                WifiList_AddFriendRecord(GameData_GetWifiList(gameData), friendIndex, 1, 0, 0);
            } else if (work->players.result == 1) {
                WifiList_AddFriendRecord(GameData_GetWifiList(gameData), friendIndex, 0, 1, 0);
            } else {
                WifiList_SetFriendLastDateToNow(GameData_GetWifiList(gameData), friendIndex);
            }
        }
    }
}

// Puts each player's party and info in their place in the battle
void EventIRC_SetBattlePlayers(EventIRCWork *work, GameSystem *gsys) {
    if (work->btlSetup->fieldSituation.unk1a) {
        if (work->ircMatch.order[GFL_NetGetHandleNetId(NetHandle_GetOwn())] < 2) {
            s32 i = 0;

            for (; i < 4; i++) {
                work->players.players[work->ircMatch.order[i]].party = work->parties[i];
                work->players.players[work->ircMatch.order[i]].info =
                    GameData_GetPlayerInfoByNetID(GSYS_GetGameData(gsys), i);
            }
        } else {
            s32 i = 0;
            u8 places[4] = { 2, 3, 0, 1 };

            for (; i < 4; i++) {
                work->players.players[places[work->ircMatch.order[i]]].party = work->parties[i];
                work->players.players[places[work->ircMatch.order[i]]].info =
                    GameData_GetPlayerInfoByNetID(GSYS_GetGameData(gsys), i);
            }
        }
    } else if (GFL_NetGetHandleNetId(NetHandle_GetOwn()) == 0) {
        s32 i = 0;

        for (; i < 2; i++) {
            work->players.players[work->ircMatch.order[i]].party = work->parties[i];
            work->players.players[work->ircMatch.order[i]].info =
                GameData_GetPlayerInfoByNetID(GSYS_GetGameData(gsys), i);
        }
    } else {
        s32 i = 0;
        u8 places[2] = { 1, 0 };

        for (; i < 2; i++) {
            work->players.players[places[work->ircMatch.order[i]]].party = work->parties[i];
            work->players.players[places[work->ircMatch.order[i]]].info =
                GameData_GetPlayerInfoByNetID(GSYS_GetGameData(gsys), i);
        }
    }
}

GameEventReturnCode EventIRC_Callback(GameEvent *event, u32 *state, void *data) {
    EventIRCWork *work = data;
    GameSystem *gsys = work->gsys;
    Field *field = GSYS_GetField(gsys);
    EvolutionDemoParam *evolution;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        *state = 2;
        break;
    case 2:
        GSYS_QueueProc(gsys, OVERLAY_ID(36), &IRC_MENU_PROC, work);
        (*state)++;
        break;
    case 3:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        if (work->mode == 8) {
            *state = 24;
        } else if (work->mode == 12) {
            *state = 21;
        } else {
            (*state)++;
        }
        break;
    case 4:
        (*state)++;
        break;
    case 5:
        battleBoxToLv50Party(work->useBattleBox, work);
        work->ircMatch.gameData = work->gameData;
        work->ircMatch.party = work->party;
        work->ircMatch.parties[0] = work->parties[0];
        work->ircMatch.parties[1] = work->parties[1];
        work->ircMatch.parties[2] = work->parties[2];
        work->ircMatch.parties[3] = work->parties[3];
        work->ircMatch.result = work->mode;
        GSYS_QueueProc(gsys, OVERLAY_IRCBATTLEMATCH, &IRCBATTLEMATCH_PROC_FUNCTIONS, &work->ircMatch);
        (*state)++;
        break;
    case 6:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        work->mode = work->ircMatch.result;
        switch (work->mode) {
        case 7:
            *state = 20;
            break;
        case 12:
            *state = 20;
            break;
        case 13:
            *state = 2;
            break;
        case 6:
            work->trade.next = 0;
            *state = 13;
            break;
        default:
            *state = 7;
            break;
        }
        break;
    case 7:
        GFL_OvlLoad(OVERLAY_BATTLE_MAIN);
        NetCommand_AddTable(0x100, data_ov167_021d7448, 9, NULL);
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x6e, 12);
        (*state)++;
        break;
    case 8:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x6e, 12)) {
            (*state)++;
        }
        if (GFL_NetErrCheck()) {
            if (GFL_NetErrShutdown() == TRUE) {
                GFL_NetErrShowScreen();
            } else {
                GFL_NetErrAbort();
            }
            GFL_OvlUnload(OVERLAY_BATTLE_MAIN);
            *state = 20;
        }
        break;
    case 9:
        work->btlSetup = BtlSetup_Create(HEAPID_GAMEEVENT);
        work->records = GameData_GetRecords(work->gameData);
        work->players.unk4C = 0;
        switch (work->mode) {
        case 1:
            work->players.unk44 = 1;
            work->players.rule = 2;
            BtlSetup_SetNet1v1Single(work->btlSetup, work->gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
            break;
        case 2:
            work->players.unk44 = 1;
            work->players.rule = 6;
            BtlSetup_SetNet1v1Double(work->btlSetup, work->gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
            break;
        case 3:
            work->players.unk44 = 1;
            work->players.rule = 10;
            BtlSetup_SetNetTriple(work->btlSetup, work->gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
            break;
        case 4:
            work->players.unk44 = 1;
            work->players.rule = 14;
            BtlSetup_SetNetRotation(work->btlSetup, work->gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
            break;
        case 5: {
            u32 positions[4] = { 0, 2, 1, 3 };

            work->players.unk44 = 3;
            work->players.rule = 18;
            BtlSetup_SetNetMultiVsNet(work->btlSetup, work->gameData, NetHandle_GetOwn(), 1,
                                      positions[work->ircMatch.order[GFL_NetGetHandleNetId(NetHandle_GetOwn())]],
                                      HEAPID_GAMEEVENT);
            work->btlSetup->fieldSituation.unk1a = 1;
            work->btlSetup->battleStyle = 1;
            TrimPartiesTo3Members(work);
            break;
        }
        }
        BtlSetup_SetParty(work->btlSetup, work->party, 0);
        BtlSetup_AllocRecBuffer(work->btlSetup, 1);
        work->bgm = GFL_SndBGMGetID();
        GFL_SndBGMStopImmediate();
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 10, 12);
        (*state)++;
        break;
    case 10:
        if (GFL_NetErrCheck()) {
            BtlSetup_Free(work->btlSetup);
            work->btlSetup = NULL;
            if (GFL_NetErrShutdown() == TRUE) {
                GFL_NetErrShowScreen();
            } else {
                GFL_NetErrAbort();
            }
            GFL_OvlUnload(OVERLAY_BATTLE_MAIN);
            *state = 20;
        } else if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 10, 12)) {
            if (GFL_NetIsParent()) {
                GFL_SndBGMPlay(SEQ_BGM_VS_TRAINER_M, SND_CHANNEL_MASK_ALL);
            } else {
                GFL_SndBGMPlay(SEQ_BGM_VS_TRAINER_S, SND_CHANNEL_MASK_ALL);
            }
            (*state)++;
        }
        break;
    case 11: {
        EventMakeArgs args;

        EventIRC_SetBattlePlayers(work, gsys);
        GFL_OvlUnload(OVERLAY_BATTLE_MAIN);
        args.setup = work->btlSetup;
        args.players = &work->players;
        args.unk08 = 1;
        GameEvent_ChainNext(event, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_BATTLE, eventMakeFunc, &args));
        (*state)++;
        break;
    }
    case 12:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        EventIRC_RecordFriendBattles(work, GSYS_GetGameData(gsys));
        BtlSetup_Free(work->btlSetup);
        work->btlSetup = NULL;
        GFL_SndBGMStopImmediate();
        EventIRC_RestoreBgm(work);
        *state = 17;
        break;
    case 13:
        work->trade.gameData = work->gameData;
        work->trade.party = work->party;
        GSYS_QueueProc(gsys, OVERLAY_POKEMONTRADE, &data_ov194_021c63dc, &work->trade);
        (*state)++;
        break;
    case 14:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        if (work->trade.next == TRADE_NEXT_EVOLVE) {
            *state = 15;
        } else {
            *state = 17;
        }
        break;
    case 15:
        evolution = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(EvolutionDemoParam), FALSE, "event_ircbattle.c", 537);
        evolution->gameData = work->gameData;
        evolution->party = work->trade.party;
        evolution->species = work->trade.evolveSpecies;
        evolution->partyIndex = 0;
        evolution->method = work->trade.evolveMethod;
        evolution->unkC = 1;
        evolution->canCancel = FALSE;
        work->trade.evolution = evolution;
        GSYS_QueueProc(gsys, OVERLAY_EVOLUTION_DEMO, &EVOLUTION_DEMO_PROC_FUNCTIONS, evolution);
        (*state)++;
        break;
    case 16:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        GFL_HeapFree(work->trade.evolution);
        work->trade.next = TRADE_NEXT_EVOLVE;
        *state = 13;
        break;
    case 17:
        if (GFL_NetIsInitialized() && !GFL_NetErrCheck()) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 0xdc, 12);
        }
        (*state)++;
        break;
    case 18:
        if (GFL_NetIsInitialized() && !GFL_NetErrCheck()) {
            if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0xdc, 12)) {
                work->counter = 0;
                (*state)++;
            }
        } else {
            work->counter = 0;
            (*state)++;
        }
        break;
    case 19:
        if (GFL_NetIsInitialized()) {
            if (++work->counter >= 60) {
                GFL_NetExit(0);
                (*state)++;
            }
        } else {
            (*state)++;
        }
        break;
    case 20:
        if (!GFL_NetIsInitialized()) {
            GFL_NetErrClear();
            *state = 21;
        }
        break;
    case 21:
        EventIRC_RestoreBgm(work);
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case 22:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case 23:
        GFL_NetIwFree();
        EventIRC_FreeParties(work);
        return GAMEEVENT_DONE;
    case 24:
        (*state)++;
        break;
    case 25:
        (*state)++;
        break;
    case 26:
        work->ov175Param = work->gsys;
        GSYS_QueueProc(gsys, OVERLAY_ID(175), &IRC_COMPATIBLE_PROC_FUNCTIONS, &work->ov175Param);
        (*state)++;
        break;
    case 27:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
        *state = 20;
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *CallIRC(GameSystem *gsys, Field *field, GameEvent *event, BOOL create) {
    EventIRCWork *work;
    s32 i;

    if (create) {
        event = GameEvent_Create(gsys, NULL, EventIRC_Callback, sizeof(EventIRCWork));
    } else {
        GameEvent_Transplant(event, EventIRC_Callback, sizeof(EventIRCWork));
    }
    work = GameEvent_GetData(event);
    work->save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    work->gameData = GSYS_GetGameData(gsys);
    work->gsys = gsys;
    for (i = 0; i < 4; i++) {
        work->parties[i] = PokeParty_Create(HEAPID_GAMEEVENT);
    }
    work->party = PokeParty_Create(HEAPID_GAMEEVENT);
    return event;
}

void EventIRC_FreeParties(EventIRCWork *work) {
    s32 i;

    for (i = 0; i < 4; i++) {
        GFL_HeapFree(work->parties[i]);
    }
    GFL_HeapFree(work->party);
}

// Brings back the music from before the battle
void EventIRC_RestoreBgm(EventIRCWork *work) {
    if (work->bgm) {
        GFL_SndBGMPlay(work->bgm, SND_CHANNEL_MASK_ALL);
        GFL_SndBGMFadeIn(60);
        work->bgm = 0;
    }
}

// Accessors for the infrared menu of overlay 36
void EventIRC_SetMode(EventIRCWork *work, u32 mode) {
    work->mode = mode;
}

GameSystem *EventIRC_GetGameSystem(EventIRCWork *work) {
    return work->gsys;
}

SaveControl *EventIRC_GetSaveControl(EventIRCWork *work) {
    return work->save;
}
