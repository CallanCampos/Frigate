#include "dwc/memfunc.h"
#include "nitro/mi.h"

// DWC's memory functions (TWL-DWC 5.3's dwc_memfunc.c, a guessed name; the ROM names none of it, and the file is ARM
// code in ARM9 main like the rest of the library's base): the library's allocation on top of the game's allocator and
// deallocator, with a header before every block that keeps its size, and the four functions that it gives GameSpy.

// The magic number of a block's header, 'DWCM'
#define DWCi_MEMFUNC_SIGNATURE 0x4457434d
// The alignment of a block that has none given
#define DWC_MEM_ALIGN 32

typedef struct DWCiAllocHeader {
    u32 signature;
    // The size of the block that the caller asked for, without this header
    u32 size;
    u8 dummy[0x18];
} DWCiAllocHeader;

typedef struct DWCMemFuncs {
    DWCFreeEx free;
    DWCAllocEx alloc;
} DWCMemFuncs;

static DWCMemFuncs sMemFuncs;

static void *DWC_ReallocEx(DWCAllocType name, void *ptr, u32 oldSize, u32 newSize, int align);
static DWCiAllocHeader *DWCi_GetAllocateHeader(void *ptr);
static void *DWCi_SetAllocateHeader(DWCiAllocHeader *header, u32 size);
static u32 DWCi_GetAllocateSize(void *ptr);
static void *DWC_Realloc(DWCAllocType name, void *ptr, u32 oldSize, u32 newSize);

// The header of a block
static DWCiAllocHeader *DWCi_GetAllocateHeader(void *ptr) {
    return (DWCiAllocHeader *)ptr - 1;
}

// Makes the header of a block that the allocator gave, and returns the block that the caller gets
static void *DWCi_SetAllocateHeader(DWCiAllocHeader *header, u32 size) {
    header->size = size;
    header->signature = DWCi_MEMFUNC_SIGNATURE;
    return header + 1;
}

// The size of a block
static u32 DWCi_GetAllocateSize(void *ptr) {
    return DWCi_GetAllocateHeader(ptr)->size;
}

void DWC_SetMemFunc(DWCAllocEx alloc, DWCFreeEx free) {
    sMemFuncs.alloc = alloc;
    sMemFuncs.free = free;
}

void *DWC_Alloc(DWCAllocType name, u32 size) {
    return DWC_AllocEx(name, size, DWC_MEM_ALIGN);
}

void *DWC_AllocEx(DWCAllocType name, u32 size, int align) {
    DWCiAllocHeader *header = sMemFuncs.alloc(name, size + sizeof(DWCiAllocHeader), align);

    if (header == NULL) {
        return NULL;
    }
    return DWCi_SetAllocateHeader(header, size);
}

void DWC_Free(DWCAllocType name, void *ptr, u32 size) {
    DWCiAllocHeader *header;

    if (ptr == NULL) {
        return;
    }
    header = DWCi_GetAllocateHeader(ptr);
    sMemFuncs.free(name, header, header->size + sizeof(DWCiAllocHeader));
}

// Gives a block another size, keeping what fits of its contents. The old size is not read: the header has it
static void *DWC_Realloc(DWCAllocType name, void *ptr, u32 oldSize, u32 newSize) {
    return DWC_ReallocEx(name, ptr, oldSize, newSize, DWC_MEM_ALIGN);
}

static void *DWC_ReallocEx(DWCAllocType name, void *ptr, u32 oldSize, u32 newSize, int align) {
    void *newPtr = DWC_AllocEx(name, newSize, align);

    if (newPtr == NULL) {
        return NULL;
    }
    if (ptr != NULL) {
        oldSize = DWCi_GetAllocateSize(ptr);
        MI_CpuCopy8(ptr, newPtr, oldSize <= newSize ? oldSize : newSize);
        DWC_Free(name, ptr, oldSize);
    }
    return newPtr;
}

void *DWCi_GsMalloc(size_t size) {
    return DWC_Alloc(DWC_ALLOCTYPE_GS, size);
}

void *DWCi_GsRealloc(void *ptr, size_t size) {
    return DWC_Realloc(DWC_ALLOCTYPE_GS, ptr, size, size);
}

void DWCi_GsFree(void *ptr) {
    DWC_Free(DWC_ALLOCTYPE_GS, ptr, 0);
}

void *DWCi_GsMemalign(size_t align, size_t size) {
    return DWC_AllocEx(DWC_ALLOCTYPE_GS, size, align);
}
