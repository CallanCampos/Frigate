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

static BOOL DWCUtilPageStep3_IsLidClosed(void);
static void DWCUtilPageStep3_LoadGraphics(void);
static void DWCUtilPageStep3_FadeIn(void);
static void DWCUtilPageStep3_WaitFadeIn(void);
static void DWCUtilPageStep3_WaitButtons(void);
static void DWCUtilPageStep3_RunSetup(void);
static void DWCUtilPageStep3_HandleInput(void);
static void func_ov182_021b08ec(void);
static void DWCUtilPageStep3_HandleCancel(void);
static void DWCUtilPageStep3_DisableButtons(void);
static void DWCUtilPageStep3_FadeOut(void);
static void DWCUtilPageStep3_WaitFadeOut(void);
static void DWCUtilPageStep3_InputTask(DWCUtilTask *task, void *arg);
static void DWCUtilPageStep3_WaitDialog(void);
static void DWCUtilPageStep3_WaitDialogClosed(void);
static void DWCUtilPageStep3_ShowSuccess(void);
static void DWCUtilPageStep3_Cancel(void);

static DWCUtilPageStep3 sPageStep3;

// Whether the lid is closed
static BOOL DWCUtilPageStep3_IsLidClosed(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void DWCUtilPageStep3_Init(void) {
    sPageStep3.frames = 0;
    DWCUtilPageStep3_LoadGraphics();
    DWCUtilBgOverlay_ShowConnectionSet();
    DWCUtilScreen_Start(0x36, -1, 0);
    DWCUtilGfxResource_ShowStep(2);
    DWCUtilGfxResource_ShowMessage(0x22);
    DWCUtilUiEffect_Init(0);
    DWCUtilAossSession_Init();
    DWCUtilSound_PlaySe(0xb);
    DWCUtilMain_SetUpdate(DWCUtilPageStep3_FadeIn);
}

static void DWCUtilPageStep3_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilPageStep3_FadeIn(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilDisplay_ShowPlanes(1, GX_PLANEMASK_BG0);
    DWCUtilDisplay_ShowPlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCUtilMain_SetUpdate(DWCUtilPageStep3_WaitFadeIn);
}

static void DWCUtilPageStep3_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(1);
        DWCUtilMain_SetUpdate(DWCUtilPageStep3_WaitButtons);
    }
}

static void DWCUtilPageStep3_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        sPageStep3.task = DWCUtilTask_Create(1, DWCUtilPageStep3_InputTask, NULL, 0x78);
        DWCUtilMain_SetUpdate(DWCUtilPageStep3_RunSetup);
    }
}

// Runs the setup
static void DWCUtilPageStep3_RunSetup(void) {
    DWCUtilPageStep3_HandleInput();
    func_ov182_021b08ec();
    DWCUtilPageStep3_HandleCancel();
    switch (DWCUtilAossSession_Run()) {
    case 0:
        break;
    case 1:
        if (sPageStep3.task != NULL) {
            sPageStep3.succeeded = TRUE;
            DWCUtilTask_Remove(1, sPageStep3.task);
            sPageStep3.task = NULL;
            DWCUtilMain_SetUpdate(DWCUtilPageStep3_ShowSuccess);
        }
        break;
    case 2:
        if (sPageStep3.task != NULL) {
            DWCUtilSound_StopSe();
            DWCUtilDialog_Open(0xc, 1, 1, -1, 0);
            DWCUtilSound_PlaySe(9);
            DWCUtilUiObject_Disable();
            DWCUtilTask_Remove(1, sPageStep3.task);
            sPageStep3.task = NULL;
            DWCUtilMain_SetUpdate(DWCUtilPageStep3_WaitDialog);
        }
        break;
    }
}

static void DWCUtilPageStep3_HandleInput(void) {
    if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (DWCUtilPageStep3_IsLidClosed()) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b08ec(void) {
}

// Cancels the setup
static void DWCUtilPageStep3_HandleCancel(void) {
    if (DWCUtilUiObject_GetState() == 0 && sPageStep3.task != NULL) {
        DWCUtilTask_QueueRemoval(1, sPageStep3.task);
        sPageStep3.task = NULL;
        DWCUtilUiObject_Disable();
        DWCUtilMain_SetUpdate(DWCUtilPageStep3_Cancel);
    }
}

static void DWCUtilPageStep3_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    if (sPageStep3.task != NULL) {
        DWCUtilTask_Remove(1, sPageStep3.task);
    }
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilPageStep3_FadeOut);
}

static void DWCUtilPageStep3_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sPageStep3.succeeded) {
            DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 0,
                          GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCUtilMain_SetUpdate(DWCUtilPageStep3_WaitFadeOut);
    }
}

static void DWCUtilPageStep3_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(0) != 0) {
        return;
    }
    if (!sPageStep3.succeeded && DWCUtilFade_IsActive(1) != 0) {
        return;
    }
    if (!DWCUtilUiObject_IsIdle()) {
        return;
    }
    DWCUtilAossSession_Exit(sPageStep3.succeeded != FALSE);
    DWCUtilUiEffect_Destroy();
    DWCUtilText_DestroyScreenBg(0);
    if (!sPageStep3.succeeded) {
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
    }
    DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    if (!sPageStep3.succeeded) {
        DWCUtilMain_SetLayout(2, 1);
        DWCUtilMain_SetUpdate(DWCUtilScreenWay_Init);
    } else {
        DWCUtilMain_SetLayout(0, 0);
        DWCUtilMain_SetUpdate(DWCUtilPageMulti4_Init);
    }
}

static void DWCUtilPageStep3_InputTask(DWCUtilTask *task, void *arg) {
    DWCUtilTouch_Update();
    DWCUtilTask_Run(0);
    DWCUtilPageStep3_HandleInput();
    DWCUtilPageStep3_HandleCancel();
}

static void DWCUtilPageStep3_WaitDialog(void) {
    if (DWCUtilDialog_GetChoice() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilDialog_Close();
        DWCUtilMain_SetUpdate(DWCUtilPageStep3_WaitDialogClosed);
    }
}

static void DWCUtilPageStep3_WaitDialogClosed(void) {
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilMain_SetUpdate(DWCUtilPageStep3_DisableButtons);
    }
}

// Keeps the message of the success for 1080 frames
static void DWCUtilPageStep3_ShowSuccess(void) {
    DWCUtilPageStep3_HandleInput();
    func_ov182_021b08ec();
    DWCUtilPageStep3_HandleCancel();
    sPageStep3.frames++;
    if (sPageStep3.frames >= 1080) {
        DWCUtilSound_StopSe();
        DWCUtilMain_SetUpdate(DWCUtilPageStep3_DisableButtons);
    }
}

static void DWCUtilPageStep3_Cancel(void) {
    DWCUtilSound_StopSe();
    DWCUtilSound_PlaySe(7);
    sPageStep3.succeeded = FALSE;
    DWCUtilMain_SetUpdate(DWCUtilPageStep3_DisableButtons);
}
