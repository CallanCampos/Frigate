#include "types.h"
#include "app/demo3d.h"
#include "field/event_3d_demo.h"
#include "field/event_sound.h"
#include "field/game_beacon_set.h"
#include "gfl/std.h"
#include "nitro/rtc.h"
#include "save/player_info.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/season.h"

GameEvent *Event3DDemo_Create(GameSystem *gsys, GameEvent *parent, u32 demoId, u32 param, u32 realTime) {
    GameEvent *event = GameEvent_Create(gsys, parent, Event3DDemo_Callback, sizeof(Event3DDemoWork));
    Event3DDemoWork *work = GameEvent_GetData(event);
    sys_memset(work, 0, sizeof(Event3DDemoWork));
    work->gameSystem = gsys;
    work->gameCommSys = GSYS_GetGameCommSystem(gsys);
    work->demoId = demoId;
    if (realTime) {
        SetupPlaySequenceEventRealTime(&work->param, gsys, demoId, param);
    } else {
        SetupPlaySequenceEventGameTime(&work->param, gsys, demoId, param);
    }
    if (demoId == 3) {
        GameBeacon_BroadcastFerrisWheel();
    }
    return event;
}

GameEventReturnCode Event3DDemo_Callback(GameEvent *event, u32 *state, void *data) {
    Event3DDemoWork *work = data;

    switch (*state) {
    case 0:
        work->savedState = func_02016b2c(work->gameSystem);
        func_02016b24(work->gameSystem, 1);
        if (!GameCommSys_BootCheck(work->gameCommSys)) {
            *state += 2;
        } else {
            GameCommSys_ExitReq(work->gameCommSys);
            (*state)++;
        }
        break;
    case 1:
        if (!GameCommSys_BootCheck(work->gameCommSys)) {
            (*state)++;
        }
        break;
    case 2:
        GSYS_QueueProc(work->gameSystem, OVERLAY_DEMO3D, &gDemo3DProcFunctions, &work->param);
        (*state)++;
        break;
    case 3:
        if (GSYS_GetProcMgrState(work->gameSystem)) {
            break;
        }
        func_02016b24(work->gameSystem, work->savedState);
        if (work->demoId == 24 || work->demoId == 25) {
            GameEvent_ChainNext(event, CreateBGMFadeOutEvent(work->gameSystem, 0));
            if (work->demoId == 25) {
                SetNowWeather(GSYS_GetGameData(work->gameSystem), 0);
            }
        }
        (*state)++;
        break;
    case 4:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void CreateSeqLoadParam(Demo3DParam *out, GameSystem *gsys, u32 demoId, u8 param, u32 arg4, u8 hour, u8 minute,
                        u8 season) {
    sys_memset(out, 0, sizeof(Demo3DParam));
    out->gsys = gsys;
    out->demoId = demoId;
    out->param = param;
    out->unk08 = arg4;
    out->hour = hour;
    out->minute = minute;
    out->season = season;
    out->playerSex = getTrainerGender(GetGameDataPlayerInfo(GSYS_GetGameData(gsys)));
}

void SetupPlaySequenceEventRealTime(Demo3DParam *out, GameSystem *gsys, u8 demoId, u8 param) {
    RTCTime time;
    u8 season;

    RTC_GetCachedTime(&time);
    season = Season_GetRealTime();
    CreateSeqLoadParam(out, gsys, demoId, param, 0, time.hour, time.minute, season);
}

void SetupPlaySequenceEventGameTime(Demo3DParam *out, GameSystem *gsys, u8 demoId, u8 param) {
    GameData *data;
    u32 hour;
    u32 minute;
    u8 season;

    data = GSYS_GetGameData(gsys);
    hour = getCurrentHour(data);
    minute = getCurrentMinute(data);
    season = GameData_GetSeason(data);
    CreateSeqLoadParam(out, gsys, demoId, param, 0, hour, minute, season);
}
