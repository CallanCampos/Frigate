// NitroSDK OS: the heap allocator (os_alloc.c, the SDK's name for the module). Thumb code. An arena holds an array of
// heaps, and each heap two doubly linked lists of cells: the free ones (in address order, neighbors joined) and the
// allocated ones. A cell is the 0x20 bytes before the memory it gives out.
#include "nitro/os.h"

// A cell: the block's neighbors in its list and its size, with the header's 0x20 bytes included
typedef struct OSAllocCell {
    struct OSAllocCell *prev;
    struct OSAllocCell *next;
    s32 size;
} OSAllocCell;

#define OS_ALLOC_HEADER_SIZE 0x20
#define OS_ALLOC_ALIGNMENT 0x20
// A free cell that would be left with less than this is given out whole
#define OS_ALLOC_MIN_CELL_SIZE 0x40
#define OS_ROUND_UP_32(x) (((u32)(x) + OS_ALLOC_ALIGNMENT - 1) & ~(OS_ALLOC_ALIGNMENT - 1))
#define OS_ROUND_DOWN_32(x) ((u32)(x) & ~(OS_ALLOC_ALIGNMENT - 1))

// A heap: its size (negative when the heap does not exist) and its lists
typedef struct OSHeap {
    s32 size;
    OSAllocCell *free;
    OSAllocCell *allocated;
} OSHeap;

// What an arena keeps of its heaps: the current heap, the array of heaps (which follows this) and the memory that is
// left for them
typedef struct OSHeapInfo {
    s32 current;
    s32 numHeaps;
    void *arenaStart;
    void *arenaEnd;
    OSHeap *heaps;
} OSHeapInfo;

// The heaps of each arena (NULL until OS_InitAlloc is called for the arena)
static OSHeapInfo *sHeapInfo[OS_ARENA_MAX];

// The extra heap that mem_bind_static_ram_alloc made (its number, and the arena it is in): -1 while there is none
static struct {
    s32 heap;
    int arena;
} sExtraHeap = { -1, -1 };

static OSAllocCell *func_0207b468(OSAllocCell *list, OSAllocCell *cell);
static OSAllocCell *DLExtract(OSAllocCell *list, OSAllocCell *cell);
static OSAllocCell *DLInsert(OSAllocCell *list, OSAllocCell *cell);

// NitroSDK's DLAddFront: puts a cell at the front of a list
static OSAllocCell *func_0207b468(OSAllocCell *list, OSAllocCell *cell) {
    cell->next = list;
    cell->prev = NULL;
    if (list != NULL) {
        list->prev = cell;
    }
    return cell;
}

// NitroSDK's DLExtract: takes a cell out of a list
static OSAllocCell *DLExtract(OSAllocCell *list, OSAllocCell *cell) {
    if (cell->next != NULL) {
        cell->next->prev = cell->prev;
    }
    if (cell->prev == NULL) {
        return cell->next;
    }
    cell->prev->next = cell->next;
    return list;
}

// NitroSDK's DLInsert: puts a cell into a list sorted by address, joining it to the cells next to it when they touch
static OSAllocCell *DLInsert(OSAllocCell *list, OSAllocCell *cell) {
    OSAllocCell *prev;
    OSAllocCell *next;

    for (next = list, prev = NULL; next != NULL; prev = next, next = next->next) {
        if (cell <= next) {
            break;
        }
    }
    cell->next = next;
    cell->prev = prev;
    if (next != NULL) {
        next->prev = cell;
        if ((u8 *)cell + cell->size == (u8 *)next) {
            cell->size += next->size;
            next = next->next;
            cell->next = next;
            if (next != NULL) {
                next->prev = cell;
            }
        }
    }
    if (prev != NULL) {
        prev->next = cell;
        if ((u8 *)prev + prev->size == (u8 *)cell) {
            prev->size += cell->size;
            prev->next = next;
            if (next != NULL) {
                next->prev = prev;
            }
        }
        return list;
    }
    return cell;
}

// NitroSDK's OS_AllocFromHeap, under swan's name: gives memory of the size, aligned to 0x20, from the heap (the
// arena's current one when negative); NULL when the arena has no heaps or no cell is big enough
void *malloc_device(int arena, int heap, s32 size) {
    OSHeapInfo *info;
    OSHeap *hp;
    OSAllocCell *cell;
    OSAllocCell *rest;
    u32 leftover;
    u32 irq = CPU_IRQDisable();

    info = sHeapInfo[arena];
    if (info == NULL) {
        CPU_SetIRQMask(irq);
        return NULL;
    }
    if (heap < 0) {
        heap = info->current;
    }
    hp = &info->heaps[heap];
    size = OS_ROUND_UP_32(size + OS_ALLOC_HEADER_SIZE);
    for (cell = hp->free; cell != NULL; cell = cell->next) {
        if (size <= cell->size) {
            break;
        }
    }
    if (cell == NULL) {
        CPU_SetIRQMask(irq);
        return NULL;
    }
    leftover = cell->size - size;
    if (leftover < OS_ALLOC_MIN_CELL_SIZE) {
        hp->free = DLExtract(hp->free, cell);
    } else {
        cell->size = size;
        rest = (OSAllocCell *)((u8 *)cell + size);
        rest->size = leftover;
        rest->prev = cell->prev;
        rest->next = cell->next;
        if (rest->next != NULL) {
            rest->next->prev = rest;
        }
        if (rest->prev != NULL) {
            rest->prev->next = rest;
        } else {
            hp->free = rest;
        }
    }
    hp->allocated = func_0207b468(hp->allocated, cell);
    CPU_SetIRQMask(irq);
    return (u8 *)cell + OS_ALLOC_HEADER_SIZE;
}

// NitroSDK's OS_FreeToHeap, under swan's name: gives memory back to the heap (the arena's current one when negative)
void free_device(int arena, int heap, void *ptr) {
    OSHeapInfo *info;
    OSHeap *hp;
    OSAllocCell *cell;
    u32 irq = CPU_IRQDisable();

    info = sHeapInfo[arena];
    if (heap < 0) {
        heap = info->current;
    }
    hp = &info->heaps[heap];
    cell = (OSAllocCell *)((u8 *)ptr - OS_ALLOC_HEADER_SIZE);
    hp->allocated = DLExtract(hp->allocated, cell);
    hp->free = DLInsert(hp->free, cell);
    CPU_SetIRQMask(irq);
}

// NitroSDK's OS_InitAlloc, under swan's name: makes the arena's table of heaps, with room for the number of them, at
// the start of the memory that it is given, and returns where the memory that is left starts
void *mem_init_alloc_area(int arena, void *start, void *end, int maxHeaps) {
    OSHeapInfo *info = start;
    u32 heapsSize;
    u32 irq = CPU_IRQDisable();
    int i;

    sHeapInfo[arena] = info;
    heapsSize = maxHeaps * sizeof(OSHeap);
    info->heaps = (OSHeap *)((u8 *)info + sizeof(OSHeapInfo));
    info->numHeaps = maxHeaps;
    for (i = 0; i < info->numHeaps; i++) {
        OSHeap *heaps = info->heaps;

        heaps[i].size = -1;
        heaps[i].allocated = NULL;
        heaps[i].free = NULL;
    }
    info->current = -1;
    info->arenaStart = (void *)OS_ROUND_UP_32((u32)info->heaps + heapsSize);
    end = (void *)OS_ROUND_DOWN_32(end);
    info->arenaEnd = end;
    CPU_SetIRQMask(irq);
    return info->arenaStart;
}

// NitroSDK's OS_CreateHeap, under swan's name: makes a heap of the memory from start to end (rounded in to 0x20), and
// returns its number, or -1 when the arena has no room for another heap
int mem_bind_range(int arena, void *start, void *end) {
    OSHeapInfo *info;
    OSHeap *hp;
    OSAllocCell *cell;
    int heap;
    u32 irq = CPU_IRQDisable();

    info = sHeapInfo[arena];
    start = (void *)OS_ROUND_UP_32(start);
    end = (void *)OS_ROUND_DOWN_32(end);
    for (heap = 0; heap < info->numHeaps; heap++) {
        hp = &info->heaps[heap];
        if (hp->size < 0) {
            hp->size = (u8 *)end - (u8 *)start;
            cell = start;
            cell->prev = NULL;
            cell->next = NULL;
            cell->size = hp->size;
            hp->free = cell;
            hp->allocated = NULL;
            CPU_SetIRQMask(irq);
            return heap;
        }
    }
    CPU_SetIRQMask(irq);
    return -1;
}

// On a DS, makes a heap of the 16 KB at the start of the main memory (which nothing else uses before the program)
// once, in the first heap of the arena that is free. Gives the heap's number, or -1
int mem_bind_static_ram_alloc(int arena) {
    OSHeapInfo *info;
    OSHeap *hp;
    OSAllocCell *cell;
    int heap;
    u32 irq = CPU_IRQDisable();

    info = sHeapInfo[arena];
    if (!hw_isDSi() && sExtraHeap.heap < 0) {
        for (heap = 0; heap < info->numHeaps; heap++) {
            hp = &info->heaps[heap];
            if (hp->size < 0) {
                hp->size = 0x4000;
                cell = (OSAllocCell *)0x02000000;
                cell->prev = NULL;
                cell->next = NULL;
                cell->size = 0x4000;
                hp->free = cell;
                hp->allocated = NULL;
                sExtraHeap.arena = arena;
                sExtraHeap.heap = heap;
                CPU_SetIRQMask(irq);
                return heap;
            }
        }
    }
    CPU_SetIRQMask(irq);
    return -1;
}

// The usable size of an allocated block, which MSL's __sys_pointer_size asks for
u32 func_0207b730(int arena, void *ptr) {
    CPU_SetIRQMask(CPU_IRQDisable());
    return ((OSAllocCell *)((u8 *)ptr - OS_ALLOC_HEADER_SIZE))->size - OS_ALLOC_HEADER_SIZE;
}
