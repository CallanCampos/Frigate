#include "dwc_util_error.h"
#include "dwc_util_address_settings.h"
#include "dwc_util_connection.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_msg_resource.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_ap.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_string.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"
#include "nitro/os.h"

static void DWCUtilError_LoadGraphics(void);
static void DWCUtilError_DrawMessage(void);
static void DWCUtilError_FadeIn(void);
static void DWCUtilError_WaitFadeIn(void);
static void DWCUtilError_WaitButtons(void);
static void DWCUtilError_Main(void);
static void DWCUtilError_HandleInput(void);
static void func_ov182_021b2c4c(void);
static void DWCUtilError_HandleChoice(void);
static void DWCUtilError_DisableButtons(void);
static void DWCUtilError_FadeOut(void);
static void DWCUtilError_WaitFadeOut(void);

// The font of the message by language, the message's box (left, top, width and height), and where the code is drawn
// by language
static const u8 sErrorFonts[7] = { 0, 1, 1, 1, 1, 1, 0 };
static const u16 sErrorMsgRect[4] = { 0xd, 0x3c, 0xe6, 0x5e };
static const u16 sErrorCodePos[7][2] = {
    { 0x62, 0x22 }, { 0x62, 0x22 }, { 0x3d, 0x22 }, { 0x65, 0x22 }, { 0x6c, 0x22 }, { 0x34, 0x22 }, { 0x4e, 0x22 },
};

// The DWC error code that the screen shows
static int sErrorCode;

void DWCUtilError_Init(void) {
    DWCUtilError_LoadGraphics();
    DWCUtilError_DrawMessage();
    DWCUtilMain_SetUpdate(DWCUtilError_FadeIn);
}

static void DWCUtilError_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/jb4Error.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

// Draws the error code and the message of its kind
static void DWCUtilError_DrawMessage(void) {
    int type = 0;
    int from;
    int code;
    DWCUtilText *text;
    u16 *msg;
    char digits[8];
    u16 str[8];
    u16 x;
    u16 y;

    DWCUtilMain_GetScreenArgs(NULL, &from);
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

    text = DWCUtilText_CreateBg(0, sErrorFonts[DWCUtilMain_GetLanguage()]);
    msg = DWCUtilMsg_Get(gDWCUtilMsg, type);
    OS_SNPrintf(digits, sizeof(digits), "%d", -sErrorCode);
    DWCUtilString_Widen(str, (u8 *)digits, 8);
    y = sErrorCodePos[DWCUtilMain_GetLanguage()][1];
    x = sErrorCodePos[DWCUtilMain_GetLanguage()][0];
    DWCUtilText_DrawCentered(text, x, y, 2, 10, str, 0);
    DWCUtilText_DrawRect(text, sErrorMsgRect[0], sErrorMsgRect[1], sErrorMsgRect[2], sErrorMsgRect[3], 2,
                         DWCUtilGfxResource_GetTextFlags(), msg);
    DWCUtilText_MarkDirty(text);
}

static void DWCUtilError_FadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilError_WaitFadeIn);
}

static void DWCUtilError_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(4);
        DWCUtilMain_SetUpdate(DWCUtilError_WaitButtons);
    }
}

static void DWCUtilError_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilError_Main);
    }
}

static void DWCUtilError_Main(void) {
    DWCUtilError_HandleInput();
    func_ov182_021b2c4c();
    DWCUtilError_HandleChoice();
}

static void DWCUtilError_HandleInput(void) {
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b2c4c(void) {
}

static void DWCUtilError_HandleChoice(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilMain_SetUpdate(DWCUtilError_DisableButtons);
    }
}

static void DWCUtilError_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilError_FadeOut);
}

static void DWCUtilError_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        DWCUtilFade_Start(3, 1, 1, 8);
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(DWCUtilError_WaitFadeOut);
    }
}

// Goes back to the first screen of the connection settings, or to the settings of the connection
static void DWCUtilError_WaitFadeOut(void) {
    int from;

    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0 && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, 1);
        DWCUtilDisplay_HidePlanes(0, 0x15);
        DWCUtilMain_GetScreenArgs(NULL, &from);
        if (from != 0) {
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(DWCUtilScreenAp_Init);
        } else {
            DWCUtilMain_SetLayout(2, 0);
            DWCUtilMain_SetScreenArgs(0, 0);
            DWCUtilAddressSettings_SetFrom(0);
            DWCUtilMain_SetUpdate(DWCUtilAddressSettings_Init);
        }
    }
}

void DWCUtilError_SetCode(int code) {
    sErrorCode = code;
}
