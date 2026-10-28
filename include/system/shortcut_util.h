#ifndef POKEBW2_SYSTEM_SHORTCUT_UTIL_H
#define POKEBW2_SYSTEM_SHORTCUT_UTIL_H

#include "types.h"

// The Y button's shortcuts (shortcut_util.c, a guessed name): the shortcut menu's position, which GameData keeps, and
// the shortcuts of the key items. Our names. The screens that register their own shortcuts take theirs from here

// The shortcuts of the Y button's menu, which ShortcutMenu_SetKeyItemID opens: those that key items register, and
// those of the screens whose own check boxes register them, each opening the screen's app where it registered
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
    // The summary's pages, in the order of PSTATUS_PAGE_*
    SHORTCUT_PSTATUS_INFO,
    SHORTCUT_PSTATUS_SKILL,
    SHORTCUT_PSTATUS_RIBBON,
    // The bag's pockets, in the order of BAG_POCKET_*
    SHORTCUT_BAG_ITEMS,
    SHORTCUT_BAG_MEDICINE,
    SHORTCUT_BAG_TMS_HMS,
    SHORTCUT_BAG_BERRIES,
    SHORTCUT_BAG_KEY_ITEMS,
    SHORTCUT_BAG_FREE_SPACE,
    // The Pokédex's list (or its awards' list, which registers the same one), search and detail pages
    SHORTCUT_POKEDEX_LIST,
    SHORTCUT_POKEDEX_SEARCH,
    SHORTCUT_POKEDEX_INFO,
    SHORTCUT_POKEDEX_MAP,
    SHORTCUT_POKEDEX_VOICE,
    SHORTCUT_POKEDEX_FORM,
    // The trainer card's front and back, and the badge case
    SHORTCUT_TRAINER_CARD_FRONT,
    SHORTCUT_TRAINER_CARD_BACK,
    SHORTCUT_TRAINER_CARD_BADGES,
    // The options
    SHORTCUT_CONFIG_PANEL,
    SHORTCUT_XTRANSCEIVER,
    SHORTCUT_MEDAL_BOX,
    // The party list, and the Pokédex opened on its awards (POKEDEX_START_AWARDS) and on its Habitat List
    SHORTCUT_POKELIST,
    SHORTCUT_POKEDEX_AWARDS,
    SHORTCUT_POKEDEX_HABITAT,
    SHORTCUT_REVEAL_GLASS,
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
