#include "dwc_util_screen_way.h"
#include "dwc_util_address_settings.h"
#include "dwc_util_ap_search.h"
#include "dwc_util_bg_buffer.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_button_sprites.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_page_step3.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_ap.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_test_start.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"

// The buttons: the search for an access point, the AOSS setup, the third setup and the settings by hand
#define SCREEN_WAY_BUTTONS 4

// What a move of the cursor does in sScreenWayMoves: nothing, go back to the button of the bottom row that the cursor
// left, or go to the button given (0 stays the top button, which also keeps the button left)
#define SCREEN_WAY_STAY -1
#define SCREEN_WAY_BACK -2

typedef struct DWCUtilScreenWay {
    // The button of the bottom row that the cursor last left, and the button under the cursor
    s8 left;
    s8 cursor;
    u8 chosen;
    // The screen whose palette rows light up the chosen button
    void *palette;
} DWCUtilScreenWay;

static void func_ov182_021b5c88(void);
static void func_ov182_021b5d20(void);
static void func_ov182_021b5d58(void);
static void func_ov182_021b5d84(void);
static void func_ov182_021b5da4(void);
static void func_ov182_021b5db4(void);
static void func_ov182_021b5eb8(void);
static void func_ov182_021b5ebc(void);
static void func_ov182_021b5ef4(void);
static void func_ov182_021b5f0c(void);
static void func_ov182_021b5f4c(void);
static void func_ov182_021b6024(int direction);
static void func_ov182_021b60d0(void);

// The slot of the prompt's number by language; for each layout, where the cursor goes from each button to the left,
// up, right and down; and for each layout, where each button is touched and where the cursor frames it: left, top,
// right and bottom
static const s8 sScreenWayMsgSlots[7] = { 3, -1, -1, -1, -1, -1, 6 };
static const s8 sScreenWayMoves[2][SCREEN_WAY_BUTTONS][4] = {
    { { -1, -2, -1, -2 }, { 3, 0, 3, 0 }, { -1, -1, -1, -1 }, { 1, 0, 1, 0 } },
    { { -1, -2, -1, -2 }, { 3, 0, 2, 0 }, { 1, 0, 3, 0 }, { 2, 0, 1, 0 } },
};
static const u16 sScreenWayTouchRects[2][SCREEN_WAY_BUTTONS][4] = {
    { { 0x08, 0x20, 0xf8, 0x5c }, { 0x08, 0x64, 0x7e, 0xa0 }, { 0, 0, 0, 0 }, { 0x83, 0x64, 0xf8, 0xa0 } },
    { { 0x08, 0x20, 0xf8, 0x5c }, { 0x08, 0x64, 0x64, 0xa0 }, { 0x68, 0x64, 0xc4, 0xa0 }, { 0xc8, 0x64, 0xf8, 0xa0 } },
};
static const u16 sScreenWayCursorRects[2][SCREEN_WAY_BUTTONS][4] = {
    { { 0x06, 0x1e, 0xea, 0x4e }, { 0x06, 0x62, 0x70, 0x92 }, { 0, 0, 0, 0 }, { 0x80, 0x62, 0xea, 0x92 } },
    { { 0x06, 0x1e, 0xea, 0x4e }, { 0x06, 0x62, 0x56, 0x92 }, { 0x66, 0x62, 0xb6, 0x92 }, { 0xc6, 0x62, 0xea, 0x92 } },
};

static DWCUtilScreenWay sScreenWay;

void func_ov182_021b5bb8(void) {
    DWCUtilSettingsEdit *edit = DWCUtilWifiSettings_GetEdit();

    sScreenWay.chosen = FALSE;
    if (sScreenWay.left == 0) {
        sScreenWay.left = 1;
    }
    if (DWCUtilMain_GetLanguage() != 0) {
        if (sScreenWay.cursor == 2) {
            sScreenWay.cursor = 0;
        }
        if (sScreenWay.left == 2) {
            sScreenWay.left = 1;
        }
    }
    func_ov182_021b5c88();
    DWCUtilBgOverlay_ShowConnectionSet();
    DWCUtilScreen_Start(0x32, sScreenWayMsgSlots[DWCUtilMain_GetLanguage()], edit->index + 1);
    DWCUtilGfxResource_ShowStep(1);
    DWCUtilButtonSprites_ShowFour(sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][0],
                                  sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][2],
                                  sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][1],
                                  sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][3]);
    DWCUtilMain_SetUpdate(func_ov182_021b5d20);
}

static void func_ov182_021b5c88(void) {
    char palettePath[] = "char/ybBgStep21.ncl.l";

    DWCUtilGfxResource_Load("char/jbBgStep2.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep2.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jb3Way.nsc.l", GX_LoadBG2Scr);
    sScreenWay.palette = DWCUtilRomResource_LoadFile(DWCUtilGfxResource_LocalizePath(palettePath), NULL, 4);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b5d20(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x14, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x14);
    DWCUtilMain_SetUpdate(func_ov182_021b5d58);
}

static void func_ov182_021b5d58(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(0);
        DWCUtilMain_SetUpdate(func_ov182_021b5d84);
    }
}

static void func_ov182_021b5d84(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(func_ov182_021b5da4);
    }
}

static void func_ov182_021b5da4(void) {
    func_ov182_021b5db4();
    func_ov182_021b5eb8();
    func_ov182_021b5ebc();
}

static void func_ov182_021b5db4(void) {
    u32 i;

    for (i = 0; i < SCREEN_WAY_BUTTONS; i++) {
        if (DWCUtilTouch_IsTriggeredIn(sScreenWayTouchRects[DWCUtilMain_HasFlag(1)][i])) {
            DWCUtilUiObject_SetStateIfIdle(1);
            sScreenWay.cursor = i;
            DWCUtilButtonSprites_ShowFour(sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][0],
                                          sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][2],
                                          sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][1],
                                          sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][3]);
            return;
        }
    }
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
    } else if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (DWCUtilTouch_IsKeyRepeated(0x40)) {
        func_ov182_021b6024(1);
    } else if (DWCUtilTouch_IsKeyRepeated(0x80)) {
        func_ov182_021b6024(3);
    } else if (DWCUtilTouch_IsKeyRepeated(0x20)) {
        func_ov182_021b6024(0);
    } else if (DWCUtilTouch_IsKeyRepeated(0x10)) {
        func_ov182_021b6024(2);
    }
}

static void func_ov182_021b5eb8(void) {
}

static void func_ov182_021b5ebc(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        break;
    case 1:
        DWCUtilSound_PlaySe(6);
        func_ov182_021b60d0();
        sScreenWay.chosen = TRUE;
        break;
    default:
        return;
    }
    DWCUtilMain_SetUpdate(func_ov182_021b5ef4);
}

static void func_ov182_021b5ef4(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(func_ov182_021b5f0c);
}

static void func_ov182_021b5f0c(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        if (sScreenWay.chosen) {
            DWCUtilUiObject_Exit();
        }
        DWCUtilFade_Start(3, 1, 1, 8);
        DWCUtilFade_Start(3, 0, 0x14, 8);
        DWCUtilMain_SetUpdate(func_ov182_021b5f4c);
    }
}

static void func_ov182_021b5f4c(void) {
    if (DWCUtilFade_IsActive(1) != 0 || DWCUtilFade_IsActive(0) != 0) {
        return;
    }
    if (!DWCUtilUiObject_IsIdle()) {
        return;
    }
    DWCUtilButtonSprites_Hide();
    DWCUtilScreen_Stop();
    DWCUtilRomResource_FreeFile(sScreenWay.palette);
    DWCUtilDisplay_HidePlanes(1, 1);
    DWCUtilDisplay_HidePlanes(0, 0x14);
    if (!sScreenWay.chosen) {
        DWCUtilMain_SetLayout(2, 0);
        DWCUtilMain_SetUpdate(func_ov182_021b536c);
        return;
    }
    switch (sScreenWay.cursor) {
    case 0:
        DWCUtilMain_SetLayout(2, 1);
        DWCUtilMain_SetUpdate(DWCUtilApSearch_Init);
        break;
    case 1:
        DWCUtilMain_SetLayout(2, 1);
        DWCUtilMain_SetUpdate(func_ov182_021b06f4);
        break;
    case 2:
        DWCUtilMain_SetLayout(2, 1);
        DWCUtilMain_SetUpdate(DWCUtilTestStart_Init);
        break;
    case 3:
        DWCUtilMain_SetLayout(2, 0);
        DWCUtilMain_SetScreenArgs(0, 0);
        DWCUtilAddressSettings_SetFrom(1);
        DWCUtilMain_SetUpdate(DWCUtilAddressSettings_Init);
        break;
    }
}

// Moves the cursor left, up, right or down (0 to 3)
static void func_ov182_021b6024(int direction) {
    int cursor = sScreenWay.cursor;
    int next = sScreenWayMoves[DWCUtilMain_HasFlag(1)][cursor][direction];

    if (next == SCREEN_WAY_STAY) {
        return;
    }
    if (next == 0) {
        sScreenWay.left = cursor;
    }
    if (next == SCREEN_WAY_BACK) {
        sScreenWay.cursor = sScreenWay.left;
    } else {
        sScreenWay.cursor = next;
    }
    DWCUtilSound_PlaySe(8);
    DWCUtilButtonSprites_ShowFour(sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][0],
                                  sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][2],
                                  sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][1],
                                  sScreenWayCursorRects[DWCUtilMain_HasFlag(1)][sScreenWay.cursor][3]);
}

// Lights up the chosen button with the palette row of its place
static void func_ov182_021b60d0(void) {
    u8 rows[SCREEN_WAY_BUTTONS] = { 1, 2, 3, 4 };

    DWCUtilBgBuffer_LoadPaletteRow(sScreenWay.palette, rows[sScreenWay.cursor], rows[sScreenWay.cursor]);
}
