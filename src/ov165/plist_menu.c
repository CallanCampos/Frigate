#include "app/pokelist.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "pml/poke_party.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"

// The party list's menus: what to do with a Pokémon, yes or no, and the buttons of a battle's selection

// The items: 0 summary, 1 the moves usable outside battle, 2 the moves to restore, 3 switch, 4 item, 5 mail,
// 6 back, 7 select, 8 joining or leaving a battle's order, 9 give, 10 take, 11 dress up, 12 read, 13 take mail,
// 14 yes, 15 no, 16 to 19 the moves, and 16 also ends a list, 20 join, 21 leave, 22 and 23 move an item or mail
#define POKELIST_MENU_MAX 8
#define POKELIST_MENU_END 16
#define POKELIST_MENU_MOVE1 16

struct PokeListMenu {
    u8 count;
    u16 items[POKELIST_MENU_MAX + 1];
    AppTaskMenuItem entries[POKELIST_MENU_MAX];
    void *taskMenu;
    u32 unk78;
    NNSG2dCharacterData *chars;
    void *charsFile;
};

static void PokeListMenu_SetItems(PokeListWork *wk, PokeListMenu *menu, const u32 *items);
static StrBuf *PokeListMenu_GetItemName(PokeListWork *wk, PokeListMenu *menu, int item);

// The message of each item
static const u32 sItemNames[] = {
    0x77, 0,    0,    0x76, 0x78, 0x79, 0x7d, 0x7c, 0,    0x85, 0x86, 0xb1,
    0x7a, 0x7b, 0xbd, 0xbe, 0,    0,    0,    0,    0x7f, 0x80, 0x8b, 0x8c,
};

PokeListMenu *PokeListMenu_Create(PokeListWork *wk) {
    PokeListMenu *menu = GFL_HeapAllocate(wk->heapId, sizeof(PokeListMenu), FALSE, "plist_menu.c", 99);

    GFL_BGSysLoadNCLRDefault(getUINarcIdx(), 31, 0, 0x20, 0x40, wk->heapId);
    menu->charsFile = GFL_G2DIOReadBGNCGR(getUINarcIdx(), 32, FALSE, &menu->chars, wk->heapId);
    return menu;
}

void PokeListMenu_Free(PokeListWork *wk, PokeListMenu *menu) {
    GFL_HeapFree(menu->charsFile);
    GFL_HeapFree(menu);
}

// Opens the menu of the items listed, which end with POKELIST_MENU_END
void PokeListMenu_Open(PokeListWork *wk, PokeListMenu *menu, const u32 *items) {
    AppTaskMenuInit setup;

    PokeListMenu_SetItems(wk, menu, items);
    setup.heapId = wk->heapId;
    setup.itemCount = menu->count;
    setup.items = menu->entries;
    setup.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    setup.x = 32;
    setup.y = 24;
    setup.width = 13;
    setup.height = 3;
    func_0203d564(wk->touch);
    menu->taskMenu = AppTaskMenu_Create(&setup, wk->taskMenuRes);
}

void PokeListMenu_OpenYesNo(PokeListWork *wk, PokeListMenu *menu) {
    AppTaskMenuInit setup;
    u32 items[3] = { 14, 15, POKELIST_MENU_END };

    PokeListMenu_SetItems(wk, menu, items);
    setup.heapId = wk->heapId;
    setup.itemCount = menu->count;
    setup.items = menu->entries;
    setup.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    setup.x = 32;
    setup.y = 18;
    setup.width = 8;
    setup.height = 3;
    func_0203d564(wk->touch);
    menu->taskMenu = AppTaskMenu_Create(&setup, wk->taskMenuRes);
}

void PokeListMenu_Close(PokeListWork *wk, PokeListMenu *menu) {
    u8 i;

    AppTaskMenu_Free(menu->taskMenu);
    for (i = 0; i < menu->count; i++) {
        GFL_StrBufFree(menu->entries[i].str);
    }
    wk->touch = func_0203d554();
}

void PokeListMenu_Update(PokeListWork *wk, PokeListMenu *menu) {
    AppTaskMenu_Update(menu->taskMenu);
}

// The item picked, or 0x19 while the menu waits
u32 PokeListMenu_GetPicked(PokeListWork *wk, PokeListMenu *menu) {
    if (AppTaskMenu_IsFlashFinished(menu->taskMenu) == FALSE) {
        return 0x19;
    }
    return menu->items[AppTaskMenu_GetCursorPos(menu->taskMenu)];
}

static void PokeListMenu_SetItems(PokeListWork *wk, PokeListMenu *menu, const u32 *items) {
    int count;
    int i;

    count = 0;
    while (items[count] != POKELIST_MENU_END) {
        count++;
        if (count >= POKELIST_MENU_MAX) {
            break;
        }
    }
    menu->count = 0;
    for (i = 0; i < count; i++) {
        switch (items[i]) {
        case 0:
            menu->items[menu->count] = 0;
            menu->count++;
            break;
        case 1: {
            u8 slot;

            for (slot = 0; slot < 4; slot++) {
                if (PokeList_GetHidenResult(wk->pkm, slot) != 0) {
                    menu->items[menu->count] = POKELIST_MENU_MOVE1 + slot;
                    menu->count++;
                }
            }
            break;
        }
        case 2: {
            u8 slot;

            for (slot = 0; slot < 4; slot++) {
                if (PokeParty_GetParam(wk->pkm, PKM_PARAM_MOVE1 + slot, NULL) != 0) {
                    menu->items[menu->count] = POKELIST_MENU_MOVE1 + slot;
                    menu->count++;
                }
            }
            break;
        }
        case 3:
            menu->items[menu->count] = 3;
            menu->count++;
            break;
        case 4:
            menu->items[menu->count] = 4;
            menu->count++;
            break;
        case 5:
            menu->items[menu->count] = 5;
            menu->count++;
            break;
        case 6:
            menu->items[menu->count] = 6;
            menu->count++;
            break;
        case 7:
            menu->items[menu->count] = 7;
            menu->count++;
            break;
        case 8: {
            int entry = PokeListPlate_GetEntry(wk->plates[wk->cursorPos]);

            if (entry == POKELIST_ENTRY_ABLE) {
                menu->items[menu->count] = 20;
                menu->count++;
            } else if (entry <= 5) {
                menu->items[menu->count] = 21;
                menu->count++;
            }
            break;
        }
        case 9:
            menu->items[menu->count] = 9;
            menu->count++;
            break;
        case 10:
            menu->items[menu->count] = 10;
            menu->count++;
            break;
        case 22:
            menu->items[menu->count] = 22;
            menu->count++;
            break;
        case 11:
            menu->items[menu->count] = 11;
            menu->count++;
            break;
        case 12:
            menu->items[menu->count] = 12;
            menu->count++;
            break;
        case 13:
            menu->items[menu->count] = 13;
            menu->count++;
            break;
        case 23:
            menu->items[menu->count] = 23;
            menu->count++;
            break;
        case 14:
            menu->items[menu->count] = 14;
            menu->count++;
            break;
        case 15:
            menu->items[menu->count] = 15;
            menu->count++;
            break;
        }
    }
    for (i = 0; i < menu->count; i++) {
        menu->entries[i].str = PokeListMenu_GetItemName(wk, menu, menu->items[i]);
        if (menu->items[i] >= POKELIST_MENU_MOVE1 && menu->items[i] <= POKELIST_MENU_MOVE1 + 3 &&
            wk->param->mode != 5) {
            menu->entries[i].color = 0x35e0;
        } else {
            menu->entries[i].color = 0x39e0;
        }
        menu->entries[i].type = menu->items[i] == 6 ? APP_TASKMENU_ITEM_RETURN : 0;
    }
}

static StrBuf *PokeListMenu_GetItemName(PokeListWork *wk, PokeListMenu *menu, int item) {
    StrBuf *str;

    if (item >= POKELIST_MENU_MOVE1 && item <= POKELIST_MENU_MOVE1 + 3) {
        u32 move = PokeParty_GetParam(wk->pkm, PKM_PARAM_MOVE1 + item - POKELIST_MENU_MOVE1, NULL);
        MsgData *moveNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MOVE_NAMES, wk->heapId);

        str = GFL_MsgDataLoadStrbufNew(moveNames, move);
        GFL_MsgDataFree(moveNames);
    } else {
        str = GFL_MsgDataLoadStrbufNew(wk->msgData, sItemNames[item]);
    }
    return str;
}

// A button of a battle's selection, at a tile of the bottom bar
void *PokeListMenu_CreateButton(PokeListWork *wk, PokeListMenu *menu, u32 msgId, u8 x, u8 y, BOOL isBack) {
    AppTaskMenuItem item;
    void *button;

    item.str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);
    item.color = 0x39e3;
    if (isBack == FALSE) {
        item.type = 0;
    } else {
        item.type = APP_TASKMENU_ITEM_RETURN;
    }
    button = AppTaskMenuWin_Create(wk->taskMenuRes, &item, x, y, 10, wk->heapId);
    GFL_StrBufFree(item.str);
    return button;
}

void PokeListMenu_FreeButton(void *button) {
    if (button != NULL) {
        AppTaskMenuWin_Free(button);
    }
}

void PokeListMenu_UpdateButton(void *button) {
    if (button != NULL) {
        AppTaskMenuWin_Update(button);
    }
}

void PokeListMenu_SetButtonActive(void *button, BOOL active) {
    if (button != NULL) {
        AppTaskMenuWin_SetActive(button, active);
    }
}

void PokeListMenu_SetButtonPressed(void *button, BOOL pressed) {
    if (button != NULL) {
        AppTaskMenuWin_SetFlashing(button, pressed);
    }
}
