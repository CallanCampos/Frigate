#include "dwc_util_option_menu.h"
#include "dwc/bm.h"
#include "dwc_util_bg_buffer.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_button_sprites.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_info_menu.h"
#include "dwc_util_main.h"
#include "dwc_util_main_menu.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_prompt_yes_no.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_sub_screen.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The chosen button, whether the player started it, the palette whose rows light up the buttons, and the cursor's
// sprite
typedef struct DWCUtilOptionMenu {
    u8 selection;
    u8 started;
    void *palette;
    GXOamAttr *cursor;
} DWCUtilOptionMenu;

static void DWCUtilOptionMenu_LoadGraphics(void);
static void DWCUtilOptionMenu_FadeIn(void);
static void DWCUtilOptionMenu_WaitFadeIn(void);
static void DWCUtilOptionMenu_WaitButtons(void);
static void DWCUtilOptionMenu_Main(void);
static void DWCUtilOptionMenu_HandleInput(void);
static void func_ov182_021af524(void);
static void DWCUtilOptionMenu_HandleChoice(void);
static void DWCUtilOptionMenu_DisableButtons(void);
static void DWCUtilOptionMenu_FadeOut(void);
static void DWCUtilOptionMenu_WaitFadeOut(void);
static void DWCUtilOptionMenu_MoveCursor(int direction);
static void DWCUtilOptionMenu_HighlightSelection(void);

// The palette rows that light up each button
static const u8 sButtonRows[3] = { 1, 2, 3 };
// Where the three buttons are touched, and where the cursor frames them: left, top, right and bottom
static const u16 sButtonRects[3][4] = {
    { 0x08, 0x24, 0xf8, 0x44 },
    { 0x08, 0x50, 0xf8, 0x70 },
    { 0x08, 0x7c, 0xf8, 0x9c },
};
static const u16 sCursorRects[3][4] = {
    { 0x06, 0x22, 0xea, 0x36 },
    { 0x06, 0x4e, 0xea, 0x62 },
    { 0x06, 0x7a, 0xea, 0x8e },
};

static DWCUtilOptionMenu sOptionMenu;

void DWCUtilOptionMenu_Init(void) {
    // The cursor sprite's position
    static const u16 cursorPos[2] = { 0xe0, 0x84 };

    sOptionMenu.started = FALSE;
    DWCUtilOptionMenu_LoadGraphics();
    DWCUtilBgOverlay_Show(0x10);
    DWCUtilSubScreen_LoadScreen(3);
    DWCUtilScreen_Start(0x39, -1, 0);
    sOptionMenu.cursor = DWCUtilCell_AllocOam(0, 0x5b);
    G2_SetOBJPosition(sOptionMenu.cursor, cursorPos[0], cursorPos[1]);
    G2_SetOBJPriority(sOptionMenu.cursor, 3);
    DWCUtilButtonSprites_ShowFour(sCursorRects[sOptionMenu.selection][0], sCursorRects[sOptionMenu.selection][2],
                                  sCursorRects[sOptionMenu.selection][1], sCursorRects[sOptionMenu.selection][3]);
    DWCUtilMain_SetUpdate(DWCUtilOptionMenu_FadeIn);
}

// Loads the menu's graphics; without a Wi-Fi ID, the rows of the erasing's and the move's buttons are greyed out
static void DWCUtilOptionMenu_LoadGraphics(void) {
    char plttPath[] = "char/ybBgOption.ncl.l";
    char rowsPlttPath[] = "char/ybBgOption1.ncl.l";
    u8 *pltt;
    DWCBmWifiIdInfo info;
    int i;

    DWCUtilGfxResource_Load("char/jbBgOption.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/jb5OptMenu.nsc.l", GX_LoadBG2Scr);
    sOptionMenu.palette = DWCUtilRomResource_LoadFile(DWCUtilGfxResource_LocalizePath(rowsPlttPath), NULL, 4);
    pltt = DWCUtilRomResource_LoadFile(DWCUtilGfxResource_LocalizePath(plttPath), NULL, 4);
    DWCBm_GetWifiIdInfo(&info);
    if (info.id == 0) {
        u8 *src = pltt + 0xc0;
        u8 *dest = pltt + 0x40;

        for (i = 0; i < 2; i++) {
            MI_CpuCopy8(src, dest, 0x20);
            src += 0x20;
            dest += 0x20;
        }
    }
    DC_FlushRange(pltt, 0x200);
    GX_LoadBGPltt(pltt, 0, 0x200);
    DWCUtilRomResource_FreeFile(pltt);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilOptionMenu_FadeIn(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x14, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x14);
    DWCUtilMain_SetUpdate(DWCUtilOptionMenu_WaitFadeIn);
}

static void DWCUtilOptionMenu_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(0);
        DWCUtilMain_SetUpdate(DWCUtilOptionMenu_WaitButtons);
    }
}

static void DWCUtilOptionMenu_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilOptionMenu_Main);
    }
}

static void DWCUtilOptionMenu_Main(void) {
    DWCUtilOptionMenu_HandleInput();
    func_ov182_021af524();
    DWCUtilOptionMenu_HandleChoice();
}

static void DWCUtilOptionMenu_HandleInput(void) {
    u32 i;

    for (i = 0; i < 3; i++) {
        if (DWCUtilTouch_IsTriggeredIn(sButtonRects[i])) {
            DWCUtilUiObject_SetStateIfIdle(1);
            sOptionMenu.selection = i;
            DWCUtilButtonSprites_ShowFour(
                sCursorRects[sOptionMenu.selection][0], sCursorRects[sOptionMenu.selection][2],
                sCursorRects[sOptionMenu.selection][1], sCursorRects[sOptionMenu.selection][3]);
            return;
        }
    }
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
    } else if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (DWCUtilTouch_IsKeyRepeated(0x40)) {
        DWCUtilOptionMenu_MoveCursor(1);
    } else if (DWCUtilTouch_IsKeyRepeated(0x80)) {
        DWCUtilOptionMenu_MoveCursor(3);
    }
}

static void func_ov182_021af524(void) {
}

// Without a Wi-Fi ID, only the console's information can be chosen
static void DWCUtilOptionMenu_HandleChoice(void) {
    DWCBmWifiIdInfo info;

    switch (DWCUtilUiObject_GetState()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        break;
    case 1:
        DWCBm_GetWifiIdInfo(&info);
        if (sOptionMenu.selection != 0 && info.id == 0) {
            DWCUtilSound_PlaySe(9);
            DWCUtilUiObject_SetState(-1);
            return;
        }
        DWCUtilSound_PlaySe(6);
        DWCUtilOptionMenu_HighlightSelection();
        sOptionMenu.started = TRUE;
        break;
    default:
        return;
    }
    DWCUtilMain_SetUpdate(DWCUtilOptionMenu_DisableButtons);
}

static void DWCUtilOptionMenu_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilOptionMenu_FadeOut);
}

static void DWCUtilOptionMenu_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        if (sOptionMenu.started) {
            DWCUtilUiObject_Exit();
        }
        DWCUtilFade_Start(3, 1, 1, 8);
        DWCUtilFade_Start(3, 0, 0x14, 8);
        DWCUtilMain_SetUpdate(DWCUtilOptionMenu_WaitFadeOut);
    }
}

static void DWCUtilOptionMenu_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0 && DWCUtilUiObject_IsIdle()) {
        DWCUtilOamBuf_Free(sOptionMenu.cursor);
        DWCUtilButtonSprites_Hide();
        DWCUtilScreen_Stop();
        DWCUtilRomResource_FreeFile(sOptionMenu.palette);
        DWCUtilDisplay_HidePlanes(1, 1);
        DWCUtilDisplay_HidePlanes(0, 0x14);
        if (!sOptionMenu.started) {
            DWCUtilMain_SetLayout(0, 0);
            DWCUtilMain_SetUpdate(DWCUtilMainMenu_Init);
        } else {
            switch (sOptionMenu.selection) {
            case 0:
                DWCUtilMain_SetLayout(0, 1);
                DWCUtilMain_SetUpdate(DWCUtilInfoMenu_InitInfo);
                break;
            case 1:
                DWCUtilMain_SetLayout(0, 1);
                DWCUtilMain_SetUpdate(DWCUtilInfoMenu_InitErase);
                break;
            case 2:
                DWCUtilMain_SetLayout(0, 1);
                DWCUtilMain_SetUpdate(DWCUtilPromptYesNo_Init);
                break;
            }
        }
    }
}

// Up and down move the cursor through the three buttons, wrapping around
static void DWCUtilOptionMenu_MoveCursor(int direction) {
    if (direction == 1) {
        sOptionMenu.selection = FX_ModS32(sOptionMenu.selection + 2, 3);
    } else {
        sOptionMenu.selection = FX_ModS32(sOptionMenu.selection + 1, 3);
    }
    DWCUtilSound_PlaySe(8);
    DWCUtilButtonSprites_ShowFour(sCursorRects[sOptionMenu.selection][0], sCursorRects[sOptionMenu.selection][2],
                                  sCursorRects[sOptionMenu.selection][1], sCursorRects[sOptionMenu.selection][3]);
}

// Lights up the chosen button with its row of the palette
static void DWCUtilOptionMenu_HighlightSelection(void) {
    DWCUtilBgBuffer_LoadPaletteRow(sOptionMenu.palette, sButtonRows[sOptionMenu.selection],
                                   sButtonRows[sOptionMenu.selection]);
}
