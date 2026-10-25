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

static void initGraphics(void);
static void DWCUtilPageApList_CreateScrollbar(void);
static void DWCUtilPageApList_CreateIcons(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void button(void);
static void DWCUtilPageApList_DisableButtons(void);
static void endScene1(void);
static void endScene2(void);
static void drawApInfo(void);
static void drawAp(int index, int row);
static void drapApObj(int index, int row);
static void setOffset(void);
static void DWCUtilPageApList_ScrollUpTask(DWCUtilTask *task, void *arg);
static void DWCUtilPageApList_ScrollDownTask(DWCUtilTask *task, void *arg);
static void DWCUtilPageApList_ScrollBgTask(DWCUtilTask *task, void *arg);
static void DWCUtilPageApList_DrawCursor(void);
static void DWCUtilPageApList_MoveCursor(int direction);
static void scrollUp(void);
static void scrollDown(void);
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

    sApList = DWCi_HEAPlAllocEx(sizeof(DWCUtilApList), 4);
    DWCi_GetParam2_(&again, NULL);
    if (again == 0) {
        sApListPos.cursor = 0;
        sApListPos.scroll = 0;
    }
    sApList->count = DWCi_SEARCHlGetApInfo(&sApList->entries);
    initGraphics();
    DWCi_HLlLoadSet();
    DWCi_COMMENTlDispEx(0x34, -1, 0);
    DWCi_CMNlStep(2);
    DWCUtilPageApList_CreateScrollbar();
    DWCUtilPageApList_CreateIcons();
    sApList->text = DWCi_FNTlFormBgEx(0, 0);
    sApList->vblankTask = DWCi_TSKlForm(1, DWCUtilPageApList_ScrollBgTask, NULL, 0x6e);
    drawApInfo();
    DWCUtilPageApList_DrawCursor();
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    char backPath[] = "char/xb4ApListBack.nsc.l";
    char palettePath[] = "char/ybBgStep31.ncl.l";

    DWCUtilGfxResource_Load("char/jb4ApList.nsc.l", GX_LoadBG3Scr);
    sApList->backScreen = DWCUtilRomResource_LoadFile(DWCi_Char(backPath), NULL, 4);
    DWCi_SCREENlInit(sApList->backScreen);
    DWCi_SCREENlRenew();
    sApList->palette = DWCUtilRomResource_LoadFile(DWCi_Char(palettePath), NULL, 4);
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

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0,
                  GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ);
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
    int i;

    if (sApList->scrollTask != NULL || sApList->dragging) {
        return;
    }
    if (DWCi_TPlCheckTrigger(gDWCUtilTouchScreenRect)) {
        sApList->touched = -1;
        for (i = 0; i < NELEMS(sApListRowRects); i++) {
            if (DWCi_TPlCheckTrigger(sApListRowRects[i])) {
                if (i < AP_LIST_ROWS) {
                    sApList->touched = i;
                    break;
                }
                DWCi_BTNlSet(1);
                sApListPos.cursor = i;
                DWCUtilPageApList_DrawCursor();
                return;
            }
        }
    }
    if (DWCi_TPlCheckRelease(gDWCUtilTouchScreenRect)) {
        for (i = 0; i < AP_LIST_ROWS; i++) {
            if (DWCi_TPlCheckRelease(sApListRowRects[i])) {
                if (sApList->touched != i) {
                    break;
                }
                if (i >= sApList->count) {
                    DWCUtilSound_PlaySe(9);
                    break;
                }
                DWCi_BTNlSet(1);
                sApListPos.cursor = i;
                DWCUtilPageApList_DrawCursor();
                return;
            }
        }
    }
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(1);
        DWCi_SBlDisable();
    } else if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    } else if (DWCi_IPTlCheckRepeat(0x200)) {
        scrollDown();
    } else if (DWCi_IPTlCheckRelease(0x200)) {
        sApList->atEnd = FALSE;
    } else if (DWCi_IPTlCheckRepeat(0x100)) {
        scrollUp();
    } else if (DWCi_IPTlCheckRelease(0x100)) {
        sApList->atEnd = FALSE;
    } else if (DWCi_IPTlCheckRepeat(0x40)) {
        DWCUtilPageApList_MoveCursor(1);
    } else if (DWCi_IPTlCheckRelease(0x40)) {
        sApList->atEnd = FALSE;
    } else if (DWCi_IPTlCheckRepeat(0x80)) {
        DWCUtilPageApList_MoveCursor(3);
    } else if (DWCi_IPTlCheckRelease(0x80)) {
        sApList->atEnd = FALSE;
    }
}

// The scroll bar: its handle dragged, or its arrows
static void draw(void) {
    int offset;

    if (sApList->scrollTask != NULL) {
        return;
    }
    if (sApList->dragDelay != 0) {
        sApList->dragDelay--;
    }
    switch (DWCi_SBlGetState()) {
    case 1:
        sApList->dragging = TRUE;
        DWCi_BTNlDisable();
        break;
    case 2:
        if (sApList->dragDelay == 0) {
            DWCi_FOCUSlDelete();
            sApListPos.scroll = FX_DivS32(DWCi_SBlGet() * sApList->scrollMax, sApList->barLength);
            drawApInfo();
            sApList->dragDelay = 4;
        }
        break;
    case 3:
        sApList->dragging = FALSE;
        DWCi_BTNlEnable();
        sApListPos.scroll = FX_DivS32(DWCi_SBlGet() * sApList->scrollMax, sApList->barLength);
        DWCUtilSound_PlaySe(0x13);
        drawApInfo();
        offset = FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
        if (offset == 0) {
            DWCUtilPageApList_DrawCursor();
        } else if (offset < AP_LIST_ROW_HEIGHT / 2) {
            sApList->scrollTask = DWCi_TSKlForm(0, DWCUtilPageApList_ScrollUpTask, NULL, 0x78);
        } else {
            sApList->scrollTask = DWCi_TSKlForm(0, DWCUtilPageApList_ScrollDownTask, NULL, 0x78);
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
            sApList->scrollTask = DWCi_TSKlForm(0, DWCUtilPageApList_ScrollUpTask, NULL, 0x78);
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
            sApList->scrollTask = DWCi_TSKlForm(0, DWCUtilPageApList_ScrollDownTask, NULL, 0x78);
        }
        break;
    case 5:
    case 7:
        sApList->barAtEnd = FALSE;
        break;
    }
}

static void button(void) {
    int index;

    if (sApList->scrollTask != NULL || sApList->dragging) {
        return;
    }
    switch (DWCi_BTNlGet()) {
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
                DWCi_SBlDisable();
                DWCi_BTNlDisable();
                DWCUtilDialog_Open(0xe, 1, 1, -1, 0);
                DWCi_ChangeScene(DWCUtilPageApList_WaitDialog);
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
    DWCi_ChangeScene(DWCUtilPageApList_DisableButtons);
}

static void DWCUtilPageApList_DisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        if (sApList->confirmed) {
            DWCi_BTNlEnd();
        } else {
            DWCi_BTNlDisable();
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ,
                      8);
        DWCi_ChangeScene(endScene2);
    }
}

// Frees the list and goes on: back, to the button's screen, or to the chosen access point's security key
static void endScene2(void) {
    int i;

    if (DWCi_EFFlCheck(1) != 0 || DWCi_EFFlCheck(0) != 0 || !DWCi_BTNlIsEnd()) {
        return;
    }
    DWCi_TSKlDeleteEx(1, sApList->vblankTask);
    for (i = 0; i < AP_LIST_ROWS + 1; i++) {
        if (sApList->securityOams[i] != NULL) {
            DWCi_OBJlDelete(sApList->securityOams[i]);
        }
        if (sApList->signalOams[i] != NULL) {
            DWCi_OBJlDelete(sApList->signalOams[i]);
        }
    }
    DWCi_FNTlDeleteBg(sApList->text);
    DWCi_SBlEnd();
    DWCi_FOCUSlDelete();
    DWCi_COMMENTlDelete();
    DWCi_SCREENlEnd();
    DWCi_ARClRelease(sApList->backScreen);
    DWCi_ARClRelease(sApList->palette);
    DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
    DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ);
    G2_SetBG0Offset(0, 0);
    G2_SetBG2Offset(0, 0);
    if (!sApList->confirmed) {
        DWCi_SEARCHlEnd();
        DWCi_SETTINGlSetSetting(DWCUtilWifiSettings_GetEdit()->index);
        DWCi_SetParam_(2, 0);
        DWCi_ChangeScene(DWCUtilScreenWay_Init);
    } else if (sApListPos.cursor == AP_LIST_BUTTON) {
        DWCi_SEARCHlEnd();
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCUtilApSearch_Init);
    } else {
        DWCi_SETTINGlSetSsid(sApList->entries[sApList->chosen].ssid);
        DWCi_SetParam_(0, 0);
        if (sApList->entries[sApList->chosen].security != 0) {
            DWCi_SetParam_(0, 1);
            DWCi_SetParam2_(1, 1);
            DWCi_ChangeScene(DWCUtilWepKeyboard_Init);
        } else {
            DWCi_SetParam_(0, 1);
            DWCi_SetParam2_(0, 1);
            DWCi_ChangeScene(DWCUtilAutoConnectIntro_Init);
        }
    }
    DWCi_HEAPlFree_(&sApList);
}

// Draws the rows on the screen from the scroll
static void drawApInfo(void) {
    int top = FX_DivS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
    int rows = sApList->count;
    int row;
    int index;

    DWCi_FNTlClear(sApList->text, 0);
    if (rows > AP_LIST_ROWS + 1) {
        rows = AP_LIST_ROWS + 1;
    }
    for (row = 0, index = top; row < rows; row++) {
        drawAp(index++, row);
    }
    for (row = 0; row < rows; row++) {
        drapApObj(top++, row);
    }
    DWCUtilText_MarkDirty(sApList->text);
    setOffset();
}

// The access point's SSID, on two lines when it is longer than 16 characters
static void drawAp(int index, int row) {
    int length = DWCi_STRlNLen(sApList->entries[index].ssid, sizeof(sApList->entries[index].ssid));
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
    DWCi_FNTlDrawStringEx(sApList->text, 10, y, 2, 10, line, 1);
    if (length > 16) {
        MI_CpuFill8(line, 0, sizeof(line));
        for (i = 0; i < length - 16; i++) {
            line[i] = sApList->entries[index].ssid[16 + i];
        }
        y += 12;
        DWCi_FNTlDrawStringEx(sApList->text, 10, y, 2, 10, line, 1);
    }
}

static void drapApObj(int index, int row) {
    if (index >= sApList->count) {
        return;
    }
    G2_SetOBJCharName(sApList->securityOams[row], sApList->securityChars[sApList->entries[index].security]);
    G2_SetOBJCharName(sApList->signalOams[row], sApList->signalChars[sApList->entries[index].linkLevel]);
}

// Moves the icons to the rows' places from the scroll, and has the BGs follow at the next VBlank
static void setOffset(void) {
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

    DWCi_SBlDisable();
    DWCi_FOCUSlDelete();
    if (sApListPos.scroll > 4) {
        sApListPos.scroll -= 4;
    } else {
        sApListPos.scroll = 0;
    }
    offset = FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
    if (offset == AP_LIST_ROW_HEIGHT - 4) {
        drawApInfo();
        return;
    }
    if (offset > AP_LIST_ROW_HEIGHT - 4) {
        sApListPos.scroll += AP_LIST_ROW_HEIGHT - offset;
        offset = 0;
    }
    setOffset();
    if (offset == 0) {
        DWCi_SBlSet(FX_DivS32(sApListPos.scroll * sApList->barLength, sApList->scrollMax));
        DWCi_SBlEnable();
        DWCUtilPageApList_DrawCursor();
        sApList->scrollTask = NULL;
        DWCi_TSKlDelete(0, task);
    }
}

// Scrolls the list down by 4 pixels a frame, until it rests on a row
static void DWCUtilPageApList_ScrollDownTask(DWCUtilTask *task, void *arg) {
    int offset;

    DWCi_SBlDisable();
    DWCi_FOCUSlDelete();
    sApListPos.scroll += 4;
    offset = FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
    if (offset >= 4) {
        setOffset();
        return;
    }
    sApListPos.scroll -= offset;
    drawApInfo();
    DWCi_SBlSet(FX_DivS32(sApListPos.scroll * sApList->barLength, sApList->scrollMax));
    DWCi_SBlEnable();
    DWCUtilPageApList_DrawCursor();
    sApList->scrollTask = NULL;
    DWCi_TSKlDelete(0, task);
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
    DWCi_FOCUSlDisp(pair, sApListCursorRects[cursor][0], sApListCursorRects[cursor][2], sApListCursorRects[cursor][1]);
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
                sApList->scrollTask = DWCi_TSKlForm(0, DWCUtilPageApList_ScrollUpTask, NULL, 0x78);
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
            scrollDown();
            return;
        }
        break;
    case AP_LIST_BUTTON:
        if (direction == 1) {
            moved = FALSE;
        } else {
            sApListPos.scroll = 0;
            sApListPos.cursor = 0;
            drawApInfo();
            DWCi_SBlSet(0);
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
static void scrollUp(void) {
    if (sApListPos.scroll == 0) {
        if (!sApList->atEnd) {
            DWCUtilSound_PlaySe(9);
            sApList->atEnd = TRUE;
        }
    } else {
        DWCUtilSound_PlaySe(0x13);
        sApList->scrollTask = DWCi_TSKlForm(0, DWCUtilPageApList_ScrollUpTask, NULL, 0x78);
    }
}

// Scrolls the list down by a row
static void scrollDown(void) {
    if (sApListPos.scroll == sApList->scrollMax || sApList->count <= AP_LIST_ROWS) {
        if (!sApList->atEnd) {
            DWCUtilSound_PlaySe(9);
            sApList->atEnd = TRUE;
        }
    } else {
        DWCUtilSound_PlaySe(0x13);
        sApList->scrollTask = DWCi_TSKlForm(0, DWCUtilPageApList_ScrollDownTask, NULL, 0x78);
    }
}

// Lights up the button: its row of the palette
static void DWCUtilPageApList_HighlightButton(void) {
    u8 rows[] = { 2 };

    DWCi_SCREENlCopyPalette(sApList->palette, rows[0], rows[0]);
}

static void DWCUtilPageApList_WaitDialog(void) {
    if (DWCi_WINlGet() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCi_WINlEnd();
        DWCi_ChangeScene(DWCUtilPageApList_WaitDialogClosed);
    }
}

static void DWCUtilPageApList_WaitDialogClosed(void) {
    if (!DWCi_WINlIsInit()) {
        DWCi_BTNlEnable();
        DWCi_SBlEnable();
        DWCi_ChangeScene(procScene);
    }
}
