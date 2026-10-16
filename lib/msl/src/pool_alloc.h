#ifndef POKEBW2_MSL_POOL_ALLOC_H
#define POKEBW2_MSL_POOL_ALLOC_H

#include <stddef.h>

// The memory that the C library's heap takes from the system (pool_alloc.c): here NitroSDK's OS heap of the main arena.
// The names are MSL's own (the library's private header)
void *__sys_alloc(size_t size);
void __sys_free(void *ptr);
size_t __sys_pointer_size(void *ptr);

#endif // POKEBW2_MSL_POOL_ALLOC_H
