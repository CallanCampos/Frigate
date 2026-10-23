// Unit tests of port/hal/gfl/heap_policy.c, the host's sizes for the game's heaps and the host memory that stands in
// for the memory the game creates a root heap in (GFL_HeapAddRoot, src/gfl/heapsys.c; ADR 0021): the field's heap, the
// trade screen's and GFL's extra heap. Each creation in the same place gets the same memory, zeroed the first time and
// keeping what was written in it after, as the DS's static keeps it; another place gets other memory; the size is
// port_heap_size's for a root, which --heap-scale-root changes. The program's options are this file's own: the test
// sets the root factor it reads. It prints each failure and returns nonzero if there was one.
#include "types.h"
#include "hal/os/port_app.h"
#include <stdio.h>
#include <string.h>

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

    // --heap-scale-root 2: twice the DS's size for the next place made
    sRootScale = "2";
    Check(port_heap_size(9, 0x3f00, TRUE) == 0x7e00, "--heap-scale-root sets the root factor");
    extraHost = port_heap_root_memory(9, extra, sizeof(extra));
    Check(extraHost != NULL && extraHost != fieldHost && extraHost != tradeHost, "a third place gets its own memory");
    memset(extraHost, 0x5a, 2 * sizeof(extra));
    Check(port_heap_root_memory(9, extra, sizeof(extra)) == extraHost, "the third place keeps its memory");
    sRootScale = NULL;

    if (sFailures == 0) {
        printf("heap_policy: every heap sized and placed as ADR 0021 says\n");
    }
    return sFailures != 0;
}
