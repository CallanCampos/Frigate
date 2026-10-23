#include "gfdi_LinkedListVramMan_Common.h"

// NitroSystem's linked-list VRAM allocator, shared by the texture and palette managers. pokediamond's link map has the
// file as gfdi_linkedlist (cut at 15 characters), and ntrtwl's reconstruction of NitroSystem has the whole name

// A free span being grown by the blocks that touch it, from start up to end
typedef struct {
    u32 start;
    u32 end;
} NNSiGfdLnkMemRegion;

static BOOL TryToMergeBlockRegion_(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool, NNSiGfdLnkMemRegion *span);

static inline void InsertBlock_(NNSiGfdLnkVramBlock **head, NNSiGfdLnkVramBlock *block) {
    if (*head != NULL) {
        (*head)->pBlkPrev = block;
    }
    block->pBlkNext = *head;
    block->pBlkPrev = NULL;
    *head = block;
}

static inline void RemoveBlock_(NNSiGfdLnkVramBlock **head, NNSiGfdLnkVramBlock *block) {
    NNSiGfdLnkVramBlock *before = block->pBlkPrev;
    NNSiGfdLnkVramBlock *after = block->pBlkNext;

    if (before != NULL) {
        before->pBlkNext = after;
    } else {
        *head = after;
    }
    if (after != NULL) {
        after->pBlkPrev = before;
    }
}

static inline NNSiGfdLnkVramBlock *GetNewBlock_(NNSiGfdLnkVramBlock **pool) {
    NNSiGfdLnkVramBlock *block = *pool;

    if (block != NULL) {
        *pool = block->pBlkNext;
    }
    return block;
}

static inline void InitBlockFromPrams_(NNSiGfdLnkVramBlock *block, u32 start, u32 size) {
    block->addr = start;
    block->szByte = size;
    block->pBlkPrev = NULL;
    block->pBlkNext = NULL;
}

// Moves the free blocks that touch the span into it, and their blocks back to the pool, and returns whether there
// were any
static BOOL TryToMergeBlockRegion_(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool, NNSiGfdLnkMemRegion *span) {
    NNSiGfdLnkVramBlock *block = list->pFreeList;
    NNSiGfdLnkVramBlock *next;
    BOOL absorbed = FALSE;

    while (block != NULL) {
        next = block->pBlkNext;

        if (block->addr == span->end) {
            span->end = block->addr + block->szByte;
            RemoveBlock_(&list->pFreeList, block);
            InsertBlock_(pool, block);
            absorbed |= TRUE;
        }
        if (span->start == block->addr + block->szByte) {
            span->start = block->addr;
            RemoveBlock_(&list->pFreeList, block);
            InsertBlock_(pool, block);
            absorbed |= TRUE;
        }

        block = next;
    }
    return absorbed;
}

void NNSi_GfdInitLnkVramMan(NNSiGfdLnkVramMan *list) {
    list->pFreeList = NULL;
}

NNSiGfdLnkVramBlock *NNSi_GfdInitLnkVramBlockPool(NNSiGfdLnkVramBlock *blocks, u32 count) {
    u32 i;

    for (i = 0; i < count - 1; i++) {
        blocks[i].pBlkNext = &blocks[i + 1];
        blocks[i + 1].pBlkPrev = &blocks[i];
    }
    blocks[0].pBlkPrev = NULL;
    (blocks + count - 1)->pBlkNext = NULL;
    return blocks;
}

BOOL NNSi_GfdAddNewFreeBlock(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool, u32 start, u32 size) {
    NNSiGfdLnkVramBlock *block = GetNewBlock_(pool);

    if (block != NULL) {
        InitBlockFromPrams_(block, start, size);
        InsertBlock_(&list->pFreeList, block);
        return TRUE;
    }
    return FALSE;
}

BOOL NNSi_GfdAllocLnkVram(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool, u32 *outAddr, u32 size) {
    return NNSi_GfdAllocLnkVramAligned(list, pool, outAddr, size, 0);
}

BOOL NNSi_GfdAllocLnkVramAligned(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool, u32 *outAddr, u32 size,
                                 u32 align) {
    NNSiGfdLnkVramBlock *block = list->pFreeList;
    NNSiGfdLnkVramBlock *chosen = NULL;
    u32 addr;
    u32 taken;
    u32 skipped;

    // The first free block that holds the size once its start is aligned
    while (block != NULL) {
        if (align > 1) {
            addr = (block->addr + (align - 1)) & ~(align - 1);
            skipped = addr - block->addr;
            taken = size + skipped;
        } else {
            addr = block->addr;
            skipped = 0;
            taken = size;
        }

        if (block->szByte >= taken) {
            chosen = block;
            break;
        }
        block = block->pBlkNext;
    }

    if (chosen != NULL) {
        // What aligning skips stays free, as a block of its own
        if (skipped > 0) {
            NNSiGfdLnkVramBlock *gap = GetNewBlock_(pool);

            if (gap == NULL) {
                goto fail;
            }
            InitBlockFromPrams_(gap, chosen->addr, skipped);
            InsertBlock_(&list->pFreeList, gap);
        }

        chosen->szByte -= taken;
        chosen->addr += taken;
        if (chosen->szByte == 0) {
            RemoveBlock_(&list->pFreeList, chosen);
            InsertBlock_(pool, chosen);
        }
        *outAddr = addr;
        return TRUE;
    }

fail:
    *outAddr = 0;
    return FALSE;
}

void NNSi_GfdMergeAllFreeBlocks(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool) {
    NNSiGfdLnkVramBlock *block = list->pFreeList;

    while (block != NULL) {
        NNSiGfdLnkMemRegion span;

        span.start = block->addr;
        span.end = block->addr + block->szByte;
        if (TryToMergeBlockRegion_(list, pool, &span)) {
            // The list changed, so start over
            block->addr = span.start;
            block->szByte = span.end - span.start;
            block = list->pFreeList;
        } else {
            block = block->pBlkNext;
        }
    }
}

BOOL NNSi_GfdFreeLnkVram(NNSiGfdLnkVramMan *list, NNSiGfdLnkVramBlock **pool, u32 addr, u32 size) {
    NNSiGfdLnkMemRegion span;
    NNSiGfdLnkVramBlock *block;

    span.start = addr;
    span.end = addr + size;
    TryToMergeBlockRegion_(list, pool, &span);

    block = GetNewBlock_(pool);
    if (block == NULL) {
        return FALSE;
    }
    block->addr = span.start;
    block->szByte = span.end - span.start;
    block->pBlkPrev = NULL;
    block->pBlkNext = NULL;
    InsertBlock_(&list->pFreeList, block);
    return TRUE;
}
