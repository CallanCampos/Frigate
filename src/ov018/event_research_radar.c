#include "types.h"
#include "app/research_radar.h"
#include "field/event_research_radar.h"
#include "field/field_event.h"
#include "gfl/heap.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The steps of the event
enum {
    STEP_FADE_OUT,
    STEP_CLOSE_FIELD,
    STEP_START_APP,
    STEP_OPEN_FIELD,
    STEP_FADE_IN,
    STEP_END,
};

struct EventResearchRadar {
    GameSystem *gsys;
    Field *field;
    GameSystem **param;
};

static char sFile[] = "event_research_radar.c";

GameEventReturnCode EventResearchRadar_Callback(GameEvent *event, u32 *state, void *data);

GameEventReturnCode EventResearchRadar_Callback(GameEvent *event, u32 *state, void *data) {
    EventResearchRadar *work = data;
    GameSystem *gsys = work->gsys;
    Field *field = work->field;

    switch (*state) {
    case STEP_FADE_OUT:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
        *state = STEP_CLOSE_FIELD;
        break;
    case STEP_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        *state = STEP_START_APP;
        break;
    case STEP_START_APP:
        GSYS_QueueProc(gsys, OVERLAY_RESEARCH_RADAR_APP, &RESEARCH_RADAR_PROC_FUNCTIONS, work->param);
        *state = STEP_OPEN_FIELD;
        break;
    case STEP_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        *state = STEP_FADE_IN;
        break;
    case STEP_FADE_IN:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        *state = STEP_END;
        break;
    case STEP_END:
        GFL_HeapFree(work->param);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventResearchRadar_Create(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventResearchRadar_Callback, sizeof(EventResearchRadar));
    EventResearchRadar *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->field = field;
    work->param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(*work->param), FALSE, sFile, 117);
    *work->param = gsys;
    return event;
}
