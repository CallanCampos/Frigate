#include "types.h"
#include "app/comm_tvt.h"
#include "app/comm_tvt_field.h"
#include "field/event_comm_tvt.h"
#include "field/field_event.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

enum {
    STEP_FADE_OUT,
    STEP_CLOSE_FIELD,
    STEP_EMPTY,
    STEP_START_FIELD_APP,
    STEP_WAIT_FIELD_APP,
    STEP_WAIT_VIDEO_CHAT,
    STEP_CLEAR_COMM_PARAM,
    STEP_OPEN_FIELD,
    STEP_FADE_IN,
    STEP_RETURN,
    STEP_END,
};

struct EventCommTvt {
    GameSystem *gsys;
    GameData *gameData;
    SaveControl *save;
    u32 commParam[5];
    u32 mode;
    u32 bgm;
    void *exitArgument;
    u32 unused2C;
    u32 unused30;
};

GameEventReturnCode EventCommTvt_Callback(GameEvent *event, u32 *state, void *data);

GameEventReturnCode EventCommTvt_Callback(GameEvent *event, u32 *state, void *data) {
    EventCommTvt *work = data;
    GameSystem *gsys = work->gsys;
    GameCommSys *comm = GSYS_GetGameCommSystem(gsys);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);

    switch (*state) {
    case STEP_FADE_OUT:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
        (*state)++;
        break;
    case STEP_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        *state = STEP_EMPTY;
        break;
    case STEP_EMPTY:
        (*state)++;
    case STEP_START_FIELD_APP:
        GSYS_QueueProc(gsys, OVERLAY_COMM_TVT_FIELD_APP, &COMM_TVT_FIELD_PROC_FUNCTIONS, work);
        (*state)++;
        break;
    case STEP_WAIT_FIELD_APP:
        if (!GSYS_GetProcMgrState(gsys)) {
            switch (work->mode) {
            case COMM_TVT_FIELD_MODE_ENTRALINK_RETURN:
                *state = STEP_CLEAR_COMM_PARAM;
                break;
            case COMM_TVT_FIELD_MODE_VIDEO_CHAT:
                work->bgm = GFL_SndBGMGetID();
                GSYS_QueueProcAsEvent(event, OVERLAY_ID(257), &COMM_TVT_PROC_FUNCTIONS, work->commParam);
                *state = STEP_WAIT_VIDEO_CHAT;
                break;
            default:
                *state = STEP_CLEAR_COMM_PARAM;
                break;
            }
        }
        break;
    case STEP_WAIT_VIDEO_CHAT:
        if (!GSYS_GetProcMgrState(gsys)) {
            GFL_SndBGMPlay(work->bgm, 0xffff);
            GFL_SndBGMFadeIn(60);
            *state = STEP_CLEAR_COMM_PARAM;
        }
        break;
    case STEP_CLEAR_COMM_PARAM:
        work->commParam[4] = 0;
        (*state)++;
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
    case STEP_RETURN:
        if (work->mode == COMM_TVT_FIELD_MODE_ENTRALINK_RETURN) {
            GameEvent_Replace(event, EventEntralinkWarpReturn_Create(gsys, work->exitArgument));
            return GAMEEVENT_CONTINUE;
        }
        *state = STEP_END;
        break;
    case STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventCommTvt_Create(GameSystem *gsys, void *args) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventCommTvt_Callback, sizeof(EventCommTvt));
    EventCommTvt *work = GameEvent_GetData(event);

    work->gameData = GSYS_GetGameData(gsys);
    work->save = GameData_GetSaveControl(work->gameData);
    work->gsys = gsys;
    return event;
}
