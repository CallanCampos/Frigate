#include "types.h"
#include "gfl/arc.h"
#include "field/event_data.h"
#include "field/field_script.h"
#include "field/field_script_event.h"
#include "field/field_script_plugin.h"
#include "field/field_script_supervisor.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/main.h"
#include "system/vm.h"

struct EventScriptCallData {
    ScriptWork *work;
    FieldScriptSupervisor *supervisor;
};

GameEvent *EventScriptCall_Create(GameSystem *gsys, u16 scriptId, FieldActor *actor, HeapID heapId) {
    return EventScriptCall_CreateCore(gsys, heapId, scriptId, actor, 0);
}

ScriptWork *EventScriptCall_Start(GameEvent *event, u16 scriptId, FieldActor *actor, u32 param, HeapID heapId) {
    GameEvent *next = EventScriptCall_CreateCore(GameEvent_GetGameSystem(event), heapId, scriptId, actor, param);
    EventScriptCallData *data = GameEvent_GetData(next);

    GameEvent_ChainNext(event, next);
    return data->work;
}

ScriptWork *EventScriptCall_Replace(GameEvent *event, u16 scriptId, FieldActor *actor, HeapID heapId) {
    GameEvent *next = EventScriptCall_CreateCore(GameEvent_GetGameSystem(event), heapId, scriptId, actor, 0);
    EventScriptCallData *data = GameEvent_GetData(next);

    GameEvent_Replace(event, next);
    return data->work;
}

BOOL FieldScript_VMExists(ScriptWork *work, u8 index) {
    FieldScriptSupervisor *supervisor = ScriptWork_GetSupervisor(work);

    if (supervisor == NULL) {
        return FALSE;
    }
    if (FieldScriptSupervisor_GetVM(supervisor, index) != NULL) {
        return TRUE;
    }
    return FALSE;
}

FieldScriptSupervisor *FieldScriptSupervisor_Create(HeapID heapId) {
    FieldScriptSupervisor *supervisor;

    supervisor = GFL_HeapAllocate(heapId, sizeof(FieldScriptSupervisor), TRUE, "script_sys.c", 0xf1);
    supervisor->heapId = heapId;
    return supervisor;
}

void FieldScriptSupervisor_Free(FieldScriptSupervisor *supervisor) {
    GFL_HeapFree(supervisor);
}

BOOL FieldScriptSupervisor_Update(FieldScriptSupervisor *supervisor) {
    int i;
    VM *vm;

    for (i = 0; i < 3; i++) {
        vm = supervisor->vms[i];
        if (vm != NULL && !VM_Run(vm)) {
            supervisor->vms[i] = NULL;
            supervisor->vmCount--;
            FieldScript_FreeVM(vm);
        }
    }
    return supervisor->vmCount != 0;
}

int FieldScriptSupervisor_AddVM(FieldScriptSupervisor *supervisor, VM *vm) {
    int i;

    for (i = 0; i < 3; i++) {
        if (supervisor->vms[i] == NULL) {
            supervisor->vms[i] = vm;
            supervisor->vmCount++;
            return i;
        }
    }
    return 3;
}

VM *FieldScriptSupervisor_GetVM(FieldScriptSupervisor *supervisor, int index) {
    return supervisor->vms[index];
}

void FieldScriptTerminator_ReplaceEvent(GameEvent *event, void *arg) {
    GameEvent_Replace(event, arg);
}

void FieldScriptSupervisor_SetPostEvent(FieldScriptSupervisor *supervisor, GameEvent *event) {
    supervisor->postFunc = FieldScriptTerminator_ReplaceEvent;
    supervisor->postArg = event;
}

void FieldScriptSupervisor_CallPostFunc(FieldScriptSupervisor *supervisor, GameEvent *event) {
    supervisor->postFunc(event, supervisor->postArg);
}

BOOL FieldScriptSupervisor_HasPostFunc(FieldScriptSupervisor *supervisor) {
    return supervisor->postFunc != NULL;
}

u16 FieldScript_GetZoneIDFromGSys(GameSystem *gsys) {
    return PlayerState_GetZoneID(GSYS_GetPlayerState(gsys));
}

GameEventReturnCode EventScriptCall_Callback(GameEvent *event, u32 *state, void *arg) {
    EventScriptCallData *data = arg;
    ScriptWork *work;
    FieldScriptSupervisor *supervisor;

    supervisor = data->supervisor;
    work = data->work;
    if (FieldScriptSupervisor_Update(supervisor) == 1) {
        return FALSE;
    }
    ScriptWork_Free(work);
    if (FieldScriptSupervisor_HasPostFunc(supervisor) == 1) {
        FieldScriptSupervisor_CallPostFunc(supervisor, event);
        FieldScriptSupervisor_Free(supervisor);
        return FALSE;
    }
    FieldScriptSupervisor_Free(supervisor);
    return TRUE;
}

GameEvent *EventScriptCall_CreateCore(GameSystem *gsys, HeapID heapId, u16 scriptId, FieldActor *actor, u32 param) {
    GameEvent *event;
    EventScriptCallData *data;
    u16 zoneId;

    event = GameEvent_Create(gsys, NULL, EventScriptCall_Callback, sizeof(EventScriptCallData));
    data = GameEvent_GetData(event);
    data->supervisor = FieldScriptSupervisor_Create(4);
    data->work = ScriptWork_Create(4, gsys, event, scriptId, param, 0);
    ScriptWork_SetParentActor(data->work, actor);
    zoneId = FieldScript_GetZoneIDFromGSys(gsys);
    ScriptWork_AddVM(data->work, zoneId, scriptId);
    return event;
}

void ScriptWork_CallEvent(ScriptWork *work, GameEvent *event) {
    GameEvent_ChainNext(ScriptWork_GetEvent(work), event);
}

void ScriptWork_SetPostEvent(ScriptWork *work, GameEvent *event) {
    FieldScriptSupervisor *supervisor = ScriptWork_GetSupervisor(work);

    if (supervisor != NULL) {
        FieldScriptSupervisor_SetPostEvent(supervisor, event);
    }
}

u32 ScriptWork_AddVM(ScriptWork *work, u16 zoneId, u16 scriptId) {
    FieldScriptSupervisor *supervisor;
    VM *vm;

    supervisor = ScriptWork_GetSupervisor(work);
    if (supervisor != NULL) {
        vm = FieldScript_CreateVM(supervisor->heapId, work, zoneId, scriptId, 0);
        return FieldScriptSupervisor_AddVM(supervisor, vm);
    }
    return 3;
}

ScriptWork *EventScriptCall_GetWork(GameEvent *event) {
    EventScriptCallData *data = GameEvent_GetData(event);

    return data->work;
}

FieldScriptSupervisor *ScriptWork_GetSupervisor(ScriptWork *work) {
    GameEvent *event = ScriptWork_GetEvent(work);

    if (event == NULL) {
        return NULL;
    }
    return ((EventScriptCallData *)GameEvent_GetData(event))->supervisor;
}

void FieldScript_ResetMapLocalEvents(EventWork *eventWork) {
    EventWork_FlagResetRange(eventWork, 0, 0x63);
    EventWork_WorkResetRange(eventWork, 0x4000, 0x401f);
}

void FieldScript_CallPlayerInitSetup(GameSystem *gsys, u32 a1) {
    FieldScript_Run(gsys, NULL, 0x2580, 2);
}

void FieldScript_CallPlayerPostHOFSetup(GameSystem *gsys, u32 unused) {
    FieldScript_Run(gsys, NULL, 0x2581, 2);
}

void FieldScript_Run(GameSystem *gsys, ScriptWork *work, u16 scriptId, u32 featureLevel) {
    ScriptWork *owned;
    u16 zoneId;
    VM *vm;

    owned = NULL;
    if (work == NULL) {
        owned = ScriptWork_Create(4, gsys, NULL, scriptId, 0, featureLevel);
        work = owned;
    }
    zoneId = FieldScript_GetZoneIDFromGSys(gsys);
    vm = FieldScript_CreateVM(4, work, zoneId, scriptId, featureLevel);
    while (VM_Run(vm) == 1) {
    }
    FieldScript_FreeVM(vm);
    if (owned != NULL) {
        ScriptWork_Free(owned);
    }
}

u32 FieldScript_CallZoneInitCore(GameSystem *gsys, u32 arg1, u32 mode, u32 featureLevel) {
    GameData *gameData;
    EventData *eventData;
    const u8 *script;
    u16 scriptId;
    u32 result;

    result = 0;
    gameData = GSYS_GetGameData(gsys);
    eventData = GameData_GetEventData(gameData);
    script = GetZoneInitScrPointer(eventData);
    do {
        script = FieldScript_GetInitSCRID(script, mode, &scriptId);
        if (scriptId != 0xffff) {
            FieldScript_Run(gsys, NULL, scriptId, featureLevel);
            result = 1;
        }
    } while (scriptId != 0xffff);
    return result;
}

void FieldScript_CallOnZoneReload(GameSystem *gsys, u32 arg1) {
    FieldScript_CallZoneInitCore(gsys, arg1, 4, 1);
}

void FieldScript_CallOnZoneNewLoad(GameSystem *gsys, u32 arg1) {
    FieldScript_CallZoneInitCore(gsys, arg1, 3, 1);
}

void FieldScript_CallOnZoneInit(GameSystem *gsys, u32 arg1) {
    GameData *gameData;
    EventWork *eventWork;

    gameData = GSYS_GetGameData(gsys);
    eventWork = GameData_GetEventWork(gameData);
    FieldScript_ResetMapLocalEvents(eventWork);
    FieldScript_CallZoneInitCore(gsys, arg1, 2, 2);
}

GameEvent *FieldScript_CheckSceneChangeEvent(GameSystem *gsys, HeapID heapId) {
    GameData *gameData;
    EventData *eventData;
    u16 scriptId;

    gameData = GSYS_GetGameData(gsys);
    eventData = GameData_GetEventData(gameData);
    scriptId = FieldScript_GetSceneChangeSCRID(gameData, GetZoneInitScrPointer(eventData), 1);
    if (scriptId == 0xffff) {
        return NULL;
    }
    return EventScriptCall_Create(gsys, scriptId, NULL, heapId);
}

const u8 *FieldScript_GetInitSCRID(const u8 *script, u32 mode, u16 *scriptId) {
    u16 type;
    u16 value;

    while (TRUE) {
        type = script[0] + (script[1] << 8);
        if (type == 0) {
            *scriptId = 0xffff;
            return script;
        }
        if (mode == type) {
            value = script[2] + (script[3] << 8);
            *scriptId = value;
            return script + 6;
        }
        script += 6;
    }
}

u16 FieldScript_GetSceneChangeSCRID(GameData *gameData, const u8 *script, u32 mode) {
    EventWork *eventWork;
    u16 type;
    u16 workId;
    u16 value;
    u32 offset;

    while (TRUE) {
        type = script[0] + (script[1] << 8);
        if (type == 0) {
            return 0xffff;
        }
        if (mode == type) {
            offset = script[2] + (script[3] << 8) + (script[4] << 16) + (script[5] << 24);
            script += 6;
            break;
        }
        script += 6;
    }
    if (offset == 0) {
        return 0xffff;
    }
    script += offset;
    eventWork = GameData_GetEventWork(gameData);
    while (TRUE) {
        workId = script[0] + (script[1] << 8);
        if (workId == 0) {
            return 0xffff;
        }
        // The original parser checks the terminator again before reading the entry.
        if (workId == 0) {
            return 0xffff;
        }
        value = script[2] + (script[3] << 8);
        if (*EventWork_GetWkPtr(eventWork, workId) == value) {
            return script[4] + (script[5] << 8);
        }
        script += 6;
    }
}

VM *FieldScript_CreateVM(HeapID heapId, ScriptWork *work, u16 zoneId, u16 scriptId, u32 featureLevel) {
    FieldScriptEnvArgs args;
    VMInitParam param;
    FieldScriptEnv *env;
    GameSystem *gsys;
    GameData *gameData;
    VM *vm;
    u16 fileId;
    u16 msgArcId;
    u16 msgFileNo;
    u32 scriptOffset;
    u32 reduced;
    u32 workSize;

    reduced = FieldScript_IsVMFeatureSetReduced(featureLevel);
    args.zoneId = zoneId;
    args.unk02 = scriptId;
    args.featureLevel = featureLevel;
    args.reducedFeatureLevel = reduced;
    args.work = work;
    env = CreateFieldScriptEnv(&args, heapId);
    gsys = ScriptWork_GetGameSystem(work);
    gameData = GSYS_GetGameData(gsys);
    param.stackSize = 0x100;
    workSize = 0x40;
    param.workSize = workSize;
    param.commands = (const VMCommand *)EVCMD_TABLE;
    param.commandCount = EVCMD_MAX;
    param.extraCommands = (const VMCommand *)GetCurrentScrPluginTable(gameData);
    param.extraCommandCount = GetCurrentScrPluginCmdCount(gameData);
    param.extraCommandStart = 1000;
    vm = VM_Create(heapId, &param);
    VM_ChangeEnv(vm, env);
    scriptOffset = FieldScript_ResolveSCRID(zoneId, scriptId, &fileId, &msgArcId, &msgFileNo);
    vm->unk38 = (u32)FieldScript_LoadData(fileId, heapId);
    workSize -= 0x41;
    if (!reduced && msgFileNo != workSize) {
        SetFieldScriptEnvMsgData(env, msgArcId, msgFileNo);
    }
    VM_LoadScript(vm, (void *)vm->unk38);
    vm->pc += scriptOffset * 4;
    vm->pc += VM_Read32(vm);
    FieldScript_AttachOpcodeGuard(vm);
    return vm;
}

void FieldScript_FreeVM(VM *vm) {
    FieldScriptEnv *env;

    env = VM_GetEnv(vm);
    FreeFieldScriptEnv(env);
    GFL_HeapFree((void *)vm->unk38);
    VM_Free(vm);
}

u32 FieldScript_ResolveSCRID(u32 zoneId, u16 scriptId, u16 *fileId, u16 *msgArcId, u16 *msgFileNo) {
    u32 i;
    u16 zoneScriptFile;
    u16 zoneTextFile;
    register const GlobalScriptEntry *table;

    table = GLOBAL_SCRIPT_TABLE;
    for (i = 0; i < 60; i++) {
        if (scriptId >= table[i].start) {
            GFL_ASSERT_MSG(scriptId <= table[i].end, "id=%d end=%d", scriptId, table[i].end);
            *fileId = table[i].fileId;
            *msgArcId = table[i].msgArcId;
            *msgFileNo = table[i].msgFileNo;
            return (u16)(scriptId - table[i].start);
        }
    }
    if (scriptId >= 1) {
        zoneScriptFile = ZoneData_GetScriptDatID(zoneId);
        zoneTextFile = ZoneData_GetTextDatID(zoneId);
        *fileId = zoneScriptFile;
        *msgArcId = 3;
        *msgFileNo = zoneTextFile;
        return (u16)(scriptId - 1);
    }
    *fileId = 0x4ce;
    *msgArcId = 3;
    *msgFileNo = 0xd0;
    return 0;
}

// Where a script file is read from. Only the archive is left in the retail game, but the test of the source stays in
// the code, which MWCC only does for a variable of an enum type
typedef enum {
    SCRIPT_SOURCE_ARCHIVE,
    SCRIPT_SOURCE_OTHER,
} ScriptSource;

void *FieldScript_LoadData(u16 fileId, HeapID heapId) {
    ScriptSource source = SCRIPT_SOURCE_ARCHIVE;
    void *data = NULL;

    if (source == SCRIPT_SOURCE_ARCHIVE) {
        data = GFL_ArcSysReadHeapNewLZ(0x38, fileId, FALSE, heapId);
    }
    return data;
}

BOOL FieldScript_CheckSCRID(u16 scriptId) {
    if (scriptId == 0x7d0) {
        return FALSE;
    }
    if (scriptId < 0x2aa2) {
        return TRUE;
    }
    return FALSE;
}

u32 FieldScript_IsVMFeatureSetReduced(u32 featureLevel) {
    if (featureLevel != 0) {
        return 1;
    }
    return 0;
}

void FieldScript_ThrowOpcodeAccessError(void) {
    if (func_02042788()) {
        func_020428a0();
        func_02042860(0);
        func_020429f0();
        do {
            func_020428e0();
        } while (!func_020427a4());
    }
    while (TRUE) {
        func_02005430();
    }
}

BOOL FieldScript_OpcodeGuard(VM *vm, void *env, void *gsys, u16 cmd) {
    u32 featureLevel;
    u32 allowed;

    GSYS_GetField((GameSystem *)gsys);
    FieldScriptEnv_IsReducedFeatureLevel((FieldScriptEnv *)env);
    featureLevel = FieldScriptEnv_GetFeatureLevel((FieldScriptEnv *)env);
    allowed = 0;
    switch (featureLevel) {
    case 0:
        allowed = EVCMD_PERM_TABLE[cmd].level0;
        break;
    case 1:
        allowed = EVCMD_PERM_TABLE[cmd].level1;
        break;
    case 2:
        allowed = EVCMD_PERM_TABLE[cmd].level2;
        break;
    }
    if (!allowed) {
        FieldScript_ThrowOpcodeAccessError();
    }
    return allowed;
}

void FieldScript_AttachOpcodeGuard(VM *vm) {
    FieldScriptEnv *env;
    GameSystem *gsys;

    env = VM_GetEnv(vm);
    gsys = FieldScriptEnv_GetGameSystem(env);
    VM_SetCallbackVerifier(vm, FieldScript_OpcodeGuard, gsys);
}
