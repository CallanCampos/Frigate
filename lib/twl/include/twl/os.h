#ifndef POKEBW2_TWL_OS_H
#define POKEBW2_TWL_OS_H

#include "types.h"
#include "nitro/os.h"

// TwlSDK's OS code in the LTD autoload: the DSi's parts of NitroSDK's OS functions, which main's OS code calls when
// the game runs on a DSi (os_interrupt_twl.c, os_arena_twl.c, os_ownerInfoEx_twl.c and os_application_jump_twl.c).
// The names are ours

// The pointer to the DSi's system settings, which the system puts in the shared memory
#define HW_TWL_SETTINGS_BUF_PTR 0x02fffdfc

// The DSi's system settings, of which only what os_ownerInfoEx.c and its DSi part read is named
typedef struct TWLSettings {
    u32 unk0_0 : 3;
    u32 wirelessEnabled : 1; // bit 3: the wireless is enabled in the settings
    u32 unk0_4 : 20;
    u32 eulaFlags : 8; // bit 0: the EULA was agreed to
    u8 unk_04;
    u8 country;
    u8 language;
    u8 unk_07[9];
    u8 agreedEULAVersion;
    u8 unk_11[0x83];
    TWLParentalControl parentalControl;
} TWLSettings;

#define TWL_SETTINGS (*(TWLSettings **)HW_TWL_SETTINGS_BUF_PTR)

// The DSi's part of OS_IsAvailableWireless and of the function next to it: a bit of the DSi's settings and a byte of
// the shared memory
BOOL OSi_IsAvailableWirelessTwl(void);
BOOL OSi_IsForceDisableWirelessTwl(void);
// The DSi's cleanup before OS_ResetSystem resets, which the card's pull-out does too
void func_02769578(void);
// TwlSDK's body of OSi_EnterNDmaCallback
void OSi_EnterNDmaCallbackCore(u32 ndmaNo, OSIrqCallback callback, void *arg);
// The DSi-only part of OS_InitArenaEx
void OSi_InitArenaExTwl(void);
// Looks a title ID up in the list that the system leaves in the shared memory (a count of at most 0x76 at 0x02ffd800,
// the 64-bit IDs from 0x02ffd850). Gives the first entry that equals titleId on the bits of mask and has bit 35 clear,
// or 0 when there is none
u64 OSi_FindSystemTitleId(u64 titleId, u64 mask);

// The title ID of the program that left the arguments, which os_application_jump.c keeps in the .bss of the DSi-only
// LTD autoload (os_application_jump_twl.c)
extern u64 OSi_SenderTitleId;

#endif // POKEBW2_TWL_OS_H
