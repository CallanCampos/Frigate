// The controller of the maps where the player moves on the grid. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_controller.h"
#include "field/field_player.h"
#include "gfl/heap.h"
#include "gfl/key.h"

static void FieldmapCtrlGrid_Create(Field *field, VecFx32 *pos, u16 dir);
static void FieldmapCtrlGrid_Free(Field *field);
static void FieldmapCtrlGrid_Update(Field *field, VecFx32 *pos);
static VecFx32 *FieldmapCtrlGrid_GetPos(Field *field);

const FieldmapCtrlVTable FIELDMAP_CTRL_GRID = {
    0, FieldmapCtrlGrid_Create, FieldmapCtrlGrid_Update, FieldmapCtrlGrid_Free, FieldmapCtrlGrid_GetPos,
};

static void FieldmapCtrlGrid_Create(Field *field, VecFx32 *pos, u16 dir) {
    HeapID heapId = Field_GetHeapID(field);
    FieldmapCtrl *controller = GFL_HeapAllocate(heapId, sizeof(FieldmapCtrlGrid), TRUE, "fieldmap_ctrl_grid.c", 104);
    FieldPlayer *player;
    FieldActor *actor;

    controller->grid.heapId = heapId;
    controller->grid.field = field;
    player = Field_GetPlayer(field);
    actor = FieldPlayer_GetActor(player);
    CheckSetActorFaceDir(actor, dir);
    FieldPlayer_InitGrid(player, heapId);
    Field_SetPlayerPosPtr(field, GetMModelWPosPtr(actor));
    Field_SetController(field, controller);
}

static void FieldmapCtrlGrid_Free(Field *field) {
    GFL_HeapFree(Field_GetController(field));
}

static void FieldmapCtrlGrid_Update(Field *field, VecFx32 *pos) {
    FieldmapCtrlGrid *controller = &Field_GetController(field)->grid;
    FieldPlayer *player = Field_GetPlayer(field);

    if (!controller->updateDisable) {
        u32 pressedKeys = GCTX_HIDGetPressedKeys();
        u32 heldKeys = GCTX_HIDGetHeldKeys();
        u32 flags = 0;

        if (FieldPlayer_CheckRunningShoesFlag(player) == TRUE) {
            flags |= 1;
        }
        FieldPlayer_UpdateGridMovement(player, pressedKeys, heldKeys, flags);
    }
}

static VecFx32 *FieldmapCtrlGrid_GetPos(Field *field) {
    return GetMModelWPosPtr(FieldPlayer_GetActor(Field_GetPlayer(field)));
}

void FieldmapCtrlGrid_SetUpdateDisable(Field *field, BOOL disable) {
    FieldmapCtrlGrid *controller = &Field_GetController(field)->grid;

    controller->updateDisable = disable;
}
