// The controller of the maps where the player moves on the rails, and the controller tables of the rail and hybrid
// maps. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_controller.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_player.h"
#include "field/fieldmap_ctrl_hybrid.h"
#include "field/player_state.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "system/game_data.h"
#include "system/game_system.h"

typedef struct {
    FieldPlayer *player;
    u32 unk04;
} FieldmapCtrlNoGrid;

static void FieldmapCtrlNoGrid_Create(Field *field, VecFx32 *pos, u16 dir);
static void FieldmapCtrlNoGrid_Free(Field *field);
static void FieldmapCtrlNoGrid_Update(Field *field, VecFx32 *pos);
static VecFx32 *FieldmapCtrlNoGrid_GetPos(Field *field);
static FieldmapCtrlNoGrid *FieldmapCtrlNoGrid_CreateWork(Field *field, HeapID heapId);
static void FieldmapCtrlNoGrid_FreeWork(FieldmapCtrlNoGrid *controller);
static void FieldmapCtrlNoGrid_MovePlayer(FieldmapCtrlNoGrid *controller);

const FieldmapCtrlVTable FIELDMAP_CTRL_NOGRID = {
    FIELDMAP_CTRL_TYPE_NOGRID, FieldmapCtrlNoGrid_Create, FieldmapCtrlNoGrid_Update,
    FieldmapCtrlNoGrid_Free,   FieldmapCtrlNoGrid_GetPos,
};

const FieldmapCtrlVTable FIELDMAP_CTRL_HYBRID = {
    FIELDMAP_CTRL_TYPE_HYBRID, FieldmapCtrlHybrid_Create, FieldmapCtrlHybrid_Update,
    FieldmapCtrlHybrid_Free,   FieldmapCtrlHybrid_GetPos,
};

static void FieldmapCtrlNoGrid_Create(Field *field, VecFx32 *pos, u16 dir) {
    FieldPlayer *player;

    Field_GetCameraSystem(field);
    Field_GetNoGridMapper(field);
    player = Field_GetPlayer(field);
    Field_SetController(field, FieldmapCtrlNoGrid_CreateWork(field, Field_GetHeapID(field)));
    Field_SetPlayerPosPtr(field, GetMModelWPosPtr(FieldPlayer_GetActor(player)));
    FieldPlayer_SetRailPos(
        player, PlayerState_GetRailPos(GameData_GetPlayerState(GSYS_GetGameData(Field_GetGameSystem(field)))));
    FieldPlayer_GetRailWorldPos(player, pos);
    FieldPlayer_SetWPos(player, pos);
    FieldPlayer_SetDirection(player, dir);
}

static void FieldmapCtrlNoGrid_Free(Field *field) {
    FieldmapCtrlNoGrid_FreeWork(Field_GetController(field));
}

static void FieldmapCtrlNoGrid_Update(Field *field, VecFx32 *pos) {
    FieldmapCtrlNoGrid_MovePlayer(Field_GetController(field));
}

static VecFx32 *FieldmapCtrlNoGrid_GetPos(Field *field) {
    return GetMModelWPosPtr(FieldPlayer_GetActor(Field_GetPlayer(field)));
}

static FieldmapCtrlNoGrid *FieldmapCtrlNoGrid_CreateWork(Field *field, HeapID heapId) {
    NoGridMapper *mapper = Field_GetNoGridMapper(field);
    FieldmapCtrlNoGrid *controller =
        GFL_HeapAllocate(heapId, sizeof(FieldmapCtrlNoGrid), TRUE, "fieldmap_ctrl_nogrid_work.c", 62);
    FieldPlayer *player = Field_GetPlayer(field);

    controller->player = player;
    FieldPlayer_CreateNogrid(player, heapId);
    FieldNoGridMapper_SetCameraParent(mapper, FieldPlayer_GetNogridRailWork(player));
    return controller;
}

static void FieldmapCtrlNoGrid_FreeWork(FieldmapCtrlNoGrid *controller) {
    GFL_HeapFree(controller);
}

static void FieldmapCtrlNoGrid_MovePlayer(FieldmapCtrlNoGrid *controller) {
    u32 pressedKeys = GCTX_HIDGetPressedKeys();
    u32 heldKeys = GCTX_HIDGetHeldKeys();

    FieldPlayer_UpdateRailMovement(controller->player, pressedKeys, heldKeys);
}
