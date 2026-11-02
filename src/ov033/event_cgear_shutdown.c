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

// The steps of the shutdown: show the C-Gear's power-off screen, wait for it to finish, and go back to the default
// subscreen
enum {
    CGEAR_SHUTDOWN_STEP_POWER_OFF,
    CGEAR_SHUTDOWN_STEP_WAIT,
    CGEAR_SHUTDOWN_STEP_END,
};

GameEventReturnCode EventCGearShutdown_Callback(GameEvent *event, u32 *state, void *eventData) {
    struct CGearShutdownData *data = eventData;

    switch (*state) {
    case CGEAR_SHUTDOWN_STEP_POWER_OFF:
        FieldSubscreen_ReqChange(data->subscreen, FLD_SUBSCREEN_ID_CGEAR_POWEROFF);
        (*state)++;
        break;
    case CGEAR_SHUTDOWN_STEP_WAIT:
        if (FieldSubscreen_GetReturnSubscreen(data->subscreen) != 0xe) {
            break;
        }
        FieldSubscreen_ResetReturnSubscreen(data->subscreen);
        (*state)++;
        break;
    case CGEAR_SHUTDOWN_STEP_END:
        FieldSubscreen_ReqChange(data->subscreen, FLD_SUBSCREEN_ID_DEFAULT);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
