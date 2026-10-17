// A stand-in for the UCRT's stdlib.h, for the Windows compile check in CMakeLists.txt: only what the spike uses. Like
// the UCRT it has no aligned_alloc ("The Universal CRT doesn't implement C11 aligned_alloc, but does provide
// _aligned_malloc and _aligned_free", Microsoft C/C++ language conformance), so a call to it fails here too
#ifndef WINDOWS_STUB_STDLIB_H
#define WINDOWS_STUB_STDLIB_H

#include <stddef.h>

void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void free(void *p);
_Noreturn void abort(void);
void *_aligned_malloc(size_t size, size_t alignment);
void _aligned_free(void *p);

#endif
