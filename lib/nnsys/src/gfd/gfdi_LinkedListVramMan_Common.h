#ifndef POKEBW2_NNSYS_GFDI_LINKEDLISTVRAMMAN_COMMON_H
#define POKEBW2_NNSYS_GFDI_LINKEDLISTVRAMMAN_COMMON_H

#include "types.h"

// The free-list allocator that the linked-list texture and palette VRAM managers share. A manager keeps its free spans
// of VRAM as blocks in a list, takes new blocks from a pool in the work memory it is given, and gives them back there.
// The names, the types' and the fields' as well, are NitroSystem's, as ntrtwl's reconstruction of NitroSystem has them

// A free span of VRAM, linked both ways
typedef struct NNSiGfdLnkVramBlock {
    u32 addr;
    u32 szByte;
    struct NNSiGfdLnkVramBlock *pBlkPrev;
    struct NNSiGfdLnkVramBlock *pBlkNext;
} NNSiGfdLnkVramBlock;

typedef struct {
    NNSiGfdLnkVramBlock *pFreeList;
} NNSiGfdLnkVramMan;

void NNSi_GfdInitLnkVramMan(NNSiGfdLnkVramMan *list);
// Links the blocks of the array into a list, the pool, and returns its head
NNSiGfdLnkVramBlock *NNSi_GfdInitLnkVramBlockPool(NNSiGfdLnkVramBlock *blocks, u32 count);
BOOL NNSi_GfdAddNewFreeBlock(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool, u32 start, u32 size);
BOOL NNSi_GfdAllocLnkVram(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool, u32 *outAddr, u32 size);
BOOL NNSi_GfdAllocLnkVramAligned(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool, u32 *outAddr, u32 size,
                                 u32 align);
// Joins the free blocks that touch
void NNSi_GfdMergeAllFreeBlocks(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool);
BOOL NNSi_GfdFreeLnkVram(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool, u32 addr, u32 size);

#endif // POKEBW2_NNSYS_GFDI_LINKEDLISTVRAMMAN_COMMON_H
