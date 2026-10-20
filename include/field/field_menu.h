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
FieldMenu *FieldMenu_Create(HeapID heapId, HeapID tmpHeapId, FieldSubscreen *subscreen, Field *field, BOOL open);
void FieldMenu_Free(FieldMenu *menu);
void FieldMenu_Update(FieldMenu *menu);
// Scrolls the menu's BGs, once a frame
void FieldMenu_UpdateScroll(FieldMenu *menu);
// Loads the bar at the bottom of the touch screen
void FieldMenu_LoadBar(HeapID heapId, BOOL hideBg4);
// The kind of menu the zone has
u8 FieldMenu_GetMenuType(GameData *gameData, EventWork *eventWork, u32 zoneId);
// The item chosen, and moves the cursor to an item
u32 FieldMenu_GetSelectedItem(FieldMenu *menu);
void FieldMenu_SetCursorItem(FieldMenu *menu, u32 itemId);

#endif // POKEBW2_FIELD_FIELD_MENU_H
