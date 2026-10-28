// The move codes by which actors look around, wander in a rectangle or by a mask of directions, turn, pace back and
// forth and walk around a square: the functions that field_actor_movedata.c's tables list for each, and the steps they
// run. The ROM gives this file no name, so field_actor_movecode.c is a guess. It follows field_actor_acmd.c, and its
// .rodata, sorted by size on its own, starts at 0x021cb87c (Black 2). The code was first written by mariano54 on
// their parallel-decomp branch

#include "types.h"
#include "constants/directions.h"
#include "field/event_trainer_eye.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/field_actor_movecode.h"
#include "gfl/key.h"
#include "gfl/random.h"

// Ends the tables of move codes, and stands for no move code where one is looked for in them
#define MOVE_CODE_LIST_END 0xff

// The facing a walking code keeps for actors that turn and then restore it: the facing before, a row of
// sMirrorTurnOrders and whether movement flag 8 was set
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

// The modes of the random walk: walk wherever nothing is in the way, only inside the move code's rectangle (which no
// move code uses), or into every collision but the first kind
enum {
    WALK_RANDOM_MODE_ANY,
    WALK_RANDOM_MODE_IN_RECT,
    WALK_RANDOM_MODE_BUMP,
};

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

// The work of an actor that turns around a rotation of directions: turn is the way it turns, DIR_LEFT or DIR_RIGHT,
// state picks the function of the table that runs
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

static void FieldActorMovecode_InitTurnRandom(FieldActor *actor, u32 mask);
static u16 FieldActorMovecode_GetNextDirInRotation(u16 moveCode, u16 faceDir);
static void FieldActorMovecode_InitWalkRandom(FieldActor *actor, u32 acmdBase, u32 dirMask, u32 mode);
static void FieldActorMovecode_GetMoveRect(FieldActor *actor, WalkRect *rect);
static BOOL FieldActorMovecode_IsDirInsideMoveRect(FieldActor *actor, u32 dir);
static void FieldActorMovecode_InitFixedDir(FieldActor *actor, u32 dir);
static void FieldActorMovecode_InitSpin(FieldActor *actor, u32 turn);
static BOOL FieldActorMovecode_SpinStepTurn(FieldActor *actor, RailTurnWork *work);
static BOOL FieldActorMovecode_SpinStepWaitTurn(FieldActor *actor, RailTurnWork *work);
static BOOL FieldActorMovecode_SpinStepWaitInterval(FieldActor *actor, RailTurnWork *work);
static BOOL FieldActorMovecode_SpinStepRotateDir(FieldActor *actor, RailTurnWork *work);
static BOOL FieldActorMovecode_SwingStepTurn(FieldActor *actor, RailTurnWork *work);
static BOOL FieldActorMovecode_SwingStepWaitTurn(FieldActor *actor, RailTurnWork *work);
static BOOL FieldActorMovecode_SwingStepWaitInterval(FieldActor *actor, RailTurnWork *work);
static BOOL FieldActorMovecode_SwingStepRotateDir(FieldActor *actor, RailTurnWork *work);
static BOOL FieldActorMovecode_BackForthStepFaceDir(FieldActor *actor, RailPaceWork *work);
static BOOL FieldActorMovecode_BackForthStepWalk(FieldActor *actor, RailPaceWork *work);
static BOOL FieldActorMovecode_BackForthStepWaitWalk(FieldActor *actor, RailPaceWork *work);
static void FieldActorMovecode_InitRoutePatrol(FieldActor *actor, u32 steps, u32 turns, u32 speed);
static BOOL FieldActorMovecode_RoutePatrolStepWalk(FieldActor *actor, ActorRoundWork *work);
static BOOL FieldActorMovecode_RoutePatrolStepWaitWalk(FieldActor *actor, ActorRoundWork *work);
static void FieldActorMovecode_InitRoutePatrolSwap(FieldActor *actor, u32 limit, u32 axis, u32 dirTable);
static void FieldActorMovecode_AdvanceRouteIndex(ActorPatrolWork *work);
static BOOL FieldActorMovecode_RoutePatrolSwapStepLookForPlayer(FieldActor *actor, ActorPatrolWork *work);
static BOOL FieldActorMovecode_RoutePatrolSwapStepSwapMoveCode(FieldActor *actor, ActorPatrolWork *work);
static BOOL FieldActorMovecode_RoutePatrolSwapStepWalk(FieldActor *actor, ActorPatrolWork *work);
static BOOL FieldActorMovecode_RoutePatrolSwapStepWaitWalk(FieldActor *actor, ActorPatrolWork *work);
static BOOL FieldActorMovecode_IsBButtonHeld(void);
static BOOL FieldActorMovecode_IsPlayerWalking(FieldActor *actor);
static BOOL FieldActorMovecode_IsBButtonPressed(void);
static u32 FieldActorMovecode_FindTableEnd(const s32 *list, u32 end);
static s32 FieldActorMovecode_GetRandomTableEntry(const s32 *list, u32 end);
static s32 FieldActorMovecode_GetRandomDirFromSet(u32 index, u32 end);
static const s32 *FieldActorMovecode_GetDirSet(u32 index);
static u32 FieldActorMovecode_GetSightDirToPlayer(FieldActor *actor);
static u32 FieldActorMovecode_GetDirTowardPlayerInSet(FieldActor *actor, u32 index, u32 end);
static BOOL FieldActorMovecode_UpdateTriggerWait(FieldActor *actor, u8 *wait);
static BOOL FieldActorMovecode_IsDashEventType(FieldActor *actor);
static BOOL FieldActorMovecode_IsMirrorEventType(FieldActor *actor);
static void FieldActorMovecode_InitMirrorSide(FieldActor *actor, ActorFaceMemory *face);
static void FieldActorMovecode_StartMirrorTurn(FieldActor *actor, ActorFaceMemory *face);
static void FieldActorMovecode_EndMirrorTurn(FieldActor *actor, ActorFaceMemory *face);

// The tables are declared in this order because MWCC lays out objects of the same size by where each was declared:
// it is one that gives the ROM's .rodata (found with tools/decomp/rodata_order.py's model), not the original's, which
// is unknown. sBackForthSteps, sPlayerSightMoveCodes and sDirSets follow
// FieldActorMovecode_SwingStepRotateDir for the same reason
static BOOL (*const sRoutePatrolSteps[])(FieldActor *actor,
                                         ActorRoundWork *work) = { FieldActorMovecode_RoutePatrolStepWalk,
                                                                   FieldActorMovecode_RoutePatrolStepWaitWalk };

static const s32 sDirSet22[] = { DIR_LEFT, DIR_UP, DIR_DOWN, DIR_RIGHT };

static const s32 sDirSet9[] = { DIR_UP, DIR_DOWN, DIR_NONE };

static const s32 sDirSet4[] = { DIR_DOWN, DIR_RIGHT, DIR_NONE };

static const s32 sDirSet1[] = { DIR_UP, DIR_LEFT, DIR_NONE };

static const s32 sDirSet10[] = { DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 sDirSet3[] = { DIR_DOWN, DIR_LEFT, DIR_NONE };

static const s32 sDirSet14[] = { DIR_UP, DIR_RIGHT, DIR_LEFT, DIR_DOWN };

static const s32 sDirSet2[] = { DIR_UP, DIR_RIGHT, DIR_NONE };

static const s32 sDirSet13[] = { DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 sDirSet30[] = { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT };

static const s32 sDirSet20[] = { DIR_DOWN, DIR_UP, DIR_LEFT, DIR_RIGHT };

static const s32 sDirSet12[] = { DIR_UP, DIR_DOWN, DIR_NONE };

static const s32 sDirSet36[] = { DIR_LEFT, DIR_UP, DIR_RIGHT, DIR_DOWN };

static const s32 sDirSet29[] = { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_UP };

static const s32 sTurnWaitTimes[] = { 0x10, 0x20, 0x30, 0x40, -1 };

static const s32 sDirSet33[] = { DIR_RIGHT, DIR_UP, DIR_LEFT, DIR_DOWN };

static const s32 sDirSet25[] = { DIR_DOWN, DIR_RIGHT, DIR_LEFT, DIR_UP };

static const s32 sDirSet23[] = { DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_LEFT };

static const s32 sDirSet7[] = { DIR_UP, DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 sDirSet24[] = { DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN };

static const s32 sDirSet5[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_NONE };

static const s32 sDirSet17[] = { DIR_LEFT, DIR_DOWN, DIR_UP, DIR_RIGHT };

static const s32 sDirSet32[] = { DIR_LEFT, DIR_DOWN, DIR_RIGHT, DIR_UP };

static const s32 sDirSet26[] = { DIR_RIGHT, DIR_UP, DIR_DOWN, DIR_LEFT };

static const s32 sDirSet27[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

static BOOL (*const sRoutePatrolSwapSteps[])(FieldActor *actor, ActorPatrolWork *work) = {
    FieldActorMovecode_RoutePatrolSwapStepLookForPlayer, FieldActorMovecode_RoutePatrolSwapStepSwapMoveCode,
    FieldActorMovecode_RoutePatrolSwapStepWalk, FieldActorMovecode_RoutePatrolSwapStepWaitWalk
};

static const s32 sDirSet16[] = { DIR_DOWN, DIR_UP, DIR_RIGHT, DIR_LEFT };

static const s32 sDirSet15[] = { DIR_RIGHT, DIR_LEFT, DIR_DOWN, DIR_UP };

static const s32 sMirrorTurnOrders[][4] = { { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT },
                                            { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT } };

static const s32 sDirSet19[] = { DIR_LEFT, DIR_RIGHT, DIR_DOWN, DIR_UP };

static const s32 sDirSet18[] = { DIR_LEFT, DIR_RIGHT, DIR_DOWN, DIR_UP };

static BOOL (*const sSpinSteps[])(FieldActor *actor, RailTurnWork *work) = {
    FieldActorMovecode_SpinStepTurn,
    FieldActorMovecode_SpinStepWaitTurn,
    FieldActorMovecode_SpinStepWaitInterval,
    FieldActorMovecode_SpinStepRotateDir,
};

static const s32 sDirSet35[] = { DIR_DOWN, DIR_LEFT, DIR_UP, DIR_RIGHT };

static const s32 sDirSet38[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 sDirSet37[] = { DIR_RIGHT, DIR_DOWN, DIR_LEFT, DIR_UP };

static const s32 sDirSet6[] = { DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_NONE };

static const s32 sDirSet8[] = { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 sDirSet21[] = { DIR_RIGHT, DIR_DOWN, DIR_UP, DIR_LEFT };

static const s32 sDirSet34[] = { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT };

static const s32 sDirSet11[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 sDirSet28[] = { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN };

static BOOL (*const sSwingSteps[])(FieldActor *actor, RailTurnWork *work) = {
    FieldActorMovecode_SwingStepTurn,
    FieldActorMovecode_SwingStepWaitTurn,
    FieldActorMovecode_SwingStepWaitInterval,
    FieldActorMovecode_SwingStepRotateDir,
};

// The directions that each move code turns between, in turn, after the move code and ended by DIR_NONE, for the event
// type that turns when the player presses B; the first two give the random walks' codes with their directions.
// MOVE_CODE_LIST_END ends the table
static const u16 sMoveCodeTurnDirs[][6] = {
    { MOVE_CODE_WALK_RANDOM_UP_DOWN, DIR_UP, DIR_DOWN, DIR_NONE },
    { MOVE_CODE_WALK_RANDOM_LEFT_RIGHT, DIR_LEFT, DIR_RIGHT, DIR_NONE },
    { MOVE_CODE_TURN_RANDOM_UP_LEFT, DIR_UP, DIR_LEFT, DIR_NONE },
    { MOVE_CODE_TURN_RANDOM_UP_RIGHT, DIR_UP, DIR_RIGHT, DIR_NONE },
    { MOVE_CODE_TURN_RANDOM_DOWN_LEFT, DIR_DOWN, DIR_LEFT, DIR_NONE },
    { MOVE_CODE_TURN_RANDOM_DOWN_RIGHT, DIR_DOWN, DIR_RIGHT, DIR_NONE },
    { MOVE_CODE_TURN_RANDOM_UP_DOWN_LEFT, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_NONE },
    { MOVE_CODE_TURN_RANDOM_UP_DOWN_RIGHT, DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_NONE },
    { MOVE_CODE_TURN_RANDOM_UP_LEFT_RIGHT, DIR_UP, DIR_RIGHT, DIR_LEFT, DIR_NONE },
    { MOVE_CODE_TURN_RANDOM_DOWN_LEFT_RIGHT, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE },
    { MOVE_CODE_LIST_END },
};

static const ActorMoveCodePair sPatrolSwapMoveCodes[] = {
    { MOVE_CODE_LOOP_UP_LEFT_DOWN_RIGHT, MOVE_CODE_LOOP_LEFT_UP_RIGHT_DOWN },
    { MOVE_CODE_LOOP_DOWN_RIGHT_UP_LEFT, MOVE_CODE_LOOP_RIGHT_DOWN_LEFT_UP },
    { MOVE_CODE_LOOP_LEFT_DOWN_RIGHT_UP, MOVE_CODE_LOOP_DOWN_LEFT_UP_RIGHT },
    { MOVE_CODE_LOOP_RIGHT_UP_LEFT_DOWN, MOVE_CODE_LOOP_UP_RIGHT_DOWN_LEFT },
    { MOVE_CODE_LIST_END, MOVE_CODE_LIST_END },
};

static const s32 sDirSet0[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE };

static const s32 sDirSet31[] = { DIR_DOWN, DIR_RIGHT, DIR_UP, DIR_LEFT };

static void FieldActorMovecode_InitTurnRandom(FieldActor *actor, u32 mask) {
    RailWanderWork *work = ClearActorPositionBlock(actor, sizeof(RailWanderWork));
    work->timer = FieldActorMovecode_GetRandomTableEntry(sTurnWaitTimes, -1);
    work->mask = mask;
    FldAct_SetDrawStatus(actor, DRAW_STATUS_STOP);
    FldAct_ClearMoving(actor);
}

void FieldActorMovecode_InitTurnRandomSet0(FieldActor *actor) {
    FieldActorMovecode_InitTurnRandom(actor, 0);
}

void FieldActorMovecode_InitTurnRandomSet1(FieldActor *actor) {
    FieldActorMovecode_InitTurnRandom(actor, 1);
}

void FieldActorMovecode_InitTurnRandomSet2(FieldActor *actor) {
    FieldActorMovecode_InitTurnRandom(actor, 2);
}

void FieldActorMovecode_InitTurnRandomSet3(FieldActor *actor) {
    FieldActorMovecode_InitTurnRandom(actor, 3);
}

void FieldActorMovecode_InitTurnRandomSet4(FieldActor *actor) {
    FieldActorMovecode_InitTurnRandom(actor, 4);
}

void FieldActorMovecode_InitTurnRandomSet5(FieldActor *actor) {
    FieldActorMovecode_InitTurnRandom(actor, 5);
}

void FieldActorMovecode_InitTurnRandomSet6(FieldActor *actor) {
    FieldActorMovecode_InitTurnRandom(actor, 6);
}

void FieldActorMovecode_InitTurnRandomSet7(FieldActor *actor) {
    FieldActorMovecode_InitTurnRandom(actor, 7);
}

void FieldActorMovecode_InitTurnRandomSet8(FieldActor *actor) {
    FieldActorMovecode_InitTurnRandom(actor, 8);
}

void FieldActorMovecode_InitTurnRandomSet9(FieldActor *actor) {
    FieldActorMovecode_InitTurnRandom(actor, 9);
}

void FieldActorMovecode_InitTurnRandomSet10(FieldActor *actor) {
    FieldActorMovecode_InitTurnRandom(actor, 10);
}

void FieldActorMovecode_MoveTurnRandom(FieldActor *actor) {
    RailWanderWork *work = GetNPCRailPosPtrAddr(actor);
    if (GetActorEvType(actor) == ACTOR_EVENT_TYPE_TRAINER_TURN_ON_B_PRESS) {
        if (FldAct_IsNearPlayer(actor) == TRUE && FieldActorMovecode_IsBButtonPressed() == TRUE) {
            u16 faceDir = GetActorFaceDir(actor);
            u16 dir = FieldActorMovecode_GetNextDirInRotation(GetActorMoveCode(actor), faceDir);
            if (dir != DIR_NONE) {
                CheckSetActorFaceDir(actor, dir);
            }
        }
    } else {
        u32 dir = FieldActorMovecode_GetDirTowardPlayerInSet(actor, work->mask, DIR_NONE);
        if (dir != DIR_NONE) {
            CheckSetActorFaceDir(actor, dir);
        } else if (work->pause == 0) {
            work->timer--;
            if (work->timer <= 0) {
                work->timer = FieldActorMovecode_GetRandomTableEntry(sTurnWaitTimes, -1);
                dir = FieldActorMovecode_GetRandomDirFromSet(work->mask, DIR_NONE);
                CheckSetActorFaceDir(actor, dir);
            }
        }
    }
    SetActorInitialGPosToNowGPos(actor);
}

static u16 FieldActorMovecode_GetNextDirInRotation(u16 moveCode, u16 faceDir) {
    s32 i = 0;
    do {
        if (moveCode == sMoveCodeTurnDirs[i][0]) {
            const u16 *entry;
            s32 j = 1;

            entry = sMoveCodeTurnDirs[i];
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
    } while (sMoveCodeTurnDirs[i][0] != MOVE_CODE_LIST_END);
    return DIR_NONE;
}

void func_ov036_021913dc(FieldActor *actor) {
}

void FieldActorMovecode_InitWalkRandomSet11(FieldActor *actor) {
    FieldActorMovecode_InitWalkRandom(actor, ACMD_WALK_8F, 11, WALK_RANDOM_MODE_ANY);
}

void FieldActorMovecode_InitWalkRandomSet12(FieldActor *actor) {
    FieldActorMovecode_InitWalkRandom(actor, ACMD_WALK_8F, 12, WALK_RANDOM_MODE_ANY);
}

void FieldActorMovecode_InitWalkRandomSet13(FieldActor *actor) {
    FieldActorMovecode_InitWalkRandom(actor, ACMD_WALK_8F, 13, WALK_RANDOM_MODE_ANY);
}

void FieldActorMovecode_InitWalkRandomSet13Bump(FieldActor *actor) {
    FieldActorMovecode_InitWalkRandom(actor, ACMD_WALK_8F, 13, WALK_RANDOM_MODE_BUMP);
}

static void FieldActorMovecode_InitWalkRandom(FieldActor *actor, u32 acmdBase, u32 dirMask, u32 mode) {
    RailWalkWork *work = ClearActorPositionBlock(actor, sizeof(RailWalkWork));
    work->mode = mode;
    work->acmdBase = acmdBase;
    work->mask = dirMask;
    FldAct_SetDrawStatus(actor, DRAW_STATUS_STOP);
    FldAct_ClearMoving(actor);
}

// The steps of the random walk: face the way the actor faces, wait a random time, pick a direction of its set and walk
// one tile if it can, and wait for the walk
enum {
    WALK_RANDOM_FACE,
    WALK_RANDOM_WAIT_FACE,
    WALK_RANDOM_WAIT,
    WALK_RANDOM_WALK,
    WALK_RANDOM_WAIT_WALK,
};

void FieldActorMovecode_MoveWalkRandom(FieldActor *actor) {
    RailWalkWork *work = GetNPCRailPosPtrAddr(actor);
    u32 dir;
    u32 collision;
    u32 acmd;
    switch (work->state) {
    case WALK_RANDOM_FACE:
        FldAct_ClearMoving(actor);
        FldAct_ClearMoveEnd(actor);
        dir = GetActorFaceDir(actor);
        acmd = GetAcmdForDir(dir, ACMD_FACE);
        FldAct_SetLocalAcmd(actor, acmd);
        work->state++;
        break;
    case WALK_RANDOM_WAIT_FACE:
        if (!FieldAcmdTCB_Update(actor)) {
            break;
        }
        work->timer = FieldActorMovecode_GetRandomTableEntry(sTurnWaitTimes, -1);
        work->state++;
    case WALK_RANDOM_WAIT:
        work->timer--;
        if (work->timer != 0) {
            break;
        }
        work->state++;
    case WALK_RANDOM_WALK:
        dir = FieldActorMovecode_GetRandomDirFromSet(work->mask, DIR_NONE);
        ChangeActorDirection(actor, dir);
        if (work->mode == WALK_RANDOM_MODE_IN_RECT && FieldActorMovecode_IsDirInsideMoveRect(actor, dir) == 0) {
            work->state = WALK_RANDOM_FACE;
            return;
        }
        collision = ActorRouteCollCheckOneTileInDir(actor, dir);
        if (collision != 0) {
            if (work->mode == WALK_RANDOM_MODE_BUMP) {
                if (collision & 1) {
                    work->state = WALK_RANDOM_FACE;
                    return;
                }
            } else {
                work->state = WALK_RANDOM_FACE;
                return;
            }
        }
        acmd = GetAcmdForDir(dir, work->acmdBase);
        FldAct_SetLocalAcmd(actor, acmd);
        FldAct_SetMoving(actor);
        work->state++;
    case WALK_RANDOM_WAIT_WALK:
        if (FieldAcmdTCB_Update(actor)) {
            FldAct_ClearMoving(actor);
            work->state = WALK_RANDOM_FACE;
        }
        break;
    default:
        break;
    }
}

static void FieldActorMovecode_GetMoveRect(FieldActor *actor, WalkRect *rect) {
    s32 z;
    s32 x = GetActorDefaultGPosX(actor);
    s32 w;
    s32 h;
    z = GetActorDefaultGPosZ(actor);
    w = GetActorWalkAreaW(actor);
    h = GetActorWalkAreaH(actor);
    switch (GetActorMoveCode(actor)) {
    case MOVE_CODE_TURN_RANDOM_UP_LEFT:
        rect->minX = x - w;
        rect->maxX = x;
        rect->minZ = z - h;
        rect->maxZ = z;
        break;
    case MOVE_CODE_TURN_RANDOM_UP_RIGHT:
        rect->minX = x;
        rect->maxX = x + w;
        rect->minZ = z - h;
        rect->maxZ = z;
        break;
    case MOVE_CODE_TURN_RANDOM_DOWN_LEFT:
        rect->minX = x - w;
        rect->maxX = x;
        rect->minZ = z;
        rect->maxZ = z + h;
        break;
    case MOVE_CODE_TURN_RANDOM_DOWN_RIGHT:
        rect->maxX = x + w;
        rect->minX = x;
        rect->minZ = z;
        rect->maxZ = z + h;
        break;
    case MOVE_CODE_TURN_RANDOM_UP_DOWN_LEFT:
        rect->minX = x - w;
        rect->maxX = x;
        rect->minZ = z - h;
        rect->maxZ = z + h;
        break;
    case MOVE_CODE_TURN_RANDOM_UP_DOWN_RIGHT:
        rect->maxX = x + w;
        rect->minX = x;
        rect->minZ = z - h;
        rect->maxZ = z + h;
        break;
    case MOVE_CODE_TURN_RANDOM_UP_LEFT_RIGHT:
        rect->minX = x - w;
        rect->maxX = x + w;
        rect->minZ = z - h;
        rect->maxZ = z;
        break;
    case MOVE_CODE_TURN_RANDOM_DOWN_LEFT_RIGHT:
        rect->minX = x - w;
        rect->maxX = x + w;
        rect->minZ = z;
        rect->maxZ = z + h;
        break;
    }
}

static BOOL FieldActorMovecode_IsDirInsideMoveRect(FieldActor *actor, u32 dir) {
    WalkRect rect;
    s32 x;
    s32 z;
    FieldActorMovecode_GetMoveRect(actor, &rect);
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

static void FieldActorMovecode_InitFixedDir(FieldActor *actor, u32 dir) {
    RailFaceWork *work = ClearActorPositionBlock(actor, sizeof(RailFaceWork));
    work->dir = dir;
    FldAct_SetDrawStatus(actor, DRAW_STATUS_STOP);
    FldAct_ClearMoving(actor);
    SetActorInitialGPosToNowGPos(actor);
}

// The steps of the codes that face one way: face it once, then stay
enum {
    FIXED_DIR_FACE,
    FIXED_DIR_STAY,
};

void FieldActorMovecode_MoveFixedDir(FieldActor *actor) {
    RailFaceWork *work = GetNPCRailPosPtrAddr(actor);
    switch (work->state) {
    case FIXED_DIR_FACE:
        CheckSetActorFaceDir(actor, work->dir);
        work->state++;
        break;
    case FIXED_DIR_STAY:
        break;
    }
}

void FieldActorMovecode_InitFixedDirUp(FieldActor *actor) {
    FieldActorMovecode_InitFixedDir(actor, DIR_UP);
}

void FieldActorMovecode_InitFixedDirDown(FieldActor *actor) {
    FieldActorMovecode_InitFixedDir(actor, DIR_DOWN);
}

void FieldActorMovecode_InitFixedDirLeft(FieldActor *actor) {
    FieldActorMovecode_InitFixedDir(actor, DIR_LEFT);
}

void FieldActorMovecode_InitFixedDirRight(FieldActor *actor) {
    FieldActorMovecode_InitFixedDir(actor, DIR_RIGHT);
}

static void FieldActorMovecode_InitSpin(FieldActor *actor, u32 turn) {
    RailTurnWork *work = ClearActorPositionBlock(actor, sizeof(RailTurnWork));
    work->turn = turn;
    FldAct_SetDrawStatus(actor, DRAW_STATUS_STOP);
    FldAct_ClearMoving(actor);
    SetActorInitialGPosToNowGPos(actor);
}

void FieldActorMovecode_InitSpinLeft(FieldActor *actor) {
    FieldActorMovecode_InitSpin(actor, DIR_LEFT);
}

void FieldActorMovecode_InitSpinRight(FieldActor *actor) {
    FieldActorMovecode_InitSpin(actor, DIR_RIGHT);
}

void FieldActorMovecode_MoveSpin(FieldActor *actor) {
    RailTurnWork *work = GetNPCRailPosPtrAddr(actor);
    while (sSpinSteps[work->state](actor, work) == TRUE) {
    }
}

static BOOL FieldActorMovecode_SpinStepTurn(FieldActor *actor, RailTurnWork *work) {
    u32 acmd;
    u32 dir = FieldActorMovecode_GetDirTowardPlayerInSet(actor, 0x26, DIR_NONE);
    if (dir == DIR_NONE) {
        dir = GetActorFaceDir(actor);
    }
    acmd = GetAcmdForDir(dir, ACMD_FACE);
    FldAct_SetLocalAcmd(actor, acmd);
    work->state = 1;
    return TRUE;
}

static BOOL FieldActorMovecode_SpinStepWaitTurn(FieldActor *actor, RailTurnWork *work) {
    if (!FieldAcmdTCB_Update(actor)) {
        return FALSE;
    }
    if (GetActorEvType(actor) != ACTOR_EVENT_TYPE_TRAINER_TURN_ON_B_PRESS) {
        work->count = 0;
    }
    work->state = 2;
    return TRUE;
}

static BOOL FieldActorMovecode_SpinStepWaitInterval(FieldActor *actor, RailTurnWork *work) {
    if (GetActorEvType(actor) == ACTOR_EVENT_TYPE_TRAINER_TURN_ON_B_PRESS) {
        if (work->count != 0) {
            work->count = 0;
            return FALSE;
        }
        if (!FldAct_IsNearPlayer(actor)) {
            return FALSE;
        }
        if (!FieldActorMovecode_IsBButtonPressed()) {
            return FALSE;
        }
        work->count++;
    } else {
        if (work->count != 0 && FieldActorMovecode_GetDirTowardPlayerInSet(actor, 0x26, DIR_NONE) != DIR_NONE) {
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

static BOOL FieldActorMovecode_SpinStepRotateDir(FieldActor *actor, RailTurnWork *work) {
    u32 counterclockwise[5] = { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT, DIR_NONE };
    u32 clockwise[5] = { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT, DIR_NONE };
    u32 *dirs = counterclockwise;
    u32 face;
    s32 i;
    if (work->turn != DIR_LEFT) {
        dirs = clockwise;
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

void FieldActorMovecode_InitSwing(FieldActor *actor) {
    FieldActorMovecode_InitSpin(actor, DIR_RIGHT);
}

void FieldActorMovecode_MoveSwing(FieldActor *actor) {
    RailTurnWork *work = GetNPCRailPosPtrAddr(actor);
    while (sSwingSteps[work->state](actor, work) == TRUE) {
    }
}

static BOOL FieldActorMovecode_SwingStepTurn(FieldActor *actor, RailTurnWork *work) {
    u32 dir = GetActorFaceDir(actor);
    u32 acmd = GetAcmdForDir(dir, ACMD_FACE);
    FldAct_SetLocalAcmd(actor, acmd);
    work->state = 1;
    return TRUE;
}

static BOOL FieldActorMovecode_SwingStepWaitTurn(FieldActor *actor, RailTurnWork *work) {
    if (!FieldAcmdTCB_Update(actor)) {
        return FALSE;
    }
    work->count = 0;
    work->state = 2;
    return TRUE;
}

static BOOL FieldActorMovecode_SwingStepWaitInterval(FieldActor *actor, RailTurnWork *work) {
    work->count++;
    if (work->count < 0x18) {
        return FALSE;
    }
    work->state = 3;
    return TRUE;
}

static BOOL FieldActorMovecode_SwingStepRotateDir(FieldActor *actor, RailTurnWork *work) {
    u32 counterclockwise[5] = { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT, DIR_NONE };
    u32 clockwise[5] = { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT, DIR_NONE };
    u32 *dirs = counterclockwise;
    u32 face;
    s32 i;
    if (work->turn != DIR_LEFT) {
        dirs = clockwise;
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

static BOOL (*const sBackForthSteps[])(FieldActor *actor, RailPaceWork *work) = {
    FieldActorMovecode_BackForthStepFaceDir,
    FieldActorMovecode_BackForthStepWalk,
    FieldActorMovecode_BackForthStepWaitWalk,
};

static const u32 sPlayerSightMoveCodes[] = {
    MOVE_CODE_TURN_RANDOM,
    MOVE_CODE_TURN_RANDOM_UP_LEFT,
    MOVE_CODE_TURN_RANDOM_UP_RIGHT,
    MOVE_CODE_TURN_RANDOM_DOWN_LEFT,
    MOVE_CODE_TURN_RANDOM_DOWN_RIGHT,
    MOVE_CODE_TURN_RANDOM_UP_DOWN_LEFT,
    MOVE_CODE_TURN_RANDOM_UP_DOWN_RIGHT,
    MOVE_CODE_TURN_RANDOM_UP_LEFT_RIGHT,
    MOVE_CODE_TURN_RANDOM_DOWN_LEFT_RIGHT,
    MOVE_CODE_TURN_RANDOM_UP_DOWN,
    MOVE_CODE_TURN_RANDOM_LEFT_RIGHT,
    MOVE_CODE_SPIN_COUNTERCLOCKWISE,
    MOVE_CODE_SPIN_CLOCKWISE,
    MOVE_CODE_LIST_END,
};

static const ActorDirTableEntry sDirSets[] = {
    { 0, sDirSet0 },   { 1, sDirSet1 },   { 2, sDirSet2 },   { 3, sDirSet3 },   { 4, sDirSet4 },   { 5, sDirSet5 },
    { 6, sDirSet6 },   { 7, sDirSet7 },   { 8, sDirSet8 },   { 9, sDirSet9 },   { 10, sDirSet10 }, { 11, sDirSet11 },
    { 12, sDirSet12 }, { 13, sDirSet13 }, { 14, sDirSet14 }, { 15, sDirSet15 }, { 16, sDirSet16 }, { 17, sDirSet17 },
    { 18, sDirSet18 }, { 19, sDirSet19 }, { 20, sDirSet20 }, { 21, sDirSet21 }, { 22, sDirSet22 }, { 23, sDirSet23 },
    { 24, sDirSet24 }, { 25, sDirSet25 }, { 26, sDirSet26 }, { 27, sDirSet27 }, { 28, sDirSet28 }, { 29, sDirSet29 },
    { 30, sDirSet30 }, { 31, sDirSet31 }, { 32, sDirSet32 }, { 33, sDirSet33 }, { 34, sDirSet34 }, { 35, sDirSet35 },
    { 36, sDirSet36 }, { 37, sDirSet37 }, { 38, sDirSet38 }, { 0x27, NULL },
};

void FieldActorMovecode_InitBackForth(FieldActor *actor) {
    RailPaceWork *work = ClearActorPositionBlock(actor, sizeof(RailPaceWork));
    if (FieldActorMovecode_IsMirrorEventType(actor) == TRUE) {
        FieldActorMovecode_InitMirrorSide(actor, &work->face);
    }
}

void FieldActorMovecode_MoveBackForth(FieldActor *actor) {
    RailPaceWork *work = GetNPCRailPosPtrAddr(actor);
    while (sBackForthSteps[work->state](actor, work) == TRUE) {
    }
}

static BOOL FieldActorMovecode_BackForthStepFaceDir(FieldActor *actor, RailPaceWork *work) {
    u32 dir = GetDefaultActorDir(actor);
    if (work->flip == 1) {
        dir = GetInverseDirection((u16)dir);
    }
    SetActorMotionDir(actor, dir);
    if (FieldActorMovecode_IsMirrorEventType(actor) == 0) {
        CheckSetActorFaceDir(actor, dir);
    }
    work->state = 1;
    return TRUE;
}

static BOOL FieldActorMovecode_BackForthStepWalk(FieldActor *actor, RailPaceWork *work) {
    u32 collision;
    u32 dir;
    u32 acmdBase;
    if (FieldActorMovecode_UpdateTriggerWait(actor, &work->wait) == 0) {
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
            if (FieldActorMovecode_IsMirrorEventType(actor) == 0) {
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
    acmdBase = ACMD_WALK_8F;
    if (FieldActorMovecode_IsDashEventType(actor) != 0) {
        if (FieldActorMovecode_IsBButtonHeld() == TRUE && FldAct_IsNearPlayer(actor) == TRUE) {
            acmdBase = ACMD_WALK_4F;
        }
    }
    if (collision != 0) {
        acmdBase = ACMD_STAY_WALK_8F;
    }
    u32 acmd = GetAcmdForDir(dir, acmdBase);
    FldAct_SetLocalAcmd(actor, acmd);
    if (FieldActorMovecode_IsMirrorEventType(actor) == TRUE) {
        FieldActorMovecode_StartMirrorTurn(actor, &work->face);
    }
    FldAct_SetMoving(actor);
    work->state = 2;
    return TRUE;
}

static BOOL FieldActorMovecode_BackForthStepWaitWalk(FieldActor *actor, RailPaceWork *work) {
    if (FieldAcmdTCB_Update(actor) == TRUE) {
        FldAct_ClearMoving(actor);
        if (FieldActorMovecode_IsMirrorEventType(actor) == TRUE) {
            FieldActorMovecode_EndMirrorTurn(actor, &work->face);
        }
        work->state = 0;
    }
    return FALSE;
}

static void FieldActorMovecode_InitRoutePatrol(FieldActor *actor, u32 steps, u32 turns, u32 speed) {
    RailStepWork *work = ClearActorPositionBlock(actor, sizeof(RailStepWork));
    work->steps = steps;
    work->turns = turns;
    work->speed = speed;
    if (FieldActorMovecode_IsMirrorEventType(actor) == TRUE) {
        FieldActorMovecode_InitMirrorSide(actor, &work->face);
    }
}

void FieldActorMovecode_InitRoutePatrolSet14(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 0, 14);
}

void FieldActorMovecode_InitRoutePatrolSet15(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 0, 15);
}

void FieldActorMovecode_InitRoutePatrolSet16(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 1, 16);
}

void FieldActorMovecode_InitRoutePatrolSet17(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 1, 17);
}

void FieldActorMovecode_InitRoutePatrolSet18(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 0, 18);
}

void FieldActorMovecode_InitRoutePatrolSet19(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 0, 19);
}

void FieldActorMovecode_InitRoutePatrolSet20(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 1, 20);
}

void FieldActorMovecode_InitRoutePatrolSet21(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 1, 21);
}

void FieldActorMovecode_InitRoutePatrolSet22(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 1, 22);
}

void FieldActorMovecode_InitRoutePatrolSet23(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 1, 23);
}

void FieldActorMovecode_InitRoutePatrolSet24(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 0, 24);
}

void FieldActorMovecode_InitRoutePatrolSet25(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 0, 25);
}

void FieldActorMovecode_InitRoutePatrolSet26(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 1, 26);
}

void FieldActorMovecode_InitRoutePatrolSet27(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 1, 27);
}

void FieldActorMovecode_InitRoutePatrolSet28(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 0, 28);
}

void FieldActorMovecode_InitRoutePatrolSet29(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrol(actor, 2, 0, 29);
}

void FieldActorMovecode_MoveRoutePatrol(FieldActor *actor) {
    ActorRoundWork *work = GetNPCRailPosPtrAddr(actor);

    while (sRoutePatrolSteps[work->state](actor, work) == TRUE) {
    }
}

static BOOL FieldActorMovecode_RoutePatrolStepWalk(FieldActor *actor, ActorRoundWork *work) {
    s32 blocked;
    const s32 *dirs;
    s32 dir;

    if (!FieldActorMovecode_UpdateTriggerWait(actor, &work->wait)) {
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
    dirs = FieldActorMovecode_GetDirSet(work->dirTable);
    dir = dirs[work->dirIdx];
    SetActorMotionDir(actor, dir);
    if (FieldActorMovecode_IsMirrorEventType(actor) == FALSE) {
        CheckSetActorFaceDir(actor, dir);
    }
    blocked = ActorRouteCollCheckOneTileInDir(actor, dir);
    if (blocked & 1) {
        work->dirIdx++;
        dir = dirs[work->dirIdx];
        SetActorMotionDir(actor, dir);
        if (FieldActorMovecode_IsMirrorEventType(actor) == FALSE) {
            CheckSetActorFaceDir(actor, dir);
        }
        blocked = ActorRouteCollCheckOneTileInDir(actor, dir);
    }
    u32 speed = 12;

    if (FieldActorMovecode_IsDashEventType(actor)) {
        if (FieldActorMovecode_IsBButtonHeld() == TRUE && FldAct_IsNearPlayer(actor) == TRUE) {
            speed = 16;
        }
    }
    if (blocked) {
        speed = 32;
    }
    StartDirAcmd(actor, dir, speed);
    if (FieldActorMovecode_IsMirrorEventType(actor) == TRUE) {
        FieldActorMovecode_StartMirrorTurn(actor, &work->face);
    }
    FldAct_SetMoving(actor);
    work->state = 1;
    return TRUE;
}

static BOOL FieldActorMovecode_RoutePatrolStepWaitWalk(FieldActor *actor, ActorRoundWork *work) {
    if (FieldAcmdTCB_Update(actor) == TRUE) {
        FldAct_ClearMoving(actor);
        if (FieldActorMovecode_IsMirrorEventType(actor) == TRUE) {
            FieldActorMovecode_EndMirrorTurn(actor, &work->face);
        }
        work->state = 0;
    }
    return FALSE;
}

static void FieldActorMovecode_InitRoutePatrolSwap(FieldActor *actor, u32 limit, u32 axis, u32 dirTable) {
    ActorRoundWork *work = ClearActorPositionBlock(actor, 12);

    work->limit = limit;
    work->axis = axis;
    work->dirTable = dirTable;
    if (FieldActorMovecode_IsMirrorEventType(actor) == TRUE) {
        FieldActorMovecode_InitMirrorSide(actor, &work->face);
    }
}

void FieldActorMovecode_InitRoutePatrolSwapSet30(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrolSwap(actor, 2, 1, 0x1e);
}

void FieldActorMovecode_InitRoutePatrolSwapSet31(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrolSwap(actor, 2, 1, 0x1f);
}

void FieldActorMovecode_InitRoutePatrolSwapSet32(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrolSwap(actor, 2, 0, 0x20);
}

void FieldActorMovecode_InitRoutePatrolSwapSet33(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrolSwap(actor, 2, 0, 0x21);
}

void FieldActorMovecode_InitRoutePatrolSwapSet34(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrolSwap(actor, 2, 1, 0x22);
}

void FieldActorMovecode_InitRoutePatrolSwapSet35(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrolSwap(actor, 2, 1, 0x23);
}

void FieldActorMovecode_InitRoutePatrolSwapSet36(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrolSwap(actor, 2, 0, 0x24);
}

void FieldActorMovecode_InitRoutePatrolSwapSet37(FieldActor *actor) {
    FieldActorMovecode_InitRoutePatrolSwap(actor, 2, 0, 0x25);
}

void FieldActorMovecode_MoveRoutePatrolSwap(FieldActor *actor) {
    ActorPatrolWork *work = GetNPCRailPosPtrAddr(actor);

    while (sRoutePatrolSwapSteps[work->state](actor, work) == TRUE) {
    }
}

static void FieldActorMovecode_AdvanceRouteIndex(ActorPatrolWork *work) {
    if (work->reverse == TRUE) {
        work->dirIdx--;
        if (work->dirIdx < 0) {
            work->dirIdx = 3;
        }
    } else {
        work->dirIdx++;
    }
}

static BOOL FieldActorMovecode_RoutePatrolSwapStepLookForPlayer(FieldActor *actor, ActorPatrolWork *work) {
    if (GetActorEvType(actor) == ACTOR_EVENT_TYPE_TURN_BACK_ON_SIGHT) {
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

static BOOL FieldActorMovecode_RoutePatrolSwapStepSwapMoveCode(FieldActor *actor, ActorPatrolWork *work) {
    if (FieldAcmdTCB_Update(actor) == TRUE) {
        u16 other = MOVE_CODE_LIST_END;
        u16 moveCode = GetActorMoveCode(actor);
        const ActorMoveCodePair *pair;

        for (pair = sPatrolSwapMoveCodes; pair->first != MOVE_CODE_LIST_END; pair++) {
            if (pair->first == moveCode) {
                other = pair->second;
            } else if (pair->second == moveCode) {
                other = pair->first;
            }
            if (other != MOVE_CODE_LIST_END) {
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

static BOOL FieldActorMovecode_RoutePatrolSwapStepWalk(FieldActor *actor, ActorPatrolWork *work) {
    s32 blocked;
    const s32 *dirs;
    s32 dir;

    if (!FieldActorMovecode_UpdateTriggerWait(actor, &work->wait)) {
        return FALSE;
    }
    if (work->dirIdx == work->limit) {
        if (work->axis == 0) {
            if (GetActorDefaultGPosX(actor) == GetGPosX(actor)) {
                FieldActorMovecode_AdvanceRouteIndex(work);
            }
        } else if (GetActorDefaultGPosZ(actor) == GetGPosZ(actor)) {
            FieldActorMovecode_AdvanceRouteIndex(work);
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
    dirs = FieldActorMovecode_GetDirSet(work->dirTable);
    dir = dirs[work->dirIdx];
    SetActorMotionDir(actor, dir);
    if (FieldActorMovecode_IsMirrorEventType(actor) == FALSE) {
        CheckSetActorFaceDir(actor, dir);
    }
    blocked = ActorRouteCollCheckOneTileInDir(actor, dir);
    if (blocked & 1) {
        FieldActorMovecode_AdvanceRouteIndex(work);
        dir = dirs[work->dirIdx];
        SetActorMotionDir(actor, dir);
        if (FieldActorMovecode_IsMirrorEventType(actor) == FALSE) {
            CheckSetActorFaceDir(actor, dir);
        }
        blocked = ActorRouteCollCheckOneTileInDir(actor, dir);
    }
    u32 speed = 12;

    if (FieldActorMovecode_IsDashEventType(actor)) {
        if (FieldActorMovecode_IsBButtonHeld() == TRUE && FldAct_IsNearPlayer(actor) == TRUE) {
            speed = 16;
        }
    }
    if (blocked) {
        speed = 32;
    }
    StartDirAcmd(actor, dir, speed);
    if (FieldActorMovecode_IsMirrorEventType(actor) == TRUE) {
        FieldActorMovecode_StartMirrorTurn(actor, &work->face);
    }
    FldAct_SetMoving(actor);
    work->state = 3;
    return TRUE;
}

static BOOL FieldActorMovecode_RoutePatrolSwapStepWaitWalk(FieldActor *actor, ActorPatrolWork *work) {
    if (FieldAcmdTCB_Update(actor) == TRUE) {
        FldAct_ClearMoving(actor);
        if (FieldActorMovecode_IsMirrorEventType(actor) == TRUE) {
            FieldActorMovecode_EndMirrorTurn(actor, &work->face);
        }
        work->state = 0;
    }
    return FALSE;
}

static BOOL FieldActorMovecode_IsBButtonHeld(void) {
    if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_B) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldActorMovecode_IsPlayerWalking(FieldActor *actor) {
    FieldActor *player = FindPlayerFieldActor(GetActorMModelSystem(actor));

    if (player != NULL) {
        switch (FldAct_GetAcmd(player)) {
        case ACMD_DASH + DIR_UP:
        case ACMD_DASH + DIR_DOWN:
        case ACMD_DASH + DIR_LEFT:
        case ACMD_DASH + DIR_RIGHT:
        case ACMD_DASH_UPHILL + DIR_UP:
        case ACMD_DASH_UPHILL + DIR_DOWN:
        case ACMD_DASH_UPHILL + DIR_LEFT:
        case ACMD_DASH_UPHILL + DIR_RIGHT:
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FieldActorMovecode_IsBButtonPressed(void) {
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        return TRUE;
    }
    return FALSE;
}

static u32 FieldActorMovecode_FindTableEnd(const s32 *list, u32 end) {
    u32 count = 0;

    while (end != list[count]) {
        count++;
    }
    return count;
}

static s32 FieldActorMovecode_GetRandomTableEntry(const s32 *list, u32 end) {
    return list[GFL_RandomLC(FieldActorMovecode_FindTableEnd(list, end))];
}

static s32 FieldActorMovecode_GetRandomDirFromSet(u32 index, u32 end) {
    const s32 *list = FieldActorMovecode_GetDirSet(index);

    return list[GFL_RandomLC(FieldActorMovecode_FindTableEnd(list, end))];
}

static const s32 *FieldActorMovecode_GetDirSet(u32 index) {
    const ActorDirTableEntry *entry = sDirSets;

    for (; entry->key != 0x27; entry++) {
        if (entry->key == index) {
            return entry->dirs;
        }
    }
    return NULL;
}

static u32 FieldActorMovecode_GetSightDirToPlayer(FieldActor *actor) {
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

    if (evType != ACTOR_EVENT_TYPE_TRAINER && evType != ACTOR_EVENT_TYPE_TRAINER_ALL_DIRS) {
        return DIR_NONE;
    }
    if (!FieldActorMovecode_IsPlayerWalking(actor)) {
        return DIR_NONE;
    }
    i = 0;
    moveCode = GetActorMoveCode(actor);
    do {
        entry = sPlayerSightMoveCodes[i];
        i++;
    } while (entry != moveCode && entry != MOVE_CODE_LIST_END);
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

static u32 FieldActorMovecode_GetDirTowardPlayerInSet(FieldActor *actor, u32 index, u32 end) {
    const s32 *dirs = FieldActorMovecode_GetDirSet(index);
    s32 count = FieldActorMovecode_FindTableEnd(dirs, end);
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
    dir = FieldActorMovecode_GetSightDirToPlayer(actor);
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

static BOOL FieldActorMovecode_UpdateTriggerWait(FieldActor *actor, u8 *wait) {
    if (GetActorEvType(actor) != ACTOR_EVENT_TYPE_TRAINER_MOVE_ON_B_PRESS) {
        return TRUE;
    }
    if (*wait == 0) {
        if (!FldAct_IsNearPlayer(actor)) {
            return FALSE;
        }
        if (!FieldActorMovecode_IsBButtonPressed()) {
            return FALSE;
        }
        *wait = GetActorUserParam(actor, 1);
    }
    if (*wait != 0) {
        (*wait)--;
    }
    return TRUE;
}

static BOOL FieldActorMovecode_IsDashEventType(FieldActor *actor) {
    u16 evType = GetActorEvType(actor);

    if (evType == ACTOR_EVENT_TYPE_TRAINER_DASH || evType == ACTOR_EVENT_TYPE_SPIN_COUNTERCLOCKWISE_DASH ||
        evType == ACTOR_EVENT_TYPE_SPIN_CLOCKWISE_DASH || evType == ACTOR_EVENT_TYPE_WALK_SPIN_COUNTERCLOCKWISE_DASH ||
        evType == ACTOR_EVENT_TYPE_WALK_SPIN_CLOCKWISE_DASH || evType == ACTOR_EVENT_TYPE_DASH) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldActorMovecode_IsMirrorEventType(FieldActor *actor) {
    int evType = GetActorEvType(actor);

    if (evType == ACTOR_EVENT_TYPE_TRAINER_WALK_SPIN_COUNTERCLOCKWISE ||
        evType == ACTOR_EVENT_TYPE_TRAINER_WALK_SPIN_CLOCKWISE ||
        evType == ACTOR_EVENT_TYPE_WALK_SPIN_COUNTERCLOCKWISE_DASH ||
        evType == ACTOR_EVENT_TYPE_WALK_SPIN_CLOCKWISE_DASH) {
        return TRUE;
    }
    return FALSE;
}

static void FieldActorMovecode_InitMirrorSide(FieldActor *actor, ActorFaceMemory *face) {
    u16 evType = GetActorEvType(actor);

    if (evType == ACTOR_EVENT_TYPE_TRAINER_WALK_SPIN_COUNTERCLOCKWISE ||
        evType == ACTOR_EVENT_TYPE_WALK_SPIN_COUNTERCLOCKWISE_DASH) {
        face->row = 0;
    } else {
        face->row = 1;
    }
}

static void FieldActorMovecode_StartMirrorTurn(FieldActor *actor, ActorFaceMemory *face) {
    u16 dir = GetActorFaceDir(actor);
    s32 i;
    s32 nextDir;

    i = 0;
    while (i < 4 && dir != sMirrorTurnOrders[face->row][i]) {
        i++;
    }
    i++;
    face->dir = dir;
    i %= 4;
    nextDir = sMirrorTurnOrders[face->row][i];
    if (CheckActorFlag(actor, ACTOR_FLAG_FACE_DIR_LOCK)) {
        face->hadFlag8 = 1;
    } else {
        face->hadFlag8 = 0;
    }
    CheckSetActorFaceDir(actor, nextDir);
    SetActorFlag(actor, ACTOR_FLAG_FACE_DIR_LOCK);
}

static void FieldActorMovecode_EndMirrorTurn(FieldActor *actor, ActorFaceMemory *face) {
    if (face->hadFlag8 == 0) {
        ClearActorFlag(actor, ACTOR_FLAG_FACE_DIR_LOCK);
    }
}
