#ifndef POKEBW2_TWL_OS_H
#define POKEBW2_TWL_OS_H

#include "types.h"
#include "nitro/os.h"

// TwlSDK's OS code in the LTD autoload: the DSi's parts of NitroSDK's OS functions, which main's OS code calls when
// the game runs on a DSi (the DSi parts of os_interrupt.c, os_arena.c, os_ownerInfoEx.c and os_application_jump.c).
// The functions' names are TwlSDK's, which aletheia's match of Black 1's code to libos.TWL.HYB.a gives; the types and
// the shared memory's addresses are those of libntr's reconstruction of TwlSDK's headers
// (os/common/ownerInfoEx_private.h, hw/common/mmap_shared.h)

// The pointer to the DSi's system settings (the preloaded parameters), which the system puts in the shared memory
#define HW_PRELOAD_PARAMETER_ADDR 0x02fffdfc

typedef struct OSTWLDate {
    u8 month;
    u8 day;
} OSTWLDate;

// The owner's information in the DSi's system settings
typedef struct OSTWLOwnerInfo {
    u8 userColor : 4;
    u8 rsv : 4;
    u8 pad;
    OSTWLDate birthday;
    u16 nickname[10 + 1];
    u16 comment[26 + 1];
} OSTWLOwnerInfo;

// The DSi's system settings
typedef struct OSTWLSettingsData {
    struct {
        u32 isFinishedInitialSetting : 1;
        u32 isFinishedInitialSetting_Launcher : 1;
        u32 isSetLanguage : 1;
        u32 isAvailableWireless : 1; // the wireless is enabled in the settings
        u32 rsv : 20;
        u32 isAgreeEULAFlagList : 8; // bit 0: the EULA was agreed to
    } flags;
    u8 rsv[1];
    u8 country;
    u8 language;
    u8 rtcLastSetYear;
    s64 rtcOffset;
    u8 agreeEulaVersion[8];
    u8 pad1[2];
    u8 pad2[6];
    u8 pad3[16];
    u8 pad4[20];
    OSTWLOwnerInfo owner;
    OSTWLParentalControl parentalControl;
} OSTWLSettingsData;

#define TWL_SETTINGS (*(OSTWLSettingsData **)HW_PRELOAD_PARAMETER_ADDR)

// The DSi's part of OS_IsAvailableWireless and of the function next to it: a bit of the DSi's settings and a byte of
// the shared memory
BOOL OS_IsAvailableWireless_ltdmain(void);
BOOL OS_IsForceDisableWireless_ltdmain(void);
// TwlSDK's body of OSi_EnterNDmaCallback
void OSi_EnterNDmaCallback_ltdmain(u32 ndmaNo, OSIrqCallback callback, void *arg);
// The DSi-only part of OS_InitArenaEx
void OSi_InitArenaEx_ltdmain(void);
// Looks a title ID up in the list that the system leaves in the shared memory (a count of at most 0x76 at 0x02ffd800,
// the 64-bit IDs from 0x02ffd850). Gives the first entry that equals titleId on the bits of mask and has bit 35 clear,
// or 0 when there is none
u64 OSi_SearchTitleIdFromList(u64 titleId, u64 mask);

#endif // POKEBW2_TWL_OS_H
