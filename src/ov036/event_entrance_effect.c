// The events that play an entrance, such as a door, when the player comes out of it or goes into it during a warp:
// the door's animations and sounds, the entrance camera, and the transitions. The name is descriptive; the ROM has no
// string for this file
#include "types.h"
#include "field/event_action_call.h"
#include "field/event_entrance_effect.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_event.h"
#include "field/field_g3d_mapper.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_prop.h"
#include "field/field_rail.h"
#include "field/field_sound.h"
#include "field/zone.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The steps of an entrance event, which it plays in the order of its steps array
enum {
    ENTRANCE_STEP_START,
    ENTRANCE_STEP_TRANSITION,
    ENTRANCE_STEP_2,
    ENTRANCE_STEP_OPEN_DOOR,
    ENTRANCE_STEP_WAIT_OPEN,
    ENTRANCE_STEP_WALK,
    ENTRANCE_STEP_CAMERA,
    ENTRANCE_STEP_CLOSE_DOOR,
    ENTRANCE_STEP_WAIT_CLOSE,
    ENTRANCE_STEP_END,
    ENTRANCE_STEP_WAIT_DELAY,
    ENTRANCE_STEP_WAIT_TIMER,
};

typedef struct {
    WarpSequence *warp;
    FieldSound *fieldSound;
    void *camera;
    EntranceCameraParam cameraParam;
    u16 timer;
    u8 dir;
    VecFx32 pos;
    FieldPropHandle *door;
    u8 steps[10];
    u8 stepIndex;
    // Whether the departure's steps return GAMEEVENT_CONTINUE_DIRECT, in a city
    BOOL continueDirect;
} EntranceEffectWork;

static void func_ov036_0219f180(Field *field);
static void func_ov036_0219f1f8(EntranceEffectWork *work);
static GameEventReturnCode func_ov036_0219f380(GameEvent *event, u32 *state, void *data);
static void func_ov036_0219f558(EntranceEffectWork *work);
static GameEventReturnCode func_ov036_0219f5dc(GameEvent *event, u32 *state, void *data);
static u8 Field_GetPlayerDirection(Field *field);
static void func_ov036_0219f7b0(Field *field, VecFx32 *pos);
static void func_ov036_0219f7c0(Field *field, VecFx32 *pos);
static void func_ov036_0219f7e4(Field *field, VecFx32 *pos);
static void func_ov036_0219f83c(Field *field, VecFx32 *pos);
static u8 func_ov036_0219f880(EntranceEffectWork *work);
static u8 func_ov036_0219f890(EntranceEffectWork *work);
static void func_ov036_0219f8a4(EntranceEffectWork *work);
static void func_ov036_0219f8b8(EntranceEffectWork *work);
static void func_ov036_0219f8d8(EntranceEffectWork *work);
static BOOL func_ov036_0219f8e8(EntranceEffectWork *work);
static BOOL func_ov036_0219f90c(EntranceEffectWork *work);
static void func_ov036_0219f91c(EntranceEffectWork *work);
static void func_ov036_0219f94c(EntranceEffectWork *work);
static void func_ov036_0219f97c(GameEvent *event, EntranceEffectWork *work);
static void func_ov036_0219f9c8(GameEvent *event, EntranceEffectWork *work);
static BOOL func_ov036_0219f9f0(EntranceEffectWork *work);

void func_ov036_0219f178(Field *field, VecFx32 *pos) {
    func_ov036_0219f7c0(field, pos);
}

static void func_ov036_0219f180(Field *field) {
    func_ov012_02166f2c(FieldPlayer_GetActor(Field_GetPlayer(field)));
}

GameEvent *func_ov036_0219f190(WarpSequence *warp) {
    GameEvent *event = GameEvent_Create(warp->gsys, NULL, func_ov036_0219f380, sizeof(EntranceEffectWork));
    EntranceEffectWork *work = GameEvent_GetData(event);

    work->warp = warp;
    work->fieldSound = GameData_GetFieldSoundSystem(warp->gameData);
    work->door = NULL;
    work->timer = 0;
    work->dir = Field_GetPlayerDirection(warp->field);
    work->stepIndex = 0;
    work->camera = func_ov036_021b7a34(warp->field);
    work->cameraParam.transitionType = warp->transitionType;
    work->cameraParam.isArrival = TRUE;
    work->cameraParam.isDeparture = FALSE;
    func_ov036_0219f7b0(warp->field, &work->pos);
    func_ov036_0219f8b8(work);
    func_ov036_0219f1f8(work);
    return event;
}

static void func_ov036_0219f1f8(EntranceEffectWork *work) {
    WarpSequence *warp = work->warp;
    Field *field = warp->field;

    if (func_ov036_0219f90c(work)) {
        switch (Field_GetPlayerDirection(field)) {
        case DIR_UP:
        case DIR_DOWN:
        default:
            work->steps[0] = ENTRANCE_STEP_START;
            work->steps[1] = ENTRANCE_STEP_TRANSITION;
            work->steps[2] = ENTRANCE_STEP_2;
            work->steps[3] = ENTRANCE_STEP_CAMERA;
            work->steps[4] = ENTRANCE_STEP_OPEN_DOOR;
            work->steps[5] = ENTRANCE_STEP_WAIT_OPEN;
            work->steps[6] = ENTRANCE_STEP_WALK;
            work->steps[7] = ENTRANCE_STEP_CLOSE_DOOR;
            work->steps[8] = ENTRANCE_STEP_WAIT_CLOSE;
            work->steps[9] = ENTRANCE_STEP_END;
            break;
        case DIR_LEFT:
        case DIR_RIGHT:
            work->steps[0] = ENTRANCE_STEP_START;
            work->steps[1] = ENTRANCE_STEP_TRANSITION;
            work->steps[2] = ENTRANCE_STEP_2;
            work->steps[3] = ENTRANCE_STEP_OPEN_DOOR;
            work->steps[4] = ENTRANCE_STEP_WAIT_OPEN;
            work->steps[5] = ENTRANCE_STEP_WALK;
            work->steps[6] = ENTRANCE_STEP_CAMERA;
            work->steps[7] = ENTRANCE_STEP_CLOSE_DOOR;
            work->steps[8] = ENTRANCE_STEP_WAIT_CLOSE;
            work->steps[9] = ENTRANCE_STEP_END;
            break;
        }
    } else if (func_ov012_0215cd98(warp->transitionType)) {
        work->steps[0] = ENTRANCE_STEP_START;
        work->steps[1] = ENTRANCE_STEP_TRANSITION;
        work->steps[2] = ENTRANCE_STEP_2;
        work->steps[3] = ENTRANCE_STEP_WALK;
        work->steps[4] = ENTRANCE_STEP_CAMERA;
        work->steps[5] = ENTRANCE_STEP_END;
    } else {
        switch (Field_GetPlayerDirection(field)) {
        case DIR_UP:
        case DIR_DOWN:
        default:
            work->steps[0] = ENTRANCE_STEP_START;
            work->steps[1] = ENTRANCE_STEP_TRANSITION;
            work->steps[2] = ENTRANCE_STEP_2;
            work->steps[3] = ENTRANCE_STEP_CAMERA;
            work->steps[4] = ENTRANCE_STEP_WALK;
            work->steps[5] = ENTRANCE_STEP_END;
            break;
        case DIR_LEFT:
        case DIR_RIGHT:
            work->steps[0] = ENTRANCE_STEP_START;
            work->steps[1] = ENTRANCE_STEP_TRANSITION;
            work->steps[2] = ENTRANCE_STEP_2;
            work->steps[3] = ENTRANCE_STEP_WALK;
            work->steps[4] = ENTRANCE_STEP_CAMERA;
            work->steps[5] = ENTRANCE_STEP_END;
            break;
        }
    }
}

// The player comes out of the entrance
static GameEventReturnCode func_ov036_0219f380(GameEvent *event, u32 *state, void *data) {
    EntranceEffectWork *work = data;
    WarpSequence *warp = work->warp;
    GameSystem *gsys = warp->gsys;
    GameData *gameData = warp->gameData;
    Field *field = warp->field;
    FieldSound *fieldSound = work->fieldSound;
    FieldCamera *camera = Field_GetCameraSystem(field);
    FieldTaskManager *taskManager = Field_GetTaskManager(field);

    Field_GetPlayerStateZoneID(field);
    switch (*state) {
    case ENTRANCE_STEP_START:
        Field_SetPlayerHidden(field, TRUE);
        if (!FieldCamera_SupportsDelay(camera)) {
            *state = func_ov036_0219f890(work);
            func_ov036_0219f8a4(work);
            break;
        }
        FieldCamera_FinishDelay(camera);
        *state = ENTRANCE_STEP_WAIT_DELAY;
        break;
    case ENTRANCE_STEP_WAIT_DELAY:
        if (!FieldCamera_IsDelayActive(camera)) {
            *state = func_ov036_0219f890(work);
            func_ov036_0219f8a4(work);
        }
        break;
    case ENTRANCE_STEP_TRANSITION:
        func_ov036_021b7a68(work->camera, &work->cameraParam);
        FieldSnd_FadeInImmediate(fieldSound, gameData);
        func_ov036_0219f97c(event, work);
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        break;
    case ENTRANCE_STEP_2:
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        break;
    case ENTRANCE_STEP_OPEN_DOOR:
        func_ov036_0219f91c(work);
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        break;
    case ENTRANCE_STEP_WAIT_OPEN:
        if (func_ov036_0219f8e8(work) == TRUE) {
            *state = func_ov036_0219f890(work);
            func_ov036_0219f8a4(work);
        }
        break;
    case ENTRANCE_STEP_WALK:
        Field_SetPlayerHidden(field, FALSE);
        GameEvent_ChainNext(event, CallMoveOneTileFrontEvent(gsys, field));
        if (func_ov036_021b7ae0(work->camera) && func_ov036_021b7ae8(work->camera)) {
            *state = ENTRANCE_STEP_WAIT_TIMER;
            break;
        }
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        break;
    case ENTRANCE_STEP_CLOSE_DOOR:
        func_ov036_0219f94c(work);
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        break;
    case ENTRANCE_STEP_WAIT_TIMER:
        if (work->timer++ > 15) {
            *state = func_ov036_0219f890(work);
            func_ov036_0219f8a4(work);
        }
        break;
    case ENTRANCE_STEP_CAMERA:
        func_ov036_021b7ac0(work->camera);
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        break;
    case ENTRANCE_STEP_WAIT_CLOSE:
        if (func_ov036_0219f8e8(work) == TRUE) {
            *state = func_ov036_0219f890(work);
            func_ov036_0219f8a4(work);
        }
        break;
    case ENTRANCE_STEP_END:
        break;
    }
    if (GSYS_GetNowEvent(gsys) == event && func_ov036_0219f880(work) == ENTRANCE_STEP_END &&
        FieldTaskManager_IsIdle(taskManager)) {
        func_ov036_021b7ad4(work->camera);
        FieldCamera_LoadDefaults(camera);
        func_ov036_021b7a58(work->camera);
        func_ov036_0219f8d8(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *func_ov036_0219f4e0(WarpSequence *warp) {
    GameEvent *event = GameEvent_Create(warp->gsys, NULL, func_ov036_0219f5dc, sizeof(EntranceEffectWork));
    EntranceEffectWork *work = GameEvent_GetData(event);

    work->warp = warp;
    work->fieldSound = GameData_GetFieldSoundSystem(warp->gameData);
    work->door = NULL;
    work->timer = 0;
    work->dir = Field_GetPlayerDirection(warp->field);
    work->stepIndex = 0;
    work->camera = func_ov036_021b7a34(warp->field);
    work->cameraParam.transitionType = warp->unk10;
    work->cameraParam.isArrival = FALSE;
    work->cameraParam.isDeparture = TRUE;
    func_ov036_0219f7c0(warp->field, &work->pos);
    func_ov036_0219f8b8(work);
    func_ov036_0219f558(work);
    if (func_ov012_0215cd98(warp->unk10)) {
        work->continueDirect = TRUE;
    } else {
        work->continueDirect = FALSE;
    }
    return event;
}

static void func_ov036_0219f558(EntranceEffectWork *work) {
    if (func_ov036_0219f90c(work)) {
        work->steps[0] = 0;
        work->steps[1] = 3;
        work->steps[2] = 1;
        work->steps[3] = 2;
        work->steps[4] = 5;
        work->steps[5] = 6;
        work->steps[6] = 7;
        work->steps[7] = 8;
    } else {
        work->steps[0] = 0;
        work->steps[1] = 1;
        work->steps[2] = 2;
        work->steps[3] = 5;
        work->steps[4] = 6;
        work->steps[5] = 7;
        work->steps[6] = 8;
    }
}

// The player goes into the entrance
static GameEventReturnCode func_ov036_0219f5dc(GameEvent *event, u32 *state, void *data) {
    EntranceEffectWork *work = data;
    WarpSequence *warp = work->warp;
    GameSystem *gsys = warp->gsys;
    GameData *gameData = warp->gameData;
    Field *field = warp->field;
    FieldSound *fieldSound = work->fieldSound;
    FieldTaskManager *taskManager = Field_GetTaskManager(field);

    switch (*state) {
    case 0:
        func_ov036_021b7a68(work->camera, &work->cameraParam);
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        if (work->continueDirect) {
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case 1:
        func_ov036_021b7ac0(work->camera);
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        if (work->continueDirect) {
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case 3:
        func_ov036_0219f91c(work);
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        if (work->continueDirect) {
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case 2:
        if (!FieldTaskManager_IsIdle(taskManager)) {
            break;
        }
        if (func_ov036_021b7ae0(work->camera) && func_ov036_021b7ae8(work->camera)) {
            *state = 9;
        } else {
            *state = func_ov036_0219f890(work);
            func_ov036_0219f8a4(work);
        }
        if (work->continueDirect) {
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case 4:
        if (func_ov036_0219f8e8(work) == TRUE) {
            *state = func_ov036_0219f890(work);
            func_ov036_0219f8a4(work);
            if (work->continueDirect) {
                return GAMEEVENT_CONTINUE_DIRECT;
            }
        }
        break;
    case 9:
        if (work->timer++ > 15) {
            *state = func_ov036_0219f890(work);
            func_ov036_0219f8a4(work);
            if (work->continueDirect) {
                return GAMEEVENT_CONTINUE_DIRECT;
            }
        }
        break;
    case 5:
        func_ov036_0219f180(field);
        GameEvent_ChainNext(event, func_ov036_021b74a0(gsys, warp));
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        if (work->continueDirect) {
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case 6:
        FieldSnd_SetZoneBGM(fieldSound, gameData, warp->spawn.zoneId, warp->endSeason);
        warp->unk48 = TRUE;
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        break;
    case 7:
        if (func_ov036_0219f9f0(work)) {
            GFL_SndSEPlay(0x559);
        }
        func_ov036_0219f9c8(event, work);
        *state = func_ov036_0219f890(work);
        func_ov036_0219f8a4(work);
        break;
    case 8:
        func_ov036_021b7ad4(work->camera);
        func_ov036_021b7a58(work->camera);
        func_ov036_0219f8d8(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static u8 Field_GetPlayerDirection(Field *field) {
    return FieldPlayer_GetFaceDir(Field_GetPlayer(field));
}

static void func_ov036_0219f7b0(Field *field, VecFx32 *pos) {
    FieldPlayer_GetWPos(Field_GetPlayer(field), pos);
}

static void func_ov036_0219f7c0(Field *field, VecFx32 *pos) {
    if (Field_GetResolvedControllerTypeID(field) == 1) {
        func_ov036_0219f83c(field, pos);
    } else {
        func_ov036_0219f7e4(field, pos);
    }
}

// On the grid: a tile in front of the player, at the map's height there
static void func_ov036_0219f7e4(Field *field, VecFx32 *pos) {
    FieldPlayer *player = Field_GetPlayer(field);
    u32 dir = FieldPlayer_GetFaceDir(player);
    VecFx32 playerPos;
    VecFx32 dirVec;
    VecFx32 front;
    fx32 height;

    FieldPlayer_GetWPos(player, &playerPos);
    func_ov036_0219aab0(player, dir, &dirVec);
    VEC_MultAdd(FX32_CONST(16), &dirVec, &playerPos, &front);
    GetHeightFromMap(FieldPlayer_GetActor(player), &front, &height);
    front.y = height;
    *pos = front;
}

// On rails: a step in front of the player on its rail
static void func_ov036_0219f83c(Field *field, VecFx32 *pos) {
    FieldPlayer *player = Field_GetPlayer(field);
    u32 dir = FieldPlayer_GetFaceDir(player);
    FieldRailSystem *rail = FieldNoGridMapper_GetRailSystem(Field_GetNoGridMapper(field));
    VecFx32 front;
    RailPosition railPos;

    func_ov036_0219ad30(player, dir, &railPos);
    func_ov036_021b06ec(rail, &railPos, &front);
    *pos = front;
}

static u8 func_ov036_0219f880(EntranceEffectWork *work) {
    return work->steps[work->stepIndex];
}

static u8 func_ov036_0219f890(EntranceEffectWork *work) {
    return work->steps[(u8)(work->stepIndex + 1)];
}

static void func_ov036_0219f8a4(EntranceEffectWork *work) {
    work->stepIndex++;
    work->timer = 0;
}

static void func_ov036_0219f8b8(EntranceEffectWork *work) {
    work->door = FieldPropSystem_CreateHandleAtPos(FieldG3DMapper_GetBMSystem(Field_GetG3DMapper(work->warp->field)),
                                                   1, &work->pos);
}

static void func_ov036_0219f8d8(EntranceEffectWork *work) {
    if (work->door != NULL) {
        FieldPropHandle_Free(work->door);
    }
}

// Whether the door's animation is done, or there is no door
static BOOL func_ov036_0219f8e8(EntranceEffectWork *work) {
    if (!func_ov036_0219f90c(work)) {
        return TRUE;
    }
    if (FieldPropHandle_IsAnmFinished(work->door)) {
        return TRUE;
    }
    return FALSE;
}

// Whether there is a door
static BOOL func_ov036_0219f90c(EntranceEffectWork *work) {
    if (work->door != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_0219f91c(EntranceEffectWork *work) {
    u16 soundId;

    if (func_ov036_0219f90c(work)) {
        FieldPropHandle_CallAnmCmd(work->door, 0, 0);
        if (FieldPropHandle_GetAnimSoundID(work->door, &soundId)) {
            GFL_SndSEPlay(soundId);
        }
    }
}

static void func_ov036_0219f94c(EntranceEffectWork *work) {
    u16 soundId;

    if (func_ov036_0219f90c(work)) {
        FieldPropHandle_CallAnmCmd(work->door, 1, 0);
        if (FieldPropHandle_GetAnimSoundID(work->door, &soundId)) {
            GFL_SndSEPlay(soundId);
        }
    }
}

static void func_ov036_0219f97c(GameEvent *event, EntranceEffectWork *work) {
    WarpSequence *warp = work->warp;
    GameEvent *transition;

    if (warp->seasonChanged) {
        transition = CallFieldMapEntranceInTransition(warp->gsys, warp->field, 3, 0, 0, warp->startSeason,
                                                      warp->endSeason);
    } else {
        transition = CallFieldMapEntranceInTransition(warp->gsys, warp->field, warp->inTransition, 0, 1, 0, 0);
    }
    GameEvent_ChainNext(event, transition);
}

static void func_ov036_0219f9c8(GameEvent *event, EntranceEffectWork *work) {
    WarpSequence *warp = work->warp;
    GameEvent *transition;

    if (warp->seasonChanged) {
        transition = CallFieldMapEntranceOutTransitionDefault(warp->gsys, warp->field, 0, 0);
    } else {
        transition = CallFieldMapEntranceOutTransitionDefault(warp->gsys, warp->field, warp->outTransition, 0);
    }
    GameEvent_ChainNext(event, transition);
}

// Whether to play the sound of going in without a door
static BOOL func_ov036_0219f9f0(EntranceEffectWork *work) {
    WarpSequence *warp = work->warp;

    if (func_ov036_0219f90c(work)) {
        return FALSE;
    }
    if (warp->outTransition != 2) {
        return TRUE;
    }
    return FALSE;
}
