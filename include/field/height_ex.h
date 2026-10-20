#ifndef POKEBW2_FIELD_HEIGHT_EX_H
#define POKEBW2_FIELD_HEIGHT_EX_H

#include "types.h"
#include "struct_decls.h"

// height_ex.c, overlay 36: the event of a communication error on the field, which fieldmap_event.c starts when the
// game's communication has failed. It shows the error and returns to the field
GameEvent *func_ov036_021ba6c8(GameSystem *gsys, Field *field);

#endif // POKEBW2_FIELD_HEIGHT_EX_H
