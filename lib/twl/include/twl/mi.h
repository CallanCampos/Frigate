#ifndef POKEBW2_TWL_MI_H
#define POKEBW2_TWL_MI_H

#include "types.h"

// TwlSDK's memory functions for the DSi's hardware: the shared WRAM B and C, which the DSP needs, and the new DMA
// channels. They are in the LTD autoload and keep their default names until it is analyzed.

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

int func_02768cf8(MIWram wram, MIWramProc proc); // MI_FreeWram
int func_02769080(MIWram wram, MIWramProc proc); // MI_CancelWram
BOOL func_02768234(u32 ndmaNo);                  // MI_IsNDmaBusy
void func_02768270(u32 ndmaNo);                  // MI_StopNDma
// Receives a camera frame by NDMA, CAMERA_DmaRecvAsync's body
void func_02768378(u32 ndmaNo, void *dest, u32 unitWords, u32 length, u32 unk, MINDmaCallback callback, void *arg);

// The setup of the shared WRAM's manager and of the new DMA, which NitroSDK's MI_Init (mi_sharedWram.c's
// func_02078000 and mi_init.c's func_020791d0) calls on a DSi
void func_02768818(void);
void func_02768138(void);
// Stops the four new DMA channels, which OS_ResetSystem does on a DSi
void func_02768298(void);

// The NDMA fills, the first with a callback and its argument, the copies, the same, and the wait for a channel to be
// done, which GX's loaders use for the DSi's channels; the linker reaches them through veneers
void func_0276814c(u32 ndmaNo, void *dest, u32 data, u32 size);
void func_027681b8(u32 ndmaNo, void *dest, u32 data, u32 size, MINDmaCallback callback, void *arg);
void func_02768180(u32 ndmaNo, const void *src, void *dest, u32 size);
void func_027681f4(u32 ndmaNo, const void *src, void *dest, u32 size, MINDmaCallback callback, void *arg);
void func_0276824c(u32 ndmaNo);

// The banks of the shared WRAM B and C go from one processor to another by their slots. One byte per bank of WRAM B:
// its master, and the slot it has in the master's memory. WRAM C's follow
#define reg_MBK_B ((vu8 *)0x04004044)
#define reg_MBK_C ((vu8 *)0x0400404c)
// Whether the bank is given to a processor
BOOL func_0276919c(MIWram wram, int bank);
// The processor that the hardware gives the bank to
MIWramProc func_02768770(MIWram wram, int bank);
// MI_AllocWramSlot: gives count free banks from bank to proc
int func_02768c04(MIWram wram, int bank, int count, MIWramProc proc);
// MI_FreeWramSlot: takes the banks back from proc
int func_02768d90(MIWram wram, int bank, int count, MIWramProc proc);
// Where WRAM B is in the ARM9's memory, WRAM C, and either of them
void *func_027687a0(void);
void *func_027687bc(void);
void *func_027687d8(MIWram wram);

#endif // POKEBW2_TWL_MI_H
