#include "types.h"
#include "app/zukan_detail.h"
#include "constants/arc.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "save/pokedex.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"

// The Pokédex's detail screen: sets up the bars and the systems that its pages share, and runs the page that the bars
// switch to

typedef struct {
    HeapID heapId;
    ZukanDetailGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    TCB *vblankTcb;
    ZukanDetailTouchbar *touchbar;
    ZukanDetailHeadbar *headbar;
    ZukanDetailCommon *common;
    ZukanDetailProcSys *procSys;
    void *pageParam;
    // The page to switch to once the current one has exited, or ZUKAN_DETAIL_PAGE_NONE
    int nextPage;
} ZukanDetailWork;

static BOOL ZukanDetail_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanDetail_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanDetail_Main(GameProc *proc, u32 *state, void *param, void *work);
static void ZukanDetail_VBlank(TCB *tcb, void *data);
static void ZukanDetail_StartPage(ZukanDetailParam *param, ZukanDetailWork *wk);

const GameProcFunctions ZUKAN_DETAIL_PROC_FUNCTIONS = { ZukanDetail_Init, ZukanDetail_Main, ZukanDetail_Exit };

static BOOL ZukanDetail_Init(GameProc *proc, u32 *state, void *param_, void *work) {
    ZukanDetailParam *param = param_;
    ZukanDetailWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_ZUKAN_DETAIL, 0x90000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(ZukanDetailWork), HEAPID_ZUKAN_DETAIL);
    MI_CpuFill8(wk, 0, sizeof(ZukanDetailWork));
    wk->heapId = HEAPID_ZUKAN_DETAIL;
    param->result = ZUKAN_DETAIL_RESULT_CLOSE;

    // The graphics are created with the 3D system and freed at once, then created for good without it
    wk->graphic = ZukanDetailGraphic_Create(0, wk->heapId, TRUE);
    ZukanDetailGraphic_Free(wk->graphic);
    wk->graphic = ZukanDetailGraphic_Create(0, wk->heapId, FALSE);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->printQueue = PrintQueue_CreateDefault(wk->heapId);
    wk->vblankTcb = GFL_VBlankTCBAdd(ZukanDetail_VBlank, wk, 1);
    wk->touchbar =
        ZukanDetailTouchbar_Create(wk->heapId, func_0200d1dc(GameData_GetPokedex(param->gameData)), param->mode);
    wk->headbar = ZukanDetailHeadbar_Create(wk->heapId, wk->font);
    wk->common = ZukanDetailCommon_Create(wk->heapId, param->gameData, wk->graphic, wk->font, wk->printQueue,
                                          wk->touchbar, wk->headbar, param->list, param->count, &param->index);
    wk->procSys = NULL;
    wk->pageParam = NULL;
    ZukanDetail_StartPage(param, wk);
    wk->nextPage = ZUKAN_DETAIL_PAGE_NONE;

    GFL_FadeSet(3, 0, 0, 0);
    func_02042ba8(TRUE, wk->heapId);
    return TRUE;
}

static BOOL ZukanDetail_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanDetailWork *wk = work;

    GFL_FadeSet(3, 16, 16, 0);
    if (wk->procSys != NULL) {
        ZukanDetailProcSys_Free(wk->procSys);
    }
    if (wk->pageParam != NULL) {
        GFL_HeapFree(wk->pageParam);
    }
    ZukanDetailCommon_Free(wk->common);
    ZukanDetailHeadbar_Free(wk->headbar);
    ZukanDetailTouchbar_Free(wk->touchbar);
    GFL_TCBRemove(wk->vblankTcb);
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    GFL_FontFree(wk->font);
    ZukanDetailGraphic_Free(wk->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_ZUKAN_DETAIL);
    return TRUE;
}

static BOOL ZukanDetail_Main(GameProc *proc, u32 *state, void *param_, void *work) {
    ZukanDetailParam *param = param_;
    ZukanDetailWork *wk = work;
    BOOL pageChanged = FALSE;
    int command;

    // The state counts the first frames, and the original compares it as signed
    if ((int)*state <= 8) {
        (*state)++;
    }

    if (ZukanDetailProcSys_Main(wk->procSys, wk->common)) {
        if (wk->nextPage != ZUKAN_DETAIL_PAGE_NONE) {
            param->page = wk->nextPage;
            ZukanDetail_StartPage(param, wk);
            wk->nextPage = ZUKAN_DETAIL_PAGE_NONE;
            pageChanged = TRUE;
        } else {
            return TRUE;
        }
    }

    if (!pageChanged) {
        command = ZukanDetailTouchbar_GetTrigger(wk->touchbar);
        if (command == ZUKAN_DETAIL_CMD_NONE) {
            command = ZukanDetailTouchbar_GetTouch(wk->touchbar);
        }
        ZukanDetailProcSys_Command(wk->procSys, wk->common, command);
        switch (command) {
        case ZUKAN_DETAIL_CMD_CLOSE:
            param->result = ZUKAN_DETAIL_RESULT_CLOSE;
            break;
        case ZUKAN_DETAIL_CMD_RETURN:
            param->result = ZUKAN_DETAIL_RESULT_RETURN;
            break;
        case ZUKAN_DETAIL_CMD_INFO:
            wk->nextPage = ZUKAN_DETAIL_PAGE_INFO;
            break;
        case ZUKAN_DETAIL_CMD_MAP:
            wk->nextPage = ZUKAN_DETAIL_PAGE_MAP;
            break;
        case ZUKAN_DETAIL_CMD_VOICE:
            wk->nextPage = ZUKAN_DETAIL_PAGE_VOICE;
            break;
        case ZUKAN_DETAIL_CMD_FORM:
            wk->nextPage = ZUKAN_DETAIL_PAGE_FORM;
            break;
        case ZUKAN_DETAIL_CMD_MAP_PLACE_TOUCH:
            param->place = ((ZukanDetailMapParam *)wk->pageParam)->place;
            param->result = ZUKAN_DETAIL_RESULT_PLACE;
            break;
        }
    }

    ZukanDetailTouchbar_Update(wk->touchbar);
    ZukanDetailHeadbar_Update(wk->headbar);
    PrintQueue_Main(wk->printQueue);
    ZukanDetailGraphic_Update(wk->graphic);
    ZukanDetailGraphic_Begin3D(wk->graphic);
    if (!pageChanged) {
        ZukanDetailProcSys_Draw(wk->procSys, wk->common);
    }
    ZukanDetailGraphic_End3D(wk->graphic);
    return FALSE;
}

static void ZukanDetail_VBlank(TCB *tcb, void *data) {
}

static void ZukanDetail_StartPage(ZukanDetailParam *param, ZukanDetailWork *wk) {
    const ZukanDetailProcFuncs *funcs;

    if (wk->procSys != NULL) {
        ZukanDetailProcSys_Free(wk->procSys);
    }
    if (wk->pageParam != NULL) {
        GFL_HeapFree(wk->pageParam);
    }

    switch (param->page) {
    case ZUKAN_DETAIL_PAGE_INFO: {
        ZukanDetailInfoParam *pageParam =
            GFL_HeapAllocate(wk->heapId, sizeof(ZukanDetailInfoParam), TRUE, "zukan_detail.c", 462);
        ZukanDetailInfo_InitParam(pageParam, wk->heapId);
        wk->pageParam = pageParam;
        funcs = &ZUKAN_DETAIL_INFO_PROC_FUNCS;
        break;
    }
    case ZUKAN_DETAIL_PAGE_MAP: {
        ZukanDetailMapParam *pageParam =
            GFL_HeapAllocate(wk->heapId, sizeof(ZukanDetailMapParam), TRUE, "zukan_detail.c", 470);
        ZukanDetailMap_InitParam(pageParam, wk->heapId);
        wk->pageParam = pageParam;
        funcs = &ZUKAN_DETAIL_MAP_PROC_FUNCS;
        break;
    }
    case ZUKAN_DETAIL_PAGE_VOICE: {
        ZukanDetailVoiceParam *pageParam =
            GFL_HeapAllocate(wk->heapId, sizeof(ZukanDetailVoiceParam), TRUE, "zukan_detail.c", 478);
        ZukanDetailVoice_InitParam(pageParam, wk->heapId);
        wk->pageParam = pageParam;
        funcs = &ZUKAN_DETAIL_VOICE_PROC_FUNCS;
        break;
    }
    case ZUKAN_DETAIL_PAGE_FORM: {
        ZukanDetailFormParam *pageParam =
            GFL_HeapAllocate(wk->heapId, sizeof(ZukanDetailFormParam), TRUE, "zukan_detail.c", 486);
        ZukanDetailForm_InitParam(pageParam, wk->heapId);
        wk->pageParam = pageParam;
        funcs = &ZUKAN_DETAIL_FORM_PROC_FUNCS;
        break;
    }
    }

    wk->procSys = ZukanDetailProcSys_Create(funcs, wk->pageParam, wk->heapId);
}
