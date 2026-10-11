#ifndef POKEBW2_APP_CG_HELP_H
#define POKEBW2_APP_CG_HELP_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The C-Gear's help (cg_help.c, overlay 200): 15 pages of text with a picture, which the touch screen's buttons or
// the keys page through. The C-Gear's overlay (79) starts it

// Without a parameter the process makes its own, with a player of the gender that the R button held at the start picks
typedef struct {
    PlayerInfo *playerInfo;
    GameSystem *gsys;
} CGHelpParam;

extern GameProcFunctions CG_HELP_PROC_FUNCTIONS;

#endif // POKEBW2_APP_CG_HELP_H
