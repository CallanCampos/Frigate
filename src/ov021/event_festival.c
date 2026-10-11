#include "types.h"
#include "app/festival.h"
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

struct FestivalEventWork {
    GameSystem *gsys;
    GameData *gameData;
    SaveControl *save;
    FestivalEventParam param;
    u32 mode;
    u32 communicationFlag;
    GameCommSys *comm;
};

GameEventReturnCode func_ov021_0216e660(GameEvent *event, u32 *state, void *data);
void func_ov021_0216e848(FestivalEventParam *param, GameSystem *gsys, GameData *gameData, u32 mode);

GameEventReturnCode func_ov021_0216e660(GameEvent *event, u32 *state, void *data) {
    FestivalEventWork *work = data;
    GameSystem *gsys = work->gsys;
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);

    switch (*state) {
    case 0: {
        GameEvent *transition;
        if (work->mode == 0) {
            transition = CallFieldMapEntranceOutTransitionDefault(gsys, field, 0, 0);
        } else {
            transition = CallFieldMapEntranceOutTransitionDefault(gsys, field, 1, 0);
        }
        GameEvent_ChainNext(event, transition);
        if (func_02042788() == 1 && func_ov036_02180f80(work->comm) == 1 && !GameCommSys_IsTransitioning(work->comm)) {
            func_ov030_02174108(1);
            work->communicationFlag = 1;
        }
        (*state)++;
        break;
    }
    case 1:
        if (work->mode == 1) {
            func_ov036_021b6690(Field_GetFesGimmick(field));
        }
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        (*state)++;
        break;
    case 2:
        FieldSnd_DuckVolume(GameData_GetFieldSoundSystem(gameData), GameSystem_GetISS(gsys));
        func_ov021_0216e848(&work->param, work->gsys, work->gameData, work->mode);
        GSYS_QueueProc(gsys, OVERLAY_FESTIVAL_APP, &data_ov309_021a01d0, &work->param);
        (*state)++;
        break;
    case 3:
        if (!GSYS_GetProcMgrState(gsys)) {
            (*state)++;
        }
        break;
    case 4: {
        u32 bgm = -1;
        if (work->mode == 1) {
            bgm = LinkFestival_GetNormalChangeBGMID(GSYS_GetLinkFestival(work->gsys));
        }
        if (bgm != (u32)-1) {
            GameEvent_ChainNext(event, EventBGMPlay_Create(work->gsys, bgm));
        }
        (*state)++;
        break;
    }
    case 5:
        FieldSnd_RestoreVolume(GameData_GetFieldSoundSystem(gameData), GameSystem_GetISS(gsys));
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case 6: {
        GameEvent *transition;
        if (work->mode == 0) {
            transition = CallFieldMapEntranceInTransition(gsys, field, 0, 0, 1, 0, 0);
        } else {
            FieldSubscreen_ChangeImm(Field_GetSubscreen(field), 0);
            transition = CallFieldMapEntranceInTransition(gsys, field, 1, 0, 1, 0, 0);
        }
        GameEvent_ChainNext(event, transition);
        (*state)++;
        break;
    }
    case 7:
        if (work->communicationFlag && func_02042788() == 1 && func_ov036_02180f80(work->comm)) {
            func_ov030_02174108(0);
            work->communicationFlag = 0;
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *func_ov021_0216e80c(GameSystem *gsys, void *args) {
    u32 mode = *(u32 *)args;
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov021_0216e660, sizeof(FestivalEventWork));
    FestivalEventWork *work = GameEvent_GetData(event);

    work->gameData = GSYS_GetGameData(gsys);
    work->save = GameData_GetSaveControl(work->gameData);
    work->gsys = gsys;
    work->mode = mode;
    work->comm = GSYS_GetGameCommSystem(gsys);
    return event;
}

void func_ov021_0216e848(FestivalEventParam *param, GameSystem *gsys, GameData *gameData, u32 mode) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    sys_memset(param, 0, sizeof(*param));
    param->gsys = gsys;
    param->gameData = gameData;
    param->mode = mode;
    param->festival = GSYS_GetLinkFestival(gsys);
    param->missionConfig = GetFestMissionCfg(param->festival);
    param->saveBlock = func_02010dec(save);
}
