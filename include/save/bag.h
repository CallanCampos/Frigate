#ifndef POKEBW2_SAVE_BAG_H
#define POKEBW2_SAVE_BAG_H

#include "types.h"
#include "constants/bag.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// An item and its count in a pocket (swan's BagItem)
struct BagItem {
    u16 item;
    u16 count;
};

BOOL BagSave_AddItem(BagSave *bag, u16 item, u16 count, u32 heapId);
// BagSave_AddItem, putting the item first in its pocket
BOOL BagSave_AddItemAsFirst(BagSave *bag, u32 item, u32 count, u32 heapId);
// Whether count of an item fit in the bag
BOOL BagSave_CheckAvailItemSpace(BagSave *bag, u16 item, u16 count, HeapID heapId);
// The pocket an item goes in
u32 BagSave_GetActualItemPocket(BagSave *bag, u16 item);
u16 BagSave_GetItemCountByID(BagSave *bag, u16 item, HeapID heapId);
// Whether the bag holds at least count of an item
BOOL BagSave_CheckAmount(BagSave *bag, u32 item, u32 count, u32 heapId);
BOOL BagSave_SubItem(BagSave *bag, u16 item, u16 count, HeapID heapId);
void BagSave_Init(BagSave *bag);
// Replaces the DNA Splicers that fuse (0) with those that separate, or those that separate (1) with those that fuse
void BagSave_SwitchOwnedDNASplicers(BagSave *bag, u32 from);
// The bag's cursor in each pocket, the pocket the bag opens on and the Free Space's filter, in the cursor that
// func_0201734c returns
void func_0200887c(void *cursor, u16 pocket, s16 *row, s16 *scroll);
u16 func_02008890(void *cursor);
void func_02008894(void *cursor, u16 pocket, s16 row, s16 scroll);
void func_020088a4(void *cursor, u16 pocket);
// The battle bag's cursor memory (0x32 bytes, allocated here): the row and page of a pocket, and the last used item and
// its pocket
void *func_02008860(HeapID heapId);
void func_020088a8(void *cursor, u16 pocket, s16 *row, s16 *page);
u16 func_020088bc(void *cursor);
u16 func_020088c0(void *cursor);
void func_020088c4(void *a0, void *a1, void *a2);
void func_020088e0(void *a0, u16 item, u8 a2);
// The Free Space's filter
u8 func_020088e8(void *cursor);
void func_020088ec(void *data, u32 value);
// The bag's Free Space and item slots. Function names from swan, except func_0200891c and func_0200896c
// Whether an item's Free Space bit is set
BOOL BagSave_IsItemFreeSpaceBit(BagSave *bag, u16 item);
// Whether an item is in the Free Space and in the bag
BOOL BagSave_IsItemInFreeSpace(BagSave *bag, u16 item);
// Sets or clears an item's Free Space bit
void BagSave_MoveBetweenFreeSpace(BagSave *bag, u16 item, BOOL toFreeSpace);
// The pocket that holds an item, or BAG_POCKET_NONE
u32 BagSave_GetExistingItemPocket(BagSave *bag, u16 item);
s32 BagSave_GetPocketItemCount(BagSave *bag, u32 pocket);
// Moves an item to the end of its pocket
void BagSave_ForceItemAsLast(BagSave *bag, u16 item);
// The slot at an index of a pocket, or NULL past its end
BagItem *BagSave_GetItemIndexHandle(BagSave *bag, u32 pocket, u16 index);
// An item's slot in a pocket, or NULL
BagItem *BagSave_GetItemHandle(BagSave *bag, u16 pocket, u16 item);
// The number of slots that hold an item
u32 BagSave_GetUniqueItemCount(BagItem *items, u32 count);
// Sorts the bag's pockets
void sortItemBlocks(BagSave *bag);
void BagSave_CopyPocketRaw(BagSave *bag, BagItem *items, u32 pocket, BOOL load);
s32 BagSave_GetPocketItemCountCore(BagSave *bag, u32 pocket, BOOL a2);
// How often an item was used
u32 func_0200854c(BagSave *bag, u16 item);
// Copies a pocket into items, leaving out the items in the Free Space, or back into the pocket
void BagSave_CopyPocket(BagSave *bag, BagItem *items, u32 pocket, BOOL load);
// Sets the flag of each item in the pockets, in an array of ITEM_LAST + 1 flags
void func_0200891c(BagSave *bag, u8 *inBag);
// A pocket's slots and their number
BagItem *func_0200896c(BagSave *bag, u32 pocket, u32 *count);

#endif // POKEBW2_SAVE_BAG_H
