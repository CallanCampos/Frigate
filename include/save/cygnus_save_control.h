#ifndef POKEBW2_SAVE_CYGNUS_SAVE_CONTROL_H
#define POKEBW2_SAVE_CYGNUS_SAVE_CONTROL_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Pokémon Dream Radar ("Cygnus"): the game's own save block, and the data that Dream Radar writes to the card outside
// the save. The accessors that overlay 332 uses are declared in save/dream_world.h and save/save_control.h

// GetDreamRadarFlag's and SetDreamRadarFlag's values
#define DREAM_RADAR_FLAG_READ 0
#define DREAM_RADAR_FLAG_SEED 1
#define DREAM_RADAR_FLAG_RECEIVED 2

// Block 0x48
struct DreamRadarSave {
    // The seed that the card's data is to be decoded with
    u32 seed;
    // The bits of the Pokémon received
    u32 received;
    u8 read;
    u8 padding09[0xe7];
}; // size: 0xf0

#define CYGNUS_DATA_MAGIC 0x43524746 // "FGRC"
#define CYGNUS_DATA_WORD_MAX 32

// ValidateDreamRadarSave's results
#define CYGNUS_DATA_INVALID 0
// The game marked the data read, and Dream Radar has not written since
#define CYGNUS_DATA_READ 1
#define CYGNUS_DATA_VALID 2
#define CYGNUS_DATA_BROKEN 3

// The data on the card, at CYGNUS_DATA_CARD_OFFSET
typedef struct {
    // 1 once the game has read the data
    u32 state;
    u32 seed;
    u32 magic;
    // The bits of the Pokémon received, encoded with the seed
    u32 received;
    // getCRC16 of everything before it
    u16 headerCrc;
    // What Dream Radar sends, encoded with the seed
    u32 words[CYGNUS_DATA_WORD_MAX];
    // getCRC16 of the words
    u16 crc;
} CygnusSaveData; // size: 0x98

u32 getDreamRadarSaveSize(void);
void initDreamRadarSave(DreamRadarSave *save);
u32 ValidateDreamRadarSave(CygnusSaveData *data);

// Whether the card holds data of Dream Radar's, read with a buffer of heapId's
BOOL func_020115a8(HeapID heapId);
// Marks the card's data read with the save's seed and Pokémon, and writes it
void func_020115f0(SaveControl *save);

#endif // POKEBW2_SAVE_CYGNUS_SAVE_CONTROL_H
