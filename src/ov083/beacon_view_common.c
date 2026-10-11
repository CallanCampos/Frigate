#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/beacon_view_common.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "system/app_taskmenu.h"
#include "system/bmp_oam.h"
#include "system/game_beacon.h"
#include "system/printsys.h"

// What the beacon view's touch screens share, named after the ROM's "beacon_view_common.c". The names are ours

// The buttons of a menu layout: how many, the x of the first in tiles (each is 8 wide), and their strings
typedef struct {
    u8 count;
    u8 x;
    u8 strIds[BEACON_VIEW_MENU_BUTTON_MAX];
} BeaconViewMenuLayout;

typedef struct {
    u32 unused;
    BeaconViewWin *win;
    int *pending;
} BeaconViewWinTask;

typedef struct {
    u32 unused;
    BeaconViewOam *oam;
    int *pending;
} BeaconViewOamTask;

typedef struct {
    u8 back;
    u8 bg;
    u8 frames;
    u8 frame;
    s8 y;
    s8 step;
    int *pending;
} BeaconViewBGSlideTask;

static void BeaconViewWin_StartFlushTask(BeaconViewWin *win, int *pending);
static void BeaconViewWin_FlushTask(TCBEx *task, void *data);
static void BeaconViewOam_PrintEx(BeaconViewOam *oam, const StrBuf *strbuf, u16 color, int *pending, BOOL now,
                                  BOOL clear);
static void BeaconViewOam_StartFlushTask(BeaconViewOam *oam, int *pending);
static void BeaconViewOam_FlushTask(TCBEx *task, void *data);
static void BeaconView_BGSlideTask(TCBEx *task, void *data);

static const BeaconViewMenuLayout sBeaconViewMenuLayouts[] = {
    { 3, 8, { 2, 0, 1 } },
    { 2, 16, { 0, 1 } },
    { 2, 16, { 2, 3 } },
};

void BeaconView_PlaySE(u32 se) {
    GFL_SndSEPlay(se);
}

void BeaconView_SetActorPos(ClActor *actor, s16 x, s16 y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c140(actor, &pos, 0);
}

void BeaconView_SetActorAnim(ClActor *actor, u16 sequence) {
    func_0204c488(actor, sequence);
    func_0204c56c(actor);
}

void BeaconView_SetPrintTimeLimit(PrintQueue *queue, BOOL longer) {
    if (longer) {
        func_02021a34(queue, 2000);
    } else {
        func_02021a34(queue, 500);
    }
}

BeaconViewMenu *BeaconViewMenu_Create(u8 bg, u8 palette, Font *font, PrintQueue *queue, HeapID heapId) {
    int i;
    BeaconViewMenu *menu = GFL_HeapAllocate(heapId, sizeof(BeaconViewMenu), TRUE, "beacon_view_common.c", 150);

    menu->heapId = heapId;
    menu->res = AppTaskMenuRes_Create(bg, palette, font, queue, heapId);
    menu->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0012, heapId);
    for (i = 0; i < BEACON_VIEW_MENU_STR_MAX; i++) {
        menu->strs[i] = GFL_MsgDataLoadStrbufNew(menu->msgData, i);
    }
    for (i = 0; i < BEACON_VIEW_MENU_BUTTON_MAX; i++) {
        menu->buttons[i].item.str = NULL;
        menu->buttons[i].item.color = PRINT_COLOR(14, 15, 3);
        menu->buttons[i].item.type = 0;
        menu->buttons[i].win = NULL;
    }
    return menu;
}

void BeaconViewMenu_Free(BeaconViewMenu *menu) {
    int i;

    for (i = 0; i < BEACON_VIEW_MENU_BUTTON_MAX; i++) {
        if (menu->buttons[i].win != NULL) {
            AppTaskMenuWin_Free(menu->buttons[i].win);
        }
    }
    for (i = 0; i < BEACON_VIEW_MENU_STR_MAX; i++) {
        GFL_StrBufFree(menu->strs[i]);
    }
    GFL_MsgDataFree(menu->msgData);
    AppTaskMenuRes_Free(menu->res);
    GFL_HeapFree(menu);
}

void BeaconViewMenu_Open(BeaconViewMenu *menu, u8 type) {
    int i;

    menu->type = type;
    menu->buttonCount = sBeaconViewMenuLayouts[menu->type].count;
    for (i = 0; i < menu->buttonCount; i++) {
        menu->buttons[i].item.str = menu->strs[sBeaconViewMenuLayouts[menu->type].strIds[i]];
        menu->buttons[i].win =
            AppTaskMenuWin_CreateEx(menu->res, &menu->buttons[i].item, sBeaconViewMenuLayouts[menu->type].x + i * 8, 21,
                                    8, 3, TRUE, FALSE, menu->heapId);
    }
}

AppTaskMenuWin *BeaconViewMenu_GetButton(BeaconViewMenu *menu, int index) {
    return menu->buttons[index].win;
}

int BeaconViewMenu_Update(BeaconViewMenu *menu) {
    int i;
    int touched = -1;

    for (i = 0; i < menu->buttonCount; i++) {
        if (AppTaskMenuWin_IsUpdatingText(menu->buttons[i].win) == TRUE) {
            AppTaskMenuWin_Update(menu->buttons[i].win);
            return -1;
        }
    }
    for (i = 0; i < menu->buttonCount; i++) {
        if (AppTaskMenuWin_IsTouched(menu->buttons[i].win)) {
            if (i < menu->buttonCount - 1) {
                BeaconView_PlaySE(SEQ_SE_DECIDE1);
            } else {
                AppTaskMenuWin_SetFlashing(menu->buttons[i].win, TRUE);
                BeaconView_PlaySE(SEQ_SE_DECIDE1);
            }
            touched = i;
        }
        AppTaskMenuWin_Update(menu->buttons[i].win);
    }
    return touched;
}

BOOL BeaconViewMenu_WaitFlash(BeaconViewMenu *menu, int index) {
    int i;

    AppTaskMenuWin_Update(menu->buttons[index].win);
    if (AppTaskMenuWin_IsFlashFinished(menu->buttons[index].win)) {
        for (i = 0; i < menu->buttonCount; i++) {
            AppTaskMenuWin_Free(menu->buttons[i].win);
            menu->buttons[i].win = NULL;
        }
        return TRUE;
    }
    return FALSE;
}

void BeaconViewMenu_Close(BeaconViewMenu *menu) {
    int i;

    for (i = 0; i < menu->buttonCount; i++) {
        AppTaskMenuWin_ClearScreen(menu->buttons[i].win);
        AppTaskMenuWin_Free(menu->buttons[i].win);
        menu->buttons[i].win = NULL;
        menu->buttons[i].item.str = NULL;
    }
    menu->type = 0;
    menu->buttonCount = 0;
}

BOOL BeaconView_WaitTimerOrTouch(u8 *timer, u8 bg) {
    u32 x;
    u32 y;
    // Any color but 0
    u16 colors = 0xfffe;

    if ((*timer)-- == 0) {
        *timer = 0;
        return TRUE;
    }
    if (func_0203dac8(&x, &y) && GFL_BGSysIsPixelOfColor(bg, x, y + 64, &colors)) {
        *timer = 0;
        return TRUE;
    }
    return FALSE;
}

u32 func_ov083_021eab38(const GameBeacon *beacon) {
    int type = func_02013eac(beacon);

    if (type == 0 || type >= 60) {
        return 22;
    }
    return type + 21;
}

void BeaconViewOam_Init(BeaconViewOam *oam, BmpOamActorSetup *setup, BmpOamSys *sys, PrintQueue *queue, Font *font,
                        TCBExManager *tcbManager) {
    oam->bitmap = setup->bitmap;
    oam->font = font;
    oam->queue = queue;
    oam->tcbManager = tcbManager;
    oam->actor = BmpOam_ActorAdd(sys, setup);
}

void BeaconViewOam_Free(BeaconViewOam *oam) {
    BmpOam_ActorSetDrawEnable(oam->actor, FALSE);
    BmpOam_ActorDel(oam->actor);
    GFL_BitmapFree(oam->bitmap);
    sys_memset(oam, 0, sizeof(BeaconViewOam));
}

void BeaconViewWin_PrintEx(BeaconViewWin *win, const StrBuf *strbuf, s16 x, s16 y, u8 fill, u16 color, int *pending,
                           BOOL now, BOOL clear) {
    if (clear) {
        GFL_BitmapFill(win->bitmap, fill);
    }
    if (now) {
        GFL_TextRendererDrawToBitmapEx(win->bitmap, x, y, strbuf, win->font, color);
        BmpWin_TransferNow(win->window);
    } else {
        PrintWindow_Print(&win->printWindow, win->queue, x, y, strbuf, win->font, color);
        BeaconViewWin_StartFlushTask(win, pending);
    }
}

void BeaconViewWin_Print(BeaconViewWin *win, const StrBuf *strbuf, s16 x, s16 y, u8 fill, u16 color, int *pending,
                         BOOL now) {
    BeaconViewWin_PrintEx(win, strbuf, x, y, fill, color, pending, now, TRUE);
}

static void BeaconViewWin_StartFlushTask(BeaconViewWin *win, int *pending) {
    BeaconViewWinTask *data =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(win->tcbManager, BeaconViewWin_FlushTask, sizeof(BeaconViewWinTask), 0));

    sys_memset(data, 0, sizeof(BeaconViewWinTask));
    data->win = win;
    if (pending != NULL) {
        data->pending = pending;
        (*pending)++;
    }
}

static void BeaconViewWin_FlushTask(TCBEx *task, void *data) {
    BeaconViewWinTask *flush = data;
    BeaconViewWin *win = flush->win;
    PrintQueue *queue = win->queue;
    PrintWindow *printWindow = &win->printWindow;

    PrintWindow_Flush(printWindow, queue);
    if (PrintWindow_IsPrinted(printWindow)) {
        BmpWin_FlushMap(flush->win->window);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(flush->win->window));
        if (flush->pending != NULL) {
            (*flush->pending)--;
        }
        GFL_TCBExRequestEnd(task);
    }
}

static void BeaconViewOam_PrintEx(BeaconViewOam *oam, const StrBuf *strbuf, u16 color, int *pending, BOOL now,
                                  BOOL clear) {
    if (clear) {
        GFL_BitmapFill(oam->bitmap, 0);
    }
    if (now) {
        GFL_TextRendererDrawToBitmapEx(oam->bitmap, 0, 0, strbuf, oam->font, color);
        BmpOam_ActorBmpTrans(oam->actor);
    } else {
        func_02021c7c(oam->queue, oam->bitmap, 0, 0, strbuf, oam->font, color);
        BeaconViewOam_StartFlushTask(oam, pending);
    }
}

void BeaconViewOam_Print(BeaconViewOam *oam, const StrBuf *strbuf, u16 color, int *pending, BOOL now) {
    BeaconViewOam_PrintEx(oam, strbuf, color, pending, now, TRUE);
}

static void BeaconViewOam_StartFlushTask(BeaconViewOam *oam, int *pending) {
    BeaconViewOamTask *data =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(oam->tcbManager, BeaconViewOam_FlushTask, sizeof(BeaconViewOamTask), 0));

    sys_memset(data, 0, sizeof(BeaconViewOamTask));
    data->oam = oam;
    if (pending != NULL) {
        data->pending = pending;
        (*pending)++;
    }
}

static void BeaconViewOam_FlushTask(TCBEx *task, void *data) {
    BeaconViewOamTask *flush = data;

    if (!func_02021c1c(flush->oam->queue, flush->oam->bitmap)) {
        BmpOam_ActorBmpTrans(flush->oam->actor);
        if (flush->pending != NULL) {
            (*flush->pending)--;
        }
        GFL_TCBExRequestEnd(task);
    }
}

void BeaconView_StartBGSlide(TCBExManager *tcbManager, u8 bg, u8 back, int *pending) {
    BeaconViewBGSlideTask *data =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(tcbManager, BeaconView_BGSlideTask, sizeof(BeaconViewBGSlideTask), 0));

    sys_memset(data, 0, sizeof(BeaconViewBGSlideTask));
    data->bg = bg;
    data->back = back;
    if (data->back == FALSE) {
        data->y = 0;
        data->step = 16;
    } else {
        data->y = 64;
        data->step = -16;
    }
    data->frames = 4;
    data->pending = pending;
    (*pending)++;
}

static void BeaconView_BGSlideTask(TCBEx *task, void *data) {
    BeaconViewBGSlideTask *slide = data;

    if (slide->frame++ < slide->frames) {
        slide->y += slide->step;
        GFL_BGSysMoveBG(slide->bg, BG_MOVE_SET_Y, slide->y);
    } else {
        (*slide->pending)--;
        GFL_TCBExRequestEnd(task);
    }
}
