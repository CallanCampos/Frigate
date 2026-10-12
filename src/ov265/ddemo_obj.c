#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/trainer_classes.h"
#include "demo/ddemo_main.h"
#include "demo/ddemo_obj.h"
#include "demo/dendou_demo.h"
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
// is a guess, after the Hall of Fame PC's dpc_obj.c

// An actor: its setup, its resources (DDEMO_CHR_*, DDEMO_PAL_* and DDEMO_CEL_*) and its surface
typedef struct {
    ClActorSetup setup;
    u32 chr;
    u32 pal;
    u32 cel;
    u16 unk14;
    u16 surface;
} DDemoObjSetup;

#define RES_NONE 0xffffffff

// The texts' color
#define FONT_COLOR PRINT_COLOR(15, 2, 0)

static void DDemoObj_Animate(DDemoWork *wk);
static void DDemoObj_InitResources(DDemoWork *wk);
static void DDemoObj_FreeChars(DDemoWork *wk, u32 id);
static void DDemoObj_FreePalette(DDemoWork *wk, u32 id);
static void DDemoObj_FreeCellAnims(DDemoWork *wk, u32 id);
static void DDemoObj_FreeResources(DDemoWork *wk);
static ClActor *DDemoObj_Add(DDemoWork *wk, const DDemoObjSetup *setup);
static void DDemoObj_Delete(DDemoWork *wk, u32 id);
static void DDemoObj_DeleteAll(DDemoWork *wk);
static void DDemoObj_DeleteFontOam(DDemoWork *wk, u32 id);
static void DDemoObj_DeleteFontOamAll(DDemoWork *wk);
static void DDemoObj_PrintFontOam(DDemoWork *wk, u32 id, Font *font, StrBuf *str, int x, int y);
static void DDemoObj_InitFontOam1st(DDemoWork *wk);
static void DDemoObj_InitFontOam2nd(DDemoWork *wk);
static void DDemoObj_LoadResources(DDemoWork *wk);

// The palette file of each type, for the plates
static const u32 sDDemoTypePalettes[] = {
    3, 9, 11, 16, 14, 18, 5, 12, 19, 10, 20, 13, 8, 17, 15, 7, 6,
};

// The actors. The order they are declared in gives the order the game has them in
static const DDemoObjSetup sDDemoObjPokeFront = {
    { -48, 144, 0, 10, 0 }, DDEMO_CHR_POKE_FRONT, DDEMO_PAL_POKE_FRONT, DDEMO_CEL_POKE_FRONT, 0, CLACT_SURFACE_MAIN,
};
static const DDemoObjSetup sDDemoObjPokeBack = {
    { 304, 144, 0, 10, 0 }, DDEMO_CHR_POKE_BACK, DDEMO_PAL_POKE_BACK, DDEMO_CEL_POKE_BACK, 0, CLACT_SURFACE_MAIN,
};
static const DDemoObjSetup sDDemoObjTrainerSub = {
    { 88, -48, 0, 10, 1 }, DDEMO_CHR_TRAINER_SUB, DDEMO_PAL_TRAINER_SUB, DDEMO_CEL_TRAINER_SUB, 0, CLACT_SURFACE_SUB,
};
static const DDemoObjSetup sDDemoObjTrainerMain = {
    { 88, -304, 0, 10, 1 }, DDEMO_CHR_TRAINER_MAIN, DDEMO_PAL_TRAINER_MAIN, DDEMO_CEL_TRAINER_MAIN, 0,
    CLACT_SURFACE_MAIN,
};
static const DDemoObjSetup sDDemoObjConfettiSub = {
    { 0, 0, 4, 8, 0 }, DDEMO_CHR_OBJ_SUB, DDEMO_PAL_OBJ_SUB, DDEMO_CEL_OBJ_SUB, 0, CLACT_SURFACE_SUB,
};
static const DDemoObjSetup sDDemoObjConfettiMain = {
    { 0, 0, 4, 8, 0 }, DDEMO_CHR_OBJ, DDEMO_PAL_OBJ, DDEMO_CEL_OBJ, 0, CLACT_SURFACE_MAIN,
};
static const DDemoObjSetup sDDemoObjPlateUpper = {
    { 128, 24, 2, 10, 0 }, DDEMO_CHR_OBJ, DDEMO_PAL_OBJ, DDEMO_CEL_OBJ, 0, CLACT_SURFACE_MAIN,
};
static const DDemoObjSetup sDDemoObjPlateLower = {
    { 128, 168, 2, 10, 0 }, DDEMO_CHR_OBJ, DDEMO_PAL_OBJ, DDEMO_CEL_OBJ, 0, CLACT_SURFACE_MAIN,
};
static const DDemoObjSetup sDDemoObjPlateName = {
    { 264, 168, 1, 10, 0 }, DDEMO_CHR_OBJ, DDEMO_PAL_OBJ, DDEMO_CEL_OBJ, 0, CLACT_SURFACE_MAIN,
};
static const DDemoObjSetup sDDemoObjPlateTitle = {
    { -8, 24, 0, 10, 0 }, DDEMO_CHR_OBJ, DDEMO_PAL_OBJ, DDEMO_CEL_OBJ, 0, CLACT_SURFACE_MAIN,
};
static const DDemoObjSetup sDDemoObjFlash = {
    { 48, 144, 6, 8, 0 }, DDEMO_CHR_OBJ, DDEMO_PAL_OBJ, DDEMO_CEL_OBJ, 0, CLACT_SURFACE_MAIN,
};

// Sets up the actors, on the main screen alone for the first scene
void DDemoObj_Init(DDemoWork *wk, u32 scene) {
    if (scene == 0) {
        ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 0, 0, 0, 222, 7, 222, 0, 16, 0 };

        ClActSys_Create(&setup, DDemoMain_GetVramConfig(scene), HEAPID_DENDOU_DEMO);
    } else {
        ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 222, 7, 222, 0, 16, 16 };

        ClActSys_Create(&setup, DDemoMain_GetVramConfig(scene), HEAPID_DENDOU_DEMO);
    }
    wk->clUnit = func_0204bf1c(DDEMOOBJ_COUNT + 216, 0, HEAPID_DENDOU_DEMO);
    wk->bmpOam = BmpOam_Init(HEAPID_DENDOU_DEMO, wk->clUnit);
    DDemoObj_InitResources(wk);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void DDemoObj_Exit(DDemoWork *wk) {
    BmpOam_Exit(wk->bmpOam);
    func_0204bf98(wk->clUnit);
    func_0204b758();
}

static void DDemoObj_Animate(DDemoWork *wk) {
    u32 i;

    for (i = 0; i < DDEMOOBJ_COUNT; i++) {
        if (wk->clact[i] != NULL) {
            if (func_0204c534(wk->clact[i]) != TRUE) {
                func_0204c4e0(wk->clact[i], FX32_ONE);
            }
        }
    }
    func_0204b794();
}

void DDemoObj_SetPos(DDemoWork *wk, u32 id, s16 x, s16 y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c140(wk->clact[id], &pos, CLACT_SURFACE_MAIN);
}

void DDemoObj_GetPos(DDemoWork *wk, u32 id, s16 *x, s16 *y) {
    ClActorPos pos;

    func_0204c178(wk->clact[id], &pos, CLACT_SURFACE_MAIN);
    *x = pos.x;
    *y = pos.y;
}

void DDemoObj_Move(DDemoWork *wk, u32 id, s16 x, s16 y) {
    s16 posX, posY;

    DDemoObj_GetPos(wk, id, &posX, &posY);
    posX += x;
    posY += y;
    DDemoObj_SetPos(wk, id, posX, posY);
}

void DDemoObj_SetVisible(DDemoWork *wk, u32 id, BOOL visible) {
    if (wk->clact[id] != NULL) {
        func_0204c124(wk->clact[id], visible);
    }
}

void DDemoObj_SetAnime(DDemoWork *wk, u32 id, u32 sequence) {
    func_0204c4d4(wk->clact[id], 0);
    func_0204c488(wk->clact[id], sequence);
    func_0204c520(wk->clact[id], TRUE);
}

BOOL DDemoObj_IsAnimating(DDemoWork *wk, u32 id) {
    return func_0204c560(wk->clact[id]);
}

static void DDemoObj_InitResources(DDemoWork *wk) {
    u32 i;

    for (i = 0; i < DDEMO_CHR_COUNT; i++) {
        wk->chrRes[i] = RES_NONE;
    }
    for (i = 0; i < DDEMO_PAL_COUNT; i++) {
        wk->palRes[i] = RES_NONE;
    }
    for (i = 0; i < DDEMO_CEL_COUNT; i++) {
        wk->celRes[i] = RES_NONE;
    }
}

static void DDemoObj_FreeChars(DDemoWork *wk, u32 id) {
    if (wk->chrRes[id] != RES_NONE) {
        func_0204b98c(wk->chrRes[id]);
        wk->chrRes[id] = RES_NONE;
    }
}

static void DDemoObj_FreePalette(DDemoWork *wk, u32 id) {
    if (wk->palRes[id] != RES_NONE) {
        func_0204bcd0(wk->palRes[id]);
        wk->palRes[id] = RES_NONE;
    }
}

static void DDemoObj_FreeCellAnims(DDemoWork *wk, u32 id) {
    if (wk->celRes[id] != RES_NONE) {
        func_0204be64(wk->celRes[id]);
        wk->celRes[id] = RES_NONE;
    }
}

static void DDemoObj_FreeResources(DDemoWork *wk) {
    u32 i;

    for (i = 0; i < DDEMO_CHR_COUNT; i++) {
        DDemoObj_FreeChars(wk, i);
    }
    for (i = 0; i < DDEMO_PAL_COUNT; i++) {
        DDemoObj_FreePalette(wk, i);
    }
    for (i = 0; i < DDEMO_CEL_COUNT; i++) {
        DDemoObj_FreeCellAnims(wk, i);
    }
}

static ClActor *DDemoObj_Add(DDemoWork *wk, const DDemoObjSetup *setup) {
    return func_0204c040(wk->clUnit, wk->chrRes[setup->chr], wk->palRes[setup->pal], wk->celRes[setup->cel],
                         &setup->setup, setup->surface, HEAPID_DENDOU_DEMO);
}

static void DDemoObj_Delete(DDemoWork *wk, u32 id) {
    if (wk->clact[id] != NULL) {
        func_0204c108(wk->clact[id]);
        wk->clact[id] = NULL;
    }
}

static void DDemoObj_DeleteAll(DDemoWork *wk) {
    u32 i;

    for (i = 0; i < DDEMOOBJ_COUNT; i++) {
        DDemoObj_Delete(wk, i);
    }
}

void DDemoObj_AddPoke(DDemoWork *wk) {
    ArcTool *arc;
    BoxPkm *pkm;
    BOOL wasEncrypted;

    if (wk->clact[DDEMOOBJ_POKE_FRONT] != NULL) {
        DDemoObj_Delete(wk, DDEMOOBJ_POKE_FRONT);
        DDemoObj_FreeChars(wk, DDEMO_CHR_POKE_FRONT);
        DDemoObj_FreePalette(wk, DDEMO_PAL_POKE_FRONT);
        DDemoObj_FreeCellAnims(wk, DDEMO_CEL_POKE_FRONT);
    }
    if (wk->clact[DDEMOOBJ_POKE_BACK] != NULL) {
        DDemoObj_Delete(wk, DDEMOOBJ_POKE_BACK);
        DDemoObj_FreeChars(wk, DDEMO_CHR_POKE_BACK);
        DDemoObj_FreePalette(wk, DDEMO_PAL_POKE_BACK);
        DDemoObj_FreeCellAnims(wk, DDEMO_CEL_POKE_BACK);
    }
    arc = MakePokeGraArcHandle(HEAPID_DENDOU_DEMO);
    pkm = func_0201d620(wk->mons[wk->monIndex]);
    wasEncrypted = PML_PkmDecrypt(pkm);
    wk->chrRes[DDEMO_CHR_POKE_FRONT] =
        PokeGra_LoadClActCharsByBoxData(arc, pkm, 0, CLACT_VRAM_MAIN, HEAPID_DENDOU_DEMO);
    wk->palRes[DDEMO_PAL_POKE_FRONT] =
        PokeGra_LoadClActPaletteByBoxData(arc, pkm, 0, CLACT_VRAM_MAIN, 0x60, HEAPID_DENDOU_DEMO);
    wk->celRes[DDEMO_CEL_POKE_FRONT] =
        PokeGra_LoadClActCellAnimsByBoxData(pkm, 0, 2, CLACT_VRAM_MAIN, HEAPID_DENDOU_DEMO);
    wk->chrRes[DDEMO_CHR_POKE_BACK] = PokeGra_LoadClActCharsByBoxData(arc, pkm, 1, CLACT_VRAM_MAIN, HEAPID_DENDOU_DEMO);
    wk->palRes[DDEMO_PAL_POKE_BACK] =
        PokeGra_LoadClActPaletteByBoxData(arc, pkm, 1, CLACT_VRAM_MAIN, 0x80, HEAPID_DENDOU_DEMO);
    wk->celRes[DDEMO_CEL_POKE_BACK] =
        PokeGra_LoadClActCellAnimsByBoxData(pkm, 1, 2, CLACT_VRAM_MAIN, HEAPID_DENDOU_DEMO);
    PML_PkmReEncrypt(pkm, wasEncrypted);
    GFL_ArcToolFree(arc);
    wk->clact[DDEMOOBJ_POKE_FRONT] = DDemoObj_Add(wk, &sDDemoObjPokeFront);
    wk->clact[DDEMOOBJ_POKE_BACK] = DDemoObj_Add(wk, &sDDemoObjPokeBack);
}

void DDemoObj_LoadTypePalette(DDemoWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_DENDOU_DEMO, HEAPID_TAIL(HEAPID_DENDOU_DEMO));
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLRArc(arc, sDDemoTypePalettes[wk->type], &palette, HEAPID_DENDOU_DEMO);

    func_0204bd10(wk->palRes[DDEMO_PAL_OBJ], palette, 1);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
}

static void DDemoObj_DeleteFontOam(DDemoWork *wk, u32 id) {
    if (wk->fontOam[id].oam != NULL) {
        BmpOam_ActorDel(wk->fontOam[id].oam);
        GFL_BitmapFree(wk->fontOam[id].bmp);
        wk->fontOam[id].oam = NULL;
    }
}

static void DDemoObj_DeleteFontOamAll(DDemoWork *wk) {
    u32 i;

    for (i = 0; i < DDEMO_FOAM_COUNT; i++) {
        DDemoObj_DeleteFontOam(wk, i);
    }
}

static void DDemoObj_PrintFontOam(DDemoWork *wk, u32 id, Font *font, StrBuf *str, int x, int y) {
    GFL_TextRendererDrawToBitmapEx(wk->fontOam[id].bmp, x, y, str, font, FONT_COLOR);
    BmpOam_ActorBmpTrans(wk->fontOam[id].oam);
}

// The first scene's texts: the title on its plate, and the Pokémon's, which DDemoObj_PrintPokeInfo draws
static void DDemoObj_InitFontOam1st(DDemoWork *wk) {
    BmpOamActorSetup setup;
    DDemoFontOam *fontOam;
    StrBuf *str;

    setup.palette = wk->palRes[DDEMO_PAL_FONT];
    setup.paletteOffset = 0;
    setup.priority = 4;
    setup.bgPriority = 0;
    setup.surface = 0xffff;
    setup.vramType = CLACT_VRAM_MAIN;

    fontOam = &wk->fontOam[DDEMO_FOAM_TITLE];
    fontOam->bmp = GFL_BitmapCreate(22, 2, BGSYS_TILE_SIZE_16, HEAPID_DENDOU_DEMO);
    setup.bitmap = fontOam->bmp;
    setup.x = -184;
    setup.y = 16;
    wk->fontOam[DDEMO_FOAM_TITLE].oam = BmpOam_ActorAdd(wk->bmpOam, &setup);
    GFL_BitmapFill(wk->fontOam[DDEMO_FOAM_TITLE].bmp, 0);
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_HallFame);
    DDemoObj_PrintFontOam(wk, DDEMO_FOAM_TITLE, wk->font, str, 12, 0);
    GFL_StrBufFree(str);

    fontOam = &wk->fontOam[DDEMO_FOAM_NAME];
    fontOam->bmp = GFL_BitmapCreate(22, 2, BGSYS_TILE_SIZE_16, HEAPID_DENDOU_DEMO);
    setup.bitmap = fontOam->bmp;
    setup.x = 264;
    setup.y = 160;
    wk->fontOam[DDEMO_FOAM_NAME].oam = BmpOam_ActorAdd(wk->bmpOam, &setup);
}

// Draws the name and level of the Pokémon being shown
void DDemoObj_PrintPokeInfo(DDemoWork *wk) {
    BoxPkm *pkm = func_0201d620(wk->mons[wk->monIndex]);
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);
    StrBuf *str;
    u32 width;

    GFL_BitmapFill(wk->fontOam[DDEMO_FOAM_NAME].bmp, 0);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_Empty);
    loadBoxPokemonNameToStrbuf(wk->wordSet, 0, pkm);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, str);
    DDemoObj_PrintFontOam(wk, DDEMO_FOAM_NAME, wk->font, wk->strbuf, 8, 0);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_Lv);
    width = GFL_FontGetBlockWidth(str, wk->font, 0);
    DDemoObj_PrintFontOam(wk, DDEMO_FOAM_NAME, wk->font, str, 112, 0);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_Empty_2);
    WordSetNumber(wk->wordSet, 0, PML_PkmGetParam(pkm, PKM_PARAM_LEVEL, NULL), 3, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, str);
    DDemoObj_PrintFontOam(wk, DDEMO_FOAM_NAME, wk->font, wk->strbuf, 112 + width, 0);
    GFL_StrBufFree(str);

    PML_PkmReEncrypt(pkm, wasEncrypted);
}

// The second scene's texts, hidden: the congratulations, and the trainer's name, ID and play time
static void DDemoObj_InitFontOam2nd(DDemoWork *wk) {
    BmpOamActorSetup setup;
    DDemoFontOam *fontOam;
    StrBuf *str;
    u32 width;

    setup.palette = wk->palRes[DDEMO_PAL_FONT];
    setup.paletteOffset = 0;
    setup.priority = 4;
    setup.bgPriority = 0;
    setup.surface = 0xffff;
    setup.vramType = CLACT_VRAM_MAIN;

    fontOam = &wk->fontOam[DDEMO_FOAM_TITLE_2ND];
    fontOam->bmp = GFL_BitmapCreate(32, 2, BGSYS_TILE_SIZE_16, HEAPID_DENDOU_DEMO);
    setup.bitmap = fontOam->bmp;
    setup.x = 0;
    setup.y = 16;
    wk->fontOam[DDEMO_FOAM_TITLE_2ND].oam = BmpOam_ActorAdd(wk->bmpOam, &setup);
    BmpOam_ActorSetDrawEnable(wk->fontOam[DDEMO_FOAM_TITLE_2ND].oam, FALSE);
    GFL_BitmapFill(wk->fontOam[DDEMO_FOAM_TITLE_2ND].bmp, 0);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_LeagueChampionCongratulations);
    width = GFL_FontGetBlockWidth(str, wk->font, 0);
    DDemoObj_PrintFontOam(wk, DDEMO_FOAM_TITLE_2ND, wk->font, str, (256 - width) / 2, 0);
    GFL_StrBufFree(str);

    fontOam = &wk->fontOam[DDEMO_FOAM_PLAYER];
    fontOam->bmp = GFL_BitmapCreate(32, 2, BGSYS_TILE_SIZE_16, HEAPID_DENDOU_DEMO);
    setup.bitmap = fontOam->bmp;
    setup.x = 0;
    setup.y = 160;
    wk->fontOam[DDEMO_FOAM_PLAYER].oam = BmpOam_ActorAdd(wk->bmpOam, &setup);
    BmpOam_ActorSetDrawEnable(wk->fontOam[DDEMO_FOAM_PLAYER].oam, FALSE);
    GFL_BitmapFill(wk->fontOam[DDEMO_FOAM_PLAYER].bmp, 0);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_Empty_3);
    copyVarForText(wk->wordSet, 0, wk->param->playerInfo);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, str);
    DDemoObj_PrintFontOam(wk, DDEMO_FOAM_PLAYER, wk->font, wk->strbuf, 8, 0);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_IdNo);
    WordSetNumber(wk->wordSet, 0, getTrainerID(wk->param->playerInfo), 5, 2, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, str);
    DDemoObj_PrintFontOam(wk, DDEMO_FOAM_PLAYER, wk->font, wk->strbuf, 92, 0);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlMain2_Text_Empty_4);
    WordSetNumber(wk->wordSet, 0, func_02008cec(wk->param->playTime), 3, 0, TRUE);
    WordSetNumber(wk->wordSet, 1, func_02008cf0(wk->param->playTime), 2, 2, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, str);
    DDemoObj_PrintFontOam(wk, DDEMO_FOAM_PLAYER, wk->font, wk->strbuf, 200, 0);
    GFL_StrBufFree(str);
}

void DDemoObj_MoveFontOam(DDemoWork *wk) {
    s16 x, y;

    DDemoObj_GetPos(wk, DDEMOOBJ_PLATE_TITLE, &x, &y);
    BmpOam_ActorSetPos(wk->fontOam[DDEMO_FOAM_TITLE].oam, x - 176, y - 8);
    DDemoObj_GetPos(wk, DDEMOOBJ_PLATE_NAME, &x, &y);
    BmpOam_ActorSetPos(wk->fontOam[DDEMO_FOAM_NAME].oam, x, y - 8);
}

void DDemoObj_SetFontOamVisible(DDemoWork *wk, u32 id, BOOL visible) {
    BmpOam_ActorSetDrawEnable(wk->fontOam[id].oam, visible);
}

// Loads the main screen's actors' graphics and the font's palette
static void DDemoObj_LoadResources(DDemoWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_DENDOU_DEMO, HEAPID_TAIL(HEAPID_DENDOU_DEMO));

    wk->chrRes[DDEMO_CHR_OBJ] = func_0204b81c(arc, NARC_DDEMO_OBJ_1ST_NCGR, FALSE, CLACT_VRAM_MAIN, HEAPID_DENDOU_DEMO);
    wk->palRes[DDEMO_PAL_OBJ] = func_0204bba0(arc, NARC_DDEMO_OBJ_NCLR, CLACT_VRAM_MAIN, 0, HEAPID_DENDOU_DEMO);
    wk->celRes[DDEMO_CEL_OBJ] =
        func_0204bde0(arc, NARC_DDEMO_OBJ_1ST_NCER, NARC_DDEMO_OBJ_1ST_NANR, HEAPID_DENDOU_DEMO);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, HEAPID_TAIL(HEAPID_DENDOU_DEMO));
    wk->palRes[DDEMO_PAL_FONT] = func_0204bc48(arc, 5, CLACT_VRAM_MAIN, 0xa0, HEAPID_DENDOU_DEMO);
    GFL_ArcToolFree(arc);
}

void DDemoObj_Init1st(DDemoWork *wk) {
    DDemoObj_LoadResources(wk);
    DDemoObj_LoadTypePalette(wk);
    wk->clact[DDEMOOBJ_PLATE_TITLE] = DDemoObj_Add(wk, &sDDemoObjPlateTitle);
    wk->clact[DDEMOOBJ_PLATE_NAME] = DDemoObj_Add(wk, &sDDemoObjPlateName);
    wk->clact[DDEMOOBJ_FLASH] = DDemoObj_Add(wk, &sDDemoObjFlash);
    DDemoObj_SetVisible(wk, DDEMOOBJ_FLASH, FALSE);
    DDemoObj_InitFontOam1st(wk);
}

void DDemoObj_Exit1st(DDemoWork *wk) {
    DDemoObj_DeleteFontOamAll(wk);
    DDemoObj_DeleteAll(wk);
    DDemoObj_FreeResources(wk);
}

// Shows the actors on the frames that draw the main screen
void DDemoObj_Main1st(DDemoWork *wk) {
    if (GFL_G3DDual3DGetRenderDisp() == TRUE) {
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, FALSE);
    } else {
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    }
    DDemoObj_Animate(wk);
}

// The second scene's actors, on a renderer of their own whose sub screen is 512 below the main one
void DDemoObj_Init2nd(DDemoWork *wk) {
    ArcTool *arc;
    u32 trainerClass;

    DDemoObj_LoadResources(wk);
    {
        ClActSurfaceSetup surfaces[] = {
            { 0, 0, 256, 192, CLACT_VRAM_MAIN, TRUE },
            { 0, 512, 256, 192, CLACT_VRAM_SUB, TRUE },
        };

        wk->renderer = func_0204be9c(surfaces, NELEMS(surfaces), HEAPID_DENDOU_DEMO);
    }
    func_0204bf14(wk->renderer, TRUE);
    func_0204c018(wk->clUnit, wk->renderer);
    wk->clact[DDEMOOBJ_PLATE_UPPER] = DDemoObj_Add(wk, &sDDemoObjPlateUpper);
    wk->clact[DDEMOOBJ_PLATE_LOWER] = DDemoObj_Add(wk, &sDDemoObjPlateLower);
    DDemoObj_SetVisible(wk, DDEMOOBJ_PLATE_UPPER, FALSE);
    DDemoObj_SetVisible(wk, DDEMOOBJ_PLATE_LOWER, FALSE);

    arc = GFL_ArcSysCreateFileHandle(ARCID_DENDOU_DEMO, HEAPID_TAIL(HEAPID_DENDOU_DEMO));
    wk->chrRes[DDEMO_CHR_OBJ_SUB] =
        func_0204b81c(arc, NARC_DDEMO_OBJ_2ND_NCGR, FALSE, CLACT_VRAM_SUB, HEAPID_DENDOU_DEMO);
    wk->palRes[DDEMO_PAL_OBJ_SUB] = func_0204bba0(arc, NARC_DDEMO_OBJ_NCLR, CLACT_VRAM_SUB, 0, HEAPID_DENDOU_DEMO);
    wk->celRes[DDEMO_CEL_OBJ_SUB] =
        func_0204bde0(arc, NARC_DDEMO_OBJ_2ND_NCER, NARC_DDEMO_OBJ_2ND_NANR, HEAPID_DENDOU_DEMO);
    GFL_ArcToolFree(arc);

    trainerClass = TRAINER_CLASS_PKMN_TRAINER_ROSA;
    if (getTrainerGender(wk->param->playerInfo) == GENDER_MALE) {
        trainerClass = TRAINER_CLASS_PKMN_TRAINER_NATE;
    }
    arc = MakeTrGraArcHandle(HEAPID_TAIL(HEAPID_DENDOU_DEMO));
    wk->chrRes[DDEMO_CHR_TRAINER_MAIN] = TrGra_LoadClActChars(arc, trainerClass, CLACT_VRAM_MAIN, HEAPID_DENDOU_DEMO);
    wk->palRes[DDEMO_PAL_TRAINER_MAIN] =
        TrGra_LoadClActPalette(arc, trainerClass, CLACT_VRAM_MAIN, 0xc0, HEAPID_DENDOU_DEMO);
    wk->celRes[DDEMO_CEL_TRAINER_MAIN] = TrGra_LoadClActCellAnims(trainerClass, 2, CLACT_VRAM_MAIN, HEAPID_DENDOU_DEMO);
    wk->chrRes[DDEMO_CHR_TRAINER_SUB] = TrGra_LoadClActChars(arc, trainerClass, CLACT_VRAM_SUB, HEAPID_DENDOU_DEMO);
    wk->palRes[DDEMO_PAL_TRAINER_SUB] =
        TrGra_LoadClActPalette(arc, trainerClass, CLACT_VRAM_SUB, 0x60, HEAPID_DENDOU_DEMO);
    wk->celRes[DDEMO_CEL_TRAINER_SUB] = TrGra_LoadClActCellAnims(trainerClass, 0, CLACT_VRAM_SUB, HEAPID_DENDOU_DEMO);
    GFL_ArcToolFree(arc);
    wk->clact[DDEMOOBJ_TRAINER_MAIN] = DDemoObj_Add(wk, &sDDemoObjTrainerMain);
    wk->clact[DDEMOOBJ_TRAINER_SUB] = DDemoObj_Add(wk, &sDDemoObjTrainerSub);
    DDemoObj_InitFontOam2nd(wk);
}

void DDemoObj_Exit2nd(DDemoWork *wk) {
    DDemoObj_DeleteFontOamAll(wk);
    DDemoObj_DeleteAll(wk);
    func_0204becc(wk->renderer);
    DDemoObj_FreeResources(wk);
}

void DDemoObj_Main2nd(DDemoWork *wk) {
    DDemoObj_Animate(wk);
}

void DDemoObj_MainConfetti(DDemoWork *wk) {
    s16 x, y;
    u16 i;
    u16 rand;

    for (i = DDEMOOBJ_CONFETTI; i < DDEMOOBJ_COUNT; i++) {
        if (wk->clact[i] != NULL) {
            if (DDemoObj_IsAnimating(wk, i) != TRUE) {
                DDemoObj_Delete(wk, i);
            }
        }
    }
    rand = GFL_RandomLC(4);
    if (rand < 2) {
        for (i = DDEMOOBJ_CONFETTI; i < DDEMOOBJ_COUNT; i++) {
            if (wk->clact[i] == NULL) {
                DDemoObjSetup setup;

                x = GFL_RandomLC(256);
                y = GFL_RandomLC(216);
                if (y < 192) {
                    setup = sDDemoObjConfettiMain;
                    setup.setup.x = x;
                    setup.setup.y = y;
                } else {
                    setup = sDDemoObjConfettiSub;
                    setup.setup.x = x;
                    setup.setup.y = y - 24;
                }
                wk->clact[i] = DDemoObj_Add(wk, &setup);
                DDemoObj_SetAnime(wk, i, rand + 4);
                break;
            }
        }
    }
}
