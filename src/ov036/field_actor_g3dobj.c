// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/directions.h"
#include "constants/sound.h"
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

// The 3D models of the field actors (the ROM's fldmmdl_g3dobj.c, by the file name that its allocation passes): which
// resource group holds the model of each object code, and the actors' per-frame tile checks and animation commands

// A resource group and the object code whose model it holds
typedef struct {
    u16 resGroup;
    u16 objCode;
} ActorG3DEntry;

struct ActorG3DSystem {
    MMSys *system;
    FieldG3DObjSystem *objSystem;
    u16 count;
    ActorG3DEntry *entries;
};

// What an animation command with a wait keeps in the actor's work
typedef struct {
    s16 unk0;
    s16 wait;
} ActorWaitWork;

#define RES_NONE 0xffff

static void FieldActorG3DSystem_InitEntries(ActorG3DSystem *sys, HeapID heapId);
static void FieldActorG3DSystem_FreeEntries(ActorG3DSystem *sys);
static u16 FieldActorG3DSystem_RegistObj(ActorG3DSystem *sys, u16 objCode, u16 resGroup);
static u16 FieldActorG3DSystem_FindResGroup(ActorG3DSystem *sys, u16 objCode);
static u16 FldActSys_SetupG3DObj(MMSys *system, ActorG3DSystem *sys, u16 objCode, const FieldActorResGroup *group);
static void FieldActorG3DSystem_UnloadObjCode(ActorG3DSystem *sys, u16 objCode);
static BOOL RailActor_CheckMoveStart(FieldActor *actor);
static void RailActor_UpdateAttrBeforeMove(FieldActor *actor);
static void RailActor_ProcMoveStartFirst(FieldActor *actor);
static void RailActor_ProcMoveStartSecond(FieldActor *actor);
static void RailActor_ProcMoveEnd(FieldActor *actor);
static BOOL RailActor_CacheTerrain(FieldActor *actor);
static void RailActor_MapAttrProcMoveStartFirst(FieldActor *actor);
static void RailActor_MapAttrProcMoveStartSecond(FieldActor *actor);
static void RailActor_MapAttrProcMoveStartJumpSecond(FieldActor *actor);
static void RailActor_MapAttrProcMoveEnd(FieldActor *actor);
static void RailActor_MapAttrProcMoveEndJump(FieldActor *actor);
static void RailActor_MapAttrGrassProc0(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void RailActor_MapAttrGrassProc12(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void RailActor_MapAttrFootMarkProc1(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void RailActor_MapAttrShadowProc(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void RailActor_EnsureShadow(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void func_ov036_02194dec(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void RailActor_MapAttrLandingDustProc(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void RailActor_StartReflection(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static void RailActor_ClearReflectionIfOffTile(FieldActor *actor, u32 tile, u32 origYTile,
                                               const FieldActorConfig *info);
static void RailActor_UpdateShadowGroup(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info);
static FieldEffects *FldAct_GetFieldEffects(FieldActor *actor);
static void RailActorAcmd_InitTurn(FieldActor *actor, u32 dir);
static void RailActorAcmd_InitMove(FieldActor *actor, u32 dir, u32 speed, u32 frames);
static void RailActorAcmd_InitWait(FieldActor *actor, u32 dir, u32 speed, u32 frames);
static void RailActorAcmd_InitJump(FieldActor *actor, u32 dir, u32 speed, u32 timer, u16 frame, s16 height, u16 step,
                                   u32 se);

// The direction an actor on the rails faces, by the sixteenth of a turn from the camera angle to its way along the rail
static const u8 FACE_DIRS_BY_ANGLE[16] = { 1, 3, 3, 3, 3, 3, 3, 0, 0, 2, 2, 2, 2, 2, 2, 1 };

void FieldActorG3DSystem_Create(MMSys *system, FieldG3DObjSystem *g3dObjSystem) {
    HeapID heapId = FldActSys_GetFieldHeapID(system);
    ActorG3DSystem *sys = GFL_HeapAllocate(heapId, sizeof(ActorG3DSystem), TRUE, "fldmmdl_g3dobj.c", 72);

    sys->system = system;
    sys->objSystem = g3dObjSystem;
    sys->count = GetActorLimit(system);
    FieldActorG3DSystem_InitEntries(sys, heapId);
    FldActSys_BindActorG3DSystem(system, sys);
}

void FieldActorG3DSystem_Delete(MMSys *system) {
    ActorG3DSystem *sys = FldActSys_GetActorG3DSystem(system);

    FieldActorG3DSystem_FreeEntries(sys);
    GFL_HeapFree(sys);
}

FieldG3DObjSystem *FldAct_GetG3DObjSystem(FieldActor *actor) {
    ActorG3DSystem *sys = FldActSys_GetActorG3DSystem(GetActorMModelSystem(actor));

    return sys->objSystem;
}

static void FieldActorG3DSystem_InitEntries(ActorG3DSystem *sys, HeapID heapId) {
    int i;

    sys->entries = GFL_HeapAllocate(heapId, sys->count * sizeof(ActorG3DEntry), TRUE, "fldmmdl_g3dobj.c", 127);
    for (i = 0; i < sys->count; i++) {
        sys->entries[i].objCode = RES_NONE;
    }
}

static void FieldActorG3DSystem_FreeEntries(ActorG3DSystem *sys) {
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

static u16 FieldActorG3DSystem_FindResGroup(ActorG3DSystem *sys, u16 objCode) {
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

static void FieldActorG3DSystem_UnloadObjCode(ActorG3DSystem *sys, u16 objCode) {
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

u16 FieldActorG3DSystem_AddObj(FieldActor *actor) {
    const FieldActorConfig *info = GetActorMdlInfo(actor);
    MMSys *system = GetActorMModelSystem(actor);
    ActorG3DSystem *sys = FldActSys_GetActorG3DSystem(system);
    u16 resGroup = FieldActorG3DSystem_FindResGroup(sys, info->uid);

    if (resGroup == RES_NONE) {
        resGroup = FldActSys_SetupG3DObj(system, sys, info->uid, GetNPCMdlInfoG3DRscGroup(info));
    }
    return FieldG3DObjSystem_AddObj(sys->objSystem, resGroup, 0, NULL);
}

void FieldActorG3DSystem_DelObj(FieldActor *actor, u16 objIdx) {
    ActorG3DSystem *sys = FldActSys_GetActorG3DSystem(GetActorMModelSystem(actor));
    u16 objCode = FldAct_GetObjCode(actor);

    FieldG3DObjSystem_FreeObj(sys->objSystem, objIdx);
    if (!FldAct_CheckObjCodeShared(actor, objCode)) {
        FieldActorG3DSystem_UnloadObjCode(sys, objCode);
    }
}

void RailActor_InitMove(FieldActor *actor) {
    FldAct_CallMoveInitProc(actor);
    SetActorFlag(actor, 2);
}

void RailActor_UpdateMove(FieldActor *actor) {
    if (CheckActorMMSysFlag(actor, 2) == 0) {
        RailActor_UpdateAttrBeforeMove(actor);
        RailActor_ProcMoveStartFirst(actor);
        if (CheckActorMovementFlag(actor, 0x200)) {
            FieldAcmdTCB_RunCmds(actor);
        } else if (IsActorMovementDisabled(actor) == FALSE) {
            if (RailActor_CheckMoveStart(actor) == TRUE) {
                FldAct_CallMoveProc(actor);
            }
        }
        RailActor_ProcMoveStartSecond(actor);
        RailActor_ProcMoveEnd(actor);
    }
}

u32 RailActor_GetLocationAttr(FieldActor *actor, const RailPosition *pos) {
    return FieldNoGridMapper_GetTileAtPos(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)), pos);
}

u8 RailActor_GetDirByAngle(FieldActor *actor, int angle) {
    VecFx16 vec;
    int diff;

    RailActor_GetDrawOffsetByDir(actor, GetActorFaceDir(actor), &vec);
    vec.y = 0;
    VEC_Fx16Normalize(&vec, &vec);
    diff = FX_Atan2Idx(vec.x, vec.z) - angle;
    if (diff < 0) {
        diff += 0x10000;
    }
    return FACE_DIRS_BY_ANGLE[diff >> 12];
}

static BOOL RailActor_CheckMoveStart(FieldActor *actor) {
    if (IsActorFlag16(actor) == TRUE) {
        return TRUE;
    }
    if (CheckActorMovementFlag(actor, 0x800) == FALSE) {
        return TRUE;
    }
    GetActorMoveCode(actor);
    return TRUE;
}

static void RailActor_UpdateAttrBeforeMove(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x800)) {
        if (RailActor_CacheTerrain(actor) == TRUE) {
            FldAct_SetMoveStart(actor);
        }
    }
}

static void RailActor_ProcMoveStartFirst(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x20)) {
        RailActor_MapAttrProcMoveStartFirst(actor);
    }
    ClearActorMovementFlag(actor, 0xa0);
}

static void RailActor_ProcMoveStartSecond(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x80)) {
        RailActor_MapAttrProcMoveStartJumpSecond(actor);
    } else if (CheckActorMovementFlag(actor, 0x20)) {
        RailActor_MapAttrProcMoveStartSecond(actor);
    }
    ClearActorMovementFlag(actor, 0xa0);
}

static void RailActor_ProcMoveEnd(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x100)) {
        RailActor_MapAttrProcMoveEndJump(actor);
    } else if (CheckActorMovementFlag(actor, 0x40)) {
        RailActor_MapAttrProcMoveEnd(actor);
    }
    ClearActorMovementFlag(actor, 0x140);
}

static BOOL RailActor_CacheTerrain(FieldActor *actor) {
    u32 origYTile = -1;
    u32 tile = -1;

    if (!FldAct_IsMapAttrDisabled(actor)) {
        NoGridMapper *mapper = FldActSys_GetNoGridMapper(GetActorMModelSystem(actor));
        RailPosition origYPos;
        RailPosition pos;

        GetActorRailPos(actor, &pos);
        RailActor_GetNowLocation(actor, &origYPos);
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

static void RailActor_MapAttrProcMoveStartFirst(FieldActor *actor) {
    if (RailActor_CacheTerrain(actor) && FldAct_IsDrawReady(actor) == TRUE) {
        u32 tile = GetCachedTileUnderActor(actor);
        u32 origYTile = GetCachedOrigYTileUnderActor(actor);
        const FieldActorConfig *info = GetActorMdlInfo(actor);

        RailActor_MapAttrGrassProc0(actor, tile, origYTile, info);
        RailActor_MapAttrShadowProc(actor, tile, origYTile, info);
        RailActor_StartReflection(actor, tile, origYTile, info);
        RailActor_UpdateShadowGroup(actor, tile, origYTile, info);
    }
}

static void RailActor_MapAttrProcMoveStartSecond(FieldActor *actor) {
    if (RailActor_CacheTerrain(actor) && FldAct_IsDrawReady(actor) == TRUE) {
        u32 tile = GetCachedTileUnderActor(actor);
        u32 origYTile = GetCachedOrigYTileUnderActor(actor);
        const FieldActorConfig *info = GetActorMdlInfo(actor);

        RailActor_MapAttrGrassProc12(actor, tile, origYTile, info);
        RailActor_MapAttrFootMarkProc1(actor, tile, origYTile, info);
        RailActor_EnsureShadow(actor, tile, origYTile, info);
        RailActor_StartReflection(actor, tile, origYTile, info);
        RailActor_UpdateShadowGroup(actor, tile, origYTile, info);
    }
}

static void RailActor_MapAttrProcMoveStartJumpSecond(FieldActor *actor) {
    if (RailActor_CacheTerrain(actor) && FldAct_IsDrawReady(actor) == TRUE) {
        u32 tile = GetCachedTileUnderActor(actor);
        u32 origYTile = GetCachedOrigYTileUnderActor(actor);
        const FieldActorConfig *info = GetActorMdlInfo(actor);

        RailActor_EnsureShadow(actor, tile, origYTile, info);
        RailActor_StartReflection(actor, tile, origYTile, info);
        RailActor_UpdateShadowGroup(actor, tile, origYTile, info);
    }
}

static void RailActor_MapAttrProcMoveEnd(FieldActor *actor) {
    if (RailActor_CacheTerrain(actor) && FldAct_IsDrawReady(actor) == TRUE) {
        u32 tile = GetCachedTileUnderActor(actor);
        u32 origYTile = GetCachedOrigYTileUnderActor(actor);
        const FieldActorConfig *info = GetActorMdlInfo(actor);

        RailActor_ClearReflectionIfOffTile(actor, tile, origYTile, info);
        func_ov036_02194dec(actor, tile, origYTile, info);
        RailActor_UpdateShadowGroup(actor, tile, origYTile, info);
    }
}

static void RailActor_MapAttrProcMoveEndJump(FieldActor *actor) {
    if (RailActor_CacheTerrain(actor) && FldAct_IsDrawReady(actor) == TRUE) {
        u32 tile = GetCachedTileUnderActor(actor);
        u32 origYTile = GetCachedOrigYTileUnderActor(actor);
        const FieldActorConfig *info = GetActorMdlInfo(actor);

        RailActor_ClearReflectionIfOffTile(actor, tile, origYTile, info);
        func_ov036_02194dec(actor, tile, origYTile, info);
        RailActor_MapAttrGrassProc12(actor, tile, origYTile, info);
        RailActor_MapAttrLandingDustProc(actor, tile, origYTile, info);
        RailActor_UpdateShadowGroup(actor, tile, origYTile, info);
    }
}

static void RailActor_MapAttrGrassProc0(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (GetTileFlags(tile) & 0x20) {
        GrassTask_Start(FldAct_GetFieldEffects(actor), actor, FALSE, 0);
    }
}

static void RailActor_MapAttrGrassProc12(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (tile != -1 && (GetTileFlags(tile) & 0x20)) {
        GrassTask_Start(FldAct_GetFieldEffects(actor), actor, TRUE, 0);
    }
}

static void RailActor_MapAttrFootMarkProc1(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (origYTile != -1 && (GetTileFlags(origYTile) & 0x8)) {
        FieldEffects *effects = FldAct_GetFieldEffects(actor);
        u32 kind = 0;

        if (info->footprintType == 2) {
            kind = 1;
        }
        Footmark_SetActor(actor, effects, kind);
    }
}

static void RailActor_MapAttrShadowProc(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    RailActor_EnsureShadow(actor, tile, origYTile, info);
}

static void RailActor_EnsureShadow(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (FldActSys_AreShadowsEnabled(GetActorMModelSystem(actor))) {
        if (!CheckActorMovementFlag(actor, 0x4000)) {
            FldeffShadow_Start(actor, FldAct_GetFieldEffects(actor));
            SetActorMovementFlag(actor, 0x4000);
        }
    }
}

static void func_ov036_02194dec(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
}

static void RailActor_MapAttrLandingDustProc(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    FldeffDust_StartFacing(actor, FldAct_GetFieldEffects(actor));
}

static void RailActor_StartReflection(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
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
                ReflectTask_SetActor(system, actor, effects, 1);
            }
        }
    }
}

static void RailActor_ClearReflectionIfOffTile(FieldActor *actor, u32 tile, u32 origYTile,
                                               const FieldActorConfig *info) {
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

static void RailActor_UpdateShadowGroup(FieldActor *actor, u32 tile, u32 origYTile, const FieldActorConfig *info) {
    if (GetTileFlags(tile) & 0x100) {
        FldAct_SetShadowGroup(actor, 1);
    } else {
        FldAct_SetShadowGroup(actor, 0);
    }
}

static FieldEffects *FldAct_GetFieldEffects(FieldActor *actor) {
    return Field_GetFieldEffects(GetMMSysField(GetActorMModelSystem(actor)));
}

BOOL RailActor_AcSetAcmdEnd(FieldActor *actor) {
    SetActorMovementFlag(actor, 0x400);
    return FALSE;
}

BOOL RailActor_AcNextState(FieldActor *actor) {
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static void RailActorAcmd_InitTurn(FieldActor *actor, u32 dir) {
    CheckSetActorFaceDir(actor, dir);
    FldAct_SetDrawStatus(actor, 0);
    SetActorInitialGPosToNowGPos(actor);
    FieldActor_NextAcmdState(actor);
}

BOOL RailActor_AcFaceU(FieldActor *actor) {
    RailActorAcmd_InitTurn(actor, DIR_UP);
    return TRUE;
}

BOOL RailActor_AcFaceD(FieldActor *actor) {
    RailActorAcmd_InitTurn(actor, DIR_DOWN);
    return TRUE;
}

BOOL RailActor_AcFaceL(FieldActor *actor) {
    RailActorAcmd_InitTurn(actor, DIR_LEFT);
    return TRUE;
}

BOOL RailActor_AcFaceR(FieldActor *actor) {
    RailActorAcmd_InitTurn(actor, DIR_RIGHT);
    return TRUE;
}

static void RailActorAcmd_InitMove(FieldActor *actor, u32 dir, u32 speed, u32 frames) {
    RailActor_SetMoveRequest(actor, dir, speed);
    RailActor_SetGridPosByDir(actor, dir);
    ChangeActorDirection(actor, dir);
    FldAct_SetDrawStatus(actor, frames);
    SetActorMovementFlag(actor, 0x20);
    FieldActor_NextAcmdState(actor);
}

BOOL RailActor_AcWalk1(FieldActor *actor) {
    VecFx32 pos;
    RailUnit *unit = FldAct_GetRailUnit(actor);

    RailActor_UpdateRailWork(actor);
    RailUnit_GetCalcPos(unit, &pos);
    SetActorWPosValue(actor, &pos);
    if (RailUnit_IsMovePending(unit)) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x440);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

BOOL RailActor_AcWalkEnd(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 0);
    return FALSE;
}

BOOL RailActor_AcWalkU16F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 0, 16, 2);
    return TRUE;
}
BOOL RailActor_AcWalkD16F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 1, 16, 2);
    return TRUE;
}
BOOL RailActor_AcWalkL16F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 2, 16, 2);
    return TRUE;
}
BOOL RailActor_AcWalkR16F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 3, 16, 2);
    return TRUE;
}
BOOL RailActor_AcWalkU8F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 0, 8, 3);
    return TRUE;
}
BOOL RailActor_AcWalkD8F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 1, 8, 3);
    return TRUE;
}
BOOL RailActor_AcWalkL8F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 2, 8, 3);
    return TRUE;
}
BOOL RailActor_AcWalkR8F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 3, 8, 3);
    return TRUE;
}
BOOL RailActor_AcWalkU4F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 0, 4, 4);
    return TRUE;
}
BOOL RailActor_AcWalkD4F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 1, 4, 4);
    return TRUE;
}
BOOL RailActor_AcWalkL4F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 2, 4, 4);
    return TRUE;
}
BOOL RailActor_AcWalkR4F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 3, 4, 4);
    return TRUE;
}
BOOL RailActor_AcWalkU2F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 0, 2, 5);
    return TRUE;
}
BOOL RailActor_AcWalkD2F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 1, 2, 5);
    return TRUE;
}
BOOL RailActor_AcWalkL2F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 2, 2, 5);
    return TRUE;
}
BOOL RailActor_AcWalkR2F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 3, 2, 5);
    return TRUE;
}
BOOL RailActor_AcDashU4F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 0, 4, 10);
    return TRUE;
}
BOOL RailActor_AcDashD4F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 1, 4, 10);
    return TRUE;
}
BOOL RailActor_AcDashL4F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 2, 4, 10);
    return TRUE;
}
BOOL RailActor_AcDashR4F0(FieldActor *actor) {
    RailActorAcmd_InitMove(actor, 3, 4, 10);
    return TRUE;
}
static void RailActorAcmd_InitWait(FieldActor *actor, u32 dir, u32 speed, u32 frames) {
    ActorWaitWork *work = FldAct_InitMoveCmdWork(actor, sizeof(ActorWaitWork));

    work->unk0 = frames;
    work->wait = speed;
    CheckSetActorFaceDir(actor, dir);
    FldAct_SetDrawStatus(actor, frames);
    SetActorInitialGPosToNowGPos(actor);
    FieldActor_NextAcmdState(actor);
}
BOOL RailActor_AcStayWalk1(FieldActor *actor) {
    ActorWaitWork *work = FldAct_GetMoveCmdWork(actor);

    work->wait--;
    if (work->wait > 0) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x400);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}
BOOL RailActor_AcStayWalkU32F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 0, 32, 1);
    return TRUE;
}
BOOL RailActor_AcStayWalkD32F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 1, 32, 1);
    return TRUE;
}
BOOL RailActor_AcStayWalkL32F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 2, 32, 1);
    return TRUE;
}
BOOL RailActor_AcStayWalkR32F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 3, 32, 1);
    return TRUE;
}
BOOL RailActor_AcStayWalkU16F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 0, 16, 2);
    return TRUE;
}
BOOL RailActor_AcStayWalkD16F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 1, 16, 2);
    return TRUE;
}
BOOL RailActor_AcStayWalkL16F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 2, 16, 2);
    return TRUE;
}
BOOL RailActor_AcStayWalkR16F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 3, 16, 2);
    return TRUE;
}
BOOL RailActor_AcStayWalkU8F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 0, 8, 3);
    return TRUE;
}
BOOL RailActor_AcStayWalkD8F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 1, 8, 3);
    return TRUE;
}
BOOL RailActor_AcStayWalkL8F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 2, 8, 3);
    return TRUE;
}
BOOL RailActor_AcStayWalkR8F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 3, 8, 3);
    return TRUE;
}
BOOL RailActor_AcStayWalkU2F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 0, 2, 5);
    return TRUE;
}
BOOL RailActor_AcStayWalkD2F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 1, 2, 5);
    return TRUE;
}
BOOL RailActor_AcStayWalkL2F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 2, 2, 5);
    return TRUE;
}
BOOL RailActor_AcStayWalkR2F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 3, 2, 5);
    return TRUE;
}
BOOL RailActor_AcStayWalkU4F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 0, 4, 4);
    return TRUE;
}
BOOL RailActor_AcStayWalkD4F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 1, 4, 4);
    return TRUE;
}
BOOL RailActor_AcStayWalkL4F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 2, 4, 4);
    return TRUE;
}
BOOL RailActor_AcStayWalkR4F0(FieldActor *actor) {
    RailActorAcmd_InitWait(actor, 3, 4, 4);
    return TRUE;
}
static void RailActorAcmd_InitJump(FieldActor *actor, u32 dir, u32 speed, u32 timer, u16 frame, s16 height, u16 step,
                                   u32 se) {
    ActorRailJumpWork *work = FldAct_InitMoveCmdWork(actor, sizeof(ActorRailJumpWork));

    work->dir = dir;
    work->speed = speed;
    work->timer = timer;
    work->unkE = frame;
    work->height = height;
    work->step = step;
    if (speed == 0) {
        SetActorInitialGPosToNowGPos(actor);
    } else {
        RailActor_SetMoveRequest(actor, dir, 8);
        RailActor_SetGridPosByDir(actor, dir);
    }
    SetActorMovementFlag(actor, 0xa0);
    ChangeActorDirection(actor, dir);
    FldAct_SetDrawStatus(actor, frame);
    FieldActor_NextAcmdState(actor);
    if (se != 0) {
        GFL_SndSEPlay(se);
    }
}

void RailActorAcmd_InitJumpWithSe(FieldActor *actor, u16 dir, u32 speed, u32 timer, u16 frame, s16 height, u16 step) {
    RailActorAcmd_InitJump(actor, dir, speed, timer, frame, height, step, SEQ_SE_DANSA);
}
