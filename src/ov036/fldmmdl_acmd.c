// The field actors' action commands (fldmmdl_acmd.c, after the file name that its allocation passes): the task that
// runs a list of them, and the start, update and end functions of each command with the tables of them that
// fldmmdl_movedata.c lists. Names such as FieldAcmdTCB_Create, GetAcmdForDir and the ACMD_ tables are swan's
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0). The code was first written by mariano54 on their
// parallel-decomp branch

#include "types.h"
#include "constants/directions.h"
#include "constants/obj_codes.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/field_effect.h"
#include "field/fldmmdl_movecode_pair.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"

// The state of an action command task, which is also the index of its function in ACMD_TCB_FUNCS
enum {
    ACMD_TCB_STATE_BEGIN,
    ACMD_TCB_STATE_WAIT_ACMD_READY,
    ACMD_TCB_STATE_COMMIT_ACMD,
    ACMD_TCB_STATE_CHECK_SEQ_PERMS,
    ACMD_TCB_STATE_ADVANCE_QUEUE,
    ACMD_TCB_STATE_END,
};

// A command of an action list, as ACMD builds it
typedef struct {
    u16 code;
    u16 count;
} FieldAcmdEntry;

typedef struct {
    u32 state;
    BOOL ended;
    s32 repeat;
    FieldActor *actor;
    const FieldAcmdEntry *cursor;
    const u32 *queue;
} FieldAcmdWork;

// The work of the moves that last some frames
typedef struct {
    u16 state;
    s16 frames;
    u32 dir;
    fx32 speed;
} FieldAcmdWalkWork;

typedef struct {
    fx32 speed;
    fx32 progress;
    u16 heightStep;
    u16 height;
    s8 dir;
    s8 frames;
    u8 state;
    s8 heightTable;
} FieldAcmdJumpWork;

typedef struct {
    s32 pos;
    s32 speed;
} AcmdPosSpeedWork;

typedef struct {
    s32 kind;
    FieldEffectTask *task;
} AcmdEffectWork;

typedef struct {
    s16 dir;
    u16 state;
    s16 steps;
    s16 index;
} AcmdDirStepsWork;

typedef struct {
    s32 frames;
    VecFx32 delta;
} AcmdShiftWork;

typedef struct {
    u16 count;
    u16 limit;
} AcmdWaitWork;

// The work of a move that slides the actor over frames while a model bobs along an axis
typedef struct {
    u8 frames;
    u8 bobAxis;
    u8 axis;
    u8 bobNegate;
    s32 step;
    s32 distance;
    u16 angle;
    u16 angleStep;
} SlideBobWork;

// The work of a move that walks the actor over a few frames in a direction
typedef struct {
    u16 anim;
    s16 frames;
    s32 dir;
    fx32 distance;
} WalkFramesWork;

// The work of a move that lifts the actor
typedef struct {
    fx32 height;
    fx32 speed;
    u16 time;
    u8 lastFrame;
} LiftWork;

// The work of a move that shifts the model while a sound plays
typedef struct {
    u16 time;
    u8 lastFrame;
} ShiftWork;

static void FieldAcmdTCB_Callback(TCB *tcb, void *data);
static BOOL FieldAcmdTCB_Begin(FieldAcmdWork *work);
static BOOL FieldAcmdTCB_WaitAcmdReady(FieldAcmdWork *work);
static BOOL FieldAcmdTCB_CommitAcmd(FieldAcmdWork *work);
static BOOL FieldAcmdTCB_CheckSeqPerms(FieldAcmdWork *work);
static BOOL FieldAcmdTCB_AdvanceQueue(FieldAcmdWork *work);
static BOOL FieldAcmdTCB_End(FieldAcmdWork *work);
static BOOL FldAct_ExecAcmd(FieldActor *actor, u32 acmd, u32 state);
static BOOL FieldAcmdTCB_SetAcmdEnd(FieldActor *actor);
static void FieldAcmdTCB_FaceInit(FieldActor *actor, u32 dir);
static BOOL FieldAcmdTCB_FaceUp(FieldActor *actor);
static BOOL FieldAcmdTCB_FaceDown(FieldActor *actor);
static BOOL FieldAcmdTCB_FaceLeft(FieldActor *actor);
static BOOL FieldAcmdTCB_FaceRight(FieldActor *actor);
static void FieldAcmdTCB_WalkInit(FieldActor *actor, u32 dir, fx32 speed, s16 frames, u16 state);
static BOOL FieldAcmdTCB_WalkMove(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkUp32F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkDown32F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkLeft32F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkRight32F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkUp16F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkDown16F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkLeft16F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkRight16F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkUp8F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkDown8F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkLeft8F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkRight8F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkUp4F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkDown4F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkLeft4F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkRight4F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkUp2F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkDown2F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkLeft2F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkRight2F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkUp1F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkDown1F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkLeft1F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkRight1F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashUp4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashDown4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashLeft4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashRight4F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkUp16FFastAnime(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkDown16FFastAnime(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkLeft16FFastAnime(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkRight16FFastAnime(FieldActor *actor);
static void FieldAcmdTCB_StayWalkInit(FieldActor *actor, u32 dir, s16 frames, u16 state);
static BOOL FieldAcmdTCB_StayWalkMove(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkUp32F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkDown32F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkLeft32F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkRight32F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkUp16F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkDown16F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkLeft16F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkRight16F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkUp8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkDown8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkLeft8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkRight8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkUp4F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkDown4F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkLeft4F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkRight4F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkUp2F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkDown2F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkLeft2F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkRight2F(FieldActor *actor);

static void FieldAcmdTCB_JumpInit(FieldActor *actor, u32 dir, fx32 speed, u32 frames, u16 state, s16 heightTable,
                                  u16 heightStep, u32 se);

static void FieldAcmdTCB_JumpInitWithSe(FieldActor *actor, u32 dir, fx32 speed, u32 frames, u16 state, s16 heightTable,
                                        u16 heightStep);

static BOOL FieldAcmdTCB_JumpMove(FieldActor *actor);
static BOOL FieldAcmdTCB_StayJumpUp16F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayJumpDown16F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayJumpLeft16F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayJumpRight16F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayJumpUp8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayJumpDown8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayJumpLeft8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayJumpRight8F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpUp1G8F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpDown1G8F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpLeft1G8F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpRight1G8F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpUp2G16F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpDown2G16F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpLeft2G16F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpRight2G16F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpLeft1G16F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpRight1G16F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpLeft3G12F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpRight3G12F(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpUp3G24FNoSe(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpDown3G24FNoSe(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpLeft3G24FNoSe(FieldActor *actor);
static BOOL FieldAcmdTCB_JumpRight3G24FNoSe(FieldActor *actor);
static void FieldAcmdTCB_WaitInit(FieldActor *actor, u32 count);
static BOOL FieldAcmdTCB_WaitMove(FieldActor *actor);
static BOOL FieldAcmdTCB_Wait1F(FieldActor *actor);
static BOOL FieldAcmdTCB_Wait2F(FieldActor *actor);
static BOOL FieldAcmdTCB_Wait4F(FieldActor *actor);
static BOOL FieldAcmdTCB_Wait8F(FieldActor *actor);
static BOOL FieldAcmdTCB_Wait15F(FieldActor *actor);
static BOOL FieldAcmdTCB_Wait16F(FieldActor *actor);
static BOOL FieldAcmdTCB_Wait32F(FieldActor *actor);
static BOOL FieldAcmdTCB_RiseInit(FieldActor *actor);
static BOOL FieldAcmdTCB_RiseMove(FieldActor *actor);
static BOOL FieldAcmdTCB_FallInit(FieldActor *actor);
static BOOL FieldAcmdTCB_FallMove(FieldActor *actor);
static BOOL FieldAcmdTCB_SetHiddenFlag(FieldActor *actor);
static BOOL FieldAcmdTCB_ClearHiddenFlag(FieldActor *actor);
static BOOL FieldAcmdTCB_SetFaceDirLockFlag(FieldActor *actor);
static BOOL FieldAcmdTCB_ClearFaceDirLockFlag(FieldActor *actor);
static BOOL FieldAcmdTCB_SetAnimePauseFlag(FieldActor *actor);
static BOOL FieldAcmdTCB_ClearAnimePauseFlag(FieldActor *actor);
static void FieldAcmdTCB_MarkWaitInit(FieldActor *actor, u32 kind, u32 unused, BOOL playSE);
static BOOL FieldAcmdTCB_MarkWait(FieldActor *actor);
static BOOL FieldAcmdTCB_MarkWaitExcl(FieldActor *actor);
static BOOL FieldAcmdTCB_MarkWaitExclNoSe(FieldActor *actor);
static BOOL FieldAcmdTCB_MarkWaitNoteNoSe(FieldActor *actor);
static BOOL func_ov036_0218fd18(FieldActor *actor);
static BOOL func_ov036_0218fd28(FieldActor *actor);
static BOOL FieldAcmdTCB_MarkWaitQuestion(FieldActor *actor);
static BOOL FieldAcmdTCB_MarkWaitNote(FieldActor *actor);
static BOOL FieldAcmdTCB_MarkWaitEllipsis(FieldActor *actor);
static void FieldAcmdTCB_WalkTableInit(FieldActor *actor, u32 dir, u32 steps, u32 state);
static BOOL FieldAcmdTCB_WalkTableMove(FieldActor *actor, const s32 *distances);
static BOOL FieldAcmdTCB_WalkUp6F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkDown6F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkLeft6F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkRight6F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashUp6F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashDown6F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashLeft6F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashRight6F(FieldActor *actor);
static BOOL FieldAcmdTCB_Walk6FMove(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkUp3F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkDown3F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkLeft3F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkRight3F(FieldActor *actor);
static BOOL FieldAcmdTCB_Walk3FMove(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkUp7F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkDown7F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkLeft7F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkRight7F(FieldActor *actor);
static BOOL FieldAcmdTCB_Walk7FMove(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkUp12F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkDown12F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkLeft12F(FieldActor *actor);
static BOOL FieldAcmdTCB_WalkRight12F(FieldActor *actor);
static BOOL FieldAcmdTCB_Walk12FMove(FieldActor *actor);
static BOOL FieldAcmdTCB_AnimeWaitEndInit10(FieldActor *actor);
static BOOL FieldAcmdTCB_AnimeWaitEndInit11(FieldActor *actor);
static BOOL FieldAcmdTCB_AnimeWaitEndInit12(FieldActor *actor);
static BOOL FieldAcmdTCB_AnimeWaitEndInit10Se(FieldActor *actor);
static BOOL FieldAcmdTCB_AnimeWaitEndInit11Se(FieldActor *actor);
static BOOL FieldAcmdTCB_AnimeWaitEndStop(FieldActor *actor);
static BOOL FieldAcmdTCB_AnimeWaitEnd(FieldActor *actor);
static BOOL FieldAcmdTCB_HideOffInit(FieldActor *actor);
static BOOL FieldAcmdTCB_HideOffMove(FieldActor *actor);
static void FieldAcmdTCB_StayInit(FieldActor *actor, u32 value, u16 limit);
static BOOL FieldAcmdTCB_StayMove(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalk32F40F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalk16F41F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayIdle8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkBobInit(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalkBobMove(FieldActor *actor);

static void FieldAcmdTCB_StepInit(FieldActor *actor, const VecFx32 *delta, u32 faceDir, u32 motionDir, u32 value,
                                  u8 param);

static BOOL FieldAcmdTCB_StepMove(FieldActor *actor);
static BOOL FieldAcmdTCB_StepUpFaceLeft8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepDownFaceRight8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepLeftFaceDown8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepRightFaceUp8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepUpFaceRight8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepDownFaceLeft8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepLeftFaceUp8F(FieldActor *actor);
static BOOL func_ov036_021904f4(FieldActor *actor);
static BOOL FieldAcmdTCB_StepUpFaceDown8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepDownFaceUp8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepLeftFaceRight8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepRightFaceLeft8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepUpFaceDown4F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepDownFaceUp4F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepLeftFaceRight4F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepRightFaceLeft4F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepUpFaceDown2F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepDownFaceUp2F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepLeftFaceRight2F(FieldActor *actor);
static BOOL FieldAcmdTCB_StepRightFaceLeft2F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashStepUpFaceLeft4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashStepDownFaceRight4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashStepLeftFaceDown4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashStepRightFaceUp4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashStepUpFaceRight4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashStepDownFaceLeft4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashStepLeftFaceUp4F(FieldActor *actor);
static BOOL func_ov036_021908e4(FieldActor *actor);
static BOOL FieldAcmdTCB_DashStepUpFaceDown4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashStepDownFaceUp4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashStepLeftFaceRight4F(FieldActor *actor);
static BOOL FieldAcmdTCB_DashStepRightFaceLeft4F(FieldActor *actor);

static void FieldAcmdTCB_HopInit(FieldActor *actor, s32 step, u32 faceDir, u32 motionDir, u8 frames, u8 anim, u8 axis,
                                 u8 bobAxis, u8 bobNegate);

static BOOL FieldAcmdTCB_HopMove(FieldActor *actor);
static BOOL FieldAcmdTCB_HopUpFaceLeft8F(FieldActor *actor);
static BOOL FieldAcmdTCB_HopDownFaceRight8F(FieldActor *actor);
static BOOL FieldAcmdTCB_HopLeftFaceDown8F(FieldActor *actor);
static BOOL FieldAcmdTCB_HopRightFaceUp8F(FieldActor *actor);
static BOOL FieldAcmdTCB_HopUpFaceRight8F(FieldActor *actor);
static BOOL FieldAcmdTCB_HopDownFaceLeft8F(FieldActor *actor);
static BOOL FieldAcmdTCB_HopLeftFaceUp8F(FieldActor *actor);
static BOOL func_ov036_02190d60(FieldActor *actor);
static BOOL FieldAcmdTCB_HopUpFaceDown8F(FieldActor *actor);
static BOOL FieldAcmdTCB_HopDownFaceUp8F(FieldActor *actor);
static BOOL FieldAcmdTCB_HopLeftFaceRight8F(FieldActor *actor);
static BOOL FieldAcmdTCB_HopRightFaceLeft8F(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalk22FInit(FieldActor *actor);
static BOOL FieldAcmdTCB_StayWalk22FMove(FieldActor *actor);
static BOOL FieldAcmdTCB_SetAnime10(FieldActor *actor);
static BOOL FieldAcmdTCB_DashLeft2F(FieldActor *actor);
static BOOL FieldAcmdTCB_Dash2FMove(FieldActor *actor);
static BOOL FieldAcmdTCB_FlyUpInit(FieldActor *actor);
static BOOL FieldAcmdTCB_FlyUpMove(FieldActor *actor);
static BOOL func_ov036_0219100c(FieldActor *actor);
static BOOL FieldAcmdTCB_AnimeWaitEndFaceDownInit(FieldActor *actor);
static BOOL FieldAcmdTCB_AnimeWaitEndFaceDown(FieldActor *actor);
static BOOL FieldAcmdTCB_ResetDrawOffset(FieldActor *actor);
static BOOL FieldAcmdTCB_FlyReadyInit(FieldActor *actor);
static BOOL FieldAcmdTCB_FlyReadyMove(FieldActor *actor);
static BOOL func_ov036_02191174(FieldActor *actor);
static BOOL FieldAcmdTCB_DrawReadyWaitInit10(FieldActor *actor);
static BOOL FieldAcmdTCB_DrawReadyWaitInit11(FieldActor *actor);
static BOOL FieldAcmdTCB_DrawReadyWaitInit12(FieldActor *actor);
static BOOL FieldAcmdTCB_DrawReadyWaitInit13(FieldActor *actor);
static BOOL FieldAcmdTCB_DrawReadyWaitStop(FieldActor *actor);
static BOOL FieldAcmdTCB_DrawReadyWait(FieldActor *actor);

static BOOL (*const ACMD_TCB_FUNCS[])(FieldAcmdWork *work) = { FieldAcmdTCB_Begin,        FieldAcmdTCB_WaitAcmdReady,
                                                               FieldAcmdTCB_CommitAcmd,   FieldAcmdTCB_CheckSeqPerms,
                                                               FieldAcmdTCB_AdvanceQueue, FieldAcmdTCB_End };

static const s32 data_ov036_021cb5b0[];
static const s32 data_ov036_021cb5d4[];
static const s32 data_ov036_021cb5ec[];
static const s32 data_ov036_021cb608[];
static const fx32 *data_ov036_021d4d34[];

void FieldAcmdTCB_DeleteDrawSys(MMSys *system) {
    if (FldActSys_GetFieldBlAct(system) != NULL) {
        FldBlAct_Exit(system);
    }
    if (FldActSys_GetActorG3DSystem(system) != NULL) {
        FieldActorG3DSystem_Delete(system);
    }
    FldActSys_SetDrawInited(system, FALSE);
}

void FieldAcmdTCB_DrawIfInitialized(FieldActor *actor) {
    if (!CheckMMSysFlag(GetActorMModelSystem(actor), 4) && FldAct_IsDrawCreated(actor)) {
        FldAct_InvokeUpdateCallback(actor);
    }
}

BOOL FieldAcmdTCB_CheckAnimePaused(FieldActor *actor) {
    if (IsActorMovementDisabled(actor) && !FldAct_CheckAcmdSet(actor)) {
        return TRUE;
    }
    if (CheckActorFlag(actor, 0x10)) {
        return TRUE;
    }
    return FALSE;
}

void CopyActorPosAllAdd(FieldActor *actor, VecFx32 *dest) {
    VecFx32 wPos;
    VecFx32 pos1;
    VecFx32 pos2;
    VecFx32 pos3;
    VecFx32 offset;

    CopyActorWPos(actor, &wPos);
    CopyActorWPosOffset(actor, &pos1);
    FldAct_GetOuterDrawOffset(actor, &pos2);
    FldAct_GetAttrDrawOffset(actor, &pos3);
    CopyActorPosOffset(actor, &offset);
    dest->x = wPos.x + pos1.x + pos2.x + pos3.x + offset.x;
    dest->y = wPos.y + pos1.y + pos2.y + pos3.y + offset.y;
    dest->z = wPos.z + pos1.z + pos2.z + pos3.z + offset.z;
}

void FieldAcmdSyncBillboardPos(FieldActor *actor) {
    VecFx32 pos;
    u32 idx = FldAct_GetBlActIdx(actor, 0);

    if (idx != 0xffff) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));

        CopyActorPosAllAdd(actor, &pos);
        pos.y += Field_GetActorYOffset(GetMMSysField(GetActorMModelSystem(actor)));
        BlActScene_SetActorPos(BlActSys_GetScene(sys), idx, &pos);
    }
}

FieldAcmdTCB *FieldAcmdTCB_Create(FieldActor *actor, const u32 *action) {
    MMSys *system = GetActorMModelSystem(actor);
    FieldAcmdWork *work = GFL_HeapAllocate(HEAPID_TAIL(FldActSys_GetFieldHeapID(system)), sizeof(FieldAcmdWork), TRUE,
                                           "fldmmdl_acmd.c", 278);
    u32 priority = FldActSys_GetTaskPriority(GetActorMModelSystem(actor)) - 2;
    TCB *tcb = GFL_TCBMgrAddTask(GetMMSysTCBMgr(system), FieldAcmdTCB_Callback, work, priority);

    work->actor = actor;
    work->cursor = (const FieldAcmdEntry *)action;
    work->queue = action;
    return (FieldAcmdTCB *)tcb;
}

BOOL FieldAcmdTCB_CheckEnded(FieldAcmdTCB *task) {
    FieldAcmdWork *work = GFL_TCBGetData((TCB *)task);

    return work->ended;
}

void FieldAcmdTCB_Remove(FieldAcmdTCB *task) {
    FieldAcmdWork *work = GFL_TCBGetData((TCB *)task);

    FldAct_EndAcmd(work->actor);
    if (work->queue != NULL) {
        GFL_HeapFree((void *)work->queue);
    }
    GFL_HeapFree(work);
    GFL_TCBRemove((TCB *)task);
}

static void FieldAcmdTCB_Callback(TCB *tcb, void *data) {
    FieldAcmdWork *work = data;

    while (ACMD_TCB_FUNCS[work->state](work) == TRUE) {
    }
}

static BOOL FieldAcmdTCB_Begin(FieldAcmdWork *work) {
    work->repeat = 0;
    work->state = ACMD_TCB_STATE_WAIT_ACMD_READY;
    return TRUE;
}

static BOOL FieldAcmdTCB_WaitAcmdReady(FieldAcmdWork *work) {
    if (!IsAllActorAcmdFinished(work->actor)) {
        return FALSE;
    }
    work->state = ACMD_TCB_STATE_COMMIT_ACMD;
    return TRUE;
}

static BOOL FieldAcmdTCB_CommitAcmd(FieldAcmdWork *work) {
    FldAct_SetAcmd(work->actor, work->cursor->code);
    work->state = ACMD_TCB_STATE_CHECK_SEQ_PERMS;
    return FALSE;
}

static BOOL FieldAcmdTCB_CheckSeqPerms(FieldAcmdWork *work) {
    if (!FldAct_CheckAcmdEnd(work->actor)) {
        return FALSE;
    }
    work->state = ACMD_TCB_STATE_ADVANCE_QUEUE;
    return TRUE;
}

static BOOL FieldAcmdTCB_AdvanceQueue(FieldAcmdWork *work) {
    const FieldAcmdEntry *cursor = work->cursor;

    work->repeat++;
    if (work->repeat < cursor->count) {
        work->state = ACMD_TCB_STATE_WAIT_ACMD_READY;
        return TRUE;
    }
    work->cursor = cursor + 1;
    if (cursor[1].code == 0xfd) {
        work->ended = TRUE;
        work->state = ACMD_TCB_STATE_END;
        return FALSE;
    }
    if (cursor[1].code != ACMD_END) {
        work->state = ACMD_TCB_STATE_BEGIN;
        return TRUE;
    }
    work->queue = NULL;
    work->ended = TRUE;
    work->state = ACMD_TCB_STATE_END;
    return FALSE;
}

static BOOL FieldAcmdTCB_End(FieldAcmdWork *work) {
    return FALSE;
}

u16 GetAcmdForDir(u16 dir, u16 acmd) {
    int j;
    const u32 *const *lut = ACMD_DIR_LUT;
    const u32 *table;

    do {
        table = *lut;
        for (j = 0; j < 4; j++) {
            if (acmd == table[j]) {
                return table[dir];
            }
        }
        lut++;
    } while (*lut != NULL);
    return acmd;
}

void FieldAcmdTCB_RunCmds(FieldActor *actor) {
    u16 acmd;

    do {
        acmd = FldAct_GetAcmd(actor);
        if (acmd == 0xff) {
            break;
        }
    } while (FldAct_ExecAcmd(actor, acmd, FldAct_GetAcmdState(actor)));
}

BOOL FieldAcmdTCB_Update(FieldActor *actor) {
    if (FldAct_GetAcmd(actor) != 0xff) {
        FieldAcmdTCB_RunCmds(actor);
        if (!CheckActorMovementFlag(actor, 0x400)) {
            return FALSE;
        }
    }
    ClearActorMovementFlag(actor, 0x400);
    SetNextActorAcmd(actor, 0xff);
    FldAct_SetAcmdState(actor, 0);
    return TRUE;
}

static BOOL FldAct_ExecAcmd(FieldActor *actor, u32 acmd, u32 state) {
    if (CheckActorFlag(actor, 0x2000)) {
        return ACTOR_ACTION_COMMAND_TABLES_RAIL[acmd][state](actor);
    }
    return ACTOR_ACTION_COMMAND_TABLES[acmd][state](actor);
}

static BOOL FieldAcmdTCB_SetAcmdEnd(FieldActor *actor) {
    SetActorMovementFlag(actor, 0x400);
    return FALSE;
}

static void FieldAcmdTCB_FaceInit(FieldActor *actor, u32 dir) {
    CheckSetActorFaceDir(actor, dir);
    FldAct_SetDrawStatus(actor, 0);
    SetActorInitialGPosToNowGPos(actor);
    FieldActor_NextAcmdState(actor);
}

static BOOL FieldAcmdTCB_FaceUp(FieldActor *actor) {
    FieldAcmdTCB_FaceInit(actor, DIR_UP);
    return TRUE;
}

static BOOL FieldAcmdTCB_FaceDown(FieldActor *actor) {
    FieldAcmdTCB_FaceInit(actor, DIR_DOWN);
    return TRUE;
}

static BOOL FieldAcmdTCB_FaceLeft(FieldActor *actor) {
    FieldAcmdTCB_FaceInit(actor, DIR_LEFT);
    return TRUE;
}

static BOOL FieldAcmdTCB_FaceRight(FieldActor *actor) {
    FieldAcmdTCB_FaceInit(actor, DIR_RIGHT);
    return TRUE;
}

static void FieldAcmdTCB_WalkInit(FieldActor *actor, u32 dir, fx32 speed, s16 frames, u16 state) {
    FieldAcmdWalkWork *work = FldAct_InitMoveCmdWork(actor, sizeof(FieldAcmdWalkWork));

    work->state = state;
    work->frames = frames;
    work->dir = dir;
    work->speed = speed;
    FldAct_StepGPosInDir(actor, dir);
    ChangeActorDirection(actor, dir);
    FldAct_SetDrawStatus(actor, state);
    SetActorMovementFlag(actor, 0x20);
    FieldActor_NextAcmdState(actor);
}

static BOOL FieldAcmdTCB_WalkMove(FieldActor *actor) {
    FieldAcmdWalkWork *work = FldAct_GetMoveCmdWork(actor);

    FldAct_MoveWPosInDir(actor, work->dir, work->speed);
    CheckRecalcActorY(actor);
    work->frames--;
    if (work->frames > 0) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x440);
    SetActorInitialGPosToNowGPos(actor);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_WalkUp32F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_UP, 0x800, 32, 1);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkDown32F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_DOWN, 0x800, 32, 1);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkLeft32F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_LEFT, 0x800, 32, 1);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkRight32F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_RIGHT, 0x800, 32, 1);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkUp16F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_UP, 0x1000, 16, 2);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkDown16F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_DOWN, 0x1000, 16, 2);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkLeft16F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_LEFT, 0x1000, 16, 2);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkRight16F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_RIGHT, 0x1000, 16, 2);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkUp8F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_UP, 0x2000, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkDown8F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_DOWN, 0x2000, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkLeft8F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_LEFT, 0x2000, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkRight8F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_RIGHT, 0x2000, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkUp4F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_UP, 0x4000, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkDown4F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_DOWN, 0x4000, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkLeft4F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_LEFT, 0x4000, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkRight4F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_RIGHT, 0x4000, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkUp2F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_UP, 0x8000, 2, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkDown2F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_DOWN, 0x8000, 2, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkLeft2F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_LEFT, 0x8000, 2, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkRight2F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_RIGHT, 0x8000, 2, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkUp1F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_UP, 0x10000, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkDown1F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_DOWN, 0x10000, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkLeft1F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_LEFT, 0x10000, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkRight1F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_RIGHT, 0x10000, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashUp4F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_UP, 0x4000, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashDown4F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_DOWN, 0x4000, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashLeft4F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_LEFT, 0x4000, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashRight4F(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_RIGHT, 0x4000, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkUp16FFastAnime(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_UP, 0x1000, 16, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkDown16FFastAnime(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_DOWN, 0x1000, 16, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkLeft16FFastAnime(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_LEFT, 0x1000, 16, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkRight16FFastAnime(FieldActor *actor) {
    FieldAcmdTCB_WalkInit(actor, DIR_RIGHT, 0x1000, 16, 5);
    return TRUE;
}

static void FieldAcmdTCB_StayWalkInit(FieldActor *actor, u32 dir, s16 frames, u16 state) {
    FieldAcmdWalkWork *work = FldAct_InitMoveCmdWork(actor, sizeof(FieldAcmdWalkWork));

    work->state = state;
    work->frames = frames;
    CheckSetActorFaceDir(actor, dir);
    FldAct_SetDrawStatus(actor, state);
    SetActorInitialGPosToNowGPos(actor);
    FieldActor_NextAcmdState(actor);
}

static BOOL FieldAcmdTCB_StayWalkMove(FieldActor *actor) {
    FieldAcmdWalkWork *work = FldAct_GetMoveCmdWork(actor);

    work->frames--;
    if (work->frames > 0) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x400);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_StayWalkUp32F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_UP, 32, 1);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkDown32F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_DOWN, 32, 1);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkLeft32F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_LEFT, 32, 1);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkRight32F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_RIGHT, 32, 1);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkUp16F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_UP, 16, 2);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkDown16F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_DOWN, 16, 2);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkLeft16F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_LEFT, 16, 2);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkRight16F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_RIGHT, 16, 2);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkUp8F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_UP, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkDown8F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_DOWN, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkLeft8F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_LEFT, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkRight8F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_RIGHT, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkUp4F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_UP, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkDown4F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_DOWN, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkLeft4F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_LEFT, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkRight4F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_RIGHT, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkUp2F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_UP, 2, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkDown2F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_DOWN, 2, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkLeft2F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_LEFT, 2, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkRight2F(FieldActor *actor) {
    FieldAcmdTCB_StayWalkInit(actor, DIR_RIGHT, 2, 5);
    return TRUE;
}

static void FieldAcmdTCB_JumpInit(FieldActor *actor, u32 dir, fx32 speed, u32 frames, u16 state, s16 heightTable,
                                  u16 heightStep, u32 se) {
    FieldAcmdJumpWork *work = FldAct_InitMoveCmdWork(actor, sizeof(FieldAcmdJumpWork));

    work->dir = dir;
    work->speed = speed;
    work->frames = frames;
    work->state = state;
    work->heightTable = heightTable;
    work->heightStep = heightStep;
    if (speed == 0) {
        SetActorInitialGPosToNowGPos(actor);
    } else {
        FldAct_StepGPosInDir(actor, dir);
    }
    SetActorMovementFlag(actor, 0xa0);
    ChangeActorDirection(actor, dir);
    FldAct_SetDrawStatus(actor, state);
    FieldActor_NextAcmdState(actor);
    if (se != 0) {
        GFL_SndSEPlay(se);
    }
}

static void FieldAcmdTCB_JumpInitWithSe(FieldActor *actor, u32 dir, fx32 speed, u32 frames, u16 state, s16 heightTable,
                                        u16 heightStep) {
    FieldAcmdTCB_JumpInit(actor, dir, speed, frames, state, heightTable, heightStep, SEQ_SE_DANSA);
}

static BOOL FieldAcmdTCB_JumpMove(FieldActor *actor) {
    FieldAcmdJumpWork *work = FldAct_GetMoveCmdWork(actor);
    VecFx32 offset;
    fx32 speed;
    const fx32 *row;
    u16 index;

    if (work->speed != 0) {
        FldAct_MoveWPosInDir(actor, work->dir, work->speed);
        CheckRecalcActorY(actor);
        if (work->progress >= 0x10000) {
            work->progress = 0;
            FldAct_StepGPosInDir(actor, work->dir);
            SetActorMovementFlag(actor, 0x20);
        }
        speed = work->speed;
        if (speed < 0) {
            speed = -speed;
        }
        work->progress += speed;
    }
    work->height += work->heightStep;
    if (work->height > 0xf00) {
        work->height = 0xf00;
    }
    index = work->height / 256;
    row = data_ov036_021d4d34[work->heightTable];
    offset.x = 0;
    offset.y = row[index];
    offset.z = 0;
    SetActorWPosOffset(actor, &offset);
    work->frames--;
    if (work->frames > 0) {
        return FALSE;
    }
    VecFx32 zero = { 0, 0, 0 };
    SetActorWPosOffset(actor, &zero);
    SetActorMovementFlag(actor, 0x540);
    SetActorInitialGPosToNowGPos(actor);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    GFL_SndSEPlay(SEQ_SE_FLD_10);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayJumpUp16F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_UP, 0x0, 16, 2, 1, 0x100);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayJumpDown16F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_DOWN, 0x0, 16, 2, 1, 0x100);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayJumpLeft16F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_LEFT, 0x0, 16, 2, 1, 0x100);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayJumpRight16F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_RIGHT, 0x0, 16, 2, 1, 0x100);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayJumpUp8F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_UP, 0x0, 8, 3, 1, 0x200);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayJumpDown8F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_DOWN, 0x0, 8, 3, 1, 0x200);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayJumpLeft8F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_LEFT, 0x0, 8, 3, 1, 0x200);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayJumpRight8F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_RIGHT, 0x0, 8, 3, 1, 0x200);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpUp1G8F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_UP, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpDown1G8F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_DOWN, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpLeft1G8F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_LEFT, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpRight1G8F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_RIGHT, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpUp2G16F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_UP, 0x2000, 16, 3, 0, 0x100);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpDown2G16F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_DOWN, 0x2000, 16, 3, 0, 0x100);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpLeft2G16F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_LEFT, 0x2000, 16, 3, 0, 0x100);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpRight2G16F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_RIGHT, 0x2000, 16, 3, 0, 0x100);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpLeft1G16F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_LEFT, 0x1000, 16, 2, 0, 0xf0);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpRight1G16F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_RIGHT, 0x1000, 16, 2, 0, 0xf0);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpLeft3G12F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_LEFT, 0x4000, 12, 3, 0, 0x140);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpRight3G12F(FieldActor *actor) {
    FieldAcmdTCB_JumpInitWithSe(actor, DIR_RIGHT, 0x4000, 12, 3, 0, 0x140);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpUp3G24FNoSe(FieldActor *actor) {
    FieldAcmdTCB_JumpInit(actor, DIR_UP, 0x2000, 24, 3, 0, 0xa0, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpDown3G24FNoSe(FieldActor *actor) {
    FieldAcmdTCB_JumpInit(actor, DIR_DOWN, 0x2000, 24, 3, 0, 0xa0, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpLeft3G24FNoSe(FieldActor *actor) {
    FieldAcmdTCB_JumpInit(actor, DIR_LEFT, 0x2000, 24, 3, 0, 0xa0, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_JumpRight3G24FNoSe(FieldActor *actor) {
    FieldAcmdTCB_JumpInit(actor, DIR_RIGHT, 0x2000, 24, 3, 0, 0xa0, 0);
    return TRUE;
}

static void FieldAcmdTCB_WaitInit(FieldActor *actor, u32 count) {
    u32 *work = FldAct_InitMoveCmdWork(actor, 4);

    *work = count;
    FieldActor_NextAcmdState(actor);
}

static BOOL FieldAcmdTCB_WaitMove(FieldActor *actor) {
    u32 *work = FldAct_GetMoveCmdWork(actor);

    if (*work != 0) {
        (*work)--;
        return FALSE;
    }
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_Wait1F(FieldActor *actor) {
    FieldAcmdTCB_WaitInit(actor, 1);
    return TRUE;
}

static BOOL FieldAcmdTCB_Wait2F(FieldActor *actor) {
    FieldAcmdTCB_WaitInit(actor, 2);
    return TRUE;
}

static BOOL FieldAcmdTCB_Wait4F(FieldActor *actor) {
    FieldAcmdTCB_WaitInit(actor, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_Wait8F(FieldActor *actor) {
    FieldAcmdTCB_WaitInit(actor, 8);
    return TRUE;
}

static BOOL FieldAcmdTCB_Wait15F(FieldActor *actor) {
    FieldAcmdTCB_WaitInit(actor, 15);
    return TRUE;
}

static BOOL FieldAcmdTCB_Wait16F(FieldActor *actor) {
    FieldAcmdTCB_WaitInit(actor, 16);
    return TRUE;
}

static BOOL FieldAcmdTCB_Wait32F(FieldActor *actor) {
    FieldAcmdTCB_WaitInit(actor, 32);
    return TRUE;
}

static BOOL FieldAcmdTCB_RiseInit(FieldActor *actor) {
    AcmdPosSpeedWork *work = FldAct_InitMoveCmdWork(actor, 8);

    work->speed = 8 << 13;
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_RiseMove(FieldActor *actor) {
    AcmdPosSpeedWork *work = FldAct_GetMoveCmdWork(actor);

    work->pos += work->speed;
    VecFx32 offset = { 0, 0, 0 };

    offset.y = work->pos;
    SetActorWPosOffset(actor, &offset);
    if (work->pos / 0x8000 < 40) {
        return FALSE;
    }
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_FallInit(FieldActor *actor) {
    AcmdPosSpeedWork *work = FldAct_InitMoveCmdWork(actor, 8);

    work->pos = 5 << 18;
    work->speed = -0x10000;
    GFL_SndSEPlay(SEQ_SE_FLD_05);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_FallMove(FieldActor *actor) {
    AcmdPosSpeedWork *work = FldAct_GetMoveCmdWork(actor);

    work->pos += work->speed;
    if (work->pos < 0) {
        work->pos = 0;
    }
    VecFx32 offset = { 0, 0, 0 };

    offset.y = work->pos;
    SetActorWPosOffset(actor, &offset);
    if (work->pos > 0) {
        return FALSE;
    }
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_SetHiddenFlag(FieldActor *actor) {
    SetActorFlag(actor, 4);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_ClearHiddenFlag(FieldActor *actor) {
    ClearActorFlag(actor, 4);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_SetFaceDirLockFlag(FieldActor *actor) {
    SetActorFlag(actor, 8);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_ClearFaceDirLockFlag(FieldActor *actor) {
    ClearActorFlag(actor, 8);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_SetAnimePauseFlag(FieldActor *actor) {
    SetActorFlag(actor, 16);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_ClearAnimePauseFlag(FieldActor *actor) {
    ClearActorFlag(actor, 16);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static void FieldAcmdTCB_MarkWaitInit(FieldActor *actor, u32 kind, u32 unused, BOOL playSE) {
    AcmdEffectWork *work = FldAct_InitMoveCmdWork(actor, 8);

    work->kind = kind;
    work->task =
        FldeffGyoe_SetMMdl(Field_GetFieldEffects(GetMMSysField(GetActorMModelSystem(actor))), actor, kind, playSE);
    FieldActor_NextAcmdState(actor);
}

static BOOL FieldAcmdTCB_MarkWait(FieldActor *actor) {
    AcmdEffectWork *work = FldAct_GetMoveCmdWork(actor);

    if (FldeffGyoe_IsFinished(work->task) == TRUE) {
        FieldEffects_DeleteObject(work->task);
        FieldActor_NextAcmdState(actor);
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldAcmdTCB_MarkWaitExcl(FieldActor *actor) {
    FieldAcmdTCB_MarkWaitInit(actor, 0, 0, TRUE);
    return FALSE;
}

static BOOL FieldAcmdTCB_MarkWaitExclNoSe(FieldActor *actor) {
    FieldAcmdTCB_MarkWaitInit(actor, 0, 0, FALSE);
    return FALSE;
}

static BOOL FieldAcmdTCB_MarkWaitNoteNoSe(FieldActor *actor) {
    FieldAcmdTCB_MarkWaitInit(actor, 2, 0, FALSE);
    return FALSE;
}

static BOOL func_ov036_0218fd18(FieldActor *actor) {
    FieldAcmdTCB_MarkWaitInit(actor, 0, 0, TRUE);
    return FALSE;
}

static BOOL func_ov036_0218fd28(FieldActor *actor) {
    FieldAcmdTCB_MarkWaitInit(actor, 0, 0, TRUE);
    return FALSE;
}

static BOOL FieldAcmdTCB_MarkWaitQuestion(FieldActor *actor) {
    FieldAcmdTCB_MarkWaitInit(actor, 1, 0, TRUE);
    return FALSE;
}

static BOOL FieldAcmdTCB_MarkWaitNote(FieldActor *actor) {
    FieldAcmdTCB_MarkWaitInit(actor, 2, 0, TRUE);
    return FALSE;
}

static BOOL FieldAcmdTCB_MarkWaitEllipsis(FieldActor *actor) {
    FieldAcmdTCB_MarkWaitInit(actor, 3, 0, TRUE);
    return FALSE;
}

static void FieldAcmdTCB_WalkTableInit(FieldActor *actor, u32 dir, u32 steps, u32 state) {
    AcmdDirStepsWork *work = FldAct_InitMoveCmdWork(actor, sizeof(AcmdDirStepsWork));

    work->dir = dir;
    work->state = state;
    work->steps = steps;
    FldAct_StepGPosInDir(actor, dir);
    ChangeActorDirection(actor, dir);
    FldAct_SetDrawStatus(actor, state);
    SetActorMovementFlag(actor, 0x20);
    FieldActor_NextAcmdState(actor);
}

static BOOL FieldAcmdTCB_WalkTableMove(FieldActor *actor, const s32 *distances) {
    AcmdDirStepsWork *work = FldAct_GetMoveCmdWork(actor);

    FldAct_MoveWPosInDir(actor, work->dir, distances[work->index]);
    CheckRecalcActorY(actor);
    work->index++;
    if (work->index < work->steps) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x440);
    SetActorInitialGPosToNowGPos(actor);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkUp6F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_UP, 6, 6);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkDown6F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_DOWN, 6, 6);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkLeft6F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_LEFT, 6, 6);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkRight6F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_RIGHT, 6, 6);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashUp6F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_UP, 6, 11);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashDown6F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_DOWN, 6, 11);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashLeft6F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_LEFT, 6, 11);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashRight6F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_RIGHT, 6, 11);
    return TRUE;
}

static BOOL FieldAcmdTCB_Walk6FMove(FieldActor *actor) {
    if (FieldAcmdTCB_WalkTableMove(actor, data_ov036_021cb5d4) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldAcmdTCB_WalkUp3F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_UP, 3, 7);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkDown3F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_DOWN, 3, 7);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkLeft3F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_LEFT, 3, 7);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkRight3F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_RIGHT, 3, 7);
    return TRUE;
}

static BOOL FieldAcmdTCB_Walk3FMove(FieldActor *actor) {
    if (FieldAcmdTCB_WalkTableMove(actor, data_ov036_021cb5b0) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldAcmdTCB_WalkUp7F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_UP, 7, 8);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkDown7F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_DOWN, 7, 8);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkLeft7F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_LEFT, 7, 8);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkRight7F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_RIGHT, 7, 8);
    return TRUE;
}

static BOOL FieldAcmdTCB_Walk7FMove(FieldActor *actor) {
    if (FieldAcmdTCB_WalkTableMove(actor, data_ov036_021cb5ec) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldAcmdTCB_WalkUp12F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_UP, 12, 9);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkDown12F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_DOWN, 12, 9);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkLeft12F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_LEFT, 12, 9);
    return TRUE;
}

static BOOL FieldAcmdTCB_WalkRight12F(FieldActor *actor) {
    FieldAcmdTCB_WalkTableInit(actor, DIR_RIGHT, 12, 9);
    return TRUE;
}

static BOOL FieldAcmdTCB_Walk12FMove(FieldActor *actor) {
    if (FieldAcmdTCB_WalkTableMove(actor, data_ov036_021cb608) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldAcmdTCB_AnimeWaitEndInit10(FieldActor *actor) {
    FldAct_InitMoveCmdWork(actor, 4);
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_AnimeWaitEndInit11(FieldActor *actor) {
    FldAct_InitMoveCmdWork(actor, 4);
    FldAct_SetDrawStatus(actor, 11);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_AnimeWaitEndInit12(FieldActor *actor) {
    FldAct_InitMoveCmdWork(actor, 4);
    FldAct_SetDrawStatus(actor, 12);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_AnimeWaitEndInit10Se(FieldActor *actor) {
    GFL_SndSEPlay(SEQ_SE_FLD_152);
    return FieldAcmdTCB_AnimeWaitEndInit10(actor);
}

static BOOL FieldAcmdTCB_AnimeWaitEndInit11Se(FieldActor *actor) {
    GFL_SndSEPlay(SEQ_SE_FLD_152);
    return FieldAcmdTCB_AnimeWaitEndInit11(actor);
}

static BOOL FieldAcmdTCB_AnimeWaitEndStop(FieldActor *actor) {
    u16 cmd;
    u32 index;

    FldAct_GetMoveCmdWork(actor);
    index = FldAct_GetBlActIdx(actor, 0);
    if (index != 0xffff) {
        if (BlActSys_GetActorLastCmd(FieldBlAct_GetBlActSys(FldActSys_GetFieldBlAct(GetActorMModelSystem(actor))),
                                     (u16)index, &cmd) == TRUE) {
            FldAct_SetDrawStatus(actor, 0);
            FieldActor_NextAcmdState(actor);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FieldAcmdTCB_AnimeWaitEnd(FieldActor *actor) {
    u16 cmd;
    u32 index;

    FldAct_GetMoveCmdWork(actor);
    index = FldAct_GetBlActIdx(actor, 0);
    if (index != 0xffff) {
        if (BlActSys_GetActorLastCmd(FieldBlAct_GetBlActSys(FldActSys_GetFieldBlAct(GetActorMModelSystem(actor))),
                                     (u16)index, &cmd) == TRUE) {
            FieldActor_NextAcmdState(actor);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FieldAcmdTCB_HideOffInit(FieldActor *actor) {
    FieldEffectTask *task;

    FldAct_InitMoveCmdWork(actor, 4);
    task = FldmmdlMove_GetHideEffect(actor);
    if (task != NULL) {
        HideTask_RequestEnd(task);
    }
    VecFx32 offset = { 0, 0, 0 };

    SetActorWPosOffset(actor, &offset);
    SetActorMovementFlag(actor, 0xa0);
    ClearActorMovementFlag(actor, 0x8000);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_HideOffMove(FieldActor *actor) {
    u32 *work = FldAct_GetMoveCmdWork(actor);
    VecFx32 offset = { 0, 0, 0 };
    FieldEffectTask *task;

    offset.y = data_ov036_021d4d34[0][*work];
    SetActorWPosOffset(actor, &offset);
    task = FldmmdlMove_GetHideEffect(actor);
    if (task != NULL) {
        if (HideTask_IsFinished(task) == TRUE) {
            FieldEffects_DeleteObject(task);
            FldmmdlMove_SetHideEffect(actor, 0);
        }
    }
    *work += 2;
    if (*work < 16) {
        return FALSE;
    }
    offset.y = 0;
    SetActorWPosOffset(actor, &offset);
    SetActorMovementFlag(actor, 0x540);
    FldmmdlMove_SetHideNoEffect(actor);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static void FieldAcmdTCB_StayInit(FieldActor *actor, u32 value, u16 limit) {
    AcmdWaitWork *work = FldAct_InitMoveCmdWork(actor, 4);

    work->limit = limit;
    FldAct_SetDrawStatus(actor, value);
}

static BOOL FieldAcmdTCB_StayMove(FieldActor *actor) {
    AcmdWaitWork *work = FldAct_GetMoveCmdWork(actor);

    work->count++;
    if (work->count >= work->limit) {
        FieldActor_NextAcmdState(actor);
    }
    return FALSE;
}

static BOOL FieldAcmdTCB_StayWalk32F40F(FieldActor *actor) {
    FieldAcmdTCB_StayInit(actor, 1, 40);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_StayWalk16F41F(FieldActor *actor) {
    FieldAcmdTCB_StayInit(actor, 2, 41);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_StayIdle8F(FieldActor *actor) {
    FieldAcmdTCB_StayInit(actor, 0, 8);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_StayWalkBobInit(FieldActor *actor) {
    FldAct_InitMoveCmdWork(actor, 4);
    FldAct_SetDrawStatus(actor, 1);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalkBobMove(FieldActor *actor) {
    VecFx32 offset = { 0, 0, 0 };
    s32 heights[10] = { 0, 0, 0, 0, 0, 0x4000, 0x4000, 0x3000, 0, 0 };
    s32 *work;
    s32 index;

    work = FldAct_GetMoveCmdWork(actor);
    index = *work;
    if (index >= 10) {
        index = 9;
    }
    offset.y = heights[index];
    SetActorWPosOffset(actor, &offset);
    (*work)++;
    if (*work >= 9) {
        FieldActor_NextAcmdState(actor);
    }
    return FALSE;
}

static void FieldAcmdTCB_StepInit(FieldActor *actor, const VecFx32 *delta, u32 faceDir, u32 motionDir, u32 value,
                                  u8 param) {
    AcmdShiftWork *work = FldAct_InitMoveCmdWork(actor, 16);

    work->frames = value;
    work->delta = *delta;
    CheckSetActorFaceDir(actor, faceDir);
    SetActorMotionDir(actor, motionDir);
    FldAct_SetDrawStatus(actor, param);
    FldAct_SetMoveStart(actor);
    SetActorInitialGPosX(actor, GetGPosX(actor));
    SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
    SetActorInitialGPosZ(actor, GetGPosZ(actor));
    if (delta->x < 0) {
        adjustXPos(actor, -1);
    } else if (delta->x > 0) {
        adjustXPos(actor, 1);
    }
    if (delta->y < 0) {
        adjustZPos(actor, -1);
    } else if (delta->y > 0) {
        adjustZPos(actor, 1);
    }
    if (delta->z < 0) {
        adjustYPos(actor, -1);
    } else if (delta->z > 0) {
        adjustYPos(actor, 1);
    }
    FieldActor_NextAcmdState(actor);
}

static BOOL FieldAcmdTCB_StepMove(FieldActor *actor) {
    AcmdShiftWork *work = FldAct_GetMoveCmdWork(actor);

    FldAct_AddWPos(actor, &work->delta);
    work->frames--;
    if (work->frames > 0) {
        return FALSE;
    }
    SetActorMovementFlag(actor, 0x440);
    SetActorInitialGPosToNowGPos(actor);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepUpFaceLeft8F(FieldActor *actor) {
    VecFx32 delta = { 0, FX32_CONST(2), 0 };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_LEFT, DIR_UP, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepDownFaceRight8F(FieldActor *actor) {
    VecFx32 delta = { 0, FX32_CONST(-2), 0 };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_RIGHT, DIR_DOWN, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepLeftFaceDown8F(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(2) };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_DOWN, DIR_LEFT, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepRightFaceUp8F(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(-2) };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_UP, DIR_RIGHT, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepUpFaceRight8F(FieldActor *actor) {
    VecFx32 delta = { 0, FX32_CONST(2), 0 };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_RIGHT, DIR_UP, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepDownFaceLeft8F(FieldActor *actor) {
    VecFx32 delta = { 0, FX32_CONST(-2), 0 };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_LEFT, DIR_DOWN, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepLeftFaceUp8F(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(-2) };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_UP, DIR_LEFT, 8, 3);
    return TRUE;
}

static BOOL func_ov036_021904f4(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(2) };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_DOWN, DIR_LEFT, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepUpFaceDown8F(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(2) };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_DOWN, DIR_UP, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepDownFaceUp8F(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(-2) };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_UP, DIR_DOWN, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepLeftFaceRight8F(FieldActor *actor) {
    VecFx32 delta = { FX32_CONST(-2), 0, 0 };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_RIGHT, DIR_LEFT, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepRightFaceLeft8F(FieldActor *actor) {
    VecFx32 delta = { FX32_CONST(2), 0, 0 };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_LEFT, DIR_RIGHT, 8, 3);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepUpFaceDown4F(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(4) };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_DOWN, DIR_UP, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepDownFaceUp4F(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(-4) };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_UP, DIR_DOWN, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepLeftFaceRight4F(FieldActor *actor) {
    VecFx32 delta = { FX32_CONST(-4), 0, 0 };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_RIGHT, DIR_LEFT, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepRightFaceLeft4F(FieldActor *actor) {
    VecFx32 delta = { FX32_CONST(4), 0, 0 };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_LEFT, DIR_RIGHT, 4, 4);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepUpFaceDown2F(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(8) };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_DOWN, DIR_UP, 2, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepDownFaceUp2F(FieldActor *actor) {
    VecFx32 delta = { 0, 0, FX32_CONST(-8) };

    FieldAcmdTCB_StepInit(actor, &delta, DIR_UP, DIR_DOWN, 2, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepLeftFaceRight2F(FieldActor *actor) {
    VecFx32 step = { -0x8000, 0, 0 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_RIGHT, DIR_LEFT, 2, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_StepRightFaceLeft2F(FieldActor *actor) {
    VecFx32 step = { 0x8000, 0, 0 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_LEFT, DIR_RIGHT, 2, 5);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashStepUpFaceLeft4F(FieldActor *actor) {
    VecFx32 step = { 0, 0x4000, 0 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_LEFT, DIR_UP, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashStepDownFaceRight4F(FieldActor *actor) {
    VecFx32 step = { 0, -0x4000, 0 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_RIGHT, DIR_DOWN, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashStepLeftFaceDown4F(FieldActor *actor) {
    VecFx32 step = { 0, 0, 0x4000 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_DOWN, DIR_LEFT, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashStepRightFaceUp4F(FieldActor *actor) {
    VecFx32 step = { 0, 0, -0x4000 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_UP, DIR_RIGHT, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashStepUpFaceRight4F(FieldActor *actor) {
    VecFx32 step = { 0, 0x4000, 0 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_RIGHT, DIR_UP, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashStepDownFaceLeft4F(FieldActor *actor) {
    VecFx32 step = { 0, -0x4000, 0 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_LEFT, DIR_DOWN, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashStepLeftFaceUp4F(FieldActor *actor) {
    VecFx32 step = { 0, 0, -0x4000 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_UP, DIR_LEFT, 4, 10);
    return TRUE;
}

static BOOL func_ov036_021908e4(FieldActor *actor) {
    VecFx32 step = { 0, 0, 0x4000 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_DOWN, DIR_LEFT, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashStepUpFaceDown4F(FieldActor *actor) {
    VecFx32 step = { 0, 0, 0x4000 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_DOWN, DIR_UP, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashStepDownFaceUp4F(FieldActor *actor) {
    VecFx32 step = { 0, 0, -0x4000 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_UP, DIR_DOWN, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashStepLeftFaceRight4F(FieldActor *actor) {
    VecFx32 step = { -0x4000, 0, 0 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_RIGHT, DIR_LEFT, 4, 10);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashStepRightFaceLeft4F(FieldActor *actor) {
    VecFx32 step = { 0x4000, 0, 0 };
    FieldAcmdTCB_StepInit(actor, &step, DIR_LEFT, DIR_RIGHT, 4, 10);
    return TRUE;
}

static void FieldAcmdTCB_HopInit(FieldActor *actor, s32 step, u32 faceDir, u32 motionDir, u8 frames, u8 anim, u8 axis,
                                 u8 bobAxis, u8 bobNegate) {
    s32 sign = 1;
    SlideBobWork *work = FldAct_InitMoveCmdWork(actor, sizeof(SlideBobWork));
    work->frames = frames;
    work->step = step;
    work->axis = axis;
    work->bobAxis = bobAxis;
    work->bobNegate = bobNegate;
    work->angleStep = 0x1000 / work->frames;
    CheckSetActorFaceDir(actor, faceDir);
    SetActorMotionDir(actor, motionDir);
    FldAct_SetDrawStatus(actor, anim);
    FldAct_SetMoveStart(actor);
    SetActorInitialGPosX(actor, GetGPosX(actor));
    SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
    SetActorInitialGPosZ(actor, GetGPosZ(actor));
    if (step != 0) {
        switch (axis) {
        case 0:
            if (step < 0) {
                sign = -1;
            }
            adjustXPos(actor, sign);
            break;
        case 1:
            if (step < 0) {
                sign = -1;
            }
            adjustZPos(actor, sign * 2);
            break;
        case 2:
            if (step < 0) {
                sign = -1;
            }
            adjustYPos(actor, sign);
            break;
        }
    }
    FieldActor_NextAcmdState(actor);
}

static BOOL FieldAcmdTCB_HopMove(FieldActor *actor) {
    SlideBobWork *work = FldAct_GetMoveCmdWork(actor);
    VecFx32 position;
    s32 bobValue;
    CopyActorWPos(actor, &position);
    switch (work->axis) {
    case 0:
        position.x += work->step;
        break;
    case 1:
        position.y += work->step;
        break;
    case 2:
        position.z += work->step;
        break;
    }
    SetActorWPosValue(actor, &position);
    work->distance += work->step < 0 ? -work->step : work->step;
    work->angle += work->angleStep;
    if (work->angle > 0xf00) {
        work->angle = 0xf00;
    }
    VecFx32 bob = { 0, 0, 0 };
    u16 index = work->angle / 256;
    bobValue = data_ov036_021d4d34[1][index];
    if (work->bobNegate == 1) {
        bobValue = -bobValue;
    }
    switch (work->bobAxis) {
    case 0:
        bob.x = bobValue;
        break;
    case 1:
        bob.y = bobValue;
        break;
    case 2:
        bob.z = bobValue;
        break;
    }
    SetActorWPosOffset(actor, &bob);
    work->frames--;
    if (work->distance >= 0x10000 && work->frames != 0) {
        s32 step;
        s32 sign;
        work->distance -= 0x10000;
        sign = 1;
        step = work->step;
        SetActorInitialGPosX(actor, GetGPosX(actor));
        SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
        SetActorInitialGPosZ(actor, GetGPosZ(actor));
        switch (work->axis) {
        case 0:
            if (step < 0) {
                sign -= 2;
            }
            adjustXPos(actor, sign);
            break;
        case 1:
            if (step < 0) {
                sign -= 2;
            }
            adjustZPos(actor, sign * 2);
            break;
        case 2:
            if (step < 0) {
                sign -= 2;
            }
            adjustYPos(actor, sign);
            break;
        }
    }
    if (work->frames != 0) {
        return FALSE;
    }
    VecFx32 zero = { 0, 0, 0 };
    SetActorWPosOffset(actor, &zero);
    SetActorMovementFlag(actor, 0x540);
    SetActorInitialGPosToNowGPos(actor);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_HopUpFaceLeft8F(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, 0x2000, DIR_LEFT, DIR_UP, 8, 3, 1, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_HopDownFaceRight8F(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, -0x2000, DIR_RIGHT, DIR_DOWN, 8, 3, 1, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_HopLeftFaceDown8F(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, 0x2000, DIR_DOWN, DIR_LEFT, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_HopRightFaceUp8F(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, -0x2000, DIR_UP, DIR_RIGHT, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_HopUpFaceRight8F(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, 0x2000, DIR_RIGHT, DIR_UP, 8, 3, 1, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_HopDownFaceLeft8F(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, -0x2000, DIR_LEFT, DIR_DOWN, 8, 3, 1, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_HopLeftFaceUp8F(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, -0x2000, DIR_UP, DIR_LEFT, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL func_ov036_02190d60(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, 0x2000, DIR_DOWN, DIR_LEFT, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_HopUpFaceDown8F(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, 0x2000, DIR_DOWN, DIR_UP, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_HopDownFaceUp8F(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, -0x2000, DIR_UP, DIR_DOWN, 8, 3, 2, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_HopLeftFaceRight8F(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, -0x2000, DIR_RIGHT, DIR_LEFT, 8, 3, 0, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_HopRightFaceLeft8F(FieldActor *actor) {
    FieldAcmdTCB_HopInit(actor, 0x2000, DIR_LEFT, DIR_RIGHT, 8, 3, 0, 1, 0);
    return TRUE;
}

static BOOL FieldAcmdTCB_StayWalk22FInit(FieldActor *actor) {
    FldAct_InitMoveCmdWork(actor, 4);
    FldAct_SetDrawStatus(actor, 1);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_StayWalk22FMove(FieldActor *actor) {
    s32 *count = FldAct_GetMoveCmdWork(actor);
    (*count)++;
    if (*count >= 22) {
        FieldActor_NextAcmdState(actor);
    }
    return FALSE;
}

static BOOL FieldAcmdTCB_SetAnime10(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_DashLeft2F(FieldActor *actor) {
    WalkFramesWork *work = FldAct_InitMoveCmdWork(actor, sizeof(WalkFramesWork));
    work->anim = 12;
    work->frames = 2;
    work->dir = 2;
    work->distance = 0x8000;
    FldAct_StepGPosInDir(actor, work->dir);
    FldAct_SetDrawStatus(actor, work->anim);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_Dash2FMove(FieldActor *actor) {
    WalkFramesWork *work = FldAct_GetMoveCmdWork(actor);
    FldAct_MoveWPosInDir(actor, work->dir, work->distance);
    CheckRecalcActorY(actor);
    work->frames--;
    if (work->frames > 0) {
        return FALSE;
    }
    SetActorInitialGPosToNowGPos(actor);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_FlyUpInit(FieldActor *actor) {
    LiftWork *work = FldAct_InitMoveCmdWork(actor, 0x10);
    work->speed = 0x2000;
    work->time = 0;
    work->lastFrame = 0;
    FldAct_SetDrawStatus(actor, 11);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_FlyUpMove(FieldActor *actor) {
    LiftWork *work = FldAct_GetMoveCmdWork(actor);
    u16 anim;
    u16 step;
    if (RailActorAnim_GetBlactAnimeTypeA(actor, &anim, &step) != TRUE) {
        return FALSE;
    }
    work->time++;
    step--;
    if (FldAct_GetObjCode(actor) == OBJ_CODE_211) {
        if (anim == 2 && work->lastFrame != step) {
            work->lastFrame = step;
            switch (step) {
            case 7:
            case 12:
            case 17:
                GFL_SndSEPlay(SEQ_SE_FLD_167);
                break;
            case 10:
            case 15:
            case 20:
                GFL_SndSEPlay(SEQ_SE_FLD_168);
                break;
            }
        }
    } else if (work->time == 0x5f) {
        GFL_SndSEPlay(SEQ_SE_FLD_169);
    }
    if (work->time < 0x5f) {
        return FALSE;
    }
    work->height += work->speed;
    VecFx32 offset = { 0, 0, 0 };
    offset.y = work->height;
    SetActorWPosOffset(actor, &offset);
    if (work->height / 0x8000 < 16) {
        return FALSE;
    }
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_0219100c(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_AnimeWaitEndFaceDownInit(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 11);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_AnimeWaitEndFaceDown(FieldActor *actor) {
    BlActSys *blAct = FieldBlAct_GetBlActSys(FldActSys_GetFieldBlAct(GetActorMModelSystem(actor)));
    u32 blActIdx;
    u16 cmd;
    blActIdx = FldAct_GetBlActIdx(actor, 0);
    if (blActIdx != 0xffff && !BlActSys_GetActorLastCmd(blAct, (u16)blActIdx, &cmd)) {
        return FALSE;
    }
    CheckSetActorFaceDir(actor, DIR_DOWN);
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_ResetDrawOffset(FieldActor *actor) {
    VecFx32 offset = { 0, 0, 0 };
    SetActorWPosOffset(actor, &offset);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_FlyReadyInit(FieldActor *actor) {
    VecFx32 offset = { 0, 0x8000, 0 };
    ShiftWork *work = FldAct_InitMoveCmdWork(actor, 6);
    work->time = 0;
    work->lastFrame = 0;
    SetActorWPosOffset(actor, &offset);
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_FlyReadyMove(FieldActor *actor) {
    ShiftWork *work = FldAct_GetMoveCmdWork(actor);
    u16 anim;
    u16 step;
    if (RailActorAnim_GetBlactAnimeTypeB(actor, &anim, &step) != TRUE) {
        return FALSE;
    }
    work->time++;
    step--;
    if (FldAct_GetObjCode(actor) == OBJ_CODE_221 && anim == 0 && work->lastFrame != step) {
        work->lastFrame = step;
        switch (step) {
        case 0:
            GFL_SndSEPlay(SEQ_SE_FLD_167);
            break;
        case 3:
            GFL_SndSEPlay(SEQ_SE_FLD_168);
            break;
        }
    }
    if (work->time < 0x5f) {
        return FALSE;
    }
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL func_ov036_02191174(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return TRUE;
}

static BOOL FieldAcmdTCB_DrawReadyWaitInit10(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 10);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_DrawReadyWaitInit11(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 11);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_DrawReadyWaitInit12(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 12);
    func_ov036_02197288(actor);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_DrawReadyWaitInit13(FieldActor *actor) {
    FldAct_SetDrawStatus(actor, 13);
    func_ov036_02197288(actor);
    FieldActor_NextAcmdState(actor);
    return FALSE;
}

static BOOL FieldAcmdTCB_DrawReadyWaitStop(FieldActor *actor) {
    if (RailActorAnim_CheckBlactReady(actor) == TRUE) {
        FldAct_SetDrawStatus(actor, 0);
        FieldActor_NextAcmdState(actor);
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldAcmdTCB_DrawReadyWait(FieldActor *actor) {
    if (RailActorAnim_CheckBlactReady(actor) == TRUE) {
        FieldActor_NextAcmdState(actor);
        return TRUE;
    }
    return FALSE;
}

// The command tables, after the functions. MWCC lays a file's data out sorted by size, and equal sizes come out in an
// order set by where every object is declared, so these are in the order the game's layout needs: that of their start
// functions (then update functions, for a shared start), except commands 88-91 (their index in
// ACTOR_ACTION_COMMAND_TABLES), which follow command 83's table, 117-120, which follow 152's, and 153-158, which follow
// 144's. The tables the functions read come last, declared at the top.
const ActorAcmdFunc ACMD_DIR_TABLE[] = { FieldAcmdTCB_FaceUp, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cad0c[] = { FieldAcmdTCB_FaceDown, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cad14[] = { FieldAcmdTCB_FaceLeft, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caccc[] = { FieldAcmdTCB_FaceRight, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_WALK32_TABLE[] = { FieldAcmdTCB_WalkUp32F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb580[] = { FieldAcmdTCB_WalkDown32F, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb574[] = { FieldAcmdTCB_WalkLeft32F, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb568[] = { FieldAcmdTCB_WalkRight32F, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_WALK16_TABLE[] = { FieldAcmdTCB_WalkUp16F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb550[] = { FieldAcmdTCB_WalkDown16F, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb544[] = { FieldAcmdTCB_WalkLeft16F, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb538[] = { FieldAcmdTCB_WalkRight16F, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_WALK8_TABLE[] = { FieldAcmdTCB_WalkUp8F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb520[] = { FieldAcmdTCB_WalkDown8F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb514[] = { FieldAcmdTCB_WalkLeft8F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb508[] = { FieldAcmdTCB_WalkRight8F, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_WALK4_TABLE[] = { FieldAcmdTCB_WalkUp4F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb4f0[] = { FieldAcmdTCB_WalkDown4F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb4e4[] = { FieldAcmdTCB_WalkLeft4F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb4d8[] = { FieldAcmdTCB_WalkRight4F, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_WALK2_TABLE[] = { FieldAcmdTCB_WalkUp2F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb4c0[] = { FieldAcmdTCB_WalkDown2F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb4b4[] = { FieldAcmdTCB_WalkLeft2F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb4a8[] = { FieldAcmdTCB_WalkRight2F, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb49c[] = { FieldAcmdTCB_WalkUp1F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb490[] = { FieldAcmdTCB_WalkDown1F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb484[] = { FieldAcmdTCB_WalkLeft1F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb478[] = { FieldAcmdTCB_WalkRight1F, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_TURN32_TABLE[] = { FieldAcmdTCB_StayWalkUp32F, FieldAcmdTCB_StayWalkMove,
                                            FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb460[] = { FieldAcmdTCB_StayWalkDown32F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb454[] = { FieldAcmdTCB_StayWalkLeft32F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb448[] = { FieldAcmdTCB_StayWalkRight32F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_TURN16_TABLE[] = { FieldAcmdTCB_StayWalkUp16F, FieldAcmdTCB_StayWalkMove,
                                            FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb430[] = { FieldAcmdTCB_StayWalkDown16F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb424[] = { FieldAcmdTCB_StayWalkLeft16F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb418[] = { FieldAcmdTCB_StayWalkRight16F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_TURN8_TABLE[] = { FieldAcmdTCB_StayWalkUp8F, FieldAcmdTCB_StayWalkMove,
                                           FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb400[] = { FieldAcmdTCB_StayWalkDown8F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb3f4[] = { FieldAcmdTCB_StayWalkLeft8F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb3e8[] = { FieldAcmdTCB_StayWalkRight8F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_TURN4_TABLE[] = { FieldAcmdTCB_StayWalkUp4F, FieldAcmdTCB_StayWalkMove,
                                           FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb010[] = { FieldAcmdTCB_StayWalkDown4F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb3c4[] = { FieldAcmdTCB_StayWalkLeft4F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb3b8[] = { FieldAcmdTCB_StayWalkRight4F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_TURN2_TABLE[] = { FieldAcmdTCB_StayWalkUp2F, FieldAcmdTCB_StayWalkMove,
                                           FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb3a0[] = { FieldAcmdTCB_StayWalkDown2F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb394[] = { FieldAcmdTCB_StayWalkLeft2F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb388[] = { FieldAcmdTCB_StayWalkRight2F, FieldAcmdTCB_StayWalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_JUMP16F[] = { FieldAcmdTCB_StayJumpUp16F, FieldAcmdTCB_JumpMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb370[] = { FieldAcmdTCB_StayJumpDown16F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb364[] = { FieldAcmdTCB_StayJumpLeft16F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb358[] = { FieldAcmdTCB_StayJumpRight16F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_JUMP8F[] = { FieldAcmdTCB_StayJumpUp8F, FieldAcmdTCB_JumpMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb340[] = { FieldAcmdTCB_StayJumpDown8F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb334[] = { FieldAcmdTCB_StayJumpLeft8F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb328[] = { FieldAcmdTCB_StayJumpRight8F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb31c[] = { FieldAcmdTCB_JumpUp1G8F, FieldAcmdTCB_JumpMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb310[] = { FieldAcmdTCB_JumpDown1G8F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb304[] = { FieldAcmdTCB_JumpLeft1G8F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb2f8[] = { FieldAcmdTCB_JumpRight1G8F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb2ec[] = { FieldAcmdTCB_JumpUp2G16F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb2e0[] = { FieldAcmdTCB_JumpDown2G16F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb2d4[] = { FieldAcmdTCB_JumpLeft2G16F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb2c8[] = { FieldAcmdTCB_JumpRight2G16F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_JUMP3G16F_TABLE[] = { FieldAcmdTCB_JumpLeft1G16F, FieldAcmdTCB_JumpMove,
                                               FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb1e4[] = { FieldAcmdTCB_JumpRight1G16F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb2a4[] = { FieldAcmdTCB_JumpLeft3G12F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb298[] = { FieldAcmdTCB_JumpRight3G12F, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb28c[] = { FieldAcmdTCB_Wait1F, FieldAcmdTCB_WaitMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb280[] = { FieldAcmdTCB_Wait2F, FieldAcmdTCB_WaitMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb250[] = { FieldAcmdTCB_Wait4F, FieldAcmdTCB_WaitMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb25c[] = { FieldAcmdTCB_Wait8F, FieldAcmdTCB_WaitMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb274[] = { FieldAcmdTCB_Wait15F, FieldAcmdTCB_WaitMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb2b0[] = { FieldAcmdTCB_Wait16F, FieldAcmdTCB_WaitMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb244[] = { FieldAcmdTCB_Wait32F, FieldAcmdTCB_WaitMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb238[] = { FieldAcmdTCB_RiseInit, FieldAcmdTCB_RiseMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb22c[] = { FieldAcmdTCB_FallInit, FieldAcmdTCB_FallMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cad04[] = { FieldAcmdTCB_SetHiddenFlag, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cacec[] = { FieldAcmdTCB_ClearHiddenFlag, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cacac[] = { FieldAcmdTCB_SetFaceDirLockFlag, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cacdc[] = { FieldAcmdTCB_ClearFaceDirLockFlag, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cacd4[] = { FieldAcmdTCB_SetAnimePauseFlag, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cacfc[] = { FieldAcmdTCB_ClearAnimePauseFlag, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cadb8[] = { FieldAcmdTCB_MarkWaitExcl, FieldAcmdTCB_MarkWait,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cad28[] = { FieldAcmdTCB_MarkWaitExclNoSe, FieldAcmdTCB_MarkWait,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb1c0[] = { FieldAcmdTCB_MarkWaitNoteNoSe, FieldAcmdTCB_MarkWait,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb1b4[] = { func_ov036_0218fd18, FieldAcmdTCB_MarkWait, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb1a8[] = { FieldAcmdTCB_MarkWaitQuestion, FieldAcmdTCB_MarkWait,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb19c[] = { FieldAcmdTCB_MarkWaitNote, FieldAcmdTCB_MarkWait,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb190[] = { FieldAcmdTCB_MarkWaitEllipsis, FieldAcmdTCB_MarkWait,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb184[] = { FieldAcmdTCB_WalkUp6F, FieldAcmdTCB_Walk6FMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb178[] = { FieldAcmdTCB_WalkDown6F, FieldAcmdTCB_Walk6FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb16c[] = { FieldAcmdTCB_WalkLeft6F, FieldAcmdTCB_Walk6FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb160[] = { FieldAcmdTCB_WalkRight6F, FieldAcmdTCB_Walk6FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb154[] = { FieldAcmdTCB_DashUp6F, FieldAcmdTCB_Walk6FMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb148[] = { FieldAcmdTCB_DashDown6F, FieldAcmdTCB_Walk6FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb13c[] = { FieldAcmdTCB_DashLeft6F, FieldAcmdTCB_Walk6FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caec0[] = { FieldAcmdTCB_DashRight6F, FieldAcmdTCB_Walk6FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb124[] = { FieldAcmdTCB_WalkUp3F, FieldAcmdTCB_Walk3FMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb118[] = { FieldAcmdTCB_WalkDown3F, FieldAcmdTCB_Walk3FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb10c[] = { FieldAcmdTCB_WalkLeft3F, FieldAcmdTCB_Walk3FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb100[] = { FieldAcmdTCB_WalkRight3F, FieldAcmdTCB_Walk3FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc ACMD_RUN4F_TABLE[] = { FieldAcmdTCB_DashUp4F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb0e8[] = { FieldAcmdTCB_DashDown4F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb0dc[] = { FieldAcmdTCB_DashLeft4F, FieldAcmdTCB_WalkMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb0d0[] = { FieldAcmdTCB_DashRight4F, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb0c4[] = { FieldAcmdTCB_WalkUp7F, FieldAcmdTCB_Walk7FMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb0b8[] = { FieldAcmdTCB_WalkDown7F, FieldAcmdTCB_Walk7FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb0ac[] = { FieldAcmdTCB_WalkLeft7F, FieldAcmdTCB_Walk7FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb0a0[] = { FieldAcmdTCB_WalkRight7F, FieldAcmdTCB_Walk7FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb094[] = { FieldAcmdTCB_WalkUp12F, FieldAcmdTCB_Walk12FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb088[] = { FieldAcmdTCB_WalkDown12F, FieldAcmdTCB_Walk12FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb07c[] = { FieldAcmdTCB_WalkLeft12F, FieldAcmdTCB_Walk12FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb070[] = { FieldAcmdTCB_WalkRight12F, FieldAcmdTCB_Walk12FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb064[] = { FieldAcmdTCB_AnimeWaitEndInit10, FieldAcmdTCB_AnimeWaitEndStop,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb058[] = { FieldAcmdTCB_AnimeWaitEndInit10, FieldAcmdTCB_AnimeWaitEnd,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb04c[] = { FieldAcmdTCB_AnimeWaitEndInit11, FieldAcmdTCB_AnimeWaitEndStop,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb040[] = { FieldAcmdTCB_AnimeWaitEndInit11, FieldAcmdTCB_AnimeWaitEnd,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb034[] = { FieldAcmdTCB_AnimeWaitEndInit12, FieldAcmdTCB_AnimeWaitEndStop,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb028[] = { FieldAcmdTCB_AnimeWaitEndInit12, FieldAcmdTCB_AnimeWaitEnd,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb01c[] = { FieldAcmdTCB_AnimeWaitEndInit10Se, FieldAcmdTCB_AnimeWaitEndStop,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae30[] = { FieldAcmdTCB_AnimeWaitEndInit11Se, FieldAcmdTCB_AnimeWaitEndStop,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb004[] = { FieldAcmdTCB_HideOffInit, FieldAcmdTCB_HideOffMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caff8[] = { FieldAcmdTCB_StayWalk32F40F, FieldAcmdTCB_StayMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cafec[] = { FieldAcmdTCB_StayWalk16F41F, FieldAcmdTCB_StayMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cafe0[] = { FieldAcmdTCB_StayIdle8F, FieldAcmdTCB_StayMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cafd4[] = { FieldAcmdTCB_StepUpFaceLeft8F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cafc8[] = { FieldAcmdTCB_StepDownFaceRight8F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cafbc[] = { FieldAcmdTCB_StepLeftFaceDown8F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cafb0[] = { FieldAcmdTCB_StepRightFaceUp8F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf80[] = { FieldAcmdTCB_StepUpFaceRight8F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf8c[] = { FieldAcmdTCB_StepDownFaceLeft8F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cafa4[] = { FieldAcmdTCB_StepLeftFaceUp8F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb1f0[] = { func_ov036_021904f4, FieldAcmdTCB_StepMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb208[] = { FieldAcmdTCB_StepUpFaceDown8F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb214[] = { FieldAcmdTCB_StepDownFaceUp8F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb268[] = { FieldAcmdTCB_StepLeftFaceRight8F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb3d0[] = { FieldAcmdTCB_StepRightFaceLeft8F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf44[] = { FieldAcmdTCB_StepUpFaceDown4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf38[] = { FieldAcmdTCB_StepDownFaceUp4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf2c[] = { FieldAcmdTCB_StepLeftFaceRight4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cadc4[] = { FieldAcmdTCB_StepRightFaceLeft4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb1d8[] = { FieldAcmdTCB_StepUpFaceDown2F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb1cc[] = { FieldAcmdTCB_StepDownFaceUp2F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caefc[] = { FieldAcmdTCB_StepLeftFaceRight2F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caef0[] = { FieldAcmdTCB_StepRightFaceLeft2F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caee4[] = { FieldAcmdTCB_JumpUp3G24FNoSe, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caed8[] = { FieldAcmdTCB_JumpDown3G24FNoSe, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caecc[] = { FieldAcmdTCB_JumpLeft3G24FNoSe, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cad94[] = { FieldAcmdTCB_JumpRight3G24FNoSe, FieldAcmdTCB_JumpMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caeb4[] = { FieldAcmdTCB_DashStepUpFaceLeft4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caea8[] = { FieldAcmdTCB_DashStepDownFaceRight4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae9c[] = { FieldAcmdTCB_DashStepLeftFaceDown4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae90[] = { FieldAcmdTCB_DashStepRightFaceUp4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae84[] = { FieldAcmdTCB_DashStepUpFaceRight4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae78[] = { FieldAcmdTCB_DashStepDownFaceLeft4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae6c[] = { FieldAcmdTCB_DashStepLeftFaceUp4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae60[] = { func_ov036_021908e4, FieldAcmdTCB_StepMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae54[] = { FieldAcmdTCB_DashStepUpFaceDown4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae48[] = { FieldAcmdTCB_DashStepDownFaceUp4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae3c[] = { FieldAcmdTCB_DashStepLeftFaceRight4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae00[] = { FieldAcmdTCB_DashStepRightFaceLeft4F, FieldAcmdTCB_StepMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae18[] = { FieldAcmdTCB_HopUpFaceLeft8F, FieldAcmdTCB_HopMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae24[] = { FieldAcmdTCB_HopDownFaceRight8F, FieldAcmdTCB_HopMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf50[] = { FieldAcmdTCB_HopLeftFaceDown8F, FieldAcmdTCB_HopMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf5c[] = { FieldAcmdTCB_HopRightFaceUp8F, FieldAcmdTCB_HopMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf98[] = { FieldAcmdTCB_HopUpFaceRight8F, FieldAcmdTCB_HopMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb1fc[] = { FieldAcmdTCB_HopDownFaceLeft8F, FieldAcmdTCB_HopMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb220[] = { FieldAcmdTCB_HopLeftFaceUp8F, FieldAcmdTCB_HopMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb598[] = { func_ov036_02190d60, FieldAcmdTCB_HopMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cadd0[] = { FieldAcmdTCB_HopUpFaceDown8F, FieldAcmdTCB_HopMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cad1c[] = { FieldAcmdTCB_HopDownFaceUp8F, FieldAcmdTCB_HopMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf08[] = { FieldAcmdTCB_HopLeftFaceRight8F, FieldAcmdTCB_HopMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cadac[] = { FieldAcmdTCB_HopRightFaceLeft8F, FieldAcmdTCB_HopMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cada0[] = { func_ov036_0218fd28, FieldAcmdTCB_MarkWait, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cad40[] = { FieldAcmdTCB_StayWalkBobInit, FieldAcmdTCB_StayWalkBobMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cad88[] = { FieldAcmdTCB_WalkUp16FFastAnime, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cad7c[] = { FieldAcmdTCB_WalkDown16FFastAnime, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cad70[] = { FieldAcmdTCB_WalkLeft16FFastAnime, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caddc[] = { FieldAcmdTCB_WalkRight16FFastAnime, FieldAcmdTCB_WalkMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cadf4[] = { FieldAcmdTCB_StayWalk22FInit, FieldAcmdTCB_StayWalk22FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cacbc[] = { FieldAcmdTCB_SetAnime10, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf20[] = { FieldAcmdTCB_DashLeft2F, FieldAcmdTCB_Dash2FMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf68[] = { FieldAcmdTCB_FlyUpInit, FieldAcmdTCB_FlyUpMove, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cacc4[] = { func_ov036_0219100c, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cb5a4[] = { FieldAcmdTCB_AnimeWaitEndFaceDownInit, FieldAcmdTCB_AnimeWaitEndFaceDown,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cace4[] = { FieldAcmdTCB_ResetDrawOffset, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf14[] = { FieldAcmdTCB_FlyReadyInit, FieldAcmdTCB_FlyReadyMove,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cacb4[] = { func_ov036_02191174, FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cad4c[] = { FieldAcmdTCB_DrawReadyWaitInit10, FieldAcmdTCB_DrawReadyWaitStop,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cade8[] = { FieldAcmdTCB_DrawReadyWaitInit11, FieldAcmdTCB_DrawReadyWait,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021cae0c[] = { FieldAcmdTCB_DrawReadyWaitInit12, FieldAcmdTCB_DrawReadyWait,
                                              FieldAcmdTCB_SetAcmdEnd };

const ActorAcmdFunc data_ov036_021caf74[] = { FieldAcmdTCB_DrawReadyWaitInit13, FieldAcmdTCB_DrawReadyWait,
                                              FieldAcmdTCB_SetAcmdEnd };

static const s32 data_ov036_021cb5b0[] = { 0x5000, 0x6000, 0x5000 };

static const fx32 data_ov036_021cb7bc[] = { 0,
                                            FX32_CONST(2),
                                            FX32_CONST(3),
                                            FX32_CONST(4),
                                            FX32_CONST(5),
                                            FX32_CONST(6),
                                            FX32_CONST(6),
                                            FX32_CONST(6),
                                            FX32_CONST(5),
                                            FX32_CONST(5),
                                            FX32_CONST(4),
                                            FX32_CONST(3),
                                            FX32_CONST(2),
                                            0,
                                            0,
                                            0 };

static const s32 data_ov036_021cb5d4[] = { 0x2000, 0x3000, 0x3000, 0x2000, 0x3000, 0x3000 };

static const s32 data_ov036_021cb608[] = { 0x1000, 0x1000, 0x2000, 0x1000, 0x1000, 0x2000,
                                           0x1000, 0x1000, 0x2000, 0x1000, 0x1000, 0x2000 };

static const s32 data_ov036_021cb5ec[] = { 0x2500, 0x2480, 0x2480, 0x2480, 0x2480, 0x2480, 0x2480 };

static const fx32 data_ov036_021cb7fc[] = { FX32_CONST(2),
                                            FX32_CONST(4),
                                            FX32_CONST(6),
                                            FX32_CONST(8),
                                            FX32_CONST(9),
                                            FX32_CONST(10),
                                            FX32_CONST(10),
                                            FX32_CONST(10),
                                            FX32_CONST(9),
                                            FX32_CONST(8),
                                            FX32_CONST(6),
                                            FX32_CONST(5),
                                            FX32_CONST(3),
                                            FX32_CONST(2),
                                            0,
                                            0 };

static const fx32 data_ov036_021cb83c[] = { FX32_CONST(4),
                                            FX32_CONST(6),
                                            FX32_CONST(8),
                                            FX32_CONST(10),
                                            FX32_CONST(11),
                                            FX32_CONST(12),
                                            FX32_CONST(12),
                                            FX32_CONST(12),
                                            FX32_CONST(11),
                                            FX32_CONST(10),
                                            FX32_CONST(9),
                                            FX32_CONST(8),
                                            FX32_CONST(6),
                                            FX32_CONST(4),
                                            0,
                                            0 };

static const fx32 *data_ov036_021d4d34[] = { data_ov036_021cb83c, data_ov036_021cb7bc, data_ov036_021cb7fc };
