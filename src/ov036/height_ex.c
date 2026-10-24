// A list of areas of the map, each a grid rectangle with a value and flags, and the events that follow the list's
// users. The name is the ROM's own, from GFL_HeapAllocate's file argument.
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/game_beacon_search.h"
#include "field/height_ex.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

struct HeightExList {
    s32 count;
    HeightExEntry *entries;
};

GameEventReturnCode EventCommError_Run(GameEvent *event, u32 *state, void *data);
BOOL func_ov036_021ba804(GameProc *proc, u32 *state, void *param, void *work);
BOOL func_ov036_021ba808(GameProc *proc, u32 *state, void *param, void *work);
BOOL func_ov036_021ba80c(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions data_ov036_021d1760 = { func_ov036_021ba804, func_ov036_021ba808, func_ov036_021ba80c };
GameEventReturnCode EventCommError_RunCommExit(GameEvent *event, u32 *state, void *data);

HeightExList *HeightEx_CreateTable(u32 count, u8 heapId) {
    u8 i = 0;
    HeightExList *list = GFL_HeapAllocate(heapId, sizeof(HeightExList), FALSE, "height_ex.c", 90);

    list->entries = GFL_HeapAllocate(heapId, count * sizeof(HeightExEntry), FALSE, "height_ex.c", 91);
    list->count = count;
    for (; i < count; i++) {
        list->entries[i].inUse = FALSE;
    }
    return list;
}

void HeightEx_SetArea(u8 index, u32 x, u32 z, u32 width, u32 depth, u32 value, u32 flags, HeightExList *list) {
    if (index < list->count) {
        if (list->entries[index].inUse == FALSE) {
            list->entries[index].x = x;
            list->entries[index].z = z;
            list->entries[index].width = width;
            list->entries[index].depth = depth;
            list->entries[index].value = value;
            list->entries[index].flags = flags;
            list->entries[index].inUse = TRUE;
        }
    }
}

void func_ov036_021ba670(HeightExList *list) {
    GFL_HeapFree(list->entries);
    GFL_HeapFree(list);
}

u32 HeightEx_GetAreaCount(HeightExList *list) {
    return list->count;
}

BOOL HeightEx_IsPointInAreaAt(s32 x, s32 z, HeightExList *list, u8 index) {
    if (HeightEx_IsPointInArea(x, z, &list->entries[index])) {
        return TRUE;
    }
    return FALSE;
}

u32 HeightEx_GetAreaHeight(u8 index, HeightExList *list) {
    return list->entries[index].value;
}

u32 HeightEx_GetAreaAttr(u8 index, HeightExList *list) {
    return list->entries[index].flags;
}

void func_ov036_021ba6bc(u8 index, BOOL inUse, HeightExList *list) {
    list->entries[index].inUse = inUse;
}

GameEvent *EventCommError_Create(GameSystem *gsys, Field *field) {
    return GameEvent_Create(gsys, NULL, EventCommError_Run, 0);
}

GameEventReturnCode EventCommError_Run(GameEvent *event, u32 *state, void *data) {
    GameSystem *gsys = GameEvent_GetGameSystem(event);
    Field *field = GSYS_GetField(gsys);
    GameEvent *next;

    switch (*state) {
    case 0:
        DisableAllActorsMovement(Field_GetActorSystem(field));
        if (GameCommSys_GetLastCommNo(GSYS_GetGameCommSystem(gsys)) == GAME_COMM_NO_INTRUDE) {
            if (GameData_IsForceSeasonSync(GSYS_GetGameData(gsys)) == TRUE) {
                GFL_NetErrMarkShown();
                next = EventFieldSubprocessTransition_Create(gsys, field, OVERLAY_ID(36), &data_ov036_021d1760, gsys);
            } else {
                next = GameEvent_Create(gsys, NULL, EventCommError_RunCommExit, 0);
            }
        } else {
            GFL_NetErrMarkShown();
            next = EventFieldSubprocessTransition_Create(gsys, field, OVERLAY_ID(36), &data_ov036_021d1760, gsys);
        }
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case 1:
        if (field != NULL && Field_CheckMapLoadFinished(field) == TRUE) {
            EnableAllActorsMovement(Field_GetActorSystem(field));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

GameEventReturnCode EventCommError_RunCommExit(GameEvent *event, u32 *state, void *data) {
    GameSystem *gsys = GameEvent_GetGameSystem(event);
    GameCommSys *comm = GSYS_GetGameCommSystem(gsys);
    void *work;

    switch (*state) {
    case 0:
        if (GFL_NetIsInitialized()) {
            GFL_NetExit(NULL);
        }
        (*state)++;
        break;
    case 1:
        if (GFL_NetIsEnded() == TRUE) {
            GFL_NetErrClear();
            GSYS_SetNetErrorPending(gsys, 0);
            (*state)++;
        }
        break;
    case 2:
        if (GSYS_TryBootGameComm(gsys) == TRUE) {
            (*state)++;
        } else {
            return TRUE;
        }
        break;
    case 3:
        work = GameCommSys_GetWork(comm);
        if (work != NULL && GameCommSys_BootCheck(comm)) {
            GameBeaconSearch_StartType35Wait(work);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL func_ov036_021ba804(GameProc *proc, u32 *state, void *param, void *work) {
    return TRUE;
}

BOOL func_ov036_021ba808(GameProc *proc, u32 *state, void *param, void *work) {
    return TRUE;
}

BOOL func_ov036_021ba80c(GameProc *proc, u32 *state, void *param, void *work) {
    GSYS_SetNetErrorPending(param, 0);
    return TRUE;
}
