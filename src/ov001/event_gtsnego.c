#include "types.h"
#include "app/gtsnego.h"
#include "app/pokemon_trade.h"
#include "app/wifi_login.h"
#include "demo/evolution_demo.h"
#include "field/event_gtsnego.h"
#include "field/field_event.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

typedef struct {
    u32 bgm;
    s32 timeout;
    u8 buffer[0x174];
    GameSystem *gsys;
    u32 unk180;
    GtsNegoParam nego;
    PokemonTradeParam trade;
    WifiLoginParam login;
    WifiLogoutParam logout;
    PokeParty *party;
    u32 unk3B0;
} EventGtsNego;

enum {
    STEP_SAVE_BGM = 0,
    STEP_CLOSE_FIELD,
    STEP_LOGIN,
    STEP_LOGIN_WAIT,
    STEP_NEGOTIATE,
    STEP_NEGOTIATE_WAIT,
    STEP_TRADE,
    STEP_TRADE_WAIT,
    STEP_WAIT_PARTNER,
    STEP_DISCONNECT,
    STEP_EVOLVE,
    STEP_EVOLVE_WAIT,
    STEP_LOGOUT,
    STEP_LOGOUT_WAIT,
    STEP_CLEANUP,
    STEP_OPEN_FIELD_WAIT,
    STEP_DONE,
};

GameEventReturnCode EventGtsNego_Callback(GameEvent *event, u32 *state, void *data);
EventGtsNego *EventGtsNego_Init(GameEvent *event, GameSystem *gsys, Field *field, u32 unk3B0);

GameEventReturnCode EventGtsNego_Callback(GameEvent *event, u32 *state, void *data) {
    EventGtsNego *wk = data;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);

    switch (*state) {
    case STEP_SAVE_BGM:
        wk->bgm = GFL_SndBGMGetID();
        GFL_SndBGMFadeOut(6);
        (*state)++;
        break;
    case STEP_CLOSE_FIELD:
        // Wait for the comm system to shut down
        if (!GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
            GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
            (*state)++;
        }
        break;
    case STEP_LOGIN:
        wk->login.buffer = wk->buffer;
        wk->login.unkC = 0x33;
        GSYS_QueueProc(gsys, OVERLAY_WIFILOGIN, &WIFILOGIN_PROC_FUNCTIONS, &wk->login);
        (*state)++;
        break;
    case STEP_LOGIN_WAIT:
        if (!GSYS_GetProcMgrState(gsys)) {
            if (wk->login.result == 0) {
                (*state)++;
            } else {
                *state = STEP_CLEANUP;
            }
        }
        break;
    case STEP_NEGOTIATE:
        wk->nego.result = 0;
        GSYS_QueueProc(gsys, OVERLAY_GTSNEGO, &GTSNEGO_PROC_FUNCTIONS, &wk->nego);
        (*state)++;
        break;
    case STEP_NEGOTIATE_WAIT:
        if (!GSYS_GetProcMgrState(gsys)) {
            if (wk->nego.result == GTSNEGO_RESULT_EXIT) {
                *state = STEP_LOGOUT;
            } else if (wk->nego.result == GTSNEGO_RESULT_RETRY_LOGIN) {
                wk->login.unk14 = 1;
                *state = STEP_LOGIN;
            } else {
                wk->trade.next = 0;
                (*state)++;
            }
        }
        break;
    case STEP_TRADE:
        wk->trade.gameData = gameData;
        wk->trade.nego = &wk->nego;
        wk->trade.buffer = wk->buffer;
        GSYS_QueueProc(gsys, OVERLAY_POKEMONTRADE, &POKEMONTRADE_PROC_FUNCTIONS, &wk->trade);
        (*state)++;
        break;
    case STEP_TRADE_WAIT:
        if (!GSYS_GetProcMgrState(gsys)) {
            if (!GFL_NetIsInitialized()) {
                wk->login.unk14 = 1;
                *state = STEP_LOGIN;
            } else if (wk->trade.next == TRADE_NEXT_NEGOTIATE) {
                *state = STEP_NEGOTIATE;
            } else if (wk->trade.next == TRADE_NEXT_EVOLVE) {
                *state = STEP_EVOLVE;
            } else {
                NetHandle_SendTimingPair(NetHandle_GetOwn(), 1, 12);
                GflNet_SetAutoExit(FALSE);
                GFL_NetSetDisconnectError(FALSE);
                wk->timeout = 100;
                *state = STEP_WAIT_PARTNER;
            }
        }
        break;
    case STEP_WAIT_PARTNER:
        wk->timeout--;
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 1, 12)) {
            *state = STEP_DISCONNECT;
        }
        if (wk->timeout < 0) {
            *state = STEP_DISCONNECT;
        }
        break;
    case STEP_DISCONNECT:
        NetState_WifiDisconnect(1);
        *state = STEP_NEGOTIATE;
        break;
    case STEP_EVOLVE: {
        EvolutionDemoParam *evolution =
            GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(EvolutionDemoParam), FALSE, "event_gtsnego.c", 204);

        evolution->gameData = wk->trade.gameData;
        evolution->party = wk->party;
        evolution->species = wk->trade.evolveSpecies;
        evolution->partyIndex = 0;
        evolution->method = wk->trade.evolveMethod;
        evolution->unkC = 1;
        evolution->canCancel = FALSE;
        wk->trade.evolution = evolution;
        GSYS_QueueProc(gsys, OVERLAY_EVOLUTION_DEMO, &EVOLUTION_DEMO_PROC_FUNCTIONS, evolution);
        (*state)++;
        break;
    }
    case STEP_EVOLVE_WAIT:
        if (!GSYS_GetProcMgrState(gsys)) {
            GFL_HeapFree(wk->trade.evolution);
            if (!GFL_NetIsInitialized()) {
                wk->login.unk14 = 1;
                *state = STEP_LOGIN;
            } else {
                wk->trade.next = TRADE_NEXT_EVOLVE;
                *state = STEP_TRADE;
            }
        }
        break;
    case STEP_LOGOUT:
        GSYS_QueueProc(gsys, OVERLAY_WIFILOGIN, &WIFILOGOUT_PROC_FUNCTIONS, &wk->logout);
        (*state)++;
        break;
    case STEP_LOGOUT_WAIT:
        if (!GSYS_GetProcMgrState(gsys)) {
            (*state)++;
        }
        break;
    case STEP_CLEANUP:
        GFL_HeapFree(wk->nego.playerInfo);
        GFL_HeapFree(wk->nego.playerInfo2);
        GFL_HeapFree(wk->party);
        GFL_SndBGMPlay(wk->bgm, SND_CHANNEL_MASK_ALL);
        GFL_SndBGMFadeIn(60);
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case STEP_OPEN_FIELD_WAIT:
        (*state)++;
        break;
    case STEP_DONE:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

EventGtsNego *EventGtsNego_Init(GameEvent *event, GameSystem *gsys, Field *field, u32 unk3B0) {
    EventGtsNego *wk;
    PlayerInfo *playerInfo;

    if (GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
        GameCommSys_ExitReq(GSYS_GetGameCommSystem(gsys));
    }

    wk = GameEvent_GetData(event);
    wk->gsys = gsys;
    wk->unk3B0 = unk3B0;
    wk->party = PokeParty_Create(HEAPID_GAMEEVENT);
    wk->trade.party = wk->party;
    MI_CpuFill8(&wk->nego, 0, sizeof(GtsNegoParam));
    wk->nego.gameData = GSYS_GetGameData(gsys);
    wk->nego.playerInfo = GFL_HeapAllocate(HEAPID_GAMEEVENT, PlayerInfo_GetSize(), TRUE, "event_gtsnego.c", 291);
    wk->nego.playerInfo2 = GFL_HeapAllocate(HEAPID_GAMEEVENT, PlayerInfo_GetSize(), TRUE, "event_gtsnego.c", 292);
    playerInfo = GetGameDataPlayerInfo(GSYS_GetGameData(gsys));
    MI_CpuCopy8(playerInfo, wk->nego.playerInfo, PlayerInfo_GetSize());

    MI_CpuFill8(&wk->login, 0, sizeof(WifiLoginParam));
    wk->login.gameData = GSYS_GetGameData(gsys);
    wk->login.unk4 = 0;
    wk->login.unk8 = 1;
    wk->logout.gameData = GSYS_GetGameData(gsys);
    wk->logout.unk4 = 0;
    wk->logout.unk8 = 1;
    return wk;
}

GameEvent *EventGtsNego_Create(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGtsNego_Callback, sizeof(EventGtsNego));

    EventGtsNego_Init(event, gsys, field, 0);
    return event;
}

// Called through GameEvent_CreateOverlayDelegate, by the NetConnectGTSNegotiation script command, which passes the
// field instead of a pointer to arguments
GameEvent *EventGtsNego_CreateFromArgs(GameSystem *gsys, void *field) {
    return EventGtsNego_Create(gsys, field);
}
