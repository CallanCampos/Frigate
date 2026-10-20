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

static void func_ov182_021b021c(void);
static void func_ov182_021b026c(void);
static void func_ov182_021b0290(void);
static void func_ov182_021b02a8(void);
static void func_ov182_021b02c4(void);
static void func_ov182_021b030c(void);
static void func_ov182_021b0310(void);
static void func_ov182_021b0314(void);
static void func_ov182_021b0350(void);
static void func_ov182_021b0390(void);
static void func_ov182_021b0414(int event);

static DWCUtilPageMove sPageMove;

void func_ov182_021b0198(void) {
    const MBUserInfo *user = func_ov182_021ae240();
    DWCUtilText *text = func_ov182_021bbdec(0, 0);
    u16 name[MB_USER_NAME_LENGTH + 1];

    sPageMove.yes = FALSE;
    sPageMove.interrupted = FALSE;
    func_ov182_021b021c();
    MI_CpuClear16(name, sizeof(name));
    MI_CpuCopy16(user->name, name, user->nameLength * 2);
    func_ov182_021bc0a4(text, 8, 0x35, 0xf0, 0x1c, 2, 0x480, name);
    func_ov182_021bc184(text);
    func_ov182_021ae224(func_ov182_021b0414);
    func_ov182_021b8cf0(0x1d, 4, 0, -1, 0);
    func_ov182_021adfbc(func_ov182_021b026c);
}

static void func_ov182_021b021c(void) {
    func_ov182_021ab67c("char/jb5Move.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b026c(void) {
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021b0290);
}

static void func_ov182_021b0290(void) {
    if (func_ov182_021bbaac(0) == 0) {
        func_ov182_021adfbc(func_ov182_021b02a8);
    }
}

static void func_ov182_021b02a8(void) {
    if (func_ov182_021b8f14() != -2) {
        func_ov182_021adfbc(func_ov182_021b02c4);
    }
}

static void func_ov182_021b02c4(void) {
    func_ov182_021b030c();
    func_ov182_021b0310();
    switch (func_ov182_021b8f14()) {
    case 0:
        sPageMove.yes = FALSE;
        func_ov182_021b80f4(7);
        break;
    case 1:
        sPageMove.yes = TRUE;
        func_ov182_021b80f4(0xe);
        break;
    default:
        return;
    }
    func_ov182_021ae224(NULL);
    func_ov182_021b8eec();
    func_ov182_021adfbc(func_ov182_021b0314);
}

static void func_ov182_021b030c(void) {
}

static void func_ov182_021b0310(void) {
}

static void func_ov182_021b0314(void) {
    if (!func_ov182_021b8f24()) {
        if (!sPageMove.yes) {
            func_ov182_021bbac4(3, 1, 1, 8);
        }
        func_ov182_021bbac4(3, 0, 0x15, 8);
        func_ov182_021adfbc(func_ov182_021b0350);
    }
}

// Sends the settings, or ends the move
static void func_ov182_021b0350(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        if (!sPageMove.interrupted && sPageMove.yes == TRUE) {
            func_ov182_021ae238();
        } else {
            func_ov182_021ae1d4();
        }
        func_ov182_021adfbc(func_ov182_021b0390);
    }
}

// Waits for the end of the move, unless it goes on
static void func_ov182_021b0390(void) {
    if ((sPageMove.interrupted || !sPageMove.yes) && !func_ov182_021ae210()) {
        return;
    }
    func_ov182_021bbf7c(0);
    func_ov182_021bc228(0, 0x15);
    if (!sPageMove.yes) {
        DWCUtilScreen_Stop();
        func_ov182_021bc228(1, 1);
    }
    func_ov182_021adfc8(0, 1);
    if (sPageMove.interrupted) {
        func_ov182_021adfbc(func_ov182_021aff3c);
    } else if (!sPageMove.yes) {
        func_ov182_021bc228(1, 1);
        func_ov182_021adfbc(func_ov182_021af25c);
    } else {
        func_ov182_021adfbc(func_ov182_021af714);
    }
}

static void func_ov182_021b0414(int event) {
    sPageMove.interrupted = TRUE;
}
