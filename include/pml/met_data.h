#ifndef POKEBW2_PML_MET_DATA_H
#define POKEBW2_PML_MET_DATA_H

#include "types.h"
#include "constants/met_locations.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Where, when and by whom a Pokémon was met, and the special event Pokémon. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except PokeParty_IsSpecialTransfer,
// PokeParty_SetSpecialTransferUsed, PML_PkmSetSpecialTransferUsed, MetLocation_GetNameFile and
// MetLocation_GetNameIndex, which are ours

// The message files of the place names, as MetLocation_GetNameFile returns them
#define PLACE_FILE_UNOVA 0x6d
#define PLACE_FILE_EVENT 0x6e
#define PLACE_FILE_EXTERNAL 0x6f
#define PLACE_FILE_SPECIAL 0x70

// Records how and where a party Pokémon was met, by kind: 0 caught or received, as met at placeName with the player
// as its Trainer; 1 an in-game trade; 2 hatched, an egg of the player's keeping where it was received as its egg's
// place; 5 an egg received at placeName; 6 a link trade, at placeName; 7 one of N's Pokémon. 3 and 4 change nothing
void PokeParty_SetupMetData(PartyPkm *pkm, u32 kind, PlayerInfo *playerInfo, u16 placeName, HeapID heapId);
void PML_UtilSetupMetData(BoxPkm *pkm, u32 kind, PlayerInfo *playerInfo, u32 placeName, HeapID heapId);
// Sets the date the Pokémon was met to today
void setMetCurrentDateTime(BoxPkm *pkm);
// Marks the Pokémon as met in a fateful encounter, at the location and on the date
void setFatefulEncounterPkmData(BoxPkm *pkm, u16 location, u32 year, u32 month, u32 day);
// Records a Pokémon as met through the Dream Radar
void setDreamRadarPokeMetInfo(BoxPkm *pkm);
// Whether the Pokémon came by one of the kinds of special event for the player: 0 to 3 the Celebi and the beasts
// whose event hasn't run or has, 4 Keldeo, 5 Meloetta, 6 Genesect, 7 Shaymin, 8 Landorus from the Dream Radar
BOOL PokeParty_IsSpecialTransfer(PartyPkm *pkm, u32 kind, PlayerInfo *playerInfo);
BOOL special_transfers(BoxPkm *pkm, u32 kind, PlayerInfo *playerInfo);
// Marks the event of an event Celebi or beast as run
void PokeParty_SetSpecialTransferUsed(PartyPkm *pkm, u32 kind, PlayerInfo *playerInfo);
// The message file of a location's name, PLACE_FILE_*, and the name's index in it
u32 MetLocation_GetNameFile(u32 location);
u32 MetLocation_GetNameIndex(u32 location);

#endif // POKEBW2_PML_MET_DATA_H
