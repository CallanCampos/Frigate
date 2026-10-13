#include "types.h"
#include "app/zukan_area_list.h"
#include "app/zukan_area_list_view.h"
#include "app/zukan_common.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/hw.h"
#include "save/medal_box.h"
#include "save/pokedex.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/0362.h"

// The Pokédex's list of areas: its process and its states. The ROM doesn't name this file; zukan_area_list.c is a
// guess after what the screen lists

// The shortcut that the Y button registers, the same as the Pokédex's list
#define SHORTCUT_POKEDEX_AREA_LIST 18

// How far the touch bar and the heading slide in from, and by how much a frame
#define SLIDE_DISTANCE 48
#define SLIDE_SPEED 8

// The frames a color of the chosen row's blink lasts for
#define BLINK_FRAMES 4

#define KIND_NONE 3

enum {
    STATE_INIT,
    STATE_FADE_IN,
    STATE_FADE_OUT,
    STATE_LIST,
    STATE_BUTTON,
    STATE_JUMP,
    STATE_DECIDE,
    STATE_END,
    STATE_COUNT = 9,
};

// What the next state of a button is given
#define STATE_PARAM_NORMAL 8
#define STATE_PARAM_NO_WAIT 9

// The rectangles of the lower screen
enum {
    TOUCH_NONE = -1,
    TOUCH_DECIDE,
    // The places of a row's marks, from the right
    TOUCH_MARK_RIGHT,
    TOUCH_MARK_MIDDLE = TOUCH_MARK_RIGHT + ZUKAN_AREA_LIST_ROWS,
    TOUCH_MARK_LEFT = TOUCH_MARK_MIDDLE + ZUKAN_AREA_LIST_ROWS,
    TOUCH_MODE = TOUCH_MARK_LEFT + ZUKAN_AREA_LIST_ROWS,
    TOUCH_SHORTCUT,
    TOUCH_EXIT,
    TOUCH_RETURN,
    TOUCH_MODE_UPPER,
    TOUCH_MODE_LOWER,
    TOUCH_MODE_HABITAT,
};

typedef void (*ZukanAreaListStateFunc)(ZukanAreaListWork *wk);

static BOOL ZukanAreaList_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanAreaList_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanAreaList_ProcEnd(GameProc *proc, u32 *state, void *param, void *work);
static void ZukanAreaList_InitVBlank(ZukanAreaListWork *wk);
static void ZukanAreaList_ExitVBlank(ZukanAreaListWork *wk);
static void ZukanAreaList_VBlankTask(TCB *tcb, void *data);
static void ZukanAreaList_InitHBlank(ZukanAreaListWork *wk);
static void ZukanAreaList_ExitHBlank(ZukanAreaListWork *wk);
static void ZukanAreaList_HBlankTask(TCB *tcb, void *data);
static ZukanAreaListWork *ZukanAreaList_CreateWork(GameProc *proc, ZukanHabitatSummaryParam *param);
static void ZukanAreaList_FreeWork(GameProc *proc, ZukanAreaListWork *wk);
static void ZukanAreaList_InitList(ZukanAreaListWork *wk);
static void ZukanAreaList_ExitList(ZukanAreaListWork *wk);
static BOOL ZukanAreaList_Main(ZukanAreaListWork *wk);
static void ZukanAreaList_SetState(ZukanAreaListWork *wk, u32 state, u32 param);
static void ZukanAreaList_StateFadeIn(ZukanAreaListWork *wk);
static void ZukanAreaList_StateFadeOut(ZukanAreaListWork *wk);
static void ZukanAreaList_StateInit(ZukanAreaListWork *wk);
static void ZukanAreaList_StateEnd(ZukanAreaListWork *wk);
static void ZukanAreaList_StateList(ZukanAreaListWork *wk);
static void ZukanAreaList_StateButton(ZukanAreaListWork *wk);
static BOOL ZukanAreaList_Input(ZukanAreaListWork *wk);
static void ZukanAreaList_StateJump(ZukanAreaListWork *wk);
static void ZukanAreaList_StateDecide(ZukanAreaListWork *wk);
static void ZukanAreaList_SetListResult(ZukanAreaListWork *wk, s32 result);
static void ZukanAreaList_PrintHeading(ZukanAreaListWork *wk);
static void ZukanAreaList_PrintModes(ZukanAreaListWork *wk);
static void ZukanAreaList_LoadNationalObtained(ZukanAreaListWork *wk);
static void ZukanAreaList_InitModes(ZukanAreaListWork *wk);
static u32 ZukanAreaList_GetMarkKind(ZukanHabitatSummary *area, u32 place);
static void ZukanAreaList_StartButton(ZukanAreaListWork *wk, u8 actor, u8 anim, u32 nextState, u32 param,
                                      u32 nextParam);
static void ZukanAreaList_SetResult(ZukanAreaListWork *wk, u32 result);
static void ZukanAreaList_ToggleShortcut(ZukanAreaListWork *wk);
static void ZukanAreaList_UpdateShortcut(ZukanAreaListWork *wk);

const GameProcFunctions ZukanAreaList_ProcFunctions = {
    ZukanAreaList_ProcInit,
    ZukanAreaList_ProcMain,
    ZukanAreaList_ProcEnd,
};

static const ZukanAreaListStateFunc sZukanAreaListStates[STATE_COUNT] = {
    ZukanAreaList_StateInit,   ZukanAreaList_StateFadeIn, ZukanAreaList_StateFadeOut,
    ZukanAreaList_StateList,   ZukanAreaList_StateButton, ZukanAreaList_StateJump,
    ZukanAreaList_StateDecide, ZukanAreaList_StateEnd,    NULL,
};

// Whether a state runs the list
static const BOOL sZukanAreaListStateRunsList[STATE_COUNT] = {
    FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,
};

static const TouchRect sZukanAreaListTouchRects[] = {
    { 0, 64, 0, 64 },       { 0, 23, 96, 127 },     { 24, 47, 96, 127 },    { 48, 71, 96, 127 },
    { 72, 95, 96, 127 },    { 96, 119, 96, 127 },   { 120, 143, 96, 127 },  { 144, 167, 96, 127 },
    { 0, 23, 80, 111 },     { 24, 47, 80, 111 },    { 48, 71, 80, 111 },    { 72, 95, 80, 111 },
    { 96, 119, 80, 111 },   { 120, 143, 80, 111 },  { 144, 167, 80, 111 },  { 0, 23, 64, 95 },
    { 24, 47, 64, 95 },     { 48, 71, 64, 95 },     { 72, 95, 64, 95 },     { 96, 119, 64, 95 },
    { 120, 143, 64, 95 },   { 144, 167, 64, 95 },   { 168, 191, 0, 71 },    { 168, 191, 184, 207 },
    { 168, 191, 208, 231 }, { 168, 191, 232, 255 }, { 98, 122, 0, 64 },     { 121, 145, 0, 64 },
    { 144, 168, 0, 64 },    { TOUCH_RECT_END, 0, 0, 0 },
};

static BOOL ZukanAreaList_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanAreaListWork *wk = work;

    switch (*state) {
    case 0:
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_ZUKAN_AREA_LIST, 0x60000);
        ZukanAreaList_CreateWork(proc, param);
        (*state)++;
        break;
    case 1:
        ZukanAreaListView_Init(&wk->view, wk->param->gameData);
        ZukanAreaList_InitVBlank(wk);
        ZukanAreaList_InitHBlank(wk);
        ZukanAreaList_InitList(wk);
        (*state)++;
        break;
    case 2:
        if (ZukanAreaListView_Draw(&wk->view) == TRUE) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL ZukanAreaList_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanAreaListWork *wk = work;

    if (ZukanAreaList_Main(wk)) {
        ZukanAreaListView_Main(&wk->view);
        return FALSE;
    }
    return TRUE;
}

static BOOL ZukanAreaList_ProcEnd(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanAreaListWork *wk = work;

    ZukanAreaList_ExitList(wk);
    ZukanAreaList_ExitVBlank(wk);
    ZukanAreaList_ExitHBlank(wk);
    ZukanAreaListView_Exit(&wk->view);
    ZukanAreaList_FreeWork(proc, wk);
    GFL_HeapDelete(HEAPID_ZUKAN_AREA_LIST);
    return TRUE;
}

static void ZukanAreaList_InitVBlank(ZukanAreaListWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(ZukanAreaList_VBlankTask, wk, 0);
}

static void ZukanAreaList_ExitVBlank(ZukanAreaListWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static void ZukanAreaList_VBlankTask(TCB *tcb, void *data) {
    ZukanAreaListWork *wk = data;

    ZukanAreaListView_VBlank(&wk->view);
}

static void ZukanAreaList_InitHBlank(ZukanAreaListWork *wk) {
    wk->hblankTask = GFL_HBlankTCBAdd(ZukanAreaList_HBlankTask, wk, 0);
}

static void ZukanAreaList_ExitHBlank(ZukanAreaListWork *wk) {
    GFL_TCBRemove(wk->hblankTask);
}

static void ZukanAreaList_HBlankTask(TCB *tcb, void *data) {
    ZukanAreaListWork *wk = data;

    ZukanAreaListView_HBlank(&wk->view);
}

static ZukanAreaListWork *ZukanAreaList_CreateWork(GameProc *proc, ZukanHabitatSummaryParam *param) {
    ZukanAreaListWork *wk = GFL_ProcInitSubsystem(proc, sizeof(ZukanAreaListWork), HEAPID_ZUKAN_AREA_LIST);

    sys_memset(wk, 0, sizeof(ZukanAreaListWork));
    wk->param = param;
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0362, HEAPID_ZUKAN_AREA_LIST);
    wk->habitatSave = func_02010cb8(GameData_GetSaveControl(param->gameData));
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_ZUKAN_AREA_LIST);
    return wk;
}

static void ZukanAreaList_FreeWork(GameProc *proc, ZukanAreaListWork *wk) {
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_MsgDataFree(wk->msgData);
    GFL_ProcReleaseSubsystem(proc);
}

static void ZukanAreaList_InitList(ZukanAreaListWork *wk) {
    ZukanHabitatSummaryParam *param = wk->param;
    u8 count = param->count;

    ZukanAreaListView_InitList(&wk->view, count, param->areas, func_02010d78(wk->habitatSave), 0);
}

static void ZukanAreaList_ExitList(ZukanAreaListWork *wk) {
    ZukanAreaListView_ExitList(&wk->view);
}

// Returns FALSE once the screen has ended
static BOOL ZukanAreaList_Main(ZukanAreaListWork *wk) {
    ZukanAreaListStateFunc func = sZukanAreaListStates[wk->state];

    if (sZukanAreaListStateRunsList[wk->state] == TRUE) {
        ZukanAreaList_SetListResult(wk, ZukanAreaListView_ListMain(&wk->view));
    }
    if (func != NULL) {
        func(wk);
    }
    if (wk->end) {
        return FALSE;
    }
    return TRUE;
}

static void ZukanAreaList_SetState(ZukanAreaListWork *wk, u32 state, u32 param) {
    wk->state = state;
    wk->stateParam = param;
    wk->step = 0;
}

// Fades in while the touch bar and the heading slide into place
static void ZukanAreaList_StateFadeIn(ZukanAreaListWork *wk) {
    switch (wk->step) {
    case 0:
        if (ZukanAreaListView_IsPrinted(&wk->view)) {
            wk->touchbarOffset = SLIDE_DISTANCE;
            wk->headingOffset = -SLIDE_DISTANCE;
            ZukanAreaListView_StartFade(&wk->view, 16, 16);
            ZukanAreaListView_Slide(&wk->view, wk->touchbarOffset, TRUE);
            ZukanAreaListView_Slide(&wk->view, wk->headingOffset, FALSE);
            wk->step++;
        }
        break;
    case 1:
        if (!ZukanAreaListView_IsFading(&wk->view)) {
            GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, 0);
            GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, 0);
            ZukanAreaListView_StartFade(&wk->view, 16, 0);
            wk->step++;
        }
        break;
    case 2:
        if (wk->touchbarOffset <= 0) {
            if (!ZukanAreaListView_IsFading(&wk->view)) {
                ZukanAreaListView_SetBlinkPaused(&wk->view, FALSE);
                ZukanAreaList_SetState(wk, STATE_LIST, STATE_PARAM_NORMAL);
            }
        } else {
            ZukanAreaListView_Slide(&wk->view, -SLIDE_SPEED, TRUE);
            ZukanAreaListView_Slide(&wk->view, SLIDE_SPEED, FALSE);
            wk->touchbarOffset -= SLIDE_SPEED;
            wk->headingOffset += SLIDE_SPEED;
        }
        break;
    }
}

static void ZukanAreaList_StateFadeOut(ZukanAreaListWork *wk) {
    switch (wk->step) {
    case 0:
        if (ZukanAreaListView_IsPrinted(&wk->view)) {
            ZukanAreaListView_SetBlinkPaused(&wk->view, TRUE);
            wk->touchbarOffset = 0;
            wk->headingOffset = 0;
            ZukanAreaListView_StartFade(&wk->view, 0, 16);
            wk->step++;
        }
        break;
    case 1:
        if (!ZukanAreaListView_IsFading(&wk->view)) {
            if (wk->touchbarOffset >= SLIDE_DISTANCE) {
                GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
                GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
                ZukanAreaList_SetState(wk, STATE_END, STATE_PARAM_NORMAL);
            } else {
                ZukanAreaListView_Slide(&wk->view, SLIDE_SPEED, TRUE);
                ZukanAreaListView_Slide(&wk->view, -SLIDE_SPEED, FALSE);
                wk->touchbarOffset += SLIDE_SPEED;
                wk->headingOffset -= SLIDE_SPEED;
            }
        }
        break;
    }
}

static void ZukanAreaList_StateInit(ZukanAreaListWork *wk) {
    ZukanAreaListView_FillWindow(ZUKAN_AREA_LIST_WIN_MAIN, 0, &wk->view);
    ZukanAreaListView_FillWindow(ZUKAN_AREA_LIST_WIN_SUB, 0, &wk->view);
    func_02042ba8(TRUE, HEAPID_ZUKAN_AREA_LIST);
    ZukanAreaList_LoadNationalObtained(wk);
    ZukanAreaList_PrintHeading(wk);
    ZukanAreaList_InitModes(wk);
    ZukanAreaList_UpdateShortcut(wk);
    ZukanAreaList_SetState(wk, STATE_FADE_IN, STATE_PARAM_NORMAL);
    wk->end = FALSE;
}

// Saves the zone of the area under the cursor, for the cursor to start on and for the screen of its habitats
static void ZukanAreaList_StateEnd(ZukanAreaListWork *wk) {
    int scroll = ZukanAreaListView_GetListScroll(&wk->view);
    s16 cursor = ZukanAreaListView_GetListCursor(&wk->view);
    ZukanHabitatSummary *area = ZukanAreaListView_GetArea(&wk->view, scroll + cursor);

    func_02010d70(wk->habitatSave, area->zone);
    wk->end = TRUE;
}

// Acts on what the list did: an item chosen, or one of FrameList_Main's other results
static void ZukanAreaList_StateList(ZukanAreaListWork *wk) {
    switch (wk->listResult) {
    case FRAMELIST_NONE:
        ZukanAreaList_Input(wk);
        break;
    case -7:
        ZukanAreaList_StartButton(wk, ZUKAN_AREA_LIST_OBJ_PAGE_UP, 12, STATE_LIST, STATE_PARAM_NO_WAIT,
                                  STATE_PARAM_NORMAL);
        break;
    case -6:
        ZukanAreaList_StartButton(wk, ZUKAN_AREA_LIST_OBJ_PAGE_DOWN, 13, STATE_LIST, STATE_PARAM_NO_WAIT,
                                  STATE_PARAM_NORMAL);
        break;
    case -5:
        ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_PAGE_UP, 18);
        break;
    case -4:
        ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_PAGE_DOWN, 19);
        break;
    case -3:
    case -2:
        ZukanAreaListView_HideUndrawnMarks(&wk->view);
        ZukanAreaList_SetState(wk, STATE_JUMP, STATE_PARAM_NORMAL);
        break;
    default:
        ZukanAreaListView_GetListSelected(&wk->view);
        ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_AREA);
        ZukanAreaList_SetState(wk, STATE_DECIDE, STATE_PARAM_NORMAL);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        break;
    case -12:
    case -11:
    case -10:
    case -9:
    case -8:
        break;
    }
}

// Plays the animation of the button that was pressed, then goes on to the state it leads to
static void ZukanAreaList_StateButton(ZukanAreaListWork *wk) {
    switch (wk->step) {
    case 0:
        ZukanAreaListView_SetActorAnim(&wk->view, wk->buttonActor, wk->buttonAnim);
        wk->step++;
    case 1:
        if (!func_0204c560(ZukanAreaListView_GetActor(&wk->view, wk->buttonActor)) ||
            wk->stateParam == STATE_PARAM_NO_WAIT) {
            ZukanAreaList_SetState(wk, wk->nextState, wk->nextStateParam);
        }
        break;
    }
}

// The touches and keys that the list leaves
static BOOL ZukanAreaList_Input(ZukanAreaListWork *wk) {
    u32 keys = GCTX_HIDGetPressedKeys();
    s32 touch = func_0203da0c(sZukanAreaListTouchRects);
    int scroll;
    int row;

    switch (touch) {
    case TOUCH_DECIDE:
        ZukanAreaListView_GetListSelected(&wk->view);
        ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_AREA);
        ZukanAreaList_SetState(wk, STATE_DECIDE, STATE_PARAM_NORMAL);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        break;
    case TOUCH_MARK_RIGHT:
    case TOUCH_MARK_RIGHT + 1:
    case TOUCH_MARK_RIGHT + 2:
    case TOUCH_MARK_RIGHT + 3:
    case TOUCH_MARK_RIGHT + 4:
    case TOUCH_MARK_RIGHT + 5:
    case TOUCH_MARK_RIGHT + 6:
        scroll = ZukanAreaListView_GetListScroll(&wk->view);
        row = touch - TOUCH_MARK_RIGHT;
        if (ZukanAreaList_GetMarkKind(ZukanAreaListView_GetArea(&wk->view, scroll + row), 2) != KIND_NONE) {
            ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_AREA);
            ZukanAreaListView_SetListCursor(&wk->view, row);
            ZukanAreaList_SetState(wk, STATE_DECIDE, STATE_PARAM_NORMAL);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        break;
    case TOUCH_MARK_MIDDLE:
    case TOUCH_MARK_MIDDLE + 1:
    case TOUCH_MARK_MIDDLE + 2:
    case TOUCH_MARK_MIDDLE + 3:
    case TOUCH_MARK_MIDDLE + 4:
    case TOUCH_MARK_MIDDLE + 5:
    case TOUCH_MARK_MIDDLE + 6:
        scroll = ZukanAreaListView_GetListScroll(&wk->view);
        row = touch - TOUCH_MARK_MIDDLE;
        if (ZukanAreaList_GetMarkKind(ZukanAreaListView_GetArea(&wk->view, scroll + row), 1) != KIND_NONE) {
            ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_AREA);
            ZukanAreaListView_SetListCursor(&wk->view, row);
            ZukanAreaList_SetState(wk, STATE_DECIDE, STATE_PARAM_NORMAL);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        break;
    case TOUCH_MARK_LEFT:
    case TOUCH_MARK_LEFT + 1:
    case TOUCH_MARK_LEFT + 2:
    case TOUCH_MARK_LEFT + 3:
    case TOUCH_MARK_LEFT + 4:
    case TOUCH_MARK_LEFT + 5:
    case TOUCH_MARK_LEFT + 6:
        scroll = ZukanAreaListView_GetListScroll(&wk->view);
        row = touch - TOUCH_MARK_LEFT;
        if (ZukanAreaList_GetMarkKind(ZukanAreaListView_GetArea(&wk->view, scroll + row), 0) != KIND_NONE) {
            ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_AREA);
            ZukanAreaListView_SetListCursor(&wk->view, row);
            ZukanAreaList_SetState(wk, STATE_DECIDE, STATE_PARAM_NORMAL);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        break;
    case TOUCH_MODE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_REGIONAL);
        ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_MODE_HABITAT, 13);
        if (wk->nationalObtained == TRUE) {
            ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_MODE_UPPER, 14);
        } else {
            ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_MODE_LOWER, 14);
        }
        ZukanAreaList_StartButton(wk, ZUKAN_AREA_LIST_OBJ_MODE, 5, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                  STATE_PARAM_NORMAL);
        break;
    case TOUCH_EXIT:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_EXIT);
        ZukanAreaList_StartButton(wk, ZUKAN_AREA_LIST_OBJ_EXIT, 8, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                  STATE_PARAM_NORMAL);
        break;
    case TOUCH_SHORTCUT:
        GFL_SndSEPlay(SEQ_SE_SYS_07);
        ZukanAreaList_ToggleShortcut(wk);
        break;
    case TOUCH_RETURN:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_RETURN);
        ZukanAreaList_StartButton(wk, ZUKAN_AREA_LIST_OBJ_RETURN, 9, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                  STATE_PARAM_NORMAL);
        break;
    case TOUCH_MODE_UPPER:
        if (wk->nationalObtained == TRUE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_REGIONAL);
            ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_MODE_HABITAT, 13);
            ZukanAreaList_StartButton(wk, ZUKAN_AREA_LIST_OBJ_MODE_UPPER, 42, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                      STATE_PARAM_NORMAL);
        }
        break;
    case TOUCH_MODE_LOWER:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (wk->nationalObtained == TRUE) {
            ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_NATIONAL);
        } else {
            ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_REGIONAL);
        }
        ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_MODE_HABITAT, 13);
        ZukanAreaList_StartButton(wk, ZUKAN_AREA_LIST_OBJ_MODE_LOWER, 42, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                  STATE_PARAM_NORMAL);
        break;
    case TOUCH_MODE_HABITAT:
        break;
    case TOUCH_NONE:
        if (keys & PAD_BUTTON_X) {
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_EXIT);
            ZukanAreaList_StartButton(wk, ZUKAN_AREA_LIST_OBJ_EXIT, 8, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                      STATE_PARAM_NORMAL);
        } else if (keys & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_RETURN);
            ZukanAreaList_StartButton(wk, ZUKAN_AREA_LIST_OBJ_RETURN, 9, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                      STATE_PARAM_NORMAL);
        } else if (keys & PAD_BUTTON_Y) {
            GFL_SndSEPlay(SEQ_SE_SYS_07);
            ZukanAreaList_ToggleShortcut(wk);
        } else if (keys & PAD_BUTTON_SELECT) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZukanAreaList_SetResult(wk, ZUKAN_AREA_LIST_RESULT_REGIONAL);
            if (wk->nationalObtained == TRUE) {
                ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_MODE_UPPER, 14);
            } else {
                ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_MODE_LOWER, 14);
            }
            ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_MODE_HABITAT, 13);
            ZukanAreaList_StartButton(wk, ZUKAN_AREA_LIST_OBJ_MODE, 5, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                      STATE_PARAM_NORMAL);
        }
        break;
    }
    return FALSE;
}

// After the list jumped to its top or bottom
static void ZukanAreaList_StateJump(ZukanAreaListWork *wk) {
    ZukanAreaList_SetState(wk, STATE_LIST, STATE_PARAM_NORMAL);
}

// Blinks the chosen area's row twice, in another color if every habitat of the area is complete, then fades out
static void ZukanAreaList_StateDecide(ZukanAreaListWork *wk) {
    BOOL complete = TRUE;
    int i;
    int scroll = ZukanAreaListView_GetListScroll(&wk->view);
    s16 cursor = ZukanAreaListView_GetListCursor(&wk->view);
    ZukanHabitatSummary *area = ZukanAreaListView_GetArea(&wk->view, scroll + cursor);

    for (i = 0; i < ZUKAN_AREA_LIST_KINDS; i++) {
        if (area->conditions[i] == 1 && area->conditionsMet[i] == 1 && area->states[i] != 2) {
            complete = FALSE;
        }
    }
    switch (wk->step) {
    case 0: {
        ClActor *frame = ZukanAreaListView_GetActor(&wk->view, ZUKAN_AREA_LIST_OBJ_MARK_FRAME);

        func_0204c488(frame, func_0204c4a0(frame) + 1);
        func_0204c520(frame, TRUE);
        ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_DECIDE, 40);
    }
    case 2:
        if (wk->blinkTimer == 0) {
            if (complete == TRUE) {
                ZukanAreaListView_SetCursorPalette(&wk->view, 6);
            } else {
                ZukanAreaListView_SetCursorPalette(&wk->view, 5);
            }
            wk->blinkTimer = BLINK_FRAMES;
            wk->step++;
        } else {
            wk->blinkTimer--;
        }
        break;
    case 1:
    case 3:
        if (wk->blinkTimer == 0) {
            ZukanAreaListView_SetCursorPalette(&wk->view, 7);
            wk->blinkTimer = BLINK_FRAMES;
            wk->step++;
        } else {
            wk->blinkTimer--;
        }
        break;
    case 4:
        if (wk->blinkTimer == 0) {
            ZukanAreaList_SetState(wk, STATE_FADE_OUT, STATE_PARAM_NORMAL);
        } else {
            wk->blinkTimer--;
        }
        break;
    }
}

static void ZukanAreaList_SetListResult(ZukanAreaListWork *wk, s32 result) {
    wk->listResult = result;
}

static void ZukanAreaList_PrintHeading(ZukanAreaListWork *wk) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, Bank0362_Text_HabitatList);

    ZukanAreaListView_Print(ZUKAN_AREA_LIST_WIN_SUB, strbuf, &wk->view, 8, 4, 256, 16, PRINT_COLOR(14, 15, 0));
    GFL_StrBufFree(strbuf);
}

// Prints the names of the buttons at the left: the habitats' at the bottom, and above it the Pokédex's lists
static void ZukanAreaList_PrintModes(ZukanAreaListWork *wk) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, Bank0362_Text_Habitat);

    ZukanAreaListView_Print(ZUKAN_AREA_LIST_WIN_MAIN, strbuf, &wk->view, 2, 149, 40, 16, PRINT_COLOR(15, 2, 1));
    GFL_StrBufFree(strbuf);
    if (wk->nationalObtained == TRUE) {
        strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, Bank0362_Text_Unova);
        ZukanAreaListView_Print(ZUKAN_AREA_LIST_WIN_MAIN, strbuf, &wk->view, 2, 103, 40, 16, PRINT_COLOR(15, 2, 1));
        GFL_StrBufFree(strbuf);
        strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, Bank0362_Text_National);
        ZukanAreaListView_Print(ZUKAN_AREA_LIST_WIN_MAIN, strbuf, &wk->view, 2, 126, 40, 16, PRINT_COLOR(15, 2, 1));
        GFL_StrBufFree(strbuf);
    } else {
        strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, Bank0362_Text_Unova);
        ZukanAreaListView_Print(ZUKAN_AREA_LIST_WIN_MAIN, strbuf, &wk->view, 2, 126, 40, 16, PRINT_COLOR(15, 2, 1));
        GFL_StrBufFree(strbuf);
        func_0204c124(ZukanAreaListView_GetActor(&wk->view, ZUKAN_AREA_LIST_OBJ_MODE_UPPER), FALSE);
    }
}

static void ZukanAreaList_LoadNationalObtained(ZukanAreaListWork *wk) {
    wk->nationalObtained = PokeDex_IsNationalObtained(GameData_GetPokedex(wk->param->gameData));
}

static void ZukanAreaList_InitModes(ZukanAreaListWork *wk) {
    ZukanAreaList_PrintModes(wk);
}

// A kind of habitat: an index of an area's conditions. An enum, which keeps the loop's test before its first pass
typedef enum {
    KIND_FIRST,
    KIND_LAST = ZUKAN_AREA_LIST_KINDS - 1,
} ZukanAreaListKind;

// The kind of habitat of the mark at a place of an area's row, 0 the leftmost: the marks of the kinds that the
// area has are put from the right. KIND_NONE if the place has no mark
static u32 ZukanAreaList_GetMarkKind(ZukanHabitatSummary *area, u32 place) {
    u8 next = ZUKAN_AREA_LIST_KINDS - 1;
    ZukanAreaListKind i;
    u32 kinds[ZUKAN_AREA_LIST_KINDS] = { KIND_NONE, KIND_NONE, KIND_NONE };

    for (i = next; i >= KIND_FIRST; i--) {
        if (area->conditions[i] == 1 && area->conditionsMet[i] == 1) {
            kinds[next] = i;
            next--;
        }
    }
    return kinds[place];
}

static void ZukanAreaList_StartButton(ZukanAreaListWork *wk, u8 actor, u8 anim, u32 nextState, u32 param,
                                      u32 nextParam) {
    wk->buttonActor = actor;
    wk->buttonAnim = anim;
    wk->nextState = nextState;
    wk->nextStateParam = nextParam;
    ZukanAreaList_SetState(wk, STATE_BUTTON, param);
}

static void ZukanAreaList_SetResult(ZukanAreaListWork *wk, u32 result) {
    wk->param->result = result;
}

static void ZukanAreaList_ToggleShortcut(ZukanAreaListWork *wk) {
    GameData *gameData = wk->param->gameData;

    if (GameData_IsShortcutRegistered(gameData, SHORTCUT_POKEDEX_AREA_LIST) == TRUE) {
        GameData_SetKeyItemRegistration(gameData, SHORTCUT_POKEDEX_AREA_LIST, FALSE);
        ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_SHORTCUT, 6);
    } else {
        GameData_SetKeyItemRegistration(gameData, SHORTCUT_POKEDEX_AREA_LIST, TRUE);
        ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_SHORTCUT, 7);
    }
}

static void ZukanAreaList_UpdateShortcut(ZukanAreaListWork *wk) {
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_AREA_LIST) == TRUE) {
        ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_SHORTCUT, 7);
    } else {
        ZukanAreaListView_SetActorAnim(&wk->view, ZUKAN_AREA_LIST_OBJ_SHORTCUT, 6);
    }
}
