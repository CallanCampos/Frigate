#include "types.h"
#include "app/musical/mus_item_data.h"
#include "constants/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"

// Overlay 210's mus_item_data.c, named after its string: the table of the musical's props

// The file of ARCID_MUSICAL_ITEM that holds the table, after the props' textures
#define MUS_ITEM_DATA_FILE 101

MusItemData *MusItemData_Init(HeapID heapId) {
    MusItemData *items = GFL_HeapAllocate(heapId, sizeof(MusItemData), FALSE, "mus_item_data.c", 71);

    items->data = GFL_ArcSysReadHeapNewLZ(ARCID_MUSICAL_ITEM, MUS_ITEM_DATA_FILE, FALSE, heapId);
    items->items = items->data;
    return items;
}

void MusItemData_Free(MusItemData *items) {
    GFL_HeapFree(items->data);
    GFL_HeapFree(items);
}

MusicalItemData *MusItemData_GetItem(MusItemData *items, u16 itemId) {
    return &items->items[itemId];
}

void MusItemData_GetOffset(MusicalItemData *item, s32 *offset) {
    offset[0] = item->offsetX;
    offset[1] = item->offsetY;
}

u32 MusItemData_GetTexSize(MusicalItemData *item) {
    return item->texSize;
}

BOOL func_ov210_021eef98(MusicalItemData *item, s32 pos) {
    switch (pos) {
    case 0:
    case 1:
        if (item->flags & 0x1) {
            return TRUE;
        }
        break;
    case 2:
        if (item->flags & 0x2) {
            return TRUE;
        }
        break;
    case 3:
        if (item->flags & 0x4) {
            return TRUE;
        }
        break;
    case 4:
        if (item->flags & 0x8) {
            return TRUE;
        }
        break;
    case 5:
        if (item->flags & 0x10) {
            return TRUE;
        }
        break;
    case 6:
        if (item->flags & 0x20) {
            return TRUE;
        }
        break;
    case 7:
    case 8:
        if (item->flags & 0x40) {
            return TRUE;
        }
        break;
    case 9:
        return FALSE;
    }
    return FALSE;
}

BOOL func_ov210_021ef018(MusicalItemData *item, s32 pos) {
    switch (pos) {
    case 0:
    case 1:
        if (item->flags & 0x1) {
            return TRUE;
        }
        break;
    case 2:
        if (item->flags & 0x2) {
            return TRUE;
        }
        break;
    case 3:
        if (item->flags & 0xc) {
            return TRUE;
        }
        break;
    case 4:
        if (item->flags & 0x10) {
            return TRUE;
        }
        break;
    case 5:
        if (item->flags & 0x20) {
            return TRUE;
        }
        break;
    case 6:
    case 7:
        if (item->flags & 0x40) {
            return TRUE;
        }
        break;
    case 8:
        return FALSE;
    }
    return FALSE;
}

BOOL func_ov210_021ef088(MusicalItemData *item, s32 pos) {
    switch (pos) {
    case 0:
    case 1:
        if (item->category == 0) {
            return TRUE;
        }
        break;
    case 2:
        if (item->category == 1) {
            return TRUE;
        }
        break;
    case 3:
        if (item->category == 2 || item->category == 3) {
            return TRUE;
        }
        break;
    case 4:
        if (item->category == 4) {
            return TRUE;
        }
        break;
    case 5:
        if (item->category == 5) {
            return TRUE;
        }
        break;
    case 6:
    case 7:
        if (item->category == 6) {
            return TRUE;
        }
        break;
    case 8:
        return FALSE;
    }
    return FALSE;
}

BOOL func_ov210_021ef0f4(MusicalItemData *item) {
    if (item->flags & 0x80) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov210_021ef104(MusicalItemData *item) {
    if (item->flags & 0x200) {
        return TRUE;
    }
    return FALSE;
}

s32 func_ov210_021ef118(s32 pos) {
    switch (pos) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    case 3:
    case 4:
        return 3;
    case 5:
        return 4;
    case 6:
        return 5;
    case 7:
        return 6;
    case 8:
        return 7;
    case 9:
        return 8;
    }
    return 9;
}

u32 func_ov210_021ef164(MusItemData *items, u16 itemId) {
    return MusItemData_GetItem(items, itemId)->unk9;
}

u8 MusItemData_GetEffect(MusItemData *items, u16 itemId) {
    return MusItemData_GetItem(items, itemId)->effect;
}

u8 MusItemData_GetCategory(MusItemData *items, u16 itemId) {
    return MusItemData_GetItem(items, itemId)->category;
}
