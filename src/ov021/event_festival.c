#include "types.h"
#include "app/festival.h"
#include "app/fmission_app.h"
#include "field/event_festival.h"
#include "field/event_sound.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_sound.h"
#include "gfl/net.h"
#include "gfl/net_whpipe.h"
#include "gfl/std.h"
#include "save/save_control.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The steps of the event
enum {
    STEP_FADE_OUT,
    STEP_CLOSE_FIELD,
    STEP_START_APP,
    STEP_WAIT_APP,
    STEP_PLAY_BGM,
    STEP_OPEN_FIELD,
    STEP_FADE_IN,
    STEP_END,
};

struct FestivalEventWork {
    GameSystem *gsys;
    GameData *gameData;
    SaveControl *save;
    FestivalEventParam param;
    u32 mode;
    // The Funfest's beacon search was paused for the screen
    u32 searchPaused;
    GameCommSys *comm;
};

GameEventReturnCode EventFestival_Callback(GameEvent *event, u32 *state, void *data);
void EventFestival_SetupParam(FestivalEventParam *param, GameSystem *gsys, GameData *gameData, u32 mode);

GameEventReturnCode EventFestival_Callback(GameEvent *event, u32 *state, void *data) {
    FestivalEventWork *work = data;
    GameSystem *gsys = work->gsys;
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);

    switch (*state) {
    case STEP_FADE_OUT: {
        GameEvent *transition;
        if (work->mode == FMISSION_APP_MODE_MISSION) {
            transition = CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0);
        } else {
            transition = CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_WHITE, 0);
        }
        GameEvent_ChainNext(event, transition);
        if (GFL_NetIsInitialized() == 1 && Field_GetCommKind(work->comm) == GAME_COMM_NO_BEACON_SEARCH &&
            !GameCommSys_IsTransitioning(work->comm)) {
            NetWhpipe_SetSearchPaused(1);
            work->searchPaused = TRUE;
        }
        (*state)++;
        break;
    }
    case STEP_CLOSE_FIELD:
        if (work->mode == FMISSION_APP_MODE_RESULT) {
            FesGimmick_EndMission(Field_GetFesGimmick(field));
        }
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        (*state)++;
        break;
    case STEP_START_APP:
        FieldSnd_DuckVolume(GameData_GetFieldSoundSystem(gameData), GameSystem_GetISS(gsys));
        EventFestival_SetupParam(&work->param, work->gsys, work->gameData, work->mode);
        GSYS_QueueProc(gsys, OVERLAY_FESTIVAL_APP, &FMISSION_APP_PROC_FUNCTIONS, &work->param);
        (*state)++;
        break;
    case STEP_WAIT_APP:
        if (!GSYS_GetProcMgrState(gsys)) {
            (*state)++;
        }
        break;
    case STEP_PLAY_BGM: {
        u32 bgm = -1;
        if (work->mode == FMISSION_APP_MODE_RESULT) {
            bgm = LinkFestival_GetNormalChangeBGMID(GSYS_GetLinkFestival(work->gsys));
        }
        if (bgm != (u32)-1) {
            GameEvent_ChainNext(event, EventBGMPlay_Create(work->gsys, bgm));
        }
        (*state)++;
        break;
    }
    case STEP_OPEN_FIELD:
        FieldSnd_RestoreVolume(GameData_GetFieldSoundSystem(gameData), GameSystem_GetISS(gsys));
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case STEP_FADE_IN: {
        GameEvent *transition;
        if (work->mode == FMISSION_APP_MODE_MISSION) {
            transition = CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0);
        } else {
            FieldSubscreen_ChangeImm(Field_GetSubscreen(field), 0);
            transition = CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_WHITE, 0, 1, 0, 0);
        }
        GameEvent_ChainNext(event, transition);
        (*state)++;
        break;
    }
    case STEP_END:
        if (work->searchPaused && GFL_NetIsInitialized() == 1 && Field_GetCommKind(work->comm)) {
            NetWhpipe_SetSearchPaused(0);
            work->searchPaused = FALSE;
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventFestival_Create(GameSystem *gsys, void *args) {
    u32 mode = *(u32 *)args;
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFestival_Callback, sizeof(FestivalEventWork));
    FestivalEventWork *work = GameEvent_GetData(event);

    work->gameData = GSYS_GetGameData(gsys);
    work->save = GameData_GetSaveControl(work->gameData);
    work->gsys = gsys;
    work->mode = mode;
    work->comm = GSYS_GetGameCommSystem(gsys);
    return event;
}

void EventFestival_SetupParam(FestivalEventParam *param, GameSystem *gsys, GameData *gameData, u32 mode) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    MI_CpuFill8(param, 0, sizeof(*param));
    param->gsys = gsys;
    param->gameData = gameData;
    param->mode = mode;
    param->festival = GSYS_GetLinkFestival(gsys);
    param->missionConfig = GetFestMissionCfg(param->festival);
    param->saveBlock = FestRecords_GetFromSave(save);
}
