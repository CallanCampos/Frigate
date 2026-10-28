// The ROM names this file zukan_detail.c.
#include "types.h"
#include "app/pokedex_detail.h"
#include "constants/arc.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "save/pokedex_savedata.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"

// The Pokédex's detail screen: sets up the bars and the systems that its pages share, and runs the page that the bars
// switch to

typedef struct {
    HeapID heapId;
    PokedexDetailGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    TCB *vblankTcb;
    PokedexDetailTouchbar *touchbar;
    PokedexDetailHeadbar *headbar;
    PokedexDetailCommon *common;
    PokedexDetailProcSys *procSys;
    void *pageParam;
    // The page to switch to once the current one has exited, or POKEDEX_DETAIL_PAGE_NONE
    int nextPage;
} PokedexDetailWork;

static BOOL PokedexDetail_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexDetail_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexDetail_Main(GameProc *proc, u32 *state, void *param, void *work);
static void PokedexDetail_VBlank(TCB *tcb, void *data);
static void PokedexDetail_StartPage(PokedexDetailParam *param, PokedexDetailWork *wk);

const GameProcFunctions POKEDEX_DETAIL_PROC_FUNCTIONS = { PokedexDetail_Init, PokedexDetail_Main, PokedexDetail_Exit };

static BOOL PokedexDetail_Init(GameProc *proc, u32 *state, void *param_, void *work) {
    PokedexDetailParam *param = param_;
    PokedexDetailWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_POKEDEX_DETAIL, 0x90000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PokedexDetailWork), HEAPID_POKEDEX_DETAIL);
    MI_CpuFill8(wk, 0, sizeof(PokedexDetailWork));
    wk->heapId = HEAPID_POKEDEX_DETAIL;
    param->result = POKEDEX_DETAIL_RESULT_CLOSE;

    // The graphics are created with the 3D system and freed at once, then created for good without it
    wk->graphic = PokedexDetailGraphic_Create(0, wk->heapId, TRUE);
    PokedexDetailGraphic_Free(wk->graphic);
    wk->graphic = PokedexDetailGraphic_Create(0, wk->heapId, FALSE);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->printQueue = PrintQueue_CreateDefault(wk->heapId);
    wk->vblankTcb = GFL_VBlankTCBAdd(PokedexDetail_VBlank, wk, 1);
    wk->touchbar =
        PokedexDetailTouchbar_Create(wk->heapId, PokeDex_IsGiven(GameData_GetPokedex(param->gameData)), param->mode);
    wk->headbar = PokedexDetailHeadbar_Create(wk->heapId, wk->font);
    wk->common = PokedexDetailCommon_Create(wk->heapId, param->gameData, wk->graphic, wk->font, wk->printQueue,
                                            wk->touchbar, wk->headbar, param->list, param->count, &param->index);
    wk->procSys = NULL;
    wk->pageParam = NULL;
    PokedexDetail_StartPage(param, wk);
    wk->nextPage = POKEDEX_DETAIL_PAGE_NONE;

    GFL_FadeSet(3, 0, 0, 0);
    GFL_NetSetIconScreen(TRUE, wk->heapId);
    return TRUE;
}

static BOOL PokedexDetail_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexDetailWork *wk = work;

    GFL_FadeSet(3, 16, 16, 0);
    if (wk->procSys != NULL) {
        PokedexDetailProcSys_Free(wk->procSys);
    }
    if (wk->pageParam != NULL) {
        GFL_HeapFree(wk->pageParam);
    }
    PokedexDetailCommon_Free(wk->common);
    PokedexDetailHeadbar_Free(wk->headbar);
    PokedexDetailTouchbar_Free(wk->touchbar);
    GFL_TCBRemove(wk->vblankTcb);
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    GFL_FontFree(wk->font);
    PokedexDetailGraphic_Free(wk->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_POKEDEX_DETAIL);
    return TRUE;
}

static BOOL PokedexDetail_Main(GameProc *proc, u32 *state, void *param_, void *work) {
    PokedexDetailParam *param = param_;
    PokedexDetailWork *wk = work;
    BOOL pageChanged = FALSE;
    int command;

    // The state counts the first frames, and the original compares it as signed
    if ((int)*state <= 8) {
        (*state)++;
    }

    if (PokedexDetailProcSys_Main(wk->procSys, wk->common)) {
        if (wk->nextPage != POKEDEX_DETAIL_PAGE_NONE) {
            param->page = wk->nextPage;
            PokedexDetail_StartPage(param, wk);
            wk->nextPage = POKEDEX_DETAIL_PAGE_NONE;
            pageChanged = TRUE;
        } else {
            return TRUE;
        }
    }

    if (!pageChanged) {
        command = PokedexDetailTouchbar_GetTrigger(wk->touchbar);
        if (command == POKEDEX_DETAIL_CMD_NONE) {
            command = PokedexDetailTouchbar_GetTouch(wk->touchbar);
        }
        PokedexDetailProcSys_Command(wk->procSys, wk->common, command);
        switch (command) {
        case POKEDEX_DETAIL_CMD_CLOSE:
            param->result = POKEDEX_DETAIL_RESULT_CLOSE;
            break;
        case POKEDEX_DETAIL_CMD_RETURN:
            param->result = POKEDEX_DETAIL_RESULT_RETURN;
            break;
        case POKEDEX_DETAIL_CMD_INFO:
            wk->nextPage = POKEDEX_DETAIL_PAGE_INFO;
            break;
        case POKEDEX_DETAIL_CMD_MAP:
            wk->nextPage = POKEDEX_DETAIL_PAGE_MAP;
            break;
        case POKEDEX_DETAIL_CMD_VOICE:
            wk->nextPage = POKEDEX_DETAIL_PAGE_VOICE;
            break;
        case POKEDEX_DETAIL_CMD_FORM:
            wk->nextPage = POKEDEX_DETAIL_PAGE_FORM;
            break;
        case POKEDEX_DETAIL_CMD_MAP_PLACE_TOUCH:
            param->place = ((PokedexDetailMapParam *)wk->pageParam)->place;
            param->result = POKEDEX_DETAIL_RESULT_PLACE;
            break;
        }
    }

    PokedexDetailTouchbar_Update(wk->touchbar);
    PokedexDetailHeadbar_Update(wk->headbar);
    PrintQueue_Main(wk->printQueue);
    PokedexDetailGraphic_Update(wk->graphic);
    PokedexDetailGraphic_Begin3D(wk->graphic);
    if (!pageChanged) {
        PokedexDetailProcSys_Draw(wk->procSys, wk->common);
    }
    PokedexDetailGraphic_End3D(wk->graphic);
    return FALSE;
}

static void PokedexDetail_VBlank(TCB *tcb, void *data) {
}

static void PokedexDetail_StartPage(PokedexDetailParam *param, PokedexDetailWork *wk) {
    const PokedexDetailProcFuncs *funcs;

    if (wk->procSys != NULL) {
        PokedexDetailProcSys_Free(wk->procSys);
    }
    if (wk->pageParam != NULL) {
        GFL_HeapFree(wk->pageParam);
    }

    switch (param->page) {
    case POKEDEX_DETAIL_PAGE_INFO: {
        PokedexDetailInfoParam *pageParam =
            GFL_HeapAllocate(wk->heapId, sizeof(PokedexDetailInfoParam), TRUE, "zukan_detail.c", 462);
        PokedexDetailInfo_InitParam(pageParam, wk->heapId);
        wk->pageParam = pageParam;
        funcs = &POKEDEX_DETAIL_INFO_PROC_FUNCS;
        break;
    }
    case POKEDEX_DETAIL_PAGE_MAP: {
        PokedexDetailMapParam *pageParam =
            GFL_HeapAllocate(wk->heapId, sizeof(PokedexDetailMapParam), TRUE, "zukan_detail.c", 470);
        PokedexDetailMap_InitParam(pageParam, wk->heapId);
        wk->pageParam = pageParam;
        funcs = &POKEDEX_DETAIL_MAP_PROC_FUNCS;
        break;
    }
    case POKEDEX_DETAIL_PAGE_VOICE: {
        PokedexDetailVoiceParam *pageParam =
            GFL_HeapAllocate(wk->heapId, sizeof(PokedexDetailVoiceParam), TRUE, "zukan_detail.c", 478);
        PokedexDetailVoice_InitParam(pageParam, wk->heapId);
        wk->pageParam = pageParam;
        funcs = &POKEDEX_DETAIL_VOICE_PROC_FUNCS;
        break;
    }
    case POKEDEX_DETAIL_PAGE_FORM: {
        PokedexDetailFormParam *pageParam =
            GFL_HeapAllocate(wk->heapId, sizeof(PokedexDetailFormParam), TRUE, "zukan_detail.c", 486);
        PokedexDetailForm_InitParam(pageParam, wk->heapId);
        wk->pageParam = pageParam;
        funcs = &POKEDEX_DETAIL_FORM_PROC_FUNCS;
        break;
    }
    }

    wk->procSys = PokedexDetailProcSys_Create(funcs, wk->pageParam, wk->heapId);
}
