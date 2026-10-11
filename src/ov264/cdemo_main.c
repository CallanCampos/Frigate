#include "types.h"
#include "app/cdemo.h"
#include "app/cdemo_main.h"
#include "app/cdemo_script.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/calctool.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "system/wipe.h"

// The size of a screen as a direct color bitmap: 256 by 192 pixels of 2 bytes
#define SCREEN_SIZE 0x18000

static BOOL CDemo_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CDemo_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CDemo_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void CDemo_VBlankTask(TCB *tcb, void *data);

const GameProcFunctions CDEMO_PROC_FUNCTIONS = {
    CDemo_Init,
    CDemo_Main,
    CDemo_Exit,
};

static const u32 sScriptOpening2[17] = {
    CDEMO_CMD_FADE_IN_BLACK,
    1,
    1,
    CDEMO_CMD_WAIT_WIPE,
    CDEMO_CMD_PLAY_BGM,
    SEQ_BGM_TITLE01,
    CDEMO_CMD_NOP_19,
    0,
    CDEMO_CMD_START_ANIMATION,
    0,
    CDEMO_CMD_NOP_19,
    1,
    CDEMO_CMD_FADE_OUT_BLACK,
    1,
    1,
    CDEMO_CMD_WAIT_WIPE,
    CDEMO_CMD_END,
};

static const u32 sScriptGFLogo[19] = {
    CDEMO_CMD_FADE_IN_BLACK,
    1,
    1,
    CDEMO_CMD_WAIT_WIPE,
    CDEMO_CMD_PLAY_BGM,
    SEQ_BGM_GF_LOGO,
    CDEMO_CMD_NOP_19,
    0,
    CDEMO_CMD_START_ANIMATION,
    0,
    CDEMO_CMD_NOP_19,
    1,
    CDEMO_CMD_WAIT,
    42,
    CDEMO_CMD_FADE_OUT_BLACK,
    1,
    1,
    CDEMO_CMD_WAIT_WIPE,
    CDEMO_CMD_END,
};

static const u32 sScriptOpening1[21] = {
    CDEMO_CMD_FADE_IN_BLACK,
    1,
    1,
    CDEMO_CMD_WAIT_WIPE,
    CDEMO_CMD_PLAY_BGM,
    SEQ_BGM_TITLE,
    CDEMO_CMD_WAIT,
    80,
    CDEMO_CMD_NOP_19,
    0,
    CDEMO_CMD_START_ANIMATION,
    0,
    CDEMO_CMD_NOP_19,
    1,
    CDEMO_CMD_WAIT,
    20,
    CDEMO_CMD_FADE_OUT_BLACK,
    1,
    1,
    CDEMO_CMD_WAIT_WIPE,
    CDEMO_CMD_END,
};

static BOOL CDemo_Init(GameProc *proc, u32 *state, void *param, void *work) {
    CDemo *demo;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_CDEMO, 0x80000);
    demo = GFL_ProcInitSubsystem(proc, sizeof(CDemo), HEAPID_CDEMO);
    sys_memset(demo, 0, sizeof(CDemo));
    demo->param = param;
    demo->initialized = FALSE;
    return TRUE;
}

static BOOL CDemo_Main(GameProc *proc, u32 *state, void *param, void *work) {
    if (CDemo_Update(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL CDemo_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_CDEMO);
    return TRUE;
}

void CDemo_SetVRAMBanks(void) {
    static const BGSysVRAMConfig sVRAMConfig = {
        GX_VRAM_B | GX_VRAM_C,
        GX_VRAM_E,
        GX_VRAM_SUB_BG_32_H,
        GX_VRAM_SUB_BGEXTPLTT_NONE,
        GX_VRAM_A,
        GX_VRAM_OBJEXTPLTT_NONE,
        GX_VRAM_SUB_OBJ_16_I,
        GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_NONE,
        GX_VRAM_TEXPLTT_NONE,
        GX_OBJVRAMMODE_CHAR_2D,
        GX_OBJVRAMMODE_CHAR_2D,
    };

    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
}

void CDemo_SetupBGs(CDemo *demo) {
    MtxFx22 matrix;
    void *buffer;

    gfxSetEngineModeA(GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BG0_AS_2D);
    G2_SetBG2ControlDCBmp(GX_BG_SCRSIZE_DCBMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x00000);
    G2_SetBG3ControlDCBmp(GX_BG_SCRSIZE_DCBMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x20000);
    MAT2_SetScaleRot(&matrix, 0, FX32_ONE, FX32_ONE, 0);
    G2_SetBG2Affine(&matrix, 0, 0, 0, 0);
    G2_SetBG3Affine(&matrix, 0, 0, 0, 0);
    buffer = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_CDEMO), SCREEN_SIZE, TRUE, "cdemo_main.c", 90);
    gfxUploadBGScreen2A(buffer, 0, SCREEN_SIZE);
    gfxUploadBGScreen3A(buffer, 0, SCREEN_SIZE);
    GFL_HeapFree(buffer);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_OBJ, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
}

void CDemo_HideBGs(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
}

void CDemo_InitScript(CDemo *demo) {
    static const u32 *const sScripts[3] = { sScriptGFLogo, sScriptOpening1, sScriptOpening2 };

    demo->script = demo->scriptStart = sScripts[demo->param->demo];
}

void CDemo_Free(CDemo *demo) {
}

void CDemo_AddVBlankTask(CDemo *demo) {
    demo->vblankTask = GFL_VBlankTCBAdd(CDemo_VBlankTask, demo, 0);
}

void CDemo_RemoveVBlankTask(CDemo *demo) {
    GFL_TCBRemove(demo->vblankTask);
}

static void CDemo_VBlankTask(TCB *tcb, void *data) {
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

void CDemo_FadeInBlack(CDemo *demo, s32 division, s32 sync, u32 nextState) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, division, sync, HEAPID_CDEMO);
    demo->nextState = nextState;
}

void CDemo_FadeOutBlack(CDemo *demo, s32 division, s32 sync, u32 nextState) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, division, sync, HEAPID_CDEMO);
    demo->nextState = nextState;
}

void CDemo_FadeInWhite(CDemo *demo, s32 division, s32 sync, u32 nextState) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_WHITE, division, sync, HEAPID_CDEMO);
    demo->nextState = nextState;
}

void CDemo_FadeOutWhite(CDemo *demo, s32 division, s32 sync, u32 nextState) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_WHITE, division, sync, HEAPID_CDEMO);
    demo->nextState = nextState;
}

void CDemo_LoadPalette(CDemo *demo, u32 index, u32 fileId) {
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLRArc(demo->arc, fileId, &palette, HEAPID_TAIL(HEAPID_CDEMO));
    CDemoPalette *dest = &demo->palettes[index];

    dest->size = palette->size;
    sys_memcpy32(palette->rawData, dest->data, dest->size);
    GFL_HeapFree(file);
}
