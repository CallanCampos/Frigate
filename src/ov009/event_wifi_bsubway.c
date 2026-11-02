#include "types.h"
#include "app/wifi_bsubway.h"
#include "field/event_wifi_bsubway.h"
#include "field/field_event.h"
#include "system/game_comm.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The steps of the event
enum {
    STEP_WAIT_COMM_EXIT,
    STEP_FADE_OUT,
    STEP_CLOSE_FIELD,
    STEP_START_PROC,
    STEP_WAIT_PROC,
    STEP_OPEN_FIELD,
    STEP_FADE_IN,
    STEP_END,
};

// Also the parameter of the Wi-Fi Battle Subway proc, which sets procResult
typedef struct {
    u32 mode;
    GameSystem *gsys;
    u32 procResult;
    u16 *result;
} EventWifiBSubway;

GameEventReturnCode EventWifiBSubway_Callback(GameEvent *event, u32 *state, void *data);

// Called through GameEvent_CreateOverlayDelegate, by a script command in overlay 50
GameEvent *EventWifiBSubway_CreateFromArgs(GameSystem *gsys, void *data) {
    EventWifiBSubwayArgs *args = data;

    return EventWifiBSubway_Create(gsys, args->mode, args->result);
}

GameEvent *EventWifiBSubway_Create(GameSystem *gsys, u32 mode, u16 *result) {
    GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWifiBSubway_Callback, sizeof(EventWifiBSubway));
    EventWifiBSubway *wk = GameEvent_GetData(event);

    wk->mode = mode;
    wk->gsys = gsys;
    wk->result = result;
    if (GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
        GameCommSys_ExitReq(GSYS_GetGameCommSystem(gsys));
    }
    return event;
}

GameEventReturnCode EventWifiBSubway_Callback(GameEvent *event, u32 *state, void *data) {
    EventWifiBSubway *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = GSYS_GetField(gsys);

    switch (*state) {
    case STEP_WAIT_COMM_EXIT:
        // Wait for the comm system to shut down
        if (!GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
            (*state)++;
        }
        break;
    case STEP_FADE_OUT:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
        (*state)++;
        break;
    case STEP_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        (*state)++;
        break;
    case STEP_START_PROC:
        GSYS_QueueProc(gsys, OVERLAY_WIFI_BSUBWAY, &WIFI_BSUBWAY_PROC_FUNCTIONS, wk);
        (*state)++;
        break;
    case STEP_WAIT_PROC:
        if (!GSYS_GetProcMgrState(gsys)) {
            if (wk->procResult == 0) {
                *wk->result = TRUE;
            } else {
                *wk->result = FALSE;
            }
            (*state)++;
        }
        break;
    case STEP_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case STEP_FADE_IN:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
