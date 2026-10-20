#include "dwc_util_page_ap_list.h"
#include "dwc_util_ap_scan.h"
#include "dwc_util_ap_search.h"
#include "dwc_util_auto_connect_intro.h"
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
#include "dwc_util_screen.h"
#include "dwc_util_screen_way.h"
#include "dwc_util_scrollbar.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_string.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_wep_keyboard.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"

// The rows of the list on the screen, the height of a row, and the cursor's place on the button above the list
#define AP_LIST_ROWS 4
#define AP_LIST_ROW_HEIGHT 28
#define AP_LIST_BUTTON 4

// The list of the access points that the scan found
typedef struct DWCUtilApList {
    DWCUtilApEntry *entries;
    // The screen behind the rows, and the palette whose row lights up the button
    void *backScreen;
    void *palette;
    DWCUtilText *text;
    // The icons of each row on the screen: the access point's security and its signal
    GXOamAttr *securityOams[AP_LIST_ROWS + 1];
    GXOamAttr *signalOams[AP_LIST_ROWS + 1];
    // The task that scrolls the list by a row, and the VBlank task that scrolls the BGs
    DWCUtilTask *scrollTask;
    DWCUtilTask *vblankTask;
    u16 scrollMax;
    u16 securityChars[3];
    u16 signalChars[4];
    s8 touched;
    u8 count;
    u8 chosen;
    // The scroll bar's travel
    u8 barLength;
    u8 confirmed;
    u8 dragDelay;
    u8 moved;
    u8 dragging;
    // Whether the end of the list has played its sound, for the scroll bar's arrows and for the buttons
    u8 barAtEnd;
    u8 atEnd;
} DWCUtilApList;

// The cursor and the scroll of the list, in pixels, which stay for the next visit
typedef struct DWCUtilApListPos {
    u8 cursor;
    u16 scroll;
} DWCUtilApListPos;

static void DWCUtilPageApList_LoadGraphics(void);
static void DWCUtilPageApList_CreateScrollbar(void);
static void DWCUtilPageApList_CreateIcons(void);
static void DWCUtilPageApList_FadeIn(void);
static void DWCUtilPageApList_WaitFadeIn(void);
static void DWCUtilPageApList_WaitButtons(void);
static void DWCUtilPageApList_Main(void);
static void DWCUtilPageApList_HandleInput(void);
static void DWCUtilPageApList_HandleScrollbar(void);
static void DWCUtilPageApList_HandleChoice(void);
static void DWCUtilPageApList_DisableButtons(void);
static void DWCUtilPageApList_FadeOut(void);
static void DWCUtilPageApList_WaitFadeOut(void);
static void DWCUtilPageApList_DrawRows(void);
static void DWCUtilPageApList_DrawSsid(int index, int row);
static void DWCUtilPageApList_SetRowIcons(int index, int row);
static void DWCUtilPageApList_MoveRows(void);
static void DWCUtilPageApList_ScrollUpTask(DWCUtilTask *task, void *arg);
static void DWCUtilPageApList_ScrollDownTask(DWCUtilTask *task, void *arg);
static void DWCUtilPageApList_ScrollBgTask(DWCUtilTask *task, void *arg);
static void DWCUtilPageApList_DrawCursor(void);
static void DWCUtilPageApList_MoveCursor(int direction);
static void DWCUtilPageApList_ScrollUp(void);
static void DWCUtilPageApList_ScrollDown(void);
static void DWCUtilPageApList_HighlightButton(void);
static void DWCUtilPageApList_WaitDialog(void);
static void DWCUtilPageApList_WaitDialogClosed(void);

// The cells of the security icons, by the access point's security, and of the signal icons, by its link level
static const u8 sApListSecurityCells[3] = { 0x2e, 0x2d, 0x33 };
static const u8 sApListSignalCells[4] = { 0x18, 0x17, 0x16, 0x15 };
// Where the rows and the button are touched, and where the cursor frames them: left, top, right and bottom
static const u16 sApListRowRects[AP_LIST_ROWS + 1][4] = {
    { 7, 0x32, 0xd0, 0x4c }, { 7, 0x4e, 0xd0, 0x68 },    { 7, 0x6a, 0xd0, 0x84 },
    { 7, 0x86, 0xd0, 0xa0 }, { 0x85, 0x1b, 0xfd, 0x2c },
};
static const u16 sApListCursorRects[AP_LIST_ROWS + 1][4] = {
    { 4, 0x2e, 0xdb, 0x3f }, { 4, 0x4a, 0xdb, 0x5b },    { 4, 0x66, 0xdb, 0x77 },
    { 4, 0x82, 0xdb, 0x93 }, { 0x82, 0x18, 0xf0, 0x2c },
};

static DWCUtilApListPos sApListPos;
static DWCUtilApList *sApList;

void DWCUtilPageApList_Init(void) {
    int again;

    sApList = DWCUtilHeap_AllocClear(sizeof(DWCUtilApList), 4);
    DWCUtilMain_GetScreenArgs(&again, NULL);
    if (again == 0) {
        sApListPos.cursor = 0;
        sApListPos.scroll = 0;
    }
    sApList->count = DWCUtilApScan_GetEntries(&sApList->entries);
    DWCUtilPageApList_LoadGraphics();
    DWCUtilBgOverlay_ShowConnectionSet();
    DWCUtilScreen_Start(0x34, -1, 0);
    DWCUtilGfxResource_ShowStep(2);
    DWCUtilPageApList_CreateScrollbar();
    DWCUtilPageApList_CreateIcons();
    sApList->text = DWCUtilText_CreateBg(0, 0);
    sApList->vblankTask = DWCUtilTask_Create(1, DWCUtilPageApList_ScrollBgTask, NULL, 0x6e);
    DWCUtilPageApList_DrawRows();
    DWCUtilPageApList_DrawCursor();
    DWCUtilMain_SetUpdate(DWCUtilPageApList_FadeIn);
}

static void DWCUtilPageApList_LoadGraphics(void) {
    char backPath[] = "char/xb4ApListBack.nsc.l";
    char palettePath[] = "char/ybBgStep31.ncl.l";

    DWCUtilGfxResource_Load("char/jb4ApList.nsc.l", GX_LoadBG3Scr);
    sApList->backScreen = DWCUtilRomResource_LoadFile(DWCUtilGfxResource_LocalizePath(backPath), NULL, 4);
    DWCUtilBgBuffer_Init(sApList->backScreen);
    DWCUtilBgBuffer_MarkDirty();
    sApList->palette = DWCUtilRomResource_LoadFile(DWCUtilGfxResource_LocalizePath(palettePath), NULL, 4);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(2);
    G2_SetBG2Priority(3);
    G2_SetBG3Priority(2);
}

// The scroll bar, with a handle for the lists of up to 8 and of more access points
static void DWCUtilPageApList_CreateScrollbar(void) {
    int pos = 0;
    int kind;

    sApList->scrollMax = (sApList->count - AP_LIST_ROWS) * AP_LIST_ROW_HEIGHT;
    if (sApList->count <= AP_LIST_ROWS) {
        kind = 0;
        sApList->barLength = 0;
    } else if (sApList->count <= AP_LIST_ROWS * 2) {
        kind = 1;
        sApList->barLength = 0x1f;
    } else {
        kind = 2;
        sApList->barLength = 0x37;
    }
    if (kind != 0) {
        pos = FX_DivS32(sApListPos.scroll * sApList->barLength, sApList->scrollMax);
    }
    DWCUtilScrollbar_Create(kind, 0x55, 0xec, 0x3f, pos);
}

static void DWCUtilPageApList_CreateIcons(void) {
    int rows = sApList->count;
    int i;
    u32 kind;
    u32 level;

    if (rows > AP_LIST_ROWS + 1) {
        rows = AP_LIST_ROWS + 1;
    }
    for (i = 0; i < rows; i++) {
        sApList->securityOams[i] = DWCUtilCell_AllocOam(0, 0x2e);
        sApList->signalOams[i] = DWCUtilCell_AllocOam(0, 0x18);
    }
    for (kind = 0; kind < NELEMS(sApListSecurityCells); kind++) {
        DWCUtilCell_Copy(0, sApListSecurityCells[kind], sApList->securityOams[0]);
        sApList->securityChars[kind] = sApList->securityOams[0]->attr2 & GX_OAM_ATTR2_NAME_MASK;
    }
    for (level = 0; level < NELEMS(sApListSignalCells); level++) {
        DWCUtilCell_Copy(0, sApListSignalCells[level], sApList->signalOams[0]);
        sApList->signalChars[level] = sApList->signalOams[0]->attr2 & GX_OAM_ATTR2_NAME_MASK;
    }
    for (i = 0; i < rows; i++) {
        G2_SetOBJPriority(sApList->securityOams[i], 3);
        G2_SetOBJPriority(sApList->signalOams[i], 3);
    }
}

static void DWCUtilPageApList_FadeIn(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x1d, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x1d);
    DWCUtilMain_SetUpdate(DWCUtilPageApList_WaitFadeIn);
}

static void DWCUtilPageApList_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(0);
        DWCUtilMain_SetUpdate(DWCUtilPageApList_WaitButtons);
    }
}

static void DWCUtilPageApList_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilPageApList_Main);
    }
}

static void DWCUtilPageApList_Main(void) {
    DWCUtilPageApList_HandleInput();
    DWCUtilPageApList_HandleScrollbar();
    DWCUtilPageApList_HandleChoice();
}

static void DWCUtilPageApList_HandleInput(void) {
    int i;

    if (sApList->scrollTask != NULL || sApList->dragging) {
        return;
    }
    if (DWCUtilTouch_IsTriggeredIn(gDWCUtilTouchScreenRect)) {
        sApList->touched = -1;
        for (i = 0; i < NELEMS(sApListRowRects); i++) {
            if (DWCUtilTouch_IsTriggeredIn(sApListRowRects[i])) {
                if (i < AP_LIST_ROWS) {
                    sApList->touched = i;
                    break;
                }
                DWCUtilUiObject_SetStateIfIdle(1);
                sApListPos.cursor = i;
                DWCUtilPageApList_DrawCursor();
                return;
            }
        }
    }
    if (DWCUtilTouch_IsReleasedIn(gDWCUtilTouchScreenRect)) {
        for (i = 0; i < AP_LIST_ROWS; i++) {
            if (DWCUtilTouch_IsReleasedIn(sApListRowRects[i])) {
                if (sApList->touched != i) {
                    break;
                }
                if (i >= sApList->count) {
                    DWCUtilSound_PlaySe(9);
                    break;
                }
                DWCUtilUiObject_SetStateIfIdle(1);
                sApListPos.cursor = i;
                DWCUtilPageApList_DrawCursor();
                return;
            }
        }
    }
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
        DWCUtilScrollbar_Lock();
    } else if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (DWCUtilTouch_IsKeyRepeated(0x200)) {
        DWCUtilPageApList_ScrollDown();
    } else if (DWCUtilTouch_IsKeyReleased(0x200)) {
        sApList->atEnd = FALSE;
    } else if (DWCUtilTouch_IsKeyRepeated(0x100)) {
        DWCUtilPageApList_ScrollUp();
    } else if (DWCUtilTouch_IsKeyReleased(0x100)) {
        sApList->atEnd = FALSE;
    } else if (DWCUtilTouch_IsKeyRepeated(0x40)) {
        DWCUtilPageApList_MoveCursor(1);
    } else if (DWCUtilTouch_IsKeyReleased(0x40)) {
        sApList->atEnd = FALSE;
    } else if (DWCUtilTouch_IsKeyRepeated(0x80)) {
        DWCUtilPageApList_MoveCursor(3);
    } else if (DWCUtilTouch_IsKeyReleased(0x80)) {
        sApList->atEnd = FALSE;
    }
}

// The scroll bar: its handle dragged, or its arrows
static void DWCUtilPageApList_HandleScrollbar(void) {
    int offset;

    if (sApList->scrollTask != NULL) {
        return;
    }
    if (sApList->dragDelay != 0) {
        sApList->dragDelay--;
    }
    switch (DWCUtilScrollbar_GetEvent()) {
    case 1:
        sApList->dragging = TRUE;
        DWCUtilUiObject_Disable();
        break;
    case 2:
        if (sApList->dragDelay == 0) {
            DWCUtilButtonSprites_Hide();
            sApListPos.scroll = FX_DivS32(DWCUtilScrollbar_GetPos() * sApList->scrollMax, sApList->barLength);
            DWCUtilPageApList_DrawRows();
            sApList->dragDelay = 4;
        }
        break;
    case 3:
        sApList->dragging = FALSE;
        DWCUtilUiObject_Enable();
        sApListPos.scroll = FX_DivS32(DWCUtilScrollbar_GetPos() * sApList->scrollMax, sApList->barLength);
        DWCUtilSound_PlaySe(0x13);
        DWCUtilPageApList_DrawRows();
        offset = FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
        if (offset == 0) {
            DWCUtilPageApList_DrawCursor();
        } else if (offset < AP_LIST_ROW_HEIGHT / 2) {
            sApList->scrollTask = DWCUtilTask_Create(0, DWCUtilPageApList_ScrollUpTask, NULL, 0x78);
        } else {
            sApList->scrollTask = DWCUtilTask_Create(0, DWCUtilPageApList_ScrollDownTask, NULL, 0x78);
        }
        break;
    case 4:
        if (sApListPos.scroll == 0) {
            if (!sApList->barAtEnd) {
                DWCUtilSound_PlaySe(9);
                sApList->barAtEnd = TRUE;
            }
        } else {
            DWCUtilSound_PlaySe(0x13);
            sApList->scrollTask = DWCUtilTask_Create(0, DWCUtilPageApList_ScrollUpTask, NULL, 0x78);
        }
        break;
    case 6:
        if (sApList->count <= AP_LIST_ROWS || sApListPos.scroll == sApList->scrollMax) {
            if (!sApList->barAtEnd) {
                DWCUtilSound_PlaySe(9);
                sApList->barAtEnd = TRUE;
            }
        } else {
            DWCUtilSound_PlaySe(0x13);
            sApList->scrollTask = DWCUtilTask_Create(0, DWCUtilPageApList_ScrollDownTask, NULL, 0x78);
        }
        break;
    case 5:
    case 7:
        sApList->barAtEnd = FALSE;
        break;
    }
}

static void DWCUtilPageApList_HandleChoice(void) {
    int index;

    if (sApList->scrollTask != NULL || sApList->dragging) {
        return;
    }
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        break;
    case 1:
        if (sApListPos.cursor == AP_LIST_BUTTON) {
            sApList->confirmed = TRUE;
            DWCUtilSound_PlaySe(6);
            DWCUtilPageApList_HighlightButton();
        } else {
            index = sApListPos.cursor + FX_DivS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
            if (sApList->entries[index].security == 2) {
                DWCUtilSound_PlaySe(9);
                DWCUtilScrollbar_Lock();
                DWCUtilUiObject_Disable();
                DWCUtilDialog_Open(0xe, 1, 1, -1, 0);
                DWCUtilMain_SetUpdate(DWCUtilPageApList_WaitDialog);
                return;
            }
            sApList->confirmed = TRUE;
            sApList->chosen = index;
            DWCUtilSound_PlaySe(6);
        }
        break;
    default:
        return;
    }
    DWCUtilMain_SetUpdate(DWCUtilPageApList_DisableButtons);
}

static void DWCUtilPageApList_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilPageApList_FadeOut);
}

static void DWCUtilPageApList_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        if (sApList->confirmed) {
            DWCUtilUiObject_Exit();
        } else {
            DWCUtilUiObject_Disable();
        }
        DWCUtilFade_Start(3, 1, 1, 8);
        DWCUtilFade_Start(3, 0, 0x1d, 8);
        DWCUtilMain_SetUpdate(DWCUtilPageApList_WaitFadeOut);
    }
}

// Frees the list and goes on: back, to the button's screen, or to the chosen access point's security key
static void DWCUtilPageApList_WaitFadeOut(void) {
    int i;

    if (DWCUtilFade_IsActive(1) != 0 || DWCUtilFade_IsActive(0) != 0 || !DWCUtilUiObject_IsIdle()) {
        return;
    }
    DWCUtilTask_Remove(1, sApList->vblankTask);
    for (i = 0; i < AP_LIST_ROWS + 1; i++) {
        if (sApList->securityOams[i] != NULL) {
            DWCUtilOamBuf_Free(sApList->securityOams[i]);
        }
        if (sApList->signalOams[i] != NULL) {
            DWCUtilOamBuf_Free(sApList->signalOams[i]);
        }
    }
    DWCUtilText_DestroyBg(sApList->text);
    DWCUtilScrollbar_Destroy();
    DWCUtilButtonSprites_Hide();
    DWCUtilScreen_Stop();
    DWCUtilBgBuffer_Exit();
    DWCUtilRomResource_FreeFile(sApList->backScreen);
    DWCUtilRomResource_FreeFile(sApList->palette);
    DWCUtilDisplay_HidePlanes(1, 1);
    DWCUtilDisplay_HidePlanes(0, 0x1d);
    G2_SetBG0Offset(0, 0);
    G2_SetBG2Offset(0, 0);
    if (!sApList->confirmed) {
        DWCUtilApScan_Exit();
        DWCUtilWifiSettings_EditConnection(DWCUtilWifiSettings_GetEdit()->index);
        DWCUtilMain_SetLayout(2, 0);
        DWCUtilMain_SetUpdate(DWCUtilScreenWay_Init);
    } else if (sApListPos.cursor == AP_LIST_BUTTON) {
        DWCUtilApScan_Exit();
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(DWCUtilApSearch_Init);
    } else {
        DWCUtilWifiSettings_SetSsid(sApList->entries[sApList->chosen].ssid);
        DWCUtilMain_SetLayout(0, 0);
        if (sApList->entries[sApList->chosen].security != 0) {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetScreenArgs(1, 1);
            DWCUtilMain_SetUpdate(DWCUtilWepKeyboard_Init);
        } else {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetScreenArgs(0, 1);
            DWCUtilMain_SetUpdate(DWCUtilAutoConnectIntro_Init);
        }
    }
    DWCUtilHeap_FreeAndNull(&sApList);
}

// Draws the rows on the screen from the scroll
static void DWCUtilPageApList_DrawRows(void) {
    int top = FX_DivS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
    int rows = sApList->count;
    int row;
    int index;

    DWCUtilText_Clear(sApList->text, 0);
    if (rows > AP_LIST_ROWS + 1) {
        rows = AP_LIST_ROWS + 1;
    }
    for (row = 0, index = top; row < rows; row++) {
        DWCUtilPageApList_DrawSsid(index++, row);
    }
    for (row = 0; row < rows; row++) {
        DWCUtilPageApList_SetRowIcons(top++, row);
    }
    DWCUtilText_MarkDirty(sApList->text);
    DWCUtilPageApList_MoveRows();
}

// The access point's SSID, on two lines when it is longer than 16 characters
static void DWCUtilPageApList_DrawSsid(int index, int row) {
    int length = DWCUtilString_Length(sApList->entries[index].ssid, sizeof(sApList->entries[index].ssid));
    int y = row * AP_LIST_ROW_HEIGHT;
    u16 line[17];
    int count;
    int i;

    if (index >= sApList->count) {
        return;
    }
    if (length <= 16) {
        y += 6;
    }
    MI_CpuFill8(line, 0, sizeof(line));
    count = length <= 16 ? length : 16;
    for (i = 0; i < count; i++) {
        line[i] = sApList->entries[index].ssid[i];
    }
    DWCUtilText_DrawCentered(sApList->text, 10, y, 2, 10, line, 1);
    if (length > 16) {
        MI_CpuFill8(line, 0, sizeof(line));
        for (i = 0; i < length - 16; i++) {
            line[i] = sApList->entries[index].ssid[16 + i];
        }
        y += 12;
        DWCUtilText_DrawCentered(sApList->text, 10, y, 2, 10, line, 1);
    }
}

static void DWCUtilPageApList_SetRowIcons(int index, int row) {
    if (index >= sApList->count) {
        return;
    }
    G2_SetOBJCharName(sApList->securityOams[row], sApList->securityChars[sApList->entries[index].security]);
    G2_SetOBJCharName(sApList->signalOams[row], sApList->signalChars[sApList->entries[index].linkLevel]);
}

// Moves the icons to the rows' places from the scroll, and has the BGs follow at the next VBlank
static void DWCUtilPageApList_MoveRows(void) {
    int y = 0x36 - FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
    int rows = sApList->count;
    int row;

    if (rows > AP_LIST_ROWS + 1) {
        rows = AP_LIST_ROWS + 1;
    }
    for (row = 0; row < rows; row++) {
        DWCUtilOam_SetPosition(sApList->securityOams[row], 0xb3, y - 2);
        DWCUtilOam_SetPosition(sApList->signalOams[row], 0xd2, y + 1);
        y += AP_LIST_ROW_HEIGHT;
    }
    sApList->moved = TRUE;
}

// Scrolls the list up by 4 pixels a frame, until it rests on a row
static void DWCUtilPageApList_ScrollUpTask(DWCUtilTask *task, void *arg) {
    int offset;

    DWCUtilScrollbar_Lock();
    DWCUtilButtonSprites_Hide();
    if (sApListPos.scroll > 4) {
        sApListPos.scroll -= 4;
    } else {
        sApListPos.scroll = 0;
    }
    offset = FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
    if (offset == AP_LIST_ROW_HEIGHT - 4) {
        DWCUtilPageApList_DrawRows();
        return;
    }
    if (offset > AP_LIST_ROW_HEIGHT - 4) {
        sApListPos.scroll += AP_LIST_ROW_HEIGHT - offset;
        offset = 0;
    }
    DWCUtilPageApList_MoveRows();
    if (offset == 0) {
        DWCUtilScrollbar_SetPos(FX_DivS32(sApListPos.scroll * sApList->barLength, sApList->scrollMax));
        DWCUtilScrollbar_Unlock();
        DWCUtilPageApList_DrawCursor();
        sApList->scrollTask = NULL;
        DWCUtilTask_QueueRemoval(0, task);
    }
}

// Scrolls the list down by 4 pixels a frame, until it rests on a row
static void DWCUtilPageApList_ScrollDownTask(DWCUtilTask *task, void *arg) {
    int offset;

    DWCUtilScrollbar_Lock();
    DWCUtilButtonSprites_Hide();
    sApListPos.scroll += 4;
    offset = FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
    if (offset >= 4) {
        DWCUtilPageApList_MoveRows();
        return;
    }
    sApListPos.scroll -= offset;
    DWCUtilPageApList_DrawRows();
    DWCUtilScrollbar_SetPos(FX_DivS32(sApListPos.scroll * sApList->barLength, sApList->scrollMax));
    DWCUtilScrollbar_Unlock();
    DWCUtilPageApList_DrawCursor();
    sApList->scrollTask = NULL;
    DWCUtilTask_QueueRemoval(0, task);
}

static void DWCUtilPageApList_ScrollBgTask(DWCUtilTask *task, void *arg) {
    if (sApList->moved) {
        int y = FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT) - 0x32;

        G2_SetBG0Offset(0, y);
        G2_SetBG2Offset(0, y);
        sApList->moved = FALSE;
    }
}

static void DWCUtilPageApList_DrawCursor(void) {
    u8 cursor = sApListPos.cursor;
    int pair = 2;

    if (cursor >= AP_LIST_ROWS) {
        pair = 3;
    }
    DWCUtilButtonSprites_ShowPair(pair, sApListCursorRects[cursor][0], sApListCursorRects[cursor][2],
                                  sApListCursorRects[cursor][1]);
}

// Moves the cursor up (1) or down (3), scrolling the list at its ends
static void DWCUtilPageApList_MoveCursor(int direction) {
    BOOL moved = TRUE;

    switch (sApListPos.cursor) {
    case 0:
        if (direction == 1) {
            if (sApListPos.scroll == 0) {
                sApListPos.cursor = AP_LIST_BUTTON;
            } else {
                DWCUtilSound_PlaySe(0x13);
                sApList->scrollTask = DWCUtilTask_Create(0, DWCUtilPageApList_ScrollUpTask, NULL, 0x78);
                return;
            }
        } else if (sApList->count > 1) {
            sApListPos.cursor++;
        } else {
            moved = FALSE;
        }
        break;
    case 1:
    case 2:
        if (direction == 1) {
            sApListPos.cursor--;
        } else if (sApList->count > sApListPos.cursor + 1) {
            sApListPos.cursor++;
        } else {
            moved = FALSE;
        }
        break;
    case 3:
        if (direction == 1) {
            sApListPos.cursor--;
        } else {
            DWCUtilPageApList_ScrollDown();
            return;
        }
        break;
    case AP_LIST_BUTTON:
        if (direction == 1) {
            moved = FALSE;
        } else {
            sApListPos.scroll = 0;
            sApListPos.cursor = 0;
            DWCUtilPageApList_DrawRows();
            DWCUtilScrollbar_SetPos(0);
        }
        break;
    }
    if (!moved) {
        if (!sApList->atEnd) {
            DWCUtilSound_PlaySe(9);
            sApList->atEnd = TRUE;
        }
    } else {
        DWCUtilSound_PlaySe(8);
        DWCUtilPageApList_DrawCursor();
    }
}

// Scrolls the list up by a row
static void DWCUtilPageApList_ScrollUp(void) {
    if (sApListPos.scroll == 0) {
        if (!sApList->atEnd) {
            DWCUtilSound_PlaySe(9);
            sApList->atEnd = TRUE;
        }
    } else {
        DWCUtilSound_PlaySe(0x13);
        sApList->scrollTask = DWCUtilTask_Create(0, DWCUtilPageApList_ScrollUpTask, NULL, 0x78);
    }
}

// Scrolls the list down by a row
static void DWCUtilPageApList_ScrollDown(void) {
    if (sApListPos.scroll == sApList->scrollMax || sApList->count <= AP_LIST_ROWS) {
        if (!sApList->atEnd) {
            DWCUtilSound_PlaySe(9);
            sApList->atEnd = TRUE;
        }
    } else {
        DWCUtilSound_PlaySe(0x13);
        sApList->scrollTask = DWCUtilTask_Create(0, DWCUtilPageApList_ScrollDownTask, NULL, 0x78);
    }
}

// Lights up the button: its row of the palette
static void DWCUtilPageApList_HighlightButton(void) {
    u8 rows[] = { 2 };

    DWCUtilBgBuffer_LoadPaletteRow(sApList->palette, rows[0], rows[0]);
}

static void DWCUtilPageApList_WaitDialog(void) {
    if (DWCUtilDialog_GetChoice() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilDialog_Close();
        DWCUtilMain_SetUpdate(DWCUtilPageApList_WaitDialogClosed);
    }
}

static void DWCUtilPageApList_WaitDialogClosed(void) {
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilUiObject_Enable();
        DWCUtilScrollbar_Unlock();
        DWCUtilMain_SetUpdate(DWCUtilPageApList_Main);
    }
}
