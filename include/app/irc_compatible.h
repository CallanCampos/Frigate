#ifndef POKEBW2_APP_IRC_COMPATIBLE_H
#define POKEBW2_APP_IRC_COMPATIBLE_H

#include "types.h"
#include "app/compatible_irc_sys.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 175's irc_compatible.c: the compatibility checker (the Link Compatibility check between two players over the
// infrared port), which runs the screens of overlays 176 to 180 in turn through a process manager of its own. Names are
// descriptive guesses

// What the event gives the checker's proc
typedef struct {
    GameSystem *gsys;
} IrcCompatibleParam;

extern const GameProcFunctions IRC_COMPATIBLE_PROC_FUNCTIONS;

// What each of the screens gets, and gives back
typedef struct {
    GameSystem *gsys;
    CompatibleIrcSys *sys;
    // The screen's first or later visit: 0 the first, 1 afterwards and 2 after a visit to the last screen
    int visit;
    // What the screen chose
    int result;
    CompatibleIrcProfile *profile;
} IrcCompatibleParam0;

typedef struct {
    GameSystem *gsys;
    CompatibleIrcSys *sys;
    CompatibleIrcProfile *profile;
    int result;
    u8 score;
    // How many times the two players had to retry, together
    u8 retries;
    // Whether the other player is new to this player's trainer log
    u8 isNew;
} IrcCompatibleParam1;

typedef struct {
    GameSystem *gsys;
    CompatibleIrcSys *sys;
    CompatibleIrcProfile *profile;
    int result;
    u8 score;
    // How far apart the two players' extra taps are
    u32 extraTapDiff;
} IrcCompatibleParam2;

typedef struct {
    GameSystem *gsys;
    CompatibleIrcSys *sys;
    CompatibleIrcProfile *profile;
    u8 result;
} IrcCompatibleParam3;

typedef struct {
    GameSystem *gsys;
} IrcCompatibleParam4;

// The screens' procs, in overlays 176 to 180. Their owners' headers declare them once the overlays are done
extern const GameProcFunctions IRC_COMPAT_RESULT_PROC_FUNCTIONS;

#endif // POKEBW2_APP_IRC_COMPATIBLE_H
