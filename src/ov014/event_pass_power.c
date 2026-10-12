#include "types.h"
#include "app/pass_power.h"
#include "field/event_pass_power.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_pass_power.h"
#include "gfl/overlay.h"
#include "save/bag.h"
#include "save/high_link.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

struct EventPassPower {
    GameSystem *gsys;
    GameData *gameData;
    PassPowerParam param;
    u32 unused50;
    u32 paramArgument;
    void *exitArgument;
};

void func_ov014_0216e660(PassPowerParam *param, u32 argument, GameSystem *gsys, GameData *gameData, u16 item);
GameEventReturnCode func_ov014_0216e73c(GameEvent *event, u32 *state, void *data);

void func_ov014_0216e660(PassPowerParam *param, u32 argument, GameSystem *gsys, GameData *gameData, u16 item) {
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
    param->itemCount = BagSave_GetItemCountByID(bag, 0x23f, item);
    highLink = getHighLinkBlockAddress(save);
    param->highLinkCount = 0;
    for (i = 0; i < 3; i++) {
        u32 id = HighLinkSave_GetPassPower(highLink, i);
        if (id != 0x30) {
            param->highLinkIds[param->highLinkCount++] = id;
        }
    }
    for (i = 0; i < 16; i++) {
        u32 id = PassPower_GetUsedIDByEffect(i);
        if (id != 0x30) {
            if (param->powerCount >= 10) {
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

GameEventReturnCode func_ov014_0216e73c(GameEvent *event, u32 *state, void *data) {
    EventPassPower *work = data;
    GameSystem *gsys = work->gsys;
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, field, 0, 0));
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
        func_ov014_0216e660(&work->param, work->paramArgument, gsys, gameData, 0x8004);
        GSYS_QueueProc(gsys, OVERLAY_PASS_POWER_APP, &GPOWER_APP_PROC_FUNCTIONS, &work->param);
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
    case 6: {
        FieldSubscreen *subscreen = Field_GetSubscreen(field);
        if (work->param.result == PASS_POWER_APP_ABORTED) {
            FieldSubscreen_ChangeImm(subscreen, 0);
        }
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, field, 0, 0, 1, 0, 0));
        (*state)++;
        break;
    }
    case 7:
        GameEvent_ChainNext(event, EventBGMFadeWait_Create(gsys));
        switch (work->param.result) {
        case PASS_POWER_APP_USE:
            *state = 8;
            break;
        case PASS_POWER_APP_ENTRALINK:
            *state = 9;
            break;
        default:
            *state = 10;
            break;
        }
        break;
    case 8: {
        u32 args[2];
        args[0] = work->param.passPower;
        args[1] = 1;
        GameEvent_Replace(event, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(156), EventPassPowerActivate_Create, args));
        return GAMEEVENT_CONTINUE;
    }
    case 9:
        GameEvent_Replace(event, func_ov033_021773e4(gsys, work->exitArgument));
        return GAMEEVENT_CONTINUE;
    case 10:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *func_ov014_0216e8ac(GameSystem *gsys, const u32 *args) {
    u32 argument = args[0];
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov014_0216e73c, sizeof(EventPassPower));
    EventPassPower *work = GameEvent_GetData(event);

    work->gameData = GSYS_GetGameData(gsys);
    work->gsys = gsys;
    work->paramArgument = argument;
    return event;
}
