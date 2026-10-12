#ifndef POKEBW2_SYSTEM_MAIN_H
#define POKEBW2_SYSTEM_MAIN_H

#include "types.h"
#include "nitro/os.h"
// For func_02005430, which this header declared before machine_use.c was told apart from main.c
#include "system/machine_use.h"

// The game's main file, with main, GameInit and GameUpdate. main is a name from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

void main(void);
// Keeps the main loop from giving the ARM9 priority on the main memory while it is set
void func_020051d8(BOOL blocked);

// Declarations that belong to headers main.c and machine_use.c do not own, to be moved there:
// nitro/os.h. NitroSDK's OS_Init, OS_InitAlarm, OS_SetIrqFunction, OS_EnableInterrupts and TwlSDK's
// OS_SleepThreadDirect, which scheduler_start_thread (OS_WakeupThreadDirect) undoes
#define OS_IE_H_BLANK 0x2

void os_init(void);
void func_0207bbf4(void);
void CPU_SetIRQFunc(u32 interrupt, void (*func)(void));
u32 CPU_IRQEnable(void);
void scheduler_join_thread(OSThread *thread, OSThreadQueue *queue);
// nitro/gx.h and nitro/fx.h. NitroSDK's GX_Init and FX_Init
void gfxInit(void);
void func_02074658(void);
// nitro/fs.h. NitroSDK's FS_Init and FS_TryLoadTable: the size of the file tables, which it reads when given memory
void fs_init(u32 dma);
u32 func_02072358(void *mem, u32 size);
// nitro/mi.h. NitroSDK's MI_WaitDma
void func_0207829c(u32 dma);
// nitro/mic.h, next to PM_SetAmp. NitroSDK's PM_GetAmp
u32 func_0207f0ac(int *amp);
// A header for TwlSDK's SCFG, which nothing has yet. SCFG_SetCpuSpeed: 1 doubles the DSi's ARM9 clock
void func_0207f6e8(int speed);
// A header for NitroSDK's CTRDG, which nothing has yet. CTRDG_DummyInit
void func_020827d0(void);
// gfl/sound.h. The sound thread of snd_sys.c
extern OSThread g_GFLSndThread;

#endif // POKEBW2_SYSTEM_MAIN_H
