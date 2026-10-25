#include "scenesetsearch.h"
#include "anime.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "headline.h"
#include "input.h"
#include "main.h"
#include "nitro/gx.h"
#include "scenesetaplist.h"
#include "scenesetselectway.h"
#include "search.h"
#include "setting.h"
#include "sound.h"
#include "window.h"

// How the search ended, and the frames it has scanned for
typedef struct DWCUtilApSearch {
    u8 result; // 1 when it found an access point that the utility can set up, 2 when none, 3 when none of those
    u16 frames;
} DWCUtilApSearch;

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void DWCUtilApSearch_Main(void);
static void DWCUtilApSearch_CheckScan(void);
static void DWCUtilApSearch_HandleInput(void);
static void draw(void);
static void DWCUtilApSearch_HandleChoice(void);
static void DWCUtilApSearch_DisableButtons(void);
static void endScene1(void);
static void endScene2(void);
static void procError(void);
static void procError1(void);

static DWCUtilApSearch sApSearch;

void DWCi_SceneSetSearch(void) {
    sApSearch.frames = 0;
    sApSearch.result = 0;
    initGraphics();
    DWCi_COMMENTlDispEx(0x33, -1, 0);
    DWCi_HLlLoadSet();
    DWCi_CMNlStep(2);
    DWCi_CMNlMsgDraw(0x33);
    DWCi_ANIMElInitEx(0);
    DWCi_SEARCHlInit();
    DWCi_SEARCHlBegin();
    DWCi_SNDlPlay(0xa);
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
        DWCi_BTNlEnable();
        DWCi_ChangeScene(DWCUtilApSearch_Main);
    }
}

static void DWCUtilApSearch_Main(void) {
    DWCUtilApSearch_HandleInput();
    draw();
    DWCUtilApSearch_HandleChoice();
    DWCUtilApSearch_CheckScan();
}

// After 300 frames, looks at what the scan found
static void DWCUtilApSearch_CheckScan(void) {
    DWCUtilApEntry *entries;
    int count;
    int i;

    sApSearch.frames++;
    if (sApSearch.frames < 300) {
        return;
    }
    DWCi_SNDlStop();
    count = DWCi_SEARCHlGetApInfo(&entries);
    if (count == 0) {
        sApSearch.result = 2;
        DWCi_WINlInitEx(0xf, 1, 1, -1, 0);
        DWCi_SNDlPlay(0x12);
        DWCi_BTNlDisable();
        DWCi_ChangeScene(procError);
        return;
    }
    for (i = 0; i < count; i++) {
        if (entries[i].security != 2) {
            break;
        }
    }
    if (i == count) {
        sApSearch.result = 3;
        DWCi_WINlInitEx(0xe, 1, 1, -1, 0);
        DWCi_SNDlPlay(0x12);
        DWCi_BTNlDisable();
        DWCi_ChangeScene(procError);
        return;
    }
    sApSearch.result = 1;
    DWCi_SNDlPlay(0xf);
    DWCi_ChangeScene(DWCUtilApSearch_DisableButtons);
}

static void DWCUtilApSearch_HandleInput(void) {
    if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_SNDlStop();
        DWCi_BTNlSet(0);
    }
}

static void draw(void) {
}

// The button ends the search
static void DWCUtilApSearch_HandleChoice(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCi_SNDlStop();
        DWCi_SNDlPlay(7);
        DWCi_ChangeScene(DWCUtilApSearch_DisableButtons);
    }
}

static void DWCUtilApSearch_DisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_BTNlEnd();
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0 && DWCi_BTNlIsEnd()) {
        while (!DWCi_SEARCHlStop()) {
        }
        DWCi_ANIMElEnd();
        DWCi_FNTlDeleteBgLcd(0);
        DWCi_COMMENTlDelete();
        DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (sApSearch.result != 1) {
            DWCi_SEARCHlEnd();
            DWCi_SETTINGlSetSetting(DWCi_SETTIGNlGet()->index);
            DWCi_SetParam_(2, 1);
            DWCi_ChangeScene(DWCi_SceneSetSelectWay);
        } else {
            DWCi_SetParam_(0, 1);
            DWCi_SetParam2_(0, 0);
            DWCi_ChangeScene(DWCi_SceneSetApList);
        }
    }
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
        DWCi_ChangeScene(DWCUtilApSearch_DisableButtons);
    }
}
