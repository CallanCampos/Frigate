#include "sceneoptinfo.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc/bm.h"
#include "dwc_util_main.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "headline.h"
#include "input.h"
#include "message.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "sceneoptmenu.h"
#include "sound.h"
#include "str.h"

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

void DWCi_SceneOptInfo(void) {
    DWCUtilInfoMenu_LoadInfoGraphics();
    DWCi_HLlLoad(0x11);
    DWCi_COMMENTlDispEx(0x3a, -1, 0);
    DWCUtilInfoMenu_DrawConsoleInfo();
    DWCi_ChangeScene(DWCUtilInfoMenu_InfoFadeIn);
}

static void DWCUtilInfoMenu_LoadInfoGraphics(void) {
    DWCi_CMNlLoadResource("char/jb5Info.nsc.l", GX_LoadBG2Scr);
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
    DWCi_FNTlRenewBg(textObj);
}

static void DWCUtilInfoMenu_InfoFadeIn(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlVisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlVisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
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
        DWCi_SNDlPlay(7);
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
        DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCi_SceneOptMenu);
    }
}
