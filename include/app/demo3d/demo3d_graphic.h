#ifndef POKEBW2_APP_DEMO3D_DEMO3D_GRAPHIC_H
#define POKEBW2_APP_DEMO3D_DEMO3D_GRAPHIC_H

// The BGs, cell actors and 3D system of the 3D demos (overlay 293), named after the ROM's "demo3d_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

typedef struct Demo3DGraphic Demo3DGraphic;

// A demo that draws 3D on both screens has no BGs and no cell actors
Demo3DGraphic *Demo3DGraphic_Create(u32 layout, u32 demoId, HeapID heapId);
void Demo3DGraphic_Free(Demo3DGraphic *graphic);
void Demo3DGraphic_Update(Demo3DGraphic *graphic);
// Around the frame's 3D drawing
void Demo3DGraphic_Begin3D(Demo3DGraphic *graphic);
void Demo3DGraphic_End3D(Demo3DGraphic *graphic);
ClActUnit *Demo3DGraphic_GetClActUnit(Demo3DGraphic *graphic);
// The four lights' colors, as FieldLightStatus_Get gives them
void Demo3DGraphic_SetLightColors(Demo3DGraphic *graphic, const GXRgb *colors, u32 unused);
void Demo3DGraphic_SetLightColor(Demo3DGraphic *graphic, u8 light, GXRgb color);
// Sets a light's direction, which it normalizes
void Demo3DGraphic_SetLightVector(Demo3DGraphic *graphic, u8 light, const VecFx16 *direction);

#endif // POKEBW2_APP_DEMO3D_DEMO3D_GRAPHIC_H
