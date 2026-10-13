#include "types.h"
#include "app/musical/mus_item_draw.h"
#include "constants/arc.h"
#include "field/musical.h"
#include "gfl/arc.h"
#include "gfl/blact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nnsys/gfd.h"

// Overlay 209's mus_item_draw.c: the props of the musical, drawn as billboard actors. A prop's texture comes from
// ARCID_MUSICAL_ITEM, its size, offset and the positions it fits from overlay 210's table

// ARCID_MUSICAL_ITEM holds the props' textures, then two other files. Out of range is drawn as prop 100

static u16 MusItemDraw_GetTexFileId(int itemId);
static void MusItemDraw_GetFaceSize(MusItemDraw *item, u8 *width, u8 *height);
static u16 MusItemDraw_GetFreeIndex(MusItemDrawSys *sys);
static MusItemDraw *MusItemDraw_InitActor(MusItemDrawSys *sys, u16 index, const VecFx32 *pos, s16 scaleX, s16 scaleY);

// Nothing reads these
const BOOL MUS_ITEM_DRAW_UNUSED_0 = TRUE;
const BOOL MUS_ITEM_DRAW_UNUSED_1 = TRUE;

static const BOOL MUS_ITEM_DRAW_SHOW = TRUE;

MusItemDrawSys *MusItemDraw_InitSystem(BlActScene *blact, u16 count, HeapID heapId) {
    int i;
    MusItemDrawSys *sys = GFL_HeapAllocate(heapId, sizeof(MusItemDrawSys), FALSE, "mus_item_draw.c", 85);

    sys->items = GFL_HeapAllocate(heapId, sizeof(MusItemDraw) * count, FALSE, "mus_item_draw.c", 87);
    sys->heapId = heapId;
    sys->count = count;
    sys->blact = blact;
    for (i = 0; i < sys->count; i++) {
        sys->items[i].active = FALSE;
        sys->items[i].update = FALSE;
    }
    sys->itemData = MusItemData_Init(heapId);
    for (i = 0; i < 16; i++) {
        sys->shadowPalette[i] = GX_RGB(22, 18, 22);
    }
    return sys;
}

void MusItemDraw_TermSystem(MusItemDrawSys *sys) {
    int i;

    GfdClearVramTransferQueue();
    MusItemData_Free(sys->itemData);
    for (i = 0; i < sys->count; i++) {
        if (sys->items[i].active == TRUE) {
            MusItemDraw_DelItem(sys, &sys->items[i]);
        }
    }
    GFL_HeapFree(sys->items);
    GFL_HeapFree(sys);
}

void MusItemDraw_UpdateSystem(MusItemDrawSys *sys) {
    int i;

    for (i = 0; i < sys->count; i++) {
        MusItemDraw *item = &sys->items[i];

        if (sys->items[i].active == TRUE && sys->items[i].update == TRUE) {
            fx32 dx;
            fx32 dy;
            VecFx32 pos;
            s32 offset[2];
            s16 scaleX;
            s16 scaleY;
            u8 width;
            u8 height;

            if (item->useOffset == TRUE) {
                MusItemData_GetOffset(item->data, offset);
                if (item->scaleX < 0) {
                    offset[0] *= -1;
                }
                dx = offset[0] * FX_CosIdx(item->rotation) - offset[1] * FX_SinIdx(item->rotation);
                dy = FX_SinIdx(item->rotation) * offset[0] + offset[1] * FX_CosIdx(item->rotation);
            } else {
                dx = 0;
                dy = 0;
            }
            pos.x = (item->pos.x - dx) / 16;
            pos.y = (FX32_CONST(192) - (item->pos.y - dy)) / 16;
            pos.z = item->pos.z;
            BlActScene_SetActorPos(sys->blact, item->actor, &pos);
            MusItemDraw_GetFaceSize(item, &width, &height);
            scaleX = item->scaleX * width;
            scaleY = item->scaleY * height;
            func_0204ea5c(sys->blact, item->actor, &scaleX, &scaleY);
            func_0204eba0(sys->blact, item->actor, &item->rotation);
            sys->items[i].update = FALSE;
        }
    }
}

void MusItemDraw_DrawSystem(MusItemDrawSys *sys) {
}

static u16 MusItemDraw_GetTexFileId(int itemId) {
    if (itemId >= (u16)GFL_ArcSysGetDataMax(ARCID_MUSICAL_ITEM) - 2) {
        return 100;
    }
    return itemId;
}

static void MusItemDraw_GetFaceSize(MusItemDraw *item, u8 *width, u8 *height) {
    u16 texWidth;
    u16 texHeight;

    func_0204e4d0(MusItemData_GetTexSize(item->data), &texWidth, &texHeight);
    *width = texWidth / 32;
    *height = texHeight / 32;
}

BOOL MusItemDraw_CanEquipPos(MusItemDraw *item, s32 pos) {
    return func_ov210_021eef98(item->data, pos);
}

BOOL MusItemDraw_ItemCanEquipPos(MusItemDrawSys *sys, u16 itemId, s32 pos) {
    return func_ov210_021ef018(MusItemData_GetItem(sys->itemData, itemId), pos);
}

BOOL MusItemDraw_IsItemOfPos(MusItemDrawSys *sys, u16 itemId, s32 pos) {
    return func_ov210_021ef088(MusItemData_GetItem(sys->itemData, itemId), pos);
}

BOOL MusItemDraw_GetItemFlag7(MusItemDraw *item) {
    return func_ov210_021ef0f4(item->data);
}

BOOL MusItemDraw_GetItemFlag9(MusItemDraw *item) {
    return func_ov210_021ef104(item->data);
}

void *MusItemDraw_LoadTexResource(u16 itemId) {
    return GFL_G3DSysReadArcSysResource(ARCID_MUSICAL_ITEM, MusItemDraw_GetTexFileId(itemId));
}

void MusItemDraw_FreeTexResource(void *resource) {
    GFL_G3DResFree(resource);
}

static u16 MusItemDraw_GetFreeIndex(MusItemDrawSys *sys) {
    int i;

    for (i = 0; i < sys->count; i++) {
        if (sys->items[i].active == FALSE) {
            break;
        }
    }
    if (i == sys->count) {
        return 0;
    }
    return i;
}

MusItemDraw *MusItemDraw_AddItem(MusItemDrawSys *sys, u16 itemId, void *texResource, const VecFx32 *pos) {
    int index = MusItemDraw_GetFreeIndex(sys);
    u32 size;
    u8 width;
    u8 height;

    sys->items[index].texFileId = MusItemDraw_GetTexFileId(itemId);
    sys->items[index].data = MusItemData_GetItem(sys->itemData, itemId);
    size = MusItemData_GetTexSize(sys->items[index].data);
    MusItemDraw_GetFaceSize(&sys->items[index], &width, &height);
    sys->items[index].material =
        BlActScene_AddMaterialNewTex(sys->blact, texResource, 0, size, width * 32, height * 32);
    sys->items[index].ownMaterial = TRUE;
    MusItemDraw_InitActor(sys, index, pos, width, height);
    return &sys->items[index];
}

MusItemDraw *MusItemDraw_AddItemWithMaterial(MusItemDrawSys *sys, u16 itemId, u32 material, const VecFx32 *pos) {
    int index = MusItemDraw_GetFreeIndex(sys);
    u8 width;
    u8 height;

    sys->items[index].texFileId = MusItemDraw_GetTexFileId(itemId);
    sys->items[index].data = MusItemData_GetItem(sys->itemData, itemId);
    MusItemData_GetTexSize(sys->items[index].data);
    MusItemDraw_GetFaceSize(&sys->items[index], &width, &height);
    sys->items[index].material = material;
    sys->items[index].ownMaterial = FALSE;
    MusItemDraw_InitActor(sys, index, pos, width, height);
    return &sys->items[index];
}

static MusItemDraw *MusItemDraw_InitActor(MusItemDrawSys *sys, u16 index, const VecFx32 *pos, s16 scaleX, s16 scaleY) {
    sys->items[index].actor =
        BlActScene_AddNewActor(sys->blact, sys->items[index].material, FX32_ONE, FX32_ONE, pos, 31, 0, 0);
    BlActScene_SetActorHidden(sys->blact, sys->items[index].actor, &MUS_ITEM_DRAW_SHOW);
    sys->items[index].active = TRUE;
    sys->items[index].update = TRUE;
    sys->items[index].useOffset = TRUE;
    sys->items[index].shadow = FALSE;
    sys->items[index].pos.x = pos->x;
    sys->items[index].pos.y = pos->y;
    sys->items[index].pos.z = pos->z;
    sys->items[index].scaleX = scaleX;
    sys->items[index].scaleY = scaleY;
    sys->items[index].rotation = 0;
    return &sys->items[index];
}

void MusItemDraw_DelItem(MusItemDrawSys *sys, MusItemDraw *item) {
    BlActScene_ClearActorMaterial(sys->blact, item->actor);
    if (item->ownMaterial == TRUE) {
        func_0204e73c(sys->blact, item->material);
    }
    item->active = FALSE;
    item->update = FALSE;
}

void MusItemDraw_ChangeItem(MusItemDrawSys *sys, MusItemDraw *item, u16 itemId, u16 material) {
    item->texFileId = MusItemDraw_GetTexFileId(itemId);
    item->data = MusItemData_GetItem(sys->itemData, itemId);
    item->shadow = FALSE;
    func_0204ea08(sys->blact, item->actor, &material);
    if (item->ownMaterial == TRUE) {
        func_0204e73c(sys->blact, item->material);
    }
    item->ownMaterial = FALSE;
    item->material = material;
    item->active = TRUE;
    item->update = TRUE;
}

void MusItemDraw_SetShadowPalette(MusItemDrawSys *sys, MusItemDraw *item, void *texResource) {
    if (item->shadow == FALSE) {
        u32 plttAddr;
        NNSGfdPlttKey plttKey;

        func_0204e7e8(sys->blact, item->material, &plttAddr);
        func_0204e820(sys->blact, item->material, &plttKey);
        NNS_GfdRegisterNewVramTransferTask(1, plttAddr, sys->shadowPalette, NNS_GfdGetPlttKeySize(plttKey));
        item->shadow = TRUE;
    }
}

void MusItemDraw_CopyShadowPos(MusItemDrawSys *sys, MusItemDraw *item, MusItemDraw *shadow) {
    shadow->pos.x = item->pos.x + FX32_CONST(2);
    shadow->pos.y = item->pos.y + FX32_CONST(2);
    shadow->pos.z = item->pos.z - FX32_CONST(0.5);
    shadow->scaleX = item->scaleX;
    shadow->scaleY = item->scaleY;
    shadow->rotation = item->rotation;
    shadow->useOffset = item->useOffset;
}

void MusItemDraw_SetVisible(MusItemDrawSys *sys, MusItemDraw *item, BOOL visible) {
    BlActScene_SetActorHidden(sys->blact, item->actor, &visible);
    item->update = TRUE;
}

void MusItemDraw_SetPosition(MusItemDrawSys *sys, MusItemDraw *item, const VecFx32 *pos) {
    item->pos.x = pos->x;
    item->pos.y = pos->y;
    item->pos.z = pos->z;
    item->update = TRUE;
}

void MusItemDraw_GetPosition(MusItemDrawSys *sys, MusItemDraw *item, VecFx32 *pos) {
    pos->x = item->pos.x;
    pos->y = item->pos.y;
    pos->z = item->pos.z;
}

void MusItemDraw_SetSize(MusItemDrawSys *sys, MusItemDraw *item, s16 scaleX, s16 scaleY) {
    item->scaleX = scaleX;
    item->scaleY = scaleY;
    item->update = TRUE;
}

void MusItemDraw_GetSize(MusItemDrawSys *sys, MusItemDraw *item, s16 *scaleX, s16 *scaleY) {
    *scaleX = item->scaleX;
    *scaleY = item->scaleY;
}

void MusItemDraw_SetRotation(MusItemDrawSys *sys, MusItemDraw *item, u16 rotation) {
    item->rotation = rotation;
    item->update = TRUE;
}

void MusItemDraw_GetRotation(MusItemDrawSys *sys, MusItemDraw *item, u16 *rotation) {
    *rotation = item->rotation;
}

void MusItemDraw_SetUseOffset(MusItemDrawSys *sys, MusItemDraw *item, BOOL useOffset) {
    item->useOffset = useOffset;
}

BOOL MusItemDraw_GetUseOffset(MusItemDrawSys *sys, MusItemDraw *item) {
    return item->useOffset;
}

void MusItemDraw_GetItemOffset(MusItemDraw *item, s32 *offset) {
    MusItemData_GetOffset(item->data, offset);
}

void *MusItemDraw_GetItemData(MusItemDrawSys *sys) {
    return sys->itemData;
}
