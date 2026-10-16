#include "system/dsi.h"
#include "types.h"
#include "nitro/os.h"

// The DSi's settings, read through TwlSDK, with what a DS answers in their place. The file's name is a guess. Names
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except isRunningOnDSi and isWirelessEnabled

BOOL isRunningOnDSi(void) {
    return hw_isDSi();
}

BOOL canPlayerExchangePhotos(void) {
    if (isRunningOnDSi()) {
        if (func_0207c4b4()->unk0_0 && func_0207c4b4()->unk0_5) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

BOOL isWirelessEnabled(void) {
    return OS_IsAvailableWireless();
}

BOOL hasLicenseBeenAccepted(void) {
    if (isRunningOnDSi() == TRUE) {
        return OS_IsAgreeEULA();
    }
    return TRUE;
}

void getBirthdayMonthDay(u8 *month, u8 *day) {
    OSOwnerInfo info;

    OS_GetOwnerInfo(&info);
    *month = info.birthday.month;
    *day = info.birthday.day;
}
