#ifndef POKEBW2_SAVE_DREAM_WORLD_H
#define POKEBW2_SAVE_DREAM_WORLD_H

#include "types.h"
#include "struct_decls.h"
#include "pml/poke_party.h"

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
u32 func_02009930(void);
void func_02009938(DreamWorldSave *dreamWorld);
// The Pokémon sent to the Dream World, and whether it is there
PartyPkm *func_02009998(DreamWorldSave *dreamWorld);
void func_0200999c(DreamWorldSave *dreamWorld, PartyPkm *pkm);
// Whether a species' bit is set in a list of species flags
BOOL func_020099b4(const u8 *flags, int species);
void func_020099d8(DreamWorldSave *dreamWorld, u8 value);
u8 func_020099e0(DreamWorldSave *dreamWorld);
void func_020099e8(DreamWorldSave *dreamWorld, u8 value);
u8 func_020099f4(DreamWorldSave *dreamWorld);
void func_02009a00(DreamWorldSave *dreamWorld, u8 asleep);
BOOL DreamWorldSave_IsPokemonAsleep(DreamWorldSave *dreamWorld);
// The items sent from the Dream World and their counts, 0 when out of range
u16 func_02009a18(DreamWorldSave *dreamWorld, int index);
u8 func_02009a38(DreamWorldSave *dreamWorld, int index);
void func_02009a50(DreamWorldSave *dreamWorld, int index, u16 item, u8 count);
void func_02009a6c(DreamWorldSave *dreamWorld, int index);
// How many items the Dream World sent
s32 func_02009a78(DreamWorldSave *dreamWorld);
// The entries of the Dream World's result, NULL when the index is out of range
u16 *func_02009a98(DreamWorldSave *dreamWorld, int index);
void func_02009ab0(DreamWorldSave *dreamWorld, int index, const void *entry);
u32 func_02009ad0(DreamWorldSave *dreamWorld);
void func_02009ad4(DreamWorldSave *dreamWorld, u32 date);
u32 func_02009ad8(DreamWorldSave *dreamWorld);
void func_02009adc(DreamWorldSave *dreamWorld, u32 id);
// The chosen result entry, DREAM_WORLD_RESULT_INDEX_NONE when it is out of range, and the flag above it
u32 func_02009ae0(DreamWorldSave *dreamWorld);
void func_02009af8(DreamWorldSave *dreamWorld, int value);
u32 func_02009b20(DreamWorldSave *dreamWorld);
void func_02009b30(DreamWorldSave *dreamWorld, u32 value);
// The Dream Radar's values in the save: 0 whether it was read, 1 the seed of the data, 2 the Pokémon received
u32 GetDreamRadarFlag(DreamRadarSave *save, u32 flag);
void SetDreamRadarFlag(DreamRadarSave *save, u32 flag, u32 value);
// The data that Pokémon Dream Radar writes to the card, read into a buffer of func_02011510's
void *func_02011510(HeapID heapId);
void func_02011528(void *buffer);
void ReadDreamRadarSaveData(void *buffer);
void *func_02011588(void *buffer);
// Writes the data back to the card
void func_02011544(void);
// Checks the data and decodes its 32 words with the seed. Returns 2 when the data is valid
u32 func_02011400(void *data, u32 seed, u32 *words);
// Marks the data read with a new seed and the Pokémon received
void func_020113c4(void *data, u32 seed, u32 received);
// The Pokémon that a word of the data names, if any: its bit in the Pokémon received, its species, form and sex
BOOL func_020115fc(u32 word);
u32 func_0201167c(u32 word);
void func_02011624(u32 word, u16 *species, u8 *form, u8 *sex);

#endif // POKEBW2_SAVE_DREAM_WORLD_H
