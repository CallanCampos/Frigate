#ifndef POKEBW2_SAVE_DREAM_WORLD_H
#define POKEBW2_SAVE_DREAM_WORLD_H

#include "types.h"
#include "pml/poke_party.h"
#include "struct_decls.h"

#define DREAM_WORLD_ITEM_MAX 20
#define DREAM_WORLD_ITEM_COUNT_MAX 20
#define DREAM_WORLD_RESULT_MAX 5
// The chosen result entry when there is none: it holds an index into results otherwise
#define DREAM_WORLD_RESULT_INDEX_NONE 0x7f

#define DREAM_WORLD_RESULT_WORDS 13
// The first word of a result entry until a result comes
#define DREAM_WORLD_RESULT_NONE 0x7e
// Block 0x23 (swan's getDreamWorldStuffAddress): the Pokémon sent to the Dream World, the items and the result
// that came back, and the Game Sync's state
struct DreamWorldSave {
    // The date of the last Game Sync, packed as year, month, day and weekday from the top byte down
    u32 syncDate;
    // The ID of the last result taken from the Dream World, -1 when there is none
    u32 lastResultId;
    PartyPkm pkm;
    u16 items[DREAM_WORLD_ITEM_MAX];
    u8 itemCounts[DREAM_WORLD_ITEM_MAX];
    // The entries of the Dream World's result: a word that is 0x7e until a result comes, then a fixed string
    u16 results[DREAM_WORLD_RESULT_MAX][DREAM_WORLD_RESULT_WORDS];
    u8 unk1A2;
    u8 asleep;
    // From the Dream World's result
    u8 unk1A4;
    // Set once the Game Sync ID was shown
    u8 idShown;
    // The index of the chosen result entry, DREAM_WORLD_RESULT_INDEX_NONE for none, and a flag with it
    u8 resultIndex : 7;
    u8 unk1A6_7 : 1;
    u8 unk1A7;
    u8 unk1A8;
    u8 unk1A9;
    u8 padding1AA[2];
}; // size: 0x1ac

// The size of the Dream World save and its init
u32 DreamWorldSave_GetSize(void);
void DreamWorldSave_Init(DreamWorldSave *dreamWorld);
// The Pokémon sent to the Dream World, and whether it is there
PartyPkm *DreamWorldSave_GetPkm(DreamWorldSave *dreamWorld);
void DreamWorldSave_SetPkm(DreamWorldSave *dreamWorld, PartyPkm *pkm);
// Whether a species' bit is set in a list of species flags
BOOL DreamWorldSave_TestSpeciesFlag(const u8 *flags, int species);
void func_020099d8(DreamWorldSave *dreamWorld, u8 value);
u8 func_020099e0(DreamWorldSave *dreamWorld);
void DreamWorldSave_SetIDShown(DreamWorldSave *dreamWorld, u8 value);
u8 DreamWorldSave_GetIDShown(DreamWorldSave *dreamWorld);
void DreamWorldSave_SetPokemonAsleep(DreamWorldSave *dreamWorld, u8 asleep);
BOOL DreamWorldSave_IsPokemonAsleep(DreamWorldSave *dreamWorld);
// The items sent from the Dream World and their counts, 0 when out of range
u16 DreamWorldSave_GetItem(DreamWorldSave *dreamWorld, int index);
u8 DreamWorldSave_GetItemCount(DreamWorldSave *dreamWorld, int index);
void DreamWorldSave_SetItem(DreamWorldSave *dreamWorld, int index, u16 item, u8 count);
void DreamWorldSave_ClearItem(DreamWorldSave *dreamWorld, int index);
// How many items the Dream World sent
s32 DreamWorldSave_CountItems(DreamWorldSave *dreamWorld);
// The entries of the Dream World's result, NULL when the index is out of range
u16 *DreamWorldSave_GetResult(DreamWorldSave *dreamWorld, int index);
void DreamWorldSave_SetResult(DreamWorldSave *dreamWorld, int index, const void *entry);
u32 DreamWorldSave_GetSyncDate(DreamWorldSave *dreamWorld);
void DreamWorldSave_SetSyncDate(DreamWorldSave *dreamWorld, u32 date);
u32 DreamWorldSave_GetLastResultID(DreamWorldSave *dreamWorld);
void DreamWorldSave_SetLastResultID(DreamWorldSave *dreamWorld, u32 id);
// The chosen result entry, DREAM_WORLD_RESULT_INDEX_NONE when it is out of range, and the flag above it
u32 DreamWorldSave_GetResultIndex(DreamWorldSave *dreamWorld);
void DreamWorldSave_SetResultIndex(DreamWorldSave *dreamWorld, int value);
u32 func_02009b20(DreamWorldSave *dreamWorld);
void func_02009b30(DreamWorldSave *dreamWorld, u32 value);
// The Dream Radar's values in the save: 0 whether it was read, 1 the seed of the data, 2 the Pokémon received
u32 GetDreamRadarFlag(DreamRadarSave *save, u32 flag);
void SetDreamRadarFlag(DreamRadarSave *save, u32 flag, u32 value);
// The data that Pokémon Dream Radar writes to the card, read into a buffer of DreamRadar_AllocBuffer's
void *DreamRadar_AllocBuffer(HeapID heapId);
void DreamRadar_FreeBuffer(void *buffer);
void ReadDreamRadarSaveData(void *buffer);
CygnusSaveData *DreamRadar_GetSaveData(void *buffer);
// Writes the data back to the card
void DreamRadar_WriteSaveData(void);
// Checks the data and decodes its 32 words with the seed. Returns 2 when the data is valid
u32 CygnusSaveData_Load(CygnusSaveData *cygnus, u32 seed, u32 *words);
// Marks the data read with a new seed and the Pokémon received
void CygnusSaveData_Store(CygnusSaveData *data, u32 seed, u32 received);
// The Pokémon that a word of the data names, if any: its bit in the Pokémon received, its species, form and sex
BOOL DreamRadar_IsPokemonWord(u32 word);
u32 DreamRadar_GetPokemonBit(u32 word);
void DreamRadar_GetPokemonByWord(u32 word, u16 *species, u8 *form, u8 *sex);

#endif // POKEBW2_SAVE_DREAM_WORLD_H
