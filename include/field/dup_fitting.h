#ifndef POKEBW2_FIELD_DUP_FITTING_H
#define POKEBW2_FIELD_DUP_FITTING_H

// Overlay 208's dup_fitting.c (named after its string, which the file guessed from "dress up" and "fitting"): the
// musical's dressing room, where the player puts props on the Pokémon before the show. Overlay 12's
// musical_dressup_sys.c runs it, with a copy of its parameter

#include "types.h"
#include "field/musical_dressup_sys.h"
#include "gfl/heap.h"

// The dressing room's work, whose layout only dup_fitting.c needs
typedef struct DupFittingWork DupFittingWork;

DupFittingWork *DupFitting_Create(MusicalDressUpParam *param, HeapID heapId);
void DupFitting_Free(DupFittingWork *work);
// Runs a frame, and returns 2 once the player is done, else 0
u32 DupFitting_Main(DupFittingWork *work);

#endif // POKEBW2_FIELD_DUP_FITTING_H
