#include "system/shortcut_util.h"
#include "types.h"
#include "constants/items.h"
#include "gfl/std.h"

// The Y button's shortcuts: the shortcut menu's position, which GameData keeps, and the shortcuts of the key items. A
// guessed name, our names

// The shortcuts of key items
#define SHORTCUT_BICYCLE 0
#define SHORTCUT_TOWN_MAP 1
#define SHORTCUT_VS_RECORDER 2
#define SHORTCUT_PAL_PAD 3
#define SHORTCUT_SUPER_ROD 4
#define SHORTCUT_DOWSING_MCHN 5
#define SHORTCUT_GRACIDEA 6
#define SHORTCUT_DNA_SPLICERS_FUSE 7
#define SHORTCUT_DNA_SPLICERS_SEPARATE 8
#define SHORTCUT_XTRANSCEIVER 28
#define SHORTCUT_MEDAL_BOX 29
#define SHORTCUT_REVEAL_GLASS 33

typedef struct {
    u16 item : 10;
    u16 shortcut : 6;
} ItemShortcut;

static const ItemShortcut sItemShortcuts[] = {
    { ITEM_BICYCLE, SHORTCUT_BICYCLE },
    { ITEM_SUPER_ROD, SHORTCUT_SUPER_ROD },
    { ITEM_TOWN_MAP, SHORTCUT_TOWN_MAP },
    { ITEM_VS_RECORDER, SHORTCUT_VS_RECORDER },
    { ITEM_PAL_PAD, SHORTCUT_PAL_PAD },
    { ITEM_DOWSING_MCHN, SHORTCUT_DOWSING_MCHN },
    { ITEM_GRACIDEA, SHORTCUT_GRACIDEA },
    { ITEM_XTRANSCEIVER_MALE, SHORTCUT_XTRANSCEIVER },
    { ITEM_XTRANSCEIVER_FEMALE, SHORTCUT_XTRANSCEIVER },
    { ITEM_MEDAL_BOX, SHORTCUT_MEDAL_BOX },
    { ITEM_DNA_SPLICERS_FUSE, SHORTCUT_DNA_SPLICERS_FUSE },
    { ITEM_DNA_SPLICERS_SEPARATE, SHORTCUT_DNA_SPLICERS_SEPARATE },
    { ITEM_REVEAL_GLASS, SHORTCUT_REVEAL_GLASS },
};

void ShortcutMenuPos_Init(ShortcutMenuPos *pos) {
    MI_CpuFill8(pos, 0, sizeof(ShortcutMenuPos));
}

u32 ShortcutUtil_GetItemShortcut(u16 item) {
    u32 i;

    for (i = 0; i < NELEMS(sItemShortcuts); i++) {
        if (item == sItemShortcuts[i].item) {
            return sItemShortcuts[i].shortcut;
        }
    }
    return SHORTCUT_NONE;
}
