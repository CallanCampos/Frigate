#ifndef POKEBW2_TWL_OS_H
#define POKEBW2_TWL_OS_H

#include "types.h"
#include "nitro/os.h"

// TwlSDK's OS code in the LTD autoload: the DSi's parts of NitroSDK's OS functions, which main's OS code calls when
// the game runs on a DSi. The functions keep their default names until the LTD autoload is analyzed

// The DSi's part of OS_IsAvailableWireless and of the function next to it: a bit of the DSi's settings and a byte of
// the shared memory
BOOL func_02769318(void);
BOOL func_02769334(void);
// The DSi's cleanup before OS_ResetSystem resets, which the card's pull-out does too
void func_02769578(void);
// TwlSDK's body of OSi_EnterNDmaCallback
void func_027692b8(u32 ndmaNo, OSIrqCallback callback, void *arg);
// The DSi-only part of OS_InitArenaEx
void func_027692ec(void);
// Looks a title ID up in the list that the system leaves in the shared memory (a count of at most 0x76 at 0x02ffd800,
// the 64-bit IDs from 0x02ffd850). Gives the first entry that equals titleId on the bits of mask and has bit 35 clear,
// or 0 when there is none
u64 func_0276934c(u64 titleId, u64 mask);

// The title ID of the program that left the arguments, which os_application_jump.c keeps in the .bss of the DSi-only
// LTD autoload. No file owns the word yet
extern u64 data_0276b0dc;

#endif // POKEBW2_TWL_OS_H
