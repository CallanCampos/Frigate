#ifndef POKEBW2_FIELD_FIELD_PLAYER_CORE_H
#define POKEBW2_FIELD_FIELD_PLAYER_CORE_H

// The part of the field's player that grid and rail movement share, overlay 36's field_player_core.c. Function names
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

FieldPlayerCore *FieldPlayerCore_Create(PlayerState *state, Field *field, const VecFx32 *pos, u32 sex, HeapID heapId);
void FieldPlayerCore_Free(FieldPlayerCore *core);
void FieldPlayerCore_UpdateActionStatusObserver(FieldPlayerCore *core);
void FieldPlayerCore_GetWPos(FieldPlayerCore *core, VecFx32 *pos);
void FieldPlayerCore_SetWPos(FieldPlayerCore *core, const VecFx32 *pos);
u16 FieldPlayerCore_GetFaceDir(FieldPlayerCore *core);
void FieldPlayerCore_SetDirection(FieldPlayerCore *core, u16 dir);
void FieldPlayerCore_ForceObjCode(FieldPlayerCore *core, u16 objCode);
void FieldPlayerCore_ClearObjCode(FieldPlayerCore *core);
Field *FieldPlayerCore_GetField(FieldPlayerCore *core);
FieldActor *FieldPlayerCore_GetActor(FieldPlayerCore *core);
u32 FieldPlayerCore_GetMoveStatus(FieldPlayerCore *core);
void FieldPlayerCore_ForceMoveObserveStatus(FieldPlayerCore *core, u32 status);
u32 FieldPlayerCore_GetMoveObserveStatus(FieldPlayerCore *core);
u32 FieldPlayerCore_GetExState(FieldPlayerCore *core);
void FieldPlayerCore_SetExState(FieldPlayerCore *core, u32 state);
u32 FieldPlayerCore_GetSex(FieldPlayerCore *core);
GameSystem *FieldPlayerCore_GetGameSystem(FieldPlayerCore *core);
PlayerState *FieldPlayerCore_GetPlayerState(FieldPlayerCore *core);
BOOL FieldPlayerCore_WaitModelReady(FieldPlayerCore *core);
u32 FieldPlayerCore_DeriveExState(FieldPlayerCore *core);
u32 FieldPlayerCore_CheckFormModel(FieldPlayerCore *core);
BOOL FieldPlayerCore_CheckAnimePaused(FieldPlayerCore *core);
u16 FieldPlayerCore_GetMoveDirByKey(FieldPlayerCore *core, u32 heldKeys);
void FieldPlayerCore_SetRideEffect(FieldPlayerCore *core, FieldEffectTask *task);
FieldEffectTask *FieldPlayerCore_GetTerrainEffectTCB(FieldPlayerCore *core);
void FieldPlayerCore_SetSpecialSeq(FieldPlayerCore *core, u32 seq);
BOOL FieldPlayerCore_StepFormChange(FieldPlayerCore *core);
void FieldPlayerCore_FinishFormChange(FieldPlayerCore *core);
void FieldPlayerCore_SetMoveStatus(FieldPlayerCore *core, u32 status);
void FieldPlayerCore_InitModel(FieldPlayerCore *core);
// Changes the player's model to a form's
void FieldPlayerCore_SetFormSlotModel(FieldPlayerCore *core, u32 forme);
BOOL FieldPlayerCore_IsAcmdFinished(FieldPlayerCore *core);
void FieldPlayerCore_CalcSeparateKeyDirs(FieldPlayerCore *core, u32 heldKeys);

#endif // POKEBW2_FIELD_FIELD_PLAYER_CORE_H
