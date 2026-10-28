// The event that returns from the field menu to a screen of the touch screen. The ROM has no name for the file;
// field_menu_event.c is descriptive. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_menu.h"
#include "field/subscreen.h"
#include "gfl/std.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

typedef struct EventFieldMenuReturn {
    GameEvent *event;
    GameSystem *gsys;
    Field *field;
    u32 screenId;
    u8 seq;
} EventFieldMenuReturn;

// The steps of the return from the field menu: change the touch screen once it is ready, remember it once the
// change is done, and let the actors move again
enum {
    MENU_RETURN_CHANGE_SCREEN,
    MENU_RETURN_SAVE_SCREEN,
    MENU_RETURN_END,
};

static GameEventReturnCode EventFieldMenuReturn_Callback(GameEvent *event, u32 *state, void *data);

GameEvent *EventFieldMenuReturn_Create(GameSystem *gsys, Field *field, u32 screenId) {
    GameEvent *event;
    EventFieldMenuReturn *work;

    Field_GetSubscreen(field);
    event = GameEvent_Create(gsys, NULL, EventFieldMenuReturn_Callback, sizeof(EventFieldMenuReturn));
    work = GameEvent_GetData(event);
    MI_CpuFill8(work, 0, sizeof(EventFieldMenuReturn));
    work->gsys = gsys;
    work->event = event;
    work->field = field;
    work->screenId = screenId;
    return event;
}

static GameEventReturnCode EventFieldMenuReturn_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldMenuReturn *work = data;
    GameData *gameData;
    MMSys *actorSystem;

    switch (work->seq) {
    case MENU_RETURN_CHANGE_SCREEN:
        if (FieldSubscreen_IsReady(Field_GetSubscreen(work->field)) == TRUE) {
            actorSystem = Field_GetActorSystem(work->field);
            gameData = GSYS_GetGameData(GameEvent_GetGameSystem(event));
            DisableAllActorsMovement(actorSystem);
            GameData_SetFieldMenuItem(gameData, FIELD_MENU_ITEM_POKEMON);
            FieldSubscreen_ReqChange(Field_GetSubscreen(work->field), work->screenId);
            work->seq++;
        }
        break;
    case MENU_RETURN_SAVE_SCREEN:
        if (FieldSubscreen_IsReady(Field_GetSubscreen(work->field)) == TRUE) {
            GameData_SetLastSubscreen(GSYS_GetGameData(GameEvent_GetGameSystem(event)), work->screenId);
            work->seq++;
        }
        break;
    case MENU_RETURN_END:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
