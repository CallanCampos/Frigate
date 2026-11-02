#include "types.h"
#include "app/pass_power.h"
#include "constants/items.h"
#include "constants/sound.h"
#include "field/event_pass_power.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_pass_power.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "save/bag.h"
#include "save/high_link.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The steps of the event: the field closes for the Pass Power screen and opens again, then what the screen chose
// follows
enum {
    STEP_FADE_OUT,
    STEP_CLOSE_FIELD,
    STEP_PUSH_BGM,
    STEP_START_APP,
    STEP_WAIT_APP,
    STEP_OPEN_FIELD,
    STEP_FADE_IN,
    STEP_CHOOSE_NEXT,
    STEP_ACTIVATE_POWER,
    STEP_ENTRALINK,
    STEP_END,
};

struct EventPassPower {
    GameSystem *gsys;
    GameData *gameData;
    PassPowerParam param;
    u32 unused50;
    u32 paramArgument;
    void *exitArgument;
};

void EventPassPower_SetupParam(PassPowerParam *param, u32 argument, GameSystem *gsys, GameData *gameData, u16 heapId);
GameEventReturnCode EventPassPower_Callback(GameEvent *event, u32 *state, void *data);

void EventPassPower_SetupParam(PassPowerParam *param, u32 argument, GameSystem *gsys, GameData *gameData, u16 heapId) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    BagSave *bag;
    HighLinkSave *highLink;
    int j;
    int i;
    int limit;
    PassPowerEntry temp;

    param->gsys = gsys;
    param->argument = argument;
    bag = GameData_GetBag(gameData);
    param->itemCount = BagSave_GetItemCountByID(bag, ITEM_PASS_ORB, heapId);
    highLink = getHighLinkBlockAddress(save);
    param->highLinkCount = 0;
    for (i = 0; i < HIGH_LINK_POWER_MAX; i++) {
        u32 id = HighLinkSave_GetPassPower(highLink, i);
        if (id != HIGH_LINK_POWER_NONE) {
            param->highLinkIds[param->highLinkCount++] = id;
        }
    }
    for (i = 0; i < PASS_POWER_EFFECT_COUNT; i++) {
        u32 id = PassPower_GetUsedIDByEffect(i);
        if (id != PASS_POWER_NONE) {
            if (param->powerCount >= NELEMS(param->powers)) {
                break;
            }
            param->powers[param->powerCount].id = id;
            param->powers[param->powerCount++].seconds = PassPower_GetRemainingSeconds(i);
        }
    }
    limit = param->powerCount - 1;
    for (i = 0; i < limit; i++) {
        for (j = limit; j > i; j--) {
            if (param->powers[j - 1].seconds > param->powers[j].seconds) {
                temp = param->powers[j];
                param->powers[j] = param->powers[j - 1];
                param->powers[j - 1] = temp;
            }
        }
        limit = param->powerCount - 1;
    }
}

GameEventReturnCode EventPassPower_Callback(GameEvent *event, u32 *state, void *data) {
    EventPassPower *work = data;
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
        GameEvent_ChainNext(event, EventBGMPlayPushEx_Create(gsys, SEQ_BGM_GAME_SYNC, 6, 0));
        (*state)++;
    case STEP_START_APP:
        EventPassPower_SetupParam(&work->param, work->paramArgument, gsys, gameData, HEAPID_TAIL(HEAPID_GAMEEVENT));
        GSYS_QueueProc(gsys, OVERLAY_PASS_POWER_APP, &GPOWER_APP_PROC_FUNCTIONS, &work->param);
        (*state)++;
        break;
    case STEP_WAIT_APP:
        if (!GSYS_GetProcMgrState(gsys)) {
            GameEvent_ChainNext(event, EventBGMPop_CreateEx(gsys, 6, 60));
            (*state)++;
        }
        break;
    case STEP_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case STEP_FADE_IN: {
        FieldSubscreen *subscreen = Field_GetSubscreen(field);
        if (work->param.result == PASS_POWER_APP_ABORTED) {
            FieldSubscreen_ChangeImm(subscreen, 0);
        }
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    }
    case STEP_CHOOSE_NEXT:
        GameEvent_ChainNext(event, EventBGMFadeWait_Create(gsys));
        switch (work->param.result) {
        case PASS_POWER_APP_USE:
            *state = STEP_ACTIVATE_POWER;
            break;
        case PASS_POWER_APP_ENTRALINK:
            *state = STEP_ENTRALINK;
            break;
        default:
            *state = STEP_END;
            break;
        }
        break;
    case STEP_ACTIVATE_POWER: {
        u32 args[2];
        args[0] = work->param.passPower;
        args[1] = 1;
        GameEvent_Replace(event,
                          GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(156), EventPassPowerActivate_Create, args));
        return GAMEEVENT_CONTINUE;
    }
    case STEP_ENTRALINK:
        GameEvent_Replace(event, EventEntralinkWarpReturn_Create(gsys, work->exitArgument));
        return GAMEEVENT_CONTINUE;
    case STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventPassPower_Create(GameSystem *gsys, const u32 *args) {
    u32 argument = args[0];
    GameEvent *event = GameEvent_Create(gsys, NULL, EventPassPower_Callback, sizeof(EventPassPower));
    EventPassPower *work = GameEvent_GetData(event);

    work->gameData = GSYS_GetGameData(gsys);
    work->gsys = gsys;
    work->paramArgument = argument;
    return event;
}
