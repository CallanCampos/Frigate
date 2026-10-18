#include "battle/b_bag_obj.h"
#include "battle/b_app_tool.h"
#include "battle/b_bag_item.h"
#include "battle/b_bag_main.h"
#include "battle/btlv_effect.h"
#include "battle/btlv_finger_cursor.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "nitro/gx.h"
#include "pml/item.h"
#include "system/palanm.h"
#include "system/shooter_item.h"

// The battle bag's OBJ resources and cell actors: the item icons, the Wonder Launcher's costs and energy gauges,
// overlay 285's cursor and overlay 168's finger cursor of the catching demo. The file name is our guess, after the
// ROM's b_bag_main.c and the b_bag_* files of Diamond and Pearl.

// How BBagObj_CreateActor creates each of the bag's actors, by index into the work's resources
typedef struct {
    u32 chars;
    u32 palette;
    u32 cellAnims;
} BBagActorSetup;

static void BBagObj_ClearResources(BBagWork *wk);
static void BBagObj_LoadItemIconRes(BBagWork *wk);
static void BBagObj_LoadItemIcon(BBagWork *wk, u16 item, u32 chars, u32 row);
static void BBagObj_LoadItemIconPalette(BBagWork *wk, u16 item, u32 row);
static void BBagObj_LoadGaugeRes(BBagWork *wk);
static void BBagObj_LoadCursorRes(BBagWork *wk);
static ClActor *BBagObj_CreateActor(BBagWork *wk, const BBagActorSetup *data);
static void BBagObj_CreateActors(BBagWork *wk);
static void BBagObj_ShowAt(ClActor *actor, const ClActorPos *pos);
static void BBagObj_ShowPockets(BBagWork *wk);
static void BBagObj_ShowItemList(BBagWork *wk);
static void BBagObj_ShowItem(BBagWork *wk);
static void BBagObj_InitCursor(BBagWork *wk);
static void BBagObj_ExitCursor(BBagWork *wk);
static void BBagObj_CreateFingerCursor(BBagWork *wk);
static void BBagObj_DeleteFingerCursor(BBagWork *wk);
static void BBagObj_ShowFingerCursor(BBagWork *wk, s16 x, s16 y);
static void BBagObj_HideFingerCursor(BBagWork *wk);
static void BBagObj_ShowCost(BBagWork *wk, u16 actor, u8 cost, u8 spent, const ClActorPos *pos);

// Declared in the order that MWCC's sort by size lays out in address order (rodata_order.py)

// The last used item on page 0
static const ClActorPos data_ov286_021f6ec8 = { 0x24, 0xb4 };
// The item's cost on page 2
static const ClActorPos data_ov286_021f6ed0 = { 0xa8, 0x28 };
// The item on page 2
static const ClActorPos data_ov286_021f6ecc = { 0x28, 0x2c };
// Page 1: the items, then their costs, then the energy
static const ClActorPos data_ov286_021f6ed4[13] = {
    { 0x24, 0x2d }, { 0xa4, 0x2d }, { 0x24, 0x5d }, { 0xa4, 0x5d }, { 0x24, 0x8d }, { 0xa4, 0x8d }, { 0x34, 0x29 },
    { 0xb4, 0x29 }, { 0x34, 0x59 }, { 0xb4, 0x59 }, { 0x34, 0x89 }, { 0xb4, 0x89 }, { 0x64, 0xac },
};

static const BBagActorSetup data_ov286_021f6f08[BBAG_ACTOR_MAX] = {
    { 0, 0, 0 }, { 1, 1, 0 }, { 2, 2, 0 }, { 3, 3, 0 }, { 4, 4, 0 }, { 5, 5, 0 }, { 6, 6, 0 }, { 7, 7, 1 }, { 7, 7, 1 },
    { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 },
    { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 },
    { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 },
    { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 },
    { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 },
    { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 }, { 7, 7, 1 },
};

void BBagObj_Init(BBagWork *wk) {
    BBagObj_ClearResources(wk);
    BBagObj_LoadItemIconRes(wk);
    BBagObj_LoadGaugeRes(wk);
    BBagObj_LoadCursorRes(wk);
    BBagObj_CreateActors(wk);
    BBagObj_InitCursor(wk);
    BBagObj_CreateFingerCursor(wk);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void BBagObj_ClearResources(BBagWork *wk) {
    u32 i;

    for (i = 0; i < NELEMS(wk->charRes); i++) {
        wk->charRes[i] = -1;
    }
    for (i = 0; i < NELEMS(wk->plttRes); i++) {
        wk->plttRes[i] = -1;
    }
    for (i = 0; i < NELEMS(wk->cellRes); i++) {
        wk->cellRes[i] = -1;
    }
}

static void BBagObj_LoadItemIconRes(BBagWork *wk) {
    ArcTool *arc;
    u16 item;
    u32 charType;
    u32 plttType;
    u16 i;

    if (wk->param->mode == 1) {
        item = ShooterItem_GetItem(0);
        charType = ITEM_FILE_LIST_ICON_CHAR;
        plttType = ITEM_FILE_LIST_ICON_PLTT;
    } else {
        charType = ITEM_FILE_ICON_CHAR;
        item = 1;
        plttType = ITEM_FILE_ICON_PLTT;
    }
    arc = GFL_ArcSysCreateFileHandle(PML_ItemGetIconArcID(), HEAPID_TAIL(wk->param->heapId));
    for (i = 0; i < 7; i++) {
        wk->charRes[i] =
            func_0204b81c(arc, GetItemGraphicsDatID(item, charType), FALSE, CLACT_VRAM_SUB, wk->param->heapId);
        wk->plttRes[i] =
            func_0204bba0(arc, GetItemGraphicsDatID(item, plttType), CLACT_VRAM_SUB, i * 0x20, wk->param->heapId);
    }
    wk->cellRes[0] = func_0204bde0(arc, PML_ItemGetIconCellDatID(), PML_ItemGetIconAnimDatID(), wk->param->heapId);
    GFL_ArcToolFree(arc);
}

static void BBagObj_LoadItemIcon(BBagWork *wk, u16 item, u32 chars, u32 row) {
    NNSG2dCharacterData *charData;
    BBagParam *param = wk->param;
    ArcTool *arc;
    void *file;
    u32 charType;
    u32 plttType;

    if (param->mode == 1) {
        charType = ITEM_FILE_LIST_ICON_CHAR;
        plttType = ITEM_FILE_LIST_ICON_PLTT;
    } else {
        charType = ITEM_FILE_ICON_CHAR;
        plttType = ITEM_FILE_ICON_PLTT;
    }
    arc = GFL_ArcSysCreateFileHandle(PML_ItemGetIconArcID(), HEAPID_TAIL(param->heapId));
    file = GFL_G2DIOReadOBJNCGRArc(arc, GetItemGraphicsDatID(item, charType), FALSE, &charData, wk->param->heapId);
    func_0204ba40(wk->charRes[chars], charData);
    GFL_HeapFree(file);
    PaletteFade_LoadArcNCLR(wk->paletteFade, arc, GetItemGraphicsDatID(item, plttType), wk->param->heapId,
                            PALFADE_BUFFER_SUB_OBJ, 0x20, row * 16);
    GFL_ArcToolFree(arc);
}

static void BBagObj_LoadItemIconPalette(BBagWork *wk, u16 item, u32 row) {
    BBagParam *param = wk->param;

    if (param->mode == 1) {
        GFL_BGSysLoadNCLRDefault(PML_ItemGetIconArcID(), GetItemGraphicsDatID(item, ITEM_FILE_LIST_ICON_PLTT),
                                 PALTYPE_SUB_OBJ, row * 0x20, 0x20, param->heapId);
    } else {
        GFL_BGSysLoadNCLRDefault(PML_ItemGetIconArcID(), GetItemGraphicsDatID(item, ITEM_FILE_ICON_PLTT),
                                 PALTYPE_SUB_OBJ, row * 0x20, 0x20, param->heapId);
    }
}

static void BBagObj_LoadGaugeRes(BBagWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_B_BAG, HEAPID_TAIL(wk->param->heapId));

    wk->charRes[7] = func_0204b81c(arc, 14, TRUE, CLACT_VRAM_SUB, wk->param->heapId);
    wk->plttRes[7] = func_0204bba0(arc, 15, CLACT_VRAM_SUB, 0xe0, wk->param->heapId);
    wk->cellRes[1] = func_0204bde0(arc, 16, 17, wk->param->heapId);
    PaletteFade_LoadArcNCLR(wk->paletteFade, arc, 15, wk->param->heapId, PALFADE_BUFFER_SUB_OBJ, 0x20, 0x70);
    GFL_ArcToolFree(arc);
}

static void BBagObj_LoadCursorRes(BBagWork *wk) {
    NNSG2dPaletteData *palette;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_BTLV, HEAPID_TAIL(wk->param->heapId));
    void *file;

    wk->charRes[8] = func_0204b81c(arc, 424, FALSE, CLACT_VRAM_SUB, wk->param->heapId);
    wk->plttRes[8] = func_0204bbb8(arc, 421, CLACT_VRAM_SUB, 0x100, 4, 1, wk->param->heapId);
    wk->cellRes[2] = func_0204bde0(arc, 425, 426, wk->param->heapId);
    file = GFL_G2DIOReadNCLRArc(arc, 421, &palette, HEAPID_TAIL(wk->param->heapId));
    PaletteFade_LoadData(wk->paletteFade, (u16 *)palette->rawData + 4 * 16, PALFADE_BUFFER_SUB_OBJ, 0x80, 0x20);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
}

static ClActor *BBagObj_CreateActor(BBagWork *wk, const BBagActorSetup *data) {
    ClActorSetup setup;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = 1;
    setup.bgPriority = 1;
    return func_0204c040(wk->actorUnit, wk->charRes[data->chars], wk->plttRes[data->palette],
                         wk->cellRes[data->cellAnims], &setup, CLACT_SURFACE_SUB, wk->param->heapId);
}

static void BBagObj_CreateActors(BBagWork *wk) {
    u32 i;

    wk->actorUnit = func_0204bf1c(BBAG_ACTOR_MAX + 4, 0, wk->param->heapId);
    for (i = 0; i < NELEMS(wk->actors); i++) {
        wk->actors[i] = BBagObj_CreateActor(wk, &data_ov286_021f6f08[i]);
    }
}

void BBagObj_Exit(BBagWork *wk) {
    u32 i;

    BBagObj_DeleteFingerCursor(wk);
    for (i = 0; i < NELEMS(wk->actors); i++) {
        func_0204c108(wk->actors[i]);
    }
    BBagObj_ExitCursor(wk);
    for (i = 0; i < NELEMS(wk->charRes); i++) {
        if (wk->charRes[i] != -1) {
            func_0204b98c(wk->charRes[i]);
        }
    }
    for (i = 0; i < NELEMS(wk->plttRes); i++) {
        if (wk->plttRes[i] != -1) {
            func_0204bcd0(wk->plttRes[i]);
        }
    }
    for (i = 0; i < NELEMS(wk->cellRes); i++) {
        if (wk->cellRes[i] != -1) {
            func_0204be64(wk->cellRes[i]);
        }
    }
    func_0204bf98(wk->actorUnit);
}

static void BBagObj_ShowAt(ClActor *actor, const ClActorPos *pos) {
    func_0204c124(actor, TRUE);
    func_0204c140(actor, pos, CLACT_SURFACE_SUB);
}

void BBagObj_SetPage(BBagWork *wk, u8 page) {
    u32 i;

    for (i = 0; i < NELEMS(wk->actors); i++) {
        func_0204c124(wk->actors[i], FALSE);
    }
    BBagObj_HideFingerCursor(wk);
    switch (page) {
    case 0:
        BBagObj_ShowPockets(wk);
        break;
    case 1:
        BBagObj_ShowItemList(wk);
        break;
    case 2:
        BBagObj_ShowItem(wk);
        break;
    }
}

static void BBagObj_ShowPockets(BBagWork *wk) {
    if (wk->lastItem != 0) {
        BBagObj_LoadItemIcon(wk, wk->lastItem, 6, 6);
        BBagObj_ShowAt(wk->actors[6], &data_ov286_021f6ec8);
    }
    if (wk->param->mode == 2) {
        BBagObj_ShowFingerCursor(wk, 192, 24);
    }
}

static void BBagObj_ShowItemList(BBagWork *wk) {
    u32 i;

    for (i = 0; i < 6; i++) {
        u16 item = BBagItem_GetSlotItem(wk, i);
        if (item != 0) {
            BBagObj_LoadItemIcon(wk, item, i, i);
            BBagObj_LoadItemIconPalette(wk, item, i);
            BBagObj_ShowAt(wk->actors[BBAG_ACTOR_ITEM + i], &data_ov286_021f6ed4[i]);
            if (wk->param->mode == 1) {
                BBagObj_ShowCost(wk, BBAG_ACTOR_COST + i * 8, BBagItem_GetShooterCost(item), 0,
                                 &data_ov286_021f6ed4[i + 6]);
            }
        }
    }
    if (wk->param->mode == 1) {
        BBagObj_ShowCost(wk, BBAG_ACTOR_ENERGY, wk->param->shooterEnergy, wk->param->shooterSpent,
                         &data_ov286_021f6ed4[i + 6]);
    }
    if (wk->param->mode == 2) {
        BBagObj_ShowFingerCursor(wk, 64, 16);
    }
}

static void BBagObj_ShowItem(BBagWork *wk) {
    u16 item = BBagItem_GetSlotItem(wk, wk->param->rows[wk->pocket]);

    BBagObj_LoadItemIcon(wk, item, 6, 6);
    BBagObj_ShowAt(wk->actors[6], &data_ov286_021f6ecc);
    if (wk->param->mode == 1) {
        BBagObj_ShowCost(wk, BBAG_ACTOR_COST, BBagItem_GetShooterCost(item), 0, &data_ov286_021f6ed0);
    }
    if (wk->param->mode == 2) {
        BBagObj_ShowFingerCursor(wk, 104, 152);
    }
}

static void BBagObj_InitCursor(BBagWork *wk) {
    BAppCursor_CreateActors(wk->cursor, wk->actorUnit, wk->charRes[8], wk->plttRes[8], wk->cellRes[2]);
    BAppCursor_SetVisible(wk->cursor, FALSE);
}

static void BBagObj_ExitCursor(BBagWork *wk) {
    BAppCursor_DeleteActors(wk->cursor);
}

static void BBagObj_CreateFingerCursor(BBagWork *wk) {
    wk->fingerCursor = BtlvFingerCursor_Create(wk->paletteFade, 9, wk->param->heapId);
}

static void BBagObj_DeleteFingerCursor(BBagWork *wk) {
    BtlvFingerCursor_Delete(wk->fingerCursor);
}

static void BBagObj_ShowFingerCursor(BBagWork *wk, s16 x, s16 y) {
    BtlvFingerCursor_Start(wk->fingerCursor, x, y, 2, 6, 20);
}

static void BBagObj_HideFingerCursor(BBagWork *wk) {
    BtlvFingerCursor_RemoveActor(wk->fingerCursor);
}

static void BBagObj_ShowCost(BBagWork *wk, u16 actor, u8 cost, u8 spent, const ClActorPos *pos) {
    ClActorPos p;
    s8 c;
    s8 s;
    u16 i;

    BBagObj_ShowAt(wk->actors[actor], pos);
    func_0204c488(wk->actors[actor], cost);
    actor++;
    c = cost;
    s = spent;
    p = *pos;
    p.x += 12;
    for (i = 0; i < 7; i++) {
        if (c <= 0) {
            if (s <= 0) {
                func_0204c488(wk->actors[actor], 15);
            } else if (s == 1) {
                func_0204c488(wk->actors[actor], 19);
                s -= 1;
            } else {
                func_0204c488(wk->actors[actor], 20);
                s -= 2;
            }
        } else if (c == 1) {
            if (s == 0) {
                func_0204c488(wk->actors[actor], 16);
            } else if (s > 0) {
                func_0204c488(wk->actors[actor], 18);
                s -= 1;
            }
            c -= 1;
        } else {
            func_0204c488(wk->actors[actor], 17);
            c -= 2;
        }
        BBagObj_ShowAt(wk->actors[actor], &p);
        p.x += 8;
        actor++;
    }
}
