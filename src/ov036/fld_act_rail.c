// The rail actors' jump along the rail, and the tables of the functions of their action commands, by the command's
// state, which ACTOR_ACTION_COMMAND_TABLES_RAIL (fldmmdl_movedata.c) lists by command. The name is descriptive. The
// size sort shows the file: its 8- and 12-byte tables and jump heights come before fldmmdl_rail.c's direction tables,
// which share a section that would sit among the 8-byte tables in one file
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
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
    FldAct_SetDrawStatus(actor, 0);
    FieldActor_NextAcmdState(actor);
    GFL_SndSEPlay(SEQ_SE_FLD_10);
    return TRUE;
}

static BOOL RailActor_AcJumpU1G8F0(FieldActor *actor) {
    func_ov036_02195354(actor, 0, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL RailActor_AcJumpD1G8F0(FieldActor *actor) {
    func_ov036_02195354(actor, 1, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL RailActor_AcJumpL1G8F0(FieldActor *actor) {
    func_ov036_02195354(actor, 2, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL RailActor_AcJumpR1G8F0(FieldActor *actor) {
    func_ov036_02195354(actor, 3, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL RailActor_AcJumpU2G16F0(FieldActor *actor) {
    func_ov036_02195354(actor, 0, 0x2000, 0x10, 3, 0, 0x100);
    return TRUE;
}

static BOOL RailActor_AcJumpD2G16F0(FieldActor *actor) {
    func_ov036_02195354(actor, 1, 0x2000, 0x10, 3, 0, 0x100);
    return TRUE;
}

static BOOL RailActor_AcJumpL2G16F0(FieldActor *actor) {
    func_ov036_02195354(actor, 2, 0x2000, 0x10, 3, 0, 0x100);
    return TRUE;
}

static BOOL RailActor_AcJumpR2G16F0(FieldActor *actor) {
    func_ov036_02195354(actor, 3, 0x2000, 0x10, 3, 0, 0x100);
    return TRUE;
}

// The turns, done in their first frame, have only a start and an end function. The moves along the rail, the steps
// in place and the jumps follow, in an order that the size sort lays out as the ROM does
const ActorAcmdFunc data_ov036_021cf394[] = { RailActor_AcNextState, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf384[] = { RailActor_AcFaceU, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf38c[] = { RailActor_AcFaceD, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf37c[] = { RailActor_AcFaceL, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf374[] = { RailActor_AcFaceR, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf588[] = { RailActor_AcWalkU16F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf57c[] = { RailActor_AcWalkD16F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf570[] = { RailActor_AcWalkL16F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf564[] = { RailActor_AcWalkR16F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf558[] = { RailActor_AcWalkU8F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf54c[] = { RailActor_AcWalkD8F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf540[] = { RailActor_AcWalkL8F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf534[] = { RailActor_AcWalkR8F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf528[] = { RailActor_AcWalkU4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf51c[] = { RailActor_AcWalkD4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf510[] = { RailActor_AcWalkL4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf504[] = { RailActor_AcWalkR4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf4f8[] = { RailActor_AcWalkU2F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf4c8[] = { RailActor_AcWalkD2F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf4d4[] = { RailActor_AcWalkL2F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf4ec[] = { RailActor_AcWalkR2F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf594[] = { RailActor_AcDashU4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf4bc[] = { RailActor_AcDashD4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf4b0[] = { RailActor_AcDashL4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf4a4[] = { RailActor_AcDashR4F0, RailActor_AcWalk1, RailActor_AcWalkEnd };
const ActorAcmdFunc data_ov036_021cf3b4[] = { RailActor_AcStayWalkU32F0, RailActor_AcStayWalk1,
                                              RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf5d0[] = { RailActor_AcStayWalkD32F0, RailActor_AcStayWalk1,
                                              RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf480[] = { RailActor_AcStayWalkL32F0, RailActor_AcStayWalk1,
                                              RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf474[] = { RailActor_AcStayWalkR32F0, RailActor_AcStayWalk1,
                                              RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf468[] = { RailActor_AcStayWalkU16F0, RailActor_AcStayWalk1,
                                              RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf45c[] = { RailActor_AcStayWalkD16F0, RailActor_AcStayWalk1,
                                              RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf450[] = { RailActor_AcStayWalkL16F0, RailActor_AcStayWalk1,
                                              RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf444[] = { RailActor_AcStayWalkR16F0, RailActor_AcStayWalk1,
                                              RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf438[] = { RailActor_AcStayWalkU8F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf414[] = { RailActor_AcStayWalkD8F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf420[] = { RailActor_AcStayWalkL8F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf4e0[] = { RailActor_AcStayWalkR8F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf5a0[] = { RailActor_AcStayWalkU4F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf3fc[] = { RailActor_AcStayWalkD4F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf39c[] = { RailActor_AcStayWalkL4F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf48c[] = { RailActor_AcStayWalkR4F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf3d8[] = { RailActor_AcStayWalkU2F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf3c0[] = { RailActor_AcStayWalkD2F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf3cc[] = { RailActor_AcStayWalkL2F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf42c[] = { RailActor_AcStayWalkR2F0, RailActor_AcStayWalk1, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf5ac[] = { RailActor_AcJumpU1G8F0, RailJump_Update, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf498[] = { RailActor_AcJumpD1G8F0, RailJump_Update, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf3e4[] = { RailActor_AcJumpL1G8F0, RailJump_Update, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf408[] = { RailActor_AcJumpR1G8F0, RailJump_Update, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf5b8[] = { RailActor_AcJumpU2G16F0, RailJump_Update, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf3a8[] = { RailActor_AcJumpD2G16F0, RailJump_Update, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf5c4[] = { RailActor_AcJumpL2G16F0, RailJump_Update, RailActor_AcSetAcmdEnd };
const ActorAcmdFunc data_ov036_021cf3f0[] = { RailActor_AcJumpR2G16F0, RailJump_Update, RailActor_AcSetAcmdEnd };
