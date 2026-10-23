// How large the host makes the game's heaps (src/gfl/heapsys.c calls port_heap_size for each root and child heap).
// The DS sizes are exact for the DS's structs; on a host a struct holding pointers is larger, so the same allocations
// need more room. Root heaps come from the OS main arena (port/hal/os/os_arena.c, 16 MiB by default) and get four
// times their DS size; child heaps, carved out of a root heap, get twice theirs, which fits inside the scaled root.
// The factors are what the boot screens, the logo movie and the title screen were measured to need; port/docs/STATUS.md
// records each change. --heap-scale-root and --heap-scale-child override them.
//
// A root heap can also be created in memory the game gives it (GFL_HeapCreateRoot), memory sized for the DS: the
// field's heap of cell actors, weather and place names in overlay 36's g_FieldHeapMemory (src/ov036/fieldmap_proc.c),
// the trade screen's in its own static (src/ov194/pokemontrade_proc.c), and GFL's extra heap at boot, from the OS heap
// (src/system/gfl_use.c). heapsys.c gives each the memory port_heap_root_memory returns in place of the caller's, at
// port_heap_size's root size (ADR 0021): the field's 64 KiB ran out on the host at its first load, and the game stopped
// itself (OS_Terminate, status 71) in FieldmapProc_Init, at CONTINUE and at New Game's first map alike. That memory
// follows the life of the caller's (heap_policy.h): it keeps what a heap left in it until the next heap is made there,
// as the DS's static does, and is zeroed when the static's overlay loads again; between two heaps, AddressSanitizer
// sees it poisoned.
#include "types.h"
#include "hal/gfl/heap_policy.h"
#include "hal/os/port_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Under AddressSanitizer, the kept memory is poisoned while no heap is in it (port/docs/sanitizers.md)
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define PORT_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) && !defined(PORT_ASAN)
#define PORT_ASAN 1
#endif
#ifdef PORT_ASAN
#include <sanitizer/asan_interface.h>
#define POISON(start, size) ASAN_POISON_MEMORY_REGION(start, size)
#define UNPOISON(start, size) ASAN_UNPOISON_MEMORY_REGION(start, size)
#else
#define POISON(start, size) ((void)(start), (void)(size))
#define UNPOISON(start, size) ((void)(start), (void)(size))
#endif

// How many distinct places in its own memory the game may create root heaps in: the game has three
#define CALLER_ROOT_MAX 8

// The host memory that stands in for one place in the game's memory where it creates a root heap: the place, at its
// DS size, and the host's memory, at its host size
typedef struct {
    u8 *caller;
    u32 callerSize;
    u8 *host;
    u32 size;
} CallerRoot;

static CallerRoot sCallerRoots[CALLER_ROOT_MAX];

static u32 Scale(const char *option, u32 fallback) {
    const char *value = port_app_option(option);
    if (value == NULL) {
        return fallback;
    }
    long factor = strtol(value, NULL, 10);
    return factor >= 1 && factor <= 64 ? (u32)factor : fallback;
}

u32 port_heap_size(u32 index, u32 size, BOOL root) {
    (void)index;
    return size * (root ? Scale("heap-scale-root", 4) : Scale("heap-scale-child", 2));
}

// The caller's memory is a static or lives as long as one (g_FieldHeapMemory, the trade screen's array, the extra
// heap's block, which is never freed), and the game creates its heap there again at each use: the field's at every map
// load. So the host's memory is kept for the caller's, as the DS keeps the static: made at the first creation there,
// zeroed as .bss is, and the same at every later one, with what the last heap left in it; port_heap_root_memory_reset
// zeroes it again where the DS clears the static. The game never creates two heaps in the same memory at once, and
// gives each place the same size every time; a larger size than the kept memory's would mean neither holds, so it
// stops the program rather than give a heap memory another may still use
void *port_heap_root_memory(u32 index, void *memory, u32 size) {
    u32 hostSize = port_heap_size(index, size, TRUE);
    CallerRoot *root = NULL;
    int i;

    for (i = 0; i < CALLER_ROOT_MAX && root == NULL; i++) {
        if (sCallerRoots[i].caller == memory || sCallerRoots[i].caller == NULL) {
            root = &sCallerRoots[i];
        }
    }
    if (root == NULL) {
        fprintf(stderr, "port: heap %u is created in a place of the game's memory beyond the %d that the host keeps\n",
                index, CALLER_ROOT_MAX);
        abort();
    }
    if (root->caller == NULL) {
        root->host = calloc(1, hostSize);
        if (root->host == NULL) {
            fprintf(stderr, "port: no memory for heap %u's %u bytes\n", index, hostSize);
            abort();
        }
        root->caller = memory;
        root->callerSize = size;
        root->size = hostSize;
    } else if (hostSize > root->size) {
        fprintf(stderr, "port: heap %u asks for %u bytes in memory that had %u for an earlier heap\n", index, hostSize,
                root->size);
        abort();
    }
    // The new heap owns all of it, as it would own memory from any other owner; its creation poisons what it holds free
    UNPOISON(root->host, root->size);
    return root->host;
}

// Only the root heap that GFL_HeapAddRoot made at the start of the kept memory releases it. A heap made inside that one
// (the field's weather, place names and scene area) has a block of it as its memory, which goes back to its parent
int port_heap_root_memory_release(const void *start) {
    int i;

    for (i = 0; i < CALLER_ROOT_MAX && sCallerRoots[i].caller != NULL; i++) {
        CallerRoot *root = &sCallerRoots[i];

        if ((const u8 *)start == root->host) {
            POISON(root->host, root->size);
            return 1;
        }
    }
    return 0;
}

// No heap is alive in an overlay's static when the overlay loads again: the code that deletes the heap is the
// overlay's own (FieldmapProc_Exit in overlay 36, the trade screen's exit in overlay 194). So the memory is zeroed, as
// the DS clears the static, and left poisoned, as it is between two heaps
void port_heap_root_memory_reset(const void *start, size_t size) {
    const u8 *from = start;
    int i;

    if (start == NULL) {
        return;
    }
    for (i = 0; i < CALLER_ROOT_MAX && sCallerRoots[i].caller != NULL; i++) {
        CallerRoot *root = &sCallerRoots[i];

        if (root->caller >= from && root->caller + root->callerSize <= from + size) {
            UNPOISON(root->host, root->size);
            memset(root->host, 0, root->size);
            POISON(root->host, root->size);
        }
    }
}
