#ifndef POKEBW2_APP_TH_AWARD_H
#define POKEBW2_APP_TH_AWARD_H

// The Trial House's results screen, overlay 313: the rank of a challenge's points, the Pokémon of its team and the
// player's name on the screens, with a jingle for the rank. The ROM names th_award_graphic.c; th_award.c, the file
// before it, is a guess. Overlay 12's event_trial_house.c runs it

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

#define OVERLAY_TH_AWARD OVERLAY_ID(313)

typedef struct {
    u8 gender;
    TrialHouseSave *save;
    // Whether the result is the downloaded challenge's rather than the game's
    BOOL downloaded;
    // Whether a jingle plays for the rank
    BOOL playJingle;
} ThAwardParam;

extern const GameProcFunctions TH_AWARD_PROC_FUNCTIONS;

#endif // POKEBW2_APP_TH_AWARD_H
