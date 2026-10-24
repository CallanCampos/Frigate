// The host memory that stands in for the memory the game creates a root heap in (GFL_HeapAddRoot, src/gfl/heapsys.c;
// port/hal/gfl/heap_policy.c, ADR 0021): the field's g_FieldHeapMemory (src/ov036/fieldmap_proc.c), the trade screen's
// static (src/ov194/pokemontrade_proc.c) and GFL's extra heap (src/system/gfl_use.c). heapsys.c gets the memory from
// port_heap_root_memory, declared in port/include/port_host.h with port_heap_size; these two let the rest of the
// platform layer tell it what becomes of the game's memory that it stands in for: a heap there destroyed
// (port/hal/nnsys/fnd_hooks.c), and the load of the overlay whose static it is (port/hal/fs/fs_overlay.c). Plain C
// types only.
#ifndef PORT_HAL_GFL_HEAP_POLICY_H
#define PORT_HAL_GFL_HEAP_POLICY_H

#include <stddef.h>

// The heap whose memory starts at start was destroyed. Returns 1 when it is the root heap that GFL_HeapAddRoot made at
// the start of memory kept for a place in the game's own: no one owns that memory until the game makes a heap there
// again, so under AddressSanitizer it is poisoned until then, and a use of the old heap's memory is a report. Returns 0
// for any other heap, a child heap inside the kept memory among them, whose memory goes back to its owner
int port_heap_root_memory_release(const void *start);

// The game's memory [start, start + size) was set back as the DS's load of an overlay sets its statics back: the host
// memory kept for a place there is zeroed, as the DS clears that place, one of the overlay's zeroed statics
void port_heap_root_memory_reset(const void *start, size_t size);

#endif // PORT_HAL_GFL_HEAP_POLICY_H
