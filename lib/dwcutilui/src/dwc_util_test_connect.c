#include "dwc_util_test_connect.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_connection.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_way.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_test_done.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"
#include "nitro/os.h"

static BOOL DWCUtilTestConnect_IsLidClosed(void);
static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void DWCUtilTestConnect_HandleInput(void);
static void draw(void);
static void button(void);
static void endScene(void);
static void endScene1(void);
static void endScene2(void);
static void DWCUtilTestConnect_PollTest(void);
static void procError(void);
static void procError1(void);

// Whether the test connected; FALSE when it was ended or failed
static u8 sTestConnectPassed;

// A copy of gfl/ui.c's GCTX_HIDIsLidClosed
static BOOL DWCUtilTestConnect_IsLidClosed(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void DWCUtilTestConnect_Init(void) {
    sTestConnectPassed = FALSE;
    initGraphics();
    DWCi_HLlLoadSet();
    DWCi_CMNlMsgDraw(0x2a);
    DWCi_ANIMElInitEx(2);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(procInit1);
}

static void procInit1(void) {
    if (DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(1);
        DWCi_ChangeScene(procInit2);
    }
}

static void procInit2(void) {
    if (DWCi_BTNlGet() != -2) {
        DWCi_BTNlEnable();
        DWCi_ChangeScene(procScene);
    }
}

static void procScene(void) {
    OS_Sleep(10);
    DWCUtilTestConnect_PollTest();
    DWCUtilTestConnect_HandleInput();
    draw();
    button();
}

static void DWCUtilTestConnect_HandleInput(void) {
    if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    } else if (DWCUtilTestConnect_IsLidClosed()) {
        DWCi_BTNlSet(0);
    }
}

static void draw(void) {
}

// The button ends the test
static void button(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(7);
        DWCi_ChangeScene(endScene);
    }
}

static void endScene(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_BTNlEnd();
        if (!sTestConnectPassed) {
            DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(0) == 0 && (sTestConnectPassed || DWCi_EFFlCheck(1) == 0) && DWCi_BTNlIsEnd()) {
        DWCi_ANIMElEnd();
        DWCi_FNTlDeleteBgLcd(0);
        if (!sTestConnectPassed) {
            DWCi_COMMENTlDelete();
            DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        }
        DWCi_RAKUlEnd();
        if (!sTestConnectPassed) {
            DWCi_SetParam_(2, 1);
            DWCi_ChangeScene(DWCUtilScreenWay_Init);
        } else {
            DWCi_SetParam_(0, 0);
            DWCi_ChangeScene(DWCUtilTestDone_Init);
        }
    }
}

// What the test reports: a connection with a WEP key or none is taken into the connection being edited
static void DWCUtilTestConnect_PollTest(void) {
    switch (DWCi_RAKUlGetState()) {
    case 3:
        sTestConnectPassed = TRUE;
        DWCUtilSound_StopSe();
        DWCi_RAKUlSetResult();
        DWCi_ChangeScene(endScene);
        break;
    case 4:
        sTestConnectPassed = FALSE;
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(9);
        DWCUtilDialog_Open(0xd, 1, 1, -1, 0);
        DWCi_BTNlDisable();
        DWCi_ChangeScene(procError);
        break;
    case 5:
        sTestConnectPassed = FALSE;
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(0x12);
        DWCUtilDialog_Open(0xe, 1, 1, -1, 0);
        DWCi_BTNlDisable();
        DWCi_ChangeScene(procError);
        break;
    }
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
        DWCi_ChangeScene(endScene);
    }
}
