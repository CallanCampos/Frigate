#ifndef POKEBW2_FIELD_FIELDMAP_TCB_CAMERA_ZOOM_H
#define POKEBW2_FIELD_FIELDMAP_TCB_CAMERA_ZOOM_H

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// Both tasks move from the current camera zoom by a signed fixed-point delta.
// The duration is truncated to 16 bits. Use 1 through 65534 to avoid division by zero or counter wrap.
// Square-root interpolation.
void FieldCameraZoomTCB_Create(Field *field, u32 frameCount, fx32 zoomDelta);
// Linear interpolation; our descriptive name.
void FieldCameraZoomTCB_CreateLinear(Field *field, u32 frameCount, fx32 zoomDelta);

#endif // POKEBW2_FIELD_FIELDMAP_TCB_CAMERA_ZOOM_H
