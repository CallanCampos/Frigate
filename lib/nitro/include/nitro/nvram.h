#ifndef POKEBW2_NITRO_NVRAM_H
#define POKEBW2_NITRO_NVRAM_H

#include "types.h"

// NitroSDK's NVRAM library: the ARM9's access to the console's NVRAM (the flash of the firmware, which keeps the Wi-Fi
// settings), through the PXI FIFO and the ARM7. The functions have the names of the SDK functions they are, from how
// the code works

// The result of a call, as the callback gets it and the synchronous functions return it. The names are by where each
// is produced: ILLEGAL_STATUS also when the library is not initialized, WRITE_ERROR when the flash's status register
// shows its error bit after a page was written, and VERIFY_ERROR when the data read back differs
typedef enum NVRAMResult {
    NVRAM_RESULT_SUCCESS,
    NVRAM_RESULT_BUSY,
    NVRAM_RESULT_ILLEGAL_PARAMETER,
    NVRAM_RESULT_SEND_ERROR,
    NVRAM_RESULT_ILLEGAL_STATUS,
    NVRAM_RESULT_WRITE_ERROR,
    NVRAM_RESULT_VERIFY_ERROR,
    NVRAM_RESULT_FATAL_ERROR,
} NVRAMResult;

typedef void (*NVRAMCallback)(NVRAMResult result, void *arg);

// Starts the library, which waits for the ARM7 to be ready. Does nothing once started
void NVRAM_Init(void);

// NVRAM_ReadDataBytesAsync: reads size bytes of the NVRAM from address into buf, and calls back with the result when
// it is done. The result of the call itself is BUSY while another call runs, SEND_ERROR when the FIFO is full, and
// ILLEGAL_STATUS before NVRAM_Init has finished; with SUCCESS the callback is still to come
NVRAMResult NVRAM_ReadDataBytesAsync(u32 address, u32 size, void *buf, NVRAMCallback callback, void *arg);
// The same, waiting for the end; it has to be called from a thread
NVRAMResult NVRAM_ReadDataBytes(u32 address, u32 size, void *buf);

// NVRAM_WriteDataBytesAsync: writes size bytes from src to the NVRAM at address, a page at a time, waiting for the
// flash to finish each page, and calls back with the result
NVRAMResult NVRAM_WriteDataBytesAsync(u32 address, u32 size, const void *src, NVRAMCallback callback, void *arg);
// The same, waiting for the end
NVRAMResult NVRAM_WriteDataBytes(u32 address, u32 size, const void *src);

// NVRAM_VerifyDataBytesAsync: compares size bytes of the NVRAM from address with src, 32 bytes at a time, and calls
// back with VERIFY_ERROR when they differ
NVRAMResult NVRAM_VerifyDataBytesAsync(u32 address, u32 size, const void *src, NVRAMCallback callback, void *arg);
// The same, waiting for the end
NVRAMResult NVRAM_VerifyDataBytes(u32 address, u32 size, const void *src);

#endif // POKEBW2_NITRO_NVRAM_H
