#include "types.h"
#include "app/wifibattlematch_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/bmp_menulist.h"
#include "system/bmp_menuwork.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"

// The task runner, message p_wk and list menu that the Wi-Fi and live competition screens share, and the match counts
// in the records (the file name is the ROM's own string)

#define MSGBOX_BG_COLOR 15

struct WifiBattleMatchTask {
    WifiBattleMatchTaskFunc func;
    u32 seq;
    void *work;
    u32 returnSeq;
};

struct WifiBattleMatchMsgBox {
    u32 unk0;
    Font *font;
    PrintStream *stream;
    TCBExManager *tcbManager;
    WaitIcon *waitIcon;
    BmpWin *window;
    StrBuf *strBuf;
    u16 bgColor;
    u16 heapId;
    PrintWindow printWindow;
    PrintQueue *queue;
    u32 mode;
    BOOL done;
    KeyCursor *p_keycursor;
    AppPrintsysCommon printsys;
};

struct WifiBattleMatchMenu {
    BmpWin *window;
    PrintQueue *queue;
    PrintWindow printWindow;
    BmpMenuList *list;
    ListMenuOption *options;
    s32 cancelValue;
};

static void WifiBattleMatchUtil_MsgBoxStart(WifiBattleMatchMsgBox *p_wk, u32 mode);
static void WifiBattleMatchUtil_MsgBoxStopWait(WifiBattleMatchMsgBox *p_wk);

static const BmpMenuListHeader sMenuListHeader = {
    NULL, NULL, NULL, 0, 0, 0, 13, 0, 3, 1, 15, 2, 0, 1, 0, 0, 0, NULL, 12, 13, NULL, NULL, NULL, NULL, 20,
};

WifiBattleMatchMsgBox *WifiBattleMatchMsgBox_Create(u32 bg, u32 palette, u16 framePalette, u16 frameChar,
                                                    PrintQueue *printQueue, Font *font, HeapID heapId) {
    WifiBattleMatchMsgBox *p_wk =
        GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchMsgBox), FALSE, "wifibattlematch_util.c", 95);
    BmpWin *window;

    MI_CpuFill8(p_wk, 0, sizeof(WifiBattleMatchMsgBox));
    p_wk->bgColor = MSGBOX_BG_COLOR;
    p_wk->font = font;
    p_wk->queue = PrintQueue_CreateDefault(heapId);
    p_wk->mode = 4;
    p_wk->heapId = heapId;
    p_wk->strBuf = GFL_StrBufCreate(0x300, heapId);
    p_wk->window = BmpWin_CreateDynamic(bg, 1, 19, 30, 4, palette, TRUE);
    PrintWindow_Init(&p_wk->printWindow, p_wk->window);
    GFL_BitmapFill(BmpWin_GetBitmap(p_wk->window), p_wk->bgColor);
    window = p_wk->window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    p_wk->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 1, 32);
    BmpWin_DrawFrame(p_wk->window, WINFRAME_TRANSFER_NOW, frameChar, framePalette);
    return p_wk;
}

void WifiBattleMatchMsgBox_Free(WifiBattleMatchMsgBox *p_wk) {
    if (p_wk->stream != NULL) {
        PrintStream_Delete(p_wk->stream);
        p_wk->stream = NULL;
    }
    if (p_wk->waitIcon != NULL) {
        WaitIcon_Free(p_wk->waitIcon);
        p_wk->waitIcon = NULL;
    }
    if (p_wk->p_keycursor != NULL) {
        KeyCursor_Free(p_wk->p_keycursor);
    }
    GFL_TCBExMgrFree(p_wk->tcbManager);
    BmpWin_ClearFrame(p_wk->window, WINFRAME_TRANSFER_NOW);
    BmpWin_Free(p_wk->window);
    GFL_StrBufFree(p_wk->strBuf);
    PrintQueue_Free(p_wk->queue);
    GFL_HeapFree(p_wk);
}

void WifiBattleMatchMsgBox_Update(WifiBattleMatchMsgBox *p_wk) {
    PrintQueue_Main(p_wk->queue);
    switch (p_wk->mode) {
    case 2:
        PrintWindow_Flush(&p_wk->printWindow, p_wk->queue);
        break;
    case 0:
        PrintWindow_Flush(&p_wk->printWindow, p_wk->queue);
        p_wk->done = PrintWindow_IsPrinted(&p_wk->printWindow);
        break;
    case 1:
        if (p_wk->stream != NULL) {
            if (p_wk->p_keycursor != NULL) {
                KeyCursor_Update(p_wk->p_keycursor, p_wk->stream, p_wk->window);
            }
            if (AppPrintsysCommon_Update(&p_wk->printsys, p_wk->stream)) {
                p_wk->done = TRUE;
            }
        }
        break;
    case 3:
        if (p_wk->stream != NULL) {
            BOOL done = FALSE;

            if (p_wk->p_keycursor != NULL) {
                KeyCursor_Update(p_wk->p_keycursor, p_wk->stream, p_wk->window);
                done = AppPrintsysCommon_Update(&p_wk->printsys, p_wk->stream);
            }
            if (done) {
                p_wk->done = TRUE;
            }
        }
        break;
    case 4:
        break;
    }
    GFL_TCBExMgrUpdate(p_wk->tcbManager);
}

void WifiBattleMatchMsgBox_Print(WifiBattleMatchMsgBox *p_wk, MsgData *msgData, u32 msgId, u32 mode) {
    GFL_MsgDataLoadStrbuf(msgData, msgId, p_wk->strBuf);
    WifiBattleMatchUtil_MsgBoxStart(p_wk, mode);
}

void WifiBattleMatchMsgBox_PrintStr(WifiBattleMatchMsgBox *p_wk, StrBuf *text, u32 mode) {
    GFL_StrBufCopy(p_wk->strBuf, text);
    WifiBattleMatchUtil_MsgBoxStart(p_wk, mode);
}

void WifiBattleMatchMsgBox_EnableKeyCursor(WifiBattleMatchMsgBox *p_wk) {
    if (p_wk->p_keycursor == NULL) {
        p_wk->p_keycursor = KeyCursor_Create(p_wk->bgColor, TRUE, FALSE, p_wk->heapId);
    }
}

// Prints the string that p_wk->strBuf holds, in a WIFIBATTLEMATCH_MSG_* way
static void WifiBattleMatchUtil_MsgBoxStart(WifiBattleMatchMsgBox *p_wk, u32 mode) {
    PrintQueue_Clear(p_wk->queue);
    GFL_BitmapFill(BmpWin_GetBitmap(p_wk->window), p_wk->bgColor);
    if (p_wk->stream != NULL) {
        PrintStream_Delete(p_wk->stream);
        p_wk->stream = NULL;
    }
    if (p_wk->p_keycursor != NULL) {
        KeyCursor_Free(p_wk->p_keycursor);
        p_wk->p_keycursor = NULL;
    }
    WifiBattleMatchUtil_MsgBoxStopWait(p_wk);
    switch (mode) {
    case WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON:
        p_wk->waitIcon = WaitIcon_Create(GFL_VBlankGetTCBMgr(), p_wk->window, p_wk->bgColor, 16, p_wk->heapId);
        PrintWindow_PrintNoColor(&p_wk->printWindow, p_wk->queue, 0, 0, p_wk->strBuf, p_wk->font);
        p_wk->mode = 0;
        break;
    case WIFIBATTLEMATCH_MSG_QUEUE:
        PrintWindow_PrintNoColor(&p_wk->printWindow, p_wk->queue, 0, 0, p_wk->strBuf, p_wk->font);
        p_wk->mode = 0;
        break;
    case WIFIBATTLEMATCH_MSG_STREAM_KEY:
        GFL_ASSERT(p_wk->p_keycursor == NULL);
        AppPrintsysCommon_Init(&p_wk->printsys, APP_PRINTSYS_COMMON_KEYS);
        p_wk->p_keycursor = KeyCursor_Create(p_wk->bgColor, TRUE, FALSE, p_wk->heapId);
        p_wk->stream = PrintStream_Create(p_wk->window, 0, 0, p_wk->strBuf, p_wk->font, TextSpeed_GetCurrentWait(),
                                          p_wk->tcbManager, 0, p_wk->heapId, p_wk->bgColor);
        p_wk->mode = 1;
        break;
    case WIFIBATTLEMATCH_MSG_STREAM:
        GFL_ASSERT(p_wk->p_keycursor == NULL);
        AppPrintsysCommon_Init(&p_wk->printsys, APP_PRINTSYS_COMMON_KEYS);
        p_wk->stream = PrintStream_Create(p_wk->window, 0, 0, p_wk->strBuf, p_wk->font, TextSpeed_GetCurrentWait(),
                                          p_wk->tcbManager, 0, p_wk->heapId, p_wk->bgColor);
        p_wk->mode = 3;
        break;
    }
    p_wk->done = FALSE;
}

BOOL WifiBattleMatchMsgBox_IsDone(WifiBattleMatchMsgBox *p_wk) {
    return p_wk->done;
}

// Ends the wait of a message: it counts as done, and its wait icon goes
static void WifiBattleMatchUtil_MsgBoxStopWait(WifiBattleMatchMsgBox *p_wk) {
    p_wk->done = TRUE;
    if (p_wk->waitIcon != NULL) {
        WaitIcon_Free(p_wk->waitIcon);
        p_wk->waitIcon = NULL;
        BmpWin_FlushMap(p_wk->window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(p_wk->window));
    }
}

void WifiBattleMatchMsgBox_Clear(WifiBattleMatchMsgBox *p_wk) {
    GFL_BitmapFill(BmpWin_GetBitmap(p_wk->window), p_wk->bgColor);
    BmpWin_FlushChar(p_wk->window);
}

WifiBattleMatchMenu *WifiBattleMatchMenu_CreateBottomRight(const WifiBattleMatchMenuParam *param, HeapID heapId) {
    u8 height = param->count * 2;
    u8 y = 17 - height;

    return WifiBattleMatchMenu_Create(param, 19, y, 12, height, heapId);
}

WifiBattleMatchMenu *WifiBattleMatchMenu_Create(const WifiBattleMatchMenuParam *param, u8 x, u8 y, u8 width, u8 height,
                                                HeapID heapId) {
    WifiBattleMatchMenu *menu =
        GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchMenu), FALSE, "wifibattlematch_util.c", 475);
    BmpMenuListHeader header;
    BmpWin *window;
    u32 i;

    MI_CpuFill8(menu, 0, sizeof(WifiBattleMatchMenu));
    menu->queue = param->printQueue;
    menu->cancelValue = param->cancelValue;
    menu->window = BmpWin_CreateDynamic(param->bg, x, y, width, height, param->palette, TRUE);
    BmpWin_DrawFrame(menu->window, WINFRAME_TRANSFER_NONE, param->frameChar, param->framePalette);
    window = menu->window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    PrintWindow_Init(&menu->printWindow, menu->window);
    menu->options = ListMenuCore_CreateOptionList(param->count, heapId);
    for (i = 0; i < param->count; i++) {
        ListMenuCore_AppendMsgOption(menu->options, param->msgData, param->msgIds[i], i, heapId);
    }
    header = sMenuListHeader;
    header.options = menu->options;
    header.count = param->count;
    header.maxShown = param->count;
    header.msgData = param->msgData;
    header.printWindow = &menu->printWindow;
    header.queue = param->printQueue;
    header.font = param->font;
    menu->list = BmpMenuList_Create(&header, 0, param->cursorRow, heapId);
    BmpMenuList_LoadCursor(menu->list, heapId);
    if (param->canCancel) {
        BmpMenuList_SetCancelDisabled(menu->list, FALSE);
    } else {
        BmpMenuList_SetCancelDisabled(menu->list, TRUE);
    }
    return menu;
}

void WifiBattleMatchMenu_Free(WifiBattleMatchMenu *menu) {
    BmpMenuList_Free(menu->list, NULL, NULL);
    ListMenuCore_FreeOptionList(menu->options);
    BmpWin_ClearFrame(menu->window, WINFRAME_TRANSFER_NOW);
    BmpWin_ClearScreen(menu->window);
    BmpWin_Free(menu->window);
    GFL_HeapFree(menu);
}

s32 WifiBattleMatchMenu_Update(WifiBattleMatchMenu *menu) {
    s32 result;

    PrintWindow_Flush(&menu->printWindow, menu->queue);
    result = BmpMenuList_Update(menu->list);
    if (result == BMPMENULIST_CANCEL) {
        result = menu->cancelValue;
    }
    return result;
}

WifiBattleMatchTask *WifiBattleMatchTask_Create(void *work, WifiBattleMatchTaskFunc func, HeapID heapId) {
    WifiBattleMatchTask *task =
        GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchTask), FALSE, "wifibattlematch_util.c", 638);

    MI_CpuFill8(task, 0, sizeof(WifiBattleMatchTask));
    task->work = work;
    WifiBattleMatchTask_SetFunc(task, func);
    return task;
}

void WifiBattleMatchTask_Free(WifiBattleMatchTask *task) {
    GFL_HeapFree(task);
}

void WifiBattleMatchTask_Run(WifiBattleMatchTask *task) {
    if (task->func != NULL) {
        task->func(task, &task->seq, task->work);
    }
}

BOOL WifiBattleMatchTask_IsEnded(WifiBattleMatchTask *task) {
    return task->func == NULL;
}

void WifiBattleMatchTask_SetFunc(WifiBattleMatchTask *task, WifiBattleMatchTaskFunc func) {
    task->func = func;
    task->seq = 0;
}

void WifiBattleMatchTask_End(WifiBattleMatchTask *task) {
    WifiBattleMatchTask_SetFunc(task, NULL);
}

void WifiBattleMatchTask_SetReturnSeq(WifiBattleMatchTask *task, u32 seq) {
    task->returnSeq = seq;
}

void WifiBattleMatchTask_ReturnToSeq(WifiBattleMatchTask *task) {
    task->seq = task->returnSeq;
}

// Counts a match in the records, by kind
void WifiBattleMatch_RecordResult(GameData *gameData, u32 mode) {
    GameRecords *records = getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData));

    switch (mode) {
    case 1:
    case 4:
        RecordAddOne(records, 0x12);
        break;
    case 0:
    case 3:
        RecordAddOne(records, 0x13);
        break;
    case 2:
        RecordAddOne(records, 0x14);
        break;
    }
    RecordAddOne(records, 0x11);
}

void WifiBattleMatch_RecordBattleTime(GameData *gameData, u32 mode) {
    RecordSave_UpdateMatchSeconds(getRecordBlkAddress(GameData_GetSaveControl(gameData)));
    if (mode != 3 && mode != 6) {
        GameData_IncCounter(gameData);
    }
}
