#ifndef POKEBW2_SAVE_EXTRA_BLOCK_SAVEDATA_H
#define POKEBW2_SAVE_EXTRA_BLOCK_SAVEDATA_H

#include "types.h"

// Two extra save blocks that are only cleared here (extra_block_savedata.c, a descriptive name): one of 0xc8 bytes,
// and the downloaded tournament of the Pokémon World Tournament, whose checksum save/wbt_save.h's func_02010428 checks.

typedef struct {
    u8 data[0x1210];
    // getCRC16 of the data
    u32 crc;
} WbtDownloadData; // size: 0x1214

// The size of the block of 0xc8 bytes and its initial state
u32 func_020103f4(void);
void func_020103f8(void *block);
// The size of a downloaded tournament and its initial state
u32 func_0201040c(void);
void func_02010414(void *block);

#endif // POKEBW2_SAVE_EXTRA_BLOCK_SAVEDATA_H
