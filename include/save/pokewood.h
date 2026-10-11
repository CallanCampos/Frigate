#ifndef POKEBW2_SAVE_POKEWOOD_H
#define POKEBW2_SAVE_POKEWOOD_H

#include "types.h"
#include "gfl/heap.h"
#include "save/battle_rec.h"
#include "struct_decls.h"

// Pokéstar Studios, which the game calls Pokewood. Its progress is save block 0x47 (PokewoodSave), and the movies
// downloaded into it are extra save data, which the Pokewood block is loaded from

// The number of movies. Each of the save's flag lists has a flag for each
#define POKEWOOD_MOVIE_COUNT 40

// A movie's best result, which the save keeps for each movie
typedef struct {
    s32 score;
    // func_02021204 of each Pokémon of the cast: its species, form and sex
    u16 cast[6];
    u16 unk10;
    // A bit for each Pokémon of the cast whose Pokéstar fame is at level 2 or more
    u8 famousCast;
    u8 unk13;
} PokewoodRecord;

// BtlSetup fields that only Pokéstar Studios' block keeps
typedef struct {
    u32 unkDF;
    u32 unk124;
    u8 unk110[0x14];
} PokewoodBattleInfo;

// The setup of a Pokéstar Studios battle, which getPokewoodBlock returns.
typedef struct {
    BattleRecSetup setup;
    PokewoodBattleInfo info;
    u32 dataSize;
    u8 data[0x164];
    BattleRecParty party;
    BattleRecClient client;
    u16 trainerClass;
    u8 unk128;
    u8 unkE0;
    u8 unk4A4[8];
    u32 unk130;
} PokewoodBlock;

PokewoodSave *func_02011040(GameData *gameData);
// Flag lists 0 to 6 of the save, with a flag for each movie
BOOL func_020110ac(PokewoodSave *save, u32 list, u32 movie);
void func_020110d4(PokewoodSave *save, u32 list, u32 movie);
// func_020110d4 for list 1
void func_02011124(PokewoodSave *save, u32 movie);
// Keeps record as the movie's best if it scored at least as much
void func_02011130(PokewoodSave *save, u32 movie, const PokewoodRecord *record);
PokewoodRecord *func_02011194(PokewoodSave *save, u32 movie);
// A byte for each of 8 slots
u8 func_020111a0(PokewoodSave *save, u32 slot);
void func_020111a8(PokewoodSave *save, u32 slot, u8 value);
// A value from 0 to 99, which func_020111d0 adds to and func_020111ec sorts into levels 0 to 4
s16 func_020111b0(PokewoodSave *save);
void func_020111d0(PokewoodSave *save, int amount);
int func_020111ec(int value);
// A bit for each of those levels
BOOL func_0201122c(PokewoodSave *save, u32 bit);
void func_02011240(PokewoodSave *save, u32 bit, u32 value);
// The series (plus 1, 0 for none) of the last four movies filmed, from the newest (0) to the oldest (3), which
// func_0201127c pushes a2 onto
u8 func_02011270(PokewoodSave *save, u32 index);
void func_0201127c(PokewoodSave *save, u32 a1, u32 a2, u8 *a3);
// How many movies have their flag set in a list
u32 func_020112d8(PokewoodSave *save, u32 list);

// The Pokewood block is the buffer that a downloaded movie is loaded into
u32 getSizeOfPokewoodBlock(void);
void allocatePokewoodBlk(HeapID heapId);
void freeAndClearPokewoodBlk(void);
void setPokewoodBlk(const void *src);
// Loads the downloaded movie of a slot (0 to 7): 1 when it loaded and its checksum is right, 2 when the checksum is
// wrong, 0 when the slot is empty and 4 when it failed
u32 func_02010644(SaveControl *save, HeapID heapId, u32 slot);
// Whether the loaded movie has data and its checksum is right
BOOL func_020107b0(void);
PokewoodBlock *getPokewoodBlock(void);
// A field of the loaded movie
u32 func_020107f0(u32 field, u32 a1);
// Loads the default movie into the Pokewood block and checks its checksum, whatever the arguments are
u32 loadPokewoodAndCheck(SaveControl *save, HeapID heapId, u32 slot);
// Copies the Pokewood block to dest
void savePokewoodBlk(void *dest);
// Saves the Pokewood block to a slot, a step each call, with state starting at 0; 2 and 3 are the results once done
u32 func_020106ec(GameData *gameData, HeapID heapId, u32 slot, u16 *state);

#endif // POKEBW2_SAVE_POKEWOOD_H
