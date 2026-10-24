#include "dwc_util_address_settings.h"
#include "dwc/bm.h"
#include "dwc/ext.h"
#include "dwc_util_address_edit.h"
#include "dwc_util_auto_connect.h"
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
#include "dwc_util_oam_attr.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_save_settings.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_ap.h"
#include "dwc_util_screen_way.h"
#include "dwc_util_scrollbar.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_string.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_wep_keyboard.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The rows of the list, which the scroll shows four of, and the height of a row in pixels
enum {
    ADDRESS_ROW_SSID,
    ADDRESS_ROW_WEP_KEY,
    ADDRESS_ROW_AUTO_ADDRESS,
    ADDRESS_ROW_IP,
    ADDRESS_ROW_NETMASK,
    ADDRESS_ROW_GATEWAY,
    ADDRESS_ROW_AUTO_DNS,
    ADDRESS_ROW_DNS1,
    ADDRESS_ROW_DNS2,
};
#define ADDRESS_ROW_HEIGHT 29
// The scroll of the list at its end, in pixels, and the travel of the scroll bar's handle
#define ADDRESS_SCROLL_MAX 0x91
#define ADDRESS_BAR_LENGTH 0x37

// What the cursor or a touch is on: a row, one of the two answers of a row of DHCP, or a button
enum {
    ADDRESS_ITEM_SSID,
    ADDRESS_ITEM_WEP_KEY,
    ADDRESS_ITEM_AUTO_ADDRESS_YES,
    ADDRESS_ITEM_AUTO_ADDRESS_NO,
    ADDRESS_ITEM_IP,
    ADDRESS_ITEM_NETMASK,
    ADDRESS_ITEM_GATEWAY,
    ADDRESS_ITEM_AUTO_DNS_YES,
    ADDRESS_ITEM_AUTO_DNS_NO,
    ADDRESS_ITEM_DNS1,
    ADDRESS_ITEM_DNS2,
    ADDRESS_ITEM_TEST,
    ADDRESS_ITEM_SAVE,
    ADDRESS_ITEM_BACK,
    ADDRESS_ITEM_NONE,
};

// A rectangle of the touch screen, as dwc_util_touch.c takes it: its left, top, width and height, or its left, top,
// right and bottom, in a struct so that it can be copied and moved down by a row
typedef struct DWCUtilAddressRect {
    u16 coords[4];
} DWCUtilAddressRect;

typedef struct DWCUtilAddressSettings {
    DWCUtilTask *task;
    // The frames that each answer of the two rows of DHCP stays lit after a touch
    u8 lit[4];
    // The screen behind the rows, the palette, and the palette whose rows light up the buttons
    u16 *backScreen;
    void *palettes[2];
    DWCUtilText *text;
    // The marks of the rows' answers and arrows, and the icon of a connection that a setup made
    GXOamAttr *marks[7];
    GXOamAttr *setupIcon;
    // The task that scrolls the list by a row, and the VBlank task that scrolls the BGs
    DWCUtilTask *scrollTask;
    DWCUtilTask *vblankTask;
    // What was chosen, and the button the cursor was on before it went up to the list
    u8 chosen;
    u8 dragDelay;
    u8 lastButton;
    // The choice of DHCP for the DNS servers, which comes back when DHCP gives the address again
    u8 savedAutoDns;
    u8 moved;
    u8 dragging;
    // Whether the end of the list has played its sound, for the scroll bar's arrows and for the buttons
    u8 barAtEnd;
    u8 atEnd;
} DWCUtilAddressSettings;

// The cursor, where the settings were opened from, the cursor's row on the screen and the scroll of the list in
// pixels, which stay for the next visit, and the screen's work
typedef struct DWCUtilAddressSettingsState {
    u8 cursor;
    u8 from;
    u8 row;
    u16 scroll;
    DWCUtilAddressSettings *work;
} DWCUtilAddressSettingsState;

static void DWCUtilAddressSettings_LoadGraphics(void);
static void DWCUtilAddressSettings_InitCursor(void);
static void DWCUtilAddressSettings_FadeIn(void);
static void DWCUtilAddressSettings_WaitFadeIn(void);
static void DWCUtilAddressSettings_Main(void);
static void DWCUtilAddressSettings_HandleInput(void);
static BOOL DWCUtilAddressSettings_HandleTouch(void);
static void DWCUtilAddressSettings_ChooseItem(int item);
static void DWCUtilAddressSettings_SetAutoAddress(BOOL autoAddress);
static void DWCUtilAddressSettings_SetAutoDns(BOOL autoDns);
static void DWCUtilAddressSettings_HandleScrollbar(void);
static void DWCUtilAddressSettings_StartExit(void);
static void DWCUtilAddressSettings_FadeOut(void);
static void DWCUtilAddressSettings_WaitFadeOut(void);
static void DWCUtilAddressSettings_DrawRows(void);
static void DWCUtilAddressSettings_DrawRowText(int index, int row);
static void DWCUtilAddressSettings_DrawRowMarks(int index, int row);
static BOOL DWCUtilAddressSettings_IsItemEnabled(int item);
static void DWCUtilAddressSettings_ShowMark(int mark, int frame, int row);
static void DWCUtilAddressSettings_DrawString(const u8 *str, int row);
static void DWCUtilAddressSettings_DrawAddress(const u8 *address, int row);
static void setOffset(void);
static int isPush(void);
static int calcColumnIdx(int index);
static int calcIdxColumn(int row);
static void DWCUtilAddressSettings_ScrollUpTask(DWCUtilTask *task, void *arg);
static void DWCUtilAddressSettings_ScrollDownTask(DWCUtilTask *task, void *arg);
static void DWCUtilAddressSettings_ScrollBgTask(DWCUtilTask *task, void *arg);
static void DWCUtilAddressSettings_ShowCursor(void);
static void applyForcus(void);
static void applyForcus1(int item);
static void DWCUtilAddressSettings_MoveCursor(int direction);
static void DWCUtilAddressSettings_LightButton(void);
static void DWCUtilAddressSettings_UpdateLitTask(DWCUtilTask *task, void *arg);
static void pushButtonYesNo(void);
static BOOL DWCUtilAddressSettings_IsValid(void);
static void DWCUtilAddressSettings_WaitInvalidDialog(void);
static void DWCUtilAddressSettings_WaitInvalidClosed(void);

static DWCUtilAddressSettingsState sAddressSettings;

// The palette rows of the three buttons
static const u8 sAddressButtonPalettes[3] = { 6, 8, 7 };
// Where a row is touched: its field, and the "No" and "Yes" answers of a row of DHCP
static const DWCUtilAddressRect sAddressFieldRect = { { 0xcc, 0x34, 0x1c, 0x18 } };
static const DWCUtilAddressRect sAddressNoRect = { { 0x8f, 0x34, 0x2c, 0x18 } };
static const DWCUtilAddressRect sAddressYesRect = { { 0xc0, 0x34, 0x2c, 0x18 } };
// The item of each row, ADDRESS_ITEM_NONE for a row of DHCP, whose answers are the items
static const u8 sAddressRowItems[9] = {
    ADDRESS_ITEM_SSID,    ADDRESS_ITEM_WEP_KEY, ADDRESS_ITEM_NONE, ADDRESS_ITEM_IP,   ADDRESS_ITEM_NETMASK,
    ADDRESS_ITEM_GATEWAY, ADDRESS_ITEM_NONE,    ADDRESS_ITEM_DNS1, ADDRESS_ITEM_DNS2,
};
// How far down the BG of the rows' labels is, by the top row
static const u8 sAddressLabelOffsets[9] = { 0, 5, 2, 7, 4, 1, 6, 3, 0 };
// The title's label by language
static const s8 sAddressTitleLabels[7] = { -1, 0x23, 0x27, -1, 0x23, 0x2f, -1 };
// The row of each item
static const u8 sAddressItemRows[11] = { 0, 1, 2, 2, 3, 4, 5, 6, 6, 7, 8 };
// The cells of the marks: none, an arrow, the lit, the unlit and the greyed answers
static const u8 sAddressMarkCells[5][3] = {
    { 0, 0x29, 0x2c }, { 0x52, 0x53, 0x30 }, { 0, 0x2a, 0x30 }, { 0x54, 0x55, 0 }, { 0, 0x2b, 0 },
};
// Where the screen behind the rows starts, in its entries, by the top row
static const u16 sAddressScreenOffsets[9] = { 0, 0x60, 0xe0, 0x140, 0x1c0, 0x240, 0x2a0, 0x320, 0x3a0 };
// Where the buttons are touched: left, top, right and bottom
static const DWCUtilAddressRect sAddressButtonRects[3] = {
    { { 0x84, 0x1b, 0xfc, 0x2c } },
    { { 0x84, 0xac, 0xfc, 0xbd } },
    { { 0x04, 0xac, 0x7c, 0xbd } },
};
// Where the cursor frames the row's field, the two answers and the three buttons: left, top, right and bottom
static const DWCUtilAddressRect sAddressCursorRects[6] = {
    { { 0xc8, 0x31, 0xe0, 0x4d } }, { { 0xbc, 0x31, 0xe0, 0x4d } }, { { 0x8b, 0x31, 0xaf, 0x4d } },
    { { 0x82, 0x18, 0xee, 0x2c } }, { { 0x82, 0xa9, 0xee, 0xbd } }, { { 0x02, 0xa9, 0x6e, 0xbd } },
};

void DWCUtilAddressSettings_Init(void) {
    DWCUtilSettingsEdit *edit = DWCUtilWifiSettings_GetEdit();
    int i;

    sAddressSettings.work = DWCi_HEAPlAllocEx(sizeof(DWCUtilAddressSettings), 4);
    sAddressSettings.work->savedAutoDns = edit->autoDns;
    sAddressSettings.work->lastButton = ADDRESS_ITEM_SAVE;
    DWCUtilAddressSettings_InitCursor();
    DWCUtilAddressSettings_LoadGraphics();
    if (sAddressSettings.from == 0) {
        DWCi_COMMENTlDispEx(0x30, sAddressTitleLabels[DWCi_Language()], edit->index + 1);
    } else {
        DWCi_COMMENTlDispEx(0x45, -1, 0);
    }
    DWCi_CMNlStep(1);
    DWCi_HLlLoadList();
    DWCUtilScrollbar_Create(2, 0x55, 0xf1, 0x41, sAddressSettings.scroll * ADDRESS_BAR_LENGTH / ADDRESS_SCROLL_MAX);
    sAddressSettings.work->text = DWCi_FNTlFormBgEx(0, 1);
    for (i = 0; i < 7; i++) {
        sAddressSettings.work->marks[i] = DWCUtilCell_AllocOam(0, 0x29);
    }
    switch (edit->connection.setType) {
    case 1:
        sAddressSettings.work->setupIcon = DWCUtilCell_AllocOam(0, 0x50);
        G2_SetOBJPriority(sAddressSettings.work->setupIcon, 3);
        break;
    case 2:
        sAddressSettings.work->setupIcon = DWCUtilCell_AllocOam(0, 0x51);
        G2_SetOBJPriority(sAddressSettings.work->setupIcon, 3);
        break;
    }
    sAddressSettings.work->vblankTask = DWCi_TSKlForm(1, DWCUtilAddressSettings_ScrollBgTask, NULL, 0x6e);
    sAddressSettings.work->task = DWCi_TSKlForm(0, DWCUtilAddressSettings_UpdateLitTask, NULL, 0x78);
    DWCUtilAddressSettings_DrawRows();
    DWCUtilAddressSettings_ShowCursor();
    DWCi_ChangeScene(DWCUtilAddressSettings_FadeIn);
}

static void DWCUtilAddressSettings_LoadGraphics(void) {
    char backPath[] = "char/jb3ListBack.nsc.l";
    char palettePath[] = "char/ybBgStep2.ncl.l";
    char litPalettePath[] = "char/ybBgStep21.ncl.l";

    G2_SetBG2ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x7000, GX_BG_CHARBASE(0x18000));
    DWCUtilGfxResource_Load("char/ybObjKb.ncl.l", GX_LoadOBJPltt);
    DWCUtilGfxResource_Load("char/jbBgStep2.ncg.l", GX_LoadBG3Char);
    DWCUtilGfxResource_Load("char/jbBgStep21.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load(palettePath, GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jb3List.nsc.l", GX_LoadBG3Scr);
    sAddressSettings.work->backScreen = DWCUtilRomResource_LoadFile(DWCi_Char(backPath), NULL, 4);
    DWCi_SCREENlInit(sAddressSettings.work->backScreen);
    DWCi_SCREENlRenew();
    sAddressSettings.work->palettes[0] = DWCUtilRomResource_LoadFile(DWCi_Char(palettePath), NULL, 4);
    sAddressSettings.work->palettes[1] = DWCUtilRomResource_LoadFile(DWCi_Char(litPalettePath), NULL, 4);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(2);
    G2_SetBG2Priority(3);
    G2_SetBG3Priority(2);
}

// The cursor and the scroll start at the top unless the screen comes back from a row's keyboard or keypad
static void DWCUtilAddressSettings_InitCursor(void) {
    int again;

    DWCUtilWifiSettings_GetEdit();
    DWCi_GetParam2_(NULL, &again);
    if (again == 0) {
        sAddressSettings.scroll = 0;
        sAddressSettings.cursor = 0;
        sAddressSettings.row = 0;
    }
}

static void DWCUtilAddressSettings_FadeIn(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0,
                  GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(DWCUtilAddressSettings_WaitFadeIn);
}

static void DWCUtilAddressSettings_WaitFadeIn(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_ChangeScene(DWCUtilAddressSettings_Main);
    }
}

static void DWCUtilAddressSettings_Main(void) {
    DWCUtilAddressSettings_HandleInput();
    DWCUtilAddressSettings_HandleScrollbar();
}

static void DWCUtilAddressSettings_HandleInput(void) {
    if (sAddressSettings.work->scrollTask != NULL || sAddressSettings.work->dragging ||
        DWCUtilAddressSettings_HandleTouch()) {
        return;
    }
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCUtilAddressSettings_ChooseItem(sAddressSettings.cursor);
        return;
    }
    if (DWCi_IPTlCheckTrigger(2)) {
        DWCUtilSound_PlaySe(7);
        sAddressSettings.work->chosen = ADDRESS_ITEM_BACK;
        DWCi_ChangeScene(DWCUtilAddressSettings_StartExit);
        return;
    }
    if (DWCi_IPTlCheckRepeat(0x200)) {
        if (sAddressSettings.scroll == ADDRESS_SCROLL_MAX) {
            if (!sAddressSettings.work->atEnd) {
                DWCUtilSound_PlaySe(9);
                sAddressSettings.work->atEnd = TRUE;
            }
            return;
        }
        DWCUtilSound_PlaySe(0x13);
        sAddressSettings.work->scrollTask = DWCi_TSKlForm(0, DWCUtilAddressSettings_ScrollDownTask, NULL, 0x78);
        return;
    }
    if (DWCi_IPTlCheckRelease(0x200)) {
        sAddressSettings.work->atEnd = FALSE;
        return;
    }
    if (DWCi_IPTlCheckRepeat(0x100)) {
        if (sAddressSettings.scroll == 0) {
            if (!sAddressSettings.work->atEnd) {
                DWCUtilSound_PlaySe(9);
                sAddressSettings.work->atEnd = TRUE;
            }
            return;
        }
        DWCUtilSound_PlaySe(0x13);
        sAddressSettings.work->scrollTask = DWCi_TSKlForm(0, DWCUtilAddressSettings_ScrollUpTask, NULL, 0x78);
        return;
    }
    if (DWCi_IPTlCheckRelease(0x100)) {
        sAddressSettings.work->atEnd = FALSE;
        return;
    }
    if (DWCi_IPTlCheckRepeat(0x40)) {
        DWCUtilAddressSettings_MoveCursor(1);
        return;
    }
    if (DWCi_IPTlCheckRelease(0x40)) {
        sAddressSettings.work->atEnd = FALSE;
        return;
    }
    if (DWCi_IPTlCheckRepeat(0x80)) {
        DWCUtilAddressSettings_MoveCursor(3);
        return;
    }
    if (DWCi_IPTlCheckRelease(0x80)) {
        sAddressSettings.work->atEnd = FALSE;
        return;
    }
    if (DWCi_IPTlCheckRepeat(0x20)) {
        DWCUtilAddressSettings_MoveCursor(0);
        return;
    }
    if (DWCi_IPTlCheckRepeat(0x10)) {
        DWCUtilAddressSettings_MoveCursor(2);
    }
}

// A touch of a row or a button: TRUE when the touch is on one
static BOOL DWCUtilAddressSettings_HandleTouch(void) {
    int item;

    DWCUtilWifiSettings_GetEdit();
    item = isPush();
    if (item == ADDRESS_ITEM_NONE) {
        return FALSE;
    }
    if (!DWCUtilAddressSettings_IsItemEnabled(item)) {
        DWCUtilSound_PlaySe(9);
        return TRUE;
    }
    applyForcus1(item);
    switch (item) {
    case ADDRESS_ITEM_AUTO_ADDRESS_YES:
    case ADDRESS_ITEM_AUTO_ADDRESS_NO:
    case ADDRESS_ITEM_AUTO_DNS_YES:
    case ADDRESS_ITEM_AUTO_DNS_NO:
        pushButtonYesNo();
    }
    DWCUtilAddressSettings_ChooseItem(item);
    return TRUE;
}

// Chooses an item: an answer of DHCP takes effect, a row opens its keyboard or keypad, a button tests, saves or goes
// back
static void DWCUtilAddressSettings_ChooseItem(int item) {
    if (!DWCUtilAddressSettings_IsItemEnabled(item)) {
        DWCUtilSound_PlaySe(9);
        return;
    }
    // The answers of a row of DHCP are yes (0) and no (1), the one chosen turns DHCP on when it is not no
    switch (item) {
    case ADDRESS_ITEM_AUTO_ADDRESS_YES:
    case ADDRESS_ITEM_AUTO_ADDRESS_NO:
        DWCUtilSound_PlaySe(6);
        DWCUtilAddressSettings_SetAutoAddress(((item - ADDRESS_ITEM_AUTO_ADDRESS_YES) ^ 1) != 0);
        return;
    case ADDRESS_ITEM_AUTO_DNS_YES:
    case ADDRESS_ITEM_AUTO_DNS_NO:
        DWCUtilSound_PlaySe(6);
        DWCUtilAddressSettings_SetAutoDns(((item - ADDRESS_ITEM_AUTO_DNS_YES) ^ 1) != 0);
        return;
    }
    sAddressSettings.work->chosen = item;
    if (item == ADDRESS_ITEM_TEST || item == ADDRESS_ITEM_SAVE) {
        DWCUtilAddressSettings_LightButton();
        if (!DWCUtilAddressSettings_IsValid()) {
            DWCUtilSound_PlaySe(9);
            DWCUtilDialog_Open(6, 1, 1, -1, 0);
            DWCi_ChangeScene(DWCUtilAddressSettings_WaitInvalidDialog);
            return;
        }
        if (item == ADDRESS_ITEM_TEST) {
            DWCUtilSound_PlaySe(6);
        } else {
            DWCUtilSound_PlaySe(0xe);
        }
    } else if (item == ADDRESS_ITEM_BACK) {
        DWCUtilAddressSettings_LightButton();
        DWCUtilSound_PlaySe(7);
    } else {
        DWCUtilSound_PlaySe(6);
        DWCUtilAddressSettings_ShowMark(0, 1, calcColumnIdx(sAddressItemRows[item]));
        setOffset();
    }
    DWCi_SBlDisable();
    DWCi_ChangeScene(DWCUtilAddressSettings_StartExit);
}

// DHCP for the address: without it, DHCP can't give the DNS servers either; with it, they come from DHCP again if
// they did
static void DWCUtilAddressSettings_SetAutoAddress(BOOL autoAddress) {
    DWCUtilSettingsEdit *edit = DWCUtilWifiSettings_GetEdit();
    BOOL autoDns;

    if (edit->autoAddress == autoAddress) {
        return;
    }
    if (autoAddress) {
        autoDns = sAddressSettings.work->savedAutoDns != 0;
    } else {
        sAddressSettings.work->savedAutoDns = edit->autoDns;
        autoDns = FALSE;
    }
    DWCi_SETTINGlSetIpAuto(autoAddress);
    DWCi_SETTINGlSetDnsAuto(autoDns);
    DWCUtilAddressSettings_DrawRows();
}

static void DWCUtilAddressSettings_SetAutoDns(BOOL autoDns) {
    DWCUtilSettingsEdit *edit = DWCUtilWifiSettings_GetEdit();

    if (edit->autoDns != autoDns) {
        sAddressSettings.work->savedAutoDns = autoDns;
        DWCi_SETTINGlSetDnsAuto(autoDns);
        DWCUtilAddressSettings_DrawRows();
    }
}

// The scroll bar: its handle dragged, or its arrows
static void DWCUtilAddressSettings_HandleScrollbar(void) {
    int offset;

    if (sAddressSettings.work->scrollTask != NULL) {
        return;
    }
    if (sAddressSettings.work->dragDelay != 0) {
        sAddressSettings.work->dragDelay--;
    }
    switch (DWCi_SBlGetState()) {
    case 1:
        sAddressSettings.work->dragging = TRUE;
        break;
    case 2:
        if (sAddressSettings.work->dragDelay == 0) {
            DWCi_FOCUSlDelete();
            sAddressSettings.scroll = DWCi_SBlGet() * ADDRESS_SCROLL_MAX / ADDRESS_BAR_LENGTH;
            DWCUtilAddressSettings_DrawRows();
            sAddressSettings.work->dragDelay = 4;
        }
        break;
    case 3:
        sAddressSettings.work->dragging = FALSE;
        sAddressSettings.scroll = DWCi_SBlGet() * ADDRESS_SCROLL_MAX / ADDRESS_BAR_LENGTH;
        DWCUtilSound_PlaySe(0x13);
        DWCUtilAddressSettings_DrawRows();
        offset = FX_ModS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
        if (offset == 0) {
            applyForcus();
        } else if (offset < 16) {
            sAddressSettings.work->scrollTask = DWCi_TSKlForm(0, DWCUtilAddressSettings_ScrollUpTask, NULL, 0x78);
        } else {
            sAddressSettings.work->scrollTask = DWCi_TSKlForm(0, DWCUtilAddressSettings_ScrollDownTask, NULL, 0x78);
        }
        break;
    case 4:
        if (sAddressSettings.scroll == 0) {
            if (!sAddressSettings.work->barAtEnd) {
                DWCUtilSound_PlaySe(9);
                sAddressSettings.work->barAtEnd = TRUE;
            }
        } else {
            DWCUtilSound_PlaySe(0x13);
            sAddressSettings.work->scrollTask = DWCi_TSKlForm(0, DWCUtilAddressSettings_ScrollUpTask, NULL, 0x78);
        }
        break;
    case 6:
        if (sAddressSettings.scroll == ADDRESS_SCROLL_MAX) {
            if (!sAddressSettings.work->barAtEnd) {
                DWCUtilSound_PlaySe(9);
                sAddressSettings.work->barAtEnd = TRUE;
            }
        } else {
            DWCUtilSound_PlaySe(0x13);
            sAddressSettings.work->scrollTask = DWCi_TSKlForm(0, DWCUtilAddressSettings_ScrollDownTask, NULL, 0x78);
        }
        break;
    case 5:
    case 7:
        sAddressSettings.work->barAtEnd = FALSE;
        break;
    }
}

static void DWCUtilAddressSettings_StartExit(void) {
    DWCi_SBlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(DWCUtilAddressSettings_FadeOut);
}

static void DWCUtilAddressSettings_FadeOut(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ,
                      8);
        DWCi_ChangeScene(DWCUtilAddressSettings_WaitFadeOut);
    }
}

// Frees the screen and goes on to what was chosen
static void DWCUtilAddressSettings_WaitFadeOut(void) {
    int i;
    DWCUtilSettingsEdit *edit;
    void (*next)(void);

    if (DWCi_EFFlCheck(1) != 0 || DWCi_EFFlCheck(0) != 0) {
        return;
    }
    DWCi_TSKlDeleteEx(0, sAddressSettings.work->task);
    DWCi_TSKlDeleteEx(1, sAddressSettings.work->vblankTask);
    for (i = 0; i < 7; i++) {
        DWCi_OBJlDelete(sAddressSettings.work->marks[i]);
    }
    if (sAddressSettings.work->setupIcon != NULL) {
        DWCi_OBJlDelete(sAddressSettings.work->setupIcon);
    }
    DWCi_SBlEnd();
    DWCi_FNTlDeleteBgLcd(0);
    DWCi_FOCUSlDelete();
    if (sAddressSettings.work->chosen != ADDRESS_ITEM_SAVE) {
        DWCi_COMMENTlDelete();
    }
    DWCi_SCREENlEnd();
    DWCi_ARClRelease(sAddressSettings.work->backScreen);
    for (i = 0; i < 2; i++) {
        DWCi_ARClRelease(sAddressSettings.work->palettes[i]);
    }
    DWCUtilGfxResource_Load("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
    DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ);
    G2_SetBG0Offset(0, 0);
    G2_SetBG2Offset(0, 0);
    G2_SetBG2ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x7000, GX_BG_CHARBASE_0x10000);
    switch (sAddressSettings.work->chosen) {
    case ADDRESS_ITEM_SSID:
    case ADDRESS_ITEM_WEP_KEY:
        DWCi_SetParam2_(sAddressSettings.work->chosen, 0);
        DWCi_SetParam_(2, 0);
        next = DWCUtilWepKeyboard_Init;
        break;
    case ADDRESS_ITEM_IP:
    case ADDRESS_ITEM_NETMASK:
    case ADDRESS_ITEM_GATEWAY:
    case ADDRESS_ITEM_DNS1:
    case ADDRESS_ITEM_DNS2:
        i = sAddressSettings.work->chosen - ADDRESS_ITEM_IP;
        if (sAddressSettings.work->chosen >= ADDRESS_ITEM_DNS1) {
            i -= 2;
        }
        DWCi_SetParam_(2, 0);
        DWCi_SetParam2_(i, 0);
        next = DWCUtilAddressEdit_Init;
        break;
    case ADDRESS_ITEM_TEST:
        edit = DWCUtilWifiSettings_GetEdit();
        edit->connection.netmask = DWCi_BACKUPlConvMaskCidr(edit->netmask);
        if (edit->autoAddress) {
            MI_CpuFill8(edit->connection.ip, 0, sizeof(edit->connection.ip));
            MI_CpuFill8(edit->connection.gateway, 0, sizeof(edit->connection.gateway));
            MI_CpuFill8(edit->netmask, 0, sizeof(edit->netmask));
            edit->connection.netmask = 0;
        }
        if (edit->autoDns) {
            MI_CpuFill8(edit->connection.dns, 0, sizeof(edit->connection.dns));
        }
        DWCi_SetParam_(2, 0);
        DWCi_SetParam2_(0, 0);
        next = DWCUtilAutoConnect_Init;
        break;
    case ADDRESS_ITEM_SAVE:
        DWCi_SetParam_(0, 0);
        next = DWCUtilSaveSettings_Init;
        break;
    case ADDRESS_ITEM_BACK:
        if (sAddressSettings.from == 0) {
            DWCi_SetParam_(2, 1);
            next = DWCUtilScreenAp_Init;
        } else {
            edit = DWCUtilWifiSettings_GetEdit();
            DWCi_SETTINGlSetSetting(edit->index);
            DWCi_SetParam_(0, 1);
            next = DWCUtilScreenWay_Init;
        }
        break;
    default:
        goto end;
    }
    DWCi_ChangeScene(next);
end:
    DWCi_HEAPlFree_(&sAddressSettings.work);
}

// Draws the rows on the screen from the scroll
static void DWCUtilAddressSettings_DrawRows(void) {
    int i;
    int top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    int index;

    DWCi_FNTlClear(sAddressSettings.work->text, 0);
    for (i = 0, index = top; i < 5; i++, index++) {
        DWCUtilAddressSettings_DrawRowText(index, i);
    }
    DWCi_SCREENlCopy(sAddressSettings.work->backScreen + sAddressScreenOffsets[top], 0, 0x1e, 0x13);
    for (i = 0; i < 5; i++, top++) {
        DWCUtilAddressSettings_DrawRowMarks(top, i);
    }
    DWCi_SCREENlRenew();
    DWCUtilText_MarkDirty(sAddressSettings.work->text);
    setOffset();
}

// The text of a row: the SSID, the WEP key as stars, or an address of the connection's own
static void DWCUtilAddressSettings_DrawRowText(int index, int row) {
    DWCUtilSettingsEdit *edit = DWCUtilWifiSettings_GetEdit();
    u8 stars[33];
    int length;

    switch (index) {
    case ADDRESS_ROW_SSID:
        DWCUtilAddressSettings_DrawString(edit->connection.ssid, row);
        return;
    case ADDRESS_ROW_WEP_KEY:
        switch (edit->connection.wepMode) {
        case 0:
            return;
        case 1:
            length = 10;
            break;
        case 2:
            length = 26;
            break;
        case 3:
            length = 32;
        }
        if (edit->connection.wepNotation == 1) {
            length /= 2;
        }
        MI_CpuFill8(stars, 0, sizeof(stars));
        memset(stars, '*', length);
        DWCUtilAddressSettings_DrawString(stars, row);
        return;
    case ADDRESS_ROW_IP:
        if (!edit->autoAddress) {
            DWCUtilAddressSettings_DrawAddress(edit->connection.ip, row);
        }
        break;
    case ADDRESS_ROW_NETMASK:
        if (!edit->autoAddress) {
            DWCUtilAddressSettings_DrawAddress(edit->netmask, row);
        }
        break;
    case ADDRESS_ROW_GATEWAY:
        if (!edit->autoAddress) {
            DWCUtilAddressSettings_DrawAddress(edit->connection.gateway, row);
        }
        break;
    case ADDRESS_ROW_DNS1:
        if (!edit->autoDns) {
            DWCUtilAddressSettings_DrawAddress(edit->connection.dns[0], row);
        }
        break;
    case ADDRESS_ROW_DNS2:
        if (!edit->autoDns) {
            DWCUtilAddressSettings_DrawAddress(edit->connection.dns[1], row);
        }
    }
}

// The marks of a row: its arrow, or its two answers lit as they were touched and greyed when they can't be chosen
static void DWCUtilAddressSettings_DrawRowMarks(int index, int row) {
    DWCUtilSettingsEdit *edit = DWCUtilWifiSettings_GetEdit();
    int mark;
    int frame;
    int other;
    BOOL lit;

    switch (index) {
    case ADDRESS_ROW_SSID:
    case ADDRESS_ROW_WEP_KEY:
        mark = 0;
        frame = 0;
        if (DWCUtilAddressSettings_IsItemEnabled(ADDRESS_ITEM_SSID)) {
            break;
        }
        frame = 2;
        break;
    case ADDRESS_ROW_AUTO_ADDRESS:
        lit = FALSE;
        frame = 0;
        if (edit->autoAddress) {
            mark = 1;
            other = 4;
        } else {
            mark = 2;
            other = 3;
        }
        if (sAddressSettings.work->lit[0] != 0) {
            frame = 1;
        }
        if (sAddressSettings.work->lit[1] != 0) {
            lit = TRUE;
        }
        DWCUtilAddressSettings_ShowMark(other, lit, row);
        break;
    case ADDRESS_ROW_IP:
    case ADDRESS_ROW_NETMASK:
    case ADDRESS_ROW_GATEWAY:
        mark = 0;
        if (edit->autoAddress) {
            frame = 2;
        } else {
            frame = mark;
        }
        break;
    case ADDRESS_ROW_AUTO_DNS:
        lit = FALSE;
        frame = 0;
        if (edit->autoDns) {
            mark = 1;
            other = 4;
        } else {
            if (!edit->autoAddress) {
                frame = 2;
            }
            mark = 2;
            other = 3;
        }
        if (sAddressSettings.work->lit[2] != 0) {
            frame = 1;
        }
        if (sAddressSettings.work->lit[3] != 0) {
            lit = TRUE;
        }
        DWCUtilAddressSettings_ShowMark(other, lit, row);
        break;
    case ADDRESS_ROW_DNS1:
    case ADDRESS_ROW_DNS2:
        mark = 0;
        if (edit->autoDns) {
            frame = 2;
        } else {
            frame = mark;
        }
        break;
    default:
        mark = 0;
        frame = 2;
        break;
    }
    DWCUtilAddressSettings_ShowMark(mark, frame, row);
}

// Whether an item can be chosen: not the SSID or the key of a connection that a setup made, nor what DHCP gives
static BOOL DWCUtilAddressSettings_IsItemEnabled(int item) {
    DWCUtilSettingsEdit *edit = DWCUtilWifiSettings_GetEdit();
    BOOL result = TRUE;

    switch (item) {
    case ADDRESS_ITEM_AUTO_DNS_YES:
        if (!edit->autoAddress) {
            result = FALSE;
        }
        break;
    case ADDRESS_ITEM_SSID:
    case ADDRESS_ITEM_WEP_KEY:
        if (edit->connection.setType == 1 || edit->connection.setType == 2) {
            result = FALSE;
        }
        break;
    case ADDRESS_ITEM_IP:
    case ADDRESS_ITEM_NETMASK:
    case ADDRESS_ITEM_GATEWAY:
        if (edit->autoAddress) {
            result = FALSE;
        }
        break;
    case ADDRESS_ITEM_DNS1:
    case ADDRESS_ITEM_DNS2:
        if (edit->autoDns) {
            result = FALSE;
        }
        break;
    }
    return result;
}

// Shows a mark of a row in a frame, the marks of the answers of the two rows of DHCP in their own entries
static void DWCUtilAddressSettings_ShowMark(int mark, int frame, int row) {
    u16 x[5];
    GXOamAttr **oam;

    x[0] = 0xcc;
    x[1] = 0xc0;
    x[2] = 0xc0;
    x[3] = 0x8f;
    x[4] = 0x8f;
    oam = &sAddressSettings.work->marks[row];
    if (mark == 1 || mark == 2) {
        if (calcIdxColumn(row) == ADDRESS_ROW_AUTO_ADDRESS) {
            oam = &sAddressSettings.work->marks[5];
        } else {
            oam = &sAddressSettings.work->marks[6];
        }
    }
    if (sAddressMarkCells[mark][frame] != 0) {
        DWCUtilCell_Copy(0, sAddressMarkCells[mark][frame], *oam);
        DWCUtilOam_SetPosition(*oam, x[mark], 0);
        G2_SetOBJPriority(*oam, 3);
        return;
    }
    DWCUtilOam_SetPosition(*oam, 0x100, 0);
}

// The SSID or the key of a row, on two lines when it is longer than 16 characters, a space shown as its own character
static void DWCUtilAddressSettings_DrawString(const u8 *str, int row) {
    u16 line[17];
    int length;
    int i;
    int count;
    int y;

    MI_CpuFill8(line, 0, sizeof(line));
    length = DWCi_MOV_WH_SetUserGameInfo(str, 32);
    count = length;
    if (count > 16) {
        count = 16;
    }
    for (i = 0; i < count; i++) {
        if (str[i] == ' ') {
            line[i] = 0xe01d;
        } else {
            line[i] = str[i];
        }
    }
    y = row * ADDRESS_ROW_HEIGHT + 2;
    if (length <= 16) {
        y += 5;
    }
    DWCi_FNTlDrawStringEx(sAddressSettings.work->text, 0x48, y, 2, 8, line, 1);
    if (length > 16) {
        MI_CpuFill8(line, 0, sizeof(line));
        for (i = 0; i < length - 16; i++) {
            if (str[16 + i] == ' ') {
                line[i] = 0xe01d;
            } else {
                line[i] = str[16 + i];
            }
        }
        DWCi_FNTlDrawStringEx(sAddressSettings.work->text, 0x48, y + 12, 2, 8, line, 1);
    }
}

// An address of a row, as four numbers of three places
static void DWCUtilAddressSettings_DrawAddress(const u8 *address, int row) {
    u16 str[16];
    char digits[16];

    OS_SNPrintf(digits, sizeof(digits), "%3d.%3d.%3d.%3d", address[0], address[1], address[2], address[3]);
    DWCUtilString_Widen(str, (u8 *)digits, 16);
    DWCi_FNTlDrawStringEx(sAddressSettings.work->text, 0x5f, row * ADDRESS_ROW_HEIGHT + 8, 2, 7, str, 1);
}

// Moves the marks and the setup's icon to the rows' places from the scroll, and has the BGs follow at the next VBlank
static void setOffset(void) {
    int top;
    int y;
    int markY;
    int x;
    int oamY;
    int i;

    top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    y = 0x34 - FX_ModS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    if (sAddressSettings.work->setupIcon != NULL) {
        if (top == 0) {
            x = 0x26;
        } else {
            x = 0x100;
        }
        DWCUtilOam_SetPosition(sAddressSettings.work->setupIcon, x, y);
    }
    for (i = 0, markY = y; i < 5; i++, markY += ADDRESS_ROW_HEIGHT) {
        G2_GetOBJPosition(sAddressSettings.work->marks[i], (u32 *)&x, (u32 *)&oamY);
        DWCUtilOam_SetPosition(sAddressSettings.work->marks[i], x, markY);
    }
    if (top <= ADDRESS_ROW_AUTO_ADDRESS) {
        G2_GetOBJPosition(sAddressSettings.work->marks[5], (u32 *)&x, (u32 *)&oamY);
        DWCUtilOam_SetPosition(sAddressSettings.work->marks[5], x,
                               y + (ADDRESS_ROW_AUTO_ADDRESS - top) * ADDRESS_ROW_HEIGHT);
    } else {
        DWCUtilOam_SetPosition(sAddressSettings.work->marks[5], 0x100, 0);
    }
    if (top >= ADDRESS_ROW_AUTO_ADDRESS && top <= ADDRESS_ROW_AUTO_DNS) {
        G2_GetOBJPosition(sAddressSettings.work->marks[6], (u32 *)&x, (u32 *)&oamY);
        DWCUtilOam_SetPosition(sAddressSettings.work->marks[6], x,
                               y + (ADDRESS_ROW_AUTO_DNS - top) * ADDRESS_ROW_HEIGHT);
    } else {
        DWCUtilOam_SetPosition(sAddressSettings.work->marks[6], 0x100, 0);
    }
    sAddressSettings.work->moved = TRUE;
}

// What a touch is on, ADDRESS_ITEM_NONE for nothing
static int isPush(void) {
    DWCUtilAddressRect rect;
    int top;
    int i;

    if (!DWCi_TPlCheckTrigger(gDWCUtilTouchScreenRect)) {
        return ADDRESS_ITEM_NONE;
    }
    top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    rect = sAddressFieldRect;
    for (i = 0; i < 4; i++) {
        if (top != ADDRESS_ROW_AUTO_ADDRESS && top != ADDRESS_ROW_AUTO_DNS && DWCi_TPlCheckATrigger(rect.coords)) {
            return sAddressRowItems[top];
        }
        top++;
        rect.coords[1] += ADDRESS_ROW_HEIGHT;
    }
    top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    for (i = 0; i < 4; i++) {
        if (top == ADDRESS_ROW_AUTO_ADDRESS) {
            rect = sAddressYesRect;
            rect.coords[1] += i * ADDRESS_ROW_HEIGHT;
            if (DWCi_TPlCheckATrigger(rect.coords)) {
                return ADDRESS_ITEM_AUTO_ADDRESS_YES;
            }
            rect = sAddressNoRect;
            rect.coords[1] += i * ADDRESS_ROW_HEIGHT;
            if (DWCi_TPlCheckATrigger(rect.coords)) {
                return ADDRESS_ITEM_AUTO_ADDRESS_NO;
            }
            break;
        }
        top++;
    }
    top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    for (i = 0; i < 4; i++) {
        if (top == ADDRESS_ROW_AUTO_DNS) {
            rect = sAddressYesRect;
            rect.coords[1] += i * ADDRESS_ROW_HEIGHT;
            if (DWCi_TPlCheckATrigger(rect.coords)) {
                return ADDRESS_ITEM_AUTO_DNS_YES;
            }
            rect = sAddressNoRect;
            rect.coords[1] += i * ADDRESS_ROW_HEIGHT;
            if (DWCi_TPlCheckATrigger(rect.coords)) {
                return ADDRESS_ITEM_AUTO_DNS_NO;
            }
            break;
        }
        top++;
    }
    for (i = 0; i < 3; i++) {
        if (DWCi_TPlCheckTrigger(sAddressButtonRects[i].coords)) {
            return i + ADDRESS_ITEM_TEST;
        }
    }
    return ADDRESS_ITEM_NONE;
}

// The row on the screen that a row of the list is on, -1 when it isn't shown
static int calcColumnIdx(int index) {
    int top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    int i;

    for (i = 0; i < 4; i++, top++) {
        if (top == index) {
            return i;
        }
    }
    return -1;
}

// The row of the list that a row on the screen shows
static int calcIdxColumn(int row) {
    int top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);

    return top + row;
}

// Scrolls the list up by 6 pixels a frame, until it rests on a row
static void DWCUtilAddressSettings_ScrollUpTask(DWCUtilTask *task, void *arg) {
    int offset;

    DWCi_SBlDisable();
    DWCi_FOCUSlDelete();
    if (sAddressSettings.scroll > 6) {
        sAddressSettings.scroll -= 6;
    } else {
        sAddressSettings.scroll = 0;
    }
    offset = FX_ModS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    if (offset == ADDRESS_ROW_HEIGHT - 6) {
        DWCUtilAddressSettings_DrawRows();
        return;
    }
    if (offset > ADDRESS_ROW_HEIGHT - 6) {
        sAddressSettings.scroll += ADDRESS_ROW_HEIGHT - offset;
        offset = 0;
    }
    setOffset();
    if (offset == 0) {
        DWCi_SBlSet(sAddressSettings.scroll * ADDRESS_BAR_LENGTH / ADDRESS_SCROLL_MAX);
        DWCi_SBlEnable();
        applyForcus();
        sAddressSettings.work->scrollTask = NULL;
        DWCi_TSKlDelete(0, task);
    }
}

// Scrolls the list down by 6 pixels a frame, until it rests on a row
static void DWCUtilAddressSettings_ScrollDownTask(DWCUtilTask *task, void *arg) {
    int offset;

    DWCi_SBlDisable();
    DWCi_FOCUSlDelete();
    sAddressSettings.scroll += 6;
    offset = FX_ModS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    if (offset >= 6) {
        setOffset();
        return;
    }
    sAddressSettings.scroll -= offset;
    DWCUtilAddressSettings_DrawRows();
    DWCi_SBlSet(sAddressSettings.scroll * ADDRESS_BAR_LENGTH / ADDRESS_SCROLL_MAX);
    DWCi_SBlEnable();
    applyForcus();
    sAddressSettings.work->scrollTask = NULL;
    DWCi_TSKlDelete(0, task);
}

static void DWCUtilAddressSettings_ScrollBgTask(DWCUtilTask *task, void *arg) {
    int top;
    int offset;

    if (sAddressSettings.work->moved) {
        top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
        offset = FX_ModS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
        G2_SetBG0Offset(0, offset - 0x33);
        G2_SetBG2Offset(0, offset - 0x33 + sAddressLabelOffsets[top]);
        sAddressSettings.work->moved = FALSE;
    }
}

// Frames the item the cursor is on
static void DWCUtilAddressSettings_ShowCursor(void) {
    u8 rects[14] = { 0, 0, 1, 2, 0, 0, 0, 1, 2, 0, 0, 3, 4, 5 };
    DWCUtilAddressRect rect;
    int index = rects[sAddressSettings.cursor];

    if (index >= 3) {
        DWCi_FOCUSlDisp(3, sAddressCursorRects[index].coords[0], sAddressCursorRects[index].coords[2],
                        sAddressCursorRects[index].coords[1]);
        return;
    }
    rect = sAddressCursorRects[index];
    rect.coords[1] += sAddressSettings.row * ADDRESS_ROW_HEIGHT;
    DWCi_FOCUSlDisp(1, rect.coords[0], rect.coords[2], rect.coords[1]);
}

// Puts the cursor on the item of its row, the chosen answer of a row of DHCP
static void applyForcus(void) {
    DWCUtilSettingsEdit *edit;
    int index;

    if (sAddressSettings.cursor == ADDRESS_ITEM_TEST || sAddressSettings.cursor == ADDRESS_ITEM_SAVE ||
        sAddressSettings.cursor == ADDRESS_ITEM_BACK) {
        DWCUtilAddressSettings_ShowCursor();
        return;
    }
    edit = DWCUtilWifiSettings_GetEdit();
    index = sAddressSettings.row + FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    switch (index) {
    case ADDRESS_ROW_AUTO_ADDRESS:
        if (edit->autoAddress) {
            sAddressSettings.cursor = ADDRESS_ITEM_AUTO_ADDRESS_YES;
        } else {
            sAddressSettings.cursor = ADDRESS_ITEM_AUTO_ADDRESS_NO;
        }
        break;
    case ADDRESS_ROW_AUTO_DNS:
        if (edit->autoDns) {
            sAddressSettings.cursor = ADDRESS_ITEM_AUTO_DNS_YES;
        } else {
            sAddressSettings.cursor = ADDRESS_ITEM_AUTO_DNS_NO;
        }
        break;
    default:
        sAddressSettings.cursor = sAddressRowItems[index];
        break;
    }
    DWCUtilAddressSettings_ShowCursor();
}

static void applyForcus1(int item) {
    sAddressSettings.cursor = item;
    sAddressSettings.row = calcColumnIdx(sAddressItemRows[item]);
    DWCUtilAddressSettings_ShowCursor();
}

// Moves the cursor: right (0), up (1), left (2) or down (3), scrolling the list at its ends
static void DWCUtilAddressSettings_MoveCursor(int direction) {
    DWCUtilSettingsEdit *edit = DWCUtilWifiSettings_GetEdit();
    u8 cursor = sAddressSettings.cursor;
    int moved = 0;

    if (sAddressSettings.cursor == ADDRESS_ITEM_AUTO_DNS_NO && !edit->autoAddress) {
        if (direction == 0) {
            return;
        }
        if (direction == 2) {
            return;
        }
    }
    switch (sAddressSettings.cursor) {
    case ADDRESS_ITEM_SSID:
        if (direction == 1) {
            sAddressSettings.cursor = ADDRESS_ITEM_TEST;
        } else if (direction == 3) {
            sAddressSettings.row++;
        } else {
            moved = 2;
        }
        break;
    case ADDRESS_ITEM_DNS2:
        if (direction == 1) {
            sAddressSettings.row--;
        } else if (direction == 3) {
            sAddressSettings.cursor = sAddressSettings.work->lastButton;
        } else {
            moved = 2;
        }
        break;
    case ADDRESS_ITEM_TEST:
        if (direction == 1) {
            if (sAddressSettings.work->atEnd) {
                return;
            }
            DWCUtilSound_PlaySe(9);
            sAddressSettings.work->atEnd = TRUE;
            return;
        }
        if (direction == 3) {
            sAddressSettings.cursor = ADDRESS_ITEM_SSID;
            sAddressSettings.row = 0;
            sAddressSettings.scroll = 0;
            DWCUtilAddressSettings_DrawRows();
            DWCi_SBlSet(0);
        } else {
            moved = 2;
        }
        break;
    case ADDRESS_ITEM_SAVE:
    case ADDRESS_ITEM_BACK:
        sAddressSettings.work->lastButton = sAddressSettings.cursor;
        if (direction == 1) {
            sAddressSettings.cursor = ADDRESS_ITEM_DNS2;
            sAddressSettings.row = 3;
            sAddressSettings.scroll = ADDRESS_SCROLL_MAX;
            DWCUtilAddressSettings_DrawRows();
            DWCi_SBlSet(ADDRESS_BAR_LENGTH);
        } else if (direction == 3) {
            if (sAddressSettings.work->atEnd) {
                return;
            }
            DWCUtilSound_PlaySe(9);
            sAddressSettings.work->atEnd = TRUE;
            return;
        } else if (cursor == ADDRESS_ITEM_SAVE) {
            sAddressSettings.cursor = ADDRESS_ITEM_BACK;
        } else {
            sAddressSettings.cursor = ADDRESS_ITEM_SAVE;
        }
        break;
    default:
        if (direction == 1) {
            if (sAddressSettings.row != 0) {
                sAddressSettings.row--;
            } else {
                DWCUtilSound_PlaySe(0x13);
                sAddressSettings.work->scrollTask = DWCi_TSKlForm(0, DWCUtilAddressSettings_ScrollUpTask, NULL, 0x78);
                return;
            }
        } else if (direction == 3) {
            if (sAddressSettings.row < 3) {
                sAddressSettings.row++;
            } else {
                DWCUtilSound_PlaySe(0x13);
                sAddressSettings.work->scrollTask = DWCi_TSKlForm(0, DWCUtilAddressSettings_ScrollDownTask, NULL, 0x78);
                return;
            }
        } else {
            moved = 2;
            if (sAddressSettings.cursor == ADDRESS_ITEM_AUTO_ADDRESS_YES) {
                sAddressSettings.cursor = ADDRESS_ITEM_AUTO_ADDRESS_NO;
            } else if (sAddressSettings.cursor == ADDRESS_ITEM_AUTO_ADDRESS_NO) {
                sAddressSettings.cursor = ADDRESS_ITEM_AUTO_ADDRESS_YES;
            } else if (sAddressSettings.cursor == ADDRESS_ITEM_AUTO_DNS_YES) {
                sAddressSettings.cursor = ADDRESS_ITEM_AUTO_DNS_NO;
            } else if (sAddressSettings.cursor == ADDRESS_ITEM_AUTO_DNS_NO) {
                sAddressSettings.cursor = ADDRESS_ITEM_AUTO_DNS_YES;
            } else {
                break;
            }
            DWCUtilSound_PlaySe(8);
            DWCUtilAddressSettings_ShowCursor();
        }
        break;
    }
    if (moved != 2) {
        DWCUtilSound_PlaySe(8);
        if (moved == 0) {
            applyForcus();
        }
    }
}

// Lights up the chosen button: its row of the palette
static void DWCUtilAddressSettings_LightButton(void) {
    DWCi_SCREENlCopyPalette(sAddressSettings.work->palettes[1], sAddressButtonPalettes[sAddressSettings.cursor - 0xb],
                            sAddressButtonPalettes[sAddressSettings.cursor - 0xb]);
}

// Counts down the frames that the answers stay lit
static void DWCUtilAddressSettings_UpdateLitTask(DWCUtilTask *task, void *arg) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sAddressSettings.work->lit[i] != 0) {
            sAddressSettings.work->lit[i]--;
            if (sAddressSettings.work->lit[i] == 0) {
                DWCUtilAddressSettings_DrawRows();
            }
        }
    }
}

// Lights the touched answer for 20 frames, and the other answer of its row no longer
static void pushButtonYesNo(void) {
    u8 items[4] = { ADDRESS_ITEM_AUTO_ADDRESS_YES, ADDRESS_ITEM_AUTO_ADDRESS_NO, ADDRESS_ITEM_AUTO_DNS_YES,
                    ADDRESS_ITEM_AUTO_DNS_NO };
    int i;

    for (i = 0; i < 4; i++) {
        if (sAddressSettings.cursor == items[i]) {
            sAddressSettings.work->lit[i] = 20;
            if (i & 1) {
                sAddressSettings.work->lit[i - 1] = 0;
                return;
            }
            sAddressSettings.work->lit[i + 1] = 0;
            return;
        }
    }
}

// Whether the settings can be saved: an SSID, a usable DNS server unless DHCP gives them, and a usable address and
// gateway that suit the mask unless DHCP gives them
static BOOL DWCUtilAddressSettings_IsValid(void) {
    DWCUtilSettingsEdit *edit = DWCUtilWifiSettings_GetEdit();

    if (edit->connection.ssid[0] == 0) {
        return FALSE;
    }
    if (!edit->autoDns && !DWC_BACKUPlCheckAddress(edit->connection.dns[0]) &&
        !DWC_BACKUPlCheckAddress(edit->connection.dns[1])) {
        return FALSE;
    }
    if (!edit->autoAddress) {
        if (!DWC_BACKUPlCheckAddress(edit->connection.ip)) {
            return FALSE;
        }
        if (!DWC_BACKUPlCheckAddress(edit->connection.gateway)) {
            return FALSE;
        }
        if (!DWC_BACKUPlCheckIp(edit->connection.ip, edit->netmask)) {
            return FALSE;
        }
    }
    return TRUE;
}

static void DWCUtilAddressSettings_WaitInvalidDialog(void) {
    if (DWCi_WINlGet() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCi_WINlEnd();
        DWCUtilBgBuffer_LoadPalette(sAddressSettings.work->palettes[0]);
        DWCi_ChangeScene(DWCUtilAddressSettings_WaitInvalidClosed);
    }
}

static void DWCUtilAddressSettings_WaitInvalidClosed(void) {
    if (!DWCi_WINlIsInit()) {
        DWCi_ChangeScene(DWCUtilAddressSettings_Main);
    }
}

void DWCi_SetListReturn(int from) {
    sAddressSettings.from = from;
}
