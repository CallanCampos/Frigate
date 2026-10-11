#include "types.h"
#include "app/zukan.h"
#include "app/zukan_common.h"
#include "app/zukan_detail.h"
#include "app/zukan_open.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "save/medal_box.h"
#include "save/pokedex.h"
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

// Which Pokédex counts, with func_0200d1f8's values
#define DEX_REGIONAL 0
#define DEX_NATIONAL 1
#define DEX_NATIONAL_OBTAINED 2

// What the list is given, 0x14 bytes (overlay 299)
typedef struct {
    GameData *gameData;
    PokeDexSave *pokedex;
    // Whether the list is a search's
    u16 searched;
    u16 count;
    u16 *list;
    int result;
} ZukanListParam;

// What the search is given, 0x18 bytes (overlay 300)
typedef struct {
    GameData *gameData;
    PokeDexSave *pokedex;
    ZukanSearchCriteria *criteria;
    int result;
    u16 *list;
    u16 count;
} ZukanSearchParam;

typedef struct {
    ZukanParam *param;
    GameProcManager *procManager;
    BOOL procRunning;
    u16 *list;
    u16 count;
    // 1 if the list is a search's
    u16 searched;
    u16 *habitatList;
    u16 habitatCount;
    ZukanSearchCriteria criteria;
    u32 state;
    u32 nextState;
    // The detail page to open
    u32 page;
    // 1 if the detail pages page through the habitat's species
    BOOL fromHabitat;
    // 1 if the habitat list came from a detail page
    BOOL fromDetail;
    void *subParam;
} ZukanWork;

typedef u32 (*ZukanStateFunc)(ZukanWork *work);

extern const GameProcFunctions data_ov299_021a25c8;
extern const GameProcFunctions data_ov300_021a4094;
extern const GameProcFunctions data_ov303_021a32d8;
extern const GameProcFunctions data_ov304_021a1dac;

static BOOL Zukan_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Zukan_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Zukan_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void Zukan_FreeList(ZukanWork *work);
static void Zukan_FreeHabitatList(ZukanWork *work);
static u32 Zukan_SetNextState(ZukanWork *work, u32 state);
static u32 Zukan_StateWaitProc(ZukanWork *work);
static u32 Zukan_StateOpen(ZukanWork *work);
static u32 Zukan_StateAfterOpen(ZukanWork *work);
static u32 Zukan_StateList(ZukanWork *work);
static u32 Zukan_StateAfterList(ZukanWork *work);
static u32 Zukan_StateSearch(ZukanWork *work);
static u32 Zukan_StateAfterSearch(ZukanWork *work);
static u32 Zukan_StateDetail(ZukanWork *work);
static u32 Zukan_StateAfterDetail(ZukanWork *work);
static u32 Zukan_StateHabitat(ZukanWork *work);
static u32 Zukan_StateAfterHabitat(ZukanWork *work);
static u32 Zukan_StateAwards(ZukanWork *work);
static u32 Zukan_StateAfterAwards(ZukanWork *work);

const GameProcFunctions ZUKAN_PROC_FUNCTIONS = { Zukan_Init, Zukan_Main, Zukan_Exit };

static const ZukanStateFunc sZukanStates[STATE_EXIT] = {
    Zukan_StateWaitProc,    Zukan_StateOpen,    Zukan_StateAfterOpen,    Zukan_StateList,
    Zukan_StateAfterList,   Zukan_StateDetail,  Zukan_StateAfterDetail,  Zukan_StateSearch,
    Zukan_StateAfterSearch, Zukan_StateHabitat, Zukan_StateAfterHabitat, Zukan_StateAwards,
    Zukan_StateAfterAwards,
};

static BOOL Zukan_Init(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanWork *wk;
    ZukanParam *zukanParam = param;
    void *habitat;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_ZUKAN, 0x10000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(ZukanWork), HEAPID_ZUKAN);
    sys_memset(wk, 0, sizeof(ZukanWork));
    wk->param = zukanParam;
    wk->procManager = CreateGameProcManager(HEAPID_ZUKAN);
    wk->count = ZukanCommon_CreateList(wk->param->pokedex, &wk->list, HEAPID_ZUKAN);
    wk->searched = FALSE;
    ZukanCommon_SelectSeen(wk->param->pokedex, wk->list, wk->count);
    ZukanCommon_InitCriteria(wk->param->pokedex, &wk->criteria);
    habitat = func_02010cb8(GameData_GetSaveControl(wk->param->gameData));
    func_02010d80(habitat, Season_GetRealTime());
    func_02010d90(habitat, 0);
    if (wk->param->start == ZUKAN_START_OPEN) {
        wk->state = STATE_OPEN;
    } else if (wk->param->start == ZUKAN_START_LIST) {
        if (func_0200d1f8(wk->param->pokedex) == DEX_NATIONAL_OBTAINED) {
            wk->state = STATE_AWARDS;
        } else {
            wk->state = STATE_LIST;
        }
    } else if (wk->param->start == ZUKAN_START_INFO) {
        wk->state = STATE_DETAIL;
        wk->page = ZUKAN_DETAIL_PAGE_INFO;
    } else if (wk->param->start == ZUKAN_START_MAP) {
        wk->state = STATE_DETAIL;
        wk->page = ZUKAN_DETAIL_PAGE_MAP;
    } else if (wk->param->start == ZUKAN_START_VOICE) {
        wk->state = STATE_DETAIL;
        wk->page = ZUKAN_DETAIL_PAGE_VOICE;
    } else if (wk->param->start == ZUKAN_START_FORM) {
        wk->state = STATE_DETAIL;
        wk->page = ZUKAN_DETAIL_PAGE_FORM;
    } else if (wk->param->start == ZUKAN_START_AWARDS) {
        wk->state = STATE_AWARDS;
    } else if (wk->param->start == ZUKAN_START_HABITATS) {
        wk->state = STATE_HABITAT;
    } else {
        wk->state = STATE_SEARCH;
    }
    return TRUE;
}

static BOOL Zukan_Main(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanWork *wk = work;

    if (wk->state == STATE_EXIT) {
        return TRUE;
    }
    wk->procRunning = GFL_ProcMgrUpdate(wk->procManager);
    wk->state = sZukanStates[wk->state](wk);
    return FALSE;
}

static BOOL Zukan_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanWork *wk = work;

    Zukan_FreeList(wk);
    Zukan_FreeHabitatList(wk);
    FreeGameProcManager(wk->procManager);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_ZUKAN);
    return TRUE;
}

static void Zukan_FreeList(ZukanWork *work) {
    if (work->list != NULL) {
        GFL_HeapFree(work->list);
        work->list = NULL;
    }
}

static void Zukan_FreeHabitatList(ZukanWork *work) {
    if (work->habitatList != NULL) {
        GFL_HeapFree(work->habitatList);
        work->habitatList = NULL;
    }
}

static u32 Zukan_SetNextState(ZukanWork *work, u32 state) {
    work->nextState = state;
    return STATE_WAIT_PROC;
}

static u32 Zukan_StateWaitProc(ZukanWork *work) {
    if (work->procRunning != TRUE) {
        return work->nextState;
    }
    return STATE_WAIT_PROC;
}

static u32 Zukan_StateOpen(ZukanWork *work) {
    ZukanOpenParam *open = GFL_HeapAllocate(HEAPID_ZUKAN, sizeof(ZukanOpenParam), FALSE, "zukan.c", 362);

    work->subParam = open;
    open->gameData = work->param->gameData;
    open->playerInfo = work->param->playerInfo;
    QueueGameProc(work->procManager, OVERLAY_ZUKAN_OPEN, &ZUKAN_OPEN_PROC_FUNCTIONS, work->subParam);
    return Zukan_SetNextState(work, STATE_AFTER_OPEN);
}

static u32 Zukan_StateAfterOpen(ZukanWork *work) {
    ZukanOpenParam *open = work->subParam;
    u32 next;

    switch (open->result) {
    case ZUKAN_OPEN_RESULT_BACK:
        work->param->result = 0;
        next = STATE_EXIT;
        break;
    case ZUKAN_OPEN_RESULT_CLOSE:
        work->param->result = 1;
        next = STATE_EXIT;
        break;
    case ZUKAN_OPEN_RESULT_DONE:
        if (func_0200d1f8(work->param->pokedex) == DEX_NATIONAL_OBTAINED) {
            next = STATE_AWARDS;
        } else {
            next = STATE_LIST;
        }
        break;
    }
    GFL_HeapFree(work->subParam);
    return next;
}

static u32 Zukan_StateList(ZukanWork *work) {
    ZukanListParam *list = GFL_HeapAllocate(HEAPID_ZUKAN, sizeof(ZukanListParam), FALSE, "zukan.c", 430);

    work->subParam = list;
    list->gameData = work->param->gameData;
    list->pokedex = work->param->pokedex;
    if (func_0200d1f8(list->pokedex) == DEX_NATIONAL_OBTAINED) {
        func_0200d1e4(work->param->pokedex, DEX_REGIONAL);
    }
    list->searched = work->searched;
    if (work->searched == 0 && ZukanCommon_IsFirstShown(work->param->pokedex) == 0) {
        list->list = work->list + 1;
        list->count = work->count - 1;
    } else {
        list->list = work->list;
        list->count = work->count;
    }
    QueueGameProc(work->procManager, OVERLAY_ID(299), &data_ov299_021a25c8, work->subParam);
    return Zukan_SetNextState(work, STATE_AFTER_LIST);
}

static u32 Zukan_StateAfterList(ZukanWork *work) {
    ZukanListParam *list = work->subParam;
    u32 next;

    switch (list->result) {
    case LIST_RESULT_BACK:
        if (work->searched == 0) {
            work->param->result = 0;
            next = STATE_EXIT;
        } else {
            Zukan_FreeList(work);
            work->count = ZukanCommon_CreateList(work->param->pokedex, &work->list, HEAPID_ZUKAN);
            work->searched = 0;
            next = STATE_LIST;
        }
        break;
    case LIST_RESULT_CLOSE:
        work->param->result = 1;
        next = STATE_EXIT;
        break;
    case LIST_RESULT_DETAIL_NEXT:
        Zukan_FreeList(work);
        work->count = ZukanCommon_CreateList(work->param->pokedex, &work->list, HEAPID_ZUKAN);
        next = STATE_LIST;
        break;
    case LIST_RESULT_DETAIL:
        next = STATE_DETAIL;
        work->page = ZUKAN_DETAIL_PAGE_INFO;
        break;
    case LIST_RESULT_SEARCH:
        if (work->searched == 1) {
            Zukan_FreeList(work);
            work->count = ZukanCommon_CreateList(work->param->pokedex, &work->list, HEAPID_ZUKAN);
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

static u32 Zukan_StateSearch(ZukanWork *work) {
    ZukanSearchParam *search = GFL_HeapAllocate(HEAPID_ZUKAN, sizeof(ZukanSearchParam), FALSE, "zukan.c", 537);

    work->subParam = search;
    search->gameData = work->param->gameData;
    search->pokedex = work->param->pokedex;
    search->criteria = &work->criteria;
    search->list = NULL;
    search->count = 0;
    QueueGameProc(work->procManager, OVERLAY_ID(300), &data_ov300_021a4094, work->subParam);
    return Zukan_SetNextState(work, STATE_AFTER_SEARCH);
}

static u32 Zukan_StateAfterSearch(ZukanWork *work) {
    ZukanSearchParam *search = work->subParam;
    u32 next;

    switch (search->result) {
    case SEARCH_RESULT_FOUND:
        Zukan_FreeList(work);
        if (func_0200d1f8(work->param->pokedex) == DEX_NATIONAL_OBTAINED) {
            func_0200d1e4(work->param->pokedex, DEX_REGIONAL);
        }
        work->list = search->list;
        work->count = search->count;
        work->searched = 1;
        next = STATE_LIST;
        break;
    case SEARCH_RESULT_CANCEL:
        if (func_0200d1f8(work->param->pokedex) == DEX_NATIONAL_OBTAINED) {
            func_0200d1e4(work->param->pokedex, DEX_REGIONAL);
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

static u32 Zukan_StateDetail(ZukanWork *work) {
    u16 current = func_0200d214(GameData_GetPokedex(work->param->gameData));
    ZukanDetailParam *detail = GFL_HeapAllocate(HEAPID_ZUKAN, sizeof(ZukanDetailParam), FALSE, "zukan.c", 622);
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
    QueueGameProc(work->procManager, OVERLAY_ZUKAN_DETAIL, &ZUKAN_DETAIL_PROC_FUNCTIONS, detail);
    work->subParam = detail;
    return Zukan_SetNextState(work, STATE_AFTER_DETAIL);
}

static u32 Zukan_StateAfterDetail(ZukanWork *work) {
    PokeDexSave *pokedex = GameData_GetPokedex(work->param->gameData);
    ZukanDetailParam *detail = work->subParam;
    u32 next;

    if (work->fromHabitat == 0) {
        work->fromDetail = 0;
    }
    func_0200d200(pokedex, detail->list[detail->index]);
    switch (detail->result) {
    case ZUKAN_DETAIL_RESULT_CLOSE:
        work->param->result = 1;
        next = STATE_EXIT;
        break;
    case ZUKAN_DETAIL_RESULT_PLACE:
        next = STATE_HABITAT;
        work->fromDetail = 1;
        break;
    case ZUKAN_DETAIL_RESULT_RETURN:
        if (work->fromHabitat == 0) {
            if (func_0200d1f8(work->param->pokedex) == DEX_NATIONAL_OBTAINED) {
                func_0200d1e4(work->param->pokedex, DEX_REGIONAL);
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
        Zukan_FreeHabitatList(work);
    }
    GFL_HeapFree(work->subParam);
    return next;
}

static u32 Zukan_StateHabitat(ZukanWork *work) {
    void *habitat = func_02010cb8(GameData_GetSaveControl(work->param->gameData));
    ZukanHabitatListParam *habitatList;

    work->subParam = GFL_HeapAllocate(HEAPID_ZUKAN, sizeof(ZukanHabitatListParam), FALSE, "zukan.c", 751);
    sys_memset(work->subParam, 0, sizeof(ZukanHabitatListParam));
    habitatList = work->subParam;
    habitatList->gameData = work->param->gameData;
    if (work->fromDetail == 1 && work->fromHabitat == 0) {
        ZukanCommon_BuildHabitatList(habitat, habitatList, TRUE);
    } else if (work->fromDetail == 1 && work->fromHabitat == 1) {
        ZukanCommon_BuildHabitatList(habitat, habitatList, TRUE);
    } else {
        ZukanCommon_BuildHabitatList(habitat, habitatList, FALSE);
    }
    QueueGameProc(work->procManager, OVERLAY_ID(303), &data_ov303_021a32d8, habitatList);
    return Zukan_SetNextState(work, STATE_AFTER_HABITAT);
}

static u32 Zukan_StateAfterHabitat(ZukanWork *work) {
    ZukanHabitatListParam *habitatList = work->subParam;
    u32 next;

    switch (habitatList->result) {
    case HABITAT_RESULT_BACK:
        work->param->result = 1;
        next = STATE_EXIT;
        work->fromHabitat = 0;
        break;
    case HABITAT_RESULT_BACK_TO_DETAIL:
        if (work->fromDetail == 1) {
            work->page = ZUKAN_DETAIL_PAGE_MAP;
            next = STATE_DETAIL;
        } else {
            func_0200d1e4(work->param->pokedex, DEX_NATIONAL_OBTAINED);
            next = STATE_AWARDS;
        }
        work->fromHabitat = 0;
        break;
    case HABITAT_RESULT_AREA: {
        void *habitat = func_02010cb8(GameData_GetSaveControl(work->param->gameData));

        next = STATE_DETAIL;
        work->page = ZUKAN_DETAIL_PAGE_INFO;
        work->fromHabitat = 1;
        work->habitatCount =
            ZukanCommon_LoadHabitatSpecies(work->param->pokedex, habitat, &work->habitatList, HEAPID_ZUKAN);
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

static u32 Zukan_StateAwards(ZukanWork *work) {
    void *habitat = func_02010cb8(GameData_GetSaveControl(work->param->gameData));
    ZukanHabitatSummaryParam *summary;

    work->subParam = GFL_HeapAllocate(HEAPID_ZUKAN, sizeof(ZukanHabitatSummaryParam), FALSE, "zukan.c", 860);
    sys_memset(work->subParam, 0, sizeof(ZukanHabitatSummaryParam));
    summary = work->subParam;
    summary->gameData = work->param->gameData;
    Zukan_FreeList(work);
    Zukan_FreeHabitatList(work);
    ZukanCommon_BuildHabitatSummary(habitat, summary);
    QueueGameProc(work->procManager, OVERLAY_ID(304), &data_ov304_021a1dac, summary);
    return Zukan_SetNextState(work, STATE_AFTER_AWARDS);
}

static u32 Zukan_StateAfterAwards(ZukanWork *work) {
    ZukanHabitatSummaryParam *summary = work->subParam;
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
        void *habitat = func_02010cb8(GameData_GetSaveControl(work->param->gameData));

        func_02010d80(habitat, Season_GetRealTime());
        func_02010d90(habitat, 0);
        func_0200d1e4(work->param->pokedex, DEX_NATIONAL_OBTAINED);
        next = STATE_HABITAT;
        break;
    }
    case AWARDS_RESULT_NATIONAL:
        func_0200d1f8(work->param->pokedex);
        func_0200d1e4(work->param->pokedex, DEX_NATIONAL);
        Zukan_FreeList(work);
        work->count = ZukanCommon_CreateList(work->param->pokedex, &work->list, HEAPID_ZUKAN);
        next = STATE_LIST;
        break;
    case AWARDS_RESULT_REGIONAL:
        func_0200d1f8(work->param->pokedex);
        func_0200d1e4(work->param->pokedex, DEX_REGIONAL);
        Zukan_FreeList(work);
        work->count = ZukanCommon_CreateList(work->param->pokedex, &work->list, HEAPID_ZUKAN);
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
