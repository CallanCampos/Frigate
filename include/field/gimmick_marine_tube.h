#ifndef POKEBW2_FIELD_GIMMICK_MARINE_TUBE_H
#define POKEBW2_FIELD_GIMMICK_MARINE_TUBE_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of the Marine Tube's zone 464 (gimmick 50), overlay 111: thirteen models of the tube and what is seen
// through it, whose animations loop and whose frames the save keeps. The name is a guess: the overlay has no string.
// Overlay 36's gimmick table calls these
void MarineTube_Init(Field *field);
void MarineTube_End(Field *field);
void MarineTube_Update(Field *field);

#endif // POKEBW2_FIELD_GIMMICK_MARINE_TUBE_H
