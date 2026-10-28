#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/field_fog.h"
#include "field/field_g3d_mapper.h"
#include "field/field_map.h"
#include "field/field_script.h"
#include "field/fieldmap_tcb_camera_zoom.h"
#include "field/gym_gimmick.h"
#include "gfl/calctool.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// Castelia City's gym. Stepping onto a ride sends the player along a 3D curve with a model around them, then to a
// destination. Two objects of the gym show whether event flags 0x109 and 0x10a are set, and the first opens a ride.
// Zone 488 has two rides that are not stepped onto from a direction, and none of the gym's objects or fog

#define GIMMICK_ID 1
#define GIMMICK_STATE_ID 6

#define ZONE_488 488

// The archive of the rides' curves and the gym's models
#define ARC_GYM_INSECT 135

#define FLAG_OBJECT_0 0x109

#define RIDE_COUNT 16
// The ride that FLAG_OBJECT_0 opens
#define RIDE_OBJECT_0 8

// The actors of the gym's scene: the two objects, the model around the player on a ride, and an effect
enum {
    ACTOR_OBJECT_0,
    ACTOR_RIDE = 2,
    ACTOR_EFFECT,
};
#define OBJECT_COUNT 2

typedef struct {
    u8 curveFile;
    // The direction to step onto the ride in
    u8 dir : 4;
    // Given to the script at the end of the ride
    u8 scriptParam : 4;
    // Rides of zone 488, which start from the tile the player stands on
    u8 zone488 : 1;
    // Runs the curve backwards
    u8 reverse : 1;
    // The first of the ride's scripts, which are numbered from it
    u8 script : 6;
    u8 x;
    u8 y;
    u8 z;
    // Where the player ends up, unless all are 0
    u8 destX;
    u8 destY;
    u8 destZ;
} RideEntry;

// Areas of the map, which the map's code reads
typedef struct {
    u8 x;
    u8 z;
    u8 width;
    u8 depth;
    u32 flags;
    u32 value;
} GymArea;

typedef struct {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    GimmickState *gimmickState;
    void *gimmickData;
    FieldExpObjSystem *expObj;
    FieldPlayer *player;
    MMSys *mmSys;
    FieldActor *playerActor;
    FieldCamera *camera;
    EventWork *eventWork;
    u16 heapId;
    u16 tailHeapId;
    // Whether each object's flag is set
    u8 objectFlags[OBJECT_COUNT];
    u8 inZone488;
} GymInsectWork;

typedef struct {
    GymInsectWork *gym;
    const RideEntry *entry;
    G3DCurve *curve;
    u16 frame;
    u16 frameCount;
    s8 step;
    u8 index;
    // The sound player of the ride's sound
    u8 soundPlayer;
    u8 wait;
} RideWork;

typedef struct {
    u8 unk0[0x28];
    FieldActor *playerActor;
    GymInsectWork *gym;
} EffectWork;

static u8 GymInsect_FindRide(GymInsectWork *wk, GameSystem *gsys, u8 dir);
static GameEvent *GymInsect_CreateRideEvent(GymInsectWork *wk, u8 index);
static GameEventReturnCode GymInsect_RideEvent(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode GymInsect_RideEventZone488(GameEvent *event, u32 *state, void *data);
static s16 GymInsect_GetRideSpeedParam(RideWork *ride);
static void GymInsect_StartRide(GymInsectWork *wk, RideWork *ride);
static void GymInsect_EndRide(GymInsectWork *wk, RideWork *ride);
static void GymInsect_UpdateRide(GymInsectWork *wk, RideWork *ride);
static BOOL GymInsect_StepRide(GymInsectWork *wk, RideWork *ride);
static void GymInsect_EffectTask(TCB *tcb, void *data);
static void GymInsect_InitAreas(Field *field);
static void GymInsect_InitObjects(GymInsectWork *wk);
static void GymInsect_FreeObjects(GymInsectWork *wk);
static void GymInsect_UpdateObjects(GymInsectWork *wk);

static const G3DSceneAnimationSetup sEffectAnimations[] = { { 6, 0 } };

// The objects' tiles
static const u8 sObjectPositions[OBJECT_COUNT][3] = { { 8, 0, 4 }, { 25, 20, 15 } };

static const G3DSceneAnimationSetup sObjectAnimations[] = { { 1, 0 }, { 2, 0 }, { 3, 0 } };

static const G3DSceneResourceSetup sResources[] = {
    { ARC_GYM_INSECT, 8, 0 },  { ARC_GYM_INSECT, 9, 0 },  { ARC_GYM_INSECT, 10, 0 }, { ARC_GYM_INSECT, 11, 0 },
    { ARC_GYM_INSECT, 12, 0 }, { ARC_GYM_INSECT, 13, 0 }, { ARC_GYM_INSECT, 14, 0 },
};

static const G3DSceneActorSetup sActors[] = {
    { 0, 0, 0, 0, sObjectAnimations, NELEMS(sObjectAnimations) },
    { 0, 0, 0, 0, sObjectAnimations, NELEMS(sObjectAnimations) },
    { 4, 0, 4, 0, NULL, 0 },
    { 5, 0, 5, 0, sEffectAnimations, NELEMS(sEffectAnimations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

static const u8 sFogTable[32] = {
    0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c, 0x21, 0x14, 0x16, 0x16, 0x16, 0x16, 0x39, 0x3d,
    0x42, 0x46, 0x4a, 0x4e, 0x52, 0x56, 0x5a, 0x5e, 0x63, 0x67, 0x6b, 0x6f, 0x73, 0x77, 0x7b, 0x7f,
};

// clang-format off
static const GymArea sAreas[] = {
    { 3, 13, 3, 3, 0x01810001, 0x140000 },  { 10, 1, 3, 4, 0x01810001, 0x140000 },
    { 19, 3, 3, 4, 0x01810001, 0x140000 },  { 24, 13, 3, 4, 0x01810001, 0x140000 },
    { 1, 13, 13, 5, 0x00800000, 0x140000 }, { 9, 1, 5, 12, 0x00800000, 0x140000 },
    { 18, 1, 5, 12, 0x00800000, 0x140000 }, { 23, 9, 3, 4, 0x00800000, 0x140000 },
    { 18, 13, 13, 5, 0x00800000, 0x140000 },
};

// clang-format on

// clang-format off
static const RideEntry sRides[RIDE_COUNT] = {
    { 0, DIR_UP, 1, FALSE, FALSE, 0, 16, 0, 11, 16, 10, 11 },
    { 0, DIR_UP, 1, FALSE, TRUE, 0, 16, 10, 11, 16, 0, 11 },
    { 1, DIR_LEFT, 1, FALSE, FALSE, 0, 16, 0, 11, 3, 10, 3 },
    { 1, DIR_UP, 3, FALSE, TRUE, 0, 3, 10, 3, 16, 0, 11 },
    { 2, DIR_UP, 1, FALSE, FALSE, 0, 26, 0, 4, 25, 20, 15 },
    { 2, DIR_UP, 1, FALSE, TRUE, 0, 25, 20, 15, 26, 0, 4 },
    { 3, DIR_LEFT, 1, FALSE, FALSE, 0, 26, 0, 4, 11, 20, 3 },
    { 3, DIR_UP, 3, FALSE, TRUE, 0, 11, 20, 3, 26, 0, 4 },
    { 4, DIR_UP, 0, FALSE, FALSE, 1, 8, 0, 4, 0, 0, 0 },
    { 4, 9, 9, TRUE, TRUE, 0, 9, 0, 13, 8, 0, 4 },
    { 5, DIR_UP, 1, FALSE, FALSE, 0, 28, 10, 3, 20, 20, 5 },
    { 5, DIR_UP, 1, FALSE, TRUE, 0, 20, 20, 5, 28, 10, 3 },
    { 6, DIR_LEFT, 1, FALSE, FALSE, 0, 11, 20, 3, 4, 20, 14 },
    { 6, DIR_UP, 3, FALSE, TRUE, 0, 4, 20, 14, 11, 20, 3 },
    { 7, DIR_RIGHT, 0, FALSE, FALSE, 1, 4, 20, 14, 0, 0, 0 },
    { 7, 9, 9, TRUE, TRUE, 1, 15, 0, 20, 4, 20, 14 },
};

// clang-format on

void GymInsect_Init(Field *field) {
    GymInsectWork *wk = Field_AllocGimmickWorkBlock(field, GIMMICK_ID, Field_GetHeapID(field), sizeof(GymInsectWork));
    EventWork *eventWork;
    s32 i;

    wk->gsys = Field_GetGameSystem(field);
    wk->gameData = GSYS_GetGameData(wk->gsys);
    wk->field = field;
    wk->gimmickState = GameData_GetGimmickState(wk->gameData);
    wk->gimmickData = GimmickState_GetUserData(wk->gimmickState, GIMMICK_STATE_ID);
    wk->heapId = HEAPID_FIELDMAP;
    wk->tailHeapId = HEAPID_TAIL(HEAPID_FIELDMAP);
    wk->expObj = Field_GetExpObjSystem(field);
    wk->player = Field_GetPlayer(field);
    wk->mmSys = GameData_GetMMSys(wk->gameData);
    wk->playerActor = FieldPlayer_GetActor(wk->player);
    wk->camera = Field_GetCameraSystem(field);
    wk->eventWork = GameData_GetEventWork(wk->gameData);
    wk->inZone488 = Field_GetPlayerStateZoneID(field) == ZONE_488;
    eventWork = GameData_GetEventWork(wk->gameData);
    for (i = 0; i < OBJECT_COUNT; i++) {
        wk->objectFlags[i] = EventWork_FlagGet(eventWork, FLAG_OBJECT_0 + i);
    }
    GymInsect_InitAreas(field);
    GymInsect_InitObjects(wk);
    if (wk->inZone488 == FALSE) {
        FieldFog *fog = Field_GetFog(field);

        FieldFog_SetAlphaMode(fog, 0);
        FieldFog_SetAlpha(fog, 31);
        FieldFog_SetTable(fog, sFogTable);
        FieldFog_SetDepthShift(fog, 6);
        FieldFog_SetOffset(fog, 0x7eef);
        FieldFog_SetEnabled(fog, TRUE);
        FieldFog_Flush(fog);
    }
}

void GymInsect_End(Field *field) {
    GymInsect_FreeObjects(Field_GetGimmickWorkBlock(field, GIMMICK_ID));
    Field_DeleteGimmickWorkBlock(field, GIMMICK_ID);
}

void GymInsect_Update(Field *field) {
    GymInsect_UpdateObjects(Field_GetGimmickWorkBlock(field, GIMMICK_ID));
}

// A ride starts from the tile two steps ahead of the player in the ride's direction, or in zone 488 from the tile
// one step ahead
static u8 GymInsect_FindRide(GymInsectWork *wk, GameSystem *gsys, u8 dir) {
    int i;
    const RideEntry *entry;
    GridPos ahead;
    GridPos pos;

    FldAct_GetGPos(wk->playerActor, &pos);
    AdjusGridXZByDir(dir, &pos.x, &pos.z, 1);
    ahead = pos;
    AdjusGridXZByDir(dir, &ahead.x, &ahead.z, 1);
    for (i = 0; i < NELEMS(sRides); i++) {
        entry = &sRides[i];
        if (wk->inZone488 != entry->zone488) {
            continue;
        }
        if (i == RIDE_OBJECT_0 && EventWork_FlagGet(wk->eventWork, FLAG_OBJECT_0) == FALSE) {
            continue;
        }
        if (entry->zone488 == FALSE) {
            if (entry->dir == dir && entry->x == ahead.x && entry->y == ahead.y && entry->z == ahead.z) {
                break;
            }
        } else {
            if (entry->x == pos.x && entry->y == pos.y && entry->z == pos.z) {
                break;
            }
        }
    }
    if (i < NELEMS(sRides)) {
        return i;
    }
    return RIDE_COUNT;
}

GameEvent *GymInsect_CheckRide(GameSystem *gsys, u8 dir) {
    GymInsectWork *wk = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_ID);
    u8 index = GymInsect_FindRide(wk, gsys, dir);

    if (index < RIDE_COUNT) {
        return GymInsect_CreateRideEvent(wk, index);
    }
    return NULL;
}

BOOL GymInsect_IsRideAhead(GameSystem *gsys, u8 dir) {
    return GymInsect_FindRide(Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_ID), gsys, dir) < RIDE_COUNT;
}

// Unpauses an object's animations, for a script
void GymInsect_PlayObject(Field *field, u8 object) {
    GymInsectWork *wk = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    int i = 0;
    u16 actor = ACTOR_OBJECT_0 + object;

    while (i < NELEMS(sObjectAnimations)) {
        FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(wk->expObj, 0, actor, i), FALSE);
        i++;
    }
}

static GameEvent *GymInsect_CreateRideEvent(GymInsectWork *wk, u8 index) {
    const RideEntry *entry = &sRides[index];
    GameEvent *event;
    RideWork *ride;

    if (entry->zone488 == FALSE) {
        event = GameEvent_Create(wk->gsys, NULL, GymInsect_RideEvent, sizeof(RideWork));
    } else {
        event = GameEvent_Create(wk->gsys, NULL, GymInsect_RideEventZone488, sizeof(RideWork));
    }
    ride = GameEvent_GetData(event);
    ride->gym = wk;
    ride->index = index;
    ride->entry = entry;
    return event;
}

static GameEventReturnCode GymInsect_RideEvent(GameEvent *event, u32 *state, void *data) {
    RideWork *ride = data;
    GymInsectWork *wk = ride->gym;
    ScriptWork *script;

    switch (*state) {
    case 0:
        if (IsAllActorAcmdFinished(wk->playerActor)) {
            DisableAllActorsMovement(wk->mmSys);
            EventScriptCall_Start(event, 1, NULL, 0, wk->tailHeapId);
            (*state)++;
        }
        break;
    case 1:
        FieldCameraZoomTCB_CreateLinear(wk->field, 20, FX32_CONST(200));
        GymInsect_StartRide(wk, ride);
        (*state)++;
        break;
    case 2:
        if (ride->entry->script != 0 && ride->frame == 20) {
            FieldFadeTCB_Start(wk->gsys, wk->field, FIELD_FADE_TYPE_OUT, 0, 0);
        }
        if (GymInsect_StepRide(wk, ride)) {
            GymInsect_EndRide(wk, ride);
            (*state)++;
        }
        break;
    case 3:
        if (ride->entry->script != 0) {
            if (Field_GetFadeFlag(wk->field)) {
                break;
            }
            *state = 0xff;
        } else {
            (*state)++;
        }
        script = EventScriptCall_Start(event, ride->entry->script + 2, NULL, 0, wk->tailHeapId);
        ScriptWork_SetParams(script, ride->index, ride->entry->scriptParam, 0, 0);
        break;
    case 4:
        FieldCameraZoomTCB_Create(wk->field, 10, -FX32_CONST(200));
        ride->wait = 10;
        (*state)++;
        break;
    case 5:
        if (--ride->wait == 0) {
            (*state)++;
        }
        break;
    default:
        EnableAllActorsMovement(wk->mmSys);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode GymInsect_RideEventZone488(GameEvent *event, u32 *state, void *data) {
    RideWork *ride = data;
    GymInsectWork *wk = ride->gym;

    switch (*state) {
    case 0:
        if (IsAllActorAcmdFinished(wk->playerActor)) {
            DisableAllActorsMovement(wk->mmSys);
            EventScriptCall_Start(event, ride->entry->script + 1, NULL, 0, wk->tailHeapId);
            (*state)++;
        }
        break;
    case 1:
        SetActorHidden(wk->playerActor, TRUE);
        GymInsect_StartRide(wk, ride);
        FieldCamera_DisableDelay(wk->camera);
        FieldCamera_CalcTransform(wk->camera, 0);
        FieldCamera_EnableDelay(wk->camera);
        FieldFadeTCB_Start(wk->gsys, wk->field, FIELD_FADE_TYPE_IN_QUICK, 0, 0);
        (*state)++;
        break;
    case 2:
        if (GymInsect_StepRide(wk, ride)) {
            GymInsect_EndRide(wk, ride);
            FldAct_SetShadowGroup(FieldPlayer_GetActor(wk->player), 1);
            (*state)++;
        }
        break;
    case 3:
        if (Field_GetFadeFlag(wk->field) == FALSE) {
            EventScriptCall_Start(event, ride->entry->script + 4, NULL, 0, wk->tailHeapId);
            (*state)++;
        }
        break;
    default:
        EnableAllActorsMovement(wk->mmSys);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// The ride's sound follows its speed, from the distance to the next frame of the curve
static s16 GymInsect_GetRideSpeedParam(RideWork *ride) {
    s16 next = ride->frame + ride->step;
    VecFx32 pos;
    VecFx32 nextPos;
    VecFx32 diff;

    if (next < 0) {
        next = 0;
    } else if (next > ride->frameCount - 1) {
        next = ride->frameCount - 1;
    }
    GFL_G3DCurveGetNowTranslationLoop(ride->curve, &pos, ride->frame);
    GFL_G3DCurveGetNowTranslationLoop(ride->curve, &nextPos, next);
    VEC_Subtract(&nextPos, &pos, &diff);
    return FX_Whole(FX_Mul(VEC_Mag(&diff) - FX32_CONST(5), FX32_CONST(70)));
}

static void GymInsect_StartRide(GymInsectWork *wk, RideWork *ride) {
    ride->curve = GFL_G3DCurveLoadFileAll(wk->tailHeapId, ARC_GYM_INSECT, ride->entry->curveFile);
    ride->frameCount = GFL_G3DCurveGetFrameCount(ride->curve);
    if (ride->entry->reverse) {
        ride->step = -1;
        ride->frame = ride->frameCount - 1;
    } else {
        ride->frame = 0;
        ride->step = 1;
    }
    ride->soundPlayer = GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_M02);
    GFL_SndSEPlay(SEQ_SE_GYM_M02);
    GFL_SndPlayerSetParams(ride->soundPlayer, -1, GymInsect_GetRideSpeedParam(ride), -1);
    GymInsect_UpdateRide(wk, ride);
}

static void GymInsect_EndRide(GymInsectWork *wk, RideWork *ride) {
    VecFx32 pos;

    if (ride->entry->destX + ride->entry->destY + ride->entry->destZ != 0) {
        ConvGXZToVector(ride->entry->destX, ride->entry->destZ, &pos);
        pos.y = ride->entry->destY * FX32_CONST(16);
        FieldPlayer_SetWPos(wk->player, &pos);
    }
    GFL_SndStop();
    FieldExpObj_SetActorHidden(wk->expObj, 0, ACTOR_RIDE, TRUE);
    GFL_G3DCurveFree(ride->curve);
}

// Moves the player and the ride's model to the curve's current frame
static void GymInsect_UpdateRide(GymInsectWork *wk, RideWork *ride) {
    VecFx32 pos = { 0, 0, 0 };
    VecFx32 rotation;
    VecFx32 scale;
    SRTMatrix *matrix;

    if (GFL_G3DCurveGetNowTranslationLoop(ride->curve, &pos, ride->frame) == FALSE) {
        return;
    }
    pos.x += FX32_CONST(256);
    pos.z += FX32_CONST(256);
    FieldPlayer_SetWPos(wk->player, &pos);
    GFL_SndPlayerSetParams(ride->soundPlayer, -1, GymInsect_GetRideSpeedParam(ride), -1);
    {
        VecFx32 rotation = { 0, 0, 0 };
        VecFx32 scale = { FX32_ONE, FX32_ONE, FX32_ONE };

        matrix = FieldExpObj_GetActorMatrixPtr(wk->expObj, 0, ACTOR_RIDE);
        matrix->translation = pos;
        if (GFL_G3DCurveGetNowRotationLoop(ride->curve, &rotation, ride->frame)) {
            MAT3_RotationEulerZYX(FX_DEG_TO_IDX(rotation.x), FX_DEG_TO_IDX(rotation.y), FX_DEG_TO_IDX(rotation.z),
                                  &matrix->rotation);
        }
        if (GFL_G3DCurveGetNowScaleLoop(ride->curve, &scale, ride->frame)) {
            if (scale.x + scale.y + scale.z > 0) {
                FieldExpObj_SetActorHidden(wk->expObj, 0, ACTOR_RIDE, FALSE);
            } else {
                FieldExpObj_SetActorHidden(wk->expObj, 0, ACTOR_RIDE, TRUE);
            }
            matrix->scale = scale;
        }
    }
}

// Returns TRUE once the ride has reached the end of its curve
static BOOL GymInsect_StepRide(GymInsectWork *wk, RideWork *ride) {
    GymInsect_UpdateRide(wk, ride);
    ride->frame += ride->step;
    if (ride->step < 0) {
        if (ride->frame == 0) {
            return TRUE;
        }
    } else if (ride->frame >= ride->frameCount) {
        return TRUE;
    }
    return FALSE;
}

// Plays the effect on the tile ahead of the player, turned the way the player faces, for a script
void GymInsect_ShowEffect(Field *field) {
    GymInsectWork *wk = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    TCBManager *tcbManager = Field_GetTCBMgr(field);
    EffectWork *effect =
        GFL_HeapAllocate(HEAPID_TAIL(Field_GetHeapID(field)), sizeof(EffectWork), TRUE, "gym_insect.c", 656);
    SRTMatrix *matrix;
    u32 dir;
    s32 angle;
    int i;

    effect->gym = wk;
    effect->playerActor = FieldPlayer_GetActor(Field_GetPlayer(field));
    matrix = FieldExpObj_GetActorMatrixPtr(wk->expObj, 0, ACTOR_EFFECT);
    dir = GetActorFaceDir(effect->playerActor);
    CopyActorWPos(effect->playerActor, &matrix->translation);
    ExpandVecInGridDir(dir, &matrix->translation, FX32_CONST(32));
    angle = 0;
    if (dir == DIR_LEFT) {
        angle = 90;
    } else if (dir == DIR_RIGHT) {
        angle = 270;
    }
    MAT3_RotationEulerZYX(0, FX_DEG_TO_IDX(FX32_CONST(angle)), 0, &matrix->rotation);
    FieldExpObj_SetActorHidden(wk->expObj, 0, ACTOR_EFFECT, FALSE);
    for (i = 0; i < NELEMS(sEffectAnimations); i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(wk->expObj, 0, ACTOR_EFFECT, i);

        FieldExpObj_SetAnmFrame(wk->expObj, 0, ACTOR_EFFECT, i, 0);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
    GFL_SndSEPlay(SEQ_SE_GYM_M01);
    GFL_TCBMgrAddTask(tcbManager, GymInsect_EffectTask, effect, 0);
}

static void GymInsect_EffectTask(TCB *tcb, void *data) {
    EffectWork *effect = data;
    GymInsectWork *wk = effect->gym;
    int i;

    for (i = 0; i < NELEMS(sEffectAnimations); i++) {
        if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(wk->expObj, 0, ACTOR_EFFECT, i)) == FALSE) {
            return;
        }
    }
    FieldExpObj_SetActorHidden(wk->expObj, 0, ACTOR_EFFECT, TRUE);
    for (i = 0; i < NELEMS(sEffectAnimations); i++) {
        FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(wk->expObj, 0, ACTOR_EFFECT, i), TRUE);
    }
    GFL_HeapFree(effect);
    GFL_TCBRemove(tcb);
}

static void GymInsect_InitAreas(Field *field) {
    int i;
    HeightExList *list = FieldG3DMapper_GetHeightEx(Field_GetG3DMapper(field));

    for (i = 0; i < NELEMS(sAreas); i++) {
        HeightEx_SetArea(i, sAreas[i].x, sAreas[i].z, sAreas[i].width, sAreas[i].depth, sAreas[i].value,
                         sAreas[i].flags, list);
    }
}

// Loads the scene, hides the ride's model and the effect, and puts each object at the start or the end of its
// animations, as its flag says
static void GymInsect_InitObjects(GymInsectWork *wk) {
    int i;
    s32 j;
    u8 object;
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    const u8 *tile;

    if (wk->inZone488) {
        return;
    }
    LoadFieldExpandObjData(wk->expObj, &sSceneSetup, 0);
    FieldExpObj_GetActorMatrixPtr(wk->expObj, 0, ACTOR_RIDE);
    FieldExpObj_SetActorCulling(wk->expObj, 0, ACTOR_RIDE, 1);
    FieldExpObj_SetActorHidden(wk->expObj, 0, ACTOR_RIDE, TRUE);
    FieldExpObj_GetActorMatrixPtr(wk->expObj, 0, ACTOR_EFFECT);
    FieldExpObj_SetActorCulling(wk->expObj, 0, ACTOR_EFFECT, 1);
    FieldExpObj_SetActorHidden(wk->expObj, 0, ACTOR_EFFECT, TRUE);
    for (i = 0; i < NELEMS(sEffectAnimations); i++) {
        anm = FieldExpObj_GetAnmInfo(wk->expObj, 0, ACTOR_EFFECT, i);
        FieldExpObj_SetAnm(wk->expObj, 0, ACTOR_EFFECT, i, TRUE);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
    }
    for (j = 0; j < OBJECT_COUNT; j++) {
        object = ACTOR_OBJECT_0 + j;
        matrix = FieldExpObj_GetActorMatrixPtr(wk->expObj, 0, object);
        FieldExpObj_SetActorCulling(wk->expObj, 0, object, 1);
        tile = sObjectPositions[j];
        matrix->translation.x = FX32_CONST(tile[0] * 16) + FX32_CONST(8);
        matrix->translation.y = FX32_CONST(tile[1] * 16);
        matrix->translation.z = FX32_CONST(tile[2] * 16) + FX32_CONST(8);
        for (i = 0; i < NELEMS(sObjectAnimations); i++) {
            anm = FieldExpObj_GetAnmInfo(wk->expObj, 0, object, i);
            FieldExpObj_SetAnm(wk->expObj, 0, object, i, TRUE);
            FieldExpObjAnm_SetLooped(anm, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
            FieldExpObj_SetAnmFrame(wk->expObj, 0, object, i, FieldExpObjAnm_GetLastFrame(anm) * wk->objectFlags[j]);
        }
    }
}

static void GymInsect_FreeObjects(GymInsectWork *wk) {
    if (wk->inZone488 == FALSE) {
        FieldExpObj_FreeScene(wk->expObj, 0);
    }
}

static void GymInsect_UpdateObjects(GymInsectWork *wk) {
    if (wk->inZone488 == FALSE) {
        FieldExpObj_StepAllAnimations(wk->expObj);
    }
}
