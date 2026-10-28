// The move codes by which actors look around, wander in a rectangle or by a mask of directions, turn, pace back and
// forth and walk around a square: the functions that fldmmdl_movedata.c's tables list for each, and the steps they
// run. The ROM gives this file no name, so fldmmdl_movecode.c is a guess. It follows fldmmdl_acmd.c, and its
// .rodata, sorted by size on its own, starts at 0x021cb87c (Black 2). The code was first written by mariano54 on
// their parallel-decomp branch

#include "types.h"
#include "constants/directions.h"
#include "field/event_trainer_eye.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/fldmmdl_movecode.h"
#include "gfl/key.h"
#include "gfl/random.h"

// The facing a walking code keeps for actors that turn and then restore it: the facing before, a row of
// data_ov036_021cbb44 and whether movement flag 8 was set
typedef struct {
    u8 dir;
    s8 row;
    u8 hadFlag8;
} ActorFaceMemory;

// The work of a wandering rail actor: pause is set while it is held, mask picks the directions it may turn to
typedef struct {
    u16 pause;
    s16 timer;
    u32 mask;
} RailWanderWork;

// The work of a rail actor that walks along the directions of a mask
typedef struct {
    s16 state;
    s16 timer;
    s32 mode;
    s32 acmdBase;
    u32 mask;
} RailWalkWork;

// The rectangle of grid positions a move code walks in
typedef struct {
    s32 minX;
    s32 minZ;
    s32 maxX;
    s32 maxZ;
} WalkRect;

// The work of an actor that faces a direction once
typedef struct {
    u32 dir;
    u32 state;
} RailFaceWork;

// The work of an actor that turns around a rotation of directions: turn is 2 for a clockwise one, state picks the
// function of the table that runs
typedef struct {
    s8 turn;
    u8 pad1;
    s8 state;
    u8 pad3;
    s32 count;
} RailTurnWork;

// The work of an actor that paces back and forth: flip is set after it was blocked, wait counts a pause down
typedef struct {
    u8 state;
    u8 flip;
    u8 wait;
    u8 pad3;
    ActorFaceMemory face;
    u8 pad;
} RailPaceWork;

// The work of an actor that paces a fixed number of steps
typedef struct {
    u8 state;
    u8 flip;
    u8 steps;
    u8 turns;
    u8 speed;
    u8 pad5[3];
    ActorFaceMemory face;
    u8 pad;
} RailStepWork;

// The block of the codes that walk back and forth along one axis
typedef struct {
    u8 state;
    s8 dirIdx;
    u8 limit;
    u8 axis;
    u8 dirTable;
    u8 reverse;
    u8 wait;
    u8 unk7;
    ActorFaceMemory face;
} ActorPatrolWork;

// The block of the codes that walk around a square
typedef struct {
    u8 state;
    u8 dirIdx;
    u8 limit;
    u8 axis;
    u8 dirTable;
    u8 wait;
    u8 unk6[2];
    ActorFaceMemory face;
} ActorRoundWork;

typedef struct {
    u16 first;
    u16 second;
} ActorMoveCodePair;

typedef struct {
    s32 key;
    const s32 *dirs;
} ActorDirTableEntry;

static void FldmmdlMovecode_InitTurnRandom(FieldActor *actor, u32 mask);
static u16 FldmmdlMovecode_GetNextDirInRotation(u16 moveCode, u16 faceDir);
static void FldmmdlMovecode_InitWalkRandom(FieldActor *actor, u32 acmdBase, u32 dirMask, u32 mode);
static void FldmmdlMovecode_GetMoveRect(FieldActor *actor, WalkRect *rect);
static BOOL FldmmdlMovecode_IsDirInsideMoveRect(FieldActor *actor, u32 dir);
static void FldmmdlMovecode_InitFixedDir(FieldActor *actor, u32 dir);
static void FldmmdlMovecode_InitSpin(FieldActor *actor, u32 turn);
static BOOL FldmmdlMovecode_SpinStepTurn(FieldActor *actor, RailTurnWork *work);
static BOOL FldmmdlMovecode_SpinStepWaitTurn(FieldActor *actor, RailTurnWork *work);
static BOOL FldmmdlMovecode_SpinStepWaitInterval(FieldActor *actor, RailTurnWork *work);
static BOOL FldmmdlMovecode_SpinStepRotateDir(FieldActor *actor, RailTurnWork *work);
static BOOL FldmmdlMovecode_SwingStepTurn(FieldActor *actor, RailTurnWork *work);
static BOOL FldmmdlMovecode_SwingStepWaitTurn(FieldActor *actor, RailTurnWork *work);
static BOOL FldmmdlMovecode_SwingStepWaitInterval(FieldActor *actor, RailTurnWork *work);
static BOOL FldmmdlMovecode_SwingStepRotateDir(FieldActor *actor, RailTurnWork *work);
static BOOL FldmmdlMovecode_BackForthStepFaceDir(FieldActor *actor, RailPaceWork *work);
static BOOL FldmmdlMovecode_BackForthStepWalk(FieldActor *actor, RailPaceWork *work);
static BOOL FldmmdlMovecode_BackForthStepWaitWalk(FieldActor *actor, RailPaceWork *work);
static void FldmmdlMovecode_InitRoutePatrol(FieldActor *actor, u32 steps, u32 turns, u32 speed);
static BOOL FldmmdlMovecode_RoutePatrolStepWalk(FieldActor *actor, ActorRoundWork *work);
static BOOL FldmmdlMovecode_RoutePatrolStepWaitWalk(FieldActor *actor, ActorRoundWork *work);
static void FldmmdlMovecode_InitRoutePatrolSwap(FieldActor *actor, u32 limit, u32 axis, u32 dirTable);
static void FldmmdlMovecode_AdvanceRouteIndex(ActorPatrolWork *work);
static BOOL FldmmdlMovecode_RoutePatrolSwapStepLookForPlayer(FieldActor *actor, ActorPatrolWork *work);
static BOOL FldmmdlMovecode_RoutePatrolSwapStepSwapMoveCode(FieldActor *actor, ActorPatrolWork *work);
static BOOL FldmmdlMovecode_RoutePatrolSwapStepWalk(FieldActor *actor, ActorPatrolWork *work);
static BOOL FldmmdlMovecode_RoutePatrolSwapStepWaitWalk(FieldActor *actor, ActorPatrolWork *work);
static BOOL FldmmdlMovecode_IsBButtonHeld(void);
static BOOL FldmmdlMovecode_IsPlayerWalking(FieldActor *actor);
static BOOL FldmmdlMovecode_IsBButtonPressed(void);
static u32 FldmmdlMovecode_FindTableEnd(const s32 *list, u32 end);
static s32 FldmmdlMovecode_GetRandomTableEntry(const s32 *list, u32 end);
static s32 FldmmdlMovecode_GetRandomDirFromSet(u32 index, u32 end);
static const s32 *FldmmdlMovecode_GetDirSet(u32 index);
static u32 FldmmdlMovecode_GetSightDirToPlayer(FieldActor *actor);
static u32 FldmmdlMovecode_GetDirTowardPlayerInSet(FieldActor *actor, u32 index, u32 end);
static BOOL FldmmdlMovecode_UpdateTriggerWait(FieldActor *actor, u8 *wait);
static BOOL FldmmdlMovecode_IsDashEventType(FieldActor *actor);
static BOOL FldmmdlMovecode_IsMirrorEventType(FieldActor *actor);
static void FldmmdlMovecode_InitMirrorSide(FieldActor *actor, ActorFaceMemory *face);
static void FldmmdlMovecode_StartMirrorTurn(FieldActor *actor, ActorFaceMemory *face);
static void FldmmdlMovecode_EndMirrorTurn(FieldActor *actor, ActorFaceMemory *face);

// The tables are declared in this order because MWCC lays out objects of the same size by where each was declared:
// it is one that gives the ROM's .rodata (found with tools/decomp/rodata_order.py's model), not the original's, which
// is unknown. data_ov036_021cb8d8, data_ov036_021cbb64 and data_ov036_021cbc70 follow
// FldmmdlMovecode_SwingStepRotateDir for the same reason
static BOOL (*const data_ov036_021cb87c[])(FieldActor *actor,
                                           ActorRoundWork *work) = { FldmmdlMovecode_RoutePatrolStepWalk,
                                                                     FldmmdlMovecode_RoutePatrolStepWaitWalk };

static const s32 data_ov036_021cb9f0[] = { DIR_LEFT, DIR_UP, DIR_DOWN, DIR_RIGHT };

static const s32 data_ov036_021cb8e4[] = { DIR_UP, DIR_DOWN, DIR_NONE };

static const s32 data_ov036_021cb8b4[] = { DIR_DOWN, DIR_RIGHT, DIR_NONE };

static const s32 data_ov036_021cb884[] = { DIR_UP, DIR_LEFT, DIR_NONE };

static const s32 data_ov036_021cb8cc[] = { DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 data_ov036_021cb890[] = { DIR_DOWN, DIR_LEFT, DIR_NONE };

static const s32 data_ov036_021cb9c0[] = { DIR_UP, DIR_RIGHT, DIR_LEFT, DIR_DOWN };

static const s32 data_ov036_021cb8a8[] = { DIR_UP, DIR_RIGHT, DIR_NONE };

static const s32 data_ov036_021cb89c[] = { DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 data_ov036_021cb990[] = { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT };

static const s32 data_ov036_021cb950[] = { DIR_DOWN, DIR_UP, DIR_LEFT, DIR_RIGHT };

static const s32 data_ov036_021cb8c0[] = { DIR_UP, DIR_DOWN, DIR_NONE };

static const s32 data_ov036_021cb980[] = { DIR_LEFT, DIR_UP, DIR_RIGHT, DIR_DOWN };

static const s32 data_ov036_021cb9b0[] = { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_UP };

static const s32 data_ov036_021cbb1c[] = { 0x10, 0x20, 0x30, 0x40, -1 };

static const s32 data_ov036_021cba90[] = { DIR_RIGHT, DIR_UP, DIR_LEFT, DIR_DOWN };

static const s32 data_ov036_021cba80[] = { DIR_DOWN, DIR_RIGHT, DIR_LEFT, DIR_UP };

static const s32 data_ov036_021cba00[] = { DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_LEFT };

static const s32 data_ov036_021cba40[] = { DIR_UP, DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 data_ov036_021cba50[] = { DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN };

static const s32 data_ov036_021cba70[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_NONE };

static const s32 data_ov036_021cbab0[] = { DIR_LEFT, DIR_DOWN, DIR_UP, DIR_RIGHT };

static const s32 data_ov036_021cba20[] = { DIR_LEFT, DIR_DOWN, DIR_RIGHT, DIR_UP };

static const s32 data_ov036_021cba10[] = { DIR_RIGHT, DIR_UP, DIR_DOWN, DIR_LEFT };

static const s32 data_ov036_021cb910[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

static BOOL (*const data_ov036_021cb8f0[])(FieldActor *actor, ActorPatrolWork *work) = {
    FldmmdlMovecode_RoutePatrolSwapStepLookForPlayer, FldmmdlMovecode_RoutePatrolSwapStepSwapMoveCode,
    FldmmdlMovecode_RoutePatrolSwapStepWalk, FldmmdlMovecode_RoutePatrolSwapStepWaitWalk
};

static const s32 data_ov036_021cb9e0[] = { DIR_DOWN, DIR_UP, DIR_RIGHT, DIR_LEFT };

static const s32 data_ov036_021cb9d0[] = { DIR_RIGHT, DIR_LEFT, DIR_DOWN, DIR_UP };

static const s32 data_ov036_021cbb44[][4] = { { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT },
                                              { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT } };

static const s32 data_ov036_021cb940[] = { DIR_LEFT, DIR_RIGHT, DIR_DOWN, DIR_UP };

static const s32 data_ov036_021cb9a0[] = { DIR_LEFT, DIR_RIGHT, DIR_DOWN, DIR_UP };

static BOOL (*const data_ov036_021cb960[])(FieldActor *actor, RailTurnWork *work) = {
    FldmmdlMovecode_SpinStepTurn,
    FldmmdlMovecode_SpinStepWaitTurn,
    FldmmdlMovecode_SpinStepWaitInterval,
    FldmmdlMovecode_SpinStepRotateDir,
};

static const s32 data_ov036_021cb970[] = { DIR_DOWN, DIR_LEFT, DIR_UP, DIR_RIGHT };

static const s32 data_ov036_021cbaf4[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 data_ov036_021cbaa0[] = { DIR_RIGHT, DIR_DOWN, DIR_LEFT, DIR_UP };

static const s32 data_ov036_021cba30[] = { DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_NONE };

static const s32 data_ov036_021cba60[] = { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 data_ov036_021cbac0[] = { DIR_RIGHT, DIR_DOWN, DIR_UP, DIR_LEFT };

static const s32 data_ov036_021cb920[] = { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT };

static const s32 data_ov036_021cbae0[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 data_ov036_021cb900[] = { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN };

static BOOL (*const data_ov036_021cb930[])(FieldActor *actor, RailTurnWork *work) = {
    FldmmdlMovecode_SwingStepTurn,
    FldmmdlMovecode_SwingStepWaitTurn,
    FldmmdlMovecode_SwingStepWaitInterval,
    FldmmdlMovecode_SwingStepRotateDir,
};

// The directions that each move code turns between, in turn, after the move code and ended by DIR_NONE. A move code
// of 0xff ends the table
static const u16 data_ov036_021cbbec[][6] = {
    { 4, DIR_UP, DIR_DOWN, DIR_NONE },
    { 5, DIR_LEFT, DIR_RIGHT, DIR_NONE },
    { 6, DIR_UP, DIR_LEFT, DIR_NONE },
    { 7, DIR_UP, DIR_RIGHT, DIR_NONE },
    { 8, DIR_DOWN, DIR_LEFT, DIR_NONE },
    { 9, DIR_DOWN, DIR_RIGHT, DIR_NONE },
    { 10, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_NONE },
    { 11, DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_NONE },
    { 12, DIR_UP, DIR_RIGHT, DIR_LEFT, DIR_NONE },
    { 13, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE },
    { 0xff },
};

static const ActorMoveCodePair data_ov036_021cbb08[] = {
    { 0x25, 0x2b }, { 0x26, 0x2c }, { 0x27, 0x2a }, { 0x28, 0x29 }, { 0xff, 0xff },
};

static const s32 data_ov036_021cbb30[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 data_ov036_021cbad0[] = { DIR_DOWN, DIR_RIGHT, DIR_UP, DIR_LEFT };

static void FldmmdlMovecode_InitTurnRandom(FieldActor *actor, u32 mask) {
    RailWanderWork *work = ClearActorPositionBlock(actor, sizeof(RailWanderWork));
    work->timer = FldmmdlMovecode_GetRandomTableEntry(data_ov036_021cbb1c, -1);
    work->mask = mask;
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
}

void FldmmdlMovecode_InitTurnRandomSet0(FieldActor *actor) {
    FldmmdlMovecode_InitTurnRandom(actor, 0);
}

void FldmmdlMovecode_InitTurnRandomSet1(FieldActor *actor) {
    FldmmdlMovecode_InitTurnRandom(actor, 1);
}

void FldmmdlMovecode_InitTurnRandomSet2(FieldActor *actor) {
    FldmmdlMovecode_InitTurnRandom(actor, 2);
}

void FldmmdlMovecode_InitTurnRandomSet3(FieldActor *actor) {
    FldmmdlMovecode_InitTurnRandom(actor, 3);
}

void FldmmdlMovecode_InitTurnRandomSet4(FieldActor *actor) {
    FldmmdlMovecode_InitTurnRandom(actor, 4);
}

void FldmmdlMovecode_InitTurnRandomSet5(FieldActor *actor) {
    FldmmdlMovecode_InitTurnRandom(actor, 5);
}

void FldmmdlMovecode_InitTurnRandomSet6(FieldActor *actor) {
    FldmmdlMovecode_InitTurnRandom(actor, 6);
}

void FldmmdlMovecode_InitTurnRandomSet7(FieldActor *actor) {
    FldmmdlMovecode_InitTurnRandom(actor, 7);
}

void FldmmdlMovecode_InitTurnRandomSet8(FieldActor *actor) {
    FldmmdlMovecode_InitTurnRandom(actor, 8);
}

void FldmmdlMovecode_InitTurnRandomSet9(FieldActor *actor) {
    FldmmdlMovecode_InitTurnRandom(actor, 9);
}

void FldmmdlMovecode_InitTurnRandomSet10(FieldActor *actor) {
    FldmmdlMovecode_InitTurnRandom(actor, 10);
}

void FldmmdlMovecode_MoveTurnRandom(FieldActor *actor) {
    RailWanderWork *work = GetNPCRailPosPtrAddr(actor);
    if (GetActorEvType(actor) == 0xb) {
        if (FldAct_IsNearPlayer(actor) == TRUE && FldmmdlMovecode_IsBButtonPressed() == TRUE) {
            u16 faceDir = GetActorFaceDir(actor);
            u16 dir = FldmmdlMovecode_GetNextDirInRotation(GetActorMoveCode(actor), faceDir);
            if (dir != DIR_NONE) {
                CheckSetActorFaceDir(actor, dir);
            }
        }
    } else {
        u32 dir = FldmmdlMovecode_GetDirTowardPlayerInSet(actor, work->mask, DIR_NONE);
        if (dir != DIR_NONE) {
            CheckSetActorFaceDir(actor, dir);
        } else if (work->pause == 0) {
            work->timer--;
            if (work->timer <= 0) {
                work->timer = FldmmdlMovecode_GetRandomTableEntry(data_ov036_021cbb1c, -1);
                dir = FldmmdlMovecode_GetRandomDirFromSet(work->mask, DIR_NONE);
                CheckSetActorFaceDir(actor, dir);
            }
        }
    }
    SetActorInitialGPosToNowGPos(actor);
}

static u16 FldmmdlMovecode_GetNextDirInRotation(u16 moveCode, u16 faceDir) {
    s32 i = 0;
    do {
        if (moveCode == data_ov036_021cbbec[i][0]) {
            const u16 *entry;
            s32 j = 1;

            entry = data_ov036_021cbbec[i];
            if (entry[1] != DIR_NONE) {
                while (TRUE) {
                    if (faceDir == entry[j]) {
                        j++;
                        if (entry[j] == DIR_NONE) {
                            j = 1;
                        }
                        return entry[j];
                    }
                    j++;
                    if (entry[j] == DIR_NONE) {
                        break;
                    }
                }
            }
            return DIR_NONE;
        }
        i++;
    } while (data_ov036_021cbbec[i][0] != 0xff);
    return DIR_NONE;
}

void func_ov036_021913dc(FieldActor *actor) {
}

void FldmmdlMovecode_InitWalkRandomSet11(FieldActor *actor) {
    FldmmdlMovecode_InitWalkRandom(actor, 12, 11, 0);
}

void FldmmdlMovecode_InitWalkRandomSet12(FieldActor *actor) {
    FldmmdlMovecode_InitWalkRandom(actor, 12, 12, 0);
}

void FldmmdlMovecode_InitWalkRandomSet13(FieldActor *actor) {
    FldmmdlMovecode_InitWalkRandom(actor, 12, 13, 0);
}

void FldmmdlMovecode_InitWalkRandomSet13Bump(FieldActor *actor) {
    FldmmdlMovecode_InitWalkRandom(actor, 12, 13, 2);
}

static void FldmmdlMovecode_InitWalkRandom(FieldActor *actor, u32 acmdBase, u32 dirMask, u32 mode) {
    RailWalkWork *work = ClearActorPositionBlock(actor, sizeof(RailWalkWork));
    work->mode = mode;
    work->acmdBase = acmdBase;
    work->mask = dirMask;
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
}

void FldmmdlMovecode_MoveWalkRandom(FieldActor *actor) {
    RailWalkWork *work = GetNPCRailPosPtrAddr(actor);
    u32 dir;
    u32 collision;
    u32 acmd;
    switch (work->state) {
    case 0:
        FldAct_ClearMoving(actor);
        FldAct_ClearMoveEnd(actor);
        dir = GetActorFaceDir(actor);
        acmd = GetAcmdForDir(dir, 0);
        FldAct_SetLocalAcmd(actor, acmd);
        work->state++;
        break;
    case 1:
        if (!FieldAcmdTCB_Update(actor)) {
            break;
        }
        work->timer = FldmmdlMovecode_GetRandomTableEntry(data_ov036_021cbb1c, -1);
        work->state++;
    case 2:
        work->timer--;
        if (work->timer != 0) {
            break;
        }
        work->state++;
    case 3:
        dir = FldmmdlMovecode_GetRandomDirFromSet(work->mask, DIR_NONE);
        ChangeActorDirection(actor, dir);
        if (work->mode == 1 && FldmmdlMovecode_IsDirInsideMoveRect(actor, dir) == 0) {
            work->state = 0;
            return;
        }
        collision = ActorRouteCollCheckOneTileInDir(actor, dir);
        if (collision != 0) {
            if (work->mode == 2) {
                if (collision & 1) {
                    work->state = 0;
                    return;
                }
            } else {
                work->state = 0;
                return;
            }
        }
        acmd = GetAcmdForDir(dir, work->acmdBase);
        FldAct_SetLocalAcmd(actor, acmd);
        FldAct_SetMoving(actor);
        work->state++;
    case 4:
        if (FieldAcmdTCB_Update(actor)) {
            FldAct_ClearMoving(actor);
            work->state = 0;
        }
        break;
    default:
        break;
    }
}

static void FldmmdlMovecode_GetMoveRect(FieldActor *actor, WalkRect *rect) {
    s32 z;
    s32 x = GetActorDefaultGPosX(actor);
    s32 w;
    s32 h;
    z = GetActorDefaultGPosZ(actor);
    w = GetActorWalkAreaW(actor);
    h = GetActorWalkAreaH(actor);
    switch (GetActorMoveCode(actor)) {
    case 6:
        rect->minX = x - w;
        rect->maxX = x;
        rect->minZ = z - h;
        rect->maxZ = z;
        break;
    case 7:
        rect->minX = x;
        rect->maxX = x + w;
        rect->minZ = z - h;
        rect->maxZ = z;
        break;
    case 8:
        rect->minX = x - w;
        rect->maxX = x;
        rect->minZ = z;
        rect->maxZ = z + h;
        break;
    case 9:
        rect->maxX = x + w;
        rect->minX = x;
        rect->minZ = z;
        rect->maxZ = z + h;
        break;
    case 10:
        rect->minX = x - w;
        rect->maxX = x;
        rect->minZ = z - h;
        rect->maxZ = z + h;
        break;
    case 11:
        rect->maxX = x + w;
        rect->minX = x;
        rect->minZ = z - h;
        rect->maxZ = z + h;
        break;
    case 12:
        rect->minX = x - w;
        rect->maxX = x + w;
        rect->minZ = z - h;
        rect->maxZ = z;
        break;
    case 13:
        rect->minX = x - w;
        rect->maxX = x + w;
        rect->minZ = z;
        rect->maxZ = z + h;
        break;
    }
}

static BOOL FldmmdlMovecode_IsDirInsideMoveRect(FieldActor *actor, u32 dir) {
    WalkRect rect;
    s32 x;
    s32 z;
    FldmmdlMovecode_GetMoveRect(actor, &rect);
    x = GetGPosX(actor) + GetDirectionVectorCompX(dir);
    z = GetGPosZ(actor) + GetDirectionVectorCompZ(dir);
    if (rect.minX > x || rect.maxX < x) {
        return FALSE;
    }
    if (rect.minZ > z || rect.maxZ < z) {
        return FALSE;
    }
    return TRUE;
}

static void FldmmdlMovecode_InitFixedDir(FieldActor *actor, u32 dir) {
    RailFaceWork *work = ClearActorPositionBlock(actor, sizeof(RailFaceWork));
    work->dir = dir;
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
    SetActorInitialGPosToNowGPos(actor);
}

void FldmmdlMovecode_MoveFixedDir(FieldActor *actor) {
    RailFaceWork *work = GetNPCRailPosPtrAddr(actor);
    switch (work->state) {
    case 0:
        CheckSetActorFaceDir(actor, work->dir);
        work->state++;
        break;
    case 1:
        break;
    }
}

void FldmmdlMovecode_InitFixedDirUp(FieldActor *actor) {
    FldmmdlMovecode_InitFixedDir(actor, DIR_UP);
}

void FldmmdlMovecode_InitFixedDirDown(FieldActor *actor) {
    FldmmdlMovecode_InitFixedDir(actor, DIR_DOWN);
}

void FldmmdlMovecode_InitFixedDirLeft(FieldActor *actor) {
    FldmmdlMovecode_InitFixedDir(actor, DIR_LEFT);
}

void FldmmdlMovecode_InitFixedDirRight(FieldActor *actor) {
    FldmmdlMovecode_InitFixedDir(actor, DIR_RIGHT);
}

static void FldmmdlMovecode_InitSpin(FieldActor *actor, u32 turn) {
    RailTurnWork *work = ClearActorPositionBlock(actor, sizeof(RailTurnWork));
    work->turn = turn;
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
    SetActorInitialGPosToNowGPos(actor);
}

void FldmmdlMovecode_InitSpinClockwise(FieldActor *actor) {
    FldmmdlMovecode_InitSpin(actor, 2);
}

void FldmmdlMovecode_InitSpinCounterClockwise(FieldActor *actor) {
    FldmmdlMovecode_InitSpin(actor, 3);
}

void FldmmdlMovecode_MoveSpin(FieldActor *actor) {
    RailTurnWork *work = GetNPCRailPosPtrAddr(actor);
    while (data_ov036_021cb960[work->state](actor, work) == TRUE) {
    }
}

static BOOL FldmmdlMovecode_SpinStepTurn(FieldActor *actor, RailTurnWork *work) {
    u32 acmd;
    u32 dir = FldmmdlMovecode_GetDirTowardPlayerInSet(actor, 0x26, DIR_NONE);
    if (dir == DIR_NONE) {
        dir = GetActorFaceDir(actor);
    }
    acmd = GetAcmdForDir(dir, 0);
    FldAct_SetLocalAcmd(actor, acmd);
    work->state = 1;
    return TRUE;
}

static BOOL FldmmdlMovecode_SpinStepWaitTurn(FieldActor *actor, RailTurnWork *work) {
    if (!FieldAcmdTCB_Update(actor)) {
        return FALSE;
    }
    if (GetActorEvType(actor) != 0xb) {
        work->count = 0;
    }
    work->state = 2;
    return TRUE;
}

static BOOL FldmmdlMovecode_SpinStepWaitInterval(FieldActor *actor, RailTurnWork *work) {
    if (GetActorEvType(actor) == 0xb) {
        if (work->count != 0) {
            work->count = 0;
            return FALSE;
        }
        if (!FldAct_IsNearPlayer(actor)) {
            return FALSE;
        }
        if (!FldmmdlMovecode_IsBButtonPressed()) {
            return FALSE;
        }
        work->count++;
    } else {
        if (work->count != 0 && FldmmdlMovecode_GetDirTowardPlayerInSet(actor, 0x26, DIR_NONE) != DIR_NONE) {
            work->state = 0;
            return TRUE;
        }
        work->count++;
        if (work->count < 0x18) {
            return FALSE;
        }
    }
    work->state = 3;
    return TRUE;
}

static BOOL FldmmdlMovecode_SpinStepRotateDir(FieldActor *actor, RailTurnWork *work) {
    u32 clockwise[5] = { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT, DIR_NONE };
    u32 counter[5] = { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT, DIR_NONE };
    u32 *dirs = clockwise;
    u32 face;
    s32 i;
    if (work->turn != 2) {
        dirs = counter;
    }
    face = GetActorFaceDir(actor);
    for (i = 0; dirs[i] != DIR_NONE; i++) {
        if (face == dirs[i]) {
            break;
        }
    }
    i++;
    if (dirs[i] == DIR_NONE) {
        i = 0;
    }
    CheckSetActorFaceDir(actor, dirs[i]);
    work->state = 0;
    return TRUE;
}

void FldmmdlMovecode_InitSwing(FieldActor *actor) {
    FldmmdlMovecode_InitSpin(actor, 3);
}

void FldmmdlMovecode_MoveSwing(FieldActor *actor) {
    RailTurnWork *work = GetNPCRailPosPtrAddr(actor);
    while (data_ov036_021cb930[work->state](actor, work) == TRUE) {
    }
}

static BOOL FldmmdlMovecode_SwingStepTurn(FieldActor *actor, RailTurnWork *work) {
    u32 dir = GetActorFaceDir(actor);
    u32 acmd = GetAcmdForDir(dir, 0);
    FldAct_SetLocalAcmd(actor, acmd);
    work->state = 1;
    return TRUE;
}

static BOOL FldmmdlMovecode_SwingStepWaitTurn(FieldActor *actor, RailTurnWork *work) {
    if (!FieldAcmdTCB_Update(actor)) {
        return FALSE;
    }
    work->count = 0;
    work->state = 2;
    return TRUE;
}

static BOOL FldmmdlMovecode_SwingStepWaitInterval(FieldActor *actor, RailTurnWork *work) {
    work->count++;
    if (work->count < 0x18) {
        return FALSE;
    }
    work->state = 3;
    return TRUE;
}

static BOOL FldmmdlMovecode_SwingStepRotateDir(FieldActor *actor, RailTurnWork *work) {
    u32 clockwise[5] = { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT, DIR_NONE };
    u32 counter[5] = { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT, DIR_NONE };
    u32 *dirs = clockwise;
    u32 face;
    s32 i;
    if (work->turn != 2) {
        dirs = counter;
    }
    face = GetActorFaceDir(actor);
    for (i = 0; dirs[i] != DIR_NONE; i++) {
        if (face == dirs[i]) {
            break;
        }
    }
    i++;
    if (dirs[i] == DIR_NONE) {
        i = 0;
    }
    CheckSetActorFaceDir(actor, dirs[i]);
    if (GetActorFaceDir(actor) == GetDefaultActorDir(actor)) {
        work->turn = GetInverseDirection(work->turn);
    }
    work->state = 0;
    return TRUE;
}

static BOOL (*const data_ov036_021cb8d8[])(FieldActor *actor, RailPaceWork *work) = {
    FldmmdlMovecode_BackForthStepFaceDir,
    FldmmdlMovecode_BackForthStepWalk,
    FldmmdlMovecode_BackForthStepWaitWalk,
};

static const u32 data_ov036_021cbb64[] = {
    0x2, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0x2d, 0x2e, 0x12, 0x13, 0xff,
};

static const ActorDirTableEntry data_ov036_021cbc70[] = {
    { 0, data_ov036_021cbb30 },  { 1, data_ov036_021cb884 },  { 2, data_ov036_021cb8a8 },  { 3, data_ov036_021cb890 },
    { 4, data_ov036_021cb8b4 },  { 5, data_ov036_021cba70 },  { 6, data_ov036_021cba30 },  { 7, data_ov036_021cba40 },
    { 8, data_ov036_021cba60 },  { 9, data_ov036_021cb8e4 },  { 10, data_ov036_021cb8cc }, { 11, data_ov036_021cbae0 },
    { 12, data_ov036_021cb8c0 }, { 13, data_ov036_021cb89c }, { 14, data_ov036_021cb9c0 }, { 15, data_ov036_021cb9d0 },
    { 16, data_ov036_021cb9e0 }, { 17, data_ov036_021cbab0 }, { 18, data_ov036_021cb9a0 }, { 19, data_ov036_021cb940 },
    { 20, data_ov036_021cb950 }, { 21, data_ov036_021cbac0 }, { 22, data_ov036_021cb9f0 }, { 23, data_ov036_021cba00 },
    { 24, data_ov036_021cba50 }, { 25, data_ov036_021cba80 }, { 26, data_ov036_021cba10 }, { 27, data_ov036_021cb910 },
    { 28, data_ov036_021cb900 }, { 29, data_ov036_021cb9b0 }, { 30, data_ov036_021cb990 }, { 31, data_ov036_021cbad0 },
    { 32, data_ov036_021cba20 }, { 33, data_ov036_021cba90 }, { 34, data_ov036_021cb920 }, { 35, data_ov036_021cb970 },
    { 36, data_ov036_021cb980 }, { 37, data_ov036_021cbaa0 }, { 38, data_ov036_021cbaf4 }, { 0x27, NULL },
};

void FldmmdlMovecode_InitBackForth(FieldActor *actor) {
    RailPaceWork *work = ClearActorPositionBlock(actor, sizeof(RailPaceWork));
    if (FldmmdlMovecode_IsMirrorEventType(actor) == TRUE) {
        FldmmdlMovecode_InitMirrorSide(actor, &work->face);
    }
}

void FldmmdlMovecode_MoveBackForth(FieldActor *actor) {
    RailPaceWork *work = GetNPCRailPosPtrAddr(actor);
    while (data_ov036_021cb8d8[work->state](actor, work) == TRUE) {
    }
}

static BOOL FldmmdlMovecode_BackForthStepFaceDir(FieldActor *actor, RailPaceWork *work) {
    u32 dir = GetDefaultActorDir(actor);
    if (work->flip == 1) {
        dir = GetInverseDirection((u16)dir);
    }
    SetActorMotionDir(actor, dir);
    if (FldmmdlMovecode_IsMirrorEventType(actor) == 0) {
        CheckSetActorFaceDir(actor, dir);
    }
    work->state = 1;
    return TRUE;
}

static BOOL FldmmdlMovecode_BackForthStepWalk(FieldActor *actor, RailPaceWork *work) {
    u32 collision;
    u32 dir;
    u32 acmdBase;
    if (FldmmdlMovecode_UpdateTriggerWait(actor, &work->wait) == 0) {
        return FALSE;
    }
    if (work->flip != 0) {
        s32 defaultX = GetActorDefaultGPosX(actor);
        s32 defaultZ = GetActorDefaultGPosZ(actor);
        s32 x = GetGPosX(actor);
        s32 z = GetGPosZ(actor);
        if (defaultX == x && defaultZ == z) {
            dir = GetInverseDirection(GetActorMotionDir(actor));
            SetActorMotionDir(actor, dir);
            if (FldmmdlMovecode_IsMirrorEventType(actor) == 0) {
                CheckSetActorFaceDir(actor, dir);
            }
            work->flip = 0;
        }
    }
    dir = GetActorMotionDir(actor);
    collision = ActorRouteCollCheckOneTileInDir(actor, dir);
    if (collision & 1) {
        work->flip = 1;
        dir = GetInverseDirection((u16)dir);
        collision = ActorRouteCollCheckOneTileInDir(actor, dir);
    }
    acmdBase = 12;
    if (FldmmdlMovecode_IsDashEventType(actor) != 0) {
        if (FldmmdlMovecode_IsBButtonHeld() == TRUE && FldAct_IsNearPlayer(actor) == TRUE) {
            acmdBase = 16;
        }
    }
    if (collision != 0) {
        acmdBase = 32;
    }
    u32 acmd = GetAcmdForDir(dir, acmdBase);
    FldAct_SetLocalAcmd(actor, acmd);
    if (FldmmdlMovecode_IsMirrorEventType(actor) == TRUE) {
        FldmmdlMovecode_StartMirrorTurn(actor, &work->face);
    }
    FldAct_SetMoving(actor);
    work->state = 2;
    return TRUE;
}

static BOOL FldmmdlMovecode_BackForthStepWaitWalk(FieldActor *actor, RailPaceWork *work) {
    if (FieldAcmdTCB_Update(actor) == TRUE) {
        FldAct_ClearMoving(actor);
        if (FldmmdlMovecode_IsMirrorEventType(actor) == TRUE) {
            FldmmdlMovecode_EndMirrorTurn(actor, &work->face);
        }
        work->state = 0;
    }
    return FALSE;
}

static void FldmmdlMovecode_InitRoutePatrol(FieldActor *actor, u32 steps, u32 turns, u32 speed) {
    RailStepWork *work = ClearActorPositionBlock(actor, sizeof(RailStepWork));
    work->steps = steps;
    work->turns = turns;
    work->speed = speed;
    if (FldmmdlMovecode_IsMirrorEventType(actor) == TRUE) {
        FldmmdlMovecode_InitMirrorSide(actor, &work->face);
    }
}

void FldmmdlMovecode_InitRoutePatrolSet14(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 0, 14);
}

void FldmmdlMovecode_InitRoutePatrolSet15(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 0, 15);
}

void FldmmdlMovecode_InitRoutePatrolSet16(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 1, 16);
}

void FldmmdlMovecode_InitRoutePatrolSet17(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 1, 17);
}

void FldmmdlMovecode_InitRoutePatrolSet18(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 0, 18);
}

void FldmmdlMovecode_InitRoutePatrolSet19(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 0, 19);
}

void FldmmdlMovecode_InitRoutePatrolSet20(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 1, 20);
}

void FldmmdlMovecode_InitRoutePatrolSet21(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 1, 21);
}

void FldmmdlMovecode_InitRoutePatrolSet22(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 1, 22);
}

void FldmmdlMovecode_InitRoutePatrolSet23(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 1, 23);
}

void FldmmdlMovecode_InitRoutePatrolSet24(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 0, 24);
}

void FldmmdlMovecode_InitRoutePatrolSet25(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 0, 25);
}

void FldmmdlMovecode_InitRoutePatrolSet26(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 1, 26);
}

void FldmmdlMovecode_InitRoutePatrolSet27(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 1, 27);
}

void FldmmdlMovecode_InitRoutePatrolSet28(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 0, 28);
}

void FldmmdlMovecode_InitRoutePatrolSet29(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrol(actor, 2, 0, 29);
}

void FldmmdlMovecode_MoveRoutePatrol(FieldActor *actor) {
    ActorRoundWork *work = GetNPCRailPosPtrAddr(actor);

    while (data_ov036_021cb87c[work->state](actor, work) == TRUE) {
    }
}

static BOOL FldmmdlMovecode_RoutePatrolStepWalk(FieldActor *actor, ActorRoundWork *work) {
    s32 blocked;
    const s32 *dirs;
    s32 dir;

    if (!FldmmdlMovecode_UpdateTriggerWait(actor, &work->wait)) {
        return FALSE;
    }
    if (work->dirIdx == work->limit) {
        if (work->axis == 0) {
            if (GetActorDefaultGPosX(actor) == GetGPosX(actor)) {
                work->dirIdx++;
            }
        } else if (GetActorDefaultGPosZ(actor) == GetGPosZ(actor)) {
            work->dirIdx++;
        }
    }
    if (work->dirIdx == 3) {
        s16 defaultX = GetActorDefaultGPosX(actor);
        s16 defaultZ = GetActorDefaultGPosZ(actor);
        s16 x = GetGPosX(actor);
        s16 z = GetGPosZ(actor);

        if (defaultX == x && defaultZ == z) {
            work->dirIdx = 0;
        }
    }
    dirs = FldmmdlMovecode_GetDirSet(work->dirTable);
    dir = dirs[work->dirIdx];
    SetActorMotionDir(actor, dir);
    if (FldmmdlMovecode_IsMirrorEventType(actor) == FALSE) {
        CheckSetActorFaceDir(actor, dir);
    }
    blocked = ActorRouteCollCheckOneTileInDir(actor, dir);
    if (blocked & 1) {
        work->dirIdx++;
        dir = dirs[work->dirIdx];
        SetActorMotionDir(actor, dir);
        if (FldmmdlMovecode_IsMirrorEventType(actor) == FALSE) {
            CheckSetActorFaceDir(actor, dir);
        }
        blocked = ActorRouteCollCheckOneTileInDir(actor, dir);
    }
    u32 speed = 12;

    if (FldmmdlMovecode_IsDashEventType(actor)) {
        if (FldmmdlMovecode_IsBButtonHeld() == TRUE && FldAct_IsNearPlayer(actor) == TRUE) {
            speed = 16;
        }
    }
    if (blocked) {
        speed = 32;
    }
    StartDirAcmd(actor, dir, speed);
    if (FldmmdlMovecode_IsMirrorEventType(actor) == TRUE) {
        FldmmdlMovecode_StartMirrorTurn(actor, &work->face);
    }
    FldAct_SetMoving(actor);
    work->state = 1;
    return TRUE;
}

static BOOL FldmmdlMovecode_RoutePatrolStepWaitWalk(FieldActor *actor, ActorRoundWork *work) {
    if (FieldAcmdTCB_Update(actor) == TRUE) {
        FldAct_ClearMoving(actor);
        if (FldmmdlMovecode_IsMirrorEventType(actor) == TRUE) {
            FldmmdlMovecode_EndMirrorTurn(actor, &work->face);
        }
        work->state = 0;
    }
    return FALSE;
}

static void FldmmdlMovecode_InitRoutePatrolSwap(FieldActor *actor, u32 limit, u32 axis, u32 dirTable) {
    ActorRoundWork *work = ClearActorPositionBlock(actor, 12);

    work->limit = limit;
    work->axis = axis;
    work->dirTable = dirTable;
    if (FldmmdlMovecode_IsMirrorEventType(actor) == TRUE) {
        FldmmdlMovecode_InitMirrorSide(actor, &work->face);
    }
}

void FldmmdlMovecode_InitRoutePatrolSwapSet30(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrolSwap(actor, 2, 1, 0x1e);
}

void FldmmdlMovecode_InitRoutePatrolSwapSet31(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrolSwap(actor, 2, 1, 0x1f);
}

void FldmmdlMovecode_InitRoutePatrolSwapSet32(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrolSwap(actor, 2, 0, 0x20);
}

void FldmmdlMovecode_InitRoutePatrolSwapSet33(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrolSwap(actor, 2, 0, 0x21);
}

void FldmmdlMovecode_InitRoutePatrolSwapSet34(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrolSwap(actor, 2, 1, 0x22);
}

void FldmmdlMovecode_InitRoutePatrolSwapSet35(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrolSwap(actor, 2, 1, 0x23);
}

void FldmmdlMovecode_InitRoutePatrolSwapSet36(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrolSwap(actor, 2, 0, 0x24);
}

void FldmmdlMovecode_InitRoutePatrolSwapSet37(FieldActor *actor) {
    FldmmdlMovecode_InitRoutePatrolSwap(actor, 2, 0, 0x25);
}

void FldmmdlMovecode_MoveRoutePatrolSwap(FieldActor *actor) {
    ActorPatrolWork *work = GetNPCRailPosPtrAddr(actor);

    while (data_ov036_021cb8f0[work->state](actor, work) == TRUE) {
    }
}

static void FldmmdlMovecode_AdvanceRouteIndex(ActorPatrolWork *work) {
    if (work->reverse == TRUE) {
        work->dirIdx--;
        if (work->dirIdx < 0) {
            work->dirIdx = 3;
        }
    } else {
        work->dirIdx++;
    }
}

static BOOL FldmmdlMovecode_RoutePatrolSwapStepLookForPlayer(FieldActor *actor, ActorPatrolWork *work) {
    if (GetActorEvType(actor) == 10) {
        u16 dir = GetActorFaceDir(actor);

        if (EventTrainerEyeSub_GetPlayerDistance(actor, dir, GetActorUserParam(actor, 0), NULL) != -1) {
            FldAct_SetLocalAcmd(actor, GetAcmdForDir(dir, 0x30));
            FldAct_SetMoving(actor);
            work->state = 1;
            return TRUE;
        }
    }
    work->state = 2;
    return TRUE;
}

static BOOL FldmmdlMovecode_RoutePatrolSwapStepSwapMoveCode(FieldActor *actor, ActorPatrolWork *work) {
    if (FieldAcmdTCB_Update(actor) == TRUE) {
        u16 other = 0xff;
        u16 moveCode = GetActorMoveCode(actor);
        const ActorMoveCodePair *pair;

        for (pair = data_ov036_021cbb08; pair->first != 0xff; pair++) {
            if (pair->first == moveCode) {
                other = pair->second;
            } else if (pair->second == moveCode) {
                other = pair->first;
            }
            if (other != 0xff) {
                s32 dirIdx = 4 - (work->dirIdx + 1);

                ChangeActorMoveCodeSeq(actor, other);
                work->dirIdx = dirIdx;
                break;
            }
        }
        FldAct_ClearMoving(actor);
        work->state = 2;
    }
    return FALSE;
}

static BOOL FldmmdlMovecode_RoutePatrolSwapStepWalk(FieldActor *actor, ActorPatrolWork *work) {
    s32 blocked;
    const s32 *dirs;
    s32 dir;

    if (!FldmmdlMovecode_UpdateTriggerWait(actor, &work->wait)) {
        return FALSE;
    }
    if (work->dirIdx == work->limit) {
        if (work->axis == 0) {
            if (GetActorDefaultGPosX(actor) == GetGPosX(actor)) {
                FldmmdlMovecode_AdvanceRouteIndex(work);
            }
        } else if (GetActorDefaultGPosZ(actor) == GetGPosZ(actor)) {
            FldmmdlMovecode_AdvanceRouteIndex(work);
        }
    }
    if (work->dirIdx == 3) {
        s16 defaultX = GetActorDefaultGPosX(actor);
        s16 defaultZ = GetActorDefaultGPosZ(actor);
        s16 x = GetGPosX(actor);
        s16 z = GetGPosZ(actor);

        if (defaultX == x && defaultZ == z) {
            work->dirIdx = 0;
        }
    }
    dirs = FldmmdlMovecode_GetDirSet(work->dirTable);
    dir = dirs[work->dirIdx];
    SetActorMotionDir(actor, dir);
    if (FldmmdlMovecode_IsMirrorEventType(actor) == FALSE) {
        CheckSetActorFaceDir(actor, dir);
    }
    blocked = ActorRouteCollCheckOneTileInDir(actor, dir);
    if (blocked & 1) {
        FldmmdlMovecode_AdvanceRouteIndex(work);
        dir = dirs[work->dirIdx];
        SetActorMotionDir(actor, dir);
        if (FldmmdlMovecode_IsMirrorEventType(actor) == FALSE) {
            CheckSetActorFaceDir(actor, dir);
        }
        blocked = ActorRouteCollCheckOneTileInDir(actor, dir);
    }
    u32 speed = 12;

    if (FldmmdlMovecode_IsDashEventType(actor)) {
        if (FldmmdlMovecode_IsBButtonHeld() == TRUE && FldAct_IsNearPlayer(actor) == TRUE) {
            speed = 16;
        }
    }
    if (blocked) {
        speed = 32;
    }
    StartDirAcmd(actor, dir, speed);
    if (FldmmdlMovecode_IsMirrorEventType(actor) == TRUE) {
        FldmmdlMovecode_StartMirrorTurn(actor, &work->face);
    }
    FldAct_SetMoving(actor);
    work->state = 3;
    return TRUE;
}

static BOOL FldmmdlMovecode_RoutePatrolSwapStepWaitWalk(FieldActor *actor, ActorPatrolWork *work) {
    if (FieldAcmdTCB_Update(actor) == TRUE) {
        FldAct_ClearMoving(actor);
        if (FldmmdlMovecode_IsMirrorEventType(actor) == TRUE) {
            FldmmdlMovecode_EndMirrorTurn(actor, &work->face);
        }
        work->state = 0;
    }
    return FALSE;
}

static BOOL FldmmdlMovecode_IsBButtonHeld(void) {
    if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_B) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FldmmdlMovecode_IsPlayerWalking(FieldActor *actor) {
    FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));

    if (player != NULL) {
        switch (FldAct_GetAcmd(player)) {
        case 0x58:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0xa3:
        case 0xa4:
        case 0xa5:
        case 0xa6:
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FldmmdlMovecode_IsBButtonPressed(void) {
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        return TRUE;
    }
    return FALSE;
}

static u32 FldmmdlMovecode_FindTableEnd(const s32 *list, u32 end) {
    u32 count = 0;

    while (end != list[count]) {
        count++;
    }
    return count;
}

static s32 FldmmdlMovecode_GetRandomTableEntry(const s32 *list, u32 end) {
    return list[GFL_RandomLC(FldmmdlMovecode_FindTableEnd(list, end))];
}

static s32 FldmmdlMovecode_GetRandomDirFromSet(u32 index, u32 end) {
    const s32 *list = FldmmdlMovecode_GetDirSet(index);

    return list[GFL_RandomLC(FldmmdlMovecode_FindTableEnd(list, end))];
}

static const s32 *FldmmdlMovecode_GetDirSet(u32 index) {
    const ActorDirTableEntry *entry = data_ov036_021cbc70;

    for (; entry->key != 0x27; entry++) {
        if (entry->key == index) {
            return entry->dirs;
        }
    }
    return NULL;
}

static u32 FldmmdlMovecode_GetSightDirToPlayer(FieldActor *actor) {
    FieldActor *player;
    s32 range;
    s32 playerX;
    s32 playerZ;
    s32 x;
    s32 z;
    s32 zMax;
    s32 xMin;
    s32 zMin;
    s32 xMax;
    u16 evType = GetActorEvType(actor);
    u32 moveCode;
    u32 entry;
    u32 i;

    if (evType != 1 && evType != 2) {
        return DIR_NONE;
    }
    if (!FldmmdlMovecode_IsPlayerWalking(actor)) {
        return DIR_NONE;
    }
    i = 0;
    moveCode = GetActorMoveCode(actor);
    do {
        entry = data_ov036_021cbb64[i];
        i++;
    } while (entry != moveCode && entry != 0xff);
    if (moveCode != entry) {
        return DIR_NONE;
    }
    player = FindPlayerFieldActor(GetActorMModelSystem(actor));
    if (player == NULL) {
        return DIR_NONE;
    }
    if (FldAct_GetGPosY(player) != FldAct_GetGPosY(actor)) {
        return DIR_NONE;
    }
    playerX = GetGPosX(player);
    playerZ = GetGPosZ(player);
    range = GetActorUserParam(actor, 0);
    x = GetGPosX(actor);
    z = GetGPosZ(actor);
    xMin = x - range;
    xMax = x + range;
    zMin = z - range;
    zMax = z + range;
    if (zMin <= playerZ && zMax >= playerZ && xMin <= playerX && xMax >= playerX) {
        return GetDirFromPosToPos(x, z, playerX, playerZ);
    }
    return DIR_NONE;
}

static u32 FldmmdlMovecode_GetDirTowardPlayerInSet(FieldActor *actor, u32 index, u32 end) {
    const s32 *dirs = FldmmdlMovecode_GetDirSet(index);
    s32 count = FldmmdlMovecode_FindTableEnd(dirs, end);
    FieldActor *player;
    s32 actorX;
    s32 actorZ;
    s32 playerX;
    s32 playerZ;
    s32 dirX;
    s32 dirZ;
    s32 dir;
    s32 i;

    if (count == 1) {
        return DIR_NONE;
    }
    dir = FldmmdlMovecode_GetSightDirToPlayer(actor);
    if (dir == DIR_NONE) {
        return dir;
    }
    i = 0;
    do {
        if (dir == dirs[i]) {
            return dir;
        }
        i++;
    } while (i < count);
    player = FindPlayerFieldActor(GetActorMModelSystem(actor));
    dirX = DIR_NONE;
    dirZ = DIR_NONE;
    actorX = GetGPosX(actor);
    actorZ = GetGPosZ(actor);
    playerX = GetGPosX(player);
    playerZ = GetGPosZ(player);
    if (actorX > playerX) {
        dirX = DIR_LEFT;
    } else if (actorX < playerX) {
        dirX = DIR_RIGHT;
    }
    if (actorZ > playerZ) {
        dirZ = DIR_UP;
    } else if (actorZ < playerZ) {
        dirZ = DIR_DOWN;
    }
    i = 0;
    if (dirX == DIR_NONE) {
        do {
            if (dirZ == dirs[i]) {
                return dirZ;
            }
            i++;
        } while (i < count);
    } else if (dirZ == DIR_NONE) {
        do {
            if (dirX == dirs[i]) {
                return dirX;
            }
            i++;
        } while (i < count);
    } else {
        do {
            if (dirX == dirs[i]) {
                return dirX;
            }
            if (dirZ == dirs[i]) {
                return dirZ;
            }
            i++;
        } while (i < count);
    }
    return DIR_NONE;
}

static BOOL FldmmdlMovecode_UpdateTriggerWait(FieldActor *actor, u8 *wait) {
    if (GetActorEvType(actor) != 12) {
        return TRUE;
    }
    if (*wait == 0) {
        if (!FldAct_IsNearPlayer(actor)) {
            return FALSE;
        }
        if (!FldmmdlMovecode_IsBButtonPressed()) {
            return FALSE;
        }
        *wait = GetActorUserParam(actor, 1);
    }
    if (*wait != 0) {
        (*wait)--;
    }
    return TRUE;
}

static BOOL FldmmdlMovecode_IsDashEventType(FieldActor *actor) {
    u16 evType = GetActorEvType(actor);

    if (evType == 13 || evType == 15 || evType == 16 || evType == 17 || evType == 18 || evType == 19) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FldmmdlMovecode_IsMirrorEventType(FieldActor *actor) {
    int evType = GetActorEvType(actor);

    if (evType == 7 || evType == 8 || evType == 17 || evType == 18) {
        return TRUE;
    }
    return FALSE;
}

static void FldmmdlMovecode_InitMirrorSide(FieldActor *actor, ActorFaceMemory *face) {
    u16 evType = GetActorEvType(actor);

    if (evType == 7 || evType == 17) {
        face->row = 0;
    } else {
        face->row = 1;
    }
}

static void FldmmdlMovecode_StartMirrorTurn(FieldActor *actor, ActorFaceMemory *face) {
    u16 dir = GetActorFaceDir(actor);
    s32 i;
    s32 nextDir;

    i = 0;
    while (i < 4 && dir != data_ov036_021cbb44[face->row][i]) {
        i++;
    }
    i++;
    face->dir = dir;
    i %= 4;
    nextDir = data_ov036_021cbb44[face->row][i];
    if (CheckActorFlag(actor, 8)) {
        face->hadFlag8 = 1;
    } else {
        face->hadFlag8 = 0;
    }
    CheckSetActorFaceDir(actor, nextDir);
    SetActorFlag(actor, 8);
}

static void FldmmdlMovecode_EndMirrorTurn(FieldActor *actor, ActorFaceMemory *face) {
    if (face->hadFlag8 == 0) {
        ClearActorFlag(actor, 8);
    }
}
