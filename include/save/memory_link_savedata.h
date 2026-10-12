#ifndef POKEBW2_SAVE_MEMORY_LINK_SAVEDATA_H
#define POKEBW2_SAVE_MEMORY_LINK_SAVEDATA_H

#include "types.h"
#include "struct_decls.h"

// The Memory Link's data (memory_link_savedata.c, a descriptive name): what was read from up to three Black or White
// saves to unlock the memories. The save keeps it at 0x68 of SaveControl, and the data that ov331 reads has it after
// the key information.

#define MEMORY_LINK_PLAYER_COUNT 3
#define MEMORY_LINK_FLAG_COUNT 16
#define MEMORY_LINK_VAR_COUNT 3
#define MEMORY_LINK_MEMORY_COUNT 8
#define MEMORY_LINK_UNK2C_COUNT 6

// The player of a Black or White save
typedef struct {
    u32 id;
    u16 name[8];
    u8 gender;
    u8 unk15;
    // Set in the Memory Link's data once the save is linked
    u8 linked;
} MemoryLinkPlayer; // size: 0x18

// What a Black or White save gives to unlock the memories
typedef struct {
    // Bits from event flags and from a word of block 52
    u8 flags[MEMORY_LINK_FLAG_COUNT / 8];
    // Event work variables
    u16 vars[MEMORY_LINK_VAR_COUNT];
    // The musical props owned
    u8 props[0x20];
    u16 unk28;
    u16 unk2A;
    u8 unk2C[MEMORY_LINK_UNK2C_COUNT][0x3c];
} MemoryLinkMemories; // size: 0x194

// The summary of a Black or White save that is linked (ov332's wb_save_convert.c makes it)
typedef struct {
    MemoryLinkPlayer player;
    MemoryLinkMemories memories;
} MemoryLinkData; // size: 0x1ac

// The Memory Link's data in the save. Each player is the one whose save gave a part of the memories: the flags and
// variables, the musical props, and unk2C
typedef struct MemoryLinkSave {
    MemoryLinkPlayer players[MEMORY_LINK_PLAYER_COUNT];
    MemoryLinkMemories memories;
} MemoryLinkSave; // size: 0x1dc

// What func_02010b0c gives
enum {
    MEMORY_LINK_KIND_UNK28,
    MEMORY_LINK_KIND_UNK2A,
    // Copies an entry of unk2C to dest
    MEMORY_LINK_KIND_UNK2C_0,
    MEMORY_LINK_KIND_UNK2C_5 = MEMORY_LINK_KIND_UNK2C_0 + MEMORY_LINK_UNK2C_COUNT - 1,
    // Copies a player to dest, if there is one
    MEMORY_LINK_KIND_PLAYER_0,
    MEMORY_LINK_KIND_PLAYER_2 = MEMORY_LINK_KIND_PLAYER_0 + MEMORY_LINK_PLAYER_COUNT - 1,
    // Copies the musical props to dest, if there is one, and gives their address
    MEMORY_LINK_KIND_PROPS,
};

// Clears the data
void zero476Bytes(MemoryLinkSave *link);
// Links a Black or White save: TRUE when it unlocked a memory
BOOL func_020108dc(MemoryLinkSave *link, const MemoryLinkData *data, SaveControl *save);
// TRUE when no save is linked yet, or when the save's player (ID, name, gender) is the one of every part that the
// save would change
BOOL func_0201090c(MemoryLinkSave *link, const MemoryLinkData *data, SaveControl *save);
// A value of a kind, MEMORY_LINK_KIND_*, copied to dest for some kinds
u32 func_02010b0c(MemoryLinkSave *link, u32 kind, void *dest);
// A flag and a variable of the memories
BOOL func_02010b90(MemoryLinkSave *link, int index);
u16 func_02010bc0(MemoryLinkSave *link, u32 index);
// Whether a player's slot is linked, and the player's gender and name
BOOL func_02010bcc(MemoryLinkSave *link, int index);
u8 func_02010bd8(MemoryLinkSave *link, u32 index);
const u16 *func_02010be4(MemoryLinkSave *link, u32 index);
// Whether a memory is unlocked, and the event work that marks it seen
BOOL func_02010c54(MemoryLinkSave *link, u32 memory);
u32 func_02010c6c(u32 memory);

#endif // POKEBW2_SAVE_MEMORY_LINK_SAVEDATA_H
