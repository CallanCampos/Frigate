#include "types.h"
#include "field/event_battle_lose.h"
#include "field/event_game_clear.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/game_beacon_set.h"
#include "field/pleasure_boat.h"
#include "field/zone.h"
#include "gfl/overlay.h"
#include "system/brightness.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

GameEventReturnCode EventBattleLose_Callback(GameEvent *event, u32 *state, void *data) {
    BattleLoseData *work = data;

    switch (*state) {
    case 0:
        BrightnessController_SetScreenBrightness(-16, 0x37, 1);
        BrightnessController_SetScreenBrightness(-16, 0x3f, 2);
        func_ov012_02160618();
        GSYS_QueueProcAsEvent(event, OVERLAY_ID(295), &BATTLE_LOSE_MESSAGE_PROC_FUNCTIONS, work);
        (*state)++;
        break;
    case 1:
        BrightnessController_SetScreenBrightness(0, 0x3f, 3);
        GameEvent_ChainNext(event, EventMapChangeBlackout_CreateExternal(work->gsys));
        (*state)++;
        break;
    case 2:
        if (!work->returnNonLeague) {
            EventScriptCall_Start(event, 2, NULL, 0, HEAPID_FIELDMAP);
        } else {
            EventScriptCall_Start(event, 0x7d1, NULL, 0, HEAPID_FIELDMAP);
        }
        (*state)++;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBattleLose_Create(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleLose_Callback, 12);
    BattleLoseData *data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->returnNonLeague = IsReturnLocationNonLeaguePokeCen(gameData);
    data->playerInfo = GetGameDataPlayerInfo(gameData);
    PleasureBoat_Free(GameData_GetPleasureBoatPtr(gameData));
    func_02016b24(gsys, 0);
    return event;
}
