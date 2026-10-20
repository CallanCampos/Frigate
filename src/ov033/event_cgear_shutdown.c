#include "types.h"
#include "field/event_cgear_shutdown.h"
#include "field/field.h"
#include "field/subscreen.h"
#include "struct_decls.h"
#include "system/game_event.h"
#include "system/game_system.h"

GameEvent *EventCGearShutdown_Create(GameSystem *gsys) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventCGearShutdown_Callback, sizeof(struct CGearShutdownData));
    struct CGearShutdownData *data = GameEvent_GetData(event);
    Field *field = GSYS_GetField(gsys);

    data->field = field;
    data->subscreen = Field_GetSubscreen(field);
    return event;
}

GameEventReturnCode EventCGearShutdown_Callback(GameEvent *event, u32 *state, void *eventData) {
    struct CGearShutdownData *data = eventData;

    switch (*state) {
    case 0:
        FieldSubscreen_ReqChange(data->subscreen, 9);
        (*state)++;
        break;
    case 1:
        if (FieldSubscreen_GetReturnSubscreen(data->subscreen) != 0xe) {
            break;
        }
        FieldSubscreen_ResetReturnSubscreen(data->subscreen);
        (*state)++;
        break;
    case 2:
        FieldSubscreen_ReqChange(data->subscreen, 0);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
