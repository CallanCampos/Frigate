// How large the host makes the game's heaps (src/gfl/heapsys.c calls port_heap_size for each root and child heap).
// The DS sizes are exact for the DS's structs; on a host a struct holding pointers is larger, so the same allocations
// need more room. Root heaps come from the OS main arena (port/hal/os/os_arena.c, 16 MiB by default) and get four
// times their DS size; child heaps, carved out of a root heap, get twice theirs, which fits inside the scaled root.
// The factors are what the boot screens, the logo movie and the title screen were measured to need; port/docs/STATUS.md
// records each change. --heap-scale-root and --heap-scale-child override them.
#include "types.h"
#include "hal/os/port_app.h"
#include <stdlib.h>

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
