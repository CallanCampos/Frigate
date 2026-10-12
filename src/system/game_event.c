#include "types.h"
#include "system/game_event.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "system/game_system.h"

// The game's events: each is a callback with a state and its own data, run once a frame by the game system until it
// is done, when the event that called it goes on. The file's name is the ROM's string, from GFL_HeapAllocate's
// calls. The names of the functions are swan's

struct GameEvent {
    // The event that goes on when this one is done
    GameEvent *parent;
    GameEventCallback callback;
    u32 state;
    void *data;
    GameSystem *gsys;
};

typedef struct EventProcessCallWork {
    s32 overlayId;
    const GameProcFunctions *functions;
    void *param;
} EventProcessCallWork;

typedef struct EventCallOverlayDelegateWork {
    u32 overlayId;
    GameEvent *event;
    // Never written or read
    u32 unused8;
    u32 unusedC;
} EventCallOverlayDelegateWork;

static GameEventReturnCode GameEvent_RunCallback(GameEvent *event);
static void GameEvent_Free(GameEvent *event);
static GameEventReturnCode EventProcessCall_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventCallOverlayDelegateEvent_Callback(GameEvent *event, u32 *state, void *data);

GameEvent *GameEvent_Create(GameSystem *gsys, GameEvent *parent, GameEventCallback callback, u32 size) {
    GameEvent *event = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(GameEvent), TRUE, "game_event.c", 55);

    event->parent = parent;
    event->callback = callback;
    event->state = 0;
    event->data = GFL_HeapAllocate(HEAPID_GAMEEVENT, size, TRUE, "game_event.c", 59);
    event->gsys = gsys;
    return event;
}

static GameEventReturnCode GameEvent_RunCallback(GameEvent *event) {
    return event->callback(event, &event->state, event->data);
}

static void GameEvent_Free(GameEvent *event) {
    if (event->data != NULL) {
        GFL_HeapFree(event->data);
    }
    GFL_HeapFree(event);
}

void GameEvent_Transplant(GameEvent *event, GameEventCallback callback, u32 size) {
    event->state = 0;
    event->callback = callback;
    if (event->data != NULL) {
        GFL_HeapFree(event->data);
    }
    event->data = GFL_HeapAllocate(HEAPID_GAMEEVENT, size, TRUE, "game_event.c", 96);
}

void GameEvent_Replace(GameEvent *event, GameEvent *next) {
    GameSystem *gsys = event->gsys;

    next->parent = event->parent;
    GameEvent_Free(event);
    GSYS_SetNowEvent(gsys, next);
}

void GameEvent_ChainNext(GameEvent *event, GameEvent *next) {
    next->parent = event;
    GSYS_SetNowEvent(event->gsys, next);
}

BOOL GSYS_UpdateEvents(GameSystem *gsys) {
    GameEvent *event;
    GameEvent *parent;

    while (TRUE) {
        event = GSYS_GetNowEvent(gsys);
        if (event == NULL) {
            return FALSE;
        }
        switch (GameEvent_RunCallback(event)) {
        case GAMEEVENT_CONTINUE:
            return FALSE;
        case GAMEEVENT_DONE:
            parent = GameEvent_GetReturnEvent(event);
            GameEvent_Free(event);
            GSYS_SetNowEvent(gsys, parent);
            if (parent == NULL) {
                return TRUE;
            }
            break;
        case GAMEEVENT_CONTINUE_DIRECT:
            break;
        }
    }
}

BOOL GSYS_CheckNowEvent(GameSystem *gsys) {
    if (GSYS_GetNowEvent(gsys) != NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL GSYS_FeedEventsFromProvider(GameSystem *gsys, GameEventProvider provider, void *data) {
    GameEvent *event;

    if (GSYS_GetNowEvent(gsys) != NULL) {
        return FALSE;
    }
    if (provider == NULL) {
        return FALSE;
    }
    event = provider(gsys, data);
    if (event != NULL) {
        GSYS_SetNowEvent(gsys, event);
        return TRUE;
    }
    return FALSE;
}

static GameEventReturnCode EventProcessCall_Callback(GameEvent *event, u32 *state, void *data) {
    EventProcessCallWork *work = data;
    GameSystem *gsys = GameEvent_GetGameSystem(event);

    switch (*state) {
    case 0:
        GSYS_QueueProc(gsys, work->overlayId, work->functions, work->param);
        (*state)++;
        break;
    case 1:
        if (!GSYS_GetProcMgrState(gsys)) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

void GSYS_QueueProcAsEvent(GameEvent *event, s32 overlayId, const GameProcFunctions *functions, void *param) {
    GameEvent *procEvent = GameEvent_Create(event->gsys, NULL, EventProcessCall_Callback, sizeof(EventProcessCallWork));
    EventProcessCallWork *work = GameEvent_GetData(procEvent);

    work->overlayId = overlayId;
    work->functions = functions;
    work->param = param;
    GameEvent_ChainNext(event, procEvent);
}

static GameEventReturnCode EventCallOverlayDelegateEvent_Callback(GameEvent *event, u32 *state, void *data) {
    EventCallOverlayDelegateWork *work = data;
    GameSystem *gsys = GameEvent_GetGameSystem(event);

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, work->event);
        (*state)++;
        break;
    case 1:
        GFL_OvlUnload(work->overlayId);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *GameEvent_CreateOverlayDelegate(GameSystem *gsys, u32 overlayId, GameEventProvider provider, void *args) {
    GameEvent *event =
        GameEvent_Create(gsys, NULL, EventCallOverlayDelegateEvent_Callback, sizeof(EventCallOverlayDelegateWork));
    EventCallOverlayDelegateWork *work = GameEvent_GetData(event);

    work->overlayId = overlayId;
    GFL_OvlLoad(overlayId);
    work->event = provider(gsys, args);
    return event;
}

GameSystem *GameEvent_GetGameSystem(GameEvent *event) {
    return event->gsys;
}

void *GameEvent_GetData(GameEvent *event) {
    return event->data;
}

u32 *GameEvent_GetStatePtr(GameEvent *event) {
    return &event->state;
}

GameEvent *GameEvent_GetReturnEvent(GameEvent *event) {
    return event->parent;
}
