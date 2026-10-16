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

typedef void (*MINDmaCallback)(void *arg);

int MI_FreeWram(MIWram wram, MIWramProc proc);
int MI_CancelWram(MIWram wram, MIWramProc proc);
BOOL MI_IsNDmaBusy(u32 ndmaNo);
void MI_StopNDma(u32 ndmaNo);
// Receives a camera frame by NDMA, CAMERA_DmaRecvAsync's body
void func_02768378(u32 ndmaNo, void *dest, u32 unitWords, u32 length, u32 unk, MINDmaCallback callback, void *arg);

// The setup of the shared WRAM's manager and of the new DMA, which NitroSDK's MI_Init (mi_sharedWram.c's
// func_02078000 and mi_init.c's MI_Init) calls on a DSi
void func_02768818(void);
void func_02768138(void);
// Stops the four new DMA channels, which OS_ResetSystem, the card's pull-out and PM_ForceToPowerOff do on a DSi
void func_02768298(void);

// The NDMA fills and copies, each without a callback and then with one and its argument, and the wait for a channel to
// be done, which GX's loaders use for the DSi's channels; the linker reaches them through veneers
void MI_NDmaFill(u32 ndmaNo, void *dest, u32 data, u32 size);
void MI_NDmaFillAsync(u32 ndmaNo, void *dest, u32 data, u32 size, MINDmaCallback callback, void *arg);
void MI_NDmaCopy(u32 ndmaNo, const void *src, void *dest, u32 size);
void MI_NDmaCopyAsync(u32 ndmaNo, const void *src, void *dest, u32 size, MINDmaCallback callback, void *arg);
void MI_WaitNDma(u32 ndmaNo);
// NitroSDK's MI_SendGXCommandAsync and MI_SendGXCommandAsyncFast for the new DMA channels, which NitroSystem G3D's
// NNS_G3dGeSendDL uses for GX's channels above 3. The names are TwlSDK's as recalled, not shown by the code
void MI_SendNDmaGXCommandAsync(u32 ndmaNo, const void *src, u32 length, MINDmaCallback callback, void *arg);
void MI_SendNDmaGXCommandAsyncFast(u32 ndmaNo, const void *src, u32 length, MINDmaCallback callback, void *arg);

// The banks of the shared WRAM B and C go from one processor to another by their slots. One byte per bank of WRAM B:
// its master, and the slot it has in the master's memory. WRAM C's follow
#define reg_MBK_B ((vu8 *)0x04004044)
#define reg_MBK_C ((vu8 *)0x0400404c)
// Whether the bank is given to a processor
BOOL func_0276919c(MIWram wram, int bank);
// The processor that the hardware gives the bank to
MIWramProc func_02768770(MIWram wram, int bank);
// Gives count free banks from bank to proc
int MI_AllocWramSlot(MIWram wram, int bank, int count, MIWramProc proc);
// Takes the banks back from proc
int MI_FreeWramSlot(MIWram wram, int bank, int count, MIWramProc proc);
// Where WRAM B is in the ARM9's memory, WRAM C, and either of them
void *func_027687a0(void);
void *func_027687bc(void);
void *func_027687d8(MIWram wram);

#endif // POKEBW2_TWL_MI_H
