// The rail actors' jump along the rail, and the tables of the functions of their action commands, by the command's
// state, which ACTOR_ACTION_COMMAND_TABLES_RAIL (field_actor_movedata.c) lists by command. The name is descriptive. The
// size sort shows the file: its 8- and 12-byte tables and jump heights come before field_actor_rail.c's direction
// tables, which share a section that would sit among the 8-byte tables in one file Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0). Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/directions.h"
#include "constants/sound.h"
#include "field/field_actor.h"
#include "field/field_rail.h"
#include "field/fld_act_rail.h"
#include "gfl/sound.h"
#include "nitro/fx.h"

// How high an actor is above the rail at each stage of its jump, by the kind of jump
static const fx32 RAIL_JUMP_HEIGHTS_HIGH[16] = { 0x4000, 0x6000, 0x8000, 0xa000, 0xb000, 0xc000, 0xc000, 0xc000,
                                                 0xb000, 0xa000, 0x9000, 0x8000, 0x6000, 0x4000, 0,      0 };
static const fx32 RAIL_JUMP_HEIGHTS_LOW[16] = { 0,      0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x6000, 0x6000,
                                                0x5000, 0x5000, 0x4000, 0x3000, 0x2000, 0,      0,      0 };
static const fx32 RAIL_JUMP_HEIGHTS_MID[16] = { 0x2000, 0x4000, 0x6000, 0x8000, 0x9000, 0xa000, 0xa000, 0xa000,
                                                0x9000, 0x8000, 0x6000, 0x5000, 0x3000, 0x2000, 0,      0 };

// The kinds of jump, ActorRailJumpWork's height: the index of sJumpHeights
enum {
    RAIL_JUMP_HIGH,
    RAIL_JUMP_LOW,
    RAIL_JUMP_MID,
};

static const fx32 *sJumpHeights[3] = { RAIL_JUMP_HEIGHTS_HIGH, RAIL_JUMP_HEIGHTS_LOW, RAIL_JUMP_HEIGHTS_MID };

static BOOL RailJump_Update(FieldActor *actor) {
    ActorRailJumpWork *work = FldAct_GetMoveCmdWork(actor);
    RailUnit *unit;
    VecFx32 pos;

    work->timer--;
    if (work->speed != 0) {
        s32 speed;

        unit = FldAct_GetRailUnit(actor);
        RailActor_UpdateRailWork(actor);
        RailUnit_GetCalcPos(unit, &pos);
        SetActorWPosValue(actor, &pos);
        if (work->progress >= 0x10000 && work->timer > 0) {
            work->progress = 0;
            RailActor_SetMoveRequest(actor, work->dir, 8);
            RailActor_SetGridPosByDir(actor, work->dir);
            SetActorMovementFlag(actor, 0x20);
        }
        speed = work->speed;
        if (speed < 0) {
            speed = -speed;
        }
        work->progress += speed;
    }
    work->phase += work->step;
    if (work->phase > 0xf00) {
        work->phase = 0xf00;
    }
    {
        VecFx32 offset;
        u16 stage = work->phase / 256;
        const fx32 *heights = sJumpHeights[work->height];

        offset.x = 0;
        offset.y = heights[stage];
        offset.z = 0;
        SetActorWPosOffset(actor, &offset);
    }
    if (work->timer > 0 || RailUnit_IsMovePending(unit)) {
        return FALSE;
    }
    {
        VecFx32 zero = { 0, 0, 0 };

        SetActorWPosOffset(actor, &zero);
    }
    SetActorMovementFlag(actor, 0x540);
    SetActorInitialGPosToNowGPos(actor);
    FldAct_SetDrawStatus(actor, DRAW_STATUS_STOP);
    FieldActor_NextAcmdState(actor);
    GFL_SndSEPlay(SEQ_SE_FLD_10);
    return TRUE;
}

static BOOL RailActor_AcJumpU1G8F0(FieldActor *actor) {
    RailActorAcmd_InitJumpWithSe(actor, DIR_UP, 0x2000, 8, DRAW_STATUS_WALK_8F, RAIL_JUMP_HIGH, 0x200);
    return TRUE;
}

static BOOL RailActor_AcJumpD1G8F0(FieldActor *actor) {
    RailActorAcmd_InitJumpWithSe(actor, DIR_DOWN, 0x2000, 8, DRAW_STATUS_WALK_8F, RAIL_JUMP_HIGH, 0x200);
    return TRUE;
}

static BOOL RailActor_AcJumpL1G8F0(FieldActor *actor) {
    RailActorAcmd_InitJumpWithSe(actor, DIR_LEFT, 0x2000, 8, DRAW_STATUS_WALK_8F, RAIL_JUMP_HIGH, 0x200);
    return TRUE;
}

static BOOL RailActor_AcJumpR1G8F0(FieldActor *actor) {
    RailActorAcmd_InitJumpWithSe(actor, DIR_RIGHT, 0x2000, 8, DRAW_STATUS_WALK_8F, RAIL_JUMP_HIGH, 0x200);
    return TRUE;
}

static BOOL RailActor_AcJumpU2G16F0(FieldActor *actor) {
    RailActorAcmd_InitJumpWithSe(actor, DIR_UP, 0x2000, 0x10, DRAW_STATUS_WALK_8F, RAIL_JUMP_HIGH, 0x100);
    return TRUE;
}

static BOOL RailActor_AcJumpD2G16F0(FieldActor *actor) {
    RailActorAcmd_InitJumpWithSe(actor, DIR_DOWN, 0x2000, 0x10, DRAW_STATUS_WALK_8F, RAIL_JUMP_HIGH, 0x100);
    return TRUE;
}

static BOOL RailActor_AcJumpL2G16F0(FieldActor *actor) {
    RailActorAcmd_InitJumpWithSe(actor, DIR_LEFT, 0x2000, 0x10, DRAW_STATUS_WALK_8F, RAIL_JUMP_HIGH, 0x100);
    return TRUE;
}

static BOOL RailActor_AcJumpR2G16F0(FieldActor *actor) {
    RailActorAcmd_InitJumpWithSe(actor, DIR_RIGHT, 0x2000, 0x10, DRAW_STATUS_WALK_8F, RAIL_JUMP_HIGH, 0x100);
    return TRUE;
}

// The turns, done in their first frame, have only a start and an end function. The moves along the rail, the steps
// in place and the jumps follow, in an order that the size sort lays out as the ROM does
const ActorAcmdFunc RAIL_ACMD_NEXT_STATE_TABLE[] = { RailActor_AcNextState, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_FACE_UP_TABLE[] = { RailActor_AcFaceU, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_FACE_DOWN_TABLE[] = { RailActor_AcFaceD, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_FACE_LEFT_TABLE[] = { RailActor_AcFaceL, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_FACE_RIGHT_TABLE[] = { RailActor_AcFaceR, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_UP_16F_TABLE[] = { RailActor_AcWalkU16F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_DOWN_16F_TABLE[] = { RailActor_AcWalkD16F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_LEFT_16F_TABLE[] = { RailActor_AcWalkL16F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_RIGHT_16F_TABLE[] = { RailActor_AcWalkR16F0, RailActor_AcWalk1,
                                                         RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_UP_8F_TABLE[] = { RailActor_AcWalkU8F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_DOWN_8F_TABLE[] = { RailActor_AcWalkD8F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_LEFT_8F_TABLE[] = { RailActor_AcWalkL8F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_RIGHT_8F_TABLE[] = { RailActor_AcWalkR8F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_UP_4F_TABLE[] = { RailActor_AcWalkU4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_DOWN_4F_TABLE[] = { RailActor_AcWalkD4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_LEFT_4F_TABLE[] = { RailActor_AcWalkL4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_RIGHT_4F_TABLE[] = { RailActor_AcWalkR4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_UP_2F_TABLE[] = { RailActor_AcWalkU2F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_DOWN_2F_TABLE[] = { RailActor_AcWalkD2F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_LEFT_2F_TABLE[] = { RailActor_AcWalkL2F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_WALK_RIGHT_2F_TABLE[] = { RailActor_AcWalkR2F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_DASH_UP_4F_TABLE[] = { RailActor_AcDashU4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_DASH_DOWN_4F_TABLE[] = { RailActor_AcDashD4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_DASH_LEFT_4F_TABLE[] = { RailActor_AcDashL4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_DASH_RIGHT_4F_TABLE[] = { RailActor_AcDashR4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_UP_32F_TABLE[] = { RailActor_AcStayWalkU32F0, RailActor_AcStayWalk1,
                                                           RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_DOWN_32F_TABLE[] = { RailActor_AcStayWalkD32F0, RailActor_AcStayWalk1,
                                                             RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_LEFT_32F_TABLE[] = { RailActor_AcStayWalkL32F0, RailActor_AcStayWalk1,
                                                             RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_RIGHT_32F_TABLE[] = { RailActor_AcStayWalkR32F0, RailActor_AcStayWalk1,
                                                              RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_UP_16F_TABLE[] = { RailActor_AcStayWalkU16F0, RailActor_AcStayWalk1,
                                                           RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_DOWN_16F_TABLE[] = { RailActor_AcStayWalkD16F0, RailActor_AcStayWalk1,
                                                             RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_LEFT_16F_TABLE[] = { RailActor_AcStayWalkL16F0, RailActor_AcStayWalk1,
                                                             RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_RIGHT_16F_TABLE[] = { RailActor_AcStayWalkR16F0, RailActor_AcStayWalk1,
                                                              RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_UP_8F_TABLE[] = { RailActor_AcStayWalkU8F0, RailActor_AcStayWalk1,
                                                          RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_DOWN_8F_TABLE[] = { RailActor_AcStayWalkD8F0, RailActor_AcStayWalk1,
                                                            RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_LEFT_8F_TABLE[] = { RailActor_AcStayWalkL8F0, RailActor_AcStayWalk1,
                                                            RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_RIGHT_8F_TABLE[] = { RailActor_AcStayWalkR8F0, RailActor_AcStayWalk1,
                                                             RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_UP_4F_TABLE[] = { RailActor_AcStayWalkU4F0, RailActor_AcStayWalk1,
                                                          RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_DOWN_4F_TABLE[] = { RailActor_AcStayWalkD4F0, RailActor_AcStayWalk1,
                                                            RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_LEFT_4F_TABLE[] = { RailActor_AcStayWalkL4F0, RailActor_AcStayWalk1,
                                                            RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_RIGHT_4F_TABLE[] = { RailActor_AcStayWalkR4F0, RailActor_AcStayWalk1,
                                                             RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_UP_2F_TABLE[] = { RailActor_AcStayWalkU2F0, RailActor_AcStayWalk1,
                                                          RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_DOWN_2F_TABLE[] = { RailActor_AcStayWalkD2F0, RailActor_AcStayWalk1,
                                                            RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_LEFT_2F_TABLE[] = { RailActor_AcStayWalkL2F0, RailActor_AcStayWalk1,
                                                            RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_STAY_WALK_RIGHT_2F_TABLE[] = { RailActor_AcStayWalkR2F0, RailActor_AcStayWalk1,
                                                             RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_JUMP_UP_1G_8F_TABLE[] = { RailActor_AcJumpU1G8F0, RailJump_Update,
                                                        RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_JUMP_DOWN_1G_8F_TABLE[] = { RailActor_AcJumpD1G8F0, RailJump_Update,
                                                          RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_JUMP_LEFT_1G_8F_TABLE[] = { RailActor_AcJumpL1G8F0, RailJump_Update,
                                                          RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_JUMP_RIGHT_1G_8F_TABLE[] = { RailActor_AcJumpR1G8F0, RailJump_Update,
                                                           RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_JUMP_UP_2G_16F_TABLE[] = { RailActor_AcJumpU2G16F0, RailJump_Update,
                                                         RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_JUMP_DOWN_2G_16F_TABLE[] = { RailActor_AcJumpD2G16F0, RailJump_Update,
                                                           RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_JUMP_LEFT_2G_16F_TABLE[] = { RailActor_AcJumpL2G16F0, RailJump_Update,
                                                           RailActor_AcSetAcmdEnd };
const ActorAcmdFunc RAIL_ACMD_JUMP_RIGHT_2G_16F_TABLE[] = { RailActor_AcJumpR2G16F0, RailJump_Update,
                                                            RailActor_AcSetAcmdEnd };
