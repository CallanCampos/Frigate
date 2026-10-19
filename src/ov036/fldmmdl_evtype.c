// The actors' event types: the init function and the two checks of each type, which swan's ACTOR_EVENTTYPE_FUNCS
// and the two tables after it hold, with the trainers that turn in place as they watch. The ROM gives this file no
// name, so fldmmdl_evtype.c is a guess after the event types. Its .text starts at 0x021925a4 and its .rodata, sorted
// by size on its own, at 0x021cbdb0 (Black 2). The code was first written by mariano54 on their parallel-decomp
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

typedef BOOL (*ActorEvTypeFunc)(FieldActor *actor);

typedef void (*ActorEvTypeInitFunc)(FieldActor *actor);

static void ExecActorEvTypeFunction(FieldActor *actor);
static BOOL func_ov036_021925ec(FieldActor *actor);
static BOOL func_ov036_02192608(FieldActor *actor);
static BOOL func_ov036_02192624(FieldActor *actor);
static BOOL func_ov036_02192658(FieldActor *actor);
static void func_ov036_0219268c(FieldActor *actor);
static BOOL func_ov036_02192690(FieldActor *actor);
static BOOL func_ov036_02192694(FieldActor *actor);
static void func_ov036_02192698(FieldActor *actor);
static BOOL func_ov036_021926b0(FieldActor *actor);
static BOOL func_ov036_0219272c(FieldActor *actor);
static void func_ov036_02192840(FieldActor *actor);
static BOOL func_ov036_02192870(FieldActor *actor);
static BOOL func_ov036_021928ec(FieldActor *actor);
static BOOL func_ov036_02192a38(void);

// The tables by event type, defined after the functions: there, MWCC's size sort lays the two 16-byte local
// initializers of func_ov036_0219272c out in the ROM's order
static const ActorEvTypeInitFunc ACTOR_EVENTTYPE_FUNCS[21];
static const ActorEvTypeFunc data_ov036_021cbe44[21];
static const ActorEvTypeFunc data_ov036_021cbe98[21];

void func_ov036_021925a4(FieldActor *actor) {
    ExecActorEvTypeFunction(actor);
}

BOOL func_ov036_021925ac(FieldActor *actor) {
    if (!func_ov036_021925ec(actor)) {
        return FALSE;
    }
    if (func_ov036_02192608(actor)) {
        return TRUE;
    }
    return FALSE;
}

static void ExecActorEvTypeFunction(FieldActor *actor) {
    ACTOR_EVENTTYPE_FUNCS[GetActorEvType(actor)](actor);
}

static BOOL func_ov036_021925ec(FieldActor *actor) {
    return data_ov036_021cbe44[GetActorEvType(actor)](actor);
}

static BOOL func_ov036_02192608(FieldActor *actor) {
    return data_ov036_021cbe98[GetActorEvType(actor)](actor);
}

static BOOL func_ov036_02192624(FieldActor *actor) {
    if (GetGPosX(actor) != FldAct_GetInitGPosX(actor)) {
        return TRUE;
    }
    if (GetGPosZ(actor) != FldAct_GetInitGPosZ(actor)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_02192658(FieldActor *actor) {
    if (GetGPosX(actor) != FldAct_GetInitGPosX(actor)) {
        return FALSE;
    }
    if (GetGPosZ(actor) == FldAct_GetInitGPosZ(actor)) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_0219268c(FieldActor *actor) {
}

static BOOL func_ov036_02192690(FieldActor *actor) {
    return FALSE;
}

static BOOL func_ov036_02192694(FieldActor *actor) {
    return FALSE;
}

static void func_ov036_02192698(FieldActor *actor) {
    ActorSpinWork *work = func_ov012_02167120(actor, 9);

    work->limit = GetActorUserParam(actor, 1);
}

static BOOL func_ov036_021926b0(FieldActor *actor) {
    ActorSpinWork *work = func_ov012_02167138(actor);

    switch (work->state) {
    case 0:
        if (func_ov036_02192624(actor) == TRUE) {
            work->state++;
        }
        break;
    case 1:
        if (!func_ov036_02192658(actor)) {
            break;
        }
        work->count++;
        if (work->count < work->limit) {
            work->state = 0;
            break;
        }
        work->state++;
    case 2:
        if (IsActorFlag16(actor) == TRUE) {
            break;
        }
        work->state++;
        work->count = 0;
        work->phase = 0;
    case 3:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_0219272c(FieldActor *actor) {
    ActorSpinWork *work = func_ov012_02167138(actor);
    s32 wait;

    switch (work->phase) {
    case 0: {
        s32 firstDirs[4] = { 0, 0, 1, 1 };
        u16 face = GetActorFaceDir(actor);

        work->dir = face;
        work->variant = firstDirs[face];
        work->phase++;
    }
    case 1: {
        s32 turnDirs[2][2] = { { DIR_LEFT, DIR_RIGHT }, { DIR_UP, DIR_DOWN } };

        StartDirAcmd(actor, turnDirs[work->variant][work->turn], 0);
        work->phase++;
    }
    case 2:
        if (func_ov036_0218f01c(actor) == FALSE) {
            return TRUE;
        }
        work->phase++;
    case 3:
        wait = 8;
        if (GetActorEvType(actor) == 14 && func_ov036_02192a38() == TRUE && func_ov012_0215ebd8(actor) == TRUE) {
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
            work->phase = 1;
            return TRUE;
        }
        CheckSetActorFaceDir(actor, work->dir);
        work->phase++;
        work->turns = 0;
        work->state = 0;
    }
    return FALSE;
}

static void func_ov036_02192840(FieldActor *actor) {
    ActorSpinWork *work = func_ov012_02167120(actor, 9);
    u32 variant = 1;
    u16 evType;

    work->limit = GetActorUserParam(actor, 1);
    evType = GetActorEvType(actor);
    if (evType == 5 || evType == 20 || evType == 15) {
        variant = 0;
    }
    work->variant = variant;
}

static BOOL func_ov036_02192870(FieldActor *actor) {
    ActorSpinWork *work = func_ov012_02167138(actor);

    switch (work->state) {
    case 0:
        if (func_ov036_02192624(actor) == TRUE) {
            work->state++;
        }
        break;
    case 1:
        if (!func_ov036_02192658(actor)) {
            break;
        }
        work->count++;
        if (work->count < work->limit) {
            work->state = 0;
            break;
        }
        work->state++;
    case 2:
        if (IsActorFlag16(actor) == TRUE) {
            break;
        }
        work->state++;
        work->count = 0;
        work->phase = 0;
    case 3:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_021928ec(FieldActor *actor) {
    s32 dirs[2][4] = { { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT }, { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT } };
    ActorSpinWork *work = func_ov012_02167138(actor);
    s32 wait;
    s32 i;
    u32 face;

    switch (work->phase) {
    case 0:
        face = GetActorFaceDir(actor);
        i = 0;
        while (i < 4 && face != dirs[work->variant][i]) {
            i++;
        }
        i++;
        work->dir = face;
        work->turn = i % 4;
        work->phase++;
    case 1:
        StartDirAcmd(actor, dirs[work->variant][work->turn], 0);
        work->phase++;
    case 2:
        if (func_ov036_0218f01c(actor) == FALSE) {
            return TRUE;
        }
        work->phase++;
    case 3:
        wait = 8;
        {
            u16 evType = GetActorEvType(actor);

            if ((evType == 15 || evType == 16) && func_ov036_02192a38() == TRUE && func_ov012_0215ebd8(actor) == TRUE) {
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
            work->phase = 1;
            return TRUE;
        }
        CheckSetActorFaceDir(actor, work->dir);
        work->phase++;
        work->turns = 0;
        work->state = 0;
    }
    return FALSE;
}

static BOOL func_ov036_02192a38(void) {
    if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_B) {
        return TRUE;
    }
    return FALSE;
}

static const ActorEvTypeInitFunc ACTOR_EVENTTYPE_FUNCS[] = {
    func_ov036_0219268c, func_ov036_0219268c, func_ov036_0219268c, func_ov036_0219268c, func_ov036_02192698,
    func_ov036_02192840, func_ov036_02192840, func_ov036_0219268c, func_ov036_0219268c, func_ov036_0219268c,
    func_ov036_0219268c, func_ov036_0219268c, func_ov036_0219268c, func_ov036_0219268c, func_ov036_02192698,
    func_ov036_02192840, func_ov036_02192840, func_ov036_0219268c, func_ov036_0219268c, func_ov036_0219268c,
    func_ov036_02192840
};

static const ActorEvTypeFunc data_ov036_021cbe44[] = { func_ov036_02192690, func_ov036_02192690, func_ov036_02192690,
                                                       func_ov036_02192690, func_ov036_021926b0, func_ov036_02192870,
                                                       func_ov036_02192870, func_ov036_02192690, func_ov036_02192690,
                                                       func_ov036_02192690, func_ov036_02192690, func_ov036_02192690,
                                                       func_ov036_02192690, func_ov036_02192690, func_ov036_021926b0,
                                                       func_ov036_02192870, func_ov036_02192870, func_ov036_02192690,
                                                       func_ov036_02192690, func_ov036_02192690, func_ov036_02192870 };

static const ActorEvTypeFunc data_ov036_021cbe98[] = { func_ov036_02192694, func_ov036_02192694, func_ov036_02192694,
                                                       func_ov036_02192694, func_ov036_0219272c, func_ov036_021928ec,
                                                       func_ov036_021928ec, func_ov036_02192694, func_ov036_02192694,
                                                       func_ov036_02192694, func_ov036_02192694, func_ov036_02192694,
                                                       func_ov036_02192694, func_ov036_02192694, func_ov036_0219272c,
                                                       func_ov036_021928ec, func_ov036_021928ec, func_ov036_02192694,
                                                       func_ov036_02192694, func_ov036_02192694, func_ov036_021928ec };
