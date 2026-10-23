#include "types.h"
#include "app/manual_common.h"
#include "app/manual_graphic.h"
#include "app/manual_touchbar.h"
#include "app/ui/touchbar.h"
#include "app/ui/ui_scene.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "system/app_menu_common.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/title_7.h"

// The manual's touch bar, named after the ROM's "manual_touchbar.c": the common touch bar's close and return icons,
// a window of text beside them, and the two arrows that turn the explanation's pages, which this file draws and
// reads itself

#define WINDOW_COUNT 2
#define ARROW_COUNT 2
#define ARROW_NONE 0xff
#define TEXT_COLOR PRINT_COLOR(15, 2, 0)

enum {
    WINDOW_UNUSED,
    WINDOW_TEXT,
};

enum {
    STATE_IDLE,
    STATE_DECIDED,
    STATE_ICON_TOUCHED,
    STATE_ARROW_TOUCHED,
};

enum {
    ARROW_STATE_IDLE,
    ARROW_STATE_TOUCHED,
    ARROW_STATE_ANIM,
    ARROW_STATE_DECIDED,
};

struct ManualTouchBar {
    ManualCommon *common;
    TouchBar *bar;
    u32 mode;
    u8 pageCount;
    u8 page;
    BmpWin *windows[WINDOW_COUNT];
    BOOL printing[WINDOW_COUNT];
    u32 state;
    s32 touched;
    s32 decided;
    u32 chars;
    u32 palette;
    u32 cellAnims;
    ClActor *arrows[ARROW_COUNT];
    u8 arrow;
    u32 arrowState;
    TCB *vblankTask;
};

// An arrow: its actor, the area and the keys that press it, the animation and the sound of a press, and the icon
// it counts as
typedef struct {
    ClActorSetup setup;
    u8 left;
    u8 right;
    u8 top;
    u8 bottom;
    u32 pressedAnim;
    u32 key;
    u32 se;
    u32 icon;
} ManualTouchBarArrow;

static void ManualTouchBar_VBlank(TCB *tcb, void *data);
static void ManualTouchBar_CreateBar(ManualTouchBar *bar);
static void ManualTouchBar_InitBG(ManualTouchBar *bar);
static void ManualTouchBar_CreateWindows(ManualTouchBar *bar);
static void ManualTouchBar_FreeWindows(ManualTouchBar *bar);
static void ManualTouchBar_UpdateWindows(ManualTouchBar *bar);
static void ManualTouchBar_ClearText(ManualTouchBar *bar);
static void ManualTouchBar_PrintCategoryPrompt(ManualTouchBar *bar);
static void ManualTouchBar_PrintTitlePrompt(ManualTouchBar *bar);
static void ManualTouchBar_PrintPage(ManualTouchBar *bar);
static void ManualTouchBar_CreateArrows(ManualTouchBar *bar);
static void ManualTouchBar_FreeArrows(ManualTouchBar *bar);
static void ManualTouchBar_UpdateArrows(ManualTouchBar *bar);
static void ManualTouchBar_Reset(ManualTouchBar *bar);

// Where the text starts in each mode but the last, which has none
static const u8 sManualTouchBarTextX[] = { 0, 0, 0, 32 };

static const ManualWindowSetup sManualTouchBarWindows[WINDOW_COUNT] = {
    { 7, 0, 0, 1, 1, 2, FALSE, 0, 0 },
    { 7, 2, 21, 26, 3, 2, FALSE, 0, 4 },
};

static const ManualTouchBarArrow sManualTouchBarArrows[ARROW_COUNT] = {
    { { 16, 168, 4, 0, 1 }, 16, 40, 168, 192, 12, PAD_BUTTON_L | PAD_KEY_LEFT, SEQ_SE_SELECT1, TOUCHBAR_ICON_LEFT },
    { { 80, 168, 5, 0, 1 }, 80, 104, 168, 192, 13, PAD_BUTTON_R | PAD_KEY_RIGHT, SEQ_SE_SELECT1, TOUCHBAR_ICON_RIGHT },
};

ManualTouchBar *ManualTouchBar_Create(ManualCommon *common) {
    ManualTouchBar *bar;

    GFL_OvlLoad(OVERLAY_APP_UI);
    bar = GFL_HeapAllocate(common->heapId, sizeof(ManualTouchBar), TRUE, "manual_touchbar.c", 235);
    bar->common = common;
    ManualTouchBar_CreateBar(bar);
    ManualTouchBar_InitBG(bar);
    ManualTouchBar_CreateWindows(bar);
    ManualTouchBar_CreateArrows(bar);
    bar->mode = MANUAL_TOUCHBAR_MODE_NONE;
    bar->pageCount = 0;
    bar->page = 0;
    ManualTouchBar_SetMode(bar, MANUAL_TOUCHBAR_MODE_TOP);
    ManualTouchBar_Reset(bar);
    bar->vblankTask = GFL_VBlankTCBAdd(ManualTouchBar_VBlank, bar, 1);
    return bar;
}

void ManualTouchBar_Free(ManualTouchBar *bar) {
    GFL_TCBRemove(bar->vblankTask);
    ManualTouchBar_FreeArrows(bar);
    ManualTouchBar_FreeWindows(bar);
    TouchBar_Free(bar->bar);
    GFL_HeapFree(bar);
    GFL_OvlUnload(OVERLAY_APP_UI);
}

void ManualTouchBar_Main(ManualTouchBar *bar) {
    bar->touched = TOUCHBAR_ICON_NONE;
    bar->decided = TOUCHBAR_ICON_NONE;
    ManualTouchBar_UpdateWindows(bar);
    if (bar->state == STATE_DECIDED) {
        bar->state = STATE_IDLE;
        return;
    }
    if (bar->state == STATE_IDLE || bar->state == STATE_ICON_TOUCHED) {
        TouchBar_Main(bar->bar);
        if (bar->state == STATE_IDLE) {
            u32 icon = TouchBar_GetTouched(bar->bar);

            if (icon != TOUCHBAR_ICON_NONE) {
                bar->state = STATE_ICON_TOUCHED;
                bar->touched = icon;
            }
        } else {
            u32 icon = TouchBar_GetDecided(bar->bar);

            if (icon != TOUCHBAR_ICON_NONE) {
                bar->state = STATE_DECIDED;
                bar->decided = icon;
            }
        }
    }
    if (bar->state == STATE_IDLE || bar->state == STATE_ARROW_TOUCHED) {
        ManualTouchBar_UpdateArrows(bar);
        if (bar->state == STATE_IDLE) {
            if (bar->arrowState == ARROW_STATE_TOUCHED) {
                bar->state = STATE_ARROW_TOUCHED;
                bar->touched = sManualTouchBarArrows[bar->arrow].icon;
            }
        } else {
            if (bar->arrowState == ARROW_STATE_DECIDED) {
                bar->state = STATE_DECIDED;
                bar->decided = sManualTouchBarArrows[bar->arrow].icon;
            }
        }
    }
}

void ManualTouchBar_SetMode(ManualTouchBar *bar, u32 mode) {
    if (bar->mode != mode) {
        switch (bar->mode) {
        case MANUAL_TOUCHBAR_MODE_TOP:
            TouchBar_SetIconVisible(bar->bar, TOUCHBAR_ICON_CLOSE, FALSE);
            break;
        case MANUAL_TOUCHBAR_MODE_CATEGORY:
            break;
        case MANUAL_TOUCHBAR_MODE_TITLE:
            break;
        case MANUAL_TOUCHBAR_MODE_EXPLAIN:
            ClActor_SetVisible(bar->arrows[0], FALSE);
            ClActor_SetVisible(bar->arrows[1], FALSE);
            break;
        case MANUAL_TOUCHBAR_MODE_NONE:
            break;
        }
        bar->mode = mode;
        ManualTouchBar_Reset(bar);
        switch (bar->mode) {
        case MANUAL_TOUCHBAR_MODE_TOP:
            TouchBar_SetIconVisible(bar->bar, TOUCHBAR_ICON_CLOSE, TRUE);
            ManualTouchBar_ClearText(bar);
            break;
        case MANUAL_TOUCHBAR_MODE_CATEGORY:
            ManualTouchBar_PrintCategoryPrompt(bar);
            break;
        case MANUAL_TOUCHBAR_MODE_TITLE:
            ManualTouchBar_PrintTitlePrompt(bar);
            break;
        case MANUAL_TOUCHBAR_MODE_EXPLAIN:
            break;
        case MANUAL_TOUCHBAR_MODE_NONE:
            break;
        }
    }
}

void ManualTouchBar_SetPage(ManualTouchBar *bar, u8 pageCount, u8 page) {
    bar->pageCount = pageCount;
    bar->page = page;
    if (bar->mode == MANUAL_TOUCHBAR_MODE_EXPLAIN) {
        if (bar->pageCount <= 1) {
            ClActor_SetVisible(bar->arrows[0], FALSE);
            ClActor_SetVisible(bar->arrows[1], FALSE);
            ManualTouchBar_ClearText(bar);
        } else {
            if (bar->page == 0) {
                ClActor_SetVisible(bar->arrows[0], FALSE);
                ClActor_SetVisible(bar->arrows[1], TRUE);
            } else if (bar->page == pageCount - 1) {
                ClActor_SetVisible(bar->arrows[0], TRUE);
                ClActor_SetVisible(bar->arrows[1], FALSE);
            } else {
                ClActor_SetVisible(bar->arrows[0], TRUE);
                ClActor_SetVisible(bar->arrows[1], TRUE);
            }
            ManualTouchBar_PrintPage(bar);
        }
    }
}

u32 ManualTouchBar_GetDecided(ManualTouchBar *bar) {
    return bar->decided;
}

u32 ManualTouchBar_GetTouched(ManualTouchBar *bar) {
    return bar->touched;
}

static void ManualTouchBar_VBlank(TCB *tcb, void *data) {
}

static void ManualTouchBar_CreateBar(ManualTouchBar *bar) {
    TouchBarSetup setup;
    TouchBarItem items[] = {
        { TOUCHBAR_ICON_RETURN, { 232, 168 } },
        { TOUCHBAR_ICON_CLOSE, { 200, 168 } },
    };

    MI_CpuFill8(&setup, 0, sizeof(TouchBarSetup));
    setup.items = items;
    setup.count = NELEMS(items);
    setup.unit = ManualGraphic_GetClActUnit(bar->common->graphic);
    setup.bg = 6;
    setup.bgPalette = 3;
    setup.objPalette = 1;
    setup.vramType = CLACT_VRAM_MAIN;
    bar->bar = TouchBar_Create(&setup, bar->common->heapId);
    TouchBar_SetIconVisible(bar->bar, TOUCHBAR_ICON_CLOSE, FALSE);
}

static void ManualTouchBar_InitBG(ManualTouchBar *bar) {
}

static void ManualTouchBar_CreateWindows(ManualTouchBar *bar) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        const ManualWindowSetup *setup = &sManualTouchBarWindows[i];

        bar->windows[i] = BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height,
                                               setup->palette, setup->fromEnd);
        GFL_BitmapFill(BmpWin_GetBitmap(bar->windows[i]), 0);
        BmpWin_FlushChar(bar->windows[i]);
        bar->printing[i] = FALSE;
    }
}

static void ManualTouchBar_FreeWindows(ManualTouchBar *bar) {
    u8 i;

    PrintQueue_Clear(bar->common->touchBarQueue);
    for (i = 0; i < WINDOW_COUNT; i++) {
        bar->printing[i] = FALSE;
        BmpWin_Free(bar->windows[i]);
    }
}

// Shows the windows whose text the queue has printed
static void ManualTouchBar_UpdateWindows(ManualTouchBar *bar) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        if (bar->printing[i]) {
            if (!PrintQueue_IsBitmapPending(bar->common->touchBarQueue, BmpWin_GetBitmap(bar->windows[i]))) {
                BmpWin *window = bar->windows[i];

                BmpWin_FlushChar(window);
                BmpWin_FlushMap(window);
                GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
                bar->printing[i] = FALSE;
            }
        }
    }
}

static void ManualTouchBar_ClearText(ManualTouchBar *bar) {
    PrintQueue_Clear(bar->common->touchBarQueue);
    GFL_BitmapFill(BmpWin_GetBitmap(bar->windows[WINDOW_TEXT]), 0);
    BmpWin_FlushChar(bar->windows[WINDOW_TEXT]);
    bar->printing[WINDOW_TEXT] = FALSE;
}

static void ManualTouchBar_PrintCategoryPrompt(ManualTouchBar *bar) {
    StrBuf *str;

    PrintQueue_Clear(bar->common->touchBarQueue);
    GFL_BitmapFill(BmpWin_GetBitmap(bar->windows[WINDOW_TEXT]), 0);
    str = GFL_MsgDataLoadStrbufNew(bar->common->msgSystem, Title7_Text_PleaseSelectCategory);
    PrintQueue_PrintColor(bar->common->touchBarQueue, BmpWin_GetBitmap(bar->windows[WINDOW_TEXT]),
                          sManualTouchBarTextX[MANUAL_TOUCHBAR_MODE_CATEGORY], 4, str, bar->common->font, TEXT_COLOR);
    GFL_StrBufFree(str);
    bar->printing[WINDOW_TEXT] = TRUE;
    ManualTouchBar_UpdateWindows(bar);
}

static void ManualTouchBar_PrintTitlePrompt(ManualTouchBar *bar) {
    StrBuf *str;

    PrintQueue_Clear(bar->common->touchBarQueue);
    GFL_BitmapFill(BmpWin_GetBitmap(bar->windows[WINDOW_TEXT]), 0);
    str = GFL_MsgDataLoadStrbufNew(bar->common->msgSystem, Title7_Text_PleaseSelectWhatWant);
    PrintQueue_PrintColor(bar->common->touchBarQueue, BmpWin_GetBitmap(bar->windows[WINDOW_TEXT]),
                          sManualTouchBarTextX[MANUAL_TOUCHBAR_MODE_TITLE], 4, str, bar->common->font, TEXT_COLOR);
    GFL_StrBufFree(str);
    bar->printing[WINDOW_TEXT] = TRUE;
    ManualTouchBar_UpdateWindows(bar);
}

static void ManualTouchBar_PrintPage(ManualTouchBar *bar) {
    WordSet *wordSet;
    StrBuf *format;
    StrBuf *str;

    PrintQueue_Clear(bar->common->touchBarQueue);
    GFL_BitmapFill(BmpWin_GetBitmap(bar->windows[WINDOW_TEXT]), 0);
    wordSet = GFL_WordSetSystemCreateDefault(bar->common->heapId);
    format = GFL_MsgDataLoadStrbufNew(bar->common->msgSystem, Title7_Text_Empty);
    str = GFL_StrBufCreate(16, bar->common->heapId);
    WordSetNumber(wordSet, 0, bar->page + 1, 1, 0, TRUE);
    WordSetNumber(wordSet, 1, bar->pageCount, 1, 0, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, str, format);
    PrintQueue_PrintColor(bar->common->touchBarQueue, BmpWin_GetBitmap(bar->windows[WINDOW_TEXT]),
                          sManualTouchBarTextX[MANUAL_TOUCHBAR_MODE_EXPLAIN], 4, str, bar->common->font, TEXT_COLOR);
    GFL_StrBufFree(str);
    GFL_StrBufFree(format);
    GFL_WordSetSystemFree(wordSet);
    bar->printing[WINDOW_TEXT] = TRUE;
    ManualTouchBar_UpdateWindows(bar);
}

static void ManualTouchBar_CreateArrows(ManualTouchBar *bar) {
    u8 i;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), bar->common->heapId);

    bar->palette = ClActSys_LoadPlttRange(arc, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_SUB, 4 * 0x20, 0, 3,
                                          bar->common->heapId);
    bar->chars =
        ClActSys_LoadChars(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_SUB, bar->common->heapId);
    bar->cellAnims = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTouchBarCellFile(0),
                                            AppMenuCommon_GetTouchBarAnimFile(0), bar->common->heapId);
    GFL_ArcToolFree(arc);
    for (i = 0; i < ARROW_COUNT; i++) {
        bar->arrows[i] = ClActor_Create(ManualGraphic_GetClActUnit(bar->common->graphic), bar->chars, bar->palette,
                                        bar->cellAnims, &sManualTouchBarArrows[i].setup, 1, bar->common->heapId);
        ClActor_SetVisible(bar->arrows[i], FALSE);
        ClActor_SetAutoAnim(bar->arrows[i], TRUE);
    }
}

static void ManualTouchBar_FreeArrows(ManualTouchBar *bar) {
    u8 i;

    for (i = 0; i < ARROW_COUNT; i++) {
        ClActor_Delete(bar->arrows[i]);
    }
    ClActSys_FreeCellAnims(bar->cellAnims);
    ClActSys_FreeChars(bar->chars);
    ClActSys_FreePltt(bar->palette);
}

// Reads the arrows: a press by key or touch plays the arrow's animation, and the arrow is decided once it ends
static void ManualTouchBar_UpdateArrows(ManualTouchBar *bar) {
    switch (bar->arrowState) {
    case ARROW_STATE_IDLE: {
        BOOL pressed = FALSE;
        u8 i;

        for (i = 0; i < ARROW_COUNT; i++) {
            if (ClActor_IsVisible(bar->arrows[i])) {
                const ManualTouchBarArrow *arrow = &sManualTouchBarArrows[i];
                u32 x;
                u32 y;

                if (GCTX_HIDGetTypedKeys() & arrow->key) {
                    bar->arrow = i;
                    GCTX_HIDSetUsingTouch(FALSE);
                    pressed = TRUE;
                    break;
                }
                if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
                    if (arrow->left <= x && x <= arrow->right && arrow->top <= y && y <= arrow->bottom) {
                        bar->arrow = i;
                        pressed = TRUE;
                        GCTX_HIDSetUsingTouch(TRUE);
                        break;
                    }
                }
            }
        }
        if (pressed) {
            ClActor_SetSequence(bar->arrows[bar->arrow], sManualTouchBarArrows[bar->arrow].pressedAnim);
            if (sManualTouchBarArrows[bar->arrow].se != 0) {
                GFL_SndSEPlay(sManualTouchBarArrows[bar->arrow].se);
            }
            bar->arrowState = ARROW_STATE_TOUCHED;
        }
        break;
    }
    case ARROW_STATE_TOUCHED:
        bar->arrowState = ARROW_STATE_ANIM;
        break;
    case ARROW_STATE_ANIM:
        if (!ClActor_IsAnimActive(bar->arrows[bar->arrow])) {
            ClActor_SetSequence(bar->arrows[bar->arrow], sManualTouchBarArrows[bar->arrow].setup.sequence);
            bar->arrowState = ARROW_STATE_DECIDED;
        }
        break;
    case ARROW_STATE_DECIDED:
        bar->arrowState = ARROW_STATE_IDLE;
        bar->arrow = ARROW_NONE;
        break;
    }
}

static void ManualTouchBar_Reset(ManualTouchBar *bar) {
    bar->state = STATE_IDLE;
    bar->touched = TOUCHBAR_ICON_NONE;
    bar->decided = TOUCHBAR_ICON_NONE;
    bar->arrow = ARROW_NONE;
    bar->arrowState = ARROW_STATE_IDLE;
}
