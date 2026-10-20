#include "dwc_util_main.h"
#include "dwc/bm.h"
#include "dwc/dwc.h"
#include "dwc/utility.h"
#include "dwc_util_bank.h"
#include "dwc_util_cell.h"
#include "dwc_util_fade.h"
#include "dwc_util_heap.h"
#include "dwc_util_irq.h"
#include "dwc_util_main_menu.h"
#include "dwc_util_oam.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_vram_alloc.h"
#include "nitro/fs.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/rtc.h"
#include "nitro/tp.h"

// The language, the starting screen and the flags that DWC_StartUtility was given, the screen's update function and
// the values the screens hand each other
typedef struct DWCUtilMain {
    u8 language;
    u8 finished;
    void *work;
    // DWC_StartUtility's param: the starting screen in the low 4 bits, and flags
    union {
        u32 raw;
        struct {
            u32 mode : 4;
            u32 flags : 28;
        } fields;
    } param;
    void (*update)(void);
    u32 value0;
    u32 value1;
    int value2;
    int value3;
} DWCUtilMain;

static char sNasUrl[] = "https://nas.nintendowifi.net/ac";
static DWCUtilMain sMain;

static BOOL DWCUtilMain_CheckParams(int language, int param);
static void DWCUtilMain_InitSystem(void);
static void DWCUtilMain_InitGraphics(void);
static void DWCUtilMain_Exit(void);

int DWC_StartUtility(void *work, int language, int param) {
    const char *nasUrl;

    if (OS_IsRunOnTwl()) {
        return -2;
    }
    sMain.work = work;
    if (!DWCUtilMain_CheckParams(language, param)) {
        return -1;
    }
    nasUrl = func_ov011_021694e0();
    func_ov011_021694d4(sNasUrl);
    sMain.finished = FALSE;
    DWCUtilMain_InitSystem();
    DWCUtilMain_InitGraphics();
    DWCUtilSound_Init();
    DWCUtilMain_SetUpdate(func_ov182_021ae574);
    do {
        func_ov182_021bc5bc();
        sMain.update();
        DWCUtilTask_Run(0);
        func_ov182_021bca28();
        func_ov182_021bd0ac();
        OS_WaitVBlankIntr();
    } while (!sMain.finished);
    DWCUtilMain_Exit();
    func_ov011_021694d4(nasUrl);
    return 0;
}

static BOOL DWCUtilMain_CheckParams(int language, int param) {
    sMain.language = language;
    sMain.param.raw = param;
    if (language < 0 || language > 6) {
        return FALSE;
    }
    if (sMain.param.fields.mode > 1) {
        return FALSE;
    }
    if ((sMain.param.fields.flags & 2) != 0) {
        return FALSE;
    }
    if (language != 0 && (sMain.param.fields.flags & 1) != 0) {
        return FALSE;
    }
    if (language == 0 && (sMain.param.fields.flags & 1) == 0) {
        return FALSE;
    }
    return TRUE;
}

static void DWCUtilMain_InitSystem(void) {
    void *work;

    OS_DisableIrq();
    GX_DispOff();
    GXS_DispOff();
    if (!OS_IsTickAvailable()) {
        OS_Terminate();
    }
    if (!OS_IsAlarmAvailable()) {
        OS_Terminate();
    }
    GX_VBlankIntr(FALSE);
    FX_Init();
    FS_Init(0xffffffff);
    TP_Init();
    RTC_Init();
    GX_DispOff();
    GXS_DispOff();
    DWCUtilBank_Init();
    DWCUtilHeap_Init(sMain.work);
    DWCUtilIrq_Init();
    DWCUtilTask_Init();
    DWCUtilRomResource_Init();
    func_ov182_021bc544();
    func_ov182_021bba6c();
    work = DWCUtilHeap_AllocClear(DWC_BM_WORK_SIZE, 0x20);
    DWC_BM_Init(work);
    DWCUtilHeap_FreeAndNull(&work);
}

static void DWCUtilMain_InitGraphics(void) {
    GX_VBlankIntr(FALSE);
    GX_SetBankForBG(GX_VRAM_BG_128_A);
    GX_SetBankForOBJ(GX_VRAM_OBJ_128_B);
    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_2D);
    GX_SetVisiblePlane(0);
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, 0);
    GX_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_128K);
    G2_BG0Mosaic(FALSE);
    G2_BG1Mosaic(FALSE);
    G2_BG2Mosaic(FALSE);
    G2_BG3Mosaic(FALSE);
    G2_SetBG0Offset(0, 0);
    G2_SetBG1Offset(0, 0);
    G2_SetBG2Offset(0, 0);
    G2_SetBG3Offset(0, 0);
    G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, 0x3f, 16);
    GX_SetBankForSubBG(GX_VRAM_SUB_BG_32_H);
    GX_SetBankForSubOBJ(GX_VRAM_SUB_OBJ_16_I);
    GXS_SetGraphicsMode(GX_BGMODE_0);
    GXS_SetVisiblePlane(0);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR, 0);
    GXS_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_32K);
    G2S_BG0Mosaic(FALSE);
    G2S_BG1Mosaic(FALSE);
    G2S_BG2Mosaic(FALSE);
    G2S_BG3Mosaic(FALSE);
    G2S_SetBG0Offset(0, 0);
    G2S_SetBG1Offset(0, 0);
    G2S_SetBG2Offset(0, 0);
    G2S_SetBG3Offset(0, 0);
    G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, 0x3f, 16);
    G2_SetBG0Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x6000, GX_BG_CHARBASE_0x00000,
                     GX_BG_EXTPLTT_01);
    G2_SetBG1Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x6800, GX_BG_CHARBASE_0x08000,
                     GX_BG_EXTPLTT_01);
    G2_SetBG2ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x7000, GX_BG_CHARBASE_0x10000);
    G2_SetBG3ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x7800, GX_BG_CHARBASE_0x10000);
    G2S_SetBG0Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x6000, GX_BG_CHARBASE_0x00000,
                      GX_BG_EXTPLTT_01);
    G2S_SetBG1Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x6800, GX_BG_CHARBASE_0x00000,
                      GX_BG_EXTPLTT_01);
    G2S_SetBG2ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x7000, GX_BG_CHARBASE_0x00000);
    G2S_SetBG3ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x7800, GX_BG_CHARBASE_0x00000);
    GX_SetBGScrOffset(GX_BGSCROFFSET_0x00000);
    GX_SetBGCharOffset(GX_BGCHAROFFSET_0x00000);
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    func_ov182_021bbc50();
    func_ov182_021bcbc8();
    DWCUtilVramAlloc_Init();
    func_ov182_021bb480();
    GX_DispOn();
    GXS_DispOn();
    GX_VBlankIntr(TRUE);
}

static void DWCUtilMain_Exit(void) {
    GX_DispOff();
    GXS_DispOff();
    func_ov182_021bd0c8();
    PM_SetLCDPower(TRUE);
    func_ov182_021bc598();
    DWCUtilIrq_Exit();
    DWCUtilSound_Exit();
    func_ov182_021bb54c();
    func_ov182_021bcc5c();
    func_ov182_021bbd00();
    func_ov182_021bba9c();
    DWCUtilRomResource_Exit();
    DWCUtilTask_Exit();
    DWCUtilHeap_Exit();
    DWCUtilBank_Exit();
}

void DWCUtilMain_SetUpdate(void (*update)(void)) {
    sMain.update = update;
}

void DWCUtilMain_SetLayout(u32 value0, u32 value1) {
    sMain.value0 = value0;
    sMain.value1 = value1;
}

void DWCUtilMain_GetLayout(u32 *value0, u32 *value1) {
    if (value0 != NULL) {
        *value0 = sMain.value0;
    }
    if (value1 != NULL) {
        *value1 = sMain.value1;
    }
}

void DWCUtilMain_SetScreenArgs(int value2, int value3) {
    sMain.value2 = value2;
    sMain.value3 = value3;
}

void DWCUtilMain_GetScreenArgs(int *value2, int *value3) {
    if (value2 != NULL) {
        *value2 = sMain.value2;
    }
    if (value3 != NULL) {
        *value3 = sMain.value3;
    }
}

u8 DWCUtilMain_GetLanguage(void) {
    return sMain.language;
}

u32 DWCUtilMain_GetStartScreen(void) {
    return sMain.param.fields.mode;
}

BOOL DWCUtilMain_HasFlag(u32 mask) {
    if (mask & sMain.param.fields.flags) {
        return TRUE;
    }
    return FALSE;
}

void DWCUtilMain_Finish(void) {
    sMain.finished = TRUE;
}

u8 func_ov182_021ae058(void) {
    u8 language = sMain.language;

    if (language == 6) {
        return 0x38;
    }
    if (language == 1 && !(sMain.param.fields.flags & 2)) {
        return 0x37;
    }
    return language + 0x31;
}
