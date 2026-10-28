// Experimental CARD DMA unit ownership for NitroSDK mi_dma_card.c: fixed source, repeat count 1, advancing output.
// Bounds/native provenance are explicit. Per-unit 9 normalized cycles is the pinned IO32->mainRAM model class;
// no general CPU-work, unknown-destination timing or ordinary DMA IRQ is supplied by this module.
#include "hal/dma/card_dma.h"
#include "hal/os/port_os.h"
#include <stdint.h>
#include <string.h>

static PortCardDmaState sState;
static uint8_t *sDest;

bool port_card_dma_arm_main(unsigned channel, void *dest, size_t bytes) {
    if (channel > 3 || sState.armed || sState.servicing || dest == NULL || bytes == 0 || (bytes & 3) != 0 ||
        ((uintptr_t)dest & 31) != 0 || !port_os_main_arena_holds(dest, bytes) || sState.generation == UINT64_MAX) {
        return false;
    }
    ++sState.generation;
    sState.channel = channel;
    sState.armed = true;
    sState.servicing = false;
    sState.remaining = bytes;
    sState.written = 0;
    sDest = dest;
    return true;
}
void port_card_dma_stop(unsigned channel) {
    if (sState.armed && sState.channel == channel) {
        sState.armed = false;
        sState.servicing = false;
        sDest = NULL;
        if (sState.generation != UINT64_MAX) {
            ++sState.generation;
        }
    }
}
void port_card_dma_reset(void) {
    // Reset invalidates external/in-flight tokens too. Never reuse an epoch: exhaustion stays fail-closed.
    uint64_t generation = sState.generation;
    if (generation != UINT64_MAX) {
        ++generation;
    }
    memset(&sState, 0, sizeof(sState));
    sState.generation = generation;
    sDest = NULL;
}
PortCardDmaState port_card_dma_state(void) {
    return sState;
}
bool port_card_dma_demand(uint64_t *generation) {
    if (!sState.armed || sState.servicing || sState.remaining < 4 || generation == NULL) {
        return false;
    }
    sState.servicing = true;
    *generation = sState.generation;
    return true;
}
bool port_card_dma_commit(uint64_t generation, uint32_t value) {
    if (!sState.armed || !sState.servicing || sState.generation != generation || sState.remaining < 4 ||
        sDest == NULL) {
        return false;
    }
    for (unsigned i = 0; i < 4; ++i) {
        sDest[i] = (uint8_t)(value >> (i * 8));
    }
    sDest += 4;
    sState.remaining -= 4;
    sState.written += 4;
    sState.servicing = false;
    // Repeat remains armed after the bounded range is exhausted; the CARD completion handler must stop it.
    return true;
}
