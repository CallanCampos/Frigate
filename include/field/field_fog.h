#ifndef POKEBW2_FIELD_FIELD_FOG_H
#define POKEBW2_FIELD_FIELD_FOG_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/gx.h"
#include "struct_decls.h"

// The field's fog, which takes effect when flushed
void FieldFog_SetEnabled(FieldFog *fog, BOOL enabled);
BOOL FieldFog_IsEnabled(FieldFog *fog);
void FieldFog_SetAlphaMode(FieldFog *fog, u32 alphaMode);
void FieldFog_SetAlpha(FieldFog *fog, u32 alpha);
void FieldFog_SetDepthShift(FieldFog *fog, u32 depthShift);
void FieldFog_SetOffset(FieldFog *fog, u32 offset);
void FieldFog_SetColor(FieldFog *fog, GXRgb color);
// The fog's density at 32 depths
void FieldFog_SetTable(FieldFog *fog, const u8 *table);
void FieldFog_Flush(FieldFog *fog);
u32 FieldFog_GetDepthShift(FieldFog *fog);
void FieldFog_Animate(FieldFog *fog, u32 color, u32 depthShift, u32 duration);
FieldFog *FieldFog_Create(HeapID heapId);
void FieldFog_Free(FieldFog *fog);
void FieldFog_Update(FieldFog *fog);
void *FieldFogCtrl_Create(HeapID heapId);
void FieldFogCtrl_Free(void *ctrl);
void FieldFogCtrl_Update(void *ctrl, HeapID heapId);
void FieldFogCtrl_RequestLoad(void *ctrl, u32 fogIndex, u32 lightIndex, BOOL a3);
void FieldFogCtrl_Reset(void *ctrl);

#endif // POKEBW2_FIELD_FIELD_FOG_H
