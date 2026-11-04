#ifndef POKEBW2_BATTLE_BATTLE_SELECT_H
#define POKEBW2_BATTLE_BATTLE_SELECT_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 213 (battle_select.c, a guessed name): the Wi-Fi Club's selection of the team for a battle, which runs
// overlay 165's party list and overlay 207's summary screen, then sends the team picked to the other player. The
// names are ours

// What EventWifiClub_SetupBattleSelect fills in
typedef struct {
    Regulation *regulation;
    PokeParty *party;
    u16 *otherName;
    u8 otherGender;
    PokeParty *otherParty;
    GameData *gameData;
    u8 unk18;
    // Gets the Pokémon picked
    PokeParty *pickedParty;
    // Where each player's team picked arrives, by net ID
    PokeParty *parties[2];
    // 1 if the selection ended because the connection was lost
    u32 result;
} BattleSelectParam;

extern const GameProcFunctions BATTLE_SELECT_PROC_FUNCTIONS;

#endif // POKEBW2_BATTLE_BATTLE_SELECT_H
