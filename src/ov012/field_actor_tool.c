#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/field_effect.h"
#include "field/field_g3d_mapper.h"
#include "field/field_map.h"
#include "field/zone.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/math.h"

// The grid height of a height on the map
#define HEIGHT_TO_GRID(height) ((s16)(((height) >> 4) / FX32_ONE))

static BOOL (*const TILE_EXIT_BLOCK_CHECKS[4])(u16 tileClass) = {
    TileExitBlockCheck_Up,
    TileExitBlockCheck_Down,
    TileExitBlockCheck_Left,
    TileExitBlockCheck_Right,
};

static const u16 data_ov012_0216cd60[4] = { 0x8000, 0x0000, 0x4000, 0xc000 };

static BOOL (*const TILE_ENTER_BLOCK_CHECKS[4])(u16 tileClass) = {
    TileExitBlockCheck_Down,
    TileExitBlockCheck_Up,
    TileExitBlockCheck_Right,
    TileExitBlockCheck_Left,
};

static const s32 DIRECTION_VEC_X[4] = { 0, 0, -1, 1 };

static const s32 DIRECTION_VEC_Z[4] = { -1, 1, 0, 0 };

static const u32 INV_DIR_TABLE[4] = { DIR_DOWN, DIR_UP, DIR_RIGHT, DIR_LEFT };

static const u8 data_ov012_0216cd68[16] = { 0, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3, 0 };

static const u8 data_ov012_0216cdc8[4][4] = {
    { 0, 1, 2, 3 },
    { 3, 2, 0, 1 },
    { 1, 0, 3, 2 },
    { 2, 3, 1, 0 },
};

void func_ov012_0215dabc(FieldActor *actor) {
    func_ov012_02167174(actor);
    func_ov036_021925a4(actor);
    SetActorFlag(actor, 2);
}

void func_ov012_0215dad4(FieldActor *actor) {
    if (func_ov012_0216748c(actor, 2) == 0) {
        func_ov012_0215dba0(actor);
        func_ov012_0215dbb8(actor);
        func_ov012_0215dbdc(actor);
        if (CheckActorMovementFlag(actor, 0x200)) {
            func_ov036_0218eff4(actor);
        } else if (func_ov012_02167600(actor) == FALSE && func_ov012_0215db3c(actor) == TRUE &&
                   func_ov036_021925ac(actor) == FALSE) {
            func_ov012_02167188(actor);
        }
        func_ov012_0215dc00(actor);
        func_ov012_0215dc34(actor);
    }
}

BOOL func_ov012_0215db3c(FieldActor *actor) {
    u32 flags;

    if (IsActorFlag16(actor) == FALSE) {
        if (CheckActorMovementFlag(actor, 0x1800) == 0) {
            return TRUE;
        }
        if (GetActorMoveCode(actor) == 0x32) {
            return TRUE;
        }
        flags = func_ov012_02166fe4(actor);
        if ((flags & 0x1000) && func_ov012_02167658(actor) == FALSE) {
            return FALSE;
        }
        if ((flags & 0x800) && func_ov012_021677a4(actor) == FALSE) {
            return FALSE;
        }
    }
    return TRUE;
}

void func_ov012_0215dba0(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x1000)) {
        CheckRecalcActorY(actor);
    }
}

void func_ov012_0215dbb8(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x800) && FldAct_CacheTerrainInfo(actor) == TRUE) {
        func_ov012_021674dc(actor);
    }
}

void func_ov012_0215dbdc(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x20)) {
        func_ov012_0215ec28(actor);
        func_ov012_0215dcdc(actor);
    }
    ClearActorMovementFlag(actor, 0xa0);
}

void func_ov012_0215dc00(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x80)) {
        func_ov012_0215ddb4(actor);
    } else if (CheckActorMovementFlag(actor, 0x20)) {
        ActorTerrainEffect_ApplyAll(actor);
    }
    ClearActorMovementFlag(actor, 0xa0);
}

void func_ov012_0215dc34(FieldActor *actor) {
    if (CheckActorMovementFlag(actor, 0x100)) {
        func_ov012_0215de7c(actor);
    } else if (CheckActorMovementFlag(actor, 0x40)) {
        func_ov012_0215de0c(actor);
    }
    ClearActorMovementFlag(actor, 0x140);
}

void ActorTerrainEffect_InitParam(FieldActor *actor, ActorTerrainEffectParam *param) {
    sys_memset_fast(param, 0, sizeof(ActorTerrainEffectParam));
    param->tileTypeOrigY = GetCachedOrigYTileUnderActor(actor);
    param->tileType = GetCachedTileUnderActor(actor);
    if (param->tileType != -1) {
        param->tileClass = GetTileClass(param->tileType);
        param->tileFlags = GetTileFlags(param->tileType);
    }
    if (param->tileTypeOrigY != -1) {
        param->tileClassOrigY = GetTileClass(param->tileTypeOrigY);
        param->tileFlagsOrigY = GetTileFlags(param->tileTypeOrigY);
    }
    param->config = GetActorMdlInfo(actor);
    param->effects = Field_GetFieldEffects(GetMMSysField(GetActorMModelSystem(actor)));
    param->season = FieldEffects_GetSeason(param->effects);
}

void func_ov012_0215dcdc(FieldActor *actor) {
    ActorTerrainEffectParam param;

    FldAct_CacheTerrainInfo(actor);
    if (CheckActorMovementFlag(actor, 0x800) == 0 && func_ov012_02167614(actor) == TRUE) {
        ActorTerrainEffect_InitParam(actor, &param);
        func_ov012_0215dfb4(actor, &param);
        func_ov012_0215e110(actor, &param);
        func_ov012_0215e1ec(actor, &param);
        func_ov012_0215def4(actor, &param);
        func_ov012_0215e2b8(actor, &param);
        ActorTerrainEffect_Shadow(actor, &param);
    }
}

void ActorTerrainEffect_ApplyAll(FieldActor *actor) {
    ActorTerrainEffectParam param;

    FldAct_CacheTerrainInfo(actor);
    if (CheckActorMovementFlag(actor, 0x800) == 0 && func_ov012_02167614(actor) == TRUE) {
        ActorTerrainEffect_InitParam(actor, &param);
        ActorTerrainEffect_TallGrass(actor, &param);
        func_ov012_0215e05c(actor, &param);
        func_ov012_0215e0f4(actor, &param);
        func_ov012_0215e148(actor, &param);
        func_ov012_0215e1ec(actor, &param);
        func_ov012_0215e278(actor, &param);
        func_ov012_0215e2b8(actor, &param);
        func_ov012_0215e39c(actor, &param);
        ActorTerrainEffect_Shadow(actor, &param);
    }
}

void func_ov012_0215ddb4(FieldActor *actor) {
    ActorTerrainEffectParam param;

    FldAct_CacheTerrainInfo(actor);
    if (CheckActorMovementFlag(actor, 0x800) == 0 && func_ov012_02167614(actor) == TRUE) {
        ActorTerrainEffect_InitParam(actor, &param);
        func_ov012_0215e1ec(actor, &param);
        func_ov012_0215e2b8(actor, &param);
        func_ov012_0215e198(actor, &param);
        func_ov012_0215e39c(actor, &param);
        ActorTerrainEffect_Shadow(actor, &param);
    }
}

void func_ov012_0215de0c(FieldActor *actor) {
    ActorTerrainEffectParam param;

    FldAct_CacheTerrainInfo(actor);
    if (CheckActorMovementFlag(actor, 0x800) == 0 && func_ov012_02167614(actor) == TRUE) {
        ActorTerrainEffect_InitParam(actor, &param);
        func_ov012_0215def4(actor, &param);
        func_ov012_0215e008(actor, &param);
        func_ov012_0215e298(actor, &param);
        func_ov012_0215e148(actor, &param);
        func_ov012_0215e33c(actor, &param);
        func_ov012_0215e228(actor, &param);
        func_ov012_0215e380(actor, &param);
        ActorTerrainEffect_Shadow(actor, &param);
    }
}

void func_ov012_0215de7c(FieldActor *actor) {
    ActorTerrainEffectParam param;

    FldAct_CacheTerrainInfo(actor);
    if (CheckActorMovementFlag(actor, 0x800) == 0 && func_ov012_02167614(actor) == TRUE) {
        ActorTerrainEffect_InitParam(actor, &param);
        func_ov012_0215def4(actor, &param);
        func_ov012_0215e298(actor, &param);
        func_ov012_0215e148(actor, &param);
        func_ov012_0215e33c(actor, &param);
        func_ov012_0215e228(actor, &param);
        func_ov012_0215e038(actor, &param);
        func_ov012_0215e258(actor, &param);
        func_ov012_0215e380(actor, &param);
        ActorTerrainEffect_Shadow(actor, &param);
    }
}

void func_ov012_0215def4(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (func_ov012_021676f0(actor) == FALSE && MapTile_IsDeepSand(param->tileClass)) {
        // How far an actor sinks into deep sand
        VecFx32 offset = { 0, -FX32_CONST(7), 0 };
        func_ov012_0216736c(actor, &offset);
    } else {
        VecFx32 offset = { 0, 0, 0 };
        func_ov012_0216736c(actor, &offset);
    }
}

u32 func_ov012_0215df40(u32 tileClass, u8 season) {
    u32 kind = 0;

    if (func_ov036_021a2df4(tileClass)) {
        kind = 6;
    } else if (func_ov036_021a2e00(tileClass)) {
        kind = 7;
    } else if (func_ov036_021a2d40(tileClass)) {
        kind = 1;
        if (season == 3) {
            kind = 5;
        }
    } else if (func_ov036_021a2d34(tileClass)) {
        if (season == 3) {
            kind = 4;
        }
    } else if (MapTile_IsNormalTallGrassDoubleBtl(tileClass)) {
        kind = 1;
    } else if (MapTile_IsReallyTallGrassSingleBtl(tileClass)) {
        kind = 2;
    } else if (MapTile_IsReallyTallGrassDoubleBtl(tileClass)) {
        kind = 3;
    }
    return kind;
}

void func_ov012_0215dfb4(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (param->tileFlags & 0x20) {
        func_ov036_021a40ac(param->effects, actor, FALSE, func_ov012_0215df40(param->tileClass, param->season));
    }
}

void ActorTerrainEffect_TallGrass(FieldActor *actor, ActorTerrainEffectParam *param) {
    u16 dir;
    u32 kind;

    if (param->tileFlags & 0x20) {
        dir = GetActorMotionDir(actor);
        kind = func_ov012_0215df40(param->tileClass, param->season);
        if (dir != DIR_UP) {
            func_ov036_021a40ac(param->effects, actor, TRUE, kind);
        }
    }
}

void func_ov012_0215e008(FieldActor *actor, ActorTerrainEffectParam *param) {
    u16 dir;
    u32 kind;

    if (param->tileFlags & 0x20) {
        dir = GetActorMotionDir(actor);
        kind = func_ov012_0215df40(param->tileClass, param->season);
        if (dir == DIR_UP) {
            func_ov036_021a40ac(param->effects, actor, TRUE, kind);
        }
    }
}

void func_ov012_0215e038(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (param->tileFlags & 0x20) {
        func_ov036_021a40ac(param->effects, actor, TRUE, func_ov012_0215df40(param->tileClass, param->season));
    }
}

void func_ov012_0215e05c(FieldActor *actor, ActorTerrainEffectParam *param) {
    u32 kind;

    if (param->tileFlagsOrigY & 8) {
        kind = 5;
        if (func_ov036_021a2ce0(param->tileClassOrigY)) {
            if (param->season == 3) {
                kind = 2;
                if (param->config->footprintType == 2) {
                    kind = 3;
                }
            }
        } else if (func_ov036_021a2cec(param->tileClassOrigY)) {
            if (param->season == 0 || param->season == 3) {
                kind = 2;
                if (param->config->footprintType == 2) {
                    kind = 3;
                }
            }
        } else if (MapTile_IsSnow(param->tileClassOrigY) == TRUE ||
                   MapTile_IsSnowNoCycling(param->tileClassOrigY) == TRUE) {
            kind = 2;
            if (param->config->footprintType == 2) {
                kind = 3;
            }
        } else if (MapTile_IsDeepSand(param->tileClassOrigY) == TRUE) {
            kind = 4;
        } else {
            kind = 0;
            if (param->config->footprintType == 2) {
                kind = 1;
            }
        }
        if (kind != 5) {
            func_ov036_021b47c8(actor, param->effects, kind);
        }
    }
}

void func_ov012_0215e0f4(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (func_ov036_021a2cd4(param->tileClassOrigY) == TRUE) {
        func_ov036_021c94e0(param->effects, actor);
    }
}

void func_ov012_0215e110(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (func_ov036_021a2c04(param->tileClass) == TRUE) {
        if (func_ov012_021676d8(actor) == FALSE) {
            func_ov036_021bea3c(param->effects, actor);
            func_ov012_021676bc(actor, TRUE);
        }
    } else {
        func_ov012_021676bc(actor, FALSE);
    }
}

void func_ov012_0215e148(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (func_ov036_021a2c04(param->tileClass) == TRUE) {
        if (func_ov012_021676d8(actor) == FALSE) {
            func_ov036_021bea3c(param->effects, actor);
            func_ov012_021676bc(actor, TRUE);
        }
    } else if (MapTile_IsSwamp(param->tileClass) == TRUE) {
        func_ov036_021be828(param->effects, actor, 1, 0);
    } else {
        func_ov012_021676bc(actor, FALSE);
    }
}

void func_ov012_0215e198(FieldActor *actor, ActorTerrainEffectParam *param) {
    func_ov012_021676bc(actor, FALSE);
}

BOOL func_ov012_0215e1a4(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (func_ov012_021673f8(GetActorMModelSystem(actor)) == FALSE || param->config->enableShadow == FALSE) {
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov012_0215e1c4(ActorTerrainEffectParam *param) {
    if (!(param->tileFlags & 0x80)) {
        return FALSE;
    }
    if (param->season == 3 && func_ov036_021a2ce0(param->tileClass)) {
        return FALSE;
    }
    return TRUE;
}

void func_ov012_0215e1ec(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (func_ov012_0215e1a4(actor, param) == TRUE && func_ov012_0215e1c4(param) == TRUE &&
        CheckActorMovementFlag(actor, 0x4000) == 0) {
        func_ov036_021a3bf0(actor, param->effects);
        SetActorMovementFlag(actor, 0x4000);
    }
}

void func_ov012_0215e228(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (func_ov012_0215e1a4(actor, param) == TRUE) {
        if (func_ov012_0215e1c4(param) == FALSE) {
            SetActorMovementFlag(actor, 0x8000);
        } else {
            ClearActorMovementFlag(actor, 0x8000);
        }
    }
}

void func_ov012_0215e258(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (!(param->tileFlags & 2) && func_ov012_021677f4(actor) == FALSE) {
        func_ov036_021a3e74(actor, param->effects);
    }
}

void func_ov012_0215e278(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (func_ov036_021a2bf4(param->tileClassOrigY) == TRUE) {
        func_ov036_021be828(param->effects, actor, 0, 1);
    }
}

void func_ov012_0215e298(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (func_ov036_021a2bf4(param->tileClass) == TRUE) {
        func_ov036_021be828(param->effects, actor, 0, 0);
    }
}

void func_ov012_0215e2b8(FieldActor *actor, ActorTerrainEffectParam *param) {
    u32 tileType;
    u32 nextTileType;
    u32 tileClass;
    u32 kind;
    MMSys *system;

    if (param->config->enableReflections && FldAct_CheckReflectingFlag(actor) == FALSE) {
        tileType = -1;
        if (param->tileFlags & 0x40) {
            tileType = param->tileType;
        } else {
            nextTileType = func_ov012_0215e9b0(actor, DIR_DOWN);
            if (GetTileFlags(nextTileType) & 0x40) {
                tileType = nextTileType;
            }
        }
        if (tileType != -1) {
            kind = 1;
            tileClass = GetTileClass(tileType);
            system = GetActorMModelSystem(actor);
            if (func_ov036_021a2d4c(tileClass) == TRUE || func_ov036_021a2c58(tileClass) == TRUE) {
                kind = 2;
            }
            func_ov036_021b49ac(system, actor, param->effects, kind);
            FldAct_SetReflectingFlag(actor, TRUE);
        }
    }
}

void func_ov012_0215e33c(FieldActor *actor, ActorTerrainEffectParam *param) {
    u32 tileType;

    if (param->config->enableReflections && FldAct_CheckReflectingFlag(actor)) {
        tileType = func_ov012_0215e9b0(actor, DIR_DOWN);
        if (tileType == -1) {
            FldAct_SetReflectingFlag(actor, FALSE);
        } else if (!(GetTileFlags(tileType) & 0x40)) {
            FldAct_SetReflectingFlag(actor, FALSE);
        }
    }
}

void func_ov012_0215e380(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (MapTile_IsElectricField(param->tileClass) == TRUE) {
        func_ov036_021c289c(actor, param->effects);
    }
}

void func_ov012_0215e39c(FieldActor *actor, ActorTerrainEffectParam *param) {
}

void ActorTerrainEffect_Shadow(FieldActor *actor, ActorTerrainEffectParam *param) {
    if (param->tileFlags & 0x100) {
        FldAct_SetShadowGroup(actor, 1);
    } else {
        FldAct_SetShadowGroup(actor, 0);
    }
}

u32 ActorRouteCollCheckCore(FieldActor *actor, const VecFx32 *position, s16 x, s16 y, s16 z, u16 dir) {
    u32 result;
    fx32 height;
    fx32 diff;
    VecFx32 newPosition;

    result = 0;
    newPosition.x = (x << 16) + 0x8000;
    newPosition.y = position->y;
    newPosition.z = (z << 16) + 0x8000;
    if (IsGPosOutsideActorWalkArea(actor, x, z) == TRUE) {
        result |= 1;
    }
    if (CheckBlockedCollPathToPosition(actor, dir, newPosition) == TRUE) {
        result |= 2;
    }
    if (GetHeightFromMap(actor, &newPosition, &height) == TRUE) {
        diff = position->y - height;
        if (diff < 0) {
            diff = -diff;
        }
        if (diff >= FX32_CONST(20)) {
            result |= 8;
        }
        y = HEIGHT_TO_GRID(height);
    } else {
        result |= 8;
    }
    if (CheckActorNewPosOtherActorCollision(actor, x, y, z) == TRUE) {
        result |= 4;
    }
    if (FieldG3DMapper_IsPosOutOfBounds(GetMMSysG3DMapper(GetActorMModelSystem(actor)), &newPosition) == TRUE) {
        result |= 0x10;
    }
    return result;
}

u32 ActorRouteCollCheck(FieldActor *actor, s16 x, s16 y, s16 z, u16 dir) {
    VecFx32 position;

    CopyActorWPos(actor, &position);
    return ActorRouteCollCheckCore(actor, &position, x, y, z, dir);
}

u32 ActorRouteCollCheckOneTileInDir(FieldActor *actor, u16 dir) {
    s16 x = GetGPosX(actor) + GetDirectionVectorCompX(dir);
    s16 y = FldAct_GetGPosY(actor);
    s16 z = GetGPosZ(actor) + GetDirectionVectorCompZ(dir);

    return ActorRouteCollCheck(actor, x, y, z, dir);
}

// How far an actor's collision reaches past its position
static inline s16 GetCollExtent(u8 size) {
    return size < 2 ? 0 : size - 1;
}

// Whether an area of the grid overlaps an actor's collision at a position
static inline BOOL CheckAreaOverActor(FieldActor *actor, const GPosXYZ *pos, s16 minX, s16 maxX, s16 minZ, s16 maxZ) {
    s16 extent = GetCollExtent(actor->collisionWidth);

    if (minX > pos->x + extent || maxX < pos->x) {
        return FALSE;
    }
    extent = GetCollExtent(actor->collisionHeight);
    if (minZ > pos->z || maxZ < pos->z - extent) {
        return FALSE;
    }
    return TRUE;
}

BOOL CheckActorNewPosOtherActorCollision(const FieldActor *actor, s16 x, s16 y, s16 z) {
    const MMSys *system = actor->actorSystem;
    u32 ignoreFlags;
    s16 maxX;
    s16 minZ;
    s16 dy;
    u32 i;
    FieldActor *other;

    if (actor->uid == ACTOR_UID_PLAYER) {
        ignoreFlags = 0x8080;
    } else {
        ignoreFlags = 0x80;
    }
    maxX = x + GetCollExtent(actor->collisionWidth);
    minZ = z - GetCollExtent(actor->collisionHeight);
    for (i = 0; i < system->actorCapacity; i++) {
        other = &system->actorHeap[i];
        if (actor != other && (other->flags & 1) && !(other->flags & ignoreFlags)) {
            dy = MATH_ABS(y - other->gPos.y);
            if (dy < 1 && CheckAreaOverActor(other, &other->gPos, x, maxX, minZ, z)) {
                return TRUE;
            }
            dy = MATH_ABS(y - other->initGPos.y);
            if (dy < 1 && CheckAreaOverActor(other, &other->initGPos, x, maxX, minZ, z)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL CheckActorVolumeOverGPos(FieldActor *actor, s16 x, s16 z, BOOL checkInit) {
    if (CheckAreaOverActor(actor, &actor->gPos, x, x, z, z)) {
        return TRUE;
    }
    if (checkInit == TRUE && CheckAreaOverActor(actor, &actor->initGPos, x, x, z, z)) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsGPosOutsideActorWalkArea(FieldActor *actor, s16 x, s16 z) {
    s16 center;
    s16 range;
    s16 min;
    s16 max;

    center = GetActorDefaultGPosX(actor);
    range = GetActorWalkAreaW(actor);
    if (range != -1) {
        max = center + range;
        min = center - range;
        if (min > x || max < x) {
            return TRUE;
        }
    }
    center = GetActorDefaultGPosZ(actor);
    range = GetActorWalkAreaH(actor);
    if (range != -1) {
        max = center + range;
        min = center - range;
        if (min > z || max < z) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CheckBlockedCollPathToPosition(FieldActor *actor, u16 dir, VecFx32 position) {
    u8 width;
    u32 firstTileType;
    BOOL found;
    u8 j;
    u8 i;
    u32 tileType;
    u8 height;
    u16 tileClass;
    VecFx32 rowPosition;
    VecFx32 tilePosition;

    if (func_ov012_021677a4(actor) == FALSE) {
        found = FALSE;
        width = GetActorCollWidth(actor);
        height = GetActorCollHeight(actor);
        rowPosition = position;
        for (j = 0; j < height; j++, rowPosition.z -= FX32_CONST(16)) {
            for (i = 0, tilePosition = rowPosition; i < width; i++, tilePosition.x += FX32_CONST(16)) {
                if (GetTileTypeAtPosByActor(actor, &tilePosition, &tileType) == FALSE) {
                    return TRUE;
                }
                if (MapTile_BlocksCollision(tileType) == TRUE) {
                    return TRUE;
                }
                if (found == FALSE) {
                    found = TRUE;
                    firstTileType = tileType;
                }
            }
        }
        tileClass = GetTileClass(GetCachedTileUnderActor(actor));
        if (TILE_EXIT_BLOCK_CHECKS[dir](tileClass) == TRUE) {
            return TRUE;
        }
        tileClass = GetTileClass(firstTileType);
        if (TILE_ENTER_BLOCK_CHECKS[dir](tileClass) == TRUE) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL GetTerrainAtPosByActor(FieldActor *actor, const VecFx32 *position, MapTerrainBuf *terrain) {
    MMSys *system;
    FieldG3DMapper *mapper;

    system = GetActorMModelSystem(actor);
    mapper = GetMMSysG3DMapper(system);
    return FieldG3DMapper_GetTerrain(mapper, position, terrain);
}

BOOL GetTileTypeAtPosByActor(FieldActor *actor, const VecFx32 *position, u32 *tileType) {
    MapTerrainBuf terrain;

    *tileType = 0;
    if (GetTerrainAtPosByActor(actor, position, &terrain) == TRUE) {
        *tileType = terrain.tileType;
        return TRUE;
    }
    return FALSE;
}

BOOL GetHeightFromMap(FieldActor *actor, const VecFx32 *position, fx32 *height) {
    MapTerrainBuf terrain;

    *height = 0;
    if (GetTerrainAtPosByActor(actor, position, &terrain) == TRUE) {
        *height = terrain.height;
        return TRUE;
    }
    return FALSE;
}

void func_ov012_0215e8ec(FieldActor *actor, u16 dir) {
    fx32 height;
    VecFx32 position;

    SetActorInitialGPosX(actor, GetGPosX(actor));
    SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
    SetActorInitialGPosZ(actor, GetGPosZ(actor));
    adjustXPos(actor, GetDirectionVectorCompX(dir));
    adjustYPos(actor, GetDirectionVectorCompZ(dir));
    height = 0;
    ConvGXZToVector(GetGPosX(actor), GetGPosZ(actor), &position);
    position.y = GetActorPosY(actor);
    GetHeightFromMap(actor, &position, &height);
    SetActorGPosY(actor, HEIGHT_TO_GRID(height));
}

void SetActorInitialGPosToNowGPos(FieldActor *actor) {
    SetActorInitialGPosX(actor, GetGPosX(actor));
    SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
    SetActorInitialGPosZ(actor, GetGPosZ(actor));
}

u32 func_ov012_0215e9b0(FieldActor *actor, u16 dir) {
    u32 tileType;
    VecFx32 position;

    tileType = -1;
    ConvGXZToVector(GetGPosX(actor), GetGPosZ(actor), &position);
    position.y = GetActorPosY(actor);
    ExpandVecInGridDir(dir, &position, FX32_CONST(16));
    GetTileTypeAtPosByActor(actor, &position, &tileType);
    return tileType;
}

void func_ov012_0215e9fc(FieldActor *actor, const VecFx32 *offset) {
    VecFx32 position;

    CopyActorWPos(actor, &position);
    position.x += offset->x;
    position.y += offset->y;
    position.z += offset->z;
    SetActorWPosValue(actor, &position);
}

void func_ov012_0215ea30(FieldActor *actor, u16 dir, fx32 distance) {
    VecFx32 position;

    CopyActorWPos(actor, &position);
    switch (dir) {
    case DIR_UP:
        position.z -= distance;
        break;
    case DIR_DOWN:
        position.z += distance;
        break;
    case DIR_LEFT:
        position.x -= distance;
        break;
    case DIR_RIGHT:
        position.x += distance;
        break;
    }
    SetActorWPosValue(actor, &position);
}

BOOL CheckRecalcActorY(FieldActor *actor) {
    BOOL result;
    fx32 height;
    VecFx32 position;
    VecFx32 terrainPosition;

    CopyActorWPos(actor, &position);
    terrainPosition = position;
    if (CheckActorFlag(actor, 0x2000)) {
        return FALSE;
    }
    if (func_ov012_02167658(actor) == TRUE) {
        ClearActorMovementFlag(actor, 0x1000);
        return FALSE;
    }
    result = GetHeightFromMap(actor, &terrainPosition, &height);
    if (result == TRUE) {
        position.y = height;
        SetActorWPosValue(actor, &position);
        SetActorInitialGPosY(actor, FldAct_GetGPosY(actor));
        SetActorGPosY(actor, HEIGHT_TO_GRID(height));
        ClearActorMovementFlag(actor, 0x1000);
    } else {
        SetActorMovementFlag(actor, 0x1000);
    }
    return result;
}

BOOL FldAct_CacheTerrainInfo(FieldActor *actor) {
    s32 x;
    s32 y;
    s32 z;
    BOOL foundOrigY;
    BOOL found;
    u32 tileTypeOrigY;
    u32 tileType;
    VecFx32 position;

    foundOrigY = FALSE;
    found = FALSE;
    tileTypeOrigY = -1;
    tileType = -1;
    if (func_ov012_021677a4(actor) == FALSE) {
        x = FldAct_GetInitGPosX(actor);
        y = FldAct_GetInitGPosY(actor);
        z = FldAct_GetInitGPosZ(actor);
        position.x = x << 16;
        position.y = y << 16;
        position.z = z << 16;
        foundOrigY = GetTileTypeAtPosByActor(actor, &position, &tileTypeOrigY);
        x = GetGPosX(actor);
        z = GetGPosZ(actor);
        ConvGXZToVector((s16)x, (s16)z, &position);
        position.y = GetActorPosY(actor);
        found = GetTileTypeAtPosByActor(actor, &position, &tileType);
    }
    if (foundOrigY == TRUE) {
        SetCachedOrigYTileUnderActor(actor, tileTypeOrigY);
    }
    if (found == TRUE) {
        SetCachedTileUnderActor(actor, tileType);
    }
    if (found == TRUE) {
        ClearActorMovementFlag(actor, 0x800);
        return TRUE;
    }
    SetActorMovementFlag(actor, 0x800);
    return FALSE;
}

BOOL func_ov012_0215ebd8(FieldActor *actor) {
    FieldActor *player;
    s16 playerX;
    s16 playerZ;
    s16 minX;
    s16 maxX;
    s16 minZ;
    s16 maxZ;

    player = FindPlayerFieldActor(GetActorMModelSystem(actor));
    if (player != NULL) {
        playerX = player->gPos.x;
        playerZ = player->gPos.z;
        minX = actor->gPos.x - 18;
        maxX = actor->gPos.x + 17;
        minZ = actor->gPos.z - 18;
        maxZ = actor->gPos.z + 17;
        if (minZ <= playerZ && maxZ >= playerZ && minX <= playerX && maxX >= playerX) {
            return TRUE;
        }
    }
    return FALSE;
}

void func_ov012_0215ec28(FieldActor *actor) {
    u16 moveCode = GetActorMoveCode(actor);

    if (moveCode == 0x33 || moveCode == 0x34 || moveCode == 0x35 || moveCode == 0x36 || moveCode == 0x55) {
        func_ov012_02167188(actor);
    }
}

s16 GetDirectionVectorCompX(u32 direction) {
    return DIRECTION_VEC_X[direction];
}

s16 GetDirectionVectorCompZ(u32 direction) {
    return DIRECTION_VEC_Z[direction];
}

void ExpandVecInGridDir(u16 direction, VecFx32 *position, fx32 amount) {
    switch (direction) {
    case 0:
        position->z -= amount;
        break;
    case 1:
        position->z += amount;
        break;
    case 2:
        position->x -= amount;
        break;
    case 3:
        position->x += amount;
        break;
    }
}

void AdjusGridXZByDir(u32 direction, s16 *x, s16 *z, s16 amount) {
    switch (direction) {
    case 0:
        *z = (s16)(*z - amount);
        break;
    case 1:
        *z = (s16)(*z + amount);
        break;
    case 2:
        *x = (s16)(*x - amount);
        break;
    case 3:
        *x = (s16)(*x + amount);
        break;
    }
}

void ConvGXZToVector(s16 x, s16 z, VecFx32 *position) {
    position->x = (x << 16) + 0x8000;
    position->z = (z << 16) + 0x8000;
}

void VecGPosToWPos(s32 x, s32 y, s32 z, VecFx32 *position) {
    position->x = x << 16;
    position->y = y << 16;
    position->z = z << 16;
}

u16 GetInverseDirection(u32 direction) {
    return INV_DIR_TABLE[direction];
}

u16 GetDirFromPosToPos(s32 x1, s32 z1, s32 x2, s32 z2) {
    s32 direction;

    if (x1 > x2) {
        return 2;
    }
    if (x1 < x2) {
        return 3;
    }
    direction = 1;
    if (z1 > z2) {
        direction = 0;
    }
    return direction;
}

u32 func_ov012_0215ed38(u32 direction, u16 angle) {
    u16 rotated = data_ov012_0216cd60[direction] + angle;

    return data_ov012_0216cdc8[data_ov012_0216cd68[rotated / 0x1000]][DIR_DOWN];
}


void SetRailActorFlag(FieldActor *actor) {
    SetActorFlag(actor, 0x2000);
}

void InitRailActor(FieldActor *actor, const ZoneNPC *npc) {
    RailPosition position;
    const ZoneNPCRailPosition *rail;

    position.componentId = npc->pos.rail.railIndex;
    position.componentIsLine = TRUE;
    rail = &npc->pos.rail;
    position.railDirection = ConvDirToRailDir((s16)npc->direction);
    position.posFront = rail->frontPos;
    position.posSide = rail->sidePos;
    if (CheckMMSysFlag(GetActorMModelSystem(actor), 0x10)) {
        SetActorInitedPositionRail(actor, &position);
        return;
    }
    ClearActorPositionBlock(actor, sizeof(ActorPositionRail));
    SetActorInitPositionRail(actor, &position);
}

void SetActorInitPositionRail(FieldActor *actor, const RailPosition *position) {
    ActorPositionRail *rail = GetNPCRailPosPtrAddr(actor);

    rail->position = *position;
    rail->unk0 = ACTOR_RAIL_POS_SET;
}

BOOL func_ov012_0215ede4(FieldActor *actor, RailPosition *position) {
    ActorPositionRail *rail = GetNPCRailPosPtrAddr(actor);

    if (rail->unk0 == ACTOR_RAIL_POS_SET) {
        *position = rail->position;
        return TRUE;
    }
    return FALSE;
}
