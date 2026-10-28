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
        (*state)++;
    case 3:
        GSYS_QueueProc(gsys, OVERLAY_COMM_TVT_FIELD_APP, &COMM_TVT_FIELD_PROC_FUNCTIONS, work);
        (*state)++;
        break;
    case 4:
        if (!GSYS_GetProcMgrState(gsys)) {
            switch (work->mode) {
            case 0:
                *state = 6;
                break;
            case 1:
                work->bgm = GFL_SndBGMGetID();
                GSYS_QueueProcAsEvent(event, OVERLAY_ID(257), &COMM_TVT_PROC_FUNCTIONS, work->commParam);
                *state = 5;
                break;
            default:
                *state = 6;
                break;
            }
        }
        break;
    case 5:
        if (!GSYS_GetProcMgrState(gsys)) {
            GFL_SndBGMPlay(work->bgm, 0xffff);
            GFL_SndBGMFadeIn(60);
            *state = 6;
        }
        break;
    case 6:
        work->commParam[4] = 0;
        (*state)++;
        break;
    case 7:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case 8:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case 9:
        if (work->mode == 0) {
            GameEvent_Replace(event, func_ov033_021773e4(gsys, work->exitArgument));
            return GAMEEVENT_CONTINUE;
        }
        *state = 10;
        break;
    case 10:
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
