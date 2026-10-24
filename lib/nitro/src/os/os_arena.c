// NitroSDK OS: memory arenas (os_arena.c, the SDK's name for the module). Thumb code. The limits of each arena are kept
// in a table of the shared main memory (HW_ARENA_INFO_BUF), and this file also knows where the arenas begin and end
// before the game moves them. The file has TwlSDK's DSi part too, what OS_InitArenaEx does on a DSi, in TwlSDK's
// LTDMAIN section, which the LTD autoload's .ltdmain holds: this file's entry in ARM9 main's delinks.txt lists it in
// an .ltdmain section, as TwlSDK's os_arena.o holds both (aletheia's match of
// Black 1's code to libos.TWL.HYB.a, which gives the DSi part's name).
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
} sArena; // swan's g_IsUserMemMarkDone

static void *OS_GetInitArenaHi(OSArenaId arena); // swan's GetUserMemRegionDefaultEnd
static void *OS_GetInitArenaLo(OSArenaId arena); // swan's GetUserMemRegionDefaultStart

// NitroSDK's OS_InitArena: sets the limits of the arenas to the ones they start with, once
void OS_InitArena(void) {
    if (!sArena.initialized) {
        sArena.initialized = TRUE;
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
// memory (region 1 is its first 4 MB, region 2 the 4 KB at 0x027ff000). A DSi does the protecting in the LTD module
void OS_InitArenaEx(void) {
    // The SDK reads the console type here and drops it; the read fills OS_GetConsoleType's cache
    OS_GetConsoleType();
    OS_SetArenaHi(OS_ARENA_MAINEX, OS_GetInitArenaHi(OS_ARENA_MAINEX));
    OS_SetArenaLo(OS_ARENA_MAINEX, OS_GetInitArenaLo(OS_ARENA_MAINEX));
    if (!OS_IsRunOnTwl()) {
        OS_SetProtectionRegion(1, 0x02000000, HW_C6_PR_4MB);
        OS_SetProtectionRegion(2, 0x027ff000, HW_C6_PR_4KB);
    } else {
        OSi_InitArenaEx_ltdmain();
    }
}

// NitroSDK's OS_GetArenaHi and OS_GetArenaLo
void *OS_GetArenaHi(OSArenaId arena) {
    return OSi_GetArenaInfo()->hi[arena];
}

void *OS_GetArenaLo(OSArenaId arena) {
    return OSi_GetArenaInfo()->lo[arena];
}

// NitroSDK's OS_GetInitArenaHi: where each arena ends before the game moves it
static void *OS_GetInitArenaHi(OSArenaId arena) {
    switch (arena) {
    case OS_ARENA_MAIN:
        if (OS_IsRunOnTwl()) {
            return (void *)0x02f80000;
        }
        return (void *)0x023e0000;
    case OS_ARENA_MAINEX: {
        u32 consoleSize;

        if (!sArena.mainExArenaEnabled) {
            return NULL;
        }
        consoleSize = OS_GetConsoleType() & OS_CONSOLE_SIZE_MASK;
        if (OS_IsRunOnTwl()) {
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
static void *OS_GetInitArenaLo(OSArenaId arena) {
    switch (arena) {
    case OS_ARENA_MAIN:
        if (OS_IsRunOnTwl()) {
            return LTD_AUTOLOAD_0_BSS_END;
        }
        return OV290_BSS_END;
    case OS_ARENA_MAINEX: {
        u32 consoleSize;

        if (!sArena.mainExArenaEnabled) {
            return NULL;
        }
        consoleSize = OS_GetConsoleType() & OS_CONSOLE_SIZE_MASK;
        if (OS_IsRunOnTwl()) {
            if (consoleSize == OS_CONSOLE_SIZE_16MB) {
                return NULL;
            }
            return (void *)SDK_MAINEX_ARENA_LO;
        } else {
            if (consoleSize == OS_CONSOLE_SIZE_4MB) {
                return NULL;
            }
            return (void *)0x02400060;
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

// NitroSDK's OS_SetArenaHi and OS_SetArenaLo
void OS_SetArenaHi(OSArenaId arena, void *end) {
    OSi_GetArenaInfo()->hi[arena] = end;
}

void OS_SetArenaLo(OSArenaId arena, void *start) {
    OSi_GetArenaInfo()->lo[arena] = start;
}

// NitroSDK's OS_AllocFromArenaLo: takes memory from the start of the arena, NULL when there is not
// enough
void *OS_AllocFromArenaLo(OSArenaId arena, u32 size, u32 alignment) {
    u32 start = (u32)OS_GetArenaLo(arena);
    u32 end;

    if (start == 0) {
        return NULL;
    }
    start = (start + alignment - 1) & ~(alignment - 1);
    end = start + size;
    end = (end + alignment - 1) & ~(alignment - 1);
    if (end > (u32)OS_GetArenaHi(arena)) {
        return NULL;
    }
    OS_SetArenaLo(arena, (void *)end);
    return (void *)start;
}

// clang-format off
// TwlSDK's section for the DSi-only code and data that the LTD autoload's .ltdmain holds
#pragma define_section LTDMAIN ".ltdmain" ".ltdmain.bss" ".ltdmain" abs32 RWX
// clang-format on
// The DSi part, in the LTD autoload: what OS_InitArenaEx does on a DSi
#pragma section LTDMAIN begin

void OSi_InitArenaEx_ltdmain(void) {
    // The SDK reads the console type here and drops it, as main's half does
    OS_GetConsoleType();
    OS_SetProtectionRegion(3, HW_CTRDG_ROM, HW_C6_PR_64MB);
}

#pragma section LTDMAIN end
