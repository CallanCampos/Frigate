// What the GX files of port/hal/gx share
#ifndef PORT_HAL_GX_GX_INTERNAL_H
#define PORT_HAL_GX_GX_INTERNAL_H

#include <stdint.h>

// The copy a GX load starts on GX's DMA channel (or with the CPU when it has none), made at once: on the host a DMA
// completes as it starts (PLAN.md §4.5), so the End that waits for it has nothing to wait for. dest is the DS address
// the SDK computes, in the CPU's view of the banks the Begin took, and the copy goes where a CPU store to it would
// (port_vram_cpu_pointer), in words as the DMA moves them: size & ~3 bytes. The pages it reaches get new generations
void port_gx_load_copy(uint32_t dest, const void *src, uint32_t size);

#endif // PORT_HAL_GX_GX_INTERNAL_H
