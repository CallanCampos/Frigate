#include "types.h"
#include "app/research_radar.h"
#include "field/event_research_radar.h"
#include "field/field_event.h"
#include "gfl/heap.h"
#include "system/game_event.h"
#include "system/game_system.h"

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
    case 0:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
        *state = 1;
        break;
    case 1:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        *state = 2;
        break;
    case 2:
        GSYS_QueueProc(gsys, OVERLAY_RESEARCH_RADAR_APP, &RESEARCH_RADAR_PROC_FUNCTIONS, work->param);
        *state = 3;
        break;
    case 3:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        *state = 4;
        break;
    case 4:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        *state = 5;
        break;
    case 5:
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
