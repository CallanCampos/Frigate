#include "types.h"
#include "app/musical/mus_item_draw.h"
#include "field/dup_fitting_item.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

// Overlay 208's dup_fitting_item.c, named after its string: the lists and nodes of the props in the dressing room.
// Its function names are guessed from what the dressing room does with them

DupFittingItemList *DupFittingItemList_Create(HeapID heapId, u16 max) {
    DupFittingItemList *list = GFL_HeapAllocate(heapId, sizeof(DupFittingItemList), FALSE, "dup_fitting_item.c", 76);

    list->heapId = heapId;
    list->count = 0;
    list->max = max;
    list->head = NULL;
    list->tail = NULL;
    return list;
}

void DupFittingItemList_Free(DupFittingItemList *list) {
    GFL_HeapFree(list);
}

void DupFittingItemList_AddTail(DupFittingItemList *list, DupFittingItemNode *node) {
    if (list->count < list->max) {
        if (list->head == NULL) {
            list->head = node;
        } else {
            list->tail->next = node;
        }
        list->tail = node;
        node->next = NULL;
        list->count++;
    }
}

void DupFittingItemList_AddHead(DupFittingItemList *list, DupFittingItemNode *node) {
    if (list->count < list->max) {
        if (list->head == NULL) {
            list->head = node;
            list->tail = node;
            node->next = NULL;
        } else {
            node->next = list->head;
            list->head = node;
        }
        list->count++;
    }
}

void DupFittingItemList_Remove(DupFittingItemList *list, DupFittingItemNode *node) {
    DupFittingItemNode *prev = NULL;
    DupFittingItemNode *cur = list->head;

    while (cur != node && cur != NULL) {
        prev = cur;
        cur = cur->next;
    }
    if (cur != NULL) {
        if (prev != NULL) {
            prev->next = cur->next;
        } else {
            list->head = cur->next;
        }
        if (list->tail == node) {
            list->tail = prev;
        }
        cur->next = NULL;
        list->count--;
    }
}

DupFittingItemNode *DupFittingItemList_GetHead(DupFittingItemList *list) {
    return list->head;
}

u16 DupFittingItemList_GetCount(DupFittingItemList *list) {
    return list->count;
}

BOOL DupFittingItemList_IsFull(DupFittingItemList *list) {
    if (list->count == list->max) {
        return TRUE;
    }
    return FALSE;
}

DupFittingItemNode *DupFittingItemNode_Create(HeapID heapId, MusItemDrawSys *sys, DupFittingItemEntry *entry,
                                              const VecFx32 *pos) {
    DupFittingItemNode *node = GFL_HeapAllocate(heapId, sizeof(DupFittingItemNode), FALSE, "dup_fitting_item.c", 204);

    node->next = NULL;
    node->entry = entry;
    node->draw = MusItemDraw_AddItemWithMaterial(sys, entry->itemId, entry->material, pos);
    node->frames = 0;
    node->equipPos = 10;
    return node;
}

void DupFittingItemNode_Delete(DupFittingItemNode *node, MusItemDrawSys *sys) {
    MusItemDraw_DelItem(sys, node->draw);
    GFL_HeapFree(node);
}

void DupFittingItemNode_ChangeEntry(DupFittingItemNode *node, MusItemDrawSys *sys, DupFittingItemEntry *entry) {
    node->entry = entry;
    MusItemDraw_ChangeItem(sys, node->draw, entry->itemId, entry->material);
}

DupFittingItemNode *DupFittingItemNode_GetNext(DupFittingItemNode *node) {
    return node->next;
}

DupFittingItemEntry *DupFittingItemNode_GetEntry(DupFittingItemNode *node) {
    return node->entry;
}

void DupFittingItemNode_SetNextEntry(DupFittingItemNode *node, DupFittingItemEntry *entry) {
    node->nextEntry = entry;
}

DupFittingItemEntry *DupFittingItemNode_GetNextEntry(DupFittingItemNode *node) {
    return node->nextEntry;
}

MusItemDraw *DupFittingItemNode_GetDraw(DupFittingItemNode *node) {
    return node->draw;
}

void DupFittingItemNode_SetPos(DupFittingItemNode *node, const DupFittingPoint *pos) {
    node->pos = *pos;
}

DupFittingPoint *DupFittingItemNode_GetPos(DupFittingItemNode *node) {
    return &node->pos;
}

void DupFittingItemNode_SetOrigin(DupFittingItemNode *node, const DupFittingPoint *origin) {
    node->origin = *origin;
}

DupFittingPoint *DupFittingItemNode_GetOrigin(DupFittingItemNode *node) {
    return &node->origin;
}

void DupFittingItemNode_SetBaseScale(DupFittingItemNode *node, s32 baseScale) {
    node->baseScale = baseScale;
}

s32 DupFittingItemNode_GetBaseScale(DupFittingItemNode *node) {
    return node->baseScale;
}

void DupFittingItemNode_SetScale(DupFittingItemNode *node, s32 scale) {
    node->scale = scale;
}

s32 DupFittingItemNode_GetScale(DupFittingItemNode *node) {
    return node->scale;
}

void DupFittingItemNode_SetFrames(DupFittingItemNode *node, u16 frames) {
    node->frames = frames;
}

u16 DupFittingItemNode_GetFrames(DupFittingItemNode *node) {
    return node->frames;
}

void DupFittingItemNode_SetEquipPos(DupFittingItemNode *node, s32 equipPos) {
    node->equipPos = equipPos;
}

s32 DupFittingItemNode_GetEquipPos(DupFittingItemNode *node) {
    return node->equipPos;
}

BOOL DupFittingItemNode_IsAtPoint(DupFittingItemNode *node, s32 x, s32 y) {
    s32 offset[2];
    u16 rotation;
    s16 dx;
    s16 dy;
    s32 py;

    MusItemDraw_GetItemOffset(node->draw, offset);
    if (MusItemDraw_GetUseOffset(NULL, node->draw) == TRUE) {
        fx16 sin;
        fx16 cos;
        s32 offX;
        s32 offY;

        MusItemDraw_GetRotation(NULL, node->draw, &rotation);
        sin = FX_SinIdx(rotation);
        cos = FX_CosIdx(rotation);
        offX = FX_FX32_TO_F32(offset[0] * cos - offset[1] * sin);
        offY = FX_FX32_TO_F32(offset[0] * sin + offset[1] * cos);
        dx = node->pos.x - offX - x;
        py = node->pos.y - offY;
    } else {
        dx = node->pos.x - x;
        py = node->pos.y;
    }
    dy = py - y;
    if (dx < 16 && dx > -16 && dy < 16 && dy > -16) {
        return TRUE;
    }
    return FALSE;
}

u32 DupFittingItemNode_GetDistanceSq(DupFittingItemNode *node, s32 x, s32 y) {
    s32 offset[2];
    u16 rotation;
    s16 dx;
    s16 dy;
    s32 py;

    MusItemDraw_GetItemOffset(node->draw, offset);
    if (MusItemDraw_GetUseOffset(NULL, node->draw) == TRUE) {
        fx16 sin;
        fx16 cos;
        s32 offX;
        s32 offY;

        MusItemDraw_GetRotation(NULL, node->draw, &rotation);
        sin = FX_SinIdx(rotation);
        cos = FX_CosIdx(rotation);
        offX = FX_FX32_TO_F32(offset[0] * cos - offset[1] * sin);
        offY = FX_FX32_TO_F32(offset[0] * sin + offset[1] * cos);
        dx = node->pos.x - offX - x;
        py = node->pos.y - offY;
    } else {
        dx = node->pos.x - x;
        py = node->pos.y;
    }
    dy = py - y;
    return dx * dx + dy * dy;
}
