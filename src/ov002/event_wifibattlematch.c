#include "types.h"
#include "app/wifibattlematch.h"
#include "field/event_wifibattlematch.h"
#include "field/field_event.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The steps of the event
enum {
    STEP_WAIT_COMM_EXIT,
    STEP_FADE_OUT,
    STEP_CLOSE_FIELD,
    STEP_CALL_PROC,
    STEP_WAIT_PROC,
    STEP_CLEAR_MATCH_IN_PROGRESS,
    STEP_OPEN_FIELD,
    STEP_FADE_IN,
    STEP_END,
};

typedef struct {
    u32 bgm;
    GameSystem *gsys;
    Field *field;
    WifiBattleMatchParam *param;
    u32 startFrom;
    u32 teamSource;
    u32 battleType;
    // Never set, so the field transitions are skipped
    BOOL useTransitions;
} EventWifiBattleMatch;

GameEventReturnCode EventWifiBattleMatch_Callback(GameEvent *event, u32 *state, void *data);

GameEventReturnCode EventWifiBattleMatch_Callback(GameEvent *event, u32 *state, void *data) {
    EventWifiBattleMatch *wk = data;
    GameSystem *gsys = wk->gsys;

    switch (*state) {
    case STEP_WAIT_COMM_EXIT:
        // Wait for the comm system to shut down
        if (!GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
            wk->bgm = GFL_SndBGMGetID();
            GFL_SndBGMFadeOut(6);
            (*state)++;
        }
        break;
    case STEP_FADE_OUT:
        if (wk->useTransitions) {
            GameEvent_ChainNext(
                event, CallFieldMapEntranceOutTransitionDefault(gsys, wk->field, FIELD_TRANSITION_FADE_BLACK, 0));
        }
        (*state)++;
        break;
    case STEP_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, wk->field));
        (*state)++;
        break;
    case STEP_CALL_PROC: {
        WifiBattleMatchParam *param =
            GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(WifiBattleMatchParam), TRUE, "event_wifibattlematch.c", 135);

        wk->param = param;
        param->startFrom = wk->startFrom;
        param->gameData = GSYS_GetGameData(gsys);
        param->battleType = wk->battleType;
        param->teamSource = wk->teamSource;
        GSYS_QueueProc(gsys, OVERLAY_WIFIBATTLEMATCH, &WIFIBATTLEMATCH_PROC_FUNCTIONS, wk->param);
        (*state)++;
        break;
    }
    case STEP_WAIT_PROC:
        if (!GSYS_GetProcMgrState(gsys)) {
            GFL_HeapFree(wk->param);
            (*state)++;
        }
        break;
    case STEP_CLEAR_MATCH_IN_PROGRESS:
        RecordSave_ClearMatchInProgress(getRecordBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))));
        (*state)++;
        break;
    case STEP_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        GFL_SndBGMPlay(wk->bgm, SND_CHANNEL_MASK_ALL);
        GFL_SndBGMFadeIn(60);
        (*state)++;
        break;
    case STEP_FADE_IN:
        if (wk->useTransitions) {
            GameEvent_ChainNext(
                event, CallFieldMapEntranceInTransition(gsys, wk->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        }
        (*state)++;
        break;
    case STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventWifiBattleMatch_Create(GameSystem *gsys, Field *field, u32 startFrom, u32 teamSource, u32 battleType) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWifiBattleMatch_Callback, sizeof(EventWifiBattleMatch));
    EventWifiBattleMatch *wk;

    if (GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
        GameCommSys_ExitReq(GSYS_GetGameCommSystem(gsys));
    }

    wk = GameEvent_GetData(event);
    wk->gsys = gsys;
    wk->field = field;
    wk->startFrom = (u16)startFrom;
    wk->teamSource = teamSource;
    wk->battleType = battleType;
    wk->useTransitions = FALSE;
    return event;
}

// Called through GameEvent_CreateOverlayDelegate, by the NetConnectWiFiBattle script command
GameEvent *EventWifiBattleMatch_CreateFromArgs(GameSystem *gsys, void *data) {
    EventWifiBattleMatchArgs *args = data;

    return EventWifiBattleMatch_Create(gsys, args->field, args->startFrom, args->teamSource, args->battleType);
}
