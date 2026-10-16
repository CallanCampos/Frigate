// MSL C's pool_alloc.c (the name is a guess from the ARM platform group's pool allocator hooks): the three functions
// through which the heap functions of alloc.c reach the memory. Here they pass the main arena's current heap to the OS.
#include "pool_alloc.h"
#include "nitro/os.h"

// NitroSDK's OS_CURRENT_HEAP_HANDLE: the heap that the arena uses for allocations
#define CURRENT_HEAP (-1)

void *__sys_alloc(size_t size) {
    return malloc_device(OS_ARENA_MAIN, CURRENT_HEAP, size);
}

void __sys_free(void *ptr) {
    free_device(OS_ARENA_MAIN, CURRENT_HEAP, ptr);
}

size_t __sys_pointer_size(void *ptr) {
    return func_0207b730(OS_ARENA_MAIN, ptr);
}
