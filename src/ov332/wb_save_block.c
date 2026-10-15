#include "types.h"
#include "app/unova_link.h"

// Finds the blocks of a Black or White save. The ROM doesn't name this file. Its table is laid out before
// wb_save_convert.c's smaller ones, which a single file's size-sorted .rodata can't do, so it is a file of its own

#define WB_SAVE_BLOCK_COUNT 71
// The blocks before this one take a multiple of 256 bytes
#define WB_SAVE_ALIGNED_BLOCKS 69

static int WBSaveBlock_GetOffset(int block);
static u32 WBSaveBlock_GetAlignedSize(u32 block);
static u32 WBSaveBlock_GetSize(u32 block);

// clang-format off
static const u32 sBlockSizes[WB_SAVE_BLOCK_COUNT] = {
    0x3e4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4,
    0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4, 0xff4,
    0xff4, 0x9c4, 0x538, 0x6c, 0xa0, 0x133c, 0x7c8, 0xd58, 0x30, 0x65c, 0xa98, 0x1b0,
    0x3f0, 0x60, 0x1e4, 0xac, 0x464, 0x1404, 0x2a8, 0x2e0, 0x350, 0x3f0, 0xfc, 0x300,
    0x98, 0x360, 0x1d0, 0x16c, 0xf0, 0x1b4, 0x20, 0x4d8, 0x38, 0x40, 0x1b0, 0xb94,
    0xa0, 0x854, 0x2c, 0x288, 0x14, 0x60, 0x170, 0x44, 0x100, 0x8c, 0x10,
};

// clang-format on

static int WBSaveBlock_GetOffset(int block) {
    int offset = 0;
    u8 i;

    for (i = 0; i < block; i++) {
        if (i >= WB_SAVE_ALIGNED_BLOCKS) {
            offset += WBSaveBlock_GetSize(i);
        } else {
            offset += WBSaveBlock_GetAlignedSize(i);
        }
    }
    return offset;
}

static u32 WBSaveBlock_GetAlignedSize(u32 block) {
    u32 size = sBlockSizes[block];
    u8 rest = size % 0x100;

    return size + (rest == 0 ? 0 : 0x100 - rest);
}

static u32 WBSaveBlock_GetSize(u32 block) {
    return sBlockSizes[block];
}

void *WBSaveBlock_Get(void *save, int block) {
    return (u8 *)save + WBSaveBlock_GetOffset(block);
}
