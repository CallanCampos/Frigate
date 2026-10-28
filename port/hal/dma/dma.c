// NitroSDK's DMA on the host (PLAN.md §4.5: "DMA is memcpy"): the functions of lib/nitro/src/mi/mi_dma.c,
// mi_dma_itcm.c, mi_dma_card.c and mi_dma_gxcommand.c, which this module replaces (port/hal/dma/replaced.txt).
//
// The SDK's versions pass every address through the channel's registers as a u32, which cuts a host pointer, and then
// spin on the channel's enable bit, which only the hardware clears. Here a transfer is done by the CPU at once, in the
// order the DMA moves its units (from the first to the last, so an overlapping copy behaves as the DS's does), and a
// generic channel is never busy. The separate card_dma.c physical milestone has test-only repeat-channel
// ownership; production CARD does not select it yet. Generic operations cannot steal that armed channel.
//
// A transfer that the DS ends with an interrupt (the Async functions with a callback, and the display list sends) ends
// with the same interrupt here: the callback is entered as the SDK enters it (OSi_EnterDmaCallback), the channel's
// interrupt is requested from the OS layer and taken before the call returns, or when interrupts are enabled again if
// they are off now (port_os_poll_irq; the fibers spike's rule for completions that the caller may busy-wait on).
//
// The geometry engine's command FIFO and ports are reached through port_hw_g3_send, so that each word sent to them is
// logged for the geometry engine as a CPU store would be (ADR 0004).
#include "hal/dma/card_dma.h"
#include "hal/hw/hw.h"
#include "hal/os/port_os.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include <stdio.h>
#include <stdlib.h>

// Experimental CARD hardware has explicit channel ownership. Generic immediate copies must not silently steal it;
// the future SDK integration will supply faithful busy/wait arbitration before enabling this path in production.
static void CheckCardOwnership(u32 channel) {
    PortCardDmaState card = port_card_dma_state();
    if (card.armed && card.channel == channel) {
        fprintf(stderr, "port: dma: generic operation conflicts with experimental CARD DMA channel ownership\n");
        exit(PORT_OS_TERMINATE_STATUS);
    }
}

// Requests the channel's interrupt for a callback the SDK entered for it, and takes it if it can be taken now
static void EndWithInterrupt(u32 dmaNo, MIDmaCallback callback, void *arg) {
    OSi_EnterDmaCallback(dmaNo, callback, arg);
    port_os_request_irq(1u << (REG_OS_IE_D0_SHIFT + dmaNo));
    port_os_poll_irq();
}

static void Fill32(void *dest, u32 data, u32 size) {
    vu32 *to = dest;
    u32 count = size >> 2;

    while (count-- != 0) {
        *to++ = data;
    }
}

static void Copy32(const void *src, void *dest, u32 size) {
    const vu32 *from = src;
    vu32 *to = dest;
    u32 count = size >> 2;

    if (port_hw_g3_send(dest, src, count, sizeof(u32))) {
        return;
    }
    while (count-- != 0) {
        *to++ = *from++;
    }
}

static void Copy16(const void *src, void *dest, u32 size) {
    const vu16 *from = src;
    vu16 *to = dest;
    u32 count = size >> 1;

    while (count-- != 0) {
        *to++ = *from++;
    }
}

// NitroSDK's MIi_DmaFill32, MIi_DmaCopy32 and MIi_DmaCopy16. Without dmaEnable the SDK sets the channel up without
// starting it, and nothing in the program starts it later, so nothing moves
void MIi_DmaFill32(u32 dmaNo, void *dest, u32 data, u32 size, BOOL dmaEnable) {
    if (dmaEnable) {
        if (size != 0) {
            CheckCardOwnership(dmaNo);
        }
        Fill32(dest, data, size);
    }
}

void MIi_DmaCopy32(u32 dmaNo, const void *src, void *dest, u32 size, BOOL dmaEnable) {
    if (dmaEnable) {
        if (size != 0) {
            CheckCardOwnership(dmaNo);
        }
        Copy32(src, dest, size);
    }
}

void MIi_DmaCopy16(u32 dmaNo, const void *src, void *dest, u32 size, BOOL dmaEnable) {
    if (dmaEnable) {
        if (size != 0) {
            CheckCardOwnership(dmaNo);
        }
        Copy16(src, dest, size);
    }
}

// NitroSDK's MIi_DmaFill32Async and MIi_DmaCopy32Async: the callback comes from the channel's interrupt, or at once
// when there is nothing to move, as in the SDK
void MIi_DmaFill32Async(u32 dmaNo, void *dest, u32 data, u32 size, MIDmaCallback callback, void *arg, BOOL dmaEnable) {
    if (size == 0) {
        if (callback != NULL) {
            callback(arg);
        }
        return;
    }
    if (!dmaEnable) {
        return;
    }
    CheckCardOwnership(dmaNo);
    Fill32(dest, data, size);
    if (callback != NULL) {
        EndWithInterrupt(dmaNo, callback, arg);
    }
}

void MIi_DmaCopy32Async(u32 dmaNo, const void *src, void *dest, u32 size, MIDmaCallback callback, void *arg,
                        BOOL dmaEnable) {
    if (size == 0) {
        if (callback != NULL) {
            callback(arg);
        }
        return;
    }
    if (!dmaEnable) {
        return;
    }
    CheckCardOwnership(dmaNo);
    Copy32(src, dest, size);
    if (callback != NULL) {
        EndWithInterrupt(dmaNo, callback, arg);
    }
}

// Generic transfers finish before return. Experimental CARD Stop invalidates its armed generation; a generic
// Wait on that channel fails visibly until the actual SDK DMA lifecycle/wait integration is implemented.
void MI_WaitDma(u32 dmaNo) {
    CheckCardOwnership(dmaNo);
}

void MI_StopDma(u32 dmaNo) {
    port_card_dma_stop(dmaNo);
}

void MI_StopAllDma(void) {
    for (unsigned channel = 0; channel < 4; ++channel) {
        port_card_dma_stop(channel);
    }
}

// NitroSDK's MIi_CheckAnotherAutoDMA and MIi_CheckDma0SourceAddress, the SDK's guards against a clash of channels
// started by the hardware's events, and against DMA0 reading the I/O registers or the cartridge. No channel runs on its
// own here, and the source is a host pointer that the DS's areas say nothing about
void MIi_CheckAnotherAutoDMA(u32 dmaNo, u32 dmaType) {
}

void MIi_CheckDma0SourceAddress(u32 dmaNo, u32 src, u32 size, u32 dir) {
}

// NitroSDK's MIi_CardDmaCopy32, which CARD's table of DMA functions holds for reading the card's data port. The host's
// CPU adapter (port/hal/card/card_rom.c) still does not select DMA. Physical tests use the explicit bounded arm API.
void MIi_CardDmaCopy32(u32 dmaNo, const void *src, void *dest, u32 size) {
    fprintf(stderr, "port: dma: MIi_CardDmaCopy32 was called, but SDK CARD DMA setup is not integrated (experimental "
                    "typed hardware only)\n");
    exit(PORT_OS_TERMINATE_STATUS);
}

// NitroSDK's MI_SendGXCommandAsync and MI_SendGXCommandAsyncFast: a display list of length bytes to the geometry
// FIFO. The SDK sends it in parts as the FIFO drains, or all at once paced by the FIFO; the geometry engine's log takes
// it whole, and the callback comes from the channel's interrupt after the last word
static void SendGXCommand(u32 dmaNo, const void *src, u32 length, MIDmaCallback callback, void *arg) {
    if (length == 0) {
        if (callback != NULL) {
            callback(arg);
        }
        return;
    }
    // Evaluating the FIFO's register logs an entry and gives its slot, which port_hw_g3_send takes back for the list
    (void)port_hw_g3_send(&reg_G3X_GXFIFO, src, length >> 2, 0);
    if (callback != NULL) {
        EndWithInterrupt(dmaNo, callback, arg);
    }
}

void MI_SendGXCommandAsync(u32 dmaNo, const void *src, u32 length, MIDmaCallback callback, void *arg) {
    SendGXCommand(dmaNo, src, length, callback, arg);
}

void MI_SendGXCommandAsyncFast(u32 dmaNo, const void *src, u32 length, MIDmaCallback callback, void *arg) {
    SendGXCommand(dmaNo, src, length, callback, arg);
}
