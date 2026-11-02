#include "types.h"
#include "app/worldtrade.h"
#include "field/event_worldtrade.h"
#include "field/field_event.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

typedef struct {
    u32 bgm;
    GameSystem *gsys;
    Field *field;
    WorldTradeParam param;
    // Never set here, so the field transitions are skipped
    BOOL useTransitions;
} EventWorldTrade;

// The steps of the event
enum {
    STEP_WAIT_COMM_EXIT,
    STEP_FADE_OUT,
    STEP_CLOSE_FIELD,
    STEP_START_PROC,
    STEP_WAIT_PROC,
    STEP_FADE_IN,
    STEP_END,
};

GameEventReturnCode EventWorldTrade_Callback(GameEvent *event, u32 *state, void *data);

// Called through GameEvent_CreateOverlayDelegate, by the NetConnectGTS script command
GameEvent *EventWorldTrade_CreateFromArgs(GameSystem *gsys, void *data) {
    EventWorldTradeArgs *args = data;

    return EventWorldTrade_Create(gsys, args->field, args->unused);
}

GameEvent *EventWorldTrade_Create(GameSystem *gsys, Field *field, u32 unused) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWorldTrade_Callback, sizeof(EventWorldTrade));
    EventWorldTrade *wk;

    if (GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
        GameCommSys_ExitReq(GSYS_GetGameCommSystem(gsys));
    }

    wk = GameEvent_GetData(event);
    MI_CpuFill8(wk, 0, sizeof(EventWorldTrade));
    wk->gsys = gsys;
    wk->field = field;
    wk->param.savedata = GameData_GetSaveControl(gameData);
    wk->param.worldtrade_data = SaveControl_GetWorldTradeData(wk->param.savedata);
    wk->param.systemdata = getSaveAdventureDataBlk(wk->param.savedata);
    wk->param.myparty = SaveControl_GetPokePartySave(wk->param.savedata);
    wk->param.mybox = GameData_GetBoxSaveAccessor(gameData);
    wk->param.pokedex = GameData_GetPokedex(gameData);
    wk->param.wifilist = GameData_GetWifiList(gameData);
    wk->param.wifihistory = getUnityTower_SurveySaveBlkAddrress(wk->param.savedata);
    wk->param.mystatus = SaveControl_GetPlayerInfo(wk->param.savedata);
    wk->param.config = getTrainerDataBlkAddress(wk->param.savedata);
    wk->param.record = getTrainerCardInfoBlkAddress(wk->param.savedata);
    wk->param.bag = GameData_GetBag(gameData);
    wk->param.gsys = gsys;
    wk->param.isNationalDex = PokeDex_IsNationalObtained(wk->param.pokedex);
    wk->param.profileId = WifiList_GetMyGSID(wk->param.wifilist);
    wk->param.unk34 = 0;
    wk->param.unk38 = 0;
    return event;
}

GameEventReturnCode EventWorldTrade_Callback(GameEvent *event, u32 *state, void *data) {
    EventWorldTrade *wk = data;

    switch (*state) {
    case STEP_WAIT_COMM_EXIT:
        // Wait for the comm system to shut down
        if (!GameCommSys_BootCheck(GSYS_GetGameCommSystem(wk->gsys))) {
            *state = STEP_FADE_OUT;
        }
        break;
    case STEP_FADE_OUT:
        if (wk->useTransitions) {
            GameEvent_ChainNext(
                event, CallFieldMapEntranceOutTransitionDefault(wk->gsys, wk->field, FIELD_TRANSITION_FADE_BLACK, 0));
        }
        wk->bgm = GFL_SndBGMGetID();
        GFL_SndBGMFadeOut(6);
        *state = STEP_CLOSE_FIELD;
        break;
    case STEP_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(wk->gsys, wk->field));
        *state = STEP_START_PROC;
        break;
    case STEP_START_PROC:
        GSYS_QueueProc(wk->gsys, OVERLAY_WORLDTRADE, &WORLDTRADE_PROC_FUNCTIONS, &wk->param);
        *state = STEP_WAIT_PROC;
        break;
    case STEP_WAIT_PROC:
        if (!GSYS_GetProcMgrState(wk->gsys)) {
            GFL_SndBGMPlay(wk->bgm, SND_CHANNEL_MASK_ALL);
            GFL_SndBGMFadeIn(60);
            GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(wk->gsys));
            *state = STEP_FADE_IN;
        }
        break;
    case STEP_FADE_IN:
        if (wk->useTransitions) {
            GameEvent_ChainNext(
                event, CallFieldMapEntranceInTransition(wk->gsys, wk->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        }
        *state = STEP_END;
        break;
    case STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
