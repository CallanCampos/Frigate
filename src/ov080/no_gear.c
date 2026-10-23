#include "types.h"
#include "constants/arc.h"
#include "field/no_gear.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "nitro/gx.h"
#include "save/player_info.h"
#include "system/game_data.h"
#include "system/game_system.h"

typedef void (*NoGearFunc)(NoGearWork *work);

struct NoGearWork {
    // Called each frame, once set
    NoGearFunc func;
    u32 unk_04;
    u16 heapId;
    u32 unk_0c;
    u32 unk_10;
    // The characters of BG 6, as GFL_BGSysLoadArcNCGRDynamic returns them
    u32 chars;
    GameSystem *gsys;
    u32 unk_1c;
};

static void NoGear_SetFuncCore(NoGearWork *work, NoGearFunc func, int line);
static void NoGear_SetFunc(NoGearWork *work, NoGearFunc func, int line);
static void NoGear_LoadGraphics(NoGearWork *work);
static void NoGear_SetupBGs(void);
static void NoGear_Init(NoGearWork *work);
static void NoGear_Release(NoGearWork *work);
static void NoGear_Idle(NoGearWork *work);

// The palette, by the player's gender
static const u32 sPaletteFileIds[2] = { 0, 1 };

static const BGSetup sBG6Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x7000),
    GX_BG_CHARBASE(0x00000),
    0x6000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup sBG5Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x7800),
    GX_BG_CHARBASE(0x00000),
    0x6000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup sBG4Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x6800),
    GX_BG_CHARBASE(0x00000),
    0x6000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

// The line is unused
static void NoGear_SetFuncCore(NoGearWork *work, NoGearFunc func, int line) {
    work->func = func;
}

static void NoGear_SetFunc(NoGearWork *work, NoGearFunc func, int line) {
    NoGear_SetFuncCore(work, func, line);
}

static void NoGear_LoadGraphics(NoGearWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_C_GEAR, work->heapId);
    u32 gender = getTrainerGender(GetGameDataPlayerInfo(GSYS_GetGameData(work->gsys)));

    GFL_G2DIOLoadArcNCLRDefault(arc, sPaletteFileIds[gender], PALTYPE_SUB_BG, 0, 0, work->heapId);
    work->chars = GFL_BGSysLoadArcNCGRDynamic(arc, 2, 6, 0, FALSE, work->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 3, 6, 0, (u16)work->chars, 0, FALSE, work->heapId);
    GFL_ArcToolFree(arc);
    GFL_NetSetIconScreen(FALSE, work->heapId);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void NoGear_SetupBGs(void) {
    int bg;

    for (bg = 4; bg <= 7; bg++) {
        GFL_BGSysSetBGEnabled(bg, FALSE);
    }

    {
        BGSetup setup = sBG6Setup;

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(6, TRUE);
        GFL_BGSysSetBGPriority(6, 3);
        GFL_BGSysFillScrArea(6, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(6);
    }
    {
        BGSetup setup = sBG5Setup;

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(5, TRUE);
        GFL_BGSysSetBGPriority(5, 1);
        GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(5);
    }
    {
        BGSetup setup = sBG4Setup;

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(4, TRUE);
        GFL_BGSysSetBGPriority(4, 2);
        GFL_BGSysFillScrArea(4, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(4);
    }
}

static void NoGear_Init(NoGearWork *work) {
    NoGear_SetupBGs();
    NoGear_LoadGraphics(work);
    NoGear_SetFunc(work, NoGear_Idle, 327);
}

static void NoGear_Release(NoGearWork *work) {
    GFL_BGSysFreeCharMemory(6, (u16)work->chars, (u16)(work->chars >> 16));
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
}

static void NoGear_Idle(NoGearWork *work) {
}

NoGearWork *NoGear_Create(CGearSettings *saveData, FieldSubscreen *subscreen, GameSystem *gsys) {
    NoGearWork *work = GFL_HeapAllocate(HEAPID_FIELDMAP, sizeof(NoGearWork), TRUE, "no_gear.c", 376);

    work->heapId = HEAPID_FIELDMAP;
    work->gsys = gsys;
    NoGear_Init(work);
    return work;
}

void NoGear_Update(NoGearWork *work) {
    if (work->func != NULL) {
        work->func(work);
    }
    // The connection state is fetched but not used
    WihDwc_GetStatus(GameData_GetWifiList(GSYS_GetGameData(work->gsys)));
}

void NoGear_Draw(NoGearWork *work) {
}

void NoGear_Free(NoGearWork *work) {
    NoGear_Release(work);
    G2S_BlendNone();
    GFL_HeapFree(work);
}
