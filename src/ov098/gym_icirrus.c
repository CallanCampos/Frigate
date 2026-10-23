#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_controller.h"
#include "field/field_exp_obj.h"
#include "field/field_map.h"
#include "field/fieldmap_ctrl_hybrid.h"
#include "field/gym_icirrus.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

#define GIMMICK_WORK_ID 1
#define GIMMICK_STATE_ID 9
// The archive of the gym's models
#define GIMMICK_ARCID 0x98

#define SWITCH_COUNT 3
// The second model of each switch comes this many models after its first
#define SECOND_MODEL_OFFSET 3
#define PLACE_COUNT 8
// The controller type of maps with both grid and rail movement
#define CONTROLLER_TYPE_HYBRID 2

// What the save keeps of the gym: whether each switch is flipped
typedef struct {
    BOOL flipped[SWITCH_COUNT];
} IcirrusGymState;

// The switch that the running event animates
typedef struct {
    u32 switchIndex;
} IcirrusGymWork;

// A place where the player can change movement, and the state that its switch must be in
typedef struct {
    u16 switchIndex;
    u16 flipped;
} PlacePrecondition;

static BOOL GymIcirrus_IsPlaceSwitchReady(IcirrusGymState *state, int place);
static void GymIcirrus_ShowSwitchState(FieldExpObjSystem *expObj, BOOL flipped, int switchIndex, int second);
static BOOL GymIcirrus_IsSwitchFlipped(IcirrusGymState *state, int switchIndex);
static GameEventReturnCode GymIcirrus_SwitchAnimEvent(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode GymIcirrus_FlipSwitchEvent(GameEvent *event, u32 *state, void *data);

// The animation of a model
static inline FieldExpObjAnm *GetAnimation(FieldExpObjSystem *expObj, u8 model, u8 animation) {
    return FieldExpObj_GetAnmInfo(expObj, 0, model, animation);
}

static const G3DSceneResourceSetup sResources[9] = {
    { GIMMICK_ARCID, 0, 0 }, { GIMMICK_ARCID, 1, 0 }, { GIMMICK_ARCID, 2, 0 },
    { GIMMICK_ARCID, 3, 0 }, { GIMMICK_ARCID, 4, 0 }, { GIMMICK_ARCID, 5, 0 },
    { GIMMICK_ARCID, 6, 0 }, { GIMMICK_ARCID, 8, 0 }, { GIMMICK_ARCID, 7, 0 },
};

static const G3DSceneAnimationSetup sModelAAnimations[] = { { 5, 0 }, { 6, 0 } };
static const G3DSceneAnimationSetup sModelBAnimations[] = { { 3, 0 }, { 4, 0 } };
static const G3DSceneAnimationSetup sMovingAnimations[] = { { 7, 0 }, { 8, 0 } };

static const G3DSceneActorSetup sActors[6] = {
    { 1, 0, 1, 0, sModelAAnimations, NELEMS(sModelAAnimations) },
    { 0, 0, 0, 0, sModelBAnimations, NELEMS(sModelBAnimations) },
    { 1, 0, 1, 0, sModelAAnimations, NELEMS(sModelAAnimations) },
    { 2, 0, 2, 0, sMovingAnimations, NELEMS(sMovingAnimations) },
    { 2, 0, 2, 0, sMovingAnimations, NELEMS(sMovingAnimations) },
    { 2, 0, 2, 0, sMovingAnimations, NELEMS(sMovingAnimations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

static const VecFx32 sFirstPositions[SWITCH_COUNT] = {
    { FX32_CONST(296), FX32_CONST(0), FX32_CONST(872) },
    { FX32_CONST(888), FX32_CONST(48), FX32_CONST(696) },
    { FX32_CONST(280), FX32_CONST(96), FX32_CONST(536) },
};

static const VecFx32 sSecondPositions[SWITCH_COUNT] = {
    { FX32_CONST(376), FX32_CONST(0), FX32_CONST(856) },
    { FX32_CONST(952), FX32_CONST(48), FX32_CONST(664) },
    { FX32_CONST(248), FX32_CONST(96), FX32_CONST(472) },
};

static const PlacePrecondition sPlacePreconditions[PLACE_COUNT] = {
    { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 }, { 2, 0 }, { 2, 0 }, { 2, 1 }, { 2, 1 },
};

void GymIcirrus_Init(Field *field) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    IcirrusGymState *state = GimmickState_GetUserData(
        GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), GIMMICK_STATE_ID);
    int i;
    int j;
    int model;
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;

    Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(IcirrusGymWork));
    LoadFieldExpandObjData(expObj, &sSceneSetup, 0);
    for (i = 0; i < SWITCH_COUNT; i++) {
        matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, i);
        matrix->translation = sFirstPositions[i];
        FieldExpObj_SetActorCulling(expObj, 0, i, 1);
        for (j = 0; j < 2; j++) {
            anm = FieldExpObj_GetAnmInfo(expObj, 0, i, j);
            FieldExpObjAnm_SetLooped(anm, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
        }
        model = i + SECOND_MODEL_OFFSET;
        matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
        matrix->translation = sSecondPositions[i];
        FieldExpObj_SetActorCulling(expObj, 0, model, 1);
        for (j = 0; j < 2; j++) {
            anm = FieldExpObj_GetAnmInfo(expObj, 0, model, j);
            FieldExpObjAnm_SetLooped(anm, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
        }
        GymIcirrus_ShowSwitchState(expObj, state->flipped[i], i, TRUE);
        GymIcirrus_ShowSwitchState(expObj, state->flipped[i], i, FALSE);
    }
}

void GymIcirrus_End(Field *field) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);

    Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
    FieldExpObj_FreeScene(expObj, 0);
}

void GymIcirrus_Update(Field *field) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);

    Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), GIMMICK_STATE_ID);
    FieldExpObj_StepAllAnimations(expObj);
}

GameEvent *GymIcirrus_CreateSwitchAnimEvent(GameSystem *gsys, int switchIndex) {
    Field *field = GSYS_GetField(gsys);
    IcirrusGymWork *work;

    GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    if (switchIndex >= SWITCH_COUNT) {
        return NULL;
    }
    work->switchIndex = switchIndex;
    return GameEvent_Create(gsys, NULL, GymIcirrus_SwitchAnimEvent, 0);
}

GameEvent *GymIcirrus_CreateFlipSwitchEvent(GameSystem *gsys, int switchIndex) {
    Field *field = GSYS_GetField(gsys);
    IcirrusGymWork *work;

    GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    if (switchIndex >= SWITCH_COUNT) {
        return NULL;
    }
    work->switchIndex = switchIndex;
    return GameEvent_Create(gsys, NULL, GymIcirrus_FlipSwitchEvent, 0);
}

void GymIcirrus_ToggleMovementAtPlace(GameSystem *gsys, int place) {
    Field *field = GSYS_GetField(gsys);
    u32 controllerType = Field_GetControllerTypeID(field);
    IcirrusGymState *state = GimmickState_GetUserData(
        GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), GIMMICK_STATE_ID);

    if (place < PLACE_COUNT && controllerType == CONTROLLER_TYPE_HYBRID) {
        if (GymIcirrus_IsPlaceSwitchReady(state, place)) {
            func_ov036_0219e434(&Field_GetController(field)->hybrid, field);
        }
    }
}

// Whether the switch of the place is in the state that the place calls for
static BOOL GymIcirrus_IsPlaceSwitchReady(IcirrusGymState *state, int place) {
    u16 flipped = state->flipped[sPlacePreconditions[place].switchIndex] != 0 ? TRUE : FALSE;

    if (flipped == sPlacePreconditions[place].flipped) {
        return TRUE;
    }
    return FALSE;
}

// Sets the switch's first or second model to the end of the animation of its state, paused
static void GymIcirrus_ShowSwitchState(FieldExpObjSystem *expObj, BOOL flipped, int switchIndex, int second) {
    int playing;
    int other;
    int model = switchIndex;

    if (flipped != FALSE) {
        playing = 1;
        other = 0;
    } else {
        playing = 0;
        other = 1;
    }
    if (second != TRUE) {
        model += SECOND_MODEL_OFFSET;
    }
    FieldExpObj_SetAnm(expObj, 0, model, playing, TRUE);
    FieldExpObj_SetAnm(expObj, 0, model, other, FALSE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, playing, 0);
    FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(expObj, 0, model, playing), TRUE);
}

static BOOL GymIcirrus_IsSwitchFlipped(IcirrusGymState *state, int switchIndex) {
    if (state->flipped[switchIndex] != 0) {
        return TRUE;
    }
    return FALSE;
}

static GameEventReturnCode GymIcirrus_SwitchAnimEvent(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    IcirrusGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    IcirrusGymState *gymState = GimmickState_GetUserData(
        GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), GIMMICK_STATE_ID);
    u32 switchIndex;
    BOOL flipped = TRUE;

    switch (*state) {
    case 0:
        switchIndex = work->switchIndex;
        FieldExpObjAnm_SetPaused(
            GetAnimation(expObj, switchIndex + SECOND_MODEL_OFFSET, GymIcirrus_IsSwitchFlipped(gymState, switchIndex)),
            FALSE);
        (*state)++;
        break;
    case 1:
        switchIndex = work->switchIndex;
        if (FieldExpObjAnm_IsPlaybackFinished(GetAnimation(expObj, switchIndex + SECOND_MODEL_OFFSET,
                                                           GymIcirrus_IsSwitchFlipped(gymState, switchIndex)))) {
            if (gymState->flipped[work->switchIndex] != 0) {
                flipped = FALSE;
            }
            GymIcirrus_ShowSwitchState(expObj, flipped, work->switchIndex, FALSE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static GameEventReturnCode GymIcirrus_FlipSwitchEvent(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    IcirrusGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    IcirrusGymState *gymState = GimmickState_GetUserData(
        GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), GIMMICK_STATE_ID);
    u32 switchIndex;
    BOOL flipped = TRUE;
    BOOL newFlipped;

    switch (*state) {
    case 0:
        switchIndex = work->switchIndex;
        FieldExpObjAnm_SetPaused(GetAnimation(expObj, switchIndex, GymIcirrus_IsSwitchFlipped(gymState, switchIndex)),
                                 FALSE);
        GFL_SndSEPlay(SEQ_SE_FLD_65);
        (*state)++;
        break;
    case 1:
        switchIndex = work->switchIndex;
        if (FieldExpObjAnm_IsPlaybackFinished(
                GetAnimation(expObj, switchIndex, GymIcirrus_IsSwitchFlipped(gymState, switchIndex)))) {
            GFL_SndStop();
            GFL_SndSEPlay(SEQ_SE_FLD_95);
            if (gymState->flipped[work->switchIndex] != 0) {
                flipped = FALSE;
            }
            GymIcirrus_ShowSwitchState(expObj, flipped, work->switchIndex, TRUE);
            newFlipped = TRUE;
            if (gymState->flipped[work->switchIndex] != 0) {
                newFlipped = FALSE;
            }
            gymState->flipped[work->switchIndex] = newFlipped;
            return TRUE;
        }
        break;
    }
    return FALSE;
}
