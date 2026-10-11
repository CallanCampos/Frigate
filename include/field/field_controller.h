#ifndef POKEBW2_FIELD_FIELD_CONTROLLER_H
#define POKEBW2_FIELD_FIELD_CONTROLLER_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// What a kind of map's controller does: its type, and making, updating and freeing it
struct FieldmapCtrlVTable {
    u32 typeId;
    void (*create)(Field *field, VecFx32 *pos, u16 dir);
    void (*update)(Field *field, VecFx32 *pos);
    void (*free)(Field *field);
};

void *Field_GetController(Field *field);
void Field_SetController(Field *field, void *controller);
u32 Field_GetControllerTypeID(Field *field);
// Sets whether the grid map's controller stops updating
void FieldmapCtrlGrid_SetUpdateDisable(Field *field, BOOL disable);

#endif // POKEBW2_FIELD_FIELD_CONTROLLER_H
