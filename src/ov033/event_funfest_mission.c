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

GameEvent *EventFunfestMission_Create(GameSystem *gsys) {
    return GameEvent_Create(gsys, NULL, EventFunfestMission_Callback, 4);
}

// The steps of a mission's start: run the mission's start event, play the start effect and move the players, wait for
// the map, play the leave effect after 70 frames, warp out of the Entralink after 30 more, and start the timer
enum {
    FUNFEST_MISSION_STEP_START,
    FUNFEST_MISSION_STEP_START_EFFECT,
    FUNFEST_MISSION_STEP_WAIT_MAP,
    FUNFEST_MISSION_STEP_WAIT_LEAVE,
    FUNFEST_MISSION_STEP_WAIT_WARP,
    FUNFEST_MISSION_STEP_END,
};

GameEventReturnCode EventFunfestMission_Callback(GameEvent *event, u32 *state, void *data) {
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
    case FUNFEST_MISSION_STEP_START:
        DisableAllActorsMovement(Field_GetActorSystem(field));
        MI_CpuFill8(&args, 0, sizeof(args));
        args.config = *GetFestMissionCfg(festival);
        args.joined = 0;
        next = GameEvent_CreateOverlayDelegate(gsys, OVERLAY_FUNFEST_MISSION, EventFestMissionStart_Create, &args);
        GameEvent_ChainNext(event, next);
        ++*state;
        break;
    case FUNFEST_MISSION_STEP_START_EFFECT:
        GimmickFunfestLevels_PlayStartEffect(gsys);
        if (GameCommSys_BootCheck(commSys) != GAME_COMM_NO_INTRUDE) {
            gameData = GSYS_GetGameData(gsys);
            positionPtr = (s32 *)&position;
            positionPtr[0] = 0;
            positionPtr[1] = 0;
            positionPtr[2] = 0;
            for (i = 0; i < 3; i++) {
                playerState = GameData_GetPlayerStateByNetID(gameData, i);
                PlayerState_SetWPos(playerState, (VecFx32 *)positionPtr);
            }
        }
        ++*state;
        break;
    case FUNFEST_MISSION_STEP_WAIT_MAP:
        if (Field_CheckMapLoadFinished(GSYS_GetField(gsys)) == TRUE) {
            *state = FUNFEST_MISSION_STEP_WAIT_LEAVE;
        }
        break;
    case FUNFEST_MISSION_STEP_WAIT_LEAVE:
        if ((*counter)++ >= 70) {
            GimmickFunfestLevels_PlayLeaveEffect(gsys);
            *counter = 0;
            ++*state;
        }
        break;
    case FUNFEST_MISSION_STEP_WAIT_WARP:
        if ((*counter)++ >= 30) {
            next = EventEntralinkWarp_CreateOut(gsys);
            GameEvent_ChainNext(event, next);
            ++*state;
        }
        break;
    case FUNFEST_MISSION_STEP_END:
        LinkFestival_StartTimer(festival, 0);
        EnableAllActorsMovement(Field_GetActorSystem(field));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
