#ifndef POKEBW2_APP_IRC_RANKING_H
#define POKEBW2_APP_IRC_RANKING_H

#include "types.h"
#include "gfl/proc.h"

// irc_ranking.c, overlay 180: the compatibility checker's ranking (the "Feeling Ranking" of the infrared link), the
// last of its screens. It lists the trainers of the player's trainer log from the best compatibility value down, with
// their rank, name, value, number of meetings and the Pokémon they led with, on a list that scrolls with the stylus,
// the Control Pad or a fling. The ROM names the file. Overlay 175 (irc_compatible.c) runs it with an
// IrcCompatibleParam4. The names are ours

extern const GameProcFunctions IRC_RANKING_PROC_FUNCTIONS;

#endif // POKEBW2_APP_IRC_RANKING_H
