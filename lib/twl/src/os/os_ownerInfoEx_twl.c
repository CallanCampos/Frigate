// TwlSDK's DSi part of os_ownerInfoEx.c (ARM9 main), in the LTD autoload: the wireless settings that main's
// OS_IsAvailableWireless and OS_IsForceDisableWireless read on a DSi. The file name is inferred from main's half; the
// functions' names are ours. It links as a file of its own: unlike card_rom.c's and pm.c's DSi parts
// (config/merged_sources.toml), nothing in it reaches main's statics from their pooled base, which would show that it
// shares main's object.
#include "nitro/os.h"
#include "twl/os.h"

// The DSi's secure hardware settings that the system leaves in the shared memory, in the layout of
// cybervisi0n/libntr's reconstruction (OSTWLHWSecureInfo). The flag is the low bit of the byte after the first word,
// which the load reaches as offset 4 of the block
typedef struct OSTWLHWSecureInfo {
    u32 validLanguageBitmap;
    struct {
        u8 forceDisableWireless : 1;
        u8 : 7;
    } flags;
    u8 pad[3];
    u8 region;
    u8 serialNo[15];
} OSTWLHWSecureInfo;

#define HW_HW_SECURE_INFO 0x02fffd68

#pragma define_section LTDMAIN ".ltdmain"                                                                              \
                               ".ltdmain.bss"                                                                          \
                               ".ltdmain" abs32 RWX
#pragma section LTDMAIN begin

BOOL OSi_IsAvailableWirelessTwl(void) {
    return TWL_SETTINGS->flags.isAvailableWireless == 1 ? TRUE : FALSE;
}

BOOL OSi_IsForceDisableWirelessTwl(void) {
    OSTWLHWSecureInfo *info = (OSTWLHWSecureInfo *)HW_HW_SECURE_INFO;

    return info->flags.forceDisableWireless == 1 ? TRUE : FALSE;
}

#pragma section LTDMAIN end
