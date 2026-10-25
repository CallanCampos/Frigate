#include "scenesetselectap.h"
#include "archive.h"
#include "button.h"
#include "cell.h"
#include "cellinfo.h"
#include "comment.h"
#include "common.h"
#include "dwc/bm.h"
#include "dwc_util_bg_buffer.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "focus.h"
#include "graphics.h"
#include "headline.h"
#include "input.h"
#include "nitro/gx.h"
#include "sceneend.h"
#include "scenemenu.h"
#include "scenesetlist.h"
#include "scenesetselectway.h"
#include "scenesetusbready.h"
#include "setting.h"
#include "sound.h"
#include "step.h"
#include "window.h"

// The buttons of the screen, as the cursor goes over them: the three connections, the wide button, and the three
// buttons that erase a connection
#define SCREEN_AP_CONNECTIONS 3
#define SCREEN_AP_WIDE_BUTTON 3
#define SCREEN_AP_FIRST_ERASE 4
#define SCREEN_AP_BUTTONS 7

// How the screen ends: not yet, with a button, or with B
enum {
    SCREEN_AP_RUNNING,
    SCREEN_AP_CHOSEN,
    SCREEN_AP_CANCELLED,
};

typedef struct DWCUtilScreenAp {
    // The screen whose palette rows light up the chosen button
    void *palette;
    // The buttons of the connections, and the buttons that erase them, none for a connection not in use
    DWCUtilOamNode *buttons[SCREEN_AP_CONNECTIONS];
    DWCUtilOamNode *eraseButtons[SCREEN_AP_CONNECTIONS];
    // The column that the cursor left the top or the bottom row from, for the wide button to go back to
    u8 column;
    u8 end;
} DWCUtilScreenAp;

typedef struct DWCUtilScreenApState {
    u8 cursor;
    DWCUtilScreenAp *work;
} DWCUtilScreenApState;

static void initGraphics(void);
static void initObj(void);
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
static void dispFocus(void);
static void moveFocus(int direction);
static void dispPushClear(void);
static void procErase(void);
static void procErase1(void);
static void dispPush(void);

// The cell of a connection's button by its status (DWCBMApInfo.status, 3 for one not in use), where the cursor
// frames each button and where it is touched (left, top, right and bottom), the slot of the erase prompt's number by
// language, and where the buttons of the connections and then their erase buttons are. Declared in the order that,
// with MWCC's sort of the file's data by size, gives the ROM's .rodata
static const u8 sScreenApCells[4] = { 0x13, 0x14, 0x12, 0x56 };
static const u16 sScreenApCursorRects[SCREEN_AP_BUTTONS][4] = {
    { 0x06, 0x1e, 0x46, 0x48 }, { 0x58, 0x1e, 0x98, 0x48 }, { 0xaa, 0x1e, 0xea, 0x48 }, { 0x06, 0x76, 0xea, 0x92 },
    { 0x09, 0x54, 0x43, 0x70 }, { 0x5b, 0x54, 0x95, 0x70 }, { 0xad, 0x54, 0xe7, 0x70 },
};
static const u16 sScreenApTouchRects[SCREEN_AP_BUTTONS][4] = {
    { 0x08, 0x20, 0x54, 0x56 }, { 0x5a, 0x20, 0xa6, 0x56 }, { 0xac, 0x20, 0xf8, 0x56 }, { 0x08, 0x78, 0xf8, 0xa0 },
    { 0x08, 0x54, 0x54, 0x70 }, { 0x5a, 0x54, 0xa6, 0x70 }, { 0xac, 0x54, 0xf8, 0x70 },
};
static const s8 sScreenApMsgSlots[8] = { 3, 0x2d, 0x27, 0x1d, 0x32, 0x30, 6, 0 };
static const u16 sScreenApButtonPos[SCREEN_AP_CONNECTIONS * 2][2] = {
    { 8, 0x30 }, { 0x5a, 0x30 }, { 0xac, 0x30 }, { 0x0c, 0x58 }, { 0x5e, 0x58 }, { 0xb0, 0x58 },
};

static DWCUtilScreenApState sScreenAp;

void DWCi_SceneSetSelectAp(void) {
    sScreenAp.work = DWCi_HEAPlAllocEx(sizeof(DWCUtilScreenAp), 4);
    sScreenAp.work->end = SCREEN_AP_RUNNING;
    initGraphics();
    DWCi_HLlLoad(1);
    DWCi_COMMENTlDispEx(0x2f, -1, 0);
    DWCi_CMNlStep(0);
    initObj();
    dispFocus();
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    char palettePath[] = "char/ybBgStep11.ncl.l";

    DWCi_CMNlLoadResource("char/ybObjWay.ncl.l", GX_LoadOBJPltt);
    DWCi_CMNlLoadResource("char/jbBgStep1.ncg.l", GX_LoadBG2Char);
    DWCi_CMNlLoadResource("char/jbBgStep1.ncl.l", GX_LoadBGPltt);
    DWCi_CMNlLoadResource("char/jb2Ap.nsc.l", GX_LoadBG2Scr);
    sScreenAp.work->palette = DWCi_ARClReadEx(DWCi_Char(palettePath), NULL, 4);
    DWCi_GXlVisible(1, GX_PLANEMASK_OBJ);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

// The buttons of the connections, and the erase buttons of those in use
static void initObj(void) {
    int i;
    int status;

    for (i = 0; i < SCREEN_AP_CONNECTIONS; i++) {
        status = DWCi_SETTINGlGetSetType(i);
        if (status == DWC_SETTYPE_NONE) {
            status = 3;
        } else {
            int pos = i + SCREEN_AP_CONNECTIONS;

            sScreenAp.work->eraseButtons[i] = DWCi_CEINlSetExCell(0, 0x11, TRUE);
            DWCi_CELLlSetPosition(sScreenAp.work->eraseButtons[i], -1, sScreenApButtonPos[pos][0],
                                  sScreenApButtonPos[pos][1]);
            DWCi_CELLlSetPriority(sScreenAp.work->eraseButtons[i], -1, 3);
        }
        sScreenAp.work->buttons[i] = DWCi_CEINlSetExCell(0, sScreenApCells[status], TRUE);
        DWCi_CELLlSetPosition(sScreenAp.work->buttons[i], -1, sScreenApButtonPos[i][0], sScreenApButtonPos[i][1]);
        DWCi_CELLlSetPriority(sScreenAp.work->buttons[i], -1, 3);
    }
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
    if (DWCi_BTNlGet() != -2 && DWCi_STEPlGet() != TRUE) {
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

    for (i = 0; i < SCREEN_AP_BUTTONS; i++) {
        if (DWCi_TPlCheckTrigger(sScreenApTouchRects[i])) {
            DWCi_BTNlSet(1);
            sScreenAp.cursor = i;
            dispFocus();
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
    int index;

    switch (DWCi_BTNlGet()) {
    case 0:
        switch (DWCi_Entry()) {
        case 0:
            DWCi_SNDlPlay(7);
            sScreenAp.work->end = SCREEN_AP_CANCELLED;
            break;
        case 1:
            DWCi_BTNlDisable();
            DWCi_ChangeScene(DWCi_SceneEnd);
            return;
        }
        break;
    case 1:
        sScreenAp.work->end = SCREEN_AP_CHOSEN;
        if (sScreenAp.cursor >= SCREEN_AP_FIRST_ERASE) {
            index = sScreenAp.cursor - SCREEN_AP_FIRST_ERASE;
            if (DWCi_SETTINGlGetSetType(index) == DWC_SETTYPE_NONE) {
                DWCi_SNDlPlay(9);
                DWCi_BTNlSetForce(-1);
                return;
            }
            DWCi_SNDlPlay(6);
            DWCi_WINlInitEx(0x46, 0, 1, sScreenApMsgSlots[DWCi_Language()], index + 1);
            dispPushClear();
            DWCi_BTNlDisable();
            DWCi_ChangeScene(procErase);
            return;
        }
        if (sScreenAp.cursor <= 2) {
            DWCi_SETTINGlSetSetting(sScreenAp.cursor);
        }
        DWCi_SNDlPlay(6);
        dispPush();
        break;
    default:
        return;
    }
    DWCi_ChangeScene(endScene);
}

static void endScene(void) {
    if (sScreenAp.work->end == SCREEN_AP_CANCELLED) {
        DWCi_STEPlEnd();
    }
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        if (sScreenAp.work->end == SCREEN_AP_CHOSEN &&
            (sScreenAp.cursor == SCREEN_AP_WIDE_BUTTON || DWCi_SETTIGNlGet()->connection.setType != DWC_SETTYPE_NONE)) {
            DWCi_BTNlEnd();
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    u32 i;

    if (DWCi_EFFlCheck(1) != 0 || DWCi_EFFlCheck(0) != 0) {
        return;
    }
    if (!DWCi_STEPlIsEnd() || !DWCi_BTNlIsEnd()) {
        return;
    }
    for (i = 0; i < SCREEN_AP_CONNECTIONS; i++) {
        if (sScreenAp.work->buttons[i] != NULL) {
            DWCi_CELLlDelete(sScreenAp.work->buttons[i]);
        }
    }
    for (i = 0; i < SCREEN_AP_CONNECTIONS; i++) {
        if (sScreenAp.work->eraseButtons[i] != NULL) {
            DWCi_CELLlDelete(sScreenAp.work->eraseButtons[i]);
        }
    }
    DWCi_FOCUSlDelete();
    DWCi_COMMENTlDelete();
    DWCi_ARClRelease(sScreenAp.work->palette);
    DWCi_CMNlLoadResource("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    if (sScreenAp.work->end == SCREEN_AP_CANCELLED) {
        DWCi_SetParam_(0, 0);
        DWCi_ChangeScene(DWCi_SceneMenu);
    } else {
        switch (sScreenAp.cursor) {
        case 0:
        case 1:
        case 2:
            DWCi_SetParam_(2, 0);
            if (DWCi_SETTIGNlGet()->connection.setType == DWC_SETTYPE_NONE) {
                DWCi_ChangeScene(DWCi_SceneSetSelectWay);
            } else {
                DWCi_SetParam2_(0, 0);
                DWCi_SetListReturn(0);
                DWCi_ChangeScene(DWCUtilAddressSettings_Init);
            }
            break;
        case SCREEN_AP_WIDE_BUTTON:
            DWCi_SetParam_(2, 1);
            DWCi_ChangeScene(DWCi_SceneSetUsbReady);
            break;
        }
    }
    DWCi_HEAPlFree_(&sScreenAp.work);
}

// Frames the button under the cursor: the buttons of the top row and the wide button with the four corners, an erase
// button with the pair of sprites
static void dispFocus(void) {
    if (sScreenAp.cursor < SCREEN_AP_FIRST_ERASE) {
        DWCi_FOCUSlDisp4(sScreenApCursorRects[sScreenAp.cursor][0], sScreenApCursorRects[sScreenAp.cursor][2],
                         sScreenApCursorRects[sScreenAp.cursor][1], sScreenApCursorRects[sScreenAp.cursor][3]);
        return;
    }
    DWCi_FOCUSlDisp(0, sScreenApCursorRects[sScreenAp.cursor][0], sScreenApCursorRects[sScreenAp.cursor][2],
                    sScreenApCursorRects[sScreenAp.cursor][1]);
}

// Moves the cursor left, up, right or down (0 to 3)
static void moveFocus(int direction) {
    BOOL moved = TRUE;

    switch (sScreenAp.cursor) {
    case 0:
        sScreenAp.work->column = 0;
        if (direction == 0) {
            sScreenAp.cursor = 2;
        } else if (direction == 2) {
            sScreenAp.cursor = 1;
        } else if (direction == 1) {
            sScreenAp.cursor = 3;
        } else {
            sScreenAp.cursor = 4;
        }
        break;
    case 1:
        sScreenAp.work->column = 1;
        if (direction == 0) {
            sScreenAp.cursor = 0;
        } else if (direction == 2) {
            sScreenAp.cursor = 2;
        } else if (direction == 1) {
            sScreenAp.cursor = 3;
        } else {
            sScreenAp.cursor = 5;
        }
        break;
    case 2:
        sScreenAp.work->column = 2;
        if (direction == 0) {
            sScreenAp.cursor = 1;
        } else if (direction == 2) {
            sScreenAp.cursor = 0;
        } else if (direction == 1) {
            sScreenAp.cursor = 3;
        } else {
            sScreenAp.cursor = 6;
        }
        break;
    case 3: {
        u8 column = sScreenAp.work->column;
        int erase = column + SCREEN_AP_FIRST_ERASE;

        if (direction == 1) {
            sScreenAp.cursor = erase;
        } else if (direction == 3) {
            sScreenAp.cursor = column;
        } else {
            moved = FALSE;
        }
        break;
    }
    case 4:
        sScreenAp.work->column = 0;
        if (direction == 0) {
            sScreenAp.cursor = 6;
        } else if (direction == 2) {
            sScreenAp.cursor = 5;
        } else if (direction == 1) {
            sScreenAp.cursor = 0;
        } else {
            sScreenAp.cursor = 3;
        }
        break;
    case 5:
        sScreenAp.work->column = 1;
        if (direction == 0) {
            sScreenAp.cursor = 4;
        } else if (direction == 2) {
            sScreenAp.cursor = 6;
        } else if (direction == 1) {
            sScreenAp.cursor = 1;
        } else {
            sScreenAp.cursor = 3;
        }
        break;
    case 6:
        sScreenAp.work->column = 2;
        if (direction == 0) {
            sScreenAp.cursor = 5;
        } else if (direction == 2) {
            sScreenAp.cursor = 4;
        } else if (direction == 1) {
            sScreenAp.cursor = 2;
        } else {
            sScreenAp.cursor = 3;
        }
        break;
    }
    if (moved) {
        DWCi_SNDlPlay(8);
        dispFocus();
    }
}

// Shows the chosen erase button pressed
static void dispPushClear(void) {
    int index = sScreenAp.cursor - SCREEN_AP_FIRST_ERASE;

    DWCi_CEINlSet(0, 0x32, DWCi_CELLlGetObj(sScreenAp.work->eraseButtons[index], 0));
    DWCi_CELLlSetPosition(sScreenAp.work->eraseButtons[index], -1, sScreenApButtonPos[index + 3][0],
                          sScreenApButtonPos[index + 3][1]);
    DWCi_CELLlSetPriority(sScreenAp.work->eraseButtons[index], -1, 3);
}

// The answer to whether to erase the connection: yes erases it and shows its button as not in use, no shows the erase
// button again
static void procErase(void) {
    int index = sScreenAp.cursor - SCREEN_AP_FIRST_ERASE;

    switch (DWCi_WINlGet()) {
    case 1:
        DWCi_SNDlPlay(0xe);
        DWCi_SETTINGlErasePage(index);
        DWCi_CEINlSet(0, 0x56, DWCi_CELLlGetObj(sScreenAp.work->buttons[index], 0));
        DWCi_CELLlSetPosition(sScreenAp.work->buttons[index], -1, sScreenApButtonPos[index][0],
                              sScreenApButtonPos[index][1]);
        DWCi_CELLlSetPriority(sScreenAp.work->buttons[index], -1, 3);
        DWCi_CELLlDelete(sScreenAp.work->eraseButtons[index]);
        sScreenAp.work->eraseButtons[index] = NULL;
        break;
    case 0:
        DWCi_SNDlPlay(7);
        DWCi_CEINlSet(0, 0x11, DWCi_CELLlGetObj(sScreenAp.work->eraseButtons[index], 0));
        DWCi_CELLlSetPosition(sScreenAp.work->eraseButtons[index], -1, sScreenApButtonPos[index + 3][0],
                              sScreenApButtonPos[index + 3][1]);
        DWCi_CELLlSetPriority(sScreenAp.work->eraseButtons[index], -1, 3);
        break;
    default:
        return;
    }
    DWCi_WINlEnd();
    DWCi_ChangeScene(procErase1);
}

static void procErase1(void) {
    if (!DWCi_WINlIsInit()) {
        DWCi_BTNlEnable();
        DWCi_ChangeScene(procScene);
    }
}

// Lights up the chosen button: the palette row of its place, and the button's own palette by the connection's status
static void dispPush(void) {
    u8 rows[4] = { 3, 4, 5, 7 };
    u8 palettes[4] = { 10, 9, 11, 6 };
    int status;

    if (sScreenAp.cursor <= SCREEN_AP_WIDE_BUTTON) {
        DWCi_SCREENlCopyPalette(sScreenAp.work->palette, rows[sScreenAp.cursor], rows[sScreenAp.cursor]);
        if (sScreenAp.cursor != SCREEN_AP_WIDE_BUTTON) {
            status = DWCi_SETTINGlGetSetType(sScreenAp.cursor);
            if (status > 2) {
                status = 3;
            }
            DWCi_CELLlSetPalette(sScreenAp.work->buttons[sScreenAp.cursor], -1, 0, palettes[status]);
        }
    }
}
