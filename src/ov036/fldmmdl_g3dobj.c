#include "types.h"
#include "constants/directions.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_g3dobj.h"
#include "field/field_map.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_rail.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"

// The 3D models of the field actors (fldmmdl_g3dobj.c, after the file name that its allocation passes): which
// resource group holds the model of each object code, and the actors' per-frame tile checks and animation commands

// A resource group and the object code whose model it holds
typedef struct {
    u16 resGroup;
    u16 objCode;
} ActorG3DEntry;

typedef struct {
    MMSys *system;
    FieldG3DObjSystem *objSystem;
    u16 count;
    ActorG3DEntry *entries;
} ActorG3DSystem;

// What an animation command with a wait keeps in the actor's work
typedef struct {
    s16 unk0;
    s16 wait;
} ActorWaitWork;

#define RES_NONE 0xffff

static void func_ov036_02194718(ActorG3DSystem *sys, HeapID heapId);
static void func_ov036_02194758(ActorG3DSystem *sys);
static u16 FieldActorG3DSystem_RegistObj(ActorG3DSystem *sys, u16 objCode, u16 resGroup);
static u16 func_ov036_021947c4(ActorG3DSystem *sys, u16 objCode);
static u16 FldActSys_SetupG3DObj(MMSys *system, ActorG3DSystem *sys, u16 objCode, const FieldActorResGroup *group);
static void func_ov036_02194870(ActorG3DSystem *sys, u16 objCode);
static BOOL func_ov036_021949ec(FieldActor *actor);
static void func_ov036_02194a18(FieldActor *actor);
static void func_ov036_02194a3c(FieldActor *actor);
static void func_ov036_02194a5c(FieldActor *actor);
static void func_ov036_02194a90(FieldActor *actor);
static BOOL func_ov036_02194ac8(FieldActor *actor);
static void func_ov036_02194b48(FieldActor *actor);
static void func_ov036_02194ba8(FieldActor *actor);
static void func_ov036_02194c14(FieldActor *actor);
static void func_ov036_02194c68(FieldActor *actor);
static void func_ov036_02194cbc(FieldActor *actor);
static void func_ov036_02194d28(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void func_ov036_02194d4c(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void func_ov036_02194d78(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void func_ov036_02194dac(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void func_ov036_02194db4(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void func_ov036_02194dec(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void func_ov036_02194df0(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void func_ov036_02194e04(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void func_ov036_02194e70(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void func_ov036_02194eb0(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static FieldEffects *FldAct_GetFieldEffects(FieldActor *actor);
static void func_ov036_02194f04(FieldActor *actor, u32 dir);
static void func_ov036_02194f58(FieldActor *actor, u32 dir, u32 speed, u32 frames);
static void func_ov036_02195134(FieldActor *actor, u32 dir, u32 speed, u32 frames);
static void func_ov036_021952dc(FieldActor *actor, u32 dir, u32 speed, u32 timer, u16 frame, s16 height, u16 step,
                                u32 se);

// The direction an actor on the rails faces, by the sixteenth of a turn from the camera angle to its way along the rail
static const u8 FACE_DIRS_BY_ANGLE[16] = { 1, 3, 3, 3, 3, 3, 3, 0, 0, 2, 2, 2, 2, 2, 2, 1 };

void FieldActorG3DSystem_Create(MMSys *system, void *g3dObjSystem) {
    HeapID heapId = FldActSys_GetFieldHeapID(system);
    ActorG3DSystem *sys = GFL_HeapAllocate(heapId, sizeof(ActorG3DSystem), TRUE, "fldmmdl_g3dobj.c", 72);

    sys->system = system;
    sys->objSystem = g3dObjSystem;
    sys->count = GetActorLimit(system);
    func_ov036_02194718(sys, heapId);
    FldActSys_BindActorG3DSystem(system, sys);
}

void func_ov036_021946f4(MMSys *system) {
    ActorG3DSystem *sys = FldActSys_GetActorG3DSystem(system);

    func_ov036_02194758(sys);
    GFL_HeapFree(sys);
}

FieldG3DObjSystem *FldAct_GetG3DObjSystem(FieldActor *actor) {
    ActorG3DSystem *sys = FldActSys_GetActorG3DSystem(GetActorMModelSystem(actor));

    return sys->objSystem;
}

static void func_ov036_02194718(ActorG3DSystem *sys, HeapID heapId) {
    int i;

    sys->entries = GFL_HeapAllocate(heapId, sys->count * sizeof(ActorG3DEntry), TRUE, "fldmmdl_g3dobj.c", 127);
    for (i = 0; i < sys->count; i++) {
        sys->entries[i].objCode = RES_NONE;
    }
}

static void func_ov036_02194758(ActorG3DSystem *sys) {
    int i;
    ActorG3DEntry *entry = sys->entries;

    for (i = 0; i < sys->count; i++, entry++) {
        if (entry->objCode != RES_NONE) {
            FieldG3DObjSystem_FreeResGroup(sys->objSystem, entry->resGroup);
        }
    }
    GFL_HeapFree(sys->entries);
}

static u16 FieldActorG3DSystem_RegistObj(ActorG3DSystem *sys, u16 objCode, u16 resGroup) {
    int i;
    ActorG3DEntry *entry = sys->entries;

    for (i = 0; i < sys->count; i++, entry++) {
        if (entry->objCode == RES_NONE) {
            entry->objCode = objCode;
            entry->resGroup = resGroup;
            return i;
        }
    }
    return 0;
}

static u16 func_ov036_021947c4(ActorG3DSystem *sys, u16 objCode) {
    int i;
    ActorG3DEntry *entry = sys->entries;

    for (i = 0; i < sys->count; i++, entry++) {
        if (entry->objCode == objCode) {
            return entry->resGroup;
        }
    }
    return RES_NONE;
}

static u16 FldActSys_SetupG3DObj(MMSys *system, ActorG3DSystem *sys, u16 objCode, const FieldActorResGroup *group) {
    FieldG3DObjResRequest req;
    ArcTool *arc = GetMMSysDataArcHandle(system);
    int i;
    u16 resGroup;

    FieldG3DObjResRequest_Clear(&req);
    if (group->res1 != RES_NONE) {
        FieldG3DObjResRequest_SetModel(&req, arc, group->res1);
    }
    if (group->res2 != RES_NONE) {
        FieldG3DObjResRequest_SetModel(&req, arc, group->res2);
    }
    FieldG3DObjResRequest_SetAnmArc(&req, arc);
    for (i = 0; i < 3; i++) {
        if (group->animations[i] != RES_NONE) {
            FieldG3DObjResRequest_AddAnm(&req, group->animations[i]);
        }
    }
    resGroup = FieldG3DObjSystem_AddResGroup(sys->objSystem, &req, FALSE);
    FieldActorG3DSystem_RegistObj(sys, objCode, resGroup);
    return resGroup;
}

static void func_ov036_02194870(ActorG3DSystem *sys, u16 objCode) {
    int i;
    ActorG3DEntry *entry = sys->entries;

    for (i = 0; i < sys->count; i++, entry++) {
        if (entry->objCode == objCode) {
            FieldG3DObjSystem_FreeResGroup(sys->objSystem, entry->resGroup);
            entry->objCode = RES_NONE;
            return;
        }
    }
}

u16 func_ov036_021948a0(FieldActor *actor) {
    const FieldActorConfig *info = GetActorMdlInfo(actor);
    MMSys *system = GetActorMModelSystem(actor);
    ActorG3DSystem *sys = FldActSys_GetActorG3DSystem(system);
    u16 resGroup = func_ov036_021947c4(sys, info->uid);

    if (resGroup == RES_NONE) {
        resGroup = FldActSys_SetupG3DObj(system, sys, info->uid, GetNPCMdlInfoG3DRscGroup(info));
    }
    return FieldG3DObjSystem_AddObj(sys->objSystem, resGroup, 0, NULL);
}

void func_ov036_021948ec(FieldActor *actor, u16 objIdx) {
    ActorG3DSystem *sys = FldActSys_GetActorG3DSystem(GetActorMModelSystem(actor));
    u16 objCode = FldAct_GetObjCode(actor);

    FieldG3DObjSystem_FreeObj(sys->objSystem, objIdx);
    if (!FldAct_CheckObjCodeShared(actor, objCode)) {
        func_ov036_02194870(sys, objCode);
    }
}

void func_ov036_02194924(FieldActor *actor) {
    FldAct_CallMoveInitProc(actor);
    SetActorFlag(actor, 2);
}

void func_ov036_02194938(FieldActor *actor) {
    if (CheckActorMMSysFlag(actor, 2) == 0) {
        func_ov036_02194a18(actor);
        func_ov036_02194a3c(actor);
        if (CheckActorMovementFlag(actor, 0x200)) {
            func_ov036_0218eff4(actor);
        } else if (IsActorMovementDisabled(actor) == FALSE) {
            if (func_ov036_021949ec(actor) == TRUE) {
                FldAct_CallMoveProc(actor);
            }
        }
        func_ov036_02194a5c(actor);
        func_ov036_02194a90(actor);
    }
}

u32 func_ov036_02194990(FieldActor *actor, const RailPosition *pos) {
    return FieldNoGridMapper_GetTileAtPos(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)), pos);
}

u8 func_ov036_021949a4(FieldActor *actor, int angle) {
    VecFx16 vec;
    int diff;

    func_ov036_02195a78(actor, GetActorFaceDir(actor), &vec);
    vec.y = 0;
    VEC_Fx16Normalize(&vec, &vec);
    diff = FX_Atan2Idx(vec.x, vec.z) - angle;
    if (diff < 0) {
        diff += 0x10000;
    }
    return FACE_DIRS_BY_ANGLE[diff >> 12];
}

static BOOL func_ov036_021949ec(FieldActor *actor) {
    if (IsActorFlag16(actor) == TRUE) {
        return TRUE;
    }
    if (CheckActorMovementFlag(actor, 0x800) == FALSE) {
        return TRUE;
    }
    GetActorMoveCode(actor);
    return TRUE;
}

static void func_ov036_02194a18(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x800)) {
        if (func_ov036_02194ac8(actor) == TRUE) {
            FldAct_SetMoveStart(actor);
        }
    }
}

static void func_ov036_02194a3c(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x20)) {
        func_ov036_02194b48(actor);
    }
    ClearActorMovementFlag(actor, 0xa0);
}

static void func_ov036_02194a5c(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x80)) {
        func_ov036_02194c14(actor);
    } else if (CheckActorMovementFlag(actor, 0x20)) {
        func_ov036_02194ba8(actor);
    }
    ClearActorMovementFlag(actor, 0xa0);
}

static void func_ov036_02194a90(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x100)) {
        func_ov036_02194cbc(actor);
    } else if (CheckActorMovementFlag(actor, 0x40)) {
        func_ov036_02194c68(actor);
    }
    ClearActorMovementFlag(actor, 0x140);
}

static BOOL func_ov036_02194ac8(FieldActor *actor) {
    u32 origYTile = -1;
    u32 tile = -1;

    if (!FldAct_IsMapAttrDisabled(actor)) {
        NoGridMapper *mapper = FldActSys_GetNoGridMapper(GetActorMModelSystem(actor));
        RailPosition origYPos;
        RailPosition pos;

        GetActorRailPos(actor, &pos);
        func_ov036_02195604(actor, &origYPos);
        origYTile = FieldNoGridMapper_GetTileAtPos(mapper, &origYPos);
        tile = FieldNoGridMapper_GetTileAtPos(mapper, &pos);
    }
    SetCachedOrigYTileUnderActor(actor, origYTile);
    SetCachedTileUnderActor(actor, tile);
    if (tile == -1) {
        SetActorMovementFlag(actor, 0x800);
        return FALSE;
    }
    ClearActorMovementFlag(actor, 0x800);
    return TRUE;
}

static void func_ov036_02194b48(FieldActor *actor) {
    if (func_ov036_02194ac8(actor) && FldAct_IsDrawReady(actor) == TRUE) {
        u32 tile = GetCachedTileUnderActor(actor);
        u32 origYTile = GetCachedOrigYTileUnderActor(actor);
        const FieldActorConfig *info = GetActorMdlInfo(actor);

        func_ov036_02194d28(actor, tile, origYTile, info);
        func_ov036_02194dac(actor, tile, origYTile, info);
        func_ov036_02194e04(actor, tile, origYTile, info);
        func_ov036_02194eb0(actor, tile, origYTile, info);
    }
}

static void func_ov036_02194ba8(FieldActor *actor) {
    if (func_ov036_02194ac8(actor) && FldAct_IsDrawReady(actor) == TRUE) {
        u32 tile = GetCachedTileUnderActor(actor);
        u32 origYTile = GetCachedOrigYTileUnderActor(actor);
        const FieldActorConfig *info = GetActorMdlInfo(actor);

        func_ov036_02194d4c(actor, tile, origYTile, info);
        func_ov036_02194d78(actor, tile, origYTile, info);
        func_ov036_02194db4(actor, tile, origYTile, info);
        func_ov036_02194e04(actor, tile, origYTile, info);
        func_ov036_02194eb0(actor, tile, origYTile, info);
    }
}

static void func_ov036_02194c14(FieldActor *actor) {
    if (func_ov036_02194ac8(actor) && FldAct_IsDrawReady(actor) == TRUE) {
        u32 tile = GetCachedTileUnderActor(actor);
        u32 origYTile = GetCachedOrigYTileUnderActor(actor);
        const FieldActorConfig *info = GetActorMdlInfo(actor);

        func_ov036_02194db4(actor, tile, origYTile, info);
        func_ov036_02194e04(actor, tile, origYTile, info);
        func_ov036_02194eb0(actor, tile, origYTile, info);
    }
}

static void func_ov036_02194c68(FieldActor *actor) {
    if (func_ov036_02194ac8(actor) && FldAct_IsDrawReady(actor) == TRUE) {
        u32 tile = GetCachedTileUnderActor(actor);
        u32 origYTile = GetCachedOrigYTileUnderActor(actor);
        const FieldActorConfig *info = GetActorMdlInfo(actor);

        func_ov036_02194e70(actor, tile, origYTile, info);
        func_ov036_02194dec(actor, tile, origYTile, info);
        func_ov036_02194eb0(actor, tile, origYTile, info);
    }
}

static void func_ov036_02194cbc(FieldActor *actor) {
    if (func_ov036_02194ac8(actor) && FldAct_IsDrawReady(actor) == TRUE) {
        u32 tile = GetCachedTileUnderActor(actor);
        u32 origYTile = GetCachedOrigYTileUnderActor(actor);
        const FieldActorConfig *info = GetActorMdlInfo(actor);

        func_ov036_02194e70(actor, tile, origYTile, info);
        func_ov036_02194dec(actor, tile, origYTile, info);
        func_ov036_02194d4c(actor, tile, origYTile, info);
        func_ov036_02194df0(actor, tile, origYTile, info);
        func_ov036_02194eb0(actor, tile, origYTile, info);
    }
}

static void func_ov036_02194d28(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (GetTileFlags(tile) & 0x20) {
        func_ov036_021a40ac(FldAct_GetFieldEffects(actor), actor, FALSE, 0);
    }
}

static void func_ov036_02194d4c(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (tile != -1 && (GetTileFlags(tile) & 0x20)) {
        func_ov036_021a40ac(FldAct_GetFieldEffects(actor), actor, TRUE, 0);
    }
}

static void func_ov036_02194d78(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (origYTile != -1 && (GetTileFlags(origYTile) & 0x8)) {
        FieldEffects *effects = FldAct_GetFieldEffects(actor);
        u32 kind = 0;

        if (info->footprintType == 2) {
            kind = 1;
        }
        func_ov036_021b47c8(actor, effects, kind);
    }
}

static void func_ov036_02194dac(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    func_ov036_02194db4(actor, tile, origYTile, info);
}

static void func_ov036_02194db4(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (FldActSys_AreShadowsEnabled(GetActorMModelSystem(actor))) {
        if (!CheckActorMovementFlag(actor, 0x4000)) {
            func_ov036_021a3bf0(actor, FldAct_GetFieldEffects(actor));
            SetActorMovementFlag(actor, 0x4000);
        }
    }
}

static void func_ov036_02194dec(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
}

static void func_ov036_02194df0(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    func_ov036_021a3ec4(actor, FldAct_GetFieldEffects(actor));
}

static void func_ov036_02194e04(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (info->enableReflections != 0 && !FldAct_CheckReflectingFlag(actor)) {
        u32 reflectTile = -1;

        if (tile != -1) {
            if (GetTileFlags(tile) & 0x40) {
                reflectTile = tile;
            } else {
                tile = FldAct_GetTileTypeInDir(actor, 1);
                if (GetTileFlags(tile) & 0x40) {
                    reflectTile = tile;
                }
            }
            if (reflectTile != -1) {
                FieldEffects *effects = FldAct_GetFieldEffects(actor);
                MMSys *system = GetActorMModelSystem(actor);

                FldAct_SetReflectingFlag(actor, TRUE);
                func_ov036_021b49ac(system, actor, effects, 1);
            }
        }
    }
}

static void func_ov036_02194e70(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (info->enableReflections != 0 && FldAct_CheckReflectingFlag(actor)) {
        u32 reflectTile = FldAct_GetTileTypeInDir(actor, 1);

        if (reflectTile == -1) {
            FldAct_SetReflectingFlag(actor, FALSE);
            return;
        }
        if (!(GetTileFlags(reflectTile) & 0x40)) {
            FldAct_SetReflectingFlag(actor, FALSE);
        }
    }
}

static void func_ov036_02194eb0(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (GetTileFlags(tile) & 0x100) {
        FldAct_SetShadowGroup(actor, 1);
    } else {
        FldAct_SetShadowGroup(actor, 0);
    }
}

static FieldEffects *FldAct_GetFieldEffects(FieldActor *actor) {
    return Field_GetFieldEffects(GetMMSysField(GetActorMModelSystem(actor)));
}

BOOL func_ov036_02194ee8(FieldActor *actor) {
    SetActorMovementFlag(actor, 0x400);
    return FALSE;
}

BOOL func_ov036_02194ef8(FieldActor *actor) {
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static void func_ov036_02194f04(FieldActor *actor, u32 dir) {
    CheckSetActorFaceDir(actor, dir);
    FldAct_SetDrawStatus(actor, 0);
    SetActorInitialGPosToNowGPos(actor);
    FieldActor_NextAcmdState(actor);
}

BOOL func_ov036_02194f28(FieldActor *actor) {
    func_ov036_02194f04(actor, DIR_UP);
    return TRUE;
}

BOOL func_ov036_02194f34(FieldActor *actor) {
    func_ov036_02194f04(actor, DIR_DOWN);
    return TRUE;
}

BOOL func_ov036_02194f40(FieldActor *actor) {
    func_ov036_02194f04(actor, DIR_LEFT);
    return TRUE;
}

BOOL func_ov036_02194f4c(FieldActor *actor) {
    func_ov036_02194f04(actor, DIR_RIGHT);
    return TRUE;
}

static void func_ov036_02194f58(FieldActor *actor, u32 dir, u32 speed, u32 frames) {
    func_ov036_02195640(actor, dir, speed);
    func_ov036_021959bc(actor, dir);
    ChangeActorDirection(actor, dir);
    FldAct_SetDrawStatus(actor, frames);
    SetActorMovementFlag(actor, 0x20);
    FieldActor_NextAcmdState(actor);
}

BOOL func_ov036_02194f94(FieldActor *actor) {
    VecFx32 pos;
    RailUnit *unit = FldAct_GetRailUnit(actor);

    func_ov036_02195624(actor);
    RailUnit_GetCalcPos(unit, &pos);
    SetActorWPosValue(actor, &pos);
    if (func_ov036_021b09f4(unit)) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x440);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

BOOL func_ov036_02194fe8(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 0);
    return FALSE;
}

BOOL func_ov036_02194ff4(FieldActor *actor) {
    func_ov036_02194f58(actor, 0, 16, 2);
    return TRUE;
}
BOOL func_ov036_02195004(FieldActor *actor) {
    func_ov036_02194f58(actor, 1, 16, 2);
    return TRUE;
}
BOOL func_ov036_02195014(FieldActor *actor) {
    func_ov036_02194f58(actor, 2, 16, 2);
    return TRUE;
}
BOOL func_ov036_02195024(FieldActor *actor) {
    func_ov036_02194f58(actor, 3, 16, 2);
    return TRUE;
}
BOOL func_ov036_02195034(FieldActor *actor) {
    func_ov036_02194f58(actor, 0, 8, 3);
    return TRUE;
}
BOOL func_ov036_02195044(FieldActor *actor) {
    func_ov036_02194f58(actor, 1, 8, 3);
    return TRUE;
}
BOOL func_ov036_02195054(FieldActor *actor) {
    func_ov036_02194f58(actor, 2, 8, 3);
    return TRUE;
}
BOOL func_ov036_02195064(FieldActor *actor) {
    func_ov036_02194f58(actor, 3, 8, 3);
    return TRUE;
}
BOOL func_ov036_02195074(FieldActor *actor) {
    func_ov036_02194f58(actor, 0, 4, 4);
    return TRUE;
}
BOOL func_ov036_02195084(FieldActor *actor) {
    func_ov036_02194f58(actor, 1, 4, 4);
    return TRUE;
}
BOOL func_ov036_02195094(FieldActor *actor) {
    func_ov036_02194f58(actor, 2, 4, 4);
    return TRUE;
}
BOOL func_ov036_021950a4(FieldActor *actor) {
    func_ov036_02194f58(actor, 3, 4, 4);
    return TRUE;
}
BOOL func_ov036_021950b4(FieldActor *actor) {
    func_ov036_02194f58(actor, 0, 2, 5);
    return TRUE;
}
BOOL func_ov036_021950c4(FieldActor *actor) {
    func_ov036_02194f58(actor, 1, 2, 5);
    return TRUE;
}
BOOL func_ov036_021950d4(FieldActor *actor) {
    func_ov036_02194f58(actor, 2, 2, 5);
    return TRUE;
}
BOOL func_ov036_021950e4(FieldActor *actor) {
    func_ov036_02194f58(actor, 3, 2, 5);
    return TRUE;
}
BOOL func_ov036_021950f4(FieldActor *actor) {
    func_ov036_02194f58(actor, 0, 4, 10);
    return TRUE;
}
BOOL func_ov036_02195104(FieldActor *actor) {
    func_ov036_02194f58(actor, 1, 4, 10);
    return TRUE;
}
BOOL func_ov036_02195114(FieldActor *actor) {
    func_ov036_02194f58(actor, 2, 4, 10);
    return TRUE;
}
BOOL func_ov036_02195124(FieldActor *actor) {
    func_ov036_02194f58(actor, 3, 4, 10);
    return TRUE;
}
static void func_ov036_02195134(FieldActor *actor, u32 dir, u32 speed, u32 frames) {
    ActorWaitWork *work = func_ov012_0216713c(actor, sizeof(ActorWaitWork));

    work->unk0 = frames;
    work->wait = speed;
    CheckSetActorFaceDir(actor, dir);
    FldAct_SetDrawStatus(actor, frames);
    SetActorInitialGPosToNowGPos(actor);
    FieldActor_NextAcmdState(actor);
}
BOOL func_ov036_02195168(FieldActor *actor) {
    ActorWaitWork *work = func_ov012_02167154(actor);

    work->wait--;
    if (work->wait > 0) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x400);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}
BOOL func_ov036_0219519c(FieldActor *actor) {
    func_ov036_02195134(actor, 0, 32, 1);
    return TRUE;
}
BOOL func_ov036_021951ac(FieldActor *actor) {
    func_ov036_02195134(actor, 1, 32, 1);
    return TRUE;
}
BOOL func_ov036_021951bc(FieldActor *actor) {
    func_ov036_02195134(actor, 2, 32, 1);
    return TRUE;
}
BOOL func_ov036_021951cc(FieldActor *actor) {
    func_ov036_02195134(actor, 3, 32, 1);
    return TRUE;
}
BOOL func_ov036_021951dc(FieldActor *actor) {
    func_ov036_02195134(actor, 0, 16, 2);
    return TRUE;
}
BOOL func_ov036_021951ec(FieldActor *actor) {
    func_ov036_02195134(actor, 1, 16, 2);
    return TRUE;
}
BOOL func_ov036_021951fc(FieldActor *actor) {
    func_ov036_02195134(actor, 2, 16, 2);
    return TRUE;
}
BOOL func_ov036_0219520c(FieldActor *actor) {
    func_ov036_02195134(actor, 3, 16, 2);
    return TRUE;
}
BOOL func_ov036_0219521c(FieldActor *actor) {
    func_ov036_02195134(actor, 0, 8, 3);
    return TRUE;
}
BOOL func_ov036_0219522c(FieldActor *actor) {
    func_ov036_02195134(actor, 1, 8, 3);
    return TRUE;
}
BOOL func_ov036_0219523c(FieldActor *actor) {
    func_ov036_02195134(actor, 2, 8, 3);
    return TRUE;
}
BOOL func_ov036_0219524c(FieldActor *actor) {
    func_ov036_02195134(actor, 3, 8, 3);
    return TRUE;
}
BOOL func_ov036_0219525c(FieldActor *actor) {
    func_ov036_02195134(actor, 0, 2, 5);
    return TRUE;
}
BOOL func_ov036_0219526c(FieldActor *actor) {
    func_ov036_02195134(actor, 1, 2, 5);
    return TRUE;
}
BOOL func_ov036_0219527c(FieldActor *actor) {
    func_ov036_02195134(actor, 2, 2, 5);
    return TRUE;
}
BOOL func_ov036_0219528c(FieldActor *actor) {
    func_ov036_02195134(actor, 3, 2, 5);
    return TRUE;
}
BOOL func_ov036_0219529c(FieldActor *actor) {
    func_ov036_02195134(actor, 0, 4, 4);
    return TRUE;
}
BOOL func_ov036_021952ac(FieldActor *actor) {
    func_ov036_02195134(actor, 1, 4, 4);
    return TRUE;
}
BOOL func_ov036_021952bc(FieldActor *actor) {
    func_ov036_02195134(actor, 2, 4, 4);
    return TRUE;
}
BOOL func_ov036_021952cc(FieldActor *actor) {
    func_ov036_02195134(actor, 3, 4, 4);
    return TRUE;
}
static void func_ov036_021952dc(FieldActor *actor, u32 dir, u32 speed, u32 timer, u16 frame, s16 height, u16 step,
                                u32 se) {
    ActorRailJumpWork *work = func_ov012_0216713c(actor, sizeof(ActorRailJumpWork));

    work->dir = dir;
    work->speed = speed;
    work->timer = timer;
    work->unkE = frame;
    work->height = height;
    work->step = step;
    if (speed == 0) {
        SetActorInitialGPosToNowGPos(actor);
    } else {
        func_ov036_02195640(actor, dir, 8);
        func_ov036_021959bc(actor, dir);
    }
    SetActorMovementFlag(actor, 0xa0);
    ChangeActorDirection(actor, dir);
    FldAct_SetDrawStatus(actor, frame);
    FieldActor_NextAcmdState(actor);
    if (se != 0) {
        GFL_SndSEPlay(se);
    }
}

void func_ov036_02195354(FieldActor *actor, u16 dir, u32 speed, u32 timer, u16 frame, s16 height, u16 step) {
    func_ov036_021952dc(actor, dir, speed, timer, frame, height, step, 0x55e);
}
