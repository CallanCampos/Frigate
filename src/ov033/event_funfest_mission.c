#include "types.h"
#include "field/event_fest_mission.h"
#include "field/event_funfest_mission.h"
#include "field/event_mapchange.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/gimmick_funfest_levels.h"
#include "field/player_state.h"
#include "gfl/std.h"
#include "struct_decls.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

GameEvent *func_ov033_02176d88(GameSystem *gsys) {
    return GameEvent_Create(gsys, NULL, func_ov033_02176d9c, 4);
}

GameEventReturnCode func_ov033_02176d9c(GameEvent *event, u32 *state, void *data) {
    GameSystem *gsys;
    Field *field;
    GameCommSys *commSys;
    LinkFestival *festival;
    FestMissionEventArgs args;
    VecFx32 position;
    s32 *positionPtr;
    GameData *gameData;
    PlayerState *playerState;
    GameEvent *next;
    s32 i;
    s32 *counter;

    counter = data;
    gsys = GameEvent_GetGameSystem(event);
    field = GSYS_GetField(gsys);
    commSys = GSYS_GetGameCommSystem(gsys);
    festival = GSYS_GetLinkFestival(gsys);
    switch (*state) {
    case 0:
        DisableAllActorsMovement(Field_GetActorSystem(field));
        sys_memset(&args, 0, sizeof(args));
        args.config = *(FestMissionConfig *)GetFestMissionCfg(festival);
        args.joined = 0;
        next = GameEvent_CreateOverlayDelegate(gsys, OVERLAY_FUNFEST_MISSION, EventFestMissionStart_Create, &args);
        GameEvent_ChainNext(event, next);
        ++*state;
        break;
    case 1:
        GimmickFunfestLevels_PlayStartEffect(gsys);
        if (GameCommSys_BootCheck(commSys) != 2) {
            gameData = GSYS_GetGameData(gsys);
            positionPtr = (s32 *)&position;
            positionPtr[0] = 0;
            positionPtr[1] = 0;
            positionPtr[2] = 0;
            for (i = 0; i < 3; i++) {
                playerState = func_020171e8(gameData, i);
                PlayerState_SetWPos(playerState, (VecFx32 *)positionPtr);
            }
        }
        ++*state;
        break;
    case 2:
        if (Field_CheckMapLoadFinished(GSYS_GetField(gsys)) == 1) {
            *state = 3;
        }
        break;
    case 3:
        if ((*counter)++ >= 70) {
            GimmickFunfestLevels_PlayLeaveEffect(gsys);
            *counter = 0;
            ++*state;
        }
        break;
    case 4:
        if ((*counter)++ >= 30) {
            next = EventEntralinkWarp_CreateOut(gsys);
            GameEvent_ChainNext(event, next);
            ++*state;
        }
        break;
    case 5:
        func_02014774(festival, 0);
        EnableAllActorsMovement(Field_GetActorSystem(field));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
