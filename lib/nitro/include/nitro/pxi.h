#ifndef POKEBW2_NITRO_PXI_H
#define POKEBW2_NITRO_PXI_H

#include "types.h"

// NitroSDK's PXI, the FIFO between the ARM9 and the ARM7. Each word carries a tag, which picks the function that the
// receiving side calls with it, and an error bit. func_0207ca48 (NitroSDK's PXI_IsCallbackReady) tells whether the
// ARM7 is ready for a tag

// The FIFO's registers: the four bits each processor sets for the other, and the FIFO's control and ports
#define reg_PXI_MAINP_INTF (*(vu16 *)0x04000180)
#define REG_PXI_MAINP_INTF_RECV_MASK 0x000f
#define REG_PXI_MAINP_INTF_SEND_SHIFT 8
#define reg_PXI_FIFO_CNT (*(vu16 *)0x04000184)
#define REG_PXI_FIFO_CNT_SEND_FULL_MASK 0x0002
#define REG_PXI_FIFO_CNT_RECV_EMPTY_MASK 0x0100
#define REG_PXI_FIFO_CNT_ERR_MASK 0x4000
#define REG_PXI_FIFO_CNT_ENABLE_MASK 0x8000
// Writing the error bit clears it
#define REG_PXI_FIFO_CNT_ERR_ACK (REG_PXI_FIFO_CNT_ENABLE_MASK | REG_PXI_FIFO_CNT_ERR_MASK)
// Clears the send FIFO, enables the FIFO and its interrupt when the receive FIFO is not empty
#define REG_PXI_FIFO_CNT_INIT 0xc408
#define reg_PXI_SEND_FIFO (*(vu32 *)0x04000188)
#define reg_PXI_RECV_FIFO (*(vu32 *)0x04100000)

// What sending a word and receiving one return
#define PXI_FIFO_SUCCESS 0
#define PXI_FIFO_FAIL_SEND_ERR (-1)
#define PXI_FIFO_FAIL_SEND_FULL (-2)
#define PXI_FIFO_FAIL_RECV_ERR (-3)
#define PXI_FIFO_FAIL_RECV_EMPTY (-4)

// The tags of the FIFO's words: each library has its own, 5 bits wide. The card's commands travel with the tag of the
// ARM7's file system, which reads the card; the DSi's libraries in the LTD autoload have theirs from 20
#define PXI_MAX_FIFO_TAG 32
#define PXI_FIFO_TAG_EX 0
#define PXI_FIFO_TAG_NVRAM 4
#define PXI_FIFO_TAG_RTC 5
#define PXI_FIFO_TAG_TOUCHPANEL 6
#define PXI_FIFO_TAG_SOUND 7
#define PXI_FIFO_TAG_PM 8
#define PXI_FIFO_TAG_MIC 9
#define PXI_FIFO_TAG_WM 10
#define PXI_FIFO_TAG_FS 11
#define PXI_FIFO_TAG_OS 12
#define PXI_FIFO_TAG_CTRDG 13
#define PXI_FIFO_TAG_FATFS 20
#define PXI_FIFO_TAG_CAMERA 21
#define PXI_FIFO_TAG_NWM 22
#define PXI_FIFO_TAG_SCFG 23
#define PXI_FIFO_TAG_SNDEX 24

// The processors, for func_0207ca48
#define PXI_PROC_ARM9 0
#define PXI_PROC_ARM7 1

// A word of the FIFO
typedef union {
    u32 raw;
    struct {
        u32 tag : 5;
        u32 err : 1;
        u32 data : 26;
    } e;
} PXIFifoMessage;

typedef void (*PXIFifoCallback)(u32 fifoTag, u32 data, BOOL err);

// Sets up the FIFO (NitroSDK's PXI_Init, which calls func_0207c954, NitroSDK's PXI_InitFifo)
void PXI_Init(void);
void func_0207c954(void);
// Sets the function called with the words the ARM7 sends with the tag, and marks the tag ready for the ARM7 in the
// shared memory; NULL clears both (PXI_SetFifoRecvCallback's code)
void func_0207c9fc(u32 fifoTag, PXIFifoCallback callback);
// Sends a word with the tag. Negative when the FIFO is full or in error
int PXI_SendWordByFifo(u32 fifoTag, u32 data, BOOL err);
// NitroSDK's PXIi_HandlerRecvFifoNotEmpty: the interrupt handler that calls the callbacks with the words received,
// which the loops that wait with the interrupts off also call
void func_0207cad4(void);
// NitroSDK's PXI_IsCallbackReady: whether the processor has a callback for the tag, by its bit in the shared memory
// (OSSystemWork's pxiSignalParam, nitro/os.h)
BOOL func_0207ca48(u32 fifoTag, u32 processor);

#endif // POKEBW2_NITRO_PXI_H
