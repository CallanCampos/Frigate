#ifndef POKEBW2_FIELD_ITEM_USE_BLOCK_H
#define POKEBW2_FIELD_ITEM_USE_BLOCK_H

#include "types.h"
#include "struct_decls.h"

// The messages of global script 2006, which tells the player that an item can't be used, in the order of
// Global10520_Text_JunipersWordsEchoedTheres and the three after it
enum {
    ITEM_USE_BLOCK_MSG_CANT_USE,
    ITEM_USE_BLOCK_MSG_CANT_DISMOUNT,
    ITEM_USE_BLOCK_MSG_NO_RIBBON,
    // The item, named by the script, can't be used while the player walks with a partner
    ITEM_USE_BLOCK_MSG_PAIRED,
};

// Tells the player why a common event (a FIELD_COMMON_EVENT_*) is blocked, by the FieldActionResult that blocks it
void EventFieldItemUseBlock_Call(GameEvent *parent, GameSystem *gsys, u32 eventId, u32 blocked);
GameEvent *EventFieldItemUseBlock_Create(GameSystem *gsys, u32 message, u32 item);

#endif // POKEBW2_FIELD_ITEM_USE_BLOCK_H
