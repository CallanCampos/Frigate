// The field menu's event (a descriptive name), which opens the app chosen on the touch screen's menu. Names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_menu.h"
#include "field/field_script_event.h"
#include "field/hidden_event.h"
#include "field/itemuse_event.h"
#include "field/player_action.h"
#include "field/subscreen.h"
#include "field/zone.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

static u32 FieldMenu_GetAppId(u32 index);
static BOOL FieldMenu_CanRetry(FieldAppCallInput *input, void *arg);
static BOOL FieldMenu_CanLeaveField(FieldAppCallInput *input, void *arg);
static BOOL FieldMenu_OnAppEnd(FieldAppCallInput *input, void *arg);

GameEvent *EventFieldMenu_Create(GameSystem *gsys, Field *field, u16 param) {
    FieldSubscreen *subscreen = Field_GetSubscreen(field);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldMenu_Callback, sizeof(FieldMenuWork));
    FieldMenuWork *work = GameEvent_GetData(event);
    u32 screenId;

    MI_CpuFill8(work, 0, sizeof(FieldMenuWork));
    work->gameSystem = gsys;
    work->event = event;
    work->field = field;
    work->code = param;
    work->state = 0;
    work->appCall.gameSystem = gsys;
    work->appCall.field = field;
    work->appCall.parent = event;
    work->appCall.canRetry = FieldMenu_CanRetry;
    work->appCall.callback1 = FieldMenu_CanLeaveField;
    work->appCall.callback2 = FieldMenu_OnAppEnd;
    work->appCall.arg = work;
    work->appCall.appParam = -1;
    screenId = FieldSubscreen_GetScreenID(subscreen);
    switch (screenId) {
    case 4:
        work->screenId = 4;
        break;
    case 10:
        work->screenId = 10;
        break;
    default:
        work->screenId = FieldSubscreen_GetIDForChange(Field_GetSubscreen(work->field), 0);
        break;
    }
    work->appCall.screenId = work->screenId;
    GCTX_HIDSetUsingTouch(TRUE);
    return event;
}

GameEvent *EventFieldMenu_CreateUnionRoom(GameSystem *gsys, Field *field, u16 param) {
    GameEvent *event = EventFieldMenu_Create(gsys, field, param);
    FieldMenuWork *work = GameEvent_GetData(event);
    u16 zoneId = Field_GetPlayerStateZoneID(field);

    if (IsZone150Or151(zoneId) == 1) {
        work->screenId = 5;
    } else {
        work->screenId = 2;
    }
    return event;
}

GameEventReturnCode EventFieldMenu_Callback(GameEvent *event, u32 *state, void *data) {
    FieldMenuWork *work = data;
    MMSys *actorSystem;
    GameData *gameData;
    u32 selection;
    u32 returnSubscreen;
    PlayerActionPossibilities action;
    HiddenEventArgs args;
    GameEvent *next;

    switch (work->state) {
    case 0:
        if (FieldSubscreen_IsReady(Field_GetSubscreen(work->field)) == TRUE) {
            actorSystem = Field_GetActorSystem(work->field);
            gameData = GSYS_GetGameData(GameEvent_GetGameSystem(event));
            DisableAllActorsMovement(actorSystem);
            GameData_SetFieldMenuItem(gameData, FIELD_MENU_ITEM_POKEMON);
            FieldSubscreen_ReqChange(Field_GetSubscreen(work->field), 1);
            work->state = 1;
        }
        break;
    case 1:
        returnSubscreen = FieldSubscreen_GetReturnSubscreen(Field_GetSubscreen(work->field));
        if (returnSubscreen == FIELD_MENU_RESULT_SELECTED) {
            work->state = 2;
            FieldSubscreen_ResetReturnSubscreen(Field_GetSubscreen(work->field));
        } else if (returnSubscreen == FIELD_MENU_RESULT_CANCELLED) {
            work->state = 3;
            FieldSubscreen_ResetReturnSubscreen(Field_GetSubscreen(work->field));
        }
        break;
    case 2:
        gameData = GSYS_GetGameData(GameEvent_GetGameSystem(event));
        selection = FieldSubscreen_GetMenuSelectedButton(Field_GetSubscreen(work->field));
        GameData_SetFieldMenuItem(gameData, selection);
        work->unk10 = FieldSubscreen_GetMenuSelectedButton(Field_GetSubscreen(work->field));
        work->appCall.appId = FieldMenu_GetAppId(selection);
        work->state = 5;
        break;
    case 3:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        if (work->appCall.eventType != 1 && work->appCall.eventType != 4) {
            GameEvent_Replace(event,
                              EventFieldMenuReturn_Create(GameEvent_GetGameSystem(event), work->field, work->screenId));
            return GAMEEVENT_CONTINUE;
        }
        return GAMEEVENT_DONE;
    case 4:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        CalcPlayerActionPossibilities(GSYS_GetField(work->gameSystem), &action);
        HiddenEventArgs_Set(&args, work->appCall.partySlot, work->appCall.eventId, work->appCall.eventValue);
        next = CreateHidenEvent(work->appCall.eventId, &args, &action);
        if (next == NULL) {
            return GAMEEVENT_DONE;
        }
        GameEvent_Replace(work->event, next);
        break;
    case 5:
        GameEvent_ChainNext(event, EventFieldAppCall_Create(&work->appCall, work->code));
        work->state = 6;
        break;
    case 6:
        if (work->appCall.eventType == 0) {
            work->state = 1;
        } else if (work->appCall.eventType == 1) {
            work->state = 3;
        } else if (work->appCall.eventType == 3) {
            work->state = 7;
        } else if (work->appCall.eventType == 2) {
            work->state = 4;
        } else if (work->appCall.eventType == 4) {
            work->state = 3;
        } else if (work->appCall.eventType == 5) {
            work->state = 9;
        }
        break;
    case 7:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        GameEvent_ChainNext(event, CallFieldCommonEventFunc(work->appCall.eventId, work->gameSystem, work->field));
        work->state = 8;
        break;
    case 8:
        return GAMEEVENT_DONE;
    case 9:
        if (FieldSubscreen_IsReady(Field_GetSubscreen(work->field))) {
            EnableAllActorsMovement(Field_GetActorSystem(work->field));
            GameEvent_Replace(work->event,
                              EventScriptCall_Create(work->gameSystem, 0x2a01, NULL, Field_GetHeapID(work->field)));
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

// The app each item of the field menu opens, an index of FIELD_PROC_LINK_LIST, or -1 for none
static const s32 sFieldMenuAppIds[9] = {
    -1,
    FIELD_APP_POKELIST,
    FIELD_APP_ZUKAN,
    FIELD_APP_BAG,
    FIELD_APP_TRAINER_CARD,
    FIELD_APP_SAVE,
    FIELD_APP_CONFIG,
    -1,
    FIELD_APP_INF_DUNGEON_RETIRE,
};

static u32 FieldMenu_GetAppId(u32 index) {
    return sFieldMenuAppIds[index];
}

static BOOL FieldMenu_CanRetry(FieldAppCallInput *input, void *arg) {
    FieldMenuWork *menu = arg;
    FieldSubscreen *subscreen = Field_GetSubscreen(menu->field);

    if (input->eventType == 0) {
        FieldSubscreen_SetMenuSelectedButton(subscreen, menu->unk10);
    }
    return TRUE;
}

static BOOL FieldMenu_CanLeaveField(FieldAppCallInput *input, void *arg) {
    return TRUE;
}

static BOOL FieldMenu_OnAppEnd(FieldAppCallInput *input, void *arg) {
    FieldMenuWork *menu = arg;
    GameData *gameData = GSYS_GetGameData(menu->gameSystem);

    switch (input->eventType) {
    case 4:
        GameData_SetLastSubscreen(gameData, 6);
        break;
    case 0:
        GameData_SetLastSubscreen(gameData, 1);
        break;
    case 5:
        GameData_SetLastSubscreen(gameData, 0);
        break;
    case 1:
    case 2:
    case 3:
        GameData_SetLastSubscreen(gameData, menu->screenId);
        break;
    }
    return TRUE;
}
