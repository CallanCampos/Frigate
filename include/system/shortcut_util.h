#ifndef POKEBW2_SYSTEM_SHORTCUT_UTIL_H
#define POKEBW2_SYSTEM_SHORTCUT_UTIL_H

#include "types.h"

// The Y button's shortcuts (shortcut_util.c, a guessed name): the shortcut menu's position, which GameData keeps, and
// the shortcuts of the key items. Our names

// The shortcuts of the Y button's menu that key items register
enum {
    SHORTCUT_BICYCLE,
    SHORTCUT_TOWN_MAP,
    SHORTCUT_VS_RECORDER,
    SHORTCUT_PAL_PAD,
    SHORTCUT_SUPER_ROD,
    SHORTCUT_DOWSING_MCHN,
    SHORTCUT_GRACIDEA,
    SHORTCUT_DNA_SPLICERS_FUSE,
    SHORTCUT_DNA_SPLICERS_SEPARATE,
    SHORTCUT_XTRANSCEIVER = 28,
    SHORTCUT_MEDAL_BOX,
    SHORTCUT_REVEAL_GLASS = 33,
};

// What ShortcutUtil_GetItemShortcut returns for an item without a shortcut
#define SHORTCUT_NONE 0xff

// Where the shortcut menu's list was left, which GameData_Create clears
typedef struct {
    u16 listTop;
    u16 cursorRow;
} ShortcutMenuPos;

void ShortcutMenuPos_Init(ShortcutMenuPos *pos);
// The shortcut that registers the key item to Y, or SHORTCUT_NONE
u32 ShortcutUtil_GetItemShortcut(u16 item);

#endif // POKEBW2_SYSTEM_SHORTCUT_UTIL_H
