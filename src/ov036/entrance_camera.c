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
    BOOL unk68;
    BOOL unk6c;
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
static void func_ov036_021b7a20(WarpInData *in);

GameEvent *EventWarpSequence_CreateOut(WarpSequence *warp) {
    const GameEventCallback callbacks[] = {
        func_ov036_021b73b4, func_ov036_021b73f8, func_ov036_021b74c8,
        func_ov036_021b7538, func_ov036_021b7570, func_ov036_021b7604,
    };
    int index;
    GameEvent *event;
    WarpOutData *data;

    switch (warp->unk10) {
    case 0:
        index = 0;
        break;
    case 1:
        index = 0;
        break;
    case 2:
        index = 2;
        break;
    case 3:
        index = 1;
        break;
    case 4:
        index = 2;
        break;
    case 5:
        index = 3;
        break;
    case 6:
        index = 5;
        break;
    default:
        index = 4;
        break;
    }
    event = GameEvent_Create(warp->gsys, warp->parent, callbacks[index], sizeof(WarpOutData));
    data = GameEvent_GetData(event);
    data->unk4 = 0;
    data->warp = warp;
    return event;
}

GameEventReturnCode func_ov036_021b73b4(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    WarpSequence *warp = out->warp;

    switch (*state) {
    case 0:
        EventWarpSequenceOut_ChangeToZoneBGM(out);
        if (warp->outTransition != 2) {
            GFL_SndSEPlay(SEQ_SE_KAIDAN);
        }
        EventWarpSequenceOut_PlayTransition(out, event);
        (*state)++;
        break;
    case 1:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode func_ov036_021b73f8(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    WarpSequence *warp = out->warp;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, func_ov036_0219f4e0(warp));
        (*state)++;
        break;
    case 1:
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

GameEvent *func_ov036_021b74a0(GameSystem *gsys, WarpSequence *warp) {
    GameEvent *event;
    WarpInData *data;

    EventWarpSequence_StartWalkTask(warp);
    event = GameEvent_Create(gsys, NULL, EventWarpSequence_WalkTaskCallback, sizeof(WarpInData));
    data = GameEvent_GetData(event);
    data->warp = warp;
    return event;
}

GameEventReturnCode func_ov036_021b74c8(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    WarpSequence *warp = out->warp;

    switch (*state) {
    case 0:
        if (EventWarpSequenceOut_ShouldKeepBGM(out)) {
            EventWarpSequenceOut_FadeOutBGM(out, event);
        } else {
            EventWarpSequenceOut_ChangeToZoneBGM(out);
        }
        EventWarpSequence_StartWalkTask(warp);
        (*state)++;
        break;
    case 1:
        if (EventWarpSequence_UpdateTCB(warp)) {
            if (warp->outTransition != 2) {
                GFL_SndSEPlay(SEQ_SE_KAIDAN);
            }
            EventWarpSequenceOut_PlayTransition(out, event);
            (*state)++;
        }
        break;
    case 2:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode func_ov036_021b7538(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    GameSystem *gsys = out->warp->gsys;
    Field *field = out->warp->field;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, func_ov036_021b9df8(NULL, gsys, field));
        (*state)++;
        break;
    case 1:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode func_ov036_021b7570(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    WarpSequence *warp = out->warp;
    FieldCamera *camera = Field_GetCameraSystem(warp->field);

    switch (*state) {
    case 0:
        if (FieldCamera_SupportsDelay(camera) && FieldCamera_IsDelayActive(camera)) {
            FieldCamera_FinishDelay(camera);
            *state = 1;
        } else {
            *state = 2;
        }
        return GAMEEVENT_CONTINUE_DIRECT;
    case 1:
        if (!FieldCamera_SupportsDelay(camera)) {
            *state = 2;
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        if (!FieldCamera_IsDelayActive(camera)) {
            FieldCamera_ClearBind(camera);
            *state = 2;
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case 2:
        GameEvent_ChainNext(event, func_ov036_0219f4e0(warp));
        *state = 3;
        return GAMEEVENT_CONTINUE_DIRECT;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode func_ov036_021b7604(GameEvent *event, u32 *state, void *work) {
    WarpOutData *out = work;
    GameSystem *gsys = out->warp->gsys;
    Field *field = out->warp->field;

    switch (*state) {
    case 0:
        EventWarpSequenceOut_ChangeToZoneBGM(out);
        GFL_SndSEPlay(SEQ_SE_FLD_102);
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, field, 2, 0));
        (*state)++;
        break;
    case 1:
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
    if (EventWarpSequenceOut_GetPlayerExState(out) == 1 && GameData_GetNowBGM(gameData) == SEQ_BGM_BICYCLE) {
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
    warp->unk48 = 1;
}

static void EventWarpSequenceOut_FadeOutBGM(WarpOutData *out, GameEvent *event) {
    WarpSequence *warp = out->warp;

    GameEvent_ChainNext(event, EventBGMFadeOut_Create(warp->gsys, 30));
    warp->unk4C = 1;
}

static void EventWarpSequenceOut_PlayTransition(WarpOutData *out, GameEvent *event) {
    WarpSequence *warp = out->warp;
    GameSystem *gsys = warp->gsys;
    Field *field = warp->field;

    if (warp->seasonChanged) {
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, field, 0, 0));
    } else {
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, field, warp->outTransition, 0));
    }
}

GameEvent *EventWarpSequence_CreateIn(WarpSequence *warp) {
    const GameEventCallback callbacks[] = {
        func_ov036_021b77d0, func_ov036_021b7810, func_ov036_021b7848, EventWarpSequence_CallbackSpinDown,
        func_ov036_021b78fc, func_ov036_021b793c,
    };
    int index;
    GameEvent *event;
    WarpInData *data;

    switch (warp->transitionType) {
    case 0:
        index = 0;
        break;
    case 1:
        index = 0;
        break;
    case 2:
        index = 2;
        break;
    case 3:
        index = 1;
        break;
    case 4:
        index = 2;
        break;
    case 5:
        index = 3;
        break;
    case 6:
        index = 5;
        break;
    default:
        index = 4;
        break;
    }
    event = GameEvent_Create(warp->gsys, warp->parent, callbacks[index], sizeof(WarpInData));
    data = GameEvent_GetData(event);
    data->warp = warp;
    if (index == 2) {
        TryRespawnFollowActor(warp->gameData, TRUE);
    } else {
        TryRespawnFollowActor(warp->gameData, FALSE);
    }
    return event;
}

GameEventReturnCode func_ov036_021b77d0(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    WarpSequence *warp = in->warp;

    switch (*state) {
    case 0:
        if (warp->unk48 != 0) {
            EventWarpSequenceIn_FadeInBGM(in);
        }
        EventWarpSequenceIn_PlayTransition(in, event);
        (*state)++;
        break;
    case 1:
        func_ov036_021b7a20(in);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode func_ov036_021b7810(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    WarpSequence *warp = in->warp;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, func_ov036_0219f190(warp));
        (*state)++;
        break;
    case 1:
        func_ov036_021b7a20(in);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode func_ov036_021b7848(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    WarpSequence *warp = in->warp;
    GameSystem *gsys = warp->gsys;
    Field *field = warp->field;

    switch (*state) {
    case 0:
        EventWarpSequenceIn_PlayTransition(in, event);
        (*state)++;
        break;
    case 1:
        if (warp->unk48 != 0) {
            EventWarpSequenceIn_FadeInBGM(in);
        } else if (warp->unk4C != 0) {
            EventWarpSequenceIn_ChainBGMFadeIn(in, event);
        }
        (*state)++;
        break;
    case 2:
        GameEvent_ChainNext(event, CallMoveOneTileFrontEvent(gsys, field));
        (*state)++;
        break;
    case 3:
        func_ov036_021b7a20(in);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventWarpSequence_CallbackSpinDown(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    GameSystem *gsys = in->warp->gsys;
    Field *field = in->warp->field;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, EventPlayerSpinDown_Create(NULL, gsys, field));
        (*state)++;
        break;
    case 1:
        func_ov036_021b7a20(in);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode func_ov036_021b78fc(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    WarpSequence *warp = in->warp;

    Field_GetCameraSystem(warp->field);
    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, func_ov036_0219f190(warp));
        (*state)++;
        break;
    case 1:
        func_ov036_021b7a20(in);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode func_ov036_021b793c(GameEvent *event, u32 *state, void *work) {
    WarpInData *in = work;
    WarpSequence *warp = in->warp;
    GameSystem *gsys = warp->gsys;
    Field *field = warp->field;

    switch (*state) {
    case 0:
        if (warp->unk48 != 0) {
            EventWarpSequenceIn_FadeInBGM(in);
        }
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, field, 2, 0, 0, 0, 0));
        (*state)++;
        break;
    case 2:
        func_ov036_021b7a20(in);
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
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, 3, 0, 0, warp->startSeason, warp->endSeason));
    } else {
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, field, warp->inTransition, 0, 1, 0, 0));
    }
}

static void func_ov036_021b7a20(WarpInData *in) {
    WarpSequence *warp = in->warp;

    func_ov036_021b50c8(Field_GetPlaceName(warp->field), (s16)warp->spawn.zoneId);
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
    if (func_ov036_021b7f40(camera)) {
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
    if (func_ov036_021b7f64(camera)) {
        FieldCameraAnimationController_StartAnimation(camera->controller);
    }
}

void EntranceCamera_FinishCamera(EntranceCamera *camera) {
    FieldCameraAnimationController_FinishCamera(camera->controller);
}

BOOL func_ov036_021b7ae0(EntranceCamera *camera) {
    return func_ov036_021b7f64(camera);
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
        camera->unk68 = 1;
        camera->unk6c = 0;
        break;
    case DIR_UP:
    case DIR_DOWN:
        camera->unk68 = 0;
        camera->unk6c = 0;
        break;
    }
    if (camera->unk68 != 0) {
        animation->unk4C = 0;
        animation->unk48 = 0;
        if (func_ov036_021b7f50(camera)) {
            func_ov036_0219f178(camera->field, &pos);
        } else {
            FieldPlayer_GetWPos(player, &pos);
        }
        FieldCameraAnimationController_PrepareCamera(camera->controller);
        FieldCamera_GetAnimationCoords(camera->camera, &animation->src);
        FieldCamera_GetAnimationCoords(camera->camera, dst);
        switch (dir) {
        case DIR_UP:
            dst->distance = 0x2d << 14;
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
        camera->unk68 = 0;
        camera->unk6c = 1;
        break;
    case DIR_UP:
        camera->unk68 = 0;
        camera->unk6c = 0;
        break;
    }
    if (camera->unk6c != 0) {
        animation->unk4C = 0;
        animation->unk48 = 0;
        if (func_ov036_021b7f50(camera)) {
            func_ov036_0219f178(camera->field, &pos);
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
        if (func_ov036_021b7f50(camera)) {
            FieldCamera_ChangeRefPos(fieldCamera, &pos);
        }
        FieldCameraAnimationController_PrepareCamera(camera->controller);
        FieldCamera_GetAnimationCoords(camera->camera, &animation->src);
        switch (dir) {
        case DIR_UP:
            break;
        case DIR_DOWN:
            animation->src.distance = 0x2d << 14;
            break;
        case DIR_LEFT:
            animation->src.yaw = 0xf5f7;
            break;
        case DIR_RIGHT:
            animation->src.yaw = 0xa09;
            break;
        }
        FieldCamera_GetAnimationCoords(camera->camera, dst);
        if (camera->unk6c != 0) {
            FieldCameraAnimationController_CommitSrcCoords(camera->controller);
        }
    }
}

void EntranceCamera_ConfigureFileDeparture(EntranceCamera *camera) {
    FieldCameraAnimation *animation = &camera->animation;
    FieldCameraCoords *dst = &animation->dst;
    FieldPlayer *player;
    VecFx32 pos;

    EntranceCamera_GetFieldCamera(camera);
    {
        EntranceCameraFile file;

        player = Field_GetPlayer(camera->field);
        FieldPlayer_GetFaceDir(player);
        EntranceCamera_ReadFile(&file, EntranceCamera_GetTransitionType(camera));
        animation->unk4C = file.unk4C;
        animation->unk48 = file.unk48;
        camera->unk68 = file.unk68;
        camera->unk6c = file.unk6c;
        if (camera->unk68 != 0) {
            if (func_ov036_021b7f50(camera)) {
                func_ov036_0219f178(camera->field, &pos);
            } else {
                FieldPlayer_GetWPos(player, &pos);
            }
            FieldCameraAnimationController_PrepareCamera(camera->controller);
            FieldCamera_GetAnimationCoords(camera->camera, &animation->src);
            animation->frames = file.frames;
            dst->pitch = file.pitch;
            dst->yaw = file.yaw;
            dst->distance = file.distance << 12;
            VEC_Set(&dst->offset, file.offset[0] << 12, file.offset[1] << 12, file.offset[2] << 12);
            if (file.useTarget != 0) {
                VEC_Set(&dst->target, file.target[0] << 12, file.target[1] << 12, file.target[2] << 12);
            } else {
                dst->target = pos;
            }
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

    {
        EntranceCameraFile file;

        player = Field_GetPlayer(camera->field);
        FieldPlayer_GetFaceDir(player);
        EntranceCamera_ReadFile(&file, EntranceCamera_GetTransitionType(camera));
        animation->unk4C = file.unk4C;
        animation->unk48 = file.unk48;
        camera->unk68 = file.unk68;
        camera->unk6c = file.unk6c;
        if (camera->unk6c != 0) {
            if (func_ov036_021b7f50(camera)) {
                func_ov036_0219f178(camera->field, &pos);
            } else {
                FieldPlayer_GetWPos(player, &pos);
            }
            if (animation->unk48 != 0 && func_ov036_021b7f50(camera)) {
                FieldCamera_ChangeRefPos(fieldCamera, &pos);
            }
            FieldCameraAnimationController_PrepareCamera(camera->controller);
            FieldCamera_GetAnimationCoords(camera->camera, dst);
            animation->frames = file.frames;
            src->pitch = file.pitch;
            src->yaw = file.yaw;
            src->distance = file.distance << 12;
            VEC_Set(&src->offset, file.offset[0] << 12, file.offset[1] << 12, file.offset[2] << 12);
            if (file.useTarget != 0) {
                VEC_Set(&src->target, file.target[0] << 12, file.target[1] << 12, file.target[2] << 12);
            } else {
                src->target = pos;
            }
            if (camera->unk6c != 0) {
                FieldCameraAnimationController_CommitSrcCoords(camera->controller);
            }
        }
    }
}

void EntranceCamera_ReadFile(EntranceCameraFile *dest, u32 index) {
    GFL_ArcSysRead(dest, ARCID_ENTRANCE_CAMERA, index - 7);
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

BOOL func_ov036_021b7f40(EntranceCamera *camera) {
    return func_ov012_0215cd98(EntranceCamera_GetParam(camera)->transitionType);
}

BOOL func_ov036_021b7f50(EntranceCamera *camera) {
    if (EntranceCamera_GetParam(camera)->isDeparture == 0) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov036_021b7f64(EntranceCamera *camera) {
    if (EntranceCamera_GetParam(camera)->isArrival == 0) {
        return camera->unk68;
    }
    return camera->unk6c;
}
