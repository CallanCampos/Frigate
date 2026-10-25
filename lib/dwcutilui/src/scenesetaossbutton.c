#include "scenesetaossbutton.h"
#include "anime.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "headline.h"
#include "input.h"
#include "libaoss.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "scenesetaosscomplete.h"
#include "scenesetselectway.h"
#include "sound.h"
#include "task.h"
#include "window.h"

// Whether the setup succeeded, the frames since, and the VBlank task that reads B and the lid while the setup runs
typedef struct DWCUtilPageStep3 {
    u8 succeeded;
    u16 frames;
    DWCUtilTask *task;
} DWCUtilPageStep3;

static BOOL DWCi_IsFold(void);
static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void button(void);
static void endScene(void);
static void endScene1(void);
static void endScene2(void);
static void taskVBlank(DWCUtilTask *task, void *arg);
static void procError(void);
static void procError1(void);
static void procWait(void);
static void procCancel(void);

static DWCUtilPageStep3 sPageStep3;

// Whether the lid is closed
static BOOL DWCi_IsFold(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void DWCi_SceneSetAossButton(void) {
    sPageStep3.frames = 0;
    initGraphics();
    DWCi_HLlLoadSet();
    DWCi_COMMENTlDispEx(0x36, -1, 0);
    DWCi_CMNlStep(2);
    DWCi_CMNlMsgDraw(0x22);
    DWCi_ANIMElInitEx(0);
    DWCi_AOSSlInit();
    DWCi_SNDlPlay(0xb);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCi_CMNlLoadResource("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCi_CMNlLoadResource("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCi_CMNlLoadResource("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
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
    DWCi_GXlVisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlVisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
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
        sPageStep3.task = DWCi_TSKlForm(1, taskVBlank, NULL, 0x78);
        DWCi_ChangeScene(procScene);
    }
}

// Runs the setup
static void procScene(void) {
    input();
    draw();
    button();
    switch (DWCi_AOSSlStart()) {
    case 0:
        break;
    case 1:
        if (sPageStep3.task != NULL) {
            sPageStep3.succeeded = TRUE;
            DWCi_TSKlDeleteEx(1, sPageStep3.task);
            sPageStep3.task = NULL;
            DWCi_ChangeScene(procWait);
        }
        break;
    case 2:
        if (sPageStep3.task != NULL) {
            DWCi_SNDlStop();
            DWCi_WINlInitEx(0xc, 1, 1, -1, 0);
            DWCi_SNDlPlay(9);
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
    } else if (DWCi_IsFold()) {
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

static void endScene(void) {
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
    DWCi_AOSSlEnd(sPageStep3.succeeded != FALSE);
    DWCi_ANIMElEnd();
    DWCi_FNTlDeleteBgLcd(0);
    if (!sPageStep3.succeeded) {
        DWCi_COMMENTlDelete();
        DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    }
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    if (!sPageStep3.succeeded) {
        DWCi_SetParam_(2, 1);
        DWCi_ChangeScene(DWCi_SceneSetSelectWay);
    } else {
        DWCi_SetParam_(0, 0);
        DWCi_ChangeScene(DWCi_SceneSetAossComplete);
    }
}

static void taskVBlank(DWCUtilTask *task, void *arg) {
    DWCi_IPTlRead();
    DWCi_TSKlAct(0);
    input();
    button();
}

static void procError(void) {
    if (DWCi_WINlGet() == 0) {
        DWCi_SNDlPlay(6);
        DWCi_WINlEnd();
        DWCi_ChangeScene(procError1);
    }
}

static void procError1(void) {
    if (!DWCi_WINlIsInit()) {
        DWCi_ChangeScene(endScene);
    }
}

// Keeps the message of the success for 1080 frames
static void procWait(void) {
    input();
    draw();
    button();
    sPageStep3.frames++;
    if (sPageStep3.frames >= 1080) {
        DWCi_SNDlStop();
        DWCi_ChangeScene(endScene);
    }
}

static void procCancel(void) {
    DWCi_SNDlStop();
    DWCi_SNDlPlay(7);
    sPageStep3.succeeded = FALSE;
    DWCi_ChangeScene(endScene);
}
