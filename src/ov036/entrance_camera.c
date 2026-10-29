// The camera of the entrance events, and the events of a warp that play the entrance, such as a door: where the player
// goes in, walks and comes out
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional camera API, struct member, helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/arc.h"
#include "constants/directions.h"
#include "constants/sound.h"
#include "field/entrance_camera.h"
#include "field/event_action_call.h"
#include "field/event_entrance_effect.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_event.h"
#include "field/field_player.h"
#include "field/field_sound.h"
#include "field/pair_sys.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/iss_dungeon_sys.h"
#include "system/iss_sys.h"

struct EntranceCamera {
    Field *field;
    FieldCamera *camera;
    FieldCameraAnimation animation;
    FieldCameraAnimationController *controller;
    EntranceCameraParam param;
    BOOL configured;
    BOOL hasDepartureAnimation;
    BOOL hasArrivalAnimation;
};

// The events of a warp, which index the callbacks that EventWarpSequence_CreateOut and EventWarpSequence_CreateIn
// pick by its transition type
enum {
    WARP_EVENT_PLAIN,
    WARP_EVENT_GROUND_ANIME,
    WARP_EVENT_OBJECT_ANIME,
    WARP_EVENT_APPEAR,
    WARP_EVENT_CAMERA,
    WARP_EVENT_FADE_SE,
};

// The data of the events that play the entrance when the player goes out of a zone and into one
typedef struct {
    u32 unk0;
    u32 unk4;
    WarpSequence *warp;
} WarpOutData;

typedef struct {
    WarpSequence *warp;
} WarpInData;

static void EventWarpSequence_StartWalkTask(WarpSequence *warp);
static BOOL EventWarpSequence_UpdateTCB(WarpSequence *warp);
static GameEventReturnCode EventWarpSequence_WalkTaskCallback(GameEvent *event, u32 *state, void *data);
static void EventWarpSequenceOut_ChangeToZoneBGM(WarpOutData *out);
static BOOL EventWarpSequenceOut_ShouldKeepBGM(WarpOutData *out);
static BOOL EventWarpSequenceOut_CheckISSPersist(WarpOutData *out);
static u32 EventWarpSequenceOut_GetPlayerExState(WarpOutData *out);
static void EventWarpSequenceOut_FadeOutBGM(WarpOutData *out, GameEvent *event);
static void EventWarpSequenceOut_PlayTransition(WarpOutData *out, GameEvent *event);
static void EventWarpSequenceIn_FadeInBGM(WarpInData *in);
static void EventWarpSequenceIn_ChainBGMFadeIn(WarpInData *in, GameEvent *event);
static void EventWarpSequenceIn_PlayTransition(WarpInData *in, GameEvent *event);
static void EventWarpSequenceIn_ShowPlaceName(WarpInData *in);

GameEvent *EventWarpSequence_CreateOut(WarpSequence *warp) {
    const GameEventCallback callbacks[] = {
        EventWarpSequenceOut_RunPlain,  EventWarpSequenceOut_RunGroundAnime, EventWarpSequenceOut_RunObjectAnime,
        EventWarpSequenceOut_RunAppear, EventWarpSequenceOut_RunCamera,      EventWarpSequenceOut_RunFadeSe,
    };
    int index;
    GameEvent *event;
    WarpOutData *data;

    switch (warp->departureTransitionType) {
    case WARP_TRANSITION_NONE:
        index = WARP_EVENT_PLAIN;
        break;
    case WARP_TRANSITION_MAT:
        index = WARP_EVENT_PLAIN;
        break;
    case WARP_TRANSITION_WALK_IN_2:
        index = WARP_EVENT_OBJECT_ANIME;
        break;
    case WARP_TRANSITION_DOOR:
        index = WARP_EVENT_GROUND_ANIME;
        break;
    case WARP_TRANSITION_WALK_IN:
        index = WARP_EVENT_OBJECT_ANIME;
        break;
    case WARP_TRANSITION_WARP_PAD:
        index = WARP_EVENT_APPEAR;
        break;
    case WARP_TRANSITION_GATE:
        index = WARP_EVENT_FADE_SE;
        break;
    default:
        index = WARP_EVENT_CAMERA;
        break;
    }
    event = GameEvent_Create(warp->gsys, warp->parent, callbacks[index], sizeof(WarpOutData));
    data = GameEvent_GetData(event);
    data->unk4 = 0;
    data->warp = warp;
    return event;
}

// The steps of the warp events that start the transition or chain one event, and end once it is done
enum {
    WARP_STEP_START,
    WARP_STEP_END,
};

GameEventReturnCode EventWarpSequenceOut_RunPlain(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    WarpSequence *warp = out->warp;

    switch (*state) {
    case WARP_STEP_START:
        EventWarpSequenceOut_ChangeToZoneBGM(out);
        if (warp->outTransition != FIELD_TRANSITION_BUILDING) {
            GFL_SndSEPlay(SEQ_SE_KAIDAN);
        }
        EventWarpSequenceOut_PlayTransition(out, event);
        (*state)++;
        break;
    case WARP_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventWarpSequenceOut_RunGroundAnime(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    WarpSequence *warp = out->warp;

    switch (*state) {
    case WARP_STEP_START:
        GameEvent_ChainNext(event, EntranceEffect_CreateB(warp));
        (*state)++;
        break;
    case WARP_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void EventWarpSequence_StartWalkTask(WarpSequence *warp) {
    MMSys *mmSys = Field_GetActorSystem(warp->field);
    FieldActor *player = FieldPlayer_GetActor(Field_GetPlayer(warp->field));
    FieldActor *pair;

    warp->walkTask = FieldAcmdTCB_CreateWalkOneTile(player, GetActorFaceDir(player));
    if (GameData_CheckPairFlag(warp->gameData)) {
        pair = FindFieldActor(mmSys, 0xfe);
        if (pair != NULL) {
            EnableActorMovement(pair);
        }
    }
}

static BOOL EventWarpSequence_UpdateTCB(WarpSequence *warp) {
    if (!FieldAcmdTCB_CheckEnded(warp->walkTask)) {
        return FALSE;
    }
    FieldAcmdTCB_Remove(warp->walkTask);
    warp->walkTask = NULL;
    return TRUE;
}

static GameEventReturnCode EventWarpSequence_WalkTaskCallback(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;

    if (EventWarpSequence_UpdateTCB(in->warp)) {
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventWarpSequence_CreateWalkEvent(GameSystem *gsys, WarpSequence *warp) {
    GameEvent *event;
    WarpInData *data;

    EventWarpSequence_StartWalkTask(warp);
    event = GameEvent_Create(gsys, NULL, EventWarpSequence_WalkTaskCallback, sizeof(WarpInData));
    data = GameEvent_GetData(event);
    data->warp = warp;
    return event;
}

// The steps of the walk out: start the walk of one tile, play the transition once it is over, and end
enum {
    WARP_OUT_WALK_START,
    WARP_OUT_WALK_WAIT,
    WARP_OUT_WALK_END,
};

GameEventReturnCode EventWarpSequenceOut_RunObjectAnime(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    WarpSequence *warp = out->warp;

    switch (*state) {
    case WARP_OUT_WALK_START:
        if (EventWarpSequenceOut_ShouldKeepBGM(out)) {
            EventWarpSequenceOut_FadeOutBGM(out, event);
        } else {
            EventWarpSequenceOut_ChangeToZoneBGM(out);
        }
        EventWarpSequence_StartWalkTask(warp);
        (*state)++;
        break;
    case WARP_OUT_WALK_WAIT:
        if (EventWarpSequence_UpdateTCB(warp)) {
            if (warp->outTransition != FIELD_TRANSITION_BUILDING) {
                GFL_SndSEPlay(SEQ_SE_KAIDAN);
            }
            EventWarpSequenceOut_PlayTransition(out, event);
            (*state)++;
        }
        break;
    case WARP_OUT_WALK_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventWarpSequenceOut_RunAppear(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    GameSystem *gsys = out->warp->gsys;
    Field *field = out->warp->field;

    switch (*state) {
    case WARP_STEP_START:
        GameEvent_ChainNext(event, EventDig_CreateRisingSpin(NULL, gsys, field));
        (*state)++;
        break;
    case WARP_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// The steps of the camera's exit: finish the camera's delay if it has one, wait for it, play the entrance effect, and
// end
enum {
    WARP_OUT_CAMERA_CHECK_DELAY,
    WARP_OUT_CAMERA_WAIT_DELAY,
    WARP_OUT_CAMERA_EFFECT,
    WARP_OUT_CAMERA_END,
};

GameEventReturnCode EventWarpSequenceOut_RunCamera(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    WarpSequence *warp = out->warp;
    FieldCamera *camera = Field_GetCameraSystem(warp->field);

    switch (*state) {
    case WARP_OUT_CAMERA_CHECK_DELAY:
        if (FieldCamera_SupportsDelay(camera) && FieldCamera_IsDelayActive(camera)) {
            FieldCamera_FinishDelay(camera);
            *state = WARP_OUT_CAMERA_WAIT_DELAY;
        } else {
            *state = WARP_OUT_CAMERA_EFFECT;
        }
        return GAMEEVENT_CONTINUE_DIRECT;
    case WARP_OUT_CAMERA_WAIT_DELAY:
        if (!FieldCamera_SupportsDelay(camera)) {
            *state = WARP_OUT_CAMERA_EFFECT;
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        if (!FieldCamera_IsDelayActive(camera)) {
            FieldCamera_ClearBind(camera);
            *state = WARP_OUT_CAMERA_EFFECT;
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case WARP_OUT_CAMERA_EFFECT:
        GameEvent_ChainNext(event, EntranceEffect_CreateB(warp));
        *state = WARP_OUT_CAMERA_END;
        return GAMEEVENT_CONTINUE_DIRECT;
    case WARP_OUT_CAMERA_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventWarpSequenceOut_RunFadeSe(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    GameSystem *gsys = out->warp->gsys;
    Field *field = out->warp->field;

    switch (*state) {
    case WARP_STEP_START:
        EventWarpSequenceOut_ChangeToZoneBGM(out);
        GFL_SndSEPlay(SEQ_SE_FLD_102);
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_BUILDING, 0));
        (*state)++;
        break;
    case WARP_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static BOOL EventWarpSequenceOut_ShouldKeepBGM(WarpOutData *out) {
    WarpSequence *warp = out->warp;
    GameData *gameData = warp->gameData;

    if (!EventWarpSequenceOut_CheckISSPersist(out)) {
        return FALSE;
    }
    if (GameData_IsZoneBGMOverriden(gameData, (u16)warp->spawn.zoneId)) {
        return FALSE;
    }
    if (EventWarpSequenceOut_GetPlayerExState(out) == FLD_PLAYER_EXSTATE_CYCLING &&
        GameData_GetNowBGM(gameData) == SEQ_BGM_BICYCLE) {
        return FALSE;
    }
    return TRUE;
}

static BOOL EventWarpSequenceOut_CheckISSPersist(WarpOutData *out) {
    WarpSequence *warp = out->warp;
    ISSDungeonSys *dungeonSys = ISS_GetDungeonSys(GameSystem_GetISS(warp->gsys));

    if (ISSDungeonSys_IsEnabled(dungeonSys) && ISSDungeonSys_IsZoneRegist(dungeonSys, warp->zoneId) &&
        ISSDungeonSys_IsZoneRegist(dungeonSys, warp->spawn.zoneId)) {
        return TRUE;
    }
    return FALSE;
}

static u32 EventWarpSequenceOut_GetPlayerExState(WarpOutData *out) {
    return FieldPlayer_GetExState(Field_GetPlayer(out->warp->field));
}

static void EventWarpSequenceOut_ChangeToZoneBGM(WarpOutData *out) {
    WarpSequence *warp = out->warp;
    GameData *gameData = warp->gameData;

    FieldSnd_SetZoneBGM(GameData_GetFieldSoundSystem(gameData), gameData, warp->spawn.zoneId, warp->endSeason);
    warp->zoneBGMChanged = 1;
}

static void EventWarpSequenceOut_FadeOutBGM(WarpOutData *out, GameEvent *event) {
    WarpSequence *warp = out->warp;

    GameEvent_ChainNext(event, EventBGMFadeOut_Create(warp->gsys, 30));
    warp->bgmFadedOut = TRUE;
}

static void EventWarpSequenceOut_PlayTransition(WarpOutData *out, GameEvent *event) {
    WarpSequence *warp = out->warp;
    GameSystem *gsys = warp->gsys;
    Field *field = warp->field;

    if (warp->seasonChanged) {
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
    } else {
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, field, warp->outTransition, 0));
    }
}

GameEvent *EventWarpSequence_CreateIn(WarpSequence *warp) {
    const GameEventCallback callbacks[] = {
        EventWarpSequenceIn_RunPlain,       EventWarpSequenceIn_RunGroundAnime, EventWarpSequenceIn_RunObjectAnime,
        EventWarpSequence_CallbackSpinDown, EventWarpSequenceIn_RunCamera,      EventWarpSequenceIn_RunFadeSe,
    };
    int index;
    GameEvent *event;
    WarpInData *data;

    switch (warp->transitionType) {
    case WARP_TRANSITION_NONE:
        index = WARP_EVENT_PLAIN;
        break;
    case WARP_TRANSITION_MAT:
        index = WARP_EVENT_PLAIN;
        break;
    case WARP_TRANSITION_WALK_IN_2:
        index = WARP_EVENT_OBJECT_ANIME;
        break;
    case WARP_TRANSITION_DOOR:
        index = WARP_EVENT_GROUND_ANIME;
        break;
    case WARP_TRANSITION_WALK_IN:
        index = WARP_EVENT_OBJECT_ANIME;
        break;
    case WARP_TRANSITION_WARP_PAD:
        index = WARP_EVENT_APPEAR;
        break;
    case WARP_TRANSITION_GATE:
        index = WARP_EVENT_FADE_SE;
        break;
    default:
        index = WARP_EVENT_CAMERA;
        break;
    }
    event = GameEvent_Create(warp->gsys, warp->parent, callbacks[index], sizeof(WarpInData));
    data = GameEvent_GetData(event);
    data->warp = warp;
    if (index == WARP_EVENT_OBJECT_ANIME) {
        TryRespawnFollowActor(warp->gameData, TRUE);
    } else {
        TryRespawnFollowActor(warp->gameData, FALSE);
    }
    return event;
}

GameEventReturnCode EventWarpSequenceIn_RunPlain(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    WarpSequence *warp = in->warp;

    switch (*state) {
    case WARP_STEP_START:
        if (warp->zoneBGMChanged != 0) {
            EventWarpSequenceIn_FadeInBGM(in);
        }
        EventWarpSequenceIn_PlayTransition(in, event);
        (*state)++;
        break;
    case WARP_STEP_END:
        EventWarpSequenceIn_ShowPlaceName(in);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventWarpSequenceIn_RunGroundAnime(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    WarpSequence *warp = in->warp;

    switch (*state) {
    case WARP_STEP_START:
        GameEvent_ChainNext(event, EntranceEffect_CreateA(warp));
        (*state)++;
        break;
    case WARP_STEP_END:
        EventWarpSequenceIn_ShowPlaceName(in);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// The steps of the walk in: play the transition, fade the BGM in, walk one tile and end
enum {
    WARP_IN_WALK_TRANSITION,
    WARP_IN_WALK_BGM,
    WARP_IN_WALK_STEP,
    WARP_IN_WALK_END,
};

GameEventReturnCode EventWarpSequenceIn_RunObjectAnime(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    WarpSequence *warp = in->warp;
    GameSystem *gsys = warp->gsys;
    Field *field = warp->field;

    switch (*state) {
    case WARP_IN_WALK_TRANSITION:
        EventWarpSequenceIn_PlayTransition(in, event);
        (*state)++;
        break;
    case WARP_IN_WALK_BGM:
        if (warp->zoneBGMChanged != 0) {
            EventWarpSequenceIn_FadeInBGM(in);
        } else if (warp->bgmFadedOut != 0) {
            EventWarpSequenceIn_ChainBGMFadeIn(in, event);
        }
        (*state)++;
        break;
    case WARP_IN_WALK_STEP:
        GameEvent_ChainNext(event, CallMoveOneTileFrontEvent(gsys, field));
        (*state)++;
        break;
    case WARP_IN_WALK_END:
        EventWarpSequenceIn_ShowPlaceName(in);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventWarpSequence_CallbackSpinDown(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    GameSystem *gsys = in->warp->gsys;
    Field *field = in->warp->field;

    switch (*state) {
    case WARP_STEP_START:
        GameEvent_ChainNext(event, EventPlayerSpinDown_Create(NULL, gsys, field));
        (*state)++;
        break;
    case WARP_STEP_END:
        EventWarpSequenceIn_ShowPlaceName(in);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventWarpSequenceIn_RunCamera(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    WarpSequence *warp = in->warp;

    Field_GetCameraSystem(warp->field);
    switch (*state) {
    case WARP_STEP_START:
        GameEvent_ChainNext(event, EntranceEffect_CreateA(warp));
        (*state)++;
        break;
    case WARP_STEP_END:
        EventWarpSequenceIn_ShowPlaceName(in);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// The steps of the gate's entrance: fade the BGM in, play the transition and end
enum {
    WARP_IN_FADE_SE_BGM,
    WARP_IN_FADE_SE_TRANSITION,
    WARP_IN_FADE_SE_END,
};

GameEventReturnCode EventWarpSequenceIn_RunFadeSe(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    WarpSequence *warp = in->warp;
    GameSystem *gsys = warp->gsys;
    Field *field = warp->field;

    switch (*state) {
    case WARP_IN_FADE_SE_BGM:
        if (warp->zoneBGMChanged != 0) {
            EventWarpSequenceIn_FadeInBGM(in);
        }
        (*state)++;
        break;
    case WARP_IN_FADE_SE_TRANSITION:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_BUILDING, 0, 0, 0, 0));
        (*state)++;
        break;
    case WARP_IN_FADE_SE_END:
        EventWarpSequenceIn_ShowPlaceName(in);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void EventWarpSequenceIn_FadeInBGM(WarpInData *in) {
    GameData *gameData = in->warp->gameData;

    FieldSnd_FadeInImmediate(GameData_GetFieldSoundSystem(gameData), gameData);
}

static void EventWarpSequenceIn_ChainBGMFadeIn(WarpInData *in, GameEvent *event) {
    WarpSequence *warp = in->warp;
    GameSystem *gsys = warp->gsys;

    GameData_GetFieldSoundSystem(warp->gameData);
    GameEvent_ChainNext(event, EventBGMFadeIn_Create(gsys, 6));
}

static void EventWarpSequenceIn_PlayTransition(WarpInData *in, GameEvent *event) {
    WarpSequence *warp = in->warp;
    GameSystem *gsys = warp->gsys;
    Field *field = warp->field;

    if (warp->seasonChanged) {
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_SEASON, 0, 0,
                                                                    warp->startSeason, warp->endSeason));
    } else {
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, field, warp->inTransition, 0, 1, 0, 0));
    }
}

static void EventWarpSequenceIn_ShowPlaceName(WarpInData *in) {
    WarpSequence *warp = in->warp;

    PlaceName_OnZoneChange(Field_GetPlaceName(warp->field), (s16)warp->spawn.zoneId);
}

EntranceCamera *EntranceCamera_Create(Field *field) {
    HeapID heapId = Field_GetHeapID(field);
    EntranceCamera *camera = EntranceCamera_Allocate(heapId);

    EntranceCamera_Init(camera, field);
    EntranceCamera_CreateController(camera, heapId);
    return camera;
}

void EntranceCamera_Destroy(EntranceCamera *camera) {
    EntranceCamera_FreeController(camera);
    EntranceCamera_Free(camera);
}

void EntranceCamera_Configure(EntranceCamera *camera, const EntranceCameraParam *param) {
    EntranceCamera_SetParam(camera, param);
    FieldCameraAnimationController_SetAnimation(camera->controller, &camera->animation);
    if (EntranceCamera_HasRecord(camera)) {
        switch (param->isArrival) {
        case FALSE:
            EntranceCamera_ConfigureFileDeparture(camera);
            break;
        case TRUE:
            EntranceCamera_ConfigureFileArrival(camera);
            break;
        }
    } else {
        switch (param->isArrival) {
        case FALSE:
            EntranceCamera_ConfigureDefaultDeparture(camera);
            break;
        case TRUE:
            EntranceCamera_ConfigureDefaultArrival(camera);
            break;
        }
    }
    camera->configured = TRUE;
}

void EntranceCamera_StartAnimation(EntranceCamera *camera) {
    if (EntranceCamera_IsPhaseActive(camera)) {
        FieldCameraAnimationController_StartAnimation(camera->controller);
    }
}

void EntranceCamera_FinishCamera(EntranceCamera *camera) {
    FieldCameraAnimationController_FinishCamera(camera->controller);
}

BOOL EntranceCamera_IsActive(EntranceCamera *camera) {
    return EntranceCamera_IsPhaseActive(camera);
}

BOOL EntranceCamera_HasZeroFrames(EntranceCamera *camera) {
    if (camera->animation.frames == 0) {
        return TRUE;
    }
    return FALSE;
}

EntranceCamera *EntranceCamera_Allocate(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(EntranceCamera), FALSE, "entrance_camera.c", 298);
}

void EntranceCamera_Free(EntranceCamera *camera) {
    GFL_HeapFree(camera);
}

void EntranceCamera_Init(EntranceCamera *camera, Field *field) {
    MI_CpuFill8(camera, 0, sizeof(EntranceCamera));
    camera->field = field;
    camera->camera = Field_GetCameraSystem(field);
    camera->configured = FALSE;
}

void EntranceCamera_CreateController(EntranceCamera *camera, HeapID heapId) {
    camera->controller = Field_CreateCameraAnimationController(camera->field);
}

void EntranceCamera_FreeController(EntranceCamera *camera) {
    if (camera->controller != NULL) {
        FieldCameraAnimationController_FreeAnimation(camera->controller);
        camera->controller = NULL;
    }
}

void EntranceCamera_ConfigureDefaultDeparture(EntranceCamera *camera) {
    FieldCameraAnimation *animation = &camera->animation;
    FieldCameraCoords *dst = &animation->dst;
    FieldPlayer *player;
    u32 dir;
    VecFx32 pos;

    EntranceCamera_GetFieldCamera(camera);
    player = Field_GetPlayer(camera->field);
    dir = FieldPlayer_GetFaceDir(player);
    switch (dir) {
    case DIR_LEFT:
    case DIR_RIGHT:
        camera->hasDepartureAnimation = TRUE;
        camera->hasArrivalAnimation = FALSE;
        break;
    case DIR_UP:
    case DIR_DOWN:
        camera->hasDepartureAnimation = FALSE;
        camera->hasArrivalAnimation = FALSE;
        break;
    }
    if (camera->hasDepartureAnimation) {
        animation->disableBoundary = FALSE;
        animation->releaseBind = FALSE;
        if (EntranceCamera_AimsAtEntrance(camera)) {
            EntranceEffect_GetEntryPos(camera->field, &pos);
        } else {
            FieldPlayer_GetWPos(player, &pos);
        }
        FieldCameraAnimationController_PrepareCamera(camera->controller);
        FieldCamera_GetAnimationCoords(camera->camera, &animation->src);
        {
            // Leftovers: this vector is zeroed and never read, as pos is fetched and never read and the field camera
            // fetched at the top is discarded
            VecFx32 unused = { 0, 0, 0 };

            FieldCamera_GetAnimationCoords(camera->camera, dst);
            switch (dir) {
            case DIR_UP:
                dst->distance = FX32_CONST(180);
                break;
            case DIR_DOWN:
                break;
            case DIR_LEFT:
                dst->yaw = 0xa09;
                break;
            case DIR_RIGHT:
                dst->yaw = 0xf5f7;
                break;
            }
        }
        switch (dir) {
        case DIR_UP:
            animation->frames = 0;
            break;
        case DIR_DOWN:
            animation->frames = 0;
            break;
        case DIR_LEFT:
            animation->frames = 10;
            break;
        case DIR_RIGHT:
            animation->frames = 10;
            break;
        }
    }
}

void EntranceCamera_ConfigureDefaultArrival(EntranceCamera *camera) {
    FieldCameraAnimation *animation = &camera->animation;
    FieldCameraCoords *dst = &animation->dst;
    FieldCamera *fieldCamera = EntranceCamera_GetFieldCamera(camera);
    FieldPlayer *player;
    u32 dir;
    VecFx32 pos;

    player = Field_GetPlayer(camera->field);
    dir = FieldPlayer_GetFaceDir(player);
    switch (dir) {
    case DIR_DOWN:
    case DIR_LEFT:
    case DIR_RIGHT:
        camera->hasDepartureAnimation = FALSE;
        camera->hasArrivalAnimation = TRUE;
        break;
    case DIR_UP:
        camera->hasDepartureAnimation = FALSE;
        camera->hasArrivalAnimation = FALSE;
        break;
    }
    if (camera->hasArrivalAnimation) {
        animation->disableBoundary = FALSE;
        animation->releaseBind = FALSE;
        if (EntranceCamera_AimsAtEntrance(camera)) {
            EntranceEffect_GetEntryPos(camera->field, &pos);
        } else {
            FieldPlayer_GetWPos(player, &pos);
        }
        switch (dir) {
        case DIR_UP:
            animation->frames = 0;
            break;
        case DIR_DOWN:
            animation->frames = 10;
            break;
        case DIR_LEFT:
            animation->frames = 10;
            break;
        case DIR_RIGHT:
            animation->frames = 10;
            break;
        }
        if (EntranceCamera_AimsAtEntrance(camera)) {
            FieldCamera_ChangeRefPos(fieldCamera, &pos);
        }
        FieldCameraAnimationController_PrepareCamera(camera->controller);
        FieldCamera_GetAnimationCoords(camera->camera, &animation->src);
        switch (dir) {
        case DIR_UP:
            break;
        case DIR_DOWN:
            animation->src.distance = FX32_CONST(180);
            break;
        case DIR_LEFT:
            animation->src.yaw = 0xf5f7;
            break;
        case DIR_RIGHT:
            animation->src.yaw = 0xa09;
            break;
        }
        FieldCamera_GetAnimationCoords(camera->camera, dst);
        if (camera->hasArrivalAnimation) {
            FieldCameraAnimationController_CommitSrcCoords(camera->controller);
        }
    }
}

void EntranceCamera_ConfigureFileDeparture(EntranceCamera *camera) {
    FieldCameraAnimation *animation = &camera->animation;
    FieldCameraCoords *dst = &animation->dst;
    FieldPlayer *player;
    VecFx32 pos;
    EntranceCameraFile file;

    EntranceCamera_GetFieldCamera(camera);
    player = Field_GetPlayer(camera->field);
    FieldPlayer_GetFaceDir(player);
    EntranceCamera_ReadFile(&file, EntranceCamera_GetTransitionType(camera));
    animation->disableBoundary = file.disableBoundary;
    animation->releaseBind = file.releaseBind;
    camera->hasDepartureAnimation = file.hasDepartureAnimation;
    camera->hasArrivalAnimation = file.hasArrivalAnimation;
    if (camera->hasDepartureAnimation) {
        if (EntranceCamera_AimsAtEntrance(camera)) {
            EntranceEffect_GetEntryPos(camera->field, &pos);
        } else {
            FieldPlayer_GetWPos(player, &pos);
        }
        FieldCameraAnimationController_PrepareCamera(camera->controller);
        FieldCamera_GetAnimationCoords(camera->camera, &animation->src);
        animation->frames = file.frames;
        dst->pitch = file.pitch;
        dst->yaw = file.yaw;
        dst->distance = file.distance << FX32_SHIFT;
        VEC_Set(&dst->offset, file.offset[0] << FX32_SHIFT, file.offset[1] << FX32_SHIFT, file.offset[2] << FX32_SHIFT);
        if (file.useTarget) {
            VEC_Set(&dst->target, file.target[0] << FX32_SHIFT, file.target[1] << FX32_SHIFT,
                    file.target[2] << FX32_SHIFT);
        } else {
            dst->target = pos;
        }
    }
}

void EntranceCamera_ConfigureFileArrival(EntranceCamera *camera) {
    FieldCameraAnimation *animation = &camera->animation;
    FieldCameraCoords *dst = &animation->dst;
    FieldCameraCoords *src = &animation->src;
    FieldCamera *fieldCamera = EntranceCamera_GetFieldCamera(camera);
    FieldPlayer *player;
    VecFx32 pos;
    EntranceCameraFile file;

    player = Field_GetPlayer(camera->field);
    FieldPlayer_GetFaceDir(player);
    EntranceCamera_ReadFile(&file, EntranceCamera_GetTransitionType(camera));
    animation->disableBoundary = file.disableBoundary;
    animation->releaseBind = file.releaseBind;
    camera->hasDepartureAnimation = file.hasDepartureAnimation;
    camera->hasArrivalAnimation = file.hasArrivalAnimation;
    if (camera->hasArrivalAnimation) {
        if (EntranceCamera_AimsAtEntrance(camera)) {
            EntranceEffect_GetEntryPos(camera->field, &pos);
        } else {
            FieldPlayer_GetWPos(player, &pos);
        }
        if (animation->releaseBind && EntranceCamera_AimsAtEntrance(camera)) {
            FieldCamera_ChangeRefPos(fieldCamera, &pos);
        }
        FieldCameraAnimationController_PrepareCamera(camera->controller);
        FieldCamera_GetAnimationCoords(camera->camera, dst);
        animation->frames = file.frames;
        src->pitch = file.pitch;
        src->yaw = file.yaw;
        src->distance = file.distance << FX32_SHIFT;
        VEC_Set(&src->offset, file.offset[0] << FX32_SHIFT, file.offset[1] << FX32_SHIFT, file.offset[2] << FX32_SHIFT);
        if (file.useTarget) {
            VEC_Set(&src->target, file.target[0] << FX32_SHIFT, file.target[1] << FX32_SHIFT,
                    file.target[2] << FX32_SHIFT);
        } else {
            src->target = pos;
        }
        if (camera->hasArrivalAnimation) {
            FieldCameraAnimationController_CommitSrcCoords(camera->controller);
        }
    }
}

void EntranceCamera_ReadFile(EntranceCameraFile *dest, u32 index) {
    GFL_ArcSysRead(dest, ARCID_ENTRANCE_CAMERA, index - WARP_TRANSITION_CAMERA);
}

void FieldCamera_ChangeRefPos(FieldCamera *camera, const VecFx32 *target) {
    FieldCamera_ClearBind(camera);
    FieldCamera_CoordsSetTarget(camera, target);
    FieldCamera_CalcTransform(camera, 0);
    FieldCamera_ResetBind(camera);
}

FieldCamera *EntranceCamera_GetFieldCamera(EntranceCamera *camera) {
    return camera->camera;
}

EntranceCameraParam *EntranceCamera_GetParam(EntranceCamera *camera) {
    return &camera->param;
}

void EntranceCamera_SetParam(EntranceCamera *camera, const EntranceCameraParam *param) {
    camera->param = *param;
}

u32 EntranceCamera_GetTransitionType(EntranceCamera *camera) {
    return EntranceCamera_GetParam(camera)->transitionType;
}

BOOL EntranceCamera_HasRecord(EntranceCamera *camera) {
    return IsEntranceCameraTransition(EntranceCamera_GetParam(camera)->transitionType);
}

BOOL EntranceCamera_AimsAtEntrance(EntranceCamera *camera) {
    if (EntranceCamera_GetParam(camera)->isDeparture == 0) {
        return TRUE;
    }
    return FALSE;
}

BOOL EntranceCamera_IsPhaseActive(EntranceCamera *camera) {
    if (EntranceCamera_GetParam(camera)->isArrival == 0) {
        return camera->hasDepartureAnimation;
    }
    return camera->hasArrivalAnimation;
}
