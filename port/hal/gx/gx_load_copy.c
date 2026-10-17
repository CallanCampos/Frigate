// The copy of a GX load (port/hal/gx/gx_internal.h)
#include "hal/gx/gx_internal.h"
#include "hal/gx/vram.h"
#include <string.h>

void port_gx_load_copy(uint32_t dest, const void *src, uint32_t size) {
    const uint8_t *from = src;
    uint32_t at = dest;
    uint32_t left = size & ~3u;

    // A slot at a time, since the DS's mirrors and the window's regions change only at slot boundaries
    while (left > 0) {
        uint32_t chunk = PORT_VRAM_SLOT_SIZE - (at & (PORT_VRAM_SLOT_SIZE - 1));
        if (chunk > left) {
            chunk = left;
        }
        memcpy(port_vram_cpu_pointer(at), from, chunk);
        at += chunk;
        from += chunk;
        left -= chunk;
    }
    port_vram_note_write(dest, size & ~3u);
}
