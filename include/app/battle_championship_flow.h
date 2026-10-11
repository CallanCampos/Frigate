#ifndef POKEBW2_APP_BATTLE_CHAMPIONSHIP_FLOW_H
#define POKEBW2_APP_BATTLE_CHAMPIONSHIP_FLOW_H

#include "types.h"
#include "app/livebattlematch_flow.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 261's battle_championship_flow.c (named by its embedded string): the process that overlay 290's Wi-Fi and
// live battle competitions (wifibattlematch_sys.c) run for the battle competition menu of Entree Forest, the Battle
// Points reward, and the live competition (livebattlematch_flow.c). Swan has none of these; the names are ours

// What overlay 290 gives the process. The last three are the live competition's flow's, but event_wifibattlematch.c's
// WifiBattleMatchParam ends at its fourth word
typedef struct {
    GameData *gameData;
    // A BATTLE_CHAMPIONSHIP_FLOW_MODE_*
    u32 mode;
    // A BATTLE_CHAMPIONSHIP_FLOW_RESULT_*
    u32 result;
    LiveBattleMatchPlayerData *playerData;
    LiveBattleMatchPlayerData *partnerData;
    LiveBattleMatchResult *matchResult;
} BattleChampionshipFlowParam;

// What the process starts with: the menu, or the reward, or one of the ways into the live competition (the modes
// of livebattlematch_flow.c, in the order 0, 2, 3, 1)
enum {
    BATTLE_CHAMPIONSHIP_FLOW_MODE_MENU,
    BATTLE_CHAMPIONSHIP_FLOW_MODE_REWARD,
    BATTLE_CHAMPIONSHIP_FLOW_MODE_MENU_2,
    BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_JOIN,
    BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_AFTER_MATCH,
    BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_QUIT,
    BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_MENU,
};

// What the process ends with: 0 if the player left the menu and 1 if they joined the Wi-Fi competition. The live
// competition's flow gives 2, 3 or 4, which the process passes on
enum {
    BATTLE_CHAMPIONSHIP_FLOW_RESULT_EXIT,
    BATTLE_CHAMPIONSHIP_FLOW_RESULT_WIFI,
};

extern const GameProcFunctions BATTLE_CHAMPIONSHIP_FLOW_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_CHAMPIONSHIP_FLOW_H
