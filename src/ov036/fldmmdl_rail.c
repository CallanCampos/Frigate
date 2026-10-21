// The rail actors' place on the rail: the rail position and rail unit every one of them holds, the collision of a
// step along the rail, and the move codes of the actors that wait and walk there. The name is descriptive. The size
// sort shows the file: its direction tables share a section laid out after fld_act_rail.c's 64-byte jump heights,
// and they come out in the ROM's order, the direction sets in DIR_LISTS's, without rail_data_handle.c's tables
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
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

static u32 RailActor_CheckCollision(FieldActor *actor, const RailPosition *from, const RailPosition *to, u32 *tile);
static BOOL RailActor_CheckActorCollision(FieldActor *actor, const RailPosition *pos);
static u32 RailActor_ConvertDirection(u32 dir);
static u32 RailActor_GetSpeedIndex(s32 speed);
static void *RailActor_InitWork(FieldActor *actor, u32 size);
static BOOL RailActor_IsOutsideWalkArea(FieldActor *actor, s32 front, s32 side, const RailPosition *pos);
static s32 RailActor_GetListLength(const s32 *list, s32 value);
static s32 RailActor_ChooseListValue(const s32 *list, s32 value);
static s32 RailActor_ChooseDirection(u32 key, s32 value);
static const s32 *RailActor_GetDirectionList(u32 key);
static void RailActor_AcquireUnit(ActorRailWork *rail, FieldActor *actor);
static void RailActor_SaveUnitPosition(ActorRailWork *rail, FieldActor *actor);
static void RailActor_ReleaseUnit(ActorRailWork *rail, FieldActor *actor);
static void RailActor_ReacquireUnit(ActorRailWork *rail, FieldActor *actor);
static void SetActorPositionRailCoordinates(ActorRailWork *rail, const RailPosition *pos);
static void RailActor_RestoreUnitPosition(ActorRailWork *rail, FieldActor *actor);
static void RailActorWait_Init(FieldActor *actor, u16 dirs);
static void RailActorWait_Update(FieldActor *actor);
static void RailActorWait_Free(FieldActor *actor);
static void RailActorWalk_Init(FieldActor *actor, u32 dir);
static void RailActorWalk_Update(FieldActor *actor);
static void RailActorWalk_Free(FieldActor *actor);
static void RailActorPatrol_Init(FieldActor *actor);
static void RailActorPatrol_Update(FieldActor *actor);
static void RailActorPatrol_Free(FieldActor *actor);

// How long an actor waits, ended by -1
static const s32 WAIT_TIMES[5] = { 0x10, 0x20, 0x30, 0x40, -1 };

// The directions an actor chooses among, ended by 9, in DIR_LISTS's order
static const s32 DIRS_ALL[5] = { 0, 1, 2, 3, 9 };
static const s32 DIRS_UP_LEFT[3] = { 0, 2, 9 };
static const s32 DIRS_UP_RIGHT[3] = { 0, 3, 9 };
static const s32 DIRS_DOWN_LEFT[3] = { 1, 2, 9 };
static const s32 DIRS_DOWN_RIGHT[3] = { 1, 3, 9 };
static const s32 DIRS_UP_DOWN_LEFT[4] = { 0, 1, 2, 9 };
static const s32 DIRS_UP_DOWN_RIGHT[4] = { 0, 1, 3, 9 };
static const s32 DIRS_UP_LEFT_RIGHT[4] = { 0, 2, 3, 9 };
static const s32 DIRS_DOWN_LEFT_RIGHT[4] = { 1, 2, 3, 9 };
static const s32 DIRS_UP_DOWN[3] = { 0, 1, 9 };
static const s32 DIRS_LEFT_RIGHT[3] = { 2, 3, 9 };

// The sets of directions, by the number in the actor's data, ended by 11
static const DirListEntry DIR_LISTS[] = {
    { 0, DIRS_ALL },           { 1, DIRS_UP_LEFT },       { 2, DIRS_UP_RIGHT },
    { 3, DIRS_DOWN_LEFT },     { 4, DIRS_DOWN_RIGHT },    { 5, DIRS_UP_DOWN_LEFT },
    { 6, DIRS_UP_DOWN_RIGHT }, { 7, DIRS_UP_LEFT_RIGHT }, { 8, DIRS_DOWN_LEFT_RIGHT },
    { 9, DIRS_UP_DOWN },       { 10, DIRS_LEFT_RIGHT },   { 11, NULL },
};

// The direction an actor ends up facing in, by the side of the rail's turn and the direction it was in
static const u8 TURN_DIRS[5][4] = { { 0, 0, 0, 0 }, { 0, 1, 2, 3 }, { 2, 3, 1, 0 }, { 1, 0, 3, 2 }, { 3, 2, 0, 1 } };

// The angle of each direction
static const u16 DIR_ANGLES[4] = { 0, 0x8000, 0x4000, 0xc000 };

// Whether the actor lacks a flag, such as 0x2000, SetRailActorFlag's flag of the actors on the rails
static inline BOOL IsActorFlagClear(FieldActor *actor, u32 flag) {
    return !CheckActorFlag(actor, flag);
}

// The actor's work for its move code
#define RAIL_WORK(actor) ((ActorRailWork *)GetNPCRailPosPtrAddr(actor))

void SetActorInitedPositionRail(FieldActor *actor, const RailPosition *position) {
    ActorRailWork *rail = RAIL_WORK(actor);
    VecFx32 pos;

    RailUnit_SetRailPos(rail->unit, position);
    SetActorPositionRailCoordinates(rail, position);
    RailUnit_GetCalcPos(rail->unit, &pos);
    SetActorWPosValue(actor, &pos);
}

void GetActorRailPos(FieldActor *actor, RailPosition *position) {
    FieldNoGridMapper_GetRailSystem(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    ActorRailWork *rail = RAIL_WORK(actor);

    if (RailUnit_IsMovePending(rail->unit)) {
        RailUnit_PredictNextPos(rail->unit, RailUnit_GetMoveDirection(rail->unit), position, NULL);
    } else {
        RailUnit_GetRailPos(rail->unit, position);
    }
}

void func_ov036_02195604(FieldActor *actor, RailPosition *position) {
    RailUnit_GetRailPos(RAIL_WORK(actor)->unit, position);
}

RailUnit *FldAct_GetRailUnit(FieldActor *actor) {
    return RAIL_WORK(actor)->unit;
}

void func_ov036_02195624(FieldActor *actor) {
    ActorRailWork *rail = RAIL_WORK(actor);

    RailUnit_UpdateMove(rail->unit);
    RailActor_SaveUnitPosition(rail, actor);
}

void func_ov036_02195640(FieldActor *actor, u16 dir, u32 speed) {
    ActorRailWork *rail = RAIL_WORK(actor);
    u32 railDir = RailActor_ConvertDirection(dir);

    RailUnit_RequestMove(rail->unit, RailActor_GetSpeedIndex(speed), railDir);
}

static u32 RailActor_CheckCollision(FieldActor *actor, const RailPosition *from, const RailPosition *to, u32 *tile) {
    u32 flags = 0;
    ActorRailWork *rail = RAIL_WORK(actor);
    NoGridMapper *mapper = FldActSys_GetNoGridMapper(GetActorMModelSystem(actor));

    if (!RailUnit_IsRailPosInBounds(rail->unit, from)) {
        flags |= 1;
    }
    if (!RailUnit_IsRailPosInBounds(rail->unit, to)) {
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
    if (RailActor_CheckActorCollision(actor, to)) {
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

    RailUnit_GetRailPos(rail->unit, &from);
    moved = GetActorFaceDir(actor);
    if (moved == dir) {
        moved = RailUnit_PredictNextPos(rail->unit, RailActor_ConvertDirection(dir), &to, NULL);
    } else {
        moved = func_ov036_021b068c(railSystem, &from, RailActor_ConvertDirection(dir), &to);
    }
    if (moved) {
        return RailActor_CheckCollision(actor, &from, &to, tile);
    }
    return 1;
}

static BOOL RailActor_CheckActorCollision(FieldActor *actor, const RailPosition *pos) {
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
        if (RailUnit_IsMovePending(unit)) {
            RailUnit_PredictNextPos(unit, RailUnit_GetMoveDirection(unit), NULL, &otherPos);
            if (func_ov036_021b0d64(&target, &otherPos, radius)) {
                return TRUE;
            }
        }
        RailUnit_GetPreviousCalcPos(unit, &otherPos);
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

BOOL func_ov036_02195928(FieldActor *actor, RailPosition *position) {
    ActorRailWork *rail;

    FldActSys_GetNoGridMapper(GetActorMModelSystem(actor));
    rail = RAIL_WORK(actor);
    return RailUnit_PredictNextPos(rail->unit, ConvDirToRailDir(GetActorFaceDir(actor)), position, NULL);
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
    return RailUnit_PredictNextPos(rail->unit, ConvDirToRailDir(dir), position, NULL);
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
    angle = DIR_ANGLES[TURN_DIRS[RailUnit_GetLineDirection(unit)][dir]];
    cos = FX_CosIdx(angle);
    dest->y = FX_MUL(cos, dest->y);
    VEC_Set(&vec, dest->x, dest->y, dest->z);
    MTX_RotY33_(&mtx, FX_SinIdx(angle), cos);
    MTX_MultVec33(&vec, &mtx, &vec);
    VEC_Normalize(&vec, &vec);
    VEC_Fx16Set(dest, vec.x, vec.y, vec.z);
}

void func_ov036_02195b1c(FieldActor *actor) {
    RailActor_InitWork(actor, 0xc);
}

void func_ov036_02195b28(FieldActor *actor) {
    RailActor_SaveUnitPosition(RAIL_WORK(actor), actor);
}

void func_ov036_02195b38(FieldActor *actor) {
    RailActor_ReleaseUnit(RAIL_WORK(actor), actor);
}

void func_ov036_02195b48(FieldActor *actor) {
    RailActor_ReacquireUnit(RAIL_WORK(actor), actor);
}

void func_ov036_02195b58(FieldActor *actor) {
    RailActorWait_Init(actor, 0);
}

void func_ov036_02195b64(FieldActor *actor) {
    RailActorWait_Init(actor, 1);
}

void func_ov036_02195b70(FieldActor *actor) {
    RailActorWait_Init(actor, 2);
}

void func_ov036_02195b7c(FieldActor *actor) {
    RailActorWait_Init(actor, 3);
}

void func_ov036_02195b88(FieldActor *actor) {
    RailActorWait_Init(actor, 4);
}

void func_ov036_02195b94(FieldActor *actor) {
    RailActorWait_Init(actor, 5);
}

void func_ov036_02195ba0(FieldActor *actor) {
    RailActorWait_Init(actor, 6);
}

void func_ov036_02195bac(FieldActor *actor) {
    RailActorWait_Init(actor, 7);
}

void func_ov036_02195bb8(FieldActor *actor) {
    RailActorWait_Init(actor, 8);
}

void func_ov036_02195bc4(FieldActor *actor) {
    RailActorWait_Init(actor, 9);
}

void func_ov036_02195bd0(FieldActor *actor) {
    RailActorWait_Init(actor, 10);
}

void func_ov036_02195bdc(FieldActor *actor) {
    RailActorWait_Update(actor);
}

void func_ov036_02195be4(FieldActor *actor) {
    RailActorWait_Free(actor);
}

void func_ov036_02195bec(FieldActor *actor) {
    RailActorWalk_Init(actor, 0);
}

void func_ov036_02195bf8(FieldActor *actor) {
    RailActorWalk_Init(actor, 9);
}

void func_ov036_02195c04(FieldActor *actor) {
    RailActorWalk_Update(actor);
}

void func_ov036_02195c0c(FieldActor *actor) {
    RailActorWalk_Free(actor);
}

void func_ov036_02195c14(FieldActor *actor) {
    RailActorPatrol_Init(actor);
}

void func_ov036_02195c1c(FieldActor *actor) {
    RailActorPatrol_Update(actor);
}

void func_ov036_02195c24(FieldActor *actor) {
    RailActorPatrol_Free(actor);
}

static u32 RailActor_ConvertDirection(u32 dir) {
    return ConvDirToRailDir(dir);
}

static u32 RailActor_GetSpeedIndex(s32 speed) {
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

static void *RailActor_InitWork(FieldActor *actor, u32 size) {
    RailPosition pos;
    BOOL set = GetActorInitPositionRail(actor, &pos);
    ActorRailWork *rail = (ActorRailWork *)ClearActorPositionBlock(actor, size);

    RailActor_AcquireUnit(rail, actor);
    if (set) {
        SetActorInitedPositionRail(actor, &pos);
    } else {
        MI_CpuFill8(&pos, 0, sizeof(RailPosition));
        SetActorInitedPositionRail(actor, &pos);
    }
    return rail;
}

static BOOL RailActor_IsOutsideWalkArea(FieldActor *actor, s32 front, s32 side, const RailPosition *pos) {
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

static s32 RailActor_GetListLength(const s32 *list, s32 value) {
    s32 i = 0;

    while (list[i] != value) {
        i++;
    }
    return i;
}

static s32 RailActor_ChooseListValue(const s32 *list, s32 value) {
    return list[GFL_RandomLC(RailActor_GetListLength(list, value))];
}

static s32 RailActor_ChooseDirection(u32 key, s32 value) {
    const s32 *list = RailActor_GetDirectionList(key);

    return list[GFL_RandomLC(RailActor_GetListLength(list, value))];
}

static const s32 *RailActor_GetDirectionList(u32 key) {
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

static void RailActor_AcquireUnit(ActorRailWork *rail, FieldActor *actor) {
    rail->unit = FieldNoGridMapper_AcquireUnit(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    rail->position.componentIsLine = 2;
}

static void RailActor_SaveUnitPosition(ActorRailWork *rail, FieldActor *actor) {
    RailPosition pos;

    RailUnit_GetRailPos(rail->unit, &pos);
    SetActorPositionRailCoordinates(rail, &pos);
}

static void RailActor_ReleaseUnit(ActorRailWork *rail, FieldActor *actor) {
    FieldNoGridMapper_ReleaseUnit(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)), rail->unit);
    ClearActorFlag(actor, 0x2000);
}

static void RailActor_ReacquireUnit(ActorRailWork *rail, FieldActor *actor) {
    rail->unit = FieldNoGridMapper_AcquireUnit(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    RailActor_RestoreUnitPosition(rail, actor);
}

static void SetActorPositionRailCoordinates(ActorRailWork *rail, const RailPosition *pos) {
    rail->position = *pos;
}

static void RailActor_RestoreUnitPosition(ActorRailWork *rail, FieldActor *actor) {
    if (rail->position.componentIsLine != 2) {
        VecFx32 pos;

        RailUnit_SetRailPos(rail->unit, &rail->position);
        RailUnit_GetCalcPos(rail->unit, &pos);
        SetActorWPosValue(actor, &pos);
    }
}

static void RailActorWait_Init(FieldActor *actor, u16 dirs) {
    ActorRailWaitWork *work = RailActor_InitWork(actor, 0x10);

    work->wait = RailActor_ChooseListValue(WAIT_TIMES, -1);
    work->dirs = dirs;
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
}

static void RailActorWait_Update(FieldActor *actor) {
    ActorRailWaitWork *work = (ActorRailWaitWork *)GetNPCRailPosPtrAddr(actor);

    work->wait--;
    if (work->wait <= 0) {
        work->wait = RailActor_ChooseListValue(WAIT_TIMES, -1);
        CheckSetActorFaceDir(actor, RailActor_ChooseDirection(work->dirs, 9));
    }
    RailActor_SaveUnitPosition(&work->rail, actor);
}

static void RailActorWait_Free(FieldActor *actor) {
    RailActor_ReleaseUnit(RAIL_WORK(actor), actor);
}

static void RailActorWalk_Init(FieldActor *actor, u32 dir) {
    ActorRailWalkWork *work = RailActor_InitWork(actor, 0x10);
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

static void RailActorWalk_Update(FieldActor *actor) {
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
        work->wait = RailActor_ChooseListValue(WAIT_TIMES, -1);
        work->state++;
    case 2:
        work->wait--;
        if (work->wait != 0) {
            break;
        }
        work->state++;
    case 3: {
        u32 dir = RailActor_ChooseDirection(work->dir, 9);
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
        if (RailActor_IsOutsideWalkArea(actor, work->front, work->side, &next)) {
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
    RailActor_SaveUnitPosition(&work->rail, actor);
}

static void RailActorWalk_Free(FieldActor *actor) {
    RailActor_ReleaseUnit(RAIL_WORK(actor), actor);
}

static void RailActorPatrol_Init(FieldActor *actor) {
    ActorRailPatrolWork *work = RailActor_InitWork(actor, 0x10);
    RailPosition pos;

    GetActorRailPos(actor, &pos);
    work->state = 0;
    work->reversed = 0;
    work->front = pos.posFront;
    work->side = pos.posSide;
    FldAct_SetDrawStatus(actor, 0);
    FldAct_ClearMoving(actor);
}

static void RailActorPatrol_Update(FieldActor *actor) {
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
        outside = RailActor_IsOutsideWalkArea(actor, work->front, work->side, &next);
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
    RailActor_SaveUnitPosition(&work->rail, actor);
}

static void RailActorPatrol_Free(FieldActor *actor) {
    RailActor_ReleaseUnit(RAIL_WORK(actor), actor);
}
