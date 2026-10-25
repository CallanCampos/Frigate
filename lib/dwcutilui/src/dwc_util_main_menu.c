#include "dwc_util_main_menu.h"
#include "archive.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_bg_buffer.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "focus.h"
#include "headline.h"
#include "input.h"
#include "message.h"
#include "nitro/gx.h"
#include "rakuraku.h"
#include "sceneoptmenu.h"
#include "scenesetselectap.h"
#include "setting.h"
#include "sound.h"
#include "step.h"
#include "task.h"

// The choice on the first screen, and the palette whose rows light up the chosen button
typedef struct DWCUtilMainMenu {
    u8 selection;
    void *palette;
} DWCUtilMainMenu;

static void DWCUtilMainMenu_WaitExitUtility(void);
static void DWCUtilMainMenu_FadeInUtility(void);
static void DWCUtilMainMenu_WaitFadeInUtility(void);
static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void button(void);
static void DWCUtilMainMenu_DisableButtons(void);
static void endScene1(void);
static void endScene2(void);
static void DWCUtilMainMenu_MoveCursor(int direction);
static void DWCUtilMainMenu_HighlightSelection(void);

// Where the cursor frames the two buttons, and where they are touched: left, top, right and bottom
static const u16 sCursorRects[2][4] = { { 6, 0x1e, 0x9e, 0x92 }, { 0xb2, 0x1e, 0xea, 0x92 } };
static const u16 sButtonRects[2][4] = { { 8, 0x20, 0xac, 0xa0 }, { 0xb4, 0x20, 0xf8, 0xa0 } };

// The message files by language
static char *sMsgPaths[7] = {
    "msg/jap.bmg.l", "msg/eng.bmg.l", "msg/fre.bmg.l", "msg/ger.bmg.l",
    "msg/ita.bmg.l", "msg/spa.bmg.l", "msg/kor.bmg.l",
};

static DWCUtilMainMenu sMainMenu;

void DWCUtilMainMenu_ExitUtility(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, DWC_UTIL_FADE_PLANES, 0x14);
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, DWC_UTIL_FADE_PLANES, 0x14);
    DWCi_ChangeScene(DWCUtilMainMenu_WaitExitUtility);
}

static void DWCUtilMainMenu_WaitExitUtility(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_TSKlOperate(0, 0);
        DWCi_TSKlOperate(1, 0);
        DWCUtilCell_Unload(1);
        DWCUtilCell_Unload(0);
        DWCi_HLlEnd();
        DWCi_FOCUSlEnd();
        DWCi_MSGlRelease(gDWCUtilMsg);
        DWCi_MSGlEnd();
        DWCi_SETTIGNlEnd();
        DWCi_EndUtility();
    }
}

void DWCUtilMainMenu_InitUtility(void) {
    DWCi_SETTIGNlInit();
    DWCi_MSGlInit();
    DWCi_FOCUSlInit();
    DWCi_HLlInit();
    DWCi_COMMENTlInit();
    if (DWCi_Language() == 1 && DWCi_Flag(2)) {
        gDWCUtilMsg = DWCi_MSGlRead("msg/usa.bmg.l");
    } else {
        gDWCUtilMsg = DWCi_MSGlRead(sMsgPaths[DWCi_Language()]);
    }
    DWCUtilCell_Load(1, DWCi_Char("char/jtMain.nce.l"));
    DWCUtilCell_Load(0, DWCi_Char("char/jbMain.nce.l"));
    DWCUtilGfxResource_Load("char/jtBgMain.ncg.l", GXS_LoadBG1Char);
    DWCUtilGfxResource_Load("char/jtBgMain.ncl.l", GXS_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jtObjMain.ncg.l", GXS_LoadOBJ);
    DWCUtilGfxResource_Load("char/xtObjMain.ncl.l", GXS_LoadOBJPltt);
    DWCUtilGfxResource_Load("char/jbBgStep1.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/jbBgStep1.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jbObjMain.ncg.l", GX_LoadOBJ);
    DWCUtilGfxResource_Load("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    switch (DWCi_Entry()) {
    case 0:
        DWCUtilGfxResource_Load("char/jtTop.nsc.l", GXS_LoadBG1Scr);
        break;
    case 1:
        DWCUtilGfxResource_Load("char/jtStep1.nsc.l", GXS_LoadBG1Scr);
        break;
    }
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG1);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG1);
    DWCi_ChangeScene(DWCUtilMainMenu_FadeInUtility);
}

static void DWCUtilMainMenu_FadeInUtility(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG1, 0x14);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG1, 0x14);
    DWCi_ChangeScene(DWCUtilMainMenu_WaitFadeInUtility);
}

static void DWCUtilMainMenu_WaitFadeInUtility(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        switch (DWCi_Entry()) {
        case 0:
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCi_SceneMenu);
            break;
        case 1:
            DWCi_SetParam_(1, 1);
            DWCi_ChangeScene(DWCUtilScreenAp_Init);
            break;
        }
    }
}

void DWCi_SceneMenu(void) {
    initGraphics();
    DWCi_HLlLoad(0);
    DWCi_COMMENTlDispEx(0x2e, -1, 0);
    DWCi_STEPlChangeScr(4);
    DWCi_FOCUSlDisp4(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                     sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    char palettePath[] = "char/ybBgStep11.ncl.l";

    DWCUtilGfxResource_Load("char/jbBgStep1.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/jbBgStep1.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jb2Menu.nsc.l", GX_LoadBG2Scr);
    sMainMenu.palette = DWCUtilRomResource_LoadFile(DWCi_Char(palettePath), NULL, 4);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
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

    for (i = 0; i < 2; i++) {
        if (DWCi_TPlCheckTrigger(sButtonRects[i])) {
            DWCi_BTNlSet(1);
            sMainMenu.selection = i;
            DWCi_FOCUSlDisp4(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                             sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
            return;
        }
    }
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(1);
    } else if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    } else if (DWCi_IPTlCheckRepeat(0x40)) {
        DWCUtilMainMenu_MoveCursor(1);
    } else if (DWCi_IPTlCheckRepeat(0x80)) {
        DWCUtilMainMenu_MoveCursor(3);
    } else if (DWCi_IPTlCheckRepeat(0x20)) {
        DWCUtilMainMenu_MoveCursor(0);
    } else if (DWCi_IPTlCheckRepeat(0x10)) {
        DWCUtilMainMenu_MoveCursor(2);
    }
}

static void draw(void) {
}

static void button(void) {
    switch (DWCi_BTNlGet()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        DWCi_ChangeScene(DWCUtilMainMenu_ExitUtility);
        break;
    case 1:
        DWCUtilSound_PlaySe(6);
        DWCUtilMainMenu_HighlightSelection();
        DWCi_ChangeScene(DWCUtilMainMenu_DisableButtons);
        break;
    }
}

static void DWCUtilMainMenu_DisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_FOCUSlDelete();
        DWCi_COMMENTlDelete();
        DWCi_ARClRelease(sMainMenu.palette);
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        switch (sMainMenu.selection) {
        case 0:
            DWCi_SetParam_(1, 0);
            DWCi_ChangeScene(DWCUtilScreenAp_Init);
            break;
        case 1:
            DWCi_SetParam_(0, 0);
            DWCi_ChangeScene(DWCUtilOptionMenu_Init);
            break;
        }
    }
}

// Left and right move the cursor to the other button; up and down don't move it
static void DWCUtilMainMenu_MoveCursor(int direction) {
    if (direction != 1 && direction != 3) {
        sMainMenu.selection ^= 1;
        DWCUtilSound_PlaySe(8);
        DWCi_FOCUSlDisp4(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                         sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
    }
}

// Lights up the chosen button with its row of the palette
static void DWCUtilMainMenu_HighlightSelection(void) {
    u8 rows[2] = { 1, 2 };

    DWCi_SCREENlCopyPalette(sMainMenu.palette, rows[sMainMenu.selection], rows[sMainMenu.selection]);
}
