#include "app/cdemo.h"
#include "gfl/arc_util.h"
#include "gfl/calctool.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "system/wipe.h"

static BOOL CDemo_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CDemo_Update(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CDemo_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void CDemo_VBlankCallback(TCB *tcb, void *data);

const GameProcFunctions CDEMO_PROC_FUNCTIONS = { CDemo_Init, CDemo_Update, CDemo_Exit };
// Demo scripts are arrays of opcode words and their arguments.
static const u32 CDEMO_SCRIPT_LOGO[] = { 1, 1, 1, 5, 14, 1004, 19, 0, 18, 0, 19, 1, 2, 1, 1, 5, 20 };
static const u32 CDEMO_SCRIPT_0[] = { 1, 1, 1, 5, 14, 1171, 19, 0, 18, 0, 19, 1, 0, 42, 2, 1, 1, 5, 20 };
static const u32 CDEMO_SCRIPT_1[] = { 1, 1, 1, 5, 14, 1003, 0, 80, 19, 0, 18, 0, 19, 1, 0, 20, 2, 1, 1, 5, 20 };
static const u32 *const CDEMO_SCRIPTS[] = { CDEMO_SCRIPT_0, CDEMO_SCRIPT_1, CDEMO_SCRIPT_LOGO };
static const BGSysVRAMConfig CDEMO_VRAM_CONFIG = { 6, 0x10, 0x80, 0, 1, 0, 0x100, 0, 0, 0, 0, 0 };

static BOOL CDemo_Init(GameProc *proc, u32 *state, void *param, void *unused) {
    CDemoWork *work;
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_CDEMO, 0x80000);
    work = GFL_ProcInitSubsystem(proc, sizeof(CDemoWork), HEAPID_CDEMO);
    MI_CpuFill8(work, 0, sizeof(CDemoWork));
    work->param = param;
    work->initialized = FALSE;
    return TRUE;
}
static BOOL CDemo_Update(GameProc *proc, u32 *state, void *param, void *work) {
    return CDemo_Main(work) == FALSE;
}
static BOOL CDemo_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_CDEMO);
    return TRUE;
}
void CDemo_ConfigureVRAM(void) {
    GFL_BGSysSetVRAMBanks(&CDEMO_VRAM_CONFIG);
}
void CDemo_InitGraphics(CDemoWork *work) {
    MtxFx22 matrix;
    void *screen;
    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BG0_AS_2D);
    G2_SetBG2ControlDCBmp(GX_BG_SCRSIZE_DCBMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x00000);
    G2_SetBG3ControlDCBmp(GX_BG_SCRSIZE_DCBMP_256x256, GX_BG_AREAOVER_XLU, (GXBGBmpScrBase)(0x20000 / 0x4000));
    MAT2_SetScaleRot(&matrix, 0, FX32_ONE, FX32_ONE, 0);
    G2_SetBG2Affine(&matrix, 0, 0, 0, 0);
    G2_SetBG3Affine(&matrix, 0, 0, 0, 0);
    screen = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_CDEMO), 0x18000, TRUE, "cdemo_main.c", 90);
    GX_LoadBG2Scr(screen, 0, 0x18000);
    GX_LoadBG3Scr(screen, 0, 0x18000);
    GFL_HeapFree(screen);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_OBJ, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
}
void CDemo_DisableGraphics(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
}
void CDemo_InitScript(CDemoWork *work) {
    const u32 *script = CDEMO_SCRIPTS[work->param->demo];
    work->scriptStart = script;
    work->scriptCursor = script;
}
void CDemo_FreeScript(CDemoWork *work) {
}
void CDemo_StartVBlank(CDemoWork *work) {
    work->vblankTask = GFL_VBlankTCBAdd(CDemo_VBlankCallback, work, 0);
}
void CDemo_StopVBlank(CDemoWork *work) {
    GFL_TCBRemove(work->vblankTask);
}
static void CDemo_VBlankCallback(TCB *tcb, void *data) {
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}
void CDemo_FadeInBlack(CDemoWork *work, int division, int sync, u32 resumeState) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, division, sync, HEAPID_CDEMO);
    work->resumeState = resumeState;
}
void CDemo_FadeOutBlack(CDemoWork *work, int division, int sync, u32 resumeState) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, division, sync, HEAPID_CDEMO);
    work->resumeState = resumeState;
}
void CDemo_FadeInWhite(CDemoWork *work, int division, int sync, u32 resumeState) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_WHITE, division, sync, HEAPID_CDEMO);
    work->resumeState = resumeState;
}
void CDemo_FadeOutWhite(CDemoWork *work, int division, int sync, u32 resumeState) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_WHITE, division, sync, HEAPID_CDEMO);
    work->resumeState = resumeState;
}
void CDemo_LoadPalette(CDemoWork *work, u32 index, u32 fileId) {
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLRArc(work->arc, fileId, &palette, HEAPID_TAIL(HEAPID_CDEMO));
    CDemoPalette *buffer = &work->palettes[index];
    buffer->size = palette->szByte;
    MIi_CpuCopy32(palette->pRawData, buffer->data, buffer->size);
    GFL_HeapFree(file);
}
