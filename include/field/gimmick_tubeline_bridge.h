#ifndef POKEBW2_FIELD_GIMMICK_TUBELINE_BRIDGE_H
#define POKEBW2_FIELD_GIMMICK_TUBELINE_BRIDGE_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of the Tubeline Bridge's zone 254 (gimmick 11), overlay 110: the sound of its two trains. Overlay 36's
// gimmick table calls these
void TubelineBridge_Setup(Field *field);
void TubelineBridge_End(Field *field);
void TubelineBridge_Update(Field *field);

#endif // POKEBW2_FIELD_GIMMICK_TUBELINE_BRIDGE_H
