#include "types.h"
#include "app/medal_info.h"
#include "field/event_medal_info_beacon.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_event.h"
#include "gfl/std.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

struct EventMedalInfoBeacon {
    GameSystem *gsys;
    GameData *gameData;
    SaveControl *save;
    MedalInfoBeaconParam param;
    u8 subscreen;
    u8 mode;
};

GameEventReturnCode EventMedalInfoBeacon_Callback(GameEvent *event, u32 *state, void *data);

GameEventReturnCode EventMedalInfoBeacon_Callback(GameEvent *event, u32 *state, void *data) {
    EventMedalInfoBeacon *work = data;
    GameSystem *gsys = work->gsys;
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
        (*state)++;
        break;
    case 2:
        GameEvent_ChainNext(event, EventBGMPlayPushEx_Create(gsys, 0x483, 6, 0));
        (*state)++;
    case 3:
        MI_CpuFill8(&work->param, 0, sizeof(work->param));
        work->param.gsys = work->gsys;
        work->param.gameData = work->gameData;
        work->param.beaconIndex = work->subscreen;
        work->param.logRow = work->mode;
        GSYS_QueueProc(gsys, OVERLAY_MEDAL_INFO, &MedalInfoBeacon_ProcFunctions, &work->param);
        (*state)++;
        break;
    case 4:
        if (!GSYS_GetProcMgrState(gsys)) {
            GameEvent_ChainNext(event, EventBGMPop_CreateEx(gsys, 6, 60));
            (*state)++;
        }
        break;
    case 5:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case 6:
        if (work->param.returnToField) {
            FieldSubscreen_ChangeImm(Field_GetSubscreen(field), 0);
        }
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case 7:
        GameEvent_ChainNext(event, EventBGMFadeWait_Create(gsys));
        (*state)++;
        break;
    case 8:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventMedalInfoBeacon_Create(GameSystem *gsys, const u32 *args) {
    u32 packed = args[0];
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMedalInfoBeacon_Callback, sizeof(EventMedalInfoBeacon));
    EventMedalInfoBeacon *work = GameEvent_GetData(event);

    work->gameData = GSYS_GetGameData(gsys);
    work->save = GameData_GetSaveControl(work->gameData);
    work->gsys = gsys;
    work->subscreen = packed;
    work->mode = packed >> 16;
    return event;
}
