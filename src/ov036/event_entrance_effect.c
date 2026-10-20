// The events that play an entrance, such as a door, when the player comes out of it or goes into it during a warp:
// the door's animations and sounds, the entrance camera, and the transitions. The name is descriptive; the ROM has no
// string for this file
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/sound.h"
#include "field/entrance_camera.h"
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

static void EntranceEffect_FreePlayerAcmd(Field *field);
static void EntranceEffect_SetArrivalSteps(EntranceEffectWork *work);
static GameEventReturnCode EntranceEffect_ArrivalCallback(GameEvent *event, u32 *state, void *data);
static void EntranceEffect_SetDepartureSteps(EntranceEffectWork *work);
static GameEventReturnCode EntranceEffect_DepartureCallback(GameEvent *event, u32 *state, void *data);
static u8 Field_GetPlayerDirection(Field *field);
static void EntranceEffect_GetPlayerPosition(Field *field, VecFx32 *pos);
static void EntranceEffect_GetFrontPosition(Field *field, VecFx32 *pos);
static void EntranceEffect_GetGridFrontPosition(Field *field, VecFx32 *pos);
static void EntranceEffect_GetRailFrontPosition(Field *field, VecFx32 *pos);
static u8 EntranceEffect_GetStep(EntranceEffectWork *work);
static u8 EntranceEffect_GetNextStep(EntranceEffectWork *work);
static void EntranceEffect_AdvanceStep(EntranceEffectWork *work);
static void EntranceEffect_CreateDoorHandle(EntranceEffectWork *work);
static void EntranceEffect_FreeDoorHandle(EntranceEffectWork *work);
static BOOL EntranceEffect_IsDoorAnimationDone(EntranceEffectWork *work);
static BOOL EntranceEffect_HasDoor(EntranceEffectWork *work);
static void EntranceEffect_OpenDoor(EntranceEffectWork *work);
static void EntranceEffect_CloseDoor(EntranceEffectWork *work);
static void EntranceEffect_ChainArrivalTransition(GameEvent *event, EntranceEffectWork *work);
static void EntranceEffect_ChainDepartureTransition(GameEvent *event, EntranceEffectWork *work);
static BOOL EntranceEffect_ShouldPlayStairsSound(EntranceEffectWork *work);

void func_ov036_0219f178(Field *field, VecFx32 *pos) {
    EntranceEffect_GetFrontPosition(field, pos);
}

static void EntranceEffect_FreePlayerAcmd(Field *field) {
    FldAct_FreeAcmd(FieldPlayer_GetActor(Field_GetPlayer(field)));
}

GameEvent *func_ov036_0219f190(WarpSequence *warp) {
    GameEvent *event = GameEvent_Create(warp->gsys, NULL, EntranceEffect_ArrivalCallback, sizeof(EntranceEffectWork));
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
    EntranceEffect_GetPlayerPosition(warp->field, &work->pos);
    EntranceEffect_CreateDoorHandle(work);
    EntranceEffect_SetArrivalSteps(work);
    return event;
}

static void EntranceEffect_SetArrivalSteps(EntranceEffectWork *work) {
    WarpSequence *warp = work->warp;
    Field *field = warp->field;

    if (EntranceEffect_HasDoor(work)) {
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
static GameEventReturnCode EntranceEffect_ArrivalCallback(GameEvent *event, u32 *state, void *data) {
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
            *state = EntranceEffect_GetNextStep(work);
            EntranceEffect_AdvanceStep(work);
            break;
        }
        FieldCamera_FinishDelay(camera);
        *state = ENTRANCE_STEP_WAIT_DELAY;
        break;
    case ENTRANCE_STEP_WAIT_DELAY:
        if (!FieldCamera_IsDelayActive(camera)) {
            *state = EntranceEffect_GetNextStep(work);
            EntranceEffect_AdvanceStep(work);
        }
        break;
    case ENTRANCE_STEP_TRANSITION:
        func_ov036_021b7a68(work->camera, &work->cameraParam);
        FieldSnd_FadeInImmediate(fieldSound, gameData);
        EntranceEffect_ChainArrivalTransition(event, work);
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
        break;
    case ENTRANCE_STEP_2:
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
        break;
    case ENTRANCE_STEP_OPEN_DOOR:
        EntranceEffect_OpenDoor(work);
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
        break;
    case ENTRANCE_STEP_WAIT_OPEN:
        if (EntranceEffect_IsDoorAnimationDone(work) == TRUE) {
            *state = EntranceEffect_GetNextStep(work);
            EntranceEffect_AdvanceStep(work);
        }
        break;
    case ENTRANCE_STEP_WALK:
        Field_SetPlayerHidden(field, FALSE);
        GameEvent_ChainNext(event, CallMoveOneTileFrontEvent(gsys, field));
        if (func_ov036_021b7ae0(work->camera) && func_ov036_021b7ae8(work->camera)) {
            *state = ENTRANCE_STEP_WAIT_TIMER;
            break;
        }
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
        break;
    case ENTRANCE_STEP_CLOSE_DOOR:
        EntranceEffect_CloseDoor(work);
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
        break;
    case ENTRANCE_STEP_WAIT_TIMER:
        if (work->timer++ > 15) {
            *state = EntranceEffect_GetNextStep(work);
            EntranceEffect_AdvanceStep(work);
        }
        break;
    case ENTRANCE_STEP_CAMERA:
        func_ov036_021b7ac0(work->camera);
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
        break;
    case ENTRANCE_STEP_WAIT_CLOSE:
        if (EntranceEffect_IsDoorAnimationDone(work) == TRUE) {
            *state = EntranceEffect_GetNextStep(work);
            EntranceEffect_AdvanceStep(work);
        }
        break;
    case ENTRANCE_STEP_END:
        break;
    }
    if (GSYS_GetNowEvent(gsys) == event && EntranceEffect_GetStep(work) == ENTRANCE_STEP_END &&
        FieldTaskManager_IsIdle(taskManager)) {
        func_ov036_021b7ad4(work->camera);
        FieldCamera_LoadDefaults(camera);
        func_ov036_021b7a58(work->camera);
        EntranceEffect_FreeDoorHandle(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *func_ov036_0219f4e0(WarpSequence *warp) {
    GameEvent *event = GameEvent_Create(warp->gsys, NULL, EntranceEffect_DepartureCallback, sizeof(EntranceEffectWork));
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
    EntranceEffect_GetFrontPosition(warp->field, &work->pos);
    EntranceEffect_CreateDoorHandle(work);
    EntranceEffect_SetDepartureSteps(work);
    if (func_ov012_0215cd98(warp->unk10)) {
        work->continueDirect = TRUE;
    } else {
        work->continueDirect = FALSE;
    }
    return event;
}

static void EntranceEffect_SetDepartureSteps(EntranceEffectWork *work) {
    if (EntranceEffect_HasDoor(work)) {
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
static GameEventReturnCode EntranceEffect_DepartureCallback(GameEvent *event, u32 *state, void *data) {
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
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
        if (work->continueDirect) {
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case 1:
        func_ov036_021b7ac0(work->camera);
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
        if (work->continueDirect) {
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case 3:
        EntranceEffect_OpenDoor(work);
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
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
            *state = EntranceEffect_GetNextStep(work);
            EntranceEffect_AdvanceStep(work);
        }
        if (work->continueDirect) {
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case 4:
        if (EntranceEffect_IsDoorAnimationDone(work) == TRUE) {
            *state = EntranceEffect_GetNextStep(work);
            EntranceEffect_AdvanceStep(work);
            if (work->continueDirect) {
                return GAMEEVENT_CONTINUE_DIRECT;
            }
        }
        break;
    case 9:
        if (work->timer++ > 15) {
            *state = EntranceEffect_GetNextStep(work);
            EntranceEffect_AdvanceStep(work);
            if (work->continueDirect) {
                return GAMEEVENT_CONTINUE_DIRECT;
            }
        }
        break;
    case 5:
        EntranceEffect_FreePlayerAcmd(field);
        GameEvent_ChainNext(event, func_ov036_021b74a0(gsys, warp));
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
        if (work->continueDirect) {
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case 6:
        FieldSnd_SetZoneBGM(fieldSound, gameData, warp->spawn.zoneId, warp->endSeason);
        warp->unk48 = TRUE;
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
        break;
    case 7:
        if (EntranceEffect_ShouldPlayStairsSound(work)) {
            GFL_SndSEPlay(SEQ_SE_KAIDAN);
        }
        EntranceEffect_ChainDepartureTransition(event, work);
        *state = EntranceEffect_GetNextStep(work);
        EntranceEffect_AdvanceStep(work);
        break;
    case 8:
        func_ov036_021b7ad4(work->camera);
        func_ov036_021b7a58(work->camera);
        EntranceEffect_FreeDoorHandle(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static u8 Field_GetPlayerDirection(Field *field) {
    return FieldPlayer_GetFaceDir(Field_GetPlayer(field));
}

static void EntranceEffect_GetPlayerPosition(Field *field, VecFx32 *pos) {
    FieldPlayer_GetWPos(Field_GetPlayer(field), pos);
}

static void EntranceEffect_GetFrontPosition(Field *field, VecFx32 *pos) {
    if (Field_GetResolvedControllerTypeID(field) == 1) {
        EntranceEffect_GetRailFrontPosition(field, pos);
    } else {
        EntranceEffect_GetGridFrontPosition(field, pos);
    }
}

// On the grid: a tile in front of the player, at the map's height there
static void EntranceEffect_GetGridFrontPosition(Field *field, VecFx32 *pos) {
    FieldPlayer *player = Field_GetPlayer(field);
    u16 dir = FieldPlayer_GetFaceDir(player);
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
static void EntranceEffect_GetRailFrontPosition(Field *field, VecFx32 *pos) {
    FieldPlayer *player = Field_GetPlayer(field);
    u16 dir = FieldPlayer_GetFaceDir(player);
    FieldRailSystem *rail = FieldNoGridMapper_GetRailSystem(Field_GetNoGridMapper(field));
    VecFx32 front;
    RailPosition railPos;

    func_ov036_0219ad30(player, dir, &railPos);
    func_ov036_021b06ec(rail, &railPos, &front);
    *pos = front;
}

static u8 EntranceEffect_GetStep(EntranceEffectWork *work) {
    return work->steps[work->stepIndex];
}

static u8 EntranceEffect_GetNextStep(EntranceEffectWork *work) {
    return work->steps[(u8)(work->stepIndex + 1)];
}

static void EntranceEffect_AdvanceStep(EntranceEffectWork *work) {
    work->stepIndex++;
    work->timer = 0;
}

static void EntranceEffect_CreateDoorHandle(EntranceEffectWork *work) {
    work->door = FieldPropSystem_CreateHandleAtPos(FieldG3DMapper_GetBMSystem(Field_GetG3DMapper(work->warp->field)), 1,
                                                   &work->pos);
}

static void EntranceEffect_FreeDoorHandle(EntranceEffectWork *work) {
    if (work->door != NULL) {
        FieldPropHandle_Free(work->door);
    }
}

// Whether the door's animation is done, or there is no door
static BOOL EntranceEffect_IsDoorAnimationDone(EntranceEffectWork *work) {
    if (!EntranceEffect_HasDoor(work)) {
        return TRUE;
    }
    if (FieldPropHandle_IsAnmFinished(work->door)) {
        return TRUE;
    }
    return FALSE;
}

// Whether there is a door
static BOOL EntranceEffect_HasDoor(EntranceEffectWork *work) {
    if (work->door != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void EntranceEffect_OpenDoor(EntranceEffectWork *work) {
    u16 soundId;

    if (EntranceEffect_HasDoor(work)) {
        FieldPropHandle_CallAnmCmd(work->door, 0, 0);
        if (FieldPropHandle_GetAnimSoundID(work->door, &soundId)) {
            GFL_SndSEPlay(soundId);
        }
    }
}

static void EntranceEffect_CloseDoor(EntranceEffectWork *work) {
    u16 soundId;

    if (EntranceEffect_HasDoor(work)) {
        FieldPropHandle_CallAnmCmd(work->door, 1, 0);
        if (FieldPropHandle_GetAnimSoundID(work->door, &soundId)) {
            GFL_SndSEPlay(soundId);
        }
    }
}

static void EntranceEffect_ChainArrivalTransition(GameEvent *event, EntranceEffectWork *work) {
    WarpSequence *warp = work->warp;
    GameEvent *transition;

    if (warp->seasonChanged) {
        transition =
            CallFieldMapEntranceInTransition(warp->gsys, warp->field, 3, 0, 0, warp->startSeason, warp->endSeason);
    } else {
        transition = CallFieldMapEntranceInTransition(warp->gsys, warp->field, warp->inTransition, 0, 1, 0, 0);
    }
    GameEvent_ChainNext(event, transition);
}

static void EntranceEffect_ChainDepartureTransition(GameEvent *event, EntranceEffectWork *work) {
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
static BOOL EntranceEffect_ShouldPlayStairsSound(EntranceEffectWork *work) {
    WarpSequence *warp = work->warp;

    if (EntranceEffect_HasDoor(work)) {
        return FALSE;
    }
    if (warp->outTransition != 2) {
        return TRUE;
    }
    return FALSE;
}
