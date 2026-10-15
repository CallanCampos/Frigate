#ifndef POKEBW2_NITRO_PXI_H
#define POKEBW2_NITRO_PXI_H

#include "types.h"

// NitroSDK's PXI, the FIFO between the ARM9 and the ARM7. Each word carries a tag, which picks the function that the
// receiving side calls with it, and an error bit. func_0207ca48 (nitro/os.h) tells whether the ARM7 is ready for a tag

typedef void (*PXIFifoCallback)(u32 fifoTag, u32 data, BOOL err);

// Sets up the FIFO
void PXI_Init(void);
// Sets the function called with the words the ARM7 sends with the tag, and marks the tag ready for the ARM7 in the
// shared memory; NULL clears both (PXI_SetFifoRecvCallback's code)
void func_0207c9fc(u32 fifoTag, PXIFifoCallback callback);
// Sends a word with the tag. Negative when the FIFO is full or in error
int PXI_SendWordByFifo(u32 fifoTag, u32 data, BOOL err);

#endif // POKEBW2_NITRO_PXI_H
