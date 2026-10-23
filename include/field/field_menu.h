#ifndef POKEBW2_FIELD_FIELD_MENU_H
#define POKEBW2_FIELD_FIELD_MENU_H

#include "types.h"
#include "field/app_call.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/game_event.h"

struct FieldMenuWork {
    u16 code;
    u16 pad02;
    GameEvent *event;
    GameSystem *gameSystem;
    Field *field;
    u32 unk10;
    u32 state;
    u32 screenId;
    // The app the menu opens, whose callbacks get the menu as their arg
    FieldAppCallInput appCall;
};

GameEventReturnCode EventFieldMenu_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventFieldMenu_Create(GameSystem *gsys, Field *field, u16 param);
GameEvent *EventFieldMenu_CreateUnionRoom(GameSystem *gsys, Field *field, u16 param);

// Overlay 36: the event that returns from the menu to the subscreen
GameEvent *EventFieldMenuReturn_Create(GameSystem *gsys, Field *field, u32 screenId);

// Overlay 36's field_menu.c: the menu on the touch screen, which the subscreen runs

// The items of the menu, which FieldMenu_GetSelectedItem returns and the game data keeps for the cursor: the return
// button at the bottom, which also ends a kind of menu's list of items, the apps' items, and an empty slot
enum {
    FIELD_MENU_ITEM_RETURN,
    FIELD_MENU_ITEM_POKEMON,
    FIELD_MENU_ITEM_POKEDEX,
    FIELD_MENU_ITEM_BAG,
    FIELD_MENU_ITEM_TRAINER_CARD,
    FIELD_MENU_ITEM_SAVE,
    FIELD_MENU_ITEM_OPTIONS,
    FIELD_MENU_ITEM_NONE,
    FIELD_MENU_ITEM_RETIRE,
    FIELD_MENU_ITEM_MAX,
};

// The kinds of menu, which FieldMenu_GetMenuType gives by the zone and the story: the full menu, the Union Room's,
// the menus before the Pokédex and before the first Pokémon, Royal Unova's, the menu while the season is forced to
// sync, and the Black Tower's and the White Treehollow's, with the item that leaves them
enum {
    FIELD_MENU_TYPE_NORMAL,
    FIELD_MENU_TYPE_UNION_ROOM,
    FIELD_MENU_TYPE_NO_POKEDEX,
    FIELD_MENU_TYPE_NO_POKEMON,
    FIELD_MENU_TYPE_ROYAL_UNOVA,
    FIELD_MENU_TYPE_FORCE_SEASON_SYNC,
    FIELD_MENU_TYPE_INF_DUNGEON,
    FIELD_MENU_TYPE_MAX,
};

// What the menu leaves with FieldSubscreen_SaveReturnSubscreen once it has closed, for EventFieldMenu_Callback: an
// item was chosen, or the menu was cancelled
enum {
    FIELD_MENU_RESULT_SELECTED = 3,
    FIELD_MENU_RESULT_CANCELLED,
};

FieldMenu *FieldMenu_Create(HeapID heapId, HeapID tmpHeapId, FieldSubscreen *subscreen, Field *field, BOOL open);
void FieldMenu_Free(FieldMenu *menu);
void FieldMenu_Update(FieldMenu *menu);
// Scrolls the menu's BGs, once a frame
void FieldMenu_UpdateScroll(FieldMenu *menu);
// Loads the bar at the bottom of the touch screen
void FieldMenu_LoadBar(HeapID heapId, BOOL hideBg4);
// The kind of menu the zone has (FIELD_MENU_TYPE_)
u8 FieldMenu_GetMenuType(GameData *gameData, EventWork *eventWork, u32 zoneId);
// The item chosen (FIELD_MENU_ITEM_), and moves the cursor to an item
u32 FieldMenu_GetSelectedItem(FieldMenu *menu);
void FieldMenu_SetCursorItem(FieldMenu *menu, u32 itemId);

#endif // POKEBW2_FIELD_FIELD_MENU_H
