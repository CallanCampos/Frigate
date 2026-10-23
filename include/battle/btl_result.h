#ifndef POKEBW2_BATTLE_BTL_RESULT_H
#define POKEBW2_BATTLE_BTL_RESULT_H

#include "types.h"

// How a battle ended for the player, swan's BattleResult (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), and
// BTL_RESULT_NOT_SET (ours), the main module's result until one is set
enum {
    BTL_RESULT_DEFEAT,
    BTL_RESULT_VICTORY,
    BTL_RESULT_DRAW,
    BTL_RESULT_FLEE,
    BTL_RESULT_FLEE_FOE,
    BTL_RESULT_CAPTURE,
    BTL_RESULT_6,
    BTL_RESULT_NOT_SET,
};

BOOL IsBattleResultDefeat(u32 result, u32 battleType);
u32 GetWildBattleResultByCombined(u32 result);

#endif // POKEBW2_BATTLE_BTL_RESULT_H
