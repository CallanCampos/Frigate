#include "app/error_window_screen.h"
#include "types.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"

// A screen with the window of save_error.c's graphics on BG2 of the main screen and no text, faded in from white and
// shown until the game is reset. Nothing in the game loads the overlay, so it may be a leftover. The file's name and
// all the names are ours, guessed from what it shows: the ROM has no string for it

// The archive of the window's graphics (save_error.c's), and its files
#define ARCID_ERROR_WINDOW_GRA 22
#define ERROR_WINDOW_GRA_PLTT 0
#define ERROR_WINDOW_GRA_CHAR 1
#define ERROR_WINDOW_GRA_SCRN 2

// The BG the window is drawn on
#define WINDOW_BG 2

void ErrorWindowScreen_Show(void) {
    GFL_BGSysCreate(HEAPID_USER);
    {
        BGSysVRAMConfig vramConfig = {
            GX_VRAM_BG_128_A, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
            GX_VRAM_OBJ_64_E, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
            GX_VRAM_TEX_0_B,  GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
        };

        GFL_BGSysSetVRAMBanks(&vramConfig);
    }
    MIi_CpuClear32(0, (void *)HW_BG_VRAM, HW_BG_VRAM_SIZE);
    MIi_CpuClear32(0, (void *)HW_DB_BG_VRAM, HW_DB_BG_VRAM_SIZE);
    MIi_CpuClear32(0, (void *)HW_OBJ_VRAM, HW_OBJ_VRAM_SIZE);
    MIi_CpuClear32(0, (void *)HW_DB_OBJ_VRAM, HW_DB_OBJ_VRAM_SIZE);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&lcdConfig);
    }
    {
        BGSetup bgSetups[] = {
            { 0, 0, 0x2000, 0, BGRES_512x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x08000),
              0x8000, GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
            { 0, 0, 0x2000, 0, BGRES_512x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000),
              0x8000, GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
            { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
              GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
            { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x08000), 0x8000,
              GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
        };

        GFL_BGSysCreateBG(WINDOW_BG, &bgSetups[0], BGMODE_TEXT);
        GFL_BGSysClearScr(WINDOW_BG);
        GFL_BGSysCreateBG(3, &bgSetups[1], BGMODE_TEXT);
        GFL_BGSysClearScr(3);
        GFL_BGSysCreateBG(BGSYS_BG_SUB + 2, &bgSetups[2], BGMODE_TEXT);
        GFL_BGSysClearScr(BGSYS_BG_SUB + 2);
        GFL_BGSysCreateBG(BGSYS_BG_SUB + 3, &bgSetups[3], BGMODE_TEXT);
        GFL_BGSysClearScr(BGSYS_BG_SUB + 3);
    }

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, TRUE);
    GX_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_32K);
    GXS_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_32K);

    GFL_BGSysLoadNCGRStatic(ARCID_ERROR_WINDOW_GRA, ERROR_WINDOW_GRA_CHAR, WINDOW_BG, 0, 0, FALSE, HEAPID_USER);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_ERROR_WINDOW_GRA, ERROR_WINDOW_GRA_SCRN, WINDOW_BG, 0, 0, FALSE,
                                           HEAPID_USER);
    GFL_BGSysLoadNCLRDefault(ARCID_ERROR_WINDOW_GRA, ERROR_WINDOW_GRA_PLTT, PALTYPE_MAIN_BG, 0, 0x100, HEAPID_USER);
    GFL_BGSysEnableEngines();
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_SUB_MAIN);
    GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 16, 0, 2);

    while (TRUE) {
        GCTX_HIDUpdate();
        GFL_FadeUpdate();
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    }
}
