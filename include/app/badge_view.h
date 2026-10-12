#ifndef POKEBW2_APP_BADGE_VIEW_H
#define POKEBW2_APP_BADGE_VIEW_H

#include "types.h"
#include "gfl/proc.h"

// Overlay 186's badge_view.c: the Gym Badge case that the trainer card opens. Touching a badge shows when it was
// obtained and spins its model, and touching a Gym Leader shows what is known of the leader. Its param is the trainer
// card's TrainerCardParam (field/trcard_sys.h)

extern const GameProcFunctions BADGE_VIEW_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BADGE_VIEW_H
