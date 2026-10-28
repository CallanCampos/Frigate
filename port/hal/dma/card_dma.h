// Bounded experimental CARD-ready DMA channel, beneath NitroSDK mi_dma_card.c's repeat-one-word setup.
// Not selected by CARDi_ReadRom yet. Only known native main-arena ranges have the pinned IO32->mainRAM service class.
#ifndef PORT_CARD_DMA_H
#define PORT_CARD_DMA_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define PORT_CARD_DMA_MAIN_WORD_CYCLES 9u

typedef struct {
    bool armed, servicing;
    unsigned channel;
    size_t remaining, written;
    uint64_t generation;
} PortCardDmaState;
bool port_card_dma_arm_main(unsigned channel, void *dest, size_t bytes);
void port_card_dma_stop(unsigned channel);
void port_card_dma_reset(void);
PortCardDmaState port_card_dma_state(void);
// Closed hardware path only: reserve a demand token, then commit exactly one little-endian word if still current.
bool port_card_dma_demand(uint64_t *generation);
bool port_card_dma_commit(uint64_t generation, uint32_t value);
#endif
