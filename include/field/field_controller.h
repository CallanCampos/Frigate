#ifndef POKEBW2_FIELD_FIELD_CONTROLLER_H
#define POKEBW2_FIELD_FIELD_CONTROLLER_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The kinds of controller, a FieldmapCtrlVTable's typeId
enum {
    FIELDMAP_CTRL_TYPE_GRID,
    FIELDMAP_CTRL_TYPE_NOGRID,
    FIELDMAP_CTRL_TYPE_HYBRID,
};

// What a kind of map's controller does: its type, making, updating and freeing it, and where its player is
struct FieldmapCtrlVTable {
    u32 typeId;
    void (*create)(Field *field, VecFx32 *pos, u16 dir);
    void (*update)(Field *field, VecFx32 *pos);
    void (*free)(Field *field);
    VecFx32 *(*getPos)(Field *field);
};

// The three kinds of controller, which fieldmap.c's table of map kinds lists: fieldmap_ctrl_grid.c's, and
// fieldmap_ctrl_nogrid_work.c's for the rail and the hybrid maps
extern const FieldmapCtrlVTable FIELDMAP_CTRL_GRID;
extern const FieldmapCtrlVTable FIELDMAP_CTRL_NOGRID;
extern const FieldmapCtrlVTable FIELDMAP_CTRL_HYBRID;

// The field's controller: a FieldmapCtrlGrid, FieldmapCtrlNoGrid or FieldmapCtrlHybrid, by the kind of map
void *Field_GetController(Field *field);
void Field_SetController(Field *field, void *controller);
u32 Field_GetControllerTypeID(Field *field);
// Sets whether the grid map's controller stops updating
void FieldmapCtrlGrid_SetUpdateDisable(Field *field, BOOL disable);

#endif // POKEBW2_FIELD_FIELD_CONTROLLER_H
