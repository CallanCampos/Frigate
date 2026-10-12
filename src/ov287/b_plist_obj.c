#include "battle/b_plist_obj.h"
#include "battle/b_app_tool.h"
#include "battle/b_plist_main.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "pml/waza.h"
#include "system/app_menu_common.h"
#include "system/hp_gauge.h"
#include "system/palanm.h"

// The battle party list's OBJ resources and cell actors: the Pokémon icons with their status and item icons, the
// type and category icons of the summary pages, and overlay 285's cursor. The file name is our guess, after the
// ROM's b_plist_main.c and b_plist_anm.c and the b_plist_* files of the earlier games.

// How BPlistObj_CreateActor creates each of the list's actors, by index into the work's resources
typedef struct {
    u32 chars;
    u32 palette;
    u32 cellAnims;
    u32 bgPriority;
    u32 priority;
} BPlistActorSetup;

static void BPlistObj_ClearResources(BPlistWork *wk);
static void BPlistObj_LoadPokeIconRes(BPlistWork *wk);
static void BPlistObj_LoadStatusIconRes(BPlistWork *wk);
static void BPlistObj_LoadTypeIconRes(BPlistWork *wk);
static void BPlistObj_LoadItemIconRes(BPlistWork *wk);
static void BPlistObj_LoadCursorRes(BPlistWork *wk);
static void BPlistObj_LoadPlateIconRes(BPlistWork *wk);
static ClActor *BPlistObj_CreateActor(BPlistWork *wk, const BPlistActorSetup *data);
static void BPlistObj_CreateActors(BPlistWork *wk);
static void BPlistObj_ShowAt(ClActor *actor, const ClActorPos *pos);
static void BPlistObj_SetPokeIconPalettes(BPlistWork *wk);
static void BPlistObj_LoadTypeIcon(BPlistWork *wk, ClActor *actor, u32 chars, u32 type);
static void BPlistObj_LoadCategoryIcon(BPlistWork *wk, u32 actor, u32 chars, u32 category);
static void BPlistObj_ShowStatusIcon(u8 status, ClActor *actor, const ClActorPos *pos);
static void BPlistObj_ShowPokeTypes(BPlistWork *wk, BPlistPokemon *pkm, const ClActorPos *pos);
static void BPlistObj_ShowItemIcon(u16 item, ClActor *actor, const ClActorPos *pos);
static void BPlistObj_ShowList(BPlistWork *wk);
static void BPlistObj_ShowPage1(BPlistWork *wk);
static void BPlistObj_ShowPage2(BPlistWork *wk);
static void BPlistObj_ShowPage3(BPlistWork *wk);
static void BPlistObj_ShowPage4(BPlistWork *wk);
static void BPlistObj_ShowPage6(BPlistWork *wk);
static void BPlistObj_ShowPage7(BPlistWork *wk);
static void BPlistObj_ShowPage5(BPlistWork *wk);
static void BPlistObj_ChangeAnim(ClActor *actor, u16 sequence);
static u16 BPlistObj_GetPokeIconAnim(BPlistPokemon *pkm);
static void BPlistObj_InitCursor(BPlistWork *wk);

// Declared in the order that MWCC's sort by size lays out in address order (rodata_order.py)
static const ClActorPos data_ov287_021fb084 = { 0xc6, 0x14 };
static const ClActorPos data_ov287_021fb08c = { 0x18, 0x0c };
static const ClActorPos data_ov287_021fb07c = { 0x18, 0x0c };
static const ClActorPos data_ov287_021fb088 = { 0x18, 0x0c };
static const ClActorPos data_ov287_021fb074 = { 0xc6, 0x14 };
static const ClActorPos data_ov287_021fb078 = { 0x88, 0x28 };
static const ClActorPos data_ov287_021fb06c = { 0x80, 0x48 };
static const ClActorPos data_ov287_021fb080 = { 0x18, 0x50 };
static const ClActorPos data_ov287_021fb070 = { 0x14, 0x84 };
static const ClActorPos data_ov287_021fb0ac = { 0xdf, 0x34 };
static const ClActorPos data_ov287_021fb0a8 = { 0x18, 0x0c };
static const ClActorPos data_ov287_021fb0a4 = { 0xc6, 0x14 };
static const ClActorPos data_ov287_021fb0a0 = { 0xc6, 0x14 };
static const ClActorPos data_ov287_021fb09c = { 0x18, 0x0c };
static const ClActorPos data_ov287_021fb098 = { 0xc6, 0x14 };
static const ClActorPos data_ov287_021fb094 = { 0x18, 0x50 };
static const ClActorPos data_ov287_021fb090 = { 0x88, 0x28 };
static const ClActorPos data_ov287_021fb0c0[2] = { { 0x82, 0x10 }, { 0xa4, 0x10 } };
static const ClActorPos data_ov287_021fb0c8[2] = { { 0x82, 0x10 }, { 0xa4, 0x10 } };
static const ClActorPos data_ov287_021fb0d0[2] = { { 0x82, 0x10 }, { 0xa4, 0x10 } };
static const ClActorPos data_ov287_021fb0b0[2] = { { 0x82, 0x10 }, { 0xa4, 0x10 } };
static const ClActorPos data_ov287_021fb0b8[2] = { { 0x82, 0x10 }, { 0xa4, 0x10 } };
static const ClActorPos data_ov287_021fb0d8[5] = {
    { 0x1e, 0x50 }, { 0x9e, 0x50 }, { 0x1e, 0x80 }, { 0x9e, 0x80 }, { 0x5e, 0xb0 }
};
static const ClActorPos data_ov287_021fb104[6] = { { 0x1c, 0x28 }, { 0x9c, 0x30 }, { 0x1c, 0x58 },
                                                   { 0x9c, 0x60 }, { 0x1c, 0x88 }, { 0x9c, 0x90 } };
static const ClActorPos data_ov287_021fb0ec[6] = { { 0x10, 0x10 }, { 0x90, 0x18 }, { 0x10, 0x40 },
                                                   { 0x90, 0x48 }, { 0x10, 0x70 }, { 0x90, 0x78 } };
static const ClActorPos data_ov287_021fb11c[6] = { { 0x57, 0x1c }, { 0xd7, 0x24 }, { 0x57, 0x4c },
                                                   { 0xd7, 0x54 }, { 0x57, 0x7c }, { 0xd7, 0x84 } };

static const BPlistActorSetup data_ov287_021fb134[41] = {
    { 7, 2, 2, 1, 0 },  { 7, 2, 2, 1, 0 },  { 7, 2, 2, 1, 0 },  { 7, 2, 2, 1, 0 },  { 7, 2, 2, 1, 0 },
    { 7, 2, 2, 1, 0 },  { 7, 2, 2, 1, 0 },  { 0, 0, 0, 1, 1 },  { 1, 0, 0, 1, 1 },  { 2, 0, 0, 1, 1 },
    { 3, 0, 0, 1, 1 },  { 4, 0, 0, 1, 1 },  { 5, 0, 0, 1, 1 },  { 6, 1, 1, 1, 1 },  { 6, 1, 1, 1, 1 },
    { 6, 1, 1, 1, 1 },  { 6, 1, 1, 1, 1 },  { 6, 1, 1, 1, 1 },  { 6, 1, 1, 1, 1 },  { 25, 5, 5, 2, 0 },
    { 25, 5, 5, 2, 0 }, { 25, 5, 5, 2, 0 }, { 25, 5, 5, 2, 0 }, { 25, 5, 5, 2, 0 }, { 25, 5, 5, 2, 0 },
    { 8, 3, 3, 1, 0 },  { 9, 3, 3, 1, 0 },  { 10, 3, 3, 1, 0 }, { 11, 3, 3, 1, 0 }, { 12, 3, 3, 1, 0 },
    { 13, 3, 3, 1, 0 }, { 14, 3, 3, 1, 0 }, { 15, 3, 3, 1, 0 }, { 16, 3, 3, 1, 0 }, { 17, 3, 3, 1, 0 },
    { 18, 3, 3, 1, 0 }, { 19, 3, 3, 1, 0 }, { 20, 3, 3, 1, 0 }, { 21, 3, 3, 1, 0 }, { 22, 3, 3, 1, 0 },
    { 23, 3, 3, 1, 0 },
};

void BPlistObj_Init(BPlistWork *wk) {
    BPlistObj_ClearResources(wk);
    BPlistObj_LoadPokeIconRes(wk);
    BPlistObj_LoadStatusIconRes(wk);
    BPlistObj_LoadItemIconRes(wk);
    BPlistObj_LoadTypeIconRes(wk);
    BPlistObj_LoadCursorRes(wk);
    BPlistObj_LoadPlateIconRes(wk);
    BPlistObj_CreateActors(wk);
    BPlistObj_InitCursor(wk);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void BPlistObj_ClearResources(BPlistWork *wk) {
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

// NONMATCHING: 4 bytes longer; the original hoists &wk->charRes out of the loop, we hoist &wk->pokemon and load 0x24a4
static void BPlistObj_LoadPokeIconRes(BPlistWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_TAIL(wk->param->heapId));
    u16 i;

    for (i = 0; i < 6; i++) {
        BPlistPokemon *pkm = &wk->pokemon[i];
        u32 file;
        if (pkm->species != 0) {
            file = PokeIcon_GetFileOfPkm(func_0201d624(pkm->pkm));
        } else {
            file = PokeParty_GetIconIndex(0, 0, 0, FALSE);
        }
        wk->charRes[i] = func_0204b81c(arc, file, FALSE, CLACT_VRAM_SUB, wk->param->heapId);
    }
    wk->plttRes[0] = func_0204bc48(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_SUB, 0, wk->param->heapId);
    wk->cellRes[0] = func_0204bde0(arc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_SubEng(), wk->param->heapId);
    GFL_ArcToolFree(arc);
}

static void BPlistObj_LoadStatusIconRes(BPlistWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_TAIL(wk->param->heapId));

    wk->charRes[6] = func_0204b81c(arc, 12, FALSE, CLACT_VRAM_SUB, wk->param->heapId);
    wk->plttRes[1] = func_0204bba0(arc, 11, CLACT_VRAM_SUB, 0x60, wk->param->heapId);
    wk->cellRes[1] = func_0204bde0(arc, 13, 16, wk->param->heapId);
    GFL_ArcToolFree(arc);
}

static void BPlistObj_LoadTypeIconRes(BPlistWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_TAIL(wk->param->heapId));
    u32 i;

    wk->plttRes[3] = func_0204bba0(arc, 33, CLACT_VRAM_SUB, 0xa0, wk->param->heapId);
    wk->cellRes[3] = func_0204bde0(arc, 59, 62, wk->param->heapId);
    for (i = 8; i <= 21; i++) {
        wk->charRes[i] = func_0204b81c(arc, func_0202d7f4(0), FALSE, CLACT_VRAM_SUB, wk->param->heapId);
    }
    wk->charRes[22] = func_0204b81c(arc, func_0202d80c(0), FALSE, CLACT_VRAM_SUB, wk->param->heapId);
    wk->charRes[23] = func_0204b81c(arc, func_0202d80c(0), FALSE, CLACT_VRAM_SUB, wk->param->heapId);
    GFL_ArcToolFree(arc);
}

static void BPlistObj_LoadItemIconRes(BPlistWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_TAIL(wk->param->heapId));

    wk->charRes[7] = func_0204b81c(arc, 65, FALSE, CLACT_VRAM_SUB, wk->param->heapId);
    wk->plttRes[2] = func_0204bba0(arc, 66, CLACT_VRAM_SUB, 0x80, wk->param->heapId);
    wk->cellRes[2] = func_0204bde0(arc, 67, 70, wk->param->heapId);
    GFL_ArcToolFree(arc);
}

static void BPlistObj_LoadCursorRes(BPlistWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(11, HEAPID_TAIL(wk->param->heapId));

    wk->charRes[24] = func_0204b81c(arc, 424, FALSE, CLACT_VRAM_SUB, wk->param->heapId);
    wk->plttRes[4] = func_0204bbb8(arc, 421, CLACT_VRAM_SUB, 0x100, 4, 1, wk->param->heapId);
    wk->cellRes[4] = func_0204bde0(arc, 425, 426, wk->param->heapId);
    GFL_ArcToolFree(arc);
}

static void BPlistObj_LoadPlateIconRes(BPlistWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_TAIL(wk->param->heapId));

    wk->charRes[25] = func_0204b81c(arc, func_0202d958(0), FALSE, CLACT_VRAM_SUB, wk->param->heapId);
    wk->plttRes[5] = func_0204bba0(arc, func_0202d954(), CLACT_VRAM_SUB, 0x120, wk->param->heapId);
    wk->cellRes[5] = func_0204bde0(arc, func_0202d95c(0), func_0202d960(0), wk->param->heapId);
    GFL_ArcToolFree(arc);
}

static ClActor *BPlistObj_CreateActor(BPlistWork *wk, const BPlistActorSetup *data) {
    ClActorSetup setup;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = data->priority;
    setup.bgPriority = data->bgPriority;
    return func_0204c040(wk->actorUnit, wk->charRes[data->chars], wk->plttRes[data->palette],
                         wk->cellRes[data->cellAnims], &setup, CLACT_SURFACE_SUB, wk->param->heapId);
}

static void BPlistObj_CreateActors(BPlistWork *wk) {
    u32 i;

    wk->actorUnit = func_0204bf1c(45, 0, HEAPID_TAIL(wk->param->heapId));
    for (i = 0; i < NELEMS(wk->actors); i++) {
        wk->actors[i] = BPlistObj_CreateActor(wk, &data_ov287_021fb134[i]);
    }
    func_02042ba8(FALSE, wk->param->heapId);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_VRAM_SUB_OBJ, 0, 0x1e0);
    BPlistObj_SetPokeIconPalettes(wk);
}

void BPlistObj_Exit(BPlistWork *wk) {
    u32 i;

    for (i = 0; i < NELEMS(wk->actors); i++) {
        func_0204c108(wk->actors[i]);
    }
    BAppCursor_DeleteActors(wk->cursor);
    for (i = 0; i < NELEMS(wk->charRes); i++) {
        func_0204b98c(wk->charRes[i]);
    }
    for (i = 0; i < NELEMS(wk->plttRes); i++) {
        func_0204bcd0(wk->plttRes[i]);
    }
    for (i = 0; i < NELEMS(wk->cellRes); i++) {
        func_0204be64(wk->cellRes[i]);
    }
    func_0204bf98(wk->actorUnit);
}

static void BPlistObj_ShowAt(ClActor *actor, const ClActorPos *pos) {
    func_0204c124(actor, TRUE);
    func_0204c140(actor, pos, CLACT_SURFACE_SUB);
}

static void BPlistObj_SetPokeIconPalettes(BPlistWork *wk) {
    s16 i;

    for (i = 0; i < 6; i++) {
        if (wk->pokemon[i].species != 0) {
            func_0204c378(
                wk->pokeIcons[i],
                PokeIcon_GetPalette(wk->pokemon[i].species, wk->pokemon[i].form, wk->pokemon[i].sex, wk->pokemon[i].isEgg),
                1);
        }
    }
}

static void BPlistObj_LoadTypeIcon(BPlistWork *wk, ClActor *actor, u32 chars, u32 type) {
    NNSG2dCharacterData *charData;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_TAIL(wk->param->heapId));
    void *file = GFL_G2DIOReadOBJNCGRArc(arc, func_0202d7f4(type), FALSE, &charData, HEAPID_TAIL(wk->param->heapId));

    func_0204ba40(wk->charRes[chars], charData);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
    func_0204c378(actor, func_0202d7e8(type), 1);
}

static void BPlistObj_LoadCategoryIcon(BPlistWork *wk, u32 actor, u32 chars, u32 category) {
    NNSG2dCharacterData *charData;
    ArcTool *arc;
    void *file;

    actor += wk->categoryIconBuf;
    chars += wk->categoryIconBuf;
    arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_TAIL(wk->param->heapId));
    file = GFL_G2DIOReadOBJNCGRArc(arc, func_0202d80c(category), FALSE, &charData, HEAPID_TAIL(wk->param->heapId));
    func_0204ba40(wk->charRes[chars], charData);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
    func_0204c378(wk->actors[actor], func_0202d800(category), 1);
    wk->categoryIconBuf ^= 1;
}

static void BPlistObj_ShowStatusIcon(u8 status, ClActor *actor, const ClActorPos *pos) {
    if (status == APP_STATUS_ICON_NONE) {
        func_0204c140(actor, pos, CLACT_SURFACE_SUB);
        return;
    }
    func_0204c488(actor, status);
    BPlistObj_ShowAt(actor, pos);
}

static void BPlistObj_ShowPokeTypes(BPlistWork *wk, BPlistPokemon *pkm, const ClActorPos *pos) {
    u32 actor;
    u32 chars;

    if (wk->pokeTypeIconBuf == 0) {
        actor = BPLIST_ACTOR_POKE_TYPE;
        chars = 8;
    } else {
        actor = BPLIST_ACTOR_POKE_TYPE + 2;
        chars = 10;
    }
    BPlistObj_LoadTypeIcon(wk, wk->actors[actor], chars, pkm->type1);
    BPlistObj_ShowAt(wk->actors[actor], &pos[0]);
    if (pkm->type1 != pkm->type2) {
        BPlistObj_LoadTypeIcon(wk, wk->actors[actor + 1], chars + 1, pkm->type2);
        BPlistObj_ShowAt(wk->actors[actor + 1], &pos[1]);
    }
    wk->pokeTypeIconBuf ^= 1;
}

static void BPlistObj_ShowItemIcon(u16 item, ClActor *actor, const ClActorPos *pos) {
    if (item == 0) {
        func_0204c140(actor, pos, CLACT_SURFACE_SUB);
        return;
    }
    if (PML_ItemIsMail(item) == TRUE) {
        func_0204c488(actor, 1);
    } else {
        func_0204c488(actor, 0);
    }
    BPlistObj_ShowAt(actor, pos);
}

void BPlistObj_SetPage(BPlistWork *wk, u8 page) {
    u32 i;

    for (i = 0; i < NELEMS(wk->actors); i++) {
        func_0204c124(wk->actors[i], FALSE);
    }
    switch (page) {
    case 0:
    case 8:
        BPlistObj_ShowList(wk);
        break;
    case 1:
        BPlistObj_ShowPage1(wk);
        break;
    case 2:
        BPlistObj_ShowPage2(wk);
        break;
    case 3:
        BPlistObj_ShowPage3(wk);
        break;
    case 4:
        BPlistObj_ShowPage4(wk);
        break;
    case 5:
        BPlistObj_ShowPage5(wk);
        break;
    case 6:
        BPlistObj_ShowPage6(wk);
        break;
    case 7:
        BPlistObj_ShowPage7(wk);
        break;
    }
}

static void BPlistObj_ShowList(BPlistWork *wk) {
    s16 i;

    for (i = 0; i < 6; i++) {
        u8 slot = BPlistMain_GetPartySlot(wk, i);
        if (wk->pokemon[slot].species != 0) {
            ClActorPos pos;
            BPlistObj_ShowAt(wk->actors[BPLIST_ACTOR_POKE + slot], &data_ov287_021fb0ec[i]);
            BPlistObj_ShowStatusIcon(wk->pokemon[slot].status, wk->actors[BPLIST_ACTOR_STATUS + slot],
                                &data_ov287_021fb104[i]);
            if (!wk->pokemon[slot].isEgg) {
                BPlistObj_ShowAt(wk->actors[BPLIST_ACTOR_UNK1F08 + slot], &data_ov287_021fb11c[i]);
            }
            pos = data_ov287_021fb0ec[i];
            pos.x += 8;
            pos.y += 8;
            BPlistObj_ShowItemIcon(wk->pokemon[slot].item, wk->actors[BPLIST_ACTOR_ITEM + slot], &pos);
        }
    }
}

static void BPlistObj_ShowPage1(BPlistWork *wk) {
    u8 slot = BPlistMain_GetPartySlot(wk, wk->param->partyIndex);
    ClActorPos pos = data_ov287_021fb06c;

    BPlistObj_ShowAt(wk->pokeIcons[slot], &pos);
    pos.x += 8;
    pos.y += 8;
    BPlistObj_ShowItemIcon(wk->pokemon[slot].item, wk->itemIcons[slot], &pos);
}

static void BPlistObj_ShowPage2(BPlistWork *wk) {
    u8 slot = BPlistMain_GetPartySlot(wk, wk->param->partyIndex);
    BPlistPokemon *pkm = &wk->pokemon[slot];
    ClActorPos pos = data_ov287_021fb07c;

    BPlistObj_ShowAt(wk->pokeIcons[slot], &pos);
    BPlistObj_ShowStatusIcon(pkm->status, wk->statusIcons[slot], &data_ov287_021fb0a0);
    BPlistObj_ShowPokeTypes(wk, pkm, data_ov287_021fb0d0);
    pos.x += 8;
    pos.y += 8;
    BPlistObj_ShowItemIcon(pkm->item, wk->itemIcons[slot], &pos);
    BPlistObj_ShowItemIcon(pkm->item, wk->itemIcons[6], &data_ov287_021fb070);
    BPlistObj_ShowAt(wk->unk1f08[0], &data_ov287_021fb0ac);
}

static void BPlistObj_ShowPage3(BPlistWork *wk) {
    u8 slot = BPlistMain_GetPartySlot(wk, wk->param->partyIndex);
    BPlistPokemon *pkm = &wk->pokemon[slot];
    ClActorPos pos = data_ov287_021fb0a8;

    BPlistObj_ShowAt(wk->pokeIcons[slot], &pos);
    BPlistObj_ShowStatusIcon(pkm->status, wk->statusIcons[slot], &data_ov287_021fb0a4);
    BPlistObj_ShowPokeTypes(wk, pkm, data_ov287_021fb0b0);
    pos.x += 8;
    pos.y += 8;
    BPlistObj_ShowItemIcon(pkm->item, wk->itemIcons[slot], &pos);
    BPlistObj_ShowMoveTypes(wk);
}

static void BPlistObj_ShowPage4(BPlistWork *wk) {
    u8 slot = BPlistMain_GetPartySlot(wk, wk->param->partyIndex);
    BPlistPokemon *pkm = &wk->pokemon[slot];
    ClActorPos pos = data_ov287_021fb09c;

    BPlistObj_ShowAt(wk->pokeIcons[slot], &pos);
    BPlistObj_ShowStatusIcon(pkm->status, wk->statusIcons[slot], &data_ov287_021fb098);
    BPlistObj_ShowPokeTypes(wk, pkm, data_ov287_021fb0c0);
    pos.x += 8;
    pos.y += 8;
    BPlistObj_ShowItemIcon(pkm->item, wk->itemIcons[slot], &pos);
    if (wk->typeIconBuf == 0) {
        BPlistObj_ShowAt(wk->actors[BPLIST_ACTOR_MOVE_TYPE + 5 + wk->param->slot], &data_ov287_021fb090);
    } else {
        BPlistObj_ShowAt(wk->actors[BPLIST_ACTOR_MOVE_TYPE + wk->param->slot], &data_ov287_021fb090);
    }
    if (wk->categoryIconBuf == 0) {
        BPlistObj_ShowAt(wk->categoryIcons[0], &data_ov287_021fb080);
    } else {
        BPlistObj_ShowAt(wk->categoryIcons[1], &data_ov287_021fb080);
    }
    BPlistObj_LoadCategoryIcon(wk, BPLIST_ACTOR_CATEGORY, 22, pkm->moves[wk->param->slot].category);
}

static void BPlistObj_ShowPage6(BPlistWork *wk) {
    u8 slot = BPlistMain_GetPartySlot(wk, wk->param->partyIndex);
    BPlistPokemon *pkm = &wk->pokemon[slot];
    ClActorPos pos = data_ov287_021fb088;

    BPlistObj_ShowAt(wk->pokeIcons[slot], &pos);
    BPlistObj_ShowStatusIcon(pkm->status, wk->statusIcons[slot], &data_ov287_021fb084);
    BPlistObj_ShowPokeTypes(wk, pkm, data_ov287_021fb0b8);
    pos.x += 8;
    pos.y += 8;
    BPlistObj_ShowItemIcon(pkm->item, wk->itemIcons[slot], &pos);
    BPlistObj_ShowMoveTypes(wk);
}

static void BPlistObj_ShowPage7(BPlistWork *wk) {
    u8 slot = BPlistMain_GetPartySlot(wk, wk->param->partyIndex);
    BPlistPokemon *pkm = &wk->pokemon[slot];
    ClActorPos pos = data_ov287_021fb08c;

    BPlistObj_ShowAt(wk->pokeIcons[slot], &pos);
    BPlistObj_ShowStatusIcon(pkm->status, wk->statusIcons[slot], &data_ov287_021fb074);
    BPlistObj_ShowPokeTypes(wk, pkm, data_ov287_021fb0c8);
    pos.x += 8;
    pos.y += 8;
    BPlistObj_ShowItemIcon(pkm->item, wk->itemIcons[slot], &pos);
    if (wk->typeIconBuf == 0) {
        BPlistObj_ShowAt(wk->actors[BPLIST_ACTOR_MOVE_TYPE + 5 + wk->param->slot], &data_ov287_021fb078);
    } else {
        BPlistObj_ShowAt(wk->actors[BPLIST_ACTOR_MOVE_TYPE + wk->param->slot], &data_ov287_021fb078);
    }
    if (wk->categoryIconBuf == 0) {
        BPlistObj_ShowAt(wk->categoryIcons[0], &data_ov287_021fb094);
    } else {
        BPlistObj_ShowAt(wk->categoryIcons[1], &data_ov287_021fb094);
    }
    if (wk->param->slot < 4) {
        BPlistObj_LoadCategoryIcon(wk, BPLIST_ACTOR_CATEGORY, 22, pkm->moves[wk->param->slot].category);
    } else {
        BPlistObj_LoadCategoryIcon(wk, BPLIST_ACTOR_CATEGORY, 22, PML_MoveGetParam(wk->param->move, MOVE_PARAM_CATEGORY));
    }
}

static void BPlistObj_ShowPage5(BPlistWork *wk) {
    u8 slot = BPlistMain_GetPartySlot(wk, wk->param->partyIndex);
    BPlistPokemon *pkm = &wk->pokemon[slot];
    ClActorPos pos = data_ov287_021fb088;

    BPlistObj_ShowAt(wk->pokeIcons[slot], &pos);
    BPlistObj_ShowStatusIcon(pkm->status, wk->statusIcons[slot], &data_ov287_021fb084);
    BPlistObj_ShowPokeTypes(wk, pkm, data_ov287_021fb0b8);
    pos.x += 8;
    pos.y += 8;
    BPlistObj_ShowItemIcon(pkm->item, wk->itemIcons[slot], &pos);
    BPlistObj_ShowMoveTypes(wk);
}

void BPlistObj_ShowMoveTypes(BPlistWork *wk) {
    BPlistPokemon *pkm;
    u8 chars;
    u8 actor;
    u32 i;

    pkm = &wk->pokemon[BPlistMain_GetPartySlot(wk, wk->param->partyIndex)];
    if (wk->typeIconBuf == 0) {
        actor = BPLIST_ACTOR_MOVE_TYPE;
        chars = 12;
    } else {
        actor = BPLIST_ACTOR_MOVE_TYPE + 5;
        chars = 17;
    }
    for (i = 0; i < 4; i++) {
        if (pkm->moves[i].move != 0) {
            BPlistObj_LoadTypeIcon(wk, wk->actors[actor + i], chars + i, pkm->moves[i].type);
            BPlistObj_ShowAt(wk->actors[actor + i], &data_ov287_021fb0d8[i]);
        }
    }
    if (wk->param->move != 0 && wk->param->unk1F == 4) {
        BPlistObj_LoadTypeIcon(wk, wk->actors[actor + i], chars + i, PML_MoveGetParam(wk->param->move, MOVE_PARAM_TYPE));
        BPlistObj_ShowAt(wk->actors[actor + i], &data_ov287_021fb0d8[i]);
    }
    wk->typeIconBuf ^= 1;
}

static void BPlistObj_ChangeAnim(ClActor *actor, u16 sequence) {
    if (sequence != func_0204c4a0(actor)) {
        func_0204c4d4(actor, 0);
        func_0204c488(actor, sequence);
    }
}

static u16 BPlistObj_GetPokeIconAnim(BPlistPokemon *pkm) {
    if (pkm->hp == 0) {
        return 0;
    }
    if (pkm->status != APP_STATUS_ICON_NONE && pkm->status != APP_STATUS_ICON_FAINTED) {
        return 5;
    }
    if (pkm->hp == pkm->maxHp) {
        return 1;
    }
    switch (HPGauge_GetColor(pkm->hp, pkm->maxHp)) {
    case HP_GAUGE_COLOR_GREEN:
        return 2;
    case HP_GAUGE_COLOR_YELLOW:
        return 3;
    case HP_GAUGE_COLOR_RED:
        return 4;
    }
    return 0;
}

void BPlistObj_UpdatePokeIconAnims(BPlistWork *wk) {
    u16 i;

    for (i = 0; i < 6; i++) {
        u8 slot = BPlistMain_GetPartySlot(wk, i);
        if (wk->pokemon[slot].species != 0) {
            u16 seq = BPlistObj_GetPokeIconAnim(&wk->pokemon[slot]);
            BPlistObj_ChangeAnim(wk->pokeIcons[slot], seq);
            func_0204c4e0(wk->pokeIcons[slot], FX32_ONE);
        }
    }
}

static void BPlistObj_InitCursor(BPlistWork *wk) {
    BAppCursor_CreateActors(wk->cursor, wk->actorUnit, wk->charRes[24], wk->plttRes[4], wk->cellRes[4]);
    BAppCursor_SetVisible(wk->cursor, FALSE);
}

void BPlistObj_MovePlate(BPlistWork *wk, int pos, s16 dx) {
    u8 slot = BPlistMain_GetPartySlot(wk, pos);
    ClActorPos p;

    func_0204c178(wk->itemIcons[slot], &p, CLACT_SURFACE_SUB);
    p.x += dx;
    func_0204c140(wk->itemIcons[slot], &p, CLACT_SURFACE_SUB);
    func_0204c178(wk->pokeIcons[slot], &p, CLACT_SURFACE_SUB);
    p.x += dx;
    func_0204c140(wk->pokeIcons[slot], &p, CLACT_SURFACE_SUB);
    func_0204c178(wk->statusIcons[slot], &p, CLACT_SURFACE_SUB);
    p.x += dx;
    func_0204c140(wk->statusIcons[slot], &p, CLACT_SURFACE_SUB);
    func_0204c178(wk->unk1f08[slot], &p, CLACT_SURFACE_SUB);
    p.x += dx;
    func_0204c140(wk->unk1f08[slot], &p, CLACT_SURFACE_SUB);
}

void BPlistObj_SwapPlates(BPlistWork *wk, int posA, int posB) {
    u8 slotA = BPlistMain_GetPartySlot(wk, posA);
    u8 slotB = BPlistMain_GetPartySlot(wk, posB);
    ClActorPos a;
    ClActorPos b;

    func_0204c178(wk->itemIcons[slotA], &a, CLACT_SURFACE_SUB);
    func_0204c178(wk->itemIcons[slotB], &b, CLACT_SURFACE_SUB);
    func_0204c140(wk->itemIcons[slotA], &b, CLACT_SURFACE_SUB);
    func_0204c140(wk->itemIcons[slotB], &a, CLACT_SURFACE_SUB);
    func_0204c178(wk->pokeIcons[slotA], &a, CLACT_SURFACE_SUB);
    func_0204c178(wk->pokeIcons[slotB], &b, CLACT_SURFACE_SUB);
    func_0204c140(wk->pokeIcons[slotA], &b, CLACT_SURFACE_SUB);
    func_0204c140(wk->pokeIcons[slotB], &a, CLACT_SURFACE_SUB);
    func_0204c178(wk->statusIcons[slotA], &a, CLACT_SURFACE_SUB);
    func_0204c178(wk->statusIcons[slotB], &b, CLACT_SURFACE_SUB);
    func_0204c140(wk->statusIcons[slotA], &b, CLACT_SURFACE_SUB);
    func_0204c140(wk->statusIcons[slotB], &a, CLACT_SURFACE_SUB);
    func_0204c178(wk->unk1f08[slotA], &a, CLACT_SURFACE_SUB);
    func_0204c178(wk->unk1f08[slotB], &b, CLACT_SURFACE_SUB);
    func_0204c140(wk->unk1f08[slotA], &b, CLACT_SURFACE_SUB);
    func_0204c140(wk->unk1f08[slotB], &a, CLACT_SURFACE_SUB);
}
