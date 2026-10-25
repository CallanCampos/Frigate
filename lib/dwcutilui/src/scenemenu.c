#include "scenemenu.h"
#include "archive.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_bg_buffer.h"
#include "dwc_util_main.h"
#include "effect.h"
#include "focus.h"
#include "graphics.h"
#include "headline.h"
#include "input.h"
#include "nitro/gx.h"
#include "sceneend.h"
#include "sceneoptmenu.h"
#include "scenesetselectap.h"
#include "sound.h"
#include "step.h"

// The choice on the first screen, and the palette whose rows light up the chosen button
typedef struct DWCUtilMainMenu {
    u8 selection;
    void *palette;
} DWCUtilMainMenu;

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
static void moveFocus(int direction);
static void dispPushButton(void);

// Where the cursor frames the two buttons, and where they are touched: left, top, right and bottom
static const u16 sCursorRects[2][4] = { { 6, 0x1e, 0x9e, 0x92 }, { 0xb2, 0x1e, 0xea, 0x92 } };
static const u16 sButtonRects[2][4] = { { 8, 0x20, 0xac, 0xa0 }, { 0xb4, 0x20, 0xf8, 0xa0 } };

static DWCUtilMainMenu sMainMenu;

void DWCi_SceneMenu(void) {
    initGraphics();
    DWCi_HLlLoad(0);
    DWCi_COMMENTlDispEx(0x2e, -1, 0);
    DWCi_STEPlChangeScr(4);
    DWCi_FOCUSlDisp4(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                     sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    char palettePath[] = "char/ybBgStep11.ncl.l";

    DWCi_CMNlLoadResource("char/jbBgStep1.ncg.l", GX_LoadBG2Char);
    DWCi_CMNlLoadResource("char/jbBgStep1.ncl.l", GX_LoadBGPltt);
    DWCi_CMNlLoadResource("char/jb2Menu.nsc.l", GX_LoadBG2Scr);
    sMainMenu.palette = DWCi_ARClReadEx(DWCi_Char(palettePath), NULL, 4);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCi_GXlVisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlVisible(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(procInit1);
}

static void procInit1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(0);
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
    input();
    draw();
    button();
}

static void input(void) {
    u32 i;

    for (i = 0; i < 2; i++) {
        if (DWCi_TPlCheckTrigger(sButtonRects[i])) {
            DWCi_BTNlSet(1);
            sMainMenu.selection = i;
            DWCi_FOCUSlDisp4(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                             sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
            return;
        }
    }
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(1);
    } else if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    } else if (DWCi_IPTlCheckRepeat(0x40)) {
        moveFocus(1);
    } else if (DWCi_IPTlCheckRepeat(0x80)) {
        moveFocus(3);
    } else if (DWCi_IPTlCheckRepeat(0x20)) {
        moveFocus(0);
    } else if (DWCi_IPTlCheckRepeat(0x10)) {
        moveFocus(2);
    }
}

static void draw(void) {
}

static void button(void) {
    switch (DWCi_BTNlGet()) {
    case 0:
        DWCi_SNDlPlay(7);
        DWCi_ChangeScene(DWCi_SceneEnd);
        break;
    case 1:
        DWCi_SNDlPlay(6);
        dispPushButton();
        DWCi_ChangeScene(endScene);
        break;
    }
}

static void endScene(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_FOCUSlDelete();
        DWCi_COMMENTlDelete();
        DWCi_ARClRelease(sMainMenu.palette);
        DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        switch (sMainMenu.selection) {
        case 0:
            DWCi_SetParam_(1, 0);
            DWCi_ChangeScene(DWCi_SceneSetSelectAp);
            break;
        case 1:
            DWCi_SetParam_(0, 0);
            DWCi_ChangeScene(DWCi_SceneOptMenu);
            break;
        }
    }
}

// Left and right move the cursor to the other button; up and down don't move it
static void moveFocus(int direction) {
    if (direction != 1 && direction != 3) {
        sMainMenu.selection ^= 1;
        DWCi_SNDlPlay(8);
        DWCi_FOCUSlDisp4(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                         sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
    }
}

// Lights up the chosen button with its row of the palette
static void dispPushButton(void) {
    u8 rows[2] = { 1, 2 };

    DWCi_SCREENlCopyPalette(sMainMenu.palette, rows[sMainMenu.selection], rows[sMainMenu.selection]);
}
