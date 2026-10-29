// The rail actors' place on the rail: the rail position and rail unit every one of them holds, the collision of a
// step along the rail, and the move codes of the actors that wait and walk there. The name is descriptive. The size
// sort shows the file: its direction tables share a section laid out after fld_act_rail.c's 64-byte jump heights,
// and they come out in the ROM's order, the direction sets in DIR_LISTS's, without rail_data_handle.c's tables
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/directions.h"
#include "field/field_acmd.h"
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

// ActorRailWork's position.componentIsLine before the actor has saved a position on the rail
#define RAIL_POSITION_NONE 2

// A set of directions an actor chooses among, by the number the actor's data gives
typedef struct {
    u32 id;
    const s32 *list;
} DirListEntry;

// The id of the entry that ends DIR_LISTS
#define DIR_LIST_END 11

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

// ActorRailWalkWork's state: face the direction the actor is in, wait for it and a random time, walk a step in a
// random direction and wait for it
enum {
    RAIL_WALK_FACE,
    RAIL_WALK_WAIT_FACE,
    RAIL_WALK_WAIT,
    RAIL_WALK_STEP,
    RAIL_WALK_WAIT_STEP,
};

// ActorRailPatrolWork's state: face the actor's direction, or the opposite one once it has turned back, take a step
// and wait for it
enum {
    RAIL_PATROL_FACE,
    RAIL_PATROL_STEP,
    RAIL_PATROL_WAIT_STEP,
};

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

// The directions an actor chooses among, ended by DIR_NONE, in DIR_LISTS's order
static const s32 DIRS_ALL[5] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE };
static const s32 DIRS_UP_LEFT[3] = { DIR_UP, DIR_LEFT, DIR_NONE };
static const s32 DIRS_UP_RIGHT[3] = { DIR_UP, DIR_RIGHT, DIR_NONE };
static const s32 DIRS_DOWN_LEFT[3] = { DIR_DOWN, DIR_LEFT, DIR_NONE };
static const s32 DIRS_DOWN_RIGHT[3] = { DIR_DOWN, DIR_RIGHT, DIR_NONE };
static const s32 DIRS_UP_DOWN_LEFT[4] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_NONE };
static const s32 DIRS_UP_DOWN_RIGHT[4] = { DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_NONE };
static const s32 DIRS_UP_LEFT_RIGHT[4] = { DIR_UP, DIR_LEFT, DIR_RIGHT, DIR_NONE };
static const s32 DIRS_DOWN_LEFT_RIGHT[4] = { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE };
static const s32 DIRS_UP_DOWN[3] = { DIR_UP, DIR_DOWN, DIR_NONE };
static const s32 DIRS_LEFT_RIGHT[3] = { DIR_LEFT, DIR_RIGHT, DIR_NONE };

// The sets of directions, by the number in the actor's data, ended by DIR_LIST_END
static const DirListEntry DIR_LISTS[] = {
    { 0, DIRS_ALL },           { 1, DIRS_UP_LEFT },       { 2, DIRS_UP_RIGHT },
    { 3, DIRS_DOWN_LEFT },     { 4, DIRS_DOWN_RIGHT },    { 5, DIRS_UP_DOWN_LEFT },
    { 6, DIRS_UP_DOWN_RIGHT }, { 7, DIRS_UP_LEFT_RIGHT }, { 8, DIRS_DOWN_LEFT_RIGHT },
    { 9, DIRS_UP_DOWN },       { 10, DIRS_LEFT_RIGHT },   { DIR_LIST_END, NULL },
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

void RailActor_GetNowLocation(FieldActor *actor, RailPosition *position) {
    RailUnit_GetRailPos(RAIL_WORK(actor)->unit, position);
}

RailUnit *FldAct_GetRailUnit(FieldActor *actor) {
    return RAIL_WORK(actor)->unit;
}

void RailActor_UpdateRailWork(FieldActor *actor) {
    ActorRailWork *rail = RAIL_WORK(actor);

    RailUnit_UpdateMove(rail->unit);
    RailActor_SaveUnitPosition(rail, actor);
}

void RailActor_SetMoveRequest(FieldActor *actor, u16 dir, u32 speed) {
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

u32 RailActor_GetHitBitsDirNoAttr(FieldActor *actor, u16 dir) {
    return RailActor_GetHitBitsDir(actor, dir, NULL);
}

u32 RailActor_GetHitBitsDir(FieldActor *actor, u16 dir, u32 *tile) {
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
        moved = FieldRailSystem_TryStepLocation(railSystem, &from, RailActor_ConvertDirection(dir), &to);
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

    FieldRailSystem_GetLocationPos(railSystem, pos, &target);
    radius = FX_Div(FieldRailSystem_GetUnitScale(railSystem), 0x4000);
    while (NextActor(mmSys, &other, &index) == TRUE) {
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
            if (FieldRailSystem_IsPosNear(&target, &otherPos, radius)) {
                return TRUE;
            }
        }
        RailUnit_GetPreviousCalcPos(unit, &otherPos);
        if (FieldRailSystem_IsPosNear(&target, &otherPos, radius)) {
            return TRUE;
        }
    }
    return FALSE;
}

FieldActor *FindActorByRailPos(MMSys *mmSys, const RailPosition *pos, BOOL checkInit) {
    return FindActorByRailPosExcluding(mmSys, pos, checkInit, NULL);
}

FieldActor *FindActorByRailPosExcluding(MMSys *mmSys, const RailPosition *pos, BOOL checkInit, FieldActor *exclude) {
    u32 index = 0;
    FieldRailSystem *railSystem;
    FieldActor *other;
    VecFx32 target;
    VecFx32 otherPos;
    fx32 radius;

    railSystem = FieldNoGridMapper_GetRailSystem(FldActSys_GetNoGridMapper(mmSys));
    FieldRailSystem_GetLocationPos(railSystem, pos, &target);
    radius = FX_Div(FieldRailSystem_GetUnitScale(railSystem), 0x4000);
    while (NextActor(mmSys, &other, &index) == TRUE) {
        RailPosition initRail;
        RailPosition otherRail;

        if (IsActorFlagClear(other, 0x2000)) {
            continue;
        }
        if (exclude != NULL && exclude == other) {
            continue;
        }
        if (GetActorUID(other) == ACTOR_UID_PLAYER) {
            continue;
        }
        if (checkInit) {
            RailActor_GetNowLocation(other, &initRail);
            FieldRailSystem_GetLocationPos(railSystem, &initRail, &otherPos);
            if (FieldRailSystem_IsPosNear(&target, &otherPos, radius)) {
                return other;
            }
        }
        GetActorRailPos(other, &otherRail);
        FieldRailSystem_GetLocationPos(railSystem, &otherRail, &otherPos);
        if (FieldRailSystem_IsPosNear(&target, &otherPos, radius)) {
            return other;
        }
    }
    return NULL;
}

BOOL RailActor_GetFrontLocation(FieldActor *actor, RailPosition *position) {
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
        return FieldRailSystem_TryStepLocation(railSystem, position, ConvDirToRailDir(dir), position);
    }
    return RailUnit_PredictNextPos(rail->unit, ConvDirToRailDir(dir), position, NULL);
}

void RailActor_SetGridPosByDir(FieldActor *actor, u16 dir) {
    FieldRailSystem *railSystem =
        FieldNoGridMapper_GetRailSystem(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    RailPosition rail;
    VecFx32 pos;

    GetActorRailPos(actor, &rail);
    FieldRailSystem_StepLocation(railSystem, &rail, ConvDirToRailDir(dir), &pos);
    SetActorInitialGPosX(actor, GetGPosX(actor));
    SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
    SetActorInitialGPosZ(actor, GetGPosZ(actor));
    SetActorGPosX(actor, (pos.x >> 4) / 4096);
    SetActorGPosY(actor, (pos.y >> 4) / 4096);
    SetActorGPosZ(actor, (pos.z >> 4) / 4096);
}

void RailActor_GetDrawOffset(FieldActor *actor, VecFx16 *dest) {
    FldAct_GetRailUnit(actor);
    RailActor_GetDrawOffsetByDir(actor, (u16)(u32)GetActorFaceDir(actor), dest);
}

void RailActor_GetDrawOffsetByDir(FieldActor *actor, u32 dir, VecFx16 *dest) {
    RailUnit *unit = FldAct_GetRailUnit(actor);
    u16 angle;
    fx16 cos;
    MtxFx33 mtx;
    VecFx32 vec;

    RailUnit_GetStepVector(unit, dest);
    angle = DIR_ANGLES[TURN_DIRS[RailUnit_GetLineDirection(unit)][dir]];
    cos = FX_CosIdx(angle);
    dest->y = FX_MUL(cos, dest->y);
    VEC_Set(&vec, dest->x, dest->y, dest->z);
    MTX_RotY33_(&mtx, FX_SinIdx(angle), cos);
    MTX_MultVec33(&vec, &mtx, &vec);
    VEC_Normalize(&vec, &vec);
    VEC_Fx16Set(dest, vec.x, vec.y, vec.z);
}

void RailActor_StayInit(FieldActor *actor) {
    RailActor_InitWork(actor, 0xc);
}

void RailActor_StayMove(FieldActor *actor) {
    RailActor_SaveUnitPosition(RAIL_WORK(actor), actor);
}

void RailActor_StayDelete(FieldActor *actor) {
    RailActor_ReleaseUnit(RAIL_WORK(actor), actor);
}

void RailActor_Recover(FieldActor *actor) {
    RailActor_ReacquireUnit(RAIL_WORK(actor), actor);
}

void RailActor_WaitInitAll(FieldActor *actor) {
    RailActorWait_Init(actor, 0);
}

void RailActor_WaitInitUpLeft(FieldActor *actor) {
    RailActorWait_Init(actor, 1);
}

void RailActor_WaitInitUpRight(FieldActor *actor) {
    RailActorWait_Init(actor, 2);
}

void RailActor_WaitInitDownLeft(FieldActor *actor) {
    RailActorWait_Init(actor, 3);
}

void RailActor_WaitInitDownRight(FieldActor *actor) {
    RailActorWait_Init(actor, 4);
}

void RailActor_WaitInitUpDownLeft(FieldActor *actor) {
    RailActorWait_Init(actor, 5);
}

void RailActor_WaitInitUpDownRight(FieldActor *actor) {
    RailActorWait_Init(actor, 6);
}

void RailActor_WaitInitUpLeftRight(FieldActor *actor) {
    RailActorWait_Init(actor, 7);
}

void RailActor_WaitInitDownLeftRight(FieldActor *actor) {
    RailActorWait_Init(actor, 8);
}

void RailActor_WaitInitUpDown(FieldActor *actor) {
    RailActorWait_Init(actor, 9);
}

void RailActor_WaitInitLeftRight(FieldActor *actor) {
    RailActorWait_Init(actor, 10);
}

void RailActor_WaitMove(FieldActor *actor) {
    RailActorWait_Update(actor);
}

void RailActor_WaitDelete(FieldActor *actor) {
    RailActorWait_Free(actor);
}

void RailActor_WalkInitAll(FieldActor *actor) {
    RailActorWalk_Init(actor, 0);
}

void RailActor_WalkInitUpDown(FieldActor *actor) {
    RailActorWalk_Init(actor, 9);
}

void RailActor_WalkMove(FieldActor *actor) {
    RailActorWalk_Update(actor);
}

void RailActor_WalkDelete(FieldActor *actor) {
    RailActorWalk_Free(actor);
}

void RailActor_PatrolInit(FieldActor *actor) {
    RailActorPatrol_Init(actor);
}

void RailActor_PatrolMove(FieldActor *actor) {
    RailActorPatrol_Update(actor);
}

void RailActor_PatrolDelete(FieldActor *actor) {
    RailActorPatrol_Free(actor);
}

static u32 RailActor_ConvertDirection(u32 dir) {
    return ConvDirToRailDir(dir);
}

static u32 RailActor_GetSpeedIndex(s32 speed) {
    u32 result = RAIL_SPEED_2;

    switch (speed) {
    case 0x10:
        result = RAIL_SPEED_1;
        break;
    case 4:
        result = RAIL_SPEED_4;
        break;
    case 2:
        result = RAIL_SPEED_8;
        break;
    case 8:
        result = RAIL_SPEED_2;
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
    } while (id != DIR_LIST_END);
    return NULL;
}

static void RailActor_AcquireUnit(ActorRailWork *rail, FieldActor *actor) {
    rail->unit = FieldNoGridMapper_AcquireUnit(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    rail->position.componentIsLine = RAIL_POSITION_NONE;
}

static void RailActor_SaveUnitPosition(ActorRailWork *rail, FieldActor *actor) {
    RailPosition pos;

    RailUnit_GetRailPos(rail->unit, &pos);
    SetActorPositionRailCoordinates(rail, &pos);
}

static void RailActor_ReleaseUnit(ActorRailWork *rail, FieldActor *actor) {
    FieldNoGridMapper_ReleaseUnit(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)), rail->unit);
    ClearActorFlag(actor, ACTOR_FLAG_RAIL);
}

static void RailActor_ReacquireUnit(ActorRailWork *rail, FieldActor *actor) {
    rail->unit = FieldNoGridMapper_AcquireUnit(FldActSys_GetNoGridMapper(GetActorMModelSystem(actor)));
    RailActor_RestoreUnitPosition(rail, actor);
}

static void SetActorPositionRailCoordinates(ActorRailWork *rail, const RailPosition *pos) {
    rail->position = *pos;
}

static void RailActor_RestoreUnitPosition(ActorRailWork *rail, FieldActor *actor) {
    if (rail->position.componentIsLine != RAIL_POSITION_NONE) {
        VecFx32 pos;

        RailUnit_SetRailPos(rail->unit, &rail->position);
        RailUnit_GetCalcPos(rail->unit, &pos);
        SetActorWPosValue(actor, &pos);
    }
}

static void RailActorWait_Init(FieldActor *actor, u16 dirs) {
    ActorRailWaitWork *work = RailActor_InitWork(actor, sizeof(ActorRailWaitWork));

    work->wait = RailActor_ChooseListValue(WAIT_TIMES, -1);
    work->dirs = dirs;
    FldAct_SetDrawStatus(actor, DRAW_STATUS_STOP);
    FldAct_ClearMoving(actor);
}

static void RailActorWait_Update(FieldActor *actor) {
    ActorRailWaitWork *work = (ActorRailWaitWork *)GetNPCRailPosPtrAddr(actor);

    work->wait--;
    if (work->wait <= 0) {
        work->wait = RailActor_ChooseListValue(WAIT_TIMES, -1);
        CheckSetActorFaceDir(actor, RailActor_ChooseDirection(work->dirs, DIR_NONE));
    }
    RailActor_SaveUnitPosition(&work->rail, actor);
}

static void RailActorWait_Free(FieldActor *actor) {
    RailActor_ReleaseUnit(RAIL_WORK(actor), actor);
}

static void RailActorWalk_Init(FieldActor *actor, u32 dir) {
    ActorRailWalkWork *work = RailActor_InitWork(actor, sizeof(ActorRailWalkWork));
    RailPosition pos;

    GetActorRailPos(actor, &pos);
    work->state = RAIL_WALK_FACE;
    work->wait = 0;
    work->dir = dir;
    work->front = pos.posFront;
    work->side = pos.posSide;
    FldAct_SetDrawStatus(actor, DRAW_STATUS_STOP);
    FldAct_ClearMoving(actor);
}

static void RailActorWalk_Update(FieldActor *actor) {
    ActorRailWalkWork *work = (ActorRailWalkWork *)GetNPCRailPosPtrAddr(actor);

    switch (work->state) {
    case RAIL_WALK_FACE:
        FldAct_ClearMoving(actor);
        FldAct_ClearMoveEnd(actor);
        FldAct_SetLocalAcmd(actor, (u32)GetAcmdForDir((u32)GetActorFaceDir(actor), ACMD_FACE));
        work->state++;
        break;
    case RAIL_WALK_WAIT_FACE:
        if (!FieldAcmdTCB_Update(actor)) {
            break;
        }
        work->wait = RailActor_ChooseListValue(WAIT_TIMES, -1);
        work->state++;
    case RAIL_WALK_WAIT:
        work->wait--;
        if (work->wait != 0) {
            break;
        }
        work->state++;
    case RAIL_WALK_STEP: {
        u32 dir = RailActor_ChooseDirection(work->dir, DIR_NONE);
        RailPosition current;
        RailPosition next;

        ChangeActorDirection(actor, dir);
        if (RailActor_GetHitBitsDirNoAttr(actor, dir) != 0) {
            work->state = RAIL_WALK_FACE;
            break;
        }
        GetActorRailPos(actor, &current);
        GetActorRailPosForDir(actor, dir, &next);
        if (current.componentId != next.componentId) {
            work->state = RAIL_WALK_FACE;
            break;
        }
        if (RailActor_IsOutsideWalkArea(actor, work->front, work->side, &next)) {
            work->state = RAIL_WALK_FACE;
            break;
        }
        FldAct_SetLocalAcmd(actor, (u32)GetAcmdForDir(dir, ACMD_WALK_8F));
        FldAct_SetMoving(actor);
        work->state++;
    }
    case RAIL_WALK_WAIT_STEP:
        if (FieldAcmdTCB_Update(actor)) {
            FldAct_ClearMoving(actor);
            work->state = RAIL_WALK_FACE;
        }
        break;
    }
    RailActor_SaveUnitPosition(&work->rail, actor);
}

static void RailActorWalk_Free(FieldActor *actor) {
    RailActor_ReleaseUnit(RAIL_WORK(actor), actor);
}

static void RailActorPatrol_Init(FieldActor *actor) {
    ActorRailPatrolWork *work = RailActor_InitWork(actor, sizeof(ActorRailPatrolWork));
    RailPosition pos;

    GetActorRailPos(actor, &pos);
    work->state = RAIL_PATROL_FACE;
    work->reversed = 0;
    work->front = pos.posFront;
    work->side = pos.posSide;
    FldAct_SetDrawStatus(actor, DRAW_STATUS_STOP);
    FldAct_ClearMoving(actor);
}

static void RailActorPatrol_Update(FieldActor *actor) {
    ActorRailPatrolWork *work = (ActorRailPatrolWork *)GetNPCRailPosPtrAddr(actor);

    switch (work->state) {
    case RAIL_PATROL_FACE: {
        u32 dir = GetDefaultActorDir(actor);

        if (work->reversed == 1) {
            dir = GetInverseDirection((u16)dir);
        }
        ChangeActorDirection(actor, dir);
        work->state = RAIL_PATROL_STEP;
    }
    case RAIL_PATROL_STEP: {
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
        blocked = RailActor_GetHitBitsDirNoAttr(actor, dir);
        GetActorRailPosForDir(actor, dir, &next);
        outside = RailActor_IsOutsideWalkArea(actor, work->front, work->side, &next);
        if (current.componentId != next.componentId) {
            otherComponent = TRUE;
        }
        if ((blocked & 1) || outside || otherComponent) {
            work->reversed = 1;
            dir = GetInverseDirection((u16)dir);
            blocked = RailActor_GetHitBitsDirNoAttr(actor, dir);
            GetActorRailPosForDir(actor, dir, &next);
        }
        acmd = ACMD_WALK_8F;
        if (blocked) {
            acmd = ACMD_STAY_WALK_8F;
        }
        FldAct_SetLocalAcmd(actor, (u32)GetAcmdForDir(dir, (u16)acmd));
        FldAct_SetMoving(actor);
        work->state = RAIL_PATROL_WAIT_STEP;
        break;
    }
    case RAIL_PATROL_WAIT_STEP:
        if (FieldAcmdTCB_Update(actor) == TRUE) {
            FldAct_ClearMoving(actor);
            work->state = RAIL_PATROL_FACE;
        }
        break;
    }
    RailActor_SaveUnitPosition(&work->rail, actor);
}

static void RailActorPatrol_Free(FieldActor *actor) {
    RailActor_ReleaseUnit(RAIL_WORK(actor), actor);
}
