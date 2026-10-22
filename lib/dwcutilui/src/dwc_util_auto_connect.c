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

static void initGraphics(void);
static void DWCUtilAutoConnect_StartAutoConnect(void);
static void procInit(void);
static void procInit1(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void button(void);
static void DWCUtilAutoConnect_FadeOut(void);
static void endScene1(void);
static void endScene2(void);
static void DWCUtilAutoConnect_ProcessTask(DWCUtilTask *task, void *arg);
static void *DWCUtilAutoConnect_Alloc(u32 name, s32 size);
static void DWCUtilAutoConnect_Free(u32 name, void *ptr, s32 size);

// What auto-connect is started with: the utility's allocator, DMA channel 3 and the radio on
static const DWCACConfig sAutoConnectParam = { DWCUtilAutoConnect_Alloc, DWCUtilAutoConnect_Free, 3, 1, 0, 0, 0, 0, 0 };

// Whether the connection test succeeded
static u8 sAutoConnectSucceeded;

void DWCUtilAutoConnect_Init(void) {
    int from;

    sAutoConnectSucceeded = FALSE;
    initGraphics();
    DWCi_GetParam2_(NULL, &from);
    if (from == 0) {
        DWCi_COMMENTlDispEx(0x31, -1, 0);
    }
    DWCi_CMNlStep(2);
    if (from == 0) {
        DWCi_HLlLoadSet();
    }
    DWCi_CMNlMsgDraw(0x2c);
    DWCi_ANIMElInitEx(0);
    DWCUtilAutoConnect_StartAutoConnect();
    DWCUtilLinkIcon_Init(0);
    DWCUtilSound_PlaySe(0xc);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
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
static void DWCUtilAutoConnect_StartAutoConnect(void) {
    DWCUtilSettingsEdit *edit = DWCUtilWifiSettings_GetEdit();
    DWCACConfig param;
    int from;

    MIi_CpuCopy32(&sAutoConnectParam, &param, sizeof(DWCACConfig));
    DWCi_GetParam2_(NULL, &from);
    if (from == 2) {
        param.mode = 4;
    } else {
        param.mode = edit->index + 1;
    }
    if (!DWC_AC_Create(&param)) {
        OS_Terminate();
    }
    if (from == 0) {
        DWCi_AC_InsertApInfo(edit->index, edit);
    }
    DWCi_TSKlForm(0, DWCUtilAutoConnect_ProcessTask, NULL, 0x78);
}

static void procInit(void) {
    int from;

    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_GetParam2_(NULL, &from);
    if (from == 0) {
        DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    }
    DWCi_ChangeScene(procInit1);
}

static void procInit1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_ChangeScene(procScene);
    }
}

static void procScene(void) {
    input();
    draw();
    button();
}

static void input(void) {
}

static void draw(void) {
}

static void button(void) {
}

static void DWCUtilAutoConnect_FadeOut(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(0) == 0) {
        DWCi_ChangeScene(endScene2);
    }
}

// Once auto-connect is down, puts the Wi-Fi ID back into the first two pages of the settings, and goes on to the
// message of a success or to the error screen
static void endScene2(void) {
    DWCMemPage *pages = (DWCMemPage *)DWCi_SETTINGlGetMemMap();

    if (DWC_AC_Destroy()) {
        MI_CpuCopy8(DWCi_BACKUPlGetWifi(), pages[0].wifi, DWC_BM_PACKED_SIZE);
        MI_CpuCopy8(DWCi_BACKUPlGetWifi(), pages[1].wifi, DWC_BM_PACKED_SIZE);
        DWCi_ANIMElEnd();
        DWCi_FNTlDeleteBgLcd(0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sAutoConnectSucceeded) {
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCUtilError_Init);
        } else {
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCUtilAutoConnectOk_Init);
        }
    }
}

// Runs auto-connect until it connects or fails, which keeps the error code for the error screen
static void DWCUtilAutoConnect_ProcessTask(DWCUtilTask *task, void *arg) {
    s32 result = DWC_AC_Process();

    if (result != 0) {
        DWCi_ICONlEnd();
        DWCUtilSound_StopSe();
        if (result > 0) {
            sAutoConnectSucceeded = TRUE;
            DWCUtilSound_PlaySe(0x11);
        } else {
            DWCi_SetErrorCode(DWC_AC_GetStatus());
            DWCUtilSound_PlaySe(0x12);
        }
        DWCi_ChangeScene(DWCUtilAutoConnect_FadeOut);
        DWCi_TSKlDelete(0, task);
    }
}

static void *DWCUtilAutoConnect_Alloc(u32 name, s32 size) {
    return DWCi_HEAPlAllocEx(size, 0x20);
}

static void DWCUtilAutoConnect_Free(u32 name, void *ptr, s32 size) {
    DWCi_HEAPlFree2(ptr);
}
