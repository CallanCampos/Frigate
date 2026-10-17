// The virtual memory calls the aliasing backends need, one set per OS. Everything else in vram.c is plain C
#ifndef VRAM_OS_H
#define VRAM_OS_H

#include "vram.h"

// The object that holds all nine banks (VRAM_BANKS_SIZE bytes, in LCDC order) and a window that maps all of it once
typedef struct VramOs {
    VramBackend backend;
    int fd;         // VRAM_BACKEND_SHM
    unsigned entry; // VRAM_BACKEND_MACH_ENTRY: a mach_port_t
    uint8_t *window;
    uint32_t calls;
} VramOs;

size_t vram_os_page_size(void);
bool vram_os_backend_available(VramBackend backend);

// A range with no access, for the views and the guard pages between them
int vram_os_reserve(size_t size, uint8_t **out);
void vram_os_release(uint8_t *addr, size_t size);

int vram_os_banks_create(VramOs *os);
void vram_os_banks_destroy(VramOs *os);

// Replaces whatever is at addr with len bytes of the banks from offset, shared with every other mapping of them
int vram_os_map_banks(VramOs *os, uint8_t *addr, size_t len, size_t offset);
// Replaces whatever is at addr with fresh private zero pages, readable and writable or with no access
int vram_os_map_anon(VramOs *os, uint8_t *addr, size_t len, bool accessible);

#endif // VRAM_OS_H
