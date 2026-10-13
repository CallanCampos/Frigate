#include "types.h"
#include "constants/sound.h"
#include "constants/trainers.h"
#include "constants/vars.h"
#include "constants/zones.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/field_fog.h"
#include "field/field_g3d_mapper.h"
#include "field/field_map.h"
#include "field/field_task.h"
#include "field/gym_opelucid.h"
#include "field/trainer_script.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The Opelucid City gym's statues, named after the zone: the overlay has no string. Two giant dragon statues carry the
// player up to the Gym Leader. The gym has four tiers and three columns; the player stands in one, and each move of the
// statues goes up or down a tier, or to a side, along curves that carry the player and the Veterans. Which of the
// statue's parts the scripts open depends on the stage, which the tier, column and whether the tier's Veterans are
// beaten pick. The names are ours

#define GIMMICK_WORK_ID 1
#define GIMMICK_STATE_ID 10
// The archive of the gym's models and curves
#define GIMMICK_ARCID 0xa0

#define OBJECT_COUNT 5
#define MOVER_COUNT 3
#define TIER_COUNT 4
#define COLUMN_COUNT 3
#define DIRECTION_COUNT 4
#define PLATE_COUNT 12

// The statue models' animations: 4 plays while the statues move, 5 holds the pose of the state
#define ANM_MOVE 4
#define ANM_STATE 5

// The tiers of the gym's state, from the entrance up to the Gym Leader
typedef enum {
    TIER_0,
    TIER_1,
    TIER_2,
    TIER_3,
} OpelucidGymTier;

// The columns of the gym's state: the player's side, or the center
typedef enum {
    COLUMN_LEFT,
    COLUMN_RIGHT,
    COLUMN_CENTER,
} OpelucidGymColumn;

// The directions of a move
#define DIRECTION_UP 0
#define DIRECTION_DOWN 1
#define DIRECTION_LEFT 2
#define DIRECTION_RIGHT 3

// What the save keeps of the gym: where the player is, and the frames that the models were left at
typedef struct {
    u8 column;
    u8 tier;
    fx32 frames[OBJECT_COUNT];
} OpelucidGymState;

// A model of the gym, as an actor of the first scene
typedef struct {
    u32 scene;
    u32 actor;
    u32 unk8;
    FieldExpObjSystem *expObj;
} OpelucidGymObject;

typedef struct {
    u16 parentHeapId;
    u16 heapId;
    FieldExpObjSystem *expObj;
    Field *field;
    EventWork *eventWork;
    OpelucidGymState *state;
    OpelucidGymObject objects[OBJECT_COUNT];
    BOOL heapCreated;
} OpelucidGymWork;

// A curve that carries a model and an actor: which file, and the frames to play from and to
typedef struct {
    u8 file;
    u32 start;
    u32 end;
} OpelucidGymCurve;

// What one move does: the curve of the player and the statue it rides, and the curves of the Veterans on the other
// statue, with the statue's model and the actors
typedef struct {
    u16 playerCurve;
    u16 object;
    u16 curve1;
    u16 actorId1;
    u16 curve2;
    u16 actorId2;
    BOOL hasMover1;
    BOOL hasMover2;
    BOOL playsEnd;
} OpelucidGymStep;

// Something that follows a curve: a model, and the actor that rides it
typedef struct {
    u8 object;
    FieldActor *actor;
    s32 frame;
    u32 frameCount;
    G3DCurve *curve;
    OpelucidGymObject *obj;
    const OpelucidGymCurve *entry;
    BOOL done;
    BOOL reverse;
} OpelucidGymMover;

typedef struct {
    OpelucidGymWork *work;
    Field *field;
    EventWork *eventWork;
    MMSys *mmSys;
    FieldExpObjSystem *expObj;
    u32 unk14;
    FieldPlayer *player;
    u16 direction;
    const OpelucidGymStep *step;
    OpelucidGymState *state;
    u32 frame;
    s32 soundStage;
    u32 cameraDefaults;
    OpelucidGymMover movers[MOVER_COUNT];
    void *cameraBind;
} OpelucidGymMoveEvent;

typedef struct {
    OpelucidGymWork *work;
    EventWork *eventWork;
    u16 button;
    u32 frame;
} OpelucidGymButtonEvent;

static void GymOpelucid_InitObjects(OpelucidGymWork *work);
static void GymOpelucid_StartMoveAnimations(OpelucidGymWork *work);
static void GymOpelucid_ShowState(OpelucidGymWork *work);
static void GymOpelucid_PlacePlates(Field *field);
static u32 GymOpelucid_GetStage(EventWork *eventWork, int tier, int column);
static BOOL GymOpelucid_IsTierCleared(EventWork *eventWork, int tier);
static BOOL GymOpelucid_IsTier0Cleared(EventWork *eventWork);
static BOOL GymOpelucid_IsTier1Cleared(EventWork *eventWork);
static BOOL GymOpelucid_IsTier2Cleared(EventWork *eventWork);
static BOOL GymOpelucid_IsTier3Cleared(EventWork *eventWork);
static void GymOpelucid_SetWork(EventWork *eventWork, u16 workId, u16 value);
static void GymOpelucid_SetVarsOfStage(int stage, EventWork *eventWork);
static GameEvent *GymOpelucid_CreateEvent(GameSystem *gsys, u16 direction);
static GameEventReturnCode GymOpelucid_MoveEventCallback(GameEvent *event, u32 *state, void *data);
static void GymOpelucid_StartMove(OpelucidGymMoveEvent *ev);
static void GymOpelucid_EndMove(OpelucidGymMoveEvent *ev);
static void GymOpelucid_UpdatePlayerMover(OpelucidGymMoveEvent *ev);
static void GymOpelucid_PlayMoveSounds(OpelucidGymMoveEvent *ev);
static void GymOpelucid_UpdateCamera(OpelucidGymMoveEvent *ev);
static int GymOpelucid_GetMoverLength(OpelucidGymMover *mover);
static void GymOpelucid_ShakeCamera(Field *field);
static void GymOpelucid_PlayEndSounds(OpelucidGymMoveEvent *ev);
static void GymOpelucid_AdvanceMover(OpelucidGymMover *mover);
static void GymOpelucid_UpdateOtherMovers(OpelucidGymMoveEvent *ev);
static void GymOpelucid_UpdateActorMover(OpelucidGymMover *mover);
static void GymOpelucid_MoveState(OpelucidGymMoveEvent *ev);
static GameEventReturnCode GymOpelucid_ButtonEventCallback(GameEvent *event, u32 *state, void *data);

static const G3DSceneAnimationSetup sPlatformAnimations[] = { { 0xb, 0 } };
static const G3DSceneAnimationSetup sDragonDAnimations[] = { { 0xe, 0 }, { 0x11, 0 }, { 0x14, 0 } };
static const G3DSceneAnimationSetup sDragonCAnimations[] = { { 0xd, 0 }, { 0x10, 0 }, { 0x13, 0 } };
static const G3DSceneAnimationSetup sDragonBAnimations[] = { { 0xc, 0 }, { 0xf, 0 }, { 0x12, 0 } };
static const G3DSceneAnimationSetup sStatueAnimations[] = {
    { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 }, { 5, 0 }, { 6, 0 },
};

static const u32 sCurveFiles[] = { 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a };

static const u8 sFogTable[32] = {
    0x00, 0x04, 0x08, 0x0c, 0x18, 0x19, 0x16, 0x1c, 0x21, 0x14, 0x16, 0x1d, 0x13, 0x16, 0x39, 0x3d,
    0x42, 0x46, 0x4a, 0x4e, 0x52, 0x56, 0x5a, 0x5e, 0x63, 0x67, 0x6b, 0x6f, 0x73, 0x77, 0x7b, 0x7f,
};

// Where each model is, added to the position of its curve
static const VecFx32 sObjectOffsets[OBJECT_COUNT] = {
    { FX32_CONST(256), FX32_CONST(0), FX32_CONST(768) }, { FX32_CONST(256), FX32_CONST(0), FX32_CONST(256) },
    { FX32_CONST(256), FX32_CONST(0), FX32_CONST(256) }, { FX32_CONST(256), FX32_CONST(0), FX32_CONST(256) },
    { FX32_CONST(256), FX32_CONST(0), FX32_CONST(256) },
};

static const G3DSceneActorSetup sGymOpelucidActors[OBJECT_COUNT] = {
    { 0, 0, 0, 0, sStatueAnimations, NELEMS(sStatueAnimations) },
    { 7, 0, 7, 0, sPlatformAnimations, NELEMS(sPlatformAnimations) },
    { 8, 0, 8, 0, sDragonBAnimations, NELEMS(sDragonBAnimations) },
    { 9, 0, 9, 0, sDragonCAnimations, NELEMS(sDragonCAnimations) },
    { 10, 0, 10, 0, sDragonDAnimations, NELEMS(sDragonDAnimations) },
};

// The rectangles of the map that the statues' platforms cover: a position, a size and a value, which the map's list
// of areas takes
typedef struct {
    u8 x;
    u8 z;
    u8 width;
    u8 depth;
    u32 value;
} OpelucidGymPlate;

static const OpelucidGymPlate sPlates[PLATE_COUNT] = {
    { 16, 23, 3, 3, 0x2e0000 }, { 17, 26, 1, 2, 0x2e0000 }, { 17, 22, 1, 2, 0x2e0000 }, { 24, 19, 3, 3, 0x2e0000 },
    { 25, 22, 1, 2, 0x2e0000 }, { 25, 17, 1, 2, 0x2e0000 }, { 8, 19, 3, 3, 0x2e0000 },  { 9, 22, 1, 2, 0x2e0000 },
    { 9, 17, 1, 2, 0x2e0000 },  { 14, 19, 3, 3, 0x390000 }, { 15, 22, 1, 2, 0x390000 }, { 15, 17, 1, 2, 0x390000 },
};

// The stage the statues are at, by tier, column and whether the tier is cleared
static const u32 sStages[TIER_COUNT][COLUMN_COUNT][2] = {
    { { 0, 0 }, { 0, 0 }, { 0, 8 } },
    { { 6, 6 }, { 5, 5 }, { 2, 1 } },
    { { 6, 6 }, { 5, 5 }, { 2, 1 } },
    { { 0, 0 }, { 0, 0 }, { 7, 7 } },
};

static const G3DSceneResourceSetup sGymOpelucidResources[21] = {
    { GIMMICK_ARCID, 0xe, 0 },  { GIMMICK_ARCID, 0xf, 0 },  { GIMMICK_ARCID, 0x10, 0 }, { GIMMICK_ARCID, 0x11, 0 },
    { GIMMICK_ARCID, 0x13, 0 }, { GIMMICK_ARCID, 0x12, 0 }, { GIMMICK_ARCID, 0x14, 0 }, { GIMMICK_ARCID, 0, 0 },
    { GIMMICK_ARCID, 1, 0 },    { GIMMICK_ARCID, 2, 0 },    { GIMMICK_ARCID, 3, 0 },    { GIMMICK_ARCID, 4, 0 },
    { GIMMICK_ARCID, 5, 0 },    { GIMMICK_ARCID, 6, 0 },    { GIMMICK_ARCID, 7, 0 },    { GIMMICK_ARCID, 8, 0 },
    { GIMMICK_ARCID, 9, 0 },    { GIMMICK_ARCID, 10, 0 },   { GIMMICK_ARCID, 11, 0 },   { GIMMICK_ARCID, 12, 0 },
    { GIMMICK_ARCID, 13, 0 },
};

static const G3DSceneSetup sGymOpelucidSceneSetup = { sGymOpelucidResources, NELEMS(sGymOpelucidResources), sGymOpelucidActors, NELEMS(sGymOpelucidActors) };
static const OpelucidGymCurve sCurves[32] = {
    { 0, 0, 179 }, { 0, 299, 359 }, { 0, 199, 259 }, { 0, 259, 289 },
    { 0, 359, 389 }, { 0, 179, 0 }, { 0, 399, 579 }, { 0, 599, 659 },
    { 0, 699, 759 }, { 0, 659, 689 }, { 0, 759, 789 }, { 0, 579, 399 },
    { 0, 799, 979 }, { 0, 980, 1099 }, { 1, 199, 259 }, { 2, 199, 259 },
    { 1, 259, 289 }, { 2, 259, 289 }, { 2, 299, 359 }, { 1, 299, 359 },
    { 2, 359, 389 }, { 1, 359, 389 }, { 3, 599, 659 }, { 4, 599, 659 },
    { 3, 659, 689 }, { 4, 659, 689 }, { 4, 699, 759 }, { 3, 699, 759 },
    { 4, 759, 789 }, { 3, 759, 789 }, { 5, 799, 979 }, { 5, 980, 1019 },
};

// What each move does, by tier, column and direction
static const OpelucidGymStep sGymOpelucidSteps[TIER_COUNT][COLUMN_COUNT][DIRECTION_COUNT] = {
    {
        {
            { 0 },
            { 0 },
            { 0 },
            { 0 },
        },
        {
            { 0 },
            { 0 },
            { 0 },
            { 0 },
        },
        {
            { 0 },
            { 0 },
            { 0 },
            { 0 },
        },
    },
    {
        {
            { 3, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 3, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 3, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 3, 2, 16, 2, 17, 3, 1, 1, 0 },
        },
        {
            { 4, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 4, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 4, 2, 20, 3, 21, 2, 1, 1, 0 },
            { 4, 0, 0, 0, 0, 0, 0, 0, 0 },
        },
        {
            { 6, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 5, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 2, 2, 14, 2, 15, 3, 1, 1, 1 },
            { 1, 2, 18, 3, 19, 2, 1, 1, 1 },
        },
    },
    {
        {
            { 9, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 9, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 9, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 9, 3, 24, 5, 25, 4, 1, 1, 0 },
        },
        {
            { 10, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 10, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 10, 3, 28, 4, 29, 5, 1, 1, 0 },
            { 10, 0, 0, 0, 0, 0, 0, 0, 0 },
        },
        {
            { 12, 4, 30, 6, 0, 0, 1, 0, 1 },
            { 11, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 7, 3, 22, 5, 23, 4, 1, 1, 1 },
            { 8, 3, 26, 4, 27, 5, 1, 1, 1 },
        },
    },
    {
        {
            { 13, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 13, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 13, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 13, 0, 0, 0, 0, 0, 0, 0, 0 },
        },
        {
            { 13, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 13, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 13, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 13, 0, 0, 0, 0, 0, 0, 0, 0 },
        },
        {
            { 13, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 13, 4, 31, 6, 0, 0, 1, 0, 0 },
            { 13, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 13, 0, 0, 0, 0, 0, 0, 0, 0 },
        },
    },
};

// Whether the Veterans of a tier are beaten, and the statue has been told so
static BOOL (*sTierChecks[TIER_COUNT])(EventWork *eventWork) = {
    GymOpelucid_IsTier0Cleared,
    GymOpelucid_IsTier1Cleared,
    GymOpelucid_IsTier2Cleared,
    GymOpelucid_IsTier3Cleared,
};

void GymOpelucid_Init(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    GimmickState *gimmickState = GameData_GetGimmickState(gameData);
    OpelucidGymWork *work = Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(OpelucidGymWork));
    FieldFog *fog;

    GFL_HeapCreateChild(HEAPID_FIELDMAP, HEAPID_GYM_OPELUCID, 0x30000);
    work->heapCreated = TRUE;
    work->parentHeapId = HEAPID_FIELDMAP;
    work->heapId = HEAPID_GYM_OPELUCID;
    work->field = field;
    work->state = GimmickState_GetUserData(gimmickState, GIMMICK_STATE_ID);
    work->expObj = Field_GetExpObjSystem(field);
    work->eventWork = GameData_GetEventWork(gameData);
    LoadFieldExpandObjData(work->expObj, &sGymOpelucidSceneSetup, 0);
    GymOpelucid_PlacePlates(work->field);
    GymOpelucid_InitObjects(work);
    fog = Field_GetFog(work->field);
    FieldFog_SetAlphaMode(fog, 0);
    FieldFog_SetAlpha(fog, 30);
    FieldFog_SetTable(fog, sFogTable);
    FieldFog_SetDepthShift(fog, 7);
    FieldFog_SetOffset(fog, 0x7f5f);
    FieldFog_SetEnabled(fog, TRUE);
    FieldFog_Flush(fog);
    GymOpelucid_ShowState(work);
}

static void GymOpelucid_InitObjects(OpelucidGymWork *work) {
    int i;
    int j;
    OpelucidGymObject *obj;
    OpelucidGymObject *objects = work->objects;

    for (i = 0; i < OBJECT_COUNT; i++) {
        objects[i].scene = 0;
        objects[i].actor = i;
        objects[i].expObj = work->expObj;
        FieldExpObj_SetActorHidden(objects[i].expObj, objects[i].scene, objects[i].actor, FALSE);
    }
    for (i = 0; i < OBJECT_COUNT; i++) {
        SRTMatrix *matrix;
        obj = &work->objects[i];
        matrix = FieldExpObj_GetActorMatrixPtr(obj->expObj, 0, obj->actor);
        matrix->translation = sObjectOffsets[i];
        FieldExpObj_SetActorHidden(obj->expObj, 0, obj->actor, FALSE);
        for (j = 0; j < sGymOpelucidActors[i].animationCount; j++) {
            FieldExpObjAnm *anm;
            FieldExpObj_SetAnm(obj->expObj, 0, obj->actor, j, TRUE);
            FieldExpObj_SetAnmFrame(obj->expObj, 0, obj->actor, j, work->state->frames[obj->actor]);
            anm = FieldExpObj_GetAnmInfo(obj->expObj, 0, obj->actor, j);
            FieldExpObjAnm_SetPaused(anm, TRUE);
            FieldExpObjAnm_SetLooped(anm, FALSE);
        }
    }
}

// The first statue plays its moving animation and not the one that holds the pose
static void GymOpelucid_StartMoveAnimations(OpelucidGymWork *work) {
    OpelucidGymObject *obj = &work->objects[0];

    FieldExpObj_SetAnm(obj->expObj, 0, obj->actor, ANM_MOVE, TRUE);
    FieldExpObj_SetAnm(obj->expObj, 0, obj->actor, ANM_STATE, FALSE);
}

// The first statue holds the pose of the stage that the state calls for
static void GymOpelucid_ShowState(OpelucidGymWork *work) {
    OpelucidGymObject *obj = &work->objects[0];
    OpelucidGymTier tier;
    OpelucidGymColumn column;
    int stage;

    FieldExpObj_SetAnm(obj->expObj, 0, obj->actor, ANM_MOVE, FALSE);
    FieldExpObj_SetAnm(obj->expObj, 0, obj->actor, ANM_STATE, TRUE);
    tier = work->state->tier;
    column = work->state->column;
    stage = GymOpelucid_GetStage(work->eventWork, tier, column);
    FieldExpObj_SetAnmFrame(obj->expObj, 0, obj->actor, ANM_STATE, FX32_CONST(stage));
}

static void GymOpelucid_PlacePlates(Field *field) {
    u32 i;
    void *list = func_ov036_02184590(Field_GetG3DMapper(field));

    for (i = 0; i < PLATE_COUNT; i++) {
        func_ov036_021ba624(i, sPlates[i].x, sPlates[i].z, sPlates[i].width, sPlates[i].depth, sPlates[i].value, 0x800000, list);
    }
}

void GymOpelucid_End(Field *field) {
    OpelucidGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    FieldExpObj_FreeScene(work->expObj, 0);
    if (work->heapCreated == TRUE) {
        GFL_HeapDelete(work->heapId);
    }
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void GymOpelucid_Update(Field *field) {
    OpelucidGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    FieldExpObj_StepAllAnimations(work->expObj);
}

static u32 GymOpelucid_GetStage(EventWork *eventWork, int tier, int column) {
    return sStages[tier][column][GymOpelucid_IsTierCleared(eventWork, tier)];
}

static BOOL GymOpelucid_IsTierCleared(EventWork *eventWork, int tier) {
    return sTierChecks[tier](eventWork);
}

static BOOL GymOpelucid_IsTier0Cleared(EventWork *eventWork) {
    u16 progress = *EventWork_GetWkPtr(eventWork, EVENT_WORK_0x40cf);

    if (TrainerFlagGet(eventWork, TRAINER_VETERAN_LUCIUS) == TRUE && progress == 2) {
        return TRUE;
    }
    return FALSE;
}

static BOOL GymOpelucid_IsTier1Cleared(EventWork *eventWork) {
    u16 progress = *EventWork_GetWkPtr(eventWork, EVENT_WORK_0x40d0);
    BOOL jerry = TrainerFlagGet(eventWork, TRAINER_VETERAN_JERRY);
    BOOL rhona = TrainerFlagGet(eventWork, TRAINER_VETERAN_RHONA);

    if ((jerry == TRUE || rhona == TRUE) && progress == 2) {
        return TRUE;
    }
    return FALSE;
}

static BOOL GymOpelucid_IsTier2Cleared(EventWork *eventWork) {
    u16 progress = *EventWork_GetWkPtr(eventWork, EVENT_WORK_0x40d1);
    BOOL ron = TrainerFlagGet(eventWork, TRAINER_VETERAN_RON);
    BOOL denae = TrainerFlagGet(eventWork, TRAINER_VETERAN_DENAE);

    if ((ron == TRUE || denae == TRUE) && progress == 2) {
        return TRUE;
    }
    return FALSE;
}

static BOOL GymOpelucid_IsTier3Cleared(EventWork *eventWork) {
    return FALSE;
}

static void GymOpelucid_SetWork(EventWork *eventWork, u16 workId, u16 value) {
    *EventWork_GetWkPtr(eventWork, workId) = value;
}

// Sets the four variables that say which of the statue's parts the scripts must open, for a stage
static void GymOpelucid_SetVarsOfStage(int stage, EventWork *eventWork) {
    GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4000, 0);
    GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4001, 0);
    GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4002, 0);
    GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4003, 0);
    if (stage == 1) {
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4000, 1);
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4001, 1);
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4002, 1);
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4003, 1);
    } else if (stage == 2) {
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4001, 1);
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4002, 1);
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4003, 1);
    } else if (stage == 3) {
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4000, 1);
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4001, 1);
    } else if (stage == 4) {
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4002, 1);
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4003, 1);
    } else if (stage == 5) {
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4002, 1);
    } else if (stage == 6) {
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4003, 1);
    } else if (stage == 7) {
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4001, 1);
    } else if (stage == 8) {
        GymOpelucid_SetWork(eventWork, EVENT_WORK_0x4000, 1);
    }
}

GameEvent *GymOpelucid_CreateMoveEvent(GameSystem *gsys, u16 direction) {
    switch (direction) {
    case DIRECTION_UP:
    case DIRECTION_DOWN:
    case DIRECTION_LEFT:
    case DIRECTION_RIGHT:
        return GymOpelucid_CreateEvent(gsys, direction);
    }
    return NULL;
}

void GymOpelucid_RefreshVars(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    OpelucidGymState *state = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_STATE_ID);
    OpelucidGymTier tier = state->tier;
    OpelucidGymColumn column = state->column;
    int stage = GymOpelucid_GetStage(eventWork, tier, column);

    GymOpelucid_SetVarsOfStage(stage, eventWork);
}

static GameEvent *GymOpelucid_CreateEvent(GameSystem *gsys, u16 direction) {
    Field *field = GSYS_GetField(gsys);
    OpelucidGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    GameEvent *event = GameEvent_Create(gsys, NULL, GymOpelucid_MoveEventCallback, sizeof(OpelucidGymMoveEvent));
    OpelucidGymMoveEvent *ev = GameEvent_GetData(event);
    GameData *gameData = GSYS_GetGameData(gsys);
    GimmickState *gimmickState = GameData_GetGimmickState(gameData);
    OpelucidGymTier tier = work->state->tier;
    OpelucidGymColumn column = work->state->column;

    ev->step = &sGymOpelucidSteps[tier][column][direction];
    ev->direction = direction;
    ev->work = work;
    ev->field = field;
    ev->state = GimmickState_GetUserData(gimmickState, GIMMICK_STATE_ID);
    ev->eventWork = GameData_GetEventWork(gameData);
    return event;
}

GameEvent *GymOpelucid_CreateButtonEvent(GameSystem *gsys, u16 button) {
    Field *field = GSYS_GetField(gsys);
    OpelucidGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    GameEvent *event = GameEvent_Create(gsys, NULL, GymOpelucid_ButtonEventCallback, sizeof(OpelucidGymButtonEvent));
    OpelucidGymButtonEvent *ev = GameEvent_GetData(event);
    GameData *gameData = GSYS_GetGameData(gsys);

    GameData_GetGimmickState(gameData);
    ev->button = button;
    ev->work = work;
    ev->eventWork = GameData_GetEventWork(gameData);
    return event;
}

static GameEventReturnCode GymOpelucid_MoveEventCallback(GameEvent *event, u32 *state, void *data) {
    OpelucidGymMoveEvent *ev = data;
    int i;

    switch (*state) {
    case 0:
        GymOpelucid_StartMove(ev);
        *state = 1;
    case 1:
        ev->frame++;
        GymOpelucid_PlayMoveSounds(ev);
        GymOpelucid_UpdatePlayerMover(ev);
        GymOpelucid_UpdateOtherMovers(ev);
        GymOpelucid_UpdateCamera(ev);
        GymOpelucid_PlayEndSounds(ev);
        for (i = 0; i < MOVER_COUNT; i++) {
            if (!ev->movers[i].done) {
                return FALSE;
            }
        }
        if (GFL_SndPlayerIsActiveAny() == TRUE) {
            return FALSE;
        }
        *state = 2;
        break;
    case 2: {
        u16 tier1 = *EventWork_GetWkPtr(ev->eventWork, EVENT_WORK_0x40d0);
        u16 tier2 = *EventWork_GetWkPtr(ev->eventWork, EVENT_WORK_0x40d1);
        GameSystem *gsys = Field_GetGameSystem(ev->field);

        if (tier1 == 1) {
            GameEvent_ChainNext(event, GymOpelucid_CreateButtonEvent(gsys, 1));
            *state = 3;
            break;
        }
        if (tier2 == 1) {
            GameEvent_ChainNext(event, GymOpelucid_CreateButtonEvent(gsys, 2));
            *state = 3;
            break;
        }
        GFL_SndSEPlay(SEQ_SE_SW_DRGGYM_07);
        *state = 3;
        break;
    }
    case 3:
        GymOpelucid_MoveState(ev);
        GymOpelucid_ShowState(ev->work);
        *state = 4;
        break;
    case 4:
        GymOpelucid_EndMove(ev);
        return TRUE;
    }
    return FALSE;
}

// Gives a mover its model and loads the curve it follows
static inline void GymOpelucid_LoadMoverCurve(OpelucidGymMoveEvent *ev, OpelucidGymMover *mover, u8 object, u16 curve) {
    mover->object = object;
    mover->entry = &sCurves[curve];
    mover->curve = GFL_G3DCurveLoadFileAll(ev->work->heapId, GIMMICK_ARCID, sCurveFiles[mover->entry->file]);
}

static void GymOpelucid_StartMove(OpelucidGymMoveEvent *ev) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(ev->field));
    int j;
    OpelucidGymMover *mover;
    const OpelucidGymStep *step;

    ev->player = Field_GetPlayer(ev->field);
    ev->expObj = ev->work->expObj;
    ev->mmSys = GameData_GetMMSys(gameData);
    ev->frame = 0;
    ev->soundStage = 0;
    GymOpelucid_StartMoveAnimations(ev->work);

    // The statue that carries the player
    mover = &ev->movers[0];
    GymOpelucid_LoadMoverCurve(ev, mover, 0, ev->step->playerCurve);
    mover->obj = &ev->work->objects[mover->object];
    mover->frameCount = GFL_G3DCurveGetFrameCount(mover->curve) - 1;
    mover->frame = mover->entry->start;
    mover->actor = FieldPlayer_GetActor(ev->player);
    mover->done = FALSE;
    if (mover->entry->start < mover->entry->end) {
        mover->reverse = FALSE;
        func_ov036_021b82ac(mover->obj->expObj, 0, FX32_ONE);
    } else {
        mover->reverse = TRUE;
        func_ov036_021b82ac(mover->obj->expObj, 0, -FX32_ONE);
    }
    for (j = 0; j < sGymOpelucidActors[mover->object].animationCount; j++) {
        if (j != ANM_STATE) {
            OpelucidGymObject *obj = mover->obj;
            FieldExpObjAnm *anm;
            FieldExpObj_SetAnmFrame(obj->expObj, 0, obj->actor, j, FX32_CONST(mover->frame));
            anm = FieldExpObj_GetAnmInfo(obj->expObj, 0, obj->actor, j);
            FieldExpObjAnm_SetPaused(anm, FALSE);
        }
    }

    // The Veterans' statue and the Veterans that ride it
    step = ev->step;
    if (step->hasMover1 == 1) {
        int j;

        mover = &ev->movers[1];
        GymOpelucid_LoadMoverCurve(ev, mover, step->object, step->curve1);
        mover->obj = &ev->work->objects[mover->object];
        mover->frameCount = GFL_G3DCurveGetFrameCount(mover->curve) - 1;
        mover->frame = mover->entry->start;
        mover->actor = FindFieldActor(ev->mmSys, step->actorId1);
        mover->done = FALSE;
        if (mover->entry->start < mover->entry->end) {
            mover->reverse = FALSE;
            func_ov036_021b82ac(mover->obj->expObj, 0, FX32_ONE);
        } else {
            mover->reverse = TRUE;
            func_ov036_021b82ac(mover->obj->expObj, 0, -FX32_ONE);
        }
        for (j = 0; j < sGymOpelucidActors[mover->object].animationCount; j++) {
            OpelucidGymObject *obj = mover->obj;
            FieldExpObjAnm *anm;
            FieldExpObj_SetAnmFrame(obj->expObj, 0, obj->actor, j, FX32_CONST(mover->frame));
            anm = FieldExpObj_GetAnmInfo(obj->expObj, 0, obj->actor, j);
            FieldExpObjAnm_SetPaused(anm, FALSE);
        }
    } else {
        mover = &ev->movers[1];
        mover->done = TRUE;
        mover->curve = NULL;
    }
    if (step->hasMover2 == 1) {
        mover = &ev->movers[2];
        GymOpelucid_LoadMoverCurve(ev, mover, step->object, step->curve2);
        mover->obj = NULL;
        mover->frameCount = GFL_G3DCurveGetFrameCount(mover->curve) - 1;
        mover->frame = mover->entry->start;
        mover->actor = FindFieldActor(ev->mmSys, step->actorId2);
        mover->done = FALSE;
        if (mover->entry->start < mover->entry->end) {
            mover->reverse = FALSE;
        } else {
            mover->reverse = TRUE;
        }
    } else {
        mover = &ev->movers[2];
        mover->done = TRUE;
        mover->curve = NULL;
    }
}

static void GymOpelucid_EndMove(OpelucidGymMoveEvent *ev) {
    int i;

    for (i = 0; i < OBJECT_COUNT; i++) {
        OpelucidGymObject *obj = &ev->work->objects[i];
        ev->state->frames[obj->actor] = func_ov036_021b8520(obj->expObj, 0, 0, 0);
    }
    for (i = 0; i < MOVER_COUNT; i++) {
        if (ev->movers[i].curve != NULL) {
            GFL_G3DCurveFree(ev->movers[i].curve);
        }
    }
}

static void GymOpelucid_UpdatePlayerMover(OpelucidGymMoveEvent *ev) {
    int j;
    OpelucidGymMover *mover = &ev->movers[0];
    int step;

    func_ov036_021b8520(mover->obj->expObj, 0, 0, 0);
    if (mover->done == 1) {
        for (j = 0; j < sGymOpelucidActors[mover->object].animationCount; j++) {
            FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(mover->obj->expObj, 0, mover->obj->actor, j);
            FieldExpObjAnm_SetPaused(anm, TRUE);
        }
    }
    step = mover->reverse == 0 ? 1 : -1;
    if (mover->done == 0) {
        VecFx32 pos = { 0, 0, 0 };

        if (GFL_G3DCurveGetNowTranslationLoop(mover->curve, &pos, mover->frame + step)) {
            pos.x += sObjectOffsets[mover->object].x;
            pos.y += sObjectOffsets[mover->object].y;
            pos.z += sObjectOffsets[mover->object].z;
            FieldPlayer_SetWPos(ev->player, &pos);
            GymOpelucid_AdvanceMover(mover);
        }
    }
}

static void GymOpelucid_PlayMoveSounds(OpelucidGymMoveEvent *ev) {
    if (ev->frame == 5) {
        GFL_SndSEPlay(SEQ_SE_SW_DRGGYM_01);
    }
    if (ev->frame == 10) {
        GFL_SndSEPlay(SEQ_SE_SW_DRGGYM_02);
    }
}

// The camera for the climb to the Gym Leader: it follows what the statue does, and returns after
static void GymOpelucid_UpdateCamera(OpelucidGymMoveEvent *ev) {
    int length;
    FieldCamera *camera;
    OpelucidGymMover *mover = &ev->movers[0];

    if (ev->direction == DIRECTION_UP && ev->state->tier == 2) {
        length = GymOpelucid_GetMoverLength(mover);
        if (length == ev->frame + 0x50) {
            camera = Field_GetCameraSystem(ev->field);
            ev->cameraDefaults = FieldCamera_GetDefaultsIndex(camera);
            ev->cameraBind = FieldCamera_GetBind(camera);
            FieldCamera_SetDefaultsIndex(camera, 0x2a);
        }
        if (length == ev->frame + 0x1e) {
            camera = Field_GetCameraSystem(ev->field);
            FieldCamera_SetRefBind(camera, ev->cameraBind);
            FieldCamera_EnableDelay(camera);
            FieldCamera_SetDefaultsIndex(camera, ev->cameraDefaults);
        }
        switch (ev->soundStage) {
        case 0:
        case 1:
            if (length == ev->frame + 0x46) {
                GFL_SndSEPlay(SEQ_SE_SW_DRGGYM_04);
                ev->soundStage++;
            }
            if (length == ev->frame + 0x5a) {
                GFL_SndStop();
                GFL_SndSEPlay(SEQ_SE_SW_DRGGYM_03);
                ev->soundStage++;
            }
            break;
        }
    }
}

// The length of the curve that a mover plays
static int GymOpelucid_GetMoverLength(OpelucidGymMover *mover) {
    if (mover->reverse == 0) {
        return mover->entry->end - mover->entry->start;
    }
    return mover->entry->start - mover->entry->end;
}

static void GymOpelucid_ShakeCamera(Field *field) {
    FieldCameraShakeSetup setup;
    FieldTask *task;

    sys_memset(&setup, 0, sizeof(setup));
    setup.amplitudeX = 1;
    setup.amplitudeY = 2;
    setup.period = 3;
    setup.steps = 4;
    setup.mode = 1;
    task = FieldCameraShakeTask_Create(field, &setup);
    FieldTaskManager_AddTask(Field_GetTaskManager(field), task, 0);
}

static void GymOpelucid_PlayEndSounds(OpelucidGymMoveEvent *ev) {
    const OpelucidGymStep *step = ev->step;
    int length = GymOpelucid_GetMoverLength(&ev->movers[0]);

    if (step->playsEnd == 0) {
        if (length == ev->frame + 5) {
            GFL_SndStop();
            GFL_SndSEPlay(SEQ_SE_SW_DRGGYM_03);
        }
    } else {
        if (length == ev->frame + 0xc) {
            GFL_SndStop();
            GFL_SndSEPlay(SEQ_SE_SW_DRGGYM_05);
        }
        if (length == ev->frame + 0xf) {
            if (ev->direction != DIRECTION_UP || ev->state->tier != 2) {
                GymOpelucid_ShakeCamera(ev->field);
            }
        }
    }
}

static void GymOpelucid_AdvanceMover(OpelucidGymMover *mover) {
    if (mover->reverse == 0) {
        mover->frame++;
        if (mover->entry->end > mover->frame) {
            return;
        }
        mover->done = TRUE;
    } else {
        mover->frame--;
        if (mover->entry->end < mover->frame) {
            return;
        }
        mover->done = TRUE;
    }
}

static void GymOpelucid_UpdateOtherMovers(OpelucidGymMoveEvent *ev) {
    int j;
    OpelucidGymMover *mover1 = &ev->movers[1];
    OpelucidGymMover *mover2 = &ev->movers[2];
    const OpelucidGymStep *step = ev->step;

    if (step->hasMover1 == 1) {
        if (mover1->done == 1) {
            for (j = 0; j < sGymOpelucidActors[mover1->object].animationCount; j++) {
                FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(mover1->obj->expObj, 0, mover1->obj->actor, j);
                FieldExpObjAnm_SetPaused(anm, TRUE);
            }
        }
        GymOpelucid_UpdateActorMover(mover1);
        if (step->hasMover2 == 1) {
            GymOpelucid_UpdateActorMover(mover2);
        }
    }
}

static void GymOpelucid_UpdateActorMover(OpelucidGymMover *mover) {
    int step = mover->reverse == 0 ? 1 : -1;

    if (mover->done == 0) {
        VecFx32 pos = { 0, 0, 0 };

        if (GFL_G3DCurveGetNowTranslationLoop(mover->curve, &pos, mover->frame + step)) {
            pos.x += sObjectOffsets[mover->object].x;
            pos.y += sObjectOffsets[mover->object].y;
            pos.z += sObjectOffsets[mover->object].z;
            if (mover->actor != NULL) {
                SetActorWPosValue(mover->actor, &pos);
                SetActorGPosX(mover->actor, (pos.x >> FX32_SHIFT) / 16);
                SetActorGPosY(mover->actor, (pos.y >> FX32_SHIFT) / 16);
                SetActorGPosZ(mover->actor, (pos.z >> FX32_SHIFT) / 16);
            }
            GymOpelucid_AdvanceMover(mover);
        }
    }
}

// The statues are at their new place: the column or tier of the state
static void GymOpelucid_MoveState(OpelucidGymMoveEvent *ev) {
    OpelucidGymTier tier;
    OpelucidGymColumn column;
    int stage;

    if (ev->direction == DIRECTION_UP) {
        ev->state->tier++;
    } else if (ev->direction == DIRECTION_DOWN) {
        ev->state->tier--;
    } else if (ev->direction == DIRECTION_LEFT) {
        if (ev->state->column == COLUMN_CENTER) {
            ev->state->column = COLUMN_LEFT;
        } else {
            ev->state->column = COLUMN_CENTER;
        }
    } else if (ev->direction == DIRECTION_RIGHT) {
        if (ev->state->column == COLUMN_CENTER) {
            ev->state->column = COLUMN_RIGHT;
        } else {
            ev->state->column = COLUMN_CENTER;
        }
    }
    tier = ev->state->tier;
    column = ev->state->column;
    stage = GymOpelucid_GetStage(ev->eventWork, tier, column);
    GymOpelucid_SetVarsOfStage(stage, ev->eventWork);
}

static GameEventReturnCode GymOpelucid_ButtonEventCallback(GameEvent *event, u32 *state, void *data) {
    OpelucidGymButtonEvent *ev = data;
    OpelucidGymWork *work = ev->work;
    OpelucidGymObject *obj = &work->objects[0];
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(obj->expObj, 0, obj->actor, ANM_STATE);

    switch (*state) {
    case 0: {
        fx32 frame;

        ev->frame = 0;
        GFL_SndSEPlay(SEQ_SE_SW_DRGGYM_07);
        FieldExpObj_SetAnm(obj->expObj, 0, obj->actor, ANM_MOVE, FALSE);
        FieldExpObj_SetAnm(obj->expObj, 0, obj->actor, ANM_STATE, TRUE);
        frame = FX32_CONST(9);
        if (ev->button != 0) {
            frame += FX32_CONST(20);
        }
        FieldExpObj_SetAnmFrame(obj->expObj, 0, obj->actor, ANM_STATE, frame);
        FieldExpObjAnm_SetPaused(anm, FALSE);
        *state = 1;
        break;
    }
    case 1: {
        u32 limit;

        ev->frame++;
        limit = 20;
        if (ev->button == 0) {
            limit = 16;
        }
        if (ev->frame >= limit) {
            FieldExpObjAnm_SetPaused(anm, TRUE);
            *state = 2;
        }
        break;
    }
    case 2:
        switch (ev->button) {
        case 0:
            GymOpelucid_SetWork(ev->eventWork, EVENT_WORK_0x40cf, 2);
            break;
        case 1:
            GymOpelucid_SetWork(ev->eventWork, EVENT_WORK_0x40d0, 2);
            break;
        case 2:
            GymOpelucid_SetWork(ev->eventWork, EVENT_WORK_0x40d1, 2);
            break;
        }
        GymOpelucid_ShowState(ev->work);
        return TRUE;
    }
    return FALSE;
}

void GymOpelucid_FreeHeap(GameSystem *gsys) {
    OpelucidGymWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    GFL_HeapDelete(work->heapId);
    work->heapCreated = FALSE;
}

void GymOpelucid_CreateHeap(GameSystem *gsys) {
    OpelucidGymWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    GFL_HeapCreateChild(HEAPID_FIELDMAP, work->heapId, 0x30000);
    work->heapCreated = TRUE;
}
