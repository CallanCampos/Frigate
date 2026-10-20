// The Tubeline Bridge's gimmick (zone 254, gimmick 11): two trains cross the bridge as 3D sounds, each moving along a
// motion curve and waiting a random time between crossings. The ROM embeds no name for this file, so its name is a
// guess. Archive 128 holds the bridge's data: each train's sound range and volume, its curve and its wait range
#include "types.h"
#include "field/field.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/field_map.h"
#include "field/gimmick_tubeline_bridge.h"
#include "field/sound_obj.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/iss_3ds_sys.h"
#include "system/iss_sys.h"

#define GIMMICK_STATE_ID 11
#define ARCID_TUBELINE_BRIDGE 128
#define TRAIN_COUNT 2

// A train's sound, from files 1 and 2 of archive 128
typedef struct {
    s32 range;
    s32 volume;
} TrainSoundParams;

// How long a train waits between crossings, from files 5 and 6 of archive 128
typedef struct {
    s32 min;
    s32 max;
} TrainWaitRange;

typedef struct {
    u16 heapId;
    ISS3DSoundSys *soundSys;
    FieldSoundEmitter *emitters[TRAIN_COUNT];
    s32 waits[TRAIN_COUNT];
    s32 waitMin[TRAIN_COUNT];
    s32 waitMax[TRAIN_COUNT];
} GimmickWork;

static void TubelineBridge_Init(GimmickWork *work, Field *field);
static void TubelineBridge_LoadWaitRanges(GimmickWork *work);
static void TubelineBridge_SaveFrames(GimmickWork *work, Field *field);
static void TubelineBridge_LoadFrames(GimmickWork *work, Field *field);
static void TubelineBridge_StartWait(GimmickWork *work, s32 train);
static void TubelineBridge_RestartTrain(GimmickWork *work, s32 train);

static const G3DSceneResourceSetup sResources[] = {
    { ARCID_TUBELINE_BRIDGE, 0, 0 },
};

static const G3DSceneActorSetup sActors[TRAIN_COUNT] = {
    { 0, 0, 0, 0, NULL, 0 },
    { 0, 0, 0, 0, NULL, 0 },
};

static G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void TubelineBridge_Setup(Field *field) {
    u16 heapId;
    GimmickWork *work;

    heapId = Field_GetHeapID(field);
    FieldExpObj_AddScene(Field_GetExpObjSystem(field), &sSceneSetup, 0);
    work = Field_AllocGimmickWorkBlock(field, 0, heapId, sizeof(GimmickWork));
    TubelineBridge_Init(work, field);
    TubelineBridge_LoadFrames(work, field);
    ISS3DSoundSys_ReqChangeMasterVolume(work->soundSys, 127);
}

void TubelineBridge_End(Field *field) {
    s32 i;
    GimmickWork *work;

    work = Field_GetGimmickWorkBlock(field, 0);
    ISS3DSoundSys_ReqChangeMasterVolume(work->soundSys, 0);
    TubelineBridge_SaveFrames(work, field);
    for (i = 0; i < TRAIN_COUNT; i++) {
        FieldSoundEmitter_Free(work->emitters[i]);
    }
    Field_DeleteGimmickWorkBlock(field, 0);
}

void TubelineBridge_Update(Field *field) {
    s32 i;
    GimmickWork *work;
    G3DCamera *camera;
    VecFx32 pos;
    VecFx32 target;

    work = Field_GetGimmickWorkBlock(field, 0);
    camera = FieldCamera_GetG3DCamera(Field_GetCameraSystem(field));
    GFL_G3DCameraGetLookatPos(camera, &pos);
    GFL_G3DCameraGetLookatTarget(camera, &target);
    ISS3DSoundSys_SetListener(work->soundSys, &pos, &target);
    for (i = 0; i < TRAIN_COUNT; i++) {
        if (work->waits[i] > 0) {
            if (--work->waits[i] <= 0) {
                TubelineBridge_RestartTrain(work, i);
            }
        } else if (FieldSoundEmitter_StepTransRot(work->emitters[i], FX32_ONE)) {
            TubelineBridge_StartWait(work, i);
        }
    }
}

static void TubelineBridge_Init(GimmickWork *work, Field *field) {
    s32 i;
    u16 heapId;
    FieldExpObjSystem *system;
    TrainSoundParams *params;
    FieldSoundEmitter *emitter;

    heapId = Field_GetHeapID(field);
    system = Field_GetExpObjSystem(field);
    work->heapId = heapId;
    work->soundSys = ISS_Get3DSoundSys(GameSystem_GetISS(Field_GetGameSystem(field)));

    params = GFL_ArcSysReadHeapNew(ARCID_TUBELINE_BRIDGE, 1, heapId);
    emitter = FieldSoundEmitter_CreateAndSet(field, FieldExpObj_GetActorMatrixPtr(system, 0, 0), 8,
                                             params->range * FX32_ONE, params->volume);
    FieldSoundEmitter_BindMotionCurve(emitter, ARCID_TUBELINE_BRIDGE, 3, 10);
    work->emitters[0] = emitter;
    GFL_HeapFree(params);

    params = GFL_ArcSysReadHeapNew(ARCID_TUBELINE_BRIDGE, 2, heapId);
    emitter = FieldSoundEmitter_CreateAndSet(field, FieldExpObj_GetActorMatrixPtr(system, 0, 1), 9,
                                             params->range * FX32_ONE, params->volume);
    FieldSoundEmitter_BindMotionCurve(emitter, ARCID_TUBELINE_BRIDGE, 4, 10);
    work->emitters[1] = emitter;
    GFL_HeapFree(params);

    for (i = 0; i < TRAIN_COUNT; i++) {
        work->waits[i] = 0;
    }
    TubelineBridge_LoadWaitRanges(work);
}

static void TubelineBridge_LoadWaitRanges(GimmickWork *work) {
    u32 fileIds[TRAIN_COUNT] = { 5, 6 };
    s32 i;
    TrainWaitRange *range;

    for (i = 0; i < TRAIN_COUNT; i++) {
        range = GFL_ArcSysReadHeapNew(ARCID_TUBELINE_BRIDGE, fileIds[i], work->heapId);
        work->waitMin[i] = range->min;
        work->waitMax[i] = range->max;
        GFL_HeapFree(range);
    }
}

// The gimmick state keeps each train's frame on its curve while the zone is unloaded
static void TubelineBridge_SaveFrames(GimmickWork *work, Field *field) {
    s32 i;
    s32 *frames;

    frames = GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))),
                                      GIMMICK_STATE_ID);
    for (i = 0; i < TRAIN_COUNT; i++) {
        frames[i] = FX_Whole(FieldSoundEmitter_GetAnimFrame(work->emitters[i]));
    }
}

static void TubelineBridge_LoadFrames(GimmickWork *work, Field *field) {
    s32 i;
    s32 *frames;

    frames = GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))),
                                      GIMMICK_STATE_ID);
    for (i = 0; i < TRAIN_COUNT; i++) {
        FieldSoundEmitter_SetAnimFrame(work->emitters[i], frames[i] * FX32_ONE);
    }
}

static void TubelineBridge_StartWait(GimmickWork *work, s32 train) {
    work->waits[train] = work->waitMin[train] + GFL_RandomLCAlt(work->waitMax[train] - work->waitMin[train] + 1);
}

static void TubelineBridge_RestartTrain(GimmickWork *work, s32 train) {
    FieldSoundEmitter_SetAnimFrame(work->emitters[train], 0);
}
