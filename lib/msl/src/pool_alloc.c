// MSL C's pool_alloc.c (the name is a guess from the ARM platform group's pool allocator hooks): the three functions
// through which the heap functions of alloc.c reach the memory. Here they pass the main arena's current heap to the OS.
#include "pool_alloc.h"
#include "nitro/os.h"

void *__sys_alloc(size_t size) {
    return OS_AllocFromHeap(OS_ARENA_MAIN, OS_CURRENT_HEAP_HANDLE, size);
}

void __sys_free(void *ptr) {
    OS_FreeToHeap(OS_ARENA_MAIN, OS_CURRENT_HEAP_HANDLE, ptr);
}

size_t __sys_pointer_size(void *ptr) {
    return OS_ReferentSize(OS_ARENA_MAIN, ptr);
}
