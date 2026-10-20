#include "dwc_util_page_move.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_move.h"
#include "dwc_util_notice_screen.h"
#include "dwc_util_option_menu.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "dwc_util_top_menu.h"
#include "nitro/gx.h"
#include "nitro/mi.h"

// Whether the move reported an event during the question, and the player's answer
typedef struct DWCUtilPageMove {
    u8 interrupted;
    u8 yes;
} DWCUtilPageMove;

static void DWCUtilPageMove_LoadGraphics(void);
static void DWCUtilPageMove_FadeIn(void);
static void DWCUtilPageMove_WaitFadeIn(void);
static void DWCUtilPageMove_WaitDialog(void);
static void DWCUtilPageMove_Main(void);
static void func_ov182_021b030c(void);
static void func_ov182_021b0310(void);
static void DWCUtilPageMove_FadeOut(void);
static void DWCUtilPageMove_WaitFadeOut(void);
static void DWCUtilPageMove_WaitMoveEnd(void);
static void DWCUtilPageMove_OnMoveEvent(int event);

static DWCUtilPageMove sPageMove;

void DWCUtilPageMove_Init(void) {
    const MBUserInfo *user = func_ov182_021ae240();
    DWCUtilText *text = DWCUtilText_CreateBg(0, 0);
    u16 name[MB_USER_NAME_LENGTH + 1];

    sPageMove.yes = FALSE;
    sPageMove.interrupted = FALSE;
    DWCUtilPageMove_LoadGraphics();
    MI_CpuClear16(name, sizeof(name));
    MI_CpuCopy16(user->name, name, user->nameLength * 2);
    DWCUtilText_DrawRect(text, 8, 0x35, 0xf0, 0x1c, 2, 0x480, name);
    DWCUtilText_MarkDirty(text);
    func_ov182_021ae224(DWCUtilPageMove_OnMoveEvent);
    DWCUtilDialog_Open(0x1d, 4, 0, -1, 0);
    DWCUtilMain_SetUpdate(DWCUtilPageMove_FadeIn);
}

static void DWCUtilPageMove_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/jb5Move.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilPageMove_FadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilPageMove_WaitFadeIn);
}

static void DWCUtilPageMove_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilMain_SetUpdate(DWCUtilPageMove_WaitDialog);
    }
}

static void DWCUtilPageMove_WaitDialog(void) {
    if (DWCUtilDialog_GetChoice() != -2) {
        DWCUtilMain_SetUpdate(DWCUtilPageMove_Main);
    }
}

static void DWCUtilPageMove_Main(void) {
    func_ov182_021b030c();
    func_ov182_021b0310();
    switch (DWCUtilDialog_GetChoice()) {
    case 0:
        sPageMove.yes = FALSE;
        DWCUtilSound_PlaySe(7);
        break;
    case 1:
        sPageMove.yes = TRUE;
        DWCUtilSound_PlaySe(0xe);
        break;
    default:
        return;
    }
    func_ov182_021ae224(NULL);
    DWCUtilDialog_Close();
    DWCUtilMain_SetUpdate(DWCUtilPageMove_FadeOut);
}

static void func_ov182_021b030c(void) {
}

static void func_ov182_021b0310(void) {
}

static void DWCUtilPageMove_FadeOut(void) {
    if (!DWCUtilDialog_IsOpen()) {
        if (!sPageMove.yes) {
            DWCUtilFade_Start(3, 1, 1, 8);
        }
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(DWCUtilPageMove_WaitFadeOut);
    }
}

// Sends the settings, or ends the move
static void DWCUtilPageMove_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        if (!sPageMove.interrupted && sPageMove.yes == TRUE) {
            func_ov182_021ae238();
        } else {
            func_ov182_021ae1d4();
        }
        DWCUtilMain_SetUpdate(DWCUtilPageMove_WaitMoveEnd);
    }
}

// Waits for the end of the move, unless it goes on
static void DWCUtilPageMove_WaitMoveEnd(void) {
    if ((sPageMove.interrupted || !sPageMove.yes) && !func_ov182_021ae210()) {
        return;
    }
    DWCUtilText_DestroyScreenBg(0);
    DWCUtilDisplay_HidePlanes(0, 0x15);
    if (!sPageMove.yes) {
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, 1);
    }
    DWCUtilMain_SetLayout(0, 1);
    if (sPageMove.interrupted) {
        DWCUtilMain_SetUpdate(func_ov182_021aff3c);
    } else if (!sPageMove.yes) {
        DWCUtilDisplay_HidePlanes(1, 1);
        DWCUtilMain_SetUpdate(func_ov182_021af25c);
    } else {
        DWCUtilMain_SetUpdate(func_ov182_021af714);
    }
}

static void DWCUtilPageMove_OnMoveEvent(int event) {
    sPageMove.interrupted = TRUE;
}
