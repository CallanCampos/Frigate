#ifndef POKEBW2_APP_IRC_SYNC_CHECK_H
#define POKEBW2_APP_IRC_SYNC_CHECK_H

#include "types.h"
#include "gfl/proc.h"

// irc_sync_check.c, overlay 177: a screen of the compatibility checker (the Feeling Check of the infrared link) that
// the ROM calls "Sync Check": both players put a thumb on an outline on the touch screen and keep it there, a pair of
// 3D thumbs follows the stylus on the other screen, and how close the two traces are gives a score. The ROM has no
// string for the file, so the name is a guess. Overlay 175 (irc_compatible.c) runs it with an IrcCompatibleParam1. The
// names are ours

// What the screen gives back in IrcCompatibleParam1's result
enum {
    IRC_SYNC_CHECK_RESULT_DONE,
    IRC_SYNC_CHECK_RESULT_CANCELLED,
    IRC_SYNC_CHECK_RESULT_INTERRUPTED,
};

extern const GameProcFunctions IRC_SYNC_CHECK_PROC_FUNCTIONS;

#endif // POKEBW2_APP_IRC_SYNC_CHECK_H
