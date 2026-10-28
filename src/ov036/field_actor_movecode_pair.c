// The move codes that follow another actor or the player, hide an actor under a field effect, turn it with the player
// and walk it along a wall: the functions that field_actor_movedata.c's tables list for each, and the steps they run.
// The ROM gives this file no name, so field_actor_movecode_pair.c is a guess after the first of them
// (FindPairedTrainerActor is swan's). Its .text starts at 0x02192a4c and its .rodata, sorted by size on its own, at
// 0x021cbeec (Black 2). The code was first written by mariano54 on their parallel-decomp branch

#include "types.h"
#include "constants/directions.h"
#include "field/event_trainer_eye.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/field_actor_movecode_pair.h"
#include "field/field_effect.h"
#include "field/field_map.h"
#include "nitro/fx.h"

// A position in fx32 and the grid square that holds it
#define FX32_TO_GRID(pos) ((s16)(((pos) >> 4) / FX32_ONE))
#define GRID_TO_FX32(n) ((n) * FX32_CONST(16))

// The block of the codes that follow another actor or the player
typedef struct {
    u8 state;
    u8 active;
    s16 x;
    s16 z;
    s16 unk6;
    FieldActor *target;
} ActorFollowWork;

typedef struct {
    u8 state;
    u8 variant;
} ActorBobWork;

typedef struct {
    u8 state;
    u8 kind;
    u8 hasEffect;
    FieldEffectTask *task;
} ActorEffectWork;

typedef struct {
    u8 state;
    s8 dir;
    u8 flag;
} ActorTurnWork;

typedef struct {
    u8 state;
    s32 mode;
    s32 variant;
} ActorWallWalkWork;

static BOOL FieldActorMove_LookPlayerWaitPlayerStep(FieldActor *actor, ActorFollowWork *work);
static BOOL FieldActorMove_LookPlayerWaitStep(FieldActor *actor, ActorFollowWork *work);
static BOOL FieldActorMove_UpdatePlayerTracking(FieldActor *actor, ActorFollowWork *work);
static void FieldActorMove_StartPlayerTracking(FieldActor *actor, ActorFollowWork *work);
static BOOL FieldActorMove_HasPlayerMoved(FieldActor *actor, ActorFollowWork *work);
static void FieldActorMove_SavePlayerGridPos(FieldActor *actor, ActorFollowWork *work);
static u32 FieldActorMove_GetPlayerCopyAcmd(FieldActor *actor);
static BOOL FieldActorMove_StepToPlayerInitGrid(FieldActor *actor);
static BOOL FieldActorMove_LookFellowTurn(FieldActor *actor, ActorFollowWork *work);
static BOOL FieldActorMove_LookFellowWaitTurn(FieldActor *actor, ActorFollowWork *work);
static BOOL FieldActorMove_UpdateFellowTracking(FieldActor *actor, ActorFollowWork *work);
static void FieldActorMove_StartFellowTracking(FieldActor *actor, ActorFollowWork *work, FieldActor *other);
static BOOL FieldActorMove_CanTurnToFellow(FieldActor *actor, ActorFollowWork *work);
static BOOL FieldActorMove_TurnToFellow(FieldActor *actor, ActorFollowWork *work);
static void FieldActorMove_InitHide(FieldActor *actor, u32 variant);
static FieldEffectTask *FldeffHide_Create(FieldActor *actor, u32 kind);
static BOOL FieldActorMove_HideCreateEffect(FieldActor *actor, ActorEffectWork *work);
static BOOL FieldActorMove_HideKeepEffect(FieldActor *actor, ActorEffectWork *work);
static void FieldActorMove_InitFaceDir(FieldActor *actor, u32 unused, u32 flag);
static BOOL FieldActorMove_FaceDirTurnSelf(FieldActor *actor, ActorTurnWork *work);
static BOOL FieldActorMove_FaceDirWaitTurn(FieldActor *actor, ActorTurnWork *work);
static BOOL FieldActorMove_FaceDirResolvePlayerDir(FieldActor *actor, ActorTurnWork *work);
static BOOL FieldActorMove_FaceDirTurnPlayerDir(FieldActor *actor, ActorTurnWork *work);
static BOOL FieldActorMove_FaceDirWaitPlayerTurn(FieldActor *actor, ActorTurnWork *work);
static void FieldActorMove_InitWallWalk(FieldActor *actor, s32 mode, s32 variant);
static BOOL FieldActorMove_WallWalkStart(FieldActor *actor, ActorWallWalkWork *work);
static BOOL FieldActorMove_WallWalkStep(FieldActor *actor, ActorWallWalkWork *work);
static BOOL FieldActorMove_WallWalkWaitStep(FieldActor *actor, ActorWallWalkWork *work);
static BOOL FieldActorMove_IsWallAtOffsetA(FieldActor *actor, const VecFx32 *pos, u32 dir, u32 variant);
static BOOL FieldActorMove_IsWallAtOffsetB(FieldActor *actor, const VecFx32 *pos, u32 dir, u32 variant);
static BOOL FieldActorMove_IsWallNearA(FieldActor *actor, u32 dir, u32 variant);
static BOOL FieldActorMove_IsWallNearB(FieldActor *actor, u32 dir, u32 variant);
static u32 FieldActorMove_GetWallWalkDir(FieldActor *actor, u32 dir, u32 variant);
static u32 FieldActorMove_HitCheckWallWalkDir(FieldActor *actor, u32 *dir, u32 variant);
static BOOL FieldActorMove_WallWalkStepOnce(FieldActor *actor, ActorWallWalkWork *work, u32 speed);

// The tables are declared in this order because MWCC lays out objects of the same size by where each was declared:
// it is one that gives the ROM's .rodata (found with tools/decomp/rodata_order.py's model)
static BOOL (*const data_ov036_021cbeec[])(FieldActor *actor,
                                           ActorFollowWork *work) = { FieldActorMove_LookPlayerWaitPlayerStep,
                                                                      FieldActorMove_LookPlayerWaitStep };

static BOOL (*const data_ov036_021cbef4[])(FieldActor *actor,
                                           ActorEffectWork *work) = { FieldActorMove_HideCreateEffect,
                                                                      FieldActorMove_HideKeepEffect };

static BOOL (*const data_ov036_021cbf18[])(FieldActor *actor, ActorTurnWork *work) = {
    FieldActorMove_FaceDirTurnSelf, FieldActorMove_FaceDirWaitTurn, FieldActorMove_FaceDirResolvePlayerDir,
    FieldActorMove_FaceDirTurnPlayerDir, FieldActorMove_FaceDirWaitPlayerTurn
};

static BOOL (*const data_ov036_021cbf04[])(FieldActor *actor,
                                           ActorFollowWork *work) = { FieldActorMove_LookFellowTurn,
                                                                      FieldActorMove_LookFellowWaitTurn };

static BOOL (*const data_ov036_021cbf0c[])(FieldActor *actor, ActorWallWalkWork *work) = {
    FieldActorMove_WallWalkStart, FieldActorMove_WallWalkStep, FieldActorMove_WallWalkWaitStep
};

static const s32 data_ov036_021cbfac[][2] = { { 0x1, 0x1 }, { -0x1, -0x1 }, { 0x1, -0x1 }, { -0x1, 0x1 } };

static const u32 data_ov036_021cbefc[] = { 0x1, 0 };

static const s32 data_ov036_021cbf2c[][2] = { { -0x1, 0x1 }, { 0x1, -0x1 }, { 0, 0 }, { 0, 0 } };

static const s32 data_ov036_021cbf4c[][2] = { { 0, 0 }, { 0, 0 }, { 0x1, -0x1 }, { -0x1, 0x1 } };

static const s32 data_ov036_021cbf8c[][2] = { { -0x1, 0x1 }, { 0x1, -0x1 }, { 0x1, 0x1 }, { -0x1, -0x1 } };

static const s32 data_ov036_021cbfcc[][2] = {
    { DIR_RIGHT, DIR_LEFT }, { DIR_LEFT, DIR_RIGHT }, { DIR_UP, DIR_DOWN }, { DIR_DOWN, DIR_UP }
};

static const s32 data_ov036_021cbf6c[][2] = {
    { DIR_LEFT, DIR_RIGHT }, { DIR_RIGHT, DIR_LEFT }, { DIR_DOWN, DIR_UP }, { DIR_UP, DIR_DOWN }
};

void FieldActorMove_InitLookPlayer(FieldActor *actor) {
    ActorFollowWork *work = ClearActorPositionBlock(actor, 8);

    FieldActorMove_UpdatePlayerTracking(actor, work);
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
    SetActorFlag8000(actor, 0);
}

void FieldActorMove_MoveLookPlayer(FieldActor *actor) {
    ActorFollowWork *work = GetNPCRailPosPtrAddr(actor);

    if (FieldActorMove_UpdatePlayerTracking(actor, work)) {
        while (data_ov036_021cbeec[work->state](actor, work) == TRUE) {
        }
    }
}

void FieldActorMove_DeleteLookPlayer(FieldActor *actor) {
}

static BOOL FieldActorMove_LookPlayerWaitPlayerStep(FieldActor *actor, ActorFollowWork *work) {
    FldAct_ClearMoving(actor);
    FldAct_ClearMoveEnd(actor);
    if (IsActorHidden(actor)) {
        return FALSE;
    }
    if (FieldActorMove_HasPlayerMoved(actor, work) == TRUE) {
        FieldActorMove_SavePlayerGridPos(actor, work);
        if (FieldActorMove_StepToPlayerInitGrid(actor) == TRUE) {
            FldAct_SetMoving(actor);
            work->state++;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FieldActorMove_LookPlayerWaitStep(FieldActor *actor, ActorFollowWork *work) {
    if (FieldAcmdTCB_Update(actor) == TRUE) {
        FldAct_ClearMoving(actor);
        work->state = 0;
    }
    return FALSE;
}

static BOOL FieldActorMove_UpdatePlayerTracking(FieldActor *actor, ActorFollowWork *work) {
    if (FindPlayerFieldActor(GetActorMModelSystem(actor)) == NULL) {
        work->active = 0;
        return FALSE;
    }
    if (work->active == 0) {
        FieldActorMove_StartPlayerTracking(actor, work);
    }
    return TRUE;
}

static void FieldActorMove_StartPlayerTracking(FieldActor *actor, ActorFollowWork *work) {
    FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));

    work->active = 1;
    work->x = GetGPosX(player);
    work->z = GetGPosZ(player);
    work->unk6 = 0xff;
}

static BOOL FieldActorMove_HasPlayerMoved(FieldActor *actor, ActorFollowWork *work) {
    FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));

    if (player != NULL) {
        s32 x = GetGPosX(player);
        s32 z = GetGPosZ(player);

        if (x != work->x || z != work->z) {
            return TRUE;
        }
    }
    return FALSE;
}

static void FieldActorMove_SavePlayerGridPos(FieldActor *actor, ActorFollowWork *work) {
    FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));

    work->x = GetGPosX(player);
    work->z = GetGPosZ(player);
}

static u32 FieldActorMove_GetPlayerCopyAcmd(FieldActor *actor) {
    u16 acmd = FldAct_GetAcmd(FindPlayerFieldActor(GetActorMModelSystem(actor)));

    switch (acmd) {
    case 0x58:
        return 0x10;
    case 0x59:
        return 0x11;
    case 0x5a:
        return 0x12;
    case 0x5b:
        return 0x13;
    case 0xa3:
        return 0x4c;
    case 0xa4:
        return 0x4d;
    case 0xa5:
        return 0x4e;
    case 0xa6:
        return 0x4f;
    }
    return acmd;
}

static BOOL FieldActorMove_StepToPlayerInitGrid(FieldActor *actor) {
    FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));
    s32 x = GetGPosX(actor);
    s32 z = GetGPosZ(actor);
    s32 playerInitX = FldAct_GetInitGPosX(player);
    s32 playerInitZ = FldAct_GetInitGPosZ(player);
    u32 acmd;

    if (x != playerInitX || z != playerInitZ) {
        acmd = FieldActorMove_GetPlayerCopyAcmd(actor);
        if (acmd == 0xff) {
            return FALSE;
        }
        StartDirAcmd(actor, GetDirFromPosToPos(x, z, playerInitX, playerInitZ), acmd);
        return TRUE;
    }
    return FALSE;
}

void FieldActorMove_InitLookFellow(FieldActor *actor) {
    ActorFollowWork *work = ClearActorPositionBlock(actor, 12);

    FieldActorMove_UpdateFellowTracking(actor, work);
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
    work->active = 0;
}

void FieldActorMove_MoveLookFellow(FieldActor *actor) {
    ActorFollowWork *work = GetNPCRailPosPtrAddr(actor);

    if (FieldActorMove_UpdateFellowTracking(actor, work)) {
        while (data_ov036_021cbf04[work->state](actor, work) == TRUE) {
        }
    }
}

void FieldActorMove_DeleteLookFellow(FieldActor *actor) {
}

void FieldActorMove_ClearFellowTracking(FieldActor *actor) {
    ActorFollowWork *work = GetNPCRailPosPtrAddr(actor);

    work->active = 0;
}

static BOOL FieldActorMove_LookFellowTurn(FieldActor *actor, ActorFollowWork *work) {
    FldAct_ClearMoving(actor);
    FldAct_ClearMoveEnd(actor);
    if (FieldActorMove_CanTurnToFellow(actor, work) == TRUE) {
        if (FieldActorMove_TurnToFellow(actor, work) == TRUE) {
            FldAct_SetMoving(actor);
            work->state++;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FieldActorMove_LookFellowWaitTurn(FieldActor *actor, ActorFollowWork *work) {
    if (!FieldAcmdTCB_Update(actor)) {
        return FALSE;
    }
    FldAct_ClearMoving(actor);
    work->state = 0;
    return FALSE;
}

FieldActor *FindPairedTrainerActor(FieldActor *actor) {
    FieldActor *other;
    u32 index = 0;
    u16 evType = GetActorEvType(actor);
    u16 zoneId = GetActorZoneID(actor);
    MMSys *mmSys = GetActorMModelSystem(actor);
    u16 trainerId = GetNPCTrainerID(actor);

    switch (evType) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 20:
        while (NextActor(mmSys, &other, &index) == TRUE) {
            if (actor != other && zoneId == GetActorZoneID(other) && trainerId == GetNPCTrainerID(other)) {
                return other;
            }
        }
        break;
    }
    return NULL;
}

static BOOL FieldActorMove_UpdateFellowTracking(FieldActor *actor, ActorFollowWork *work) {
    u16 zoneId;
    u16 trainerId;
    u32 index;
    FieldActor *other;
    MMSys *mmSys;

    mmSys = GetActorMModelSystem(actor);
    index = 0;
    zoneId = GetActorZoneID(actor);
    trainerId = GetNPCTrainerID(actor);
    while (NextActor(mmSys, &other, &index) == TRUE) {
        if (actor != other && zoneId == GetActorZoneID(other) && trainerId == GetNPCTrainerID(other)) {
            if (work->active == 0) {
                FieldActorMove_StartFellowTracking(actor, work, other);
            }
            return TRUE;
        }
    }
    work->active = 0;
    return FALSE;
}

static void FieldActorMove_StartFellowTracking(FieldActor *actor, ActorFollowWork *work, FieldActor *other) {
    work->active = 1;
    work->x = GetGPosX(other);
    work->z = GetGPosZ(other);
    work->unk6 = 0xff;
    work->target = other;
}

static BOOL FieldActorMove_CanTurnToFellow(FieldActor *actor, ActorFollowWork *work) {
    FieldActor *target = work->target;
    s32 x = GetGPosX(actor);
    s32 z = GetGPosZ(actor);
    s32 targetInitX = FldAct_GetInitGPosX(target);
    s32 targetInitZ = FldAct_GetInitGPosZ(target);

    if (x != targetInitX || z != targetInitZ) {
        if (IsActorFlag16(target) == TRUE || !CheckActorMovementFlag(target, 0x1808)) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FieldActorMove_TurnToFellow(FieldActor *actor, ActorFollowWork *work) {
    s32 x = GetGPosX(actor);
    s32 z = GetGPosZ(actor);
    s32 targetX = GetGPosX(work->target);
    s32 targetZ = GetGPosZ(work->target);
    s32 targetInitX = FldAct_GetInitGPosX(work->target);
    s32 targetInitZ = FldAct_GetInitGPosZ(work->target);
    s32 dir;

    if (x == targetX && z == targetZ) {
        return FALSE;
    }
    dir = GetDirFromPosToPos(x, z, targetInitX, targetInitZ);
    x += GetDirectionVectorCompX(dir);
    z += GetDirectionVectorCompZ(dir);
    if (x != targetX || z != targetZ) {
        StartDirAcmd(actor, dir, 12);
        return TRUE;
    }
    return FALSE;
}

static void FieldActorMove_InitHide(FieldActor *actor, u32 variant) {
    ActorBobWork *work = ClearActorPositionBlock(actor, 8);

    work->variant = variant;
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
    SetActorMovementFlag(actor, 0x8000);
    VecFx32 offset = { 0, -32 * FX32_ONE, 0 };

    SetActorWPosOffset(actor, &offset);
}

void FieldActorMove_InitHideKind0Code33(FieldActor *actor) {
    FieldActorMove_InitHide(actor, 0);
}

void FieldActorMove_InitHideKind1Code34(FieldActor *actor) {
    FieldActorMove_InitHide(actor, 1);
}

void FieldActorMove_InitHideKind1Code35(FieldActor *actor) {
    FieldActorMove_InitHide(actor, 1);
}

void FieldActorMove_InitHideKind0Code36(FieldActor *actor) {
    FieldActorMove_InitHide(actor, 0);
}

void FieldActorMove_InitHideKind2Code55(FieldActor *actor) {
    FieldActorMove_InitHide(actor, 2);
}

void FieldActorMove_MoveHide(FieldActor *actor) {
    ActorEffectWork *work = GetNPCRailPosPtrAddr(actor);

    while (data_ov036_021cbef4[work->state](actor, work) == TRUE) {
    }
}

void FieldActorMove_DeleteHide(FieldActor *actor) {
    FieldEffectTask *task = FieldActorMove_GetHideEffect(actor);

    if (task != NULL) {
        FieldEffects_DeleteObject(task);
    }
}

void FieldActorMove_RecoverHide(FieldActor *actor) {
    ActorEffectWork *work = GetNPCRailPosPtrAddr(actor);

    work->state = 0;
    FieldActorMove_SetHideEffect(actor, NULL);
    if (work->hasEffect == 0) {
        VecFx32 offset = { 0, -32 * FX32_ONE, 0 };

        SetActorWPosOffset(actor, &offset);
        SetActorMovementFlag(actor, 0x8000);
    }
}

static FieldEffectTask *FldeffHide_Create(FieldActor *actor, u32 kind) {
    return HideTask_Start(Field_GetFieldEffects(GetMMSysField(GetActorMModelSystem(actor))), actor, kind);
}

static BOOL FieldActorMove_HideCreateEffect(FieldActor *actor, ActorEffectWork *work) {
    if (work->hasEffect == 0) {
        FieldActorMove_SetHideEffect(actor, FldeffHide_Create(actor, work->kind));
    }
    FldAct_ClearMoving(actor);
    FldAct_ClearMoveEnd(actor);
    work->state++;
    return FALSE;
}

static BOOL FieldActorMove_HideKeepEffect(FieldActor *actor, ActorEffectWork *work) {
    if (work->hasEffect == 0) {
        if (FieldActorMove_GetHideEffect(actor) == NULL) {
            if (FldAct_IsDrawReady(actor) == TRUE) {
                FieldActorMove_SetHideEffect(actor, FldeffHide_Create(actor, work->kind));
            }
        }
        SetActorMovementFlag(actor, 0x8000);
    }
    return FALSE;
}

void FieldActorMove_SetHideEffect(FieldActor *actor, FieldEffectTask *task) {
    ActorEffectWork *work = GetNPCRailPosPtrAddr(actor);

    work->task = task;
}

FieldEffectTask *FieldActorMove_GetHideEffect(FieldActor *actor) {
    ActorEffectWork *work = GetNPCRailPosPtrAddr(actor);

    return work->task;
}

void FieldActorMove_SetHideNoEffect(FieldActor *actor) {
    ActorEffectWork *work = GetNPCRailPosPtrAddr(actor);

    work->hasEffect = 1;
}

static void FieldActorMove_InitFaceDir(FieldActor *actor, u32 unused, u32 flag) {
    ActorTurnWork *work = ClearActorPositionBlock(actor, 4);

    work->dir = DIR_NONE;
    work->flag = flag;
    CheckSetActorFaceDir(actor, DIR_UP);
}

void FieldActorMove_InitFaceDirSet0(FieldActor *actor) {
    FieldActorMove_InitFaceDir(actor, 0, 0);
}

void FieldActorMove_InitFaceDirSet1(FieldActor *actor) {
    FieldActorMove_InitFaceDir(actor, 1, 0);
}

void FieldActorMove_InitFaceDirSet2(FieldActor *actor) {
    FieldActorMove_InitFaceDir(actor, 2, 0);
}

void FieldActorMove_InitFaceDirSet3(FieldActor *actor) {
    FieldActorMove_InitFaceDir(actor, 3, 0);
}

void FieldActorMove_InitFaceDirFlagSet0(FieldActor *actor) {
    FieldActorMove_InitFaceDir(actor, 0, 1);
}

void FieldActorMove_InitFaceDirFlagSet1(FieldActor *actor) {
    FieldActorMove_InitFaceDir(actor, 1, 1);
}

void FieldActorMove_InitFaceDirFlagSet2(FieldActor *actor) {
    FieldActorMove_InitFaceDir(actor, 2, 1);
}

void FieldActorMove_InitFaceDirFlagSet3(FieldActor *actor) {
    FieldActorMove_InitFaceDir(actor, 3, 1);
}

void FieldActorMove_MoveFaceDir(FieldActor *actor) {
    ActorTurnWork *work = GetNPCRailPosPtrAddr(actor);

    while (data_ov036_021cbf18[work->state](actor, work) == TRUE) {
    }
}

static BOOL FieldActorMove_FaceDirTurnSelf(FieldActor *actor, ActorTurnWork *work) {
    u32 dir = GetActorFaceDir(actor);

    StartDirAcmd(actor, dir, 0);
    FldAct_ClearMoving(actor);
    FldAct_ClearMoveEnd(actor);
    work->state = 1;
    return TRUE;
}

static BOOL FieldActorMove_FaceDirWaitTurn(FieldActor *actor, ActorTurnWork *work) {
    if (FieldAcmdTCB_Update(actor) == TRUE) {
        work->state = 2;
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldActorMove_FaceDirResolvePlayerDir(FieldActor *actor, ActorTurnWork *work) {
    if (work->dir == DIR_NONE) {
        FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));

        if (player != NULL) {
            work->dir = GetActorMotionDir(player);
        }
    }
    FldAct_ClearMoving(actor);
    FldAct_ClearMoveEnd(actor);
    work->state = 3;
    return TRUE;
}

static BOOL FieldActorMove_FaceDirTurnPlayerDir(FieldActor *actor, ActorTurnWork *work) {
    u32 acmd = GetAcmdForDir(GetActorMotionDir(FindPlayerFieldActor(GetActorMModelSystem(actor))), 0);

    FldAct_SetLocalAcmd(actor, acmd);
    work->state = 4;
    return TRUE;
}

static BOOL FieldActorMove_FaceDirWaitPlayerTurn(FieldActor *actor, ActorTurnWork *work) {
    if (!FieldAcmdTCB_Update(actor)) {
        return FALSE;
    }
    FldAct_ClearMoving(actor);
    FldAct_ClearMoveEnd(actor);
    work->state = 2;
    return FALSE;
}

static void FieldActorMove_InitWallWalk(FieldActor *actor, s32 mode, s32 variant) {
    ActorWallWalkWork *work = ClearActorPositionBlock(actor, 12);

    work->mode = mode;
    work->variant = variant;
}

void FieldActorMove_InitWallWalkLeftHand(FieldActor *actor) {
    FieldActorMove_InitWallWalk(actor, 0, 0);
}

void FieldActorMove_InitWallWalkRightHand(FieldActor *actor) {
    FieldActorMove_InitWallWalk(actor, 1, 1);
}

void FieldActorMove_InitWallWalkLeftHandReversing(FieldActor *actor) {
    FieldActorMove_InitWallWalk(actor, 2, 0);
}

void FieldActorMove_InitWallWalkRightHandReversing(FieldActor *actor) {
    FieldActorMove_InitWallWalk(actor, 2, 1);
}

void FieldActorMove_MoveWallWalk(FieldActor *actor) {
    ActorWallWalkWork *work = GetNPCRailPosPtrAddr(actor);

    while (data_ov036_021cbf0c[work->state](actor, work) == TRUE) {
    }
}

static BOOL FieldActorMove_WallWalkStart(FieldActor *actor, ActorWallWalkWork *work) {
    FldAct_ClearMoving(actor);
    work->state++;
    return TRUE;
}

static BOOL FieldActorMove_WallWalkStep(FieldActor *actor, ActorWallWalkWork *work) {
    FieldActorMove_WallWalkStepOnce(actor, work, 12);
    work->state++;
    return TRUE;
}

static BOOL FieldActorMove_WallWalkWaitStep(FieldActor *actor, ActorWallWalkWork *work) {
    if (FieldAcmdTCB_Update(actor) == TRUE) {
        FldAct_ClearMoving(actor);
        work->state = 0;
    }
    return FALSE;
}

static BOOL FieldActorMove_IsWallAtOffsetA(FieldActor *actor, const VecFx32 *pos, u32 dir, u32 variant) {
    u32 tileType;
    BOOL blocked = FALSE;
    VecFx32 target = *pos;
    s16 x = FX32_TO_GRID(pos->x);
    s16 z = FX32_TO_GRID(pos->z);

    x += data_ov036_021cbf2c[dir][variant];
    z += data_ov036_021cbf4c[dir][variant];
    target.x = GRID_TO_FX32(x);
    target.z = GRID_TO_FX32(z);
    if (GetTileTypeAtPosByActor(actor, &target, &tileType) == TRUE) {
        if (MapTile_BlocksCollision(tileType)) {
            blocked = TRUE;
        }
    }
    return blocked;
}

static BOOL FieldActorMove_IsWallAtOffsetB(FieldActor *actor, const VecFx32 *pos, u32 dir, u32 variant) {
    u32 tileType;
    BOOL blocked = FALSE;
    VecFx32 target = *pos;
    s16 x = FX32_TO_GRID(pos->x);
    s16 z = FX32_TO_GRID(pos->z);

    x += data_ov036_021cbf8c[dir][variant];
    z += data_ov036_021cbfac[dir][variant];
    target.x = GRID_TO_FX32(x);
    target.z = GRID_TO_FX32(z);
    if (GetTileTypeAtPosByActor(actor, &target, &tileType) == TRUE) {
        if (MapTile_BlocksCollision(tileType)) {
            blocked = TRUE;
        }
    }
    return blocked;
}

static BOOL FieldActorMove_IsWallNearA(FieldActor *actor, u32 dir, u32 variant) {
    VecFx32 pos;

    CopyActorWPos(actor, &pos);
    return FieldActorMove_IsWallAtOffsetA(actor, &pos, dir, variant);
}

static BOOL FieldActorMove_IsWallNearB(FieldActor *actor, u32 dir, u32 variant) {
    VecFx32 pos;

    CopyActorWPos(actor, &pos);
    return FieldActorMove_IsWallAtOffsetB(actor, &pos, dir, variant);
}

static u32 FieldActorMove_GetWallWalkDir(FieldActor *actor, u32 dir, u32 variant) {
    if (!FieldActorMove_IsWallNearA(actor, dir, variant)) {
        if (!FieldActorMove_IsWallNearB(actor, dir, variant)) {
            return DIR_NONE;
        }
        dir = data_ov036_021cbf6c[dir][variant];
    }
    return dir;
}

static u32 FieldActorMove_HitCheckWallWalkDir(FieldActor *actor, u32 *dir, u32 variant) {
    *dir = FieldActorMove_GetWallWalkDir(actor, *dir, variant);
    if (*dir != DIR_NONE) {
        return ActorRouteCollCheckOneTileInDir(actor, *dir);
    }
    return 0;
}

static BOOL FieldActorMove_WallWalkStepOnce(FieldActor *actor, ActorWallWalkWork *work, u32 speed) {
    u32 acmd;
    u32 variant = work->variant;
    u32 dir = GetActorMotionDir(actor);
    u32 result = FieldActorMove_HitCheckWallWalkDir(actor, &dir, variant);

    if (dir == DIR_NONE) {
        dir = GetActorMotionDir(actor);
        acmd = GetAcmdForDir(dir, 0x1c);
        FldAct_SetLocalAcmd(actor, acmd);
        return FALSE;
    }
    if (result == 0) {
        acmd = GetAcmdForDir(dir, speed);
        FldAct_SetMoving(actor);
        FldAct_SetLocalAcmd(actor, acmd);
        return TRUE;
    }
    if ((result & 1) && work->mode == 2) {
        dir = GetInverseDirection(GetActorMotionDir(actor));
        variant = data_ov036_021cbefc[variant];
        work->variant = variant;
        result = FieldActorMove_HitCheckWallWalkDir(actor, &dir, variant);
        if (dir == DIR_NONE) {
            dir = GetActorMotionDir(actor);
            acmd = GetAcmdForDir(dir, 0x1c);
            FldAct_SetLocalAcmd(actor, acmd);
            return FALSE;
        }
        if (result == 0) {
            acmd = GetAcmdForDir(dir, speed);
            FldAct_SetMoving(actor);
            FldAct_SetLocalAcmd(actor, acmd);
            return TRUE;
        }
    }
    if (result & 2) {
        dir = data_ov036_021cbfcc[dir][variant];
        result = FieldActorMove_HitCheckWallWalkDir(actor, &dir, variant);
        if (dir == DIR_NONE) {
            dir = GetActorMotionDir(actor);
            acmd = GetAcmdForDir(dir, 0x1c);
            FldAct_SetLocalAcmd(actor, acmd);
            return FALSE;
        }
        if (result == 0) {
            acmd = GetAcmdForDir(dir, speed);
            FldAct_SetMoving(actor);
            FldAct_SetLocalAcmd(actor, acmd);
            return TRUE;
        }
    }
    dir = GetActorMotionDir(actor);
    acmd = GetAcmdForDir(dir, 0x1c);
    FldAct_SetLocalAcmd(actor, acmd);
    return FALSE;
}
