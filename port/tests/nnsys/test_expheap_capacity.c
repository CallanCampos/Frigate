// Checks NitroSystem's expanded-heap capacity query (lib/nnsys/src/fnd/expheap.c) when a native free block's
// address range crosses a 4 GiB boundary. The query reads only the heap/free-list headers: this fixture supplies
// synthetic metadata, never allocates its declared payload and never calls an allocator or free operation on it.
// All real pointer arithmetic stays within the metadata objects; the query computes payload addresses as integers.
#include "nnsys/fnd.h"
#include <stddef.h>
#include <stdio.h>

struct HeapFixture {
    NNSiFndHeapHead common;
    NNSiFndExpHeapHead expanded;
};

_Static_assert(sizeof(uptr) > sizeof(u32), "the regression exercises native address bits above bit 31");
_Static_assert(offsetof(struct HeapFixture, expanded) == sizeof(NNSiFndHeapHead),
               "the expanded header must immediately follow the common header");

int main(void) {
    struct HeapFixture heap = { 0 };
    NNSiFndExpHeapMBlockHead blocks[2] = { 0 };
    NNSiFndExpHeapMBlockHead *block = &blocks[0];
    uptr data = (uptr)block + sizeof(*block);
    u32 low = (u32)data;
    u32 expected;
    u32 actual;

    // Two adjacent headers ensure at least one data address has low bits greater than 16, even if the first
    // happens to lie on the boundary itself. Only those headers are read; no storage for the payload is needed.
    if (low <= 16) {
        block = &blocks[1];
        data = (uptr)block + sizeof(*block);
        low = (u32)data;
    }
    if (low <= 16 || (data & 3) != 0) {
        fprintf(stderr, "expheap capacity fixture cannot represent its aligned boundary case\n");
        return 1;
    }
    // The declared payload ends 16 bytes past the next 4 GiB boundary. Its byte count still fits in u32.
    expected = (u32)(0u - low) + 16u;
    block->blockSize = expected;
    heap.expanded.mbFreeList.head = block;
    heap.expanded.mbFreeList.tail = block;

    actual = NNS_FndGetAllocatableSizeForExpHeapEx(&heap.common, 4);
    if (actual != expected) {
        fprintf(stderr, "expheap capacity across 4 GiB: expected %u bytes, got %u\n", expected, actual);
        return 1;
    }
    return 0;
}
