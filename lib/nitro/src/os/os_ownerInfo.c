// NitroSDK OS: the owner's settings (os_ownerInfo.c). The ARM7 reads the user settings from the console's firmware into
// the shared main memory, with the console's MAC address after them, and these functions give them out. The file
// also has the table of the sixteen favorite colors that the settings pick from.
#include "nitro/mi.h"
#include "nitro/os.h"

// The favorite colors, as the RGB555 values that a color setting indexes
static const u16 OSi_FavoriteColorTable[16] = {
    0x4e0c, 0x0137, 0x0c1f, 0x7e3f, 0x025f, 0x039e, 0x03f5, 0x03e0,
    0x1e80, 0x4769, 0x7ae6, 0x7960, 0x4800, 0x6811, 0x741a, 0x481f,
};

void OS_GetMacAddress(u8 *macAddress) {
    MI_CpuCopy8(((NVRAMConfig *)HW_NVRAM_USER_INFO)->macAddress, macAddress, 6);
}

// NitroSDK's OS_GetOwnerInfo
void OS_GetOwnerInfo(OSOwnerInfo *info) {
    NVRAMConfig *config = (NVRAMConfig *)HW_NVRAM_USER_INFO;

    info->language = config->language;
    info->favoriteColor = config->owner.favoriteColor;
    info->birthday.month = config->owner.birthday.month;
    info->birthday.day = config->owner.birthday.day;
    info->nickNameLength = config->owner.nickNameLength;
    info->commentLength = config->owner.commentLength;
    MI_CpuCopy16(config->owner.nickName, info->nickName, OS_OWNERINFO_NICKNAME_MAX * sizeof(u16));
    MI_CpuCopy16(config->owner.comment, info->comment, OS_OWNERINFO_COMMENT_MAX * sizeof(u16));
    info->nickName[OS_OWNERINFO_NICKNAME_MAX] = 0;
    info->comment[OS_OWNERINFO_COMMENT_MAX] = 0;
}

// NitroSDK's OS_GetOwnerRtcOffset: the RTC offset the owner set in the settings, in seconds
s64 OS_GetOwnerRtcOffset(void) {
    NVRAMConfig *config = (NVRAMConfig *)HW_NVRAM_USER_INFO;

    return config->rtcOffset;
}

const u16 *OS_GetFavoriteColorTable(void) {
    return OSi_FavoriteColorTable;
}
