#include "scenesetselectway.h"
#include "archive.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_bg_buffer.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "focus.h"
#include "graphics.h"
#include "headline.h"
#include "input.h"
#include "nitro/gx.h"
#include "scenesetaossbutton.h"
#include "scenesetlist.h"
#include "scenesetrakuready.h"
#include "scenesetsearch.h"
#include "scenesetselectap.h"
#include "setting.h"
#include "sound.h"

// The buttons: the search for an access point, the AOSS setup, the third setup and the settings by hand
#define SCREEN_WAY_BUTTONS 4

// What a move of the cursor does in sScreenWayMoves: nothing, go back to the button of the bottom row that the cursor
// left, or go to the button given (0 stays the top button, which also keeps the button left)
#define SCREEN_WAY_STAY -1
#define SCREEN_WAY_BACK -2

typedef struct DWCUtilScreenWay {
    // The button of the bottom row that the cursor last left, and the button under the cursor
    s8 left;
    s8 cursor;
    u8 chosen;
    // The screen whose palette rows light up the chosen button
    void *palette;
} DWCUtilScreenWay;

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
static void dispPush(void);

// The slot of the prompt's number by language; for each layout, where the cursor goes from each button to the left,
// up, right and down; and for each layout, where each button is touched and where the cursor frames it: left, top,
// right and bottom
static const s8 sScreenWayMsgSlots[7] = { 3, -1, -1, -1, -1, -1, 6 };
static const s8 sScreenWayMoves[2][SCREEN_WAY_BUTTONS][4] = {
    { { -1, -2, -1, -2 }, { 3, 0, 3, 0 }, { -1, -1, -1, -1 }, { 1, 0, 1, 0 } },
    { { -1, -2, -1, -2 }, { 3, 0, 2, 0 }, { 1, 0, 3, 0 }, { 2, 0, 1, 0 } },
};
static const u16 sScreenWayTouchRects[2][SCREEN_WAY_BUTTONS][4] = {
    { { 0x08, 0x20, 0xf8, 0x5c }, { 0x08, 0x64, 0x7e, 0xa0 }, { 0, 0, 0, 0 }, { 0x83, 0x64, 0xf8, 0xa0 } },
    { { 0x08, 0x20, 0xf8, 0x5c }, { 0x08, 0x64, 0x64, 0xa0 }, { 0x68, 0x64, 0xc4, 0xa0 }, { 0xc8, 0x64, 0xf8, 0xa0 } },
};
static const u16 sScreenWayCursorRects[2][SCREEN_WAY_BUTTONS][4] = {
    { { 0x06, 0x1e, 0xea, 0x4e }, { 0x06, 0x62, 0x70, 0x92 }, { 0, 0, 0, 0 }, { 0x80, 0x62, 0xea, 0x92 } },
    { { 0x06, 0x1e, 0xea, 0x4e }, { 0x06, 0x62, 0x56, 0x92 }, { 0x66, 0x62, 0xb6, 0x92 }, { 0xc6, 0x62, 0xea, 0x92 } },
};

static DWCUtilScreenWay sScreenWay;

void DWCi_SceneSetSelectWay(void) {
    DWCUtilSettingsEdit *edit = DWCi_SETTIGNlGet();

    sScreenWay.chosen = FALSE;
    if (sScreenWay.left == 0) {
        sScreenWay.left = 1;
    }
    if (DWCi_Language() != 0) {
        if (sScreenWay.cursor == 2) {
            sScreenWay.cursor = 0;
        }
        if (sScreenWay.left == 2) {
            sScreenWay.left = 1;
        }
    }
    initGraphics();
    DWCi_HLlLoadSet();
    DWCi_COMMENTlDispEx(0x32, sScreenWayMsgSlots[DWCi_Language()], edit->index + 1);
    DWCi_CMNlStep(1);
    DWCi_FOCUSlDisp4(sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][0],
                     sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][2],
                     sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][1],
                     sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][3]);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    char palettePath[] = "char/ybBgStep21.ncl.l";

    DWCi_CMNlLoadResource("char/jbBgStep2.ncg.l", GX_LoadBG2Char);
    DWCi_CMNlLoadResource("char/ybBgStep2.ncl.l", GX_LoadBGPltt);
    DWCi_CMNlLoadResource("char/jb3Way.nsc.l", GX_LoadBG2Scr);
    sScreenWay.palette = DWCi_ARClReadEx(DWCi_Char(palettePath), NULL, 4);
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

    for (i = 0; i < SCREEN_WAY_BUTTONS; i++) {
        if (DWCi_TPlCheckTrigger(sScreenWayTouchRects[DWCi_Flag(1)][i])) {
            DWCi_BTNlSet(1);
            sScreenWay.cursor = i;
            DWCi_FOCUSlDisp4(sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][0],
                             sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][2],
                             sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][1],
                             sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][3]);
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
        break;
    case 1:
        DWCi_SNDlPlay(6);
        dispPush();
        sScreenWay.chosen = TRUE;
        break;
    default:
        return;
    }
    DWCi_ChangeScene(endScene);
}

static void endScene(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        if (sScreenWay.chosen) {
            DWCi_BTNlEnd();
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(1) != 0 || DWCi_EFFlCheck(0) != 0) {
        return;
    }
    if (!DWCi_BTNlIsEnd()) {
        return;
    }
    DWCi_FOCUSlDelete();
    DWCi_COMMENTlDelete();
    DWCi_ARClRelease(sScreenWay.palette);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    if (!sScreenWay.chosen) {
        DWCi_SetParam_(2, 0);
        DWCi_ChangeScene(DWCi_SceneSetSelectAp);
        return;
    }
    switch (sScreenWay.cursor) {
    case 0:
        DWCi_SetParam_(2, 1);
        DWCi_ChangeScene(DWCi_SceneSetSearch);
        break;
    case 1:
        DWCi_SetParam_(2, 1);
        DWCi_ChangeScene(DWCi_SceneSetAossButton);
        break;
    case 2:
        DWCi_SetParam_(2, 1);
        DWCi_ChangeScene(DWCUtilTestStart_Init);
        break;
    case 3:
        DWCi_SetParam_(2, 0);
        DWCi_SetParam2_(0, 0);
        DWCi_SetListReturn(1);
        DWCi_ChangeScene(DWCUtilAddressSettings_Init);
        break;
    }
}

// Moves the cursor left, up, right or down (0 to 3)
static void moveFocus(int direction) {
    int cursor = sScreenWay.cursor;
    int next = sScreenWayMoves[DWCi_Flag(1)][cursor][direction];

    if (next == SCREEN_WAY_STAY) {
        return;
    }
    if (next == 0) {
        sScreenWay.left = cursor;
    }
    if (next == SCREEN_WAY_BACK) {
        sScreenWay.cursor = sScreenWay.left;
    } else {
        sScreenWay.cursor = next;
    }
    DWCi_SNDlPlay(8);
    DWCi_FOCUSlDisp4(sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][0],
                     sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][2],
                     sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][1],
                     sScreenWayCursorRects[DWCi_Flag(1)][sScreenWay.cursor][3]);
}

// Lights up the chosen button with the palette row of its place
static void dispPush(void) {
    u8 rows[SCREEN_WAY_BUTTONS] = { 1, 2, 3, 4 };

    DWCi_SCREENlCopyPalette(sScreenWay.palette, rows[sScreenWay.cursor], rows[sScreenWay.cursor]);
}
