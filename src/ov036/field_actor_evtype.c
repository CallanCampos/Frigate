// The actors' event types: the init function and the two checks of each type, which swan's ACTOR_EVENTTYPE_FUNCS
// and the two tables after it hold, with the trainers that turn in place as they watch. The ROM gives this file no
// name, so field_actor_evtype.c is a guess after the event types. Its .text starts at 0x021925a4 and its .rodata,
// sorted by size on its own, at 0x021cbdb0 (Black 2). The code was first written by mariano54 on their parallel-decomp
// branch

#include "types.h"
#include "constants/directions.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "gfl/key.h"

// The 9-byte block of the codes that turn in place: a state machine in state, one in phase
typedef struct {
    u8 state;
    u8 phase;
    s8 count;
    s8 limit;
    s8 dir;
    s8 variant;
    s8 turn;
    s8 turns;
    s8 wait;
} ActorSpinWork;

// ActorSpinWork's state, which the checks advance: wait for the actor to leave its first grid square and come back,
// limit times, then for it to stop moving, then let the turns run
enum {
    SPIN_STATE_LEAVE_GRID,
    SPIN_STATE_RETURN_TO_GRID,
    SPIN_STATE_WAIT_STOP,
    SPIN_STATE_TURN,
};

// ActorSpinWork's phase, which the turns advance: note the facing, start a turn, wait for it, then wait between turns
enum {
    SPIN_PHASE_START,
    SPIN_PHASE_START_TURN,
    SPIN_PHASE_WAIT_TURN,
    SPIN_PHASE_WAIT,
};

typedef BOOL (*ActorEvTypeFunc)(FieldActor *actor);

typedef void (*ActorEvTypeInitFunc)(FieldActor *actor);

static void ExecActorEvTypeFunction(FieldActor *actor);
static BOOL FieldActorMovecode_CheckByEventType(FieldActor *actor);
static BOOL FieldActorMovecode_UpdateByEventType(FieldActor *actor);
static BOOL FieldActorMovecode_HasLeftOldGrid(FieldActor *actor);
static BOOL FieldActorMovecode_IsAtOldGrid(FieldActor *actor);
static void FieldActorMove_InitEvTypeNone(FieldActor *actor);
static BOOL FieldActorMove_CheckEvTypeNone(FieldActor *actor);
static BOOL FieldActorMove_UpdateEvTypeNone(FieldActor *actor);
static void FieldActorMove_InitWalkRepeatLookAround(FieldActor *actor);
static BOOL FieldActorMove_CheckWalkRepeatLookAround(FieldActor *actor);
static BOOL FieldActorMove_TurnLookAround(FieldActor *actor);
static void FieldActorMove_InitWalkRepeatSpin(FieldActor *actor);
static BOOL FieldActorMove_CheckWalkRepeatSpin(FieldActor *actor);
static BOOL FieldActorMove_TurnSpin(FieldActor *actor);
static BOOL FieldActorMove_IsButtonBHeld(void);

// The tables by event type, defined after the functions: there, MWCC's size sort lays the two 16-byte local
// initializers of FieldActorMove_TurnLookAround out in the ROM's order
static const ActorEvTypeInitFunc ACTOR_EVENTTYPE_FUNCS[ACTOR_EVENT_TYPE_MAX];
static const ActorEvTypeFunc ACTOR_EVENTTYPE_CHECK_FUNCS[ACTOR_EVENT_TYPE_MAX];
static const ActorEvTypeFunc ACTOR_EVENTTYPE_UPDATE_FUNCS[ACTOR_EVENT_TYPE_MAX];

void FieldActorMovecode_InitEventTypeProcThunk(FieldActor *actor) {
    ExecActorEvTypeFunction(actor);
}

BOOL FieldActorMovecode_UpdateEventTypeProc(FieldActor *actor) {
    if (!FieldActorMovecode_CheckByEventType(actor)) {
        return FALSE;
    }
    if (FieldActorMovecode_UpdateByEventType(actor)) {
        return TRUE;
    }
    return FALSE;
}

static void ExecActorEvTypeFunction(FieldActor *actor) {
    ACTOR_EVENTTYPE_FUNCS[GetActorEvType(actor)](actor);
}

static BOOL FieldActorMovecode_CheckByEventType(FieldActor *actor) {
    return ACTOR_EVENTTYPE_CHECK_FUNCS[GetActorEvType(actor)](actor);
}

static BOOL FieldActorMovecode_UpdateByEventType(FieldActor *actor) {
    return ACTOR_EVENTTYPE_UPDATE_FUNCS[GetActorEvType(actor)](actor);
}

static BOOL FieldActorMovecode_HasLeftOldGrid(FieldActor *actor) {
    if (GetGPosX(actor) != FldAct_GetInitGPosX(actor)) {
        return TRUE;
    }
    if (GetGPosZ(actor) != FldAct_GetInitGPosZ(actor)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldActorMovecode_IsAtOldGrid(FieldActor *actor) {
    if (GetGPosX(actor) != FldAct_GetInitGPosX(actor)) {
        return FALSE;
    }
    if (GetGPosZ(actor) == FldAct_GetInitGPosZ(actor)) {
        return TRUE;
    }
    return FALSE;
}

static void FieldActorMove_InitEvTypeNone(FieldActor *actor) {
}

static BOOL FieldActorMove_CheckEvTypeNone(FieldActor *actor) {
    return FALSE;
}

static BOOL FieldActorMove_UpdateEvTypeNone(FieldActor *actor) {
    return FALSE;
}

static void FieldActorMove_InitWalkRepeatLookAround(FieldActor *actor) {
    ActorSpinWork *work = FldAct_InitMoveSubProcWork(actor, sizeof(ActorSpinWork));

    work->limit = GetActorUserParam(actor, 1);
}

static BOOL FieldActorMove_CheckWalkRepeatLookAround(FieldActor *actor) {
    ActorSpinWork *work = FldAct_GetMoveSubProcWork(actor);

    switch (work->state) {
    case SPIN_STATE_LEAVE_GRID:
        if (FieldActorMovecode_HasLeftOldGrid(actor) == TRUE) {
            work->state++;
        }
        break;
    case SPIN_STATE_RETURN_TO_GRID:
        if (!FieldActorMovecode_IsAtOldGrid(actor)) {
            break;
        }
        work->count++;
        if (work->count < work->limit) {
            work->state = SPIN_STATE_LEAVE_GRID;
            break;
        }
        work->state++;
    case SPIN_STATE_WAIT_STOP:
        if (IsActorFlag16(actor) == TRUE) {
            break;
        }
        work->state++;
        work->count = 0;
        work->phase = SPIN_PHASE_START;
    case SPIN_STATE_TURN:
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldActorMove_TurnLookAround(FieldActor *actor) {
    ActorSpinWork *work = FldAct_GetMoveSubProcWork(actor);
    s32 wait;

    switch (work->phase) {
    case SPIN_PHASE_START: {
        s32 firstDirs[4] = { 0, 0, 1, 1 };
        u16 face = GetActorFaceDir(actor);

        work->dir = face;
        work->variant = firstDirs[face];
        work->phase++;
    }
    case SPIN_PHASE_START_TURN: {
        s32 turnDirs[2][2] = { { DIR_LEFT, DIR_RIGHT }, { DIR_UP, DIR_DOWN } };

        StartDirAcmd(actor, turnDirs[work->variant][work->turn], 0);
        work->phase++;
    }
    case SPIN_PHASE_WAIT_TURN:
        if (FieldAcmdTCB_Update(actor) == FALSE) {
            return TRUE;
        }
        work->phase++;
    case SPIN_PHASE_WAIT:
        wait = 8;
        if (GetActorEvType(actor) == ACTOR_EVENT_TYPE_LOOK_AROUND_DASH && FieldActorMove_IsButtonBHeld() == TRUE &&
            FldAct_IsNearPlayer(actor) == TRUE) {
            wait = 4;
        }
        work->wait++;
        if (work->wait < wait) {
            return TRUE;
        }
        work->wait = 0;
        work->turns++;
        if (work->turns < 4) {
            work->turn = (work->turn + 1) & 1;
            work->phase = SPIN_PHASE_START_TURN;
            return TRUE;
        }
        CheckSetActorFaceDir(actor, work->dir);
        work->phase++;
        work->turns = 0;
        work->state = SPIN_STATE_LEAVE_GRID;
    }
    return FALSE;
}

static void FieldActorMove_InitWalkRepeatSpin(FieldActor *actor) {
    ActorSpinWork *work = FldAct_InitMoveSubProcWork(actor, sizeof(ActorSpinWork));
    u32 variant = 1;
    u16 evType;

    work->limit = GetActorUserParam(actor, 1);
    evType = GetActorEvType(actor);
    if (evType == ACTOR_EVENT_TYPE_TRAINER_SPIN_COUNTERCLOCKWISE || evType == ACTOR_EVENT_TYPE_SPIN_COUNTERCLOCKWISE ||
        evType == ACTOR_EVENT_TYPE_SPIN_COUNTERCLOCKWISE_DASH) {
        variant = 0;
    }
    work->variant = variant;
}

static BOOL FieldActorMove_CheckWalkRepeatSpin(FieldActor *actor) {
    ActorSpinWork *work = FldAct_GetMoveSubProcWork(actor);

    switch (work->state) {
    case SPIN_STATE_LEAVE_GRID:
        if (FieldActorMovecode_HasLeftOldGrid(actor) == TRUE) {
            work->state++;
        }
        break;
    case SPIN_STATE_RETURN_TO_GRID:
        if (!FieldActorMovecode_IsAtOldGrid(actor)) {
            break;
        }
        work->count++;
        if (work->count < work->limit) {
            work->state = SPIN_STATE_LEAVE_GRID;
            break;
        }
        work->state++;
    case SPIN_STATE_WAIT_STOP:
        if (IsActorFlag16(actor) == TRUE) {
            break;
        }
        work->state++;
        work->count = 0;
        work->phase = SPIN_PHASE_START;
    case SPIN_STATE_TURN:
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldActorMove_TurnSpin(FieldActor *actor) {
    s32 dirs[2][4] = { { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT }, { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT } };
    ActorSpinWork *work = FldAct_GetMoveSubProcWork(actor);
    s32 wait;
    s32 i;
    u32 face;

    switch (work->phase) {
    case SPIN_PHASE_START:
        face = GetActorFaceDir(actor);
        i = 0;
        while (i < 4 && face != dirs[work->variant][i]) {
            i++;
        }
        i++;
        work->dir = face;
        work->turn = i % 4;
        work->phase++;
    case SPIN_PHASE_START_TURN:
        StartDirAcmd(actor, dirs[work->variant][work->turn], 0);
        work->phase++;
    case SPIN_PHASE_WAIT_TURN:
        if (FieldAcmdTCB_Update(actor) == FALSE) {
            return TRUE;
        }
        work->phase++;
    case SPIN_PHASE_WAIT:
        wait = 8;
        {
            u16 evType = GetActorEvType(actor);

            if ((evType == ACTOR_EVENT_TYPE_SPIN_COUNTERCLOCKWISE_DASH ||
                 evType == ACTOR_EVENT_TYPE_SPIN_CLOCKWISE_DASH) &&
                FieldActorMove_IsButtonBHeld() == TRUE && FldAct_IsNearPlayer(actor) == TRUE) {
                wait = 4;
            }
        }
        work->wait++;
        if (work->wait < wait) {
            return TRUE;
        }
        work->wait = 0;
        work->turns++;
        if (work->turns < 4) {
            work->turn = (work->turn + 1) % 4;
            work->phase = SPIN_PHASE_START_TURN;
            return TRUE;
        }
        CheckSetActorFaceDir(actor, work->dir);
        work->phase++;
        work->turns = 0;
        work->state = SPIN_STATE_LEAVE_GRID;
    }
    return FALSE;
}

static BOOL FieldActorMove_IsButtonBHeld(void) {
    if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_B) {
        return TRUE;
    }
    return FALSE;
}

static const ActorEvTypeInitFunc ACTOR_EVENTTYPE_FUNCS[] = {
    FieldActorMove_InitEvTypeNone,     FieldActorMove_InitEvTypeNone,           FieldActorMove_InitEvTypeNone,
    FieldActorMove_InitEvTypeNone,     FieldActorMove_InitWalkRepeatLookAround, FieldActorMove_InitWalkRepeatSpin,
    FieldActorMove_InitWalkRepeatSpin, FieldActorMove_InitEvTypeNone,           FieldActorMove_InitEvTypeNone,
    FieldActorMove_InitEvTypeNone,     FieldActorMove_InitEvTypeNone,           FieldActorMove_InitEvTypeNone,
    FieldActorMove_InitEvTypeNone,     FieldActorMove_InitEvTypeNone,           FieldActorMove_InitWalkRepeatLookAround,
    FieldActorMove_InitWalkRepeatSpin, FieldActorMove_InitWalkRepeatSpin,       FieldActorMove_InitEvTypeNone,
    FieldActorMove_InitEvTypeNone,     FieldActorMove_InitEvTypeNone,           FieldActorMove_InitWalkRepeatSpin
};

static const ActorEvTypeFunc ACTOR_EVENTTYPE_CHECK_FUNCS[] = {
    FieldActorMove_CheckEvTypeNone,           FieldActorMove_CheckEvTypeNone,
    FieldActorMove_CheckEvTypeNone,           FieldActorMove_CheckEvTypeNone,
    FieldActorMove_CheckWalkRepeatLookAround, FieldActorMove_CheckWalkRepeatSpin,
    FieldActorMove_CheckWalkRepeatSpin,       FieldActorMove_CheckEvTypeNone,
    FieldActorMove_CheckEvTypeNone,           FieldActorMove_CheckEvTypeNone,
    FieldActorMove_CheckEvTypeNone,           FieldActorMove_CheckEvTypeNone,
    FieldActorMove_CheckEvTypeNone,           FieldActorMove_CheckEvTypeNone,
    FieldActorMove_CheckWalkRepeatLookAround, FieldActorMove_CheckWalkRepeatSpin,
    FieldActorMove_CheckWalkRepeatSpin,       FieldActorMove_CheckEvTypeNone,
    FieldActorMove_CheckEvTypeNone,           FieldActorMove_CheckEvTypeNone,
    FieldActorMove_CheckWalkRepeatSpin
};

static const ActorEvTypeFunc ACTOR_EVENTTYPE_UPDATE_FUNCS[] = {
    FieldActorMove_UpdateEvTypeNone, FieldActorMove_UpdateEvTypeNone, FieldActorMove_UpdateEvTypeNone,
    FieldActorMove_UpdateEvTypeNone, FieldActorMove_TurnLookAround,   FieldActorMove_TurnSpin,
    FieldActorMove_TurnSpin,         FieldActorMove_UpdateEvTypeNone, FieldActorMove_UpdateEvTypeNone,
    FieldActorMove_UpdateEvTypeNone, FieldActorMove_UpdateEvTypeNone, FieldActorMove_UpdateEvTypeNone,
    FieldActorMove_UpdateEvTypeNone, FieldActorMove_UpdateEvTypeNone, FieldActorMove_TurnLookAround,
    FieldActorMove_TurnSpin,         FieldActorMove_TurnSpin,         FieldActorMove_UpdateEvTypeNone,
    FieldActorMove_UpdateEvTypeNone, FieldActorMove_UpdateEvTypeNone, FieldActorMove_TurnSpin
};
