#include "dwc_util_auto_connect.h"
#include "app/download_play_sys.h"
#include "dwc/ac.h"
#include "dwc/bm.h"
#include "dwc_util_auto_connect_ok.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_error.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_heap.h"
#include "dwc_util_link_icon.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

static void func_ov182_021b616c(void);
static void func_ov182_021b61dc(void);
static void func_ov182_021b6248(void);
static void func_ov182_021b6290(void);
static void func_ov182_021b62b4(void);
static void func_ov182_021b62c4(void);
static void func_ov182_021b62c8(void);
static void func_ov182_021b62cc(void);
static void func_ov182_021b62d0(void);
static void func_ov182_021b62ec(void);
static void func_ov182_021b6304(void);
static void func_ov182_021b637c(DWCUtilTask *task, void *arg);
static void *func_ov182_021b63c4(u32 name, u32 size);
static void func_ov182_021b63d0(u32 name, void *ptr, u32 size);

// What auto-connect is started with: the utility's allocator, DMA channel 3 and the radio on
static const DWCAcParam sAutoConnectParam = { func_ov182_021b63c4, func_ov182_021b63d0, 3, 1, 0, 0, 0, 0, 0 };

// Whether the connection test succeeded
static u8 sAutoConnectSucceeded;

void func_ov182_021b6104(void) {
    int from;

    sAutoConnectSucceeded = FALSE;
    func_ov182_021b616c();
    DWCUtilMain_GetScreenArgs(NULL, &from);
    if (from == 0) {
        DWCUtilScreen_Start(0x31, -1, 0);
    }
    DWCUtilGfxResource_ShowStep(2);
    if (from == 0) {
        DWCUtilBgOverlay_ShowConnectionSet();
    }
    DWCUtilGfxResource_ShowMessage(0x2c);
    DWCUtilUiEffect_Init(0);
    func_ov182_021b61dc();
    DWCUtilLinkIcon_Init(0);
    DWCUtilSound_PlaySe(0xc);
    DWCUtilMain_SetUpdate(func_ov182_021b6248);
}

static void func_ov182_021b616c(void) {
    DWCUtilGfxResource_Load("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

// Starts auto-connect with the connection being edited, or with the console's settings when the utility was started
// for that, and the task that waits for its result
static void func_ov182_021b61dc(void) {
    DWCUtilSettingsEdit *edit = func_ov182_021b781c();
    DWCAcParam param;
    int from;

    MIi_CpuCopy32(&sAutoConnectParam, &param, sizeof(DWCAcParam));
    DWCUtilMain_GetScreenArgs(NULL, &from);
    if (from == 2) {
        param.mode = 4;
    } else {
        param.mode = edit->index + 1;
    }
    if (!DWCAc_Create(&param)) {
        OS_Terminate();
    }
    if (from == 0) {
        DWCAc_SetDsConnection(edit->index, edit);
    }
    DWCUtilTask_Create(0, func_ov182_021b637c, NULL, 0x78);
}

static void func_ov182_021b6248(void) {
    int from;

    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_GetScreenArgs(NULL, &from);
    if (from == 0) {
        DWCUtilFade_Start(2, 1, 1, 8);
        DWCUtilDisplay_ShowPlanes(1, 1);
    }
    DWCUtilMain_SetUpdate(func_ov182_021b6290);
}

static void func_ov182_021b6290(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilMain_SetUpdate(func_ov182_021b62b4);
    }
}

static void func_ov182_021b62b4(void) {
    func_ov182_021b62c4();
    func_ov182_021b62c8();
    func_ov182_021b62cc();
}

static void func_ov182_021b62c4(void) {
}

static void func_ov182_021b62c8(void) {
}

static void func_ov182_021b62cc(void) {
}

static void func_ov182_021b62d0(void) {
    DWCUtilFade_Start(3, 0, 0x15, 8);
    DWCUtilMain_SetUpdate(func_ov182_021b62ec);
}

static void func_ov182_021b62ec(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilMain_SetUpdate(func_ov182_021b6304);
    }
}

// Once auto-connect is down, puts the Wi-Fi ID back into the first two pages of the settings, and goes on to the
// message of a success or to the error screen
static void func_ov182_021b6304(void) {
    DWCBmNtrPage *pages = (DWCBmNtrPage *)func_ov182_021b7e6c();

    if (DWCAc_Destroy()) {
        MI_CpuCopy8(DWCBm_GetPackedWifiIdInfo(), pages[0].wifiId, DWC_BM_PACKED_SIZE);
        MI_CpuCopy8(DWCBm_GetPackedWifiIdInfo(), pages[1].wifiId, DWC_BM_PACKED_SIZE);
        DWCUtilUiEffect_Destroy();
        DWCUtilText_DestroyScreenBg(0);
        DWCUtilDisplay_HidePlanes(0, 0x15);
        if (!sAutoConnectSucceeded) {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(func_ov182_021b293c);
        } else {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(func_ov182_021b63dc);
        }
    }
}

// Runs auto-connect until it connects or fails, which keeps the error code for the error screen
static void func_ov182_021b637c(DWCUtilTask *task, void *arg) {
    s32 result = DWCAc_Process();

    if (result != 0) {
        DWCUtilLinkIcon_Exit();
        DWCUtilSound_StopSe();
        if (result > 0) {
            sAutoConnectSucceeded = TRUE;
            DWCUtilSound_PlaySe(0x11);
        } else {
            func_ov182_021b2d3c(DWCAc_GetStatus());
            DWCUtilSound_PlaySe(0x12);
        }
        DWCUtilMain_SetUpdate(func_ov182_021b62d0);
        DWCUtilTask_QueueRemoval(0, task);
    }
}

static void *func_ov182_021b63c4(u32 name, u32 size) {
    return DWCUtilHeap_AllocClear(size, 0x20);
}

static void func_ov182_021b63d0(u32 name, void *ptr, u32 size) {
    DWCUtilHeap_Free(ptr);
}
