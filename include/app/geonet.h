#ifndef POKEBW2_APP_GEONET_H
#define POKEBW2_APP_GEONET_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// Geonet, the globe of the Pokémon Centers (geonet.c, overlay 312), where the player registers where they live and
// sees the places of the people they have met. The process takes the game data. The names are ours

#define OVERLAY_GEONET OVERLAY_ID(312)

extern const GameProcFunctions GEONET_PROC_FUNCTIONS;

#endif // POKEBW2_APP_GEONET_H
