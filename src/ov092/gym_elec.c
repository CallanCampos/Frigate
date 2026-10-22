#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/field_fog.h"
#include "field/field_map.h"
#include "field/gym_gimmick.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "save/player_info.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/iss_switch_sys.h"
#include "system/iss_sys.h"

// Nimbasa City's gym. Its progress, from 0 to 4, is saved, and each step fades fog out of more of the gym, unmutes more
// of the gym's music and shows more of its objects. Objects follow field actors, effects burst around the player, and
// one object is animated in time with the music

#define GIMMICK_ID 0
#define GIMMICK_STATE_ID 2

// The archive of the gym's models
#define ARC_GYM_ELEC 108

#define OBJECT_COUNT 22
// The object that is animated in time with the music, at 30 frames a second
#define OBJECT_MUSIC 21
#define OBJECT_BURST_0 10

#define FOLLOWER_COUNT 4
#define STAGE_OBJECT_COUNT 7
#define FADE_COUNT 2
#define MODEL_COUNT 3
#define EFFECT_OBJECT_COUNT 2
#define BURST_COUNT 2
#define BURST_OBJECT_COUNT 3

#define FOG_TABLE_SIZE 32

// The gym's saved state
typedef struct {
    u16 progress;
} GymElecSave;

// An actor of the gym's scene
typedef struct {
    u32 scene;
    u32 actor;
    u32 unk8;
    FieldExpObjSystem *expObj;
    BOOL visible;
} GymElecObject;

typedef struct {
    G3DActor *musicActor;
    FieldExpObjSystem *expObj;
    GymElecObject objects[OBJECT_COUNT];
    // The VBlank count when the music's object was last stepped, and whether half a frame is left over
    u32 vblankCount;
    u32 halfFrame;
} GymElecScene;

// The fog's offset moves from start to end as the camera's target moves from startZ to endZ
typedef struct {
    s32 startZ;
    s32 endZ;
    s32 startOffset;
    s32 endOffset;
} FogStage;

typedef struct {
    FieldFog *fog;
    FieldCamera *camera;
    BOOL followCamera;
    u32 stage;
    u8 table[FOG_TABLE_SIZE];
    // Fades the fog's table out to 0 over fadeFrames
    BOOL fading;
    s16 fadeFrame;
    s16 fadeFrames;
    BOOL stageChanged;
    FogStage current;
    const FogStage *stages;
    u32 stageCount;
    u32 unk54;
} GymElecFog;

// An object that follows a field actor
typedef struct {
    FieldActor *actor;
    GymElecObject *object;
    BOOL attached;
} Follower;

typedef struct {
    MMSys *mmSys;
    u32 unk4;
    Follower followers[FOLLOWER_COUNT];
} GymElecFollowers;

// Fades an object's material in
typedef struct {
    BOOL active;
    GymElecObject *object;
    u16 frame;
    u16 frames;
    s32 alpha;
} Fade;

typedef struct {
    GymElecObject *objects[STAGE_OBJECT_COUNT];
    Fade fades[FADE_COUNT];
} GymElecStageObjects;

// Objects of which one is shown at a time
typedef struct {
    GymElecObject *objects[MODEL_COUNT];
} GymElecModels;

// Objects shown at points around the player until their animations end
typedef struct {
    GymElecObject *objects[BURST_OBJECT_COUNT];
    BOOL active;
    s16 state;
    s16 count;
    VecFx32 offsets[BURST_OBJECT_COUNT];
} Burst;

typedef struct GymElecEffects GymElecEffects;

typedef void (*GymElecEffectsFunc)(GymElecEffects *effects);

struct GymElecEffects {
    GymElecObject *objects[EFFECT_OBJECT_COUNT];
    Burst bursts[BURST_COUNT];
    FieldPlayer *player;
    u32 state;
    s32 wait;
    GymElecEffectsFunc func;
    u32 burstCount;
};

typedef struct {
    GymElecFog *fog;
    GymElecFollowers *followers;
    GymElecScene *scene;
    ISSSwitchSys *switchSys;
    GymElecStageObjects *stageObjects;
    GymElecModels *models;
    GymElecEffects *effects;
    GymElecSave *save;
} GymElecWork;

typedef struct {
    GymElecFog *fog;
    ISSSwitchSys *switchSys;
    GymElecStageObjects *stageObjects;
    GymElecEffects *effects;
    GameSystem *gsys;
    u32 progress;
} ProgressEventWork;

// An object's position, and whether its animations loop
typedef struct {
    u32 actor;
    VecFx32 pos;
    BOOL looped;
} StageObjectSetup;

typedef struct {
    u32 actor;
    VecFx32 pos;
} ObjectSetup;

// An area where a burst puts an object
typedef struct {
    fx32 x;
    fx32 z;
    u32 width;
    u32 depth;
} BurstArea;

static GymElecSave *GymElec_GetSave(Field *field);
static void GymElec_ApplyProgress(GymElecWork *wk, GymElecSave *save, Field *field);
static GymElecScene *GymElecScene_Create(FieldExpObjSystem *expObj, HeapID heapId);
static void GymElecScene_Delete(GymElecScene *scene);
static void GymElecScene_Update(GymElecScene *scene);
static GymElecObject *GymElecScene_GetObject(GymElecScene *scene, u32 index);
static void GymElecObject_SetVisible(GymElecObject *obj, BOOL visible);
static void GymElecObject_SetTranslation(GymElecObject *obj, const VecFx32 *pos);
static void GymElecObject_Play(GymElecObject *obj, u32 anm);
static void GymElecObject_PlayFrom(GymElecObject *obj, u32 anm, fx32 frame);
static void GymElecObject_SetPaused(GymElecObject *obj, u32 anm, BOOL paused);
static BOOL GymElecObject_IsFinished(GymElecObject *obj, u32 anm);
static void GymElecObject_PlayAll(GymElecObject *obj);
static void GymElecObject_PlayAllFrom(GymElecObject *obj, fx32 frame);
static void GymElecObject_ClearAll(GymElecObject *obj);
static BOOL GymElecObject_IsAllFinished(GymElecObject *obj);
static void GymElecObject_SetLooped(GymElecObject *obj, BOOL looped);
static void GymElecObject_ShowEnd(GymElecObject *obj);
static G3DActor *GymElecObject_GetActor(GymElecObject *obj);
static GymElecFog *GymElecFog_Create(FieldFog *fieldFog, FieldCamera *camera, const FogStage *stages, u32 stageCount,
                                     u32 stage, HeapID heapId);
static void GymElecFog_Delete(GymElecFog *fog);
static void GymElecFog_Update(GymElecFog *fog);
static void GymElecFog_StartFade(GymElecFog *fog, s16 frames);
static BOOL GymElecFog_IsFadeDone(GymElecFog *fog);
static void GymElecFog_SetStage(GymElecFog *fog, u32 stage);
static void GymElecFog_SetEnabled(GymElecFog *fog, BOOL enabled);
static GymElecFollowers *GymElecFollowers_Create(MMSys *mmSys, GymElecScene *scene, HeapID heapId);
static void GymElecFollowers_Delete(GymElecFollowers *followers);
static void GymElecFollowers_Update(GymElecFollowers *followers);
static void GymElecFollowers_Attach(GymElecFollowers *followers, u16 actorId, u32 index);
static void GymElecFollowers_Detach(GymElecFollowers *followers, u16 actorId, u32 index);
static Follower *GymElecFollowers_Get(GymElecFollowers *followers, u32 index);
static void GymElecFollower_Init(Follower *follower, GymElecObject *object);
static void GymElecFollower_Update(Follower *follower);
static void GymElecFollower_Attach(Follower *follower, FieldActor *actor);
static void GymElecFollower_Detach(Follower *follower);
static BOOL GymElecFollower_IsAttached(Follower *follower);
static GymElecStageObjects *GymElecStageObjects_Create(GymElecScene *scene, HeapID heapId);
static void GymElecStageObjects_Delete(GymElecStageObjects *stageObjects);
static void GymElecStageObjects_Update(GymElecStageObjects *stageObjects);
static void GymElecStageObjects_ShowEnd(GymElecStageObjects *stageObjects, u32 index);
static void GymElecStageObjects_Play(GymElecStageObjects *stageObjects, u32 index);
static void GymElecStageObjects_FadeIn(GymElecStageObjects *stageObjects, u32 index, u32 frames);
static void GymElecFade_Start(Fade *fade, GymElecObject *object, u32 frames);
static void GymElecFade_Update(Fade *fade);
static BOOL GymElecFade_IsActive(const Fade *fade);
static GymElecModels *GymElecModels_Create(GymElecScene *scene, HeapID heapId);
static void GymElecModels_Delete(GymElecModels *models);
static void GymElecModels_Update(GymElecModels *models);
static void GymElecModels_Show(GymElecModels *models, u32 index);
static GymElecEffects *GymElecEffects_Create(GymElecScene *scene, FieldPlayer *player, HeapID heapId);
static void GymElecEffects_Delete(GymElecEffects *effects);
static void GymElecEffects_Update(GymElecEffects *effects);
static void GymElecEffects_SetMode(GymElecEffects *effects, u32 mode);
static void GymElecEffects_StartBurst(GymElecEffects *effects);
static void GymElecEffects_Mode0(GymElecEffects *effects);
static void GymElecEffects_Mode1(GymElecEffects *effects);
static void GymElecEffects_Mode2(GymElecEffects *effects);
static void GymElecBurst_Init(Burst *burst, GymElecObject *obj0, GymElecObject *obj1, GymElecObject *obj2);
static void GymElecBurst_Clear(Burst *burst);
static void GymElecBurst_Update(Burst *burst, const VecFx32 *playerPos);
static void GymElecBurst_Start(Burst *burst, const VecFx32 *offsets, s32 count);
static BOOL GymElecBurst_IsActive(Burst *burst);
static GameEvent *GymElec_CreateProgressEvent(GameSystem *gsys, GymElecFog *fog, GymElecStageObjects *stageObjects,
                                              GymElecEffects *effects, u32 progress);
static GameEventReturnCode GymElec_ProgressEvent(GameEvent *event, u32 *state, void *data);
static void GymElecSave_SetProgress(GymElecSave *save, u16 progress);
static u16 GymElecSave_GetProgress(GymElecSave *save);

static const FogStage sFogStages[] = {
    { 968, 872, 0x7f8a, 0x7f6f },
    { 872, 632, 0x7fb4, 0x7f6f },
    { 632, 392, 0x7faf, 0x7f6f },
};

// The music's ticks for each loop of the music object's 30-frame animation
const u32 GYM_ELEC_MUSIC_LOOP_TICKS = 11520;

static const G3DSceneAnimationSetup sAnimations05[] = { { 5, 0 } };
static const G3DSceneAnimationSetup sAnimations07[] = { { 7, 0 } };
static const G3DSceneAnimationSetup sAnimations09[] = { { 9, 0 } };
static const G3DSceneAnimationSetup sAnimations0E[] = { { 14, 0 }, { 15, 0 } };
static const G3DSceneAnimationSetup sAnimations11[] = { { 17, 0 }, { 18, 0 } };
static const G3DSceneAnimationSetup sAnimations0B[] = { { 11, 0 }, { 12, 0 } };
static const G3DSceneAnimationSetup sAnimations18[] = { { 24, 0 } };
static const G3DSceneAnimationSetup sAnimations16[] = { { 22, 0 } };
static const G3DSceneAnimationSetup sAnimations1A[] = { { 26, 0 } };
static const G3DSceneAnimationSetup sAnimations21[] = { { 33, 0 } };
static const G3DSceneAnimationSetup sAnimations1D[] = { { 29, 0 }, { 30, 0 }, { 31, 0 } };
static const G3DSceneAnimationSetup sAnimations14[] = { { 20, 0 } };

static const G3DSceneActorSetup sActors[OBJECT_COUNT] = {
    { 0, 0, 0, 0, NULL, 0 },
    { 2, 0, 2, 0, NULL, 0 },
    { 3, 0, 3, 0, NULL, 0 },
    { 1, 0, 1, 0, NULL, 0 },
    { 4, 0, 4, 0, sAnimations05, NELEMS(sAnimations05) },
    { 6, 0, 6, 0, sAnimations07, NELEMS(sAnimations07) },
    { 8, 0, 8, 0, sAnimations09, NELEMS(sAnimations09) },
    { 10, 0, 10, 0, sAnimations0B, NELEMS(sAnimations0B) },
    { 13, 0, 13, 0, sAnimations0E, NELEMS(sAnimations0E) },
    { 16, 0, 16, 0, sAnimations11, NELEMS(sAnimations11) },
    { 19, 0, 19, 0, sAnimations14, NELEMS(sAnimations14) },
    { 19, 0, 19, 0, sAnimations14, NELEMS(sAnimations14) },
    { 19, 0, 19, 0, sAnimations14, NELEMS(sAnimations14) },
    { 19, 0, 19, 0, sAnimations14, NELEMS(sAnimations14) },
    { 19, 0, 19, 0, sAnimations14, NELEMS(sAnimations14) },
    { 19, 0, 19, 0, sAnimations14, NELEMS(sAnimations14) },
    { 21, 0, 21, 0, sAnimations16, NELEMS(sAnimations16) },
    { 23, 0, 23, 0, sAnimations18, NELEMS(sAnimations18) },
    { 25, 0, 25, 0, sAnimations1A, NELEMS(sAnimations1A) },
    { 27, 0, 27, 0, sAnimations1D, NELEMS(sAnimations1D) },
    { 28, 0, 28, 0, sAnimations1D, NELEMS(sAnimations1D) },
    { 32, 0, 32, 0, sAnimations21, NELEMS(sAnimations21) },
};

static const G3DSceneResourceSetup sResources[] = {
    { ARC_GYM_ELEC, 7, 0 },  { ARC_GYM_ELEC, 8, 0 },  { ARC_GYM_ELEC, 9, 0 },  { ARC_GYM_ELEC, 10, 0 },
    { ARC_GYM_ELEC, 22, 0 }, { ARC_GYM_ELEC, 23, 0 }, { ARC_GYM_ELEC, 24, 0 }, { ARC_GYM_ELEC, 25, 0 },
    { ARC_GYM_ELEC, 26, 0 }, { ARC_GYM_ELEC, 27, 0 }, { ARC_GYM_ELEC, 18, 0 }, { ARC_GYM_ELEC, 17, 0 },
    { ARC_GYM_ELEC, 19, 0 }, { ARC_GYM_ELEC, 12, 0 }, { ARC_GYM_ELEC, 11, 0 }, { ARC_GYM_ELEC, 13, 0 },
    { ARC_GYM_ELEC, 15, 0 }, { ARC_GYM_ELEC, 14, 0 }, { ARC_GYM_ELEC, 16, 0 }, { ARC_GYM_ELEC, 6, 0 },
    { ARC_GYM_ELEC, 5, 0 },  { ARC_GYM_ELEC, 28, 0 }, { ARC_GYM_ELEC, 29, 0 }, { ARC_GYM_ELEC, 30, 0 },
    { ARC_GYM_ELEC, 31, 0 }, { ARC_GYM_ELEC, 32, 0 }, { ARC_GYM_ELEC, 33, 0 }, { ARC_GYM_ELEC, 3, 0 },
    { ARC_GYM_ELEC, 4, 0 },  { ARC_GYM_ELEC, 0, 0 },  { ARC_GYM_ELEC, 1, 0 },  { ARC_GYM_ELEC, 2, 0 },
    { ARC_GYM_ELEC, 20, 0 }, { ARC_GYM_ELEC, 21, 0 },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

static const StageObjectSetup sStageObjectSetups[STAGE_OBJECT_COUNT] = {
    { 4, { FX32_CONST(256), 0, FX32_CONST(768) }, FALSE },
    { 5, { FX32_CONST(256), 0, FX32_CONST(256) }, FALSE },
    { 6, { FX32_CONST(256), 0, FX32_CONST(256) }, FALSE },
    { 16, { FX32_CONST(256), 0, FX32_CONST(256) }, TRUE },
    { 17, { FX32_CONST(256), 0, FX32_CONST(256) }, TRUE },
    { 18, { FX32_CONST(256), 0, FX32_CONST(256) }, TRUE },
    { OBJECT_MUSIC, { FX32_CONST(256), 0, FX32_CONST(256) }, TRUE },
};

static const u8 sFogTable[FOG_TABLE_SIZE] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68,
};

static const u32 sFollowerObjects[FOLLOWER_COUNT] = { 0, 1, 2, 3 };

static const ObjectSetup sModelSetups[MODEL_COUNT] = {
    { 7, { FX32_CONST(256), 0, FX32_CONST(256) } },
    { 8, { FX32_CONST(256), 0, FX32_CONST(256) } },
    { 9, { FX32_CONST(256), 0, FX32_CONST(256) } },
};

static const BurstArea sBurstAreas[] = {
    { FX32_CONST(-112), FX32_CONST(-109), FX32_CONST(40), FX32_CONST(194) },
    { FX32_CONST(70), FX32_CONST(-109), FX32_CONST(40), FX32_CONST(192) },
};

static const ObjectSetup sEffectSetups[EFFECT_OBJECT_COUNT] = {
    { 19, { FX32_CONST(256), 0, FX32_CONST(768) } },
    { 20, { FX32_CONST(256), 0, FX32_CONST(256) } },
};

// How many objects each burst shows, in turn
static const s8 sBurstSizes[] = { 3, 2, 1, 3, 2, 3, 1, 3, 2 };

void GymElec_Init(Field *field) {
    HeapID heapId = Field_GetHeapID(field);
    GymElecWork *wk = Field_AllocGimmickWorkBlock(field, GIMMICK_ID, heapId, sizeof(GymElecWork));

    wk->save = GymElec_GetSave(field);
    wk->switchSys = ISS_GetSwitchSys(GameSystem_GetISS(Field_GetGameSystem(field)));
    wk->scene = GymElecScene_Create(Field_GetExpObjSystem(field), heapId);
    wk->fog =
        GymElecFog_Create(Field_GetFog(field), Field_GetCameraSystem(field), sFogStages, NELEMS(sFogStages), 0, heapId);
    wk->followers = GymElecFollowers_Create(Field_GetActorSystem(field), wk->scene, heapId);
    wk->stageObjects = GymElecStageObjects_Create(wk->scene, heapId);
    wk->models = GymElecModels_Create(wk->scene, heapId);
    wk->effects = GymElecEffects_Create(wk->scene, Field_GetPlayer(field), heapId);
    GymElec_ApplyProgress(wk, wk->save, field);
}

void GymElec_End(Field *field) {
    GymElecWork *wk = Field_GetGimmickWorkBlock(field, GIMMICK_ID);

    ISSSwitchSys_ResetMuteStateChangeRequests(wk->switchSys);
    GymElecEffects_Delete(wk->effects);
    GymElecModels_Delete(wk->models);
    GymElecStageObjects_Delete(wk->stageObjects);
    GymElecFollowers_Delete(wk->followers);
    GymElecFog_Delete(wk->fog);
    GymElecScene_Delete(wk->scene);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_ID);
}

void GymElec_Update(Field *field) {
    GymElecWork *wk = Field_GetGimmickWorkBlock(field, GIMMICK_ID);

    GymElecFog_Update(wk->fog);
    GymElecFollowers_Update(wk->followers);
    GymElecStageObjects_Update(wk->stageObjects);
    GymElecModels_Update(wk->models);
    GymElecEffects_Update(wk->effects);
    GymElecScene_Update(wk->scene);
}

// Saves the gym's progress, and returns the event that shows the step to it, if any
GameEvent *GymElec_SetProgress(GameSystem *gsys, u8 progress) {
    Field *field = GSYS_GetField(gsys);
    GymElecWork *wk = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    FieldPlayer *player = Field_GetPlayer(field);

    GymElecSave_SetProgress(wk->save, progress);
    switch (progress) {
    case 0:
        return NULL;
    case 1:
        return GymElec_CreateProgressEvent(gsys, wk->fog, wk->stageObjects, wk->effects, 0);
    case 2:
        return GymElec_CreateProgressEvent(gsys, wk->fog, wk->stageObjects, wk->effects, 1);
    case 3:
        return GymElec_CreateProgressEvent(gsys, wk->fog, wk->stageObjects, wk->effects, 2);
    case 4:
        return NULL;
    }
    return NULL;
}

void GymElec_SetFollower(Field *field, BOOL attach, u16 actorId, u32 index) {
    GymElecWork *wk = Field_GetGimmickWorkBlock(field, GIMMICK_ID);

    if (attach) {
        GFL_SndSEPlay(SEQ_SE_GYM_E03);
        GymElecFollowers_Attach(wk->followers, actorId, index);
    } else {
        GymElecFollowers_Detach(wk->followers, actorId, index);
    }
}

void GymElec_SetEffectsMode(Field *field, u32 mode) {
    GymElecWork *wk = Field_GetGimmickWorkBlock(field, GIMMICK_ID);

    GymElecEffects_SetMode(wk->effects, mode);
}

// Shows the first model, or the one for the player's gender
void GymElec_ShowModel(Field *field, u32 mode) {
    GymElecWork *wk = Field_GetGimmickWorkBlock(field, GIMMICK_ID);

    switch (mode) {
    case 0:
        GymElecModels_Show(wk->models, 0);
        break;
    case 1:
        if (FieldPlayer_GetSex(Field_GetPlayer(field)) == GENDER_MALE) {
            GymElecModels_Show(wk->models, 1);
        } else {
            GymElecModels_Show(wk->models, 2);
        }
        break;
    }
}

void GymElec_ShowStageObject5(Field *field, u32 mode) {
    GymElecWork *wk = Field_GetGimmickWorkBlock(field, GIMMICK_ID);

    switch (mode) {
    case 0:
        GymElecStageObjects_FadeIn(wk->stageObjects, 5, 60);
        break;
    case 1:
        GymElecStageObjects_Play(wk->stageObjects, 5);
        break;
    }
}

void GymElec_ReapplyProgress(Field *field) {
    GymElecWork *wk = Field_GetGimmickWorkBlock(field, GIMMICK_ID);

    GymElec_ApplyProgress(wk, wk->save, field);
}

void GymElec_SetBrightness(Field *field, s32 brightness) {
    GX_SetMasterBrightness(brightness);
}

static GymElecSave *GymElec_GetSave(Field *field) {
    return GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))),
                                    GIMMICK_STATE_ID);
}

static void GymElec_ApplyProgress(GymElecWork *wk, GymElecSave *save, Field *field) {
    switch (GymElecSave_GetProgress(save)) {
    case 0:
        GymElecFog_SetEnabled(wk->fog, TRUE);
        GymElecFog_SetStage(wk->fog, 0);
        ISSSwitch_ReqSwitchMuteStateChange(wk->switchSys, 1, FALSE, SEQ_BGM_ERECTRIC_GYM_01);
        ISSSwitch_ReqSwitchMuteStateChange(wk->switchSys, 2, FALSE, SEQ_BGM_ERECTRIC_GYM_01);
        break;
    case 1:
        GymElecFog_SetEnabled(wk->fog, TRUE);
        GymElecFog_SetStage(wk->fog, 1);
        ISSSwitch_ReqSwitchMuteStateChange(wk->switchSys, 1, TRUE, SEQ_BGM_ERECTRIC_GYM_01);
        GymElecStageObjects_ShowEnd(wk->stageObjects, 0);
        GymElecStageObjects_Play(wk->stageObjects, 3);
        break;
    case 2:
        GymElecFog_SetEnabled(wk->fog, TRUE);
        GymElecFog_SetStage(wk->fog, 2);
        ISSSwitch_ReqSwitchMuteStateChange(wk->switchSys, 1, TRUE, SEQ_BGM_ERECTRIC_GYM_01);
        ISSSwitch_ReqSwitchMuteStateChange(wk->switchSys, 2, TRUE, SEQ_BGM_ERECTRIC_GYM_01);
        GymElecStageObjects_ShowEnd(wk->stageObjects, 0);
        GymElecStageObjects_ShowEnd(wk->stageObjects, 1);
        GymElecStageObjects_Play(wk->stageObjects, 3);
        GymElecStageObjects_Play(wk->stageObjects, 4);
        break;
    case 3:
        GymElecFog_SetEnabled(wk->fog, FALSE);
        GymElecStageObjects_ShowEnd(wk->stageObjects, 0);
        GymElecStageObjects_ShowEnd(wk->stageObjects, 1);
        GymElecStageObjects_ShowEnd(wk->stageObjects, 2);
        GymElecStageObjects_Play(wk->stageObjects, 3);
        GymElecStageObjects_Play(wk->stageObjects, 4);
        GymElecModels_Show(wk->models, 0);
        break;
    case 4:
        GymElecFog_SetEnabled(wk->fog, FALSE);
        GymElecStageObjects_ShowEnd(wk->stageObjects, 0);
        GymElecStageObjects_ShowEnd(wk->stageObjects, 1);
        GymElecStageObjects_ShowEnd(wk->stageObjects, 2);
        GymElecStageObjects_Play(wk->stageObjects, 3);
        GymElecStageObjects_Play(wk->stageObjects, 4);
        GymElecStageObjects_Play(wk->stageObjects, 5);
        GymElec_ShowModel(field, 1);
        break;
    }
}

static GymElecScene *GymElecScene_Create(FieldExpObjSystem *expObj, HeapID heapId) {
    GymElecScene *scene = GFL_HeapAllocate(heapId, sizeof(GymElecScene), TRUE, "gym_elec.c", 1247);
    GymElecObject *obj;
    int i;
    u32 frame;

    scene->expObj = expObj;
    scene->vblankCount = OS_GetVBlankCount();
    FieldExpObj_AddScene(scene->expObj, &sSceneSetup, 0);
    for (i = 0; i < OBJECT_COUNT; i++) {
        obj = GymElecScene_GetObject(scene, i);
        obj->scene = 0;
        obj->actor = i;
        obj->expObj = expObj;
        GymElecObject_SetVisible(obj, FALSE);
    }
    obj = GymElecScene_GetObject(scene, OBJECT_MUSIC);
    scene->musicActor = GymElecObject_GetActor(obj);
    frame = GFL_SndBGMGetTick() % GYM_ELEC_MUSIC_LOOP_TICKS * 30 / GYM_ELEC_MUSIC_LOOP_TICKS;
    GymElecObject_PlayAllFrom(obj, FX32_CONST(frame));
    GymElecObject_SetPaused(obj, 0, TRUE);
    return scene;
}

static void GymElecScene_Delete(GymElecScene *scene) {
    FieldExpObj_FreeScene(scene->expObj, 0);
    GFL_HeapFree(scene);
}

// Steps the music's object a frame for every two VBlanks since the last update
static void GymElecScene_Update(GymElecScene *scene) {
    u32 vblankCount = OS_GetVBlankCount();
    u32 halfFrames = scene->halfFrame + (vblankCount - scene->vblankCount);
    u32 frames;

    frames = halfFrames / 2;
    scene->halfFrame = halfFrames % 2;
    while (frames != 0) {
        GFL_G3DActorStepAnmFrameLoop(scene->musicActor, 0, FX32_ONE);
        frames--;
    }
    scene->vblankCount = vblankCount;
    GymElecObject_SetVisible(GymElecScene_GetObject(scene, OBJECT_MUSIC), TRUE);
    FieldExpObj_StepAllAnimations(scene->expObj);
}

static GymElecObject *GymElecScene_GetObject(GymElecScene *scene, u32 index) {
    return &scene->objects[index];
}

static void GymElecObject_SetVisible(GymElecObject *obj, BOOL visible) {
    FieldExpObj_SetActorHidden(obj->expObj, obj->scene, obj->actor, visible ? FALSE : TRUE);
    obj->visible = visible;
}

static void GymElecObject_SetTranslation(GymElecObject *obj, const VecFx32 *pos) {
    SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(obj->expObj, obj->scene, obj->actor);

    matrix->translation = *pos;
}

static void GymElecObject_Play(GymElecObject *obj, u32 anm) {
    GymElecObject_PlayFrom(obj, anm, 0);
}

static void GymElecObject_PlayFrom(GymElecObject *obj, u32 anm, fx32 frame) {
    FieldExpObj_SetAnm(obj->expObj, obj->scene, obj->actor, anm, TRUE);
    FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(obj->expObj, obj->scene, obj->actor, anm), FALSE);
    FieldExpObj_SetAnmFrame(obj->expObj, obj->scene, obj->actor, anm, frame);
}

static void GymElecObject_SetPaused(GymElecObject *obj, u32 anm, BOOL paused) {
    FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(obj->expObj, obj->scene, obj->actor, anm), paused);
}

static BOOL GymElecObject_IsFinished(GymElecObject *obj, u32 anm) {
    return FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(obj->expObj, obj->scene, obj->actor, anm));
}

static void GymElecObject_PlayAll(GymElecObject *obj) {
    int i;

    for (i = 0; i < sActors[obj->actor].animationCount; i++) {
        GymElecObject_Play(obj, i);
    }
}

static void GymElecObject_PlayAllFrom(GymElecObject *obj, fx32 frame) {
    int i;

    for (i = 0; i < sActors[obj->actor].animationCount; i++) {
        GymElecObject_PlayFrom(obj, i, frame);
    }
}

static void GymElecObject_ClearAll(GymElecObject *obj) {
    int i;

    for (i = 0; i < sActors[obj->actor].animationCount; i++) {
        FieldExpObj_SetAnm(obj->expObj, obj->scene, obj->actor, i, FALSE);
    }
}

static BOOL GymElecObject_IsAllFinished(GymElecObject *obj) {
    int i;

    for (i = 0; i < sActors[obj->actor].animationCount; i++) {
        if (GymElecObject_IsFinished(obj, i) == FALSE) {
            return FALSE;
        }
    }
    return TRUE;
}

static void GymElecObject_SetLooped(GymElecObject *obj, BOOL looped) {
    int i;

    for (i = 0; i < sActors[obj->actor].animationCount; i++) {
        FieldExpObjAnm_SetLooped(FieldExpObj_GetAnmInfo(obj->expObj, obj->scene, obj->actor, i), looped);
    }
}

// Shows the animations' last frames
static void GymElecObject_ShowEnd(GymElecObject *obj) {
    int i;
    FieldExpObjAnm *anm;

    for (i = 0; i < sActors[obj->actor].animationCount; i++) {
        FieldExpObj_SetAnm(obj->expObj, obj->scene, obj->actor, i, TRUE);
        anm = FieldExpObj_GetAnmInfo(obj->expObj, obj->scene, obj->actor, i);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnmFrame(obj->expObj, obj->scene, obj->actor, i, FieldExpObjAnm_GetLastFrame(anm));
    }
}

static G3DActor *GymElecObject_GetActor(GymElecObject *obj) {
    return FieldExpObj_GetActor(obj->expObj, obj->scene, obj->actor);
}

static GymElecFog *GymElecFog_Create(FieldFog *fieldFog, FieldCamera *camera, const FogStage *stages, u32 stageCount,
                                     u32 stage, HeapID heapId) {
    GymElecFog *fog = GFL_HeapAllocate(heapId, sizeof(GymElecFog), TRUE, "gym_elec.c", 1685);

    fog->fog = fieldFog;
    fog->camera = camera;
    fog->stages = stages;
    fog->stageCount = stageCount;
    MI_CpuCopy8(sFogTable, fog->table, sizeof(fog->table));
    FieldFog_SetAlphaMode(fog->fog, 0);
    FieldFog_SetDepthShift(fog->fog, 9);
    FieldFog_SetOffset(fog->fog, 0);
    FieldFog_SetColor(fog->fog, 0);
    FieldFog_SetTable(fog->fog, fog->table);
    FieldFog_SetEnabled(fog->fog, TRUE);
    fog->followCamera = TRUE;
    GymElecFog_SetStage(fog, stage);
    GymElecFog_Update(fog);
    return fog;
}

static void GymElecFog_Delete(GymElecFog *fog) {
    GFL_HeapFree(fog);
}

static void GymElecFog_Update(GymElecFog *fog) {
    int i;

    if (fog->stageChanged) {
        fog->current = fog->stages[fog->stage];
        MI_CpuCopy8(sFogTable, fog->table, sizeof(fog->table));
        FieldFog_SetTable(fog->fog, fog->table);
        fog->stageChanged = FALSE;
    }
    if (fog->followCamera) {
        FogStage *current = &fog->current;
        VecFx32 target;
        s32 offset;

        FieldCamera_CoordsGetTarget(fog->camera, &target);
        offset = current->startOffset - (current->startZ - (target.z >> FX32_SHIFT)) *
                                            (current->startOffset - current->endOffset) /
                                            (current->startZ - current->endZ);
        if (offset > 0x7fff) {
            offset = 0x7fff;
        } else if (offset < 0) {
            offset = 0;
        }
        FieldFog_SetOffset(fog->fog, offset);
    }
    if (fog->fading) {
        for (i = 0; i < FOG_TABLE_SIZE; i++) {
            s16 value = sFogTable[i] - sFogTable[i] * fog->fadeFrame / fog->fadeFrames;

            if (value > 0xff) {
                value = 0xff;
            } else if (value < 0) {
                value = 0;
            }
            fog->table[i] = value;
        }
        FieldFog_SetTable(fog->fog, fog->table);
        if (fog->fadeFrame++ > fog->fadeFrames) {
            fog->fading = FALSE;
        }
    }
}

static void GymElecFog_StartFade(GymElecFog *fog, s16 frames) {
    fog->fading = TRUE;
    fog->fadeFrame = 0;
    fog->fadeFrames = frames;
}

static BOOL GymElecFog_IsFadeDone(GymElecFog *fog) {
    return fog->fading == FALSE;
}

static void GymElecFog_SetStage(GymElecFog *fog, u32 stage) {
    fog->stageChanged = TRUE;
    fog->stage = stage;
}

static void GymElecFog_SetEnabled(GymElecFog *fog, BOOL enabled) {
    FieldFog_SetEnabled(fog->fog, enabled);
}

static GymElecFollowers *GymElecFollowers_Create(MMSys *mmSys, GymElecScene *scene, HeapID heapId) {
    GymElecFollowers *followers = GFL_HeapAllocate(heapId, sizeof(GymElecFollowers), TRUE, "gym_elec.c", 1947);
    int i;

    followers->mmSys = mmSys;
    for (i = 0; i < FOLLOWER_COUNT; i++) {
        GymElecFollower_Init(&followers->followers[i], GymElecScene_GetObject(scene, sFollowerObjects[i]));
    }
    return followers;
}

static void GymElecFollowers_Delete(GymElecFollowers *followers) {
    GFL_HeapFree(followers);
}

static void GymElecFollowers_Update(GymElecFollowers *followers) {
    int i;

    for (i = 0; i < FOLLOWER_COUNT; i++) {
        GymElecFollower_Update(&followers->followers[i]);
    }
}

static void GymElecFollowers_Attach(GymElecFollowers *followers, u16 actorId, u32 index) {
    Follower *follower = GymElecFollowers_Get(followers, index);

    if (GymElecFollower_IsAttached(follower) == FALSE) {
        GymElecFollower_Attach(follower, FindFieldActor(followers->mmSys, actorId));
    }
}

static void GymElecFollowers_Detach(GymElecFollowers *followers, u16 actorId, u32 index) {
    Follower *follower = GymElecFollowers_Get(followers, index);

    if (GymElecFollower_IsAttached(follower)) {
        GymElecFollower_Detach(follower);
    }
}

static Follower *GymElecFollowers_Get(GymElecFollowers *followers, u32 index) {
    return &followers->followers[index];
}

static void GymElecFollower_Init(Follower *follower, GymElecObject *object) {
    MI_CpuFill8(follower, 0, sizeof(Follower));
    follower->object = object;
}

static void GymElecFollower_Update(Follower *follower) {
    VecFx32 pos;

    if (follower->actor != NULL) {
        CopyActorWPos(follower->actor, &pos);
        GymElecObject_SetTranslation(follower->object, &pos);
    }
}

static void GymElecFollower_Attach(Follower *follower, FieldActor *actor) {
    follower->actor = actor;
    follower->attached = TRUE;
    GymElecObject_SetVisible(follower->object, TRUE);
}

static void GymElecFollower_Detach(Follower *follower) {
    GymElecObject_SetVisible(follower->object, FALSE);
    follower->actor = NULL;
    follower->attached = FALSE;
}

static BOOL GymElecFollower_IsAttached(Follower *follower) {
    return follower->attached;
}

static GymElecStageObjects *GymElecStageObjects_Create(GymElecScene *scene, HeapID heapId) {
    GymElecStageObjects *stageObjects = GFL_HeapAllocate(heapId, sizeof(GymElecStageObjects), TRUE, "gym_elec.c", 2268);
    int i;

    for (i = 0; i < STAGE_OBJECT_COUNT; i++) {
        stageObjects->objects[i] = GymElecScene_GetObject(scene, sStageObjectSetups[i].actor);
        GymElecObject_SetTranslation(stageObjects->objects[i], &sStageObjectSetups[i].pos);
        GymElecObject_SetLooped(stageObjects->objects[i], sStageObjectSetups[i].looped);
    }
    return stageObjects;
}

static void GymElecStageObjects_Delete(GymElecStageObjects *stageObjects) {
    GFL_HeapFree(stageObjects);
}

static void GymElecStageObjects_Update(GymElecStageObjects *stageObjects) {
    int i;

    for (i = 0; i < FADE_COUNT; i++) {
        if (GymElecFade_IsActive(&stageObjects->fades[i])) {
            GymElecFade_Update(&stageObjects->fades[i]);
        }
    }
}

static void GymElecStageObjects_ShowEnd(GymElecStageObjects *stageObjects, u32 index) {
    GymElecObject_SetVisible(stageObjects->objects[index], TRUE);
    GymElecObject_ShowEnd(stageObjects->objects[index]);
}

static void GymElecStageObjects_Play(GymElecStageObjects *stageObjects, u32 index) {
    GymElecObject_SetVisible(stageObjects->objects[index], TRUE);
    GymElecObject_PlayAll(stageObjects->objects[index]);
}

static void GymElecStageObjects_FadeIn(GymElecStageObjects *stageObjects, u32 index, u32 frames) {
    int i;

    for (i = 0; i < FADE_COUNT; i++) {
        if (GymElecFade_IsActive(&stageObjects->fades[i]) == FALSE) {
            GymElecFade_Start(&stageObjects->fades[i], stageObjects->objects[index], frames);
            break;
        }
    }
    GymElecStageObjects_Play(stageObjects, index);
}

static void GymElecFade_Start(Fade *fade, GymElecObject *object, u32 frames) {
    NNSG3dResMdl *mdl;

    MI_CpuFill8(fade, 0, sizeof(Fade));
    fade->object = object;
    fade->frames = frames;
    mdl = GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(GymElecObject_GetActor(object)))->resMdl;
    fade->alpha = NNS_G3dMdlGetMdlAlpha(mdl, 0);
    NNS_G3dMdlSetMdlAlpha(mdl, 0, 0);
    fade->active = TRUE;
}

static void GymElecFade_Update(Fade *fade) {
    s32 alpha;
    NNSG3dResMdl *mdl;

    if (fade->active) {
        alpha = fade->alpha * fade->frame / fade->frames;
        mdl = GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(GymElecObject_GetActor(fade->object)))->resMdl;
        NNS_G3dMdlSetMdlAlpha(mdl, 0, alpha);
        if (fade->frame++ > fade->frames) {
            fade->active = FALSE;
        }
    }
}

static BOOL GymElecFade_IsActive(const Fade *fade) {
    return fade->active;
}

static GymElecModels *GymElecModels_Create(GymElecScene *scene, HeapID heapId) {
    GymElecModels *models = GFL_HeapAllocate(heapId, sizeof(GymElecModels), TRUE, "gym_elec.c", 2525);
    int i;

    for (i = 0; i < MODEL_COUNT; i++) {
        models->objects[i] = GymElecScene_GetObject(scene, sModelSetups[i].actor);
        GymElecObject_SetTranslation(models->objects[i], &sModelSetups[i].pos);
    }
    GymElecObject_SetVisible(models->objects[0], TRUE);
    GymElecObject_ClearAll(models->objects[0]);
    return models;
}

static void GymElecModels_Delete(GymElecModels *models) {
    GFL_HeapFree(models);
}

static void GymElecModels_Update(GymElecModels *models) {
}

static void GymElecModels_Show(GymElecModels *models, u32 index) {
    int i;

    for (i = 0; i < MODEL_COUNT; i++) {
        GymElecObject_SetVisible(models->objects[i], FALSE);
    }
    GymElecObject_SetVisible(models->objects[index], TRUE);
    GymElecObject_PlayAll(models->objects[index]);
}

static GymElecEffects *GymElecEffects_Create(GymElecScene *scene, FieldPlayer *player, HeapID heapId) {
    GymElecEffects *effects = GFL_HeapAllocate(heapId, sizeof(GymElecEffects), TRUE, "gym_elec.c", 2718);
    int i;

    effects->player = player;
    for (i = 0; i < EFFECT_OBJECT_COUNT; i++) {
        effects->objects[i] = GymElecScene_GetObject(scene, sEffectSetups[i].actor);
        GymElecObject_SetTranslation(effects->objects[i], &sEffectSetups[i].pos);
        GymElecObject_SetVisible(effects->objects[i], TRUE);
    }
    for (i = 0; i < BURST_COUNT; i++) {
        GymElecBurst_Init(&effects->bursts[i], GymElecScene_GetObject(scene, OBJECT_BURST_0 + i * BURST_OBJECT_COUNT),
                          GymElecScene_GetObject(scene, OBJECT_BURST_0 + i * BURST_OBJECT_COUNT + 1),
                          GymElecScene_GetObject(scene, OBJECT_BURST_0 + i * BURST_OBJECT_COUNT + 2));
    }
    GymElecEffects_SetMode(effects, 0);
    return effects;
}

static void GymElecEffects_Delete(GymElecEffects *effects) {
    int i;

    for (i = 0; i < BURST_COUNT; i++) {
        GymElecBurst_Clear(&effects->bursts[i]);
    }
    GFL_HeapFree(effects);
}

static void GymElecEffects_Update(GymElecEffects *effects) {
    VecFx32 pos;
    int i;

    effects->func(effects);
    FieldPlayer_GetWPos(effects->player, &pos);
    for (i = 0; i < BURST_COUNT; i++) {
        GymElecBurst_Update(&effects->bursts[i], &pos);
    }
}

static void GymElecEffects_SetMode(GymElecEffects *effects, u32 mode) {
    effects->state = 0;
    switch (mode) {
    case 0:
        if (GFL_SndIsPlaying(SEQ_SE_GYM_E04)) {
            GFL_SndStop();
        }
        effects->func = GymElecEffects_Mode0;
        break;
    case 1:
        effects->func = GymElecEffects_Mode1;
        break;
    case 2:
        effects->func = GymElecEffects_Mode2;
        break;
    }
}

static inline void GetBurstOffset(VecFx32 *offset, const BurstArea *area) {
    offset->x = area->x + GFL_RandomLCAlt(area->width);
    offset->z = area->z + GFL_RandomLCAlt(area->depth);
    offset->y = 0;
}

// Starts a burst, if one is free
static void GymElecEffects_StartBurst(GymElecEffects *effects) {
    VecFx32 offsets[BURST_OBJECT_COUNT];
    int i;
    s8 size;
    s8 area;

    for (i = 0; i < BURST_COUNT; i++) {
        if (GymElecBurst_IsActive(&effects->bursts[i])) {
            continue;
        }
        size = sBurstSizes[effects->burstCount++ % NELEMS(sBurstSizes)];
        area = GFL_RandomLCAlt(1);
        switch (size) {
        case 1:
            GetBurstOffset(&offsets[0], &sBurstAreas[area]);
            break;
        case 2:
            GetBurstOffset(&offsets[0], &sBurstAreas[0]);
            GetBurstOffset(&offsets[1], &sBurstAreas[1]);
            break;
        default:
            size = 3;
        case 3:
            GetBurstOffset(&offsets[0], &sBurstAreas[0]);
            GetBurstOffset(&offsets[1], &sBurstAreas[1]);
            GetBurstOffset(&offsets[2], &sBurstAreas[area]);
            break;
        }
        GymElecBurst_Start(&effects->bursts[i], offsets, size);
        return;
    }
}

static void GymElecEffects_Mode0(GymElecEffects *effects) {
    int i;

    switch (effects->state) {
    case 0:
        for (i = 0; i < EFFECT_OBJECT_COUNT; i++) {
            GymElecObject_ClearAll(effects->objects[i]);
            GymElecObject_Play(effects->objects[i], 0);
        }
        effects->state++;
        break;
    case 1:
        break;
    }
}

static void GymElecEffects_Mode1(GymElecEffects *effects) {
    int i;

    switch (effects->state) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_GYM_E01);
        for (i = 0; i < EFFECT_OBJECT_COUNT; i++) {
            GymElecObject_ClearAll(effects->objects[i]);
            GymElecObject_Play(effects->objects[i], 1);
        }
        effects->state++;
        break;
    case 1:
        effects->wait = GFL_RandomLCAlt(4) + 5;
        GymElecEffects_StartBurst(effects);
        effects->state++;
        break;
    case 2:
        if (effects->wait-- <= 0) {
            effects->state = 1;
        }
        break;
    }
}

static void GymElecEffects_Mode2(GymElecEffects *effects) {
    int i;

    switch (effects->state) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_GYM_E04);
        for (i = 0; i < EFFECT_OBJECT_COUNT; i++) {
            GymElecObject_ClearAll(effects->objects[i]);
            GymElecObject_Play(effects->objects[i], 2);
        }
        effects->state++;
        break;
    case 1:
        effects->wait = GFL_RandomLCAlt(4) + 4;
        GymElecEffects_StartBurst(effects);
        effects->state++;
        break;
    case 2:
        if (effects->wait-- <= 0) {
            effects->state = 1;
        }
        break;
    }
}

static void GymElecBurst_Init(Burst *burst, GymElecObject *obj0, GymElecObject *obj1, GymElecObject *obj2) {
    MI_CpuFill8(burst, 0, sizeof(Burst));
    burst->objects[0] = obj0;
    burst->objects[1] = obj1;
    burst->objects[2] = obj2;
}

static void GymElecBurst_Clear(Burst *burst) {
    MI_CpuFill8(burst, 0, sizeof(Burst));
}

static void GymElecBurst_Update(Burst *burst, const VecFx32 *playerPos) {
    VecFx32 pos;
    int i;
    BOOL done;

    if (burst->active == FALSE) {
        return;
    }
    switch (burst->state) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_GYM_E02);
        for (i = 0; i < burst->count; i++) {
            VEC_Add(&burst->offsets[i], playerPos, &pos);
            pos.y = FX32_CONST(85);
            pos.z = pos.z > FX32_CONST(900) ? FX32_CONST(900) : pos.z < FX32_CONST(160) ? FX32_CONST(160) : pos.z;
            GymElecObject_SetTranslation(burst->objects[i], &pos);
            GymElecObject_SetVisible(burst->objects[i], TRUE);
            GymElecObject_PlayAll(burst->objects[i]);
        }
        burst->state++;
        break;
    case 1:
        done = TRUE;
        for (i = 0; i < burst->count; i++) {
            if (GymElecObject_IsAllFinished(burst->objects[i])) {
                GymElecObject_SetVisible(burst->objects[i], FALSE);
            } else {
                done = FALSE;
            }
        }
        if (done) {
            burst->state++;
        }
        break;
    case 2:
        burst->active = FALSE;
        break;
    }
}

static void GymElecBurst_Start(Burst *burst, const VecFx32 *offsets, s32 count) {
    int i;

    burst->active = TRUE;
    burst->state = 0;
    burst->count = count;
    for (i = 0; i < burst->count; i++) {
        burst->offsets[i] = offsets[i];
    }
}

static BOOL GymElecBurst_IsActive(Burst *burst) {
    return burst->active;
}

static GameEvent *GymElec_CreateProgressEvent(GameSystem *gsys, GymElecFog *fog, GymElecStageObjects *stageObjects,
                                              GymElecEffects *effects, u32 progress) {
    GameEvent *event = GameEvent_Create(gsys, NULL, GymElec_ProgressEvent, sizeof(ProgressEventWork));
    ProgressEventWork *work = GameEvent_GetData(event);

    work->fog = fog;
    work->switchSys = ISS_GetSwitchSys(GameSystem_GetISS(gsys));
    work->progress = progress;
    work->gsys = gsys;
    work->stageObjects = stageObjects;
    work->effects = effects;
    return event;
}

// Fades the fog out and unmutes the next switch of the music, then moves the fog to the next stage
static GameEventReturnCode GymElec_ProgressEvent(GameEvent *event, u32 *state, void *data) {
    ProgressEventWork *work = data;

    switch (*state) {
    case 0:
        switch (work->progress) {
        case 0:
            ISSSwitchSys_ReqSwitchFadeIn(work->switchSys, 1);
            GymElecStageObjects_FadeIn(work->stageObjects, 3, 60);
            break;
        case 1:
            ISSSwitchSys_ReqSwitchFadeIn(work->switchSys, 2);
            GymElecStageObjects_FadeIn(work->stageObjects, 4, 60);
            break;
        case 2:
            break;
        }
        GymElecFog_StartFade(work->fog, 60);
        (*state)++;
        break;
    case 1:
        if (GymElecFog_IsFadeDone(work->fog)) {
            (*state)++;
        }
        break;
    case 2:
        switch (work->progress) {
        case 0:
            GymElecStageObjects_Play(work->stageObjects, 0);
            GymElecFog_SetStage(work->fog, 1);
            break;
        case 1:
            GymElecStageObjects_Play(work->stageObjects, 1);
            GymElecFog_SetStage(work->fog, 2);
            break;
        case 2:
            GymElecStageObjects_Play(work->stageObjects, 2);
            GymElecFog_SetEnabled(work->fog, FALSE);
            break;
        }
        (*state)++;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void GymElecSave_SetProgress(GymElecSave *save, u16 progress) {
    save->progress = progress;
}

static u16 GymElecSave_GetProgress(GymElecSave *save) {
    return save->progress;
}
