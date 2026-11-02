#include "types.h"
#include "app/beacon_detail.h"
#include "field/event_beacon_detail.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_event.h"
#include "gfl/std.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The steps of the event
enum {
    STEP_FADE_OUT,
    STEP_CLOSE_FIELD,
    STEP_PUSH_BGM,
    STEP_START_SCREEN,
    STEP_WAIT_SCREEN,
    STEP_OPEN_FIELD,
    STEP_FADE_IN,
    STEP_WAIT_BGM,
    STEP_END,
};

struct EventBeaconDetail {
    GameSystem *gsys;
    GameData *gameData;
    SaveControl *save;
    BeaconDetailParam param;
    u8 logIndex;
    u8 logRow;
};

GameEventReturnCode EventBeaconDetail_Callback(GameEvent *event, u32 *state, void *data);

GameEventReturnCode EventBeaconDetail_Callback(GameEvent *event, u32 *state, void *data) {
    EventBeaconDetail *work = data;
    GameSystem *gsys = work->gsys;
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
        (*state)++;
        break;
    case STEP_PUSH_BGM:
        GameEvent_ChainNext(event, EventBGMPlayPushEx_Create(gsys, 0x483, 6, 0));
        (*state)++;
    case STEP_START_SCREEN:
        MI_CpuFill8(&work->param, 0, sizeof(work->param));
        work->param.gsys = work->gsys;
        work->param.gameData = work->gameData;
        work->param.logIndex = work->logIndex;
        work->param.logRow = work->logRow;
        GSYS_QueueProc(gsys, OVERLAY_BEACON_DETAIL, &BeaconDetail_ProcFunctions, &work->param);
        (*state)++;
        break;
    case STEP_WAIT_SCREEN:
        if (!GSYS_GetProcMgrState(gsys)) {
            GameEvent_ChainNext(event, EventBGMPop_CreateEx(gsys, 6, 60));
            (*state)++;
        }
        break;
    case STEP_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case STEP_FADE_IN:
        if (work->param.lowBatteryExit) {
            FieldSubscreen_ChangeImm(Field_GetSubscreen(field), 0);
        }
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case STEP_WAIT_BGM:
        GameEvent_ChainNext(event, EventBGMFadeWait_Create(gsys));
        (*state)++;
        break;
    case STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBeaconDetail_Create(GameSystem *gsys, const u32 *args) {
    u32 packed = args[0];
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBeaconDetail_Callback, sizeof(EventBeaconDetail));
    EventBeaconDetail *work = GameEvent_GetData(event);

    work->gameData = GSYS_GetGameData(gsys);
    work->save = GameData_GetSaveControl(work->gameData);
    work->gsys = gsys;
    work->logIndex = packed;
    work->logRow = packed >> 16;
    return event;
}
