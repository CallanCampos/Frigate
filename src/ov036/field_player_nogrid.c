// The field player's movement on a rail: each frame the held direction and the rail ahead decide a command, by
// whether the player walks, cycles or surfs, and the command sets the actor's next animation command. Function names
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/obj_codes.h"
#include "constants/sound.h"
#include "field/event_data.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_player_core.h"
#include "field/field_player_nogrid.h"
#include "field/field_rail.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "nitro/mi.h"
#include "system/game_data.h"
#include "system/game_system.h"

// What the player is doing, which the next command depends on
enum {
    NOGRID_STATE_IDLE,
    NOGRID_STATE_MOVE,
    NOGRID_STATE_TURN,
    NOGRID_STATE_BRAKE,
};

// What the player does next
enum {
    NOGRID_COMMAND_NONE,
    NOGRID_COMMAND_IDLE,
    NOGRID_COMMAND_MOVE,
    NOGRID_COMMAND_TURN,
    NOGRID_COMMAND_BRAKE,
    NOGRID_COMMAND_JUMP,
};

#define NOGRID_FLAG_SLIDING 0x1
#define NOGRID_FLAG_SLIDE_END_SPOT 0x2
#define NOGRID_FLAG_4 0x4
#define NOGRID_FLAG_SLIDE_A 0x8
#define NOGRID_FLAG_SLIDE_B 0x10
#define NOGRID_FLAG_SLIDE_C 0x20
#define NOGRID_FLAG_SLIDE_PENDING 0x40

// The forced moves of the tiles under the player, which FieldPlayerRail_GetForcedMoveKind returns: by the tile checks
// that find them, ice, the four ways that force a move, the tile that stops it, and the slope down
enum {
    FORCED_MOVE_NONE,
    FORCED_MOVE_ICE,
    FORCED_MOVE_UP,
    FORCED_MOVE_DOWN,
    FORCED_MOVE_LEFT,
    FORCED_MOVE_RIGHT,
    FORCED_MOVE_STOP,
    FORCED_MOVE_SLIP_DOWN,
    FORCED_MOVE_MAX,
};

typedef struct {
    u8 active;
    u8 kind;
    s16 step;
} StepSound;

typedef struct {
    u8 active : 1;
    u8 held : 1;
    u8 pending : 1;
    u8 mode : 5;
    s16 time;
    u32 dir;
} SlideState;

struct FieldPlayerNoGrid {
    u32 state;
    u32 flags;
    StepSound sound;
    SlideState slide;
    FieldPlayerCore *core;
    Field *field;
    FieldActor *actor;
    RailUnit *railUnit;
};

typedef u16 (*SlideFunc)(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove);

static BOOL FieldPlayerRail_CheckAllowUpdate(FieldPlayerNoGrid *nogrid, u16 dir);
static u32 FieldPlayerRail_CheckAction(FieldPlayerNoGrid *nogrid, u32 dir, u32 pressedKeys, u32 heldKeys, BOOL force);
static void FieldPlayerRail_StartAction(FieldPlayerNoGrid *nogrid, u32 command, u32 dir, u32 pressedKeys, u32 heldKeys,
                                        BOOL force);
static void FieldPlayerRail_UpdateFlagsAfterCheck(FieldPlayerNoGrid *nogrid, u32 heldKeys, u32 command);
static u32 FieldPlayerRail_GetHitBits(FieldPlayerNoGrid *nogrid, FieldActor *actor, u16 dir, u32 *tileType);
static void FieldPlayerRail_UpdateSpin(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_StartActionWalk(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys,
                                            u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckActionWalk(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir,
                                           BOOL force);
static u32 FieldPlayerRail_CheckWalkFromStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                              u32 heldKeys, u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckWalkStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                         u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckWalkFromTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckWalkFromBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                                u32 heldKeys, u16 dir, BOOL force);
static void FieldPlayerRail_StartWalkNone(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force);
static void FieldPlayerRail_StartWalkStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                           u16 dir, BOOL force);
static void FieldPlayerRail_StartWalkStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force);
static void FieldPlayerRail_StartWalkTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force);
static void FieldPlayerRail_StartWalkBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force);
static void FieldPlayerRail_StartWalkLedgeJump(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                               u32 heldKeys, u16 dir, BOOL force);
static void FieldPlayerRail_StartActionSlow(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys,
                                            u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckActionSlow(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir,
                                           BOOL force);
static u32 FieldPlayerRail_CheckSlowFromStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                              u32 heldKeys, u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckSlowStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                         u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckSlowFromTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckSlowFromBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                                u32 heldKeys, u16 dir, BOOL force);
static void FieldPlayerRail_StartSlowNone(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force);
static void FieldPlayerRail_StartSlowStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                           u16 dir, BOOL force);
static void FieldPlayerRail_StartSlowStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force);
static void FieldPlayerRail_StartSlowTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force);
static void FieldPlayerRail_StartSlowBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force);
static void FieldPlayerRail_StartSlowLedgeJump(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                               u32 heldKeys, u16 dir, BOOL force);
static void FieldPlayerRail_StartActionRail(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys,
                                            u16 dir, BOOL force);
static void FieldPlayerRail_StartRailNone(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force);
static void FieldPlayerRail_StartRailStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                           u16 dir, BOOL force);
static void FieldPlayerRail_StartRailStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force);
static void FieldPlayerRail_StartRailTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force);
static void FieldPlayerRail_StartRailBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckActionRail(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir,
                                           BOOL force);
static u32 FieldPlayerRail_CheckRailFromStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                              u32 heldKeys, u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckRailStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                         u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckRailFromTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force);
static u32 FieldPlayerRail_CheckRailFromBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                                u32 heldKeys, u16 dir, BOOL force);
static void FieldPlayerRail_SetFlags(FieldPlayerNoGrid *nogrid, u32 flag);
static void FieldPlayerRail_ClearFlags(FieldPlayerNoGrid *nogrid, u32 flag);
static BOOL FieldPlayerRail_HasFlags(FieldPlayerNoGrid *nogrid, u32 flag);
static void FieldPlayerRail_SetForcedMove(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_ClearForcedMove(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_SetForcedMoveEnded(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_ClearForcedMoveEnded(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_ClearPending(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_SetSpinA(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_ClearSpinA(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_SetSpinB(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_ClearSpinB(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_SetFaceRight(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_ClearFaceRight(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_SetInputHold(FieldPlayerNoGrid *nogrid);
static void FieldPlayerRail_ClearInputHold(FieldPlayerNoGrid *nogrid);
static u32 FieldPlayerRail_GetForcedMoveKind(FieldPlayerNoGrid *nogrid);
static u16 FieldPlayerRail_RunForcedMove(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove);
static void FieldPlayerRail_EndForcedMove(FieldPlayerNoGrid *nogrid);
static u16 FieldPlayerRail_ForcedSlide(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove);
static u16 FieldPlayerRail_ForcedSlideSpinA(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove);
static u16 FieldPlayerRail_ForcedSlideSpinB(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove);
static u16 FieldPlayerRail_ForcedSlideWarnDir2(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove);
static u16 FieldPlayerRail_ForcedSlideWarnDir3(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove);
static u16 FieldPlayerRail_ForcedSlideFaceRight(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove);
static u16 FieldPlayerRail_ForcedInputHold(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove);
static void FieldPlayerRail_StartSpin(StepSound *sound, u32 kind);
static void FieldPlayerRail_StopSpin(StepSound *sound);
static void FieldPlayerRail_AdvanceSpin(StepSound *sound, FieldActor *actor);
static void FieldPlayerRail_InitInputHold(SlideState *slide);
static void FieldPlayerRail_EnableInputHold(SlideState *slide);
static void FieldPlayerRail_ResetInputHold(SlideState *slide);
static void FieldPlayerRail_UpdateInputHold(SlideState *slide, u16 dir, FieldActor *actor);
static void FieldPlayerRail_EndInputHoldFrame(SlideState *slide);
static u32 FieldPlayerRail_GetInputHoldDir(SlideState *slide, u16 dir);
static u32 FieldPlayerRail_IsInputHoldKept(SlideState *slide);
static BOOL FieldPlayerRail_HasConnectionAhead(FieldPlayerNoGrid *nogrid, u16 dir);

// The directions the player faces in turn while it spins, by the way it spins
static const u8 SPIN_FACE_DIRS[2][4] = {
    { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT },
    { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT },
};

// What the player does on each kind of slide, by FieldPlayerRail_GetForcedMoveKind's result
static const SlideFunc SLIDE_FUNCS[FORCED_MOVE_MAX] = {
    NULL,
    FieldPlayerRail_ForcedSlide,
    FieldPlayerRail_ForcedSlideSpinA,
    FieldPlayerRail_ForcedSlideSpinB,
    FieldPlayerRail_ForcedSlideWarnDir2,
    FieldPlayerRail_ForcedSlideWarnDir3,
    FieldPlayerRail_ForcedSlideFaceRight,
    FieldPlayerRail_ForcedInputHold,
};

// The player's actor on a rail, whose object code is filled in
static ZoneNPC sPlayerTemplate = {
    0xff, OBJ_CODE_HERO, MOVE_CODE_RAIL_STAY, ACTOR_EVENT_TYPE_NONE, 0, 0, 0, 0, 0, 0, 0xffff, 0xffff, TRUE
};

FieldPlayerNoGrid *FieldPlayerRail_Create(FieldPlayerCore *core, HeapID heapId) {
    FieldPlayerNoGrid *nogrid = GFL_HeapAllocate(heapId, sizeof(FieldPlayerNoGrid), TRUE, "field_player_nogrid.c", 416);

    nogrid->core = core;
    nogrid->field = FieldPlayerCore_GetField(core);
    nogrid->actor = FieldPlayerCore_GetActor(nogrid->core);
    if (GetActorMoveCode(nogrid->actor) != MOVE_CODE_RAIL_STAY) {
        sPlayerTemplate.modelId = FldAct_GetObjCode(nogrid->actor);
        FldAct_Transplant(nogrid->actor, &sPlayerTemplate);
    }
    nogrid->railUnit = FldAct_GetRailUnit(nogrid->actor);
    {
        RailPosition pos = { 0 };

        SetActorInitedPositionRail(nogrid->actor, &pos);
    }
    FieldPlayerRail_InitInputHold(&nogrid->slide);

    switch (FieldPlayerCore_GetExState(nogrid->core)) {
    case FLD_PLAYER_EXSTATE_SURF:
        FieldPlayerCore_SetSpecialSeq(nogrid->core, 4);
        FieldPlayerCore_FinishFormChange(nogrid->core);
        break;
    case FLD_PLAYER_EXSTATE_DIVE:
        FieldPlayerCore_SetSpecialSeq(nogrid->core, 0x100);
        FieldPlayerCore_FinishFormChange(nogrid->core);
        break;
    }
    return nogrid;
}

void FieldPlayerRail_Delete(FieldPlayerNoGrid *nogrid) {
    GFL_HeapFree(nogrid);
}

void FieldPlayerRail_AttachActor(FieldPlayerNoGrid *nogrid, const RailPosition *pos) {
    if (GetActorMoveCode(nogrid->actor) != MOVE_CODE_RAIL_STAY) {
        sPlayerTemplate.modelId = FldAct_GetObjCode(nogrid->actor);
        FldAct_Transplant(nogrid->actor, &sPlayerTemplate);
    }
    nogrid->railUnit = FldAct_GetRailUnit(nogrid->actor);
    SetActorInitedPositionRail(nogrid->actor, pos);
}

void FieldPlayerRail_ClearRailWork(FieldPlayerNoGrid *nogrid) {
    nogrid->railUnit = NULL;
}

void FieldPlayerRail_UpdateMovement(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys) {
    u16 dir = FieldPlayerCore_GetMoveDirByKey(nogrid->core, heldKeys);

    if (FieldPlayerRail_CheckAllowUpdate(nogrid, dir)) {
        u32 command;

        FieldPlayerCore_StepFormChange(nogrid->core);
        FieldPlayerRail_UpdateInputHold(&nogrid->slide, dir, nogrid->actor);
        dir = FieldPlayerRail_RunForcedMove(nogrid, dir, 0);
        command = FieldPlayerRail_CheckAction(nogrid, dir, pressedKeys, heldKeys, FALSE);
        FieldPlayerRail_UpdateFlagsAfterCheck(nogrid, heldKeys, command);
        FieldPlayerRail_StartAction(nogrid, command, dir, pressedKeys, heldKeys, FALSE);
        FieldPlayerRail_UpdateSpin(nogrid);
        FieldPlayerRail_EndInputHoldFrame(&nogrid->slide);
    }
}

BOOL FieldPlayerRail_IsBlocked(FieldPlayerNoGrid *nogrid) {
    if (nogrid->state == NOGRID_STATE_BRAKE) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldPlayerRail_IsForcedMove(FieldPlayerNoGrid *nogrid) {
    return FieldPlayerRail_HasFlags(nogrid, NOGRID_FLAG_SLIDING) != 0;
}

void FieldPlayerRail_SetRailPos(FieldPlayerNoGrid *nogrid, const RailPosition *pos) {
    SetActorInitedPositionRail(nogrid->actor, pos);
}

void FieldPlayerCore_GetRailPos(FieldPlayerNoGrid *nogrid, RailPosition *pos) {
    GetActorRailPos(nogrid->actor, pos);
}

void FieldPlayerRail_GetRailLocationByDir(FieldPlayerNoGrid *nogrid, u16 dir, RailPosition *pos) {
    GetActorRailPosForDir(nogrid->actor, dir, pos);
}

void FieldPlayerRail_GetWorldPos(FieldPlayerNoGrid *nogrid, VecFx32 *pos) {
    RailUnit_GetCalcPos(nogrid->railUnit, pos);
}

RailUnit *FieldPlayerRail_GetRailWork(FieldPlayerNoGrid *nogrid) {
    return nogrid->railUnit;
}

void FieldPlayerRail_EndBlockedWalk(FieldPlayerNoGrid *nogrid) {
    if (nogrid->state == NOGRID_STATE_BRAKE) {
        FieldActor *actor = nogrid->actor;
        u16 dir = GetActorFaceDir(actor);

        FldAct_FreeAcmd(actor);
        CheckSetActorFaceDir(actor, dir);
        FldAct_SetDrawStatus(actor, DRAW_STATUS_STOP);
        FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
    }
}

static u32 FieldPlayerRail_CheckAction(FieldPlayerNoGrid *nogrid, u32 dir, u32 pressedKeys, u32 heldKeys, BOOL force) {
    u32 command;

    switch (FieldPlayerCore_GetExState(nogrid->core)) {
    case FLD_PLAYER_EXSTATE_NONE:
        command = FieldPlayerRail_CheckActionWalk(nogrid, pressedKeys, heldKeys, dir, force);
        break;
    case FLD_PLAYER_EXSTATE_CYCLING:
        command = FieldPlayerRail_CheckActionSlow(nogrid, pressedKeys, heldKeys, dir, force);
        break;
    case FLD_PLAYER_EXSTATE_SURF:
    case FLD_PLAYER_EXSTATE_DIVE:
        command = FieldPlayerRail_CheckActionRail(nogrid, pressedKeys, heldKeys, dir, force);
        break;
    }
    return command;
}

static void FieldPlayerRail_StartAction(FieldPlayerNoGrid *nogrid, u32 command, u32 dir, u32 pressedKeys, u32 heldKeys,
                                        BOOL force) {
    switch (FieldPlayerCore_GetExState(nogrid->core)) {
    case FLD_PLAYER_EXSTATE_NONE:
        FieldPlayerRail_StartActionWalk(nogrid, command, pressedKeys, heldKeys, dir, force);
        break;
    case FLD_PLAYER_EXSTATE_CYCLING:
        FieldPlayerRail_StartActionSlow(nogrid, command, pressedKeys, heldKeys, dir, force);
        break;
    case FLD_PLAYER_EXSTATE_SURF:
    case FLD_PLAYER_EXSTATE_DIVE:
        FieldPlayerRail_StartActionRail(nogrid, command, pressedKeys, heldKeys, dir, force);
        break;
    }
}

static void FieldPlayerRail_UpdateFlagsAfterCheck(FieldPlayerNoGrid *nogrid, u32 heldKeys, u32 command) {
    FieldPlayerCore_CalcSeparateKeyDirs(nogrid->core, heldKeys);
    FieldPlayerRail_ClearPending(nogrid);
    if (command == NOGRID_COMMAND_MOVE) {
        FieldPlayerRail_ClearForcedMoveEnded(nogrid);
    }
}

static u32 FieldPlayerRail_GetHitBits(FieldPlayerNoGrid *nogrid, FieldActor *actor, u16 dir, u32 *tileType) {
    return RailActor_GetHitBitsDir(actor, dir, tileType);
}

static void FieldPlayerRail_UpdateSpin(FieldPlayerNoGrid *nogrid) {
    if (FieldPlayerRail_HasFlags(nogrid, NOGRID_FLAG_SLIDE_A)) {
        FieldPlayerRail_StartSpin(&nogrid->sound, 0);
    } else if (FieldPlayerRail_HasFlags(nogrid, NOGRID_FLAG_SLIDE_B)) {
        FieldPlayerRail_StartSpin(&nogrid->sound, 1);
    } else {
        FieldPlayerRail_StopSpin(&nogrid->sound);
    }
    FieldPlayerRail_AdvanceSpin(&nogrid->sound, nogrid->actor);
}

static BOOL FieldPlayerRail_CheckAllowUpdate(FieldPlayerNoGrid *nogrid, u16 dir) {
    FieldActor *actor = nogrid->actor;

    if (IsAllActorAcmdFinished(actor) == TRUE) {
        return TRUE;
    }
    if (dir != DIR_NONE && nogrid->state == NOGRID_STATE_BRAKE && dir != GetActorFaceDir(actor)) {
        return TRUE;
    }
    if (FieldPlayerRail_HasFlags(nogrid, NOGRID_FLAG_SLIDE_A | NOGRID_FLAG_SLIDE_B) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static void FieldPlayerRail_StartActionWalk(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys,
                                            u16 dir, BOOL force) {
    FieldActor *actor = nogrid->actor;

    switch (command) {
    case NOGRID_COMMAND_NONE:
        FieldPlayerRail_StartWalkNone(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_IDLE:
        FieldPlayerRail_StartWalkStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_MOVE:
        FieldPlayerRail_StartWalkStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_TURN:
        FieldPlayerRail_StartWalkTurn(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_BRAKE:
        FieldPlayerRail_StartWalkBlocked(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_JUMP:
        FieldPlayerRail_StartWalkLedgeJump(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    }
}

static u32 FieldPlayerRail_CheckActionWalk(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir,
                                           BOOL force) {
    u32 command = NOGRID_COMMAND_NONE;
    FieldActor *actor = nogrid->actor;

    switch (nogrid->state) {
    case NOGRID_STATE_IDLE:
        command = FieldPlayerRail_CheckWalkFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_STATE_MOVE:
        command = FieldPlayerRail_CheckWalkStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_STATE_TURN:
        command = FieldPlayerRail_CheckWalkFromTurn(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_STATE_BRAKE:
        command = FieldPlayerRail_CheckWalkFromBlocked(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    }
    return command;
}

static u32 FieldPlayerRail_CheckWalkFromStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                              u32 heldKeys, u16 dir, BOOL force) {
    if (IsAllActorAcmdFinished(actor) == TRUE) {
        if (dir != DIR_NONE) {
            if (dir != GetActorFaceDir(actor) && force == FALSE) {
                return NOGRID_COMMAND_TURN;
            }
            return FieldPlayerRail_CheckWalkStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
        }
        return NOGRID_COMMAND_IDLE;
    }
    return NOGRID_COMMAND_NONE;
}

static u32 FieldPlayerRail_CheckWalkStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                         u16 dir, BOOL force) {
    u32 hit;

    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return FieldPlayerRail_CheckWalkFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }

    hit = FieldPlayerRail_GetHitBits(nogrid, actor, dir, NULL);
    if (force == TRUE && (hit & 2)) {
        hit = 0;
    }
    if (hit == 0) {
        return NOGRID_COMMAND_MOVE;
    }
    if (hit & 2) {
        RailPosition pos;
        u32 tile;
        u32 tileClass;
        u32 ledgeDir;

        GetActorRailPosForDir(actor, dir, &pos);
        tile = RailActor_GetLocationAttr(actor, &pos);
        ledgeDir = DIR_NONE;
        tileClass = GetTileClass(tile);
        GetTileFlags(tile);
        if (MapTile_IsLedgeU(tileClass)) {
            ledgeDir = DIR_UP;
        } else if (MapTile_IsLedgeD(tileClass)) {
            ledgeDir = DIR_DOWN;
        } else if (MapTile_IsLedgeL(tileClass)) {
            ledgeDir = DIR_LEFT;
        } else if (MapTile_IsLedgeR(tileClass)) {
            ledgeDir = DIR_RIGHT;
        }
        if (ledgeDir != DIR_NONE && ledgeDir == dir) {
            return NOGRID_COMMAND_JUMP;
        }
    }
    return NOGRID_COMMAND_BRAKE;
}

static u32 FieldPlayerRail_CheckWalkFromTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force) {
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return FieldPlayerRail_CheckWalkFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }
    return FieldPlayerRail_CheckWalkStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
}

static u32 FieldPlayerRail_CheckWalkFromBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                                u32 heldKeys, u16 dir, BOOL force) {
    u16 faceDir = GetActorFaceDir(actor);

    if (dir != DIR_NONE && dir != faceDir) {
        FldAct_FreeAcmd(actor);
        return FieldPlayerRail_CheckWalkStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return FieldPlayerRail_CheckWalkFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }
    return FieldPlayerRail_CheckWalkStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
}

static void FieldPlayerRail_StartWalkNone(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force) {
}

static void FieldPlayerRail_StartWalkStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                           u16 dir, BOOL force) {
    if (dir == DIR_NONE) {
        dir = GetActorFaceDir(actor);
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_FACE));
    nogrid->state = NOGRID_STATE_IDLE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
}

static void FieldPlayerRail_StartWalkStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force) {
    u16 acmd;

    if (force == TRUE) {
        acmd = ACMD_WALK_2F;
    } else if (heldKeys & PAD_BUTTON_B) {
        acmd = ACMD_DASH;
    } else {
        acmd = ACMD_WALK_8F;
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, acmd));
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void FieldPlayerRail_StartWalkTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_2F));
    nogrid->state = NOGRID_STATE_TURN;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_TURN);
}

static void FieldPlayerRail_StartWalkBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_16F));
    nogrid->state = NOGRID_STATE_BRAKE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
    if (!FieldPlayerRail_HasConnectionAhead(nogrid, dir)) {
        GFL_SndSEPlay(SEQ_SE_WALL_HIT);
    }
}

static void FieldPlayerRail_StartWalkLedgeJump(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                               u32 heldKeys, u16 dir, BOOL force) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_JUMP_2));
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void FieldPlayerRail_StartActionSlow(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys,
                                            u16 dir, BOOL force) {
    FieldActor *actor = nogrid->actor;

    switch (command) {
    case NOGRID_COMMAND_NONE:
        FieldPlayerRail_StartSlowNone(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_IDLE:
        FieldPlayerRail_StartSlowStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_MOVE:
        FieldPlayerRail_StartSlowStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_TURN:
        FieldPlayerRail_StartSlowTurn(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_BRAKE:
        FieldPlayerRail_StartSlowBlocked(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_JUMP:
        FieldPlayerRail_StartSlowLedgeJump(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    }
}

static u32 FieldPlayerRail_CheckActionSlow(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir,
                                           BOOL force) {
    u32 command = NOGRID_COMMAND_NONE;
    FieldActor *actor = nogrid->actor;

    switch (nogrid->state) {
    case NOGRID_STATE_IDLE:
        command = FieldPlayerRail_CheckSlowFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_STATE_MOVE:
        command = FieldPlayerRail_CheckSlowStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_STATE_TURN:
        command = FieldPlayerRail_CheckSlowFromTurn(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_STATE_BRAKE:
        command = FieldPlayerRail_CheckSlowFromBlocked(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    }
    return command;
}

static u32 FieldPlayerRail_CheckSlowFromStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                              u32 heldKeys, u16 dir, BOOL force) {
    if (IsAllActorAcmdFinished(actor) == TRUE) {
        if (dir != DIR_NONE) {
            if (dir != GetActorFaceDir(actor) && force == FALSE) {
                return NOGRID_COMMAND_TURN;
            }
            return FieldPlayerRail_CheckSlowStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
        }
        return NOGRID_COMMAND_IDLE;
    }
    return NOGRID_COMMAND_NONE;
}

static u32 FieldPlayerRail_CheckSlowStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                         u16 dir, BOOL force) {
    u32 hit;

    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return FieldPlayerRail_CheckSlowFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }

    hit = FieldPlayerRail_GetHitBits(nogrid, actor, dir, NULL);
    if (force == TRUE && (hit & 2)) {
        hit = 0;
    }
    if (hit == 0) {
        return NOGRID_COMMAND_MOVE;
    }
    if (hit & 2) {
        RailPosition pos;
        u32 tile;
        u32 tileClass;
        u32 ledgeDir;

        GetActorRailPosForDir(actor, dir, &pos);
        tile = RailActor_GetLocationAttr(actor, &pos);
        ledgeDir = DIR_NONE;
        tileClass = GetTileClass(tile);
        GetTileFlags(tile);
        if (MapTile_IsLedgeU(tileClass)) {
            ledgeDir = DIR_UP;
        } else if (MapTile_IsLedgeD(tileClass)) {
            ledgeDir = DIR_DOWN;
        } else if (MapTile_IsLedgeL(tileClass)) {
            ledgeDir = DIR_LEFT;
        } else if (MapTile_IsLedgeR(tileClass)) {
            ledgeDir = DIR_RIGHT;
        }
        if (ledgeDir != DIR_NONE && ledgeDir == dir) {
            return NOGRID_COMMAND_JUMP;
        }
    }
    return NOGRID_COMMAND_BRAKE;
}

static u32 FieldPlayerRail_CheckSlowFromTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force) {
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return FieldPlayerRail_CheckSlowFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }
    return FieldPlayerRail_CheckSlowStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
}

static u32 FieldPlayerRail_CheckSlowFromBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                                u32 heldKeys, u16 dir, BOOL force) {
    u16 faceDir = GetActorFaceDir(actor);

    if (dir != DIR_NONE && dir != faceDir) {
        FldAct_FreeAcmd(actor);
        return FieldPlayerRail_CheckSlowStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return FieldPlayerRail_CheckSlowFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }
    return FieldPlayerRail_CheckSlowStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
}

static void FieldPlayerRail_StartSlowNone(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force) {
}

static void FieldPlayerRail_StartSlowStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                           u16 dir, BOOL force) {
    if (dir == DIR_NONE) {
        dir = GetActorFaceDir(actor);
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_FACE));
    nogrid->state = NOGRID_STATE_IDLE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
}

static void FieldPlayerRail_StartSlowStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_WALK_2F));
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void FieldPlayerRail_StartSlowTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_2F));
    nogrid->state = NOGRID_STATE_TURN;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_TURN);
}

static void FieldPlayerRail_StartSlowBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_16F));
    nogrid->state = NOGRID_STATE_BRAKE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
    if (!FieldPlayerRail_HasConnectionAhead(nogrid, dir)) {
        GFL_SndSEPlay(SEQ_SE_WALL_HIT);
    }
}

static void FieldPlayerRail_StartSlowLedgeJump(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                               u32 heldKeys, u16 dir, BOOL force) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_JUMP_2));
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void FieldPlayerRail_StartActionRail(FieldPlayerNoGrid *nogrid, u32 command, u32 pressedKeys, u32 heldKeys,
                                            u16 dir, BOOL force) {
    FieldActor *actor = FieldPlayerCore_GetActor(nogrid->core);

    switch (command) {
    case NOGRID_COMMAND_NONE:
        FieldPlayerRail_StartRailNone(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_IDLE:
        FieldPlayerRail_StartRailStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_MOVE:
        FieldPlayerRail_StartRailStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_TURN:
        FieldPlayerRail_StartRailTurn(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_COMMAND_BRAKE:
        FieldPlayerRail_StartRailBlocked(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    }
}

static void FieldPlayerRail_StartRailNone(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force) {
}

static void FieldPlayerRail_StartRailStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                           u16 dir, BOOL force) {
    if (dir == DIR_NONE) {
        dir = GetActorFaceDir(actor);
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_FACE));
    nogrid->state = NOGRID_STATE_IDLE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
}

static void FieldPlayerRail_StartRailStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force) {
    u16 acmd;

    if (force == TRUE) {
        acmd = ACMD_WALK_2F;
    } else {
        acmd = ACMD_WALK_4F;
    }
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, acmd));
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
}

static void FieldPlayerRail_StartRailTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                          u16 dir, BOOL force) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_2F));
    nogrid->state = NOGRID_STATE_TURN;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_TURN);
}

static void FieldPlayerRail_StartRailBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force) {
    FldAct_SetAcmd(actor, GetAcmdForDir(dir, ACMD_STAY_WALK_16F));
    nogrid->state = NOGRID_STATE_BRAKE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
    if (!FieldPlayerRail_HasConnectionAhead(nogrid, dir)) {
        GFL_SndSEPlay(SEQ_SE_WALL_HIT);
    }
}

static u32 FieldPlayerRail_CheckActionRail(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys, u16 dir,
                                           BOOL force) {
    FieldActor *actor = FieldPlayerCore_GetActor(nogrid->core);
    u32 command = NOGRID_COMMAND_NONE;

    switch (nogrid->state) {
    case NOGRID_STATE_IDLE:
        command = FieldPlayerRail_CheckRailFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_STATE_MOVE:
        command = FieldPlayerRail_CheckRailStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_STATE_TURN:
        command = FieldPlayerRail_CheckRailFromTurn(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    case NOGRID_STATE_BRAKE:
        command = FieldPlayerRail_CheckRailFromBlocked(nogrid, actor, pressedKeys, heldKeys, dir, force);
        break;
    }
    return command;
}

static u32 FieldPlayerRail_CheckRailFromStand(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                              u32 heldKeys, u16 dir, BOOL force) {
    if (IsAllActorAcmdFinished(actor) == TRUE) {
        if (dir != DIR_NONE) {
            if (dir != GetActorFaceDir(actor) && force == FALSE) {
                return NOGRID_COMMAND_TURN;
            }
            return FieldPlayerRail_CheckRailStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
        }
        return NOGRID_COMMAND_IDLE;
    }
    return NOGRID_COMMAND_NONE;
}

static u32 FieldPlayerRail_CheckRailStep(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys, u32 heldKeys,
                                         u16 dir, BOOL force) {
    u32 hit;
    u32 tile;

    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return FieldPlayerRail_CheckRailFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }

    hit = FieldPlayerRail_GetHitBits(nogrid, actor, dir, &tile);
    if (force == TRUE && hit != 0 && !(hit & 0x10)) {
        hit = 0;
    }
    if (tile != TILE_TYPE_NONE) {
        u32 flags;

        GetTileClass(tile);
        flags = GetTileFlags(tile);
        if (hit == 0) {
            if (flags & 2) {
                return NOGRID_COMMAND_MOVE;
            }
            if (force == TRUE) {
                return NOGRID_COMMAND_MOVE;
            }
        }
    }
    return NOGRID_COMMAND_BRAKE;
}

static u32 FieldPlayerRail_CheckRailFromTurn(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                             u32 heldKeys, u16 dir, BOOL force) {
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return FieldPlayerRail_CheckRailFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }
    return FieldPlayerRail_CheckRailStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
}

static u32 FieldPlayerRail_CheckRailFromBlocked(FieldPlayerNoGrid *nogrid, FieldActor *actor, u32 pressedKeys,
                                                u32 heldKeys, u16 dir, BOOL force) {
    u16 faceDir = GetActorFaceDir(actor);

    if (dir != DIR_NONE && dir != faceDir) {
        FldAct_FreeAcmd(actor);
        return FieldPlayerRail_CheckRailStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }
    if (!IsAllActorAcmdFinished(actor)) {
        return NOGRID_COMMAND_NONE;
    }
    if (dir == DIR_NONE) {
        return FieldPlayerRail_CheckRailFromStand(nogrid, actor, pressedKeys, heldKeys, dir, force);
    }
    return FieldPlayerRail_CheckRailStep(nogrid, actor, pressedKeys, heldKeys, dir, force);
}

static void FieldPlayerRail_SetFlags(FieldPlayerNoGrid *nogrid, u32 flag) {
    nogrid->flags |= flag;
}

static void FieldPlayerRail_ClearFlags(FieldPlayerNoGrid *nogrid, u32 flag) {
    nogrid->flags &= ~flag;
}

static BOOL FieldPlayerRail_HasFlags(FieldPlayerNoGrid *nogrid, u32 flag) {
    if (nogrid->flags & flag) {
        return TRUE;
    }
    return FALSE;
}

static void FieldPlayerRail_SetForcedMove(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_SetFlags(nogrid, NOGRID_FLAG_SLIDING);
}

static void FieldPlayerRail_ClearForcedMove(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_ClearFlags(nogrid, NOGRID_FLAG_SLIDING);
}

static void FieldPlayerRail_SetForcedMoveEnded(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_SetFlags(nogrid, NOGRID_FLAG_SLIDE_END_SPOT);
}

static void FieldPlayerRail_ClearForcedMoveEnded(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_ClearFlags(nogrid, NOGRID_FLAG_SLIDE_END_SPOT);
}

static void FieldPlayerRail_ClearPending(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_ClearFlags(nogrid, NOGRID_FLAG_4);
}

static void FieldPlayerRail_SetSpinA(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_SetFlags(nogrid, NOGRID_FLAG_SLIDE_A);
}

static void FieldPlayerRail_ClearSpinA(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_ClearFlags(nogrid, NOGRID_FLAG_SLIDE_A);
}

static void FieldPlayerRail_SetSpinB(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_SetFlags(nogrid, NOGRID_FLAG_SLIDE_B);
}

static void FieldPlayerRail_ClearSpinB(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_ClearFlags(nogrid, NOGRID_FLAG_SLIDE_B);
}

static void FieldPlayerRail_SetFaceRight(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_SetFlags(nogrid, NOGRID_FLAG_SLIDE_C);
}

static void FieldPlayerRail_ClearFaceRight(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_ClearFlags(nogrid, NOGRID_FLAG_SLIDE_C);
}

static void FieldPlayerRail_SetInputHold(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_SetFlags(nogrid, NOGRID_FLAG_SLIDE_PENDING);
}

static void FieldPlayerRail_ClearInputHold(FieldPlayerNoGrid *nogrid) {
    FieldPlayerRail_ClearFlags(nogrid, NOGRID_FLAG_SLIDE_PENDING);
}

static u32 FieldPlayerRail_GetForcedMoveKind(FieldPlayerNoGrid *nogrid) {
    u32 tile = GetCachedTileUnderActor(nogrid->actor);
    u32 tileClass;

    if (!FieldPlayerRail_HasFlags(nogrid, NOGRID_FLAG_SLIDE_END_SPOT)) {
        tileClass = GetTileClass(tile);
        if (IsTileSlidingIce(tileClass) == TRUE) {
            return FORCED_MOVE_ICE;
        }
        if (FieldRailTilemap_IsForceMoveUp(tileClass) == TRUE) {
            return FORCED_MOVE_UP;
        }
        if (FieldRailTilemap_IsForceMoveDown(tileClass) == TRUE) {
            return FORCED_MOVE_DOWN;
        }
        if (FieldRailTilemap_IsForceMoveLeft(tileClass) == TRUE) {
            return FORCED_MOVE_LEFT;
        }
        if (FieldRailTilemap_IsForceMoveRight(tileClass) == TRUE) {
            return FORCED_MOVE_RIGHT;
        }
        if (FieldRailTilemap_IsForceMoveStop(tileClass) == TRUE) {
            return FORCED_MOVE_STOP;
        }
        if (FieldRailTilemap_IsSlipDown(tileClass) == TRUE) {
            return FORCED_MOVE_SLIP_DOWN;
        }
    }
    return FORCED_MOVE_NONE;
}

static u16 FieldPlayerRail_RunForcedMove(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove) {
    u32 slide;

    if (!IsAllActorAcmdFinished(nogrid->actor)) {
        return dir;
    }
    if (skipForcedMove == 0 && (slide = FieldPlayerRail_GetForcedMoveKind(nogrid)) != FORCED_MOVE_NONE) {
        return SLIDE_FUNCS[slide](nogrid, dir, skipForcedMove);
    }
    FieldPlayerRail_EndForcedMove(nogrid);
    return dir;
}

static void FieldPlayerRail_EndForcedMove(FieldPlayerNoGrid *nogrid) {
    FieldActor *actor = nogrid->actor;

    if (FieldPlayerRail_HasFlags(nogrid, NOGRID_FLAG_SLIDING | NOGRID_FLAG_SLIDE_A | NOGRID_FLAG_SLIDE_B |
                                             NOGRID_FLAG_SLIDE_PENDING)) {
        ClearActorFlag(actor, 0x18);
    }
    if (FieldPlayerRail_HasFlags(nogrid, NOGRID_FLAG_SLIDING)) {
        FieldPlayerRail_ClearForcedMove(nogrid);
        FieldPlayerRail_SetForcedMoveEnded(nogrid);
        FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_NONE);
    }
    if (FieldPlayerRail_HasFlags(nogrid, NOGRID_FLAG_SLIDE_A | NOGRID_FLAG_SLIDE_B)) {
        FieldPlayerRail_ClearSpinA(nogrid);
        FieldPlayerRail_ClearSpinB(nogrid);
        ChangeActorDirection(actor, GetInverseDirection(GetActorMotionDir(actor)));
    }
    if (FieldPlayerRail_HasFlags(nogrid, NOGRID_FLAG_SLIDE_C)) {
        FieldPlayerRail_ClearFaceRight(nogrid);
        ChangeActorDirection(actor, DIR_RIGHT);
    }
    if (FieldPlayerRail_HasFlags(nogrid, NOGRID_FLAG_SLIDE_PENDING)) {
        FieldPlayerRail_ClearInputHold(nogrid);
        FieldPlayerRail_ResetInputHold(&nogrid->slide);
    }
}

static u16 FieldPlayerRail_ForcedSlide(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove) {
    FieldActor *actor = nogrid->actor;
    u16 motionDir = GetActorMotionDir(actor);

    if (FieldPlayerRail_GetHitBits(nogrid, actor, motionDir, NULL) != 0) {
        FieldPlayerRail_EndForcedMove(nogrid);
        return dir;
    }
    FieldPlayerRail_SetForcedMove(nogrid);
    FieldPlayerRail_ClearSpinA(nogrid);
    FieldPlayerRail_ClearSpinB(nogrid);
    CheckSetActorFaceDir(actor, motionDir);
    FldAct_SetAcmd(actor, GetAcmdForDir(motionDir, ACMD_WALK_4F));
    SetActorFlag(actor, 0x18);
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
    return motionDir;
}

static u16 FieldPlayerRail_ForcedSlideSpinA(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove) {
    FieldActor *actor = nogrid->actor;
    u16 motionDir = GetActorMotionDir(actor);

    if (FieldPlayerRail_GetHitBits(nogrid, actor, motionDir, NULL) != 0) {
        FieldPlayerRail_EndForcedMove(nogrid);
        return dir;
    }
    FieldPlayerRail_SetForcedMove(nogrid);
    FieldPlayerRail_SetSpinA(nogrid);
    FieldPlayerRail_ClearSpinB(nogrid);
    FldAct_SetAcmd(actor, GetAcmdForDir(motionDir, ACMD_WALK_4F));
    SetActorFlag(actor, 0x18);
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
    return motionDir;
}

static u16 FieldPlayerRail_ForcedSlideSpinB(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove) {
    FieldActor *actor = nogrid->actor;
    u16 motionDir = GetActorMotionDir(actor);

    if (FieldPlayerRail_GetHitBits(nogrid, actor, motionDir, NULL) != 0) {
        FieldPlayerRail_EndForcedMove(nogrid);
        return dir;
    }
    FieldPlayerRail_SetForcedMove(nogrid);
    FieldPlayerRail_SetSpinB(nogrid);
    FieldPlayerRail_ClearSpinA(nogrid);
    FldAct_SetAcmd(actor, GetAcmdForDir(motionDir, ACMD_WALK_4F));
    SetActorFlag(actor, 0x18);
    nogrid->state = NOGRID_STATE_MOVE;
    FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
    return motionDir;
}

static u16 FieldPlayerRail_ForcedSlideWarnDir2(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove) {
    if (GetActorMotionDir(nogrid->actor) == DIR_LEFT) {
        GFL_SndSEPlay(SEQ_SE_DANSA);
    }
    return FieldPlayerRail_ForcedSlide(nogrid, dir, skipForcedMove);
}

static u16 FieldPlayerRail_ForcedSlideWarnDir3(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove) {
    if (GetActorMotionDir(nogrid->actor) == DIR_RIGHT) {
        GFL_SndSEPlay(SEQ_SE_DANSA);
    }
    return FieldPlayerRail_ForcedSlide(nogrid, dir, skipForcedMove);
}

static u16 FieldPlayerRail_ForcedSlideFaceRight(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove) {
    u16 result = FieldPlayerRail_ForcedSlide(nogrid, dir, skipForcedMove);

    FieldPlayerRail_SetFaceRight(nogrid);
    return result;
}

static u16 FieldPlayerRail_ForcedInputHold(FieldPlayerNoGrid *nogrid, u16 dir, u32 skipForcedMove) {
    u16 result;

    if (!FieldPlayerRail_HasFlags(nogrid, NOGRID_FLAG_SLIDE_PENDING)) {
        FieldPlayerRail_SetInputHold(nogrid);
        FieldPlayerRail_EnableInputHold(&nogrid->slide);
    }
    result = FieldPlayerRail_GetInputHoldDir(&nogrid->slide, dir);
    if (FieldPlayerRail_IsInputHoldKept(&nogrid->slide)) {
        SetActorFlag(nogrid->actor, 0x18);
        FldAct_SetAcmd(nogrid->actor, GetAcmdForDir(result, ACMD_WALK_16F));
        FieldPlayerCore_SetMoveStatus(nogrid->core, PLAYER_MOVE_STATUS_MOVE);
        result = DIR_NONE;
    } else {
        ClearActorFlag(nogrid->actor, 0x18);
    }
    return result;
}

static void FieldPlayerRail_StartSpin(StepSound *sound, u32 kind) {
    if (sound->active != 1 || kind != sound->kind) {
        GFL_SndSEPlay(SEQ_SE_FLD_66);
        sound->active = 1;
        sound->kind = kind;
        sound->step = 0;
    }
}

static void FieldPlayerRail_StopSpin(StepSound *sound) {
    if (sound->active != 0) {
        sound->active = 0;
        sound->step = 0;
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_66));
    }
}

static void FieldPlayerRail_AdvanceSpin(StepSound *sound, FieldActor *actor) {
    if (sound->active != 0) {
        sound->step = (sound->step + 1) % 4;
        SetActorFaceDir(actor, SPIN_FACE_DIRS[sound->kind][sound->step]);
    }
}

static void FieldPlayerRail_InitInputHold(SlideState *slide) {
    MI_CpuFill8(slide, 0, sizeof(SlideState));
    slide->dir = DIR_NONE;
}

static void FieldPlayerRail_EnableInputHold(SlideState *slide) {
    slide->active = 1;
}

static void FieldPlayerRail_ResetInputHold(SlideState *slide) {
    FieldPlayerRail_InitInputHold(slide);
}

static void FieldPlayerRail_UpdateInputHold(SlideState *slide, u16 dir, FieldActor *actor) {
    if (slide->active) {
        if (slide->mode != 0) {
            if (dir != DIR_NONE) {
                slide->time = 0;
                slide->mode = 0;
                if (slide->held) {
                    slide->dir = dir;
                }
            }
        } else if (dir == DIR_NONE) {
            if (slide->time < 30) {
                slide->time++;
            } else {
                slide->mode = 1;
            }
        }
        if (IsAllActorAcmdFinished(actor) == TRUE) {
            if (slide->mode == 0) {
                slide->held = 0;
                slide->pending = 0;
            } else {
                slide->held = 1;
                slide->pending = 1;
            }
        }
    }
}

static void FieldPlayerRail_EndInputHoldFrame(SlideState *slide) {
    if (slide->pending) {
        slide->pending = 0;
    }
    if (!slide->held) {
        slide->dir = DIR_NONE;
    }
}

static u32 FieldPlayerRail_GetInputHoldDir(SlideState *slide, u16 dir) {
    u32 ret;

    if (slide->pending) {
        return 1;
    }
    ret = slide->dir;
    if (ret == DIR_NONE) {
        ret = dir;
    }
    return ret;
}

static u32 FieldPlayerRail_IsInputHoldKept(SlideState *slide) {
    return slide->held;
}

static BOOL FieldPlayerRail_HasConnectionAhead(FieldPlayerNoGrid *nogrid, u16 dir) {
    FieldActor *actor = FieldPlayerCore_GetActor(nogrid->core);
    EventData *eventData = GameData_GetEventData(GSYS_GetGameData(Field_GetGameSystem(nogrid->field)));
    RailPosition pos;

    GetActorRailPosForDir(actor, dir, &pos);
    if (GetWarpIDByPlayerPosRail(eventData, &pos) != WARP_ID_NONE) {
        return TRUE;
    }
    return FALSE;
}
