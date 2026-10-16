// The Skyarrow Bridge's gimmick (zone 249, gimmick 1): five models of archive 148 move along motion curves, waiting a
// random time between runs, three of them with a 3D sound and the other two following the first and the third; the
// zone's music fades in as the camera climbs; and a sound plays when the player passes a point on the way down. The
// ROM embeds no name for this file, so its name is a guess
#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/gimmick_skyarrow_bridge.h"
#include "field/sound_obj.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/math.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/iss_3ds_sys.h"
#include "system/iss_sys.h"

#define GIMMICK_STATE_ID 1
#define ARCID_SKYARROW_BRIDGE 148
#define OBJ_COUNT 5
#define EYE_HISTORY_SIZE 32

// An object's sound, from files 1, 6 and 12 of archive 148
typedef struct {
    s32 range;
    s32 volume;
} ObjSoundParams;

// How long an object waits between runs, from files 2, 4, 7, 9 and 13 of archive 148
typedef struct {
    s32 min;
    s32 max;
} ObjWaitRange;

// File 0 of archive 148: the tracks of the music to fade, and the camera heights between which it fades in
typedef struct {
    u16 trackMask;
    f32 minHeight;
    f32 maxHeight;
} BGMParams;

// The sound of passing a point on the bridge while the camera descends
typedef struct {
    fx32 prevEyeZ;
    // The camera's height on the last frames it moved, a ring buffer
    fx32 eyeY[EYE_HISTORY_SIZE];
    s16 head;
    s16 count;
    s16 fadeVolume;
    // 0: idle, 1: playing, 2: fading out
    u16 state;
    u16 se;
} PassingSE;

typedef struct {
    u16 heapId;
    ISS3DSoundSys *soundSys;
    FieldSoundEmitter *emitters[OBJ_COUNT];
    s32 waits[OBJ_COUNT];
    s32 waitMin[OBJ_COUNT];
    s32 waitMax[OBJ_COUNT];
    BGMParams bgmParams;
    s32 bgmVolume;
    PassingSE passingSE;
} GimmickWork;

static void SkyarrowBridge_Init(GimmickWork *work, Field *field);
static void SkyarrowBridge_LoadWaitRanges(GimmickWork *work);
static void SkyarrowBridge_LoadBGMParams(GimmickWork *work);
static void SkyarrowBridge_SaveFrames(GimmickWork *work, Field *field);
static void SkyarrowBridge_LoadFrames(GimmickWork *work, Field *field);
static void SkyarrowBridge_StartWait(GimmickWork *work, s32 obj);
static void SkyarrowBridge_RestartObj(GimmickWork *work, s32 obj);
static void SkyarrowBridge_ApplyBGMVolume(GimmickWork *work);
static void SkyarrowBridge_UpdateBGMVolume(GimmickWork *work, Field *field);
static s32 SkyarrowBridge_GetTargetBGMVolume(GimmickWork *work, Field *field);
static void SkyarrowBridge_RaiseBGMVolume(GimmickWork *work, s32 target);
static void SkyarrowBridge_LowerBGMVolume(GimmickWork *work, s32 target);
static void PassingSE_Init(PassingSE *se);
static void PassingSE_Update(PassingSE *se, FieldCamera *camera, FieldPlayer *player);

static const G3DSceneAnimationSetup sAnimations[] = { { 3, 0 } };

static const G3DSceneResourceSetup sResources[] = {
    { ARCID_SKYARROW_BRIDGE, 14, 0 },
    { ARCID_SKYARROW_BRIDGE, 15, 0 },
    { ARCID_SKYARROW_BRIDGE, 16, 0 },
    { ARCID_SKYARROW_BRIDGE, 17, 0 },
};

static const G3DSceneActorSetup sActors[OBJ_COUNT] = {
    { 0, 0, 0, 0, NULL, 0 },
    { 1, 0, 1, 0, NULL, 0 },
    { 0, 0, 0, 0, NULL, 0 },
    { 1, 0, 1, 0, NULL, 0 },
    { 2, 0, 2, 0, sAnimations, NELEMS(sAnimations) },
};

static G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void func_ov109_021eec80(Field *field) {
    u16 heapId;
    FieldExpObjSystem *system;
    FieldExpObjAnm *anm;
    GimmickWork *work;

    heapId = Field_GetHeapID(field);
    system = Field_GetExpObjSystem(field);
    LoadFieldExpandObjData(system, &sSceneSetup, 0);
    FieldExpObj_SetAnm(system, 0, 4, 0, TRUE);
    anm = FieldExpObj_GetAnmInfo(system, 0, 4, 0);
    FieldExpObjAnm_SetPaused(anm, FALSE);
    FieldExpObjAnm_SetLooped(anm, TRUE);
    work = Field_AllocGimmickWorkBlock(field, 0, heapId, sizeof(GimmickWork));
    SkyarrowBridge_Init(work, field);
    SkyarrowBridge_LoadFrames(work, field);
    ISS3DSoundSys_ReqChangeMasterVolume(work->soundSys, 127);
}

void func_ov109_021eecf4(Field *field) {
    s32 i;
    GimmickWork *work;

    work = Field_GetGimmickWorkBlock(field, 0);
    ISS3DSoundSys_ReqChangeMasterVolume(work->soundSys, 0);
    GFL_SndBGMSetVolume(work->bgmParams.trackMask, 0);
    SkyarrowBridge_SaveFrames(work, field);
    for (i = 0; i < OBJ_COUNT; i++) {
        FieldSoundEmitter_Free(work->emitters[i]);
    }
    Field_DeleteGimmickWorkBlock(field, 0);
}

void func_ov109_021eed38(Field *field) {
    s32 i;
    GimmickWork *work;
    FieldCamera *camera;
    VecFx32 eye;
    VecFx32 target;

    work = Field_GetGimmickWorkBlock(field, 0);
    camera = Field_GetCameraSystem(field);
    FieldCamera_CoordsGetEye(camera, &eye);
    FieldCamera_CoordsGetTarget(camera, &target);
    ISS3DSoundSys_SetListener(work->soundSys, &eye, &target);
    for (i = 0; i < OBJ_COUNT; i++) {
        if (work->waits[i] > 0) {
            if (--work->waits[i] <= 0) {
                SkyarrowBridge_RestartObj(work, i);
                // The second and the fourth object set off on the frame after the one they follow
                switch (i) {
                case 0:
                    work->waits[1] = 1;
                    break;
                case 2:
                    work->waits[3] = 1;
                    break;
                }
            }
        } else if (FieldSoundEmitter_StepTransRot(work->emitters[i], FX32_ONE)) {
            SkyarrowBridge_StartWait(work, i);
        }
    }
    SkyarrowBridge_UpdateBGMVolume(work, field);
    SkyarrowBridge_ApplyBGMVolume(work);
    FieldExpObj_StepAllAnimations(Field_GetExpObjSystem(field));
    PassingSE_Update(&work->passingSE, camera, Field_GetPlayer(field));
}

static void SkyarrowBridge_Init(GimmickWork *work, Field *field) {
    s32 i;
    u16 heapId;
    FieldExpObjSystem *system;
    ObjSoundParams *params;
    FieldSoundEmitter *emitter;

    heapId = Field_GetHeapID(field);
    system = Field_GetExpObjSystem(field);
    work->heapId = heapId;
    work->bgmVolume = 0;
    work->soundSys = ISS_Get3DSoundSys(GameSystem_GetISS(Field_GetGameSystem(field)));

    params = GFL_ArcSysReadHeapNew(ARCID_SKYARROW_BRIDGE, 1, heapId);
    emitter = FieldSoundEmitter_CreateAndSet(field, FieldExpObj_GetActorMatrixPtr(system, 0, 0), 8,
                                             params->range * FX32_ONE, params->volume);
    FieldSoundEmitter_BindMotionCurve(emitter, ARCID_SKYARROW_BRIDGE, 3, 10);
    work->emitters[0] = emitter;
    GFL_HeapFree(params);

    emitter = FieldSoundEmitter_Create(field, FieldExpObj_GetActorMatrixPtr(system, 0, 1));
    FieldSoundEmitter_BindMotionCurve(emitter, ARCID_SKYARROW_BRIDGE, 5, 10);
    work->emitters[1] = emitter;

    params = GFL_ArcSysReadHeapNew(ARCID_SKYARROW_BRIDGE, 6, heapId);
    emitter = FieldSoundEmitter_CreateAndSet(field, FieldExpObj_GetActorMatrixPtr(system, 0, 2), 9,
                                             params->range * FX32_ONE, params->volume);
    FieldSoundEmitter_BindMotionCurve(emitter, ARCID_SKYARROW_BRIDGE, 8, 10);
    work->emitters[2] = emitter;
    GFL_HeapFree(params);

    emitter = FieldSoundEmitter_Create(field, FieldExpObj_GetActorMatrixPtr(system, 0, 3));
    FieldSoundEmitter_BindMotionCurve(emitter, ARCID_SKYARROW_BRIDGE, 10, 10);
    work->emitters[3] = emitter;

    params = GFL_ArcSysReadHeapNew(ARCID_SKYARROW_BRIDGE, 12, heapId);
    emitter = FieldSoundEmitter_CreateAndSet(field, FieldExpObj_GetActorMatrixPtr(system, 0, 4), 7,
                                             params->range * FX32_ONE, params->volume);
    FieldSoundEmitter_BindMotionCurve(emitter, ARCID_SKYARROW_BRIDGE, 11, 10);
    work->emitters[4] = emitter;
    GFL_HeapFree(params);

    for (i = 0; i < OBJ_COUNT; i++) {
        work->waits[i] = 0;
    }
    SkyarrowBridge_LoadWaitRanges(work);
    SkyarrowBridge_LoadBGMParams(work);
    PassingSE_Init(&work->passingSE);
}

static void SkyarrowBridge_LoadWaitRanges(GimmickWork *work) {
    u32 fileIds[OBJ_COUNT] = { 2, 4, 7, 9, 13 };
    s32 i;
    ObjWaitRange *range;

    for (i = 0; i < OBJ_COUNT; i++) {
        range = GFL_ArcSysReadHeapNew(ARCID_SKYARROW_BRIDGE, fileIds[i], work->heapId);
        work->waitMin[i] = range->min;
        work->waitMax[i] = range->max;
        GFL_HeapFree(range);
    }
}

static void SkyarrowBridge_LoadBGMParams(GimmickWork *work) {
    GFL_ArcSysReadRange(&work->bgmParams, ARCID_SKYARROW_BRIDGE, 0, 0, sizeof(BGMParams));
}

// The gimmick state keeps each object's frame on its curve while the zone is unloaded
static void SkyarrowBridge_SaveFrames(GimmickWork *work, Field *field) {
    s32 i;
    s32 *frames;

    frames = GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))),
                                      GIMMICK_STATE_ID);
    for (i = 0; i < OBJ_COUNT; i++) {
        frames[i] = FX_Whole(FieldSoundEmitter_GetAnimFrame(work->emitters[i]));
    }
}

static void SkyarrowBridge_LoadFrames(GimmickWork *work, Field *field) {
    s32 i;
    s32 *frames;

    frames = GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))),
                                      GIMMICK_STATE_ID);
    for (i = 0; i < OBJ_COUNT; i++) {
        FieldSoundEmitter_SetAnimFrame(work->emitters[i], frames[i] * FX32_ONE);
    }
}

static void SkyarrowBridge_StartWait(GimmickWork *work, s32 obj) {
    work->waits[obj] = work->waitMin[obj] + GFL_RandomLCAlt(work->waitMax[obj] - work->waitMin[obj] + 1);
}

static void SkyarrowBridge_RestartObj(GimmickWork *work, s32 obj) {
    FieldSoundEmitter_SetAnimFrame(work->emitters[obj], 0);
}

static void SkyarrowBridge_ApplyBGMVolume(GimmickWork *work) {
    GFL_SndBGMSetVolume(work->bgmParams.trackMask, work->bgmVolume);
}

static void SkyarrowBridge_UpdateBGMVolume(GimmickWork *work, Field *field) {
    s32 target;

    target = SkyarrowBridge_GetTargetBGMVolume(work, field);
    if (work->bgmVolume < target) {
        SkyarrowBridge_RaiseBGMVolume(work, target);
    } else if (target < work->bgmVolume) {
        SkyarrowBridge_LowerBGMVolume(work, target);
    }
}

// Silent at or below the low height, full above the high one, and linear between them
static s32 SkyarrowBridge_GetTargetBGMVolume(GimmickWork *work, Field *field) {
    VecFx32 eye;
    f32 height;
    f32 range;

    FieldCamera_CoordsGetEye(Field_GetCameraSystem(field), &eye);
    height = FX_FX32_TO_F32(eye.y);
    if (height <= work->bgmParams.minHeight) {
        return 0;
    }
    if (work->bgmParams.maxHeight < height) {
        return 127;
    }
    range = work->bgmParams.maxHeight - work->bgmParams.minHeight;
    return 127.0f * (height - work->bgmParams.minHeight) / range;
}

static void SkyarrowBridge_RaiseBGMVolume(GimmickWork *work, s32 target) {
    work->bgmVolume += 16;
    if (target < work->bgmVolume) {
        work->bgmVolume = target;
    }
}

static void SkyarrowBridge_LowerBGMVolume(GimmickWork *work, s32 target) {
    work->bgmVolume -= 16;
    if (work->bgmVolume < target) {
        work->bgmVolume = target;
    }
}

static void PassingSE_Init(PassingSE *se) {
    MIi_CpuClear32(0, se, sizeof(PassingSE));
}

static void PassingSE_Update(PassingSE *se, FieldCamera *camera, FieldPlayer *player) {
    VecFx32 eye;
    s32 i;
    fx32 border;
    u32 seNo;

    if (se->state == 1 || se->state == 2) {
        if (!GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_171))) {
            se->state = 0;
        }
        if (se->state == 2) {
            se->fadeVolume -= 16;
            if (se->fadeVolume < 0) {
                GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(se->se));
                se->state = 0;
            } else {
                GFL_SndPlayerSetVolume(GFL_SndSeqGetPlayerIndex(se->se), se->fadeVolume);
            }
        }
    }

    FieldCamera_CoordsGetEye(camera, &eye);
    if (MATH_ABS(eye.z - se->prevEyeZ) > 0) {
        if (se->count < EYE_HISTORY_SIZE) {
            i = (se->head + se->count) % EYE_HISTORY_SIZE;
            se->count++;
        } else {
            i = se->head;
            se->head = (se->head + 1) % EYE_HISTORY_SIZE;
        }
        se->eyeY[i] = eye.y;
    } else {
        se->count = 0;
        se->head = 0;
        if (se->state == 1 && eye.z > FX32_CONST(1816)) {
            se->state = 2;
            se->fadeVolume = 127;
        }
    }

    if (se->count > 0) {
        if (FieldPlayer_DeriveExState(player) == 0) {
            border = FX32_CONST(1884);
            seNo = SEQ_SE_FLD_173;
        } else {
            border = FX32_CONST(1980);
            seNo = SEQ_SE_FLD_171;
        }
        if ((eye.z < border && se->prevEyeZ > border) || (eye.z > border && se->prevEyeZ < border)) {
            if (se->eyeY[se->head] - eye.y >= FX32_CONST(100)) {
                GFL_SndSEPlay(seNo);
                se->state = 1;
                se->se = seNo;
            }
        }
    }
    se->prevEyeZ = eye.z;
}
