// TwlSDK OS: the DSi's settings (os_ownerInfoEx.c, a guessed name). On a DSi the system leaves its own settings in the
// shared main memory, and these functions give out what is in them, on top of the owner's settings that the DS has;
// each checks OS_IsRunOnTwl (hw_isDSi) first. The calls to the LTD autoload's functions go through linker veneers.
#include "nitro/mi.h"
#include "nitro/os.h"
#include "twl/os.h"

// The pointer to the DSi's system settings, which the system puts in the shared memory
#define HW_TWL_SETTINGS_BUF_PTR 0x02fffdfc
// A byte of the TWL ROM header that the system puts in main memory
#define HW_TWL_ROM_HEADER_BYTE_20E 0x02ffe20e

// The DSi's system settings, of which only what this file reads is named
typedef struct TWLSettings {
    u32 unk0_0 : 24;
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
    if (hw_isDSi()) {
        TWLSettings *settings = TWL_SETTINGS;

        info->country = settings->country;
        info->language = settings->language;
    } else {
        info->country = 0;
        info->language = config->language;
    }
}

// TwlSDK's OS_IsAvailableWireless: TRUE on a DS
BOOL OS_IsAvailableWireless(void) {
    if (hw_isDSi()) {
        return func_02769318();
    }
    return TRUE;
}

// TwlSDK's OS_IsAgreeEULA: FALSE on a DS
BOOL OS_IsAgreeEULA(void) {
    if (hw_isDSi()) {
        return (TWL_SETTINGS->eulaFlags & 1) != 0;
    }
    return FALSE;
}

// TwlSDK's OS_GetAgreedEULAVersion: 0 on a DS
u8 OS_GetAgreedEULAVersion(void) {
    if (hw_isDSi()) {
        return TWL_SETTINGS->agreedEULAVersion;
    }
    return 0;
}

// A byte of the TWL ROM header (what TwlSDK's function is, is not known), 0 on a DS
u8 func_0207c49c(void) {
    if (hw_isDSi()) {
        return *(u8 *)HW_TWL_ROM_HEADER_BYTE_20E;
    }
    return 0;
}

// The DSi's parental controls, NULL on a DS
TWLParentalControl *func_0207c4b4(void) {
    if (hw_isDSi()) {
        return &TWL_SETTINGS->parentalControl;
    }
    return NULL;
}

// Probably TwlSDK's OS_IsForceDisableWireless: FALSE on a DS
BOOL func_0207c4d0(void) {
    if (hw_isDSi()) {
        return func_02769334();
    }
    return FALSE;
}
