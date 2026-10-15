#ifndef POKEBW2_FIELD_DUP_FITTING_ITEM_H
#define POKEBW2_FIELD_DUP_FITTING_ITEM_H

// Overlay 208's dup_fitting_item.c (named after its string): the dressing room's props. Each prop on the screen is a
// node of one of the dressing room's lists, which holds the entry of the prop that it shows and the overlay 209
// drawing of it

#include "types.h"
#include "app/musical/mus_item_draw.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

// A point of the touch screen
typedef struct {
    s32 x;
    s32 y;
} DupFittingPoint;

// A prop that the player owns, as the dressing room keeps it
typedef struct {
    u16 itemId;
    // The materials of the BlAct scene that hold its texture, in colors and dimmed
    u32 material;
    u32 dimMaterial;
    // Whether it is on the stage or worn
    BOOL used;
    // Whether the game flags it, which sorts it first
    BOOL isNew;
} DupFittingItemEntry;

typedef struct DupFittingItemNode DupFittingItemNode;

struct DupFittingItemNode {
    u32 unk0;
    // The prop that it shows now, and the one that it will show when the drawing is next updated
    DupFittingItemEntry *entry;
    DupFittingItemEntry *nextEntry;
    MusItemDraw *draw;
    // Where it is on the touch screen
    DupFittingPoint pos;
    s32 scale;
    // How many frames it still moves for, when it goes back to the shelf
    u16 frames;
    // The position of the Pokémon that it is worn at, or 10 when it is not worn
    s32 equipPos;
    // Where it was before it moved
    DupFittingPoint origin;
    s32 baseScale;
    DupFittingItemNode *next;
};

// A list of nodes, with the most that it holds
typedef struct {
    u16 heapId;
    u16 unk2;
    u16 count;
    u16 max;
    DupFittingItemNode *head;
    DupFittingItemNode *tail;
} DupFittingItemList;

// Creates a list that holds at most max nodes
DupFittingItemList *DupFittingItemList_Create(HeapID heapId, u16 max);
void DupFittingItemList_Free(DupFittingItemList *list);
// Adds a node after the last one, or before the first one. Nothing happens when the list is full
void DupFittingItemList_AddTail(DupFittingItemList *list, DupFittingItemNode *node);
void DupFittingItemList_AddHead(DupFittingItemList *list, DupFittingItemNode *node);
void DupFittingItemList_Remove(DupFittingItemList *list, DupFittingItemNode *node);
DupFittingItemNode *DupFittingItemList_GetHead(DupFittingItemList *list);
u16 DupFittingItemList_GetCount(DupFittingItemList *list);
BOOL DupFittingItemList_IsFull(DupFittingItemList *list);
// Makes a node that shows an entry's prop at a position of the scene
DupFittingItemNode *DupFittingItemNode_Create(HeapID heapId, MusItemDrawSys *sys, DupFittingItemEntry *entry,
                                              const VecFx32 *pos);
void DupFittingItemNode_Delete(DupFittingItemNode *node, MusItemDrawSys *sys);
// Shows another entry's prop
void DupFittingItemNode_ChangeEntry(DupFittingItemNode *node, MusItemDrawSys *sys, DupFittingItemEntry *entry);
DupFittingItemNode *DupFittingItemNode_GetNext(DupFittingItemNode *node);
DupFittingItemEntry *DupFittingItemNode_GetEntry(DupFittingItemNode *node);
void DupFittingItemNode_SetNextEntry(DupFittingItemNode *node, DupFittingItemEntry *entry);
DupFittingItemEntry *DupFittingItemNode_GetNextEntry(DupFittingItemNode *node);
MusItemDraw *DupFittingItemNode_GetDraw(DupFittingItemNode *node);
void DupFittingItemNode_SetPos(DupFittingItemNode *node, const DupFittingPoint *pos);
DupFittingPoint *DupFittingItemNode_GetPos(DupFittingItemNode *node);
void DupFittingItemNode_SetOrigin(DupFittingItemNode *node, const DupFittingPoint *origin);
DupFittingPoint *DupFittingItemNode_GetOrigin(DupFittingItemNode *node);
void DupFittingItemNode_SetBaseScale(DupFittingItemNode *node, s32 baseScale);
s32 DupFittingItemNode_GetBaseScale(DupFittingItemNode *node);
void DupFittingItemNode_SetScale(DupFittingItemNode *node, s32 scale);
s32 DupFittingItemNode_GetScale(DupFittingItemNode *node);
void DupFittingItemNode_SetFrames(DupFittingItemNode *node, u16 frames);
u16 DupFittingItemNode_GetFrames(DupFittingItemNode *node);
void DupFittingItemNode_SetEquipPos(DupFittingItemNode *node, s32 equipPos);
s32 DupFittingItemNode_GetEquipPos(DupFittingItemNode *node);
// Whether a point of the touch screen is on the node's prop, within 16 pixels of where its center is
BOOL DupFittingItemNode_IsAtPoint(DupFittingItemNode *node, s32 x, s32 y);
// How far a point is from the node's prop, squared
u32 DupFittingItemNode_GetDistanceSq(DupFittingItemNode *node, s32 x, s32 y);

#endif // POKEBW2_FIELD_DUP_FITTING_ITEM_H
