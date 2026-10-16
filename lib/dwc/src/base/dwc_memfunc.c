#include "dwc/memfunc.h"
#include "nitro/mi.h"

// DWC's memory functions (TWL-DWC 5.3's dwc_memfunc.c, a guessed name; the ROM names none of it, and the file is ARM
// code in ARM9 main like the rest of the library's base): the library's allocation on top of the game's allocator and
// deallocator, with a header before every block that keeps its size, and the four functions that it gives GameSpy.

// The magic number of a block's header, 'DWCM'
#define DWC_MEM_MAGIC 0x4457434d
// The name that GameSpy's blocks are asked for with
#define DWC_MEM_GAMESPY 9
// The alignment of a block that has none given
#define DWC_MEM_ALIGN 32

typedef struct DWCMemHeader {
    u32 magic;
    // The size of the block that the caller asked for, without this header
    u32 size;
    u8 unk8[0x18];
} DWCMemHeader;

typedef struct DWCMemFuncs {
    DWCFreeFunc free;
    DWCAllocFunc alloc;
} DWCMemFuncs;

static DWCMemFuncs sMemFuncs;

static void *DWC_ReallocEx(u32 name, void *ptr, u32 oldSize, u32 newSize, int align);

// The header of a block
static DWCMemHeader *DWCi_GetMemHeader(void *ptr) {
    return (DWCMemHeader *)ptr - 1;
}

// Makes the header of a block that the allocator gave, and returns the block that the caller gets
static void *DWCi_InitMemHeader(DWCMemHeader *header, u32 size) {
    header->size = size;
    header->magic = DWC_MEM_MAGIC;
    return header + 1;
}

// The size of a block
static u32 DWCi_GetMemSize(void *ptr) {
    return DWCi_GetMemHeader(ptr)->size;
}

void DWCi_SetMemFunc(DWCAllocFunc alloc, DWCFreeFunc free) {
    sMemFuncs.alloc = alloc;
    sMemFuncs.free = free;
}

void *DWC_Alloc(u32 name, u32 size) {
    return DWC_AllocEx(name, size, DWC_MEM_ALIGN);
}

void *DWC_AllocEx(u32 name, u32 size, int align) {
    DWCMemHeader *header = sMemFuncs.alloc(name, size + sizeof(DWCMemHeader), align);

    if (header == NULL) {
        return NULL;
    }
    return DWCi_InitMemHeader(header, size);
}

void DWC_Free(u32 name, void *ptr, u32 size) {
    DWCMemHeader *header;

    if (ptr == NULL) {
        return;
    }
    header = DWCi_GetMemHeader(ptr);
    sMemFuncs.free(name, header, header->size + sizeof(DWCMemHeader));
}

// Gives a block another size, keeping what fits of its contents. The old size is not read: the header has it
static void *DWC_Realloc(u32 name, void *ptr, u32 oldSize, u32 newSize) {
    return DWC_ReallocEx(name, ptr, oldSize, newSize, DWC_MEM_ALIGN);
}

static void *DWC_ReallocEx(u32 name, void *ptr, u32 oldSize, u32 newSize, int align) {
    void *newPtr = DWC_AllocEx(name, newSize, align);
    u32 blockSize;
    u32 copySize;

    if (newPtr == NULL) {
        return NULL;
    }
    if (ptr != NULL) {
        blockSize = DWCi_GetMemSize(ptr);
        copySize = newSize;
        if (blockSize <= copySize) {
            copySize = blockSize;
        }
        MI_CpuCopy8(ptr, newPtr, copySize);
        DWC_Free(name, ptr, blockSize);
    }
    return newPtr;
}

void *DWCi_GsAlloc(u32 size) {
    return DWC_Alloc(DWC_MEM_GAMESPY, size);
}

void *DWCi_GsRealloc(void *ptr, u32 size) {
    return DWC_Realloc(DWC_MEM_GAMESPY, ptr, size, size);
}

void DWCi_GsFree(void *ptr) {
    DWC_Free(DWC_MEM_GAMESPY, ptr, 0);
}

void *DWCi_GsMemalign(u32 align, u32 size) {
    return DWC_AllocEx(DWC_MEM_GAMESPY, size, align);
}
