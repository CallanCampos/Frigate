#include "sceneseteditaddress.h"
#include "cellinfo.h"
#include "comment.h"
#include "common.h"
#include "dwc/bm.h"
#include "dwc/ext.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_oam_attr.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "headline.h"
#include "keyboard10.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "obj.h"
#include "scenesetlist.h"
#include "setting.h"
#include "sound.h"
#include "stdlib.h"
#include "window.h"

// The address's four numbers of three digits, and the last of them
#define ADDRESS_EDIT_DIGITS 12
#define ADDRESS_EDIT_LAST_GROUP 3

// How the entry ended
enum {
    ADDRESS_EDIT_CANCELLED,
    ADDRESS_EDIT_ENTERED,
    ADDRESS_EDIT_INVALID,
};

// The digits being entered, each number right-aligned in its three with blanks before it, and the number the
// cursor is on
typedef struct DWCUtilAddressEdit {
    DWCUtilText *text;
    GXOamAttr *cursor;
    u8 digits[ADDRESS_EDIT_DIGITS];
    u8 group;
    u8 result;
} DWCUtilAddressEdit;

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static BOOL DWCUtilAddressEdit_IsNumberFull(int min);
static void draw(void);
static void DWCUtilAddressEdit_EndEntry(void);
static void endScene1(void);
static void endScene2(void);
static void endScene3(void);
static void drawAddress(void);
static void dispCaret(void);
static void procWin(void);
static void procWin1(void);
static void arrangeAddress(void);
static BOOL isCorrect(void);
static void procError(void);
static void procError1(void);

// Where each digit is drawn
static const u8 sAddressEditDigitX[ADDRESS_EDIT_DIGITS] = {
    0x31, 0x3d, 0x49, 0x5a, 0x66, 0x72, 0x83, 0x8f, 0x9b, 0xac, 0xb8, 0xc4,
};

static DWCUtilAddressEdit *sAddressEdit;

void DWCi_SceneSetEditAddress(void) {
    void (*read[5])(u8 *) = {
        DWCi_SETTINGlGetIp,   DWCi_SETTINGlGetNetMask, DWCi_SETTINGlGetGateway,
        DWCi_SETTINGlGetDnsP, DWCi_SETTINGlGetDnsS,
    };
    u8 msgIndices[5] = { 0x3f, 0x40, 0x41, 0x42, 0x43 };
    int field;
    DWCUtilAddressEdit *edit;

    sAddressEdit = DWCi_HEAPlAllocEx(sizeof(DWCUtilAddressEdit), 4);
    DWCi_GetParam2_(&field, NULL);
    read[field](sAddressEdit->digits);
    edit = sAddressEdit;
    if (memcmp(edit->digits, "  0", 3) != 0) {
        edit->group = ADDRESS_EDIT_LAST_GROUP;
    } else {
        MI_CpuFill8(edit->digits, 0, sizeof(edit->digits));
        sAddressEdit->group = 0;
    }
    initGraphics();
    DWCi_HLlLoad(field + 0xb);
    DWCi_COMMENTlDispEx(msgIndices[field], -1, 0);
    DWCi_CMNlStep(2);
    sAddressEdit->text = DWCi_FNTlFormBgEx(0, 0);
    sAddressEdit->cursor = DWCi_CEINlSetExObj(0, 0x3f);
    G2_SetOBJPriority(sAddressEdit->cursor, 3);
    dispCaret();
    drawAddress();
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCi_CMNlLoadResource("char/ybObjKb.ncl.l", GX_LoadOBJPltt);
    DWCi_CMNlLoadResource("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCi_CMNlLoadResource("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCi_CMNlLoadResource("char/xb4EditAddr.nsc.l", GX_LoadBG2Scr);
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
        DWCi_KB10lInit();
        DWCi_SNDlPlay(0x14);
        if (sAddressEdit->group == 0) {
            DWCi_KB10lSetErase(FALSE);
            DWCi_KB10lSetShift(FALSE);
        } else {
            if (DWCUtilAddressEdit_IsNumberFull(26)) {
                DWCi_KB10lSetInput(FALSE);
            }
            DWCi_KB10lSetShift(FALSE);
        }
        DWCi_ChangeScene(procInit2);
    }
}

static void procInit2(void) {
    if (DWCi_KB10lGet() != 0x1f) {
        DWCi_ChangeScene(procScene);
    }
}

static void procScene(void) {
    input();
    draw();
}

static void input(void) {
    int key = DWCi_KB10lGet();
    u8 *digits;
    int first;

    switch (key) {
    case 0:
        return;
    case 0x10:
        if (sAddressEdit->group == 0 && sAddressEdit->digits[2] == 0) {
            break;
        }
        DWCi_SNDlPlay(3);
        if (sAddressEdit->digits[sAddressEdit->group * 3 + 2] == 0) {
            sAddressEdit->group--;
        }
        MI_CpuFill8(&sAddressEdit->digits[sAddressEdit->group * 3], 0, 3);
        if (sAddressEdit->group == 0 && sAddressEdit->digits[2] == 0) {
            DWCi_KB10lSetErase(FALSE);
        }
        DWCi_KB10lSetInput(TRUE);
        DWCi_KB10lSetShift(FALSE);
        break;
    case 0x11:
        if (sAddressEdit->group < ADDRESS_EDIT_LAST_GROUP && sAddressEdit->digits[sAddressEdit->group * 3 + 2] != 0) {
            DWCi_SNDlPlay(1);
            sAddressEdit->group++;
            DWCi_KB10lSetShift(FALSE);
        }
        break;
    case 0x12:
        sAddressEdit->result = ADDRESS_EDIT_CANCELLED;
        DWCi_SNDlPlay(7);
        DWCi_ChangeScene(DWCUtilAddressEdit_EndEntry);
        return;
    case 0x13:
        if (isCorrect()) {
            DWCi_SNDlPlay(6);
            sAddressEdit->result = ADDRESS_EDIT_ENTERED;
        } else {
            sAddressEdit->result = ADDRESS_EDIT_INVALID;
            DWCi_SNDlPlay(9);
        }
        sAddressEdit->group = ADDRESS_EDIT_LAST_GROUP;
        G2_SetOBJEffect(sAddressEdit->cursor, GX_OAM_EFFECT_NODISPLAY, 0);
        dispCaret();
        arrangeAddress();
        DWCi_ChangeScene(DWCUtilAddressEdit_EndEntry);
        return;
    default:
        if (sAddressEdit->group == ADDRESS_EDIT_LAST_GROUP && DWCUtilAddressEdit_IsNumberFull(26)) {
            break;
        }
        DWCi_SNDlPlay(1);
        digits = sAddressEdit->digits;
        first = sAddressEdit->group * 3;
        if (digits[first + 2] == 0) {
            digits[first + 2] = key;
        } else if (digits[first + 1] == 0) {
            digits[first + 1] = digits[first + 2];
            digits[first + 2] = key;
            if (DWCUtilAddressEdit_IsNumberFull(26)) {
                if (sAddressEdit->group < ADDRESS_EDIT_LAST_GROUP) {
                    sAddressEdit->group++;
                }
            }
        } else {
            digits[first] = digits[first + 1];
            digits[first + 1] = digits[first + 2];
            digits[first + 2] = key;
            if (sAddressEdit->group < ADDRESS_EDIT_LAST_GROUP) {
                sAddressEdit->group++;
            }
        }
        DWCi_KB10lSetErase(TRUE);
        if (sAddressEdit->group < ADDRESS_EDIT_LAST_GROUP) {
            DWCi_KB10lSetShift(TRUE);
        } else {
            DWCi_KB10lSetShift(FALSE);
        }
        if (sAddressEdit->group == ADDRESS_EDIT_LAST_GROUP && DWCUtilAddressEdit_IsNumberFull(26)) {
            DWCi_KB10lSetInput(FALSE);
        }
        break;
    }
    drawAddress();
    dispCaret();
}

// Whether the number the cursor is on has three digits, or is at least min
static BOOL DWCUtilAddressEdit_IsNumberFull(int min) {
    u8 number[4];
    u8 *group = &sAddressEdit->digits[sAddressEdit->group * 3];
    int i;

    if (group[0] != 0 && group[0] != ' ') {
        return TRUE;
    }
    MI_CpuCopy8(group, number, 3);
    number[3] = '\0';
    i = 0;
    do {
        if (number[i] != 0) {
            break;
        }
        number[i] = ' ';
        i++;
    } while (i < 3);
    return atol((char *)number) >= min;
}

static void draw(void) {
}

static void DWCUtilAddressEdit_EndEntry(void) {
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_KB10lEnd();
        DWCi_SNDlPlay(0x15);
        DWCi_ChangeScene(endScene2);
    }
}

// Once the keypad has closed, asks whether to keep the address, or shows that it isn't valid
static void endScene2(void) {
    if (!DWCi_KB10lIsInit()) {
        if (sAddressEdit->result == ADDRESS_EDIT_CANCELLED) {
            DWCi_ChangeScene(endScene3);
            return;
        }
        if (sAddressEdit->result == ADDRESS_EDIT_INVALID) {
            DWCi_WINlInitEx(6, 3, 1, -1, 0);
            DWCi_ChangeScene(procError);
            return;
        }
        DWCi_WINlInitEx(0x47, 2, 1, -1, 0);
        DWCi_ChangeScene(procWin);
    }
}

// Frees the screen and goes back to the connection's settings, on the address's row
static void endScene3(void) {
    int field;

    DWCi_COMMENTlDelete();
    DWCi_OBJlDelete(sAddressEdit->cursor);
    DWCi_FNTlDeleteBgLcd(0);
    DWCi_CMNlLoadResource("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_GetParam2_(&field, NULL);
    if (field >= 3) {
        field++;
    }
    DWCi_SetParam_(2, 1);
    DWCi_SetParam2_(0, field + 3);
    DWCi_ChangeScene(DWCUtilAddressSettings_Init);
    DWCi_HEAPlFree_(&sAddressEdit);
}

// Draws the digits, one at a time, as a string of one character
static void drawAddress(void) {
    // The size of a digit
    static const u16 charSize[2] = { 11, 16 };
    u16 rect[4] = { 0, 0x29, charSize[0], charSize[1] };
    u16 str[2];
    int i;

    DWCi_FNTlClear(sAddressEdit->text, 0);
    str[1] = 0;
    for (i = 0; i < ADDRESS_EDIT_DIGITS; i++) {
        str[0] = sAddressEdit->digits[i];
        rect[0] = sAddressEditDigitX[i];
        DWCi_FNTlDrawRect(sAddressEdit->text, rect[0], rect[1], rect[2], rect[3], 2, 0x480, str);
    }
    DWCi_FNTlRenewBg(sAddressEdit->text);
}

// Moves the cursor under the last digit of its number
static void dispCaret(void) {
    int group = sAddressEdit->group;

    if (group > ADDRESS_EDIT_LAST_GROUP) {
        group = ADDRESS_EDIT_LAST_GROUP;
    }
    DWCUtilOam_SetPosition(sAddressEdit->cursor, sAddressEditDigitX[group * 3 + 2], 0x28);
}

static void procWin(void) {
    sAddressEdit->result = DWCi_WINlGet();
    switch (sAddressEdit->result) {
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

// Takes the address into the connection being edited, or goes back to the keypad
static void procWin1(void) {
    void (*take[5])(const u8 *) = {
        DWCi_SETTINGlSetIp,   DWCi_SETTINGlSetNetMask, DWCi_SETTINGlSetGateway,
        DWCi_SETTINGlSetDnsP, DWCi_SETTINGlSetDnsS,
    };
    int field;

    if (!DWCi_WINlIsInit()) {
        if (sAddressEdit->result == 0) {
            G2_SetOBJEffect(sAddressEdit->cursor, GX_OAM_EFFECT_NONE, 0);
            DWCi_ChangeScene(procInit1);
            return;
        }
        DWCi_GetParam2_(&field, NULL);
        take[field](sAddressEdit->digits);
        DWCi_ChangeScene(endScene3);
    }
}

// Fills each number's empty and leading digits: blanks, and a 0 for a number of none
static void arrangeAddress(void) {
    u8 *group;
    int i;
    int j;

    for (i = 0; i < 4; i++) {
        group = &sAddressEdit->digits[i * 3];
        for (j = 0; j < 3; j++) {
            if (group[j] != '0' && group[j] != ' ' && group[j] != 0) {
                break;
            }
            group[j] = j == 2 ? '0' : ' ';
        }
    }
    drawAddress();
}

// Whether every number is at most 255, and the address is a subnet mask of contiguous bits, or a usable address
static BOOL isCorrect(void) {
    u8 *group;
    int i;
    u8 max[4] = "255";
    int field;
    u8 address[4];
    int j;
    BOOL result;

    for (i = 0; i < 4; i++) {
        group = &sAddressEdit->digits[i * 3];
        if (group[0] != ' ') {
            for (j = 0; j < 3; j++) {
                if (group[j] > max[j]) {
                    return FALSE;
                }
                if (group[j] < max[j]) {
                    break;
                }
            }
        }
    }
    DWCi_SETTINGlConvAddress(sAddressEdit->digits, address);
    result = FALSE;
    DWCi_GetParam2_(&field, NULL);
    if (field == 1) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 8; j++) {
                if (result) {
                    if (address[i] & (1 << (7 - j))) {
                        return FALSE;
                    }
                } else if (!(address[i] & (1 << (7 - j)))) {
                    result = TRUE;
                }
            }
        }
        return TRUE;
    }
    if (DWC_BACKUPlCheckAddress(address)) {
        result = TRUE;
    }
    return result;
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
        G2_SetOBJEffect(sAddressEdit->cursor, GX_OAM_EFFECT_NONE, 0);
        DWCi_ChangeScene(procInit1);
    }
}
