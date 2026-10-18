#ifndef POKEBW2_FIELD_FIELD_MOVE_TCB_H
#define POKEBW2_FIELD_FIELD_MOVE_TCB_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

void *FieldSurfTCB_Create(FieldPlayer *player, u32 direction, u32 tileType, HeapID heapId);
BOOL FieldSurfTCB_CheckEnd(void *tcb);
void FieldSurfTCB_Free(void *tcb);
void *FieldWaterfallTCB_Create(FieldPlayer *player, u32 direction, u32 param, HeapID heapId);
BOOL FieldWaterfallTCB_CheckEnd(void *tcb);
void FieldWaterfallTCB_Free(void *tcb);
void func_ov036_021c2e70(FieldActor *actor, FieldEffects *effects);

#endif // POKEBW2_FIELD_FIELD_MOVE_TCB_H
