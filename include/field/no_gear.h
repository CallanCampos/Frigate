#ifndef POKEBW2_FIELD_NO_GEAR_H
#define POKEBW2_FIELD_NO_GEAR_H

#include "types.h"
#include "gfl/overlay.h"
#include "struct_decls.h"

// The plain touch screen shown in place of the C-Gear (FLD_SUBSCREEN_ID_BLANK_STANDBY and
// FLD_SUBSCREEN_ID_BLANK_STANDBY_2), overlay 80, named after the ROM's "no_gear.c". Overlay 36's field subscreen table
// calls these
#define OVERLAY_NO_GEAR OVERLAY_ID(80)

typedef struct NoGearWork NoGearWork;

// The first two arguments are unused
NoGearWork *NoGear_Create(CGearSettings *saveData, FieldSubscreen *subscreen, GameSystem *gsys);
void NoGear_Update(NoGearWork *work);
void NoGear_Draw(NoGearWork *work);
void NoGear_Free(NoGearWork *work);

#endif // POKEBW2_FIELD_NO_GEAR_H
