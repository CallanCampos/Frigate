#include "types.h"
#include "app/box2_bmp.h"
#include "app/box2_main.h"
#include "app/box2_obj.h"
#include "app/box2_ui.h"
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

static void CreateClActSys(Box2AppWork *app);
static void LoadResources(Box2AppWork *app);
static void FreeResources(Box2AppWork *app);
static void CreateActors(Box2SysWork *syswk);
static ClActor *CreateActor(Box2AppWork *app, const Box2ActorData *data);
static void DeleteBoxActor(Box2AppWork *app, u32 id);
static void DeleteBoxActors(Box2AppWork *app);
static void SetBgPriority(Box2AppWork *app, u32 id, u32 priority);
static void SetSoftPriority(Box2AppWork *app, u32 id, u32 priority);
static void SetBlend(Box2AppWork *app, u32 id, BOOL blend);
static void SetPalette(Box2AppWork *app, u32 id, u32 palette);
static void LoadBoxResources(Box2AppWork *app);
static void LoadUiResources(Box2AppWork *app);
static void LoadPokeIconResources(Box2AppWork *app);
static void CreatePokeIconActors(Box2AppWork *app);
static void *ReadPokeIconChar(Box2AppWork *app, BoxPkm *pkm, NNSG2dCharacterData **chr);
static BOOL MatchesSearch(Box2SearchParam *search, BoxPkm *pkm);
static u32 GetPokeIconPalette(Box2SysWork *syswk, BoxPkm *pkm);
static void TransferIconChar(Box2AppWork *app, void *chr, u32 id, u32 palette);
static void SetPokeIcon(Box2SysWork *syswk, BoxPkm *pkm, u32 id);
static void SetIconsBlend(Box2SysWork *syswk, BOOL blend, u32 start, u32 end, BOOL check);
static void SetIconBlendChecked(Box2SysWork *syswk, u32 pos);
static void ClearPokeGraResources(Box2AppWork *app);
static void LoadInfoResources(Box2AppWork *app);
static void CreateTypeIconActors(Box2AppWork *app);
static void ShowTypeIcons(Box2AppWork *app, u32 type1, u32 type2);
static void LoadItemIconPalette(Box2AppWork *app);
static void CreateFixedActors(Box2SysWork *syswk);
static void CreateTrayIconActors(Box2AppWork *app);
static void PlaceTrayIcons(Box2SysWork *syswk);
static void SetTrayIconChar(Box2SysWork *syswk, u32 tray, u32 id);
static void RefreshTrayIconRow(Box2SysWork *syswk, u32 tray, u32 row);
static void ReplaceTrayIconColor(Box2SysWork *syswk, u8 *chr, u32 wallpaper, u32 color, u32 size);
static void DrawTrayIconPokes(Box2SysWork *syswk, u32 tray, u8 *chr);
static void PutBoxNameFrame(Box2SysWork *syswk, u32 index);
static void CreateFontOams(Box2AppWork *app);
static void DeleteFontOams(Box2AppWork *app);
static void InitRangeSelect(Box2SysWork *syswk);
static void SortPair(s32 *a, s32 *b);
static void SetRangeRect(Box2SysWork *syswk, s32 left, s32 top, s32 right, s32 bottom);
static s32 Min4(s32 a, s32 b, s32 c, s32 d);
static s32 Max4(s32 a, s32 b, s32 c, s32 d);
static void GetTrayPokeRect(int pos, Box2Rect *rect);
static void GetPartyPokeRect(int pos, Box2Rect *rect);
static void GetPartyPokeRectCompact(int pos, Box2Rect *rect);
static void GetTrayRangeCorners(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2);
static void GetPartyRangeCorners(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2);
static BOOL IsInPickedRange(Box2SysWork *syswk, u32 pos);

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
// clang-format off
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

// clang-format on

void Box2Obj_Init(Box2SysWork *syswk) {
    CreateClActSys(syswk->app);
    LoadResources(syswk->app);
    CreateActors(syswk);
    Box2Obj_SetButtons(syswk, syswk->unk1F, syswk->unk20, syswk->unk21, syswk->unk22);
    CreateFontOams(syswk->app);
    InitRangeSelect(syswk);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void Box2Obj_Exit(Box2AppWork *app) {
    DeleteFontOams(app);
    DeleteBoxActors(app);
    FreeResources(app);
    ClActSys_Delete();
}

static void CreateClActSys(Box2AppWork *app) {
    ClActSysSetup setup = sClActSysSetup;
    ClActSys_Create(&setup, Box2Main_GetVramBanks(), HEAPID_BOX2_APP);
}

static void LoadResources(Box2AppWork *app) {
    LoadBoxResources(app);
    LoadUiResources(app);
    LoadPokeIconResources(app);
    LoadItemIconPalette(app);
    ClearPokeGraResources(app);
    LoadInfoResources(app);
}

static void FreeResources(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < NELEMS(app->chrRes); i++) {
        if (app->chrRes[i] != BOX2_RES_NONE) {
            ClActSys_FreeChars(app->chrRes[i]);
        }
    }
    for (i = 0; i < NELEMS(app->palRes); i++) {
        if (app->palRes[i] != BOX2_RES_NONE) {
            ClActSys_FreePltt(app->palRes[i]);
        }
    }
    for (i = 0; i < NELEMS(app->cellRes); i++) {
        if (app->cellRes[i] != BOX2_RES_NONE) {
            ClActSys_FreeCellAnims(app->cellRes[i]);
        }
    }
}

static void CreateActors(Box2SysWork *syswk) {
    syswk->app->clunit = ClActUnit_Create(233, 0, HEAPID_BOX2_APP);
    CreateTrayIconActors(syswk->app);
    CreatePokeIconActors(syswk->app);
    CreateFixedActors(syswk);
    CreateTypeIconActors(syswk->app);
}

static ClActor *CreateActor(Box2AppWork *app, const Box2ActorData *data) {
    ClActor *actor = ClActor_Create(app->clunit, app->chrRes[data->chr], app->palRes[data->pal],
                                    app->cellRes[data->cell], &data->setup, data->surface, HEAPID_BOX2_APP);
    ClActor_SetDrawAnim(actor, FALSE);
    return actor;
}

static void DeleteBoxActor(Box2AppWork *app, u32 id) {
    if (app->actors[id] != NULL) {
        ClActor_Delete(app->actors[id]);
        app->actors[id] = NULL;
    }
}

static void DeleteBoxActors(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < NELEMS(app->actors); i++) {
        DeleteBoxActor(app, i);
    }
    ClActUnit_Delete(app->clunit);
}

void Box2Obj_UpdateAnim(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < 49; i++) {
        if (app->actors[i] != NULL && ClActor_IsAutoAnim(app->actors[i]) != TRUE) {
            ClActor_SetAnimSpeed(app->actors[i], FX32_ONE);
        }
    }
    ClActSys_Draw();
}

void Box2Obj_SetAnim(Box2AppWork *app, u32 id, u32 anim) {
    ClActor_SetAnimTime(app->actors[id], 0);
    ClActor_SetSequence(app->actors[id], anim);
}

void Box2Obj_SetAnimAndPlay(Box2AppWork *app, u32 id, u32 anim) {
    Box2Obj_SetAnim(app, id, anim);
    ClActor_SetAutoAnim(app->actors[id], TRUE);
}

BOOL Box2Obj_IsAnimActive(Box2AppWork *app, u32 id) {
    return ClActor_IsAnimActive(app->actors[id]);
}

void Box2Obj_SetVisible(Box2AppWork *app, u32 id, BOOL visible) {
    if (app->actors[id] != NULL) {
        ClActor_SetVisible(app->actors[id], visible);
    }
}

BOOL Box2Obj_IsVisible(Box2AppWork *app, u32 id) {
    return ClActor_IsVisible(app->actors[id]);
}

static void SetBgPriority(Box2AppWork *app, u32 id, u32 priority) {
    ClActor_SetBgPriority(app->actors[id], (u8)priority);
}

static void SetSoftPriority(Box2AppWork *app, u32 id, u32 priority) {
    ClActor_SetPriority(app->actors[id], priority);
}

static void SetBlend(Box2AppWork *app, u32 id, BOOL blend) {
    if (blend == TRUE) {
        ClActor_SetObjMode(app->actors[id], GX_OAM_MODE_XLU);
    } else {
        ClActor_SetObjMode(app->actors[id], GX_OAM_MODE_NORMAL);
    }
}

void Box2Obj_SetPos(Box2AppWork *app, u32 id, s16 x, s16 y, u16 surface) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    ClActor_SetSurfacePos(app->actors[id], &pos, surface);
}

void Box2Obj_GetPos(Box2AppWork *app, u32 id, s16 *x, s16 *y, u16 surface) {
    ClActorPos pos;

    ClActor_GetSurfacePos(app->actors[id], &pos, surface);
    *x = pos.x;
    *y = pos.y;
}

static void SetPalette(Box2AppWork *app, u32 id, u32 palette) {
    ClActor_SetPalette(app->actors[id], (u8)palette, 1);
}

static void LoadBoxResources(Box2AppWork *app) {
    ArcTool *arc;
    u32 i;
    u32 j;

    arc = GFL_ArcSysCreateFileHandle(ARCID_BOX2, HEAPID_BOX2_APP);
    for (i = 0; i < 6; i++) {
        app->chrRes[116 + i] = ClActSys_LoadChars(arc, 71, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    }
    app->palRes[5] = ClActSys_LoadPltt(arc, 72, CLACT_VRAM_MAIN, 0, HEAPID_BOX2_APP);
    app->cellRes[5] = ClActSys_LoadCellAnims(arc, 73, 74, HEAPID_BOX2_APP);
    for (j = 0; j < 96; j++) {
        app->chrRes[j] = ClActSys_LoadChars(arc, 75, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    }
    app->chrRes[122] = ClActSys_LoadChars(arc, 67, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->palRes[6] = ClActSys_LoadPltt(arc, 70, CLACT_VRAM_MAIN, 0xe0, HEAPID_BOX2_APP);
    app->cellRes[6] = ClActSys_LoadCellAnims(arc, 68, 69, HEAPID_BOX2_APP);
    app->chrRes[123] = ClActSys_LoadChars(arc, 64, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[7] = ClActSys_LoadCellAnims(arc, 65, 66, HEAPID_BOX2_APP);
    app->chrRes[98] = ClActSys_LoadChars(arc, 76, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[3] = ClActSys_LoadCellAnims(arc, 77, 78, HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void LoadUiResources(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_BOX2_APP);

    app->chrRes[124] =
        ClActSys_LoadChars(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->palRes[7] =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_MAIN, 0x120, HEAPID_BOX2_APP);
    app->cellRes[8] = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTouchBarCellFile(2),
                                             AppMenuCommon_GetTouchBarAnimFile(2), HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void LoadPokeIconResources(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_BOX2_APP);

    app->palRes[0] = ClActSys_LoadPlttEx(arc, PokeIcon_GetAltPaletteFile(), CLACT_VRAM_MAIN, 0x60, HEAPID_BOX2_APP);
    app->cellRes[0] =
        ClActSys_LoadCellAnims(arc, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void CreatePokeIconActors(Box2AppWork *app) {
    Box2ActorData data;
    Box2ActorData base = sPokeIconData;
    u32 i;

    for (i = 0; i < NELEMS(app->pokeIconId); i++) {
        data = base;

        data.setup.x += (s16)(i % 6 * 24);
        data.setup.y += (s16)(i / 6 * 24);
        data.setup.priority = 146 - i * 2;
        data.chr = i;
        app->actors[BOX2_ACTOR_POKEICON + i] = CreateActor(app, &data);
        app->pokeIconId[i] = BOX2_ACTOR_POKEICON + i;
        Box2Obj_SetVisible(app, BOX2_ACTOR_POKEICON + i, FALSE);
    }
}

static void *ReadPokeIconChar(Box2AppWork *app, BoxPkm *pkm, NNSG2dCharacterData **chr) {
    return GFL_G2DIOReadOBJNCGRArc(app->pokeIconArc, PokeIcon_GetFileOfPkm(pkm), FALSE, chr, HEAPID_BOX2_APP);
}

void Box2Obj_LoadTrayPokeIcons(Box2SysWork *syswk, u32 tray) {
    BoxPkm *pkm;
    void *buf;
    NNSG2dCharacterData *chr;
    u32 i;

    for (i = 0; i < BOX2_TRAY_POKE_MAX; i++) {
        if (BoxSaveAccessor_GetPkmParam(syswk->param->boxes, tray, i, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            syswk->app->pokeIconExist[i] = FALSE;
        } else if (syswk->param->mode != 3 && syswk->tray == syswk->getTray && IsInPickedRange(syswk, i) == TRUE) {
            syswk->app->pokeIconExist[i] = FALSE;
        } else {
            pkm = Box2Main_GetBoxPkm(syswk, tray, i);
            buf = ReadPokeIconChar(syswk->app, pkm, &chr);
            MIi_CpuCopy32(chr->pRawData, syswk->app->pokeIconChar[i], 0x200);
            GFL_HeapFree(buf);
            syswk->app->pokeIconPal[i] = GetPokeIconPalette(syswk, pkm);
            syswk->app->pokeIconExist[i] = TRUE;
        }
    }
}

static BOOL MatchesSearch(Box2SearchParam *search, BoxPkm *pkm) {
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

static u32 GetPokeIconPalette(Box2SysWork *syswk, BoxPkm *pkm) {
    if (MatchesSearch(&syswk->search, pkm)) {
        return PokeIcon_GetPaletteOfPkm(pkm);
    }
    return 3;
}

static void TransferIconChar(Box2AppWork *app, void *chr, u32 id, u32 palette) {
    ClActSys_WriteCharData(ClActor_GetCharIdx(app->actors[id]), chr, 0x200, 0, CLACT_VRAM_MAIN);
    ClActor_SetPalette(app->actors[id], (u8)palette, 1);
}

static void SetPokeIcon(Box2SysWork *syswk, BoxPkm *pkm, u32 id) {
    NNSG2dCharacterData *chr;
    void *buf = ReadPokeIconChar(syswk->app, pkm, &chr);

    TransferIconChar(syswk->app, chr->pRawData, id, GetPokeIconPalette(syswk, pkm));
    GFL_HeapFree(buf);
}

void Box2Obj_SetTrayPokeIcon(Box2SysWork *syswk, u32 tray, u32 pos, u32 id) {
    Box2AppWork *app = syswk->app;

    Box2Obj_SetVisible(app, id, FALSE);
    if (Box2Main_GetPokeParam(syswk, pos, tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
        SetPokeIcon(syswk, Box2Main_GetBoxPkm(syswk, tray, pos), id);
        Box2Obj_SetVisible(app, id, TRUE);
    }
}

void Box2Obj_SetTrayPokeIcons(Box2SysWork *syswk) {
    u32 i;

    for (i = 0; i < BOX2_TRAY_POKE_MAX; i++) {
        Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, i, BOX2_ACTOR_POKEICON + i);
    }
}

void Box2Obj_SetPartyPokeIcons(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < count; i++) {
        SetPokeIcon(syswk, PokeParty_ToBoxPkm(PokeParty_GetPkm(syswk->param->party, i)),
                    syswk->app->pokeIconId[BOX2_PARTY_POS + i]);
        Box2Obj_SetVisible(syswk->app, syswk->app->pokeIconId[BOX2_PARTY_POS + i], FALSE);
    }
}

void Box2Obj_GetIconPos(u32 pos, s16 *x, s16 *y, u32 mode) {
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

void Box2Obj_PutPartyIconsLeftBelow(Box2SysWork *syswk, BOOL hideGet) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < 6; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        Box2Obj_SetPos(syswk->app, id, sPartyPokePos[i].x, sPartyPokePos[i].y, 0);
        Box2Obj_SetIconPriority(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count && (hideGet == FALSE || syswk->pos != BOX2_PARTY_POS + i)) {
            Box2Obj_SetVisible(syswk->app, id, TRUE);
        } else {
            Box2Obj_SetVisible(syswk->app, id, FALSE);
        }
    }
}

void Box2Obj_HidePartyRangeIcons(Box2SysWork *syswk) {
    u32 i;
    u32 pos;
    u32 x;

    if (syswk->pos >= BOX2_PARTY_POS) {
        pos = syswk->pos - BOX2_PARTY_POS;
        x = pos & 1;
        for (i = 0; i < 6; i++) {
            if ((i & 1) >= x && (i & 1) < x + syswk->app->rangeWidth && (i >> 1) >= (pos >> 1) &&
                (i >> 1) < (pos >> 1) + syswk->app->rangeHeight) {
                Box2Obj_SetVisible(syswk->app, syswk->app->pokeIconId[BOX2_PARTY_POS + i], FALSE);
            }
        }
    }
}

void Box2Obj_PutPartyIconsRightBelow(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < 6; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        Box2Obj_SetPos(syswk->app, id, sPartyPokePos[i].x + 152, sPartyPokePos[i].y, 0);
        Box2Obj_SetIconPriority(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count) {
            Box2Obj_SetVisible(syswk->app, id, TRUE);
        } else {
            Box2Obj_SetVisible(syswk->app, id, FALSE);
        }
    }
}

void Box2Obj_PutPartyIconsLeft(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < 6; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        Box2Obj_SetPos(syswk->app, id, sPartyPokePos[i].x, sPartyPokePos[i].y - 144, 0);
        Box2Obj_SetIconPriority(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count) {
            Box2Obj_SetVisible(syswk->app, id, TRUE);
        } else {
            Box2Obj_SetVisible(syswk->app, id, FALSE);
        }
    }
}

void Box2Obj_PutPartyIconsRight(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < 6; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        Box2Obj_SetPos(syswk->app, id, sPartyPokePos[i].x + 152, sPartyPokePos[i].y - 144, 0);
        Box2Obj_SetIconPriority(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count) {
            Box2Obj_SetVisible(syswk->app, id, TRUE);
        } else {
            Box2Obj_SetVisible(syswk->app, id, FALSE);
        }
    }
}

void Box2Obj_SetIconPriority(Box2AppWork *app, u32 iconPos, BOOL put) {
    u32 id = app->pokeIconId[iconPos];

    if (put == FALSE) {
        SetBgPriority(app, id, 0);
        SetSoftPriority(app, id, 20);
    } else {
        if (iconPos < BOX2_TRAY_POKE_MAX || iconPos == BOX2_BOXLIST_POS) {
            SetBgPriority(app, id, 3);
        } else {
            SetBgPriority(app, id, 1);
        }
        SetSoftPriority(app, id, 146 - iconPos * 2);
    }
}

void Box2Obj_SetIconPriorityAt(Box2AppWork *app, u32 iconPos, u32 pos, BOOL put) {
    u32 id = app->pokeIconId[iconPos];

    if (put == FALSE) {
        SetBgPriority(app, id, 0);
        SetSoftPriority(app, id, 20);
    } else {
        if (pos < BOX2_TRAY_POKE_MAX) {
            SetBgPriority(app, id, 3);
        } else {
            SetBgPriority(app, id, 1);
        }
        SetSoftPriority(app, id, 146 - pos * 2);
    }
}

void Box2Obj_ScrollTrayIcons(Box2SysWork *syswk, s32 mv) {
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
        Box2Obj_GetPos(syswk->app, id, &x, &y, 0);
        x = x + mv;
        if (x == end) {
            x = start;
            TransferIconChar(syswk->app, syswk->app->pokeIconChar[i], id, syswk->app->pokeIconPal[i]);
            Box2Obj_SetVisible(syswk->app, id, syswk->app->pokeIconExist[i]);
            if (syswk->app->unkA55A == 0 && (syswk->param->mode == 3 || syswk->param->mode == 5)) {
                SetIconBlendChecked(syswk, i);
            }
        }
        Box2Obj_SetPos(syswk->app, id, x, y, 0);
    }
}

void Box2Obj_MovePartyIconsWithFrame(Box2SysWork *syswk) {
    s8 x;
    s8 y;
    s16 px;
    s16 py;
    u16 i;

    BGWinFrame_GetPos(syswk->app->bgWinFrame, 8, &x, &y);
    py = y * 8;
    px = x * 8;
    for (i = 0; i < 6; i++) {
        Box2Obj_SetPos(syswk->app, syswk->app->pokeIconId[BOX2_PARTY_POS + i], px + sPartyFramePokePos[i][0],
                       py + sPartyFramePokePos[i][1], 0);
    }
}

void Box2PokeFree_Start(Box2PokeFreeWork *wk) {
    ClActorScale scale;
    ClActorPos center;

    wk->scale = 4096.0f;
    wk->scaleCnt = 0;
    ClActor_SetAffineMode(wk->cap, 1);
    scale.x = wk->scale;
    scale.y = wk->scale;
    ClActor_SetScale(wk->cap, &scale);
    center.x = 0;
    center.y = 8;
    ClActor_SetAffineCenter(wk->cap, &center);
}

BOOL Box2PokeFree_Shrink(Box2PokeFreeWork *wk) {
    ClActorScale scale;

    wk->scaleCnt++;
    wk->scale -= 102.0f;
    if (wk->scaleCnt == 40) {
        return FALSE;
    }
    scale.x = wk->scale;
    scale.y = wk->scale;
    ClActor_SetScale(wk->cap, &scale);
    return TRUE;
}

BOOL Box2PokeFree_Grow(Box2PokeFreeWork *wk) {
    ClActorScale scale;

    wk->scaleCnt -= 2;
    wk->scale += 102.0f;
    wk->scale += 102.0f;
    if (wk->scaleCnt == 0) {
        return FALSE;
    }
    scale.x = wk->scale;
    scale.y = wk->scale;
    ClActor_SetScale(wk->cap, &scale);
    return TRUE;
}

void Box2PokeFree_End(Box2PokeFreeWork *wk) {
    ClActor_SetVisible(wk->cap, FALSE);
    Box2PokeFree_ResetActor(wk);
}

void Box2PokeFree_ResetActor(Box2PokeFreeWork *wk) {
    ClActorScale scale;
    ClActorPos center;

    center.x = 0;
    center.y = 0;
    ClActor_SetAffineCenter(wk->cap, &center);
    scale.x = FX32_ONE;
    scale.y = FX32_ONE;
    ClActor_SetScale(wk->cap, &scale);
    ClActor_SetAffineMode(wk->cap, 0);
}

static void SetIconsBlend(Box2SysWork *syswk, BOOL blend, u32 start, u32 end, BOOL check) {
    u32 i;

    if (check == TRUE) {
        if (syswk->param->mode == BOX2_MODE_DREAM_WORLD) {
            for (i = start; i < end; i++) {
                if (Box2Main_IsSpeciesFlagged(syswk, i) == TRUE &&
                    Box2Main_GetPokeParam(syswk, i, syswk->tray, PKM_PARAM_IS_EGG, NULL) == 0) {
                    SetBlend(syswk->app, syswk->app->pokeIconId[i], FALSE);
                } else {
                    SetBlend(syswk->app, syswk->app->pokeIconId[i], TRUE);
                }
            }
        } else {
            for (i = start; i < end; i++) {
                if (Box2Main_GetPokeParam(syswk, i, syswk->tray, PKM_PARAM_ITEM, NULL) == 0) {
                    SetBlend(syswk->app, syswk->app->pokeIconId[i], TRUE);
                } else {
                    SetBlend(syswk->app, syswk->app->pokeIconId[i], FALSE);
                }
            }
        }
    } else {
        for (i = start; i < end; i++) {
            SetBlend(syswk->app, syswk->app->pokeIconId[i], blend);
        }
    }
}

void Box2Obj_SetIconsBlend(Box2SysWork *syswk, u32 flags, BOOL blend) {
    BOOL check;

    if (flags & 0x80) {
        check = TRUE;
    } else {
        check = FALSE;
    }
    if (flags & 1) {
        SetIconsBlend(syswk, blend, 0, BOX2_TRAY_POKE_MAX, check);
    }
    if (flags & 2) {
        SetIconsBlend(syswk, blend, BOX2_TRAY_POKE_MAX, NELEMS(syswk->app->pokeIconId), check);
    }
}

void Box2Obj_SetIconBlend(Box2AppWork *app, u32 iconPos, BOOL blend) {
    SetBlend(app, app->pokeIconId[iconPos], blend);
}

static void SetIconBlendChecked(Box2SysWork *syswk, u32 pos) {
    SetIconsBlend(syswk, TRUE, pos, pos + 1, TRUE);
}

void Box2Obj_PickUpRangeIcons(Box2SysWork *syswk, u32 pos, int width, int height) {
    int i;
    int j;
    Box2AppWork *app;
    unsigned int y;
    u32 x;
    unsigned int rowWidth;
    s16 px;
    s16 py;
    u8 tmp;

    app = syswk->app;
    rowWidth = Box2Main_GetRowWidth(syswk, pos);
    x = pos % rowWidth;
    y = pos / rowWidth;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            Box2Obj_GetPos(app, app->pokeIconId[x + (y + j) * rowWidth + i], &px, &py, 0);
            tmp = app->pokeIconId[BOX2_BOXLIST_POS + j * width + i];
            app->pokeIconId[BOX2_BOXLIST_POS + j * width + i] = app->pokeIconId[x + (y + j) * rowWidth + i];
            app->pokeIconId[x + (y + j) * rowWidth + i] = tmp;
            Box2Obj_SetPos(app, app->pokeIconId[x + (y + j) * rowWidth + i], px, py, 0);
            Box2Obj_SetIconPriority(app, x + (y + j) * rowWidth + i, TRUE);
        }
    }
    app->rangeWidth = width;
    app->rangeHeight = height;
}

void Box2Obj_PutDownRangeIcons(Box2AppWork *app, u32 pos, int width, int height) {
    s16 px;
    s16 py;
    u32 x;
    unsigned int y;
    int i;
    int j;
    u8 tmp;

    Box2Obj_GetPos(app, app->pokeIconId[BOX2_BOXLIST_POS], &px, &py, 0);
    x = pos % 6;
    y = pos / 6;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            Box2Obj_GetPos(app, app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i], &px, &py, 0);
            tmp = app->pokeIconId[x + (y + j) * 6 + i];
            app->pokeIconId[x + (y + j) * 6 + i] = app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i];
            app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i] = tmp;

            Box2Obj_SetPos(app, app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i], px, py, 0);
        }
    }
}

void Box2Obj_MoveHeldIcons(Box2SysWork *syswk) {
    s16 x;
    s16 y;
    int j;
    int i;

    if (syswk->unk18 != 0) {
        Box2Obj_GetPos(syswk->app, BOX2_ACTOR_CURSOR, &x, &y, 0);
        if (syswk->param->mode == 3) {
            y += 8;
            Box2Obj_SetPos(syswk->app, BOX2_ACTOR_ITEM_ICON, x, y, 0);
        } else {
            y += 4;
            for (j = 0; j < syswk->app->rangeHeight; j++) {
                for (i = 0; i < syswk->app->rangeWidth; i++) {
                    Box2Obj_SetPos(syswk->app,
                                   syswk->app->pokeIconId[BOX2_BOXLIST_POS + j * syswk->app->rangeWidth + i],
                                   x + i * 24, y + j * 24, 0);
                }
            }
            Box2Obj_PutRangeCount(syswk->app, x, y + 9);
        }
        if (syswk->param->mode == 3) {
            Box2Obj_MoveItemOutline(syswk->app);
        } else {
            if (syswk->app->actors[121] == NULL) {
                Box2Obj_PutPokeOutline(syswk);
            }
            if (syswk->moveMode != 2) {
                Box2Obj_MoveOutlineToIcon(syswk->app, BOX2_BOXLIST_POS);
            } else {
                Box2Obj_ShowPokeOutline(syswk, 0);
            }
        }
    }
}

void Box2Obj_UpdatePokeIconPalette(Box2SysWork *syswk, u32 tray, u32 pos) {
    u32 palette = GetPokeIconPalette(syswk, Box2Main_GetBoxPkm(syswk, tray, pos));

    if (pos < BOX2_TRAY_POKE_MAX) {
        syswk->app->pokeIconPal[pos] = palette;
    }
    SetPalette(syswk->app, syswk->app->pokeIconId[pos], palette);
}

static void ClearPokeGraResources(Box2AppWork *app) {
    app->chrRes[96] = BOX2_RES_NONE;
    app->palRes[1] = BOX2_RES_NONE;
    app->cellRes[1] = BOX2_RES_NONE;
    app->chrRes[97] = BOX2_RES_NONE;
    app->palRes[2] = BOX2_RES_NONE;
    app->cellRes[2] = BOX2_RES_NONE;
}

void Box2Obj_ShowPokeGra(Box2SysWork *syswk, Box2PokeInfo *info, u32 id) {
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
        ClActor_Delete(syswk->app->actors[id]);
        ClActSys_FreeChars(*chr);
        ClActSys_FreePltt(*pal);
        ClActSys_FreeCellAnims(*cell);
    }
    arc = MakePokeGraArcHandle(HEAPID_BOX2_APP);
    encrypted = PML_PkmDecrypt(info->pkm);
    *chr = PokeGra_LoadClActCharsByBoxData(arc, info->pkm, POKEGRA_DIR_FRONT, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    *pal = PokeGra_LoadClActPaletteByBoxData(arc, info->pkm, POKEGRA_DIR_FRONT, CLACT_VRAM_SUB, palOffset,
                                             HEAPID_BOX2_APP);
    *cell = PokeGra_LoadClActCellAnimsByBoxData(info->pkm, POKEGRA_DIR_FRONT, 2, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    PML_PkmReEncrypt(info->pkm, encrypted);
    GFL_ArcToolFree(arc);
    syswk->app->actors[id] = CreateActor(syswk->app, &sActorData[id]);
    Box2Obj_SetVisible(syswk->app, id, TRUE);
    if (syswk->app->actors[other] != NULL) {
        Box2Obj_SetVisible(syswk->app, other, FALSE);
    }
    syswk->app->pokegraSwap ^= 1;
}

static void LoadInfoResources(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_BOX2_APP | HEAPID_TAIL_BIT);
    u32 i;

    for (i = 0; i < 17; i++) {
        app->chrRes[99 + i] =
            ClActSys_LoadChars(arc, AppMenuCommon_GetTypeIconCharFile(i), FALSE, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    }
    app->cellRes[4] = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTypeIconCellFile(2),
                                             AppMenuCommon_GetTypeIconAnimFile(2), HEAPID_BOX2_APP);
    app->palRes[4] = ClActSys_LoadPltt(arc, AppMenuCommon_GetTypeIconPaletteFile(), CLACT_VRAM_SUB, 0, HEAPID_BOX2_APP);
    app->chrRes[126] =
        ClActSys_LoadChars(arc, AppMenuCommon_GetMarkingIconCharFile(2), FALSE, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    app->cellRes[10] = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetMarkingIconCellFile(2),
                                              AppMenuCommon_GetMarkingIconAnimFile(2), HEAPID_BOX2_APP);
    app->palRes[9] =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetMarkingIconPaletteFile(), CLACT_VRAM_SUB, 0xa0, HEAPID_BOX2_APP);
    app->chrRes[127] =
        ClActSys_LoadChars(arc, AppMenuCommon_GetPokerusIconCharFile(2), FALSE, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    app->cellRes[11] = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetPokerusIconCellFile(2),
                                              AppMenuCommon_GetPokerusIconAnimFile(2), HEAPID_BOX2_APP);
    app->palRes[10] =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetPokerusIconPaletteFile(), CLACT_VRAM_SUB, 0xc0, HEAPID_BOX2_APP);
    app->chrRes[125] =
        ClActSys_LoadChars(arc, AppMenuCommon_GetMarkingIconCharFile(2), FALSE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[9] = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetMarkingIconCellFile(2),
                                             AppMenuCommon_GetMarkingIconAnimFile(2), HEAPID_BOX2_APP);
    app->palRes[8] =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetMarkingIconPaletteFile(), CLACT_VRAM_MAIN, 0x1c0, HEAPID_BOX2_APP);
    app->chrRes[128] = ClActSys_LoadChars(arc, 175, FALSE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[12] = ClActSys_LoadCellAnims(arc, 174, 173, HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void CreateTypeIconActors(Box2AppWork *app) {
    Box2ActorData data = sTypeIconData;
    u32 i;

    for (i = 0; i < 17; i++) {
        data.chr = 99 + i;
        app->actors[32 + i] = CreateActor(app, &data);
        ClActor_SetPalette(app->actors[32 + i], AppMenuCommon_GetTypeIconPaletteIndex(i), 1);
        Box2Obj_SetVisible(app, 32 + i, FALSE);
    }
}

static void ShowTypeIcons(Box2AppWork *app, u32 type1, u32 type2) {
    u32 id = BOX2_ACTOR_TYPE_ICON + type1;

    Box2Obj_SetVisible(app, id, TRUE);
    Box2Obj_SetPos(app, id, 88, 56, 1);
    if (type2 != 0 && type1 != type2) {
        id = BOX2_ACTOR_TYPE_ICON + type2;
        Box2Obj_SetVisible(app, id, TRUE);
        Box2Obj_SetPos(app, id, 122, 56, 1);
    }
}

void Box2Obj_PutTypeIcons(Box2AppWork *app, Box2PokeInfo *info) {
    u32 i;

    for (i = BOX2_ACTOR_TYPE_ICON; i < BOX2_ACTOR_TYPE_ICON + 17; i++) {
        Box2Obj_SetVisible(app, i, FALSE);
    }
    if (info->egg == 0) {
        ShowTypeIcons(app, info->type1, info->type2);
    }
}

static void LoadItemIconPalette(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMGRA, HEAPID_BOX2_APP);

    app->palRes[3] = ClActSys_LoadPltt(arc, GetItemGraphicsDatID(0, 2), CLACT_VRAM_MAIN, 0x1a0, HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

void Box2Obj_SetItemIcon(Box2AppWork *app, u16 item) {
    NNSG2dCharacterData *chr;
    NNSG2dPaletteData *pal;
    void *buf;

    buf = GFL_G2DIOReadOBJNCGR(PML_ItemGetIconArcID(), GetItemGraphicsDatID(item, 1), FALSE, &chr, HEAPID_BOX2_APP);
    ClActSys_ReplaceCharData(ClActor_GetCharIdx(app->actors[BOX2_ACTOR_ITEM_ICON]), chr);
    GFL_HeapFree(buf);
    buf = GFL_G2DIOReadNCLR(PML_ItemGetIconArcID(), GetItemGraphicsDatID(item, 2), &pal, HEAPID_BOX2_APP);
    ClActSys_ReplacePlttData(ClActor_GetPlttIdx(app->actors[BOX2_ACTOR_ITEM_ICON]), pal, 1);
    GFL_HeapFree(buf);
}

void Box2Obj_SetItemIconAffine(Box2AppWork *app, BOOL affine) {
    ClActorPos center;

    if (affine == TRUE) {
        ClActor_SetAffineMode(app->actors[BOX2_ACTOR_ITEM_ICON], 1);
        center.x = 12;
        center.y = 12;
        ClActor_SetAffineCenter(app->actors[BOX2_ACTOR_ITEM_ICON], &center);
    } else {
        center.x = 0;
        center.y = 0;
        ClActor_SetAffineCenter(app->actors[BOX2_ACTOR_ITEM_ICON], &center);
        ClActor_SetAffineMode(app->actors[BOX2_ACTOR_ITEM_ICON], 0);
    }
}

void Box2Obj_SetItemIconPos(Box2AppWork *app, s16 x, s16 y) {
    Box2Obj_SetPos(app, BOX2_ACTOR_ITEM_ICON, x, y, 0);
}

void Box2Obj_PutItemIconAtIcon(Box2AppWork *app, u32 pos, u32 mode) {
    s16 x;
    s16 y;

    Box2Obj_GetIconPos(pos, &x, &y, mode);
    Box2Obj_SetItemIconPos(app, x + 8, y + 8);
}

void Box2Obj_PutItemIconAtCursor(Box2AppWork *app, u32 pos, u32 mode) {
    s16 x;
    s16 y;

    Box2Obj_GetIconPos(pos, &x, &y, mode);
    Box2Obj_SetItemIconPos(app, x, y + 4);
}

void Box2Obj_InitItemOutline(Box2AppWork *app) {
    Box2ActorData data;
    s16 x;
    s16 y;
    u32 i;

    if (app->getItem != 0) {
        Box2Obj_GetPos(app, BOX2_ACTOR_ITEM_ICON, &x, &y, 0);
        if (app->actors[BOX2_ACTOR_ITEM_OUTLINE] == NULL) {
            data = sActorData[BOX2_ACTOR_ITEM_ICON];
            data.setup.priority = ClActor_GetPriority(app->actors[BOX2_ACTOR_ITEM_ICON]) + 1;
            data.setup.bgPriority = ClActor_GetBgPriority(app->actors[BOX2_ACTOR_ITEM_ICON]);
            data.chr = 98;
            data.pal = 6;
            data.palette = 1;
            for (i = 0; i < 8; i++) {
                data.setup.x = x + sItemOutlineX[i];
                data.setup.y = y + sItemOutlineY[i];
                app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] = CreateActor(app, &data);
                ClActor_SetPalette(app->actors[BOX2_ACTOR_ITEM_OUTLINE + i], (u8)data.palette, 1);
                Box2Obj_SetVisible(app, BOX2_ACTOR_ITEM_OUTLINE + i, FALSE);
            }
        } else {
            u32 priority = ClActor_GetPriority(app->actors[BOX2_ACTOR_ITEM_ICON]);
            u32 bgPriority = ClActor_GetBgPriority(app->actors[BOX2_ACTOR_ITEM_ICON]);

            for (i = 0; i < 8; i++) {
                Box2Obj_SetPos(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i], 0);
                SetSoftPriority(app, BOX2_ACTOR_ITEM_OUTLINE + i, priority + 1);
                SetBgPriority(app, BOX2_ACTOR_ITEM_OUTLINE + i, bgPriority);
                Box2Obj_SetVisible(app, BOX2_ACTOR_ITEM_OUTLINE + i, FALSE);
            }
        }
    }
}

void Box2Obj_ShowItemOutline(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < 8; i++) {
        Box2Obj_SetVisible(app, BOX2_ACTOR_ITEM_OUTLINE + i, TRUE);
    }
}

void Box2Obj_MoveItemOutline(Box2AppWork *app) {
    s16 x;
    s16 y;
    u32 i;

    if (app->actors[BOX2_ACTOR_ITEM_OUTLINE] != NULL) {
        Box2Obj_GetPos(app, BOX2_ACTOR_ITEM_ICON, &x, &y, 0);
        for (i = 0; i < 8; i++) {
            Box2Obj_SetPos(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i], 0);
        }
    }
}

static void CreateFixedActors(Box2SysWork *syswk) {
    Box2AppWork *app = syswk->app;
    u32 i;

    app->actors[0] = CreateActor(app, &sActorData[0]);
    app->actors[1] = CreateActor(app, &sActorData[1]);
    app->actors[2] = CreateActor(app, &sActorData[2]);
    app->actors[3] = CreateActor(app, &sActorData[3]);
    app->actors[4] = CreateActor(app, &sActorData[4]);
    app->actors[5] = CreateActor(app, &sActorData[5]);
    app->actors[12] = CreateActor(app, &sActorData[12]);
    app->actors[6] = CreateActor(app, &sActorData[6]);
    app->actors[7] = CreateActor(app, &sActorData[7]);
    app->actors[8] = CreateActor(app, &sActorData[8]);
    app->actors[9] = CreateActor(app, &sActorData[9]);
    app->actors[10] = CreateActor(app, &sActorData[10]);
    app->actors[11] = CreateActor(app, &sActorData[11]);
    for (i = 0; i < 6; i++) {
        app->actors[15 + i] = CreateActor(app, &sActorData[15 + i]);
        app->actors[21 + i] = CreateActor(app, &sActorData[21 + i]);
    }
    app->actors[27] = CreateActor(app, &sActorData[27]);
    app->actors[28] = CreateActor(app, &sActorData[28]);
    app->actors[29] = CreateActor(app, &sActorData[29]);
    app->actors[30] = CreateActor(app, &sActorData[30]);
    app->actors[31] = CreateActor(app, &sActorData[31]);
    Box2Obj_SetVisible(app, 5, FALSE);
    Box2Obj_SetVisible(app, BOX2_ACTOR_ITEM_ICON, FALSE);
    Box2Obj_SetVisible(app, 27, FALSE);
    Box2Obj_SetVisible(app, 28, FALSE);
    Box2Obj_SetVisible(app, 29, FALSE);
    Box2Obj_SetVisible(app, 31, FALSE);
    for (i = 0; i < 16; i++) {
        app->actors[BOX2_ACTOR_RANGE_DOT + i] = CreateActor(app, &sRangeDotData);
        Box2Obj_SetVisible(app, BOX2_ACTOR_RANGE_DOT + i, FALSE);
    }
    Box2Obj_SetToolButtons(syswk, 9, TRUE);
}

void Box2Obj_SetToolButtons(Box2SysWork *syswk, unsigned int set, BOOL visible) {
    int i;

    if (syswk->param->mode != 2 && syswk->param->mode != 4 && syswk->param->mode != 3) {
        for (i = 9; i <= 11; i++) {
            Box2Obj_SetVisible(syswk->app, i, FALSE);
        }
    } else {
        for (i = 0; i < 3; i++) {
            Box2Obj_SetAnim(syswk->app, 9 + i, sButtonAnims[set - 9][i]);
            Box2Obj_SetVisible(syswk->app, 9 + i, visible);
        }
        if (syswk->param->mode == 3) {
            Box2Obj_SetVisible(syswk->app, 11, FALSE);
        }
    }
}

void Box2Obj_SetCursorAnim(Box2SysWork *syswk, u32 anim) {
    if (syswk->moveMode == 1) {
        anim += 4;
    } else if (syswk->moveMode == 2) {
        anim += 9;
    }
    Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_CURSOR, anim);
}

void Box2Obj_ShowCursor(Box2SysWork *syswk, BOOL show) {
    if (show == TRUE) {
        if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
        }
    } else {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
    }
}

void Box2Obj_PutPokeOutline(Box2SysWork *syswk) {
    Box2Obj_PutPokeOutlineAt(syswk, syswk->pos);
}

void Box2Obj_PutPokeOutlineAt(Box2SysWork *syswk, u32 pos) {
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
        Box2Obj_GetPos(app, id, &x, &y, 0);
        if (app->actors[BOX2_ACTOR_ITEM_OUTLINE] == NULL) {
            data = sPokeIconData;
            data.setup.priority = ClActor_GetPriority(app->actors[id]) + 1;
            data.setup.bgPriority = ClActor_GetBgPriority(app->actors[id]);
            data.chr = id - BOX2_ACTOR_POKEICON;
            data.pal = 6;
            data.palette = 1;
            for (i = 0; i < 8; i++) {
                data.setup.x = x + sItemOutlineX[i];
                data.setup.y = y + sItemOutlineY[i];
                app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] = CreateActor(app, &data);
                ClActor_SetPalette(app->actors[BOX2_ACTOR_ITEM_OUTLINE + i], (u8)data.palette, 1);
            }
        } else {
            u32 priority;
            u32 bgPriority;

            ClActor_GetImageProxy(app->actors[id], &proxy);
            priority = ClActor_GetPriority(app->actors[id]);
            bgPriority = ClActor_GetBgPriority(app->actors[id]);
            for (i = 0; i < 8; i++) {
                ClActor_SetImageProxy(app->actors[BOX2_ACTOR_ITEM_OUTLINE + i], &proxy);
                Box2Obj_SetPos(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i], 0);
                SetSoftPriority(app, BOX2_ACTOR_ITEM_OUTLINE + i, priority + 1);
                SetBgPriority(app, BOX2_ACTOR_ITEM_OUTLINE + i, bgPriority);
                Box2Obj_SetVisible(app, BOX2_ACTOR_ITEM_OUTLINE + i, TRUE);
            }
        }
    }
}

void Box2Obj_ShowPokeOutline(Box2SysWork *syswk, BOOL visible) {
    u32 i;
    u32 id;

    for (i = 0; i < 8; i++) {
        if (syswk->app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] != NULL) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, visible);
            if (visible == TRUE && syswk->pos != BOX2_GET_NONE) {
                id = syswk->app->pokeIconId[syswk->pos];
                SetSoftPriority(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i,
                                ClActor_GetPriority(syswk->app->actors[id]) + 1);
                SetBgPriority(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, ClActor_GetBgPriority(syswk->app->actors[id]));
            }
        }
    }
}

void Box2Obj_UpdateOutlinePriority(Box2SysWork *syswk, u32 pos) {
    u32 id = syswk->app->pokeIconId[pos];
    u32 priority = ClActor_GetPriority(syswk->app->actors[id]);
    u32 bgPriority = ClActor_GetBgPriority(syswk->app->actors[id]);
    u32 i;

    for (i = 0; i < 8; i++) {
        if (syswk->app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] != NULL && syswk->pos != BOX2_GET_NONE) {
            SetSoftPriority(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, priority + 1);
            SetBgPriority(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, bgPriority);
        }
    }
}

void Box2Obj_MoveOutlineToIcon(Box2AppWork *app, u32 pos) {
    s16 x;
    s16 y;
    u32 i;

    if (pos != BOX2_GET_NONE) {
        Box2Obj_GetPos(app, app->pokeIconId[pos], &x, &y, 0);
        for (i = 0; i < 8; i++) {
            Box2Obj_SetPos(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i], 0);
        }
    }
}

static void CreateTrayIconActors(Box2AppWork *app) {
    Box2ActorData data;
    Box2ActorData base = sTrayIconData;
    u32 i;

    for (i = 0; i < 6; i++) {
        data = base;
        data.setup.y += (s16)(i * 34);
        data.chr = 116 + i;
        app->actors[BOX2_ACTOR_TRAY_ICON + i] = CreateActor(app, &data);
    }
}

void Box2Obj_ShowScrollArrows(Box2AppWork *app, BOOL visible) {
    Box2Obj_SetVisible(app, 0, visible);
    Box2Obj_SetVisible(app, 1, visible);
}

void Box2Obj_MoveMarkIcons(Box2SysWork *syswk) {
    s8 x;
    s8 y;
    s16 px;
    s16 py;
    u32 i;

    BGWinFrame_GetPos(syswk->app->bgWinFrame, 7, &x, &y);
    py = y * 8 + 8;
    px = x * 8 + 23;
    for (i = 0; i < 6; i++) {
        Box2Obj_SetPos(syswk->app, 15 + i, px + (i % 2) * 32, py + (i / 2) * 24, 0);
    }
}

void Box2Obj_ResetBoxList(Box2SysWork *syswk) {
    Box2Obj_MakeTrayIconChar(syswk, syswk->tray);
    PlaceTrayIcons(syswk);
}

void Box2Obj_ScrollBoxListX(Box2SysWork *syswk, s32 mv) {
    s16 x;
    s16 y;
    u32 i;

    Box2Obj_GetPos(syswk->app, 2, &x, &y, 0);
    Box2Obj_SetPos(syswk->app, 2, x - mv, y, 0);
    for (i = 0; i < 6; i++) {
        Box2Obj_GetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, &x, &y, 0);
        Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x - mv, y, 0);
        BmpOam_ActorGetPos(syswk->app->fontOam[1 + i].oam, &x, &y);
        BmpOam_ActorSetPos(syswk->app->fontOam[1 + i].oam, x - mv, y);
    }
}

void Box2Obj_HideTrayHighlight(Box2AppWork *app) {
    Box2Obj_SetPos(app, 2, 300, 34, 0);
}

static void PlaceTrayIcons(Box2SysWork *syswk) {
    Box2ActorData data;
    Box2ActorData base = sTrayIconData;
    u32 tray;
    s16 i;

    for (i = 0; i < 6; i++) {
        tray = Box2Main_GetTrayScroll(syswk, i - 1);
        data = base;
        data.setup.y += (s16)(i * 34);
        Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, data.setup.x, data.setup.y, 0);
        SetTrayIconChar(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
        BmpOam_ActorSetPos(syswk->app->fontOam[1 + i].oam, 316, i * 34 + 2);
        Box2Bmp_PutTrayCount(syswk, tray, i + 1);
    }
}

void Box2Obj_GetTrayIconPos(Box2AppWork *app, u32 row, s16 *x, s16 *y) {
    u32 i;

    row = (row + 1) * 34;
    for (i = 0; i < 6; i++) {
        Box2Obj_GetPos(app, BOX2_ACTOR_TRAY_ICON + i, x, y, 0);
        if (row == *y) {
            return;
        }
    }
    *x = 0;
    *y = 0;
}

void Box2Obj_MakeTrayIconChar(Box2SysWork *syswk, u32 tray) {
    NNSG2dCharacterData *chr;
    void *buf = GFL_G2DIOReadOBJNCGR(ARCID_BOX2, 71, TRUE, &chr, HEAPID_BOX2_APP);

    MI_CpuCopy8(chr->pRawData, syswk->app->trayIconChar[tray], 0x400);
    GFL_HeapFree(buf);
    ReplaceTrayIconColor(syswk, syswk->app->trayIconChar[tray], Box2Main_GetWallPaperNumber(syswk, tray), 8, 0x400);
    DrawTrayIconPokes(syswk, tray, syswk->app->trayIconChar[tray]);
}

void Box2Obj_MakeAllTrayIconChars(Box2SysWork *syswk) {
    u32 i;

    for (i = 0; i < BOX2_TRAY_MAX; i++) {
        Box2Obj_MakeTrayIconChar(syswk, i);
    }
}

void Box2Obj_RefreshTrayIcon(Box2SysWork *syswk, u32 tray) {
    s16 i;

    for (i = 0; i < 6; i++) {
        u32 scroll = Box2Main_GetTrayScroll(syswk, i - 1);

        if (scroll == tray) {
            RefreshTrayIconRow(syswk, tray, i);
            return;
        }
    }
}

static void SetTrayIconChar(Box2SysWork *syswk, u32 tray, u32 id) {
    Box2AppWork *app = syswk->app;

    ClActSys_WriteCharData(ClActor_GetCharIdx(app->actors[id]), app->trayIconChar[tray], 0x400, 0, CLACT_VRAM_MAIN);
}

static void RefreshTrayIconRow(Box2SysWork *syswk, u32 tray, u32 row) {
    s16 x;
    s16 y;
    u32 i;

    row *= 34;
    for (i = 0; i < 6; i++) {
        Box2Obj_GetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, &x, &y, 0);
        if (row == y) {
            SetTrayIconChar(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
            Box2Bmp_PutTrayCount(syswk, tray, i + 1);
            return;
        }
    }
}

static void ReplaceTrayIconColor(Box2SysWork *syswk, u8 *chr, u32 wallpaper, u32 color, u32 size) {
    u32 i;

    for (i = 0; i < size; i++) {
        if (chr[i] == color) {
            chr[i] = sTrayIconColors[wallpaper];
        }
    }
}

static void DrawTrayIconPokes(Box2SysWork *syswk, u32 tray, u8 *chr) {
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
                    MIi_CpuClear16(color, &chr[((y & 7) << 3) + ((((y >> 3) << 2) + (px >> 3)) << 6) + (px & 7)], 2);
                }
            }
            PML_PkmReEncrypt(pkm, encrypted);
            px += 2;
        }
        py += 2;
    }
}

void Box2Obj_ScrollBoxListY(Box2SysWork *syswk, s16 mv) {
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
        Box2Obj_GetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, &x, &y, 0);
        ny = y + speed;
        Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x, ny, 0);
        BmpOam_ActorGetPos(syswk->app->fontOam[1 + i].oam, &fx, &fy);
        nfy = fy + speed;
        BmpOam_ActorSetPos(syswk->app->fontOam[1 + i].oam, fx, nfy);
        if (ny < -16) {
            u32 tray = Box2Main_GetTrayScroll(syswk, 4);

            SetTrayIconChar(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
            Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x, ny + 204, 0);
            Box2Bmp_PutTrayCount(syswk, tray, i + 1);
            BmpOam_ActorSetPos(syswk->app->fontOam[1 + i].oam, fx, nfy + 204);
        } else if (ny >= 186) {
            u32 tray = Box2Main_GetTrayScroll(syswk, -1);

            SetTrayIconChar(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
            Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x, ny - 204, 0);
            Box2Bmp_PutTrayCount(syswk, tray, i + 1);
            BmpOam_ActorSetPos(syswk->app->fontOam[1 + i].oam, fx, nfy - 204);
        }
    }
    if (Box2Obj_IsVisible(syswk->app, 2) == TRUE) {
        Box2Obj_GetPos(syswk->app, 2, &x, &y, 0);
        Box2Obj_SetPos(syswk->app, 2, x, y + speed, 0);
    }
}

void Box2Obj_StartHighlightScroll(Box2SysWork *syswk, s32 mv) {
    s16 x;
    s16 y;

    if (mv < 0 && Box2Main_GetTrayScroll(syswk, -1) == syswk->tray) {
        Box2Obj_GetPos(syswk->app, 2, &x, &y, 0);
        Box2Obj_SetPos(syswk->app, 2, x, -34, 0);
        Box2Obj_SetVisible(syswk->app, 2, TRUE);
    } else if (mv > 0 && Box2Main_GetTrayScroll(syswk, 4) == syswk->tray) {
        Box2Obj_GetPos(syswk->app, 2, &x, &y, 0);
        Box2Obj_SetPos(syswk->app, 2, x, 204, 0);
        Box2Obj_SetVisible(syswk->app, 2, TRUE);
    }
}

void Box2Obj_PlaceTrayHighlight(Box2SysWork *syswk) {
    s16 x;
    s16 y;
    u32 i;

    for (i = 0; i < 6; i++) {
        if (Box2Main_GetTrayScroll(syswk, i - 1) == syswk->tray) {
            Box2Obj_GetPos(syswk->app, 2, &x, &y, 0);
            Box2Obj_SetPos(syswk->app, 2, x, i * 34, 0);
            Box2Obj_SetVisible(syswk->app, 2, TRUE);
            return;
        }
    }
    Box2Obj_SetVisible(syswk->app, 2, FALSE);
}

static void PutBoxNameFrame(Box2SysWork *syswk, u32 index) {
    s16 y = sBoxNameY[index];

    Box2Obj_SetPos(syswk->app, 3, 206, y, 0);
    Box2Obj_SetVisible(syswk->app, 3, TRUE);
    BmpOam_ActorSetPos(syswk->app->fontOam[0].oam, 158, y - 8);
    BmpOam_ActorSetDrawEnable(syswk->app->fontOam[0].oam, TRUE);
}

void Box2Obj_ShowBoxNameFrame(Box2SysWork *syswk, u32 index, BOOL show) {
    if (show == TRUE) {
        Box2Bmp_DrawScrolledTrayName(syswk, index);
        PutBoxNameFrame(syswk, index);
    } else {
        Box2Obj_SetVisible(syswk->app, 3, FALSE);
        BmpOam_ActorSetDrawEnable(syswk->app->fontOam[0].oam, FALSE);
    }
}

void Box2Obj_SetButtons(Box2SysWork *syswk, u8 button1, u8 button2, u8 button3, u8 button4) {
    syswk->unk20 = button2;
    syswk->unk21 = button3;
    syswk->unk1F = button1;
    syswk->unk22 = button4;
    if (button1 == 0) {
        Box2Obj_SetVisible(syswk->app, 6, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, 6, 1);
    } else if (button1 == 1) {
        Box2Obj_SetVisible(syswk->app, 6, FALSE);
    } else {
        Box2Obj_SetVisible(syswk->app, 6, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, 6, 15);
    }
    if (button2 == 0) {
        Box2Obj_SetVisible(syswk->app, 7, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, 7, 0);
    } else if (button2 == 1) {
        Box2Obj_SetVisible(syswk->app, 7, FALSE);
    } else {
        Box2Obj_SetVisible(syswk->app, 7, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, 7, 14);
    }
    if (button3 == 0) {
        Box2Obj_SetVisible(syswk->app, 8, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, 8, 0);
    } else if (button3 == 1) {
        Box2Obj_SetVisible(syswk->app, 8, FALSE);
    } else {
        Box2Obj_SetVisible(syswk->app, 8, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, 8, 2);
    }
    if (button4 == 0) {
        Box2Obj_SetVisible(syswk->app, 30, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, 30, 0);
    } else if (button4 == 1) {
        Box2Obj_SetVisible(syswk->app, 30, FALSE);
    } else {
        Box2Obj_SetVisible(syswk->app, 30, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, 30, 2);
    }
}

void Box2Obj_HideButtons(Box2SysWork *syswk) {
    Box2Obj_SetVisible(syswk->app, 6, FALSE);
    Box2Obj_SetVisible(syswk->app, 7, FALSE);
    Box2Obj_SetVisible(syswk->app, 8, FALSE);
    Box2Obj_SetVisible(syswk->app, 30, FALSE);
}

static void CreateFontOams(Box2AppWork *app) {
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
    Box2Obj_SetFontOamVisible(app, 9, FALSE);
}

static void DeleteFontOams(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < NELEMS(app->fontOam); i++) {
        BmpOam_ActorDel(app->fontOam[i].oam);
        GFL_BitmapFree(app->fontOam[i].bitmap);
    }
    BmpOam_Exit(app->bmpOam);
}

void Box2Obj_SetFontOamVisible(Box2AppWork *app, u32 index, BOOL visible) {
    BmpOam_ActorSetDrawEnable(app->fontOam[index].oam, visible);
}

BOOL Box2Obj_IsFontOamVisible(Box2AppWork *app, u32 index) {
    return BmpOam_ActorGetDrawEnable(app->fontOam[index].oam);
}

void Box2Obj_PutFontOamOffscreen(Box2AppWork *app, u32 index, u32 dir) {
    if (dir == 0) {
        BmpOam_ActorSetPos(app->fontOam[index].oam, -148, 20);
    } else {
        BmpOam_ActorSetPos(app->fontOam[index].oam, 220, 20);
    }
}

void Box2Obj_ScrollFontOams(Box2AppWork *app, s32 mv) {
    s16 x;
    s16 y;
    u32 i;

    for (i = 7; i <= 8; i++) {
        BmpOam_ActorGetPos(app->fontOam[i].oam, &x, &y);
        BmpOam_ActorSetPos(app->fontOam[i].oam, x + mv, y);
        if (x + mv == -148 || x + mv == 220) {
            Box2Obj_SetFontOamVisible(app, i, FALSE);
        }
    }
}

static void InitRangeSelect(Box2SysWork *syswk) {
    MI_CpuFill8(&syswk->app->rangeSelect, 0, sizeof(Box2RangeSelect));
    syswk->app->rangeSelect.anmMax = 384;
}

void Box2Obj_SetRangeAnm(Box2SysWork *syswk, u32 anm) {
    syswk->app->rangeSelect.anm = anm;
}

void Box2Obj_UpdateRangeDots(Box2SysWork *syswk) {
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
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_RANGE_DOT + i, rs->anm);
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
        Box2Obj_GetPos(syswk->app, syswk->app->pokeIconId[BOX2_BOXLIST_POS], &x, &y, 0);
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
        Box2Obj_SetPos(syswk->app, BOX2_ACTOR_RANGE_DOT + i, px, py, 0);
    }
}

static void SortPair(s32 *a, s32 *b) {
    s32 tmp;

    if (*a > *b) {
        tmp = *a;
        *a = *b;
        *b = tmp;
    }
}

static void SetRangeRect(Box2SysWork *syswk, s32 left, s32 top, s32 right, s32 bottom) {
    Box2RangeSelect *rs = &syswk->app->rangeSelect;

    SortPair(&left, &right);
    SortPair(&top, &bottom);
    rs->left = left;
    rs->top = top;
    rs->right = right;
    rs->bottom = bottom;
}

static s32 Min4(s32 a, s32 b, s32 c, s32 d) {
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

static s32 Max4(s32 a, s32 b, s32 c, s32 d) {
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

static void GetTrayPokeRect(int pos, Box2Rect *rect) {
    rect->left = pos % 6 * 24 + 10;
    rect->top = pos / 6 * 24 + 42;
    rect->right = rect->left + 26;
    rect->bottom = rect->top + 22;
}

static void GetPartyPokeRect(int pos, Box2Rect *rect) {
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

static void GetPartyPokeRectCompact(int pos, Box2Rect *rect) {
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

void Box2Obj_SetRangeRect(Box2SysWork *syswk, int start, int end, u32 mode) {
    Box2Rect rect1;
    Box2Rect rect2;
    s32 left;
    s32 top;
    s32 right;

    if (start < BOX2_TRAY_POKE_MAX) {
        GetTrayPokeRect(start, &rect1);
        GetTrayPokeRect(end, &rect2);
    } else if (mode == 1) {
        GetPartyPokeRectCompact(start, &rect1);
        GetPartyPokeRectCompact(end, &rect2);
    } else {
        GetPartyPokeRect(start, &rect1);
        GetPartyPokeRect(end, &rect2);
    }
    left = Min4(rect1.left, rect1.right, rect2.left, rect2.right);
    top = Min4(rect1.top, rect1.bottom, rect2.top, rect2.bottom);
    right = Max4(rect1.left, rect1.right, rect2.left, rect2.right);
    SetRangeRect(syswk, left, top, right, Max4(rect1.top, rect1.bottom, rect2.top, rect2.bottom));
}

static void GetTrayRangeCorners(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2) {
    *x1 = start % 6;
    *y1 = start / 6;
    *x2 = end % 6;
    *y2 = end / 6;
    SortPair(x1, x2);
    SortPair(y1, y2);
}

static void GetPartyRangeCorners(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2) {
    *x1 = start % 2;
    *y1 = start / 2;
    *x2 = end % 2;
    *y2 = end / 2;
    SortPair(x1, x2);
    SortPair(y1, y2);
}

void Box2Obj_GetRangeSize(int start, int end, u32 *width, u32 *height) {
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;

    if (start >= BOX2_TRAY_POKE_MAX) {
        GetPartyRangeCorners(start - BOX2_TRAY_POKE_MAX, end - BOX2_TRAY_POKE_MAX, &x1, &y1, &x2, &y2);
    } else {
        GetTrayRangeCorners(start, end, &x1, &y1, &x2, &y2);
    }
    *width = x2 - x1 + 1;
    *height = y2 - y1 + 1;
}

u32 Box2Obj_GetTrayRangeStart(int start, int end) {
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;

    GetTrayRangeCorners(start, end, &x1, &y1, &x2, &y2);
    return x1 + y1 * 6;
}

u32 Box2Obj_GetPartyRangeStart(int start, int end) {
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;

    GetPartyRangeCorners(start - BOX2_TRAY_POKE_MAX, end - BOX2_TRAY_POKE_MAX, &x1, &y1, &x2, &y2);
    return x1 + y1 * 2 + BOX2_TRAY_POKE_MAX;
}

void Box2Obj_ResetRangeIconPriority(Box2AppWork *app, u32 pos, u32 width, u32 height, BOOL put) {
    u32 id;
    u32 i;
    u32 j;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            id = app->pokeIconId[BOX2_BOXLIST_POS + j * width + i];
            if (put == FALSE) {
                SetBgPriority(app, id, 0);
                SetSoftPriority(app, id, 20);
            }
        }
    }
}

static BOOL IsInPickedRange(Box2SysWork *syswk, u32 pos) {
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

void Box2Obj_ShowRangeCount(Box2AppWork *app, BOOL visible) {
    Box2Obj_SetVisible(app, 31, visible);
    Box2Obj_SetFontOamVisible(app, 9, visible);
}

void Box2Obj_PutRangeCount(Box2AppWork *app, s16 x, s16 y) {
    Box2Obj_SetPos(app, 31, x, y, 0);
    BmpOam_ActorSetPos(app->fontOam[9].oam, x - 16, y - 16);
}

void Box2Obj_ShowRangeIcons(Box2SysWork *syswk, BOOL visible) {
    u32 rowWidth;
    u16 j;
    u16 i;

    if (syswk->moveMode == 2) {
        Box2Obj_ShowRangeCount(syswk->app, visible);
        visible ^= 1;
        rowWidth = Box2Main_GetRowWidth(syswk, syswk->app->rangeSelect.startPos);
        for (j = 0; j < syswk->app->rangeHeight; j++) {
            for (i = 0; i < syswk->app->rangeWidth; i++) {
                if (syswk->app->rangeFlags[j * rowWidth + i] != 0) {
                    Box2Obj_SetVisible(
                        syswk->app, syswk->app->pokeIconId[BOX2_BOXLIST_POS + j * syswk->app->rangeWidth + i], visible);
                }
            }
        }
        Box2Obj_SetRangeAnm(syswk, visible);
    }
}
