#ifndef POKEBW2_SAVE_MUSICAL_DIST_SAVE_H
#define POKEBW2_SAVE_MUSICAL_DIST_SAVE_H

#include "types.h"
#include "struct_decls.h"

// The downloaded musical, extra save block 6: a header with the size of its archive, then the archive.
// musical_dist_save.c is the ROM's string. The work's functions are declared in save/save_control.h

#define MUSICAL_DIST_SAVE_SIZE 0x17c00
#define MUSICAL_DIST_BUFFER_SIZE 0x18000

typedef struct {
    // The size of the archive
    u32 size;
    u8 unk4[0xc];
} MusicalDistHeader;

// The work that reads the block, or that writes a downloaded musical to it
typedef struct {
    u16 heapId;
    // Whether the block was read
    BOOL loaded;
    u32 seq;
    MusicalDistHeader header;
    // The block, its header and the archive after it
    void *block;
    MusicalDistHeader *blockHeader;
    void *data;
    GameData *gameData;
} MusicalDistWork; // size: 0x2c

// The size of the block and its init
u32 func_0200cc94(void);
void func_0200cc9c(void *block);

#endif // POKEBW2_SAVE_MUSICAL_DIST_SAVE_H
