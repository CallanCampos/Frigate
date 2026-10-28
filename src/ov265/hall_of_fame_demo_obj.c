#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/trainer_classes.h"
#include "demo/hall_of_fame_demo.h"
#include "demo/hall_of_fame_demo_main.h"
#include "demo/hall_of_fame_demo_obj.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/double3Ddisp.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "save/playtime.h"
#include "system/bmp_oam.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/btl_main_2.h"

// The Hall of Fame entry demo's actors, their resources and the texts drawn on sprites (overlay 265). The file's name
// is a guess, after the Hall of Fame PC's hall_of_fame_pc_obj.c

// An actor: its setup, its resources (HALL_OF_FAME_DEMO_CHR_*, HALL_OF_FAME_DEMO_PAL_* and HALL_OF_FAME_DEMO_CEL_*) and
// its surface
typedef struct {
    ClActorSetup setup;
    u32 chr;
    u32 pal;
    u32 cel;
    u16 unk14;
    u16 surface;
} HallOfFameDemoObjSetup;

#define RES_NONE 0xffffffff

// The texts' color
#define FONT_COLOR PRINT_COLOR(15, 2, 0)

static void HallOfFameDemoObj_Animate(HallOfFameDemoWork *wk);
static void HallOfFameDemoObj_InitResources(HallOfFameDemoWork *wk);
static void HallOfFameDemoObj_FreeChars(HallOfFameDemoWork *wk, u32 id);
static void HallOfFameDemoObj_FreePalette(HallOfFameDemoWork *wk, u32 id);
static void HallOfFameDemoObj_FreeCellAnims(HallOfFameDemoWork *wk, u32 id);
static void HallOfFameDemoObj_FreeResources(HallOfFameDemoWork *wk);
static ClActor *HallOfFameDemoObj_Add(HallOfFameDemoWork *wk, const HallOfFameDemoObjSetup *setup);
static void HallOfFameDemoObj_Delete(HallOfFameDemoWork *wk, u32 id);
static void HallOfFameDemoObj_DeleteAll(HallOfFameDemoWork *wk);
static void HallOfFameDemoObj_DeleteFontOam(HallOfFameDemoWork *wk, u32 id);
static void HallOfFameDemoObj_DeleteFontOamAll(HallOfFameDemoWork *wk);
static void HallOfFameDemoObj_PrintFontOam(HallOfFameDemoWork *wk, u32 id, Font *font, StrBuf *str, int x, int y);
static void HallOfFameDemoObj_InitFontOam1st(HallOfFameDemoWork *wk);
static void HallOfFameDemoObj_InitFontOam2nd(HallOfFameDemoWork *wk);
static void HallOfFameDemoObj_LoadResources(HallOfFameDemoWork *wk);

// The palette file of each type, for the plates
static const u32 sHallOfFameDemoTypePalettes[] = {
    3, 9, 11, 16, 14, 18, 5, 12, 19, 10, 20, 13, 8, 17, 15, 7, 6,
};

// The actors. The order they are declared in gives the order the game has them in
static const HallOfFameDemoObjSetup sHallOfFameDemoObjPokeFront = {
    { -48, 144, 0, 10, 0 },
    HALL_OF_FAME_DEMO_CHR_POKE_FRONT,
    HALL_OF_FAME_DEMO_PAL_POKE_FRONT,
    HALL_OF_FAME_DEMO_CEL_POKE_FRONT,
    0,
    CLACT_SURFACE_MAIN,
};
static const HallOfFameDemoObjSetup sHallOfFameDemoObjPokeBack = {
    { 304, 144, 0, 10, 0 },
    HALL_OF_FAME_DEMO_CHR_POKE_BACK,
    HALL_OF_FAME_DEMO_PAL_POKE_BACK,
    HALL_OF_FAME_DEMO_CEL_POKE_BACK,
    0,
    CLACT_SURFACE_MAIN,
};
static const HallOfFameDemoObjSetup sHallOfFameDemoObjTrainerSub = {
    { 88, -48, 0, 10, 1 },
    HALL_OF_FAME_DEMO_CHR_TRAINER_SUB,
    HALL_OF_FAME_DEMO_PAL_TRAINER_SUB,
    HALL_OF_FAME_DEMO_CEL_TRAINER_SUB,
    0,
    CLACT_SURFACE_SUB,
};
static const HallOfFameDemoObjSetup sHallOfFameDemoObjTrainerMain = {
    { 88, -304, 0, 10, 1 },
    HALL_OF_FAME_DEMO_CHR_TRAINER_MAIN,
    HALL_OF_FAME_DEMO_PAL_TRAINER_MAIN,
    HALL_OF_FAME_DEMO_CEL_TRAINER_MAIN,
    0,
    CLACT_SURFACE_MAIN,
};
static const HallOfFameDemoObjSetup sHallOfFameDemoObjConfettiSub = {
    { 0, 0, 4, 8, 0 }, HALL_OF_FAME_DEMO_CHR_OBJ_SUB, HALL_OF_FAME_DEMO_PAL_OBJ_SUB, HALL_OF_FAME_DEMO_CEL_OBJ_SUB, 0,
    CLACT_SURFACE_SUB,
};
static const HallOfFameDemoObjSetup sHallOfFameDemoObjConfettiMain = {
    { 0, 0, 4, 8, 0 },  HALL_OF_FAME_DEMO_CHR_OBJ, HALL_OF_FAME_DEMO_PAL_OBJ, HALL_OF_FAME_DEMO_CEL_OBJ, 0,
    CLACT_SURFACE_MAIN,
};
static const HallOfFameDemoObjSetup sHallOfFameDemoObjPlateUpper = {
    { 128, 24, 2, 10, 0 }, HALL_OF_FAME_DEMO_CHR_OBJ, HALL_OF_FAME_DEMO_PAL_OBJ, HALL_OF_FAME_DEMO_CEL_OBJ, 0,
    CLACT_SURFACE_MAIN,
};
static const HallOfFameDemoObjSetup sHallOfFameDemoObjPlateLower = {
    { 128, 168, 2, 10, 0 }, HALL_OF_FAME_DEMO_CHR_OBJ, HALL_OF_FAME_DEMO_PAL_OBJ, HALL_OF_FAME_DEMO_CEL_OBJ, 0,
    CLACT_SURFACE_MAIN,
};
static const HallOfFameDemoObjSetup sHallOfFameDemoObjPlateName = {
    { 264, 168, 1, 10, 0 }, HALL_OF_FAME_DEMO_CHR_OBJ, HALL_OF_FAME_DEMO_PAL_OBJ, HALL_OF_FAME_DEMO_CEL_OBJ, 0,
    CLACT_SURFACE_MAIN,
};
static const HallOfFameDemoObjSetup sHallOfFameDemoObjPlateTitle = {
    { -8, 24, 0, 10, 0 }, HALL_OF_FAME_DEMO_CHR_OBJ, HALL_OF_FAME_DEMO_PAL_OBJ, HALL_OF_FAME_DEMO_CEL_OBJ, 0,
    CLACT_SURFACE_MAIN,
};
static const HallOfFameDemoObjSetup sHallOfFameDemoObjFlash = {
    { 48, 144, 6, 8, 0 }, HALL_OF_FAME_DEMO_CHR_OBJ, HALL_OF_FAME_DEMO_PAL_OBJ, HALL_OF_FAME_DEMO_CEL_OBJ, 0,
    CLACT_SURFACE_MAIN,
};

// Sets up the actors, on the main screen alone for the first scene
void HallOfFameDemoObj_Init(HallOfFameDemoWork *wk, u32 scene) {
    if (scene == 0) {
        ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 0, 0, 0, 222, 7, 222, 0, 16, 0 };

        ClActSys_Create(&setup, HallOfFameDemoMain_GetVramConfig(scene), HEAPID_HALL_OF_FAME_DEMO);
    } else {
        ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 222, 7, 222, 0, 16, 16 };

        ClActSys_Create(&setup, HallOfFameDemoMain_GetVramConfig(scene), HEAPID_HALL_OF_FAME_DEMO);
    }
    wk->clUnit = ClActUnit_Create(HALL_OF_FAME_DEMO_OBJ_COUNT + 216, 0, HEAPID_HALL_OF_FAME_DEMO);
    wk->bmpOam = BmpOam_Init(HEAPID_HALL_OF_FAME_DEMO, wk->clUnit);
    HallOfFameDemoObj_InitResources(wk);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void HallOfFameDemoObj_Exit(HallOfFameDemoWork *wk) {
    BmpOam_Exit(wk->bmpOam);
    ClActUnit_Delete(wk->clUnit);
    ClActSys_Delete();
}

static void HallOfFameDemoObj_Animate(HallOfFameDemoWork *wk) {
    u32 i;

    for (i = 0; i < HALL_OF_FAME_DEMO_OBJ_COUNT; i++) {
        if (wk->clact[i] != NULL) {
            if (ClActor_IsAutoAnim(wk->clact[i]) != TRUE) {
                ClActor_SetAnimSpeed(wk->clact[i], FX32_ONE);
            }
        }
    }
    ClActSys_Draw();
}

void HallOfFameDemoObj_SetPos(HallOfFameDemoWork *wk, u32 id, s16 x, s16 y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    ClActor_SetSurfacePos(wk->clact[id], &pos, CLACT_SURFACE_MAIN);
}

void HallOfFameDemoObj_GetPos(HallOfFameDemoWork *wk, u32 id, s16 *x, s16 *y) {
    ClActorPos pos;

    ClActor_GetSurfacePos(wk->clact[id], &pos, CLACT_SURFACE_MAIN);
    *x = pos.x;
    *y = pos.y;
}

void HallOfFameDemoObj_Move(HallOfFameDemoWork *wk, u32 id, s16 x, s16 y) {
    s16 posX, posY;

    HallOfFameDemoObj_GetPos(wk, id, &posX, &posY);
    posX += x;
    posY += y;
    HallOfFameDemoObj_SetPos(wk, id, posX, posY);
}

void HallOfFameDemoObj_SetVisible(HallOfFameDemoWork *wk, u32 id, BOOL visible) {
    if (wk->clact[id] != NULL) {
        ClActor_SetVisible(wk->clact[id], visible);
    }
}

void HallOfFameDemoObj_SetAnime(HallOfFameDemoWork *wk, u32 id, u32 sequence) {
    ClActor_SetAnimTime(wk->clact[id], 0);
    ClActor_SetSequence(wk->clact[id], sequence);
    ClActor_SetAutoAnim(wk->clact[id], TRUE);
}

BOOL HallOfFameDemoObj_IsAnimating(HallOfFameDemoWork *wk, u32 id) {
    return ClActor_IsAnimActive(wk->clact[id]);
}

static void HallOfFameDemoObj_InitResources(HallOfFameDemoWork *wk) {
    u32 i;

    for (i = 0; i < HALL_OF_FAME_DEMO_CHR_COUNT; i++) {
        wk->chrRes[i] = RES_NONE;
    }
    for (i = 0; i < HALL_OF_FAME_DEMO_PAL_COUNT; i++) {
        wk->palRes[i] = RES_NONE;
    }
    for (i = 0; i < HALL_OF_FAME_DEMO_CEL_COUNT; i++) {
        wk->celRes[i] = RES_NONE;
    }
}

static void HallOfFameDemoObj_FreeChars(HallOfFameDemoWork *wk, u32 id) {
    if (wk->chrRes[id] != RES_NONE) {
        ClActSys_FreeChars(wk->chrRes[id]);
        wk->chrRes[id] = RES_NONE;
    }
}

static void HallOfFameDemoObj_FreePalette(HallOfFameDemoWork *wk, u32 id) {
    if (wk->palRes[id] != RES_NONE) {
        ClActSys_FreePltt(wk->palRes[id]);
        wk->palRes[id] = RES_NONE;
    }
}

static void HallOfFameDemoObj_FreeCellAnims(HallOfFameDemoWork *wk, u32 id) {
    if (wk->celRes[id] != RES_NONE) {
        ClActSys_FreeCellAnims(wk->celRes[id]);
        wk->celRes[id] = RES_NONE;
    }
}

static void HallOfFameDemoObj_FreeResources(HallOfFameDemoWork *wk) {
    u32 i;

    for (i = 0; i < HALL_OF_FAME_DEMO_CHR_COUNT; i++) {
        HallOfFameDemoObj_FreeChars(wk, i);
    }
    for (i = 0; i < HALL_OF_FAME_DEMO_PAL_COUNT; i++) {
        HallOfFameDemoObj_FreePalette(wk, i);
    }
    for (i = 0; i < HALL_OF_FAME_DEMO_CEL_COUNT; i++) {
        HallOfFameDemoObj_FreeCellAnims(wk, i);
    }
}

static ClActor *HallOfFameDemoObj_Add(HallOfFameDemoWork *wk, const HallOfFameDemoObjSetup *setup) {
    return ClActor_Create(wk->clUnit, wk->chrRes[setup->chr], wk->palRes[setup->pal], wk->celRes[setup->cel],
                          &setup->setup, setup->surface, HEAPID_HALL_OF_FAME_DEMO);
}

static void HallOfFameDemoObj_Delete(HallOfFameDemoWork *wk, u32 id) {
    if (wk->clact[id] != NULL) {
        ClActor_Delete(wk->clact[id]);
        wk->clact[id] = NULL;
    }
}

static void HallOfFameDemoObj_DeleteAll(HallOfFameDemoWork *wk) {
    u32 i;

    for (i = 0; i < HALL_OF_FAME_DEMO_OBJ_COUNT; i++) {
        HallOfFameDemoObj_Delete(wk, i);
    }
}

void HallOfFameDemoObj_AddPoke(HallOfFameDemoWork *wk) {
    ArcTool *arc;
    BoxPkm *pkm;
    BOOL wasEncrypted;

    if (wk->clact[HALL_OF_FAME_DEMO_OBJ_POKE_FRONT] != NULL) {
        HallOfFameDemoObj_Delete(wk, HALL_OF_FAME_DEMO_OBJ_POKE_FRONT);
        HallOfFameDemoObj_FreeChars(wk, HALL_OF_FAME_DEMO_CHR_POKE_FRONT);
        HallOfFameDemoObj_FreePalette(wk, HALL_OF_FAME_DEMO_PAL_POKE_FRONT);
        HallOfFameDemoObj_FreeCellAnims(wk, HALL_OF_FAME_DEMO_CEL_POKE_FRONT);
    }
    if (wk->clact[HALL_OF_FAME_DEMO_OBJ_POKE_BACK] != NULL) {
        HallOfFameDemoObj_Delete(wk, HALL_OF_FAME_DEMO_OBJ_POKE_BACK);
        HallOfFameDemoObj_FreeChars(wk, HALL_OF_FAME_DEMO_CHR_POKE_BACK);
        HallOfFameDemoObj_FreePalette(wk, HALL_OF_FAME_DEMO_PAL_POKE_BACK);
        HallOfFameDemoObj_FreeCellAnims(wk, HALL_OF_FAME_DEMO_CEL_POKE_BACK);
    }
    arc = MakePokeGraArcHandle(HEAPID_HALL_OF_FAME_DEMO);
    pkm = PokeParty_GetBoxPkm(wk->mons[wk->monIndex]);
    wasEncrypted = PML_PkmDecrypt(pkm);
    wk->chrRes[HALL_OF_FAME_DEMO_CHR_POKE_FRONT] =
        PokeGra_LoadClActCharsByBoxData(arc, pkm, POKEGRA_DIR_FRONT, CLACT_VRAM_MAIN, HEAPID_HALL_OF_FAME_DEMO);
    wk->palRes[HALL_OF_FAME_DEMO_PAL_POKE_FRONT] =
        PokeGra_LoadClActPaletteByBoxData(arc, pkm, POKEGRA_DIR_FRONT, CLACT_VRAM_MAIN, 0x60, HEAPID_HALL_OF_FAME_DEMO);
    wk->celRes[HALL_OF_FAME_DEMO_CEL_POKE_FRONT] =
        PokeGra_LoadClActCellAnimsByBoxData(pkm, POKEGRA_DIR_FRONT, 2, CLACT_VRAM_MAIN, HEAPID_HALL_OF_FAME_DEMO);
    wk->chrRes[HALL_OF_FAME_DEMO_CHR_POKE_BACK] =
        PokeGra_LoadClActCharsByBoxData(arc, pkm, POKEGRA_DIR_BACK, CLACT_VRAM_MAIN, HEAPID_HALL_OF_FAME_DEMO);
    wk->palRes[HALL_OF_FAME_DEMO_PAL_POKE_BACK] =
        PokeGra_LoadClActPaletteByBoxData(arc, pkm, POKEGRA_DIR_BACK, CLACT_VRAM_MAIN, 0x80, HEAPID_HALL_OF_FAME_DEMO);
    wk->celRes[HALL_OF_FAME_DEMO_CEL_POKE_BACK] =
        PokeGra_LoadClActCellAnimsByBoxData(pkm, POKEGRA_DIR_BACK, 2, CLACT_VRAM_MAIN, HEAPID_HALL_OF_FAME_DEMO);
    PML_PkmReEncrypt(pkm, wasEncrypted);
    GFL_ArcToolFree(arc);
    wk->clact[HALL_OF_FAME_DEMO_OBJ_POKE_FRONT] = HallOfFameDemoObj_Add(wk, &sHallOfFameDemoObjPokeFront);
    wk->clact[HALL_OF_FAME_DEMO_OBJ_POKE_BACK] = HallOfFameDemoObj_Add(wk, &sHallOfFameDemoObjPokeBack);
}

void HallOfFameDemoObj_LoadTypePalette(HallOfFameDemoWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_HALL_OF_FAME_DEMO, HEAPID_TAIL(HEAPID_HALL_OF_FAME_DEMO));
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLRArc(arc, sHallOfFameDemoTypePalettes[wk->type], &palette, HEAPID_HALL_OF_FAME_DEMO);

    ClActSys_ReplacePlttData(wk->palRes[HALL_OF_FAME_DEMO_PAL_OBJ], palette, 1);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
}

static void HallOfFameDemoObj_DeleteFontOam(HallOfFameDemoWork *wk, u32 id) {
    if (wk->fontOam[id].oam != NULL) {
        BmpOam_ActorDel(wk->fontOam[id].oam);
        GFL_BitmapFree(wk->fontOam[id].bmp);
        wk->fontOam[id].oam = NULL;
    }
}

static void HallOfFameDemoObj_DeleteFontOamAll(HallOfFameDemoWork *wk) {
    u32 i;

    for (i = 0; i < HALL_OF_FAME_DEMO_FOAM_COUNT; i++) {
        HallOfFameDemoObj_DeleteFontOam(wk, i);
    }
}

static void HallOfFameDemoObj_PrintFontOam(HallOfFameDemoWork *wk, u32 id, Font *font, StrBuf *str, int x, int y) {
    GFL_TextRendererDrawToBitmapEx(wk->fontOam[id].bmp, x, y, str, font, FONT_COLOR);
    BmpOam_ActorBmpTrans(wk->fontOam[id].oam);
}

// The first scene's texts: the title on its plate, and the Pokémon's, which HallOfFameDemoObj_PrintPokeInfo draws
static void HallOfFameDemoObj_InitFontOam1st(HallOfFameDemoWork *wk) {
    BmpOamActorSetup setup;
    HallOfFameDemoFontOam *fontOam;
    StrBuf *str;

    setup.palette = wk->palRes[HALL_OF_FAME_DEMO_PAL_FONT];
    setup.paletteOffset = 0;
    setup.priority = 4;
    setup.bgPriority = 0;
    setup.surface = 0xffff;
    setup.vramType = CLACT_VRAM_MAIN;

    fontOam = &wk->fontOam[HALL_OF_FAME_DEMO_FOAM_TITLE];
    fontOam->bmp = GFL_BitmapCreate(22, 2, BGSYS_TILE_SIZE_16, HEAPID_HALL_OF_FAME_DEMO);
    setup.bitmap = fontOam->bmp;
    setup.x = -184;
    setup.y = 16;
    wk->fontOam[HALL_OF_FAME_DEMO_FOAM_TITLE].oam = BmpOam_ActorAdd(wk->bmpOam, &setup);
    GFL_BitmapFill(wk->fontOam[HALL_OF_FAME_DEMO_FOAM_TITLE].bmp, 0);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_HallFame);
    HallOfFameDemoObj_PrintFontOam(wk, HALL_OF_FAME_DEMO_FOAM_TITLE, wk->font, str, 12, 0);
    GFL_StrBufFree(str);

    fontOam = &wk->fontOam[HALL_OF_FAME_DEMO_FOAM_NAME];
    fontOam->bmp = GFL_BitmapCreate(22, 2, BGSYS_TILE_SIZE_16, HEAPID_HALL_OF_FAME_DEMO);
    setup.bitmap = fontOam->bmp;
    setup.x = 264;
    setup.y = 160;
    wk->fontOam[HALL_OF_FAME_DEMO_FOAM_NAME].oam = BmpOam_ActorAdd(wk->bmpOam, &setup);
}

// Draws the name and level of the Pokémon being shown
void HallOfFameDemoObj_PrintPokeInfo(HallOfFameDemoWork *wk) {
    BoxPkm *pkm = PokeParty_GetBoxPkm(wk->mons[wk->monIndex]);
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);
    StrBuf *str;
    u32 width;

    GFL_BitmapFill(wk->fontOam[HALL_OF_FAME_DEMO_FOAM_NAME].bmp, 0);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_Empty);
    loadBoxPokemonNameToStrbuf(wk->wordSet, 0, pkm);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, str);
    HallOfFameDemoObj_PrintFontOam(wk, HALL_OF_FAME_DEMO_FOAM_NAME, wk->font, wk->strbuf, 8, 0);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_Lv);
    width = GFL_FontGetBlockWidth(str, wk->font, 0);
    HallOfFameDemoObj_PrintFontOam(wk, HALL_OF_FAME_DEMO_FOAM_NAME, wk->font, str, 112, 0);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_Empty_2);
    WordSetNumber(wk->wordSet, 0, PML_PkmGetParam(pkm, PKM_PARAM_LEVEL, NULL), 3, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, str);
    HallOfFameDemoObj_PrintFontOam(wk, HALL_OF_FAME_DEMO_FOAM_NAME, wk->font, wk->strbuf, 112 + width, 0);
    GFL_StrBufFree(str);

    PML_PkmReEncrypt(pkm, wasEncrypted);
}

// The second scene's texts, hidden: the congratulations, and the trainer's name, ID and play time
static void HallOfFameDemoObj_InitFontOam2nd(HallOfFameDemoWork *wk) {
    BmpOamActorSetup setup;
    HallOfFameDemoFontOam *fontOam;
    StrBuf *str;
    u32 width;

    setup.palette = wk->palRes[HALL_OF_FAME_DEMO_PAL_FONT];
    setup.paletteOffset = 0;
    setup.priority = 4;
    setup.bgPriority = 0;
    setup.surface = 0xffff;
    setup.vramType = CLACT_VRAM_MAIN;

    fontOam = &wk->fontOam[HALL_OF_FAME_DEMO_FOAM_TITLE_2ND];
    fontOam->bmp = GFL_BitmapCreate(32, 2, BGSYS_TILE_SIZE_16, HEAPID_HALL_OF_FAME_DEMO);
    setup.bitmap = fontOam->bmp;
    setup.x = 0;
    setup.y = 16;
    wk->fontOam[HALL_OF_FAME_DEMO_FOAM_TITLE_2ND].oam = BmpOam_ActorAdd(wk->bmpOam, &setup);
    BmpOam_ActorSetDrawEnable(wk->fontOam[HALL_OF_FAME_DEMO_FOAM_TITLE_2ND].oam, FALSE);
    GFL_BitmapFill(wk->fontOam[HALL_OF_FAME_DEMO_FOAM_TITLE_2ND].bmp, 0);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_LeagueChampionCongratulations);
    width = GFL_FontGetBlockWidth(str, wk->font, 0);
    HallOfFameDemoObj_PrintFontOam(wk, HALL_OF_FAME_DEMO_FOAM_TITLE_2ND, wk->font, str, (256 - width) / 2, 0);
    GFL_StrBufFree(str);

    fontOam = &wk->fontOam[HALL_OF_FAME_DEMO_FOAM_PLAYER];
    fontOam->bmp = GFL_BitmapCreate(32, 2, BGSYS_TILE_SIZE_16, HEAPID_HALL_OF_FAME_DEMO);
    setup.bitmap = fontOam->bmp;
    setup.x = 0;
    setup.y = 160;
    wk->fontOam[HALL_OF_FAME_DEMO_FOAM_PLAYER].oam = BmpOam_ActorAdd(wk->bmpOam, &setup);
    BmpOam_ActorSetDrawEnable(wk->fontOam[HALL_OF_FAME_DEMO_FOAM_PLAYER].oam, FALSE);
    GFL_BitmapFill(wk->fontOam[HALL_OF_FAME_DEMO_FOAM_PLAYER].bmp, 0);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_Empty_3);
    copyVarForText(wk->wordSet, 0, wk->param->playerInfo);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, str);
    HallOfFameDemoObj_PrintFontOam(wk, HALL_OF_FAME_DEMO_FOAM_PLAYER, wk->font, wk->strbuf, 8, 0);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_IdNo);
    WordSetNumber(wk->wordSet, 0, getTrainerID(wk->param->playerInfo), 5, 2, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, str);
    HallOfFameDemoObj_PrintFontOam(wk, HALL_OF_FAME_DEMO_FOAM_PLAYER, wk->font, wk->strbuf, 92, 0);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_Empty_4);
    WordSetNumber(wk->wordSet, 0, PlayTime_GetHours(wk->param->playTime), 3, 0, TRUE);
    WordSetNumber(wk->wordSet, 1, PlayTime_GetMinutes(wk->param->playTime), 2, 2, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, str);
    HallOfFameDemoObj_PrintFontOam(wk, HALL_OF_FAME_DEMO_FOAM_PLAYER, wk->font, wk->strbuf, 200, 0);
    GFL_StrBufFree(str);
}

void HallOfFameDemoObj_MoveFontOam(HallOfFameDemoWork *wk) {
    s16 x, y;

    HallOfFameDemoObj_GetPos(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_TITLE, &x, &y);
    BmpOam_ActorSetPos(wk->fontOam[HALL_OF_FAME_DEMO_FOAM_TITLE].oam, x - 176, y - 8);
    HallOfFameDemoObj_GetPos(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_NAME, &x, &y);
    BmpOam_ActorSetPos(wk->fontOam[HALL_OF_FAME_DEMO_FOAM_NAME].oam, x, y - 8);
}

void HallOfFameDemoObj_SetFontOamVisible(HallOfFameDemoWork *wk, u32 id, BOOL visible) {
    BmpOam_ActorSetDrawEnable(wk->fontOam[id].oam, visible);
}

// Loads the main screen's actors' graphics and the font's palette
static void HallOfFameDemoObj_LoadResources(HallOfFameDemoWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_HALL_OF_FAME_DEMO, HEAPID_TAIL(HEAPID_HALL_OF_FAME_DEMO));

    wk->chrRes[HALL_OF_FAME_DEMO_CHR_OBJ] =
        ClActSys_LoadChars(arc, NARC_HALL_OF_FAME_DEMO_OBJ_1ST_NCGR, FALSE, CLACT_VRAM_MAIN, HEAPID_HALL_OF_FAME_DEMO);
    wk->palRes[HALL_OF_FAME_DEMO_PAL_OBJ] =
        ClActSys_LoadPltt(arc, NARC_HALL_OF_FAME_DEMO_OBJ_NCLR, CLACT_VRAM_MAIN, 0, HEAPID_HALL_OF_FAME_DEMO);
    wk->celRes[HALL_OF_FAME_DEMO_CEL_OBJ] = ClActSys_LoadCellAnims(
        arc, NARC_HALL_OF_FAME_DEMO_OBJ_1ST_NCER, NARC_HALL_OF_FAME_DEMO_OBJ_1ST_NANR, HEAPID_HALL_OF_FAME_DEMO);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, HEAPID_TAIL(HEAPID_HALL_OF_FAME_DEMO));
    wk->palRes[HALL_OF_FAME_DEMO_PAL_FONT] =
        ClActSys_LoadPlttEx(arc, 5, CLACT_VRAM_MAIN, 0xa0, HEAPID_HALL_OF_FAME_DEMO);
    GFL_ArcToolFree(arc);
}

void HallOfFameDemoObj_Init1st(HallOfFameDemoWork *wk) {
    HallOfFameDemoObj_LoadResources(wk);
    HallOfFameDemoObj_LoadTypePalette(wk);
    wk->clact[HALL_OF_FAME_DEMO_OBJ_PLATE_TITLE] = HallOfFameDemoObj_Add(wk, &sHallOfFameDemoObjPlateTitle);
    wk->clact[HALL_OF_FAME_DEMO_OBJ_PLATE_NAME] = HallOfFameDemoObj_Add(wk, &sHallOfFameDemoObjPlateName);
    wk->clact[HALL_OF_FAME_DEMO_OBJ_FLASH] = HallOfFameDemoObj_Add(wk, &sHallOfFameDemoObjFlash);
    HallOfFameDemoObj_SetVisible(wk, HALL_OF_FAME_DEMO_OBJ_FLASH, FALSE);
    HallOfFameDemoObj_InitFontOam1st(wk);
}

void HallOfFameDemoObj_Exit1st(HallOfFameDemoWork *wk) {
    HallOfFameDemoObj_DeleteFontOamAll(wk);
    HallOfFameDemoObj_DeleteAll(wk);
    HallOfFameDemoObj_FreeResources(wk);
}

// Shows the actors on the frames that draw the main screen
void HallOfFameDemoObj_Main1st(HallOfFameDemoWork *wk) {
    if (GFL_G3DDual3DGetRenderDisp() == TRUE) {
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, FALSE);
    } else {
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    }
    HallOfFameDemoObj_Animate(wk);
}

// The second scene's actors, on a renderer of their own whose sub screen is 512 below the main one
void HallOfFameDemoObj_Init2nd(HallOfFameDemoWork *wk) {
    ArcTool *arc;
    u32 trainerClass;

    HallOfFameDemoObj_LoadResources(wk);
    {
        ClActSurfaceSetup surfaces[] = {
            { 0, 0, 256, 192, CLACT_VRAM_MAIN, TRUE },
            { 0, 512, 256, 192, CLACT_VRAM_SUB, TRUE },
        };

        wk->renderer = ClActRenderer_Create(surfaces, NELEMS(surfaces), HEAPID_HALL_OF_FAME_DEMO);
    }
    ClActRenderer_SetCulling(wk->renderer, TRUE);
    ClActUnit_SetRenderer(wk->clUnit, wk->renderer);
    wk->clact[HALL_OF_FAME_DEMO_OBJ_PLATE_UPPER] = HallOfFameDemoObj_Add(wk, &sHallOfFameDemoObjPlateUpper);
    wk->clact[HALL_OF_FAME_DEMO_OBJ_PLATE_LOWER] = HallOfFameDemoObj_Add(wk, &sHallOfFameDemoObjPlateLower);
    HallOfFameDemoObj_SetVisible(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_UPPER, FALSE);
    HallOfFameDemoObj_SetVisible(wk, HALL_OF_FAME_DEMO_OBJ_PLATE_LOWER, FALSE);

    arc = GFL_ArcSysCreateFileHandle(ARCID_HALL_OF_FAME_DEMO, HEAPID_TAIL(HEAPID_HALL_OF_FAME_DEMO));
    wk->chrRes[HALL_OF_FAME_DEMO_CHR_OBJ_SUB] =
        ClActSys_LoadChars(arc, NARC_HALL_OF_FAME_DEMO_OBJ_2ND_NCGR, FALSE, CLACT_VRAM_SUB, HEAPID_HALL_OF_FAME_DEMO);
    wk->palRes[HALL_OF_FAME_DEMO_PAL_OBJ_SUB] =
        ClActSys_LoadPltt(arc, NARC_HALL_OF_FAME_DEMO_OBJ_NCLR, CLACT_VRAM_SUB, 0, HEAPID_HALL_OF_FAME_DEMO);
    wk->celRes[HALL_OF_FAME_DEMO_CEL_OBJ_SUB] = ClActSys_LoadCellAnims(
        arc, NARC_HALL_OF_FAME_DEMO_OBJ_2ND_NCER, NARC_HALL_OF_FAME_DEMO_OBJ_2ND_NANR, HEAPID_HALL_OF_FAME_DEMO);
    GFL_ArcToolFree(arc);

    trainerClass = TRAINER_CLASS_PKMN_TRAINER_ROSA;
    if (getTrainerGender(wk->param->playerInfo) == GENDER_MALE) {
        trainerClass = TRAINER_CLASS_PKMN_TRAINER_NATE;
    }
    arc = MakeTrGraArcHandle(HEAPID_TAIL(HEAPID_HALL_OF_FAME_DEMO));
    wk->chrRes[HALL_OF_FAME_DEMO_CHR_TRAINER_MAIN] =
        TrGra_LoadClActChars(arc, trainerClass, CLACT_VRAM_MAIN, HEAPID_HALL_OF_FAME_DEMO);
    wk->palRes[HALL_OF_FAME_DEMO_PAL_TRAINER_MAIN] =
        TrGra_LoadClActPalette(arc, trainerClass, CLACT_VRAM_MAIN, 0xc0, HEAPID_HALL_OF_FAME_DEMO);
    wk->celRes[HALL_OF_FAME_DEMO_CEL_TRAINER_MAIN] =
        TrGra_LoadClActCellAnims(trainerClass, 2, CLACT_VRAM_MAIN, HEAPID_HALL_OF_FAME_DEMO);
    wk->chrRes[HALL_OF_FAME_DEMO_CHR_TRAINER_SUB] =
        TrGra_LoadClActChars(arc, trainerClass, CLACT_VRAM_SUB, HEAPID_HALL_OF_FAME_DEMO);
    wk->palRes[HALL_OF_FAME_DEMO_PAL_TRAINER_SUB] =
        TrGra_LoadClActPalette(arc, trainerClass, CLACT_VRAM_SUB, 0x60, HEAPID_HALL_OF_FAME_DEMO);
    wk->celRes[HALL_OF_FAME_DEMO_CEL_TRAINER_SUB] =
        TrGra_LoadClActCellAnims(trainerClass, 0, CLACT_VRAM_SUB, HEAPID_HALL_OF_FAME_DEMO);
    GFL_ArcToolFree(arc);
    wk->clact[HALL_OF_FAME_DEMO_OBJ_TRAINER_MAIN] = HallOfFameDemoObj_Add(wk, &sHallOfFameDemoObjTrainerMain);
    wk->clact[HALL_OF_FAME_DEMO_OBJ_TRAINER_SUB] = HallOfFameDemoObj_Add(wk, &sHallOfFameDemoObjTrainerSub);
    HallOfFameDemoObj_InitFontOam2nd(wk);
}

void HallOfFameDemoObj_Exit2nd(HallOfFameDemoWork *wk) {
    HallOfFameDemoObj_DeleteFontOamAll(wk);
    HallOfFameDemoObj_DeleteAll(wk);
    ClActRenderer_Delete(wk->renderer);
    HallOfFameDemoObj_FreeResources(wk);
}

void HallOfFameDemoObj_Main2nd(HallOfFameDemoWork *wk) {
    HallOfFameDemoObj_Animate(wk);
}

void HallOfFameDemoObj_MainConfetti(HallOfFameDemoWork *wk) {
    s16 x, y;
    u16 i;
    u16 rand;

    for (i = HALL_OF_FAME_DEMO_OBJ_CONFETTI; i < HALL_OF_FAME_DEMO_OBJ_COUNT; i++) {
        if (wk->clact[i] != NULL) {
            if (HallOfFameDemoObj_IsAnimating(wk, i) != TRUE) {
                HallOfFameDemoObj_Delete(wk, i);
            }
        }
    }
    rand = GFL_RandomLC(4);
    if (rand < 2) {
        for (i = HALL_OF_FAME_DEMO_OBJ_CONFETTI; i < HALL_OF_FAME_DEMO_OBJ_COUNT; i++) {
            if (wk->clact[i] == NULL) {
                HallOfFameDemoObjSetup setup;

                x = GFL_RandomLC(256);
                y = GFL_RandomLC(216);
                if (y < 192) {
                    setup = sHallOfFameDemoObjConfettiMain;
                    setup.setup.x = x;
                    setup.setup.y = y;
                } else {
                    setup = sHallOfFameDemoObjConfettiSub;
                    setup.setup.x = x;
                    setup.setup.y = y - 24;
                }
                wk->clact[i] = HallOfFameDemoObj_Add(wk, &setup);
                HallOfFameDemoObj_SetAnime(wk, i, rand + 4);
                break;
            }
        }
    }
}
