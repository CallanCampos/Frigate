#include "types.h"
#include "app/pokedex_area_list.h"
#include "app/pokedex_area_list_view.h"
#include "app/pokedex_common.h"
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
#include "save/pokedex_savedata.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/pokedex_habitat.h"

// The Pokédex's list of areas: its process and its states. The ROM doesn't name this file; pokedex_area_list.c is a
// guess after what the screen lists

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
    TOUCH_MARK_MIDDLE = TOUCH_MARK_RIGHT + POKEDEX_AREA_LIST_ROWS,
    TOUCH_MARK_LEFT = TOUCH_MARK_MIDDLE + POKEDEX_AREA_LIST_ROWS,
    TOUCH_MODE = TOUCH_MARK_LEFT + POKEDEX_AREA_LIST_ROWS,
    TOUCH_SHORTCUT,
    TOUCH_EXIT,
    TOUCH_RETURN,
    TOUCH_MODE_UPPER,
    TOUCH_MODE_LOWER,
    TOUCH_MODE_HABITAT,
};

typedef void (*PokedexAreaListStateFunc)(PokedexAreaListWork *wk);

static BOOL PokedexAreaList_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexAreaList_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexAreaList_ProcEnd(GameProc *proc, u32 *state, void *param, void *work);
static void PokedexAreaList_InitVBlank(PokedexAreaListWork *wk);
static void PokedexAreaList_ExitVBlank(PokedexAreaListWork *wk);
static void PokedexAreaList_VBlankTask(TCB *tcb, void *data);
static void PokedexAreaList_InitHBlank(PokedexAreaListWork *wk);
static void PokedexAreaList_ExitHBlank(PokedexAreaListWork *wk);
static void PokedexAreaList_HBlankTask(TCB *tcb, void *data);
static PokedexAreaListWork *PokedexAreaList_CreateWork(GameProc *proc, PokedexHabitatSummaryParam *param);
static void PokedexAreaList_FreeWork(GameProc *proc, PokedexAreaListWork *wk);
static void PokedexAreaList_InitList(PokedexAreaListWork *wk);
static void PokedexAreaList_ExitList(PokedexAreaListWork *wk);
static BOOL PokedexAreaList_Main(PokedexAreaListWork *wk);
static void PokedexAreaList_SetState(PokedexAreaListWork *wk, u32 state, u32 param);
static void PokedexAreaList_StateFadeIn(PokedexAreaListWork *wk);
static void PokedexAreaList_StateFadeOut(PokedexAreaListWork *wk);
static void PokedexAreaList_StateInit(PokedexAreaListWork *wk);
static void PokedexAreaList_StateEnd(PokedexAreaListWork *wk);
static void PokedexAreaList_StateList(PokedexAreaListWork *wk);
static void PokedexAreaList_StateButton(PokedexAreaListWork *wk);
static BOOL PokedexAreaList_Input(PokedexAreaListWork *wk);
static void PokedexAreaList_StateJump(PokedexAreaListWork *wk);
static void PokedexAreaList_StateDecide(PokedexAreaListWork *wk);
static void PokedexAreaList_SetListResult(PokedexAreaListWork *wk, s32 result);
static void PokedexAreaList_PrintHeading(PokedexAreaListWork *wk);
static void PokedexAreaList_PrintModes(PokedexAreaListWork *wk);
static void PokedexAreaList_LoadNationalObtained(PokedexAreaListWork *wk);
static void PokedexAreaList_InitModes(PokedexAreaListWork *wk);
static u32 PokedexAreaList_GetMarkKind(PokedexHabitatSummary *area, u32 place);
static void PokedexAreaList_StartButton(PokedexAreaListWork *wk, u8 actor, u8 anim, u32 nextState, u32 param,
                                        u32 nextParam);
static void PokedexAreaList_SetResult(PokedexAreaListWork *wk, u32 result);
static void PokedexAreaList_ToggleShortcut(PokedexAreaListWork *wk);
static void PokedexAreaList_UpdateShortcut(PokedexAreaListWork *wk);

const GameProcFunctions PokedexAreaList_ProcFunctions = {
    PokedexAreaList_ProcInit,
    PokedexAreaList_ProcMain,
    PokedexAreaList_ProcEnd,
};

static const PokedexAreaListStateFunc sPokedexAreaListStates[STATE_COUNT] = {
    PokedexAreaList_StateInit,   PokedexAreaList_StateFadeIn, PokedexAreaList_StateFadeOut,
    PokedexAreaList_StateList,   PokedexAreaList_StateButton, PokedexAreaList_StateJump,
    PokedexAreaList_StateDecide, PokedexAreaList_StateEnd,    NULL,
};

// Whether a state runs the list
static const BOOL sPokedexAreaListStateRunsList[STATE_COUNT] = {
    FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,
};

// clang-format off
static const TouchRect sPokedexAreaListTouchRects[] = {
    { 0, 64, 0, 64 },       { 0, 23, 96, 127 },     { 24, 47, 96, 127 },    { 48, 71, 96, 127 },
    { 72, 95, 96, 127 },    { 96, 119, 96, 127 },   { 120, 143, 96, 127 },  { 144, 167, 96, 127 },
    { 0, 23, 80, 111 },     { 24, 47, 80, 111 },    { 48, 71, 80, 111 },    { 72, 95, 80, 111 },
    { 96, 119, 80, 111 },   { 120, 143, 80, 111 },  { 144, 167, 80, 111 },  { 0, 23, 64, 95 },
    { 24, 47, 64, 95 },     { 48, 71, 64, 95 },     { 72, 95, 64, 95 },     { 96, 119, 64, 95 },
    { 120, 143, 64, 95 },   { 144, 167, 64, 95 },   { 168, 191, 0, 71 },    { 168, 191, 184, 207 },
    { 168, 191, 208, 231 }, { 168, 191, 232, 255 }, { 98, 122, 0, 64 },     { 121, 145, 0, 64 },
    { 144, 168, 0, 64 },    { TOUCH_RECT_END, 0, 0, 0 },
};

// clang-format on

static BOOL PokedexAreaList_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexAreaListWork *wk = work;

    switch (*state) {
    case 0:
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_POKEDEX_AREA_LIST, 0x60000);
        PokedexAreaList_CreateWork(proc, param);
        (*state)++;
        break;
    case 1:
        PokedexAreaListView_Init(&wk->view, wk->param->gameData);
        PokedexAreaList_InitVBlank(wk);
        PokedexAreaList_InitHBlank(wk);
        PokedexAreaList_InitList(wk);
        (*state)++;
        break;
    case 2:
        if (PokedexAreaListView_Draw(&wk->view) == TRUE) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL PokedexAreaList_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexAreaListWork *wk = work;

    if (PokedexAreaList_Main(wk)) {
        PokedexAreaListView_Main(&wk->view);
        return FALSE;
    }
    return TRUE;
}

static BOOL PokedexAreaList_ProcEnd(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexAreaListWork *wk = work;

    PokedexAreaList_ExitList(wk);
    PokedexAreaList_ExitVBlank(wk);
    PokedexAreaList_ExitHBlank(wk);
    PokedexAreaListView_Exit(&wk->view);
    PokedexAreaList_FreeWork(proc, wk);
    GFL_HeapDelete(HEAPID_POKEDEX_AREA_LIST);
    return TRUE;
}

static void PokedexAreaList_InitVBlank(PokedexAreaListWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(PokedexAreaList_VBlankTask, wk, 0);
}

static void PokedexAreaList_ExitVBlank(PokedexAreaListWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static void PokedexAreaList_VBlankTask(TCB *tcb, void *data) {
    PokedexAreaListWork *wk = data;

    PokedexAreaListView_VBlank(&wk->view);
}

static void PokedexAreaList_InitHBlank(PokedexAreaListWork *wk) {
    wk->hblankTask = GFL_HBlankTCBAdd(PokedexAreaList_HBlankTask, wk, 0);
}

static void PokedexAreaList_ExitHBlank(PokedexAreaListWork *wk) {
    GFL_TCBRemove(wk->hblankTask);
}

static void PokedexAreaList_HBlankTask(TCB *tcb, void *data) {
    PokedexAreaListWork *wk = data;

    PokedexAreaListView_HBlank(&wk->view);
}

static PokedexAreaListWork *PokedexAreaList_CreateWork(GameProc *proc, PokedexHabitatSummaryParam *param) {
    PokedexAreaListWork *wk = GFL_ProcInitSubsystem(proc, sizeof(PokedexAreaListWork), HEAPID_POKEDEX_AREA_LIST);

    MI_CpuFill8(wk, 0, sizeof(PokedexAreaListWork));
    wk->param = param;
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_POKEDEX_HABITAT, HEAPID_POKEDEX_AREA_LIST);
    wk->habitatSave = SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(param->gameData));
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_POKEDEX_AREA_LIST);
    return wk;
}

static void PokedexAreaList_FreeWork(GameProc *proc, PokedexAreaListWork *wk) {
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_MsgDataFree(wk->msgData);
    GFL_ProcReleaseSubsystem(proc);
}

static void PokedexAreaList_InitList(PokedexAreaListWork *wk) {
    PokedexHabitatSummaryParam *param = wk->param;
    u8 count = param->count;

    PokedexAreaListView_InitList(&wk->view, count, param->areas, PokedexHabitatSave_GetZone(wk->habitatSave), 0);
}

static void PokedexAreaList_ExitList(PokedexAreaListWork *wk) {
    PokedexAreaListView_ExitList(&wk->view);
}

// Returns FALSE once the screen has ended
static BOOL PokedexAreaList_Main(PokedexAreaListWork *wk) {
    PokedexAreaListStateFunc func = sPokedexAreaListStates[wk->state];

    if (sPokedexAreaListStateRunsList[wk->state] == TRUE) {
        PokedexAreaList_SetListResult(wk, PokedexAreaListView_ListMain(&wk->view));
    }
    if (func != NULL) {
        func(wk);
    }
    if (wk->end) {
        return FALSE;
    }
    return TRUE;
}

static void PokedexAreaList_SetState(PokedexAreaListWork *wk, u32 state, u32 param) {
    wk->state = state;
    wk->stateParam = param;
    wk->step = 0;
}

// Fades in while the touch bar and the heading slide into place
static void PokedexAreaList_StateFadeIn(PokedexAreaListWork *wk) {
    switch (wk->step) {
    case 0:
        if (PokedexAreaListView_IsPrinted(&wk->view)) {
            wk->touchbarOffset = SLIDE_DISTANCE;
            wk->headingOffset = -SLIDE_DISTANCE;
            PokedexAreaListView_StartFade(&wk->view, 16, 16);
            PokedexAreaListView_Slide(&wk->view, wk->touchbarOffset, TRUE);
            PokedexAreaListView_Slide(&wk->view, wk->headingOffset, FALSE);
            wk->step++;
        }
        break;
    case 1:
        if (!PokedexAreaListView_IsFading(&wk->view)) {
            GX_SetMasterBrightness(0);
            GXS_SetMasterBrightness(0);
            PokedexAreaListView_StartFade(&wk->view, 16, 0);
            wk->step++;
        }
        break;
    case 2:
        if (wk->touchbarOffset <= 0) {
            if (!PokedexAreaListView_IsFading(&wk->view)) {
                PokedexAreaListView_SetBlinkPaused(&wk->view, FALSE);
                PokedexAreaList_SetState(wk, STATE_LIST, STATE_PARAM_NORMAL);
            }
        } else {
            PokedexAreaListView_Slide(&wk->view, -SLIDE_SPEED, TRUE);
            PokedexAreaListView_Slide(&wk->view, SLIDE_SPEED, FALSE);
            wk->touchbarOffset -= SLIDE_SPEED;
            wk->headingOffset += SLIDE_SPEED;
        }
        break;
    }
}

static void PokedexAreaList_StateFadeOut(PokedexAreaListWork *wk) {
    switch (wk->step) {
    case 0:
        if (PokedexAreaListView_IsPrinted(&wk->view)) {
            PokedexAreaListView_SetBlinkPaused(&wk->view, TRUE);
            wk->touchbarOffset = 0;
            wk->headingOffset = 0;
            PokedexAreaListView_StartFade(&wk->view, 0, 16);
            wk->step++;
        }
        break;
    case 1:
        if (!PokedexAreaListView_IsFading(&wk->view)) {
            if (wk->touchbarOffset >= SLIDE_DISTANCE) {
                GX_SetMasterBrightness(-16);
                GXS_SetMasterBrightness(-16);
                PokedexAreaList_SetState(wk, STATE_END, STATE_PARAM_NORMAL);
            } else {
                PokedexAreaListView_Slide(&wk->view, SLIDE_SPEED, TRUE);
                PokedexAreaListView_Slide(&wk->view, -SLIDE_SPEED, FALSE);
                wk->touchbarOffset += SLIDE_SPEED;
                wk->headingOffset -= SLIDE_SPEED;
            }
        }
        break;
    }
}

static void PokedexAreaList_StateInit(PokedexAreaListWork *wk) {
    PokedexAreaListView_FillWindow(POKEDEX_AREA_LIST_WIN_MAIN, 0, &wk->view);
    PokedexAreaListView_FillWindow(POKEDEX_AREA_LIST_WIN_SUB, 0, &wk->view);
    GFL_NetSetIconScreen(TRUE, HEAPID_POKEDEX_AREA_LIST);
    PokedexAreaList_LoadNationalObtained(wk);
    PokedexAreaList_PrintHeading(wk);
    PokedexAreaList_InitModes(wk);
    PokedexAreaList_UpdateShortcut(wk);
    PokedexAreaList_SetState(wk, STATE_FADE_IN, STATE_PARAM_NORMAL);
    wk->end = FALSE;
}

// Saves the zone of the area under the cursor, for the cursor to start on and for the screen of its habitats
static void PokedexAreaList_StateEnd(PokedexAreaListWork *wk) {
    int scroll = PokedexAreaListView_GetListScroll(&wk->view);
    s16 cursor = PokedexAreaListView_GetListCursor(&wk->view);
    PokedexHabitatSummary *area = PokedexAreaListView_GetArea(&wk->view, scroll + cursor);

    PokedexHabitatSave_SetZone(wk->habitatSave, area->zone);
    wk->end = TRUE;
}

// Acts on what the list did: an item chosen, or one of FrameList_Main's other results
static void PokedexAreaList_StateList(PokedexAreaListWork *wk) {
    switch (wk->listResult) {
    case FRAMELIST_NONE:
        PokedexAreaList_Input(wk);
        break;
    case FRAMELIST_PAGE_UP:
        PokedexAreaList_StartButton(wk, POKEDEX_AREA_LIST_OBJ_PAGE_UP, 12, STATE_LIST, STATE_PARAM_NO_WAIT,
                                    STATE_PARAM_NORMAL);
        break;
    case FRAMELIST_PAGE_DOWN:
        PokedexAreaList_StartButton(wk, POKEDEX_AREA_LIST_OBJ_PAGE_DOWN, 13, STATE_LIST, STATE_PARAM_NO_WAIT,
                                    STATE_PARAM_NORMAL);
        break;
    case FRAMELIST_CURSOR_TOP:
        PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_PAGE_UP, 18);
        break;
    case FRAMELIST_CURSOR_BOTTOM:
        PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_PAGE_DOWN, 19);
        break;
    case FRAMELIST_JUMP_TOP:
    case FRAMELIST_JUMP_BOTTOM:
        PokedexAreaListView_HideUndrawnMarks(&wk->view);
        PokedexAreaList_SetState(wk, STATE_JUMP, STATE_PARAM_NORMAL);
        break;
    default:
        PokedexAreaListView_GetListSelected(&wk->view);
        PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_AREA);
        PokedexAreaList_SetState(wk, STATE_DECIDE, STATE_PARAM_NORMAL);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        break;
    case FRAMELIST_CURSOR_SHOWN:
    case FRAMELIST_CURSOR_MOVE:
    case FRAMELIST_SCROLL:
    case FRAMELIST_BAR:
    case FRAMELIST_DRAG:
        break;
    }
}

// Plays the animation of the button that was pressed, then goes on to the state it leads to
static void PokedexAreaList_StateButton(PokedexAreaListWork *wk) {
    switch (wk->step) {
    case 0:
        PokedexAreaListView_SetActorAnim(&wk->view, wk->buttonActor, wk->buttonAnim);
        wk->step++;
    case 1:
        if (!ClActor_IsAnimActive(PokedexAreaListView_GetActor(&wk->view, wk->buttonActor)) ||
            wk->stateParam == STATE_PARAM_NO_WAIT) {
            PokedexAreaList_SetState(wk, wk->nextState, wk->nextStateParam);
        }
        break;
    }
}

// The touches and keys that the list leaves
static BOOL PokedexAreaList_Input(PokedexAreaListWork *wk) {
    u32 keys = GCTX_HIDGetPressedKeys();
    s32 touch = GFL_HIDGetTouchPressedRect(sPokedexAreaListTouchRects);
    int scroll;
    int row;

    switch (touch) {
    case TOUCH_DECIDE:
        PokedexAreaListView_GetListSelected(&wk->view);
        PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_AREA);
        PokedexAreaList_SetState(wk, STATE_DECIDE, STATE_PARAM_NORMAL);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        break;
    case TOUCH_MARK_RIGHT:
    case TOUCH_MARK_RIGHT + 1:
    case TOUCH_MARK_RIGHT + 2:
    case TOUCH_MARK_RIGHT + 3:
    case TOUCH_MARK_RIGHT + 4:
    case TOUCH_MARK_RIGHT + 5:
    case TOUCH_MARK_RIGHT + 6:
        scroll = PokedexAreaListView_GetListScroll(&wk->view);
        row = touch - TOUCH_MARK_RIGHT;
        if (PokedexAreaList_GetMarkKind(PokedexAreaListView_GetArea(&wk->view, scroll + row), 2) != KIND_NONE) {
            PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_AREA);
            PokedexAreaListView_SetListCursor(&wk->view, row);
            PokedexAreaList_SetState(wk, STATE_DECIDE, STATE_PARAM_NORMAL);
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
        scroll = PokedexAreaListView_GetListScroll(&wk->view);
        row = touch - TOUCH_MARK_MIDDLE;
        if (PokedexAreaList_GetMarkKind(PokedexAreaListView_GetArea(&wk->view, scroll + row), 1) != KIND_NONE) {
            PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_AREA);
            PokedexAreaListView_SetListCursor(&wk->view, row);
            PokedexAreaList_SetState(wk, STATE_DECIDE, STATE_PARAM_NORMAL);
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
        scroll = PokedexAreaListView_GetListScroll(&wk->view);
        row = touch - TOUCH_MARK_LEFT;
        if (PokedexAreaList_GetMarkKind(PokedexAreaListView_GetArea(&wk->view, scroll + row), 0) != KIND_NONE) {
            PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_AREA);
            PokedexAreaListView_SetListCursor(&wk->view, row);
            PokedexAreaList_SetState(wk, STATE_DECIDE, STATE_PARAM_NORMAL);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        break;
    case TOUCH_MODE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_REGIONAL);
        PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_MODE_HABITAT, 13);
        if (wk->nationalObtained == TRUE) {
            PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_MODE_UPPER, 14);
        } else {
            PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_MODE_LOWER, 14);
        }
        PokedexAreaList_StartButton(wk, POKEDEX_AREA_LIST_OBJ_MODE, 5, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                    STATE_PARAM_NORMAL);
        break;
    case TOUCH_EXIT:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_EXIT);
        PokedexAreaList_StartButton(wk, POKEDEX_AREA_LIST_OBJ_EXIT, 8, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                    STATE_PARAM_NORMAL);
        break;
    case TOUCH_SHORTCUT:
        GFL_SndSEPlay(SEQ_SE_SYS_07);
        PokedexAreaList_ToggleShortcut(wk);
        break;
    case TOUCH_RETURN:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_RETURN);
        PokedexAreaList_StartButton(wk, POKEDEX_AREA_LIST_OBJ_RETURN, 9, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                    STATE_PARAM_NORMAL);
        break;
    case TOUCH_MODE_UPPER:
        if (wk->nationalObtained == TRUE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_REGIONAL);
            PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_MODE_HABITAT, 13);
            PokedexAreaList_StartButton(wk, POKEDEX_AREA_LIST_OBJ_MODE_UPPER, 42, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                        STATE_PARAM_NORMAL);
        }
        break;
    case TOUCH_MODE_LOWER:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (wk->nationalObtained == TRUE) {
            PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_NATIONAL);
        } else {
            PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_REGIONAL);
        }
        PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_MODE_HABITAT, 13);
        PokedexAreaList_StartButton(wk, POKEDEX_AREA_LIST_OBJ_MODE_LOWER, 42, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                    STATE_PARAM_NORMAL);
        break;
    case TOUCH_MODE_HABITAT:
        break;
    case TOUCH_NONE:
        if (keys & PAD_BUTTON_X) {
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_EXIT);
            PokedexAreaList_StartButton(wk, POKEDEX_AREA_LIST_OBJ_EXIT, 8, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                        STATE_PARAM_NORMAL);
        } else if (keys & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_RETURN);
            PokedexAreaList_StartButton(wk, POKEDEX_AREA_LIST_OBJ_RETURN, 9, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                        STATE_PARAM_NORMAL);
        } else if (keys & PAD_BUTTON_Y) {
            GFL_SndSEPlay(SEQ_SE_SYS_07);
            PokedexAreaList_ToggleShortcut(wk);
        } else if (keys & PAD_BUTTON_SELECT) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexAreaList_SetResult(wk, POKEDEX_AREA_LIST_RESULT_REGIONAL);
            if (wk->nationalObtained == TRUE) {
                PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_MODE_UPPER, 14);
            } else {
                PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_MODE_LOWER, 14);
            }
            PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_MODE_HABITAT, 13);
            PokedexAreaList_StartButton(wk, POKEDEX_AREA_LIST_OBJ_MODE, 5, STATE_FADE_OUT, STATE_PARAM_NORMAL,
                                        STATE_PARAM_NORMAL);
        }
        break;
    }
    return FALSE;
}

// After the list jumped to its top or bottom
static void PokedexAreaList_StateJump(PokedexAreaListWork *wk) {
    PokedexAreaList_SetState(wk, STATE_LIST, STATE_PARAM_NORMAL);
}

// Blinks the chosen area's row twice, in another color if every habitat of the area is complete, then fades out
static void PokedexAreaList_StateDecide(PokedexAreaListWork *wk) {
    BOOL complete = TRUE;
    int i;
    int scroll = PokedexAreaListView_GetListScroll(&wk->view);
    s16 cursor = PokedexAreaListView_GetListCursor(&wk->view);
    PokedexHabitatSummary *area = PokedexAreaListView_GetArea(&wk->view, scroll + cursor);

    for (i = 0; i < POKEDEX_AREA_LIST_KINDS; i++) {
        if (area->conditions[i] == 1 && area->conditionsMet[i] == 1 && area->states[i] != 2) {
            complete = FALSE;
        }
    }
    switch (wk->step) {
    case 0: {
        ClActor *frame = PokedexAreaListView_GetActor(&wk->view, POKEDEX_AREA_LIST_OBJ_MARK_FRAME);

        ClActor_SetSequence(frame, ClActor_GetSequence(frame) + 1);
        ClActor_SetAutoAnim(frame, TRUE);
        PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_DECIDE, 40);
    }
    case 2:
        if (wk->blinkTimer == 0) {
            if (complete == TRUE) {
                PokedexAreaListView_SetCursorPalette(&wk->view, 6);
            } else {
                PokedexAreaListView_SetCursorPalette(&wk->view, 5);
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
            PokedexAreaListView_SetCursorPalette(&wk->view, 7);
            wk->blinkTimer = BLINK_FRAMES;
            wk->step++;
        } else {
            wk->blinkTimer--;
        }
        break;
    case 4:
        if (wk->blinkTimer == 0) {
            PokedexAreaList_SetState(wk, STATE_FADE_OUT, STATE_PARAM_NORMAL);
        } else {
            wk->blinkTimer--;
        }
        break;
    }
}

static void PokedexAreaList_SetListResult(PokedexAreaListWork *wk, s32 result) {
    wk->listResult = result;
}

static void PokedexAreaList_PrintHeading(PokedexAreaListWork *wk) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexHabitat_Text_HabitatList);

    PokedexAreaListView_Print(POKEDEX_AREA_LIST_WIN_SUB, strbuf, &wk->view, 8, 4, 256, 16, PRINT_COLOR(14, 15, 0));
    GFL_StrBufFree(strbuf);
}

// Prints the names of the buttons at the left: the habitats' at the bottom, and above it the Pokédex's lists
static void PokedexAreaList_PrintModes(PokedexAreaListWork *wk) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexHabitat_Text_Habitat);

    PokedexAreaListView_Print(POKEDEX_AREA_LIST_WIN_MAIN, strbuf, &wk->view, 2, 149, 40, 16, PRINT_COLOR(15, 2, 1));
    GFL_StrBufFree(strbuf);
    if (wk->nationalObtained == TRUE) {
        strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexHabitat_Text_Unova);
        PokedexAreaListView_Print(POKEDEX_AREA_LIST_WIN_MAIN, strbuf, &wk->view, 2, 103, 40, 16, PRINT_COLOR(15, 2, 1));
        GFL_StrBufFree(strbuf);
        strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexHabitat_Text_National);
        PokedexAreaListView_Print(POKEDEX_AREA_LIST_WIN_MAIN, strbuf, &wk->view, 2, 126, 40, 16, PRINT_COLOR(15, 2, 1));
        GFL_StrBufFree(strbuf);
    } else {
        strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexHabitat_Text_Unova);
        PokedexAreaListView_Print(POKEDEX_AREA_LIST_WIN_MAIN, strbuf, &wk->view, 2, 126, 40, 16, PRINT_COLOR(15, 2, 1));
        GFL_StrBufFree(strbuf);
        ClActor_SetVisible(PokedexAreaListView_GetActor(&wk->view, POKEDEX_AREA_LIST_OBJ_MODE_UPPER), FALSE);
    }
}

static void PokedexAreaList_LoadNationalObtained(PokedexAreaListWork *wk) {
    wk->nationalObtained = PokeDex_IsNationalObtained(GameData_GetPokedex(wk->param->gameData));
}

static void PokedexAreaList_InitModes(PokedexAreaListWork *wk) {
    PokedexAreaList_PrintModes(wk);
}

// A kind of habitat: an index of an area's conditions. An enum, which keeps the loop's test before its first pass
typedef enum {
    KIND_FIRST,
    KIND_LAST = POKEDEX_AREA_LIST_KINDS - 1,
} PokedexAreaListKind;

// The kind of habitat of the mark at a place of an area's row, 0 the leftmost: the marks of the kinds that the
// area has are put from the right. KIND_NONE if the place has no mark
static u32 PokedexAreaList_GetMarkKind(PokedexHabitatSummary *area, u32 place) {
    u8 next = POKEDEX_AREA_LIST_KINDS - 1;
    PokedexAreaListKind i;
    u32 kinds[POKEDEX_AREA_LIST_KINDS] = { KIND_NONE, KIND_NONE, KIND_NONE };

    for (i = next; i >= KIND_FIRST; i--) {
        if (area->conditions[i] == 1 && area->conditionsMet[i] == 1) {
            kinds[next] = i;
            next--;
        }
    }
    return kinds[place];
}

static void PokedexAreaList_StartButton(PokedexAreaListWork *wk, u8 actor, u8 anim, u32 nextState, u32 param,
                                        u32 nextParam) {
    wk->buttonActor = actor;
    wk->buttonAnim = anim;
    wk->nextState = nextState;
    wk->nextStateParam = nextParam;
    PokedexAreaList_SetState(wk, STATE_BUTTON, param);
}

static void PokedexAreaList_SetResult(PokedexAreaListWork *wk, u32 result) {
    wk->param->result = result;
}

static void PokedexAreaList_ToggleShortcut(PokedexAreaListWork *wk) {
    GameData *gameData = wk->param->gameData;

    if (GameData_IsShortcutRegistered(gameData, SHORTCUT_POKEDEX_LIST) == TRUE) {
        GameData_SetKeyItemRegistration(gameData, SHORTCUT_POKEDEX_LIST, FALSE);
        PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_SHORTCUT, 6);
    } else {
        GameData_SetKeyItemRegistration(gameData, SHORTCUT_POKEDEX_LIST, TRUE);
        PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_SHORTCUT, 7);
    }
}

static void PokedexAreaList_UpdateShortcut(PokedexAreaListWork *wk) {
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_LIST) == TRUE) {
        PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_SHORTCUT, 7);
    } else {
        PokedexAreaListView_SetActorAnim(&wk->view, POKEDEX_AREA_LIST_OBJ_SHORTCUT, 6);
    }
}
