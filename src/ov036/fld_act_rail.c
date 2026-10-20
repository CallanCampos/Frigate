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
    ActorRailJumpWork *work = func_ov012_02167154(actor);
    RailUnit *unit;
    VecFx32 pos;

    work->timer--;
    if (work->speed != 0) {
        s32 speed;

        unit = FldAct_GetRailUnit(actor);
        func_ov036_02195624(actor);
        RailUnit_GetCalcPos(unit, &pos);
        SetActorWPosValue(actor, &pos);
        if (work->progress >= 0x10000 && work->timer > 0) {
            work->progress = 0;
            func_ov036_02195640(actor, work->dir, 8);
            func_ov036_021959bc(actor, work->dir);
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

static BOOL func_ov036_02195480(FieldActor *actor) {
    func_ov036_02195354(actor, 0, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL func_ov036_021954a0(FieldActor *actor) {
    func_ov036_02195354(actor, 1, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL func_ov036_021954c4(FieldActor *actor) {
    func_ov036_02195354(actor, 2, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL func_ov036_021954e8(FieldActor *actor) {
    func_ov036_02195354(actor, 3, 0x2000, 8, 3, 0, 0x200);
    return TRUE;
}

static BOOL func_ov036_02195508(FieldActor *actor) {
    func_ov036_02195354(actor, 0, 0x2000, 0x10, 3, 0, 0x100);
    return TRUE;
}

static BOOL func_ov036_02195528(FieldActor *actor) {
    func_ov036_02195354(actor, 1, 0x2000, 0x10, 3, 0, 0x100);
    return TRUE;
}

static BOOL func_ov036_0219554c(FieldActor *actor) {
    func_ov036_02195354(actor, 2, 0x2000, 0x10, 3, 0, 0x100);
    return TRUE;
}

static BOOL func_ov036_0219556c(FieldActor *actor) {
    func_ov036_02195354(actor, 3, 0x2000, 0x10, 3, 0, 0x100);
    return TRUE;
}

// The turns, done in their first frame, have only a start and an end function. The moves along the rail, the steps
// in place and the jumps follow, in an order that the size sort lays out as the ROM does
const ActorAcmdFunc data_ov036_021cf394[] = { func_ov036_02194ef8, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf384[] = { func_ov036_02194f28, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf38c[] = { func_ov036_02194f34, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf37c[] = { func_ov036_02194f40, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf374[] = { func_ov036_02194f4c, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf588[] = { func_ov036_02194ff4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf57c[] = { func_ov036_02195004, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf570[] = { func_ov036_02195014, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf564[] = { func_ov036_02195024, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf558[] = { func_ov036_02195034, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf54c[] = { func_ov036_02195044, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf540[] = { func_ov036_02195054, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf534[] = { func_ov036_02195064, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf528[] = { func_ov036_02195074, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf51c[] = { func_ov036_02195084, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf510[] = { func_ov036_02195094, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf504[] = { func_ov036_021950a4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf4f8[] = { func_ov036_021950b4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf4c8[] = { func_ov036_021950c4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf4d4[] = { func_ov036_021950d4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf4ec[] = { func_ov036_021950e4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf594[] = { func_ov036_021950f4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf4bc[] = { func_ov036_02195104, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf4b0[] = { func_ov036_02195114, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf4a4[] = { func_ov036_02195124, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFunc data_ov036_021cf3b4[] = { func_ov036_0219519c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf5d0[] = { func_ov036_021951ac, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf480[] = { func_ov036_021951bc, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf474[] = { func_ov036_021951cc, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf468[] = { func_ov036_021951dc, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf45c[] = { func_ov036_021951ec, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf450[] = { func_ov036_021951fc, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf444[] = { func_ov036_0219520c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf438[] = { func_ov036_0219521c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf414[] = { func_ov036_0219522c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf420[] = { func_ov036_0219523c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf4e0[] = { func_ov036_0219524c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf5a0[] = { func_ov036_0219529c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf3fc[] = { func_ov036_021952ac, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf39c[] = { func_ov036_021952bc, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf48c[] = { func_ov036_021952cc, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf3d8[] = { func_ov036_0219525c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf3c0[] = { func_ov036_0219526c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf3cc[] = { func_ov036_0219527c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf42c[] = { func_ov036_0219528c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf5ac[] = { func_ov036_02195480, RailJump_Update, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf498[] = { func_ov036_021954a0, RailJump_Update, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf3e4[] = { func_ov036_021954c4, RailJump_Update, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf408[] = { func_ov036_021954e8, RailJump_Update, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf5b8[] = { func_ov036_02195508, RailJump_Update, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf3a8[] = { func_ov036_02195528, RailJump_Update, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf5c4[] = { func_ov036_0219554c, RailJump_Update, func_ov036_02194ee8 };
const ActorAcmdFunc data_ov036_021cf3f0[] = { func_ov036_0219556c, RailJump_Update, func_ov036_02194ee8 };
