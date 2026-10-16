// MSL C's alloc.c: the heap functions of <stdlib.h>. They take their memory from the system hooks of pool_alloc.c, and
// have no lock of their own.
#include "types.h"
#include "pool_alloc.h"
#include <stdlib.h>
#include <string.h>

void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    return __sys_alloc(size);
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    __sys_free(ptr);
}

// Moves the block into a new one when it has to change, copying the smaller of the two sizes
void *realloc(void *ptr, size_t size) {
    void *newPtr;
    size_t oldSize;

    if (ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    oldSize = __sys_pointer_size(ptr);
    newPtr = malloc(size);
    if (newPtr != NULL) {
        if (oldSize >= size) {
            oldSize = size;
        }
        memcpy(newPtr, ptr, oldSize);
        free(ptr);
    }
    return newPtr;
}

// The size of the whole block is the one that the memory is cleared for
void *calloc(size_t nmemb, size_t size) {
    void *ptr;

    size *= nmemb;
    ptr = malloc(size);
    if (ptr != NULL) {
        memset(ptr, 0, size);
    }
    return ptr;
}
