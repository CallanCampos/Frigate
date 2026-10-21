#include "dwc_util_main_menu.h"
#include "dwc_util_bg_buffer.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_button_sprites.h"
#include "dwc_util_cell.h"
#include "dwc_util_connection.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_msg_resource.h"
#include "dwc_util_option_menu.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_ap.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_sub_screen.h"
#include "dwc_util_task.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"

// The choice on the first screen, and the palette whose rows light up the chosen button
typedef struct DWCUtilMainMenu {
    u8 selection;
    void *palette;
} DWCUtilMainMenu;

static void DWCUtilMainMenu_WaitExitUtility(void);
static void DWCUtilMainMenu_FadeInUtility(void);
static void DWCUtilMainMenu_WaitFadeInUtility(void);
static void DWCUtilMainMenu_LoadGraphics(void);
static void DWCUtilMainMenu_FadeIn(void);
static void DWCUtilMainMenu_WaitFadeIn(void);
static void DWCUtilMainMenu_WaitButtons(void);
static void DWCUtilMainMenu_Main(void);
static void DWCUtilMainMenu_HandleInput(void);
static void func_ov182_021ae990(void);
static void DWCUtilMainMenu_HandleChoice(void);
static void DWCUtilMainMenu_DisableButtons(void);
static void DWCUtilMainMenu_FadeOut(void);
static void DWCUtilMainMenu_WaitFadeOut(void);
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
    DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 1, DWC_UTIL_FADE_PLANES, 0x14);
    DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 0, DWC_UTIL_FADE_PLANES, 0x14);
    DWCUtilMain_SetUpdate(DWCUtilMainMenu_WaitExitUtility);
}

static void DWCUtilMainMenu_WaitExitUtility(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilTask_SetActive(0, 0);
        DWCUtilTask_SetActive(1, 0);
        DWCUtilCell_Unload(1);
        DWCUtilCell_Unload(0);
        DWCUtilBgOverlay_Exit();
        DWCUtilButtonSprites_Exit();
        DWCUtilMsg_Close(gDWCUtilMsg);
        DWCUtilMsg_Exit();
        DWCUtilWifiSettings_Exit();
        DWCUtilMain_Finish();
    }
}

void DWCUtilMainMenu_InitUtility(void) {
    DWCUtilWifiSettings_Init();
    DWCUtilMsg_Init();
    DWCUtilButtonSprites_Init();
    DWCUtilBgOverlay_Init();
    DWCUtilScreen_Reset();
    if (DWCUtilMain_GetLanguage() == 1 && DWCUtilMain_HasFlag(2)) {
        gDWCUtilMsg = DWCUtilMsg_Open("msg/usa.bmg.l");
    } else {
        gDWCUtilMsg = DWCUtilMsg_Open(sMsgPaths[DWCUtilMain_GetLanguage()]);
    }
    DWCUtilCell_Load(1, DWCUtilGfxResource_LocalizePath("char/jtMain.nce.l"));
    DWCUtilCell_Load(0, DWCUtilGfxResource_LocalizePath("char/jbMain.nce.l"));
    DWCUtilGfxResource_Load("char/jtBgMain.ncg.l", GXS_LoadBG1Char);
    DWCUtilGfxResource_Load("char/jtBgMain.ncl.l", GXS_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jtObjMain.ncg.l", GXS_LoadOBJ);
    DWCUtilGfxResource_Load("char/xtObjMain.ncl.l", GXS_LoadOBJPltt);
    DWCUtilGfxResource_Load("char/jbBgStep1.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/jbBgStep1.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jbObjMain.ncg.l", GX_LoadOBJ);
    DWCUtilGfxResource_Load("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    switch (DWCUtilMain_GetStartScreen()) {
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
    DWCUtilDisplay_ShowPlanes(1, GX_PLANEMASK_BG1);
    DWCUtilDisplay_ShowPlanes(0, GX_PLANEMASK_BG1);
    DWCUtilMain_SetUpdate(DWCUtilMainMenu_FadeInUtility);
}

static void DWCUtilMainMenu_FadeInUtility(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG1, 0x14);
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG1, 0x14);
    DWCUtilMain_SetUpdate(DWCUtilMainMenu_WaitFadeInUtility);
}

static void DWCUtilMainMenu_WaitFadeInUtility(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        switch (DWCUtilMain_GetStartScreen()) {
        case 0:
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(DWCUtilMainMenu_Init);
            break;
        case 1:
            DWCUtilMain_SetLayout(1, 1);
            DWCUtilMain_SetUpdate(DWCUtilScreenAp_Init);
            break;
        }
    }
}

void DWCUtilMainMenu_Init(void) {
    DWCUtilMainMenu_LoadGraphics();
    DWCUtilBgOverlay_Show(0);
    DWCUtilScreen_Start(0x2e, -1, 0);
    DWCUtilSubScreen_LoadScreen(4);
    DWCUtilButtonSprites_ShowFour(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                                  sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
    DWCUtilMain_SetUpdate(DWCUtilMainMenu_FadeIn);
}

static void DWCUtilMainMenu_LoadGraphics(void) {
    char palettePath[] = "char/ybBgStep11.ncl.l";

    DWCUtilGfxResource_Load("char/jbBgStep1.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/jbBgStep1.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jb2Menu.nsc.l", GX_LoadBG2Scr);
    sMainMenu.palette = DWCUtilRomResource_LoadFile(DWCUtilGfxResource_LocalizePath(palettePath), NULL, 4);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilMainMenu_FadeIn(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilDisplay_ShowPlanes(1, GX_PLANEMASK_BG0);
    DWCUtilDisplay_ShowPlanes(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCUtilMain_SetUpdate(DWCUtilMainMenu_WaitFadeIn);
}

static void DWCUtilMainMenu_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(0);
        DWCUtilMain_SetUpdate(DWCUtilMainMenu_WaitButtons);
    }
}

static void DWCUtilMainMenu_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilMainMenu_Main);
    }
}

static void DWCUtilMainMenu_Main(void) {
    DWCUtilMainMenu_HandleInput();
    func_ov182_021ae990();
    DWCUtilMainMenu_HandleChoice();
}

static void DWCUtilMainMenu_HandleInput(void) {
    u32 i;

    for (i = 0; i < 2; i++) {
        if (DWCUtilTouch_IsTriggeredIn(sButtonRects[i])) {
            DWCUtilUiObject_SetStateIfIdle(1);
            sMainMenu.selection = i;
            DWCUtilButtonSprites_ShowFour(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                                          sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
            return;
        }
    }
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
    } else if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (DWCUtilTouch_IsKeyRepeated(0x40)) {
        DWCUtilMainMenu_MoveCursor(1);
    } else if (DWCUtilTouch_IsKeyRepeated(0x80)) {
        DWCUtilMainMenu_MoveCursor(3);
    } else if (DWCUtilTouch_IsKeyRepeated(0x20)) {
        DWCUtilMainMenu_MoveCursor(0);
    } else if (DWCUtilTouch_IsKeyRepeated(0x10)) {
        DWCUtilMainMenu_MoveCursor(2);
    }
}

static void func_ov182_021ae990(void) {
}

static void DWCUtilMainMenu_HandleChoice(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        DWCUtilMain_SetUpdate(DWCUtilMainMenu_ExitUtility);
        break;
    case 1:
        DWCUtilSound_PlaySe(6);
        DWCUtilMainMenu_HighlightSelection();
        DWCUtilMain_SetUpdate(DWCUtilMainMenu_DisableButtons);
        break;
    }
}

static void DWCUtilMainMenu_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilMainMenu_FadeOut);
}

static void DWCUtilMainMenu_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 0,
                          GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCUtilMain_SetUpdate(DWCUtilMainMenu_WaitFadeOut);
    }
}

static void DWCUtilMainMenu_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilButtonSprites_Hide();
        DWCUtilScreen_Stop();
        DWCUtilRomResource_FreeFile(sMainMenu.palette);
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        switch (sMainMenu.selection) {
        case 0:
            DWCUtilMain_SetLayout(1, 0);
            DWCUtilMain_SetUpdate(DWCUtilScreenAp_Init);
            break;
        case 1:
            DWCUtilMain_SetLayout(0, 0);
            DWCUtilMain_SetUpdate(DWCUtilOptionMenu_Init);
            break;
        }
    }
}

// Left and right move the cursor to the other button; up and down don't move it
static void DWCUtilMainMenu_MoveCursor(int direction) {
    if (direction != 1 && direction != 3) {
        sMainMenu.selection ^= 1;
        DWCUtilSound_PlaySe(8);
        DWCUtilButtonSprites_ShowFour(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                                      sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
    }
}

// Lights up the chosen button with its row of the palette
static void DWCUtilMainMenu_HighlightSelection(void) {
    u8 rows[2] = { 1, 2 };

    DWCUtilBgBuffer_LoadPaletteRow(sMainMenu.palette, rows[sMainMenu.selection], rows[sMainMenu.selection]);
}
