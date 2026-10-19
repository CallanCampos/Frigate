#include "types.h"
#include "app/manual.h"
#include "app/manual_common.h"
#include "app/manual_data.h"
#include "app/manual_explain.h"
#include "app/manual_touchbar.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "system/printsys.h"

// The pages of a title of the manual, named after the ROM's "manual_explain.c": the title's name and a page's text
// on the lower screen, and the page's picture on the upper one. The touch bar's arrows turn the pages

#define TITLE_COLOR PRINT_COLOR(15, 2, 0)
#define TEXT_COLOR PRINT_COLOR(1, 2, 0)

enum {
    WINDOW_UNUSED,
    WINDOW_TITLE,
    WINDOW_TEXT,
    WINDOW_COUNT,
};

enum {
    STATE_IDLE,
    STATE_END,
    STATE_TOUCH_BAR,
};

struct ManualExplain {
    ManualExplainParam *param;
    ManualCommon *common;
    u8 page;
    BOOL pageChanged;
    BmpWin *windows[WINDOW_COUNT];
    PrintQueue *queues[WINDOW_COUNT];
    BOOL printing[WINDOW_COUNT];
    TCB *vblankTask;
    u32 state;
    // Counts down to the end once the screen is left
    s8 endTimer;
};

static void ManualExplain_VBlank(TCB *tcb, void *data);
static void ManualExplain_LoadGraphics(ManualExplain *explain);
static void ManualExplain_QueueScreen(ManualExplain *explain);
static void ManualExplain_FreeGraphics(ManualExplain *explain);
static void ManualExplain_CreateWindows(ManualExplain *explain);
static void ManualExplain_FreeWindows(ManualExplain *explain);
static void ManualExplain_UpdateWindows(ManualExplain *explain);
static void ManualExplain_PrintPage(ManualExplain *explain);

static const ManualWindowSetup sManualExplainWindows[WINDOW_COUNT] = {
    { 5, 0, 0, 1, 1, 2, FALSE, 0, 0 },
    { 5, 1, 0, 30, 2, 2, FALSE, 0, 1 },
    { 5, 0, 2, 32, 19, 2, FALSE, 4, 6 },
};

ManualExplain *ManualExplain_Create(ManualExplainParam *param, ManualCommon *common) {
    ManualExplain *explain = GFL_HeapAllocate(common->heapId, sizeof(ManualExplain), TRUE, "manual_explain.c", 156);

    explain->param = param;
    param->result = 0;
    explain->common = common;
    explain->page = 0;
    explain->pageChanged = FALSE;
    ManualExplain_LoadGraphics(explain);
    ManualExplain_QueueScreen(explain);
    ManualExplain_CreateWindows(explain);
    explain->vblankTask = GFL_VBlankTCBAdd(ManualExplain_VBlank, explain, 1);
    explain->state = STATE_IDLE;
    explain->endTimer = -1;
    ManualTouchBar_SetMode(explain->common->touchBar, MANUAL_TOUCHBAR_MODE_EXPLAIN);
    ManualTouchBar_SetPage(explain->common->touchBar, explain->param->pageCount, explain->page);
    ManualExplain_PrintPage(explain);
    return explain;
}

void ManualExplain_Free(ManualExplain *explain) {
    ManualCommon_LoadImage(explain->common, ManualData_GetDefaultImage(explain->common->data));
    GFL_TCBRemove(explain->vblankTask);
    GFL_HeapFree(explain);
}

// Returns whether the screen has ended
BOOL ManualExplain_Main(ManualExplain *explain) {
    BOOL ended = FALSE;

    if (explain->endTimer == 0) {
        ended = TRUE;
    } else if (explain->endTimer > 0) {
        if (explain->endTimer == 1) {
            ManualExplain_FreeWindows(explain);
            ManualExplain_FreeGraphics(explain);
        }
        explain->endTimer--;
    }
    if (explain->state != STATE_END) {
        ManualExplain_UpdateWindows(explain);
    }
    if (explain->state == STATE_IDLE || explain->state == STATE_TOUCH_BAR) {
        ManualTouchBar_Main(explain->common->touchBar);
        if (explain->state == STATE_IDLE) {
            u32 touched = ManualTouchBar_GetTouched(explain->common->touchBar);

            if (touched != -1) {
                explain->state = STATE_TOUCH_BAR;
                if (touched == MANUAL_TOUCHBAR_ICON_PREV) {
                    if (explain->page != 0) {
                        explain->page--;
                        explain->pageChanged = TRUE;
                    }
                } else if (touched == MANUAL_TOUCHBAR_ICON_NEXT) {
                    if (explain->page < explain->param->pageCount - 1) {
                        explain->page++;
                        explain->pageChanged = TRUE;
                    }
                }
                if (explain->pageChanged) {
                    ManualExplain_PrintPage(explain);
                }
            }
        } else {
            u32 decided = ManualTouchBar_GetDecided(explain->common->touchBar);

            if (decided != -1) {
                switch (decided) {
                case MANUAL_TOUCHBAR_ICON_RETURN:
                    explain->param->result = 0;
                    explain->state = STATE_END;
                    explain->endTimer = 1;
                    break;
                case MANUAL_TOUCHBAR_ICON_PREV:
                case MANUAL_TOUCHBAR_ICON_NEXT:
                    explain->state = STATE_IDLE;
                    if (explain->pageChanged) {
                        ManualTouchBar_SetPage(explain->common->touchBar, explain->param->pageCount, explain->page);
                        explain->pageChanged = FALSE;
                    }
                    break;
                }
            }
        }
    }
    return ended;
}

static void ManualExplain_VBlank(TCB *tcb, void *data) {
}

static void ManualExplain_LoadGraphics(ManualExplain *explain) {
    GFL_G2DIOLoadArcNCLRDefault(explain->common->arc, MANUAL_ARC_BG_NCLR, PALTYPE_SUB_BG, 0, 2 * 0x20,
                                explain->common->heapId);
    GFL_BGSysLoadArcNCGRStatic(explain->common->arc, MANUAL_ARC_BG_NCGR, 4, 0, 0, FALSE, explain->common->heapId);
    GFL_G2DIOLoadNSCRAsync(explain->common->arc, MANUAL_ARC_EXPLAIN_NSCR, 4, 0, 0, 0, FALSE, explain->common->heapId);
}

static void ManualExplain_QueueScreen(ManualExplain *explain) {
    GFL_BGSysQueueScrLoad(4);
}

static void ManualExplain_FreeGraphics(ManualExplain *explain) {
}

static void ManualExplain_CreateWindows(ManualExplain *explain) {
    u8 i;
    StrBuf *str;

    for (i = 0; i < WINDOW_COUNT; i++) {
        const ManualWindowSetup *setup = &sManualExplainWindows[i];

        explain->windows[i] = BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height,
                                                   setup->palette, setup->fromEnd);
        GFL_BitmapFill(BmpWin_GetBitmap(explain->windows[i]), 0);
        BmpWin_FlushChar(explain->windows[i]);
        explain->printing[i] = FALSE;
    }
    explain->queues[WINDOW_UNUSED] = explain->common->titleQueue;
    explain->queues[WINDOW_TITLE] = explain->common->titleQueue;
    explain->queues[WINDOW_TEXT] = explain->common->queue;
    str = GFL_MsgDataLoadStrbufNew(explain->common->msgManual, explain->param->titleMsg);
    PrintQueue_PrintColor(explain->queues[WINDOW_TITLE], BmpWin_GetBitmap(explain->windows[WINDOW_TITLE]), 0, 1, str,
                          explain->common->font, TITLE_COLOR);
    GFL_StrBufFree(str);
    explain->printing[WINDOW_TITLE] = TRUE;
    ManualExplain_UpdateWindows(explain);
}

static void ManualExplain_FreeWindows(ManualExplain *explain) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        explain->printing[i] = FALSE;
        PrintQueue_Clear(explain->queues[i]);
        BmpWin_ClearScreen(explain->windows[i]);
        BmpWin_Free(explain->windows[i]);
    }
    GFL_BGSysQueueScrLoad(5);
}

// Shows the windows whose text their queue has printed
static void ManualExplain_UpdateWindows(ManualExplain *explain) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        if (explain->printing[i]) {
            if (!PrintQueue_IsBitmapPending(explain->queues[i], BmpWin_GetBitmap(explain->windows[i]))) {
                BmpWin *window = explain->windows[i];

                BmpWin_FlushChar(window);
                BmpWin_FlushMap(window);
                GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
                explain->printing[i] = FALSE;
            }
        }
    }
}

static void ManualExplain_PrintPage(ManualExplain *explain) {
    StrBuf *str;

    PrintQueue_Clear(explain->queues[WINDOW_TEXT]);
    GFL_BitmapFill(BmpWin_GetBitmap(explain->windows[WINDOW_TEXT]), 0);
    str = GFL_MsgDataLoadStrbufNew(explain->common->msgManual, explain->param->pages[explain->page].msg);
    PrintQueue_PrintColor(explain->queues[WINDOW_TEXT], BmpWin_GetBitmap(explain->windows[WINDOW_TEXT]), 4, 6, str,
                          explain->common->font, TEXT_COLOR);
    GFL_StrBufFree(str);
    explain->printing[WINDOW_TEXT] = TRUE;
    ManualExplain_UpdateWindows(explain);
    ManualCommon_LoadImage(explain->common, explain->param->pages[explain->page].image);
}
