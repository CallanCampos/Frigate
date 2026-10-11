#ifndef POKEBW2_FIELD_WBT_MAKEMATCH_H
#define POKEBW2_FIELD_WBT_MAKEMATCH_H

#include "types.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "struct_decls.h"

// wbt_makematch.c, overlay 135, which the ROM names: the Pokémon World Tournament's entrance (zone 192), which
// script plugin 6 drives: the entry and the party the player enters with

void WbtMakeMatch_Make(WbtSystem *sys, GameData *gameData);
// The Battle Points for winning the tournament
u8 WbtMakeMatch_GetBattlePoints(WbtSystem *sys);
u32 WbtMakeMatch_CountDownloads(WbtSystem *sys, GameData *gameData);
void WbtMakeMatch_LoadDownload(WbtSystem *sys, GameData *gameData);
// Takes the picks from overlay 22's party screen
void WbtMakeMatch_ReadPick(WbtSystem *sys, PokeListParam *param);
// The party the player enters the tournament with
PokeParty *WbtMakeMatch_GetParty(WbtSystem *sys, GameData *gameData);
// Copies the Battle Box's party, if it has one, to the system
void WbtMakeMatch_LoadBattleBoxParty(WbtSystem *sys, GameData *gameData);
// Fill in overlay 22's party screen and summary screen for picking the Pokémon to enter
void WbtMakeMatch_SetupPokeList(WbtSystem *sys, GameData *gameData, PokeListParam *param);
void WbtMakeMatch_SetupPStatus(WbtSystem *sys, GameData *gameData, PStatusParam *param);

#endif // POKEBW2_FIELD_WBT_MAKEMATCH_H
