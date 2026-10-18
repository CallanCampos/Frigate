// ARM9 pointers that a PXI protocol carries in 32 bits (pxi_bus.h's port_pxi_pointer and its notes). On the DS a
// pointer is 32 bits and the protocols send it whole, in one word or in two halves; on a 64-bit host the ARM9's code
// cuts it to its low 32 bits when it puts it in the words ((u32)buf). The ARM7's services get the whole pointer back in
// two ways:
//   - from the places the ARM9 keeps such data: the program's static data (the libraries' buffers, the sound heap
//     g_GFLSndHeap), from the executable's bounds, and the main memory arena, which the game carves its heaps from, as
//     seen when the ARM9's libraries set their PXI callbacks (the first ones do in OS_Init, after OS_InitArena and
//     before the game takes anything from the arena). Each place is far smaller than 4 GiB, so the low 32 bits name at
//     most one address in each. The two must not share low 32 bits either, or a pointer in one could be taken for one
//     in the other, in the launches where the loader and the allocator put them so (about one in twenty, for the
//     177 MiB of static data and the 16 MiB arena): the OS layer takes the arena's memory from port_pxi_alloc_arena,
//     which keeps it apart, and port_pxi_observe_arena stops the program when an arena doesn't. So the answer is the
//     same in every launch;
//   - from the pointers that the ARM9's side of a protocol noted under its tag before it sent them (the host's
//     wrappers of nvram.c and snd_interface.c), with the size of the buffer when it is known: the newest of the tag's
//     notes that is the pointer or holds it is the answer. A note bounds its buffer, and finds one that is in neither
//     place (a local on a thread's stack). Each tag keeps its last NOTE_COUNT notes, and only its own services look at
//     them, so the protocols that note nothing (the card's, the file system's) never get another protocol's buffer.
#include "hal/host/host.h"
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// How many of the pointers a tag sent last are kept
#define NOTE_COUNT 64
// The span of the low 32 bits
#define LOW_BITS_SPAN ((uint64_t)1 << 32)
// How many blocks port_pxi_alloc_arena tries before it gives up. Each one it rejects stays allocated until it has one,
// so that the next is elsewhere; the static data's span is passed after a dozen 16 MiB blocks
#define ARENA_TRIES 64

typedef struct Region {
    uintptr_t start;
    uintptr_t end;
} Region;

typedef struct Note {
    const u8 *pointer;
    u32 size;
} Note;

// A tag's notes: a ring with the newest at next - 1
typedef struct NoteRing {
    Note notes[NOTE_COUNT];
    u32 next;
} NoteRing;

// The main arena's widest extent seen so far, the static data's (found once), and the notes of each tag
static Region sArena;
static Region sData;
static NoteRing sNotes[PXI_MAX_FIFO_TAG];

static const Region *StaticData(void) {
    if (sData.end != 0) {
        return &sData;
    }
    // The executable's writable data, from the host's image: Mach-O's __DATA, ELF's .data and .bss, PE's writable
    // sections (port/hal/host)
    port_host_static_data(&sData.start, &sData.end);
    return &sData;
}

// Whether an address of a and one of b can have the same low 32 bits: whether their spans of low 32 bits meet. Each
// span starts at the place's start's low 32 bits and wraps around at 4 GiB
static BOOL LowBitsMeet(const Region *a, const Region *b) {
    uint64_t sizeA = a->end - a->start;
    uint64_t sizeB = b->end - b->start;

    if (a->end <= a->start || b->end <= b->start) {
        return FALSE;
    }
    if (sizeA >= LOW_BITS_SPAN || sizeB >= LOW_BITS_SPAN) {
        return TRUE;
    }
    // One starts inside the other, counting in the low 32 bits from the other's start
    return (uint32_t)(b->start - a->start) < sizeA || (uint32_t)(a->start - b->start) < sizeB;
}

BOOL port_pxi_unique_low_bits(const void *start, const void *end) {
    Region block = { (uintptr_t)start, (uintptr_t)end };

    return block.end >= block.start && !LowBitsMeet(&block, StaticData());
}

void *port_pxi_alloc_arena(size_t size) {
    void *rejected[ARENA_TRIES];
    void *block = NULL;
    int count = 0;
    int i;

    while (count < ARENA_TRIES) {
        block = calloc(1, size);
        if (block == NULL || port_pxi_unique_low_bits(block, (const u8 *)block + size)) {
            break;
        }
        rejected[count++] = block;
        block = NULL;
    }
    for (i = 0; i < count; i++) {
        free(rejected[i]);
    }
    if (block == NULL && count == ARENA_TRIES) {
        fprintf(stderr,
                "port: pxi: no host memory of %zu bytes for the main memory arena was found apart from the low 32 "
                "bits of the program's static data in %d tries\n",
                size, ARENA_TRIES);
    }
    return block;
}

void port_pxi_observe_arena(void) {
    uintptr_t lo = (uintptr_t)OS_GetArenaLo(OS_ARENA_MAIN);
    uintptr_t hi = (uintptr_t)OS_GetArenaHi(OS_ARENA_MAIN);
    const Region *data;

    if (lo == 0 || hi <= lo) {
        return;
    }
    if (sArena.end == 0) {
        sArena.start = lo;
        sArena.end = hi;
    } else {
        if (lo < sArena.start) {
            sArena.start = lo;
        }
        if (hi > sArena.end) {
            sArena.end = hi;
        }
    }
    data = StaticData();
    if (LowBitsMeet(&sArena, data)) {
        fprintf(stderr,
                "port: pxi: the main memory arena (%p to %p) has addresses with the low 32 bits of ones of the "
                "program's static data (%p to %p), so the 32-bit pointers that the PXI protocols carry can't be told "
                "apart; the arena's memory must come from port_pxi_alloc_arena (port/hal/pxi/pxi_bus.h)\n",
                (void *)sArena.start, (void *)sArena.end, (void *)data->start, (void *)data->end);
        exit(PORT_PXI_EXIT_STATUS);
    }
}

void port_pxi_note_pointer(u32 tag, const void *pointer, u32 size) {
    NoteRing *ring;

    if (pointer == NULL || tag >= PXI_MAX_FIFO_TAG) {
        return;
    }
    ring = &sNotes[tag];
    ring->notes[ring->next % NOTE_COUNT].pointer = pointer;
    ring->notes[ring->next % NOTE_COUNT].size = size;
    ring->next++;
}

void port_pxi_forget_pointers(void) {
    memset(sNotes, 0, sizeof(sNotes));
}

// The address in the region whose low 32 bits are address, or 0
static uintptr_t FindIn(const Region *region, u32 address) {
    uint64_t candidate;

    if (region->end <= region->start) {
        return 0;
    }
    candidate = ((uint64_t)region->start & ~(uint64_t)0xffffffff) | address;
    if (candidate < region->start) {
        candidate += LOW_BITS_SPAN;
    }
    return candidate < region->end ? (uintptr_t)candidate : 0;
}

static BOOL Contains(const Region *region, uintptr_t address) {
    return address >= region->start && address < region->end;
}

void *port_pxi_pointer(u32 address, const u8 **end) {
    const Region *data = StaticData();
    uintptr_t found;

    port_pxi_observe_arena();
    // The two places share no low 32 bits (port_pxi_observe_arena), so at most one has the address
    found = FindIn(data, address);
    if (found != 0) {
        if (end != NULL) {
            *end = (const u8 *)data->end;
        }
        return (void *)found;
    }
    found = FindIn(&sArena, address);
    if (found != 0) {
        if (end != NULL) {
            *end = (const u8 *)sArena.end;
        }
        return (void *)found;
    }
    return NULL;
}

void *port_pxi_noted_pointer(u32 tag, u32 address, const u8 **end) {
    const NoteRing *ring;
    u32 i;

    if (tag >= PXI_MAX_FIFO_TAG) {
        return port_pxi_pointer(address, end);
    }
    ring = &sNotes[tag];
    for (i = 1; i <= NOTE_COUNT && i <= ring->next; i++) {
        const Note *note = &ring->notes[(ring->next - i) % NOTE_COUNT];
        u32 offset = address - (u32)(uintptr_t)note->pointer;
        uintptr_t found;

        if (note->pointer == NULL || (note->size == 0 ? offset != 0 : offset >= note->size)) {
            continue;
        }
        found = (uintptr_t)note->pointer + offset;
        if (end != NULL) {
            if (note->size != 0) {
                *end = note->pointer + note->size;
            } else {
                // A pointer without a size is bounded by its place, if it is in one
                port_pxi_observe_arena();
                if (Contains(StaticData(), found)) {
                    *end = (const u8 *)StaticData()->end;
                } else if (Contains(&sArena, found)) {
                    *end = (const u8 *)sArena.end;
                } else {
                    return NULL;
                }
            }
        }
        return (void *)found;
    }
    return port_pxi_pointer(address, end);
}
