// NitroSDK's memory arenas (os_arena.c) over host memory: the decomp's lib/nitro/src/os/os_arena.c with the DS's
// memory map replaced. On the DS each arena is a range of the memory map between the linker's symbols and fixed ends;
// here each is a block of host memory, sized from the DS's and larger where the game's structures grow with 64-bit
// pointers:
//   - main memory: the DS's arena runs from the end of the last overlay (OV290_BSS_END, 0x021fd080) to 0x023e0000,
//     1.9 MiB, from which the game carves its heaps (GFLAppInit, gfl/heapsys.c). The host's is 16 MiB by default
//     (port_os_set_main_arena_size), so that heap sizes can be measured and raised without running out of arena;
//   - the extended main memory: none, as on a retail DS with 4 MiB (OS_GetConsoleType);
//   - ITCM: the DS's arena is what the ITCM module leaves (0x01ff93a0 to 0x02000000, 27 KiB); the host's is 4 times
//     that, rounded up;
//   - DTCM: inside the host's DTCM block (memory_map.c), from the end of the system's part (0xc0) to the system stack
//     under the IRQ stack (0x1f80), as on the DS. The launcher runs on its own fiber, so the host leaves the system
//     stack's 6 KiB unused, but the arena is the DS's; the game moves its end itself (gfl/heap_dtcm.c);
//   - the shared memory: the DS's is 0x02fff000 to 0x02fff680; the host's is 4 times that;
//   - the shared WRAM: empty, as on the DS (both ends 0x03040000).
// The table of the arenas' limits is kept here, not in the DS's shared memory (HW_ARENA_INFO_BUF), where 64-bit
// pointers would not fit; nothing but this file reads it.
#include "hal/hw/hw.h"
#include "hal/os/os_internal.h"
#include "hal/os/port_os.h"
#include "hal/pxi/pxi_bus.h"
#include "nitro/os.h"
#include "twl/os.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The memory size in the low bits of the console type (NitroSDK's OS_GetConsoleType): the extended main memory has an
// arena only when the console has more memory than a retail one, which is 4 MB on a DS and 16 MB on a DSi
#define OS_CONSOLE_SIZE_MASK 0xf
#define OS_CONSOLE_SIZE_4MB 1
#define OS_CONSOLE_SIZE_16MB 4

// The arenas' alignment on the DS, which the host's blocks keep
#define ARENA_ALIGNMENT 32
#define DEFAULT_MAIN_ARENA_SIZE (16 * 1024 * 1024)
#define ITCM_ARENA_SIZE (128 * 1024)
#define SHARED_ARENA_SIZE (4 * 0x680)
// The shared WRAM's empty arena on the DS
#define WRAM_MAIN_ARENA 0x03040000

typedef struct OSArenaInfo {
    void *lo[OS_ARENA_MAX];
    void *hi[OS_ARENA_MAX];
} OSArenaInfo;

// A block of host memory for an arena: the allocation, its aligned start and its size, and whether it is as calloc
// gave it (zeroed, and on most hosts not yet committed)
typedef struct HostArena {
    void *block;
    u8 *start;
    size_t size;
    BOOL fresh;
} HostArena;

static OSArenaInfo sArenaInfo;

// Whether OS_InitArena has run, and whether the extended main memory has an arena (nothing in the program turns that
// on, so the arena stays empty)
static struct {
    BOOL initialized;
    BOOL mainExArenaEnabled;
} sArena; // swan's g_IsUserMemMarkDone

static size_t sMainArenaSize = DEFAULT_MAIN_ARENA_SIZE;
static HostArena sMainArena;
static HostArena sItcmArena;
static HostArena sSharedArena;

static void *OS_GetInitArenaHi(int arena); // swan's GetUserMemRegionDefaultEnd
static void *OS_GetInitArenaLo(int arena); // swan's GetUserMemRegionDefaultStart

void port_os_set_main_arena_size(size_t bytes) {
    sMainArenaSize = bytes;
}

// The arena's block, made once per process and zeroed for each run (the DS's memory has no defined contents there; a
// host's zeros keep runs deterministic). A new block is zeroed already, so its pages stay uncommitted until the game
// touches them. The main arena's comes from the PXI bus, which keeps the low 32 bits of its addresses apart from the
// program's static data's, so that the 32-bit pointers the PXI protocols carry name one place in every launch
// (port/hal/pxi/pxi_bus.h)
static void PrepareArena(HostArena *arena, size_t size) {
    if (arena->block != NULL && arena->size != size) {
        free(arena->block);
        arena->block = NULL;
    }
    if (arena->block == NULL) {
        arena->block =
            arena == &sMainArena ? port_pxi_alloc_arena(size + ARENA_ALIGNMENT) : calloc(1, size + ARENA_ALIGNMENT);
        if (arena->block == NULL) {
            fprintf(stderr, "port: os: no host memory for an arena of %zu bytes\n", size);
            exit(1);
        }
        arena->start = (u8 *)(((uintptr_t)arena->block + ARENA_ALIGNMENT - 1) & ~(uintptr_t)(ARENA_ALIGNMENT - 1));
        arena->size = size;
        arena->fresh = TRUE;
    }
    if (!arena->fresh) {
        memset(arena->start, 0, arena->size);
    }
    arena->fresh = FALSE;
}

void port_osi_reset_arenas(void) {
    sArena.initialized = FALSE;
    sArena.mainExArenaEnabled = FALSE;
    memset(&sArenaInfo, 0, sizeof(sArenaInfo));
}

// NitroSDK's OS_InitArena: sets the limits of the arenas to the ones they start with, once
void OS_InitArena(void) {
    if (!sArena.initialized) {
        sArena.initialized = TRUE;
        PrepareArena(&sMainArena, sMainArenaSize);
        PrepareArena(&sItcmArena, ITCM_ARENA_SIZE);
        PrepareArena(&sSharedArena, SHARED_ARENA_SIZE);
        OS_SetArenaHi(OS_ARENA_MAIN, OS_GetInitArenaHi(OS_ARENA_MAIN));
        OS_SetArenaLo(OS_ARENA_MAIN, OS_GetInitArenaLo(OS_ARENA_MAIN));
        OS_SetArenaLo(OS_ARENA_MAINEX, NULL);
        OS_SetArenaHi(OS_ARENA_MAINEX, NULL);
        OS_SetArenaHi(OS_ARENA_ITCM, OS_GetInitArenaHi(OS_ARENA_ITCM));
        OS_SetArenaLo(OS_ARENA_ITCM, OS_GetInitArenaLo(OS_ARENA_ITCM));
        OS_SetArenaHi(OS_ARENA_DTCM, OS_GetInitArenaHi(OS_ARENA_DTCM));
        OS_SetArenaLo(OS_ARENA_DTCM, OS_GetInitArenaLo(OS_ARENA_DTCM));
        OS_SetArenaHi(OS_ARENA_SHARED, OS_GetInitArenaHi(OS_ARENA_SHARED));
        OS_SetArenaLo(OS_ARENA_SHARED, OS_GetInitArenaLo(OS_ARENA_SHARED));
        OS_SetArenaHi(OS_ARENA_WRAM_MAIN, OS_GetInitArenaHi(OS_ARENA_WRAM_MAIN));
        OS_SetArenaLo(OS_ARENA_WRAM_MAIN, OS_GetInitArenaLo(OS_ARENA_WRAM_MAIN));
    }
}

// NitroSDK's OS_InitArenaEx: sets the limits of the extended main memory's arena and, on a DS, protects the main
// memory (region 1 is its first 4 MB, region 2 the 4 KB at 0x027ff000; no-ops on the host, os_protection.c). A DSi
// does the protecting in the LTD module
void OS_InitArenaEx(void) {
    // The SDK reads the console type here and drops it; the read fills OS_GetConsoleType's cache
    OS_GetConsoleType();
    OS_SetArenaHi(OS_ARENA_MAINEX, OS_GetInitArenaHi(OS_ARENA_MAINEX));
    OS_SetArenaLo(OS_ARENA_MAINEX, OS_GetInitArenaLo(OS_ARENA_MAINEX));
    if (!OS_IsRunOnTwl()) {
        OS_SetProtectionRegion(1, 0x02000000, OS_PROTECTION_SIZE_4MB);
        OS_SetProtectionRegion(2, 0x027ff000, OS_PROTECTION_SIZE_4KB);
    } else {
        func_027692ec();
    }
}

// NitroSDK's OS_GetArenaHi and OS_GetArenaLo
void *OS_GetArenaHi(int arena) {
    return sArenaInfo.hi[arena];
}

void *OS_GetArenaLo(int arena) {
    return sArenaInfo.lo[arena];
}

// The extended main memory's arena exists only on a console with more memory than a retail one; the host is a retail
// DS, and nothing turns the arena on
static BOOL HasMainExArena(void) {
    u32 consoleSize;

    if (!sArena.mainExArenaEnabled) {
        return FALSE;
    }
    consoleSize = OS_GetConsoleType() & OS_CONSOLE_SIZE_MASK;
    return OS_IsRunOnTwl() ? consoleSize != OS_CONSOLE_SIZE_16MB : consoleSize != OS_CONSOLE_SIZE_4MB;
}

// NitroSDK's OS_GetInitArenaHi: where each arena ends before the game moves it
static void *OS_GetInitArenaHi(int arena) {
    switch (arena) {
    case OS_ARENA_MAIN:
        return sMainArena.start + sMainArena.size;
    case OS_ARENA_MAINEX:
        if (HasMainExArena()) {
            fprintf(stderr, "port: os: the extended main memory's arena has no host memory\n");
            exit(1);
        }
        return NULL;
    case OS_ARENA_ITCM:
        return sItcmArena.start + sItcmArena.size;
    case OS_ARENA_DTCM:
        // Under the system stack, which is under the IRQ stack (a positive SDK_SYS_STACKSIZE, as BW2's)
        return port_osi_dtcm() + PORT_OSI_IRQ_STACK_TOP - PORT_OSI_IRQ_STACK_SIZE - PORT_OSI_SYS_STACK_SIZE;
    case OS_ARENA_SHARED:
        return sSharedArena.start + sSharedArena.size;
    case OS_ARENA_WRAM_MAIN:
        return port_hw_pointer(WRAM_MAIN_ARENA);
    }
    return NULL;
}

// NitroSDK's OS_GetInitArenaLo: where each arena begins before the game moves it
static void *OS_GetInitArenaLo(int arena) {
    switch (arena) {
    case OS_ARENA_MAIN:
        return sMainArena.start;
    case OS_ARENA_MAINEX:
        return NULL;
    case OS_ARENA_ITCM:
        return sItcmArena.start;
    case OS_ARENA_DTCM:
        return port_osi_dtcm() + PORT_OSI_DTCM_SYSRV_END;
    case OS_ARENA_SHARED:
        return sSharedArena.start;
    case OS_ARENA_WRAM_MAIN:
        return port_hw_pointer(WRAM_MAIN_ARENA);
    }
    return NULL;
}

// NitroSDK's OS_SetArenaHi and OS_SetArenaLo
void OS_SetArenaHi(int arena, void *end) {
    sArenaInfo.hi[arena] = end;
}

void OS_SetArenaLo(int arena, void *start) {
    sArenaInfo.lo[arena] = start;
}

// NitroSDK's OS_AllocFromArenaLo: takes memory from the start of the arena, NULL when there is not enough. The
// addresses are host pointers, which the SDK's u32 arithmetic would truncate
void *OS_AllocFromArenaLo(int arena, u32 size, u32 alignment) {
    uintptr_t start = (uintptr_t)OS_GetArenaLo(arena);
    uintptr_t end;

    if (start == 0) {
        return NULL;
    }
    start = (start + alignment - 1) & ~(uintptr_t)(alignment - 1);
    end = start + size;
    end = (end + alignment - 1) & ~(uintptr_t)(alignment - 1);
    if (end > (uintptr_t)OS_GetArenaHi(arena)) {
        return NULL;
    }
    OS_SetArenaLo(arena, (void *)end);
    return (void *)start;
}
