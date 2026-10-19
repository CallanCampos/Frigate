#include "types.h"
#include "app/research_radar/palette_anime.h"
#include "app/research_radar/research_common.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/game_system.h"
#include "system/palanm.h"

#define OBJ_RES_COUNT 3
#define UNIT_COUNT 1
#define ACTOR_COUNT 1
#define ANIME_COUNT 1
#define TOUCH_RECT_COUNT 2

// The return button's resources in ARCID_APP_MENU_COMMON
enum {
    OBJ_RES_CHARS,
    OBJ_RES_PALETTE,
    OBJ_RES_CELL_ANIMS,
};

struct ResearchCommon {
    HeapID heapId;
    GameSystem *gsys;
    GameData *gameData;
    TouchRect touchRects[TOUCH_RECT_COUNT];
    u32 objRes[OBJ_RES_COUNT];
    ClActUnit *units[UNIT_COUNT];
    ClActor *actors[ACTOR_COUNT];
    PaletteFade *paletteFade;
    PaletteAnime *animes[ANIME_COUNT];
    u32 prevSeq;
    u32 seq;
    BOOL touchMode;
    BOOL forceExit;
};

static void ResearchCommon_InitWork(ResearchCommon *common);
static ResearchCommon *ResearchCommon_Alloc(HeapID heapId);
static void ResearchCommon_Free(ResearchCommon *common);
static void ResearchCommon_Setup(ResearchCommon *common, GameSystem *gsys, HeapID heapId);
static void ResearchCommon_Teardown(ResearchCommon *common);
static void ResearchCommon_InitClAct(ResearchCommon *common);
static void ResearchCommon_ExitClAct(ResearchCommon *common);
static void ResearchCommon_ClearObjRes(ResearchCommon *common);
static void ResearchCommon_LoadObjRes(ResearchCommon *common);
static void ResearchCommon_FreeObjRes(ResearchCommon *common);
static void ResearchCommon_ClearUnits(ResearchCommon *common);
static void ResearchCommon_CreateUnits(ResearchCommon *common);
static void ResearchCommon_DeleteUnits(ResearchCommon *common);
static void ResearchCommon_ClearActors(ResearchCommon *common);
static void ResearchCommon_CreateActors(ResearchCommon *common);
static void ResearchCommon_DeleteActors(ResearchCommon *common);
static void ResearchCommon_ClearPaletteFade(ResearchCommon *common);
static void ResearchCommon_CreatePaletteFade(ResearchCommon *common);
static void ResearchCommon_DeletePaletteFade(ResearchCommon *common);
static void ResearchCommon_SetupPaletteFade(ResearchCommon *common);
static void ResearchCommon_FreePaletteFade(ResearchCommon *common);
static void ResearchCommon_ClearPaletteAnimes(ResearchCommon *common);
static void ResearchCommon_CreatePaletteAnimes(ResearchCommon *common);
static void ResearchCommon_DeletePaletteAnimes(ResearchCommon *common);
static void ResearchCommon_SetupPaletteAnimes(ResearchCommon *common);
static void ResearchCommon_RestorePalettes(ResearchCommon *common);
static void ResearchCommon_StartPaletteAnimeCore(ResearchCommon *common, u32 index);
static void ResearchCommon_StopPaletteAnimeCore(ResearchCommon *common, u32 index);
static void ResearchCommon_RestorePaletteCore(ResearchCommon *common, u32 index);
static void ResearchCommon_UpdatePaletteAnimes(ResearchCommon *common);
static void ResearchCommon_InitTouchRects(ResearchCommon *common);
static u32 ResearchCommon_GetObjRes(ResearchCommon *common, u32 index);
static ClActUnit *ResearchCommon_GetUnit(ResearchCommon *common, u32 index);

// Nothing reads it. It is a global, since an unreferenced static would not be emitted; as the smallest object, its
// section of its own comes first in the file's .rodata
const u32 ResearchCommon_Unused = 0x00010002;

static const ResearchRect sCommonTouchRects[TOUCH_RECT_COUNT] = {
    { 216, 240, 168, 192 },
#ifdef BUGFIX
    { 0, 0, TOUCH_RECT_END, 0 },
#else
    // BUG: As in research_top.c, the end of the table is written as a TouchRect, so the copy in the work has no end
    { TOUCH_RECT_END, 0, 0, 0 },
#endif
};

static const ResearchPaletteAnimeSetup sCommonPaletteAnimes[ANIME_COUNT] = {
    { (u16 *)HW_OBJ_PLTT, (const u16 *)HW_OBJ_PLTT, 16, 6, 0xffff },
};

static const ClActSysSetup sCommonClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 256, 32, 32, 32, 16, 16 };

// The return button
static const ResearchActorSetup sCommonActors[ACTOR_COUNT] = {
    { 216, 168, 1, 3, 1, 0, OBJ_RES_CHARS, OBJ_RES_PALETTE, OBJ_RES_CELL_ANIMS, 0 },
};

static const BGSysVRAMConfig sCommonVRAMConfig = {
    GX_VRAM_BG_128_A, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_B,  GX_VRAM_TEXPLTT_0_G,     GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

ResearchCommon *ResearchCommon_Create(HeapID heapId, GameSystem *gsys) {
    ResearchCommon *common = ResearchCommon_Alloc(heapId);

    ResearchCommon_InitWork(common);
    ResearchCommon_Setup(common, gsys, heapId);
    return common;
}

void ResearchCommon_Delete(ResearchCommon *common) {
    ResearchCommon_Teardown(common);
    ResearchCommon_Free(common);
}

HeapID ResearchCommon_GetHeapID(ResearchCommon *common) {
    return common->heapId;
}

GameSystem *ResearchCommon_GetGameSystem(ResearchCommon *common) {
    return common->gsys;
}

GameData *ResearchCommon_GetGameData(ResearchCommon *common) {
    return common->gameData;
}

const TouchRect *ResearchCommon_GetTouchRects(ResearchCommon *common) {
    return common->touchRects;
}

void ResearchCommon_SetSeq(ResearchCommon *common, u32 seq) {
    common->prevSeq = common->seq;
    common->seq = seq;
}

void ResearchCommon_SetTouchMode(ResearchCommon *common, BOOL touch) {
    common->touchMode = touch;
}

u32 ResearchCommon_GetPrevSeq(ResearchCommon *common) {
    return common->prevSeq;
}

BOOL ResearchCommon_GetTouchMode(ResearchCommon *common) {
    return common->touchMode;
}

void ResearchCommon_UpdatePaletteAnime(ResearchCommon *common) {
    ResearchCommon_UpdatePaletteAnimes(common);
}

void ResearchCommon_StartPaletteAnime(ResearchCommon *common, u32 index) {
    ResearchCommon_StartPaletteAnimeCore(common, index);
}

void ResearchCommon_StopPaletteAnime(ResearchCommon *common) {
    ResearchCommon_StopPaletteAnimeCore(common, 0);
}

void ResearchCommon_RestorePalette(ResearchCommon *common) {
    ResearchCommon_RestorePaletteCore(common, 0);
}

BOOL ResearchCommon_IsForceExit(ResearchCommon *common) {
    return common->forceExit;
}

void ResearchCommon_SetForceExit(ResearchCommon *common) {
    common->forceExit = TRUE;
}

static void ResearchCommon_InitWork(ResearchCommon *common) {
    MI_CpuFill8(common, 0, sizeof(ResearchCommon));
    common->prevSeq = RESEARCH_SEQ_INIT;
    common->seq = RESEARCH_SEQ_INIT;
    ResearchCommon_ClearObjRes(common);
    ResearchCommon_ClearUnits(common);
    ResearchCommon_ClearActors(common);
    ResearchCommon_ClearPaletteFade(common);
    ResearchCommon_ClearPaletteAnimes(common);
}

static ResearchCommon *ResearchCommon_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(ResearchCommon), FALSE, "research_common.c", 481);
}

static void ResearchCommon_Free(ResearchCommon *common) {
    GFL_HeapFree(common);
}

static void ResearchCommon_Setup(ResearchCommon *common, GameSystem *gsys, HeapID heapId) {
    common->heapId = heapId;
    common->gsys = gsys;
    common->gameData = GSYS_GetGameData(gsys);
    ResearchCommon_InitClAct(common);
    ResearchCommon_LoadObjRes(common);
    ResearchCommon_CreateUnits(common);
    ResearchCommon_CreateActors(common);
    ResearchCommon_CreatePaletteFade(common);
    ResearchCommon_SetupPaletteFade(common);
    ResearchCommon_InitTouchRects(common);
    ResearchCommon_CreatePaletteAnimes(common);
    ResearchCommon_SetupPaletteAnimes(common);
}

static void ResearchCommon_Teardown(ResearchCommon *common) {
    ResearchCommon_RestorePalettes(common);
    ResearchCommon_DeletePaletteAnimes(common);
    ResearchCommon_FreePaletteFade(common);
    ResearchCommon_DeletePaletteFade(common);
    ResearchCommon_DeleteActors(common);
    ResearchCommon_DeleteUnits(common);
    ResearchCommon_FreeObjRes(common);
    ResearchCommon_ExitClAct(common);
}

static void ResearchCommon_InitClAct(ResearchCommon *common) {
    ClActSys_Create(&sCommonClActSetup, &sCommonVRAMConfig, common->heapId);
}

static void ResearchCommon_ExitClAct(ResearchCommon *common) {
    ClActSys_Delete();
}

static void ResearchCommon_ClearObjRes(ResearchCommon *common) {
    int i;

    for (i = 0; i < OBJ_RES_COUNT; i++) {
        common->objRes[i] = 0;
    }
}

static void ResearchCommon_LoadObjRes(ResearchCommon *common) {
    HeapID heapId = common->heapId;
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_TAIL(heapId));
    u32 chars = ClActSys_LoadChars(handle, 20, FALSE, CLACT_VRAM_MAIN, heapId);
    u32 palette = ClActSys_LoadPlttRange(handle, 19, CLACT_VRAM_MAIN, 0, 0, 4, heapId);
    u32 cellAnims = ClActSys_LoadCellAnims(handle, 21, 24, heapId);

    common->objRes[OBJ_RES_CHARS] = chars;
    common->objRes[OBJ_RES_PALETTE] = palette;
    common->objRes[OBJ_RES_CELL_ANIMS] = cellAnims;
    GFL_ArcToolFree(handle);
}

static void ResearchCommon_FreeObjRes(ResearchCommon *common) {
    ClActSys_FreeChars(common->objRes[OBJ_RES_CHARS]);
    ClActSys_FreePltt(common->objRes[OBJ_RES_PALETTE]);
    ClActSys_FreeCellAnims(common->objRes[OBJ_RES_CELL_ANIMS]);
}

static void ResearchCommon_ClearUnits(ResearchCommon *common) {
    common->units[0] = NULL;
}

static void ResearchCommon_CreateUnits(ResearchCommon *common) {
    common->units[0] = ClActUnit_Create(1, 2, common->heapId);
}

static void ResearchCommon_DeleteUnits(ResearchCommon *common) {
    ClActUnit_Delete(common->units[0]);
}

static void ResearchCommon_ClearActors(ResearchCommon *common) {
    common->actors[0] = NULL;
}

static void ResearchCommon_CreateActors(ResearchCommon *common) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        const ResearchActorSetup *entry = &sCommonActors[i];
        ClActorSetup setup;
        ClActUnit *unit;
        u32 chars;
        u32 palette;
        u32 cellAnims;

        setup.x = entry->x;
        setup.y = entry->y;
        setup.sequence = entry->sequence;
        setup.priority = entry->priority;
        setup.bgPriority = entry->bgPriority;
        unit = ResearchCommon_GetUnit(common, entry->unit);
        chars = ResearchCommon_GetObjRes(common, entry->chars);
        palette = ResearchCommon_GetObjRes(common, entry->palette);
        cellAnims = ResearchCommon_GetObjRes(common, entry->cellAnims);
        common->actors[i] = ClActor_Create(unit, chars, palette, cellAnims, &setup, entry->surface, common->heapId);
        ClActor_SetVisible(common->actors[i], TRUE);
    }
}

static void ResearchCommon_DeleteActors(ResearchCommon *common) {
    ClActor_Delete(common->actors[0]);
}

static void ResearchCommon_ClearPaletteFade(ResearchCommon *common) {
    common->paletteFade = NULL;
}

static void ResearchCommon_CreatePaletteFade(ResearchCommon *common) {
    common->paletteFade = PaletteFade_Create(common->heapId);
}

static void ResearchCommon_DeletePaletteFade(ResearchCommon *common) {
    PaletteFade_Free(common->paletteFade);
}

static void ResearchCommon_SetupPaletteFade(ResearchCommon *common) {
    PaletteFade *fade = common->paletteFade;

    PaletteFade_AllocBuffer(fade, PALFADE_BUFFER_MAIN_BG, 0x200, common->heapId);
    PaletteFade_AllocBuffer(fade, PALFADE_BUFFER_MAIN_OBJ, 0x200, common->heapId);
    PaletteFade_AllocBuffer(fade, PALFADE_BUFFER_SUB_BG, 0x200, common->heapId);
    PaletteFade_AllocBuffer(fade, PALFADE_BUFFER_SUB_OBJ, 0x200, common->heapId);
    PaletteFade_LoadFromVRAM(fade, PALFADE_VRAM_MAIN_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(fade, PALFADE_VRAM_MAIN_OBJ, 0, 0x200);
    PaletteFade_LoadFromVRAM(fade, PALFADE_VRAM_SUB_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(fade, PALFADE_VRAM_SUB_OBJ, 0, 0x200);
}

static void ResearchCommon_FreePaletteFade(ResearchCommon *common) {
    PaletteFade *fade = common->paletteFade;

    PaletteFade_FreeBuffer(fade, PALFADE_BUFFER_MAIN_BG);
    PaletteFade_FreeBuffer(fade, PALFADE_BUFFER_MAIN_OBJ);
    PaletteFade_FreeBuffer(fade, PALFADE_BUFFER_SUB_BG);
    PaletteFade_FreeBuffer(fade, PALFADE_BUFFER_SUB_OBJ);
}

static void ResearchCommon_ClearPaletteAnimes(ResearchCommon *common) {
    common->animes[0] = NULL;
}

static void ResearchCommon_CreatePaletteAnimes(ResearchCommon *common) {
    common->animes[0] = PaletteAnime_Create(common->heapId);
}

static void ResearchCommon_DeletePaletteAnimes(ResearchCommon *common) {
    PaletteAnime_Delete(common->animes[0]);
}

static void ResearchCommon_SetupPaletteAnimes(ResearchCommon *common) {
    int i;

    for (i = 0; i < ANIME_COUNT; i++) {
        PaletteAnime_Setup(common->animes[i], sCommonPaletteAnimes[i].dst, sCommonPaletteAnimes[i].src,
                           sCommonPaletteAnimes[i].count);
    }
}

static void ResearchCommon_RestorePalettes(ResearchCommon *common) {
    PaletteAnime_Restore(common->animes[0]);
}

static void ResearchCommon_StartPaletteAnimeCore(ResearchCommon *common, u32 index) {
    PaletteAnime_Start(common->animes[index], sCommonPaletteAnimes[index].mode, sCommonPaletteAnimes[index].color);
}

static void ResearchCommon_StopPaletteAnimeCore(ResearchCommon *common, u32 index) {
    PaletteAnime_Stop(common->animes[index]);
}

static void ResearchCommon_RestorePaletteCore(ResearchCommon *common, u32 index) {
    PaletteAnime_Restore(common->animes[index]);
}

static void ResearchCommon_UpdatePaletteAnimes(ResearchCommon *common) {
    PaletteAnime_Update(common->animes[0]);
}

static void ResearchCommon_InitTouchRects(ResearchCommon *common) {
    int i;

    for (i = 0; i < TOUCH_RECT_COUNT; i++) {
        common->touchRects[i].left = sCommonTouchRects[i].left;
        common->touchRects[i].right = sCommonTouchRects[i].right;
        common->touchRects[i].top = sCommonTouchRects[i].top;
        common->touchRects[i].bottom = sCommonTouchRects[i].bottom;
    }
}

static u32 ResearchCommon_GetObjRes(ResearchCommon *common, u32 index) {
    return common->objRes[index];
}

static ClActUnit *ResearchCommon_GetUnit(ResearchCommon *common, u32 index) {
    return common->units[index];
}
