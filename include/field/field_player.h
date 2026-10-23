#ifndef POKEBW2_FIELD_FIELD_PLAYER_H
#define POKEBW2_FIELD_FIELD_PLAYER_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The direction that stands for the player's own tile, and for no direction held
#define PLAYER_DIR_NONE 9

BOOL Field_HasPlayer(Field *field);
BOOL Field_ToggleCycling(Field *field);
void FieldPlayer_GetWPosInDir(FieldPlayer *player, u16 direction, VecFx32 *position);
u32 FieldPlayer_GetExState(FieldPlayer *player);
u32 FieldPlayer_DeriveExState(FieldPlayer *player);
void FieldPlayer_SetSpecialState(FieldPlayer *player, u32 state);
void FieldPlayer_SetSpecialSeq(FieldPlayer *player, u32 seq);
BOOL func_ov036_0219a580(FieldPlayer *player);
u32 func_ov036_0219a864(FieldPlayer *player);
BOOL func_ov036_0219a834(FieldPlayer *player);
BOOL func_ov036_0219a870(FieldPlayer *player);
BOOL func_ov036_0219ab24(FieldPlayer *player);
u32 FieldPlayer_GetTileTypeUnder(FieldPlayer *player);
// The player's grid and rail movement
void FieldPlayer_InitGrid(FieldPlayer *player, HeapID heapId);
void func_ov036_0219accc(FieldPlayer *player, HeapID heapId);
u16 FieldPlayer_GetMoveDirByKey(FieldPlayer *player, u32 heldKeys);
BOOL func_ov036_0219a5dc(FieldPlayer *player);
BOOL FieldPlayer_CheckRunningShoesFlag(FieldPlayer *player);
void FieldPlayer_UpdateGridMovement(FieldPlayer *player, u32 pressedKeys, u32 heldKeys, u32 flags);
void FieldPlayer_UpdateRailMovement(FieldPlayer *player, u32 pressedKeys, u32 heldKeys);
void func_ov036_0219ace8(FieldPlayer *player, const void *pos);
void func_ov036_0219acf4(FieldPlayer *player);
BOOL func_ov036_0219ad00(FieldPlayer *player);
void *func_ov036_0219ad0c(FieldPlayer *player);
u32 FieldPlayer_GetTileTypeInDir(FieldPlayer *player, u16 direction);
FieldActor *FieldPlayer_GetActorInFront(FieldPlayer *player);
FieldActor *FieldPlayer_GetActorInFrontEx(FieldPlayer *player, fx32 maxHeightDiff);
BOOL CheckSurfHeightAllow(FieldPlayer *player, u32 direction);
BOOL CheckCanInteractWaterfall(FieldPlayer *player, u32 tileUnder, u32 tileInFront);
FieldPlayer *FieldPlayer_Create(PlayerState *state, Field *field, const VecFx32 *pos, u32 sex, HeapID heapId);
void FieldPlayer_Free(FieldPlayer *player);
void FieldPlayer_SyncState(FieldPlayer *player);
void FieldPlayer_ForceBrake(FieldPlayer *player);
void FieldPlayer_SetRailPos(FieldPlayer *player, const RailPosition *pos);
void FieldPlayer_GetRailWorldPos(FieldPlayer *player, VecFx32 *pos);
// The event of walking into an actor, or NULL. Only on the grid
GameEvent *FieldPlayer_CheckObjContactEvent(FieldPlayer *player, u32 pressedKeys, u32 heldKeys, u32 flags);
void func_ov036_0219a58c(FieldPlayer *player);
// The core's terrain effect task: set and get
void func_ov036_0219a5b8(FieldPlayer *player, FieldEffectTask *task);
FieldEffectTask *func_ov036_0219a5c4(FieldPlayer *player);
void func_ov036_0219a5fc(FieldPlayer *player);
void FieldPlayer_UpdateActionStatusObserver(FieldPlayer *player);
Field *FieldPlayer_GetField(FieldPlayer *player);
// What FieldPlayer_GetMoveObserveStatus says of the player's last action command: none, one that has started, one
// that goes on, and one that has just ended
enum {
    PLAYER_OBSERVE_NONE,
    PLAYER_OBSERVE_START,
    PLAYER_OBSERVE_MOVING,
    PLAYER_OBSERVE_END,
};

// What FieldPlayer_GetMoveStatus says the player does, as the grid's player sets it: stands (or brakes against a wall),
// moves to the next tile, or turns on the spot
enum {
    PLAYER_MOVE_STATUS_NONE,
    PLAYER_MOVE_STATUS_MOVE,
    PLAYER_MOVE_STATUS_TURN,
};

u32 FieldPlayer_GetMoveStatus(FieldPlayer *player);
u32 FieldPlayer_GetMoveObserveStatus(FieldPlayer *player);
GameSystem *FieldPlayer_GetGameSystem(FieldPlayer *player);
// The tile type under the player (PLAYER_DIR_NONE) or a step in the direction, and its height. 0xffffffff if there is
// no terrain
u32 FieldPlayer_GetTileTypeInDirEx(FieldPlayer *player, u16 direction, fx32 *height);
// The form with the object code, 10 if it isn't one of the player's
u32 FieldPlayer_GetExStateByObjCode(u32 objCode);
void FieldPlayer_ForceObjCode(FieldPlayer *player, u16 objCode);
void FieldPlayer_ClearObjCode(FieldPlayer *player);
void FieldPlayer_GetGPosInFront(FieldPlayer *player, s16 *x, s16 *y, s16 *z);
void FieldPlayer_InterruptCatwalkBalance(FieldPlayer *player, BOOL keep);
void func_ov036_0219abd8(FieldPlayer *player);
BOOL FieldPlayer_GetVerticalMoveOnlyFlag(FieldPlayer *player);
void func_ov036_0219ac0c(FieldPlayer *player);
FieldPlayerGrid *func_ov036_0219ad48(FieldPlayer *player);

#endif // POKEBW2_FIELD_FIELD_PLAYER_H
