// The virtual memory calls of the aliasing backends, one set per OS (port/spikes/vram-alias): they map the DS's nine
// VRAM banks, which hold every graphic the game draws (backgrounds, sprites, 3D textures and palettes), at more than
// one address, as the DS's bank mapping does. Everything else in vram.c is plain C
#ifndef PORT_HAL_GX_VRAM_OS_H
#define PORT_HAL_GX_VRAM_OS_H

#include "hal/gx/vram.h"

// The object that holds all nine banks (PORT_VRAM_BANKS_SIZE bytes, in LCDC order) and a window that maps all of it
typedef struct PortVramOs {
    PortVramBackend backend;
    int fd;         // PORT_VRAM_BACKEND_SHM
    unsigned entry; // PORT_VRAM_BACKEND_MACH_ENTRY: a mach_port_t
    uint8_t *window;
    uint32_t calls;
} PortVramOs;

size_t port_vram_os_page_size(void);
int port_vram_os_backend_available(PortVramBackend backend);

// Readable and writable zero pages anywhere, for the views only the GPU reads
int port_vram_os_allocate(size_t size, uint8_t **out);
void port_vram_os_release(uint8_t *addr, size_t size);

int port_vram_os_banks_create(PortVramOs *os);
void port_vram_os_banks_destroy(PortVramOs *os);

// Replaces whatever is at addr with len bytes of the banks from offset, shared with every other mapping of them
int port_vram_os_map_banks(PortVramOs *os, uint8_t *addr, size_t len, size_t offset);
// Replaces whatever is at addr with fresh private zero pages, readable and writable
int port_vram_os_map_zero(PortVramOs *os, uint8_t *addr, size_t len);

#endif // PORT_HAL_GX_VRAM_OS_H
