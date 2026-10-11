#ifndef POKEBW2_SAVE_TOURNAMENT_BLOCK_H
#define POKEBW2_SAVE_TOURNAMENT_BLOCK_H

#include "types.h"
#include "struct_decls.h"

// The save block that swan calls the tournament block, of entries of 0x160 bytes whose fields a number picks

// What of an entry the code reads without calling the accessors
typedef struct {
    u8 unk0[0x15b];
    // Field 10 of func_0200b798
    s8 unk15B;
    u8 unk15C[0x4];
} TournamentEntry;

void *getTournamentBlockAddress(SaveControl *save);
// The CRC16 of the first 0x15e bytes of an entry
u16 CRC16_350bytes(const void *entry);
// An entry of the block
void *func_0200b8f4(void *block, u32 index);
// An entry's field, and setting it. The Wi-Fi login reads field 2 and field 10 and sets field 10
u32 func_0200b798(void *entry, u32 field);
// An entry's regulation, which is the entry itself, and a parameter of the regulation. The battle type is 0x15
Regulation *func_0200b530(void *entry);
u32 func_0200b574(Regulation *regulation, u32 param);
void func_0200b830(void *entry, u32 field, u32 value);
// The entry's data of 0x92 bytes
void *func_0200b794(void *entry);
// The block's other entries, of 0x1e bytes
void *func_0200b924(void *block, u32 index);
// A field of the Pokémon number index of such an entry: the species (0), the form (1), whether it holds an item (2)
// and its gender (3)
u32 func_0200b934(void *record, u32 field, u8 index);
// Clears the player's VS data
void clearVSPlayer(void);

#endif // POKEBW2_SAVE_TOURNAMENT_BLOCK_H
