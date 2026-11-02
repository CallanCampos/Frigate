#ifndef POKEBW2_BATTLE_BATTLE_REC_TOOL_H
#define POKEBW2_BATTLE_BATTLE_REC_TOOL_H

#include "types.h"
#include "gfl/heap.h"
#include "pml/poke_party.h"
#include "save/battle_rec.h"
#include "save/pokewood.h"
#include "struct_decls.h"

// battle_rec_tool.c (overlay 273, the name its allocations pass): converts a battle's setup to the form a battle video
// or Pokéstar Studios' block keeps, and back

// Makes a party Pokémon of a recorded one, which it encrypts
void BattleRecTool_LoadPkm(BattleRecPkm *rec, PartyPkm *pkm);
// Store a battle's setup in the loaded video, or in Pokéstar Studios' block when setup->studioMode is 1 or 2, and load
// it from either, by mode
void BattleRecTool_StoreSetup(BtlSetup *setup);
void BattleRecTool_LoadSetup(BtlSetup *setup, u32 mode, HeapID heapId);

#endif // POKEBW2_BATTLE_BATTLE_REC_TOOL_H
