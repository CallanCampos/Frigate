#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_script.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/wordset.h"

void UpdateScriptFieldWk(ScriptFieldWork *fieldWork, GameSystem *gsys) {
    Field *field;

    field = GSYS_GetField(gsys);
    MIi_CpuClear32(0, fieldWork, sizeof(ScriptFieldWork));
    fieldWork->field = field;
    if (field != NULL) {
        fieldWork->msgBGSys = Field_GetMsgBGSys(field);
    }
}

ScriptWork *ScriptWork_Create(HeapID heapId, GameSystem *gsys, GameEvent *event, u16 scriptId, u32 arg4,
                              u32 featureLevel) {
    ScriptWork *work;
    u32 reduced;

    work = GFL_HeapAllocate(heapId, sizeof(ScriptWork), TRUE, "script_work.c", 0xaf);
    work->unk00 = 0x3643f;
    work->heapId = heapId;
    work->gsys = gsys;
    work->event = event;
    work->scriptId = scriptId;
    work->parentActor = NULL;
    work->unk20 = arg4;
    work->featureLevel = featureLevel;
    reduced = FieldScript_IsVMFeatureSetReduced(featureLevel);
    work->reducedFeatureLevel = reduced;
    if (reduced == 0) {
        work->wordSet = GFL_WordSetSystemCreate(0x1c, 0x40, heapId);
        work->mainStrBuf = GFL_StrBufCreate(0x500, heapId);
        work->altStrBuf = GFL_StrBufCreate(0x500, heapId);
    } else {
        work->wordSet = NULL;
        work->mainStrBuf = NULL;
        work->altStrBuf = NULL;
    }
    work->subwork = InitScriptSubwork(work, heapId);
    UpdateScriptFieldWk(&work->fieldWork, gsys);
    if (work->reducedFeatureLevel == 0) {
        FieldScriptSubEvent_ResetAll();
    }
    return work;
}

void ScriptWork_Free(ScriptWork *work) {
    work->unk00 = 0;
    if (work->wordSet != NULL) {
        GFL_WordSetSystemFree(work->wordSet);
    }
    if (work->mainStrBuf != NULL) {
        GFL_StrBufFree(work->mainStrBuf);
    }
    if (work->altStrBuf != NULL) {
        GFL_StrBufFree(work->altStrBuf);
    }
    FreeScriptSubwork(work->subwork);
    GFL_HeapFree(work);
}

GameEvent *ScriptWork_GetEvent(ScriptWork *work) {
    return work->event;
}

GameSystem *ScriptWork_GetGameSystem(ScriptWork *work) {
    return work->gsys;
}

ScriptFieldWork *ScriptWork_GetFieldWork(ScriptWork *work) {
    UpdateScriptFieldWk(&work->fieldWork, work->gsys);
    return &work->fieldWork;
}

ScriptSubwork *ScriptWork_GetSubwork(ScriptWork *work) {
    return work->subwork;
}

WordSet *ScriptWork_GetWordSet(ScriptWork *work) {
    return work->wordSet;
}

StrBuf *ScriptWork_GetMainStrBuf(ScriptWork *work) {
    return work->mainStrBuf;
}

StrBuf *ScriptWork_GetAltStrBuf(ScriptWork *work) {
    return work->altStrBuf;
}

void ScriptWork_SetSignFont(ScriptWork *work, Font *font) {
    work->signFont = font;
}

Font *ScriptWork_GetSignFont(ScriptWork *work) {
    return work->signFont;
}

u8 *ScriptWork_GetSEBitMask(ScriptWork *work) {
    return &work->seBitMask;
}

u16 ScriptWork_GetSCRID(ScriptWork *work) {
    return work->scriptId;
}

FieldActor *ScriptWork_GetParentActor(ScriptWork *work) {
    return work->parentActor;
}

void ScriptWork_SetParentActor(ScriptWork *work, FieldActor *actor) {
    work->parentActor = actor;
    if (actor != NULL) {
        *ScriptWork_GetLocalWork(work, 0x8011) = GetActorUID(actor);
    }
}

void **ScriptWork_GetUserHeapPtr(ScriptWork *work) {
    return &work->userHeap;
}

void *ScriptWork_GetUserHeap(ScriptWork *work) {
    return work->userHeap;
}

void ScriptWork_FreeUserHeap(ScriptWork *work) {
    if (work->userHeap != NULL) {
        GFL_HeapFree(work->userHeap);
        work->userHeap = NULL;
    }
}

u16 *ScriptWork_GetLocalWork(ScriptWork *work, u16 id) {
    return &work->localWork[id - 0x8000];
}

u16 *ScriptWork_GetWkAddr(ScriptWork *work, GameData *gameData, u16 id) {
    EventWork *eventWork = GameData_GetEventWork(gameData);
    if (id < 0x4000) {
        return NULL;
    }
    if (id < 0x8000) {
        return EventWork_GetWkPtr(eventWork, id);
    }
    if (id < 0xc000) {
        return ScriptWork_GetLocalWork(work, id);
    }
    return NULL;
}

u16 ScriptWork_ResolveHybridValue(ScriptWork *work, GameData *gameData, u16 value) {
    u16 *ptr = ScriptWork_GetWkAddr(work, gameData, value);
    if (ptr != NULL) {
        value = *ptr;
    }
    return value;
}

BOOL ScriptWork_SetWkValue(ScriptWork *work, u16 id, u32 value) {
    GameData *gameData = GSYS_GetGameData(work->gsys);
    u16 *ptr = ScriptWork_GetWkAddr(work, gameData, id);
    if (ptr == NULL) {
        return FALSE;
    }
    *ptr = value;
    return TRUE;
}

void ScriptWork_SetParams(ScriptWork *work, u16 param0, u32 param1, u16 param2, u16 param3) {
    ScriptWork_SetWkValue(work, 0x8000, param0);
    ScriptWork_SetWkValue(work, 0x8001, param1);
    ScriptWork_SetWkValue(work, 0x8002, param2);
    ScriptWork_SetWkValue(work, 0x8003, param3);
}

void *ScriptWork_CreateVarCopy(ScriptWork *work) {
    void *copy;
    u16 *vars;

    copy = GFL_HeapAllocate(work->heapId, 0x7e, TRUE, "script_work.c", 0x21b);
    vars = ScriptWork_GetLocalWork(work, 0x8020);
    MIi_CpuCopy16(vars, copy, 0x7e);
    return copy;
}

void ScriptWork_RestoreVarCopy(ScriptWork *work, void *copy) {
    u16 *vars;

    vars = ScriptWork_GetLocalWork(work, 0x8020);
    MIi_CpuCopy16(copy, vars, 0x7e);
    GFL_HeapFree(copy);
}

void ScriptWork_SetActorAnmProc(ScriptWork *work, FieldActorAnmProc *proc) {
    work->actorAnmProc = proc;
}

FieldActorAnmProc *ScriptWork_GetActorAnmProc(ScriptWork *work) {
    return work->actorAnmProc;
}

void ScriptWork_SetStadiumTrainers(ScriptWork *work, StadiumTrainerEntry *trainers) {
    work->stadiumTrainers = trainers;
}

StadiumTrainerEntry *ScriptWork_GetStadiumTrainers(ScriptWork *work) {
    return work->stadiumTrainers;
}

TrainerClashSlot *ScriptWork_GetTrainerState(ScriptWork *work, int index) {
    return &work->trainerState[index];
}
