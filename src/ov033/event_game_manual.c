#include "types.h"
#include "field/event_game_manual.h"
#include "field/field_event.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/game_event.h"
#include "system/game_system.h"

GameEvent *EventGameManual_Create(GameSystem *gsys, u16 *result) {
    GameData *gameData;
    Field *field;
    GameEvent *event;
    GameManualEventWork *work;
    GameManualSubwork *subwork;

    gameData = GSYS_GetGameData(gsys);
    field = GSYS_GetField(gsys);
    event = GameEvent_Create(gsys, NULL, EventGameManual_Main, sizeof(GameManualEventWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = field;
    work->result = result;
    subwork = GFL_HeapAllocate(4, sizeof(GameManualSubwork), FALSE, "event_game_manual.c", 0x4a);
    work->subwork = subwork;
    subwork->gameData = gameData;
    return event;
}

GameEventReturnCode EventGameManual_Main(GameEvent *event, u32 *state, void *data) {
    GameManualEventWork *work;
    GameEvent *next;

    work = data;
    switch (*state) {
    case 0:
        next =
            EventFieldSubprocessTransition_Create(work->gsys, work->field, 0x13f, &gManualProcFunctions, work->subwork);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case 1:
        EventGameManual_SetResult(work);
        GFL_HeapFree(work->subwork);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void EventGameManual_SetResult(GameManualEventWork *work) {
    if (work->subwork->result == 0) {
        *work->result = 0;
    } else {
        *work->result = 1;
    }
}
