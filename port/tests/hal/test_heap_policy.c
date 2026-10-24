// Unit tests of port/hal/gfl/heap_policy.c, the host's sizes for the game's heaps and the host memory that stands in
// for the memory the game creates a root heap in (GFL_HeapAddRoot, src/gfl/heapsys.c; ADR 0021): the field's heap, the
// trade screen's and GFL's extra heap. Each creation in the same place gets the same memory, zeroed the first time and
// keeping what was written in it after, as the DS's static keeps it, until the static's overlay loads again
// (port_heap_root_memory_reset, which port/hal/fs/fs_overlay.c calls), which zeroes it as the DS clears the static;
// another place gets other memory; the size is port_heap_size's for a root, which --heap-scale-root changes. The root
// heap destroyed at the start of kept memory (port_heap_root_memory_release, which port/hal/nnsys/fnd_hooks.c calls)
// leaves it poisoned under AddressSanitizer until the next creation there; a child heap inside it, and memory that
// isn't kept, are left to their owners. The program's
// options are this file's own: the test sets the root factor it reads. It prints each failure and returns nonzero if
// there was one.
#include "types.h"
#include "hal/gfl/heap_policy.h"
#include "hal/os/port_app.h"
#include <stdio.h>
#include <string.h>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define TEST_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) && !defined(TEST_ASAN)
#define TEST_ASAN 1
#endif
#ifdef TEST_ASAN
#include <sanitizer/asan_interface.h>
#endif

// Whether ASan sees [start, start + size) poisoned from end to end; without ASan nothing is poisoned, and it holds
static int AllPoisoned(u8 *start, size_t size) {
#ifdef TEST_ASAN
    size_t i;

    for (i = 0; i < size; i++) {
        if (!__asan_address_is_poisoned(start + i)) {
            return FALSE;
        }
    }
#else
    (void)start;
    (void)size;
#endif
    return TRUE;
}

// Whether ASan sees [start, start + size) addressable from end to end
static int NonePoisoned(u8 *start, size_t size) {
#ifdef TEST_ASAN
    return __asan_region_is_poisoned(start, size) == NULL;
#else
    (void)start;
    (void)size;
    return TRUE;
#endif
}

// port_heap_size and port_heap_root_memory are declared in port/include/port_host.h, which the build force-includes

static int sFailures;
static const char *sRootScale;

const char *port_app_option(const char *name) {
    return strcmp(name, "heap-scale-root") == 0 ? sRootScale : NULL;
}

static void Check(int ok, const char *what) {
    if (!ok) {
        printf("FAIL %s\n", what);
        sFailures++;
    }
}

int main(void) {
    // The DS-sized memory of three callers, as the field's, the trade screen's and the extra heap's are
    static u8 field[0x100], trade[0x80], extra[0x40];
    u8 *fieldHost, *tradeHost, *extraHost;
    u32 i;
    int zeroed = TRUE;

    Check(port_heap_size(0x89, 0x10000, TRUE) == 0x40000, "a root heap is four times its DS size");
    Check(port_heap_size(0x8a, 0x6400, FALSE) == 0xc800, "a child heap is twice its DS size");

    fieldHost = port_heap_root_memory(0x89, field, sizeof(field));
    Check(fieldHost != NULL && fieldHost != field, "a root heap in the game's memory gets host memory");
    for (i = 0; i < 4 * sizeof(field); i++) {
        zeroed = zeroed && fieldHost[i] == 0;
    }
    Check(zeroed, "the host memory is zeroed at first, as .bss is, over the root's whole size");
    // What the heap leaves there stays, as it does in the DS's static
    memset(fieldHost, 0xa5, 4 * sizeof(field));
    Check(port_heap_root_memory(0x89, field, sizeof(field)) == fieldHost, "the same place gets the same memory again");
    Check(fieldHost[0] == 0xa5 && fieldHost[4 * sizeof(field) - 1] == 0xa5, "what the last heap left stays");

    tradeHost = port_heap_root_memory(0x5f, trade, sizeof(trade));
    Check(tradeHost != NULL && tradeHost != fieldHost, "another place gets other memory");

    // A smaller size in the same place reuses the memory it has
    Check(port_heap_root_memory(0x89, field, sizeof(field) / 2) == fieldHost, "a smaller size keeps the memory");

    // A heap destroyed in kept memory: the memory has no owner until the next heap there, and ASan sees it poisoned;
    // memory that isn't kept, a heap's in the arena or in another heap's block, goes back to its owner
    Check(port_heap_root_memory_release(trade) == FALSE, "the game's own memory isn't the host's kept memory");
    Check(port_heap_root_memory_release(fieldHost + 0x100) == FALSE, "a child heap inside goes back to its parent");
    Check(NonePoisoned(fieldHost, 4 * sizeof(field)), "a child heap's end leaves its parent's memory to it");
    Check(port_heap_root_memory_release(fieldHost) == TRUE, "the root heap at its start releases kept memory");
    Check(AllPoisoned(fieldHost, 4 * sizeof(field)), "released kept memory is poisoned, all of it");
    Check(NonePoisoned(tradeHost, 4 * sizeof(trade)), "other kept memory stays as it was");
    Check(port_heap_root_memory(0x89, field, sizeof(field)) == fieldHost, "the next heap gets the same memory again");
    Check(NonePoisoned(fieldHost, 4 * sizeof(field)), "the next heap's creation makes all of it addressable");
    Check(fieldHost[0] == 0xa5 && fieldHost[4 * sizeof(field) - 1] == 0xa5, "what the destroyed heap left stays");

    // The static's overlay loads again: the DS clears the static, and the host its kept memory, which stays poisoned
    // as between any two heaps; a range that doesn't hold the whole place leaves it alone
    memset(tradeHost, 0x3c, 4 * sizeof(trade));
    Check(port_heap_root_memory_release(fieldHost) == TRUE, "the field's heap is destroyed before its overlay loads");
    port_heap_root_memory_reset(field + 1, sizeof(field));
    Check(port_heap_root_memory(0x89, field, sizeof(field)) == fieldHost && fieldHost[0] == 0xa5,
          "a range without the whole place doesn't touch its memory");
    Check(port_heap_root_memory_release(fieldHost) == TRUE, "the field's heap is destroyed again");
    port_heap_root_memory_reset(field, sizeof(field));
    Check(AllPoisoned(fieldHost, 4 * sizeof(field)), "the overlay's load leaves the kept memory poisoned");
    Check(port_heap_root_memory(0x89, field, sizeof(field)) == fieldHost, "the field's memory is kept across a load");
    zeroed = TRUE;
    for (i = 0; i < 4 * sizeof(field); i++) {
        zeroed = zeroed && fieldHost[i] == 0;
    }
    Check(zeroed, "the overlay's load zeroes the kept memory, as the DS clears the static");
    Check(tradeHost[0] == 0x3c && tradeHost[4 * sizeof(trade) - 1] == 0x3c, "another place's memory is untouched");
    port_heap_root_memory_reset(NULL, 0);

    // --heap-scale-root 2: twice the DS's size for the next place made
    sRootScale = "2";
    Check(port_heap_size(9, 0x3f00, TRUE) == 0x7e00, "--heap-scale-root sets the root factor");
    extraHost = port_heap_root_memory(9, extra, sizeof(extra));
    Check(extraHost != NULL && extraHost != fieldHost && extraHost != tradeHost, "a third place gets its own memory");
    memset(extraHost, 0x5a, 2 * sizeof(extra));
    Check(port_heap_root_memory(9, extra, sizeof(extra)) == extraHost, "the third place keeps its memory");
    sRootScale = NULL;

    if (sFailures == 0) {
        printf("heap_policy: every heap sized, placed, kept and cleared as ADR 0021 says\n");
    }
    return sFailures != 0;
}
