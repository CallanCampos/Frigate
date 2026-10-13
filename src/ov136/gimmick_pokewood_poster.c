// The movie posters of Pokéstar Studios (Pokewood). The file's name is descriptive: the overlay has no name string.
// A model with four poster textures, which show the series of the last four movies filmed, as the save keeps them
#include "types.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_pokewood_poster.h"
#include "gfl/arc_util.h"
#include "gfl/bmp.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nnsys/g3d.h"
#include "nnsys/gfd.h"
#include "save/pokewood.h"
#include "system/game_system.h"

#define POSTER_COUNT 4

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
} GimmickWork;

static void func_ov136_021eece0(GimmickWork *work);
static void func_ov136_021eed38(GimmickWork *work);
static void func_ov136_021eed44(GimmickWork *work);
static void func_ov136_021eed50(GimmickWork *work, u32 poster);
static void func_ov136_021eedc0(void *texResource, const char *texName, const char *plttName, u32 image,
                                HeapID heapId);
static u32 func_ov136_021eee94(const NNSG3dResTex *tex, const NNSG3dResDictTexData *texData);
static u32 func_ov136_021eeea4(const NNSG3dResTex *tex, const NNSG3dResDictPlttData *plttData);
static void func_ov136_021eeeb4(NNSG3dResName *resName, const char *name);
static u32 func_ov136_021eeee8(const NNSG3dResTex *tex, const char *name);
static u32 func_ov136_021eef20(const NNSG3dResTex *tex, const char *name);

// The poster model, from archive 0x125
static const G3DSceneResourceSetup sResources[] = { { 0x125, 0, 0 } };

static const G3DSceneActorSetup sActors[] = { { 0, 0, 0, 0, NULL, 0 } };

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

// The textures and palettes of the posters, newest movie first
static const char *sPosterTexNames[POSTER_COUNT] = { "c15_poster1", "c15_poster2", "c15_poster3", "c15_poster4" };
static const char *sPosterPlttNames[POSTER_COUNT] = { "c15_poster1_pl", "c15_poster2_pl", "c15_poster3_pl",
                                                      "c15_poster4_pl" };

void func_ov136_021eec80(Field *field) {
    int i;
    GimmickWork *work = Field_AllocGimmickWorkBlock(field, 1, Field_GetHeapID(field), sizeof(GimmickWork));

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    func_ov136_021eece0(work);
    for (i = 0; i < POSTER_COUNT; i++) {
        func_ov136_021eed50(work, i);
    }
}

void func_ov136_021eecb8(Field *field) {
    func_ov136_021eed38(Field_GetGimmickWorkBlock(field, 1));
    Field_DeleteGimmickWorkBlock(field, 1);
}

void func_ov136_021eecd0(Field *field) {
    func_ov136_021eed44(Field_GetGimmickWorkBlock(field, 1));
}

static void func_ov136_021eece0(GimmickWork *work) {
    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    {
        VecFx32 position = { FX32_CONST(728), FX32_CONST(16), FX32_CONST(288) };
        SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, 0);

        matrix->translation.x = position.x;
        matrix->translation.y = position.y;
        matrix->translation.z = position.z;
    }
    func_ov036_021b8248(work->expObj, 0, 0, 1);
    FieldExpObj_SetActorHidden(work->expObj, 0, 0, FALSE);
}

static void func_ov136_021eed38(GimmickWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void func_ov136_021eed44(GimmickWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

static void func_ov136_021eed50(GimmickWork *work, u32 poster) {
    HeapID heapId = HEAPID_TAIL(Field_GetHeapID(work->field));
    PokewoodSave *save = func_02011040(GSYS_GetGameData(Field_GetGameSystem(work->field)));
    void *texResource =
        GFL_G3DMdlGetTexResource(GFL_G3DActorGetMdl(FieldExpObj_GetActor(work->expObj, 0, 0)));
    int series = func_02011270(save, poster);

    if (series > 0) {
        func_ov136_021eedc0(texResource, sPosterTexNames[poster], sPosterPlttNames[poster], series - 1, heapId);
    }
}

static void func_ov136_021eedc0(void *texResource, const char *texName, const char *plttName, u32 image,
                                HeapID heapId) {
    // A poster's size in tiles and a tile's in pixels. The reads are folded, but a function's constants stay in
    // .rodata, as three bytes of 8; which 8 is which is a guess
    static const u8 sTileWidth = 8;
    static const u8 sTileHeight = 8;
    static const u8 sTilePixels = 8;
    NNSGfdTexKey texKey = GFL_G3DResGetTexVRAMHandle(texResource);
    NNSGfdPlttKey plttKey = GFL_G3DResGetPltVRAMHandle(texResource);
    NNSG3dResTex *tex = NNS_G3DResGetTexBlock(GFL_G3DResGetResData(texResource));
    u32 texAddr = func_ov136_021eeee8(tex, texName);
    u32 plttAddr = func_ov136_021eef20(tex, plttName);
    NNSG2dCharacterData *character = NULL;
    NNSG2dPaletteData *palette = NULL;
    void *characterFile = GFL_G2DIOReadBGNCGR(0x126, image * 2 + 1, TRUE, &character, heapId);
    void *paletteFile = GFL_G2DIOReadNCLR(0x126, image * 2, &palette, heapId);
    GFLBitmap *bitmap =
        GFL_BitmapWrap(character->rawData, sTileWidth, sTileHeight, sTilePixels * sTilePixels / 2, heapId);

    GFL_BitmapMakeLinear(bitmap, FALSE, heapId);

    gfxBeginTextureUpload();
    cp15_flushDC(character->rawData, 0x800);
    gfxUploadTexture(character->rawData, texAddr, 0x800);
    gfxEndTextureUpload();

    gfxBeginPaletteUpload();
    cp15_flushDC(palette->rawData, 0x20);
    gfxUploadPalette(palette->rawData, plttAddr, 0x20);
    gfxEndPaletteUpload();

    GFL_HeapFree(characterFile);
    GFL_HeapFree(paletteFile);
    GFL_BitmapFree(bitmap);
}

static u32 func_ov136_021eee94(const NNSG3dResTex *tex, const NNSG3dResDictTexData *texData) {
    return ((texData->texImageParam & 0xffff) << 3) + NNS_GfdGetTexKeyAddr(tex->texInfo.vramKey);
}

static u32 func_ov136_021eeea4(const NNSG3dResTex *tex, const NNSG3dResDictPlttData *plttData) {
    return (plttData->offset << 3) + NNS_GfdGetPlttKeyAddr(tex->plttInfo.vramKey);
}

static void func_ov136_021eeeb4(NNSG3dResName *resName, const char *name) {
    int len = NNS_STD_StrLen(name);
    u8 i;

    for (i = 0; i < 4; i++) {
        resName->val[i] = 0;
    }
    for (i = 0; i < len; i++) {
        resName->name[i] = name[i];
    }
}

static u32 func_ov136_021eeee8(const NNSG3dResTex *tex, const char *name) {
    NNSG3dResName resName;
    const NNSG3dResDictTexData *texData;

    func_ov136_021eeeb4(&resName, name);
    if (tex != NULL) {
        texData = NNS_G3DFind(&tex->dict, &resName);
    } else {
        texData = NULL;
    }
    if (texData == NULL) {
        return 0;
    }
    return func_ov136_021eee94(tex, texData);
}

static u32 func_ov136_021eef20(const NNSG3dResTex *tex, const char *name) {
    NNSG3dResName resName;
    const NNSG3dResDictPlttData *plttData;

    func_ov136_021eeeb4(&resName, name);
    if (tex != NULL && tex->plttInfo.ofsDict != 0) {
        plttData = NNS_G3DFind((const NNSG3dResDict *)((u8 *)tex + tex->plttInfo.ofsDict), &resName);
    } else {
        plttData = NULL;
    }
    if (plttData == NULL) {
        return 0;
    }
    return func_ov136_021eeea4(tex, plttData);
}
