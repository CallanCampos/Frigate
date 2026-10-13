#include "types.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/player_action.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/os.h"
#include "save/event_work.h"

// The field actors (fldmmdl.c, after the file name that its allocations pass): the system that holds them, their
// creation from a zone's entities, their state, and the saved positions of the strength boulders

// The grid coordinate of a world coordinate
#define POS_TO_GRID(pos) ((s16)(((pos) >> 4) / FX32_ONE))

// The object code of a boulder, which sits lower on the ground than the others
#define OBJCODE_STRENGTH_ROCK 0x6d
// An object code of no actor
#define OBJCODE_NONE 0xffff
// The object codes that stand for the object code in a work value, from WKOBJCODE00
#define OBJCODE_WKOBJCODE_FIRST 0xa2
#define OBJCODE_WKOBJCODE_LAST 0xb1
#define WORK_OBJCODE_FIRST 0x4020

// The configs that the cache can hold
#define CONFIG_CACHE_MAX 26

// The slot of a boulder that has none in the table
#define STRENGTH_ROCK_SLOT_NONE 80

// A boulder whose position is saved, by the zone and the ID of its actor
typedef struct {
    u16 zoneId;
    u16 uid;
    u16 slot;
    u16 unk6;
} StrengthRockSlot;

// The actors on a grid position
typedef struct {
    s32 count;
    FieldActor *actors[16];
} FieldActorList;

static void func_ov012_02166744(MMSys *system);
static FieldActor *CreateNewActorByEntity(MMSys *system, const ZoneNPC *npc, s32 zoneId, EventWork *eventWork);
static void LoadZoneNPCParamsToMMDL(FieldActor *actor, const ZoneNPC *npc);
static void SetupNPCPositionBasic(FieldActor *actor, const ZoneNPC *npc);
static void LoadActorParamsByZoneNPC(FieldActor *actor, const ZoneNPC *npc, EventWork *eventWork, MMSys *system);
static void SetupActorOnRailIfRail(FieldActor *actor, const ZoneNPC *npc, u32 unused);
static void InitActorZoneNPCGridPositon(FieldActor *actor, const ZoneNPC *npc);
static void InitNewActor(FieldActor *actor, MMSys *system, s32 zoneId);
static void SetAllActorDirToDefault(FieldActor *actor);
static void UpdateActorMoveCodeExecutor(FieldActor *actor);
static void func_ov012_02166c88(MMSys *system, FieldActor *actor);
static void func_ov012_02166cac(MMSys *system, FieldActor *actor);
static void func_ov012_02166cd0(FieldActor *actor);
static void func_ov012_02166e04(FieldActor *actor);
static void func_ov012_02166e20(TCB *tcb, void *data);
static void func_ov012_02166e2c(FieldActor *actor);
static void FldActSys_SetFlag(MMSys *system, u32 flag);
static void ClearMMSysFlag(MMSys *system, u32 flag);
static void IncrementMMSysActorCount(MMSys *system);
static void FldActSys_DecrementActorCount(MMSys *system);
static void SetMMDLActorUID(FieldActor *actor, u16 uid);
static void SetActorZoneID(FieldActor *actor, u16 zoneId);
static void FldAct_SetObjCode(FieldActor *actor, u16 objCode);
static void SetActorMoveCode(FieldActor *actor, u16 moveCode);
static void SetActorSpawnFlag(FieldActor *actor, u16 flag);
static BOOL GetActorSCRIDIsMinusOne(FieldActor *actor);
static void func_ov012_0216719c(FieldActor *actor);
static void FldAct_InvokeOnCreateCallback(FieldActor *actor);
static void FldAct_InvokeSuspendCallback(FieldActor *actor);
static void FldAct_InvokeWakeUpCallback(FieldActor *actor);
static void SetActorDefaultGPosX(FieldActor *actor, s16 x);
static void SetActorDefaultGPosY(FieldActor *actor, s16 y);
static void SetActorDefaultGPosZ(FieldActor *actor, s16 z);
static BOOL func_ov012_021673b4(MMSys *system);
static void func_ov012_021674f4(FieldActor *actor);
static void func_ov012_02167500(FieldActor *actor);
static void SetActorIsNonInteractibleFlag(FieldActor *actor, BOOL value);
static BOOL checkBit11Set(FieldActor *actor);
static BOOL isBit19Set(FieldActor *actor);
static BOOL FldAct_IsForceOffShadow(FieldActor *actor);
static s32 GetAllActorsOverGPos(MMSys *system, s16 x, s16 z, BOOL checkInit, FieldActorList *list);
static FieldActor *GetPtrToFirstFreeActorSlot(MMSys *system);
static void func_ov012_02167b34(FieldActor *actor, MMSys *system);
static void func_ov012_02167ce0(FieldActor *actor);
static void FldAct_LoadVTable(FieldActor *actor);
static void func_ov012_02167d40(FieldActor *actor);
static void FldAct_Reset(FieldActor *actor);
static void func_ov012_02167d74(FieldActor *actor);
static void FldActSys_InitConfigArc(MMSys *system, HeapID heapId);
static void FldActSys_FreeConfigArc(MMSys *system);
static void LoadInfoForNewActorModelID(MMSys *system, u16 objCode, FieldActor *actor);
static u16 GetWKOBJCODENPCOBJCODE(EventWork *eventWork, u16 objCode);
static const FieldActorMoveCode *GetMoveCodeExecutorFunction(u16 moveCode);
static const FieldActorSceneNodeVTable *FldAct_GetMainVTableAddr(u8 sceneNodeType);
static BOOL IsNPCSCRID0xFFFF(const ZoneNPC *npc);
static BOOL CheckNPCSpawnFlag(EventWork *eventWork, u16 flag);
static BOOL IsStrengthRockLastPositionInSaveData(StrengthRockSave *save, s32 slot);
static u16 GetStrengthRockSaveDataPositionSlotID(u16 zoneId, u16 uid);
static u16 GetActorSaveDataPositionSlotID(FieldActor *actor);
static BOOL IsActorLastPositionInSaveData(FieldActor *actor);
static BOOL LoadSaveDataStrengthRockPosition(FieldActor *actor, VecFx32 *pos);

static StrengthRockSlot RELOADABLE_STRENGTH_ROCKS[] = {
    { 0x13f, 0, 0 },
    { 0x14d, 0, 1 },
    { 0x14d, 1, 2 },
    { 0x14d, 2, 3 },
    { 0x14e, 0, 4 },
    { 0x14e, 1, 5 },
    { 0x14e, 2, 6 },
    { 0x14f, 0, 7 },
    { 0x14f, 1, 8 },
    { 0xe2, 0, 9 },
    { 0xe0, 0, 10 },
    { 0xd8, 0, 11 },
    { 0x172, 0, 12 },
    { 0x172, 1, 13 },
    { 0x17a, 0, 14 },
    { 0x17a, 1, 15 },
    { 0x183, 0, 16 },
    { 0x183, 1, 17 },
    { 0x98, 1, 18 },
    { 0xd0, 0, 19 },
    { 0xe7, 0, 20 },
    { 0xe7, 1, 21 },
    { 0xe7, 3, 22 },
    { 0xe7, 2, 23 },
    { 0x17f, 1, 24 },
    { 0x15a, 1, 25 },
    { 0x1e5, 0, 26 },
    { 0x1e5, 1, 27 },
    { 0x1e5, 2, 28 },
    { 0x1e5, 3, 29 },
    { 0x24b, 0, 30 },
    { 0x24b, 1, 31 },
    { 0x252, 0, 32 },
    { 0x252, 1, 33 },
    { 0x24e, 0, 34 },
    { 0x24e, 1, 35 },
    { 0x24e, 2, 36 },
    { 0x24e, 3, 37 },
    { 0x1db, 10, 38 },
    { 0x1db, 11, 39 },
    { 0x1db, 12, 40 },
    { 0x1db, 13, 41 },
    { 0x1db, 14, 42 },
    { 0x1db, 15, 43 },
    { 0x1fa, 2, 44 },
    { 0x1fa, 3, 45 },
    { 0x1fb, 3, 46 },
    { 0x1fb, 4, 47 },
    { 0x1fc, 4, 48 },
    { 0x1fc, 5, 49 },
    { 0x204, 4, 50 },
    { 0x204, 5, 51 },
    { 0x204, 6, 52 },
    { 0x204, 7, 53 },
    { 0x204, 8, 54 },
    { 0x204, 9, 55 },
    { 0x204, 10, 56 },
    { 0x1da, 12, 57 },
    { 0x1fd, 0, 58 },
    { 0x1fd, 1, 59 },
    { 0x1fd, 2, 60 },
    { 0x1f7, 5, 61 },
    { 0x1f8, 8, 62 },
    { 0x1f8, 9, 63 },
    { 0x1f9, 4, 64 },
    { 0xffff, 0xffff, STRENGTH_ROCK_SLOT_NONE },
};

MMSys *FldActSys_Create(HeapID heapId, u32 capacity, StrengthRockSave *save) {
    MMSys *system = GFL_HeapAllocate(heapId, sizeof(MMSys), TRUE, "fldmmdl.c", 202);

    system->actorHeap = GFL_HeapAllocate(heapId, capacity * sizeof(FieldActor), TRUE, "fldmmdl.c", 203);
    system->actorCapacity = capacity;
    system->heapId = heapId;
    system->mmodelSave = save;
    system->unkC = 0x80;
    system->unk10 = 1;
    FldActSys_InitConfigArc(system, heapId);
    system->actorConfigCache = GFL_HeapAllocate(heapId, 0x310, TRUE, "fldmmdl.c", 215);
    return system;
}

void FldActSys_Free(MMSys *system) {
    FldActSys_FreeConfigArc(system);
    GFL_HeapFree(system->actorConfigCache);
    GFL_HeapFree(system->actorHeap);
    GFL_HeapFree(system);
}

void FldActSys_ClearCache(MMSys *system) {
    sys_memset(system->actorConfigCache, 0, 0x310);
}

void FldActSys_AttachField(MMSys *system, HeapID heapId, GameData *gameData, Field *field, FieldG3DMapper *mapper,
                           NoGridMapper *noGridMapper, void *colorPostFx) {
    system->g3dMapper = mapper;
    system->fieldHeapId = heapId;
    system->colorPostFx = colorPostFx;
    system->gameData = gameData;
    system->field = field;
    system->tcbMgrHeap =
        GFL_HeapAllocate(heapId, GFL_TCBMgrCalcAllocSize(system->actorCapacity), FALSE, "fldmmdl.c", 274);
    system->tcbMgr = GFL_TCBMgrCreate(system->actorCapacity, system->tcbMgrHeap);
    system->dataArc = GFL_ArcSysCreateFileHandle(0x30, heapId);
    system->noGridMapper = noGridMapper;
    FldActSys_SetFlag(system, 0x10);
}

static void func_ov012_02166744(MMSys *system) {
    func_0203a610(system->tcbMgr);
    GFL_HeapFree(system->tcbMgrHeap);
    GFL_ArcToolFree(system->dataArc);
    ClearMMSysFlag(system, 0x10);
}

void func_ov012_02166764(MMSys *system) {
    func_ov036_0218ed18(system);
    system->cameraAngle = NULL;
    func_ov012_02166744(system);
}

void FldActSys_Update(MMSys *system) {
    GFL_TCBMgrUpdate(system->tcbMgr);
    FldActSys_UpdateResourceLoader(system);
}

void FldActSys_FinishAsyncMatLoadSafe(MMSys *system) {
    if (system->fieldBlAct != NULL) {
        FldActSys_FinishAsyncMatLoad(system);
    }
}

void FldActSys_ForceFullSync(MMSys *system) {
    while (FldActSys_IsAsyncLoadPending(system) == TRUE) {
        FldActSys_Update(system);
        CPU_WaitIntrBit(TRUE, 1);
        FldActSys_FinishAsyncMatLoadSafe(system);
    }
}

void func_ov012_021667cc(MMSys *system, u16 *cameraAngle) {
    system->cameraAngle = cameraAngle;
    func_ov012_021673c8(system, TRUE);
}

static FieldActor *CreateNewActorByEntity(MMSys *system, const ZoneNPC *npc, s32 zoneId, EventWork *eventWork) {
    FieldActor *actor;
    ZoneNPC entity = *npc;

    actor = GetPtrToFirstFreeActorSlot(system);
    InitNewActor(actor, system, zoneId);
    LoadActorParamsByZoneNPC(actor, &entity, eventWork, system);
    SetAllActorDirToDefault(actor);
    if (IsActorLastPositionInSaveData(actor) == TRUE) {
        SetActorFlag(actor, 0x280);
    }
    if (CheckMMSysFlag(system, 0x10)) {
        func_ov012_02166c88(system, actor);
        func_ov012_02166cac(system, actor);
    }
    if (CheckMMSysFlag(system, 1)) {
        func_ov012_02167ce0(actor);
    }
    IncrementMMSysActorCount(GetActorMModelSystem(actor));
    SetupActorOnRailIfRail(actor, &entity, 0);
    return actor;
}

FieldActor *CreateNewActorByEntityNoWKOBJCODE(MMSys *mmSys, const ZoneNPC *npc, u32 zoneId) {
    return CreateNewActorByEntity(mmSys, npc, zoneId, NULL);
}

FieldActor *CreateNewActorByParam(MMSys *mmSys, s16 x, s16 z, u16 dir, u16 id, u16 objCode, u16 moveCode,
                                  u32 zoneId) {
    ZoneNPC npc;
    ZoneNPCGridPosition *pos;

    sys_memset(&npc, 0, sizeof(ZoneNPC));
    npc.uid = id;
    npc.modelId = objCode;
    npc.moveCode = moveCode;
    npc.direction = dir;
    npc.isRail = FALSE;
    pos = &npc.pos.grid;
    pos->x = x;
    pos->z = z;
    return CreateNewActorByEntityNoWKOBJCODE(mmSys, &npc, zoneId);
}

void SpawnAllZoneNPCs(MMSys *mmSys, ZoneNPC *npcs, s32 zoneId, u32 count, EventWork *eventWork) {
    do {
        if (IsNPCSCRID0xFFFF(npcs) == TRUE || CheckNPCSpawnFlag(eventWork, npcs->spawnFlag) == FALSE) {
            CreateNewActorByEntity(mmSys, npcs, zoneId, eventWork);
        }
        npcs++;
    } while (--count);
}

FieldActor *func_ov012_021668f8(MMSys *mmSys, ZoneNPC *npcs, s32 zoneId, u32 count, EventWork *eventWork, u16 uid) {
    // BUG: The actor is not set when no entity has the ID
#ifdef BUGFIX
    FieldActor *actor = NULL;
#else
    FieldActor *actor;
#endif

    do {
        if (IsNPCSCRID0xFFFF(npcs) == FALSE && npcs->uid == uid && CheckNPCSpawnFlag(eventWork, npcs->spawnFlag) == FALSE) {
            actor = CreateNewActorByEntity(mmSys, npcs, zoneId, eventWork);
            break;
        }
        npcs++;
    } while (--count);
    return actor;
}

static inline void RemoveActorTCB(FieldActor *actor) {
    if (actor->tcb != NULL) {
        GFL_TCBRemove(actor->tcb);
        actor->tcb = NULL;
    }
}

void DeleteActor(FieldActor *actor) {
    if (func_ov012_021673b4(actor->actorSystem)) {
        if (actor->sceneNodeVTable->onDelete != NULL) {
            actor->sceneNodeVTable->onDelete(actor);
        }
    }
    if (actor->flags & 2) {
        if (actor->moveCodeVTable->unkC != NULL) {
            actor->moveCodeVTable->unkC(actor);
        }
        RemoveActorTCB(actor);
    }
    actor->actorSystem->actorCount--;
    sys_memset32(0, actor, sizeof(FieldActor));
}

void FldActSys_DeleteAllActors(MMSys *mmSys) {
    u32 index = 0;
    FieldActor *actor;

    while (NextActor(mmSys, &actor, &index)) {
        if (func_ov012_021673b4(mmSys) == TRUE) {
            if (actor->sceneNodeVTable->onDelete != NULL) {
                actor->sceneNodeVTable->onDelete(actor);
            }
        }
        // The result is not used: the actor's task is not removed here, as DeleteActor does
        CheckActorFlag(actor, 2);
        FldActSys_DecrementActorCount(actor->actorSystem);
        FldAct_Reset(actor);
    }
}

static void LoadZoneNPCParamsToMMDL(FieldActor *actor, const ZoneNPC *npc) {
    SetMMDLActorUID(actor, npc->uid);
    SetActorMoveCode(actor, npc->moveCode);
    SetActorEvType(actor, npc->evType);
    SetActorSpawnFlag(actor, npc->spawnFlag);
    SetActorSCRID(actor, npc->scrId);
    actor->defaultDir = npc->direction;
    actor->motionDir = npc->direction;
    SetActorUserParam(actor, npc->param0, 0);
    SetActorUserParam(actor, npc->param1, 1);
    SetActorUserParam(actor, npc->param2, 2);
    SetActorAreaW(actor, npc->areaW);
    SetActorAreaH(actor, npc->areaH);
}

static void SetupNPCPositionBasic(FieldActor *actor, const ZoneNPC *npc) {
    if (npc->isRail == FALSE) {
        InitActorZoneNPCGridPositon(actor, npc);
    } else {
        SetRailActorFlag(actor);
    }
}

static void LoadActorParamsByZoneNPC(FieldActor *actor, const ZoneNPC *npc, EventWork *eventWork, MMSys *system) {
    u16 objCode;
    const FieldActorConfig *config;

    LoadZoneNPCParamsToMMDL(actor, npc);
    objCode = GetWKOBJCODENPCOBJCODE(eventWork, npc->modelId);
    FldAct_SetObjCode(actor, objCode);
    LoadInfoForNewActorModelID(system, objCode, actor);
    config = &actor->config;
    if (objCode == 0xb9) {
        actor->collisionWidth = npc->param0;
        actor->collisionHeight = npc->param1;
        if (actor->collisionWidth == 0) {
            actor->collisionWidth++;
        }
        if (actor->collisionHeight == 0) {
            actor->collisionHeight++;
        }
    } else {
        actor->collisionWidth = config->collWidth;
        actor->collisionHeight = config->collHeight;
    }
    actor->modelPosOffsetX = config->wPosOffsetX;
    actor->modelPosOffsetY = config->wPosOffsetY;
    actor->modelPosOffsetZ = config->wPosOffsetZ;
    SetupNPCPositionBasic(actor, npc);
}

static void SetupActorOnRailIfRail(FieldActor *actor, const ZoneNPC *npc, u32 unused) {
    if (npc->isRail == TRUE) {
        InitRailActor(actor, npc);
    }
}

static void InitActorZoneNPCGridPositon(FieldActor *actor, const ZoneNPC *npc) {
    s32 x;
    s32 z;
    s32 gridY;
    fx32 y;
    VecFx32 pos;
    const ZoneNPCGridPosition *gridPos = &npc->pos.grid;

    if (IsActorLastPositionInSaveData(actor) == FALSE) {
        x = gridPos->x;
        z = gridPos->z;
        y = gridPos->y;
    } else {
        LoadSaveDataStrengthRockPosition(actor, &pos);
        x = (pos.x >> 4) / FX32_ONE;
        z = (pos.z >> 4) / FX32_ONE;
        y = pos.y;
    }
    pos.x = (x << 16) + 0x8000;
    SetActorDefaultGPosX(actor, gridPos->x);
    SetActorInitialGPosX(actor, x);
    SetActorGPosX(actor, x);
    gridY = (gridPos->y >> 4) / FX32_ONE;
    SetActorDefaultGPosY(actor, gridY);
    pos.y = y;
    gridY = (y >> 4) / FX32_ONE;
    SetActorInitialGPosY(actor, gridY);
    SetActorGPosY(actor, gridY);
    pos.z = (z << 16) + 0x8000;
    SetActorDefaultGPosZ(actor, gridPos->z);
    SetActorInitialGPosZ(actor, z);
    SetActorGPosZ(actor, z);
    SetActorWPosValue(actor, &pos);
}

static void InitNewActor(FieldActor *actor, MMSys *system, s32 zoneId) {
    actor->actorSystem = system;
    SetActorZoneID(actor, zoneId);
    SetActorFlag(actor, 1);
    SetActorMovementFlag(actor, 0x1800);
    if (GetActorSCRIDIsMinusOne(actor) == TRUE) {
        SetActorIsNonInteractibleFlag(actor, TRUE);
    }
    func_ov012_02166f2c(actor);
}

static void SetAllActorDirToDefault(FieldActor *actor) {
    SetActorFaceDir(actor, GetDefaultActorDir(actor));
    SetActorMotionDir(actor, GetDefaultActorDir(actor));
}

static void UpdateActorMoveCodeExecutor(FieldActor *actor) {
    actor->moveCodeVTable = GetMoveCodeExecutorFunction(GetActorMoveCode(actor));
}

static void func_ov012_02166c88(MMSys *system, FieldActor *actor) {
    UpdateActorMoveCodeExecutor(actor);
    func_ov012_02167b34(actor, system);
    SetActorMovementFlag(actor, 0x1820);
}

static void func_ov012_02166cac(MMSys *system, FieldActor *actor) {
    if (!CheckActorFlag(actor, 0x2000)) {
        func_ov012_0215dabc(actor);
    } else {
        func_ov036_02194924(actor);
    }
}

static void func_ov012_02166cd0(FieldActor *actor) {
    if (!CheckActorFlag(actor, 0x2000)) {
        func_ov012_0215dad4(actor);
    } else {
        func_ov036_02194938(actor);
    }
}

void FldActSys_SuspendAllActors(MMSys *system) {
    u32 index = 0;
    FieldActor *actor;

    if (func_ov012_021673b4(system)) {
        while (NextActor(system, &actor, &index) == TRUE) {
            RemoveActorTCB(actor);
            FldAct_InvokeSuspendCallback(actor);
            SetActorMovementFlag(actor, 5);
        }
    }
}

void func_ov012_02166d48(MMSys *system) {
    u16 moveCode;
    u32 index = 0;
    FieldActor *actor;

    while (NextActor(system, &actor, &index) == TRUE) {
        moveCode = GetActorMoveCode(actor);
        func_ov012_02166c88(system, actor);
        if (!CheckActorFlag(actor, 2)) {
            func_ov012_02166cac(system, actor);
        }
        if (moveCode != 0x55 && CheckActorMovementFlag(actor, 1)) {
            func_ov012_0216719c(actor);
            ClearActorMovementFlag(actor, 1);
        }
        if (!func_ov012_0216750c(actor)) {
            func_ov012_02167ce0(actor);
        }
        if (CheckActorMovementFlag(actor, 4)) {
            FldAct_InvokeWakeUpCallback(actor);
            ClearActorMovementFlag(actor, 4);
        }
        func_ov012_02167d40(actor);
        if (moveCode == 0x55 && CheckActorMovementFlag(actor, 1)) {
            func_ov012_0216719c(actor);
            ClearActorMovementFlag(actor, 1);
        }
    }
}

static void func_ov012_02166e04(FieldActor *actor) {
    func_ov012_02166cd0(actor);
    if (func_ov012_0216749c(actor) == TRUE) {
        func_ov012_02166e2c(actor);
    }
}

static void func_ov012_02166e20(TCB *tcb, void *data) {
    func_ov012_02166e04(data);
}

static void func_ov012_02166e2c(FieldActor *actor) {
    if (func_ov012_021673b4(GetActorMModelSystem(actor)) == TRUE) {
        func_ov036_0218ed44(actor);
    }
}

BOOL IsAllActorAcmdFinished(FieldActor *actor) {
    if (!CheckActorFlag(actor, 1)) {
        return FALSE;
    }
    if (CheckActorMovementFlag(actor, 0x10)) {
        return FALSE;
    }
    if (CheckActorMovementFlag(actor, 0x200) && !CheckActorMovementFlag(actor, 0x400)) {
        return FALSE;
    }
    return TRUE;
}

void FldAct_SetAcmd(FieldActor *actor, u16 acmd) {
    SetNextActorAcmd(actor, acmd);
    func_ov012_02167220(actor, 0);
    SetActorMovementFlag(actor, 0x200);
    ClearActorMovementFlag(actor, 0x400);
}

void func_ov012_02166eb0(FieldActor *actor, u16 acmd) {
    SetNextActorAcmd(actor, acmd);
    func_ov012_02167220(actor, 0);
    ClearActorMovementFlag(actor, 0x400);
}

BOOL func_ov012_02166ecc(FieldActor *actor) {
    if (!CheckActorMovementFlag(actor, 0x200)) {
        return TRUE;
    }
    if (CheckActorMovementFlag(actor, 0x400)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_02166ef8(FieldActor *actor) {
    if (!CheckActorMovementFlag(actor, 0x200)) {
        return TRUE;
    }
    if (!CheckActorMovementFlag(actor, 0x400)) {
        return FALSE;
    }
    ClearActorMovementFlag(actor, 0x600);
    return TRUE;
}

void func_ov012_02166f2c(FieldActor *actor) {
    ClearActorMovementFlag(actor, 0x600);
    SetNextActorAcmd(actor, 0xff);
    func_ov012_02167220(actor, 0);
}

u32 CheckMMSysFlag(MMSys *system, u32 flag) {
    return system->flags & flag;
}

static void FldActSys_SetFlag(MMSys *system, u32 flag) {
    system->flags |= flag;
}

static void ClearMMSysFlag(MMSys *system, u32 flag) {
    system->flags &= ~flag;
}

u16 GetActorLimit(const MMSys *system) {
    return system->actorCapacity;
}

u16 func_ov012_02166f6c(MMSys *system) {
    return system->unkC;
}

HeapID FldActSys_GetFieldHeapID(MMSys *system) {
    return system->fieldHeapId;
}

static void IncrementMMSysActorCount(MMSys *system) {
    system->actorCount++;
}

static void FldActSys_DecrementActorCount(MMSys *system) {
    system->actorCount--;
}

TCBManager *GetMMSysTCBMgr(MMSys *system) {
    return system->tcbMgr;
}

ArcTool *GetMMSysDataArcHandle(MMSys *system) {
    return system->dataArc;
}

void FldActSys_BindFieldBlAct(MMSys *system, void *fieldBlAct) {
    system->fieldBlAct = fieldBlAct;
}

void *FldActSys_GetFieldBlAct(MMSys *system) {
    return system->fieldBlAct;
}

void FldActSys_BindActorG3DSystem(MMSys *system, void *actorG3DSystem) {
    system->actorG3DSystem = actorG3DSystem;
}

void *FldActSys_GetActorG3DSystem(MMSys *system) {
    return system->actorG3DSystem;
}

FieldG3DMapper *GetMMSysG3DMapper(MMSys *system) {
    return system->g3dMapper;
}

NoGridMapper *FldActSys_GetNoGridMapper(MMSys *system) {
    return system->noGridMapper;
}

Field *GetMMSysField(MMSys *mmSys) {
    return mmSys->field;
}

u16 func_ov012_02166fb0(MMSys *system) {
    if (system->cameraAngle != NULL) {
        return *system->cameraAngle;
    }
    return 0;
}

u32 func_ov012_02166fc0(MMSys *system) {
    return system->unk10;
}

void func_ov012_02166fc4(MMSys *system, u32 value) {
    system->unk10 = value;
}

void SetActorFlag(FieldActor *actor, u32 flag) {
    actor->flags |= flag;
}

void ClearActorFlag(FieldActor *actor, u32 flag) {
    actor->flags &= ~flag;
}

BOOL CheckActorFlag(FieldActor *actor, u32 flag) {
    return actor->flags & flag;
}

u32 func_ov012_02166fe4(FieldActor *actor) {
    return actor->movementFlags;
}

void SetActorMovementFlag(FieldActor *actor, u32 flag) {
    actor->movementFlags |= flag;
}

void ClearActorMovementFlag(FieldActor *actor, u32 flag) {
    actor->movementFlags &= ~flag;
}

BOOL CheckActorMovementFlag(FieldActor *actor, u32 flag) {
    return actor->movementFlags & flag;
}

static void SetMMDLActorUID(FieldActor *actor, u16 uid) {
    actor->uid = uid;
}

u16 GetActorUID(FieldActor *actor) {
    return actor->uid;
}

static void SetActorZoneID(FieldActor *actor, u16 zoneId) {
    actor->zoneId = zoneId;
}

u16 GetActorZoneID(FieldActor *actor) {
    return actor->zoneId;
}

static void FldAct_SetObjCode(FieldActor *actor, u16 objCode) {
    actor->modelId = objCode;
}

u16 FldAct_GetObjCode(FieldActor *actor) {
    return actor->modelId;
}

static void SetActorMoveCode(FieldActor *actor, u16 moveCode) {
    actor->moveCode = moveCode;
}

u16 GetActorMoveCode(FieldActor *actor) {
    return actor->moveCode;
}

void SetActorEvType(FieldActor *actor, u16 evType) {
    actor->evType = evType;
}

u16 GetActorEvType(FieldActor *actor) {
    return actor->evType;
}

static void SetActorSpawnFlag(FieldActor *actor, u16 flag) {
    actor->spawnFlag = flag;
}

u16 GetActorSpawnFlag(FieldActor *actor) {
    return actor->spawnFlag;
}

void SetActorSCRID(FieldActor *actor, u16 scriptId) {
    actor->scrId = scriptId;
}

u16 FldAct_GetSCRID(FieldActor *actor) {
    return actor->scrId;
}

static BOOL GetActorSCRIDIsMinusOne(FieldActor *actor) {
    if (FldAct_GetSCRID(actor) == 0xffff) {
        return TRUE;
    }
    return FALSE;
}

u32 GetDefaultActorDir(FieldActor *actor) {
    return actor->defaultDir;
}

void SetActorFaceDir(FieldActor *actor, u16 dir) {
    actor->lastFaceDir = actor->faceDir;
    actor->faceDir = dir;
}

void CheckSetActorFaceDir(FieldActor *actor, u16 dir) {
    if (!CheckActorFlag(actor, 8)) {
        actor->lastFaceDir = actor->faceDir;
        actor->faceDir = dir;
    }
}

u16 GetActorFaceDir(FieldActor *actor) {
    return actor->faceDir;
}

u16 func_ov012_0216707c(FieldActor *actor) {
    return actor->lastFaceDir;
}

void SetActorMotionDir(FieldActor *actor, u16 dir) {
    actor->lastMotionDir = actor->motionDir;
    actor->motionDir = dir;
}

u16 GetActorMotionDir(FieldActor *actor) {
    return actor->motionDir;
}

void ChangeActorDirection(FieldActor *actor, u16 dir) {
    CheckSetActorFaceDir(actor, dir);
    SetActorMotionDir(actor, dir);
}

void SetActorUserParam(FieldActor *actor, u16 value, u32 index) {
    switch (index) {
    case 0:
        actor->param0 = value;
        break;
    case 1:
        actor->param1 = value;
        break;
    case 2:
        actor->param2 = value;
        break;
    }
}

u16 GetActorUserParam(FieldActor *actor, u32 index) {
    switch (index) {
    case 0:
        return actor->param0;
    case 1:
        return actor->param1;
    case 2:
        return actor->param2;
    }
    return 0;
}

void SetActorAreaW(FieldActor *actor, s16 w) {
    actor->areaW = w;
}

s16 GetActorWalkAreaW(FieldActor *actor) {
    return actor->areaW;
}

void SetActorAreaH(FieldActor *actor, s16 h) {
    actor->areaH = h;
}

s16 GetActorWalkAreaH(FieldActor *actor) {
    return actor->areaH;
}

void func_ov012_021670f4(FieldActor *actor, u32 value) {
    actor->unk2A = value;
}

u16 func_ov012_021670f8(FieldActor *actor) {
    return actor->unk2A;
}

MMSys *GetActorMModelSystem(FieldActor *actor) {
    return actor->actorSystem;
}

ActorPositionRail *ClearActorPositionBlock(FieldActor *actor, u32 size) {
    ActorPositionRail *block = GetNPCRailPosPtrAddr(actor);

    sys_memset(block, 0, size);
    return block;
}

ActorPositionRail *GetNPCRailPosPtrAddr(FieldActor *actor) {
    return &actor->defaultRailPos;
}

void *func_ov012_02167120(FieldActor *actor, u32 size) {
    void *block = func_ov012_02167138(actor);

    sys_memset(block, 0, size);
    return block;
}

void *func_ov012_02167138(FieldActor *actor) {
    return actor->unkA4;
}

void *func_ov012_0216713c(FieldActor *actor, u32 size) {
    void *block = func_ov012_02167154(actor);

    sys_memset(block, 0, size);
    return block;
}

void *func_ov012_02167154(FieldActor *actor) {
    return actor->unkB4;
}

void *FldAct_ResetBlActWork(FieldActor *actor, u32 size) {
    void *work = FldAct_GetBlActWorkPtr(actor);

    sys_memset(work, 0, size);
    return work;
}

void *FldAct_GetBlActWorkPtr(FieldActor *actor) {
    return actor->blActWork;
}

void func_ov012_02167174(FieldActor *actor) {
    if (actor->moveCodeVTable->unk4 != NULL) {
        actor->moveCodeVTable->unk4(actor);
    }
}

void func_ov012_02167188(FieldActor *actor) {
    if (actor->moveCodeVTable->unk8 != NULL) {
        actor->moveCodeVTable->unk8(actor);
    }
}

static void func_ov012_0216719c(FieldActor *actor) {
    if (actor->moveCodeVTable->unk10 != NULL) {
        actor->moveCodeVTable->unk10(actor);
    }
}

static void FldAct_InvokeOnCreateCallback(FieldActor *actor) {
    if (actor->sceneNodeVTable->onCreate != NULL) {
        actor->sceneNodeVTable->onCreate(actor);
    }
}

void FldAct_InvokeUpdateCallback(FieldActor *actor) {
    if (actor->sceneNodeVTable->update != NULL) {
        actor->sceneNodeVTable->update(actor);
    }
}

static void FldAct_InvokeSuspendCallback(FieldActor *actor) {
    if (actor->sceneNodeVTable->suspend != NULL) {
        actor->sceneNodeVTable->suspend(actor);
    }
}

static void FldAct_InvokeWakeUpCallback(FieldActor *actor) {
    if (actor->sceneNodeVTable->wakeUp != NULL) {
        actor->sceneNodeVTable->wakeUp(actor);
    }
}

u32 FldAct_GetBlActIdx(FieldActor *actor, u32 param) {
    if (actor->sceneNodeVTable->getBlActIdx != NULL) {
        return actor->sceneNodeVTable->getBlActIdx(actor, param);
    }
    return 0;
}

void SetNextActorAcmd(FieldActor *actor, u16 acmd) {
    actor->nextAcmd = acmd;
}

u16 FldAct_GetAcmd(FieldActor *actor) {
    return actor->nextAcmd;
}

void func_ov012_02167220(FieldActor *actor, u16 state) {
    actor->acmdState = state;
}

void FieldActor_NextAcmdState(FieldActor *actor) {
    actor->acmdState++;
}

u16 FldAct_GetAcmdState(FieldActor *actor) {
    return actor->acmdState;
}

void SetCachedTileUnderActor(FieldActor *actor, u32 tileType) {
    actor->currentTileUnder = tileType;
}

u32 GetCachedTileUnderActor(FieldActor *actor) {
    return actor->currentTileUnder;
}

void SetCachedOrigYTileUnderActor(FieldActor *actor, u32 tileType) {
    actor->currentTileUnderOrigY = tileType;
}

u32 GetCachedOrigYTileUnderActor(FieldActor *actor) {
    return actor->currentTileUnderOrigY;
}

s16 GetActorDefaultGPosX(FieldActor *actor) {
    return actor->defaultGPos.x;
}

static void SetActorDefaultGPosX(FieldActor *actor, s16 x) {
    actor->defaultGPos.x = x;
}

static void SetActorDefaultGPosY(FieldActor *actor, s16 y) {
    actor->defaultGPos.y = y;
}

s16 GetActorDefaultGPosZ(FieldActor *actor) {
    return actor->defaultGPos.z;
}

static void SetActorDefaultGPosZ(FieldActor *actor, s16 z) {
    actor->defaultGPos.z = z;
}

void GetActorInitGPos(FieldActor *actor, GridPos *pos) {
    pos->x = actor->initGPos.x;
    pos->y = actor->initGPos.y;
    pos->z = actor->initGPos.z;
}

s16 FldAct_GetInitGPosX(FieldActor *actor) {
    return actor->initGPos.x;
}

void SetActorInitialGPosX(FieldActor *actor, s16 x) {
    actor->initGPos.x = x;
}

s16 FldAct_GetInitGPosY(FieldActor *actor) {
    return actor->initGPos.y;
}

void SetActorInitialGPosY(FieldActor *actor, s16 y) {
    actor->initGPos.y = y;
}

s16 FldAct_GetInitGPosZ(FieldActor *actor) {
    return actor->initGPos.z;
}

void SetActorInitialGPosZ(FieldActor *actor, s16 z) {
    actor->initGPos.z = z;
}

void FldAct_GetGPos(FieldActor *actor, GridPos *pos) {
    pos->x = actor->gPos.x;
    pos->y = actor->gPos.y;
    pos->z = actor->gPos.z;
}

s16 GetGPosX(FieldActor *actor) {
    return actor->gPos.x;
}

void SetActorGPosX(FieldActor *actor, s16 x) {
    actor->gPos.x = x;
}

void adjustXPos(FieldActor *actor, s16 dx) {
    actor->gPos.x += dx;
}

s16 FldAct_GetGPosY(FieldActor *actor) {
    return actor->gPos.y;
}

void SetActorGPosY(FieldActor *actor, s16 y) {
    actor->gPos.y = y;
}

void adjustZPos(FieldActor *actor, s16 dy) {
    actor->gPos.y += dy;
}

s16 GetGPosZ(FieldActor *actor) {
    return actor->gPos.z;
}

void SetActorGPosZ(FieldActor *actor, s16 z) {
    actor->gPos.z = z;
}

void adjustYPos(FieldActor *actor, s16 dz) {
    actor->gPos.z += dz;
}

VecFx32 *GetMModelWPosPtr(FieldActor *actor) {
    return &actor->wPos;
}

void CopyActorWPos(FieldActor *actor, VecFx32 *dest) {
    *dest = actor->wPos;
}

void SetActorWPosValue(FieldActor *actor, const VecFx32 *pos) {
    actor->wPos = *pos;
}

fx32 GetActorPosY(FieldActor *actor) {
    return actor->wPos.y;
}

void func_ov012_0216731c(FieldActor *actor, VecFx32 *dest) {
    *dest = actor->wPosOffset;
}

void SetActorWPosOffset(FieldActor *actor, const VecFx32 *offset) {
    actor->wPosOffset = *offset;
}

void func_ov012_0216733c(FieldActor *actor, VecFx32 *dest) {
    *dest = actor->unk5C;
}

void func_ov012_0216734c(FieldActor *actor, const VecFx32 *value) {
    actor->unk5C = *value;
}

void func_ov012_0216735c(FieldActor *actor, VecFx32 *dest) {
    *dest = actor->unk68;
}

void func_ov012_0216736c(FieldActor *actor, const VecFx32 *offset) {
    actor->unk68 = *offset;
}

void *FldAct_GetFieldBlAct(FieldActor *actor) {
    return FldActSys_GetFieldBlAct(GetActorMModelSystem(actor));
}

u8 GetActorCollWidth(FieldActor *actor) {
    return actor->collisionWidth;
}

u8 GetActorCollHeight(FieldActor *actor) {
    return actor->collisionHeight;
}

void CopyActorPosOffset(FieldActor *actor, VecFx32 *offset) {
    offset->x = actor->modelPosOffsetX * FX32_ONE;
    offset->y = actor->modelPosOffsetY * FX32_ONE;
    offset->z = actor->modelPosOffsetZ * FX32_ONE;
}

static BOOL func_ov012_021673b4(MMSys *system) {
    BOOL result = TRUE;

    if (!CheckMMSysFlag(system, 1)) {
        result = FALSE;
    }
    return result;
}

void func_ov012_021673c8(MMSys *system, BOOL flag) {
    if (flag == TRUE) {
        FldActSys_SetFlag(system, 1);
    } else {
        ClearMMSysFlag(system, 1);
    }
}

void func_ov012_021673e0(MMSys *system, BOOL flag) {
    if (flag == FALSE) {
        FldActSys_SetFlag(system, 8);
    } else {
        ClearMMSysFlag(system, 8);
    }
}

BOOL func_ov012_021673f8(MMSys *system) {
    if (!CheckMMSysFlag(system, 8)) {
        return TRUE;
    }
    return FALSE;
}

void DisableAllActorsMovement(MMSys *mmSys) {
    u32 index = 0;
    FieldActor *actor;

    while (NextActor(mmSys, &actor, &index) == TRUE) {
        if (!isBit19Set(actor)) {
            DisableActorMovement(actor);
        }
    }
    FldActSys_SetFlag(mmSys, 0x20);
}

void EnableAllActorsMovement(MMSys *mmSys) {
    u32 index = 0;
    FieldActor *actor;

    while (NextActor(mmSys, &actor, &index) == TRUE) {
        EnableActorMovement(actor);
    }
    ClearMMSysFlag(mmSys, 0x20);
}

u32 func_ov012_0216748c(FieldActor *actor, u32 flag) {
    return CheckMMSysFlag(GetActorMModelSystem(actor), flag);
}

BOOL func_ov012_0216749c(FieldActor *actor) {
    BOOL result = TRUE;

    if (!CheckActorFlag(actor, 1)) {
        result = FALSE;
    }
    return result;
}

void func_ov012_021674b0(FieldActor *actor) {
    SetActorMovementFlag(actor, 0x10);
}

void func_ov012_021674bc(FieldActor *actor) {
    ClearActorMovementFlag(actor, 0x10);
}

BOOL IsActorFlag16(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x10)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov012_021674dc(FieldActor *actor) {
    SetActorMovementFlag(actor, 0x20);
}

void func_ov012_021674e8(FieldActor *actor) {
    ClearActorMovementFlag(actor, 0x40);
}

static void func_ov012_021674f4(FieldActor *actor) {
    SetActorMovementFlag(actor, 2);
}

static void func_ov012_02167500(FieldActor *actor) {
    ClearActorMovementFlag(actor, 2);
}

BOOL func_ov012_0216750c(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 2)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_02167520(FieldActor *actor) {
    if (CheckActorFlag(actor, 4)) {
        return TRUE;
    }
    return FALSE;
}

void SetActorHidden(FieldActor *actor, BOOL hidden) {
    if (hidden == TRUE) {
        SetActorFlag(actor, 4);
    } else {
        ClearActorFlag(actor, 4);
    }
}

BOOL func_ov012_0216754c(FieldActor *actor) {
    if (CheckActorFlag(actor, 0x4000)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov012_02167564(FieldActor *actor, u32 value) {
    if (value == TRUE) {
        SetActorFlag(actor, 0x4000);
    } else {
        ClearActorFlag(actor, 0x4000);
    }
}

void func_ov012_02167580(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        ClearActorFlag(actor, 0x80);
    } else {
        SetActorFlag(actor, 0x80);
    }
}

void SetActorFlag8000(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        ClearActorFlag(actor, 0x8000);
    } else {
        SetActorFlag(actor, 0x8000);
    }
}

BOOL func_ov012_021675b4(FieldActor *actor) {
    if (!CheckActorFlag(actor, 0x100)) {
        return TRUE;
    }
    return FALSE;
}

void SetActorFlag256(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        SetActorFlag(actor, 0x100);
    } else {
        ClearActorFlag(actor, 0x100);
    }
}

void DisableActorMovement(FieldActor *actor) {
    SetActorMovementFlag(actor, 8);
}

void EnableActorMovement(FieldActor *actor) {
    ClearActorMovementFlag(actor, 8);
}

BOOL func_ov012_02167600(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 8)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_02167614(FieldActor *actor) {
    if (!func_ov012_021673b4(GetActorMModelSystem(actor))) {
        return FALSE;
    }
    if (func_ov012_0216750c(actor)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov012_0216763c(FieldActor *actor, BOOL a1) {
    if (a1 == TRUE) {
        SetActorFlag(actor, 0x200);
    } else {
        ClearActorFlag(actor, 0x200);
    }
}

BOOL func_ov012_02167658(FieldActor *actor) {
    if (CheckActorFlag(actor, 0x200)) {
        return TRUE;
    }
    return FALSE;
}

void SetActorFlag32(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        SetActorFlag(actor, 0x20);
    } else {
        ClearActorFlag(actor, 0x20);
    }
}

static void SetActorIsNonInteractibleFlag(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        SetActorFlag(actor, 0x800);
    } else {
        ClearActorFlag(actor, 0x800);
    }
}

static BOOL checkBit11Set(FieldActor *actor) {
    if (CheckActorFlag(actor, 0x800)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov012_021676bc(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        SetActorMovementFlag(actor, 0x40000);
    } else {
        ClearActorMovementFlag(actor, 0x40000);
    }
}

BOOL func_ov012_021676d8(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x40000)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_021676f0(FieldActor *actor) {
    if (CheckActorFlag(actor, 0x1000)) {
        return TRUE;
    }
    return FALSE;
}

void FldAct_SetReflectingFlag(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        SetActorMovementFlag(actor, 0x20000);
    } else {
        ClearActorMovementFlag(actor, 0x20000);
    }
}

BOOL FldAct_CheckReflectingFlag(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x20000)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_0216773c(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x200)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov012_02167754(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        SetActorMovementFlag(actor, 0x80000);
    } else {
        ClearActorMovementFlag(actor, 0x80000);
    }
}

static BOOL isBit19Set(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x80000)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov012_02167788(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        SetActorFlag(actor, 0x400);
    } else {
        ClearActorFlag(actor, 0x400);
    }
}

BOOL func_ov012_021677a4(FieldActor *actor) {
    if (CheckActorFlag(actor, 0x400)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov012_021677bc(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        SetActorMovementFlag(actor, 0x100000);
    } else {
        ClearActorMovementFlag(actor, 0x100000);
    }
}

void func_ov012_021677d8(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        SetActorMovementFlag(actor, 0x200000);
    } else {
        ClearActorMovementFlag(actor, 0x200000);
    }
}

BOOL func_ov012_021677f4(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x200000)) {
        return TRUE;
    }
    return FALSE;
}

void FldAct_SetShadowGroup(FieldActor *actor, u32 group) {
    if (FldAct_IsForceOffShadow(actor)) {
        group = 0;
    }
    actor->shadowGroup = group;
}

u8 FldAct_GetShadowGroup(FieldActor *actor) {
    if (FldAct_IsForceOffShadow(actor)) {
        return 0;
    }
    return actor->shadowGroup;
}

void func_ov012_0216783c(FieldActor *actor, BOOL value) {
    if (value == TRUE) {
        SetActorFlag(actor, 0x10000);
    } else {
        ClearActorFlag(actor, 0x10000);
    }
}

static BOOL FldAct_IsForceOffShadow(FieldActor *actor) {
    if (CheckActorFlag(actor, 0x10000)) {
        return TRUE;
    }
    return FALSE;
}

BOOL NextActor(const MMSys *mmSys, FieldActor **actor, u32 *index) {
    u32 limit = GetActorLimit(mmSys);
    FieldActor *candidate;

    if (*index < limit) {
        candidate = &mmSys->actorHeap[*index];
        do {
            (*index)++;
            if (CheckActorFlag(candidate, 1)) {
                *actor = candidate;
                return TRUE;
            }
            candidate++;
        } while (*index < limit);
    }
    return FALSE;
}

static s32 GetAllActorsOverGPos(MMSys *system, s16 x, s16 z, BOOL checkInit, FieldActorList *list) {
    u32 index = 0;
    FieldActor *actor;

    sys_memset(list, 0, sizeof(FieldActorList));
    while (NextActor(system, &actor, &index) == TRUE) {
        if (CheckActorVolumeOverGPos(actor, x, z, checkInit) == TRUE) {
            list->actors[list->count++] = actor;
            if (list->count >= 16) {
                break;
            }
        }
    }
    return list->count;
}

FieldActor *GetFirstActorOnGPos(MMSys *system, s16 x, s16 z, BOOL checkInit) {
    FieldActorList list;

    if (GetAllActorsOverGPos(system, x, z, checkInit, &list)) {
        return list.actors[0];
    }
    return NULL;
}

FieldActor *FindActorByGPos(MMSys *system, s16 x, s16 z, fx32 y, fx32 maxHeightDiff, BOOL checkInit) {
    return FindActorByGPos_(system, x, z, y, maxHeightDiff, checkInit, NULL);
}

FieldActor *FindActorByGPos_(MMSys *system, s16 x, s16 z, fx32 y, fx32 maxHeightDiff, BOOL checkInit,
                             FieldActor *exclude) {
    u32 i;
    fx32 diff;
    FieldActorList list;
    u32 count = GetAllActorsOverGPos(system, x, z, checkInit, &list);

    if (count == 0) {
        return NULL;
    }
    for (i = 0; i < count; i++) {
        if (exclude != NULL && list.actors[i] == exclude) {
            continue;
        }
        diff = GetActorPosY(list.actors[i]) - y;
        if (diff < 0) {
            diff = FX_MUL(diff, -FX32_ONE);
        }
        if (diff < maxHeightDiff) {
            return list.actors[i];
        }
    }
    return NULL;
}

FieldActor *FindActorByMoveCode(MMSys *mmSys, u16 code) {
    u32 index = 0;
    FieldActor *actor;

    while (NextActor(mmSys, &actor, &index) == TRUE) {
        if (code == GetActorMoveCode(actor)) {
            return actor;
        }
    }
    return NULL;
}

FieldActor *FindFieldActor(MMSys *mmSys, u16 id) {
    u32 index = 0;
    FieldActor *actor;

    while (NextActor(mmSys, &actor, &index) == TRUE) {
        if (!checkBit11Set(actor) && id == GetActorUID(actor)) {
            return actor;
        }
    }
    return NULL;
}

FieldActor *FindPlayerFieldActor(MMSys *mmSys) {
    return FindFieldActor(mmSys, ACTOR_UID_PLAYER);
}

static FieldActor *GetPtrToFirstFreeActorSlot(MMSys *system) {
    s32 i = 0;
    s32 limit = GetActorLimit(system);
    FieldActor *actor = system->actorHeap;

    do {
        if (!CheckActorFlag(actor, 1)) {
            return actor;
        }
        i++;
        actor++;
    } while (i < limit);
    return NULL;
}

void DeleteAllActors(MMSys *system) {
    u32 i;
    FieldActor *actor;
    u32 limit = GetActorLimit(system);
    BOOL created = func_ov012_021673b4(system);

    for (i = 0; i < limit; i++) {
        actor = &system->actorHeap[i];
        if ((actor->flags & 1) && !(actor->flags & 0x20) && actor->uid != ACTOR_UID_PLAYER) {
            if (created && actor->sceneNodeVTable->onDelete != NULL) {
                actor->sceneNodeVTable->onDelete(actor);
            }
            if (actor->flags & 2) {
                if (actor->moveCodeVTable->unkC != NULL) {
                    actor->moveCodeVTable->unkC(actor);
                }
                RemoveActorTCB(actor);
            }
            actor->actorSystem->actorCount--;
            sys_memset32(0, actor, sizeof(FieldActor));
        }
    }
}

static void func_ov012_02167b34(FieldActor *actor, MMSys *system) {
    s32 priorityOffset = 0;
    u16 priority = func_ov012_02166f6c(system);
    u16 moveCode = GetActorMoveCode(actor);

    if (moveCode == 0x30 || moveCode == 0x32) {
        priorityOffset = 1;
    } else if (GetActorUID(actor) == ACTOR_UID_PLAYER) {
        priorityOffset--;
    }
    actor->tcb = GFL_TCBMgrAddTask(GetMMSysTCBMgr(system), func_ov012_02166e20, actor, priority + priorityOffset);
}

BOOL FldAct_CheckObjCodeShared(FieldActor *actor, u16 objCode) {
    u16 otherObjCode;
    u32 index = 0;
    FieldActor *other;
    MMSys *system = GetActorMModelSystem(actor);

    while (NextActor(system, &other, &index) == TRUE) {
        if (other != actor) {
            otherObjCode = FldAct_GetObjCode(other);
            if (otherObjCode != OBJCODE_NONE && otherObjCode == objCode) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void SetActorWPosAll(FieldActor *actor, const VecFx32 *pos, u16 dir) {
    SetActorGPosX(actor, POS_TO_GRID(pos->x));
    SetActorGPosY(actor, POS_TO_GRID(pos->y));
    SetActorGPosZ(actor, POS_TO_GRID(pos->z));
    SetActorWPosValue(actor, pos);
    SetActorInitialGPosToNowGPos(actor);
    SetActorFaceDir(actor, dir);
    func_ov012_02166f2c(actor);
    SetActorMovementFlag(actor, 0x20);
    ClearActorMovementFlag(actor, 0x50);
}

void SetActorGPos(FieldActor *actor, s16 x, s16 y, s16 z, u16 dir) {
    VecFx32 pos;

    pos.x = (x << 16) + 0x8000;
    pos.y = y << 16;
    pos.z = (z << 16) + 0x8000;
    SetActorWPosAll(actor, &pos, dir);
}

void FldAct_Transplant(FieldActor *actor, const ZoneNPC *npc) {
    MMSys *system = GetActorMModelSystem(actor);

    if (actor->moveCodeVTable->unkC != NULL) {
        actor->moveCodeVTable->unkC(actor);
    }
    LoadZoneNPCParamsToMMDL(actor, npc);
    SetupNPCPositionBasic(actor, npc);
    UpdateActorMoveCodeExecutor(actor);
    func_ov012_02166cac(system, actor);
    SetupActorOnRailIfRail(actor, npc, 0);
}

void ChangeActorMoveCodeSeq(FieldActor *actor, u16 moveCode) {
    MMSys *system = GetActorMModelSystem(actor);

    if (actor->moveCodeVTable->unkC != NULL) {
        actor->moveCodeVTable->unkC(actor);
    }
    SetActorMoveCode(actor, moveCode);
    UpdateActorMoveCodeExecutor(actor);
    func_ov012_02166cac(system, actor);
}

static void func_ov012_02167ce0(FieldActor *actor) {
    if (func_ov012_021673b4(GetActorMModelSystem(actor))) {
        if (CheckActorMovementFlag(actor, 0x1000)) {
            CheckRecalcActorY(actor);
        }
        func_ov012_021670f4(actor, 0);
        if (!func_ov012_0216750c(actor)) {
            FldAct_LoadVTable(actor);
            FldAct_InvokeOnCreateCallback(actor);
            func_ov012_021674f4(actor);
        }
    }
}

static void FldAct_LoadVTable(FieldActor *actor) {
    actor->sceneNodeVTable = FldAct_GetMainVTableAddr(GetActorMdlInfo(actor)->sceneNodeType);
}

static void func_ov012_02167d40(FieldActor *actor) {
    ClearActorMovementFlag(actor, 0x6c000);
}

void ChangeActorUID(FieldActor *actor, u16 uid) {
    SetMMDLActorUID(actor, uid);
    func_ov012_021674dc(actor);
    func_ov012_02167d40(actor);
}

static void FldAct_Reset(FieldActor *actor) {
    sys_memset(actor, 0, sizeof(FieldActor));
}

static void func_ov012_02167d74(FieldActor *actor) {
    SetActorMovementFlag(actor, 0x20);
    func_ov012_02167d40(actor);
}

void func_ov012_02167d88(FieldActor *actor, FieldActorIdentity *identity) {
    identity->uid = GetActorUID(actor);
    identity->objCode = FldAct_GetObjCode(actor);
    identity->zoneId = GetActorZoneID(actor);
}

BOOL func_ov012_02167da8(FieldActor *actor, const FieldActorIdentity *identity) {
    if (CheckActorFlag(actor, 1) && identity->uid == GetActorUID(actor) &&
        identity->objCode == FldAct_GetObjCode(actor) && identity->zoneId == GetActorZoneID(actor)) {
        return TRUE;
    }
    return FALSE;
}

void FldAct_UpdateBlInfoForNewObjCode(FieldActor *actor, u16 objCode) {
    const FieldActorConfig *config;
    MMSys *system = GetActorMModelSystem(actor);

    if (func_ov012_021673b4(system) == TRUE && func_ov012_0216750c(actor) == TRUE &&
        actor->sceneNodeVTable->onDelete != NULL) {
        actor->sceneNodeVTable->onDelete(actor);
    }
    FldAct_SetObjCode(actor, objCode);
    LoadInfoForNewActorModelID(system, objCode, actor);
    config = &actor->config;
    actor->collisionWidth = config->collWidth;
    actor->collisionHeight = config->collHeight;
    actor->modelPosOffsetX = config->wPosOffsetX;
    actor->modelPosOffsetY = config->wPosOffsetY;
    actor->modelPosOffsetZ = config->wPosOffsetZ;
    func_ov012_02167500(actor);
    func_ov012_02167d74(actor);
    func_ov012_02167ce0(actor);
}

static void FldActSys_InitConfigArc(MMSys *system, HeapID heapId) {
    system->actorConfigArc = GFL_ArcSysCreateFileHandle(0x2f, heapId);
}

static void FldActSys_FreeConfigArc(MMSys *system) {
    GFL_ArcToolFree(system->actorConfigArc);
}

void LoadMModelSystemInfoCache(MMSys *mmSys, s32 index) {
    GFL_ArcSysRead(mmSys->actorConfigCache, 0x39, index);
}

void GetMMSysMdlInfoCacheEntry(MMSys *system, u32 index, FieldActorConfig *config) {
    u16 *cache = system->actorConfigCache;
    u16 count = cache[0];

    if (index < count) {
        u16 codesSize = (count + 1) / 2 * 2;
        FieldActorConfig *configs = (FieldActorConfig *)&cache[codesSize + 2];

        *config = configs[index];
    }
}

u16 GetMMSysMdlInfoCacheEntryCount(MMSys *system) {
    return system->actorConfigCache[0];
}

void GetNPCMdlInfoForOBJCODE(const MMSys *actorSystem, u16 objCode, FieldActorConfig *config) {
    s32 i;
    u32 index;
    FieldActor *actor;
    u16 offset;
    u16 *cache = actorSystem->actorConfigCache;
    u16 count = cache[0];

    if (count < CONFIG_CACHE_MAX) {
        u16 codesSize = (count + 1) / 2 * 2;
        FieldActorConfig *configs = (FieldActorConfig *)&cache[codesSize + 2];

        for (i = 0; i < count; i++) {
            if (objCode == cache[1 + i]) {
                *config = configs[i];
                return;
            }
        }
    }
    index = 0;
    while (NextActor(actorSystem, &actor, &index)) {
        if (objCode == FldAct_GetObjCode(actor)) {
            *config = actor->config;
            return;
        }
    }
    offset = GetIndexOfObjID(objCode) * sizeof(FieldActorConfig) + 4;
    GFL_ArcToolReadRange(actorSystem->actorConfigArc, 0, offset, sizeof(FieldActorConfig), config);
}

static void LoadInfoForNewActorModelID(MMSys *system, u16 objCode, FieldActor *actor) {
    ClearActorFlag(actor, 1);
    GetNPCMdlInfoForOBJCODE(system, objCode, &actor->config);
    SetActorFlag(actor, 1);
}

const FieldActorConfig *GetActorMdlInfo(FieldActor *actor) {
    return &actor->config;
}

const FieldActorResGroup *GetNPCMdlInfoG2DRscGroup(const FieldActorConfig *config) {
    return &config->rscIndices;
}

const FieldActorResGroup *GetNPCMdlInfoG3DRscGroup(const FieldActorConfig *config) {
    return &config->rscIndices;
}

u16 ResolvePossibleWKOBJCODE(EventWork *eventWork, u16 objCode) {
    return GetWKOBJCODENPCOBJCODE(eventWork, objCode);
}

static u16 GetWKOBJCODENPCOBJCODE(EventWork *eventWork, u16 objCode) {
    if (objCode >= OBJCODE_WKOBJCODE_FIRST && objCode <= OBJCODE_WKOBJCODE_LAST) {
        if (eventWork == NULL) {
            objCode = 10;
        } else {
            objCode = *EventWork_GetWkPtr(eventWork, objCode - OBJCODE_WKOBJCODE_FIRST + WORK_OBJCODE_FIRST);
        }
    }
    return objCode;
}

static const FieldActorMoveCode *GetMoveCodeExecutorFunction(u16 moveCode) {
    return ACTOR_MOVE_CODE_FUNCTION_TABLES[moveCode];
}

static const FieldActorSceneNodeVTable *FldAct_GetMainVTableAddr(u8 sceneNodeType) {
    return FIELD_ACTOR_MAIN_VTABLES[sceneNodeType];
}

static BOOL IsNPCSCRID0xFFFF(const ZoneNPC *npc) {
    if (npc->scrId == 0xffff) {
        return TRUE;
    }
    return FALSE;
}

static BOOL CheckNPCSpawnFlag(EventWork *eventWork, u16 flag) {
    return EventWork_FlagGet(eventWork, flag);
}

u16 func_ov012_02168024(u16 type) {
    switch (type) {
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 11:
    case 12:
    case 13:
        return 1;
    }
    return type;
}

void func_ov012_02168054(FieldActor *actor) {
}

void func_ov012_02168058(FieldActor *actor) {
}

void func_ov012_0216805c(FieldActor *actor) {
}

void func_ov012_02168060(FieldActor *actor) {
}

u32 func_ov012_02168064(void) {
    return sizeof(StrengthRockSave);
}

void func_ov012_0216806c(StrengthRockSave *save) {
    sys_memset16(STRENGTH_ROCK_POS_NONE, save, sizeof(StrengthRockSave));
}

static BOOL IsStrengthRockLastPositionInSaveData(StrengthRockSave *save, s32 slot) {
    if (save->x[slot] != STRENGTH_ROCK_POS_NONE || save->y[slot] != STRENGTH_ROCK_POS_NONE ||
        save->z[slot] != STRENGTH_ROCK_POS_NONE) {
        return TRUE;
    }
    return FALSE;
}

static u16 GetStrengthRockSaveDataPositionSlotID(u16 zoneId, u16 uid) {
    u16 entryZoneId;
    u16 entryUid;
    s32 i = 0;

    do {
        entryZoneId = RELOADABLE_STRENGTH_ROCKS[i].zoneId;
        entryUid = RELOADABLE_STRENGTH_ROCKS[i].uid;
        if (entryZoneId == zoneId && entryUid == uid) {
            return RELOADABLE_STRENGTH_ROCKS[i].slot;
        }
        i++;
    } while (entryZoneId != 0xffff && entryUid != 0xffff);
    return STRENGTH_ROCK_SLOT_NONE;
}

static u16 GetActorSaveDataPositionSlotID(FieldActor *actor) {
    u16 slot = STRENGTH_ROCK_SLOT_NONE;

    if (IsNPCStrengthRock(FldAct_GetObjCode(actor)) == TRUE) {
        slot = GetStrengthRockSaveDataPositionSlotID(GetActorZoneID(actor), GetActorUID(actor));
    }
    return slot;
}

static BOOL IsActorLastPositionInSaveData(FieldActor *actor) {
    u16 slot = GetActorSaveDataPositionSlotID(actor);

    if (slot != STRENGTH_ROCK_SLOT_NONE) {
        return IsStrengthRockLastPositionInSaveData(GetActorMModelSystem(actor)->mmodelSave, slot);
    }
    return FALSE;
}

static BOOL LoadSaveDataStrengthRockPosition(FieldActor *actor, VecFx32 *pos) {
    StrengthRockSave *save;
    fx32 y;
    u16 slot = GetActorSaveDataPositionSlotID(actor);

    if (slot != STRENGTH_ROCK_SLOT_NONE) {
        save = GetActorMModelSystem(actor)->mmodelSave;
        if (IsStrengthRockLastPositionInSaveData(save, slot)) {
            pos->x = save->x[slot] << 16;
            pos->y = y = save->y[slot] << 16;
            pos->z = save->z[slot] << 16;
            pos->y = y -= (FldAct_GetObjCode(actor) == OBJCODE_STRENGTH_ROCK ? 14 : 26) * FX32_ONE;
            pos->y = y + ((save->y[slot] <= 0 ? 1 : 2) << 16);
            return TRUE;
        }
    }
    return FALSE;
}

void func_ov012_021681b0(FieldActor *actor) {
    StrengthRockSave *save;
    VecFx32 pos;
    u16 slot = GetActorSaveDataPositionSlotID(actor);

    if (slot != STRENGTH_ROCK_SLOT_NONE) {
        save = GetActorMModelSystem(actor)->mmodelSave;
        CopyActorWPos(actor, &pos);
        save->x[slot] = (pos.x >> 4) / FX32_ONE;
        save->y[slot] = (pos.y >> 4) / FX32_ONE;
        save->z[slot] = (pos.z >> 4) / FX32_ONE;
    }
}

BOOL func_ov012_0216820c(MMSys *actorSystem, const VecFx32 *position) {
    FieldActor *actor = FindActorByGPos(actorSystem, POS_TO_GRID(position->x), POS_TO_GRID(position->z), position->y,
                                        0x20000, FALSE);

    if (actor != NULL && IsActorLastPositionInSaveData(actor) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

void func_ov012_02168258(MMSys *system, u16 zoneId, u16 uid) {
    StrengthRockSave *save;
    u16 slot = GetStrengthRockSaveDataPositionSlotID(zoneId, uid);

    if (slot != STRENGTH_ROCK_SLOT_NONE) {
        save = system->mmodelSave;
        save->x[slot] = STRENGTH_ROCK_POS_NONE;
        save->y[slot] = STRENGTH_ROCK_POS_NONE;
        save->z[slot] = STRENGTH_ROCK_POS_NONE;
    }
}

BOOL IsNPCStrengthRock(u16 objCode) {
    if (objCode == OBJCODE_STRENGTH_ROCK || objCode == 0x2003 || objCode == 0x2004 || objCode == 0x2005) {
        return TRUE;
    }
    return FALSE;
}

u32 func_ov012_021682a0(MMSys *system) {
    s32 i;
    s32 count = 0;

    for (i = 0; i < STRENGTH_ROCK_SLOT_COUNT; i++) {
        if (IsStrengthRockLastPositionInSaveData(system->mmodelSave, i)) {
            count++;
        }
    }
    return count;
}

void func_ov012_021682c0(ZoneNPC *npc, u16 x, u16 z, s32 y) {
    ZoneNPCGridPosition *pos;

    npc->isRail = FALSE;
    pos = &npc->pos.grid;
    pos->x = x;
    pos->z = z;
    pos->y = y;
}

void func_ov012_021682d4(ZoneNPC *npc, u16 railIndex, u16 frontPos, u16 sidePos) {
    ZoneNPCRailPosition *pos;

    npc->isRail = TRUE;
    pos = &npc->pos.rail;
    pos->railIndex = railIndex;
    pos->frontPos = frontPos;
    pos->sidePos = sidePos;
}

void func_ov012_021682e8(FieldActor *actor, const FieldActorMoveCode *moveCode) {
    MMSys *system = GetActorMModelSystem(actor);

    if (actor->moveCodeVTable->unkC != NULL) {
        actor->moveCodeVTable->unkC(actor);
    }
    SetActorMoveCode(actor, moveCode->code);
    actor->moveCodeVTable = moveCode;
    func_ov012_02166cac(system, actor);
}
