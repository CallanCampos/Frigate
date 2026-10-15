#include "types.h"
#include "app/box2_obj.h"
#include "app/box2_main.h"
#include "app/box2_ui.h"
#include "app/box2_bmp.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "pml/item.h"
#include "pml/personal.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "system/app_menu_common.h"
#include "system/bgwinfrm.h"
#include "system/bmp_oam.h"
#include "system/cursor_move.h"

// The PC box's actors: the cursor, the Pokémon and item icons, the tray icons of the box list and the buttons. The ROM
// doesn't name this file; box2_obj.c is a guess after box2_main.c. Names are ours

// No resource loaded
#define BOX2_RES_NONE 0xffffffff

// An actor to create: where, its resources and its surface
typedef struct {
    ClActorSetup setup;
    u32 chr;
    u32 pal;
    u32 cell;
    // A palette set once the actor is created, by the code that uses it
    u16 palette;
    u16 surface;
} Box2ActorData;

// An area of the screen, with its right and bottom edges in it
typedef struct {
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
} Box2Rect;

static void func_ov255_021cf430(Box2AppWork *app);
static void func_ov255_021cf460(Box2AppWork *app);
static void func_ov255_021cf488(Box2AppWork *app);
static void func_ov255_021cf4e8(Box2SysWork *syswk);
static ClActor *func_ov255_021cf51c(Box2AppWork *app, const Box2ActorData *data);
static void func_ov255_021cf570(Box2AppWork *app, u32 id);
static void func_ov255_021cf58c(Box2AppWork *app);
static void func_ov255_021cf66c(Box2AppWork *app, u32 id, u32 priority);
static void func_ov255_021cf684(Box2AppWork *app, u32 id, u32 priority);
static void func_ov255_021cf69c(Box2AppWork *app, u32 id, BOOL blend);
static void func_ov255_021cf718(Box2AppWork *app, u32 id, u32 palette);
static void func_ov255_021cf734(Box2AppWork *app);
static void func_ov255_021cf84c(Box2AppWork *app);
static void func_ov255_021cf8b8(Box2AppWork *app);
static void func_ov255_021cf904(Box2AppWork *app);
static void *func_ov255_021cf9a4(Box2AppWork *app, BoxPkm *pkm, NNSG2dCharacterData **chr);
static BOOL func_ov255_021cfa9c(Box2SearchParam *search, BoxPkm *pkm);
static u32 func_ov255_021cfba0(Box2SysWork *syswk, BoxPkm *pkm);
static void func_ov255_021cfbbc(Box2AppWork *app, void *chr, u32 id, u32 palette);
static void func_ov255_021cfbf0(Box2SysWork *syswk, BoxPkm *pkm, u32 id);
static void func_ov255_021d025c(Box2SysWork *syswk, BOOL blend, u32 start, u32 end, BOOL check);
static void func_ov255_021d0364(Box2SysWork *syswk, u32 pos);
static void func_ov255_021d0678(Box2AppWork *app);
static void func_ov255_021d07d0(Box2AppWork *app);
static void func_ov255_021d0978(Box2AppWork *app);
static void func_ov255_021d09dc(Box2AppWork *app, u32 type1, u32 type2);
static void func_ov255_021d0a58(Box2AppWork *app);
static void func_ov255_021d0d70(Box2SysWork *syswk);
static void func_ov255_021d12e4(Box2AppWork *app);
static void func_ov255_021d1488(Box2SysWork *syswk);
static void func_ov255_021d1624(Box2SysWork *syswk, u32 tray, u32 id);
static void func_ov255_021d1654(Box2SysWork *syswk, u32 tray, u32 row);
static void func_ov255_021d16a8(Box2SysWork *syswk, u8 *chr, u32 wallpaper, u32 color, u32 size);
static void func_ov255_021d16cc(Box2SysWork *syswk, u32 tray, u8 *chr);
static void func_ov255_021d1a84(Box2SysWork *syswk, u32 index);
static void func_ov255_021d1c30(Box2AppWork *app);
static void func_ov255_021d1d44(Box2AppWork *app);
static void func_ov255_021d1e08(Box2SysWork *syswk);
static void func_ov255_021d1f40(s32 *a, s32 *b);
static void func_ov255_021d1f50(Box2SysWork *syswk, s32 left, s32 top, s32 right, s32 bottom);
static s32 func_ov255_021d1f88(s32 a, s32 b, s32 c, s32 d);
static s32 func_ov255_021d1fb8(s32 a, s32 b, s32 c, s32 d);
static void func_ov255_021d1fe8(int pos, Box2Rect *rect);
static void func_ov255_021d201c(int pos, Box2Rect *rect);
static void func_ov255_021d205c(int pos, Box2Rect *rect);
static void func_ov255_021d2118(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2);
static void func_ov255_021d2160(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2);
static BOOL func_ov255_021d229c(Box2SysWork *syswk, u32 pos);


// Where the copies of the held item's icon that outline it are, around the icon
static const s8 sItemOutlineX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
static const s8 sItemOutlineY[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };


// Where the box name's frame is, by its position
static const s16 sBoxNameY[4] = { 52, 86, 120, 154 };

// The animations of the three buttons, by set
static const u8 sButtonAnims[3][3] = {
    { 19, 20, 22 },
    { 18, 21, 22 },
    { 18, 20, 23 },
};

// Where the party's icons are on the party's frame
static const u8 sPartyFramePokePos[6][2] = {
    { 24, 16 }, { 64, 24 }, { 24, 48 }, { 64, 56 }, { 24, 80 }, { 64, 88 },
};

// The type icons on the upper screen
static const Box2ActorData sTypeIconData = {
    { 160, 48, 0, 0, 1 }, 99, 4, 4, 0, 1,
};

// The first of the tray's Pokémon icons
static const Box2ActorData sPokeIconData = {
    { 24, 48, 0, 10, 3 }, 0, 0, 0, 0, 0,
};

// Where the party's icons are on the lower screen
static const ClActorPos sPartyPokePos[6] = {
    { 40, 208 }, { 80, 216 }, { 40, 240 }, { 80, 248 }, { 40, 272 }, { 80, 280 },
};

// The dots that run around the picked range
static const Box2ActorData sRangeDotData = {
    { 220, 120, 25, 19, 0 }, 122, 6, 6, 0, 0,
};

// The tray icons of the box list
static const Box2ActorData sTrayIconData = {
    { 300, 0, 0, 80, 1 }, 116, 5, 5, 0, 0,
};

static const ClActSysSetup sClActSysSetup = {
    0, 0, 0, 512, 4, 124, 4, 124, 0, 217, 11, 101, 0, 16, 16,
};

// The color of the tray icons' background, by wallpaper
static const u8 sTrayIconColors[24] = {
    0x1b, 0x17, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x10, 0x28, 0x18, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
};

// The actors that are created from one setup each, by their ID
static const Box2ActorData sActorData[32] = {
    { { 12, 28, 1, 0, 3 }, 122, 6, 6, 0, 0 },
    { { 156, 28, 3, 0, 3 }, 122, 6, 6, 0, 0 },
    { { 0, -21, 0, 6, 1 }, 122, 6, 6, 0, 0 },
    { { 128, -40, 5, 40, 0 }, 122, 6, 6, 0, 0 },
    { { 128, 128, 6, 0, 0 }, 122, 6, 6, 0, 0 },
    { { 128, 160, 9, 0, 0 }, 122, 6, 6, 0, 0 },
    { { 232, 168, 1, 64, 0 }, 124, 7, 8, 0, 0 },
    { { 200, 168, 0, 64, 0 }, 124, 7, 8, 0, 0 },
    { { 112, 168, 0, 64, 0 }, 123, 7, 7, 3, 0 },
    { { 32, 7, 19, 100, 0 }, 122, 6, 6, 0, 0 },
    { { 84, 7, 20, 100, 0 }, 122, 6, 6, 0, 0 },
    { { 136, 7, 22, 100, 0 }, 122, 6, 6, 0, 0 },
    { { 0, 0, 0, 0, 0 }, 98, 3, 3, 0, 0 },
    { { 200, 56, 0, 10, 1 }, 96, 1, 1, 0, 1 },
    { { 200, 56, 0, 10, 1 }, 97, 2, 2, 0, 1 },
    { { 192, 200, 0, 10, 1 }, 125, 8, 9, 0, 0 },
    { { 224, 200, 0, 10, 1 }, 125, 8, 9, 0, 0 },
    { { 192, 224, 0, 10, 1 }, 125, 8, 9, 0, 0 },
    { { 224, 224, 0, 10, 1 }, 125, 8, 9, 0, 0 },
    { { 192, 248, 0, 10, 1 }, 125, 8, 9, 0, 0 },
    { { 224, 248, 0, 10, 1 }, 125, 8, 9, 0, 0 },
    { { 200, 103, 0, 10, 1 }, 126, 9, 10, 0, 1 },
    { { 208, 103, 0, 10, 1 }, 126, 9, 10, 0, 1 },
    { { 216, 103, 0, 10, 1 }, 126, 9, 10, 0, 1 },
    { { 224, 103, 0, 10, 1 }, 126, 9, 10, 0, 1 },
    { { 232, 103, 0, 10, 1 }, 126, 9, 10, 0, 1 },
    { { 240, 103, 0, 10, 1 }, 126, 9, 10, 0, 1 },
    { { 160, 103, 12, 10, 1 }, 126, 9, 10, 0, 1 },
    { { 168, 103, 13, 10, 1 }, 126, 9, 10, 0, 1 },
    { { 116, 104, 0, 10, 1 }, 127, 10, 11, 0, 1 },
    { { 168, 168, 0, 64, 0 }, 128, 7, 12, 0, 0 },
    { { 0, 0, 24, 50, 0 }, 122, 6, 6, 0, 0 },
};

void func_ov255_021cf3c0(Box2SysWork *syswk) {
    func_ov255_021cf430(syswk->app);
    func_ov255_021cf460(syswk->app);
    func_ov255_021cf4e8(syswk);
    func_ov255_021d1af8(syswk, syswk->unk1F, syswk->unk20, syswk->unk21, syswk->unk22);
    func_ov255_021d1c30(syswk->app);
    func_ov255_021d1e08(syswk);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void func_ov255_021cf414(Box2AppWork *app) {
    func_ov255_021d1d44(app);
    func_ov255_021cf58c(app);
    func_ov255_021cf488(app);
    func_0204b758();
}

static void func_ov255_021cf430(Box2AppWork *app) {
    ClActSysSetup setup = sClActSysSetup;
    ClActSys_Create(&setup, Box2Main_GetVramBanks(), HEAPID_BOX2_APP);
}

static void func_ov255_021cf460(Box2AppWork *app) {
    func_ov255_021cf734(app);
    func_ov255_021cf84c(app);
    func_ov255_021cf8b8(app);
    func_ov255_021d0a58(app);
    func_ov255_021d0678(app);
    func_ov255_021d07d0(app);
}

static void func_ov255_021cf488(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < NELEMS(app->chrRes); i++) {
        if (app->chrRes[i] != BOX2_RES_NONE) {
            func_0204b98c(app->chrRes[i]);
        }
    }
    for (i = 0; i < NELEMS(app->palRes); i++) {
        if (app->palRes[i] != BOX2_RES_NONE) {
            func_0204bcd0(app->palRes[i]);
        }
    }
    for (i = 0; i < NELEMS(app->cellRes); i++) {
        if (app->cellRes[i] != BOX2_RES_NONE) {
            func_0204be64(app->cellRes[i]);
        }
    }
}

static void func_ov255_021cf4e8(Box2SysWork *syswk) {
    syswk->app->clunit = func_0204bf1c(233, 0, HEAPID_BOX2_APP);
    func_ov255_021d12e4(syswk->app);
    func_ov255_021cf904(syswk->app);
    func_ov255_021d0d70(syswk);
    func_ov255_021d0978(syswk->app);
}

static ClActor *func_ov255_021cf51c(Box2AppWork *app, const Box2ActorData *data) {
    ClActor *actor = func_0204c040(app->clunit, app->chrRes[data->chr], app->palRes[data->pal],
                                   app->cellRes[data->cell], &data->setup, data->surface, HEAPID_BOX2_APP);
    func_0204c5c8(actor, FALSE);
    return actor;
}

static void func_ov255_021cf570(Box2AppWork *app, u32 id) {
    if (app->actors[id] != NULL) {
        func_0204c108(app->actors[id]);
        app->actors[id] = NULL;
    }
}

static void func_ov255_021cf58c(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < NELEMS(app->actors); i++) {
        func_ov255_021cf570(app, i);
    }
    func_0204bf98(app->clunit);
}

void func_ov255_021cf5b0(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < 49; i++) {
        if (app->actors[i] != NULL && func_0204c534(app->actors[i]) != TRUE) {
            func_0204c4e0(app->actors[i], FX32_ONE);
        }
    }
    func_0204b794();
}

void func_ov255_021cf5e4(Box2AppWork *app, u32 id, u32 anim) {
    func_0204c4d4(app->actors[id], 0);
    func_0204c488(app->actors[id], anim);
}

void func_ov255_021cf608(Box2AppWork *app, u32 id, u32 anim) {
    func_ov255_021cf5e4(app, id, anim);
    func_0204c520(app->actors[id], TRUE);
}

BOOL func_ov255_021cf628(Box2AppWork *app, u32 id) {
    return func_0204c560(app->actors[id]);
}

void func_ov255_021cf63c(Box2AppWork *app, u32 id, BOOL visible) {
    if (app->actors[id] != NULL) {
        func_0204c124(app->actors[id], visible);
    }
}

BOOL func_ov255_021cf658(Box2AppWork *app, u32 id) {
    return func_0204c138(app->actors[id]);
}

static void func_ov255_021cf66c(Box2AppWork *app, u32 id, u32 priority) {
    func_0204c468(app->actors[id], (u8)priority);
}

static void func_ov255_021cf684(Box2AppWork *app, u32 id, u32 priority) {
    func_0204c438(app->actors[id], priority);
}

static void func_ov255_021cf69c(Box2AppWork *app, u32 id, BOOL blend) {
    if (blend == TRUE) {
        func_0204c318(app->actors[id], GX_OAM_MODE_XLU);
    } else {
        func_0204c318(app->actors[id], GX_OAM_MODE_NORMAL);
    }
}

void func_ov255_021cf6c8(Box2AppWork *app, u32 id, s16 x, s16 y, u16 surface) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c140(app->actors[id], &pos, surface);
}

void func_ov255_021cf6ec(Box2AppWork *app, u32 id, s16 *x, s16 *y, u16 surface) {
    ClActorPos pos;

    func_0204c178(app->actors[id], &pos, surface);
    *x = pos.x;
    *y = pos.y;
}

static void func_ov255_021cf718(Box2AppWork *app, u32 id, u32 palette) {
    func_0204c378(app->actors[id], (u8)palette, 1);
}

static void func_ov255_021cf734(Box2AppWork *app) {
    ArcTool *arc;
    u32 i;
    u32 j;

    arc = GFL_ArcSysCreateFileHandle(ARCID_BOX2, HEAPID_BOX2_APP);
    for (i = 0; i < 6; i++) {
        app->chrRes[116 + i] = func_0204b81c(arc, 71, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    }
    app->palRes[5] = func_0204bba0(arc, 72, CLACT_VRAM_MAIN, 0, HEAPID_BOX2_APP);
    app->cellRes[5] = func_0204bde0(arc, 73, 74, HEAPID_BOX2_APP);
    for (j = 0; j < 96; j++) {
        app->chrRes[j] = func_0204b81c(arc, 75, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    }
    app->chrRes[122] = func_0204b81c(arc, 67, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->palRes[6] = func_0204bba0(arc, 70, CLACT_VRAM_MAIN, 0xe0, HEAPID_BOX2_APP);
    app->cellRes[6] = func_0204bde0(arc, 68, 69, HEAPID_BOX2_APP);
    app->chrRes[123] = func_0204b81c(arc, 64, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[7] = func_0204bde0(arc, 65, 66, HEAPID_BOX2_APP);
    app->chrRes[98] = func_0204b81c(arc, 76, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[3] = func_0204bde0(arc, 77, 78, HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void func_ov255_021cf84c(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_BOX2_APP);

    app->chrRes[124] = func_0204b81c(arc, func_0202d814(), FALSE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->palRes[7] = func_0204bba0(arc, func_0202d810(), CLACT_VRAM_MAIN, 0x120, HEAPID_BOX2_APP);
    app->cellRes[8] = func_0204bde0(arc, func_0202d818(2), func_0202d81c(2), HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void func_ov255_021cf8b8(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_BOX2_APP);

    app->palRes[0] = func_0204bc48(arc, PokeIcon_GetAltPaletteFile(), CLACT_VRAM_MAIN, 0x60, HEAPID_BOX2_APP);
    app->cellRes[0] = func_0204bde0(arc, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void func_ov255_021cf904(Box2AppWork *app) {
    Box2ActorData data;
    Box2ActorData base = sPokeIconData;
    u32 i;

    for (i = 0; i < NELEMS(app->pokeIconId); i++) {
        data = base;

        data.setup.x += (s16)(i % 6 * 24);
        data.setup.y += (s16)(i / 6 * 24);
        data.setup.priority = 146 - i * 2;
        data.chr = i;
        app->actors[BOX2_ACTOR_POKEICON + i] = func_ov255_021cf51c(app, &data);
        app->pokeIconId[i] = BOX2_ACTOR_POKEICON + i;
        func_ov255_021cf63c(app, BOX2_ACTOR_POKEICON + i, FALSE);
    }
}

static void *func_ov255_021cf9a4(Box2AppWork *app, BoxPkm *pkm, NNSG2dCharacterData **chr) {
    return GFL_G2DIOReadOBJNCGRArc(app->pokeIconArc, PokeIcon_GetFileOfPkm(pkm), FALSE, chr, HEAPID_BOX2_APP);
}

void func_ov255_021cf9c8(Box2SysWork *syswk, u32 tray) {
    BoxPkm *pkm;
    void *buf;
    NNSG2dCharacterData *chr;
    u32 i;

    for (i = 0; i < BOX2_TRAY_POKE_MAX; i++) {
        if (BoxSaveAccessor_GetPkmParam(syswk->param->boxes, tray, i, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            syswk->app->pokeIconExist[i] = FALSE;
        } else if (syswk->param->mode != 3 && syswk->tray == syswk->getTray &&
                   func_ov255_021d229c(syswk, i) == TRUE) {
            syswk->app->pokeIconExist[i] = FALSE;
        } else {
            pkm = Box2Main_GetBoxPkm(syswk, tray, i);
            buf = func_ov255_021cf9a4(syswk->app, pkm, &chr);
            sys_memcpy32(chr->rawData, syswk->app->pokeIconChar[i], 0x200);
            GFL_HeapFree(buf);
            syswk->app->pokeIconPal[i] = func_ov255_021cfba0(syswk, pkm);
            syswk->app->pokeIconExist[i] = TRUE;
        }
    }
}

static BOOL func_ov255_021cfa9c(Box2SearchParam *search, BoxPkm *pkm) {
    BOOL egg;
    u32 marks;
    u32 i;

    if (search->active == FALSE) {
        return TRUE;
    }
    egg = PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    if (search->species != 0) {
        if (search->species != PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL) || egg != FALSE) {
            return FALSE;
        }
    }
    if (search->item == 1) {
        if (PML_PkmGetParam(pkm, PKM_PARAM_ITEM, NULL) != 0 || egg != FALSE) {
            return FALSE;
        }
    } else if (search->item == 2) {
        if (PML_PkmGetParam(pkm, PKM_PARAM_ITEM, NULL) == 0 || egg != FALSE) {
            return FALSE;
        }
    }
    if (search->nature != 0) {
        if (search->nature - 1 != PML_PkmGetParam(pkm, 0x70, NULL) || egg != FALSE) {
            return FALSE;
        }
    }
    if (search->ability != 0) {
        if (search->ability != PML_PkmGetParam(pkm, PKM_PARAM_ABILITY, NULL) || egg != FALSE) {
            return FALSE;
        }
    }
    if (search->sex != 0) {
        if (search->sex - 1 != PML_PkmGetParam(pkm, PKM_PARAM_SEX, NULL) || egg != FALSE) {
            return FALSE;
        }
    }
    if (search->marks != 0) {
        marks = PML_PkmGetParam(pkm, PKM_PARAM_MARKINGS, NULL);
        for (i = 0; i < 6; i++) {
            if ((search->marks & (1 << i)) && !(marks & (1 << i))) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static u32 func_ov255_021cfba0(Box2SysWork *syswk, BoxPkm *pkm) {
    if (func_ov255_021cfa9c(&syswk->search, pkm)) {
        return PokeIcon_GetPaletteOfPkm(pkm);
    }
    return 3;
}

static void func_ov255_021cfbbc(Box2AppWork *app, void *chr, u32 id, u32 palette) {
    func_0204bab8(func_0204c428(app->actors[id]), chr, 0x200, 0, CLACT_VRAM_MAIN);
    func_0204c378(app->actors[id], (u8)palette, 1);
}

static void func_ov255_021cfbf0(Box2SysWork *syswk, BoxPkm *pkm, u32 id) {
    NNSG2dCharacterData *chr;
    void *buf = func_ov255_021cf9a4(syswk->app, pkm, &chr);

    func_ov255_021cfbbc(syswk->app, chr->rawData, id, func_ov255_021cfba0(syswk, pkm));
    GFL_HeapFree(buf);
}

void func_ov255_021cfc20(Box2SysWork *syswk, u32 tray, u32 pos, u32 id) {
    Box2AppWork *app = syswk->app;

    func_ov255_021cf63c(app, id, FALSE);
    if (Box2Main_GetPokeParam(syswk, pos, tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
        func_ov255_021cfbf0(syswk, Box2Main_GetBoxPkm(syswk, tray, pos), id);
        func_ov255_021cf63c(app, id, TRUE);
    }
}

void func_ov255_021cfc74(Box2SysWork *syswk) {
    u32 i;

    for (i = 0; i < BOX2_TRAY_POKE_MAX; i++) {
        func_ov255_021cfc20(syswk, syswk->tray, i, BOX2_ACTOR_POKEICON + i);
    }
}

void func_ov255_021cfc90(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < count; i++) {
        func_ov255_021cfbf0(syswk, func_0201d624(PokeParty_GetPkm(syswk->param->party, i)),
                    syswk->app->pokeIconId[BOX2_PARTY_POS + i]);
        func_ov255_021cf63c(syswk->app, syswk->app->pokeIconId[BOX2_PARTY_POS + i], FALSE);
    }
}

void func_ov255_021cfcdc(u32 pos, s16 *x, s16 *y, u32 mode) {
    if (pos < BOX2_TRAY_POKE_MAX) {
        *x = (pos % 6 + 1) * 24;
        *y = pos / 6 * 24 + 48;
    } else {
        pos -= BOX2_TRAY_POKE_MAX;
        *x = sPartyPokePos[pos].x;
        *y = sPartyPokePos[pos].y - 144;
        if (mode == 2) {
            *x += 152;
        }
    }
}

void func_ov255_021cfd34(Box2SysWork *syswk, BOOL hideGet) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < 6; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        func_ov255_021cf6c8(syswk->app, id, sPartyPokePos[i].x, sPartyPokePos[i].y, 0);
        func_ov255_021cff58(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count && (hideGet == FALSE || syswk->pos != BOX2_PARTY_POS + i)) {
            func_ov255_021cf63c(syswk->app, id, TRUE);
        } else {
            func_ov255_021cf63c(syswk->app, id, FALSE);
        }
    }
}

void func_ov255_021cfdb0(Box2SysWork *syswk) {
    u32 i;
    u32 pos;
    u32 x;

    if (syswk->pos >= BOX2_PARTY_POS) {
        pos = syswk->pos - BOX2_PARTY_POS;
        x = pos & 1;
        for (i = 0; i < 6; i++) {
            if
 ((i & 1) >= x && (i & 1) < x + syswk->app->rangeWidth && (i >> 1) >= (pos >> 1) &&
                (i >> 1) < (pos >> 1) + syswk->app->rangeHeight) {
                func_ov255_021cf63c(syswk->app, syswk->app->pokeIconId[BOX2_PARTY_POS + i], FALSE);
            }
        }
    }
}

void func_ov255_021cfe0c(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < 6; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        func_ov255_021cf6c8(syswk->app, id, sPartyPokePos[i].x + 152, sPartyPokePos[i].y, 0);
        func_ov255_021cff58(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count) {
            func_ov255_021cf63c(syswk->app, id, TRUE);
        } else {
            func_ov255_021cf63c(syswk->app, id, FALSE);
        }
    }
}

void func_ov255_021cfe78(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < 6; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        func_ov255_021cf6c8(syswk->app, id, sPartyPokePos[i].x, sPartyPokePos[i].y - 144, 0);
        func_ov255_021cff58(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count) {
            func_ov255_021cf63c(syswk->app, id, TRUE);
        } else {
            func_ov255_021cf63c(syswk->app, id, FALSE);
        }
    }
}

void func_ov255_021cfee4(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < 6; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        func_ov255_021cf6c8(syswk->app, id, sPartyPokePos[i].x + 152, sPartyPokePos[i].y - 144, 0);
        func_ov255_021cff58(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count) {
            func_ov255_021cf63c(syswk->app, id, TRUE);
        } else {
            func_ov255_021cf63c(syswk->app, id, FALSE);
        }
    }
}

void func_ov255_021cff58(Box2AppWork *app, u32 iconPos, BOOL put) {
    u32 id = app->pokeIconId[iconPos];

    if (put == FALSE) {
        func_ov255_021cf66c(app, id, 0);
        func_ov255_021cf684(app, id, 20);
    } else {
        if (iconPos < BOX2_TRAY_POKE_MAX || iconPos == BOX2_BOXLIST_POS) {
            func_ov255_021cf66c(app, id, 3);
        } else {
            func_ov255_021cf66c(app, id, 1);
        }
        func_ov255_021cf684(app, id, 146 - iconPos * 2);
    }
}

void func_ov255_021cffa8(Box2AppWork *app, u32 iconPos, u32 pos, BOOL put) {
    u32 id = app->pokeIconId[iconPos];

    if (put == FALSE) {
        func_ov255_021cf66c(app, id, 0);
        func_ov255_021cf684(app, id, 20);
    } else {
        if (pos < BOX2_TRAY_POKE_MAX) {
            func_ov255_021cf66c(app, id, 3);
        } else {
            func_ov255_021cf66c(app, id, 1);
        }
        func_ov255_021cf684(app, id, 146 - pos * 2);
    }
}

void func_ov255_021cfff4(Box2SysWork *syswk, s32 mv) {
    s16 start;
    int end;
    s16 x;
    s16 y;
    u16 i;
    u32 id;

    if (mv >= 0) {
        end = 176;
        start = -8;
    } else {
        end = -8;
        start = 176;
    }
    for (i = 0; i < BOX2_TRAY_POKE_MAX; i++) {
        id = syswk->app->pokeIconId[i];
        func_ov255_021cf6ec(syswk->app, id, &x, &y, 0);
        x = x + mv;
        if (x == end) {
            x = start;
            func_ov255_021cfbbc(syswk->app, syswk->app->pokeIconChar[i], id, syswk->app->pokeIconPal[i]);
            func_ov255_021cf63c(syswk->app, id, syswk->app->pokeIconExist[i]);
            if (syswk->app->unkA55A == 0 && (syswk->param->mode == 3 || syswk->param->mode == 5)) {
                func_ov255_021d0364(syswk, i);
            }
        }
        func_ov255_021cf6c8(syswk->app, id, x, y, 0);
    }
}

void func_ov255_021d00d4(Box2SysWork *syswk) {
    s8 x;
    s8 y;
    s16 px;
    s16 py;
    u16 i;

    BGWinFrame_GetPos(syswk->app->bgWinFrame, 8, &x, &y);
    py = y * 8;
    px = x * 8;
    for (i = 0; i < 6; i++) {
        func_ov255_021cf6c8(syswk->app, syswk->app->pokeIconId[BOX2_PARTY_POS + i], px + sPartyFramePokePos[i][0],
                            py + sPartyFramePokePos[i][1], 0);
    }
}

void func_ov255_021d013c(Box2PokeFreeWork *wk) {
    ClActorScale scale;
    ClActorPos center;

    wk->scale = 4096.0f;
    wk->scaleCnt = 0;
    func_0204c244(wk->cap, 1);
    scale.x = wk->scale;
    scale.y = wk->scale;
    func_0204c270(wk->cap, &scale);
    center.x = 0;
    center.y = 8;
    func_0204c258(wk->cap, &center);
}

BOOL func_ov255_021d0184(Box2PokeFreeWork *wk) {
    ClActorScale scale;

    wk->scaleCnt++;
    wk->scale -= 102.0f;
    if (wk->scaleCnt == 40) {
        return FALSE;
    }
    scale.x = wk->scale;
    scale.y = wk->scale;
    func_0204c270(wk->cap, &scale);
    return TRUE;
}

BOOL func_ov255_021d01c8(Box2PokeFreeWork *wk) {
    ClActorScale scale;

    wk->scaleCnt -= 2;
    wk->scale += 102.0f;
    wk->scale += 102.0f;
    if (wk->scaleCnt == 0) {
        return FALSE;
    }
    scale.x = wk->scale;
    scale.y = wk->scale;
    func_0204c270(wk->cap, &scale);
    return TRUE;
}

void func_ov255_021d0214(Box2PokeFreeWork *wk) {
    func_0204c124(wk->cap, FALSE);
    func_ov255_021d0228(wk);
}

void func_ov255_021d0228(Box2PokeFreeWork *wk) {
    ClActorScale scale;
    ClActorPos center;

    center.x = 0;
    center.y = 0;
    func_0204c258(wk->cap, &center);
    scale.x = FX32_ONE;
    scale.y = FX32_ONE;
    func_0204c270(wk->cap, &scale);
    func_0204c244(wk->cap, 0);
}

static void func_ov255_021d025c(Box2SysWork *syswk, BOOL blend, u32 start, u32 end, BOOL check) {
    u32 i;

    if (check == TRUE) {
        if (syswk->param->mode == BOX2_MODE_DREAM_WORLD) {
            for (i = start; i < end; i++) {
                if (Box2Main_IsSpeciesFlagged(syswk, i) == TRUE &&
                    Box2Main_GetPokeParam(syswk, i, syswk->tray, PKM_PARAM_IS_EGG, NULL) == 0) {
                    func_ov255_021cf69c(syswk->app, syswk->app->pokeIconId[i], FALSE);
                } else {
                    func_ov255_021cf69c(syswk->app, syswk->app->pokeIconId[i], TRUE);
                }
            }
        } else {
            for (i = start; i < end; i++) {
                if (Box2Main_GetPokeParam(syswk, i, syswk->tray, PKM_PARAM_ITEM, NULL) == 0) {
                    func_ov255_021cf69c(syswk->app, syswk->app->pokeIconId[i], TRUE);
                } else {
                    func_ov255_021cf69c(syswk->app, syswk->app->pokeIconId[i], FALSE);
                }
            }
        }
    } else {
        for (i = start; i < end; i++) {
            func_ov255_021cf69c(syswk->app, syswk->app->pokeIconId[i], blend);
        }
    }
}

void func_ov255_021d0310(Box2SysWork *syswk, u32 flags, BOOL blend) {
    BOOL check;

    if (flags & 0x80) {
        check = TRUE;
    } else {
        check = FALSE;
    }
    if (flags & 1) {
        func_ov255_021d025c(syswk, blend, 0, BOX2_TRAY_POKE_MAX, check);
    }
    if (flags & 2) {
        func_ov255_021d025c(syswk, blend, BOX2_TRAY_POKE_MAX, NELEMS(syswk->app->pokeIconId), check);
    }
}

void func_ov255_021d0350(Box2AppWork *app, u32 iconPos, BOOL blend) {
    func_ov255_021cf69c(app, app->pokeIconId[iconPos], blend);
}

static void func_ov255_021d0364(Box2SysWork *syswk, u32 pos) {
    func_ov255_021d025c(syswk, TRUE, pos, pos + 1, TRUE);
}

void func_ov255_021d0374(Box2SysWork *syswk, u32 pos, int width, int height) {
    int i;
    int j;
    Box2AppWork *app;
    u32 y;
    u32 x;
    u32 rowWidth;
    s16 px;
    s16 py;
    u8 tmp;

    app = syswk->app;
    rowWidth = Box2Main_GetRowWidth(syswk, pos);
    x = pos % rowWidth;
    y = pos / rowWidth;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            func_ov255_021cf6ec(app, app->pokeIconId[x + (y + j) * rowWidth + i], &px, &py, 0);
            tmp = app->pokeIconId[BOX2_BOXLIST_POS + j * width + i];
            app->pokeIconId[BOX2_BOXLIST_POS + j * width + i] = app->pokeIconId[x + (y + j) * rowWidth + i];
            app->pokeIconId[x + (y + j) * rowWidth + i] = tmp;
            func_ov255_021cf6c8(app, app->pokeIconId[x + (y + j) * rowWidth + i], px, py, 0);
            func_ov255_021cff58(app, x + (y + j) * rowWidth + i, TRUE);
        }
    }
    app->rangeWidth = width;
    app->rangeHeight = height;
}

void func_ov255_021d045c(Box2AppWork *app, u32 pos, int width, int height) {
    s16 px;
    s16 py;
    u32 x;
    u32 y;
    int i;
    int j;
    u8 tmp;

    func_ov255_021cf6ec(app, app->pokeIconId[BOX2_BOXLIST_POS], &px, &py, 0);
    x = pos % 6;
    y = pos / 6;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            func_ov255_021cf6ec(app, app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i], &px, &py, 0);
            tmp = app->pokeIconId[x + (y + j) * 6 + i];
            app->pokeIconId[x + (y + j) * 6 + i] = app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i];
            app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i] = tmp;

            func_ov255_021cf6c8(app, app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i], px, py, 0);
        }
    }
}

void func_ov255_021d052c(Box2SysWork *syswk) {
    s16 x;
    s16 y;
    int j;
    int i;


    if (syswk->unk18 != 0) {
        func_ov255_021cf6ec(syswk->app, BOX2_ACTOR_CURSOR, &x, &y, 0);
        if (syswk->param->mode == 3) {
            y += 8;
            func_ov255_021cf6c8(syswk->app, BOX2_ACTOR_ITEM_ICON, x, y, 0);
        } else {
            y += 4;
            for (j = 0; j < syswk->app->rangeHeight; j++) {
                for (i = 0; i < syswk->app->rangeWidth; i++) {
                    func_ov255_021cf6c8(syswk->app,
                                        syswk->app->pokeIconId[BOX2_BOXLIST_POS + j * syswk->app->rangeWidth + i],
                                        x + i * 24, y + j * 24, 0);
                }
            }
            func_ov255_021d22fc(syswk->app, x, y + 9);
        }
        if (syswk->param->mode == 3) {
            func_ov255_021d0d10(syswk->app);
        } else {
            if (syswk->app->actors[121] == NULL) {
                func_ov255_021d1048(syswk);
            }
            if (syswk->moveMode != 2) {
                func_ov255_021d1284(syswk->app, BOX2_BOXLIST_POS);
            } else {
                func_ov255_021d11a4(syswk, 0);
            }
        }
    }
}

void func_ov255_021d0640(Box2SysWork *syswk, u32 tray, u32 pos) {
    u32 palette = func_ov255_021cfba0(syswk, Box2Main_GetBoxPkm(syswk, tray, pos));

    if (pos < BOX2_TRAY_POKE_MAX) {
        syswk->app->pokeIconPal[pos] = palette;
    }
    func_ov255_021cf718(syswk->app, syswk->app->pokeIconId[pos], palette);
}

static void func_ov255_021d0678(Box2AppWork *app) {
    app->chrRes[96] = BOX2_RES_NONE;
    app->palRes[1] = BOX2_RES_NONE;
    app->cellRes[1] = BOX2_RES_NONE;
    app->chrRes[97] = BOX2_RES_NONE;
    app->palRes[2] = BOX2_RES_NONE;
    app->cellRes[2] = BOX2_RES_NONE;
}

void func_ov255_021d06a4(Box2SysWork *syswk, Box2PokeInfo *info, u32 id) {
    u32 *chr;
    u32 *pal;
    u32 *cell;
    u32 other;
    u16 palOffset;
    ArcTool *arc;
    BOOL encrypted;

    if (syswk->app->pokegraSwap == 0) {
        chr = &syswk->app->chrRes[96];
        pal = &syswk->app->palRes[1];
        cell = &syswk->app->cellRes[1];
        palOffset = 0x60;
        other = id + 1;
    } else {
        chr = &syswk->app->chrRes[97];
        pal = &syswk->app->palRes[2];
        cell = &syswk->app->cellRes[2];
        palOffset = 0x80;
        other = id;
        id++;
    }
    if (syswk->app->actors[id] != NULL) {
        func_0204c108(syswk->app->actors[id]);
        func_0204b98c(*chr);
        func_0204bcd0(*pal);
        func_0204be64(*cell);
    }
    arc = MakePokeGraArcHandle(HEAPID_BOX2_APP);
    encrypted = PML_PkmDecrypt(info->pkm);
    *chr = PokeGra_LoadClActCharsByBoxData(arc, info->pkm, POKEGRA_DIR_FRONT, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    *pal = PokeGra_LoadClActPaletteByBoxData(arc, info->pkm, POKEGRA_DIR_FRONT, CLACT_VRAM_SUB, palOffset,
                                             HEAPID_BOX2_APP);
    *cell = PokeGra_LoadClActCellAnimsByBoxData(info->pkm, POKEGRA_DIR_FRONT, 2, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    PML_PkmReEncrypt(info->pkm, encrypted);
    GFL_ArcToolFree(arc);
    syswk->app->actors[id] = func_ov255_021cf51c(syswk->app, &sActorData[id]);
    func_ov255_021cf63c(syswk->app, id, TRUE);
    if (syswk->app->actors[other] != NULL) {
        func_ov255_021cf63c(syswk->app, other, FALSE);
    }
    syswk->app->pokegraSwap ^= 1;
}

static void func_ov255_021d07d0(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_BOX2_APP | HEAPID_TAIL_BIT);
    u32 i;

    for (i = 0; i < 17; i++) {
        app->chrRes[99 + i] = func_0204b81c(arc, func_0202d7f4(i), FALSE, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    }
    app->cellRes[4] = func_0204bde0(arc, func_0202d7f8(2), func_0202d7fc(2), HEAPID_BOX2_APP);
    app->palRes[4] = func_0204bba0(arc, func_0202d7e4(), CLACT_VRAM_SUB, 0, HEAPID_BOX2_APP);
    app->chrRes[126] = func_0204b81c(arc, func_0202d948(2), FALSE, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    app->cellRes[10] = func_0204bde0(arc, func_0202d94c(2), func_0202d950(2), HEAPID_BOX2_APP);
    app->palRes[9] = func_0204bba0(arc, func_0202d944(), CLACT_VRAM_SUB, 0xa0, HEAPID_BOX2_APP);
    app->chrRes[127] = func_0204b81c(arc, func_0202d968(2), FALSE, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    app->cellRes[11] = func_0204bde0(arc, func_0202d96c(2), func_0202d970(2), HEAPID_BOX2_APP);
    app->palRes[10] = func_0204bba0(arc, func_0202d964(), CLACT_VRAM_SUB, 0xc0, HEAPID_BOX2_APP);
    app->chrRes[125] = func_0204b81c(arc, func_0202d948(2), FALSE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[9] = func_0204bde0(arc, func_0202d94c(2), func_0202d950(2), HEAPID_BOX2_APP);
    app->palRes[8] = func_0204bba0(arc, func_0202d944(), CLACT_VRAM_MAIN, 0x1c0, HEAPID_BOX2_APP);
    app->chrRes[128] = func_0204b81c(arc, 175, FALSE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[12] = func_0204bde0(arc, 174, 173, HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void func_ov255_021d0978(Box2AppWork *app) {
    Box2ActorData data = sTypeIconData;
    u32 i;

    for (i = 0; i < 17; i++) {
        data.chr = 99 + i;
        app->actors[32 + i] = func_ov255_021cf51c(app, &data);
        func_0204c378(app->actors[32 + i], func_0202d7e8(i), 1);
        func_ov255_021cf63c(app, 32 + i, FALSE);
    }
}

static void func_ov255_021d09dc(Box2AppWork *app, u32 type1, u32 type2) {
    u32 id = BOX2_ACTOR_TYPE_ICON + type1;

    func_ov255_021cf63c(app, id, TRUE);
    func_ov255_021cf6c8(app, id, 88, 56, 1);
    if (type2 != 0 && type1 != type2) {
        id = BOX2_ACTOR_TYPE_ICON + type2;
        func_ov255_021cf63c(app, id, TRUE);
        func_ov255_021cf6c8(app, id, 122, 56, 1);
    }
}

void func_ov255_021d0a28(Box2AppWork *app, Box2PokeInfo *info) {
    u32 i;

    for (i = BOX2_ACTOR_TYPE_ICON; i < BOX2_ACTOR_TYPE_ICON + 17; i++) {
        func_ov255_021cf63c(app, i, FALSE);
    }
    if (info->egg == 0) {
        func_ov255_021d09dc(app, info->type1, info->type2);
    }
}

static void func_ov255_021d0a58(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMGRA, HEAPID_BOX2_APP);

    app->palRes[3] = func_0204bba0(arc, GetItemGraphicsDatID(0, 2), CLACT_VRAM_MAIN, 0x1a0, HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

void func_ov255_021d0a94(Box2AppWork *app, u16 item) {
    NNSG2dCharacterData *chr;
    NNSG2dPaletteData *pal;
    void *buf;

    buf = GFL_G2DIOReadOBJNCGR(PML_ItemGetIconArcID(), GetItemGraphicsDatID(item, 1), FALSE, &chr, HEAPID_BOX2_APP);
    func_0204ba40(func_0204c428(app->actors[BOX2_ACTOR_ITEM_ICON]), chr);
    GFL_HeapFree(buf);
    buf = GFL_G2DIOReadNCLR(PML_ItemGetIconArcID(), GetItemGraphicsDatID(item, 2), &pal, HEAPID_BOX2_APP);
    func_0204bd10(func_0204c430(app->actors[BOX2_ACTOR_ITEM_ICON]), pal, 1);
    GFL_HeapFree(buf);
}

void func_ov255_021d0b08(Box2AppWork *app, BOOL affine) {
    ClActorPos center;

    if (affine == TRUE) {
        func_0204c244(app->actors[BOX2_ACTOR_ITEM_ICON], 1);
        center.x = 12;
        center.y = 12;
        func_0204c258(app->actors[BOX2_ACTOR_ITEM_ICON], &center);
    } else {
        center.x = 0;
        center.y = 0;
        func_0204c258(app->actors[BOX2_ACTOR_ITEM_ICON], &center);
        func_0204c244(app->actors[BOX2_ACTOR_ITEM_ICON], 0);
    }
}

void func_ov255_021d0b4c(Box2AppWork *app, s16 x, s16 y) {
    func_ov255_021cf6c8(app, BOX2_ACTOR_ITEM_ICON, x, y, 0);
}

void func_ov255_021d0b64(Box2AppWork *app, u32 pos, u32 mode) {
    s16 x;
    s16 y;

    func_ov255_021cfcdc(pos, &x, &y, mode);
    func_ov255_021d0b4c(app, x + 8, y + 8);
}

void func_ov255_021d0b98(Box2AppWork *app, u32 pos, u32 mode) {
    s16 x;
    s16 y;

    func_ov255_021cfcdc(pos, &x, &y, mode);
    func_ov255_021d0b4c(app, x, y + 4);
}

void func_ov255_021d0bc8(Box2AppWork *app) {
    Box2ActorData data;
    s16 x;
    s16 y;
    u32 i;

    if (app->getItem != 0) {
        func_ov255_021cf6ec(app, BOX2_ACTOR_ITEM_ICON, &x, &y, 0);
        if (app->actors[BOX2_ACTOR_ITEM_OUTLINE] == NULL) {
            data = sActorData[BOX2_ACTOR_ITEM_ICON];
            data.setup.priority = func_0204c45c(app->actors[BOX2_ACTOR_ITEM_ICON]) + 1;
            data.setup.bgPriority = func_0204c47c(app->actors[BOX2_ACTOR_ITEM_ICON]);
            data.chr = 98;
            data.pal = 6;
            data.palette = 1;
            for (i = 0; i < 8; i++) {
                data.setup.x = x + sItemOutlineX[i];
                data.setup.y = y + sItemOutlineY[i];
                app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] = func_ov255_021cf51c(app, &data);
                func_0204c378(app->actors[BOX2_ACTOR_ITEM_OUTLINE + i], (u8)data.palette, 1);
                func_ov255_021cf63c(app, BOX2_ACTOR_ITEM_OUTLINE + i, FALSE);
            }
        } else {
            u32 priority = func_0204c45c(app->actors[BOX2_ACTOR_ITEM_ICON]);
            u32 bgPriority = func_0204c47c(app->actors[BOX2_ACTOR_ITEM_ICON]);

            for (i = 0; i < 8; i++) {
                func_ov255_021cf6c8(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i], 0);
                func_ov255_021cf684(app, BOX2_ACTOR_ITEM_OUTLINE + i, priority + 1);
                func_ov255_021cf66c(app, BOX2_ACTOR_ITEM_OUTLINE + i, bgPriority);
                func_ov255_021cf63c(app, BOX2_ACTOR_ITEM_OUTLINE + i, FALSE);
            }
        }
    }
}

void func_ov255_021d0cf4(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < 8; i++) {
        func_ov255_021cf63c(app, BOX2_ACTOR_ITEM_OUTLINE + i, TRUE);
    }
}

void func_ov255_021d0d10(Box2AppWork *app) {
    s16 x;
    s16 y;
    u32 i;

    if (app->actors[BOX2_ACTOR_ITEM_OUTLINE] != NULL) {
        func_ov255_021cf6ec(app, BOX2_ACTOR_ITEM_ICON, &x, &y, 0);
        for (i = 0; i < 8; i++) {
            func_ov255_021cf6c8(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i], 0);
        }
    }
}

static void func_ov255_021d0d70(Box2SysWork *syswk) {
    Box2AppWork *app = syswk->app;
    // The SDK's u32 is unsigned long, which MWCC indexes with differently than unsigned int
    unsigned long i;

    app->actors[0] = func_ov255_021cf51c(app, &sActorData[0]);
    app->actors[1] = func_ov255_021cf51c(app, &sActorData[1]);
    app->actors[2] = func_ov255_021cf51c(app, &sActorData[2]);
    app->actors[3] = func_ov255_021cf51c(app, &sActorData[3]);
    app->actors[4] = func_ov255_021cf51c(app, &sActorData[4]);
    app->actors[5] = func_ov255_021cf51c(app, &sActorData[5]);
    app->actors[12] = func_ov255_021cf51c(app, &sActorData[12]);
    app->actors[6] = func_ov255_021cf51c(app, &sActorData[6]);
    app->actors[7] = func_ov255_021cf51c(app, &sActorData[7]);
    app->actors[8] = func_ov255_021cf51c(app, &sActorData[8]);
    app->actors[9] = func_ov255_021cf51c(app, &sActorData[9]);
    app->actors[10] = func_ov255_021cf51c(app, &sActorData[10]);
    app->actors[11] = func_ov255_021cf51c(app, &sActorData[11]);
    for (i = 0; i < 6; i++) {
        app->actors[15 + i] = func_ov255_021cf51c(app, &sActorData[15 + i]);
        app->actors[21 + i] = func_ov255_021cf51c(app, &sActorData[21 + i]);
    }
    app->actors[27] = func_ov255_021cf51c(app, &sActorData[27]);
    app->actors[28] = func_ov255_021cf51c(app, &sActorData[28]);
    app->actors[29] = func_ov255_021cf51c(app, &sActorData[29]);
    app->actors[30] = func_ov255_021cf51c(app, &sActorData[30]);
    app->actors[31] = func_ov255_021cf51c(app, &sActorData[31]);
    func_ov255_021cf63c(app, 5, FALSE);
    func_ov255_021cf63c(app, BOX2_ACTOR_ITEM_ICON, FALSE);
    func_ov255_021cf63c(app, 27, FALSE);
    func_ov255_021cf63c(app, 28, FALSE);
    func_ov255_021cf63c(app, 29, FALSE);
    func_ov255_021cf63c(app, 31, FALSE);
    for (i = 0; i < 16; i++) {
        app->actors[BOX2_ACTOR_RANGE_DOT + i] = func_ov255_021cf51c(app, &sRangeDotData);
        func_ov255_021cf63c(app, BOX2_ACTOR_RANGE_DOT + i, FALSE);
    }
    func_ov255_021d0f88(syswk, 9, TRUE);
}

void func_ov255_021d0f88(Box2SysWork *syswk, u32 set, BOOL visible) {
    int i;

    if (syswk->param->mode != 2 && syswk->param->mode != 4 && syswk->param->mode != 3) {
        for (i = 9; i <= 11; i++) {
            func_ov255_021cf63c(syswk->app, i, FALSE);
        }
    } else {
        for (i = 0; i < 3; i++) {
            func_ov255_021cf5e4(syswk->app, 9 + i, sButtonAnims[set - 9][i]);
            func_ov255_021cf63c(syswk->app, 9 + i, visible);
        }
        if (syswk->param->mode == 3) {
            func_ov255_021cf63c(syswk->app, 11, FALSE);
        }
    }
}

void func_ov255_021d0ff8(Box2SysWork *syswk, u32 anim) {
    if (syswk->moveMode == 1) {
        anim += 4;
    } else if (syswk->moveMode == 2) {
        anim += 9;
    }
    func_ov255_021cf5e4(syswk->app, BOX2_ACTOR_CURSOR, anim);
}

void func_ov255_021d101c(Box2SysWork *syswk, BOOL show) {
    if (show == TRUE) {
        if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
            func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
        }
    } else {
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
    }
}

void func_ov255_021d1048(Box2SysWork *syswk) {
    func_ov255_021d1054(syswk, syswk->pos);
}

void func_ov255_021d1054(Box2SysWork *syswk, u32 pos) {
    Box2ActorData data;
    NNSG2dImageProxy proxy;
    s16 x;
    s16 y;
    Box2AppWork *app;
    u32 id;
    u16 i;

    if (pos != BOX2_GET_NONE) {
        app = syswk->app;
        id = app->pokeIconId[pos];
        func_ov255_021cf6ec(app, id, &x, &y, 0);
        if (app->actors[BOX2_ACTOR_ITEM_OUTLINE] == NULL) {
            data = sPokeIconData;
            data.setup.priority = func_0204c45c(app->actors[id]) + 1;
            data.setup.bgPriority = func_0204c47c(app->actors[id]);
            data.chr = id - BOX2_ACTOR_POKEICON;
            data.pal = 6;
            data.palette = 1;
            for (i = 0; i < 8; i++) {
                data.setup.x = x + sItemOutlineX[i];
                data.setup.y = y + sItemOutlineY[i];
                app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] = func_ov255_021cf51c(app, &data);
                func_0204c378(app->actors[BOX2_ACTOR_ITEM_OUTLINE + i], (u8)data.palette, 1);
            }
        } else {
            u32 priority;
            u32 bgPriority;

            func_0204c40c(app->actors[id], &proxy);
            priority = func_0204c45c(app->actors[id]);
            bgPriority = func_0204c47c(app->actors[id]);
            for (i = 0; i < 8; i++) {
                func_0204c3e4(app->actors[BOX2_ACTOR_ITEM_OUTLINE + i], &proxy);
                func_ov255_021cf6c8(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i], 0);
                func_ov255_021cf684(app, BOX2_ACTOR_ITEM_OUTLINE + i, priority + 1);
                func_ov255_021cf66c(app, BOX2_ACTOR_ITEM_OUTLINE + i, bgPriority);
                func_ov255_021cf63c(app, BOX2_ACTOR_ITEM_OUTLINE + i, TRUE);
            }
        }
    }
}

void func_ov255_021d11a4(Box2SysWork *syswk, BOOL visible) {
    u32 i;
    u32 id;

    for (i = 0; i < 8; i++) {
        if (syswk->app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] != NULL) {
            func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, visible);
            if (visible == TRUE && syswk->pos != BOX2_GET_NONE) {
                id = syswk->app->pokeIconId[syswk->pos];
                func_ov255_021cf684(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i,
                                    func_0204c45c(syswk->app->actors[id]) + 1);
                func_ov255_021cf66c(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, func_0204c47c(syswk->app->actors[id]));
            }
        }
    }
}

void func_ov255_021d121c(Box2SysWork *syswk, u32 pos) {
    u32 id = syswk->app->pokeIconId[pos];
    u32 priority = func_0204c45c(syswk->app->actors[id]);
    u32 bgPriority = func_0204c47c(syswk->app->actors[id]);
    u32 i;

    for (i = 0; i < 8; i++) {
        if (syswk->app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] != NULL && syswk->pos != BOX2_GET_NONE) {
            func_ov255_021cf684(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, priority + 1);
            func_ov255_021cf66c(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, bgPriority);
        }
    }
}

void func_ov255_021d1284(Box2AppWork *app, u32 pos) {
    s16 x;
    s16 y;
    u32 i;

    if (pos != BOX2_GET_NONE) {
        func_ov255_021cf6ec(app, app->pokeIconId[pos], &x, &y, 0);
        for (i = 0; i < 8; i++) {
            func_ov255_021cf6c8(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i], 0);
        }
    }
}

static void func_ov255_021d12e4(Box2AppWork *app) {
    Box2ActorData data;
    Box2ActorData base = sTrayIconData;
    u32 i;

    for (i = 0; i < 6; i++) {
        data = base;
        data.setup.y += (s16)(i * 34);
        data.chr = 116 + i;
        app->actors[BOX2_ACTOR_TRAY_ICON + i] = func_ov255_021cf51c(app, &data);
    }
}

void func_ov255_021d1348(Box2AppWork *app, BOOL visible) {
    func_ov255_021cf63c(app, 0, visible);
    func_ov255_021cf63c(app, 1, visible);
}

void func_ov255_021d1364(Box2SysWork *syswk) {
    s8 x;
    s8 y;
    s16 px;
    s16 py;
    u32 i;

    BGWinFrame_GetPos(syswk->app->bgWinFrame, 7, &x, &y);
    py = y * 8 + 8;
    px = x * 8 + 23;
    for (i = 0; i < 6; i++) {
        func_ov255_021cf6c8(syswk->app, 15 + i, px + (i % 2) * 32, py + (i / 2) * 24, 0);
    }
}

void func_ov255_021d13c4(Box2SysWork *syswk) {
    func_ov255_021d1570(syswk, syswk->tray);
    func_ov255_021d1488(syswk);
}

void func_ov255_021d13d8(Box2SysWork *syswk, s32 mv) {
    s16 x;
    s16 y;
    u32 i;

    func_ov255_021cf6ec(syswk->app, 2, &x, &y, 0);
    func_ov255_021cf6c8(syswk->app, 2, x - mv, y, 0);
    for (i = 0; i < 6; i++) {
        func_ov255_021cf6ec(syswk->app, BOX2_ACTOR_TRAY_ICON + i, &x, &y, 0);
        func_ov255_021cf6c8(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x - mv, y, 0);
        BmpOam_ActorGetPos(syswk->app->fontOam[1 + i].oam, &x, &y);
        BmpOam_ActorSetPos(syswk->app->fontOam[1 + i].oam, x - mv, y);
    }
}

void func_ov255_021d1474(Box2AppWork *app) {
    func_ov255_021cf6c8(app, 2, 300, 34, 0);
}

static void func_ov255_021d1488(Box2SysWork *syswk) {
    Box2ActorData data;
    Box2ActorData base = sTrayIconData;
    u32 tray;
    s16 i;

    for (i = 0; i < 6; i++) {
        tray = Box2Main_GetTrayScroll(syswk, i - 1);
        data = base;
        data.setup.y += (s16)(i * 34);
        func_ov255_021cf6c8(syswk->app, BOX2_ACTOR_TRAY_ICON + i, data.setup.x, data.setup.y, 0);
        func_ov255_021d1624(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
        BmpOam_ActorSetPos(syswk->app->fontOam[1 + i].oam, 316, i * 34 + 2);
        func_ov255_021cedb4(syswk, tray, i + 1);
    }
}

void func_ov255_021d1530(Box2AppWork *app, u32 row, s16 *x, s16 *y) {
    u32 i;

    row = (row + 1) * 34;
    for (i = 0; i < 6; i++) {
        func_ov255_021cf6ec(app, BOX2_ACTOR_TRAY_ICON + i, x, y, 0);
        if (row == *y) {
            return;
        }
    }
    *x = 0;
    *y = 0;
}

void func_ov255_021d1570(Box2SysWork *syswk, u32 tray) {
    NNSG2dCharacterData *chr;
    void *buf = GFL_G2DIOReadOBJNCGR(ARCID_BOX2, 71, TRUE, &chr, HEAPID_BOX2_APP);

    sys_memcpy(chr->rawData, syswk->app->trayIconChar[tray], 0x400);
    GFL_HeapFree(buf);
    func_ov255_021d16a8(syswk, syswk->app->trayIconChar[tray], Box2Main_GetWallPaperNumber(syswk, tray), 8, 0x400);
    func_ov255_021d16cc(syswk, tray, syswk->app->trayIconChar[tray]);
}

void func_ov255_021d15dc(Box2SysWork *syswk) {
    u32 i;

    for (i = 0; i < BOX2_TRAY_MAX; i++) {
        func_ov255_021d1570(syswk, i);
    }
}

void func_ov255_021d15f4(Box2SysWork *syswk, u32 tray) {
    s16 i;

    for (i = 0; i < 6; i++) {
        u32 scroll = Box2Main_GetTrayScroll(syswk, i - 1);

        if (scroll == tray) {
            func_ov255_021d1654(syswk, tray, i);
            return;
        }
    }
}

static void func_ov255_021d1624(Box2SysWork *syswk, u32 tray, u32 id) {
    Box2AppWork *app = syswk->app;

    func_0204bab8(func_0204c428(app->actors[id]), app->trayIconChar[tray], 0x400, 0, CLACT_VRAM_MAIN);
}

static void func_ov255_021d1654(Box2SysWork *syswk, u32 tray, u32 row) {
    s16 x;
    s16 y;
    u32 i;

    row *= 34;
    for (i = 0; i < 6; i++) {
        func_ov255_021cf6ec(syswk->app, BOX2_ACTOR_TRAY_ICON + i, &x, &y, 0);
        if (row == y) {
            func_ov255_021d1624(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
            func_ov255_021cedb4(syswk, tray, i + 1);
            return;
        }
    }
}

static void func_ov255_021d16a8(Box2SysWork *syswk, u8 *chr, u32 wallpaper, u32 color, u32 size) {
    u32 i;

    for (i = 0; i < size; i++) {
        if (chr[i] == color) {
            chr[i] = sTrayIconColors[wallpaper];
        }
    }
}

static void func_ov255_021d16cc(Box2SysWork *syswk, u32 tray, u8 *chr) {
    BoxPkm *pkm;
    BOOL encrypted;
    u8 j;
    u8 i;
    u8 px;
    u8 py;
    u32 species;
    void *personal;
    u8 y;
    u16 color;
    u16 form;

    py = 11;
    for (j = 0; j < 5; j++) {
        px = 10;
        for (i = 0; i < 6; i++) {
            pkm = BoxSaveAccessor_GetPkm(syswk->param->boxes, tray, i + j * 6);
            encrypted = PML_PkmDecrypt(pkm);
            species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
            if (PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                if (PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
                    form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
                    personal = PML_PersonalLoad(species, form, HEAPID_BOX2_APP);
                    color = PML_PersonalGetParam(personal, 33);
                    PML_PersonalFree(personal);
                } else if (species == SPECIES_MANAPHY) {
                    color = 1;
                } else {
                    color = 8;
                }
                color += 16;
                color = (color << 8) | color;
                for (y = py; y < py + 2; y++) {
                    sys_memset16(color, &chr[((y & 7) << 3) + ((((y >> 3) << 2) + (px >> 3)) << 6) + (px & 7)], 2);
                }
            }
            PML_PkmReEncrypt(pkm, encrypted);
            px += 2;
        }
        py += 2;
    }
}

void func_ov255_021d17f8(Box2SysWork *syswk, s16 mv) {
    s16 x;
    s16 y;
    s16 fx;
    s16 fy;
    s32 speed;
    s16 ny;
    s16 nfy;
    u32 i;

    speed = (mv < 0 ? -mv : mv) == 5 ? 2 : 8;
    if (mv < 0) {
        speed *= -1;
    }
    for (i = 0; i < 6; i++) {
        func_ov255_021cf6ec(syswk->app, BOX2_ACTOR_TRAY_ICON + i, &x, &y, 0);
        ny = y + speed;
        func_ov255_021cf6c8(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x, ny, 0);
        BmpOam_ActorGetPos(syswk->app->fontOam[1 + i].oam, &fx, &fy);
        nfy = fy + speed;
        BmpOam_ActorSetPos(syswk->app->fontOam[1 + i].oam, fx, nfy);
        if (ny < -16) {
            u32 tray = Box2Main_GetTrayScroll(syswk, 4);

            func_ov255_021d1624(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
            func_ov255_021cf6c8(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x, ny + 204, 0);
            func_ov255_021cedb4(syswk, tray, i + 1);
            BmpOam_ActorSetPos(syswk->app->fontOam[1 + i].oam, fx, nfy + 204);
        } else if (ny >= 186) {
            u32 tray = Box2Main_GetTrayScroll(syswk, -1);

            func_ov255_021d1624(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
            func_ov255_021cf6c8(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x, ny - 204, 0);
            func_ov255_021cedb4(syswk, tray, i + 1);
            BmpOam_ActorSetPos(syswk->app->fontOam[1 + i].oam, fx, nfy - 204);
        }
    }
    if (func_ov255_021cf658(syswk->app, 2) == TRUE) {
        func_ov255_021cf6ec(syswk->app, 2, &x, &y, 0);
        func_ov255_021cf6c8(syswk->app, 2, x, y + speed, 0);
    }
}

void func_ov255_021d198c(Box2SysWork *syswk, s32 mv) {
    s16 x;
    s16 y;

    if (mv < 0 && Box2Main_GetTrayScroll(syswk, -1) == syswk->tray) {
        func_ov255_021cf6ec(syswk->app, 2, &x, &y, 0);
        func_ov255_021cf6c8(syswk->app, 2, x, -34, 0);
        func_ov255_021cf63c(syswk->app, 2, TRUE);
    } else if (mv > 0 && Box2Main_GetTrayScroll(syswk, 4) == syswk->tray) {
        func_ov255_021cf6ec(syswk->app, 2, &x, &y, 0);
        func_ov255_021cf6c8(syswk->app, 2, x, 204, 0);
        func_ov255_021cf63c(syswk->app, 2, TRUE);
    }
}

void func_ov255_021d1a1c(Box2SysWork *syswk) {
    s16 x;
    s16 y;
    u32 i;

    for (i = 0; i < 6; i++) {
        if (Box2Main_GetTrayScroll(syswk, i - 1) == syswk->tray) {
            func_ov255_021cf6ec(syswk->app, 2, &x, &y, 0);
            func_ov255_021cf6c8(syswk->app, 2, x, i * 34, 0);
            func_ov255_021cf63c(syswk->app, 2, TRUE);
            return;
        }
    }
    func_ov255_021cf63c(syswk->app, 2, FALSE);
}

static void func_ov255_021d1a84(Box2SysWork *syswk, u32 index) {
    s16 y = sBoxNameY[index];

    func_ov255_021cf6c8(syswk->app, 3, 206, y, 0);
    func_ov255_021cf63c(syswk->app, 3, TRUE);
    BmpOam_ActorSetPos(syswk->app->fontOam[0].oam, 158, y - 8);
    BmpOam_ActorSetDrawEnable(syswk->app->fontOam[0].oam, TRUE);
}

void func_ov255_021d1ac8(Box2SysWork *syswk, u32 index, BOOL show) {
    if (show == TRUE) {
        func_ov255_021ced8c(syswk, index);
        func_ov255_021d1a84(syswk, index);
    } else {
        func_ov255_021cf63c(syswk->app, 3, FALSE);
        BmpOam_ActorSetDrawEnable(syswk->app->fontOam[0].oam, FALSE);
    }
}

void func_ov255_021d1af8(Box2SysWork *syswk, u8 button1, u8 button2, u8 button3, u8 button4) {
    syswk->unk20 = button2;
    syswk->unk21 = button3;
    syswk->unk1F = button1;
    syswk->unk22 = button4;
    if (button1 == 0) {
        func_ov255_021cf63c(syswk->app, 6, TRUE);
        func_ov255_021cf608(syswk->app, 6, 1);
    } else if (button1 == 1) {
        func_ov255_021cf63c(syswk->app, 6, FALSE);
    } else {
        func_ov255_021cf63c(syswk->app, 6, TRUE);
        func_ov255_021cf608(syswk->app, 6, 15);
    }
    if (button2 == 0) {
        func_ov255_021cf63c(syswk->app, 7, TRUE);
        func_ov255_021cf608(syswk->app, 7, 0);
    } else if (button2 == 1) {
        func_ov255_021cf63c(syswk->app, 7, FALSE);
    } else {
        func_ov255_021cf63c(syswk->app, 7, TRUE);
        func_ov255_021cf608(syswk->app, 7, 14);
    }
    if (button3 == 0) {
        func_ov255_021cf63c(syswk->app, 8, TRUE);
        func_ov255_021cf608(syswk->app, 8, 0);
    } else if (button3 == 1) {
        func_ov255_021cf63c(syswk->app, 8, FALSE);
    } else {
        func_ov255_021cf63c(syswk->app, 8, TRUE);
        func_ov255_021cf608(syswk->app, 8, 2);
    }
    if (button4 == 0) {
        func_ov255_021cf63c(syswk->app, 30, TRUE);
        func_ov255_021cf608(syswk->app, 30, 0);
    } else if (button4 == 1) {
        func_ov255_021cf63c(syswk->app, 30, FALSE);
    } else {
        func_ov255_021cf63c(syswk->app, 30, TRUE);
        func_ov255_021cf608(syswk->app, 30, 2);
    }
}

void func_ov255_021d1c00(Box2SysWork *syswk) {
    func_ov255_021cf63c(syswk->app, 6, FALSE);
    func_ov255_021cf63c(syswk->app, 7, FALSE);
    func_ov255_021cf63c(syswk->app, 8, FALSE);
    func_ov255_021cf63c(syswk->app, 30, FALSE);
}

static void func_ov255_021d1c30(Box2AppWork *app) {
    BmpOamActorSetup setup;
    u32 i;
    Box2FontOam *font;

    app->bmpOam = BmpOam_Init(HEAPID_BOX2_APP, app->clunit);
    app->fontOam[0].bitmap = GFL_BitmapCreate(12, 2, 0x20, HEAPID_BOX2_APP);
    setup.bitmap = app->fontOam[0].bitmap;
    setup.x = 0;
    setup.y = 0;
    setup.palette = app->palRes[6];
    setup.paletteOffset = 0;
    setup.priority = 30;
    setup.bgPriority = 0;
    setup.surface = 0xffff;
    setup.vramType = CLACT_VRAM_MAIN;
    app->fontOam[0].oam = BmpOam_ActorAdd(app->bmpOam, &setup);
    BmpOam_ActorSetDrawEnable(app->fontOam[0].oam, FALSE);
    for (i = 7; i <= 8; i++) {
        font = &app->fontOam[i];
        font->bitmap = GFL_BitmapCreate(12, 2, 0x20, HEAPID_BOX2_APP);
        setup.bitmap = font->bitmap;
        setup.x = 36;
        setup.y = 20;
        setup.bgPriority = 3;
        font->oam = BmpOam_ActorAdd(app->bmpOam, &setup);
    }
    for (i = 0; i < 6; i++) {
        font = &app->fontOam[1 + i];
        font->bitmap = GFL_BitmapCreate(2, 1, 0x20, HEAPID_BOX2_APP);
        setup.bitmap = font->bitmap;
        setup.x = 316;
        setup.y = i * 34 + 2;
        setup.bgPriority = 1;
        font->oam = BmpOam_ActorAdd(app->bmpOam, &setup);
    }
    font = &app->fontOam[9];
    font->bitmap = GFL_BitmapCreate(2, 2, 0x20, HEAPID_BOX2_APP);
    setup.bitmap = font->bitmap;
    setup.x = 0;
    setup.y = 0;
    setup.priority = 45;
    setup.bgPriority = 0;
    font->oam = BmpOam_ActorAdd(app->bmpOam, &setup);
    func_ov255_021d1d68(app, 9, FALSE);
}

static void func_ov255_021d1d44(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < NELEMS(app->fontOam); i++) {
        BmpOam_ActorDel(app->fontOam[i].oam);
        GFL_BitmapFree(app->fontOam[i].bitmap);
    }
    BmpOam_Exit(app->bmpOam);
}

void func_ov255_021d1d68(Box2AppWork *app, u32 index, BOOL visible) {
    BmpOam_ActorSetDrawEnable(app->fontOam[index].oam, visible);
}

BOOL func_ov255_021d1d78(Box2AppWork *app, u32 index) {
    return BmpOam_ActorGetDrawEnable(app->fontOam[index].oam);
}

void func_ov255_021d1d88(Box2AppWork *app, u32 index, u32 dir) {
    if (dir == 0) {
        BmpOam_ActorSetPos(app->fontOam[index].oam, -148, 20);
    } else {
        BmpOam_ActorSetPos(app->fontOam[index].oam, 220, 20);
    }
}

void func_ov255_021d1db0(Box2AppWork *app, s32 mv) {
    s16 x;
    s16 y;
    u32 i;

    for (i = 7; i <= 8; i++) {
        BmpOam_ActorGetPos(app->fontOam[i].oam, &x, &y);
        BmpOam_ActorSetPos(app->fontOam[i].oam, x + mv, y);
        if (x + mv == -148 || x + mv == 220) {
            func_ov255_021d1d68(app, i, FALSE);
        }
    }
}

static void func_ov255_021d1e08(Box2SysWork *syswk) {
    sys_memset(&syswk->app->rangeSelect, 0, sizeof(Box2RangeSelect));
    syswk->app->rangeSelect.anmMax = 384;
}

void func_ov255_021d1e2c(Box2SysWork *syswk, u32 anm) {
    syswk->app->rangeSelect.anm = anm;
}

void func_ov255_021d1e38(Box2SysWork *syswk) {
    s16 x;
    s16 y;
    s32 length;
    s32 half;
    s32 top;
    s32 right;
    s32 bottom;
    s32 left;
    s32 width;
    s32 height;
    s32 pos;
    s32 px;
    s32 py;
    int i;
    Box2RangeSelect *rs = &syswk->app->rangeSelect;

    if (rs->anm != rs->prevAnm) {
        for (i = 0; i < 16; i++) {
            func_ov255_021cf63c(syswk->app, BOX2_ACTOR_RANGE_DOT + i, rs->anm);
        }
    }
    rs->prevAnm = rs->anm;
    rs->anmCnt++;
    if (rs->anmCnt >= rs->anmMax) {
        rs->anmCnt = 0;
    }
    width = rs->right - rs->left;
    height = rs->bottom - rs->top;
    left = rs->left;
    top = rs->top;
    right = rs->right;
    bottom = rs->bottom;
    half = width + height;
    length = half * 2;
    if (syswk->unk18 == 2) {
        func_ov255_021cf6ec(syswk->app, syswk->app->pokeIconId[BOX2_BOXLIST_POS], &x, &y, 0);
        left = x - 14;
        top = y - 6;
        right = left + width;
        bottom = top + height;
    }
    for (i = 0; i < 16; i++) {
        pos = (rs->anmCnt + i * 24) * length / rs->anmMax % length;
        if (pos < half) {
            if (pos < width) {
                px = left + pos;
                py = top;
            } else {
                px = right;
                py = top + (pos - width);
            }
        } else {
            pos -= half;
            if (pos < width) {
                px = right - pos;
                py = bottom;
            } else {
                px = left;
                py = bottom - (pos - width);
            }
        }
        func_ov255_021cf6c8(syswk->app, BOX2_ACTOR_RANGE_DOT + i, px, py, 0);
    }
}

static void func_ov255_021d1f40(s32 *a, s32 *b) {
    s32 tmp;

    if (*a > *b) {
        tmp = *a;
        *a = *b;
        *b = tmp;
    }
}

static void func_ov255_021d1f50(Box2SysWork *syswk, s32 left, s32 top, s32 right, s32 bottom) {
    Box2RangeSelect *rs = &syswk->app->rangeSelect;

    func_ov255_021d1f40(&left, &right);
    func_ov255_021d1f40(&top, &bottom);
    rs->left = left;
    rs->top = top;
    rs->right = right;
    rs->bottom = bottom;
}

static s32 func_ov255_021d1f88(s32 a, s32 b, s32 c, s32 d) {
    if (a <= b && a <= c && a <= d) {
        return a;
    }
    if (b <= a && b <= c && b <= d) {
        return b;
    }
    if (c <= a && c <= b && c <= d) {
        return c;
    }
    return d;
}

static s32 func_ov255_021d1fb8(s32 a, s32 b, s32 c, s32 d) {
    if (a >= b && a >= c && a >= d) {
        return a;
    }
    if (b >= a && b >= c && b >= d) {
        return b;
    }
    if (c >= a && c >= b && c >= d) {
        return c;
    }
    return d;
}

static void func_ov255_021d1fe8(int pos, Box2Rect *rect) {
    rect->left = pos % 6 * 24 + 10;
    rect->top = pos / 6 * 24 + 42;
    rect->right = rect->left + 26;
    rect->bottom = rect->top + 22;
}

static void func_ov255_021d201c(int pos, Box2Rect *rect) {
    int col;
    int row;

    pos -= BOX2_PARTY_POS;
    col = pos % 2;
    row = pos / 2;
    rect->left = col * 40 + 178;
    if (pos & 1) {
        rect->top = row * 32 + 66;
    } else {
        rect->top = row * 32 + 58;
    }
    rect->right = rect->left + 26;
    rect->bottom = rect->top + 22;
}

static void func_ov255_021d205c(int pos, Box2Rect *rect) {
    s32 x;
    s32 y;

    pos -= BOX2_PARTY_POS;
    x = pos % 2 * 24 + 178;
    y = pos / 2 * 24 + 58;
    rect->left = x;
    rect->top = y;
    rect->right = x + 26;
    rect->bottom = y + 22;
}

void func_ov255_021d208c(Box2SysWork *syswk, int start, int end, u32 mode) {
    Box2Rect rect1;
    Box2Rect rect2;
    s32 left;
    s32 top;
    s32 right;

    if (start < BOX2_TRAY_POKE_MAX) {
        func_ov255_021d1fe8(start, &rect1);
        func_ov255_021d1fe8(end, &rect2);
    } else if (mode == 1) {
        func_ov255_021d205c(start, &rect1);
        func_ov255_021d205c(end, &rect2);
    } else {
        func_ov255_021d201c(start, &rect1);
        func_ov255_021d201c(end, &rect2);
    }
    left = func_ov255_021d1f88(rect1.left, rect1.right, rect2.left, rect2.right);
    top = func_ov255_021d1f88(rect1.top, rect1.bottom, rect2.top, rect2.bottom);
    right = func_ov255_021d1fb8(rect1.left, rect1.right, rect2.left, rect2.right);
    func_ov255_021d1f50(syswk, left, top, right, func_ov255_021d1fb8(rect1.top, rect1.bottom, rect2.top, rect2.bottom));
}

static void func_ov255_021d2118(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2) {
    *x1 = start % 6;
    *y1 = start / 6;
    *x2 = end % 6;
    *y2 = end / 6;
    func_ov255_021d1f40(x1, x2);
    func_ov255_021d1f40(y1, y2);
}

static void func_ov255_021d2160(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2) {
    *x1 = start % 2;
    *y1 = start / 2;
    *x2 = end % 2;
    *y2 = end / 2;
    func_ov255_021d1f40(x1, x2);
    func_ov255_021d1f40(y1, y2);
}

void func_ov255_021d21a4(int start, int end, u32 *width, u32 *height) {
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;

    if (start >= BOX2_TRAY_POKE_MAX) {
        func_ov255_021d2160(start - BOX2_TRAY_POKE_MAX, end - BOX2_TRAY_POKE_MAX, &x1, &y1, &x2, &y2);
    } else {
        func_ov255_021d2118(start, end, &x1, &y1, &x2, &y2);
    }
    *width = x2 - x1 + 1;
    *height = y2 - y1 + 1;
}

u32 func_ov255_021d21ec(int start, int end) {
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;

    func_ov255_021d2118(start, end, &x1, &y1, &x2, &y2);
    return x1 + y1 * 6;
}

u32 func_ov255_021d2210(int start, int end) {
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;

    func_ov255_021d2160(start - BOX2_TRAY_POKE_MAX, end - BOX2_TRAY_POKE_MAX, &x1, &y1, &x2, &y2);
    return x1 + y1 * 2 + BOX2_TRAY_POKE_MAX;
}

void func_ov255_021d2238(Box2AppWork *app, u32 pos, u32 width, u32 height, BOOL put) {
    u32 id;
    u32 i;
    u32 j;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            id = app->pokeIconId[BOX2_BOXLIST_POS + j * width + i];
            if (put == FALSE) {
                func_ov255_021cf66c(app, id, 0);
                func_ov255_021cf684(app, id, 20);
            }
        }
    }
}

static BOOL func_ov255_021d229c(Box2SysWork *syswk, u32 pos) {
    u32 j;

    if (syswk->pos >= BOX2_TRAY_POKE_MAX) {
        return FALSE;
    }
    for (j = 0; j < syswk->app->rangeHeight; j++) {
        if (pos >= syswk->pos + j * 6 && pos < syswk->app->rangeWidth + (syswk->pos + j * 6)) {
            return TRUE;
        }
    }
    return FALSE;
}

void func_ov255_021d22e0(Box2AppWork *app, BOOL visible) {
    func_ov255_021cf63c(app, 31, visible);
    func_ov255_021d1d68(app, 9, visible);
}

void func_ov255_021d22fc(Box2AppWork *app, s16 x, s16 y) {
    func_ov255_021cf6c8(app, 31, x, y, 0);
    BmpOam_ActorSetPos(app->fontOam[9].oam, x - 16, y - 16);
}

void func_ov255_021d232c(Box2SysWork *syswk, BOOL visible) {
    u32 rowWidth;
    u16 j;
    u16 i;

    if (syswk->moveMode == 2) {
        func_ov255_021d22e0(syswk->app, visible);
        visible ^= 1;
        rowWidth = Box2Main_GetRowWidth(syswk, syswk->app->rangeSelect.startPos);
        for (j = 0; j < syswk->app->rangeHeight; j++) {
            for (i = 0; i < syswk->app->rangeWidth; i++) {
                if (syswk->app->rangeFlags[j * rowWidth + i] != 0) {
                    func_ov255_021cf63c(syswk->app,
                                        syswk->app->pokeIconId[BOX2_BOXLIST_POS + j * syswk->app->rangeWidth + i],
                                        visible);
                }
            }
        }
        func_ov255_021d1e2c(syswk, visible);
    }
}
