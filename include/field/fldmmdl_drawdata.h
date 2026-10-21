#ifndef POKEBW2_FIELD_FLDMMDL_DRAWDATA_H
#define POKEBW2_FIELD_FLDMMDL_DRAWDATA_H

#include "types.h"
#include "gfl/blact.h"

// Overlay 36's fldmmdl_drawdata.c: the scale of a billboard actor, and the size of its faces in pixels, by its
// billboard size, which fldmmdl_blact.c reads. The file also holds billboard animation sets and the functions of each
// kind of scene node (FIELD_ACTOR_MAIN_VTABLES, in field/field_actor_internal.h).
typedef struct {
    u16 x;
    u16 y;
} BillboardScale;

typedef struct {
    u16 width;
    u16 height;
} BillboardSize;

// An animation set selected by an actor's sprite controller type.
typedef struct {
    const BlActAnimStep *const *animations;
    u32 count;
} FieldBillboardAnimSet;

extern const BillboardScale BILLBOARD_SCALES[3];
extern const BillboardSize MMDL_BILLBOARD_SIZES_INT[3];
extern const FieldBillboardAnimSet FIELD_BILLBOARD_ANIM_SETS[];

#endif // POKEBW2_FIELD_FLDMMDL_DRAWDATA_H
