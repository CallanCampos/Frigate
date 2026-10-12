#include "types.h"
#include "app/codein.h"
#include "app/codein_pv.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/codein_param.h"
#include "system/gf_font.h"
#include "system/palanm.h"

// The code input screen's process. The ROM has no name for this file; "codein.c" is a guess from pokeplatinum, whose
// code input screen has the same three files. Our names

// The archive of the screen's graphics
#define ARCID_CODEIN 58
#define FRIEND_CODE_LENGTH 12

static BOOL CodeIn_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CodeIn_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CodeIn_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static void CodeIn_InitBG(void);
static void CodeIn_VBlank(TCB *tcb, void *data);

const GameProcFunctions CODEIN_PROC_FUNCTIONS = { CodeIn_ProcInit, CodeIn_ProcMain, CodeIn_ProcExit };

static BGSysVRAMConfig sCodeInVRAMConfig = {
    GX_VRAM_BG_128_A,
    GX_VRAM_BGEXTPLTT_NONE,
    GX_VRAM_H,
    GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,
    GX_VRAM_OBJEXTPLTT_NONE,
    GX_VRAM_SUB_OBJ_16_I,
    GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_B | GX_VRAM_C,
    GX_VRAM_F | GX_VRAM_G,
    GX_OBJVRAMMODE_CHAR_1D_64K,
    GX_OBJVRAMMODE_CHAR_1D_32K,
};

static BOOL CodeIn_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    CodeInWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_CODEIN, 0x40000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(CodeInWork), HEAPID_CODEIN);
    sys_memset(wk, 0, sizeof(CodeInWork));
    if (param == NULL) {
        int blocks[CODEIN_BLOCK_COUNT];
        CodeInParam *friendCode;

        blocks[0] = 4;
        blocks[1] = 4;
        blocks[2] = 4;
        friendCode = CodeInParam_Create(HEAPID_CODEIN, 0, 0, FRIEND_CODE_LENGTH, blocks);
        wk->param = *friendCode;
        GFL_HeapFree(friendCode);
    } else {
        wk->param = *(CodeInParam *)param;
    }
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    wk->arc = GFL_ArcSysCreateFileHandle(ARCID_CODEIN, HEAPID_CODEIN);
    GFL_BGSysCreate(HEAPID_CODEIN);
    BmpWin_InitAllocator(HEAPID_CODEIN);
    wk->fade = PaletteFade_Create(HEAPID_CODEIN);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_CODEIN);
    PaletteFade_SetTransferAll(wk->fade, TRUE);
    PaletteFade_AllocBuffer(wk->fade, 0, 0x200, HEAPID_CODEIN);
    CodeIn_InitBG();
    CodeInDisp_InitActors(wk, &sCodeInVRAMConfig);
    CodeIn_InitParam(wk);
    CodeIn_InitButtons(wk);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 0, GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2, 15, 7);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, 0, GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, 7, 8);
    wk->vblankTask = GFL_VBlankTCBAdd(CodeIn_VBlank, wk, 16);
    func_02042ba8(TRUE, HEAPID_CODEIN);
    return TRUE;
}

static BOOL CodeIn_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BOOL done = CodeIn_Main(work);

    func_0204b794();
    if (done) {
        return TRUE;
    }
    return FALSE;
}

static BOOL CodeIn_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    CodeInWork *wk = work;

    GFL_TCBRemove(wk->vblankTask);
    GFL_FontFree(wk->font);
    CodeInDisp_Free(wk);
    BmpWin_FreeAllocator();
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG2, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysFree();
    PaletteFade_FreeBuffer(wk->fade, 0);
    PaletteFade_Free(wk->fade);
    GFL_ArcToolFree(wk->arc);
    GFL_BMN_Delete(wk->buttons);
    // The screen's own parameter was freed without its string
    if (param == NULL) {
        GFL_StrBufFree(wk->param.str);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_CODEIN);
    return TRUE;
}

static void CodeIn_InitBG(void) {
    GFL_BGSysDisableAllA();
    GFL_BGSysSetVRAMBanks(&sCodeInVRAMConfig);
    sys_memset32(0, (void *)HW_BG_VRAM, HW_BG_VRAM_SIZE);
    sys_memset32(0, (void *)HW_DB_BG_VRAM, HW_DB_BG_VRAM_SIZE);
    sys_memset32(0, (void *)HW_OBJ_VRAM, HW_OBJ_VRAM_SIZE);
    sys_memset32(0, (void *)HW_DB_OBJ_VRAM, HW_DB_OBJ_VRAM_SIZE);
    {
        BGSetup setups[] = {
            { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
              GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
            { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000),
              0x4000, GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
            { 0, 0, 0x1800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x10000),
              0x8000, GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
        };

        GFL_BGSysCreateBG(1, &setups[0], BGMODE_TEXT);
        GFL_BGSysCreateBG(2, &setups[1], BGMODE_TEXT);
        GFL_BGSysCreateBG(3, &setups[2], BGMODE_TEXT);
        GFL_BGSysClearScr(1);
        GFL_BGSysClearScr(2);
        GFL_BGSysClearScr(3);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, FALSE);
    {
        BGSetup setups[] = {
            { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6800), GX_BG_CHARBASE(0x00000), 0x4000,
              GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
            { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7000), GX_BG_CHARBASE(0x04000), 0x2000,
              GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
            { 0 },
            { 0 },
        };

        GFL_BGSysCreateBG(4, &setups[0], BGMODE_TEXT);
        GFL_BGSysCreateBG(5, &setups[1], BGMODE_TEXT);
        GFL_BGSysCreateBG(6, &setups[2], BGMODE_TEXT);
        GFL_BGSysCreateBG(7, &setups[3], BGMODE_TEXT);
        GFL_BGSysClearScr(4);
        GFL_BGSysClearScr(5);
        GFL_BGSysClearScr(6);
        GFL_BGSysClearScr(7);
    }
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG2, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void CodeIn_VBlank(TCB *tcb, void *data) {
    CodeInWork *wk = data;

    PaletteFade_Transfer(wk->fade);
    func_0204b7c8();
}
