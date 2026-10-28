#ifndef POKEBW2_FIELD_FIELD_MOVE_TCB_H
#define POKEBW2_FIELD_FIELD_MOVE_TCB_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

TCB *FieldSurfTCB_Create(FieldPlayer *player, u32 direction, u32 tileType, HeapID heapId);
BOOL FieldSurfTCB_CheckEnd(TCB *tcb);
void FieldSurfTCB_Free(TCB *tcb);
TCB *FieldWaterfallTCB_Create(FieldPlayer *player, u32 direction, u32 param, HeapID heapId);
BOOL FieldWaterfallTCB_CheckEnd(TCB *tcb);
void FieldWaterfallTCB_Free(TCB *tcb);
void FldeffCut_Start(FieldActor *actor, FieldEffects *effects);

#endif // POKEBW2_FIELD_FIELD_MOVE_TCB_H
