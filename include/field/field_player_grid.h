#ifndef POKEBW2_FIELD_FIELD_PLAYER_GRID_H
#define POKEBW2_FIELD_FIELD_PLAYER_GRID_H

// The field player's grid movement, overlay 36's field_player_grid.c. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// How a held direction continues the player's movement, which FieldPlayerGrid_GetMoveDirContType returns
enum {
    MOVE_DIR_CONT_NONE,
    MOVE_DIR_CONT_MOVE,
    MOVE_DIR_CONT_TURN,
};

FieldPlayerGrid *FieldPlayerGrid_Create(FieldPlayerCore *core, HeapID heapId);
void FieldPlayerGrid_Free(FieldPlayerGrid *grid);
// One frame of the player's movement. Bit 0 of the flags allows running
void FieldPlayerGrid_UpdateMovement(FieldPlayerGrid *grid, u32 pressedKeys, u32 heldKeys, u32 flags);
u32 FieldPlayerGrid_GetMoveDirContType(FieldPlayerGrid *grid, u16 dir);
BOOL FieldPlayerGrid_IsAcmdFinished(FieldPlayerGrid *grid, u16 dir);
// Ends the animation of walking into a wall
void FieldPlayerGrid_ForceBrake(FieldPlayerGrid *grid);
// The same, for a turn too. FALSE while the player is moving
BOOL func_ov036_0219ca30(FieldPlayerGrid *grid);
// Whether the player is sliding on ice
BOOL FieldPlayerGrid_IsConstMoving(FieldPlayerGrid *grid);
// Whether the player is stepping or falling off a catwalk
BOOL FieldPlayerGrid_IsCatwalkExiting(FieldPlayerGrid *grid);
// Puts the player back on the ground after the fall from a catwalk
void FieldPlayerGrid_SanitizeHeightMismatch(FieldPlayerGrid *grid);
FieldPlayerCore *FieldPlayerGrid_GetPlayerCore(FieldPlayerGrid *grid);
FieldActor *FieldPlayerGrid_GetPlayerActor(FieldPlayerGrid *grid);
// Stops the balancing on a catwalk, keeping its time if asked
void FieldPlayerGrid_InterruptCatwalkBalance(FieldPlayerGrid *grid, BOOL keep);
// Ends a slide
void func_ov036_0219cd84(FieldPlayerGrid *grid);
// Set when the fall from a catwalk lands, until the next frame of movement
BOOL FieldPlayerGrid_GetVerticalMoveOnlyFlag(FieldPlayerGrid *grid);
void FieldPlayerGrid_SetVerticalMoveOnlyFlag(FieldPlayerGrid *grid);
// Whether the player is walking into a wall
BOOL FieldPlayerGrid_IsBraking(FieldPlayerGrid *grid);
// Later in overlay 36, with the grid player's events
GameEvent *FieldPlayerGrid_CheckObjContactEvent(FieldPlayerGrid *grid, u32 pressedKeys, u32 heldKeys, u32 flags);

#endif // POKEBW2_FIELD_FIELD_PLAYER_GRID_H
