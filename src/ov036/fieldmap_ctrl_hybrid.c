#include "types.h"
#include "constants/obj_codes.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_controller.h"
#include "field/field_map.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_player.h"
#include "field/field_rail.h"
#include "field/fieldmap_ctrl_hybrid.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_system.h"

// What FieldPlayer_GetMoveDirByKey returns when no direction is held
#define MOVE_DIR_NONE 9

// The controller types
#define HYBRID_GRID 0
#define HYBRID_RAIL 1
// The type before the first is set
#define HYBRID_NONE 2

void FieldmapCtrlHybrid_ReplacePlayer(FieldmapCtrlHybrid *controller, Field *field);
void FieldmapCtrlHybrid_UpdateGrid(Field *field, FieldmapCtrlHybrid *controller);
void FieldmapCtrlHybrid_UpdateNogrid(Field *field, FieldmapCtrlHybrid *controller);
void FieldmapCtrlHybrid_SwitchToNogrid(Field *field, FieldmapCtrlHybrid *controller, u16 dir, BOOL keyDir,
                                       const RailPosition *railPos, u32 pressedKeys, u32 heldKeys);
void FieldmapCtrlHybrid_SwitchToGrid(Field *field, FieldmapCtrlHybrid *controller, u16 dir, BOOL keyDir,
                                     const VecFx32 *pos, u32 pressedKeys, u32 heldKeys);
BOOL FieldmapCtrlHybrid_FindGridPos(Field *field, FieldmapCtrlHybrid *controller, VecFx32 *pos, u16 dir);
BOOL FieldmapCtrlHybrid_FindRailLocation(Field *field, FieldmapCtrlHybrid *controller, RailPosition *railPos, u16 dir);
void FieldmapCtrlHybrid_SetMode(Field *field, FieldmapCtrlHybrid *controller, u32 type, const void *pos, u16 dir,
                                u16 anim, u16 step);

// The update of each controller type
static void (*sUpdateFuncs[2])(Field *field, FieldmapCtrlHybrid *controller) = {
    FieldmapCtrlHybrid_UpdateGrid,
    FieldmapCtrlHybrid_UpdateNogrid,
};

// The player's actor on the grid, whose object code is filled in
static ZoneNPC sGridPlayer = { 0xff,   OBJ_CODE_HERO, MOVE_CODE_NONE, ACTOR_EVENT_TYPE_NONE, 0, 0, 0, 0, 0, 0,
                               0xffff, 0xffff,        FALSE };

u32 FieldmapCtrlHybrid_GetActiveTypeID(FieldmapCtrlHybrid *controller) {
    return controller->activeType;
}

// Moves the player to the other kind of movement where it stands, if it can
void FieldmapCtrlHybrid_ReplacePlayer(FieldmapCtrlHybrid *controller, Field *field) {
    u16 dir = GetActorFaceDir(FieldPlayer_GetActor(controller->player));
    RailPosition railPos;
    VecFx32 pos;

    if (controller->activeType == HYBRID_GRID) {
        if (FieldmapCtrlHybrid_FindRailLocation(field, controller, &railPos, dir)) {
            FieldmapCtrlHybrid_SwitchToNogrid(field, controller, dir, FALSE, &railPos, 0, 0);
        }
    } else if (FieldmapCtrlHybrid_FindGridPos(field, controller, &pos, dir)) {
        FieldmapCtrlHybrid_SwitchToGrid(field, controller, dir, FALSE, &pos, 0, 0);
    }
    controller->forcedMove = 0;
}

void FieldmapCtrlHybrid_Create(Field *field, VecFx32 *pos, u16 dir) {
    PlayerState *playerState = GameData_GetPlayerState(GSYS_GetGameData(Field_GetGameSystem(field)));
    HeapID heapId = Field_GetHeapID(field);
    FieldmapCtrlHybrid *controller =
        GFL_HeapAllocate(heapId, sizeof(FieldmapCtrlHybrid), TRUE, "fieldmap_ctrl_hybrid.c", 189);
    u32 isRail;
    FieldActor *actor;
    u16 step;
    u16 anim;
    RailPosition *railPos;

    Field_SetController(field, controller);
    controller->player = Field_GetPlayer(field);
    isRail = PlayerState_IsRail(playerState);
    controller->activeType = HYBRID_NONE;
    actor = FieldPlayer_GetActor(controller->player);
    RailActorAnim_GetBillboardActorAnime(actor, &anim, &step);
    if (isRail == FALSE) {
        FieldPlayer_InitGrid(controller->player, heapId);
        FieldPlayer_CreateNogrid(controller->player, heapId);
        FieldPlayer_SetDirection(controller->player, dir);
        FieldmapCtrlHybrid_SetMode(field, controller, isRail, pos, dir, anim, step);
    } else {
        railPos = PlayerState_GetRailPos(playerState);
        FieldPlayer_CreateNogrid(controller->player, heapId);
        FieldPlayer_InitGrid(controller->player, heapId);
        FieldPlayer_SetDirection(controller->player, dir);
        FieldmapCtrlHybrid_SetMode(field, controller, isRail, railPos, dir, anim, step);
    }
    Field_SetPlayerPosPtr(field, GetMModelWPosPtr(actor));
    FieldPlayer_GetWPos(controller->player, pos);
}

void FieldmapCtrlHybrid_Free(Field *field) {
    GFL_HeapFree(Field_GetController(field));
}

void FieldmapCtrlHybrid_Update(Field *field, VecFx32 *pos) {
    FieldmapCtrlHybrid *controller = Field_GetController(field);

    sUpdateFuncs[controller->activeType](field, controller);
    FieldPlayer_GetWPos(controller->player, pos);
}

VecFx32 *FieldmapCtrlHybrid_GetPos(Field *field) {
    return GetMModelWPosPtr(FieldPlayer_GetActor(Field_GetPlayer(field)));
}

// On the grid: walks onto a rail when the player walks against a wall from a rail's tile
void FieldmapCtrlHybrid_UpdateGrid(Field *field, FieldmapCtrlHybrid *controller) {
    u32 pressedKeys = GCTX_HIDGetPressedKeys();
    u32 heldKeys = GCTX_HIDGetHeldKeys();
    u32 flags = 0;
    FieldActor *actor = FieldPlayer_GetActor(controller->player);
    u16 dir = FieldPlayer_GetMoveDirByKey(controller->player, heldKeys);
    u32 tileUnder;
    u32 tileAhead;
    RailPosition railPos;

    if ((IsAllActorAcmdFinished(actor) == TRUE || FieldPlayer_CheckMoveBusy(controller->player) == TRUE) &&
        dir != MOVE_DIR_NONE) {
        tileUnder = FieldPlayer_GetTileTypeUnder(controller->player);
        tileAhead = FieldPlayer_GetTileTypeInDir(controller->player, dir);
        if (FieldRailTilemap_IsJumpStart(GetTileClass(tileUnder)) && MapTile_BlocksCollision(tileAhead) &&
            FieldmapCtrlHybrid_FindRailLocation(field, controller, &railPos, dir)) {
            FieldmapCtrlHybrid_SwitchToNogrid(field, controller, dir, TRUE, &railPos, pressedKeys, heldKeys);
            return;
        }
    }
    if (FieldPlayer_CheckRunningShoesFlag(controller->player) == TRUE) {
        flags |= MOVE_FLAG_CAN_RUN;
    }
    FieldPlayer_UpdateGridMovement(controller->player, pressedKeys, heldKeys, flags);
}

// On a rail: steps off onto the grid the same way
void FieldmapCtrlHybrid_UpdateNogrid(Field *field, FieldmapCtrlHybrid *controller) {
    u32 pressedKeys = GCTX_HIDGetPressedKeys();
    u32 heldKeys = GCTX_HIDGetHeldKeys();
    FieldActor *actor = FieldPlayer_GetActor(controller->player);
    u16 dir;
    BOOL keyDir;
    u32 tileUnder;
    u32 tileAhead;
    VecFx32 pos;

    if (controller->forcedMove == 0) {
        dir = FieldPlayer_GetMoveDirByKey(controller->player, heldKeys);
        keyDir = TRUE;
    } else {
        dir = GetActorMotionDir(actor);
        keyDir = FALSE;
    }
    if ((IsAllActorAcmdFinished(actor) == TRUE || FieldPlayer_CheckMoveBusy(controller->player) == TRUE) &&
        dir != MOVE_DIR_NONE) {
        tileUnder = FieldPlayer_GetTileTypeUnder(controller->player);
        tileAhead = FieldPlayer_GetTileTypeInDir(controller->player, dir);
        if (FieldRailTilemap_IsJumpStart(GetTileClass(tileUnder)) && MapTile_BlocksCollision(tileAhead) &&
            FieldmapCtrlHybrid_FindGridPos(field, controller, &pos, dir)) {
            FieldmapCtrlHybrid_SwitchToGrid(field, controller, dir, keyDir, &pos, pressedKeys, heldKeys);
            controller->forcedMove = 0;
            return;
        }
    }
    controller->forcedMove = FieldPlayer_IsNogridForcedMove(controller->player);
    FieldPlayer_UpdateRailMovement(controller->player, pressedKeys, heldKeys);
}

void FieldmapCtrlHybrid_SwitchToNogrid(Field *field, FieldmapCtrlHybrid *controller, u16 dir, BOOL keyDir,
                                       const RailPosition *railPos, u32 pressedKeys, u32 heldKeys) {
    FieldActor *actor = FieldPlayer_GetActor(controller->player);
    u16 step;
    u16 anim;

    RailActorAnim_GetBillboardActorAnime(actor, &anim, &step);
    FieldPlayer_UpdateGridMovement(controller->player, 0, 0, 0);
    if (!keyDir) {
        dir = GetActorFaceDir(actor);
    }
    FieldmapCtrlHybrid_SetMode(field, controller, HYBRID_RAIL, railPos, dir, anim, step);
    FieldPlayer_UpdateRailMovement(controller->player, pressedKeys, heldKeys);
}

void FieldmapCtrlHybrid_SwitchToGrid(Field *field, FieldmapCtrlHybrid *controller, u16 dir, BOOL keyDir,
                                     const VecFx32 *pos, u32 pressedKeys, u32 heldKeys) {
    u32 flags = 0;
    FieldActor *actor = FieldPlayer_GetActor(controller->player);
    u16 step;
    u16 anim;

    RailActorAnim_GetBillboardActorAnime(actor, &anim, &step);
    FieldPlayer_UpdateRailMovement(controller->player, 0, 0);
    if (!keyDir) {
        dir = GetActorFaceDir(actor);
    }
    FieldmapCtrlHybrid_SetMode(field, controller, HYBRID_GRID, pos, dir, anim, step);
    if (FieldPlayer_CheckRunningShoesFlag(controller->player) == TRUE) {
        flags |= MOVE_FLAG_CAN_RUN;
    }
    FieldPlayer_UpdateGridMovement(controller->player, pressedKeys, heldKeys, flags);
}

// Whether the grid tile in the direction is free, with the player's position
BOOL FieldmapCtrlHybrid_FindGridPos(Field *field, FieldmapCtrlHybrid *controller, VecFx32 *pos, u16 dir) {
    FieldG3DMapper *mapper = Field_GetG3DMapper(field);
    VecFx32 ahead;

    FieldPlayer_GetWPos(controller->player, pos);
    ahead = *pos;
    ExpandVecInGridDir(dir, &ahead, FX32_CONST(16));
    if (!MapTile_BlocksCollision(GetTileTypeAtPos(mapper, &ahead))) {
        return TRUE;
    }
    return FALSE;
}

// Whether there is a rail where the player stands, and where on it
BOOL FieldmapCtrlHybrid_FindRailLocation(Field *field, FieldmapCtrlHybrid *controller, RailPosition *railPos, u16 dir) {
    FieldRailSystem *rail = FieldNoGridMapper_GetRailSystem(Field_GetNoGridMapper(field));
    VecFx32 hit;
    VecFx32 pos;
    VecFx32 top;
    RailPosition found;
    RailPosition next;
    BOOL result;

    FieldPlayer_GetWPos(controller->player, &pos);
    top.x = pos.x;
    top.y = pos.y + FX32_CONST(4);
    top.z = pos.z;
    pos.y -= FX32_CONST(4);
    result = CalculateRailCurves(rail, &pos, &top, &found, &hit);
    if (result) {
        result = FieldRailSystem_TryStepLocation(rail, &found, ConvDirToRailDir(dir), &next);
    }
    *railPos = found;
    return result;
}

// Switches the player to the controller type at the position, a VecFx32 for the grid and a RailPosition for a rail
void FieldmapCtrlHybrid_SetMode(Field *field, FieldmapCtrlHybrid *controller, u32 type, const void *pos, u16 dir,
                                u16 anim, u16 step) {
    NoGridMapper *mapper = Field_GetNoGridMapper(field);
    FieldCamera *camera = Field_GetCameraSystem(field);
    FieldActor *actor;
    const VecFx32 *vec;
    VecFx32 position;

    if (controller->activeType == type) {
        return;
    }
    actor = FieldPlayer_GetActor(controller->player);
    if (FldAct_CheckAcmdSet(actor)) {
        FldAct_FreeAcmd(actor);
    }
    if (type == HYBRID_GRID) {
        vec = pos;
        position = *vec;
        sGridPlayer.modelId = FldAct_GetObjCode(actor);
        FieldPlayer_ResetNogrid(controller->player);
        FldAct_Transplant(actor, &sGridPlayer);
        SetActorGPos(actor, FX_Whole(position.x) / 16, FX_Whole(position.y) / 16, FX_Whole(position.z) / 16, dir);
        CopyActorWPos(actor, &position);
        FieldPlayer_SetWPos(controller->player, &position);
        FieldNoGridMapper_ClearCameraParent(mapper);
        FieldCamera_ChangeTransformType(camera, 0);
        FieldCamera_ResetBind(camera);
        FieldCamera_LoadDefaults(camera);
    } else {
        FieldPlayer_AttachNogridActor(controller->player, pos);
        FieldNoGridMapper_SetCameraParent(mapper, FieldPlayer_GetNogridRailWork(controller->player));
        FieldCamera_EnableDelay(camera);
    }
    CheckSetActorFaceDir(actor, dir);
    SetActorMotionDir(actor, dir);
    RailActorAnim_SetBillboardActorAnime(actor, anim, step);
    controller->activeType = type;
}
