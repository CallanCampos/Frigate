#ifndef POKEBW2_SAVE_POKEWOOD_H
#define POKEBW2_SAVE_POKEWOOD_H

#include "types.h"
#include "gfl/heap.h"
#include "save/battle_rec.h"
#include "struct_decls.h"
#include <stddef.h>

// Pokéstar Studios, which the game calls Pokewood. Its progress is save block 0x47 (PokewoodSave), and the movies
// downloaded into it are extra save data, which the Pokewood block is loaded from

// The number of movies. Each of the save's flag lists has a flag for each
#define POKEWOOD_MOVIE_COUNT 40

// A movie's best result, which the save keeps for each movie
typedef struct {
    s32 score;
    // PokeIcon_PackSpecies of each Pokémon of the cast: its species, form and sex
    u16 cast[6];
    // Set once the movie has a record, and whether that record was beaten and PokewoodSave_MarkRecordSeen has not seen
    // it yet
    u8 hasRecord;
    u8 isNewRecord;
    // A bit for each Pokémon of the cast whose Pokéstar fame is at level 2 or more
    u8 famousCast;
    u8 unk13;
} PokewoodRecord;

// BtlSetup fields that only Pokéstar Studios' block keeps
typedef struct {
    u32 movie;
    u32 ending;
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
    // The battle setup's audienceCount and studioCast
    u8 audienceCount;
    u8 studioCast;
    // When the movie was saved: the year, month, day and day of the week, and the hour, minute and second, a byte
    // each from the top
    u32 date;
    u32 time;
    u32 score;
    // getCRC16 of everything before it
    u32 crc;
} PokewoodBlock; // size: 0x4b4

// The size that the block's CRC covers
#define POKEWOOD_BLOCK_CRC_SIZE offsetof(PokewoodBlock, crc)
// The size of the extra save block that holds a movie, and of the buffer the block is loaded into
#define POKEWOOD_EXTRA_BLOCK_SIZE 0x600
// The downloaded movies the save has room for
#define POKEWOOD_DOWNLOAD_SLOT_COUNT 8

// The fields of the loaded movie that Pokewood_GetLoadedMovieField gives
enum {
    POKEWOOD_BLOCK_FIELD_MOVIE,
    POKEWOOD_BLOCK_FIELD_ENDING,
    // Makes the party Pokémon that the second argument points to of a Pokémon of the cast, and gives 1
    POKEWOOD_BLOCK_FIELD_PKM_0,
    POKEWOOD_BLOCK_FIELD_PKM_5 = POKEWOOD_BLOCK_FIELD_PKM_0 + 5,
    POKEWOOD_BLOCK_FIELD_AUDIENCE_COUNT,
    POKEWOOD_BLOCK_FIELD_STUDIO_CAST,
    POKEWOOD_BLOCK_FIELD_DATE,
    POKEWOOD_BLOCK_FIELD_TIME,
    POKEWOOD_BLOCK_FIELD_SCORE,
};

// The flag lists of the save, with a flag for each movie
#define POKEWOOD_FLAG_LIST_COUNT 7
#define POKEWOOD_FLAG_WORDS ((POKEWOOD_MOVIE_COUNT + 31) / 32)
#define POKEWOOD_SLOT_COUNT 8
#define POKEWOOD_LAST_SERIES_COUNT 4

// Block 0x47 (pokewood_savedata.c, a descriptive name)
struct PokewoodSave {
    // Flag list 0, the movies that can be filmed
    u32 unlockedMovies[POKEWOOD_FLAG_WORDS];
    // Flag list 1, the movies that have been filmed
    u32 filmedMovies[POKEWOOD_FLAG_WORDS];
    u32 flags2[POKEWOOD_FLAG_WORDS];
    // Flag list 3, the unlocked movies that the scripts have announced
    u32 announcedMovies[POKEWOOD_FLAG_WORDS];
    // Whether the movie of each slot has been watched, cleared when a new one is filmed into it
    u8 slotWatched[POKEWOOD_SLOT_COUNT];
    PokewoodRecord records[POKEWOOD_MOVIE_COUNT];
    // The points, 0 to 99, that give the level from 0 to 4
    s16 levelPoints;
    // A bit for each of those levels
    s16 levelBits;
    // The series (plus 1, 0 for none) of the last movies filmed, from the newest
    u8 lastSeries[POKEWOOD_LAST_SERIES_COUNT];
    u32 flags4[POKEWOOD_FLAG_WORDS];
    u32 flags5[POKEWOOD_FLAG_WORDS];
    u32 flags6[POKEWOOD_FLAG_WORDS];
    u8 padding368[0x7c];
}; // size: 0x3e4

// The block's size and its initial state
u32 PokewoodSave_GetSize(void);
void PokewoodSave_Init(PokewoodSave *save);
PokewoodSave *PokewoodSave_Get(GameData *gameData);
// Flag lists 0 to 6 of the save, with a flag for each movie
BOOL PokewoodSave_CheckFlag(PokewoodSave *save, u32 list, u32 movie);
void PokewoodSave_SetFlag(PokewoodSave *save, u32 list, u32 movie);
// PokewoodSave_CheckFlag for list 0, whether a movie is unlocked, and for list 1, whether it has been filmed
BOOL PokewoodSave_IsMovieUnlocked(PokewoodSave *save, u32 movie);
BOOL PokewoodSave_IsMovieFilmed(PokewoodSave *save, u32 movie);
// PokewoodSave_SetFlag for list 1
void PokewoodSave_SetMovieFilmed(PokewoodSave *save, u32 movie);
// Keeps record as the movie's best if it scored at least as much
void PokewoodSave_UpdateRecord(PokewoodSave *save, u32 movie, const PokewoodRecord *record);
// Marks a movie's record as there and as seen
void PokewoodSave_MarkRecordSeen(PokewoodSave *save, u32 movie);
PokewoodRecord *PokewoodSave_GetRecord(PokewoodSave *save, u32 movie);
// Whether the movie of a slot has been watched
u8 PokewoodSave_IsSlotWatched(PokewoodSave *save, u32 slot);
void PokewoodSave_SetSlotWatched(PokewoodSave *save, u32 slot, u8 value);
// The level points, 0 to 99, adding to them, and the level from 0 to 4 that points give
s16 PokewoodSave_GetLevelPoints(PokewoodSave *save);
void PokewoodSave_AddLevelPoints(PokewoodSave *save, int amount);
int PokewoodSave_GetLevelOfPoints(int value);
// A bit for each of those levels
BOOL PokewoodSave_GetLevelBit(PokewoodSave *save, u32 bit);
void PokewoodSave_SetLevelBit(PokewoodSave *save, u32 bit, u32 value);
// The series (plus 1, 0 for none) of the last four movies filmed, from the newest (0) to the oldest (3), which
// PokewoodSave_PushLastSeries pushes a series onto, keeping only those still in a slot of slotSeries
u8 PokewoodSave_GetLastSeries(PokewoodSave *save, u32 index);
void PokewoodSave_PushLastSeries(PokewoodSave *save, u32 slot, u32 series, u8 *slotSeries);
// How many movies have their flag set in a list
u32 PokewoodSave_CountFlags(PokewoodSave *save, u32 list);

// The Pokewood block is the buffer that a downloaded movie is loaded into (pokewood_rec.c). The size of the block
// and its initial state, as an extra save block
void clearPokewoodBlock(PokewoodBlock *block);
u32 getSizeOfPokewoodBlock(void);
void allocatePokewoodBlk(HeapID heapId);
void freeAndClearPokewoodBlk(void);
void setPokewoodBlk(const PokewoodBlock *src);
// Loads the downloaded movie of a slot (0 to 7): 1 when it loaded and its checksum is right, 2 when the checksum is
// wrong, 0 when the slot is empty and 4 when it failed
u32 Pokewood_LoadDownloadedMovie(SaveControl *save, HeapID heapId, u32 slot);
// Whether the loaded movie has data and its checksum is right
BOOL Pokewood_IsLoadedMovieValid(void);
PokewoodBlock *getPokewoodBlock(void);
// A field of the loaded movie
u32 Pokewood_GetLoadedMovieField(u32 field, u32 a1);
// Loads the default movie into the Pokewood block and checks its checksum, whatever the arguments are
u32 loadPokewoodAndCheck(SaveControl *save, HeapID heapId, u32 slot);
// Copies the Pokewood block to dest
void savePokewoodBlk(PokewoodBlock *dest);
// Saves the Pokewood block to a slot, a step each call, with state starting at 0; 2 and 3 are the results once done
u32 Pokewood_SaveDownloadedMovie(GameData *gameData, HeapID heapId, u32 slot, u16 *state);

#endif // POKEBW2_SAVE_POKEWOOD_H
