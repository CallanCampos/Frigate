#include "types.h"
#include "app/ui/ui_scene.h"
#include "app/ui/frame_list.h"
#include "app/t_download.h"
#include "app/t_download/t_download_graphic.h"
#include "app/t_download/t_download_local.h"
#include "app/t_download/t_download_save.h"
#include "app/t_download/t_download_util.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"

// The Pokémon World Tournament's downloaded tournaments: the tournaments received over Wi-Fi, which can be saved in one
// of the save's three slots, the tournaments in the slots, and the details of one. The name is ours, after the ROM's
// "t_download_graphic.c" and "t_download_save.c". Each screen steps through its own seq: 0 starts it, 100 is its main
// loop and 10000 ends the process

#define T_DOWNLOAD_SEQ_END 10000

// The bits of the work's flags
// The save has no tournament
#define T_DOWNLOAD_FLAG_NO_SAVED 0x1
// The details are to be printed again
#define T_DOWNLOAD_FLAG_REDRAW 0x200
#define T_DOWNLOAD_FLAG_RECEIVED_SHOWN 0x400
#define T_DOWNLOAD_FLAG_SELECT_SHOWN 0x800
#define T_DOWNLOAD_FLAG_SELECTED 0x4000
#define T_DOWNLOAD_FLAG_PAGE 0x8000
#define T_DOWNLOAD_FLAG_SAVED_FULL 0x10000
#define T_DOWNLOAD_FLAG_SAVED 0x20000
#define T_DOWNLOAD_FLAG_DELETED 0x40000
#define T_DOWNLOAD_FLAG_SELECT_MOVED 0x100000
#define T_DOWNLOAD_FLAG_SAVED_MOVED 0x200000
// The page of the tab can't change, as it has only one
#define T_DOWNLOAD_FLAG_ONE_PAGE 0x400000
#define T_DOWNLOAD_FLAG_TAB 0x800000
#define T_DOWNLOAD_FLAG_LIST_PRINTED 0x1000000

// The bits of the work's objFlags
#define T_DOWNLOAD_OBJ_FLAG_RESULT 0xc0
#define T_DOWNLOAD_OBJ_FLAG_YES 0x80
#define T_DOWNLOAD_OBJ_FLAG_LIST_MOVED 0x8000

// How a button was pressed
#define T_DOWNLOAD_INPUT_KEY 1
#define T_DOWNLOAD_INPUT_TOUCH 2

static BOOL TDownload_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL TDownload_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL TDownload_Main(GameProc *proc, u32 *state, void *param, void *work);
static void TDownload_InitWork(TDownloadWork *wk);
static void TDownload_Update(TDownloadWork *wk);
static void TDownload_ExitWork(TDownloadWork *wk);
static void TDownload_InitMode(TDownloadWork *wk);
static void TDownload_RunMode(TDownloadWork *wk);
static void TDownload_FlushWindows(TDownloadWork *wk);
static void TDownload_InitSelect(TDownloadWork *wk);
static void TDownload_SelectMain(TDownloadWork *wk);
static BOOL TDownload_SelectTouch(TDownloadWork *wk);
static BOOL TDownload_SelectKeys(TDownloadWork *wk);
static BOOL TDownload_WaitInput(TDownloadWork *wk);
static void TDownload_Select(TDownloadWork *wk);
static void TDownload_ExitSelect(TDownloadWork *wk);
static void TDownload_InitDetail(TDownloadWork *wk);
static void TDownload_DetailMain(TDownloadWork *wk);
static BOOL TDownload_DetailTouch(TDownloadWork *wk);
static BOOL TDownload_DetailKeys(TDownloadWork *wk);
static void TDownload_Detail(TDownloadWork *wk);
static void TDownload_ExitDetail(TDownloadWork *wk);
static void TDownload_InitReceived(TDownloadWork *wk);
static void TDownload_ReceivedMain(TDownloadWork *wk);
static BOOL TDownload_ReceivedTouch(TDownloadWork *wk);
static BOOL TDownload_ReceivedKeys(TDownloadWork *wk);
static void TDownload_Received(TDownloadWork *wk);
static void TDownload_ExitReceived(TDownloadWork *wk);
static void TDownload_InitSaved(TDownloadWork *wk);
static void TDownload_SavedMain(TDownloadWork *wk);
static BOOL TDownload_SavedTouch(TDownloadWork *wk);
static BOOL TDownload_SavedKeys(TDownloadWork *wk);
static void TDownload_Saved(TDownloadWork *wk);
static void TDownload_ExitSaved(TDownloadWork *wk);

const GameProcFunctions T_DOWNLOAD_PROC_FUNCTIONS = {
    TDownload_Init,
    TDownload_Main,
    TDownload_Exit,
};

static void (*sTDownloadModeInits[T_DOWNLOAD_MODE_COUNT])(TDownloadWork *wk) = {
    TDownload_InitSaved,
    TDownload_InitReceived,
    TDownload_InitSelect,
    TDownload_InitDetail,
};

static void (*sTDownloadModeMains[T_DOWNLOAD_MODE_COUNT])(TDownloadWork *wk) = {
    TDownload_Saved,
    TDownload_Received,
    TDownload_Select,
    TDownload_Detail,
};

static BOOL TDownload_Init(GameProc *proc, u32 *state, void *param, void *work) {
    TDownloadWork *wk;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WBT_RECORD, 0x40000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(TDownloadWork), HEAPID_WBT_RECORD);
    sys_memset(wk, 0, sizeof(TDownloadWork));
    wk->heapId = HEAPID_WBT_RECORD;
    wk->param = param;
    wk->graphic = TDownloadGraphic_Create(0, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->wordSet = GFL_WordSetSystemCreate(8, 40, wk->heapId);
    wk->msgData[0] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0404, wk->heapId);
    wk->msgData[1] = NULL;
    wk->printQueue = func_02021998(wk->heapId);
    wk->tcbEx = GFL_TCBExMgrCreate(wk->heapId, wk->heapId, 32, 32);
    wk->stream = NULL;
    wk->streamStr = NULL;
    wk->waitIcon = NULL;
    TDownload_InitWork(wk);
    GFL_WipeSet(0, 1, 1, 0, 6, 1, wk->heapId);
    return TRUE;
}

static BOOL TDownload_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    TDownloadWork *wk = work;
    HeapID heapId;
    int i;

    TDownload_ExitWork(wk);
    func_ov326_021a2580(wk);
    KeyCursor_Free(wk->keyCursor);
    AppTaskMenuRes_Free(wk->taskMenuRes);
    for (i = 0; i < 2; i++) {
        if (wk->msgData[i] != NULL) {
            GFL_MsgDataFree(wk->msgData[i]);
            wk->msgData[i] = NULL;
        }
    }
    func_ov326_021a2864(wk);
    func_02021a18(wk->printQueue);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_FontFree(wk->font);
    GFL_TCBExMgrFree(wk->tcbEx);
    TDownloadGraphic_Delete(wk->graphic);
    heapId = wk->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL TDownload_Main(GameProc *proc, u32 *state, void *param, void *work) {
    TDownloadWork *wk = work;

    switch (*state) {
    case 0:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 1:
        if (wk->seq == T_DOWNLOAD_SEQ_END && func_02021c0c(wk->printQueue) == TRUE) {
            (*state)++;
        }
        break;
    case 2:
        GFL_WipeSet(0, 0, 0, 0, 6, 1, wk->heapId);
        (*state)++;
        break;
    case 3:
        if (GFL_WipeIsFinished()) {
            return TRUE;
        }
        break;
    }
    TDownload_Update(wk);
    GFL_TCBExMgrUpdate(wk->tcbEx);
    func_ov326_021a2850(wk);
    TDownload_FlushWindows(wk);
    TDownloadGraphic_Main(wk->graphic);
    TDownloadGraphic_Begin3D(wk->graphic);
    TDownloadGraphic_End3D(wk->graphic);
    return FALSE;
}

static void TDownload_InitWork(TDownloadWork *wk) {
    int i;

    wk->seq = 0;
    wk->mode = wk->param->unk8;
    wk->flags = 0;
    wk->objFlags = 0;
    wk->bgScroll = 0;
    wk->listCount = 0;
    wk->mapFlushMask = 0;
    wk->saveFlags = 0;
    wk->copySrc = 0;
    wk->copyDest = 0;
    wk->slotPos = 0;
    wk->slotCursor = 0;
    for (i = 0; i < 3; i++) {
        wk->listPos[i] = 0;
        wk->listCursor[i] = 0;
        wk->listScroll[i] = 0;
    }
    for (i = 0; i < T_DOWNLOAD_TABS; i++) {
        wk->itemCount[i] = 0;
        wk->pageCount[i] = 0;
        wk->page[i] = 0;
    }
    wk->lastSlot = -1;
    wk->palAnimCount = 0;
    wk->tab = 0;
    wk->returnMode = -1;
    wk->seqAfterList = 0;
    for (i = 0; i < T_DOWNLOAD_WINDOW_COUNT; i++) {
        wk->windows[i].window = NULL;
    }
    wk->list = NULL;
    wk->record = NULL;
    for (i = 0; i < T_DOWNLOAD_SAVE_SLOTS; i++) {
        wk->savedData[i] = NULL;
    }
    wk->objPos = GFL_ArcSysReadHeapNew(ARCID_T_DOWNLOAD, 16, wk->heapId);
    wk->unk284 = NULL;
    for (i = 0; i < T_DOWNLOAD_RECEIVED_MAX; i++) {
        wk->receivedData[i] = NULL;
    }
    wk->slotCount = func_ov326_021a24ac(wk);
    wk->slotPos = func_ov326_021a2a44(wk, 0);
    wk->receivedCount = 0;
    if (wk->mode == T_DOWNLOAD_MODE_RECEIVED) {
        wk->receivedCount = func_ov326_021a25a4(wk);
    }
    func_ov326_021a0a88(wk);
    AppPrintsysCommon_Init(&wk->printWait, 6);
    wk->keyCursor = KeyCursor_Create(15, TRUE, TRUE, wk->heapId);
    TDownload_InitMode(wk);
}

static void TDownload_Update(TDownloadWork *wk) {
    TDownload_RunMode(wk);
    func_ov326_021a1508(wk);
    func_ov326_021a0a84(wk);
}

static void TDownload_ExitWork(TDownloadWork *wk) {
    int i;

    if (wk->objFlags & T_DOWNLOAD_OBJ_FLAG_YES) {
        if (wk->param->unkC != NULL) {
            *wk->param->unkC = TRUE;
        }
    } else {
        if (wk->param->unkC != NULL) {
            *wk->param->unkC = FALSE;
        }
    }
    if (wk->flags & T_DOWNLOAD_FLAG_SELECTED) {
        if (wk->param->unk10 != NULL) {
            *wk->param->unk10 = wk->slotPos + 1;
        }
    } else {
        if (wk->param->unk10 != NULL) {
            *wk->param->unk10 = 0;
        }
    }
    func_ov326_021a1898(wk);
    for (i = 0; i < T_DOWNLOAD_RECEIVED_MAX; i++) {
        if (wk->receivedData[i] != NULL) {
            GFL_HeapFree(wk->receivedData[i]);
            wk->receivedData[i] = NULL;
        }
    }
    GFL_HeapFree(wk->objPos);
    if (wk->unk284 != NULL) {
        GFL_HeapFree(wk->unk284);
    }
    func_ov326_021a0ac8(wk);
}

static void TDownload_InitMode(TDownloadWork *wk) {
    if (wk->mode >= 0 && wk->mode < T_DOWNLOAD_MODE_COUNT) {
        sTDownloadModeInits[wk->mode](wk);
    }
}

static void TDownload_RunMode(TDownloadWork *wk) {
    if (wk->mode >= 0 && wk->mode < T_DOWNLOAD_MODE_COUNT) {
        sTDownloadModeMains[wk->mode](wk);
    }
}

static void TDownload_FlushWindows(TDownloadWork *wk) {
    int i;

    func_02021a3c(wk->printQueue);
    for (i = 0; i < T_DOWNLOAD_WINDOW_COUNT; i++) {
        if (wk->windows[i].window != NULL) {
            PrintWindow_Flush(&wk->windows[i], wk->printQueue);
            if (wk->mapFlushMask & (1 << i)) {
                BmpWin_FlushMap(wk->windows[i].window);
                GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->windows[i].window));
                wk->mapFlushMask &= ~(1 << i);
            }
        }
    }
}

static void TDownload_InitSelect(TDownloadWork *wk) {
    int i;

    GFL_BGSysMoveBG(2, BG_MOVE_SET_Y, 0);
    wk->listCount = wk->lastSlot + 1;
    wk->flags &= ~T_DOWNLOAD_FLAG_NO_SAVED;
    if (wk->listCount == 0) {
        wk->flags |= T_DOWNLOAD_FLAG_NO_SAVED;
    }
    for (i = 0; i < wk->listCount; i++) {
        if (wk->saveFlags & (1 << (i + 4))) {
            func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 6, TRUE);
        }
    }
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 0, FALSE);
    func_ov326_021a20b4(wk, wk->mode, 0);
    if (wk->flags & T_DOWNLOAD_FLAG_NO_SAVED) {
        func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
    } else {
        func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, TRUE);
        func_ov326_021a1a10(wk, 1, wk->slotPos);
        func_ov326_021a1a78(wk, FALSE);
    }
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 2, FALSE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 0, FALSE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 1, FALSE);
    func_ov326_021a0d4c(wk, wk->mode);
    for (i = 0; i < wk->listCount; i++) {
        if (wk->saveFlags & (1 << (i + 4))) {
            func_ov326_021a21d0(wk, 0, i);
        }
    }
    if (wk->flags & T_DOWNLOAD_FLAG_NO_SAVED) {
        wk->seq = 500;
        return;
    }
    if (!(wk->flags & T_DOWNLOAD_FLAG_SELECT_SHOWN)) {
        func_ov326_021a1388(wk, 8, 0, TRUE);
        func_ov326_021a2138(wk, TRUE);
        func_ov326_021a2194(wk, TRUE);
        wk->seq = 10;
        wk->flags |= T_DOWNLOAD_FLAG_SELECT_SHOWN;
    }
    func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
    func_ov326_021a1544(wk, 0, wk->slotPos);
}

static void TDownload_SelectMain(TDownloadWork *wk) {
    if (func_02021c0c(wk->printQueue)) {
        if (TDownload_SelectTouch(wk) == FALSE) {
            TDownload_SelectKeys(wk);
        }
        if (wk->flags & T_DOWNLOAD_FLAG_SELECT_MOVED) {
            func_ov326_021a1a10(wk, 1, wk->slotPos);
            func_ov326_021a1544(wk, 0, wk->slotPos);
            wk->flags &= ~T_DOWNLOAD_FLAG_SELECT_MOVED;
        }
    }
}

static BOOL TDownload_SelectTouch(TDownloadWork *wk) {
    int touched;
    s16 slot;
    BOOL result = FALSE;
    int input = 0;
    BOOL moved = FALSE;

    touched = func_ov326_021a2114(wk, FALSE, wk->mode);
    if (touched == 3 || touched == 4 || touched == 5) {
        slot = touched - 3;
        if (wk->saveFlags & (1 << (slot + 4))) {
            wk->slotPos = slot;
            moved = TRUE;
            func_0203d564(TRUE);
        }
    }
    if (moved == FALSE && touched == 0) {
        func_ov326_021a14d4(wk, 3, 0);
        input = T_DOWNLOAD_INPUT_TOUCH;
        func_0203d564(TRUE);
    }
    if (moved == TRUE) {
        wk->flags |= T_DOWNLOAD_FLAG_SELECT_MOVED;
        func_ov326_021a1a78(wk, TRUE);
        wk->seq = 130;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_0203d564(TRUE);
    }
    if (input != 0) {
        result = TRUE;
        func_ov326_021a2298(wk, FALSE, TRUE, FALSE);
        wk->seq = 110;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    }
    return result;
}

static BOOL TDownload_SelectKeys(TDownloadWork *wk) {
    int input = 0;
    BOOL moved = FALSE;
    BOOL done = FALSE;
    BOOL result = FALSE;
    int pos;

    if (func_0203d554() == TRUE) {
        if (GCTX_HIDGetPressedKeys()) {
            func_ov326_021a2298(wk, TRUE, input, input);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        done = TRUE;
    }
    if (done == FALSE) {
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            pos = func_ov326_021a2a98(wk, FALSE, wk->slotPos);
            if (pos != wk->slotPos) {
                moved = TRUE;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            wk->slotPos = pos;
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            pos = func_ov326_021a2a98(wk, TRUE, wk->slotPos);
            if (pos != wk->slotPos) {
                moved = TRUE;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            wk->slotPos = pos;
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            func_ov326_021a1a78(wk, TRUE);
            wk->seq = 130;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            func_0203d564(FALSE);
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            func_ov326_021a14d4(wk, 3, 0);
            input = T_DOWNLOAD_INPUT_KEY;
            func_ov326_021a2298(wk, TRUE, FALSE, FALSE);
        }
    }
    if (moved == TRUE) {
        wk->flags |= T_DOWNLOAD_FLAG_SELECT_MOVED;
        func_ov326_021a2298(wk, TRUE, FALSE, FALSE);
    }
    if (input == T_DOWNLOAD_INPUT_KEY) {
        result = TRUE;
        wk->seq = 110;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    }
    return result;
}

static BOOL TDownload_WaitInput(TDownloadWork *wk) {
    BOOL result = FALSE;

    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        result = TRUE;
        func_ov326_021a2298(wk, TRUE, FALSE, FALSE);
    } else if (func_0203da48() == TRUE) {
        result = TRUE;
        func_ov326_021a2298(wk, FALSE, TRUE, FALSE);
    }
    return result;
}

static void TDownload_Select(TDownloadWork *wk) {
    int result;
    int i;

    switch (wk->seq) {
    case 0:
        wk->seq = 100;
    case 10:
        if (func_ov326_021a1338(wk) == FALSE) {
            func_ov326_021a1370(wk, TRUE);
            func_ov326_021a2138(wk, FALSE);
            func_ov326_021a2194(wk, FALSE);
            func_ov326_021a20b4(wk, wk->mode, 0);
            wk->seq = 100;
            func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
        }
        break;
    case 100:
        TDownload_SelectMain(wk);
        break;
    case 110:
        if (func_ov326_021a23d4(wk, 0) == TRUE) {
            wk->seq = T_DOWNLOAD_SEQ_END;
        }
        break;
    case 120:
        if (func_ov326_021a23d4(wk, 1) == TRUE) {
            wk->objFlags |= T_DOWNLOAD_OBJ_FLAG_LIST_MOVED;
            wk->seq = 100;
        }
        break;
    case 130:
        if (func_02021c0c(wk->printQueue) == TRUE) {
            func_ov326_021a2298(wk, FALSE, FALSE, TRUE);
            wk->seq = 200;
        }
        break;
    case 200:
        if (func_ov326_021a1134(wk, T_DOWNLOAD_ACTOR_OBJ, 2) == FALSE) {
            func_ov326_021a1388(wk, 7, 1, FALSE);
            func_ov326_021a1188(wk, 0);
            func_ov326_021a2138(wk, TRUE);
            func_ov326_021a2194(wk, TRUE);
            wk->seq = 210;
        }
        break;
    case 210:
        result = func_ov326_021a128c(wk);
        if (result != -1) {
            func_ov326_021a1274(wk);
            func_ov326_021a2138(wk, FALSE);
            func_ov326_021a2194(wk, FALSE);
            if (result == 0) {
                func_ov326_021a1370(wk, TRUE);
                wk->seq = T_DOWNLOAD_SEQ_END;
                wk->flags |= T_DOWNLOAD_FLAG_SELECTED;
            } else if (result == 1) {
                wk->seq = 300;
                func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
                for (i = 0; i < 3; i++) {
                    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 6, FALSE);
                }
            } else {
                func_ov326_021a1370(wk, TRUE);
                wk->seq = 100;
                func_ov326_021a20b4(wk, wk->mode, 0);
                func_ov326_021a1a78(wk, FALSE);
                func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
            }
        }
        break;
    case 300:
        func_ov326_021a1370(wk, FALSE);
        wk->returnMode = wk->mode;
        TDownload_ExitSelect(wk);
        wk->seq = 0;
        wk->mode = T_DOWNLOAD_MODE_DETAIL;
        TDownload_InitMode(wk);
        break;
    case 500:
        if (TDownload_WaitInput(wk) == TRUE) {
            func_ov326_021a14d4(wk, 3, 0);
            wk->seq = 110;
        }
        break;
    case T_DOWNLOAD_SEQ_END:
        break;
    }
}

static void TDownload_ExitSelect(TDownloadWork *wk) {
    int i;

    for (i = 0; i < 3; i++) {
        func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 6, FALSE);
    }
    func_ov326_021a0da4(wk, wk->mode);
}

static void TDownload_InitDetail(TDownloadWork *wk) {
    int i;

    GFL_BGSysMoveBG(2, BG_MOVE_SET_Y, 0);
    for (i = 0; i < 3; i++) {
        func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 3, TRUE);
    }
    func_ov326_021a2298(wk, FALSE, FALSE, TRUE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 0, FALSE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 1, FALSE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 2, FALSE);
    func_ov326_021a0b84(wk, wk->mode);
    func_ov326_021a1c0c(wk, TRUE);
    func_ov326_021a0d4c(wk, wk->mode);
    wk->tab = 0;
    wk->flags |= T_DOWNLOAD_FLAG_REDRAW;
    wk->flags |= T_DOWNLOAD_FLAG_TAB;
    wk->palAnimCount = 0x8000;
    if (wk->returnMode == T_DOWNLOAD_MODE_RECEIVED) {
        func_ov326_021a1f34(wk, 1, wk->listPos[2]);
        func_ov326_021a1cf8(wk, 1, wk->listPos[2], wk->tab);
    } else if (wk->returnMode == T_DOWNLOAD_MODE_SELECT) {
        func_ov326_021a1f34(wk, 0, wk->slotPos);
        func_ov326_021a1cf8(wk, 0, wk->slotPos, wk->tab);
    } else {
        func_ov326_021a1f34(wk, 0, wk->slotCursor);
        func_ov326_021a1cf8(wk, 0, wk->slotCursor, wk->tab);
    }
    func_ov326_021a20b4(wk, wk->mode, 0);
    func_ov326_021a2350(wk, 1, 0, wk->pageCount[wk->tab] - 1, wk->page[wk->tab]);
}

static void TDownload_DetailMain(TDownloadWork *wk) {
    if (func_02021c0c(wk->printQueue)) {
        if (TDownload_DetailTouch(wk) == FALSE) {
            TDownload_DetailKeys(wk);
        }
        if (wk->flags & T_DOWNLOAD_FLAG_REDRAW) {
            if (wk->returnMode == T_DOWNLOAD_MODE_RECEIVED) {
                func_ov326_021a1cf8(wk, 1, wk->listPos[2], wk->tab);
            } else if (wk->returnMode == T_DOWNLOAD_MODE_SELECT) {
                func_ov326_021a1cf8(wk, 0, wk->slotPos, wk->tab);
            } else {
                func_ov326_021a1cf8(wk, 0, wk->slotCursor, wk->tab);
            }
            if (wk->flags & T_DOWNLOAD_FLAG_TAB) {
                func_ov326_021a1c34(wk, wk->tab);
                wk->flags &= ~T_DOWNLOAD_FLAG_TAB;
            }
            func_ov326_021a1c78(wk, wk->tab);
            wk->flags &= ~T_DOWNLOAD_FLAG_ONE_PAGE;
            if (wk->pageCount[wk->tab] == 1) {
                wk->flags |= T_DOWNLOAD_FLAG_ONE_PAGE;
            }
            wk->flags &= ~T_DOWNLOAD_FLAG_REDRAW;
        }
        if (wk->flags & T_DOWNLOAD_FLAG_PAGE) {
            func_ov326_021a20b4(wk, wk->mode, wk->tab);
            wk->flags &= ~T_DOWNLOAD_FLAG_PAGE;
        }
    }
}

static BOOL TDownload_DetailTouch(TDownloadWork *wk) {
    int input = 0;
    BOOL result = FALSE;
    int button = 0;
    int page = wk->page[wk->tab];
    int touched = func_ov326_021a2114(wk, FALSE, wk->mode);
    int held = func_ov326_021a2114(wk, TRUE, wk->mode);
    int newPage;

    if (touched == 1) {
        func_ov326_021a14d4(wk, 3, 0);
        button = 1;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        func_0203d564(TRUE);
    } else if (touched == 3) {
        if (!(wk->flags & T_DOWNLOAD_FLAG_ONE_PAGE) && wk->page[wk->tab] != 0) {
            wk->page[wk->tab]--;
            if (wk->page[wk->tab] <= 0) {
                wk->page[wk->tab] = 0;
            }
            func_ov326_021a14d4(wk, 5, 0);
            button = 2;
            input = 1;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_0203d564(TRUE);
        }
    } else if (touched == 2) {
        if (!(wk->flags & T_DOWNLOAD_FLAG_ONE_PAGE) && wk->page[wk->tab] != wk->pageCount[wk->tab] - 1) {
            wk->page[wk->tab]++;
            if (wk->page[wk->tab] >= wk->pageCount[wk->tab] - 1) {
                wk->page[wk->tab] = wk->pageCount[wk->tab] - 1;
            }
            func_ov326_021a14d4(wk, 4, 0);
            button = 3;
            input = 1;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_0203d564(TRUE);
        }
    } else if (touched == 4) {
        wk->tab = 0;
        input = 1;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        wk->flags |= T_DOWNLOAD_FLAG_TAB;
        func_0203d564(TRUE);
    } else if (touched == 5) {
        wk->tab = 1;
        input = 1;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        wk->flags |= T_DOWNLOAD_FLAG_TAB;
        func_0203d564(TRUE);
    } else if (touched == 6) {
        input = 1;
        wk->tab = 2;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        wk->flags |= T_DOWNLOAD_FLAG_TAB;
        func_0203d564(TRUE);
    } else if (held == 7) {
        if (wk->pageCount[wk->tab] > 1) {
            input = 2;
            func_0203d564(TRUE);
        }
    }
    if (input == 1) {
        func_ov326_021a2028(wk, wk->tab, wk->page[wk->tab], FALSE);
        wk->flags |= T_DOWNLOAD_FLAG_REDRAW;
        wk->flags |= T_DOWNLOAD_FLAG_PAGE;
        func_ov326_021a2350(wk, 1, 0, wk->pageCount[wk->tab] - 1, wk->page[wk->tab]);
    } else if (input == 2) {
        func_ov326_021a2028(wk, wk->tab, 0, TRUE);
        newPage = func_ov326_021a1fec(wk, wk->tab);
        if (newPage != -1 && newPage != page) {
            wk->page[wk->tab] = newPage;
            wk->flags |= T_DOWNLOAD_FLAG_REDRAW;
            func_ov326_021a2350(wk, 1, 0, wk->pageCount[wk->tab] - 1, wk->page[wk->tab]);
            GFL_SndSEPlay(SEQ_SE_SYS_06);
        }
    }
    if (button != 0) {
        result = TRUE;
        if (button == 1) {
            wk->seq = 110;
            func_0203d564(result);
        } else if (button == 2) {
            wk->seq = 120;
        } else if (button == 3) {
            wk->seq = 130;
        }
    }
    return result;
}

static BOOL TDownload_DetailKeys(TDownloadWork *wk) {
    BOOL result = FALSE;
    int button = 0;
    int input = 0;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        func_ov326_021a14d4(wk, 3, 0);
        button = 1;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
        if (--wk->tab < 0) {
            wk->tab = 2;
        }
        input = 1;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        wk->flags |= T_DOWNLOAD_FLAG_TAB;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
        if (++wk->tab >= T_DOWNLOAD_TABS) {
            wk->tab = 0;
        }
        input = 1;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        wk->flags |= T_DOWNLOAD_FLAG_TAB;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        if (!(wk->flags & T_DOWNLOAD_FLAG_ONE_PAGE) && wk->page[wk->tab] != 0) {
            wk->page[wk->tab]--;
            if (wk->page[wk->tab] <= 0) {
                wk->page[wk->tab] = 0;
            }
            func_ov326_021a14d4(wk, 5, 0);
            button = 2;
            input = 2;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        if (!(wk->flags & T_DOWNLOAD_FLAG_ONE_PAGE) && wk->page[wk->tab] != wk->pageCount[wk->tab] - 1) {
            wk->page[wk->tab]++;
            if (wk->page[wk->tab] >= wk->pageCount[wk->tab] - 1) {
                wk->page[wk->tab] = wk->pageCount[wk->tab] - 1;
            }
            func_ov326_021a14d4(wk, 4, 0);
            button = 3;
            input = 2;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_R) {
        if (!(wk->flags & T_DOWNLOAD_FLAG_ONE_PAGE)) {
            wk->page[wk->tab] = wk->pageCount[wk->tab] - 1;
            input = 3;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_L) {
        if (!(wk->flags & T_DOWNLOAD_FLAG_ONE_PAGE)) {
            wk->page[wk->tab] = 0;
            input = 3;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
    }
    if (input != 0) {
        func_ov326_021a2028(wk, wk->tab, wk->page[wk->tab], FALSE);
        wk->flags |= T_DOWNLOAD_FLAG_REDRAW;
        if (input == 1) {
            wk->flags |= T_DOWNLOAD_FLAG_PAGE;
        }
        if (input == 1 || input == 3) {
            func_ov326_021a2350(wk, 1, 0, wk->pageCount[wk->tab] - 1, wk->page[wk->tab]);
        }
    }
    if (button != 0) {
        result = TRUE;
        if (button == 1) {
            wk->seq = 110;
            func_0203d564(FALSE);
        } else if (button == 2) {
            wk->seq = 120;
        } else if (button == 3) {
            wk->seq = 130;
        }
    }
    return result;
}

static void TDownload_Detail(TDownloadWork *wk) {
    int i;

    switch (wk->seq) {
    case 0:
        wk->seq = 100;
    case 100:
        TDownload_DetailMain(wk);
        break;
    case 110:
        if (func_ov326_021a23d4(wk, 0) == TRUE) {
            if (wk->returnMode == -1) {
                wk->seq = T_DOWNLOAD_SEQ_END;
            } else {
                wk->seq = 300;
                for (i = 0; i < 3; i++) {
                    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 3, FALSE);
                }
                func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 5, FALSE);
                func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 4, FALSE);
            }
        }
        break;
    case 120:
        if (func_ov326_021a23d4(wk, 1) == TRUE) {
            wk->seq = 100;
            func_ov326_021a2350(wk, 1, 0, wk->pageCount[wk->tab] - 1, wk->page[wk->tab]);
        }
        break;
    case 130:
        if (func_ov326_021a23d4(wk, 1) == TRUE) {
            wk->seq = 100;
            func_ov326_021a2350(wk, 1, 0, wk->pageCount[wk->tab] - 1, wk->page[wk->tab]);
        }
        break;
    case 300:
        func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
        TDownload_ExitDetail(wk);
        wk->seq = 0;
        wk->mode = wk->returnMode;
        wk->returnMode = -1;
        TDownload_InitMode(wk);
        break;
    case T_DOWNLOAD_SEQ_END:
        break;
    }
    func_ov326_021a1b00(wk);
}

static void TDownload_ExitDetail(TDownloadWork *wk) {
    int i;

    func_ov326_021a1c0c(wk, FALSE);
    for (i = 0; i < 3; i++) {
        func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 3, FALSE);
    }
    func_ov326_021a0da4(wk, wk->mode);
    GFL_BGSysClearBG(2);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 0, TRUE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 1, TRUE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 5, FALSE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 4, FALSE);
}

static void TDownload_InitReceived(TDownloadWork *wk) {
    wk->listCount = wk->receivedCount;
    wk->listPos[0] = wk->listPos[2];
    wk->listCursor[0] = wk->listCursor[2];
    wk->listScroll[0] = wk->listScroll[2];
    GFL_BGSysSetBGEnabled(2, FALSE);
    wk->flags &= ~T_DOWNLOAD_FLAG_LIST_PRINTED;
    func_ov326_021a16e8(wk, wk->mode);
    if (FrameList_Draw(wk->list) == FALSE) {
        wk->flags |= T_DOWNLOAD_FLAG_LIST_PRINTED;
    }
    FrameList_SetShownRows(wk->list, 7);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, TRUE);
    func_ov326_021a1a78(wk, FALSE);
    if (wk->listCount <= 6) {
        func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 0, FALSE);
    } else {
        func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 0, TRUE);
        func_ov326_021a1cb4(wk);
    }
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 0, TRUE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 1, TRUE);
    func_ov326_021a2350(wk, 0, 0, wk->listCount - 1, wk->listPos[0]);
    func_ov326_021a1a10(wk, 0, wk->listCursor[0]);
    func_ov326_021a20b4(wk, wk->mode, 0);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 2, FALSE);
    func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
    func_ov326_021a1544(wk, 1, wk->listPos[0]);
    wk->seq = 5;
    wk->seqAfterList = 100;
    if (!(wk->flags & T_DOWNLOAD_FLAG_RECEIVED_SHOWN)) {
        func_ov326_021a1388(wk, 8, 5, TRUE);
        func_ov326_021a2138(wk, TRUE);
        func_ov326_021a2194(wk, TRUE);
        wk->seqAfterList = 10;
        wk->flags |= T_DOWNLOAD_FLAG_RECEIVED_SHOWN;
    }
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
    if (wk->flags & T_DOWNLOAD_FLAG_LIST_PRINTED) {
        wk->seq = wk->seqAfterList;
        GFL_BGSysSetBGEnabled(2, TRUE);
        if (func_0203d554() == FALSE) {
            func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, TRUE);
            func_ov326_021a1a10(wk, 0, wk->listCursor[0]);
        }
    }
}

static void TDownload_ReceivedMain(TDownloadWork *wk) {
    BOOL handled = FALSE;
    int result;

    if (func_02021c0c(wk->printQueue)) {
        if (func_0203d554() == TRUE && GCTX_HIDGetPressedKeys()) {
            func_ov326_021a2298(wk, FALSE, FALSE, TRUE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        result = func_ov326_021a17e4(wk, func_ov326_021a17c8(wk));
        if (result != 0) {
            handled = TRUE;
            if (result != 2) {
                func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
            }
        }
        if (result == 2) {
            handled = TRUE;
            func_ov326_021a1a78(wk, TRUE);
            wk->seq = 130;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if (result == 3) {
            wk->seq = 120;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            handled = TRUE;
        }
        if (handled == FALSE && TDownload_ReceivedTouch(wk) == FALSE) {
            TDownload_ReceivedKeys(wk);
        }
        if (wk->objFlags & T_DOWNLOAD_OBJ_FLAG_LIST_MOVED) {
            func_ov326_021a1544(wk, 1, wk->listPos[0]);
            func_ov326_021a1a10(wk, 0, wk->listCursor[0]);
            func_ov326_021a2350(wk, 0, 0, wk->listCount - 1, wk->listPos[0]);
            wk->objFlags &= ~T_DOWNLOAD_OBJ_FLAG_LIST_MOVED;
        }
        func_ov326_021a1cb4(wk);
    }
}

static BOOL TDownload_ReceivedTouch(TDownloadWork *wk) {
    BOOL pressed = FALSE;
    BOOL result = FALSE;

    if (func_ov326_021a2114(wk, FALSE, wk->mode) == 1) {
        func_ov326_021a14d4(wk, 3, 0);
        pressed = TRUE;
        func_0203d564(TRUE);
    }
    if (pressed) {
        wk->seq = 110;
        result = TRUE;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        func_ov326_021a2298(wk, FALSE, TRUE, FALSE);
    }
    return result;
}

static BOOL TDownload_ReceivedKeys(TDownloadWork *wk) {
    int button = 0;
    BOOL result = FALSE;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        func_ov326_021a14d4(wk, 3, 0);
        button = 1;
    }
    if (button != 0) {
        result = TRUE;
        func_ov326_021a2298(wk, TRUE, FALSE, FALSE);
        if (button == 1) {
            wk->seq = 110;
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else {
            wk->seq = 120;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
    }
    return result;
}

static void TDownload_Received(TDownloadWork *wk) {
    int result;

    switch (wk->seq) {
    case 0:
        wk->seq = 100;
    case 5:
        if (!(wk->flags & T_DOWNLOAD_FLAG_LIST_PRINTED)) {
            if (FrameList_Draw(wk->list) == FALSE) {
                wk->flags |= T_DOWNLOAD_FLAG_LIST_PRINTED;
                wk->seq = wk->seqAfterList;
                GFL_BGSysSetBGEnabled(2, TRUE);
                if (func_0203d554() == FALSE) {
                    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, TRUE);
                    func_ov326_021a1a10(wk, 0, wk->listCursor[0]);
                }
            }
        } else {
            wk->seq = wk->seqAfterList;
        }
        break;
    case 10:
        if (func_ov326_021a1338(wk) == FALSE) {
            func_ov326_021a1370(wk, TRUE);
            func_ov326_021a2138(wk, FALSE);
            func_ov326_021a2194(wk, FALSE);
            func_ov326_021a20b4(wk, wk->mode, 0);
            wk->seq = 100;
            func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
        }
        break;
    case 100:
        TDownload_ReceivedMain(wk);
        break;
    case 110:
        if (func_ov326_021a23d4(wk, 0) == TRUE) {
            func_ov326_021a1388(wk, 7, 7, FALSE);
            func_ov326_021a1188(wk, 2);
            func_ov326_021a2138(wk, TRUE);
            func_ov326_021a2194(wk, TRUE);
            wk->seq = 115;
        }
        break;
    case 115:
        result = func_ov326_021a128c(wk);
        if (result != -1) {
            func_ov326_021a1274(wk);
            func_ov326_021a2138(wk, FALSE);
            func_ov326_021a2194(wk, FALSE);
            if (result == 0) {
                wk->seq = T_DOWNLOAD_SEQ_END;
            } else {
                func_ov326_021a1370(wk, TRUE);
                wk->seq = 100;
                func_ov326_021a20b4(wk, wk->mode, 0);
                func_ov326_021a1a78(wk, FALSE);
                wk->objFlags &= ~T_DOWNLOAD_OBJ_FLAG_RESULT;
                func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
            }
        }
        break;
    case 120:
        if (func_ov326_021a17c8(wk) == -1 && func_ov326_021a23d4(wk, 1) == TRUE) {
            func_ov326_021a2350(wk, 0, 0, wk->listCount - 1, wk->listPos[0]);
            wk->seq = 100;
        }
        break;
    case 130:
        if (func_02021c0c(wk->printQueue) == TRUE) {
            func_ov326_021a2298(wk, FALSE, FALSE, TRUE);
            wk->seq = 200;
        }
        break;
    case 200:
        if (func_ov326_021a1134(wk, T_DOWNLOAD_ACTOR_OBJ, 2) == FALSE) {
            func_ov326_021a1388(wk, 7, 2, FALSE);
            func_ov326_021a1188(wk, 1);
            func_ov326_021a2138(wk, TRUE);
            func_ov326_021a2194(wk, TRUE);
            wk->seq = 210;
        }
        break;
    case 210:
        result = func_ov326_021a128c(wk);
        if (result != -1) {
            func_ov326_021a1274(wk);
            if (result == 0) {
                if (func_ov326_021a2888(wk, wk->listPos[0]) == TRUE) {
                    func_ov326_021a1370(wk, TRUE);
                    func_ov326_021a20b4(wk, wk->mode, 0);
                    wk->seq = 230;
                } else if (func_ov326_021a287c(wk) == -1) {
                    func_ov326_021a2138(wk, FALSE);
                    func_ov326_021a2194(wk, FALSE);
                    wk->seq = 400;
                    func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
                    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
                    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 0, FALSE);
                    wk->slotCursor = 0;
                } else {
                    func_ov326_021a2194(wk, FALSE);
                    wk->seq = 400;
                    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
                    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 0, FALSE);
                    wk->flags |= T_DOWNLOAD_FLAG_SAVED_FULL;
                }
            } else if (result == 1) {
                func_ov326_021a2138(wk, FALSE);
                func_ov326_021a2194(wk, FALSE);
                wk->seq = 300;
                func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
            } else {
                func_ov326_021a2138(wk, FALSE);
                func_ov326_021a2194(wk, FALSE);
                func_ov326_021a1370(wk, TRUE);
                wk->seq = 100;
                func_ov326_021a20b4(wk, wk->mode, 0);
                func_ov326_021a1a78(wk, FALSE);
                func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
            }
        }
        break;
    case 230:
        if (func_ov326_021a1134(wk, T_DOWNLOAD_ACTOR_OBJ, 2) == FALSE) {
            func_ov326_021a1388(wk, 8, 6, TRUE);
            func_ov326_021a2138(wk, TRUE);
            func_ov326_021a2194(wk, TRUE);
            wk->seq = 240;
        }
        break;
    case 240:
        if (func_ov326_021a1338(wk) == FALSE) {
            func_ov326_021a1370(wk, TRUE);
            func_ov326_021a2138(wk, FALSE);
            func_ov326_021a2194(wk, FALSE);
            func_ov326_021a20b4(wk, wk->mode, 0);
            wk->seq = 100;
            func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
        }
        break;
    case 300:
        func_ov326_021a1370(wk, FALSE);
        wk->returnMode = wk->mode;
        TDownload_ExitReceived(wk);
        wk->seq = 0;
        wk->mode = T_DOWNLOAD_MODE_DETAIL;
        TDownload_InitMode(wk);
        break;
    case 400:
        func_ov326_021a1370(wk, TRUE);
        wk->returnMode = wk->mode;
        TDownload_ExitReceived(wk);
        wk->seq = 0;
        wk->mode = T_DOWNLOAD_MODE_SAVED;
        TDownload_InitMode(wk);
        break;
    case T_DOWNLOAD_SEQ_END:
        break;
    }
}

static void TDownload_ExitReceived(TDownloadWork *wk) {
    func_ov326_021a1898(wk);
    GFL_BGSysClearBG(2);
    wk->listPos[2] = wk->listPos[0];
    wk->listCursor[2] = wk->listCursor[0];
    wk->listScroll[2] = wk->listScroll[0];
}

static void TDownload_InitSaved(TDownloadWork *wk) {
    int i;

    GFL_BGSysMoveBG(2, BG_MOVE_SET_Y, 0);
    wk->listCount = wk->lastSlot + 1;
    wk->flags &= ~T_DOWNLOAD_FLAG_NO_SAVED;
    if (wk->listCount == 0) {
        wk->flags |= T_DOWNLOAD_FLAG_NO_SAVED;
    }
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 0, FALSE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 2, FALSE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 0, FALSE);
    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_BUTTON, 1, FALSE);
    func_ov326_021a0d4c(wk, wk->mode);
    for (i = 0; i < wk->listCount; i++) {
        if (wk->saveFlags & (1 << (i + 4))) {
            func_ov326_021a21d0(wk, 0, i);
            func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 6, TRUE);
        }
    }
    if (!(wk->flags & T_DOWNLOAD_FLAG_NO_SAVED)) {
        func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, TRUE);
        func_ov326_021a1a10(wk, 1, wk->slotCursor);
        if (!(wk->flags & T_DOWNLOAD_FLAG_SAVED_FULL)) {
            func_ov326_021a1544(wk, 0, wk->slotCursor);
        }
    }
    func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
    wk->seq = 100;
    if (!(wk->flags & T_DOWNLOAD_FLAG_DELETED)) {
        if (!(wk->flags & T_DOWNLOAD_FLAG_SAVED_FULL)) {
            func_ov326_021a1388(wk, 8, 9, TRUE);
            func_ov326_021a215c(wk, TRUE);
            func_ov326_021a2194(wk, TRUE);
            wk->seq = 10;
        } else {
            func_ov326_021a1388(wk, 8, 11, FALSE);
            func_ov326_021a2194(wk, TRUE);
            wk->seq = 405;
            func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
        }
    }
    func_ov326_021a20b4(wk, wk->mode, 0);
    wk->flags &= ~T_DOWNLOAD_FLAG_DELETED;
}

static void TDownload_SavedMain(TDownloadWork *wk) {
    if (func_02021c0c(wk->printQueue)) {
        if (TDownload_SavedTouch(wk) == FALSE) {
            TDownload_SavedKeys(wk);
        }
        if (wk->flags & T_DOWNLOAD_FLAG_SAVED_MOVED) {
            func_ov326_021a1a10(wk, 1, wk->slotCursor);
            func_ov326_021a1544(wk, 0, wk->slotCursor);
            wk->flags &= ~T_DOWNLOAD_FLAG_SAVED_MOVED;
        }
    }
}

static BOOL TDownload_SavedTouch(TDownloadWork *wk) {
    int touched;
    s16 slot;
    int input = 0;
    BOOL result = FALSE;
    BOOL moved = FALSE;

    touched = func_ov326_021a2114(wk, FALSE, wk->mode);
    if (touched == 3 || touched == 4 || touched == 5) {
        slot = touched - 3;
        if (wk->saveFlags & (1 << (slot + 4))) {
            wk->slotCursor = slot;
            moved = TRUE;
            func_0203d564(TRUE);
        }
    }
    if (moved == FALSE && touched == 0) {
        func_ov326_021a14d4(wk, 3, 0);
        input = T_DOWNLOAD_INPUT_TOUCH;
        func_0203d564(TRUE);
    }
    if (moved == TRUE) {
        wk->flags |= T_DOWNLOAD_FLAG_SAVED_MOVED;
        func_ov326_021a1a78(wk, TRUE);
        wk->seq = 130;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_0203d564(TRUE);
    }
    if (input != 0) {
        result = TRUE;
        wk->seq = 110;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        func_ov326_021a2298(wk, FALSE, TRUE, FALSE);
    }
    return result;
}

static BOOL TDownload_SavedKeys(TDownloadWork *wk) {
    int input = 0;
    BOOL moved = FALSE;
    BOOL done = FALSE;
    BOOL result = FALSE;
    int pos;

    if (func_0203d554() == TRUE) {
        if (GCTX_HIDGetPressedKeys()) {
            func_ov326_021a2298(wk, TRUE, input, input);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        done = TRUE;
    }
    if (done == FALSE) {
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            pos = func_ov326_021a2a98(wk, FALSE, wk->slotCursor);
            if (pos != wk->slotCursor) {
                moved = TRUE;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            wk->slotCursor = pos;
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            pos = func_ov326_021a2a98(wk, TRUE, wk->slotCursor);
            if (pos != wk->slotCursor) {
                moved = TRUE;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            wk->slotCursor = pos;
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            func_ov326_021a1a78(wk, TRUE);
            wk->seq = 130;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            func_0203d564(FALSE);
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            func_ov326_021a14d4(wk, 3, 0);
            input = T_DOWNLOAD_INPUT_KEY;
        }
    }
    if (moved == TRUE) {
        wk->flags |= T_DOWNLOAD_FLAG_SAVED_MOVED;
        func_ov326_021a2298(wk, TRUE, FALSE, FALSE);
    }
    if (input == T_DOWNLOAD_INPUT_KEY) {
        result = TRUE;
        wk->seq = 110;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        func_ov326_021a2298(wk, TRUE, FALSE, FALSE);
    }
    return result;
}

static void TDownload_Saved(TDownloadWork *wk) {
    int result;
    int i;

    switch (wk->seq) {
    case 0:
        wk->seq = 100;
    case 10:
        if (func_ov326_021a1338(wk) == FALSE) {
            func_ov326_021a1370(wk, TRUE);
            func_ov326_021a2138(wk, FALSE);
            func_ov326_021a2194(wk, FALSE);
            func_ov326_021a20b4(wk, wk->mode, 0);
            wk->seq = 100;
            func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
        }
        break;
    case 100:
        TDownload_SavedMain(wk);
        break;
    case 110:
        if (func_ov326_021a23d4(wk, 0) == TRUE) {
            if (wk->flags & T_DOWNLOAD_FLAG_SAVED) {
                wk->seq = 300;
                func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
                for (i = 0; i < 3; i++) {
                    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 6, FALSE);
                }
            } else {
                func_ov326_021a1388(wk, 7, 8, FALSE);
                func_ov326_021a1188(wk, 2);
                func_ov326_021a2138(wk, TRUE);
                func_ov326_021a2194(wk, TRUE);
                wk->seq = 115;
            }
        }
        break;
    case 115:
        result = func_ov326_021a128c(wk);
        if (result != -1) {
            func_ov326_021a1274(wk);
            func_ov326_021a2138(wk, FALSE);
            func_ov326_021a2194(wk, FALSE);
            if (result == 0) {
                func_ov326_021a1370(wk, TRUE);
                if (wk->returnMode == -1) {
                    wk->seq = T_DOWNLOAD_SEQ_END;
                } else {
                    wk->seq = 300;
                    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
                    for (i = 0; i < 3; i++) {
                        func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 6, FALSE);
                    }
                }
                wk->flags &= ~(T_DOWNLOAD_FLAG_SAVED_FULL | T_DOWNLOAD_FLAG_SAVED);
            } else {
                func_ov326_021a1370(wk, TRUE);
                wk->seq = 100;
                func_ov326_021a20b4(wk, wk->mode, 0);
                func_ov326_021a1a78(wk, FALSE);
                wk->objFlags &= ~T_DOWNLOAD_OBJ_FLAG_RESULT;
                func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
            }
        }
        break;
    case 120:
        if (func_ov326_021a23d4(wk, 1) == TRUE) {
            wk->objFlags |= T_DOWNLOAD_OBJ_FLAG_LIST_MOVED;
            wk->seq = 100;
        }
        break;
    case 130:
        if (func_02021c0c(wk->printQueue) == TRUE) {
            func_ov326_021a2298(wk, FALSE, FALSE, TRUE);
            wk->seq = 200;
        }
        break;
    case 200:
        if (func_ov326_021a1134(wk, T_DOWNLOAD_ACTOR_OBJ, 2) == FALSE) {
            if (!(wk->flags & T_DOWNLOAD_FLAG_SAVED)) {
                func_ov326_021a1388(wk, 7, 3, FALSE);
                func_ov326_021a1188(wk, 2);
                func_ov326_021a2138(wk, TRUE);
                func_ov326_021a2194(wk, TRUE);
                wk->seq = 210;
            } else {
                func_ov326_021a1388(wk, 7, 2, FALSE);
                func_ov326_021a1188(wk, 3);
                func_ov326_021a2138(wk, TRUE);
                func_ov326_021a2194(wk, TRUE);
                wk->seq = 220;
            }
        }
        break;
    case 210:
        result = func_ov326_021a128c(wk);
        if (result != -1) {
            func_ov326_021a1274(wk);
            if (result == 0) {
                func_ov326_021a1388(wk, 7, 4, FALSE);
                func_ov326_021a1188(wk, 2);
                wk->seq = 240;
            } else {
                func_ov326_021a2138(wk, FALSE);
                func_ov326_021a2194(wk, FALSE);
                func_ov326_021a1370(wk, TRUE);
                wk->seq = 100;
                func_ov326_021a20b4(wk, wk->mode, 0);
                func_ov326_021a1a78(wk, FALSE);
                func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
            }
        }
        break;
    case 220:
        result = func_ov326_021a128c(wk);
        if (result != -1) {
            func_ov326_021a1274(wk);
            if (result == 0) {
                func_ov326_021a1370(wk, TRUE);
                func_ov326_021a2138(wk, FALSE);
                func_ov326_021a2194(wk, FALSE);
                wk->seq = 310;
                wk->flags |= T_DOWNLOAD_FLAG_DELETED;
                func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, 2, FALSE);
                for (i = 0; i < 3; i++) {
                    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 6, FALSE);
                }
            } else {
                func_ov326_021a2138(wk, FALSE);
                func_ov326_021a2194(wk, FALSE);
                func_ov326_021a1370(wk, TRUE);
                wk->seq = 100;
                func_ov326_021a20b4(wk, wk->mode, 0);
                func_ov326_021a1a78(wk, FALSE);
                func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
            }
        }
        break;
    case 240:
        result = func_ov326_021a128c(wk);
        if (result != -1) {
            func_ov326_021a1274(wk);
            if (result == 0) {
                for (i = 0; i < 6; i++) {
                    func_ov326_021a0d2c(wk, i);
                }
                func_ov326_021a1370(wk, TRUE);
                func_ov326_021a20b4(wk, wk->mode, 0);
                func_ov326_021a1388(wk, 8, 10, TRUE);
                func_ov326_021a2194(wk, TRUE);
                func_ov326_021a2470(wk, 1, wk->slotCursor);
                wk->seq = 400;
            } else {
                func_ov326_021a2138(wk, FALSE);
                func_ov326_021a2194(wk, FALSE);
                func_ov326_021a1370(wk, TRUE);
                wk->seq = 100;
                func_ov326_021a20b4(wk, wk->mode, 0);
                func_ov326_021a1a78(wk, FALSE);
                func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
            }
        }
        break;
    case 300:
        func_ov326_021a1370(wk, FALSE);
        wk->returnMode = wk->mode;
        TDownload_ExitSaved(wk);
        wk->seq = 0;
        wk->mode = T_DOWNLOAD_MODE_RECEIVED;
        TDownload_InitMode(wk);
        wk->flags &= ~(T_DOWNLOAD_FLAG_SAVED_FULL | T_DOWNLOAD_FLAG_SAVED);
        break;
    case 310:
        func_ov326_021a1370(wk, FALSE);
        wk->returnMode = wk->mode;
        TDownload_ExitSaved(wk);
        wk->seq = 0;
        wk->mode = T_DOWNLOAD_MODE_DETAIL;
        TDownload_InitMode(wk);
        wk->flags &= ~T_DOWNLOAD_FLAG_SAVED_FULL;
        break;
    case 400:
        if (func_ov326_021a1338(wk) == FALSE) {
            func_ov326_021a1388(wk, 8, 11, FALSE);
            wk->seq = 410;
        }
        break;
    case 405:
        if (func_02021c0c(wk->printQueue)) {
            func_ov326_021a2734(wk, wk->listPos[2], func_ov326_021a287c(wk));
            func_ov326_021a2828(wk);
            wk->seq = 500;
        }
        break;
    case 410:
        if (func_02021c0c(wk->printQueue)) {
            func_ov326_021a2734(wk, wk->listPos[2], wk->slotCursor);
            func_ov326_021a2828(wk);
            wk->seq = 500;
        }
        break;
    case 500:
        if (func_ov326_021a27c0(wk) == FALSE) {
            wk->listCount = wk->lastSlot + 1;
            wk->slotCursor = wk->copyDest;
            func_ov326_021a2864(wk);
            func_ov326_021a1388(wk, 8, 12, TRUE);
            wk->seq = 510;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            func_ov326_021a2470(wk, 0, wk->slotCursor);
            for (i = 0; i < wk->listCount; i++) {
                if (wk->saveFlags & (1 << (i + 4))) {
                    func_ov326_021a21d0(wk, 0, i);
                    func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 6, TRUE);
                }
            }
            func_ov326_021a1a10(wk, 1, wk->slotCursor);
            func_ov326_021a2194(wk, TRUE);
            func_ov326_021a1544(wk, 0, wk->slotCursor);
        }
        break;
    case 510:
        if (func_ov326_021a1338(wk) == FALSE) {
            func_ov326_021a1370(wk, TRUE);
            func_ov326_021a2138(wk, FALSE);
            func_ov326_021a2194(wk, FALSE);
            func_ov326_021a20b4(wk, wk->mode, 0);
            func_ov326_021a1a78(wk, FALSE);
            wk->seq = 100;
            wk->flags |= T_DOWNLOAD_FLAG_SAVED;
            func_ov326_021a2298(wk, FALSE, FALSE, FALSE);
        }
        break;
    case T_DOWNLOAD_SEQ_END:
        break;
    }
}

static void TDownload_ExitSaved(TDownloadWork *wk) {
    int i;

    for (i = 0; i < 3; i++) {
        func_ov326_021a1104(wk, T_DOWNLOAD_ACTOR_OBJ, i + 6, FALSE);
    }
    func_ov326_021a0da4(wk, wk->mode);
}
