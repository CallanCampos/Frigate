#include "dwc_util_info_menu.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc/bm.h"
#include "dwc_util_display.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "headline.h"
#include "input.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "sceneoptmenu.h"
#include "setting.h"
#include "sound.h"
#include "str.h"
#include "window.h"

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void button(void);
static void DWCUtilInfoMenu_EraseDisableButtons(void);
static void endScene1(void);
static void endScene2(void);
static void procConfirm(void);
static void procConfirm1(void);
static void DWCUtilInfoMenu_EraseSettings(void);
static void DWCUtilInfoMenu_LoadErasedGraphics(void);
static void DWCUtilInfoMenu_ErasedFadeIn(void);
static void DWCUtilInfoMenu_ErasedWaitFadeIn(void);
static void DWCUtilInfoMenu_ErasedWaitButtons(void);
static void DWCUtilInfoMenu_ErasedMain(void);
static void DWCUtilInfoMenu_ErasedHandleInput(void);
static void func_ov182_021aeea4(void);
static void DWCUtilInfoMenu_ErasedHandleChoice(void);
static void DWCUtilInfoMenu_ErasedDisableButtons(void);
static void DWCUtilInfoMenu_ErasedFadeOut(void);
static void DWCUtilInfoMenu_PowerOff(void);
static void DWCUtilInfoMenu_LoadInfoGraphics(void);
static void DWCUtilInfoMenu_DrawConsoleInfo(void);
static void DWCUtilInfoMenu_InfoFadeIn(void);
static void DWCUtilInfoMenu_InfoWaitFadeIn(void);
static void DWCUtilInfoMenu_InfoWaitButtons(void);
static void DWCUtilInfoMenu_InfoMain(void);
static void DWCUtilInfoMenu_InfoHandleInput(void);
static void func_ov182_021af19c(void);
static void DWCUtilInfoMenu_InfoHandleChoice(void);
static void DWCUtilInfoMenu_InfoDisableButtons(void);
static void DWCUtilInfoMenu_InfoFadeOut(void);
static void DWCUtilInfoMenu_InfoWaitFadeOut(void);

// Whether the player confirmed the erasing
static u8 sErase;

void DWCUtilInfoMenu_InitErase(void) {
    sErase = FALSE;
    initGraphics();
    DWCi_HLlLoad(0x12);
    DWCi_COMMENTlDispEx(0x3b, -1, 0);
    DWCi_CMNlMsgDraw(0x17);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(procInit1);
}

static void procInit1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(3);
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
    DWCUtilInfoMenu_EraseHandleInputInline();
}

static void draw(void) {
}

static void button(void) {
    switch (DWCi_BTNlGet()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        DWCi_ChangeScene(DWCUtilInfoMenu_EraseDisableButtons);
        break;
    case 1:
        DWCUtilSound_PlaySe(6);
        DWCUtilDialog_Open(0x18, 0, 1, -1, 0);
        DWCi_BTNlDisable();
        DWCi_ChangeScene(procConfirm);
        break;
    }
}

static void DWCUtilInfoMenu_EraseDisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_BTNlEnd();
        if (!sErase) {
            DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(0) == 0 && (sErase || DWCi_EFFlCheck(1) == 0) && DWCi_BTNlIsEnd()) {
        DWCi_FNTlDeleteBgLcd(0);
        if (!sErase) {
            DWCi_COMMENTlDelete();
            DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        }
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sErase) {
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCUtilOptionMenu_Init);
        } else {
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCUtilInfoMenu_EraseSettings);
        }
    }
}

// The confirmation: 1 erases the settings
static void procConfirm(void) {
    switch (DWCi_WINlGet()) {
    case 1:
        DWCUtilSound_PlaySe(0xe);
        sErase = TRUE;
        break;
    case 0:
        DWCUtilSound_PlaySe(7);
        break;
    default:
        return;
    }
    DWCi_WINlEnd();
    DWCi_ChangeScene(procConfirm1);
}

static void procConfirm1(void) {
    if (!DWCi_WINlIsInit()) {
        DWCi_ChangeScene(DWCUtilInfoMenu_EraseDisableButtons);
    }
}

// Erases the settings, and tells the player to turn the console off
static void DWCUtilInfoMenu_EraseSettings(void) {
    DWCUtilInfoMenu_LoadErasedGraphics();
    DWCi_CMNlMsgDraw(0x19);
    DWCi_SETTINGlEraseAll();
    DWCi_ChangeScene(DWCUtilInfoMenu_ErasedFadeIn);
}

static void DWCUtilInfoMenu_LoadErasedGraphics(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilInfoMenu_ErasedFadeIn(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(DWCUtilInfoMenu_ErasedWaitFadeIn);
}

static void DWCUtilInfoMenu_ErasedWaitFadeIn(void) {
    if (DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(5);
        DWCi_ChangeScene(DWCUtilInfoMenu_ErasedWaitButtons);
    }
}

static void DWCUtilInfoMenu_ErasedWaitButtons(void) {
    if (DWCi_BTNlGet() != -2) {
        DWCi_BTNlEnable();
        DWCi_ChangeScene(DWCUtilInfoMenu_ErasedMain);
    }
}

static void DWCUtilInfoMenu_ErasedMain(void) {
    DWCUtilInfoMenu_ErasedHandleInput();
    func_ov182_021aeea4();
    DWCUtilInfoMenu_ErasedHandleChoice();
}

static void DWCUtilInfoMenu_ErasedHandleInput(void) {
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(0);
    }
}

static void func_ov182_021aeea4(void) {
}

static void DWCUtilInfoMenu_ErasedHandleChoice(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCi_ChangeScene(DWCUtilInfoMenu_ErasedDisableButtons);
    }
}

static void DWCUtilInfoMenu_ErasedDisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(DWCUtilInfoMenu_ErasedFadeOut);
}

static void DWCUtilInfoMenu_ErasedFadeOut(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, DWC_UTIL_FADE_PLANES, 0x40);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, DWC_UTIL_FADE_PLANES, 0x40);
        DWCi_ChangeScene(DWCUtilInfoMenu_PowerOff);
    }
}

// Turns the console off once both screens have faded out
static void DWCUtilInfoMenu_PowerOff(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        OS_SpinWaitSysCycles(0x800000);
        PM_ForceToPowerOff();
    }
}

void DWCi_SceneOptInfo(void) {
    DWCUtilInfoMenu_LoadInfoGraphics();
    DWCi_HLlLoad(0x11);
    DWCi_COMMENTlDispEx(0x3a, -1, 0);
    DWCUtilInfoMenu_DrawConsoleInfo();
    DWCi_ChangeScene(DWCUtilInfoMenu_InfoFadeIn);
}

static void DWCUtilInfoMenu_LoadInfoGraphics(void) {
    DWCUtilGfxResource_Load("char/jb5Info.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

// Writes the console's MAC address and its Wi-Fi ID, in four groups of four digits, the last of them the ID's last
// digit and three zeros
static void DWCUtilInfoMenu_DrawConsoleInfo(void) {
    u64 id;
    int i;
    DWCWiFiInfo info;
    int groups[4];
    u16 wide[0x14];
    u8 mac[6];
    char text[0x14];
    DWCUtilText *textObj;

    textObj = DWCi_FNTlFormBgEx(0, 0);
    OS_GetMacAddress(mac);
    OS_SNPrintf(text, sizeof(text), "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    DWCi_STRlConvertStringAsciiToUnicode(wide, (const u8 *)text, sizeof(text));
    DWCi_FNTlDrawRect(textObj, 8, 0x40, 0xf0, 0x1c, 2, 0x480, wide);
    DWCi_BM_GetWiFiInfo(&info);
    id = info.attestedUserId;
    if (id != 0) {
        groups[3] = (id % 10) * 1000;
        id /= 10;
        for (i = 0; i < 3; i++) {
            groups[2 - i] = id % 10000;
            id /= 10000;
        }
        OS_SNPrintf(text, sizeof(text), "%04d-%04d-%04d-%04d", groups[0], groups[1], groups[2], groups[3]);
        DWCi_STRlConvertStringAsciiToUnicode(wide, (const u8 *)text, sizeof(text));
    } else {
        OS_SNPrintf(text, sizeof(text), "-------------------");
        DWCi_STRlConvertStringAsciiToUnicode(wide, (const u8 *)text, sizeof(text));
    }
    DWCi_FNTlDrawRect(textObj, 8, 0x78, 0xf0, 0x1c, 2, 0x480, wide);
    DWCUtilText_MarkDirty(textObj);
}

static void DWCUtilInfoMenu_InfoFadeIn(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(DWCUtilInfoMenu_InfoWaitFadeIn);
}

static void DWCUtilInfoMenu_InfoWaitFadeIn(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(2);
        DWCi_ChangeScene(DWCUtilInfoMenu_InfoWaitButtons);
    }
}

static void DWCUtilInfoMenu_InfoWaitButtons(void) {
    if (DWCi_BTNlGet() != -2) {
        DWCi_BTNlEnable();
        DWCi_ChangeScene(DWCUtilInfoMenu_InfoMain);
    }
}

static void DWCUtilInfoMenu_InfoMain(void) {
    DWCUtilInfoMenu_InfoHandleInput();
    func_ov182_021af19c();
    DWCUtilInfoMenu_InfoHandleChoice();
}

static void DWCUtilInfoMenu_InfoHandleInput(void) {
    if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    }
}

static void func_ov182_021af19c(void) {
}

static void DWCUtilInfoMenu_InfoHandleChoice(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCUtilSound_PlaySe(7);
        DWCi_ChangeScene(DWCUtilInfoMenu_InfoDisableButtons);
    }
}

static void DWCUtilInfoMenu_InfoDisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(DWCUtilInfoMenu_InfoFadeOut);
}

static void DWCUtilInfoMenu_InfoFadeOut(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_BTNlEnd();
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(DWCUtilInfoMenu_InfoWaitFadeOut);
    }
}

static void DWCUtilInfoMenu_InfoWaitFadeOut(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0 && DWCi_BTNlIsEnd()) {
        DWCi_FNTlDeleteBgLcd(0);
        DWCi_COMMENTlDelete();
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCUtilOptionMenu_Init);
    }
}
