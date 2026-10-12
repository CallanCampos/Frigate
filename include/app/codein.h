#ifndef POKEBW2_APP_CODEIN_H
#define POKEBW2_APP_CODEIN_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// The code input screen (overlay 205), where the player types a number such as a Friend Code. Its parameter is the
// CodeInParam of system/codein_param.h. The names are ours
#define OVERLAY_CODEIN OVERLAY_ID(205)

extern const GameProcFunctions CODEIN_PROC_FUNCTIONS;

#endif // POKEBW2_APP_CODEIN_H
