#include "types.h"
#include "app/manual.h"
#include "app/manual_category.h"
#include "app/manual_common.h"
#include "app/manual_data.h"
#include "app/manual_explain.h"
#include "app/manual_list.h"
#include "app/manual_title.h"
#include "app/manual_top.h"
#include "field/event_game_manual.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/tcb.h"

// The in-game manual's process: it goes between the top screen, the categories, the titles and a title's pages. The
// ROM has no name for this file; "manual.c" is a guess from its nine others

enum {
    SCENE_TOP,
    SCENE_CATEGORY,
    SCENE_TITLE,
    SCENE_EXPLAIN,
};

enum {
    STATE_START,
    STATE_FADE_IN,
    STATE_MAIN,
    STATE_FADE_OUT,
    STATE_END,
};

typedef struct {
    HeapID heapId;
    GameManualSubwork *param;
    ManualCommon *common;
    TCB *vblankTask;
    u32 scene;
    ManualTopParam topParam;
    ManualTop *top;
    ManualCategoryParam categoryParam;
    ManualCategory *category;
    ManualTitleParam titleParam;
    ManualTitle *title;
    ManualExplainParam explainParam;
    ManualExplain *explain;
} Manual;

static BOOL Manual_Init(GameProc *proc, u32 *state, void *param, void *data);
static BOOL Manual_Exit(GameProc *proc, u32 *state, void *param, void *data);
static BOOL Manual_Main(GameProc *proc, u32 *state, void *param, void *data);
static void Manual_VBlank(TCB *tcb, void *data);

const GameProcFunctions gManualProcFunctions = { Manual_Init, Manual_Main, Manual_Exit };

static BOOL Manual_Init(GameProc *proc, u32 *state, void *param, void *data) {
    Manual *work;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MANUAL, 0x50000);
    work = GFL_ProcInitSubsystem(proc, sizeof(Manual), HEAPID_MANUAL);
    sys_memset(work, 0, sizeof(Manual));
    work->heapId = HEAPID_MANUAL;
    work->param = param;
    work->param->result = MANUAL_RESULT_RETURN;
    work->common = ManualCommon_Create(work->param->gameData, work->heapId);
    GFL_BGSysSetBGPriority(4, 3);
    GFL_BGSysSetBGPriority(5, 2);
    GFL_BGSysSetBGPriority(6, 1);
    GFL_BGSysSetBGPriority(7, 0);
    work->vblankTask = GFL_VBlankTCBAdd(Manual_VBlank, work, 1);
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
    func_02042ba8(FALSE, work->heapId);
    work->topParam.cursor = 0;
    work->categoryParam.scroll = 0;
    work->categoryParam.cursor = 0;
    work->titleParam.mode = MANUAL_TITLE_MODE_CATEGORY;
    work->titleParam.category = 0;
    work->titleParam.scroll = 0;
    work->titleParam.cursor = 0;
    work->explainParam.pageCount = 1;
    work->explainParam.titleMsg = 0;
    work->explainParam.pages[0].image = MANUAL_IMAGE_NONE;
    work->explainParam.pages[0].msg = 0;
    work->scene = SCENE_TOP;
    work->top = ManualTop_Create(&work->topParam, work->common);
    return TRUE;
}

static BOOL Manual_Exit(GameProc *proc, u32 *state, void *param, void *data) {
    Manual *work = data;

    switch (work->scene) {
    case SCENE_TOP:
        ManualTop_Free(work->top);
        break;
    case SCENE_CATEGORY:
        ManualCategory_Free(work->category);
        break;
    case SCENE_TITLE:
        ManualTitle_Free(work->title);
        break;
    case SCENE_EXPLAIN:
        ManualExplain_Free(work->explain);
        break;
    }
    GFL_TCBRemove(work->vblankTask);
    ManualCommon_Free(work->common);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_MANUAL);
    return TRUE;
}

static BOOL Manual_Main(GameProc *proc, u32 *state, void *param, void *data) {
    Manual *work = data;

    switch (*state) {
    case STATE_START:
        *state = STATE_FADE_IN;
        break;
    case STATE_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            *state = STATE_MAIN;
        }
        break;
    case STATE_MAIN: {
        BOOL end = FALSE;

        switch (work->scene) {
        case SCENE_TOP:
            if (ManualTop_Main(work->top)) {
                if (work->topParam.result <= MANUAL_TOP_RESULT_RETURN) {
                    if (work->topParam.result == MANUAL_TOP_RESULT_CLOSE) {
                        work->param->result = MANUAL_RESULT_CLOSE;
                    } else if (work->topParam.result == MANUAL_TOP_RESULT_RETURN) {
                        work->param->result = MANUAL_RESULT_RETURN;
                    }
                    end = TRUE;
                } else {
                    ManualTop_Free(work->top);
                    if (work->topParam.result == MANUAL_TOP_RESULT_CATEGORY) {
                        work->scene = SCENE_CATEGORY;
                        work->categoryParam.scroll = 0;
                        work->categoryParam.cursor = 0;
                        work->category = ManualCategory_Create(&work->categoryParam, work->common);
                    } else if (work->topParam.result == MANUAL_TOP_RESULT_TITLE) {
                        work->scene = SCENE_TITLE;
                        work->titleParam.mode = MANUAL_TITLE_MODE_ALL;
                        work->titleParam.category = 0;
                        work->titleParam.scroll = 0;
                        work->titleParam.cursor = 0;
                        work->title = ManualTitle_Create(&work->titleParam, work->common);
                    }
                }
            }
            break;
        case SCENE_CATEGORY:
            if (ManualCategory_Main(work->category)) {
                ManualCategory_Free(work->category);
                if (work->categoryParam.result == MANUAL_LIST_RESULT_RETURN) {
                    work->scene = SCENE_TOP;
                    work->top = ManualTop_Create(&work->topParam, work->common);
                } else {
                    work->scene = SCENE_TITLE;
                    work->titleParam.mode = MANUAL_TITLE_MODE_CATEGORY;
                    work->titleParam.category = work->categoryParam.selected;
                    work->titleParam.scroll = 0;
                    work->titleParam.cursor = 0;
                    work->title = ManualTitle_Create(&work->titleParam, work->common);
                }
            }
            break;
        case SCENE_TITLE:
            if (ManualTitle_Main(work->title)) {
                ManualTitle_Free(work->title);
                if (work->titleParam.result == MANUAL_LIST_RESULT_RETURN) {
                    if (work->titleParam.mode == MANUAL_TITLE_MODE_CATEGORY) {
                        work->scene = SCENE_CATEGORY;
                        work->category = ManualCategory_Create(&work->categoryParam, work->common);
                    } else if (work->titleParam.mode == MANUAL_TITLE_MODE_ALL) {
                        work->scene = SCENE_TOP;
                        work->top = ManualTop_Create(&work->topParam, work->common);
                    }
                } else {
                    u16 title;
                    u16 pageCount;
                    u16 flag;
                    u16 i;

                    work->scene = SCENE_EXPLAIN;
                    title = work->titleParam.selected;
                    pageCount = ManualData_GetTitlePageCount(work->common->data, title);
                    flag = ManualData_GetTitleReadFlag(work->common->data, title);
                    if (!ManualData_IsRead(work->common->data, flag, work->common->gameData)) {
                        ManualData_SetRead(work->common->data, flag, work->common->gameData);
                    }
                    work->explainParam.titleMsg = ManualData_GetTitleMsg(work->common->data, title);
                    work->explainParam.pageCount = ManualData_GetTitlePageCount(work->common->data, title);
                    for (i = 0; i < pageCount; i++) {
                        u16 image = ManualData_GetPageImage(work->common->data, title, i);

                        if (!ManualData_IsImageValid(work->common->data, image)) {
                            image = MANUAL_IMAGE_NONE;
                        }
                        work->explainParam.pages[i].image = image;
                        work->explainParam.pages[i].msg = ManualData_GetPageMsg(work->common->data, title, i);
                    }
                    work->explain = ManualExplain_Create(&work->explainParam, work->common);
                }
            }
            break;
        case SCENE_EXPLAIN:
            if (ManualExplain_Main(work->explain)) {
                ManualExplain_Free(work->explain);
                if (work->explainParam.result == 0) {
                    work->scene = SCENE_TITLE;
                    work->title = ManualTitle_Create(&work->titleParam, work->common);
                }
            }
            break;
        }
        if (end) {
            *state = STATE_FADE_OUT;
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        }
        break;
    }
    case STATE_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            *state = STATE_END;
        }
        break;
    case STATE_END:
        return TRUE;
    }
    ManualCommon_Main(work->common);
    return FALSE;
}

static void Manual_VBlank(TCB *tcb, void *data) {
}
