#include "types.h"
#include "field/field_event.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "field/event_game_clear.h"
#include "field/field.h"
#include "field/field_sound.h"
#include "gfl/heap.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The events that close and open the field around another screen: closing it, opening it again, running a proc as a
// field subprocess with or without the map's fade, and the party list with its summary screen. The name is a guess,
// since the ROM has no string of this file.

typedef struct {
    GameSystem *gsys;
    Field *field;
    GameData *gameData;
    // Whether the field's glue overlay is unloaded once the field is closed
    BOOL unloadGlue;
} EventFieldCloseWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    GameData *gameData;
    u32 unk0C;
} EventFieldOpenWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    s32 overlayId;
    const GameProcFunctions *functions;
    void *param;
} EventFieldSubprocessWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    s32 overlayId;
    const GameProcFunctions *functions;
    void *param;
    void (*callback)(void *work);
    void *work;
} EventFieldSubprocessCallbackWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    PokeListParam *param;
    PStatusParam *summaryParam;
} EventPokeListWork;

static GameEventReturnCode EventFieldClose_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldCloseKeepSound_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldOpen_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldSubprocessTransition_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldSubprocessCall_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldSubprocessTransitionWithCallback_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldSubprocessCallWithCallback_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventPokeList_Callback(GameEvent *event, u32 *state, void *data);

static GameEventReturnCode EventFieldClose_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldCloseWork *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(wk->gameData);

    switch (*state) {
    case 0:
        Field_RequestClose(field);
        (*state)++;
        break;
    case 1:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        if (FieldSnd_IsBusy(fieldSound) == TRUE) {
            break;
        }
        if (wk->unloadGlue) {
            UnloadFieldGlueOverlay();
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *CreateFieldCloseEvent(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldClose_Callback, sizeof(EventFieldCloseWork));
    EventFieldCloseWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->gameData = GSYS_GetGameData(gsys);
    wk->unloadGlue = TRUE;
    return event;
}

// The same without waiting for the field's sound
static GameEventReturnCode EventFieldCloseKeepSound_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldCloseWork *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(wk->gameData);

    switch (*state) {
    case 0:
        Field_RequestClose(field);
        (*state)++;
        break;
    case 1:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        if (wk->unloadGlue) {
            UnloadFieldGlueOverlay();
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Closes the field without waiting for its sound, and leaves the field's glue overlay loaded
GameEvent *EventFieldCloseKeepSound_Create(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldCloseKeepSound_Callback, sizeof(EventFieldCloseWork));
    EventFieldCloseWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->gameData = GSYS_GetGameData(gsys);
    wk->unloadGlue = FALSE;
    return event;
}

static GameEventReturnCode EventFieldOpen_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldOpenWork *wk = data;
    GameSystem *gsys = wk->gsys;

    switch (*state) {
    case 0:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        GSYS_StartFieldmapProc(gsys);
        (*state)++;
        break;
    case 1:
        if (!Field_CheckMapLoadFinished(GSYS_GetField(gsys))) {
            break;
        }
        (*state)++;
        break;
    case 2:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Opens the field after CreateFieldCloseEvent, loading the field's glue overlay again
GameEvent *EventFieldOpen_CreateHeadless(GameSystem *gsys) {
    Field *field = NULL;
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldOpen_Callback, sizeof(EventFieldOpenWork));
    EventFieldOpenWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->gameData = GSYS_GetGameData(gsys);
    LoadFieldGlueOverlay();
    return event;
}

// Opens the field after EventFieldCloseKeepSound_Create
GameEvent *EventFieldOpen_Create(GameSystem *gsys) {
    Field *field = NULL;
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldOpen_Callback, sizeof(EventFieldOpenWork));
    EventFieldOpenWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->gameData = GSYS_GetGameData(gsys);
    return event;
}

static GameEventReturnCode EventFieldSubprocessTransition_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldSubprocessWork *wk = data;
    GameSystem *gsys = wk->gsys;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, wk->field, 0, 0));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event,
                            EventFieldSubprocessCall_Create(gsys, wk->field, wk->overlayId, wk->functions, wk->param));
        (*state)++;
        break;
    case 2:
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, wk->field, 0, 0, 1, 0, 0));
        (*state)++;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode EventFieldSubprocessCall_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldSubprocessWork *wk = data;
    GameSystem *gsys = wk->gsys;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, wk->field));
        (*state)++;
        break;
    case 1:
        GSYS_QueueProc(gsys, wk->overlayId, wk->functions, wk->param);
        (*state)++;
        break;
    case 2:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case 4:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Runs the proc as a field subprocess, fading the map out before and in after
GameEvent *EventFieldSubprocessTransition_Create(GameSystem *gsys, Field *field, s32 overlayId,
                                                 const GameProcFunctions *functions, void *param) {
    GameEvent *event =
        GameEvent_Create(gsys, NULL, EventFieldSubprocessTransition_Callback, sizeof(EventFieldSubprocessWork));
    EventFieldSubprocessWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->overlayId = overlayId;
    wk->functions = functions;
    wk->param = param;
    return event;
}

GameEvent *EventFieldSubprocessCall_Create(GameSystem *gsys, Field *field, s32 overlayId, const GameProcFunctions *functions,
                                           void *param) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldSubprocessCall_Callback, sizeof(EventFieldSubprocessWork));
    EventFieldSubprocessWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->overlayId = overlayId;
    wk->functions = functions;
    wk->param = param;
    return event;
}

static GameEventReturnCode EventFieldSubprocessTransitionWithCallback_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldSubprocessCallbackWork *wk = data;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, EventFieldSubprocessTransition_Create(wk->gsys, wk->field, wk->overlayId,
                                                                         wk->functions, wk->param));
        (*state)++;
        break;
    case 1:
        if (wk->callback != NULL) {
            wk->callback(wk->work);
        }
        (*state)++;
        break;
    case 2:
        if (wk->work != NULL) {
            GFL_HeapFree(wk->work);
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode EventFieldSubprocessCallWithCallback_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldSubprocessCallbackWork *wk = data;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(
            event, EventFieldSubprocessCall_Create(wk->gsys, wk->field, wk->overlayId, wk->functions, wk->param));
        (*state)++;
        break;
    case 1:
        if (wk->callback != NULL) {
            wk->callback(wk->work);
        }
        (*state)++;
        break;
    case 2:
        if (wk->work != NULL) {
            GFL_HeapFree(wk->work);
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventFieldSubprocessTransition_CreateWithCallback(GameSystem *gsys, Field *field, u32 overlayId, const GameProcFunctions *functions, void *param,
                         void (*callback)(void *work), void *work) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldSubprocessTransitionWithCallback_Callback, sizeof(EventFieldSubprocessCallbackWork));
    EventFieldSubprocessCallbackWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->overlayId = overlayId;
    wk->functions = functions;
    wk->param = param;
    wk->callback = callback;
    wk->work = work;
    return event;
}

GameEvent *EventFieldSubprocessCall_CreateWithCallback(GameSystem *gsys, Field *field, s32 overlayId,
                                                       const GameProcFunctions *functions, void *param,
                                                       void (*callback)(void *work), void *work) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldSubprocessCallWithCallback_Callback, sizeof(EventFieldSubprocessCallbackWork));
    EventFieldSubprocessCallbackWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->overlayId = overlayId;
    wk->functions = functions;
    wk->param = param;
    wk->callback = callback;
    wk->work = work;
    return event;
}

// Switches between the party list and the summary screen until the list is done. The field is closed already.
static GameEventReturnCode EventPokeList_Callback(GameEvent *event, u32 *state, void *data) {
    EventPokeListWork *wk = data;
    GameSystem *gsys = wk->gsys;

    switch (*state) {
    case 0:
        GSYS_QueueProc(gsys, OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, wk->param);
        *state = 1;
        break;
    case 1:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        *state = 2;
        break;
    case 2:
        if (wk->param->result == 0) {
            *state = 5;
        } else if (wk->param->result == 1) {
            wk->summaryParam->partyIndex = wk->param->index;
            *state = 3;
        } else {
            *state = 5;
        }
        break;
    case 3:
        GSYS_QueueProc(gsys, OVERLAY_PSTATUS, &PSTATUS_PROC_FUNCTIONS, wk->summaryParam);
        *state = 4;
        break;
    case 4:
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        wk->param->index = wk->summaryParam->partyIndex;
        *state = 0;
        break;
    case 5:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventPokeList_Create(GameSystem *gsys, Field *field, PokeListParam *param, void *summaryParam) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventPokeList_Callback, sizeof(EventPokeListWork));
    EventPokeListWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->param = param;
    wk->summaryParam = summaryParam;
    return event;
}
