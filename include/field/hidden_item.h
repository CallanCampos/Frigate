#ifndef POKEBW2_FIELD_HIDDEN_ITEM_H
#define POKEBW2_FIELD_HIDDEN_ITEM_H

// Overlay 12's hidden_item.c (a descriptive name): the table of the items hidden in the field, by their script ID.
// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

typedef struct {
    // The item's flag, from HIDDEN_ITEM_FLAG_START
    u16 index;
    // Whether the Dowsing MCHN finds it from the neighboring zones of the outdoor map
    u8 outdoors;
    // Whether the item comes back after it is found
    u8 respawn;
    u16 zoneId;
    // Its position in the map
    u16 x;
    u16 z;
} HiddenItem;

// The event flag of the first hidden item, which HiddenItem.index is added to
#define HIDDEN_ITEM_FLAG_START 0xb04

u16 GetHiddenItemNoBySCRID(u16 scrId);
u16 GetHiddenItemEventFlagNoBySCRID(u16 scrId);
void TryClearRepeatableHiddenItemFlags(EventWork *eventWork);
// The table, for overlay 86
const HiddenItem *func_ov012_0215f2c4(u16 *count);

#endif // POKEBW2_FIELD_HIDDEN_ITEM_H
