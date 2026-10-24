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

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void button(void);
static void DWCUtilOptionMenu_DisableButtons(void);
static void endScene1(void);
static void endScene2(void);
static void DWCUtilOptionMenu_MoveCursor(int direction);
static void dispPush(void);

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
    initGraphics();
    DWCi_HLlLoad(0x10);
    DWCi_STEPlChangeScr(3);
    DWCi_COMMENTlDispEx(0x39, -1, 0);
    sOptionMenu.cursor = DWCUtilCell_AllocOam(0, 0x5b);
    G2_SetOBJPosition(sOptionMenu.cursor, cursorPos[0], cursorPos[1]);
    G2_SetOBJPriority(sOptionMenu.cursor, 3);
    DWCi_FOCUSlDisp4(sCursorRects[sOptionMenu.selection][0], sCursorRects[sOptionMenu.selection][2],
                     sCursorRects[sOptionMenu.selection][1], sCursorRects[sOptionMenu.selection][3]);
    DWCi_ChangeScene(procInit);
}

// Loads the menu's graphics; without a Wi-Fi ID, the rows of the erasing's and the move's buttons are greyed out
static void initGraphics(void) {
    char plttPath[] = "char/ybBgOption.ncl.l";
    char rowsPlttPath[] = "char/ybBgOption1.ncl.l";
    u8 *pltt;
    DWCWiFiInfo info;
    int i;

    DWCUtilGfxResource_Load("char/jbBgOption.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/jb5OptMenu.nsc.l", GX_LoadBG2Scr);
    sOptionMenu.palette = DWCUtilRomResource_LoadFile(DWCi_Char(rowsPlttPath), NULL, 4);
    pltt = DWCUtilRomResource_LoadFile(DWCi_Char(plttPath), NULL, 4);
    DWCi_BM_GetWiFiInfo(&info);
    if (info.attestedUserId == 0) {
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
    DWCi_ARClRelease(pltt);
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

    for (i = 0; i < 3; i++) {
        if (DWCi_TPlCheckTrigger(sButtonRects[i])) {
            DWCi_BTNlSet(1);
            sOptionMenu.selection = i;
            DWCi_FOCUSlDisp4(sCursorRects[sOptionMenu.selection][0], sCursorRects[sOptionMenu.selection][2],
                             sCursorRects[sOptionMenu.selection][1], sCursorRects[sOptionMenu.selection][3]);
            return;
        }
    }
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(1);
    } else if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    } else if (DWCi_IPTlCheckRepeat(0x40)) {
        DWCUtilOptionMenu_MoveCursor(1);
    } else if (DWCi_IPTlCheckRepeat(0x80)) {
        DWCUtilOptionMenu_MoveCursor(3);
    }
}

static void draw(void) {
}

// Without a Wi-Fi ID, only the console's information can be chosen
static void button(void) {
    DWCWiFiInfo info;

    switch (DWCi_BTNlGet()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        break;
    case 1:
        DWCi_BM_GetWiFiInfo(&info);
        if (sOptionMenu.selection != 0 && info.attestedUserId == 0) {
            DWCUtilSound_PlaySe(9);
            DWCi_BTNlSetForce(-1);
            return;
        }
        DWCUtilSound_PlaySe(6);
        dispPush();
        sOptionMenu.started = TRUE;
        break;
    default:
        return;
    }
    DWCi_ChangeScene(DWCUtilOptionMenu_DisableButtons);
}

static void DWCUtilOptionMenu_DisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        if (sOptionMenu.started) {
            DWCi_BTNlEnd();
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0 && DWCi_BTNlIsEnd()) {
        DWCi_OBJlDelete(sOptionMenu.cursor);
        DWCi_FOCUSlDelete();
        DWCi_COMMENTlDelete();
        DWCi_ARClRelease(sOptionMenu.palette);
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sOptionMenu.started) {
            DWCi_SetParam_(0, 0);
            DWCi_ChangeScene(DWCi_SceneMenu);
        } else {
            switch (sOptionMenu.selection) {
            case 0:
                DWCi_SetParam_(0, 1);
                DWCi_ChangeScene(DWCi_SceneOptInfo);
                break;
            case 1:
                DWCi_SetParam_(0, 1);
                DWCi_ChangeScene(DWCUtilInfoMenu_InitErase);
                break;
            case 2:
                DWCi_SetParam_(0, 1);
                DWCi_ChangeScene(DWCUtilPromptYesNo_Init);
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
    DWCi_FOCUSlDisp4(sCursorRects[sOptionMenu.selection][0], sCursorRects[sOptionMenu.selection][2],
                     sCursorRects[sOptionMenu.selection][1], sCursorRects[sOptionMenu.selection][3]);
}

// Lights up the chosen button with its row of the palette
static void dispPush(void) {
    DWCi_SCREENlCopyPalette(sOptionMenu.palette, sButtonRows[sOptionMenu.selection],
                            sButtonRows[sOptionMenu.selection]);
}
