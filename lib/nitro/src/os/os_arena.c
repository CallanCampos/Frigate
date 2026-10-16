// NitroSDK OS: memory arenas (os_arena.c, the SDK's name for the module). Thumb code. The limits of each arena are kept
// in a table of the shared main memory (HW_ARENA_INFO_BUF), and this file also knows where the arenas begin and end
// before the game moves them.
#include "nitro/os.h"
#include "twl/os.h"

// The table of the arenas' limits in the shared main memory: the lowest address that is free in each arena, and the
// address where it ends
#define HW_ARENA_INFO_BUF 0x02fffda0

typedef struct OSArenaInfo {
    void *lo[OS_ARENA_MAX];
    void *hi[OS_ARENA_MAX];
} OSArenaInfo;

#define OSi_GetArenaInfo() ((OSArenaInfo *)HW_ARENA_INFO_BUF)

// The memory size in the low bits of the console type (NitroSDK's OS_GetConsoleType): the extended main memory has an
// arena only when the console has more memory than a retail one, which is 4 MB on a DS and 16 MB on a DSi
#define OS_CONSOLE_SIZE_MASK 0xf
#define OS_CONSOLE_SIZE_4MB 1
#define OS_CONSOLE_SIZE_16MB 4

// Whether OS_InitArena has run, and whether the extended main memory has an arena (nothing in the program turns that
// on, so the arena stays empty). The two words are reached from one literal, as one object
static struct {
    BOOL initialized;
    BOOL mainExArenaEnabled;
} sArena;

static void *GetUserMemRegionDefaultEnd(int arena);
static void *GetUserMemRegionDefaultStart(int arena);

// NitroSDK's OS_InitArena: sets the limits of the arenas to the ones they start with, once
void OS_InitArena(void) {
    if (!sArena.initialized) {
        sArena.initialized = TRUE;
        SetUserMemRegionEnd(OS_ARENA_MAIN, GetUserMemRegionDefaultEnd(OS_ARENA_MAIN));
        SetUserMemRegionStart(OS_ARENA_MAIN, GetUserMemRegionDefaultStart(OS_ARENA_MAIN));
        SetUserMemRegionStart(OS_ARENA_MAINEX, NULL);
        SetUserMemRegionEnd(OS_ARENA_MAINEX, NULL);
        SetUserMemRegionEnd(OS_ARENA_ITCM, GetUserMemRegionDefaultEnd(OS_ARENA_ITCM));
        SetUserMemRegionStart(OS_ARENA_ITCM, GetUserMemRegionDefaultStart(OS_ARENA_ITCM));
        SetUserMemRegionEnd(OS_ARENA_DTCM, GetUserMemRegionDefaultEnd(OS_ARENA_DTCM));
        SetUserMemRegionStart(OS_ARENA_DTCM, GetUserMemRegionDefaultStart(OS_ARENA_DTCM));
        SetUserMemRegionEnd(OS_ARENA_SHARED, GetUserMemRegionDefaultEnd(OS_ARENA_SHARED));
        SetUserMemRegionStart(OS_ARENA_SHARED, GetUserMemRegionDefaultStart(OS_ARENA_SHARED));
        SetUserMemRegionEnd(OS_ARENA_WRAM_MAIN, GetUserMemRegionDefaultEnd(OS_ARENA_WRAM_MAIN));
        SetUserMemRegionStart(OS_ARENA_WRAM_MAIN, GetUserMemRegionDefaultStart(OS_ARENA_WRAM_MAIN));
    }
}

// NitroSDK's OS_InitArenaEx: sets the limits of the extended main memory's arena and, on a DS, protects the main
// memory (region 1 is its first 4 MB, region 2 the 4 KB at 0x027ff000). A DSi does the protecting in the LTD module
void func_0207b23c(void) {
    func_0207ac24();
    SetUserMemRegionEnd(OS_ARENA_MAINEX, GetUserMemRegionDefaultEnd(OS_ARENA_MAINEX));
    SetUserMemRegionStart(OS_ARENA_MAINEX, GetUserMemRegionDefaultStart(OS_ARENA_MAINEX));
    if (!hw_isDSi()) {
        SetMemoryProtectionRegion(1, 0x02000000, OS_PROTECTION_SIZE_4MB);
        SetMemoryProtectionRegion(2, 0x027ff000, OS_PROTECTION_SIZE_4KB);
    } else {
        func_027692ec();
    }
}

// NitroSDK's OS_GetArenaHi and OS_GetArenaLo, under swan's names
void *GetUserMemRegionEnd(int arena) {
    return OSi_GetArenaInfo()->hi[arena];
}

void *GetUserMemRegionStart(int arena) {
    return OSi_GetArenaInfo()->lo[arena];
}

// NitroSDK's OS_GetInitArenaHi: where each arena ends before the game moves it
static void *GetUserMemRegionDefaultEnd(int arena) {
    switch (arena) {
    case OS_ARENA_MAIN:
        if (hw_isDSi()) {
            return (void *)0x02f80000;
        }
        return (void *)0x023e0000;
    case OS_ARENA_MAINEX: {
        u32 consoleSize;

        if (!sArena.mainExArenaEnabled) {
            return NULL;
        }
        consoleSize = func_0207ac24() & OS_CONSOLE_SIZE_MASK;
        if (hw_isDSi()) {
            if (consoleSize == OS_CONSOLE_SIZE_16MB) {
                return NULL;
            }
            return (void *)0x0df00000;
        }
        if (consoleSize == OS_CONSOLE_SIZE_4MB) {
            return NULL;
        }
        return (void *)0x02700000;
    }
    case OS_ARENA_ITCM:
        return (void *)0x02000000;
    case OS_ARENA_DTCM: {
        u32 irqStackEnd = HW_IRQ_STACK_END;
        int sysStackSize = HW_SYS_STACK_SIZE;

        if (sysStackSize == 0) {
            if (HW_DTCM < HW_DTCM_SYSRV_END) {
                return (void *)HW_DTCM_SYSRV_END;
            }
            return (void *)HW_DTCM;
        } else if (sysStackSize < 0) {
            return (void *)(HW_DTCM_SYSRV_END - sysStackSize);
        }
        return (void *)(irqStackEnd - sysStackSize);
    }
    case OS_ARENA_SHARED:
        return (void *)0x02fff680;
    case OS_ARENA_WRAM_MAIN:
        return (void *)0x03040000;
    }
    return NULL;
}

// NitroSDK's OS_GetInitArenaLo: where each arena begins before the game moves it
static void *GetUserMemRegionDefaultStart(int arena) {
    switch (arena) {
    case OS_ARENA_MAIN:
        if (hw_isDSi()) {
            return LTD_AUTOLOAD_0_BSS_END;
        }
        return OV290_BSS_END;
    case OS_ARENA_MAINEX: {
        u32 consoleSize;

        if (!sArena.mainExArenaEnabled) {
            return NULL;
        }
        consoleSize = func_0207ac24() & OS_CONSOLE_SIZE_MASK;
        if (hw_isDSi()) {
            void *lo = NULL;
            if (consoleSize != OS_CONSOLE_SIZE_16MB) {
                lo = (void *)SDK_MAINEX_ARENA_LO;
            }
            return lo;
        } else {
            void *lo = NULL;
            if (consoleSize != OS_CONSOLE_SIZE_4MB) {
                lo = (void *)0x02400060;
            }
            return lo;
        }
    }
    case OS_ARENA_ITCM:
        return ITCM_BSS_END;
    case OS_ARENA_DTCM:
        return (void *)HW_DTCM_SYSRV_END;
    case OS_ARENA_SHARED:
        return (void *)0x02fff000;
    case OS_ARENA_WRAM_MAIN:
        return (void *)0x03040000;
    }
    return NULL;
}

// NitroSDK's OS_SetArenaHi and OS_SetArenaLo, under swan's names
void SetUserMemRegionEnd(int arena, void *end) {
    OSi_GetArenaInfo()->hi[arena] = end;
}

void SetUserMemRegionStart(int arena, void *start) {
    OSi_GetArenaInfo()->lo[arena] = start;
}

// NitroSDK's OS_AllocFromArenaLo, under swan's name: takes memory from the start of the arena, NULL when there is not
// enough
void *mem_alloc_direct(int arena, u32 size, u32 alignment) {
    u32 start = (u32)GetUserMemRegionStart(arena);
    u32 end;

    if (start == 0) {
        return NULL;
    }
    start = (start + alignment - 1) & ~(alignment - 1);
    end = start + size;
    end = (end + alignment - 1) & ~(alignment - 1);
    if (end > (u32)GetUserMemRegionEnd(arena)) {
        return NULL;
    }
    SetUserMemRegionStart(arena, (void *)end);
    return (void *)start;
}
