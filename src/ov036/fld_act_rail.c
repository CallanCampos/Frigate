// How the no-grid maps' actors move along the rails: the jumps of their action commands, the rail position and rail
// unit every one of them holds, and the move codes of the actors that wait and walk there. The name is descriptive
#include "types.h"
#include "field/field_actor.h"
#include "field/field_map.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_rail.h"
#include "field/zone.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "nitro/mi.h"

// The place an actor holds on the rail, with the rail unit that follows it, at the start of the actor's work for its
// move code
typedef struct {
    RailUnit *unit;
    RailPosition position;
} ActorRailWork;

// A set of directions an actor chooses among, by the number the actor's data gives
typedef struct {
    u32 id;
    const s32 *list;
} DirListEntry;

typedef struct {
    ActorRailWork rail;
    s16 wait;
    u16 dirs;
} ActorRailWaitWork;

typedef struct {
    ActorRailWork rail;
    u8 state : 4;
    u8 dir : 4;
    s8 wait;
    s8 side;
    u8 front;
} ActorRailWalkWork;

typedef struct {
    ActorRailWork rail;
    u8 state;
    u8 reversed;
    s8 side;
    u8 front;
} ActorRailPatrolWork;

static BOOL func_ov036_02195378(FieldActor *actor);
static BOOL func_ov036_02195480(FieldActor *actor);
static BOOL func_ov036_021954a0(FieldActor *actor);
static BOOL func_ov036_021954c4(FieldActor *actor);
static BOOL func_ov036_021954e8(FieldActor *actor);
static BOOL func_ov036_02195508(FieldActor *actor);
static BOOL func_ov036_02195528(FieldActor *actor);
static BOOL func_ov036_0219554c(FieldActor *actor);
static BOOL func_ov036_0219556c(FieldActor *actor);
static u32 func_ov036_02195668(FieldActor *actor, const RailPosition *from, const RailPosition *to, u32 *tile);
static BOOL func_ov036_02195768(FieldActor *actor, const RailPosition *pos);
static u32 func_ov036_02195c2c(u32 dir);
static u32 func_ov036_02195c34(s32 speed);
static void *func_ov036_02195c5c(FieldActor *actor, u32 size);
static BOOL func_ov036_02195c9c(FieldActor *actor, s32 front, s32 side, const RailPosition *pos);
static s32 func_ov036_02195cec(const s32 *list, s32 value);
static s32 func_ov036_02195d04(const s32 *list, s32 value);
static s32 func_ov036_02195d18(u32 key, s32 value);
static const s32 *func_ov036_02195d34(u32 key);
static void func_ov036_02195d50(ActorRailWork *rail, FieldActor *actor);
static void func_ov036_02195d6c(ActorRailWork *rail, FieldActor *actor);
static void func_ov036_02195d88(ActorRailWork *rail, FieldActor *actor);
static void func_ov036_02195dac(ActorRailWork *rail, FieldActor *actor);
static void SetActorPositionRailCoordinates(ActorRailWork *rail, const RailPosition *pos);
static void func_ov036_02195de0(ActorRailWork *rail, FieldActor *actor);
static void func_ov036_02195e0c(FieldActor *actor, u16 dirs);
static void func_ov036_02195e40(FieldActor *actor);
static void func_ov036_02195e84(FieldActor *actor);
static void func_ov036_02195e94(FieldActor *actor, u32 dir);
static void func_ov036_02195ee8(FieldActor *actor);
static void func_ov036_02196070(FieldActor *actor);
static void func_ov036_02196080(FieldActor *actor);
static void func_ov036_021960b8(FieldActor *actor);
static void func_ov036_021961f8(FieldActor *actor);

const ActorAcmdFuncs data_ov036_021cf3a8 = { func_ov036_02195528, func_ov036_02195378, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf3b4 = { func_ov036_0219519c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf3c0 = { func_ov036_0219526c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf3cc = { func_ov036_0219527c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf3d8 = { func_ov036_0219525c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf3e4 = { func_ov036_021954c4, func_ov036_02195378, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf3f0 = { func_ov036_0219556c, func_ov036_02195378, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf3fc = { func_ov036_021952ac, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf408 = { func_ov036_021954e8, func_ov036_02195378, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf414 = { func_ov036_0219522c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf420 = { func_ov036_0219523c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf42c = { func_ov036_0219528c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf438 = { func_ov036_0219521c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf444 = { func_ov036_0219520c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf450 = { func_ov036_021951fc, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf45c = { func_ov036_021951ec, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf468 = { func_ov036_021951dc, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf474 = { func_ov036_021951cc, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf480 = { func_ov036_021951bc, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf48c = { func_ov036_021952cc, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf498 = { func_ov036_021954a0, func_ov036_02195378, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf4a4 = { func_ov036_02195124, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf4b0 = { func_ov036_02195114, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf4bc = { func_ov036_02195104, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf4c8 = { func_ov036_021950c4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf4d4 = { func_ov036_021950d4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf4e0 = { func_ov036_0219524c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf4ec = { func_ov036_021950e4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf4f8 = { func_ov036_021950b4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf504 = { func_ov036_021950a4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf510 = { func_ov036_02195094, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf51c = { func_ov036_02195084, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf528 = { func_ov036_02195074, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf534 = { func_ov036_02195064, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf540 = { func_ov036_02195054, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf54c = { func_ov036_02195044, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf558 = { func_ov036_02195034, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf564 = { func_ov036_02195024, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf570 = { func_ov036_02195014, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf57c = { func_ov036_02195004, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf588 = { func_ov036_02194ff4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf594 = { func_ov036_021950f4, func_ov036_02194f94, func_ov036_02194fe8 };
const ActorAcmdFuncs data_ov036_021cf5a0 = { func_ov036_0219529c, func_ov036_02195168, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf5ac = { func_ov036_02195480, func_ov036_02195378, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf5b8 = { func_ov036_02195508, func_ov036_02195378, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf5c4 = { func_ov036_0219554c, func_ov036_02195378, func_ov036_02194ee8 };
const ActorAcmdFuncs data_ov036_021cf5d0 = { func_ov036_021951ac, func_ov036_02195168, func_ov036_02194ee8 };

// How high an actor is above the rail at each stage of its jump, by the kind of jump
static const fx32 RAIL_JUMP_HEIGHTS_HIGH[16] = { 0x4000, 0x6000, 0x8000, 0xa000, 0xb000, 0xc000, 0xc000, 0xc000,
                                                 0xb000, 0xa000, 0x9000, 0x8000, 0x6000, 0x4000, 0,      0 };
static const fx32 RAIL_JUMP_HEIGHTS_LOW[16] = { 0,      0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x6000, 0x6000,
                                                0x5000, 0x5000, 0x4000, 0x3000, 0x2000, 0,      0,      0 };
static const fx32 RAIL_JUMP_HEIGHTS_MID[16] = { 0x2000, 0x4000, 0x6000, 0x8000, 0x9000, 0xa000, 0xa000, 0xa000,
                                                0x9000, 0x8000, 0x6000, 0x5000, 0x3000, 0x2000, 0,      0 };

// The angle of each direction
static const u16 DIR_ANGLES[4] = { 0, 0x8000, 0x4000, 0xc000 };

// The directions an actor chooses among, ended by 9
static const s32 DIRS_UP_LEFT[3] = { 0, 2, 9 };
static const s32 DIRS_UP_RIGHT[3] = { 0, 3, 9 };
static const s32 DIRS_UP_DOWN[3] = { 0, 1, 9 };
static const s32 DIRS_DOWN_LEFT[3] = { 1, 2, 9 };
static const s32 DIRS_DOWN_RIGHT[3] = { 1, 3, 9 };
static const s32 DIRS_LEFT_RIGHT[3] = { 2, 3, 9 };
static const s32 DIRS_UP_LEFT_RIGHT[4] = { 0, 2, 3, 9 };
static const s32 DIRS_DOWN_LEFT_RIGHT[4] = { 1, 2, 3, 9 };
static const s32 DIRS_UP_DOWN_LEFT[4] = { 0, 1, 2, 9 };
static const s32 DIRS_UP_DOWN_RIGHT[4] = { 0, 1, 3, 9 };

// How long an actor waits, ended by -1
static const s32 WAIT_TIMES[5] = { 0x10, 0x20, 0x30, 0x40, -1 };
static const s32 DIRS_ALL[5] = { 0, 1, 2, 3, 9 };

// The direction an actor ends up facing in, by the side of the rail's turn and the direction it was in
static const u8 TURN_DIRS[5][4] = { { 0, 0, 0, 0 }, { 0, 1, 2, 3 }, { 2, 3, 1, 0 }, { 1, 0, 3, 2 }, { 3, 2, 0, 1 } };

// The sets of directions, by the number in the actor's data, ended by 11
static const DirListEntry DIR_LISTS[] = {
    { 0, DIRS_ALL },           { 1, DIRS_UP_LEFT },       { 2, DIRS_UP_RIGHT },
    { 3, DIRS_DOWN_LEFT },     { 4, DIRS_DOWN_RIGHT },    { 5, DIRS_UP_DOWN_LEFT },
    { 6, DIRS_UP_DOWN_RIGHT }, { 7, DIRS_UP_LEFT_RIGHT }, { 8, DIRS_DOWN_LEFT_RIGHT },
    { 9, DIRS_UP_DOWN },       { 10, DIRS_LEFT_RIGHT },   { 11, NULL },
};

static const fx32 *sJumpHeights[3] = { RAIL_JUMP_HEIGHTS_HIGH, RAIL_JUMP_HEIGHTS_LOW, RAIL_JUMP_HEIGHTS_MID };

// Whether the actor lacks a flag, such as 0x2000, SetRailActorFlag's flag of the actors on the rails
static inline BOOL IsActorFlagClear(FieldActor *actor, u32 flag) {
    return !CheckActorFlag(actor, flag);
}

// The actor's work for its move code
#define RAIL_WORK(actor) ((ActorRailWork *)GetNPCRailPosPtrAddr(actor))

static BOOL func_ov036_02195378(FieldActor *actor) {
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
        u16 stage = work->phase >> 8;

        VEC_Set(&offset, 0, sJumpHeights[work->height][stage], 0);
        SetActorWPosOffset(actor, &offset);
    }
    if (work->timer > 0 || func_ov036_021b09f4(unit)) {
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
    GFL_SndSEPlay(0x67b);
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

void SetActorInitedPositionRail(FieldActor *actor, const RailPosition *position) {
    ActorRailWork *rail = RAIL_WORK(actor);
    VecFx32 pos;

    RailUnit_SetRailPos(rail->unit, position);
    SetActorPositionRailCoordinates(rail, position);
    RailUnit_GetCalcPos(rail->unit, &pos);
    SetActorWPosValue(actor, &pos);
}

void GetActorRailPos(FieldActor *actor, RailPosition *position) {
    FieldRailSystem *railSystem =
        FieldNoGridMapper_GetRailSystem(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    ActorRailWork *rail = RAIL_WORK(actor);

    if (func_ov036_021b09f4(rail->unit)) {
        func_ov036_021b0814(rail->unit, func_ov036_021b09ec(rail->unit), position, NULL);
    } else {
        func_ov036_021b0774(rail->unit, position);
    }
}

void func_ov036_02195604(FieldActor *actor, RailPosition *position) {
    func_ov036_021b0774(RAIL_WORK(actor)->unit, position);
}

RailUnit *FldAct_GetRailUnit(FieldActor *actor) {
    return RAIL_WORK(actor)->unit;
}

void func_ov036_02195624(FieldActor *actor) {
    ActorRailWork *rail = RAIL_WORK(actor);

    func_ov036_021b0ac0(rail->unit);
    func_ov036_02195d6c(rail, actor);
}

void func_ov036_02195640(FieldActor *actor, u16 dir, u32 speed) {
    ActorRailWork *rail = RAIL_WORK(actor);
    u32 railDir = func_ov036_02195c2c(dir);

    func_ov036_021b0a94(rail->unit, func_ov036_02195c34(speed), railDir);
}

static u32 func_ov036_02195668(FieldActor *actor, const RailPosition *from, const RailPosition *to, u32 *tile) {
    u32 flags = 0;
    ActorRailWork *rail = RAIL_WORK(actor);
    NoGridMapper *mapper = FldActSys_GetNoGridMapper(GetActorMModelSystem(actor));

    if (!func_ov036_021b0794(rail->unit, from)) {
        flags |= 1;
    }
    if (!func_ov036_021b0794(rail->unit, to)) {
        flags |= 1;
    }
    if (!FldAct_IsMapAttrDisabled(actor)) {
        u32 tileType = FieldNoGridMapper_GetTileAtPos(mapper, to);

        if (GetTileFlags(tileType) & 1) {
            flags |= 2;
        }
        if (tile != NULL) {
            *tile = tileType;
        }
    }
    if (func_ov036_02195768(actor, to)) {
        flags |= 4;
    }
    return flags;
}

u32 func_ov036_021956e8(FieldActor *actor, u16 dir) {
    return func_ov036_021956f4(actor, dir, NULL);
}

u32 func_ov036_021956f4(FieldActor *actor, u16 dir, u32 *tile) {
    FieldRailSystem *railSystem =
        FieldNoGridMapper_GetRailSystem(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    ActorRailWork *rail = RAIL_WORK(actor);
    RailPosition from;
    RailPosition to;
    BOOL moved;

    func_ov036_021b0774(rail->unit, &from);
    moved = GetActorFaceDir(actor);
    if (moved == dir) {
        moved = func_ov036_021b0814(rail->unit, func_ov036_02195c2c(dir), &to, NULL);
    } else {
        moved = func_ov036_021b068c(railSystem, &from, func_ov036_02195c2c(dir), &to);
    }
    if (moved) {
        return func_ov036_02195668(actor, &from, &to, tile);
    }
    return 1;
}

static BOOL func_ov036_02195768(FieldActor *actor, const RailPosition *pos) {
    MMSys *mmSys = GetActorMModelSystem(actor);
    FieldRailSystem *railSystem = FieldNoGridMapper_GetRailSystem(FldActSys_GetNoGridMapper(mmSys));
    FieldActor *other;
    u32 index = 0;
    VecFx32 target;
    VecFx32 otherPos;
    RailUnit *unit;
    fx32 radius;

    func_ov036_021b06ec(railSystem, pos, &target);
    radius = FX_Div(func_ov036_021b05ec(railSystem), 0x4000);
    while (NextActor(mmSys, &other, &index) == 1) {
        if (other == actor) {
            continue;
        }
        if (CheckActorFlag(other, 0x80)) {
            continue;
        }
        if (IsActorFlagClear(other, 0x2000)) {
            continue;
        }
        unit = FldAct_GetRailUnit(other);
        if (func_ov036_021b09f4(unit)) {
            func_ov036_021b0814(unit, func_ov036_021b09ec(unit), NULL, &otherPos);
            if (func_ov036_021b0d64(&target, &otherPos, radius)) {
                return TRUE;
            }
        }
        func_ov036_021b0888(unit, &otherPos);
        if (func_ov036_021b0d64(&target, &otherPos, radius)) {
            return TRUE;
        }
    }
    return FALSE;
}

FieldActor *FindActorByRailPos(MMSys *mmSys, const RailPosition *pos, BOOL checkInit) {
    return func_ov036_0219584c(mmSys, pos, checkInit, NULL);
}

FieldActor *func_ov036_0219584c(MMSys *mmSys, const RailPosition *pos, BOOL checkInit, FieldActor *exclude) {
    u32 index = 0;
    FieldRailSystem *railSystem;
    FieldActor *other;
    VecFx32 target;
    VecFx32 otherPos;
    fx32 radius;

    railSystem = FieldNoGridMapper_GetRailSystem(FldActSys_GetNoGridMapper(mmSys));
    func_ov036_021b06ec(railSystem, pos, &target);
    radius = FX_Div(func_ov036_021b05ec(railSystem), 0x4000);
    while (NextActor(mmSys, &other, &index) == 1) {
        RailPosition initRail;
        RailPosition otherRail;

        if (IsActorFlagClear(other, 0x2000)) {
            continue;
        }
        if (exclude != NULL && exclude == other) {
            continue;
        }
        if (GetActorUID(other) == 0xff) {
            continue;
        }
        if (checkInit) {
            func_ov036_02195604(other, &initRail);
            func_ov036_021b06ec(railSystem, &initRail, &otherPos);
            if (func_ov036_021b0d64(&target, &otherPos, radius)) {
                return other;
            }
        }
        GetActorRailPos(other, &otherRail);
        func_ov036_021b06ec(railSystem, &otherRail, &otherPos);
        if (func_ov036_021b0d64(&target, &otherPos, radius)) {
            return other;
        }
    }
    return NULL;
}

void func_ov036_02195928(FieldActor *actor, RailPosition *position) {
    ActorRailWork *rail;

    FldActSys_GetNoGridMapper(GetActorMModelSystem(actor));
    rail = RAIL_WORK(actor);
    func_ov036_021b0814(rail->unit, ConvDirToRailDir(GetActorFaceDir(actor)), position, NULL);
}

BOOL GetActorRailPosForDir(FieldActor *actor, u16 dir, RailPosition *position) {
    FieldRailSystem *railSystem =
        FieldNoGridMapper_GetRailSystem(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    u16 faceDir = GetActorFaceDir(actor);
    ActorRailWork *rail = RAIL_WORK(actor);

    if (faceDir != dir) {
        GetActorRailPos(actor, position);
        return func_ov036_021b068c(railSystem, position, ConvDirToRailDir(dir), position);
    }
    return func_ov036_021b0814(rail->unit, ConvDirToRailDir(dir), position, NULL);
}

void func_ov036_021959bc(FieldActor *actor, u16 dir) {
    FieldRailSystem *railSystem =
        FieldNoGridMapper_GetRailSystem(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    RailPosition rail;
    VecFx32 pos;

    GetActorRailPos(actor, &rail);
    func_ov036_021b0644(railSystem, &rail, ConvDirToRailDir(dir), &pos);
    SetActorInitialGPosX(actor, GetGPosX(actor));
    SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
    SetActorInitialGPosZ(actor, GetGPosZ(actor));
    SetActorGPosX(actor, (pos.x >> 4) / 4096);
    SetActorGPosY(actor, (pos.y >> 4) / 4096);
    SetActorGPosZ(actor, (pos.z >> 4) / 4096);
}

void func_ov036_02195a58(FieldActor *actor, VecFx16 *dest) {
    FldAct_GetRailUnit(actor);
    func_ov036_02195a78(actor, (u16)(u32)GetActorFaceDir(actor), dest);
}

void func_ov036_02195a78(FieldActor *actor, u32 dir, VecFx16 *dest) {
    RailUnit *unit = FldAct_GetRailUnit(actor);
    u16 angle;
    fx16 cos;
    MtxFx33 mtx;
    VecFx32 vec;

    func_ov036_021b0a04(unit, dest);
    angle = DIR_ANGLES[TURN_DIRS[func_ov036_021b0a8c(unit)][dir]];
    cos = FX_CosIdx(angle);
    dest->y = FX_MUL(cos, dest->y);
    VEC_Set(&vec, dest->x, dest->y, dest->z);
    MTX_RotY33_(&mtx, FX_SinIdx(angle), cos);
    MTX_MultVec33(&vec, &mtx, &vec);
    VEC_Normalize(&vec, &vec);
    VEC_Fx16Set(dest, vec.x, vec.y, vec.z);
}

void func_ov036_02195b1c(FieldActor *actor) {
    func_ov036_02195c5c(actor, 0xc);
}

void func_ov036_02195b28(FieldActor *actor) {
    func_ov036_02195d6c(RAIL_WORK(actor), actor);
}

void func_ov036_02195b38(FieldActor *actor) {
    func_ov036_02195d88(RAIL_WORK(actor), actor);
}

void func_ov036_02195b48(FieldActor *actor) {
    func_ov036_02195dac(RAIL_WORK(actor), actor);
}

void func_ov036_02195b58(FieldActor *actor) {
    func_ov036_02195e0c(actor, 0);
}

void func_ov036_02195b64(FieldActor *actor) {
    func_ov036_02195e0c(actor, 1);
}

void func_ov036_02195b70(FieldActor *actor) {
    func_ov036_02195e0c(actor, 2);
}

void func_ov036_02195b7c(FieldActor *actor) {
    func_ov036_02195e0c(actor, 3);
}

void func_ov036_02195b88(FieldActor *actor) {
    func_ov036_02195e0c(actor, 4);
}

void func_ov036_02195b94(FieldActor *actor) {
    func_ov036_02195e0c(actor, 5);
}

void func_ov036_02195ba0(FieldActor *actor) {
    func_ov036_02195e0c(actor, 6);
}

void func_ov036_02195bac(FieldActor *actor) {
    func_ov036_02195e0c(actor, 7);
}

void func_ov036_02195bb8(FieldActor *actor) {
    func_ov036_02195e0c(actor, 8);
}

void func_ov036_02195bc4(FieldActor *actor) {
    func_ov036_02195e0c(actor, 9);
}

void func_ov036_02195bd0(FieldActor *actor) {
    func_ov036_02195e0c(actor, 10);
}

void func_ov036_02195bdc(FieldActor *actor) {
    func_ov036_02195e40(actor);
}

void func_ov036_02195be4(FieldActor *actor) {
    func_ov036_02195e84(actor);
}

void func_ov036_02195bec(FieldActor *actor) {
    func_ov036_02195e94(actor, 0);
}

void func_ov036_02195bf8(FieldActor *actor) {
    func_ov036_02195e94(actor, 9);
}

void func_ov036_02195c04(FieldActor *actor) {
    func_ov036_02195ee8(actor);
}

void func_ov036_02195c0c(FieldActor *actor) {
    func_ov036_02196070(actor);
}

void func_ov036_02195c14(FieldActor *actor) {
    func_ov036_02196080(actor);
}

void func_ov036_02195c1c(FieldActor *actor) {
    func_ov036_021960b8(actor);
}

void func_ov036_02195c24(FieldActor *actor) {
    func_ov036_021961f8(actor);
}

static u32 func_ov036_02195c2c(u32 dir) {
    return ConvDirToRailDir(dir);
}

static u32 func_ov036_02195c34(s32 speed) {
    u32 result = 1;

    switch (speed) {
    case 0x10:
        result = 0;
        break;
    case 4:
        result = 2;
        break;
    case 2:
        result = 3;
        break;
    case 8:
        result = 1;
        break;
    }
    return result;
}

static void *func_ov036_02195c5c(FieldActor *actor, u32 size) {
    RailPosition posData;
    RailPosition *pos = &posData;
    BOOL set = GetActorInitPositionRail(actor, pos);
    ActorRailWork *rail = (ActorRailWork *)ClearActorPositionBlock(actor, size);

    func_ov036_02195d50(rail, actor);
    if (set) {
    } else {
        MI_CpuFill8(pos, 0, sizeof(RailPosition));
    }
    SetActorInitedPositionRail(actor, pos);
    return rail;
}

static BOOL func_ov036_02195c9c(FieldActor *actor, s32 front, s32 side, const RailPosition *pos) {
    s32 width = GetActorWalkAreaW(actor);
    s32 height = GetActorWalkAreaH(actor);

    if (width != -1) {
        s32 min = side - width;
        s32 max = side + width;

        if (min > pos->posSide || max < pos->posSide) {
            return TRUE;
        }
    }
    if (height != -1) {
        s32 min = front - height;
        s32 max = front + height;

        if (min > pos->posFront || max < pos->posFront) {
            return TRUE;
        }
    }
    return FALSE;
}

static s32 func_ov036_02195cec(const s32 *list, s32 value) {
    s32 i = 0;

    while (list[i] != value) {
        i++;
    }
    return i;
}

static s32 func_ov036_02195d04(const s32 *list, s32 value) {
    return list[GFL_RandomLC(func_ov036_02195cec(list, value))];
}

static s32 func_ov036_02195d18(u32 key, s32 value) {
    const s32 *list = func_ov036_02195d34(key);

    return list[GFL_RandomLC(func_ov036_02195cec(list, value))];
}

static const s32 *func_ov036_02195d34(u32 key) {
    const DirListEntry *entry = DIR_LISTS;
    u32 id = 0;

    do {
        if (id == key) {
            return entry->list;
        }
        entry++;
        id = entry->id;
    } while (id != 0xb);
    return NULL;
}

static void func_ov036_02195d50(ActorRailWork *rail, FieldActor *actor) {
    rail->unit = FieldNoGridMapper_AcquireUnit(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    rail->position.componentIsLine = 2;
}

static void func_ov036_02195d6c(ActorRailWork *rail, FieldActor *actor) {
    RailPosition pos;

    func_ov036_021b0774(rail->unit, &pos);
    SetActorPositionRailCoordinates(rail, &pos);
}

static void func_ov036_02195d88(ActorRailWork *rail, FieldActor *actor) {
    FieldNoGridMapper_ReleaseUnit(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)), rail->unit);
    ClearActorFlag(actor, 0x2000);
}

static void func_ov036_02195dac(ActorRailWork *rail, FieldActor *actor) {
    rail->unit = FieldNoGridMapper_AcquireUnit(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    func_ov036_02195de0(rail, actor);
}

static void SetActorPositionRailCoordinates(ActorRailWork *rail, const RailPosition *pos) {
    rail->position = *pos;
}

static void func_ov036_02195de0(ActorRailWork *rail, FieldActor *actor) {
    if (rail->position.componentIsLine != 2) {
        VecFx32 pos;

        RailUnit_SetRailPos(rail->unit, &rail->position);
        RailUnit_GetCalcPos(rail->unit, &pos);
        SetActorWPosValue(actor, &pos);
    }
}

static void func_ov036_02195e0c(FieldActor *actor, u16 dirs) {
    ActorRailWaitWork *work = func_ov036_02195c5c(actor, 0x10);

    work->wait = func_ov036_02195d04(WAIT_TIMES, -1);
    work->dirs = dirs;
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
}

static void func_ov036_02195e40(FieldActor *actor) {
    ActorRailWaitWork *work = (ActorRailWaitWork *)GetNPCRailPosPtrAddr(actor);

    work->wait--;
    if (work->wait <= 0) {
        work->wait = func_ov036_02195d04(WAIT_TIMES, -1);
        CheckSetActorFaceDir(actor, func_ov036_02195d18(work->dirs, 9));
    }
    func_ov036_02195d6c(&work->rail, actor);
}

static void func_ov036_02195e84(FieldActor *actor) {
    func_ov036_02195d88(RAIL_WORK(actor), actor);
}

static void func_ov036_02195e94(FieldActor *actor, u32 dir) {
    ActorRailWalkWork *work = func_ov036_02195c5c(actor, 0x10);
    RailPosition pos;

    GetActorRailPos(actor, &pos);
    work->state = 0;
    work->wait = 0;
    work->dir = dir;
    work->front = pos.posFront;
    work->side = pos.posSide;
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
}

static void func_ov036_02195ee8(FieldActor *actor) {
    ActorRailWalkWork *work = (ActorRailWalkWork *)GetNPCRailPosPtrAddr(actor);

    switch (work->state) {
    case 0:
        FldAct_ClearMoving(actor);
        FldAct_ClearMoveEnd(actor);
        FldAct_SetLocalAcmd(actor, (u32)GetAcmdForDir((u32)GetActorFaceDir(actor), 0));
        work->state++;
        break;
    case 1:
        if (!func_ov036_0218f01c(actor)) {
            break;
        }
        work->wait = func_ov036_02195d04(WAIT_TIMES, -1);
        work->state++;
    case 2:
        work->wait--;
        if (work->wait != 0) {
            break;
        }
        work->state++;
    case 3: {
        u32 dir = func_ov036_02195d18(work->dir, 9);
        RailPosition current;
        RailPosition next;

        ChangeActorDirection(actor, dir);
        if (func_ov036_021956e8(actor, dir) != 0) {
            work->state = 0;
            break;
        }
        GetActorRailPos(actor, &current);
        GetActorRailPosForDir(actor, dir, &next);
        if (current.componentId != next.componentId) {
            work->state = 0;
            break;
        }
        if (func_ov036_02195c9c(actor, work->front, work->side, &next)) {
            work->state = 0;
            break;
        }
        FldAct_SetLocalAcmd(actor, (u32)GetAcmdForDir(dir, 0xc));
        FldAct_SetMoving(actor);
        work->state++;
    }
    case 4:
        if (func_ov036_0218f01c(actor)) {
            FldAct_ClearMoving(actor);
            work->state = 0;
        }
        break;
    }
    func_ov036_02195d6c(&work->rail, actor);
}

static void func_ov036_02196070(FieldActor *actor) {
    func_ov036_02195d88(RAIL_WORK(actor), actor);
}

static void func_ov036_02196080(FieldActor *actor) {
    ActorRailPatrolWork *work = func_ov036_02195c5c(actor, 0x10);
    RailPosition pos;

    GetActorRailPos(actor, &pos);
    work->state = 0;
    work->reversed = 0;
    work->front = pos.posFront;
    work->side = pos.posSide;
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
}

static void func_ov036_021960b8(FieldActor *actor) {
    ActorRailPatrolWork *work = (ActorRailPatrolWork *)GetNPCRailPosPtrAddr(actor);

    switch (work->state) {
    case 0: {
        u32 dir = GetDefaultActorDir(actor);

        if (work->reversed == 1) {
            dir = GetInverseDirection((u16)dir);
        }
        ChangeActorDirection(actor, dir);
        work->state = 1;
    }
    case 1: {
        u32 blocked;
        u32 acmd;
        BOOL outside;
        BOOL otherComponent;
        u32 dir;
        RailPosition current;
        RailPosition next;

        GetActorRailPos(actor, &current);
        if (work->reversed != 0 && work->front == current.posFront && work->side == current.posSide) {
            ChangeActorDirection(actor, (u32)GetInverseDirection(GetActorMotionDir(actor)));
            work->reversed = 0;
        }
        otherComponent = FALSE;
        dir = (u32)GetActorMotionDir(actor);
        blocked = func_ov036_021956e8(actor, dir);
        GetActorRailPosForDir(actor, dir, &next);
        outside = func_ov036_02195c9c(actor, work->front, work->side, &next);
        if (current.componentId != next.componentId) {
            otherComponent = TRUE;
        }
        if ((blocked & 1) || outside || otherComponent) {
            work->reversed = 1;
            dir = GetInverseDirection((u16)dir);
            blocked = func_ov036_021956e8(actor, dir);
            GetActorRailPosForDir(actor, dir, &next);
        }
        acmd = 0xc;
        if (blocked) {
            acmd = 0x20;
        }
        FldAct_SetLocalAcmd(actor, (u32)GetAcmdForDir(dir, (u16)acmd));
        FldAct_SetMoving(actor);
        work->state = 2;
        break;
    }
    case 2:
        if (func_ov036_0218f01c(actor) == 1) {
            FldAct_ClearMoving(actor);
            work->state = 0;
        }
        break;
    }
    func_ov036_02195d6c(&work->rail, actor);
}

static void func_ov036_021961f8(FieldActor *actor) {
    func_ov036_02195d88(RAIL_WORK(actor), actor);
}

// The curves' conversions between rail and world positions, and the rails' camera functions, by their IDs in the rail
// data. Names from swan
static RailToWorldFunc RAIL_CONV_RAIL_TO_WORLD_FUNCS[3] = {
    RailMath_RailToWorldNoCurve,
    RailMath_RailToWorldSlerpXZ,
    RailMath_RailToWorldSlerpXYZ,
};
static WorldToRailFunc RAIL_CONV_WORLD_TO_RAIL_FUNCS[3] = {
    RailMath_WorldToRailNoCurve,
    RailMath_WorldToRailSlerp,
    RailMath_WorldToRailSlerp,
};
static RailCameraFunc RAIL_CAMERA_CURVE_FUNC_SET[20] = {
    RailCameraFuncSetStaticPYR, RailCameraFuncSetStaticPYDist, func_ov036_021b2cfc, func_ov036_021b2eb0,
    func_ov036_021b2f04,        func_ov036_021b309c,           func_ov036_021b3164, func_ov036_021b3294,
    func_ov036_021b3368,        func_ov036_021b33e4,           func_ov036_021b3450, func_ov036_021b34e4,
    func_ov036_021b3568,        func_ov036_021b3590,           func_ov036_021b2e48, func_ov036_021b2cc4,
    func_ov036_021b2e78,        func_ov036_021b35b8,           func_ov036_021b365c, func_ov036_021b3628,
};

void RailDataHandle_Reset(RailDataHandle *handle) {
    MI_CpuFill8(handle, 0, sizeof(RailDataHandle));
    handle->camFuncCount = NELEMS(RAIL_CAMERA_CURVE_FUNC_SET);
    handle->railToWorldCount = NELEMS(RAIL_CONV_RAIL_TO_WORLD_FUNCS);
    handle->worldToRailCount = NELEMS(RAIL_CONV_WORLD_TO_RAIL_FUNCS);
    handle->camFuncs = RAIL_CAMERA_CURVE_FUNC_SET;
    handle->railToWorldFuncs = RAIL_CONV_RAIL_TO_WORLD_FUNCS;
    handle->worldToRailFuncs = RAIL_CONV_WORLD_TO_RAIL_FUNCS;
}
