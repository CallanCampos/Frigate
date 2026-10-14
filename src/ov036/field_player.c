#include "types.h"
#include "constants/flags.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_g3d_mapper.h"
#include "field/field_map.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_player.h"
#include "field/field_player_core.h"
#include "field/field_player_grid.h"
#include "field/field_player_nogrid.h"
#include "field/field_rail.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The field's player: the core, and the grid and rail movement that the map's controller sets up

struct FieldPlayer {
    Field *field;
    FieldPlayerCore *core;
    FieldPlayerGrid *grid;
    FieldPlayerNoGrid *nogrid;
};

// The controller types that Field_GetResolvedControllerTypeID returns
#define CONTROLLER_GRID 0

#define PLAYER_FORM_COUNT 10

#define GRID_TO_FX32(n) ((n) * FX32_CONST(16))

// How far below the player the water may be for them to start surfing
#define SURF_HEIGHT_MAX FX32_CONST(20)

typedef struct {
    u16 objCode;
    u16 exState;
} PlayerSprite;

// The object code and extra state of each form of the player, by sex
static const PlayerSprite PLAYER_SPRITE_MAP[2][PLAYER_FORM_COUNT] = {
    {
        {0xe7, FLD_PLAYER_EXSTATE_NONE},
        {0xe8, FLD_PLAYER_EXSTATE_CYCLING},
        {0xe9, FLD_PLAYER_EXSTATE_SURF},
        {0xe9, FLD_PLAYER_EXSTATE_DIVE},
        {0xea, 4},
        {0xeb, 4},
        {0xec, 4},
        {0xee, 4},
        {0xef, 4},
        {0xed, 4},
    },
    {
        {0xf0, FLD_PLAYER_EXSTATE_NONE},
        {0xf1, FLD_PLAYER_EXSTATE_CYCLING},
        {0xf2, FLD_PLAYER_EXSTATE_SURF},
        {0xf2, FLD_PLAYER_EXSTATE_DIVE},
        {0xf3, 4},
        {0xf4, 4},
        {0xf5, 4},
        {0xf7, 4},
        {0xf8, 4},
        {0xf6, 4},
    },
};

FieldPlayer *FieldPlayer_Create(PlayerState *state, Field *field, const VecFx32 *pos, u32 sex, HeapID heapId) {
    FieldPlayer *player = GFL_HeapAllocate(heapId, sizeof(FieldPlayer), TRUE, "field_player.c", 76);

    player->field = field;
    player->core = FieldPlayerCore_Create(state, field, pos, sex, heapId);
    return player;
}

void FieldPlayer_Free(FieldPlayer *player) {
    if (player->grid != NULL) {
        FieldPlayerGrid_Free(player->grid);
    }
    if (player->nogrid != NULL) {
        func_ov036_0219cf40(player->nogrid);
    }
    FieldPlayerCore_Free(player->core);
    GFL_HeapFree(player);
}

void FieldPlayer_SyncState(FieldPlayer *player) {
    VecFx32 pos;
    RailPosition railPos;
    PlayerState *state = FieldPlayerCore_GetPlayerState(player->core);
    FieldActor *actor = FieldPlayerCore_GetActor(player->core);

    CopyActorWPos(actor, &pos);
    PlayerState_SetWPos(state, &pos);
    PlayerState_SetRotationByDir(state, GetActorFaceDir(actor));
    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        PlayerState_SetIsRail(state, FALSE);
    } else {
        GetActorRailPos(actor, &railPos);
        PlayerState_SetRailPos(state, &railPos);
        PlayerState_SetIsRail(state, TRUE);
    }
}

GameEvent *FieldPlayer_CheckObjContactEvent(FieldPlayer *player, u32 pressedKeys, u32 heldKeys, u32 flags) {
    GameEvent *event = NULL;

    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        event = FieldPlayerGrid_CheckObjContactEvent(player->grid, pressedKeys, heldKeys, flags);
    }
    return event;
}

void FieldPlayer_SetSpecialSeq(FieldPlayer *player, u32 seq) {
    FieldPlayerCore_SetSpecialSeq(player->core, seq);
}

BOOL func_ov036_0219a580(FieldPlayer *player) {
    return func_ov036_0219b328(player->core);
}

void func_ov036_0219a58c(FieldPlayer *player) {
    func_ov036_0219b350(player->core);
}

void FieldPlayer_ForceBrake(FieldPlayer *player) {
    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        FieldPlayerGrid_ForceBrake(player->grid);
    } else {
        func_ov036_0219d060(player->nogrid);
    }
}

void func_ov036_0219a5b8(FieldPlayer *player, TCB *tcb) {
    func_ov036_0219b138(player->core, tcb);
}

TCB *func_ov036_0219a5c4(FieldPlayer *player) {
    return FieldPlayerCore_GetTerrainEffectTCB(player->core);
}

u16 FieldPlayer_GetMoveDirByKey(FieldPlayer *player, u32 heldKeys) {
    return FieldPlayerCore_GetMoveDirByKey(player->core, heldKeys);
}

BOOL func_ov036_0219a5dc(FieldPlayer *player) {
    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        return FieldPlayerGrid_IsBraking(player->grid);
    }
    return func_ov036_0219d008(player->nogrid);
}

void func_ov036_0219a5fc(FieldPlayer *player) {
    FieldPlayerCore_SetSpecialSeq(player->core, 1);
    func_ov036_0219b350(player->core);
}

void FieldPlayer_UpdateActionStatusObserver(FieldPlayer *player) {
    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        if (FieldPlayerGrid_IsConstMoving(player->grid) == TRUE) {
            FieldPlayerCore_ForceMoveObserveStatus(player->core, 2);
            return;
        }
        if (FieldPlayerGrid_IsCatwalkExiting(player->grid) == TRUE) {
            FieldPlayerCore_ForceMoveObserveStatus(player->core, 2);
            return;
        }
    } else if (func_ov036_0219d018(player->nogrid) == TRUE) {
        FieldPlayerCore_ForceMoveObserveStatus(player->core, 2);
        return;
    }
    FieldPlayerCore_UpdateActionStatusObserver(player->core);
}

void FieldPlayer_GetWPos(FieldPlayer *player, VecFx32 *pos) {
    FieldPlayerCore_GetWPos(player->core, pos);
}

void FieldPlayer_SetWPos(FieldPlayer *player, const VecFx32 *pos) {
    FieldPlayerCore_SetWPos(player->core, pos);
}

u32 FieldPlayer_GetFaceDir(FieldPlayer *player) {
    return FieldPlayerCore_GetFaceDir(player->core);
}

void FieldPlayer_SetDirection(FieldPlayer *player, u32 dir) {
    FieldPlayerCore_SetDirection(player->core, dir);
}

Field *FieldPlayer_GetField(FieldPlayer *player) {
    return FieldPlayerCore_GetField(player->core);
}

FieldActor *FieldPlayer_GetActor(FieldPlayer *player) {
    return FieldPlayerCore_GetActor(player->core);
}

u32 FieldPlayer_GetMoveStatus(FieldPlayer *player) {
    return FieldPlayerCore_GetMoveStatus(player->core);
}

u32 FieldPlayer_GetMoveObserveStatus(FieldPlayer *player) {
    return func_ov036_0219afd8(player->core);
}

u32 FieldPlayer_GetExState(FieldPlayer *player) {
    return FieldPlayerCore_GetExState(player->core);
}

void FieldPlayer_SetSpecialState(FieldPlayer *player, u32 state) {
    FieldPlayerCore_SetExState(player->core, state);
}

u32 FieldPlayer_GetSex(FieldPlayer *player) {
    return FieldPlayerCore_GetSex(player->core);
}

GameSystem *FieldPlayer_GetGameSystem(FieldPlayer *player) {
    return FieldPlayerCore_GetGameSystem(player->core);
}

u32 FieldPlayer_GetTileTypeUnder(FieldPlayer *player) {
    fx32 height;

    return FieldPlayer_GetTileTypeInDirEx(player, PLAYER_DIR_NONE, &height);
}

u32 FieldPlayer_GetTileTypeInDir(FieldPlayer *player, u16 direction) {
    fx32 height;

    return FieldPlayer_GetTileTypeInDirEx(player, direction, &height);
}

u32 FieldPlayer_GetTileTypeInDirEx(FieldPlayer *player, u16 direction, fx32 *height) {
    VecFx32 pos;
    MapTerrainBuf terrain;
    RailPosition railPos;

    *height = 0;
    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        FieldG3DMapper *mapper = Field_GetG3DMapper(player->field);

        if (direction == PLAYER_DIR_NONE) {
            FieldPlayer_GetWPos(player, &pos);
        } else {
            FieldPlayer_GetWPosInDir(player, direction, &pos);
        }
        if (FieldG3DMapper_GetTerrain(mapper, &pos, &terrain) == TRUE) {
            *height = terrain.height;
            return terrain.tileType;
        }
        return 0xffffffff;
    } else {
        FieldActor *actor = FieldPlayer_GetActor(player);
        NoGridMapper *mapper = Field_GetNoGridMapper(player->field);
        FieldRailSystem *rail = FieldNoGridMapper_GetRailSystem(mapper);

        if (direction == PLAYER_DIR_NONE) {
            GetActorRailPos(actor, &railPos);
        } else if (!GetActorRailPosForDir(actor, direction, &railPos)) {
            return -1;
        }
        func_ov036_021b06ec(rail, &railPos, &pos);
        *height = pos.y;
        return FieldNoGridMapper_GetTileAtPos(mapper, &railPos);
    }
}

u16 FieldPlayer_GetObjCodeByForme(u32 sex, u32 forme) {
    return PLAYER_SPRITE_MAP[sex][forme].objCode;
}

u16 FieldPlayer_GetObjCodeByExState(u32 sex, u32 exState) {
    const PlayerSprite *sprite;
    int i;

    for (i = 0, sprite = PLAYER_SPRITE_MAP[sex]; i < PLAYER_FORM_COUNT; i++, sprite++) {
        if (sprite->exState == exState) {
            return sprite->objCode;
        }
    }
    return 0xe7;
}

// The form with the object code, PLAYER_FORM_COUNT if it isn't one of the player's
u32 FieldPlayer_GetExStateByObjCode(u32 objCode) {
    int sex, i;

    for (sex = 0; sex < 2; sex++) {
        const PlayerSprite *sprite = PLAYER_SPRITE_MAP[sex];

        for (i = 0; i < PLAYER_FORM_COUNT; i++, sprite++) {
            if (sprite->objCode == objCode) {
                return i;
            }
        }
    }
    return PLAYER_FORM_COUNT;
}

BOOL func_ov036_0219a834(FieldPlayer *player) {
    switch (FieldPlayer_GetExState(player)) {
    case FLD_PLAYER_EXSTATE_SURF:
    case FLD_PLAYER_EXSTATE_DIVE:
        return FALSE;
    }
    return TRUE;
}

void FieldPlayer_ForceObjCode(FieldPlayer *player, u32 objCode) {
    FieldPlayerCore_ForceObjCode(player->core, objCode);
}

void FieldPlayer_ClearObjCode(FieldPlayer *player) {
    FieldPlayerCore_ClearObjCode(player->core);
}

u32 func_ov036_0219a864(FieldPlayer *player) {
    return func_ov036_0219b088(player->core);
}

BOOL func_ov036_0219a870(FieldPlayer *player) {
    return func_ov036_0219b0a4(player->core);
}

FieldActor *FieldPlayer_GetActorInFront(FieldPlayer *player) {
    s16 x, y, z;
    RailPosition railPos;
    MMSys *system = Field_GetActorSystem(player->field);

    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        FieldPlayer_GetGPosInFront(player, &x, &y, &z);
        return GetFirstActorOnGPos(system, x, z, FALSE);
    }
    func_ov036_02195928(FieldPlayerCore_GetActor(player->core), &railPos);
    return FindActorByRailPos(system, &railPos, FALSE);
}

FieldActor *FieldPlayer_GetActorInFrontEx(FieldPlayer *player, fx32 maxHeightDiff) {
    s16 x, y, z;
    RailPosition railPos;
    MMSys *system = Field_GetActorSystem(player->field);

    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        FieldPlayer_GetGPosInFront(player, &x, &y, &z);
        return FindActorByGPos(system, x, z, GRID_TO_FX32(y), maxHeightDiff, FALSE);
    }
    func_ov036_02195928(FieldPlayerCore_GetActor(player->core), &railPos);
    return FindActorByRailPos(system, &railPos, FALSE);
}

void GetPlayerGPosPlusDir(FieldPlayer *player, u16 dir, s16 *x, s16 *y, s16 *z) {
    FieldActor *actor = FieldPlayer_GetActor(player);

    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        *x = GetGPosX(actor);
        *y = FldAct_GetGPosY(actor);
        *z = GetGPosZ(actor);
        *x += GetDirectionVectorCompX(dir);
        *z += GetDirectionVectorCompZ(dir);
    }
}

void FieldPlayer_GetWPosInDir(FieldPlayer *player, u16 direction, VecFx32 *position) {
    VecFx32 pos;
    VecFx16 dirVec;
    RailPosition railPos;
    s16 x, y, z;
    FieldActor *actor = FieldPlayerCore_GetActor(player->core);

    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        GetPlayerGPosPlusDir(player, direction, &x, &y, &z);
        ConvGXZToVector(x, z, position);
        position->y = GRID_TO_FX32(y);
    } else {
        FieldRailSystem *rail = FieldNoGridMapper_GetRailSystem(Field_GetNoGridMapper(player->field));
        fx32 step;

        GetActorRailPos(actor, &railPos);
        func_ov036_021b06ec(rail, &railPos, &pos);
        func_ov036_02195a78(actor, direction, &dirVec);
        step = func_ov036_021b05ec(rail);
        position->x = pos.x + FX_Mul(dirVec.x, step);
        position->y = pos.y + FX_Mul(dirVec.y, step);
        position->z = pos.z + FX_Mul(dirVec.z, step);
    }
}

void FieldPlayer_GetGPosInFront(FieldPlayer *player, s16 *x, s16 *y, s16 *z) {
    GetPlayerGPosPlusDir(player, GetActorFaceDir(FieldPlayer_GetActor(player)), x, y, z);
}

void func_ov036_0219aab0(FieldPlayer *player, u32 dir, VecFx32 *vec) {
    VecFx16 dirVec;
    FieldActor *actor = FieldPlayerCore_GetActor(player->core);

    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        vec->y = 0;
        vec->x = GetDirectionVectorCompX(dir);
        vec->z = GetDirectionVectorCompZ(dir);
        vec->x = GRID_TO_FX32(vec->x);
        vec->z = GRID_TO_FX32(vec->z);
        VEC_Normalize(vec, vec);
    } else {
        func_ov036_02195a78(actor, dir, &dirVec);
        VEC_Set(vec, dirVec.x, dirVec.y, dirVec.z);
    }
}

u32 FieldPlayer_DeriveExState(FieldPlayer *player) {
    return FieldPlayerCore_DeriveExState(player->core);
}

BOOL func_ov036_0219ab24(FieldPlayer *player) {
    return func_ov036_0219b04c(player->core);
}

// Whether the player can start surfing onto the tile in the direction: they aren't surfing or diving, the tiles allow
// it, and the water is level with them or less than SURF_HEIGHT_MAX below
BOOL CheckSurfHeightAllow(FieldPlayer *player, u32 direction) {
    fx32 height;
    fx32 frontHeight;
    u32 tileUnder;
    u32 exState = FieldPlayer_GetExState(player);

    if (exState == FLD_PLAYER_EXSTATE_SURF || exState == FLD_PLAYER_EXSTATE_DIVE) {
        return FALSE;
    }
    tileUnder = FieldPlayer_GetTileTypeInDirEx(player, PLAYER_DIR_NONE, &height);
    if (!CheckSurfBeginTiles(tileUnder, FieldPlayer_GetTileTypeInDirEx(player, direction, &frontHeight))) {
        return FALSE;
    }
    height -= frontHeight;
    if (height < 0 || height >= SURF_HEIGHT_MAX) {
        return FALSE;
    }
    return TRUE;
}

BOOL CheckCanInteractWaterfall(FieldPlayer *player, u32 tileUnder, u32 tileInFront) {
    u32 exState = FieldPlayer_GetExState(player);

    if (MapTile_BlocksCollision(tileUnder) || exState != FLD_PLAYER_EXSTATE_SURF) {
        return FALSE;
    }
    return IsTileWaterfall(GetTileClass(tileInFront));
}

void FieldPlayer_InterruptCatwalkBalance(FieldPlayer *player, BOOL keep) {
    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        FieldPlayerGrid_InterruptCatwalkBalance(player->grid, keep);
    }
}

void func_ov036_0219abd8(FieldPlayer *player) {
    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        func_ov036_0219cd84(player->grid);
    }
}

BOOL FieldPlayer_GetVerticalMoveOnlyFlag(FieldPlayer *player) {
    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        return FieldPlayerGrid_GetVerticalMoveOnlyFlag(player->grid);
    }
    return FALSE;
}

void func_ov036_0219ac0c(FieldPlayer *player) {
    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID) {
        FieldPlayerGrid_SetVerticalMoveOnlyFlag(player->grid);
    }
}

BOOL FieldPlayer_CheckRunningShoesFlag(FieldPlayer *player) {
    EventWork *eventWork = GameData_GetEventWork(GSYS_GetGameData(FieldPlayer_GetGameSystem(player)));

    if (EventWork_FlagGet(eventWork, EVENT_FLAG_RUNNING_SHOES)) {
        return TRUE;
    }
    return FALSE;
}

void FieldPlayer_InitGrid(FieldPlayer *player, HeapID heapId) {
    player->grid = FieldPlayerGrid_Create(player->core, heapId);
}

void FieldPlayer_UpdateGridMovement(FieldPlayer *player, u32 pressedKeys, u32 heldKeys, u32 flags) {
    FieldPlayerGrid_UpdateMovement(player->grid, pressedKeys, heldKeys, flags);
}

void FieldPlayer_GetGPos(FieldPlayer *player, s16 *x, s16 *y, s16 *z) {
    FieldActor *actor = FieldPlayerCore_GetActor(player->core);

    *x = GetGPosX(actor);
    *y = FldAct_GetGPosY(actor);
    *z = GetGPosZ(actor);
}

BOOL func_ov036_0219ac8c(FieldPlayer *player) {
    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID
        && FieldPlayerGrid_IsCatwalkExiting(player->grid) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

void func_ov036_0219acac(FieldPlayer *player) {
    if (Field_GetResolvedControllerTypeID(player->field) == CONTROLLER_GRID
        && FieldPlayerGrid_IsCatwalkExiting(player->grid) == TRUE) {
        FieldPlayerGrid_SanitizeHeightMismatch(player->grid);
    }
}

void func_ov036_0219accc(FieldPlayer *player, HeapID heapId) {
    player->nogrid = func_ov036_0219cea0(player->core, heapId);
}

void FieldPlayer_UpdateRailMovement(FieldPlayer *player, u32 pressedKeys, u32 heldKeys) {
    FieldPlayerRail_UpdateMovement(player->nogrid, pressedKeys, heldKeys);
}

void func_ov036_0219ace8(FieldPlayer *player, const void *pos) {
    func_ov036_0219cf48(player->nogrid, pos);
}

void func_ov036_0219acf4(FieldPlayer *player) {
    func_ov036_0219cf84(player->nogrid);
}

BOOL func_ov036_0219ad00(FieldPlayer *player) {
    return func_ov036_0219d018(player->nogrid);
}

void *func_ov036_0219ad0c(FieldPlayer *player) {
    return func_ov036_0219d05c(player->nogrid);
}

void FieldPlayer_SetRailPos(FieldPlayer *player, const RailPosition *pos) {
    FieldPlayerRail_SetRailPos(player->nogrid, pos);
}

void func_ov036_0219ad24(FieldPlayer *player, RailPosition *pos) {
    FieldPlayerCore_GetRailPos(player->nogrid, pos);
}

void func_ov036_0219ad30(FieldPlayer *player, u32 dir, RailPosition *pos) {
    func_ov036_0219d044(player->nogrid, dir, pos);
}

void FieldPlayer_GetRailWorldPos(FieldPlayer *player, VecFx32 *pos) {
    FieldPlayerRail_GetWorldPos(player->nogrid, pos);
}

FieldPlayerGrid *func_ov036_0219ad48(FieldPlayer *player) {
    return player->grid;
}
