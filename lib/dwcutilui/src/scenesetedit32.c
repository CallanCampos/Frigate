#include "scenesetedit32.h"
#include "cellinfo.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_oam_attr.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "headline.h"
#include "keyboard.h"
#include "nitro/gx.h"
#include "obj.h"
#include "scenesetaplist.h"
#include "scenesetlist.h"
#include "scenesettestconfirm.h"
#include "setting.h"
#include "sound.h"
#include "str.h"
#include "window.h"

// The longest text, and the font's character of the space
#define WEP_KEYBOARD_LENGTH 32
#define WEP_KEYBOARD_SPACE 0xe01d

// How the entry ended
enum {
    WEP_KEYBOARD_CANCELLED,
    WEP_KEYBOARD_ENTERED,
    WEP_KEYBOARD_INVALID,
};

// The text being entered and the cursor after it
typedef struct DWCUtilWepKeyboard {
    DWCUtilText *text;
    GXOamAttr *cursor;
    u8 str[WEP_KEYBOARD_LENGTH + 1];
    u8 length;
    u8 result;
} DWCUtilWepKeyboard;

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void DWCUtilWepKeyboard_StartClose(void);
static void endScene1(void);
static void endScene2(void);
static void endScene3(void);
static void drawSsid(void);
static void dispCaret(void);
static void procWin(void);
static void procWin1(void);
static BOOL isCorrect(void);
static void procError(void);
static void procError1(void);

static DWCUtilWepKeyboard *sWepKeyboard;

void DWCi_SceneSetEdit32(void) {
    u8 msgIndices[2] = { 0x3e, 0x3d };
    int field;
    int fromList;

    sWepKeyboard = DWCi_HEAPlAllocEx(sizeof(DWCUtilWepKeyboard), 4);
    DWCi_GetParam2_(&field, &fromList);
    if (field == 0) {
        DWCi_SETTINGlGetSsid(sWepKeyboard->str);
        sWepKeyboard->length = DWCi_STRlNLen(sWepKeyboard->str, WEP_KEYBOARD_LENGTH);
    }
    initGraphics();
    DWCi_HLlLoad(field + 9);
    if (fromList == 1) {
        DWCi_COMMENTlDispEx(0x35, -1, 0);
    } else {
        DWCi_COMMENTlDispEx(msgIndices[field], -1, 0);
    }
    DWCi_CMNlStep(2);
    sWepKeyboard->text = DWCi_FNTlFormBgEx(0, 0);
    sWepKeyboard->cursor = DWCi_CEINlSetExObj(0, 0x3e);
    G2_SetOBJPriority(sWepKeyboard->cursor, 3);
    dispCaret();
    drawSsid();
    DWCi_ChangeScene(procInit);
}

// Where the characters are drawn: the two rows' y, the size of a character and each column's x
static const u8 sWepKeyboardRowY[2] = { 0x20, 0x31 };
static const u16 sWepKeyboardCharSize[2] = { 14, 16 };
static const u8 sWepKeyboardColumnX[16] = {
    0x08, 0x17, 0x26, 0x35, 0x44, 0x53, 0x62, 0x71, 0x80, 0x8f, 0x9e, 0xad, 0xbc, 0xcb, 0xda, 0xe9,
};

static void initGraphics(void) {
    DWCi_CMNlLoadResource("char/ybObjKb.ncl.l", GX_LoadOBJPltt);
    DWCi_CMNlLoadResource("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCi_CMNlLoadResource("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCi_CMNlLoadResource("char/xb4Edit.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(2);
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
        DWCi_KBlInit();
        DWCi_SNDlPlay(0x14);
        if (sWepKeyboard->length == 0) {
            DWCi_KBlSetErase(FALSE);
        }
        if (sWepKeyboard->length == WEP_KEYBOARD_LENGTH) {
            DWCi_KBlSetInput(FALSE);
        }
        DWCi_ChangeScene(procInit2);
    }
}

static void procInit2(void) {
    if (DWCi_KBlGet() != 0xff) {
        DWCi_ChangeScene(procScene);
    }
}

static void procScene(void) {
    input();
    draw();
}

static void input(void) {
    int key = DWCi_KBlGet();

    switch (key) {
    case 0:
        break;
    case 0x80:
        if (sWepKeyboard->length != 0) {
            DWCi_SNDlPlay(3);
            sWepKeyboard->length--;
            sWepKeyboard->str[sWepKeyboard->length] = '\0';
            if (sWepKeyboard->length == 0) {
                DWCi_KBlSetErase(FALSE);
            }
            DWCi_KBlSetInput(TRUE);
        }
        break;
    case 0x82:
        DWCi_SNDlPlay(7);
        sWepKeyboard->result = WEP_KEYBOARD_CANCELLED;
        DWCi_ChangeScene(DWCUtilWepKeyboard_StartClose);
        return;
    case 0x83:
        if (isCorrect()) {
            DWCi_SNDlPlay(6);
            sWepKeyboard->result = WEP_KEYBOARD_ENTERED;
        } else {
            sWepKeyboard->result = WEP_KEYBOARD_INVALID;
            DWCi_SNDlPlay(9);
        }
        G2_SetOBJEffect(sWepKeyboard->cursor, GX_OAM_EFFECT_NODISPLAY, 0);
        DWCi_ChangeScene(DWCUtilWepKeyboard_StartClose);
        return;
    case WEP_KEYBOARD_SPACE:
    default:
        if (sWepKeyboard->length != WEP_KEYBOARD_LENGTH) {
            DWCi_SNDlPlay(1);
            sWepKeyboard->str[sWepKeyboard->length] = key;
            sWepKeyboard->length++;
            DWCi_KBlSetErase(TRUE);
            if (sWepKeyboard->length == WEP_KEYBOARD_LENGTH) {
                DWCi_KBlSetInput(FALSE);
            }
        }
        break;
    }
    drawSsid();
    dispCaret();
}

static void draw(void) {
}

static void DWCUtilWepKeyboard_StartClose(void) {
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_KBlEnd();
        DWCi_SNDlPlay(0x15);
        DWCi_ChangeScene(endScene2);
    }
}

// Once the keyboard has closed, asks whether to keep the text, or shows that it isn't a WEP key
static void endScene2(void) {
    int msgIndices[2] = { 0x47, 0x48 };
    int fromList;

    if (!DWCi_KBlIsInit()) {
        if (sWepKeyboard->result == WEP_KEYBOARD_CANCELLED) {
            DWCi_ChangeScene(endScene3);
            return;
        }
        if (sWepKeyboard->result == WEP_KEYBOARD_INVALID) {
            DWCi_WINlInitEx(6, 3, 1, -1, 0);
            DWCi_ChangeScene(procError);
            return;
        }
        DWCi_GetParam2_(NULL, &fromList);
        DWCi_WINlInitEx(msgIndices[fromList], 2, 1, -1, 0);
        DWCi_ChangeScene(procWin);
    }
}

// Frees the keyboard and goes back to the connection's settings or to the list of access points, or on to the
// connection test
static void endScene3(void) {
    int field;
    int fromList;

    DWCi_COMMENTlDelete();
    DWCi_OBJlDelete(sWepKeyboard->cursor);
    DWCi_FNTlDeleteBgLcd(0);
    DWCi_CMNlLoadResource("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_GetParam2_(&field, &fromList);
    if (fromList == 0) {
        DWCi_SetParam_(2, 1);
        DWCi_SetParam2_(0, field);
        DWCi_ChangeScene(DWCUtilAddressSettings_Init);
    } else if (sWepKeyboard->result == WEP_KEYBOARD_CANCELLED) {
        DWCi_SetParam_(0, 1);
        DWCi_SetParam2_(1, 0);
        DWCi_ChangeScene(DWCi_SceneSetApList);
    } else {
        DWCi_SetParam_(0, 0);
        DWCi_SetParam2_(0, 1);
        DWCi_ChangeScene(DWCi_SceneSetTestConfirm);
    }
    DWCi_HEAPlFree_(&sWepKeyboard);
}

// Draws the text, one character at a time, as a string of one character
static void drawSsid(void) {
    u16 rect[4] = { 0, 0, 0, 0 };
    u16 str[2];
    int i;
    int column;

    rect[1] = sWepKeyboardRowY[0];
    rect[2] = sWepKeyboardCharSize[0];
    rect[3] = sWepKeyboardCharSize[1];
    DWCi_FNTlClear(sWepKeyboard->text, 0);
    str[1] = 0;
    for (i = 0, column = 0; i < WEP_KEYBOARD_LENGTH; i++, column++) {
        if (i == 16) {
            rect[1] = sWepKeyboardRowY[1];
            column = 0;
        }
        if (sWepKeyboard->str[i] == ' ') {
            str[0] = WEP_KEYBOARD_SPACE;
        } else {
            str[0] = sWepKeyboard->str[i];
        }
        rect[0] = sWepKeyboardColumnX[column];
        DWCi_FNTlDrawRect(sWepKeyboard->text, rect[0], rect[1], rect[2], rect[3], 2, 0x480, str);
    }
    DWCi_FNTlRenewBg(sWepKeyboard->text);
}

// Moves the cursor after the text
static void dispCaret(void) {
    u8 length = sWepKeyboard->length;
    int column = length & 15;
    int row = length >> 4;

    if (length >= WEP_KEYBOARD_LENGTH) {
        column = 15;
        row = 1;
    }
    DWCUtilOam_SetPosition(sWepKeyboard->cursor, sWepKeyboardColumnX[column], sWepKeyboardRowY[row]);
}

static void procWin(void) {
    sWepKeyboard->result = DWCi_WINlGet();
    switch (sWepKeyboard->result) {
    case 0:
        DWCi_SNDlPlay(7);
        break;
    case 1:
        DWCi_SNDlPlay(0xe);
        break;
    default:
        return;
    }
    DWCi_WINlEnd();
    DWCi_ChangeScene(procWin1);
}

// Takes the text into the connection being edited as its SSID or its WEP key, or goes back to the keyboard
static void procWin1(void) {
    void (*take[2])(const u8 *) = { DWCi_SETTINGlSetSsid, DWCi_SETTINGlSetWep };
    int field;

    if (!DWCi_WINlIsInit()) {
        if (sWepKeyboard->result == 0) {
            G2_SetOBJEffect(sWepKeyboard->cursor, GX_OAM_EFFECT_NONE, 0);
            DWCi_ChangeScene(procInit1);
            return;
        }
        DWCi_GetParam2_(&field, NULL);
        take[field](sWepKeyboard->str);
        DWCi_ChangeScene(endScene3);
    }
}

// Whether the text can be kept: an SSID that isn't empty, or a WEP key of 5, 13 or 16 characters or of 10, 26 or 32
// hexadecimal digits
static BOOL isCorrect(void) {
    int field;
    int fromList;
    int length;
    int i;

    DWCi_GetParam2_(&field, &fromList);
    if (fromList == 1) {
        DWCi_COMMENTlDispEx(0x35, -1, 0);
    }
    DWCi_GetParam2_(&field, &fromList);
    if (field == 0) {
        if (sWepKeyboard->str[0] != '\0') {
            return TRUE;
        }
        return FALSE;
    }
    if (fromList == 1 && sWepKeyboard->str[0] == '\0') {
        return FALSE;
    }
    length = DWCi_STRlNLen(sWepKeyboard->str, WEP_KEYBOARD_LENGTH);
    switch (length) {
    case 0:
    case 5:
    case 13:
    case 16:
        return TRUE;
    case 10:
    case 26:
    case 32:
        for (i = 0; i < length; i++) {
            u8 c = sWepKeyboard->str[i];

            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
                return FALSE;
            }
        }
        return TRUE;
    default:
        return FALSE;
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
        G2_SetOBJEffect(sWepKeyboard->cursor, GX_OAM_EFFECT_NONE, 0);
        DWCi_ChangeScene(procInit1);
    }
}
