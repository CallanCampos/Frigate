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
// field_zonefog.c: loads the zone's fog file, delay frames after a request
FieldFogCtrl *FieldFogCtrl_Create(HeapID heapId);
void FieldFogCtrl_Free(FieldFogCtrl *ctrl);
void FieldFogCtrl_Update(FieldFogCtrl *ctrl, HeapID heapId);
void FieldFogCtrl_RequestLoad(FieldFogCtrl *ctrl, u32 fileId, u32 staticLightIndex, s8 delay);
// Whether a load is waiting for its delay
u8 func_ov036_021977a4(FieldFogCtrl *ctrl);
void FieldFogCtrl_Reset(FieldFogCtrl *ctrl);
BOOL FieldFogCtrl_CheckArc(FieldFogCtrl *ctrl);
BOOL FieldFogCtrl_IsStaticLightExists(FieldFogCtrl *ctrl);
u32 FieldFogCtrl_GetFogParam0(FieldFogCtrl *ctrl);
u32 FieldFogCtrl_GetFogParam1(FieldFogCtrl *ctrl);
u32 FieldFogCtrl_GetStaticLightIndex(FieldFogCtrl *ctrl);

#endif // POKEBW2_FIELD_FIELD_FOG_H
