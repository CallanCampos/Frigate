#ifndef POKEBW2_FIELD_FIELD_ENVIRONMENT_H
#define POKEBW2_FIELD_FIELD_ENVIRONMENT_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "struct_decls.h"

void *Field_GetLightSystem(Field *field);
void *Field_GetFogCtrl(Field *field);
void *Field_GetWeatherSystem(Field *field);
u16 func_ov036_02199220(void *weatherSystem);
u32 Field_GetWeatherForZone(Field *field, u16 zoneId);
// field_light.c: the field's lights, which follow the time of day
FieldLight *FieldLight_Create(u32 lightsId, s32 daySeconds, u32 season, FieldFog *fog, G3DLight *lights, HeapID heapId);
void FieldLight_Free(FieldLight *light);
void FieldLight_Update(FieldLight *light, s32 daySeconds);
// Sends the lights to the geometry engine if they changed, or always with force
void FieldLight_Flush(FieldLight *light, BOOL force);
// Fades to the light setups of file fileId of archive arcId over frames, or back to the zone's own
void FieldLight_ChangeData(FieldLight *light, u32 arcId, u32 fileId, u32 frames);
void FieldLight_ChangeDataDefault(FieldLight *light);
// Flashes the lights to a color over frames, and back over returnFrames, or without fading back
void FieldLight_StartFlash(FieldLight *light, u16 color, u32 frames, u32 returnFrames);
void FieldLight_StartFlashOneWay(FieldLight *light, u16 color, u32 frames);
BOOL FieldLight_IsFlashRunning(FieldLight *light);

#endif // POKEBW2_FIELD_FIELD_ENVIRONMENT_H
