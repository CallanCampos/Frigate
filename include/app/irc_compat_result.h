#ifndef POKEBW2_APP_IRC_COMPAT_RESULT_H
#define POKEBW2_APP_IRC_COMPAT_RESULT_H

#include "types.h"
#include "gfl/proc.h"

// irc_compat_result.c, overlay 179: the compatibility checker's result screen (the SEQ_ME_AISHOU_* jingles, "aishou",
// are compatibility). It counts a score from 0 to 100 up in three digits, flashes the background tier by tier, plays a
// jingle for the tier, gives the Medal Rally medal for a high one and, the first time the other player is logged
// today, Sweet Hearts. The ROM has no string for the file, so the name is a guess. Overlay 175 (irc_compatible.c) runs
// it with an IrcCompatibleParam3. The names are ours

extern const GameProcFunctions IRC_COMPAT_RESULT_PROC_FUNCTIONS;

#endif // POKEBW2_APP_IRC_COMPAT_RESULT_H
