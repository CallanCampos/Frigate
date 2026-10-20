#include "dwc_util_screen_ap.h"
#include "dwc/bm.h"
#include "dwc_util_address_settings.h"
#include "dwc_util_bg_buffer.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_button_sprites.h"
#include "dwc_util_cell.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_main_menu.h"
#include "dwc_util_oam.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_way.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_sub_screen.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_usb_intro.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"

// The buttons of the screen, as the cursor goes over them: the three connections, the wide button, and the three
// buttons that erase a connection
#define SCREEN_AP_CONNECTIONS 3
#define SCREEN_AP_WIDE_BUTTON 3
#define SCREEN_AP_FIRST_ERASE 4
#define SCREEN_AP_BUTTONS 7

// How the screen ends: not yet, with a button, or with B
enum {
    SCREEN_AP_RUNNING,
    SCREEN_AP_CHOSEN,
    SCREEN_AP_CANCELLED,
};

typedef struct DWCUtilScreenAp {
    // The screen whose palette rows light up the chosen button
    void *palette;
    // The buttons of the connections, and the buttons that erase them, none for a connection not in use
    DWCUtilOamNode *buttons[SCREEN_AP_CONNECTIONS];
    DWCUtilOamNode *eraseButtons[SCREEN_AP_CONNECTIONS];
    // The column that the cursor left the top or the bottom row from, for the wide button to go back to
    u8 column;
    u8 end;
} DWCUtilScreenAp;

typedef struct DWCUtilScreenApState {
    u8 cursor;
    DWCUtilScreenAp *work;
} DWCUtilScreenApState;

static void DWCUtilScreenAp_LoadGraphics(void);
static void DWCUtilScreenAp_CreateButtons(void);
static void DWCUtilScreenAp_FadeIn(void);
static void DWCUtilScreenAp_WaitFadeIn(void);
static void DWCUtilScreenAp_WaitButtons(void);
static void DWCUtilScreenAp_Main(void);
static void DWCUtilScreenAp_HandleInput(void);
static void func_ov182_021b5648(void);
static void DWCUtilScreenAp_HandleChoice(void);
static void DWCUtilScreenAp_DisableButtons(void);
static void DWCUtilScreenAp_FadeOut(void);
static void DWCUtilScreenAp_WaitFadeOut(void);
static void DWCUtilScreenAp_DrawCursor(void);
static void DWCUtilScreenAp_MoveCursor(int direction);
static void DWCUtilScreenAp_ShowErasePressed(void);
static void DWCUtilScreenAp_WaitEraseAnswer(void);
static void DWCUtilScreenAp_WaitDialogClosed(void);
static void DWCUtilScreenAp_HighlightButton(void);

// The cell of a connection's button by its status (DWCBmConnection.status, 3 for one not in use), where the cursor
// frames each button and where it is touched (left, top, right and bottom), the slot of the erase prompt's number by
// language, and where the buttons of the connections and then their erase buttons are. Declared in the order that,
// with MWCC's sort of the file's data by size, gives the ROM's .rodata
static const u8 sScreenApCells[4] = { 0x13, 0x14, 0x12, 0x56 };
static const u16 sScreenApCursorRects[SCREEN_AP_BUTTONS][4] = {
    { 0x06, 0x1e, 0x46, 0x48 }, { 0x58, 0x1e, 0x98, 0x48 }, { 0xaa, 0x1e, 0xea, 0x48 }, { 0x06, 0x76, 0xea, 0x92 },
    { 0x09, 0x54, 0x43, 0x70 }, { 0x5b, 0x54, 0x95, 0x70 }, { 0xad, 0x54, 0xe7, 0x70 },
};
static const u16 sScreenApTouchRects[SCREEN_AP_BUTTONS][4] = {
    { 0x08, 0x20, 0x54, 0x56 }, { 0x5a, 0x20, 0xa6, 0x56 }, { 0xac, 0x20, 0xf8, 0x56 }, { 0x08, 0x78, 0xf8, 0xa0 },
    { 0x08, 0x54, 0x54, 0x70 }, { 0x5a, 0x54, 0xa6, 0x70 }, { 0xac, 0x54, 0xf8, 0x70 },
};
static const s8 sScreenApMsgSlots[8] = { 3, 0x2d, 0x27, 0x1d, 0x32, 0x30, 6, 0 };
static const u16 sScreenApButtonPos[SCREEN_AP_CONNECTIONS * 2][2] = {
    { 8, 0x30 }, { 0x5a, 0x30 }, { 0xac, 0x30 }, { 0x0c, 0x58 }, { 0x5e, 0x58 }, { 0xb0, 0x58 },
};

static DWCUtilScreenApState sScreenAp;

void DWCUtilScreenAp_Init(void) {
    sScreenAp.work = DWCUtilHeap_AllocClear(sizeof(DWCUtilScreenAp), 4);
    sScreenAp.work->end = SCREEN_AP_RUNNING;
    DWCUtilScreenAp_LoadGraphics();
    DWCUtilBgOverlay_Show(1);
    DWCUtilScreen_Start(0x2f, -1, 0);
    DWCUtilGfxResource_ShowStep(0);
    DWCUtilScreenAp_CreateButtons();
    DWCUtilScreenAp_DrawCursor();
    DWCUtilMain_SetUpdate(DWCUtilScreenAp_FadeIn);
}

static void DWCUtilScreenAp_LoadGraphics(void) {
    char palettePath[] = "char/ybBgStep11.ncl.l";

    DWCUtilGfxResource_Load("char/ybObjWay.ncl.l", GX_LoadOBJPltt);
    DWCUtilGfxResource_Load("char/jbBgStep1.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/jbBgStep1.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jb2Ap.nsc.l", GX_LoadBG2Scr);
    sScreenAp.work->palette = DWCUtilRomResource_LoadFile(DWCUtilGfxResource_LocalizePath(palettePath), NULL, 4);
    DWCUtilDisplay_ShowPlanes(1, 0x10);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

// The buttons of the connections, and the erase buttons of those in use
static void DWCUtilScreenAp_CreateButtons(void) {
    int i;
    int status;

    for (i = 0; i < SCREEN_AP_CONNECTIONS; i++) {
        status = DWCUtilWifiSettings_GetStatus(i);
        if (status == DWC_BM_CONNECTION_UNUSED) {
            status = 3;
        } else {
            int pos = i + SCREEN_AP_CONNECTIONS;

            sScreenAp.work->eraseButtons[i] = DWCUtilCell_AllocOamRun(0, 0x11, TRUE);
            DWCUtilOam_SetNodePosition(sScreenAp.work->eraseButtons[i], -1, sScreenApButtonPos[pos][0],
                                       sScreenApButtonPos[pos][1]);
            DWCUtilOam_SetPriority(sScreenAp.work->eraseButtons[i], -1, 3);
        }
        sScreenAp.work->buttons[i] = DWCUtilCell_AllocOamRun(0, sScreenApCells[status], TRUE);
        DWCUtilOam_SetNodePosition(sScreenAp.work->buttons[i], -1, sScreenApButtonPos[i][0], sScreenApButtonPos[i][1]);
        DWCUtilOam_SetPriority(sScreenAp.work->buttons[i], -1, 3);
    }
}

static void DWCUtilScreenAp_FadeIn(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x14, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x14);
    DWCUtilMain_SetUpdate(DWCUtilScreenAp_WaitFadeIn);
}

static void DWCUtilScreenAp_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(0);
        DWCUtilMain_SetUpdate(DWCUtilScreenAp_WaitButtons);
    }
}

static void DWCUtilScreenAp_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2 && DWCUtilSubScreen_IsSliding() != TRUE) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilScreenAp_Main);
    }
}

static void DWCUtilScreenAp_Main(void) {
    DWCUtilScreenAp_HandleInput();
    func_ov182_021b5648();
    DWCUtilScreenAp_HandleChoice();
}

static void DWCUtilScreenAp_HandleInput(void) {
    u32 i;

    for (i = 0; i < SCREEN_AP_BUTTONS; i++) {
        if (DWCUtilTouch_IsTriggeredIn(sScreenApTouchRects[i])) {
            DWCUtilUiObject_SetStateIfIdle(1);
            sScreenAp.cursor = i;
            DWCUtilScreenAp_DrawCursor();
            return;
        }
    }
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
    } else if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (DWCUtilTouch_IsKeyRepeated(0x40)) {
        DWCUtilScreenAp_MoveCursor(1);
    } else if (DWCUtilTouch_IsKeyRepeated(0x80)) {
        DWCUtilScreenAp_MoveCursor(3);
    } else if (DWCUtilTouch_IsKeyRepeated(0x20)) {
        DWCUtilScreenAp_MoveCursor(0);
    } else if (DWCUtilTouch_IsKeyRepeated(0x10)) {
        DWCUtilScreenAp_MoveCursor(2);
    }
}

static void func_ov182_021b5648(void) {
}

static void DWCUtilScreenAp_HandleChoice(void) {
    int index;

    switch (DWCUtilUiObject_GetState()) {
    case 0:
        switch (DWCUtilMain_GetStartScreen()) {
        case 0:
            DWCUtilSound_PlaySe(7);
            sScreenAp.work->end = SCREEN_AP_CANCELLED;
            break;
        case 1:
            DWCUtilUiObject_Disable();
            DWCUtilMain_SetUpdate(DWCUtilMainMenu_ExitUtility);
            return;
        }
        break;
    case 1:
        sScreenAp.work->end = SCREEN_AP_CHOSEN;
        if (sScreenAp.cursor >= SCREEN_AP_FIRST_ERASE) {
            index = sScreenAp.cursor - SCREEN_AP_FIRST_ERASE;
            if (DWCUtilWifiSettings_GetStatus(index) == DWC_BM_CONNECTION_UNUSED) {
                DWCUtilSound_PlaySe(9);
                DWCUtilUiObject_SetState(-1);
                return;
            }
            DWCUtilSound_PlaySe(6);
            DWCUtilDialog_Open(0x46, 0, 1, sScreenApMsgSlots[DWCUtilMain_GetLanguage()], index + 1);
            DWCUtilScreenAp_ShowErasePressed();
            DWCUtilUiObject_Disable();
            DWCUtilMain_SetUpdate(DWCUtilScreenAp_WaitEraseAnswer);
            return;
        }
        if (sScreenAp.cursor <= 2) {
            DWCUtilWifiSettings_EditConnection(sScreenAp.cursor);
        }
        DWCUtilSound_PlaySe(6);
        DWCUtilScreenAp_HighlightButton();
        break;
    default:
        return;
    }
    DWCUtilMain_SetUpdate(DWCUtilScreenAp_DisableButtons);
}

static void DWCUtilScreenAp_DisableButtons(void) {
    if (sScreenAp.work->end == SCREEN_AP_CANCELLED) {
        DWCUtilSubScreen_SlideOutRow();
    }
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilScreenAp_FadeOut);
}

static void DWCUtilScreenAp_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        if (sScreenAp.work->end == SCREEN_AP_CHOSEN &&
            (sScreenAp.cursor == SCREEN_AP_WIDE_BUTTON ||
             DWCUtilWifiSettings_GetEdit()->connection.status != DWC_BM_CONNECTION_UNUSED)) {
            DWCUtilUiObject_Exit();
        }
        DWCUtilFade_Start(3, 1, 1, 8);
        DWCUtilFade_Start(3, 0, 0x14, 8);
        DWCUtilMain_SetUpdate(DWCUtilScreenAp_WaitFadeOut);
    }
}

static void DWCUtilScreenAp_WaitFadeOut(void) {
    u32 i;

    if (DWCUtilFade_IsActive(1) != 0 || DWCUtilFade_IsActive(0) != 0) {
        return;
    }
    if (!DWCUtilSubScreen_IsNotSlidingOut() || !DWCUtilUiObject_IsIdle()) {
        return;
    }
    for (i = 0; i < SCREEN_AP_CONNECTIONS; i++) {
        if (sScreenAp.work->buttons[i] != NULL) {
            DWCUtilOam_Free(sScreenAp.work->buttons[i]);
        }
    }
    for (i = 0; i < SCREEN_AP_CONNECTIONS; i++) {
        if (sScreenAp.work->eraseButtons[i] != NULL) {
            DWCUtilOam_Free(sScreenAp.work->eraseButtons[i]);
        }
    }
    DWCUtilButtonSprites_Hide();
    DWCUtilScreen_Stop();
    DWCUtilRomResource_FreeFile(sScreenAp.work->palette);
    DWCUtilGfxResource_Load("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    DWCUtilDisplay_HidePlanes(1, 1);
    DWCUtilDisplay_HidePlanes(0, 0x14);
    if (sScreenAp.work->end == SCREEN_AP_CANCELLED) {
        DWCUtilMain_SetLayout(0, 0);
        DWCUtilMain_SetUpdate(DWCUtilMainMenu_Init);
    } else {
        switch (sScreenAp.cursor) {
        case 0:
        case 1:
        case 2:
            DWCUtilMain_SetLayout(2, 0);
            if (DWCUtilWifiSettings_GetEdit()->connection.status == DWC_BM_CONNECTION_UNUSED) {
                DWCUtilMain_SetUpdate(DWCUtilScreenWay_Init);
            } else {
                DWCUtilMain_SetScreenArgs(0, 0);
                DWCUtilAddressSettings_SetFrom(0);
                DWCUtilMain_SetUpdate(DWCUtilAddressSettings_Init);
            }
            break;
        case SCREEN_AP_WIDE_BUTTON:
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(func_ov182_021b688c);
            break;
        }
    }
    DWCUtilHeap_FreeAndNull(&sScreenAp.work);
}

// Frames the button under the cursor: the buttons of the top row and the wide button with the four corners, an erase
// button with the pair of sprites
static void DWCUtilScreenAp_DrawCursor(void) {
    if (sScreenAp.cursor < SCREEN_AP_FIRST_ERASE) {
        DWCUtilButtonSprites_ShowFour(
            sScreenApCursorRects[sScreenAp.cursor][0], sScreenApCursorRects[sScreenAp.cursor][2],
            sScreenApCursorRects[sScreenAp.cursor][1], sScreenApCursorRects[sScreenAp.cursor][3]);
        return;
    }
    DWCUtilButtonSprites_ShowPair(0, sScreenApCursorRects[sScreenAp.cursor][0],
                                  sScreenApCursorRects[sScreenAp.cursor][2], sScreenApCursorRects[sScreenAp.cursor][1]);
}

// Moves the cursor left, up, right or down (0 to 3)
static void DWCUtilScreenAp_MoveCursor(int direction) {
    BOOL moved = TRUE;

    switch (sScreenAp.cursor) {
    case 0:
        sScreenAp.work->column = 0;
        if (direction == 0) {
            sScreenAp.cursor = 2;
        } else if (direction == 2) {
            sScreenAp.cursor = 1;
        } else if (direction == 1) {
            sScreenAp.cursor = 3;
        } else {
            sScreenAp.cursor = 4;
        }
        break;
    case 1:
        sScreenAp.work->column = 1;
        if (direction == 0) {
            sScreenAp.cursor = 0;
        } else if (direction == 2) {
            sScreenAp.cursor = 2;
        } else if (direction == 1) {
            sScreenAp.cursor = 3;
        } else {
            sScreenAp.cursor = 5;
        }
        break;
    case 2:
        sScreenAp.work->column = 2;
        if (direction == 0) {
            sScreenAp.cursor = 1;
        } else if (direction == 2) {
            sScreenAp.cursor = 0;
        } else if (direction == 1) {
            sScreenAp.cursor = 3;
        } else {
            sScreenAp.cursor = 6;
        }
        break;
    case 3: {
        u8 column = sScreenAp.work->column;
        int erase = column + SCREEN_AP_FIRST_ERASE;

        if (direction == 1) {
            sScreenAp.cursor = erase;
        } else if (direction == 3) {
            sScreenAp.cursor = column;
        } else {
            moved = FALSE;
        }
        break;
    }
    case 4:
        sScreenAp.work->column = 0;
        if (direction == 0) {
            sScreenAp.cursor = 6;
        } else if (direction == 2) {
            sScreenAp.cursor = 5;
        } else if (direction == 1) {
            sScreenAp.cursor = 0;
        } else {
            sScreenAp.cursor = 3;
        }
        break;
    case 5:
        sScreenAp.work->column = 1;
        if (direction == 0) {
            sScreenAp.cursor = 4;
        } else if (direction == 2) {
            sScreenAp.cursor = 6;
        } else if (direction == 1) {
            sScreenAp.cursor = 1;
        } else {
            sScreenAp.cursor = 3;
        }
        break;
    case 6:
        sScreenAp.work->column = 2;
        if (direction == 0) {
            sScreenAp.cursor = 5;
        } else if (direction == 2) {
            sScreenAp.cursor = 4;
        } else if (direction == 1) {
            sScreenAp.cursor = 2;
        } else {
            sScreenAp.cursor = 3;
        }
        break;
    }
    if (moved) {
        DWCUtilSound_PlaySe(8);
        DWCUtilScreenAp_DrawCursor();
    }
}

// Shows the chosen erase button pressed
static void DWCUtilScreenAp_ShowErasePressed(void) {
    int index = sScreenAp.cursor - SCREEN_AP_FIRST_ERASE;

    DWCUtilCell_Copy(0, 0x32, DWCUtilOam_GetEntry(sScreenAp.work->eraseButtons[index], 0));
    DWCUtilOam_SetNodePosition(sScreenAp.work->eraseButtons[index], -1, sScreenApButtonPos[index + 3][0],
                               sScreenApButtonPos[index + 3][1]);
    DWCUtilOam_SetPriority(sScreenAp.work->eraseButtons[index], -1, 3);
}

// The answer to whether to erase the connection: yes erases it and shows its button as not in use, no shows the erase
// button again
static void DWCUtilScreenAp_WaitEraseAnswer(void) {
    int index = sScreenAp.cursor - SCREEN_AP_FIRST_ERASE;

    switch (DWCUtilDialog_GetChoice()) {
    case 1:
        DWCUtilSound_PlaySe(0xe);
        DWCUtilWifiSettings_Erase(index);
        DWCUtilCell_Copy(0, 0x56, DWCUtilOam_GetEntry(sScreenAp.work->buttons[index], 0));
        DWCUtilOam_SetNodePosition(sScreenAp.work->buttons[index], -1, sScreenApButtonPos[index][0],
                                   sScreenApButtonPos[index][1]);
        DWCUtilOam_SetPriority(sScreenAp.work->buttons[index], -1, 3);
        DWCUtilOam_Free(sScreenAp.work->eraseButtons[index]);
        sScreenAp.work->eraseButtons[index] = NULL;
        break;
    case 0:
        DWCUtilSound_PlaySe(7);
        DWCUtilCell_Copy(0, 0x11, DWCUtilOam_GetEntry(sScreenAp.work->eraseButtons[index], 0));
        DWCUtilOam_SetNodePosition(sScreenAp.work->eraseButtons[index], -1, sScreenApButtonPos[index + 3][0],
                                   sScreenApButtonPos[index + 3][1]);
        DWCUtilOam_SetPriority(sScreenAp.work->eraseButtons[index], -1, 3);
        break;
    default:
        return;
    }
    DWCUtilDialog_Close();
    DWCUtilMain_SetUpdate(DWCUtilScreenAp_WaitDialogClosed);
}

static void DWCUtilScreenAp_WaitDialogClosed(void) {
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilScreenAp_Main);
    }
}

// Lights up the chosen button: the palette row of its place, and the button's own palette by the connection's status
static void DWCUtilScreenAp_HighlightButton(void) {
    u8 rows[4] = { 3, 4, 5, 7 };
    u8 palettes[4] = { 10, 9, 11, 6 };
    int status;

    if (sScreenAp.cursor <= SCREEN_AP_WIDE_BUTTON) {
        DWCUtilBgBuffer_LoadPaletteRow(sScreenAp.work->palette, rows[sScreenAp.cursor], rows[sScreenAp.cursor]);
        if (sScreenAp.cursor != SCREEN_AP_WIDE_BUTTON) {
            status = DWCUtilWifiSettings_GetStatus(sScreenAp.cursor);
            if (status > 2) {
                status = 3;
            }
            DWCUtilOam_SetMode(sScreenAp.work->buttons[sScreenAp.cursor], -1, 0, palettes[status]);
        }
    }
}
