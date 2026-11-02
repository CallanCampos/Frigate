// MSL's exit as the game's heap check calls it (port/hal/msl/msl_exit.c; port/cmake/msl.cmake compiles src/gfl/heap.c
// with exit renamed to it), on the whole program's objects: GFL's heaps (src/gfl/heap.c, src/gfl/heapsys.c) over
// NitroSystem's expanded heaps and the main arena, made as the game makes them at boot (src/system/gfl_use.c). The
// check is GFL_HeapDumpOnFailure, whose exit only notes a failure and returns on the DS. With the host's exit, each of
// these ended the program in the check, with status 1 and no message:
//   heap_check  the check's dump of a heap that holds a block returns and leaves the heap as it was, as the dumps
//               that the Pokémon Musical's end and the C-Gear's beacon search make whatever their heaps hold do; and
//               a heap deleted with a block still in it is deleted after its check
//   heap_full   an allocation that a heap has no room for: the check returns and GFL_HeapAllocate stops the game with
//               OS_Terminate, status 71 and its "port:" line, which port/tests/app/check_exit.py checks
//
//   test_msl_exit heap_check | heap_full
#include "gfl/heap.h"
#include "gfl/heapsys.h"
#include "hal/hw/hw.h"
#include "hal/os/port_os.h"
#include "nitro/os.h"
#include <stdio.h>
#include <string.h>

// A child of the user heap that no part of the game uses here, and its DS size (the host gives it twice that)
#define TEST_HEAP 2
#define TEST_HEAP_SIZE 0x1000
#define HEAP_ID_COUNT 32

// The two root heaps, the system's and the user's, smaller than the game's (src/system/gfl_use.c)
static const HeapDef sRoots[] = {
    { 0x1000, 0 },
    { 0x20000, 0 },
};

static int sFailed;

static void Check(int ok, const char *what) {
    if (!ok) {
        printf("  check failed: %s\n", what);
        sFailed = 1;
    }
}

// The OS as far as GFL's heaps and OS_Terminate need it (as port/tests/card/test_card_hardware.c boots it), the root
// heaps from the main arena, and the test heap
static void Boot(void) {
    OS_InitArena();
    OS_InitLock();
    OS_InitIrqTable();
    OS_InitThread();
    OS_InitTick();
    OS_InitAlarm();
    OS_EnableIrq();
    OS_EnableInterrupts();
    GFL_MemInit(sRoots, 2, HEAP_ID_COUNT, 0);
    GFL_HeapCreateChild(HEAPID_USER, TEST_HEAP, TEST_HEAP_SIZE);
}

static void HeapCheck(void) {
    void *block;
    u32 freeSize;

    Boot();
    Check(GFL_HeapGetBase(TEST_HEAP) != NULL, "the test heap is made");
    block = GFL_HeapAllocate(TEST_HEAP, 64, TRUE, "test_msl_exit.c", __LINE__);
    Check(block != NULL, "the test heap gives a block");
    freeSize = GFL_HeapGetFreeSizeCore(TEST_HEAP);
    GFL_HeapDumpOnFailure(TEST_HEAP);
    Check(GFL_HeapGetAllocationCount(TEST_HEAP) == 1 && GFL_HeapGetFreeSizeCore(TEST_HEAP) == freeSize,
          "the check's dump returns and leaves the heap as it was");
    GFL_HeapDelete(TEST_HEAP);
    Check(GFL_HeapGetBase(TEST_HEAP) == NULL, "a heap deleted with a block in it is deleted after its check");
    GFL_HeapCreateChild(HEAPID_USER, TEST_HEAP, TEST_HEAP_SIZE);
    Check(GFL_HeapGetBase(TEST_HEAP) != NULL && GFL_HeapGetAllocationCount(TEST_HEAP) == 0,
          "and its heap ID makes a new heap");
}

static void HeapFull(void) {
    Boot();
    GFL_HeapAllocate(TEST_HEAP, TEST_HEAP_SIZE * 4, FALSE, "test_msl_exit.c", __LINE__);
    Check(0, "GFL_HeapAllocate returns from a heap with no room for the block, where the game stops itself");
}

int main(int argc, char **argv) {
    const char *name = argc > 1 ? argv[1] : "";
    void (*run)(void);

    setvbuf(stdout, NULL, _IONBF, 0);
    if (strcmp(name, "heap_check") == 0) {
        run = HeapCheck;
    } else if (strcmp(name, "heap_full") == 0) {
        run = HeapFull;
    } else {
        printf("usage: test_msl_exit heap_check | heap_full\n");
        return 2;
    }
    port_hw_init();
    Check(port_os_run(run) == PORT_OS_RETURNED, "the run ended normally");
    printf("%s: %s\n", name, sFailed ? "FAIL" : "ok");
    return sFailed;
}
