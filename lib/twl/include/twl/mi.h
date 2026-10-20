#ifndef POKEBW2_TWL_MI_H
#define POKEBW2_TWL_MI_H

#include "types.h"

// TwlSDK's memory functions for the DSi's hardware: the shared WRAM B and C, which the DSP needs, and the new DMA
// channels. They are in the LTD autoload. The functions have TwlSDK's names where the code shows them; the ones still
// named by address are not known.

typedef enum {
    MI_WRAM_A,
    MI_WRAM_B,
    MI_WRAM_C,
} MIWram;

typedef enum {
    MI_WRAM_ARM9,
    MI_WRAM_ARM7,
    MI_WRAM_DSP,
} MIWramProc;

// mi_sharedWram_twl.c: the DSi part of the shared WRAM manager

// The setup of the shared WRAM's manager, which NitroSDK's MI_Init (mi_sharedWram.c's func_02078000) calls on a DSi
void MIi_InitWramManagerCore(void);
// The banks' registers: the processor that each bank is given to and whether it is enabled, by WRAM and for each
void MI_SetWramBankMaster_A(int bank, MIWramProc proc);
void MI_SetWramBankMaster_B(int bank, MIWramProc proc);
void MI_SetWramBankMaster_C(int bank, MIWramProc proc);
void MI_SetWramBankMaster(MIWram wram, int bank, MIWramProc proc);
void MI_SetWramBankEnable_A(int bank, BOOL enable);
void MI_SetWramBankEnable_B(int bank, BOOL enable);
void MI_SetWramBankEnable_C(int bank, BOOL enable);
void MI_SetWramBankEnable(MIWram wram, int bank, BOOL enable);
MIWramProc MI_GetWramBankMaster_A(int bank);
MIWramProc MI_GetWramBankMaster_B(int bank);
MIWramProc MI_GetWramBankMaster_C(int bank);
// The processor that the hardware gives the bank to
MIWramProc MI_GetWramBankMaster(MIWram wram, int bank);
// Where each WRAM is in the ARM9's memory, and any of them
void *MI_GetWramMapStart_A(void);
void *MI_GetWramMapStart_B(void);
void *MI_GetWramMapStart_C(void);
void *MI_GetWramMapStart(MIWram wram);
// Frees every bank that proc has, and cancels proc's reservations; each returns how many banks it changed
int MI_FreeWram(MIWram wram, MIWramProc proc);
int MI_CancelWram(MIWram wram, MIWramProc proc);

// mi_ndma.c: the DSi's four new DMA channels

typedef void (*MINDmaCallback)(void *arg);

// The settings of a channel's transfers: the interval between blocks and its prescaler, the words of a block, and the
// words that a start moves (-1: the whole transfer at once)
typedef struct MINDmaConfig {
    u32 intervalTimer;
    u32 prescaler;
    u32 blockWord;
    u32 wordCount;
} MINDmaConfig;

#define MI_NDMA_MAX_NUM 3

// A register of a channel: the source, the destination, the words of the transfer and of each start, the interval
// between blocks, the data of a fill and the control. The channels' registers are 0x1c bytes apart
#define MI_NDMA_REG(ndmaNo, offset) (*(vu32 *)(0x04004104 + (ndmaNo) * 0x1c + (offset)))
#define MI_NDMA_SAD 0x0
#define MI_NDMA_DAD 0x4
#define MI_NDMA_TCNT 0x8
#define MI_NDMA_WCNT 0xc
#define MI_NDMA_BCNT 0x10
#define MI_NDMA_FDATA 0x14
#define MI_NDMA_CNT 0x18
// The bits of a channel's control
#define MI_NDMA_DEST_FIX (2 << 10)
#define MI_NDMA_DEST_RELOAD (1 << 12)
#define MI_NDMA_SRC_FIX (2 << 13)
#define MI_NDMA_SRC_FILLDATA (3 << 13)
#define MI_NDMA_BWORD_16 (4 << 16)
#define MI_NDMA_TIMING_MASK (0xf << 24)
#define MI_NDMA_TIMING_CARD (4 << 24)
#define MI_NDMA_TIMING_DISP (8 << 24)
#define MI_NDMA_TIMING_DISP_MMEM (9 << 24)
#define MI_NDMA_TIMING_CAMERA (0xb << 24)
#define MI_NDMA_IMM (0x10 << 24)
#define MI_NDMA_CONTINUOUS (1 << 29)
#define MI_NDMA_IF_ENABLE (1 << 30)
#define MI_NDMA_ENABLE (1u << 31)
#define MI_NDMA_ARBITRAMENT_ROUND (1u << 31)

// Each channel's settings
extern MINDmaConfig MIi_NDmaConfig[MI_NDMA_MAX_NUM + 1];

// Sets the arbitration between the channels and the default settings of each, which NitroSDK's MI_Init (mi_init.c)
// calls on a DSi
void MI_InitNDma(void);
// The NDMA fills and copies, each without a callback and then with one and its argument, and the wait for a channel to
// be done, which GX's loaders use for the DSi's channels; the linker reaches them through veneers
void MI_NDmaFill(u32 ndmaNo, void *dest, u32 data, u32 size);
void MI_NDmaCopy(u32 ndmaNo, const void *src, void *dest, u32 size);
void MI_NDmaFillAsync(u32 ndmaNo, void *dest, u32 data, u32 size, MINDmaCallback callback, void *arg);
void MI_NDmaCopyAsync(u32 ndmaNo, const void *src, void *dest, u32 size, MINDmaCallback callback, void *arg);
BOOL MI_IsNDmaBusy(u32 ndmaNo);
void MI_WaitNDma(u32 ndmaNo);
void MI_StopNDma(u32 ndmaNo);
// Stops the four channels, which OS_ResetSystem, the card's pull-out and PM_ForceToPowerOff do on a DSi
void MI_StopAllNDma(void);
void MI_SetNDmaArbitrament(u32 mode, u32 cycle);
void MI_InitNDmaConfig(void);
void MI_GetNDmaConfig(u32 ndmaNo, MINDmaConfig *config);
// The transfer under all of the asynchronous NDMA functions, which mi_ndma_gxcommand.c also calls: a fill of data or a
// copy, by the address modes of mode, started at timing (-1: at once) with the channel's settings in config
void MIi_NDmaAsync(u32 mode, u32 ndmaNo, const void *src, void *dest, u32 data, u32 size, MINDmaCallback callback,
                   void *arg, const MINDmaConfig *config, u32 timing, u32 enable);
// The card's reads by NDMA: one word at each request of the card, from its data register, until the card's transfer
// ends. The second is the one that CARDi_NdmaInterface gives the card library
void MIi_StartCardNDma(u32 ndmaNo, const void *src, void *dest);
void MIi_CardNDmaRecv(u32 ndmaNo, const void *src, void *dest, u32 size);
// Receives a camera frame by NDMA, CAMERA_DmaRecvAsync's body: unitWords words at each request of the camera, again
// and again if continuous
void MIi_CameraNDmaRecv(u32 ndmaNo, void *dest, u32 unitWords, u32 length, BOOL continuous, MINDmaCallback callback,
                        void *arg);

// NitroSDK's MI_SendGXCommandAsync and MI_SendGXCommandAsyncFast for the new DMA channels, which NitroSystem G3D's
// NNS_G3dGeSendDL uses for GX's channels above 3. The names are TwlSDK's as recalled, not shown by the code
void MI_SendNDmaGXCommandAsync(u32 ndmaNo, const void *src, u32 length, MINDmaCallback callback, void *arg);
void MI_SendNDmaGXCommandAsyncFast(u32 ndmaNo, const void *src, u32 length, MINDmaCallback callback, void *arg);

// The banks of the shared WRAM A, B and C go from one processor to another by their slots. One byte per bank of WRAM
// A: its master, and the slot it has in the master's memory. WRAM B's and C's follow
#define reg_MBK_A ((vu8 *)0x04004040)
#define reg_MBK_B ((vu8 *)0x04004044)
#define reg_MBK_C ((vu8 *)0x0400404c)
// Whether the manager has given the bank to a processor
BOOL MIi_IsWramSlotUsed(MIWram wram, int slot);
// Gives the count banks from slot to proc if they are free, and returns their address (0 if not)
u32 MI_AllocWramSlot(MIWram wram, int slot, int count, MIWramProc proc);
// Takes the banks back from proc, and returns how many it had
int MI_FreeWramSlot(MIWram wram, int slot, int count, MIWramProc proc);

#endif // POKEBW2_TWL_MI_H
