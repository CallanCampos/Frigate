// NitroSDK's STD_GetStringLength (lib/nitro/src/std/std_string.c), called through its actual nitro/std.h interface.
// R10: the DS's aligned word load reads outside a short string's C object. Exact-sized allocations and literals
// expose that load under ASan; all four starting alignments, high-bit bytes and strings ending at an inaccessible
// page check that the host reads only through the terminator. Plain and sanitized targets compile the actual SDK C.
#include "nitro/std.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// Keep the call through the SDK interface, rather than let the caller fold a known literal's length.
static int (*volatile StringLength)(const char *) = STD_GetStringLength;
static int sFailures;
static const char sShortLiteral[11] = "ABCDEFGHIJ";
static const char sEmptyLiteral[1] = "";

static void CheckLength(const char *str, int expected) {
    int actual = StringLength(str);

    if (actual != expected) {
        fprintf(stderr, "STD_GetStringLength: expected %d, got %d\n", expected, actual);
        sFailures++;
    }
}

static void TestObjectBounds(void) {
    CheckLength(sEmptyLiteral, 0);
    CheckLength(sShortLiteral, 10);
    for (int alignment = 0; alignment < 4; alignment++) {
        for (int length = 0; length <= 33; length++) {
            // The allocation ends exactly after the NUL; ASan poisons the following bytes, even within a page.
            char *allocation = malloc((size_t)(alignment + length + 1));
            char *str;

            if (allocation == NULL) {
                fputs("STD_GetStringLength: allocation failed\n", stderr);
                sFailures++;
                return;
            }
            memset(allocation, 0xa5, (size_t)(alignment + length + 1));
            str = allocation + alignment;
            str[length] = '\0';
            CheckLength(str, length);
            free(allocation);
        }
    }
}

static void TestPageBounds(void) {
    size_t pageSize;
    char *pages;
    char *readable;
#ifdef _WIN32
    SYSTEM_INFO info;
    DWORD previous;

    GetSystemInfo(&info);
    pageSize = info.dwPageSize;
    pages = VirtualAlloc(NULL, pageSize * 3, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
    if (pages == NULL || !VirtualProtect(pages + pageSize, pageSize, PAGE_READWRITE, &previous)) {
        fputs("STD_GetStringLength: guarded page allocation failed\n", stderr);
        sFailures++;
        if (pages != NULL) {
            VirtualFree(pages, 0, MEM_RELEASE);
        }
        return;
    }
#else
    long systemPageSize = sysconf(_SC_PAGESIZE);

    if (systemPageSize <= 0) {
        fputs("STD_GetStringLength: page size unavailable\n", stderr);
        sFailures++;
        return;
    }
    pageSize = (size_t)systemPageSize;
    pages = mmap(NULL, pageSize * 3, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (pages == MAP_FAILED) {
        fputs("STD_GetStringLength: guarded page allocation failed\n", stderr);
        sFailures++;
        return;
    }
    if (mprotect(pages + pageSize, pageSize, PROT_READ | PROT_WRITE) != 0) {
        fputs("STD_GetStringLength: guarded page protection failed\n", stderr);
        sFailures++;
        munmap(pages, pageSize * 3);
        return;
    }
#endif
    readable = pages + pageSize;
    for (int length = 0; length <= 33; length++) {
        char *str = readable + pageSize - (size_t)length - 1;

        memset(str, 0x80, (size_t)length);
        str[length] = '\0';
        CheckLength(str, length);
    }
#ifdef _WIN32
    if (!VirtualFree(pages, 0, MEM_RELEASE)) {
#else
    if (munmap(pages, pageSize * 3) != 0) {
#endif
        fputs("STD_GetStringLength: guarded page release failed\n", stderr);
        sFailures++;
    }
}

int main(void) {
    TestObjectBounds();
    TestPageBounds();
    return sFailures != 0;
}
