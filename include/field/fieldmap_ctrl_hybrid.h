#ifndef POKEBW2_FIELD_FIELDMAP_CTRL_HYBRID_H
#define POKEBW2_FIELD_FIELDMAP_CTRL_HYBRID_H

// The controller of maps with both grid and rail movement, overlay 36's fieldmap_ctrl_hybrid.c. Function names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"

typedef struct {
    // The controller type in use, 0 for the grid and 1 for a rail, 2 until the first is set
    u32 activeType;
    FieldPlayer *player;
    // On a rail, what FieldPlayer_IsNogridForcedMove last returned: then the player keeps moving its way
    u32 forcedMove;
} FieldmapCtrlHybrid;

u32 FieldmapCtrlHybrid_GetActiveTypeID(FieldmapCtrlHybrid *controller);
void FieldmapCtrlHybrid_Create(Field *field, VecFx32 *pos, u16 dir);
void FieldmapCtrlHybrid_Free(Field *field);
void FieldmapCtrlHybrid_Update(Field *field, VecFx32 *pos);
VecFx32 *FieldmapCtrlHybrid_GetPos(Field *field);
// Moves the player to the other kind of movement where it stands, if it can
void FieldmapCtrlHybrid_ReplacePlayer(FieldmapCtrlHybrid *controller, Field *field);

#endif // POKEBW2_FIELD_FIELDMAP_CTRL_HYBRID_H
