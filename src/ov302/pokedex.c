// The ROM names this file zukan.c.
#include "types.h"
#include "app/pokedex.h"
#include "app/pokedex_area_list.h"
#include "app/pokedex_common.h"
#include "app/pokedex_detail.h"
#include "app/pokedex_habitat.h"
#include "app/pokedex_list.h"
#include "app/pokedex_open.h"
#include "app/pokedex_search.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "save/medal_box.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/season.h"

// The Pokédex: a state machine that starts the process of each of its screens, waits for it, and picks the next from
// what it returns

// The heap of the Pokédex

// The states
enum {
    STATE_WAIT_PROC,
    STATE_OPEN,
    STATE_AFTER_OPEN,
    STATE_LIST,
    STATE_AFTER_LIST,
    STATE_DETAIL,
    STATE_AFTER_DETAIL,
    STATE_SEARCH,
    STATE_AFTER_SEARCH,
    STATE_HABITAT,
    STATE_AFTER_HABITAT,
    STATE_AWARDS,
    STATE_AFTER_AWARDS,
    STATE_EXIT,
};

// What the list returns
enum {
    LIST_RESULT_BACK,
    LIST_RESULT_CLOSE,
    LIST_RESULT_DETAIL_NEXT,
    LIST_RESULT_DETAIL,
    LIST_RESULT_SEARCH,
    LIST_RESULT_AWARDS,
};

// What the search returns
enum {
    SEARCH_RESULT_FOUND,
    SEARCH_RESULT_CANCEL,
    SEARCH_RESULT_CLOSE,
};

// What the habitat list returns
enum {
    HABITAT_RESULT_BACK,
    HABITAT_RESULT_BACK_TO_DETAIL,
    HABITAT_RESULT_AREA,
};

// What the awards' list returns
enum {
    AWARDS_RESULT_NONE,
    AWARDS_RESULT_CLOSE,
    AWARDS_RESULT_HABITATS,
    AWARDS_RESULT_NATIONAL,
    AWARDS_RESULT_REGIONAL,
};

typedef struct {
    PokedexParam *param;
    GameProcManager *procManager;
    BOOL procRunning;
    u16 *list;
    u16 count;
    // 1 if the list is a search's
    u16 searched;
    u16 *habitatList;
    u16 habitatCount;
    PokedexSearchCriteria criteria;
    u32 state;
    u32 nextState;
    // The detail page to open
    u32 page;
    // 1 if the detail pages page through the habitat's species
    BOOL fromHabitat;
    // 1 if the habitat list came from a detail page
    BOOL fromDetail;
    void *subParam;
} PokedexWork;

typedef u32 (*PokedexStateFunc)(PokedexWork *work);

static BOOL Pokedex_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Pokedex_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Pokedex_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void Pokedex_FreeList(PokedexWork *work);
static void Pokedex_FreeHabitatList(PokedexWork *work);
static u32 Pokedex_SetNextState(PokedexWork *work, u32 state);
static u32 Pokedex_StateWaitProc(PokedexWork *work);
static u32 Pokedex_StateOpen(PokedexWork *work);
static u32 Pokedex_StateAfterOpen(PokedexWork *work);
static u32 Pokedex_StateList(PokedexWork *work);
static u32 Pokedex_StateAfterList(PokedexWork *work);
static u32 Pokedex_StateSearch(PokedexWork *work);
static u32 Pokedex_StateAfterSearch(PokedexWork *work);
static u32 Pokedex_StateDetail(PokedexWork *work);
static u32 Pokedex_StateAfterDetail(PokedexWork *work);
static u32 Pokedex_StateHabitat(PokedexWork *work);
static u32 Pokedex_StateAfterHabitat(PokedexWork *work);
static u32 Pokedex_StateAwards(PokedexWork *work);
static u32 Pokedex_StateAfterAwards(PokedexWork *work);

const GameProcFunctions POKEDEX_PROC_FUNCTIONS = { Pokedex_Init, Pokedex_Main, Pokedex_Exit };

static const PokedexStateFunc sPokedexStates[STATE_EXIT] = {
    Pokedex_StateWaitProc,    Pokedex_StateOpen,    Pokedex_StateAfterOpen,    Pokedex_StateList,
    Pokedex_StateAfterList,   Pokedex_StateDetail,  Pokedex_StateAfterDetail,  Pokedex_StateSearch,
    Pokedex_StateAfterSearch, Pokedex_StateHabitat, Pokedex_StateAfterHabitat, Pokedex_StateAwards,
    Pokedex_StateAfterAwards,
};

static BOOL Pokedex_Init(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexWork *wk;
    PokedexParam *pokedexParam = param;
    PokedexHabitatSave *habitat;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_POKEDEX, 0x10000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PokedexWork), HEAPID_POKEDEX);
    MI_CpuFill8(wk, 0, sizeof(PokedexWork));
    wk->param = pokedexParam;
    wk->procManager = CreateGameProcManager(HEAPID_POKEDEX);
    wk->count = PokedexCommon_CreateList(wk->param->pokedex, &wk->list, HEAPID_POKEDEX);
    wk->searched = FALSE;
    PokedexCommon_SelectSeen(wk->param->pokedex, wk->list, wk->count);
    PokedexCommon_InitCriteria(wk->param->pokedex, &wk->criteria);
    habitat = SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(wk->param->gameData));
    PokedexHabitatSave_SetSeason(habitat, Season_GetRealTime());
    PokedexHabitatSave_SetType(habitat, 0);
    if (wk->param->start == POKEDEX_START_OPEN) {
        wk->state = STATE_OPEN;
    } else if (wk->param->start == POKEDEX_START_LIST) {
        if (PokeDex_GetMode(wk->param->pokedex) == DEX_HABITAT) {
            wk->state = STATE_AWARDS;
        } else {
            wk->state = STATE_LIST;
        }
    } else if (wk->param->start == POKEDEX_START_INFO) {
        wk->state = STATE_DETAIL;
        wk->page = POKEDEX_DETAIL_PAGE_INFO;
    } else if (wk->param->start == POKEDEX_START_MAP) {
        wk->state = STATE_DETAIL;
        wk->page = POKEDEX_DETAIL_PAGE_MAP;
    } else if (wk->param->start == POKEDEX_START_VOICE) {
        wk->state = STATE_DETAIL;
        wk->page = POKEDEX_DETAIL_PAGE_VOICE;
    } else if (wk->param->start == POKEDEX_START_FORM) {
        wk->state = STATE_DETAIL;
        wk->page = POKEDEX_DETAIL_PAGE_FORM;
    } else if (wk->param->start == POKEDEX_START_AWARDS) {
        wk->state = STATE_AWARDS;
    } else if (wk->param->start == POKEDEX_START_HABITATS) {
        wk->state = STATE_HABITAT;
    } else {
        wk->state = STATE_SEARCH;
    }
    return TRUE;
}

static BOOL Pokedex_Main(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexWork *wk = work;

    if (wk->state == STATE_EXIT) {
        return TRUE;
    }
    wk->procRunning = GFL_ProcMgrUpdate(wk->procManager);
    wk->state = sPokedexStates[wk->state](wk);
    return FALSE;
}

static BOOL Pokedex_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexWork *wk = work;

    Pokedex_FreeList(wk);
    Pokedex_FreeHabitatList(wk);
    FreeGameProcManager(wk->procManager);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_POKEDEX);
    return TRUE;
}

static void Pokedex_FreeList(PokedexWork *work) {
    if (work->list != NULL) {
        GFL_HeapFree(work->list);
        work->list = NULL;
    }
}

static void Pokedex_FreeHabitatList(PokedexWork *work) {
    if (work->habitatList != NULL) {
        GFL_HeapFree(work->habitatList);
        work->habitatList = NULL;
    }
}

static u32 Pokedex_SetNextState(PokedexWork *work, u32 state) {
    work->nextState = state;
    return STATE_WAIT_PROC;
}

static u32 Pokedex_StateWaitProc(PokedexWork *work) {
    if (work->procRunning != TRUE) {
        return work->nextState;
    }
    return STATE_WAIT_PROC;
}

static u32 Pokedex_StateOpen(PokedexWork *work) {
    PokedexOpenParam *open = GFL_HeapAllocate(HEAPID_POKEDEX, sizeof(PokedexOpenParam), FALSE, "zukan.c", 362);

    work->subParam = open;
    open->gameData = work->param->gameData;
    open->playerInfo = work->param->playerInfo;
    QueueGameProc(work->procManager, OVERLAY_POKEDEX_OPEN, &POKEDEX_OPEN_PROC_FUNCTIONS, work->subParam);
    return Pokedex_SetNextState(work, STATE_AFTER_OPEN);
}

static u32 Pokedex_StateAfterOpen(PokedexWork *work) {
    PokedexOpenParam *open = work->subParam;
    u32 next;

    switch (open->result) {
    case POKEDEX_OPEN_RESULT_BACK:
        work->param->result = 0;
        next = STATE_EXIT;
        break;
    case POKEDEX_OPEN_RESULT_CLOSE:
        work->param->result = 1;
        next = STATE_EXIT;
        break;
    case POKEDEX_OPEN_RESULT_DONE:
        if (PokeDex_GetMode(work->param->pokedex) == DEX_HABITAT) {
            next = STATE_AWARDS;
        } else {
            next = STATE_LIST;
        }
        break;
    }
    GFL_HeapFree(work->subParam);
    return next;
}

static u32 Pokedex_StateList(PokedexWork *work) {
    PokedexListParam *list = GFL_HeapAllocate(HEAPID_POKEDEX, sizeof(PokedexListParam), FALSE, "zukan.c", 430);

    work->subParam = list;
    list->gameData = work->param->gameData;
    list->pokedex = work->param->pokedex;
    if (PokeDex_GetMode(list->pokedex) == DEX_HABITAT) {
        PokeDex_SetMode(work->param->pokedex, DEX_REGIONAL);
    }
    list->searched = work->searched;
    if (work->searched == 0 && PokedexCommon_IsFirstShown(work->param->pokedex) == 0) {
        list->list = work->list + 1;
        list->count = work->count - 1;
    } else {
        list->list = work->list;
        list->count = work->count;
    }
    QueueGameProc(work->procManager, OVERLAY_ID(299), &PokedexList_ProcFunctions, work->subParam);
    return Pokedex_SetNextState(work, STATE_AFTER_LIST);
}

static u32 Pokedex_StateAfterList(PokedexWork *work) {
    PokedexListParam *list = work->subParam;
    u32 next;

    switch (list->result) {
    case LIST_RESULT_BACK:
        if (work->searched == 0) {
            work->param->result = 0;
            next = STATE_EXIT;
        } else {
            Pokedex_FreeList(work);
            work->count = PokedexCommon_CreateList(work->param->pokedex, &work->list, HEAPID_POKEDEX);
            work->searched = 0;
            next = STATE_LIST;
        }
        break;
    case LIST_RESULT_CLOSE:
        work->param->result = 1;
        next = STATE_EXIT;
        break;
    case LIST_RESULT_DETAIL_NEXT:
        Pokedex_FreeList(work);
        work->count = PokedexCommon_CreateList(work->param->pokedex, &work->list, HEAPID_POKEDEX);
        next = STATE_LIST;
        break;
    case LIST_RESULT_DETAIL:
        next = STATE_DETAIL;
        work->page = POKEDEX_DETAIL_PAGE_INFO;
        break;
    case LIST_RESULT_SEARCH:
        if (work->searched == 1) {
            Pokedex_FreeList(work);
            work->count = PokedexCommon_CreateList(work->param->pokedex, &work->list, HEAPID_POKEDEX);
        }
        next = STATE_SEARCH;
        break;
    case LIST_RESULT_AWARDS:
        next = STATE_AWARDS;
        break;
    default:
        work->param->result = 0;
        next = STATE_EXIT;
        break;
    }
    GFL_HeapFree(work->subParam);
    return next;
}

static u32 Pokedex_StateSearch(PokedexWork *work) {
    PokedexSearchParam *search = GFL_HeapAllocate(HEAPID_POKEDEX, sizeof(PokedexSearchParam), FALSE, "zukan.c", 537);

    work->subParam = search;
    search->gameData = work->param->gameData;
    search->pokedex = work->param->pokedex;
    search->criteria = &work->criteria;
    search->list = NULL;
    search->count = 0;
    QueueGameProc(work->procManager, OVERLAY_ID(300), &PokedexSearch_ProcFunctions, work->subParam);
    return Pokedex_SetNextState(work, STATE_AFTER_SEARCH);
}

static u32 Pokedex_StateAfterSearch(PokedexWork *work) {
    PokedexSearchParam *search = work->subParam;
    u32 next;

    switch (search->result) {
    case SEARCH_RESULT_FOUND:
        Pokedex_FreeList(work);
        if (PokeDex_GetMode(work->param->pokedex) == DEX_HABITAT) {
            PokeDex_SetMode(work->param->pokedex, DEX_REGIONAL);
        }
        work->list = search->list;
        work->count = search->count;
        work->searched = 1;
        next = STATE_LIST;
        break;
    case SEARCH_RESULT_CANCEL:
        if (PokeDex_GetMode(work->param->pokedex) == DEX_HABITAT) {
            PokeDex_SetMode(work->param->pokedex, DEX_REGIONAL);
        }
        work->searched = 0;
        next = STATE_LIST;
        break;
    case SEARCH_RESULT_CLOSE:
    default:
        work->param->result = 1;
        next = STATE_EXIT;
        break;
    }
    GFL_HeapFree(work->subParam);
    return next;
}

static u32 Pokedex_StateDetail(PokedexWork *work) {
    u16 current = PokeDex_GetLastSpecies(GameData_GetPokedex(work->param->gameData));
    PokedexDetailParam *detail = GFL_HeapAllocate(HEAPID_POKEDEX, sizeof(PokedexDetailParam), FALSE, "zukan.c", 622);
    u16 i;

    detail->gameData = work->param->gameData;
    detail->page = work->page;
    detail->index = 0;
    detail->mode = work->fromHabitat;
    if (work->fromHabitat == 1) {
        detail->list = work->habitatList;
        detail->count = work->habitatCount;
    } else {
        detail->list = work->list;
        detail->count = work->count;
    }
    for (i = 0; i < detail->count; i++) {
        if (current == detail->list[i]) {
            detail->index = i;
            break;
        }
    }
    QueueGameProc(work->procManager, OVERLAY_POKEDEX_DETAIL, &POKEDEX_DETAIL_PROC_FUNCTIONS, detail);
    work->subParam = detail;
    return Pokedex_SetNextState(work, STATE_AFTER_DETAIL);
}

static u32 Pokedex_StateAfterDetail(PokedexWork *work) {
    PokeDexSave *pokedex = GameData_GetPokedex(work->param->gameData);
    PokedexDetailParam *detail = work->subParam;
    u32 next;

    if (work->fromHabitat == 0) {
        work->fromDetail = 0;
    }
    PokeDex_SetLastSpecies(pokedex, detail->list[detail->index]);
    switch (detail->result) {
    case POKEDEX_DETAIL_RESULT_CLOSE:
        work->param->result = 1;
        next = STATE_EXIT;
        break;
    case POKEDEX_DETAIL_RESULT_PLACE:
        next = STATE_HABITAT;
        work->fromDetail = 1;
        break;
    case POKEDEX_DETAIL_RESULT_RETURN:
        if (work->fromHabitat == 0) {
            if (PokeDex_GetMode(work->param->pokedex) == DEX_HABITAT) {
                PokeDex_SetMode(work->param->pokedex, DEX_REGIONAL);
            }
            work->param->result = 0;
            next = STATE_LIST;
        } else {
            work->param->result = 0;
            next = STATE_HABITAT;
        }
        break;
    }
    if (work->fromHabitat == 1) {
        Pokedex_FreeHabitatList(work);
    }
    GFL_HeapFree(work->subParam);
    return next;
}

static u32 Pokedex_StateHabitat(PokedexWork *work) {
    PokedexHabitatSave *habitat = SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(work->param->gameData));
    PokedexHabitatListParam *habitatList;

    work->subParam = GFL_HeapAllocate(HEAPID_POKEDEX, sizeof(PokedexHabitatListParam), FALSE, "zukan.c", 751);
    MI_CpuFill8(work->subParam, 0, sizeof(PokedexHabitatListParam));
    habitatList = work->subParam;
    habitatList->gameData = work->param->gameData;
    if (work->fromDetail == 1 && work->fromHabitat == 0) {
        PokedexCommon_BuildHabitatList(habitat, habitatList, TRUE);
    } else if (work->fromDetail == 1 && work->fromHabitat == 1) {
        PokedexCommon_BuildHabitatList(habitat, habitatList, TRUE);
    } else {
        PokedexCommon_BuildHabitatList(habitat, habitatList, FALSE);
    }
    QueueGameProc(work->procManager, OVERLAY_ID(303), &PokedexHabitat_ProcFunctions, habitatList);
    return Pokedex_SetNextState(work, STATE_AFTER_HABITAT);
}

static u32 Pokedex_StateAfterHabitat(PokedexWork *work) {
    PokedexHabitatListParam *habitatList = work->subParam;
    u32 next;

    switch (habitatList->result) {
    case HABITAT_RESULT_BACK:
        work->param->result = 1;
        next = STATE_EXIT;
        work->fromHabitat = 0;
        break;
    case HABITAT_RESULT_BACK_TO_DETAIL:
        if (work->fromDetail == 1) {
            work->page = POKEDEX_DETAIL_PAGE_MAP;
            next = STATE_DETAIL;
        } else {
            PokeDex_SetMode(work->param->pokedex, DEX_HABITAT);
            next = STATE_AWARDS;
        }
        work->fromHabitat = 0;
        break;
    case HABITAT_RESULT_AREA: {
        PokedexHabitatSave *habitat = SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(work->param->gameData));

        next = STATE_DETAIL;
        work->page = POKEDEX_DETAIL_PAGE_INFO;
        work->fromHabitat = 1;
        work->habitatCount =
            PokedexCommon_LoadHabitatSpecies(work->param->pokedex, habitat, &work->habitatList, HEAPID_POKEDEX);
        break;
    }
    default:
        work->param->result = 1;
        next = STATE_EXIT;
        work->fromHabitat = 0;
        break;
    }
    GFL_HeapFree(work->subParam);
    return next;
}

static u32 Pokedex_StateAwards(PokedexWork *work) {
    PokedexHabitatSave *habitat = SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(work->param->gameData));
    PokedexHabitatSummaryParam *summary;

    work->subParam = GFL_HeapAllocate(HEAPID_POKEDEX, sizeof(PokedexHabitatSummaryParam), FALSE, "zukan.c", 860);
    MI_CpuFill8(work->subParam, 0, sizeof(PokedexHabitatSummaryParam));
    summary = work->subParam;
    summary->gameData = work->param->gameData;
    Pokedex_FreeList(work);
    Pokedex_FreeHabitatList(work);
    PokedexCommon_BuildHabitatSummary(habitat, summary);
    QueueGameProc(work->procManager, OVERLAY_ID(304), &PokedexAreaList_ProcFunctions, summary);
    return Pokedex_SetNextState(work, STATE_AFTER_AWARDS);
}

static u32 Pokedex_StateAfterAwards(PokedexWork *work) {
    PokedexHabitatSummaryParam *summary = work->subParam;
    u32 next;

    switch (summary->result) {
    case AWARDS_RESULT_NONE:
        next = STATE_EXIT;
        work->param->result = 1;
        break;
    case AWARDS_RESULT_CLOSE:
        next = STATE_EXIT;
        work->param->result = 0;
        break;
    case AWARDS_RESULT_HABITATS: {
        PokedexHabitatSave *habitat = SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(work->param->gameData));

        PokedexHabitatSave_SetSeason(habitat, Season_GetRealTime());
        PokedexHabitatSave_SetType(habitat, 0);
        PokeDex_SetMode(work->param->pokedex, DEX_HABITAT);
        next = STATE_HABITAT;
        break;
    }
    case AWARDS_RESULT_NATIONAL:
        PokeDex_GetMode(work->param->pokedex);
        PokeDex_SetMode(work->param->pokedex, DEX_NATIONAL);
        Pokedex_FreeList(work);
        work->count = PokedexCommon_CreateList(work->param->pokedex, &work->list, HEAPID_POKEDEX);
        next = STATE_LIST;
        break;
    case AWARDS_RESULT_REGIONAL:
        PokeDex_GetMode(work->param->pokedex);
        PokeDex_SetMode(work->param->pokedex, DEX_REGIONAL);
        Pokedex_FreeList(work);
        work->count = PokedexCommon_CreateList(work->param->pokedex, &work->list, HEAPID_POKEDEX);
        next = STATE_LIST;
        break;
    default:
        next = STATE_EXIT;
        work->param->result = 1;
        break;
    }
    GFL_HeapFree(work->subParam);
    return next;
}
