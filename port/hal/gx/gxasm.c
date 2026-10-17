// The GX functions the SDK wrote in assembly, on the host: GX_SendFifo48B and GX_SendFifo64B (gxasm.c, which
// port/hal/replaced.txt keeps out of the host build) and GXi_NopClearFifo128_ (g3x.c, where the asm is behind
// #ifndef PLATFORM_PC). Each stores words to the geometry FIFO port with STMs that have no write-back, so every STM
// starts again at the port; GXFIFO's 64 bytes (0x04000400-0x0400043f) all reach the FIFO, a word per store.
//
// Every caller passes &reg_G3X_GXFIFO (g3imm.c's G3_LoadMtx44 and the other matrix commands, G3X_ClearFifo). On the
// host that expression is a fresh entry of port/hal/hw's geometry command log, which holds one word; port_hw_g3_send
// takes it back and logs each word the STMs store as its own write of GXFIFO, in order. For a command port instead of
// GXFIFO the DS's STMs would spread the words over that port and the next ones, which this doesn't reproduce: no caller
// does that. A dest that isn't a log entry is memory, and gets the stores as the DS makes them
#include "hal/hw/hw.h"
#include "nitro/gx.h"

// Stores count words of src, in STMs of perStore words that each start again at dest, as the SDK's loops do
static void send_words(const void *src, volatile void *dest, u32 count, u32 perStore) {
    const u32 *words = src;
    volatile u32 *port = dest;

    if (port_hw_g3_send(dest, src, count, 0)) {
        return;
    }
    for (u32 i = 0; i < count; i++) {
        port[i % perStore] = words[i];
    }
}

// The SDK's 4x3 matrix copy: four LDM/STM pairs of three words
void GX_SendFifo48B(const void *src, volatile void *dest) {
    send_words(src, dest, 12, 3);
}

// The SDK's 4x4 matrix copy: two LDM/STM pairs of eight words
void GX_SendFifo64B(const void *src, volatile void *dest) {
    send_words(src, dest, 16, 8);
}

// The SDK's 128 words of 0 to the FIFO port, which push out whatever it still holds: 32 STMs of four words
void GXi_NopClearFifo128_(void *fifo) {
    static const u32 zeros[128];

    send_words(zeros, fifo, 128, 4);
}
