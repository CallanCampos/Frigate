#ifndef POKEBW2_FIELD_FIELD_CONTROLLER_H
#define POKEBW2_FIELD_FIELD_CONTROLLER_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/fieldmap_ctrl_hybrid.h"
#include "nitro/fx.h"
#include "struct_decls.h"

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

// The grid maps' controller (fieldmap_ctrl_grid.c)
typedef struct {
    u16 heapId;
    Field *field;
    u32 unk08;
    BOOL updateDisable;
} FieldmapCtrlGrid;

// The rail maps' controller (fieldmap_ctrl_nogrid_work.c)
typedef struct {
    FieldPlayer *player;
    u32 unk04;
} FieldmapCtrlNoGrid;

// The controller the field keeps, which its kind's create makes and the rest of its kind's functions use as their own
typedef union {
    FieldmapCtrlGrid grid;
    FieldmapCtrlNoGrid noGrid;
    FieldmapCtrlHybrid hybrid;
} FieldmapCtrl;

FieldmapCtrl *Field_GetController(Field *field);
void Field_SetController(Field *field, FieldmapCtrl *controller);
u32 Field_GetControllerTypeID(Field *field);
// Sets whether the grid map's controller stops updating
void FieldmapCtrlGrid_SetUpdateDisable(Field *field, BOOL disable);

#endif // POKEBW2_FIELD_FIELD_CONTROLLER_H
