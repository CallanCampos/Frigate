// TwlSDK OS: the DSi's settings (os_ownerInfoEx.c, a guessed name). On a DSi the system leaves its own settings in the
// shared main memory, and these functions give out what is in them, on top of the owner's settings that the DS has;
// each checks OS_IsRunOnTwl (OS_IsRunOnTwl) first. The calls to the LTD autoload's functions go through linker veneers.
#include "nitro/mi.h"
#include "nitro/os.h"
#include "twl/os.h"

// A byte of the TWL ROM header that the system puts in main memory
#define HW_TWL_ROM_HEADER_BYTE_20E 0x02ffe20e

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
        return OSi_IsAvailableWirelessTwl();
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

// TwlSDK's OS_IsForceDisableWireless (the name is recalled, not shown): FALSE on a DS
BOOL OS_IsForceDisableWireless(void) {
    if (OS_IsRunOnTwl()) {
        return OSi_IsForceDisableWirelessTwl();
    }
    return FALSE;
}
