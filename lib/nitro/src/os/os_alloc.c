// NitroSDK OS: the heap allocator (os_alloc.c, the SDK's name for the module). Thumb code. An arena holds an array of
// heaps, and each heap two doubly linked lists of cells: the free ones (in address order, neighbors joined) and the
// allocated ones. A cell is the 0x20 bytes before the memory it gives out.
#include "nitro/os.h"

// A cell: the block's neighbors in its list and its size, with the header's 0x20 bytes included
typedef struct Cell {
    struct Cell *prev;
    struct Cell *next;
    s32 size;
} Cell;

#define HEADERSIZE 0x20
#define ALIGNMENT 0x20
// A free cell that would be left with less than this is given out whole
#define MINOBJSIZE 0x40
#define ROUND(x) (((u32)(x) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))
#define TRUNC(x) ((u32)(x) & ~(ALIGNMENT - 1))

// A heap: its size (negative when the heap does not exist) and its lists
typedef struct HeapDesc {
    s32 size;
    Cell *free;
    Cell *allocated;
} HeapDesc;

// What an arena keeps of its heaps: the current heap, the array of heaps (which follows this) and the memory that is
// left for them
typedef struct OSHeapInfo {
    s32 currentHeap;
    s32 numHeaps;
    void *arenaStart;
    void *arenaEnd;
    HeapDesc *heapArray;
} OSHeapInfo;

// The heaps of each arena (NULL until OS_InitAlloc is called for the arena)
static OSHeapInfo *OSiHeapInfo[OS_ARENA_MAX]; // swan's g_AllocAreas

// The extra heap that OS_CreateExtraHeap made (its number, and the arena it is in): -1 while there is none
static struct {
    s32 heap;
    int arena;
} sExtraHeap = { -1, -1 };

static Cell *DLAddFront(Cell *list, Cell *cell);
static Cell *DLExtract(Cell *list, Cell *cell);
static Cell *DLInsert(Cell *list, Cell *cell);

// NitroSDK's DLAddFront: puts a cell at the front of a list
static Cell *DLAddFront(Cell *list, Cell *cell) {
    cell->next = list;
    cell->prev = NULL;
    if (list != NULL) {
        list->prev = cell;
    }
    return cell;
}

// NitroSDK's DLExtract: takes a cell out of a list
static Cell *DLExtract(Cell *list, Cell *cell) {
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
static Cell *DLInsert(Cell *list, Cell *cell) {
    Cell *prev;
    Cell *next;

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

// NitroSDK's OS_AllocFromHeap: gives memory of the size, aligned to 0x20, from the heap (the
// arena's current one when negative); NULL when the arena has no heaps or no cell is big enough
void *OS_AllocFromHeap(OSArenaId arena, OSHeapHandle heap, u32 size) {
    OSHeapInfo *info;
    HeapDesc *hp;
    Cell *cell;
    Cell *rest;
    u32 leftover;
    OSIntrMode irq = OS_DisableInterrupts();

    info = OSiHeapInfo[arena];
    if (info == NULL) {
        OS_RestoreInterrupts(irq);
        return NULL;
    }
    if (heap < 0) {
        heap = info->currentHeap;
    }
    hp = &info->heapArray[heap];
    size = ROUND(size + HEADERSIZE);
    for (cell = hp->free; cell != NULL; cell = cell->next) {
        if ((s32)size <= cell->size) {
            break;
        }
    }
    if (cell == NULL) {
        OS_RestoreInterrupts(irq);
        return NULL;
    }
    leftover = cell->size - size;
    if (leftover < MINOBJSIZE) {
        hp->free = DLExtract(hp->free, cell);
    } else {
        cell->size = size;
        rest = (Cell *)((u8 *)cell + size);
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
    hp->allocated = DLAddFront(hp->allocated, cell);
    OS_RestoreInterrupts(irq);
    return (u8 *)cell + HEADERSIZE;
}

// NitroSDK's OS_FreeToHeap: gives memory back to the heap (the arena's current one when negative)
void OS_FreeToHeap(OSArenaId arena, OSHeapHandle heap, void *ptr) {
    OSHeapInfo *info;
    HeapDesc *hp;
    Cell *cell;
    OSIntrMode irq = OS_DisableInterrupts();

    info = OSiHeapInfo[arena];
    if (heap < 0) {
        heap = info->currentHeap;
    }
    hp = &info->heapArray[heap];
    cell = (Cell *)((u8 *)ptr - HEADERSIZE);
    hp->allocated = DLExtract(hp->allocated, cell);
    hp->free = DLInsert(hp->free, cell);
    OS_RestoreInterrupts(irq);
}

// NitroSDK's OS_InitAlloc: makes the arena's table of heaps, with room for the number of them, at
// the start of the memory that it is given, and returns where the memory that is left starts
void *OS_InitAlloc(OSArenaId arena, void *start, void *end, int maxHeaps) {
    OSHeapInfo *info = start;
    u32 heapsSize;
    OSIntrMode irq = OS_DisableInterrupts();
    int i;

    OSiHeapInfo[arena] = info;
    heapsSize = maxHeaps * sizeof(HeapDesc);
    info->heapArray = (HeapDesc *)((u8 *)info + sizeof(OSHeapInfo));
    info->numHeaps = maxHeaps;
    for (i = 0; i < info->numHeaps; i++) {
        HeapDesc *hp = &info->heapArray[i];

        hp->size = -1;
        hp->free = hp->allocated = NULL;
    }
    info->currentHeap = -1;
    info->arenaStart = (void *)ROUND((u32)info->heapArray + heapsSize);
    end = (void *)TRUNC(end);
    info->arenaEnd = end;
    OS_RestoreInterrupts(irq);
    return info->arenaStart;
}

// NitroSDK's OS_CreateHeap: makes a heap of the memory from start to end (rounded in to 0x20), and
// returns its number, or -1 when the arena has no room for another heap
OSHeapHandle OS_CreateHeap(OSArenaId arena, void *start, void *end) {
    OSHeapInfo *info;
    HeapDesc *hp;
    Cell *cell;
    int heap;
    OSIntrMode irq = OS_DisableInterrupts();

    info = OSiHeapInfo[arena];
    start = (void *)ROUND(start);
    end = (void *)TRUNC(end);
    for (heap = 0; heap < info->numHeaps; heap++) {
        hp = &info->heapArray[heap];
        if (hp->size < 0) {
            hp->size = (u8 *)end - (u8 *)start;
            cell = start;
            cell->prev = NULL;
            cell->next = NULL;
            cell->size = hp->size;
            hp->free = cell;
            hp->allocated = NULL;
            OS_RestoreInterrupts(irq);
            return heap;
        }
    }
    OS_RestoreInterrupts(irq);
    return -1;
}

// On a DS, makes a heap of the 16 KB at the start of the main memory (which nothing else uses before the program)
// once, in the first heap of the arena that is free. Gives the heap's number, or -1
OSHeapHandle OS_CreateExtraHeap(OSArenaId arena) {
    OSHeapInfo *info;
    HeapDesc *hp;
    Cell *cell;
    int heap;
    OSIntrMode irq = OS_DisableInterrupts();

    info = OSiHeapInfo[arena];
    if (!OS_IsRunOnTwl() && sExtraHeap.heap < 0) {
        for (heap = 0; heap < info->numHeaps; heap++) {
            hp = &info->heapArray[heap];
            if (hp->size < 0) {
                hp->size = 0x4000;
                cell = (Cell *)0x02000000;
                cell->prev = NULL;
                cell->next = NULL;
                cell->size = 0x4000;
                hp->free = cell;
                hp->allocated = NULL;
                sExtraHeap.arena = arena;
                sExtraHeap.heap = heap;
                OS_RestoreInterrupts(irq);
                return heap;
            }
        }
    }
    OS_RestoreInterrupts(irq);
    return -1;
}

// The usable size of an allocated block, which MSL's __sys_pointer_size asks for
u32 OS_ReferentSize(OSArenaId arena, void *ptr) {
    OSIntrMode irq = OS_DisableInterrupts();

    OS_RestoreInterrupts(irq);
    return ((Cell *)((u8 *)ptr - HEADERSIZE))->size - HEADERSIZE;
}
