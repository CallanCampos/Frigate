#include "scenesettest.h"
#include "anime.h"
#include "comment.h"
#include "common.h"
#include "dwc/ac.h"
#include "dwc/bm.h"
#include "dwc_util_shared.h"
#include "dwcutil/dwc_moveid.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "headline.h"
#include "heap.h"
#include "icon.h"
#include "main.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "sceneseterror.h"
#include "scenesettestcomp.h"
#include "setting.h"
#include "sound.h"
#include "task.h"

static void initGraphics(void);
static void initAc(void);
static void procInit(void);
static void procInit1(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void button(void);
static void DWCUtilAutoConnect_FadeOut(void);
static void endScene1(void);
static void endScene2(void);
static void taskAc(DWCUtilTask *task, void *arg);
static void *acAlloc(u32 name, s32 size);
static void acFree(u32 name, void *ptr, s32 size);

// What auto-connect is started with: the utility's allocator, DMA channel 3 and the radio on
static const DWCACConfig sAutoConnectParam = { acAlloc, acFree, 3, 1, 0, 0, 0, 0, 0 };

// Whether the connection test succeeded
static u8 sAutoConnectSucceeded;

void DWCi_SceneSetTest(void) {
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
    initAc();
    DWCi_ICONlInit(0);
    DWCi_SNDlPlay(0xc);
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

// Starts auto-connect with the connection being edited, or with the console's settings when the utility was started
// for that, and the task that waits for its result
static void initAc(void) {
    DWCUtilSettingsEdit *edit = DWCi_SETTIGNlGet();
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
    DWCi_TSKlForm(0, taskAc, NULL, 0x78);
}

static void procInit(void) {
    int from;

    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlVisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_GetParam2_(NULL, &from);
    if (from == 0) {
        DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_GXlVisible(1, GX_PLANEMASK_BG0);
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
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sAutoConnectSucceeded) {
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCi_SceneSetError);
        } else {
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCi_SceneSetTestCompletet);
        }
    }
}

// Runs auto-connect until it connects or fails, which keeps the error code for the error screen
static void taskAc(DWCUtilTask *task, void *arg) {
    s32 result = DWC_AC_Process();

    if (result != 0) {
        DWCi_ICONlEnd();
        DWCi_SNDlStop();
        if (result > 0) {
            sAutoConnectSucceeded = TRUE;
            DWCi_SNDlPlay(0x11);
        } else {
            DWCi_SetErrorCode(DWC_AC_GetStatus());
            DWCi_SNDlPlay(0x12);
        }
        DWCi_ChangeScene(DWCUtilAutoConnect_FadeOut);
        DWCi_TSKlDelete(0, task);
    }
}

static void *acAlloc(u32 name, s32 size) {
    return DWCi_HEAPlAllocEx(size, 0x20);
}

static void acFree(u32 name, void *ptr, s32 size) {
    DWCi_HEAPlFree2(ptr);
}
