#ifndef POKEBW2_APP_IRC_COMPAT_MENU_H
#define POKEBW2_APP_IRC_COMPAT_MENU_H

#include "types.h"
#include "gfl/proc.h"

// irc_compat_menu.c, overlay 176: the first screen of the compatibility checker (the Feeling Check of the infrared
// link): a title and two buttons, BEGIN CHECK and RANKING, with hearts floating up the background. Choosing BEGIN CHECK
// makes the infrared link with the other player. The ROM has no string for the file, so the name is a guess. Overlay
// 175 (irc_compatible.c) runs it with an IrcCompatibleParam0. The names are ours

// What the screen gives back in IrcCompatibleParam0's result
enum {
    IRC_COMPAT_MENU_RESULT_CHECK,
    IRC_COMPAT_MENU_RESULT_RANKING,
    IRC_COMPAT_MENU_RESULT_QUIT,
};

// IrcCompatibleParam0's visit
enum {
    IRC_COMPAT_VISIT_FIRST,
    IRC_COMPAT_VISIT_AGAIN,
    IRC_COMPAT_VISIT_AFTER_RESULT,
};

extern const GameProcFunctions IRC_COMPAT_MENU_PROC_FUNCTIONS;

#endif // POKEBW2_APP_IRC_COMPAT_MENU_H
