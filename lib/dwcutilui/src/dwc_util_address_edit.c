#include "dwc_util_address_edit.h"
#include "dwc/bm.h"
#include "dwc/ext.h"
#include "dwc_util_address_settings.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_menu.h"
#include "dwc_util_oam_attr.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "stdlib.h"

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

static void func_ov182_021b2210(void);
static void func_ov182_021b2290(void);
static void func_ov182_021b22c8(void);
static void func_ov182_021b2324(void);
static void func_ov182_021b233c(void);
static void func_ov182_021b2348(void);
static BOOL func_ov182_021b2510(int min);
static void func_ov182_021b2568(void);
static void func_ov182_021b256c(void);
static void func_ov182_021b2580(void);
static void func_ov182_021b25a4(void);
static void func_ov182_021b2604(void);
static void func_ov182_021b267c(void);
static void func_ov182_021b26f4(void);
static void func_ov182_021b272c(void);
static void func_ov182_021b2764(void);
static void func_ov182_021b27d4(void);
static BOOL func_ov182_021b281c(void);
static void func_ov182_021b28f0(void);
static void func_ov182_021b2910(void);

// Where each digit is drawn
static const u8 sAddressEditDigitX[ADDRESS_EDIT_DIGITS] = {
    0x31, 0x3d, 0x49, 0x5a, 0x66, 0x72, 0x83, 0x8f, 0x9b, 0xac, 0xb8, 0xc4,
};

static DWCUtilAddressEdit *sAddressEdit;

void func_ov182_021b2120(void) {
    void (*read[5])(u8 *) = {
        func_ov182_021b7a24, func_ov182_021b7a50, func_ov182_021b7a7c, func_ov182_021b7aac, func_ov182_021b7adc,
    };
    u8 msgIndices[5] = { 0x3f, 0x40, 0x41, 0x42, 0x43 };
    int field;
    DWCUtilAddressEdit *edit;

    sAddressEdit = func_ov182_021bc4d8(sizeof(DWCUtilAddressEdit), 4);
    func_ov182_021adffc(&field, NULL);
    read[field](sAddressEdit->digits);
    edit = sAddressEdit;
    if (memcmp(edit->digits, "  0", 3) != 0) {
        edit->group = ADDRESS_EDIT_LAST_GROUP;
    } else {
        MI_CpuFill8(edit->digits, 0, sizeof(edit->digits));
        sAddressEdit->group = 0;
    }
    func_ov182_021b2210();
    func_ov182_021ab96c(field + 0xb);
    DWCUtilScreen_Start(msgIndices[field], -1, 0);
    func_ov182_021ab6a4(2);
    sAddressEdit->text = func_ov182_021bbdec(0, 0);
    sAddressEdit->cursor = func_ov182_021bb960(0, 0x3f);
    G2_SetOBJPriority(sAddressEdit->cursor, 3);
    func_ov182_021b26f4();
    func_ov182_021b267c();
    func_ov182_021adfbc(func_ov182_021b2290);
}

static void func_ov182_021b2210(void) {
    func_ov182_021ab67c("char/ybObjKb.ncl.l", GX_LoadOBJPltt);
    func_ov182_021ab67c("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    func_ov182_021ab67c("char/xb4EditAddr.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(2);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b2290(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021b22c8);
}

static void func_ov182_021b22c8(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021acd80();
        func_ov182_021b80f4(0x14);
        if (sAddressEdit->group == 0) {
            func_ov182_021acfac(FALSE);
            func_ov182_021acfcc(FALSE);
        } else {
            if (func_ov182_021b2510(26)) {
                func_ov182_021acfbc(FALSE);
            }
            func_ov182_021acfcc(FALSE);
        }
        func_ov182_021adfbc(func_ov182_021b2324);
    }
}

static void func_ov182_021b2324(void) {
    if (func_ov182_021acf9c() != 0x1f) {
        func_ov182_021adfbc(func_ov182_021b233c);
    }
}

static void func_ov182_021b233c(void) {
    func_ov182_021b2348();
    func_ov182_021b2568();
}

static void func_ov182_021b2348(void) {
    int key = func_ov182_021acf9c();
    u8 *digits;
    int first;

    switch (key) {
    case 0:
        return;
    case 0x10:
        if (sAddressEdit->group == 0 && sAddressEdit->digits[2] == 0) {
            break;
        }
        func_ov182_021b80f4(3);
        if (sAddressEdit->digits[sAddressEdit->group * 3 + 2] == 0) {
            sAddressEdit->group--;
        }
        MI_CpuFill8(&sAddressEdit->digits[sAddressEdit->group * 3], 0, 3);
        if (sAddressEdit->group == 0 && sAddressEdit->digits[2] == 0) {
            func_ov182_021acfac(FALSE);
        }
        func_ov182_021acfbc(TRUE);
        func_ov182_021acfcc(FALSE);
        break;
    case 0x11:
        if (sAddressEdit->group < ADDRESS_EDIT_LAST_GROUP && sAddressEdit->digits[sAddressEdit->group * 3 + 2] != 0) {
            func_ov182_021b80f4(1);
            sAddressEdit->group++;
            func_ov182_021acfcc(FALSE);
        }
        break;
    case 0x12:
        sAddressEdit->result = ADDRESS_EDIT_CANCELLED;
        func_ov182_021b80f4(7);
        func_ov182_021adfbc(func_ov182_021b256c);
        return;
    case 0x13:
        if (func_ov182_021b281c()) {
            func_ov182_021b80f4(6);
            sAddressEdit->result = ADDRESS_EDIT_ENTERED;
        } else {
            sAddressEdit->result = ADDRESS_EDIT_INVALID;
            func_ov182_021b80f4(9);
        }
        sAddressEdit->group = ADDRESS_EDIT_LAST_GROUP;
        G2_SetOBJEffect(sAddressEdit->cursor, GX_OAM_EFFECT_NODISPLAY, 0);
        func_ov182_021b26f4();
        func_ov182_021b27d4();
        func_ov182_021adfbc(func_ov182_021b256c);
        return;
    default:
        if (sAddressEdit->group == ADDRESS_EDIT_LAST_GROUP && func_ov182_021b2510(26)) {
            break;
        }
        func_ov182_021b80f4(1);
        digits = sAddressEdit->digits;
        first = sAddressEdit->group * 3;
        if (digits[first + 2] == 0) {
            digits[first + 2] = key;
        } else if (digits[first + 1] == 0) {
            digits[first + 1] = digits[first + 2];
            digits[first + 2] = key;
            if (func_ov182_021b2510(26)) {
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
        func_ov182_021acfac(TRUE);
        if (sAddressEdit->group < ADDRESS_EDIT_LAST_GROUP) {
            func_ov182_021acfcc(TRUE);
        } else {
            func_ov182_021acfcc(FALSE);
        }
        if (sAddressEdit->group == ADDRESS_EDIT_LAST_GROUP && func_ov182_021b2510(26)) {
            func_ov182_021acfbc(FALSE);
        }
        break;
    }
    func_ov182_021b267c();
    func_ov182_021b26f4();
}

// Whether the number the cursor is on has three digits, or is at least min
static BOOL func_ov182_021b2510(int min) {
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

static void func_ov182_021b2568(void) {
}

static void func_ov182_021b256c(void) {
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021b2580);
}

static void func_ov182_021b2580(void) {
    if (func_ov182_021bbaac(1) == 0) {
        func_ov182_021acf7c();
        func_ov182_021b80f4(0x15);
        func_ov182_021adfbc(func_ov182_021b25a4);
    }
}

// Once the keypad has closed, asks whether to keep the address, or shows that it isn't valid
static void func_ov182_021b25a4(void) {
    if (!func_ov182_021acfdc()) {
        if (sAddressEdit->result == ADDRESS_EDIT_CANCELLED) {
            func_ov182_021adfbc(func_ov182_021b2604);
            return;
        }
        if (sAddressEdit->result == ADDRESS_EDIT_INVALID) {
            func_ov182_021b8cf0(6, 3, 1, -1, 0);
            func_ov182_021adfbc(func_ov182_021b28f0);
            return;
        }
        func_ov182_021b8cf0(0x47, 2, 1, -1, 0);
        func_ov182_021adfbc(func_ov182_021b272c);
    }
}

// Frees the screen and goes back to the connection's settings, on the address's row
static void func_ov182_021b2604(void) {
    int field;

    DWCUtilScreen_Stop();
    func_ov182_021bcc9c(sAddressEdit->cursor);
    func_ov182_021bbf7c(0);
    func_ov182_021ab67c("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    func_ov182_021bc228(1, 1);
    func_ov182_021bc228(0, 0x15);
    func_ov182_021adffc(&field, NULL);
    if (field >= 3) {
        field++;
    }
    func_ov182_021adfc8(2, 1);
    func_ov182_021adff0(0, field + 3);
    func_ov182_021adfbc(func_ov182_021b2d48);
    func_ov182_021bc4f0(&sAddressEdit);
}

// Draws the digits, one at a time, as a string of one character
static void func_ov182_021b267c(void) {
    // The size of a digit
    static const u16 charSize[2] = { 11, 16 };
    u16 rect[4] = { 0, 0x29, charSize[0], charSize[1] };
    u16 str[2];
    int i;

    func_ov182_021bc118(sAddressEdit->text, 0);
    str[1] = 0;
    for (i = 0; i < ADDRESS_EDIT_DIGITS; i++) {
        str[0] = sAddressEdit->digits[i];
        rect[0] = sAddressEditDigitX[i];
        func_ov182_021bc0a4(sAddressEdit->text, rect[0], rect[1], rect[2], rect[3], 2, 0x480, str);
    }
    func_ov182_021bc184(sAddressEdit->text);
}

// Moves the cursor under the last digit of its number
static void func_ov182_021b26f4(void) {
    int group = sAddressEdit->group;

    if (group > ADDRESS_EDIT_LAST_GROUP) {
        group = ADDRESS_EDIT_LAST_GROUP;
    }
    DWCUtilOam_SetPosition(sAddressEdit->cursor, sAddressEditDigitX[group * 3 + 2], 0x28);
}

static void func_ov182_021b272c(void) {
    sAddressEdit->result = func_ov182_021b8f14();
    switch (sAddressEdit->result) {
    case 0:
        func_ov182_021b80f4(7);
        break;
    case 1:
        func_ov182_021b80f4(0xe);
        break;
    default:
        return;
    }
    func_ov182_021b8eec();
    func_ov182_021adfbc(func_ov182_021b2764);
}

// Takes the address into the connection being edited, or goes back to the keypad
static void func_ov182_021b2764(void) {
    void (*take[5])(const u8 *) = {
        func_ov182_021b7990, func_ov182_021b79a8, func_ov182_021b79c0, func_ov182_021b79d8, func_ov182_021b79f0,
    };
    int field;

    if (!func_ov182_021b8f24()) {
        if (sAddressEdit->result == 0) {
            G2_SetOBJEffect(sAddressEdit->cursor, GX_OAM_EFFECT_NONE, 0);
            func_ov182_021adfbc(func_ov182_021b22c8);
            return;
        }
        func_ov182_021adffc(&field, NULL);
        take[field](sAddressEdit->digits);
        func_ov182_021adfbc(func_ov182_021b2604);
    }
}

// Fills each number's empty and leading digits: blanks, and a 0 for a number of none
static void func_ov182_021b27d4(void) {
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
    func_ov182_021b267c();
}

// Whether every number is at most 255, and the address is a subnet mask of contiguous bits, or a usable address
static BOOL func_ov182_021b281c(void) {
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
    func_ov182_021b7ff0(sAddressEdit->digits, address);
    result = FALSE;
    func_ov182_021adffc(&field, NULL);
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
    if (DWCBm_IsUsableAddress(address)) {
        result = TRUE;
    }
    return result;
}

static void func_ov182_021b28f0(void) {
    if (func_ov182_021b8f14() == 0) {
        func_ov182_021b80f4(6);
        func_ov182_021b8eec();
        func_ov182_021adfbc(func_ov182_021b2910);
    }
}

static void func_ov182_021b2910(void) {
    if (!func_ov182_021b8f24()) {
        G2_SetOBJEffect(sAddressEdit->cursor, GX_OAM_EFFECT_NONE, 0);
        func_ov182_021adfbc(func_ov182_021b22c8);
    }
}
