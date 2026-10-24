// TwlSDK OS: the DSi's settings (os_ownerInfoEx.c, TwlSDK's os_ownerinfoex.o, which aletheia's match of Black 1's
// code to libos.TWL.HYB.a gives with its functions' names). On a DSi the system leaves its own settings in the shared
// main memory, and these functions give out what is in them, on top of the owner's settings that the DS has; each
// checks OS_IsRunOnTwl first. The file has TwlSDK's DSi part too, the wireless settings, in TwlSDK's LTDMAIN section,
// which the LTD autoload's .ltdmain holds: dsd lists it as lib/twl/src/os/os_ownerInfoEx_twl.c, which
// config/merged_sources.toml links from this file's object. The main part calls it through linker veneers.
#include "nitro/mi.h"
#include "nitro/os.h"
#include "twl/os.h"

// A byte of the TWL ROM header that the system puts in main memory
#define HW_TWL_ROM_HEADER_BYTE_20E 0x02ffe20e

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

// TwlSDK's OS_GetOwnerInfoEx: the owner's settings, with the language and the country from the DSi's settings on a DSi
void OS_GetOwnerInfoEx(OSOwnerInfoEx *info) {
    NVRAMConfig *config = (NVRAMConfig *)HW_NVRAM_USER_INFO;

    info->favoriteColor = config->owner.favoriteColor;
    info->birthday.month = config->owner.birthday.month;
    info->birthday.day = config->owner.birthday.day;
    info->nickNameLength = config->owner.nickNameLength;
    info->commentLength = config->owner.commentLength;
    MI_CpuCopy16(config->owner.nickName, info->nickName, OS_OWNERINFO_NICKNAME_MAX * sizeof(u16));
    MI_CpuCopy16(config->owner.comment, info->comment, OS_OWNERINFO_COMMENT_MAX * sizeof(u16));
    info->nickName[OS_OWNERINFO_NICKNAME_MAX] = 0;
    info->comment[OS_OWNERINFO_COMMENT_MAX] = 0;
    if (OS_IsRunOnTwl()) {
        OSTWLSettingsData *settings = TWL_SETTINGS;

        info->country = settings->country;
        info->language = settings->language;
    } else {
        info->country = 0;
        info->language = config->language;
    }
}

// TwlSDK's OS_IsAvailableWireless: TRUE on a DS
BOOL OS_IsAvailableWireless(void) {
    if (OS_IsRunOnTwl()) {
        return OS_IsAvailableWireless_ltdmain();
    }
    return TRUE;
}

// TwlSDK's OS_IsAgreeEULA: FALSE on a DS
BOOL OS_IsAgreeEULA(void) {
    if (OS_IsRunOnTwl()) {
        return (TWL_SETTINGS->flags.isAgreeEULAFlagList & 1) != 0;
    }
    return FALSE;
}

// TwlSDK's OS_GetAgreedEULAVersion: 0 on a DS
u8 OS_GetAgreedEULAVersion(void) {
    if (OS_IsRunOnTwl()) {
        return TWL_SETTINGS->agreeEulaVersion[0];
    }
    return 0;
}

// A byte of the TWL ROM header (what TwlSDK's function is, is not known), 0 on a DS
u8 OS_GetROMHeaderEULAVersion(void) {
    if (OS_IsRunOnTwl()) {
        return *(u8 *)HW_TWL_ROM_HEADER_BYTE_20E;
    }
    return 0;
}

// The DSi's parental controls, NULL on a DS
OSTWLParentalControl *OS_GetParentalControlInfoPtr(void) {
    if (OS_IsRunOnTwl()) {
        return &TWL_SETTINGS->parentalControl;
    }
    return NULL;
}

// TwlSDK's OS_IsForceDisableWireless: FALSE on a DS
BOOL OS_IsForceDisableWireless(void) {
    if (OS_IsRunOnTwl()) {
        return OS_IsForceDisableWireless_ltdmain();
    }
    return FALSE;
}

// clang-format off
// TwlSDK's section for the DSi-only code and data that the LTD autoload's .ltdmain holds
#pragma define_section LTDMAIN ".ltdmain" ".ltdmain.bss" ".ltdmain" abs32 RWX
// clang-format on
// The DSi part, in the LTD autoload: the wireless settings that OS_IsAvailableWireless and
// OS_IsForceDisableWireless read on a DSi
#pragma section LTDMAIN begin

BOOL OS_IsAvailableWireless_ltdmain(void) {
    return TWL_SETTINGS->flags.isAvailableWireless == 1 ? TRUE : FALSE;
}

BOOL OS_IsForceDisableWireless_ltdmain(void) {
    OSTWLHWSecureInfo *info = (OSTWLHWSecureInfo *)HW_HW_SECURE_INFO;

    return info->flags.forceDisableWireless == 1 ? TRUE : FALSE;
}

#pragma section LTDMAIN end
