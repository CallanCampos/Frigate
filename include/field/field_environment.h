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
// The field's lights
void *FieldLight_Create(u32 lightsId, u32 daySeconds, u32 season, FieldFog *fog, G3DLight *lights, HeapID heapId);
void FieldLight_Free(void *light);
void FieldLight_Update(void *light, u32 daySeconds);
void FieldLight_Flush(void *light, BOOL a1);
// Flashes the lights to a color, without fading back
void FieldLight_StartFlashOneWay(void *light, u16 color, u16 a2);

#endif // POKEBW2_FIELD_FIELD_ENVIRONMENT_H
