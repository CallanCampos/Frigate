#ifndef POKEBW2_FIELD_C_GEAR_SW_H
#define POKEBW2_FIELD_C_GEAR_SW_H

#include "types.h"
#include "struct_decls.h"

// The C-Gear, the field's touch screen while it is on: overlay 79, named after the ROM's "c_gear_sw.c". Overlay 36's
// field subscreen table calls these. The names are ours

typedef struct CGearSw CGearSw;

typedef void (*CGearSwCallback)(void *work);

CGearSw *CGearSw_Create(CGearSettings *settings, FieldSubscreen *subscreen, GameSystem *gsys, BOOL startup);
// Calls the callback with its work once the C-Gear shows, which with startup is after its panels have appeared
CGearSw *CGearSw_CreateWithCallback(CGearSettings *settings, FieldSubscreen *subscreen, GameSystem *gsys,
                                    CGearSwCallback callback, void *callbackWork, BOOL startup);
// active is whether the player can use the C-Gear, which they can't during an event
void CGearSw_Update(CGearSw *work, BOOL active);
void CGearSw_Draw(CGearSw *work);
void CGearSw_Free(CGearSw *work);
// Loads everything again, as after the skin was changed, and returns whether it did, which it does only while idle
BOOL CGearSw_Reload(CGearSw *work);
// The event of what the player touched, or NULL
GameEvent *CGearSw_CheckEvent(CGearSw *work, BOOL enabled);
// Whether the C-Gear is leaving for the Xtransceiver
BOOL CGearSw_IsStartingTvt(CGearSw *work);
void CGearSw_StopRinging(CGearSw *work);

#endif // POKEBW2_FIELD_C_GEAR_SW_H
