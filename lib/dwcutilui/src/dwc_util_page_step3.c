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
static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void button(void);
static void DWCUtilPageStep3_DisableButtons(void);
static void endScene1(void);
static void endScene2(void);
static void DWCUtilPageStep3_InputTask(DWCUtilTask *task, void *arg);
static void procError(void);
static void procError1(void);
static void DWCUtilPageStep3_ShowSuccess(void);
static void procCancel(void);

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
    initGraphics();
    DWCi_HLlLoadSet();
    DWCi_COMMENTlDispEx(0x36, -1, 0);
    DWCi_CMNlStep(2);
    DWCi_CMNlMsgDraw(0x22);
    DWCi_ANIMElInitEx(0);
    DWCi_AOSSlInit();
    DWCUtilSound_PlaySe(0xb);
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

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(procInit1);
}

static void procInit1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(1);
        DWCi_ChangeScene(procInit2);
    }
}

static void procInit2(void) {
    if (DWCi_BTNlGet() != -2) {
        sPageStep3.task = DWCi_TSKlForm(1, DWCUtilPageStep3_InputTask, NULL, 0x78);
        DWCi_ChangeScene(procScene);
    }
}

// Runs the setup
static void procScene(void) {
    input();
    draw();
    button();
    switch (DWCUtilAossSession_Run()) {
    case 0:
        break;
    case 1:
        if (sPageStep3.task != NULL) {
            sPageStep3.succeeded = TRUE;
            DWCi_TSKlDeleteEx(1, sPageStep3.task);
            sPageStep3.task = NULL;
            DWCi_ChangeScene(DWCUtilPageStep3_ShowSuccess);
        }
        break;
    case 2:
        if (sPageStep3.task != NULL) {
            DWCUtilSound_StopSe();
            DWCUtilDialog_Open(0xc, 1, 1, -1, 0);
            DWCUtilSound_PlaySe(9);
            DWCi_BTNlDisable();
            DWCi_TSKlDeleteEx(1, sPageStep3.task);
            sPageStep3.task = NULL;
            DWCi_ChangeScene(procError);
        }
        break;
    }
}

static void input(void) {
    if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    } else if (DWCUtilPageStep3_IsLidClosed()) {
        DWCi_BTNlSet(0);
    }
}

static void draw(void) {
}

// Cancels the setup
static void button(void) {
    if (DWCi_BTNlGet() == 0 && sPageStep3.task != NULL) {
        DWCi_TSKlDelete(1, sPageStep3.task);
        sPageStep3.task = NULL;
        DWCi_BTNlDisable();
        DWCi_ChangeScene(procCancel);
    }
}

static void DWCUtilPageStep3_DisableButtons(void) {
    DWCi_BTNlDisable();
    if (sPageStep3.task != NULL) {
        DWCi_TSKlDeleteEx(1, sPageStep3.task);
    }
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_BTNlEnd();
        if (!sPageStep3.succeeded) {
            DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(0) != 0) {
        return;
    }
    if (!sPageStep3.succeeded && DWCi_EFFlCheck(1) != 0) {
        return;
    }
    if (!DWCi_BTNlIsEnd()) {
        return;
    }
    DWCUtilAossSession_Exit(sPageStep3.succeeded != FALSE);
    DWCi_ANIMElEnd();
    DWCi_FNTlDeleteBgLcd(0);
    if (!sPageStep3.succeeded) {
        DWCi_COMMENTlDelete();
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
    }
    DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    if (!sPageStep3.succeeded) {
        DWCi_SetParam_(2, 1);
        DWCi_ChangeScene(DWCUtilScreenWay_Init);
    } else {
        DWCi_SetParam_(0, 0);
        DWCi_ChangeScene(DWCUtilPageMulti4_Init);
    }
}

static void DWCUtilPageStep3_InputTask(DWCUtilTask *task, void *arg) {
    DWCi_IPTlRead();
    DWCUtilTask_Run(0);
    input();
    button();
}

static void procError(void) {
    if (DWCi_WINlGet() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCi_WINlEnd();
        DWCi_ChangeScene(procError1);
    }
}

static void procError1(void) {
    if (!DWCi_WINlIsInit()) {
        DWCi_ChangeScene(DWCUtilPageStep3_DisableButtons);
    }
}

// Keeps the message of the success for 1080 frames
static void DWCUtilPageStep3_ShowSuccess(void) {
    input();
    draw();
    button();
    sPageStep3.frames++;
    if (sPageStep3.frames >= 1080) {
        DWCUtilSound_StopSe();
        DWCi_ChangeScene(DWCUtilPageStep3_DisableButtons);
    }
}

static void procCancel(void) {
    DWCUtilSound_StopSe();
    DWCUtilSound_PlaySe(7);
    sPageStep3.succeeded = FALSE;
    DWCi_ChangeScene(DWCUtilPageStep3_DisableButtons);
}
