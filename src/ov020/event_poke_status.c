#include "types.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "field/event_poke_status.h"
#include "field/field_event.h"
#include "gfl/heap.h"
#include "pml/poke_party.h"
#include "save/pokedex_savedata.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The party list events' work
typedef struct {
    GameSystem *gsys;
    Field *field;
    PokeListParam *listParam;
    // The summary screen the list can show, or NULL
    PStatusParam *summaryParam;
    u16 *picked;
    u16 *index;
} PokeSelectWork;

// The move-forgetting summary screen's work
typedef struct {
    GameSystem *gsys;
    Field *field;
    PStatusParam *summaryParam;
    u16 *forgot;
    u16 *slot;
} PokeMoveReplaceWork;

static GameEventReturnCode EventPokeSelect_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventPokeMoveReplace_Callback(GameEvent *event, u32 *state, void *data);

GameEvent *EventPokeSelect_Create(GameSystem *gsys, void *args) {
    PokeSelectArgs *selectArgs = args;
    GameData *gameData = GSYS_GetGameData(gsys);
    PokeListParam *listParam =
        PokeListParam_Create(gameData, selectArgs->mode, GameData_GetParty(gameData), HEAPID_GAMEEVENT);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventPokeSelect_Callback, sizeof(PokeSelectWork));
    PokeSelectWork *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->listParam = listParam;
    work->summaryParam = NULL;
    work->picked = selectArgs->picked;
    work->index = selectArgs->index;
    return event;
}

GameEvent *EventMoveTutorPokeSelect_Create(GameSystem *gsys, void *args) {
    MoveTutorPokeSelectArgs *selectArgs = args;
    GameData *gameData = GSYS_GetGameData(gsys);
    PokeListParam *listParam = PokeListParam_Create(gameData, 0x18, GameData_GetParty(gameData), HEAPID_GAMEEVENT);
    GameEvent *event;
    PokeSelectWork *work;

    listParam->unk6E = selectArgs->learnable;
    listParam->move = selectArgs->move;
    event = GameEvent_Create(gsys, NULL, EventPokeSelect_Callback, sizeof(PokeSelectWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->listParam = listParam;
    work->summaryParam = NULL;
    work->picked = selectArgs->picked;
    work->index = selectArgs->index;
    return event;
}

GameEvent *EventMusicalPokeSelect_Create(GameSystem *gsys, void *args) {
    PokeSelectArgs *selectArgs = args;
    GameData *gameData = GSYS_GetGameData(gsys);
    PokeParty *party = GameData_GetParty(gameData);
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);
    PokeListParam *listParam = PokeListParam_Create(gameData, 0x19, party, HEAPID_GAMEEVENT);
    PStatusParam *summaryParam =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PStatusParam), TRUE, "event_poke_status.c", 138);
    GameEvent *event;
    PokeSelectWork *work;

    summaryParam->party = party;
    summaryParam->gameData = gameData;
    summaryParam->dataType = PSTATUS_DATA_PARTY;
    summaryParam->mode = PSTATUS_MODE_1;
    summaryParam->partyCount = PokeParty_GetPkmCount(party);
    summaryParam->page = PSTATUS_PAGE_INFO;
    summaryParam->isNationalDex = PokeDex_IsNationalObtained(pokedex);
    summaryParam->fromFieldMenu = FALSE;
    event = GameEvent_Create(gsys, NULL, EventPokeSelect_Callback, sizeof(PokeSelectWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->listParam = listParam;
    work->summaryParam = summaryParam;
    work->picked = selectArgs->picked;
    work->index = selectArgs->index;
    return event;
}

static GameEventReturnCode EventPokeSelect_Callback(GameEvent *event, u32 *state, void *data) {
    PokeSelectWork *work = data;
    GameSystem *gsys = work->gsys;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceOutTransitionDefault(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0));
        *state = 1;
        break;
    case 1:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, work->field));
        *state = 2;
        break;
    case 2:
        GameEvent_ChainNext(event, EventPokeList_Create(gsys, work->field, work->listParam, work->summaryParam));
        *state = 3;
        break;
    case 3:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        *state = 4;
        break;
    case 4:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceInTransition(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        *state = 5;
        break;
    case 5:
        // A signed compare: the index is an s32 in the original, which PokeListParam doesn't declare yet
        if ((s32)work->listParam->index <= 5) {
            *work->index = work->listParam->index;
            *work->picked = TRUE;
        } else {
            *work->index = 0;
            *work->picked = FALSE;
        }
        GFL_HeapFree(work->listParam);
        if (work->summaryParam != NULL) {
            GFL_HeapFree(work->summaryParam);
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventPokeMoveReplace_Create(GameSystem *gsys, void *args) {
    PokeMoveReplaceArgs *replaceArgs = args;
    GameData *gameData = GSYS_GetGameData(gsys);
    PokeParty *party = GameData_GetParty(gameData);
    PStatusParam *summaryParam =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PStatusParam), TRUE, "event_poke_status.c", 269);
    GameEvent *event;
    PokeMoveReplaceWork *work;

    summaryParam->party = party;
    summaryParam->dataType = PSTATUS_DATA_PARTY;
    summaryParam->partyCount = PokeParty_GetPkmCount(party);
    summaryParam->partyIndex = replaceArgs->partyIndex;
    summaryParam->page = PSTATUS_PAGE_SKILL;
    summaryParam->move = replaceArgs->move;
    if (replaceArgs->forgetHm == TRUE) {
        summaryParam->mode = PSTATUS_MODE_FORGET_HM;
    } else {
        summaryParam->mode = PSTATUS_MODE_FORGET_MOVE;
    }
    summaryParam->fromFieldMenu = FALSE;
    summaryParam->gameData = gameData;
    event = GameEvent_Create(gsys, NULL, EventPokeMoveReplace_Callback, sizeof(PokeMoveReplaceWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->summaryParam = summaryParam;
    work->forgot = replaceArgs->forgot;
    work->slot = replaceArgs->slot;
    return event;
}

static GameEventReturnCode EventPokeMoveReplace_Callback(GameEvent *event, u32 *state, void *data) {
    PokeMoveReplaceWork *work = data;
    GameSystem *gsys = work->gsys;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceOutTransitionDefault(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0));
        *state = 1;
        break;
    case 1:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, work->field));
        *state = 2;
        break;
    case 2:
        GSYS_QueueProc(gsys, OVERLAY_PSTATUS, &PSTATUS_PROC_FUNCTIONS, work->summaryParam);
        *state = 3;
        break;
    case 3:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        *state = 4;
        break;
    case 4:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceInTransition(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        *state = 5;
        break;
    case 5:
        if (work->summaryParam->result == PSTATUS_RESULT_FORGET) {
            *work->slot = work->summaryParam->slot;
            *work->forgot = TRUE;
        } else {
            *work->slot = 0;
            *work->forgot = FALSE;
        }
        GFL_HeapFree(work->summaryParam);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
