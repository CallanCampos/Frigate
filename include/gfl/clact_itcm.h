#ifndef POKEBW2_GFL_CLACT_ITCM_H
#define POKEBW2_GFL_CLACT_ITCM_H

#include "types.h"
#include "gfl/clact.h"

// The renderers of a unit's actors, which clact.c picks by the unit's visibleAnimOnly and sharedParams. mainRenderer
// is swan's name
void mainRenderer(ClActUnit *unit);
void visibleAnimRenderer(ClActUnit *unit);
void sharedParamsRenderer(ClActUnit *unit);
void sharedParamsVisibleAnimRenderer(ClActUnit *unit);

#endif // POKEBW2_GFL_CLACT_ITCM_H
