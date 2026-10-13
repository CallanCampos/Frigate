#ifndef POKEBW2_FIELD_WBT_BATTLE_H
#define POKEBW2_FIELD_WBT_BATTLE_H

#include "types.h"
#include "field/wbt.h"
#include "gfl/str.h"
#include "struct_decls.h"

// wbt_battle.c, overlay 134, which the ROM names: the Pokémon World Tournament's rounds, their opponents and battles

void WbtBattle_SwapPickedPokemon(WbtSystem *sys);
void WbtBattle_ResetPicks(WbtSystem *sys);
void WbtBattle_SetPick(WbtSystem *sys, u32 index, u8 value);
void WbtBattle_RandomPick(WbtSystem *sys, u32 a1);
u8 WbtBattle_GetPick(WbtSystem *sys, u32 index);
void *WbtBattle_CreatePartyList(FieldScriptEnv *env, WbtSystem *sys);
void WbtBattle_ShowPartyList(void *work);
void WbtBattle_ClosePartyList(void *work);
void WbtBattle_FreePartyList(void *work);
void WbtBattle_MakeOpponentParty(WbtSystem *sys, GameSystem *gsys, const WbtEntrant *opponent);
// The battle of the current round, which overlay 22 starts, and its end and freeing
BtlSetup *WbtBattle_SetupBattle(WbtSystem *sys, GameSystem *gsys);
void WbtBattle_FreeBattle(WbtSystem *sys, BtlSetup *setup);
u8 WbtBattle_GetRoundRemaining(WbtSystem *sys, u16 round);
void WbtBattle_EndBattle(WbtSystem *sys, GameData *gameData, BtlSetup *setup);
// The player's opponent in the current round
WbtEntrant *WbtBattle_GetOpponent(WbtSystem *sys);
u32 WbtBattle_GetOpponentFlag(WbtSystem *sys);
u16 WbtBattle_GetOpponentObjCode(WbtSystem *sys);
// The opponent's name
void WbtBattle_GetOpponentName(WbtSystem *sys, StrBuf *strbuf);
void WbtBattle_LoadOpponentLine(WbtSystem *sys, int a1, StrBuf *strbuf);

#endif // POKEBW2_FIELD_WBT_BATTLE_H
