// A list of areas of the map, each a grid rectangle with a value and flags, and the events that follow the list's
// users. The name is the ROM's own, from GFL_HeapAllocate's file argument.
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/game_beacon_search.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

typedef struct HeightExEntry {
    s32 x;
    s32 z;
    s32 width;
    s32 depth;
    s32 value;
    BOOL inUse;
    s32 flags;
} HeightExEntry;

typedef struct HeightExList {
    s32 count;
    HeightExEntry *entries;
} HeightExList;

GameEventReturnCode func_ov036_021ba6dc(GameEvent *event, u32 *state, void *data);
GameEvent *func_ov036_021ba6c8(GameSystem *gsys);
u32 func_ov036_021ba684(void *a0);
BOOL func_ov036_021ba804(GameProc *proc, u32 *state, void *param, void *work);
BOOL func_ov036_021ba808(GameProc *proc, u32 *state, void *param, void *work);
BOOL func_ov036_021ba80c(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions data_ov036_021d1760 = { func_ov036_021ba804, func_ov036_021ba808, func_ov036_021ba80c };
GameEventReturnCode func_ov036_021ba77c(GameEvent *event, u32 *state, void *data);

void *func_ov036_021ba5d0(u32 count, u8 heapId) {
    u8 i = 0;
    HeightExList *list = GFL_HeapAllocate(heapId, sizeof(HeightExList), FALSE, "height_ex.c", 90);

    list->entries = GFL_HeapAllocate(heapId, count * sizeof(HeightExEntry), FALSE, "height_ex.c", 91);
    list->count = count;
    for (; i < count; i++) {
        list->entries[i].inUse = FALSE;
    }
    return list;
}

void func_ov036_021ba624(u8 index, u32 x, u32 z, u32 width, u32 depth, u32 value, u32 flags, void *listPtr) {
    HeightExList *list = listPtr;

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

void func_ov036_021ba670(void *listPtr) {
    HeightExList *list = listPtr;

    GFL_HeapFree(list->entries);
    GFL_HeapFree(list);
}

u32 func_ov036_021ba684(void *a0) {
    return *(u32 *)a0;
}

BOOL func_ov036_021ba688(s32 x, s32 z, void *listPtr, u8 index) {
    HeightExList *list = listPtr;

    if (func_ov036_021ba594(x, z, &list->entries[index])) {
        return TRUE;
    }
    return FALSE;
}

u32 func_ov036_021ba6a4(u8 index, void *listPtr) {
    HeightExList *list = listPtr;

    return list->entries[index].value;
}

u32 func_ov036_021ba6b0(u8 index, void *listPtr) {
    HeightExList *list = listPtr;

    return list->entries[index].flags;
}

void func_ov036_021ba6bc(u8 index, BOOL inUse, void *listPtr) {
    HeightExList *list = listPtr;

    list->entries[index].inUse = inUse;
}

GameEvent *func_ov036_021ba6c8(GameSystem *gsys) {
    return GameEvent_Create(gsys, NULL, func_ov036_021ba6dc, 0);
}

GameEventReturnCode func_ov036_021ba6dc(GameEvent *event, u32 *state, void *data) {
    GameSystem *gsys = GameEvent_GetGameSystem(event);
    Field *field = GSYS_GetField(gsys);
    GameEvent *next;

    switch (*state) {
    case 0:
        DisableAllActorsMovement(Field_GetActorSystem(field));
        if (GameCommSys_GetLastCommNo(GSYS_GetGameCommSystem(gsys)) == 2) {
            if (GameData_IsForceSeasonSync(GSYS_GetGameData(gsys)) == TRUE) {
                GFL_NetErrMarkShown();
                next = EventFieldSubprocessTransition_Create(gsys, field, OVERLAY_ID(36), &data_ov036_021d1760, gsys);
            } else {
                next = GameEvent_Create(gsys, NULL, func_ov036_021ba77c, 0);
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

GameEventReturnCode func_ov036_021ba77c(GameEvent *event, u32 *state, void *data) {
    GameSystem *gsys = GameEvent_GetGameSystem(event);
    GameCommSys *comm = GSYS_GetGameCommSystem(gsys);
    void *work;

    switch (*state) {
    case 0:
        if (func_02042788()) {
            func_02042860(NULL);
        }
        (*state)++;
        break;
    case 1:
        if (func_020427a4() == TRUE) {
            func_02012144();
            func_02016b0c(gsys, 0);
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
            func_ov012_0215f94c(work);
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
    func_02016b0c(param, 0);
    return TRUE;
}
