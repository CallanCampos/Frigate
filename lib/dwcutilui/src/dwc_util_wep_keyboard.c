#include "dwc_util_wep_keyboard.h"
#include "dwc_util_address_settings.h"
#include "dwc_util_auto_connect_intro.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_heap.h"
#include "dwc_util_keyboard.h"
#include "dwc_util_main.h"
#include "dwc_util_oam_attr.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_page_ap_list.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_string.h"
#include "dwc_util_text.h"
#include "nitro/gx.h"

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

static void func_ov182_021b1ab4(void);
static void func_ov182_021b1b34(void);
static void func_ov182_021b1b6c(void);
static void func_ov182_021b1bc4(void);
static void func_ov182_021b1bdc(void);
static void func_ov182_021b1be8(void);
static void func_ov182_021b1d00(void);
static void func_ov182_021b1d04(void);
static void func_ov182_021b1d18(void);
static void func_ov182_021b1d3c(void);
static void func_ov182_021b1dc4(void);
static void func_ov182_021b1e74(void);
static void func_ov182_021b1f04(void);
static void func_ov182_021b1f50(void);
static void func_ov182_021b1f8c(void);
static BOOL func_ov182_021b1ff8(void);
static void func_ov182_021b20d4(void);
static void func_ov182_021b20f4(void);

static DWCUtilWepKeyboard *sWepKeyboard;

void func_ov182_021b19f4(void) {
    u8 msgIndices[2] = { 0x3e, 0x3d };
    int field;
    int fromList;

    sWepKeyboard = func_ov182_021bc4d8(sizeof(DWCUtilWepKeyboard), 4);
    func_ov182_021adffc(&field, &fromList);
    if (field == 0) {
        func_ov182_021b7a08(sWepKeyboard->str);
        sWepKeyboard->length = func_ov182_021bcef8(sWepKeyboard->str, WEP_KEYBOARD_LENGTH);
    }
    func_ov182_021b1ab4();
    func_ov182_021ab96c(field + 9);
    if (fromList == 1) {
        DWCUtilScreen_Start(0x35, -1, 0);
    } else {
        DWCUtilScreen_Start(msgIndices[field], -1, 0);
    }
    func_ov182_021ab6a4(2);
    sWepKeyboard->text = func_ov182_021bbdec(0, 0);
    sWepKeyboard->cursor = func_ov182_021bb960(0, 0x3e);
    G2_SetOBJPriority(sWepKeyboard->cursor, 3);
    func_ov182_021b1f04();
    func_ov182_021b1e74();
    func_ov182_021adfbc(func_ov182_021b1b34);
}

// Where the characters are drawn: the two rows' y, the size of a character and each column's x
static const u8 sWepKeyboardRowY[2] = { 0x20, 0x31 };
static const u16 sWepKeyboardCharSize[2] = { 14, 16 };
static const u8 sWepKeyboardColumnX[16] = {
    0x08, 0x17, 0x26, 0x35, 0x44, 0x53, 0x62, 0x71, 0x80, 0x8f, 0x9e, 0xad, 0xbc, 0xcb, 0xda, 0xe9,
};

static void func_ov182_021b1ab4(void) {
    func_ov182_021ab67c("char/ybObjKb.ncl.l", GX_LoadOBJPltt);
    func_ov182_021ab67c("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    func_ov182_021ab67c("char/xb4Edit.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(2);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b1b34(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021b1b6c);
}

static void func_ov182_021b1b6c(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021abb10();
        func_ov182_021b80f4(0x14);
        if (sWepKeyboard->length == 0) {
            func_ov182_021abda4(FALSE);
        }
        if (sWepKeyboard->length == WEP_KEYBOARD_LENGTH) {
            func_ov182_021abdb8(FALSE);
        }
        func_ov182_021adfbc(func_ov182_021b1bc4);
    }
}

static void func_ov182_021b1bc4(void) {
    if (func_ov182_021abd94() != 0xff) {
        func_ov182_021adfbc(func_ov182_021b1bdc);
    }
}

static void func_ov182_021b1bdc(void) {
    func_ov182_021b1be8();
    func_ov182_021b1d00();
}

static void func_ov182_021b1be8(void) {
    int key = func_ov182_021abd94();

    switch (key) {
    case 0:
        break;
    case 0x80:
        if (sWepKeyboard->length != 0) {
            func_ov182_021b80f4(3);
            sWepKeyboard->length--;
            sWepKeyboard->str[sWepKeyboard->length] = '\0';
            if (sWepKeyboard->length == 0) {
                func_ov182_021abda4(FALSE);
            }
            func_ov182_021abdb8(TRUE);
        }
        break;
    case 0x82:
        func_ov182_021b80f4(7);
        sWepKeyboard->result = WEP_KEYBOARD_CANCELLED;
        func_ov182_021adfbc(func_ov182_021b1d04);
        return;
    case 0x83:
        if (func_ov182_021b1ff8()) {
            func_ov182_021b80f4(6);
            sWepKeyboard->result = WEP_KEYBOARD_ENTERED;
        } else {
            sWepKeyboard->result = WEP_KEYBOARD_INVALID;
            func_ov182_021b80f4(9);
        }
        G2_SetOBJEffect(sWepKeyboard->cursor, GX_OAM_EFFECT_NODISPLAY, 0);
        func_ov182_021adfbc(func_ov182_021b1d04);
        return;
    case WEP_KEYBOARD_SPACE:
    default:
        if (sWepKeyboard->length != WEP_KEYBOARD_LENGTH) {
            func_ov182_021b80f4(1);
            sWepKeyboard->str[sWepKeyboard->length] = key;
            sWepKeyboard->length++;
            func_ov182_021abda4(TRUE);
            if (sWepKeyboard->length == WEP_KEYBOARD_LENGTH) {
                func_ov182_021abdb8(FALSE);
            }
        }
        break;
    }
    func_ov182_021b1e74();
    func_ov182_021b1f04();
}

static void func_ov182_021b1d00(void) {
}

static void func_ov182_021b1d04(void) {
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021b1d18);
}

static void func_ov182_021b1d18(void) {
    if (func_ov182_021bbaac(1) == 0) {
        func_ov182_021abd6c();
        func_ov182_021b80f4(0x15);
        func_ov182_021adfbc(func_ov182_021b1d3c);
    }
}

// Once the keyboard has closed, asks whether to keep the text, or shows that it isn't a WEP key
static void func_ov182_021b1d3c(void) {
    int msgIndices[2] = { 0x47, 0x48 };
    int fromList;

    if (!func_ov182_021abdc8()) {
        if (sWepKeyboard->result == WEP_KEYBOARD_CANCELLED) {
            func_ov182_021adfbc(func_ov182_021b1dc4);
            return;
        }
        if (sWepKeyboard->result == WEP_KEYBOARD_INVALID) {
            func_ov182_021b8cf0(6, 3, 1, -1, 0);
            func_ov182_021adfbc(func_ov182_021b20d4);
            return;
        }
        func_ov182_021adffc(NULL, &fromList);
        func_ov182_021b8cf0(msgIndices[fromList], 2, 1, -1, 0);
        func_ov182_021adfbc(func_ov182_021b1f50);
    }
}

// Frees the keyboard and goes back to the connection's settings or to the list of access points, or on to the
// connection test
static void func_ov182_021b1dc4(void) {
    int field;
    int fromList;

    DWCUtilScreen_Stop();
    func_ov182_021bcc9c(sWepKeyboard->cursor);
    func_ov182_021bbf7c(0);
    func_ov182_021ab67c("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    func_ov182_021bc228(1, 1);
    func_ov182_021bc228(0, 0x15);
    func_ov182_021adffc(&field, &fromList);
    if (fromList == 0) {
        func_ov182_021adfc8(2, 1);
        func_ov182_021adff0(0, field);
        func_ov182_021adfbc(func_ov182_021b2d48);
    } else if (sWepKeyboard->result == WEP_KEYBOARD_CANCELLED) {
        func_ov182_021adfc8(0, 1);
        func_ov182_021adff0(1, 0);
        func_ov182_021adfbc(func_ov182_021b0c28);
    } else {
        func_ov182_021adfc8(0, 0);
        func_ov182_021adff0(0, 1);
        func_ov182_021adfbc(func_ov182_021b6588);
    }
    func_ov182_021bc4f0(&sWepKeyboard);
}

// Draws the text, one character at a time, as a string of one character
static void func_ov182_021b1e74(void) {
    u16 rect[4] = { 0, 0, 0, 0 };
    u16 str[2];
    int i;
    int column;

    rect[1] = sWepKeyboardRowY[0];
    rect[2] = sWepKeyboardCharSize[0];
    rect[3] = sWepKeyboardCharSize[1];
    func_ov182_021bc118(sWepKeyboard->text, 0);
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
        func_ov182_021bc0a4(sWepKeyboard->text, rect[0], rect[1], rect[2], rect[3], 2, 0x480, str);
    }
    func_ov182_021bc184(sWepKeyboard->text);
}

// Moves the cursor after the text
static void func_ov182_021b1f04(void) {
    u8 length = sWepKeyboard->length;
    int column = length & 15;
    int row = length >> 4;

    if (length >= WEP_KEYBOARD_LENGTH) {
        column = 15;
        row = 1;
    }
    DWCUtilOam_SetPosition(sWepKeyboard->cursor, sWepKeyboardColumnX[column], sWepKeyboardRowY[row]);
}

static void func_ov182_021b1f50(void) {
    sWepKeyboard->result = func_ov182_021b8f14();
    switch (sWepKeyboard->result) {
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
    func_ov182_021adfbc(func_ov182_021b1f8c);
}

// Takes the text into the connection being edited as its SSID or its WEP key, or goes back to the keyboard
static void func_ov182_021b1f8c(void) {
    void (*take[2])(const u8 *) = { func_ov182_021b7854, func_ov182_021b7874 };
    int field;

    if (!func_ov182_021b8f24()) {
        if (sWepKeyboard->result == 0) {
            G2_SetOBJEffect(sWepKeyboard->cursor, GX_OAM_EFFECT_NONE, 0);
            func_ov182_021adfbc(func_ov182_021b1b6c);
            return;
        }
        func_ov182_021adffc(&field, NULL);
        take[field](sWepKeyboard->str);
        func_ov182_021adfbc(func_ov182_021b1dc4);
    }
}

// Whether the text can be kept: an SSID that isn't empty, or a WEP key of 5, 13 or 16 characters or of 10, 26 or 32
// hexadecimal digits
static BOOL func_ov182_021b1ff8(void) {
    int field;
    int fromList;
    int length;
    int i;

    func_ov182_021adffc(&field, &fromList);
    if (fromList == 1) {
        DWCUtilScreen_Start(0x35, -1, 0);
    }
    func_ov182_021adffc(&field, &fromList);
    if (field == 0) {
        if (sWepKeyboard->str[0] != '\0') {
            return TRUE;
        }
        return FALSE;
    }
    if (fromList == 1 && sWepKeyboard->str[0] == '\0') {
        return FALSE;
    }
    length = func_ov182_021bcef8(sWepKeyboard->str, WEP_KEYBOARD_LENGTH);
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

static void func_ov182_021b20d4(void) {
    if (func_ov182_021b8f14() == 0) {
        func_ov182_021b80f4(6);
        func_ov182_021b8eec();
        func_ov182_021adfbc(func_ov182_021b20f4);
    }
}

static void func_ov182_021b20f4(void) {
    if (!func_ov182_021b8f24()) {
        G2_SetOBJEffect(sWepKeyboard->cursor, GX_OAM_EFFECT_NONE, 0);
        func_ov182_021adfbc(func_ov182_021b1b6c);
    }
}
