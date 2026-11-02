// The Pokémon World Tournament's events, which the script plugins of its entrance (overlay 56) and stadium (overlay 57)
// and overlay 55's commands start: picking the Pokémon to enter, the list of Trainers, a round's battle, and the
// records and downloaded tournaments. The name is descriptive
#include "field/event_wbt.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "app/t_download.h"
#include "app/wbt_list.h"
#include "app/win_record.h"
#include "field/event_battle.h"
#include "field/field_event.h"
#include "field/wbt.h"
#include "field/wbt_battle.h"
#include "field/wbt_makematch.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The steps of picking the Pokémon to enter
enum {
    POKE_SELECT_FADE_OUT,
    POKE_SELECT_CLOSE_FIELD,
    POKE_SELECT_START_LIST,
    POKE_SELECT_OPEN_FIELD,
    POKE_SELECT_FADE_IN,
    POKE_SELECT_END,
};

// The steps of the events that run one thing: start it, then clean up after it
enum {
    RUN_START,
    RUN_END,
};

typedef struct {
    GameSystem *gsys;
    Field *field;
    WbtSystem *sys;
    PStatusParam summaryParam;
    PokeListParam partyParam;
} WbtPokeSelectWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    WbtSetup *setup;
    // Whether the list runs through EventFieldSubprocessTransition_Create
    u32 useTransition;
} WbtListWork;

typedef struct {
    GameSystem *gsys;
    WbtSystem *sys;
    BtlSetup *setup;
} WbtBattleWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    void *param;
} WbtProcWork;

static GameEventReturnCode EventWbtPokeSelect_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventWbtList_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventWbtBattle_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventWbtWinRecord_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventWbtDownload_Callback(GameEvent *event, u32 *state, void *data);

static GameEventReturnCode EventWbtPokeSelect_Callback(GameEvent *event, u32 *state, void *data) {
    WbtPokeSelectWork *work = data;
    GameSystem *gsys = work->gsys;

    switch (*state) {
    case POKE_SELECT_FADE_OUT:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceOutTransitionDefault(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0));
        (*state)++;
        break;
    case POKE_SELECT_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, work->field));
        (*state)++;
        break;
    case POKE_SELECT_START_LIST:
        GameEvent_ChainNext(event, EventPokeList_Create(gsys, work->field, &work->partyParam, &work->summaryParam));
        (*state)++;
        break;
    case POKE_SELECT_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case POKE_SELECT_FADE_IN:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceInTransition(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case POKE_SELECT_END:
        WbtMakeMatch_ReadPick(work->sys, &work->partyParam);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventWbtPokeSelect_Create(GameSystem *gsys, void *args) {
    WbtSystem *sys = args;
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWbtPokeSelect_Callback, sizeof(WbtPokeSelectWork));
    WbtPokeSelectWork *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->field = field;
    work->sys = sys;
    WbtMakeMatch_SetupPokeList(sys, gameData, &work->partyParam);
    WbtMakeMatch_SetupPStatus(sys, gameData, &work->summaryParam);
    return event;
}

GameEvent *EventWbtList_Create(GameSystem *gsys, void *args) {
    WbtSetup *setup = args;
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWbtList_Callback, sizeof(WbtListWork));
    WbtListWork *work = GameEvent_GetData(event);

    MI_CpuFill8(work, 0, sizeof(WbtListWork));
    work->gsys = gsys;
    work->field = field;
    work->setup = setup;
    work->useTransition = setup->useTransition;
    return event;
}

static GameEventReturnCode EventWbtList_Callback(GameEvent *event, u32 *state, void *data) {
    WbtListWork *work = data;
    GameEvent *next;

    switch (*state) {
    case RUN_START:
        GFL_SndPlayerSetVolume(2, 63);
        if (work->useTransition) {
            next = EventFieldSubprocessTransition_Create(work->gsys, work->field, OVERLAY_WBT_LIST,
                                                         &WBT_LIST_PROC_FUNCTIONS, work->setup);
        } else {
            next = EventFieldSubprocessCall_Create(work->gsys, work->field, OVERLAY_WBT_LIST, &WBT_LIST_PROC_FUNCTIONS,
                                                   work->setup);
        }
        GameEvent_ChainNext(event, next);
        *state = RUN_END;
        break;
    case RUN_END:
        WbtSetup_Free(work->setup);
        GFL_SndPlayerSetVolume(2, 127);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode EventWbtBattle_Callback(GameEvent *event, u32 *state, void *data) {
    WbtBattleWork *work = data;
    GameSystem *gsys = work->gsys;
    WbtSystem *sys = work->sys;
    Field *field;

    switch (*state) {
    case RUN_START:
        field = GSYS_GetField(gsys);
        work->setup = WbtBattle_SetupBattle(sys, gsys);
        GameEvent_ChainNext(event, EventWbtBattleCall_Create(gsys, field, work->setup));
        *state = RUN_END;
        break;
    case RUN_END:
        WbtBattle_EndBattle(sys, GSYS_GetGameData(gsys), work->setup);
        WbtBattle_FreeBattle(sys, work->setup);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventWbtBattle_Create(GameSystem *gsys, void *args) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWbtBattle_Callback, sizeof(WbtBattleWork));
    WbtBattleWork *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->sys = args;
    return event;
}

GameEvent *EventWbtWinRecord_Create(GameSystem *gsys, void *args) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWbtWinRecord_Callback, sizeof(WbtProcWork));
    WbtProcWork *work = GameEvent_GetData(event);

    MI_CpuFill8(work, 0, sizeof(WbtProcWork));
    work->gsys = gsys;
    work->field = field;
    work->param = args;
    return event;
}

static GameEventReturnCode EventWbtWinRecord_Callback(GameEvent *event, u32 *state, void *data) {
    WbtProcWork *work = data;

    switch (*state) {
    case RUN_START:
        GameEvent_ChainNext(event, EventFieldSubprocessTransition_Create(work->gsys, work->field, OVERLAY_WIN_RECORD,
                                                                         &WIN_RECORD_PROC_FUNCTIONS, work->param));
        *state = RUN_END;
        break;
    case RUN_END:
        WbtSetup_FreeWinRecordParam(work->param);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventWbtDownload_Create(GameSystem *gsys, void *args) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWbtDownload_Callback, sizeof(WbtProcWork));
    WbtProcWork *work = GameEvent_GetData(event);

    MI_CpuFill8(work, 0, sizeof(WbtProcWork));
    work->gsys = gsys;
    work->field = field;
    work->param = args;
    return event;
}

static GameEventReturnCode EventWbtDownload_Callback(GameEvent *event, u32 *state, void *data) {
    WbtProcWork *work = data;

    switch (*state) {
    case RUN_START:
        GameEvent_ChainNext(event, EventFieldSubprocessTransition_Create(work->gsys, work->field, OVERLAY_T_DOWNLOAD,
                                                                         &T_DOWNLOAD_PROC_FUNCTIONS, work->param));
        *state = RUN_END;
        break;
    case RUN_END:
        WbtSetup_FreeDownloadParam(work->param);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
