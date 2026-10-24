#ifndef POKEBW2_APP_BAG_ITEM_H
#define POKEBW2_APP_BAG_ITEM_H

#include "types.h"
#include "constants/items.h"
#include "gfl/heap.h"
#include "save/bag.h"
#include "struct_decls.h"

// The bag's list of the items in the Free Space, in overlay 142 (bag_item.c)

// The filters of the list: every item, or the items of one pocket
enum {
    BAG_ITEM_FILTER_ALL,
    BAG_ITEM_FILTER_ITEMS,
    BAG_ITEM_FILTER_MEDICINE,
    BAG_ITEM_FILTER_TMS_HMS,
    BAG_ITEM_FILTER_BERRIES,
    BAG_ITEM_FILTER_KEY_ITEMS,
};

typedef struct {
    u16 item;
    // The pocket the item belongs in, or BAG_POCKET_NONE for an empty entry
    u16 pocket;
    // Whether the filter shows the item
    BOOL shown;
} BagItemListEntry;

struct BagItemList {
    // What the list returns for an item it doesn't have
    BagItem empty;
    BagSave *bag;
    HeapID heapId;
    BagItemListEntry entries[ITEM_LAST + 1];
    // Whether each item is in one of the bag's pockets
    u8 inBag[ITEM_LAST + 1];
};

// filter is a BAG_ITEM_FILTER_*
void BagItemList_Init(BagItemList *list, BagSave *bag, u32 filter, HeapID heapId);
void BagItemList_Exit(BagItemList *list);
// The slot of the index-th item of a pocket that isn't in the Free Space, or of the Free Space's index-th shown item
BagItem *BagItemList_GetItem(BagItemList *list, u16 pocket, u16 index);
// Takes the index-th shown item off the list, and out of the Free Space if release is TRUE
void BagItemList_Remove(BagItemList *list, u16 index, BOOL release);
// Moves an item to the Free Space and adds it to the list
void BagItemList_Add(BagItemList *list, u32 item, s16 pocket);
// The number of items on the list that belong in a pocket
s32 BagItemList_CountInPocket(BagItemList *list, u32 pocket);
// Shows the items the filter, a BAG_ITEM_FILTER_*, picks and sorts the list for it
void BagItemList_SetFilter(BagItemList *list, u16 filter);
// The number of items a pocket shows: those not in the Free Space, or the Free Space's shown items
u32 BagItemList_CountShown(BagItemList *list, u32 pocket);

#endif // POKEBW2_APP_BAG_ITEM_H
