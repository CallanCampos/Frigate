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

static void *func_02058778(u32 name, void *ptr, u32 oldSize, u32 newSize, int align);

// The header of a block
static DWCMemHeader *func_020586a4(void *ptr) {
    return (DWCMemHeader *)ptr - 1;
}

// Makes the header of a block that the allocator gave, and returns the block that the caller gets
static void *func_020586ac(DWCMemHeader *header, u32 size) {
    header->size = size;
    header->magic = DWC_MEM_MAGIC;
    return header + 1;
}

// The size of a block
static u32 func_020586c0(void *ptr) {
    return func_020586a4(ptr)->size;
}

void func_020586d0(DWCAllocFunc alloc, DWCFreeFunc free) {
    sMemFuncs.alloc = alloc;
    sMemFuncs.free = free;
}

void *func_020586e4(u32 name, u32 size) {
    return func_020586f4(name, size, DWC_MEM_ALIGN);
}

void *func_020586f4(u32 name, u32 size, int align) {
    DWCMemHeader *header = sMemFuncs.alloc(name, size + sizeof(DWCMemHeader), align);

    if (header == NULL) {
        return NULL;
    }
    return func_020586ac(header, size);
}

void func_02058728(u32 name, void *ptr, u32 size) {
    DWCMemHeader *header;

    if (ptr == NULL) {
        return;
    }
    header = func_020586a4(ptr);
    sMemFuncs.free(name, header, header->size + sizeof(DWCMemHeader));
}

// Gives a block another size, keeping what fits of its contents. The old size is not read: the header has it
static void *func_02058764(u32 name, void *ptr, u32 oldSize, u32 newSize) {
    return func_02058778(name, ptr, oldSize, newSize, DWC_MEM_ALIGN);
}

static void *func_02058778(u32 name, void *ptr, u32 oldSize, u32 newSize, int align) {
    void *newPtr = func_020586f4(name, newSize, align);
    u32 blockSize;
    u32 copySize;

    if (newPtr == NULL) {
        return NULL;
    }
    if (ptr != NULL) {
        blockSize = func_020586c0(ptr);
        copySize = newSize;
        if (blockSize <= copySize) {
            copySize = blockSize;
        }
        sys_memcpy(ptr, newPtr, copySize);
        func_02058728(name, ptr, blockSize);
    }
    return newPtr;
}

void *func_020587e4(u32 size) {
    return func_020586e4(DWC_MEM_GAMESPY, size);
}

void *func_020587f8(void *ptr, u32 size) {
    return func_02058764(DWC_MEM_GAMESPY, ptr, size, size);
}

void func_02058814(void *ptr) {
    func_02058728(DWC_MEM_GAMESPY, ptr, 0);
}

void *func_0205882c(u32 align, u32 size) {
    return func_020586f4(DWC_MEM_GAMESPY, size, align);
}
