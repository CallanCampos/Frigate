#ifndef POKEBW2_APP_MUSICAL_MUS_ITEM_DRAW_H
#define POKEBW2_APP_MUSICAL_MUS_ITEM_DRAW_H

// Overlay 209's mus_item_draw.c: draws the props the musical's Pokémon wear, each a billboard actor of a BlAct scene.
// The dressing room (overlay 208) and the stage share it

#include "types.h"
#include "field/musical.h"
#include "gfl/blact.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// A prop being drawn
typedef struct {
    u16 active : 1;
    // Whether its actor needs its position, scale and rotation again
    u16 update : 1;
    // Whether it is moved by its offset from where it is worn
    u16 useOffset : 1;
    // Whether its palette is the shadow's
    u16 shadow : 1;
    // Whether its material is its own, to free with it
    u16 ownMaterial : 1;
    u16 rotation;
    u32 material;
    u32 actor;
    u32 texFileId;
    VecFx32 pos;
    s16 scaleX;
    s16 scaleY;
    MusicalItemData *data;
} MusItemDraw;

struct MusItemDrawSys {
    HeapID heapId;
    BlActScene *blact;
    MusItemDraw *items;
    // Overlay 210's table of the props
    void *itemData;
    u16 count;
    GXRgb shadowPalette[16];
};

MusItemDrawSys *MusItemDraw_InitSystem(BlActScene *blact, u16 count, HeapID heapId);
void MusItemDraw_TermSystem(MusItemDrawSys *sys);
void MusItemDraw_UpdateSystem(MusItemDrawSys *sys);
void MusItemDraw_DrawSystem(MusItemDrawSys *sys);
BOOL MusItemDraw_CanEquipPos(MusItemDraw *item, s32 pos);
BOOL MusItemDraw_ItemCanEquipPos(MusItemDrawSys *sys, u16 itemId, s32 pos);
BOOL MusItemDraw_IsItemOfPos(MusItemDrawSys *sys, u16 itemId, s32 pos);
BOOL MusItemDraw_GetItemFlag7(MusItemDraw *item);
BOOL MusItemDraw_GetItemFlag9(MusItemDraw *item);
// The texture of a prop, as a 3D resource
void *MusItemDraw_LoadTexResource(u16 itemId);
void MusItemDraw_FreeTexResource(void *resource);
MusItemDraw *MusItemDraw_AddItem(MusItemDrawSys *sys, u16 itemId, void *texResource, const VecFx32 *pos);
// Adds a prop that draws with a material the scene already has
MusItemDraw *MusItemDraw_AddItemWithMaterial(MusItemDrawSys *sys, u16 itemId, u32 material, const VecFx32 *pos);
void MusItemDraw_DelItem(MusItemDrawSys *sys, MusItemDraw *item);
void MusItemDraw_ChangeItem(MusItemDrawSys *sys, MusItemDraw *item, u16 itemId, u16 material);
// Gives the prop the shadow's palette
void MusItemDraw_SetShadowPalette(MusItemDrawSys *sys, MusItemDraw *item, void *texResource);
// Puts a shadow prop a little behind and below right of its prop
void MusItemDraw_CopyShadowPos(MusItemDrawSys *sys, MusItemDraw *item, MusItemDraw *shadow);
void MusItemDraw_SetVisible(MusItemDrawSys *sys, MusItemDraw *item, BOOL visible);
void MusItemDraw_SetPosition(MusItemDrawSys *sys, MusItemDraw *item, const VecFx32 *pos);
void MusItemDraw_GetPosition(MusItemDrawSys *sys, MusItemDraw *item, VecFx32 *pos);
void MusItemDraw_SetSize(MusItemDrawSys *sys, MusItemDraw *item, s16 scaleX, s16 scaleY);
void MusItemDraw_GetSize(MusItemDrawSys *sys, MusItemDraw *item, s16 *scaleX, s16 *scaleY);
void MusItemDraw_SetRotation(MusItemDrawSys *sys, MusItemDraw *item, u16 rotation);
void MusItemDraw_GetRotation(MusItemDrawSys *sys, MusItemDraw *item, u16 *rotation);
void MusItemDraw_SetUseOffset(MusItemDrawSys *sys, MusItemDraw *item, BOOL useOffset);
BOOL MusItemDraw_GetUseOffset(MusItemDrawSys *sys, MusItemDraw *item);
void MusItemDraw_GetItemOffset(MusItemDraw *item, s32 *offset);
void *MusItemDraw_GetItemData(MusItemDrawSys *sys);

#endif // POKEBW2_APP_MUSICAL_MUS_ITEM_DRAW_H
