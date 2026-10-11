#ifndef POKEBW2_APP_IRC_RHYTHM_CHECK_H
#define POKEBW2_APP_IRC_RHYTHM_CHECK_H

#include "types.h"
#include "gfl/proc.h"

// irc_rhythm_check.c, overlay 178: a screen of the compatibility checker (the Feeling Check of the infrared link) that
// the ROM calls "Rhythm Check": both players tap the touch screen ten times in a rhythm of their own, a countdown
// starts it, sparkles fly from each tap, and how close the two rhythms are gives a score. The ROM has no string for
// the file, so the name is a guess. Overlay 175 (irc_compatible.c) runs it with an IrcCompatibleParam2. The names are
// ours

// What the screen gives back in IrcCompatibleParam2's result
enum {
    IRC_RHYTHM_CHECK_RESULT_DONE,
    IRC_RHYTHM_CHECK_RESULT_CANCELLED,
    IRC_RHYTHM_CHECK_RESULT_INTERRUPTED,
};

extern const GameProcFunctions IRC_RHYTHM_CHECK_PROC_FUNCTIONS;
// A constant that nothing refers to
extern const u32 IRC_RHYTHM_CHECK_UNUSED_400;

#endif // POKEBW2_APP_IRC_RHYTHM_CHECK_H
