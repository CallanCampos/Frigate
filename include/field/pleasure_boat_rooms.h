#ifndef POKEBW2_FIELD_PLEASURE_BOAT_ROOMS_H
#define POKEBW2_FIELD_PLEASURE_BOAT_ROOMS_H

#include "types.h"
#include "struct_decls.h"

// Overlay 89: who is in the Royal Unova's rooms, picked when the cruise is set up. The ROM has no string for the file;
// the name is a guess from what it does. PleasureBoat_Create (overlay 36) loads the overlay and calls this

// Places the Trainers (as many as the day of the week allows), the pairs and the other people in the 15 rooms at random.
// flag picks one of two tables of Trainers
void PleasureBoat_SetUpRooms(PleasureBoat *boat, BOOL flag);

#endif // POKEBW2_FIELD_PLEASURE_BOAT_ROOMS_H
