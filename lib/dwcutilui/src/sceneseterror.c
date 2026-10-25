#include "sceneseterror.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "input.h"
#include "main.h"
#include "message.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "sceneinit.h"
#include "scenesetlist.h"
#include "scenesetselectap.h"
#include "sound.h"
#include "str.h"

static void initGraphics(void);
static void drawMessage(void);
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

// The font of the message by language, the message's box (left, top, width and height), and where the code is drawn
// by language
static const u8 sErrorFonts[7] = { 0, 1, 1, 1, 1, 1, 0 };
static const u16 sErrorMsgRect[4] = { 0xd, 0x3c, 0xe6, 0x5e };
static const u16 sErrorCodePos[7][2] = {
    { 0x62, 0x22 }, { 0x62, 0x22 }, { 0x3d, 0x22 }, { 0x65, 0x22 }, { 0x6c, 0x22 }, { 0x34, 0x22 }, { 0x4e, 0x22 },
};

// The DWC error code that the screen shows
static int sErrorCode;

void DWCi_SceneSetError(void) {
    initGraphics();
    drawMessage();
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCi_CMNlLoadResource("char/jb4Error.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

// Draws the error code and the message of its kind
static void drawMessage(void) {
    int type = 0;
    int from;
    int code;
    DWCUtilText *text;
    u16 *msg;
    char digits[8];
    u16 str[8];
    u16 x;
    u16 y;

    DWCi_GetParam2_(NULL, &from);
    code = sErrorCode;
    if (code >= -20099) {
        type = 0;
    } else if (code >= -20100) {
        type = 2;
    } else if (code >= -20101) {
        type = 0x13;
    } else if (code >= -20107) {
        type = 2;
    } else if (code >= -20108) {
        type = 0x12;
    } else if (code >= -20109) {
        type = 2;
    } else if (code >= -20110) {
        type = 3;
    } else if (code >= -20999) {
        type = 2;
    } else if (code >= -22999) {
        type = 0;
    } else if (code >= -23999) {
        type = 0x13;
    } else if (code >= -49999) {
        type = 0;
    } else if (code >= -50002) {
        type = 7;
    } else if (code >= -50003) {
        type = 5;
    } else if (code >= -50098) {
        type = 0;
    } else if (code >= -50099) {
        if (from == 2) {
            type = 5;
        } else {
            type = 7;
        }
    } else if (code >= -51098) {
        type = 0;
    } else if (code >= -51099) {
        if (from == 2) {
            type = 5;
        } else {
            type = 8;
        }
    } else if (code >= -51102) {
        type = 9;
    } else if (code >= -51103) {
        type = 5;
    } else if (code >= -51199) {
        type = 0;
    } else if (code >= -51299) {
        type = 0x14;
    } else if (code >= -51302) {
        type = 0x15;
    } else if (code >= -51303) {
        type = 4;
    } else if (code >= -51999) {
        type = 0;
    } else if (code >= -52002) {
        type = 0xa;
    } else if (code >= -52003) {
        type = 0x16;
    } else if (code >= -52099) {
        type = 0;
    } else if (code >= -52103) {
        type = 0xb;
    } else if (code >= -52199) {
        type = 0;
    } else if (code >= -52203) {
        type = 0xb;
    } else if (code >= -52299) {
        type = 0;
    } else if (code >= -52399) {
        type = 2;
    } else if (code >= -52999) {
        type = 0;
    } else if (code >= -53299) {
        type = 2;
    }

    text = DWCi_FNTlFormBgEx(0, sErrorFonts[DWCi_Language()]);
    msg = DWCi_MSGlGet(gDWCUtilMsg, type);
    OS_SNPrintf(digits, sizeof(digits), "%d", -sErrorCode);
    DWCi_STRlConvertStringAsciiToUnicode(str, (u8 *)digits, 8);
    y = sErrorCodePos[DWCi_Language()][1];
    x = sErrorCodePos[DWCi_Language()][0];
    DWCi_FNTlDrawStringEx(text, x, y, 2, 10, str, 0);
    DWCi_FNTlDrawRect(text, sErrorMsgRect[0], sErrorMsgRect[1], sErrorMsgRect[2], sErrorMsgRect[3], 2,
                      DWCi_CMNlFontFlag(), msg);
    DWCi_FNTlRenewBg(text);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlVisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(procInit1);
}

static void procInit1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(4);
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
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(0);
    }
}

static void draw(void) {
}

static void button(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCi_SNDlPlay(6);
        DWCi_ChangeScene(endScene);
    }
}

static void endScene(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_BTNlEnd();
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

// Goes back to the first screen of the connection settings, or to the settings of the connection
static void endScene2(void) {
    int from;

    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0 && DWCi_BTNlIsEnd()) {
        DWCi_FNTlDeleteBgLcd(0);
        DWCi_COMMENTlDelete();
        DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_GetParam2_(NULL, &from);
        if (from != 0) {
            DWCi_SetParam_(2, 1);
            DWCi_ChangeScene(DWCi_SceneSetSelectAp);
        } else {
            DWCi_SetParam_(2, 0);
            DWCi_SetParam2_(0, 0);
            DWCi_SetListReturn(0);
            DWCi_ChangeScene(DWCUtilAddressSettings_Init);
        }
    }
}

void DWCi_SetErrorCode(int code) {
    sErrorCode = code;
}
