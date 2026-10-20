#include "dwc_util_page_step3.h"
#include "dwc_util_aoss_session.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_page_multi4.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_way.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"
#include "nitro/os.h"

// Whether the setup succeeded, the frames since, and the VBlank task that reads B and the lid while the setup runs
typedef struct DWCUtilPageStep3 {
    u8 succeeded;
    u16 frames;
    DWCUtilTask *task;
} DWCUtilPageStep3;

static BOOL func_ov182_021b06d8(void);
static void func_ov182_021b073c(void);
static void func_ov182_021b07ac(void);
static void func_ov182_021b07e4(void);
static void func_ov182_021b0810(void);
static void func_ov182_021b0844(void);
static void func_ov182_021b08c8(void);
static void func_ov182_021b08ec(void);
static void func_ov182_021b08f0(void);
static void func_ov182_021b0920(void);
static void func_ov182_021b094c(void);
static void func_ov182_021b0990(void);
static void func_ov182_021b0a24(DWCUtilTask *task, void *arg);
static void func_ov182_021b0a3c(void);
static void func_ov182_021b0a5c(void);
static void func_ov182_021b0a74(void);
static void func_ov182_021b0aac(void);

static DWCUtilPageStep3 sPageStep3;

// Whether the lid is closed
static BOOL func_ov182_021b06d8(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov182_021b06f4(void) {
    sPageStep3.frames = 0;
    func_ov182_021b073c();
    DWCUtilBgOverlay_ShowConnectionSet();
    DWCUtilScreen_Start(0x36, -1, 0);
    DWCUtilGfxResource_ShowStep(2);
    DWCUtilGfxResource_ShowMessage(0x22);
    DWCUtilUiEffect_Init(0);
    func_ov182_021ada94();
    DWCUtilSound_PlaySe(0xb);
    DWCUtilMain_SetUpdate(func_ov182_021b07ac);
}

static void func_ov182_021b073c(void) {
    DWCUtilGfxResource_Load("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b07ac(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021b07e4);
}

static void func_ov182_021b07e4(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(1);
        DWCUtilMain_SetUpdate(func_ov182_021b0810);
    }
}

static void func_ov182_021b0810(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        sPageStep3.task = DWCUtilTask_Create(1, func_ov182_021b0a24, NULL, 0x78);
        DWCUtilMain_SetUpdate(func_ov182_021b0844);
    }
}

// Runs the setup
static void func_ov182_021b0844(void) {
    func_ov182_021b08c8();
    func_ov182_021b08ec();
    func_ov182_021b08f0();
    switch (func_ov182_021adb74()) {
    case 0:
        break;
    case 1:
        if (sPageStep3.task != NULL) {
            sPageStep3.succeeded = TRUE;
            DWCUtilTask_Remove(1, sPageStep3.task);
            sPageStep3.task = NULL;
            DWCUtilMain_SetUpdate(func_ov182_021b0a74);
        }
        break;
    case 2:
        if (sPageStep3.task != NULL) {
            DWCUtilSound_StopSe();
            func_ov182_021b8cf0(0xc, 1, 1, -1, 0);
            DWCUtilSound_PlaySe(9);
            DWCUtilUiObject_Disable();
            DWCUtilTask_Remove(1, sPageStep3.task);
            sPageStep3.task = NULL;
            DWCUtilMain_SetUpdate(func_ov182_021b0a3c);
        }
        break;
    }
}

static void func_ov182_021b08c8(void) {
    if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (func_ov182_021b06d8()) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b08ec(void) {
}

// Cancels the setup
static void func_ov182_021b08f0(void) {
    if (DWCUtilUiObject_GetState() == 0 && sPageStep3.task != NULL) {
        DWCUtilTask_QueueRemoval(1, sPageStep3.task);
        sPageStep3.task = NULL;
        DWCUtilUiObject_Disable();
        DWCUtilMain_SetUpdate(func_ov182_021b0aac);
    }
}

static void func_ov182_021b0920(void) {
    DWCUtilUiObject_Disable();
    if (sPageStep3.task != NULL) {
        DWCUtilTask_Remove(1, sPageStep3.task);
    }
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(func_ov182_021b094c);
}

static void func_ov182_021b094c(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sPageStep3.succeeded) {
            DWCUtilFade_Start(3, 1, 1, 8);
        }
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(func_ov182_021b0990);
    }
}

static void func_ov182_021b0990(void) {
    if (DWCUtilFade_IsActive(0) != 0) {
        return;
    }
    if (!sPageStep3.succeeded && DWCUtilFade_IsActive(1) != 0) {
        return;
    }
    if (!DWCUtilUiObject_IsIdle()) {
        return;
    }
    func_ov182_021adb38(sPageStep3.succeeded != FALSE);
    DWCUtilUiEffect_Destroy();
    DWCUtilText_DestroyScreenBg(0);
    if (!sPageStep3.succeeded) {
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, 1);
    }
    DWCUtilDisplay_HidePlanes(0, 0x15);
    if (!sPageStep3.succeeded) {
        DWCUtilMain_SetLayout(2, 1);
        DWCUtilMain_SetUpdate(func_ov182_021b5bb8);
    } else {
        DWCUtilMain_SetLayout(0, 0);
        DWCUtilMain_SetUpdate(func_ov182_021b0ad0);
    }
}

static void func_ov182_021b0a24(DWCUtilTask *task, void *arg) {
    DWCUtilTouch_Update();
    DWCUtilTask_Run(0);
    func_ov182_021b08c8();
    func_ov182_021b08f0();
}

static void func_ov182_021b0a3c(void) {
    if (func_ov182_021b8f14() == 0) {
        DWCUtilSound_PlaySe(6);
        func_ov182_021b8eec();
        DWCUtilMain_SetUpdate(func_ov182_021b0a5c);
    }
}

static void func_ov182_021b0a5c(void) {
    if (!func_ov182_021b8f24()) {
        DWCUtilMain_SetUpdate(func_ov182_021b0920);
    }
}

// Keeps the message of the success for 1080 frames
static void func_ov182_021b0a74(void) {
    func_ov182_021b08c8();
    func_ov182_021b08ec();
    func_ov182_021b08f0();
    sPageStep3.frames++;
    if (sPageStep3.frames >= 1080) {
        DWCUtilSound_StopSe();
        DWCUtilMain_SetUpdate(func_ov182_021b0920);
    }
}

static void func_ov182_021b0aac(void) {
    DWCUtilSound_StopSe();
    DWCUtilSound_PlaySe(7);
    sPageStep3.succeeded = FALSE;
    DWCUtilMain_SetUpdate(func_ov182_021b0920);
}
