#include "sceneoptmovuser.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "move.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "sceneoptmenu.h"
#include "sceneoptmovcomm.h"
#include "sceneoptmovfailure.h"
#include "sound.h"
#include "window.h"

// Whether the move reported an event during the question, and the player's answer
typedef struct DWCUtilPageMove {
    u8 interrupted;
    u8 yes;
} DWCUtilPageMove;

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void DWCUtilPageMove_FadeOut(void);
static void endScene1(void);
static void endScene2(void);
static void cbMov(int event);

static DWCUtilPageMove sPageMove;

void DWCi_SceneOptMovUser(void) {
    const MBUserInfo *user = DWCi_MOVlGetChild();
    DWCUtilText *text = DWCi_FNTlFormBgEx(0, 0);
    u16 name[MB_USER_NAME_LENGTH + 1];

    sPageMove.yes = FALSE;
    sPageMove.interrupted = FALSE;
    initGraphics();
    MI_CpuClear16(name, sizeof(name));
    MI_CpuCopy16(user->name, name, user->nameLength * 2);
    DWCi_FNTlDrawRect(text, 8, 0x35, 0xf0, 0x1c, 2, 0x480, name);
    DWCi_FNTlRenewBg(text);
    DWCi_MOVlChangeCallback(cbMov);
    DWCi_WINlInitEx(0x1d, 4, 0, -1, 0);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCi_CMNlLoadResource("char/jb5Move.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlVisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(procInit1);
}

static void procInit1(void) {
    if (DWCi_EFFlCheck(0) == 0) {
        DWCi_ChangeScene(procInit2);
    }
}

static void procInit2(void) {
    if (DWCi_WINlGet() != -2) {
        DWCi_ChangeScene(procScene);
    }
}

static void procScene(void) {
    input();
    draw();
    switch (DWCi_WINlGet()) {
    case 0:
        sPageMove.yes = FALSE;
        DWCi_SNDlPlay(7);
        break;
    case 1:
        sPageMove.yes = TRUE;
        DWCi_SNDlPlay(0xe);
        break;
    default:
        return;
    }
    DWCi_MOVlChangeCallback(NULL);
    DWCi_WINlEnd();
    DWCi_ChangeScene(DWCUtilPageMove_FadeOut);
}

static void input(void) {
}

static void draw(void) {
}

static void DWCUtilPageMove_FadeOut(void) {
    if (!DWCi_WINlIsInit()) {
        if (!sPageMove.yes) {
            DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene1);
    }
}

// Sends the settings, or ends the move
static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        if (!sPageMove.interrupted && sPageMove.yes == TRUE) {
            DWCi_MOVlAdmitChild();
        } else {
            DWCi_MOVlEnd();
        }
        DWCi_ChangeScene(endScene2);
    }
}

// Waits for the end of the move, unless it goes on
static void endScene2(void) {
    if ((sPageMove.interrupted || !sPageMove.yes) && !DWCi_MOVlIsEnd()) {
        return;
    }
    DWCi_FNTlDeleteBgLcd(0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    if (!sPageMove.yes) {
        DWCi_COMMENTlDelete();
        DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    }
    DWCi_SetParam_(0, 1);
    if (sPageMove.interrupted) {
        DWCi_ChangeScene(DWCi_SceneOptMovFailure);
    } else if (!sPageMove.yes) {
        DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
        DWCi_ChangeScene(DWCi_SceneOptMenu);
    } else {
        DWCi_ChangeScene(DWCi_SceneOptMovComm);
    }
}

static void cbMov(int event) {
    sPageMove.interrupted = TRUE;
}
