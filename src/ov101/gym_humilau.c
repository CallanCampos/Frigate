#include "types.h"
#include "constants/arc.h"
#include "constants/directions.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/field_g3d_mapper.h"
#include "field/field_player.h"
#include "field/field_task.h"
#include "field/gym_humilau.h"
#include "gfl/calctool.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "save/gimmick_savedata.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The Humilau City gym's floating platforms, named after the zone: the overlay has no string. Eleven platforms float
// on the water, each a cross of five squares. Stepping from a ride spot onto the platform in front starts a ride: the
// platform carries the player along its lane until the lane ends or it hits another platform, which it pushes on by
// what is left of the lane. The platforms' positions are saved, and the gym's areas of the map follow the platform
// that the player stands on and the ones next to it. The names are ours

#define GIMMICK_WORK_ID 1
#define GIMMICK_ID 38

#define PLATFORM_COUNT 11
#define RIDE_COUNT 37
#define EFFECT_COUNT 9
// The expanded objects' actors after the platforms
#define ACTOR_STOP_SPLASH 11
#define ACTOR_HIT_SPLASH 12
#define ACTOR_WAKE_FIRST 13
#define ACTOR_WAKE_END 19
#define ACTOR_RIDE_SPLASH 19
#define ACTOR_PUSH_SPLASH 20
#define ACTOR_LEADER_SPLASH 21
#define ACTOR_LEADER_WAVES 22

// The platforms' models' animations: 0 floats, looping; 1 tilts the platform under the player's step
#define ANM_FLOAT 0
#define ANM_TILT 1

// The platforms float this far below the ground
#define PLATFORM_Y FX32_CONST(-4)

// No platform in a slot of the state
#define PLATFORM_NONE 0xff

// The ride whose lane runs back down to the gym's entrance; once it has been ridden, the platforms that were never
// moved start at the second set of positions
#define RIDE_SHORTCUT 3

// The player's hop in place before a ride, one per direction
#define ACMD_HOP_FIRST 0x38

// Where the player stands: on the platform of the last ride, on the one next to it, or on neither
enum {
    STANDING_RIDE_PLATFORM,
    STANDING_NEXT_PLATFORM,
    STANDING_NONE,
};

// The kinds of splash at the start and end of a ride
enum {
    SPLASH_STOP,
    SPLASH_RIDE,
    SPLASH_HIT,
};

// What the save keeps of the gym: the platforms' positions, 0 for one that has never moved, and the platforms that
// the player stood on and next to
typedef struct {
    fx32 x[PLATFORM_COUNT];
    fx32 z[PLATFORM_COUNT];
    BOOL saved;
    u8 platforms[2];
} HumilauGymState;

// An effect actor whose animation plays until it ends, then hides
typedef struct {
    BOOL active;
    FieldExpObjSystem *expObj;
    u16 actor;
    u8 anmCount;
} HumilauGymEffect;

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    HumilauGymState *state;
    HumilauGymEffect effects[EFFECT_COUNT];
    int standing;
    BOOL tilting;
    u16 tiltPlatform;
    u8 areaCount;
} HumilauGymWork;

// A ride spot: a grid square, the direction of the platform in front of it and the length of its lane
typedef struct {
    u8 x;
    u8 z;
    u8 dir;
    u8 length;
} HumilauGymRide;

// A platform's move from one position to another, bobbing as it goes
typedef struct {
    int frame;
    int frames;
    VecFx32 start;
    VecFx32 end;
    s16 amplitude;
    u32 period;
    u32 counter;
    BOOL firstBob;
} HumilauGymMover;

// The platform's rocking in the player's direction after it stops
typedef struct {
    VecFx32 pos;
    s16 amplitude;
    u32 period;
    u32 counter;
} HumilauGymRock;

typedef struct {
    GameSystem *gsys;
    Field *field;
    FieldPlayer *player;
    HumilauGymMover mover;
    HumilauGymRock rock;
    u8 ride;
    u16 platform;
    BOOL hit;
    u16 hitPlatform;
    u16 hitLength;
    s16 wait;
    FieldCamera *camera;
    void *cameraBind;
} HumilauGymRideEvent;

static void GymHumilau_SetupObjects(HumilauGymWork *work);
static void GymHumilau_SavePlatforms(HumilauGymWork *work);
static void GymHumilau_StepAnimations(HumilauGymWork *work);
static GameEvent *GymHumilau_CreateRideEvent(GameEvent *parent, GameSystem *gsys, u8 ride, u16 platform);
static GameEventReturnCode GymHumilau_RideEventCallback(GameEvent *event, u32 *state, void *data);
static BOOL GymHumilau_FindPlatform(FieldExpObjSystem *expObj, fx32 x, fx32 z, u16 *platform);
static BOOL GymHumilau_GetRidePlatformPos(u8 ride, int *x, int *z);
static void GymHumilau_AddNextAreas(HumilauGymWork *work);
static void GymHumilau_RestoreAreas(HumilauGymWork *work);
static void GymHumilau_ClearSlots(HumilauGymWork *work);
static int GymHumilau_GetRideLength(HumilauGymRideEvent *ev, u8 ride);
static void GymHumilau_StartRide(HumilauGymRideEvent *ev, int frames, VecFx32 *target);
static BOOL GymHumilau_UpdateRide(HumilauGymRideEvent *ev);
static BOOL GymHumilau_StartPush(HumilauGymRideEvent *ev);
static BOOL GymHumilau_UpdatePush(HumilauGymRideEvent *ev);
static void GymHumilau_StartRock(HumilauGymRideEvent *ev);
static BOOL GymHumilau_UpdateRock(HumilauGymRideEvent *ev);
static void GymHumilau_StartTilt(HumilauGymWork *work, u8 slot);
static BOOL GymHumilau_IsTiltFinished(HumilauGymWork *work);
static void GymHumilau_EndTilt(HumilauGymWork *work);
static void GymHumilau_PlaySplash(HumilauGymRideEvent *ev, int kind);
static void GymHumilau_PlayWake(HumilauGymRideEvent *ev, u16 platform);
static void GymHumilau_PlayStopSplash(HumilauGymRideEvent *ev, u16 platform, BOOL hit);
static int GymHumilau_GetStanding(HumilauGymWork *work, int x, int z);
static void GymHumilau_ClearAreas(HumilauGymWork *work);
static void GymHumilau_AddRideAreas(HumilauGymWork *work, u8 ride);
static void GymHumilau_AddPlatformAreas(HumilauGymWork *work, u16 platform);
static void GymHumilau_StartEffect(HumilauGymWork *work, u16 actor, u8 anmCount);
static void GymHumilau_EffectTask(TCB *tcb, void *data);
static void GymHumilauState_SetPlatformPos(HumilauGymState *state, fx32 x, fx32 z, u16 platform);
static void GymHumilauState_GetPlatformPos(HumilauGymState *state, fx32 *x, fx32 *z, u16 platform);
static void GymHumilauState_SetSaved(HumilauGymState *state);
static BOOL GymHumilauState_IsSaved(HumilauGymState *state);
static void GymHumilauState_SetSlot(HumilauGymState *state, int slot, u8 platform);
static u8 GymHumilauState_GetSlot(HumilauGymState *state, int slot);
static void GymHumilau_SetShortcutUsed(GimmickState *gimmickState, BOOL used);
static BOOL GymHumilau_IsShortcutUsed(GimmickState *gimmickState);

static const G3DSceneAnimationSetup sHumilauPlatformAnimations[] = { { 1, 0 }, { 2, 0 } };
static const G3DSceneAnimationSetup sHitSplashAnimations[] = { { 7, 0 }, { 8, 0 } };
static const G3DSceneAnimationSetup sWakeAnimations[] = { { 10, 0 }, { 11, 0 } };
static const G3DSceneAnimationSetup sStopSplashAnimations[] = { { 4, 0 }, { 5, 0 } };
static const G3DSceneAnimationSetup sLeaderSplashAnimations[] = { { 19, 0 }, { 20, 0 } };
static const G3DSceneAnimationSetup sRideSplashAnimations[] = { { 13, 0 }, { 14, 0 } };
static const G3DSceneAnimationSetup sPushSplashAnimations[] = { { 16, 0 }, { 17, 0 } };
// Not used. Not static: the ROM keeps it, in a section of its own ahead of the other tables, and an unused static would
// be dropped
const G3DSceneAnimationSetup GymHumilau_UnusedAnimations[] = { { 3, 0 }, { 2, 0 } };
static const G3DSceneAnimationSetup sLeaderWavesAnimations[] = { { 22, 0 }, { 23, 0 }, { 24, 0 } };

static const G3DSceneResourceSetup sGymHumilauResources[25];
static const G3DSceneActorSetup sGymHumilauActors[23];

static const G3DSceneSetup sGymHumilauSceneSetup = {
    sGymHumilauResources,
    NELEMS(sGymHumilauResources),
    sGymHumilauActors,
    NELEMS(sGymHumilauActors),
};

static const HumilauGymRide sRides[RIDE_COUNT] = {
    { 5, 10, DIR_RIGHT, 3 },  { 9, 10, DIR_LEFT, 3 },    { 10, 11, DIR_DOWN, 15 }, { 16, 11, DIR_DOWN, 51 },
    { 22, 11, DIR_DOWN, 12 }, { 25, 11, DIR_DOWN, 3 },   { 25, 15, DIR_UP, 3 },    { 5, 19, DIR_RIGHT, 6 },
    { 12, 19, DIR_LEFT, 6 },  { 7, 21, DIR_UP, 12 },     { 8, 22, DIR_RIGHT, 15 }, { 24, 22, DIR_LEFT, 15 },
    { 22, 24, DIR_UP, 12 },   { 10, 27, DIR_UP, 15 },    { 6, 29, DIR_DOWN, 9 },   { 4, 31, DIR_RIGHT, 19 },
    { 24, 31, DIR_LEFT, 19 }, { 9, 35, DIR_DOWN, 3 },    { 25, 35, DIR_DOWN, 3 },  { 4, 37, DIR_RIGHT, 22 },
    { 6, 39, DIR_UP, 9 },     { 9, 39, DIR_UP, 3 },      { 25, 39, DIR_UP, 3 },    { 19, 47, DIR_DOWN, 6 },
    { 25, 47, DIR_DOWN, 3 },  { 14, 49, DIR_RIGHT, 12 }, { 25, 51, DIR_UP, 3 },    { 19, 54, DIR_UP, 6 },
    { 13, 56, DIR_DOWN, 6 },  { 25, 56, DIR_DOWN, 3 },   { 5, 58, DIR_RIGHT, 21 }, { 25, 60, DIR_UP, 3 },
    { 13, 63, DIR_UP, 6 },    { 16, 63, DIR_UP, 51 },    { 5, 64, DIR_RIGHT, 6 },  { 12, 64, DIR_LEFT, 6 },
    { 5, 13, DIR_RIGHT, 21 },
};

void GymHumilau_Init(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    HumilauGymWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(HumilauGymWork));

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    work->state = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_ID);
    work->standing = STANDING_NONE;
    work->areaCount = 0;
    GymHumilau_SetupObjects(work);
    if (GymHumilauState_IsSaved(work->state) == TRUE) {
        GymHumilau_RestoreAreas(work);
    }
}

void GymHumilau_End(Field *field) {
    HumilauGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    GymHumilau_ClearSlots(work);
    GymHumilau_SavePlatforms(work);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void GymHumilau_Update(Field *field) {
    HumilauGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    FieldPlayer *player = Field_GetPlayer(field);

    if (work->tilting == TRUE && GymHumilau_IsTiltFinished(work) == TRUE) {
        GymHumilau_EndTilt(work);
    }
    if (work->standing != STANDING_NONE) {
        FieldActor *actor = FieldPlayer_GetActor(player);
        s16 x = FldAct_GetInitGPosX(actor);

        switch (GymHumilau_GetStanding(work, x, FldAct_GetInitGPosZ(actor))) {
        case STANDING_RIDE_PLATFORM:
            if (work->standing == STANDING_NEXT_PLATFORM) {
                work->standing = STANDING_RIDE_PLATFORM;
                GymHumilau_StartTilt(work, 1);
                GFL_SndSEPlay(SEQ_SE_SW_MIZUGYM_02);
            }
            break;
        case STANDING_NEXT_PLATFORM:
            if (work->standing == STANDING_RIDE_PLATFORM) {
                work->standing = STANDING_NEXT_PLATFORM;
                GymHumilau_StartTilt(work, 0);
                GFL_SndSEPlay(SEQ_SE_SW_MIZUGYM_02);
            }
            break;
        case STANDING_NONE:
            if (work->standing == STANDING_RIDE_PLATFORM) {
                GymHumilau_StartTilt(work, 0);
            } else {
                GymHumilau_StartTilt(work, 1);
            }
            work->standing = STANDING_NONE;
            GymHumilau_ClearAreas(work);
            GFL_SndSEPlay(SEQ_SE_SW_MIZUGYM_02);
            break;
        }
    }
    GymHumilau_StepAnimations(work);
}

GameEvent *GymHumilau_CheckRide(GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    HumilauGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    FieldPlayer *player = Field_GetPlayer(field);
    s16 x, y, z;
    u16 platform = 0;
    u8 i;
    int platformX, platformZ;

    FieldPlayer_GetGPos(player, &x, &y, &z);
    for (i = 0; i < RIDE_COUNT; i++) {
        if (x == sRides[i].x && z == sRides[i].z) {
            GymHumilau_GetRidePlatformPos(i, &platformX, &platformZ);
            if (GymHumilau_FindPlatform(work->expObj, platformX * FX32_CONST(16) + FX32_CONST(8),
                                        platformZ * FX32_CONST(16) + FX32_CONST(8), &platform)) {
                return GymHumilau_CreateRideEvent(NULL, gsys, i, platform);
            }
        }
    }
    return NULL;
}

void GymHumilau_PlayLeaderSplash(GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    MMSys *mmSys = Field_GetActorSystem(field);
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    VecFx32 pos;
    SRTMatrix *matrix;
    int i;

    CopyActorWPos(FindFieldActor(mmSys, 0), &pos);
    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, ACTOR_LEADER_SPLASH);
    matrix->translation.x = pos.x;
    matrix->translation.z = pos.z;
    FieldExpObj_SetActorHidden(expObj, 0, ACTOR_LEADER_SPLASH, FALSE);
    for (i = 0; i < 2; i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(expObj, 0, ACTOR_LEADER_SPLASH, i);

        FieldExpObj_SetAnm(expObj, 0, ACTOR_LEADER_SPLASH, i, TRUE);
        FieldExpObj_SetAnmFrame(expObj, 0, ACTOR_LEADER_SPLASH, i, 0);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
    GFL_SndSEPlay(SEQ_SE_SW_SHIZUI);
}

void GymHumilau_PlayLeaderWaves(GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    MMSys *mmSys = Field_GetActorSystem(field);
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    VecFx32 pos;
    SRTMatrix *matrix;
    int i;

    CopyActorWPos(FindFieldActor(mmSys, 0), &pos);
    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, ACTOR_LEADER_WAVES);
    matrix->translation.x = pos.x;
    matrix->translation.z = pos.z;
    FieldExpObj_SetActorHidden(expObj, 0, ACTOR_LEADER_WAVES, FALSE);
    for (i = 0; i < 3; i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(expObj, 0, ACTOR_LEADER_WAVES, i);

        FieldExpObj_SetAnm(expObj, 0, ACTOR_LEADER_WAVES, i, TRUE);
        FieldExpObj_SetAnmFrame(expObj, 0, ACTOR_LEADER_WAVES, i, 0);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}

static void GymHumilau_SetupObjects(HumilauGymWork *work) {
    BOOL shortcutUsed =
        GymHumilau_IsShortcutUsed(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(work->field))));
    u8 i = 0;
    u8 actor;
    u16 splash;
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    fx32 x, z;
    int j;

    LoadFieldExpandObjData(work->expObj, &sGymHumilauSceneSetup, 0);
    {
        VecFx32 startPositions[PLATFORM_COUNT] = {
            { FX32_CONST(168), 0, FX32_CONST(1032) }, { FX32_CONST(120), 0, FX32_CONST(936) },
            { FX32_CONST(264), 0, FX32_CONST(936) },  { FX32_CONST(264), 0, FX32_CONST(792) },
            { FX32_CONST(360), 0, FX32_CONST(792) },  { FX32_CONST(104), 0, FX32_CONST(504) },
            { FX32_CONST(200), 0, FX32_CONST(600) },  { FX32_CONST(360), 0, FX32_CONST(360) },
            { FX32_CONST(120), 0, FX32_CONST(312) },  { FX32_CONST(168), 0, FX32_CONST(216) },
            { FX32_CONST(312), 0, FX32_CONST(216) },
        };
        VecFx32 shortcutPositions[PLATFORM_COUNT] = {
            { FX32_CONST(168), 0, FX32_CONST(1032) }, { FX32_CONST(216), 0, FX32_CONST(936) },
            { FX32_CONST(408), 0, FX32_CONST(936) },  { FX32_CONST(312), 0, FX32_CONST(840) },
            { FX32_CONST(408), 0, FX32_CONST(792) },  { FX32_CONST(152), 0, FX32_CONST(600) },
            { FX32_CONST(408), 0, FX32_CONST(600) },  { FX32_CONST(168), 0, FX32_CONST(408) },
            { FX32_CONST(120), 0, FX32_CONST(168) },  { FX32_CONST(264), 0, FX32_CONST(984) },
            { FX32_CONST(408), 0, FX32_CONST(216) },
        };

        for (; i < PLATFORM_COUNT; i++) {
            matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, i);
            GymHumilauState_GetPlatformPos(work->state, &x, &z, i);
            if (x == 0 && z == 0) {
                if (!shortcutUsed) {
                    matrix->translation.x = startPositions[i].x;
                    matrix->translation.z = startPositions[i].z;
                } else {
                    matrix->translation.x = shortcutPositions[i].x;
                    matrix->translation.z = shortcutPositions[i].z;
                }
            } else {
                matrix->translation.x = x;
                matrix->translation.z = z;
            }
            matrix->translation.y = PLATFORM_Y;
            func_ov036_021b8248(work->expObj, 0, i, 1);
            FieldExpObj_SetActorHidden(work->expObj, 0, i, FALSE);

            anm = FieldExpObj_GetAnmInfo(work->expObj, 0, i, ANM_FLOAT);
            FieldExpObj_SetAnm(work->expObj, 0, i, ANM_FLOAT, TRUE);
            FieldExpObjAnm_SetLooped(anm, TRUE);
            FieldExpObjAnm_SetPaused(anm, FALSE);

            anm = FieldExpObj_GetAnmInfo(work->expObj, 0, i, ANM_TILT);
            FieldExpObj_SetAnm(work->expObj, 0, i, ANM_TILT, FALSE);
            FieldExpObjAnm_SetLooped(anm, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
        }
    }

    for (actor = ACTOR_STOP_SPLASH; actor < ACTOR_WAKE_END; actor++) {
        matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, actor);
        matrix->translation.x = 0;
        matrix->translation.z = 0;
        matrix->translation.y = PLATFORM_Y;
        func_ov036_021b8248(work->expObj, 0, actor, 1);
        FieldExpObj_SetActorHidden(work->expObj, 0, actor, TRUE);
        for (j = 0; j < 2; j++) {
            anm = FieldExpObj_GetAnmInfo(work->expObj, 0, actor, j);
            FieldExpObj_SetAnm(work->expObj, 0, actor, j, FALSE);
            FieldExpObjAnm_SetLooped(anm, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
        }
    }

    for (i = 0; i < 2; i++) {
        splash = ACTOR_RIDE_SPLASH + i;
        matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, splash);
        matrix->translation.x = 0;
        matrix->translation.z = 0;
        matrix->translation.y = PLATFORM_Y;
        func_ov036_021b8248(work->expObj, 0, splash, 1);
        FieldExpObj_SetActorHidden(work->expObj, 0, splash, TRUE);
        for (j = 0; j < 2; j++) {
            anm = FieldExpObj_GetAnmInfo(work->expObj, 0, splash, j);
            FieldExpObj_SetAnm(work->expObj, 0, splash, j, FALSE);
            FieldExpObjAnm_SetLooped(anm, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
        }
    }

    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, ACTOR_LEADER_SPLASH);
    matrix->translation.x = 0;
    matrix->translation.y = 0;
    matrix->translation.z = 0;
    func_ov036_021b8248(work->expObj, 0, ACTOR_LEADER_SPLASH, 1);
    FieldExpObj_SetActorHidden(work->expObj, 0, ACTOR_LEADER_SPLASH, TRUE);
    for (j = 0; j < 2; j++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, ACTOR_LEADER_SPLASH, j);
        FieldExpObj_SetAnm(work->expObj, 0, ACTOR_LEADER_SPLASH, j, FALSE);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
    }

    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, ACTOR_LEADER_WAVES);
    matrix->translation.x = 0;
    matrix->translation.y = 0;
    matrix->translation.z = 0;
    func_ov036_021b8248(work->expObj, 0, ACTOR_LEADER_WAVES, 1);
    FieldExpObj_SetActorHidden(work->expObj, 0, ACTOR_LEADER_WAVES, TRUE);
    for (j = 0; j < 3; j++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, ACTOR_LEADER_WAVES, j);
        FieldExpObj_SetAnm(work->expObj, 0, ACTOR_LEADER_WAVES, j, FALSE);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
    }
}

static void GymHumilau_SavePlatforms(HumilauGymWork *work) {
    int i;

    for (i = 0; i < PLATFORM_COUNT; i++) {
        u16 platform = i;
        SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, platform);

        GymHumilauState_SetPlatformPos(work->state, matrix->translation.x, matrix->translation.z, platform);
    }
    GymHumilauState_SetSaved(work->state);
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void GymHumilau_StepAnimations(HumilauGymWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

static GameEvent *GymHumilau_CreateRideEvent(GameEvent *parent, GameSystem *gsys, u8 ride, u16 platform) {
    Field *field = GSYS_GetField(gsys);
    FieldPlayer *player = Field_GetPlayer(field);
    GameEvent *event = GameEvent_Create(gsys, parent, GymHumilau_RideEventCallback, sizeof(HumilauGymRideEvent));
    HumilauGymRideEvent *ev = GameEvent_GetData(event);

    ev->gsys = gsys;
    ev->field = field;
    ev->player = player;
    ev->ride = ride;
    ev->platform = platform;
    ev->hit = FALSE;
    ev->hitPlatform = 0;
    ev->hitLength = 0;
    ev->wait = 0;
    ev->camera = Field_GetCameraSystem(field);
    return event;
}

static GameEventReturnCode GymHumilau_RideEventCallback(GameEvent *event, u32 *state, void *data) {
    HumilauGymRideEvent *ev = data;
    HumilauGymWork *work = Field_GetGimmickWorkBlock(ev->field, GIMMICK_WORK_ID);

    switch (*state) {
    case 0: {
        FieldActor *actor;

        GymHumilau_ClearAreas(work);
        GymHumilau_AddRideAreas(work, ev->ride);
        actor = FieldPlayer_GetActor(ev->player);
        if (!func_ov012_02166ecc(actor) && func_ov036_0219a5dc(ev->player) == TRUE) {
            func_ov012_02166f2c(actor);
        }
        *state = 1;
        break;
    }
    case 1:
        if (IsAllActorAcmdFinished(FieldPlayer_GetActor(ev->player))) {
            u32 dir = FieldPlayer_GetFaceDir(ev->player);

            FldAct_SetAcmd(FieldPlayer_GetActor(ev->player), dir + ACMD_HOP_FIRST);
            ev->wait = 15;
            *state = 2;
        }
        break;
    case 2:
        if (ev->wait-- > 0) {
            break;
        }
        FieldCamera_FinishDelay(ev->camera);
        *state = 3;
        // fallthrough
    case 3:
        // The original asks whether the delay is still active and ignores the answer
        FieldCamera_IsDelayActive(ev->camera);
        FieldCamera_EVCameraInit(ev->camera);
        ev->cameraBind = FieldCamera_GetBind(ev->camera);
        FieldCamera_ClearBind(ev->camera);
        FieldCamera_ChangeTransformType(ev->camera, 0);
        *state = 4;
        // fallthrough
    case 4: {
        int length = GymHumilau_GetRideLength(ev, ev->ride);
        VecFx32 target, from;
        int frames;
        FieldTask *task;

        if (length - 3 == 0) {
            GymHumilau_PlaySplash(ev, SPLASH_STOP);
            GymHumilau_PlayStopSplash(ev, ev->platform, FALSE);
            GFL_SndSEPlay(SEQ_SE_SW_MIZUGYM_01);
            *state = 8;
            break;
        }
        FieldPlayer_GetWPos(ev->player, &target);
        switch (sRides[ev->ride].dir) {
        case DIR_UP:
            target.z -= (length - 3) * FX32_CONST(16);
            break;
        case DIR_DOWN:
            target.z += (length - 3) * FX32_CONST(16);
            break;
        case DIR_LEFT:
            target.x -= (length - 3) * FX32_CONST(16);
            break;
        case DIR_RIGHT:
            target.x += (length - 3) * FX32_CONST(16);
            break;
        }
        frames = (length - 3) * 6;
        GymHumilau_StartRide(ev, frames, &target);
        FieldPlayer_GetWPos(ev->player, &from);
        {
            VecFx32 to = target;

            task = FieldCameraMoveTaskTarget_Create(ev->field, frames, &from, &to);
        }
        FieldTaskManager_AddTask(Field_GetTaskManager(ev->field), task, 0);
        GymHumilau_PlaySplash(ev, SPLASH_RIDE);
        GymHumilau_PlayWake(ev, ev->platform);
        GFL_SndSEPlay(SEQ_SE_SW_MIZUGYM_01);
        *state = 5;
        break;
    }
    case 5:
        if (GymHumilau_UpdateRide(ev) == TRUE) {
            GymHumilau_PlayStopSplash(ev, ev->platform, FALSE);
            if (ev->hit == TRUE) {
                GymHumilau_PlaySplash(ev, SPLASH_HIT);
                GymHumilau_PlayStopSplash(ev, ev->hitPlatform, TRUE);
                GymHumilau_StartRock(ev);
                if (GymHumilau_StartPush(ev)) {
                    *state = 6;
                } else {
                    *state = 8;
                }
            } else {
                *state = 8;
            }
            GFL_SndSEPlay(SEQ_SE_WALL_HIT);
        }
        break;
    case 6:
        GymHumilau_UpdateRock(ev);
        if (GymHumilau_UpdatePush(ev) == TRUE) {
            GymHumilau_PlayStopSplash(ev, ev->hitPlatform, FALSE);
            GFL_SndSEPlay(SEQ_SE_WALL_HIT);
            *state = 7;
        }
        break;
    case 7:
        if (GymHumilau_UpdateRock(ev) == TRUE) {
            *state = 8;
        }
        break;
    case 8: {
        GymHumilau_ClearAreas(work);
        GymHumilau_AddPlatformAreas(work, ev->platform);
        GymHumilau_AddNextAreas(work);
        work->standing = STANDING_RIDE_PLATFORM;
        {
            VecFx32 offset = { 0, 0, 0 };

            FieldCamera_CoordsSetEyeOffset(ev->camera, &offset);
        }
        FieldCameraAnm_EVCameraEnd(ev->camera);
        FieldCamera_SetBind(ev->camera, ev->cameraBind);
        FieldCamera_EnableDelay(ev->camera);
        *state = 9;
        break;
    }
    case 9: {
        SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, ev->platform);

        GymHumilauState_SetPlatformPos(work->state, matrix->translation.x, matrix->translation.z, ev->platform);
        if (ev->hit == TRUE) {
            matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, ev->hitPlatform);
            GymHumilauState_SetPlatformPos(work->state, matrix->translation.x, matrix->translation.z, ev->hitPlatform);
        }
        GymHumilauState_SetSlot(work->state, 0, ev->platform);
        GymHumilauState_SetSaved(work->state);
        if (ev->ride == RIDE_SHORTCUT) {
            GymHumilau_SetShortcutUsed(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(ev->field))),
                                       TRUE);
        }
        *state = 10;
        // fallthrough
    }
    case 10:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static BOOL GymHumilau_FindPlatform(FieldExpObjSystem *expObj, fx32 x, fx32 z, u16 *platform) {
    u8 i;

    for (i = 0; i < PLATFORM_COUNT; i++) {
        SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, i);

        if (matrix->translation.x == x && matrix->translation.z == z) {
            *platform = i;
            return TRUE;
        }
    }
    return FALSE;
}

// The grid square of the center of the platform in front of a ride spot
static BOOL GymHumilau_GetRidePlatformPos(u8 ride, int *x, int *z) {
    int rideX, rideZ;

    // BUG: lets RIDE_COUNT through, which would read past sRides; no caller passes it
#ifdef BUGFIX
    if (ride >= RIDE_COUNT) {
#else
    if (ride > RIDE_COUNT) {
#endif
        return FALSE;
    }
    rideX = sRides[ride].x;
    rideZ = sRides[ride].z;
    switch (sRides[ride].dir) {
    case DIR_UP:
        rideZ -= 2;
        break;
    case DIR_DOWN:
        rideZ += 2;
        break;
    case DIR_LEFT:
        rideX -= 2;
        break;
    case DIR_RIGHT:
        rideX += 2;
        break;
    }
    *x = rideX;
    *z = rideZ;
    return TRUE;
}

// Adds the areas of the platforms next to the one that the player stands on
static void GymHumilau_AddNextAreas(HumilauGymWork *work) {
    FieldPlayer *player = Field_GetPlayer(work->field);
    u16 platform = 0;
    s16 x, y, z;
    int dir;

    FieldPlayer_GetGPos(player, &x, &y, &z);
    for (dir = 0; dir < 4; dir++) {
        int nextX = x;
        int nextZ = z;

        switch (dir) {
        case DIR_UP:
            nextZ -= 3;
            break;
        case DIR_DOWN:
            nextZ += 3;
            break;
        case DIR_LEFT:
            nextX -= 3;
            break;
        case DIR_RIGHT:
            nextX += 3;
            break;
        }
        if (GymHumilau_FindPlatform(work->expObj, nextX * FX32_CONST(16) + FX32_CONST(8),
                                    nextZ * FX32_CONST(16) + FX32_CONST(8), &platform)) {
            GymHumilau_AddPlatformAreas(work, platform);
            GymHumilauState_SetSlot(work->state, 1, platform);
        }
    }
}

static void GymHumilau_RestoreAreas(HumilauGymWork *work) {
    int i;
    FieldActor *actor;
    s16 x;

    for (i = 0; i < 2; i++) {
        u8 platform = GymHumilauState_GetSlot(work->state, i);

        if (platform != PLATFORM_NONE) {
            GymHumilau_AddPlatformAreas(work, platform);
        }
    }
    actor = FieldPlayer_GetActor(Field_GetPlayer(work->field));
    x = GetGPosX(actor);
    work->standing = GymHumilau_GetStanding(work, x, GetGPosZ(actor));
    CheckRecalcActorY(FieldPlayer_GetActor(Field_GetPlayer(work->field)));
}

static void GymHumilau_ClearSlots(HumilauGymWork *work) {
    if (work->standing == STANDING_NONE) {
        GymHumilauState_SetSlot(work->state, 0, PLATFORM_NONE);
        GymHumilauState_SetSlot(work->state, 1, PLATFORM_NONE);
    }
}

// How far the platform goes along the ride's lane, from the center of the platform to where it stops: the lane's
// length, or the distance to the platform in the way, which the rest of the lane pushes on
static int GymHumilau_GetRideLength(HumilauGymRideEvent *ev, u8 ride) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(ev->field);
    u16 platform = 0;
    int x, z;
    int length, i;

    GymHumilau_GetRidePlatformPos(ride, &x, &z);
    for (i = 1, length = sRides[ride].length; i < sRides[ride].length; i++) {
        int nextX = x;
        int nextZ = z;

        switch (sRides[ride].dir) {
        case DIR_UP:
            nextZ -= i;
            break;
        case DIR_DOWN:
            nextZ += i;
            break;
        case DIR_LEFT:
            nextX -= i;
            break;
        case DIR_RIGHT:
            nextX += i;
            break;
        }
        if (GymHumilau_FindPlatform(expObj, nextX * FX32_CONST(16) + FX32_CONST(8),
                                    nextZ * FX32_CONST(16) + FX32_CONST(8), &platform)) {
            ev->hit = TRUE;
            ev->hitPlatform = platform;
            ev->hitLength = length - i;
            return i;
        }
    }
    return length;
}

static void GymHumilau_StartRide(HumilauGymRideEvent *ev, int frames, VecFx32 *target) {
    HumilauGymMover *mover = &ev->mover;

    mover->frame = 0;
    mover->frames = frames;
    VEC_Set(&mover->end, target->x, target->y, target->z);
    FieldPlayer_GetWPos(ev->player, &mover->start);
    mover->counter = 0;
    mover->start.y = PLATFORM_Y;
    mover->end.y = PLATFORM_Y;
    mover->amplitude = 3;
    mover->period = 18;
    mover->firstBob = TRUE;
}

static BOOL GymHumilau_UpdateRide(HumilauGymRideEvent *ev) {
    HumilauGymMover *mover = &ev->mover;
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(ev->field);
    BOOL done = FALSE;
    fx32 t, bob;
    u16 angle;
    VecFx32 pos, fromStart, fromEnd;

    mover->frame++;
    if (mover->frames <= mover->frame) {
        done = TRUE;
    }
    t = FX_F32_TO_FX32((f32)mover->frame / mover->frames);
    vecfx_mul(&mover->start, FX32_ONE - t, &fromStart);
    vecfx_mul(&mover->end, t, &fromEnd);
    VEC_Add(&fromStart, &fromEnd, &pos);

    mover->counter++;
    angle = (mover->counter << 16) / mover->period + 0x8000;
    if (mover->firstBob == TRUE) {
        bob = FX_SinIdx(angle) * 8;
        if (angle == 0) {
            mover->firstBob = FALSE;
            mover->period = 12;
        }
    } else {
        bob = mover->amplitude * FX_SinIdx(angle);
    }
    if (done) {
        bob = 0;
    }
    pos.y += bob;
    FieldPlayer_SetWPos(ev->player, &pos);
    FieldExpObj_GetActorMatrixPtr(expObj, 0, ev->platform)->translation = pos;
    if (mover->frame % 12 == 0) {
        GymHumilau_PlayWake(ev, ev->platform);
    }
    return done;
}

static BOOL GymHumilau_StartPush(HumilauGymRideEvent *ev) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(ev->field);
    HumilauGymMover *mover = &ev->mover;
    SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, ev->hitPlatform);
    int length = ev->hitLength;
    VecFx32 target;

    if (length - 3 == 0) {
        return FALSE;
    }
    target = matrix->translation;
    switch (sRides[ev->ride].dir) {
    case DIR_UP:
        target.z -= (length - 3) * FX32_CONST(16);
        break;
    case DIR_DOWN:
        target.z += (length - 3) * FX32_CONST(16);
        break;
    case DIR_LEFT:
        target.x -= (length - 3) * FX32_CONST(16);
        break;
    case DIR_RIGHT:
        target.x += (length - 3) * FX32_CONST(16);
        break;
    }
    mover->frame = 0;
    mover->frames = (length - 3) * 6;
    VEC_Set(&mover->end, target.x, target.y, target.z);
    mover->start = matrix->translation;
    mover->amplitude = 3;
    mover->period = 12;
    mover->counter = 0;
    return TRUE;
}

static BOOL GymHumilau_UpdatePush(HumilauGymRideEvent *ev) {
    HumilauGymMover *mover = &ev->mover;
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(ev->field);
    BOOL done = FALSE;
    fx32 t;
    VecFx32 pos, fromStart, fromEnd;

    mover->frame++;
    if (mover->frames <= mover->frame) {
        done = TRUE;
    }
    t = FX_F32_TO_FX32((f32)mover->frame / mover->frames);
    vecfx_mul(&mover->start, FX32_ONE - t, &fromStart);
    vecfx_mul(&mover->end, t, &fromEnd);
    VEC_Add(&fromStart, &fromEnd, &pos);
    FieldExpObj_GetActorMatrixPtr(expObj, 0, ev->hitPlatform)->translation = pos;
    if (mover->frame % 12 == 0) {
        GymHumilau_PlayWake(ev, ev->hitPlatform);
    }
    return done;
}

static void GymHumilau_StartRock(HumilauGymRideEvent *ev) {
    HumilauGymRock *rock = &ev->rock;

    rock->amplitude = 2;
    rock->period = 16;
    rock->counter = 0;
    rock->pos = FieldExpObj_GetActorMatrixPtr(Field_GetExpObjSystem(ev->field), 0, ev->platform)->translation;
}

static BOOL GymHumilau_UpdateRock(HumilauGymRideEvent *ev) {
    HumilauGymRock *rock = &ev->rock;
    SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(Field_GetExpObjSystem(ev->field), 0, ev->platform);
    u32 period = rock->period;
    u32 counter = ++rock->counter;
    u16 angle = (counter << 16) / period;
    fx16 sin;
    fx32 offset;

    if (counter >= period) {
        rock->counter = 0;
    }
    sin = FX_SinIdx(angle);
    if (sin == 0) {
        rock->amplitude--;
        if (rock->amplitude < 0) {
            rock->amplitude = 0;
        }
    }
    if (rock->amplitude == 0) {
        matrix->translation = rock->pos;
        FieldPlayer_SetWPos(ev->player, &rock->pos);
        return TRUE;
    }
    offset = rock->amplitude * sin;
    switch (FieldPlayer_GetFaceDir(ev->player)) {
    case DIR_UP:
        matrix->translation.z = rock->pos.z - offset;
        break;
    case DIR_DOWN:
        matrix->translation.z = rock->pos.z + offset;
        break;
    case DIR_LEFT:
        matrix->translation.x = rock->pos.x - offset;
        break;
    case DIR_RIGHT:
        matrix->translation.x = rock->pos.x + offset;
        break;
    }
    FieldPlayer_SetWPos(ev->player, &matrix->translation);
    return FALSE;
}

// Tilts the platform in a slot of the state toward the player, who steps onto or off it
static void GymHumilau_StartTilt(HumilauGymWork *work, u8 slot) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(work->field);
    u16 platform = GymHumilauState_GetSlot(work->state, slot);
    FieldPlayer *player = Field_GetPlayer(work->field);
    u16 rotations[4][3] = {
        { 0, 0xc000, 0 },
        { 0, 0xc000, 0 },
        { 0, 0, 0 },
        { 0, 0x8000, 0 },
    };
    u16 *rotation = rotations[FieldPlayer_GetFaceDir(player)];
    SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, platform);
    FieldExpObjAnm *anm;

    MAT3_RotationEulerZYX(rotation[0], rotation[1], rotation[2], &matrix->rotation);
    anm = FieldExpObj_GetAnmInfo(work->expObj, 0, platform, ANM_FLOAT);
    FieldExpObjAnm_SetPaused(anm, TRUE);
    FieldExpObj_SetAnm(expObj, 0, platform, ANM_FLOAT, FALSE);
    anm = FieldExpObj_GetAnmInfo(work->expObj, 0, platform, ANM_TILT);
    FieldExpObj_SetAnm(expObj, 0, platform, ANM_TILT, TRUE);
    FieldExpObj_SetAnmFrame(expObj, 0, platform, ANM_TILT, 0);
    FieldExpObjAnm_SetPaused(anm, FALSE);
    work->tilting = TRUE;
    work->tiltPlatform = platform;
}

static BOOL GymHumilau_IsTiltFinished(HumilauGymWork *work) {
    if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(work->expObj, 0, work->tiltPlatform, ANM_TILT))) {
        return TRUE;
    }
    return FALSE;
}

static void GymHumilau_EndTilt(HumilauGymWork *work) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(work->field);
    u16 platform = work->tiltPlatform;
    FieldExpObjAnm *anm;

    anm = FieldExpObj_GetAnmInfo(work->expObj, 0, platform, ANM_TILT);
    FieldExpObj_SetAnm(expObj, 0, platform, ANM_TILT, FALSE);
    FieldExpObjAnm_SetPaused(anm, TRUE);
    anm = FieldExpObj_GetAnmInfo(work->expObj, 0, platform, ANM_FLOAT);
    FieldExpObj_SetAnm(expObj, 0, platform, ANM_FLOAT, TRUE);
    FieldExpObjAnm_SetPaused(anm, FALSE);
    work->tilting = FALSE;
}

// The splash where the platform stands at the start of a ride, in front of the player as it leaves, or between the
// platform and the one it hits
static void GymHumilau_PlaySplash(HumilauGymRideEvent *ev, int kind) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(ev->field);
    int i;
    u16 actor;
    SRTMatrix *splash, *platform;

    switch (kind) {
    case SPLASH_STOP:
    case SPLASH_RIDE:
        actor = ACTOR_RIDE_SPLASH;
        break;
    case SPLASH_HIT:
        actor = ACTOR_PUSH_SPLASH;
        break;
    default:
        return;
    }
    splash = FieldExpObj_GetActorMatrixPtr(expObj, 0, actor);
    platform = FieldExpObj_GetActorMatrixPtr(expObj, 0, ev->platform);
    switch (kind) {
    case SPLASH_STOP:
        splash->translation = platform->translation;
        break;
    case SPLASH_RIDE: {
        s16 frontX, frontY, frontZ;
        s16 x, y, z;

        FieldPlayer_GetGPosInFront(ev->player, &frontX, &frontY, &frontZ);
        FieldPlayer_GetGPos(ev->player, &x, &y, &z);
        splash->translation = platform->translation;
        splash->translation.x += (frontX - x) * FX32_CONST(6);
        splash->translation.z += (frontZ - z) * FX32_CONST(6);
        break;
    }
    case SPLASH_HIT: {
        VecFx32 half;

        VEC_Subtract(&FieldExpObj_GetActorMatrixPtr(expObj, 0, ev->hitPlatform)->translation, &platform->translation,
                     &half);
        vecfx_div(&half, FX32_CONST(2), &half);
        VEC_Add(&platform->translation, &half, &splash->translation);
        splash->translation.y = PLATFORM_Y;
        break;
    }
    }
    FieldExpObj_SetActorHidden(expObj, 0, actor, FALSE);
    for (i = 0; i < 2; i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(expObj, 0, actor, i);

        FieldExpObj_SetAnm(expObj, 0, actor, i, TRUE);
        FieldExpObj_SetAnmFrame(expObj, 0, actor, i, 0);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
    GymHumilau_StartEffect(Field_GetGimmickWorkBlock(ev->field, GIMMICK_WORK_ID), actor, 2);
}

// Leaves a wake behind the moving platform, in the first of the wake actors that is free
static void GymHumilau_PlayWake(HumilauGymRideEvent *ev, u16 platform) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(ev->field);
    BOOL allBusy = TRUE;
    int actor;
    u16 wake;

    for (actor = ACTOR_WAKE_FIRST; actor < ACTOR_WAKE_END; actor++) {
        if (func_ov036_021b84ec(FieldExpObj_GetAnmInfo(expObj, 0, actor, 0))) {
            wake = actor;
            allBusy = FALSE;
            break;
        }
    }
    if (!allBusy) {
        SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, wake);
        int i;

        matrix->translation = FieldExpObj_GetActorMatrixPtr(expObj, 0, platform)->translation;
        matrix->translation.y = PLATFORM_Y;
        {
            u16 wakeRotations[4][3] = {
                { 0, 0x4000, 0 },
                { 0, 0xc000, 0 },
                { 0, 0x8000, 0 },
                { 0, 0, 0 },
            };

            u16 *rotation = wakeRotations[FieldPlayer_GetFaceDir(ev->player)];

            MAT3_RotationEulerZYX(rotation[0], rotation[1], rotation[2], &matrix->rotation);
        }
        FieldExpObj_SetActorHidden(expObj, 0, wake, FALSE);
        for (i = 0; i < 2; i++) {
            FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(expObj, 0, wake, i);

            FieldExpObj_SetAnm(expObj, 0, wake, i, TRUE);
            FieldExpObj_SetAnmFrame(expObj, 0, wake, i, 0);
            FieldExpObjAnm_SetPaused(anm, FALSE);
        }
        GymHumilau_StartEffect(Field_GetGimmickWorkBlock(ev->field, GIMMICK_WORK_ID), wake, 2);
    }
}

// The splash around a platform that stops, a bigger one for the platform that is hit
static void GymHumilau_PlayStopSplash(HumilauGymRideEvent *ev, u16 platform, BOOL hit) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(ev->field);
    int i;
    u16 actor = ACTOR_STOP_SPLASH;
    SRTMatrix *matrix;

    if (hit) {
        actor = ACTOR_HIT_SPLASH;
    }
    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, actor);
    matrix->translation = FieldExpObj_GetActorMatrixPtr(expObj, 0, platform)->translation;
    matrix->translation.y = PLATFORM_Y;
    FieldExpObj_SetActorHidden(expObj, 0, actor, FALSE);
    for (i = 0; i < 2; i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(expObj, 0, actor, i);

        FieldExpObj_SetAnm(expObj, 0, actor, i, TRUE);
        FieldExpObj_SetAnmFrame(expObj, 0, actor, i, 0);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
    GymHumilau_StartEffect(Field_GetGimmickWorkBlock(ev->field, GIMMICK_WORK_ID), actor, 2);
}

// Which platform a grid square is on, from the areas: each platform has two, the first ones the platform of the ride
static int GymHumilau_GetStanding(HumilauGymWork *work, int x, int z) {
    void *areas = func_ov036_02184590(Field_GetG3DMapper(work->field));
    BOOL onRidePlatform = FALSE;
    BOOL onNextPlatform = FALSE;
    int i;

    for (i = 0; i < work->areaCount; i++) {
        if (func_ov036_021ba688(x, z, areas, i)) {
            if (i / 2 == 0) {
                onRidePlatform = TRUE;
            } else if (i / 2 == 1) {
                onNextPlatform = TRUE;
            }
        }
    }
    if (onRidePlatform) {
        return STANDING_RIDE_PLATFORM;
    }
    if (onNextPlatform) {
        return STANDING_NEXT_PLATFORM;
    }
    return STANDING_NONE;
}

static void GymHumilau_ClearAreas(HumilauGymWork *work) {
    void *areas = func_ov036_02184590(Field_GetG3DMapper(work->field));
    int count = func_ov036_021ba684(areas);
    int i;

    for (i = 0; i < count; i++) {
        func_ov036_021ba6bc(i, FALSE, areas);
    }
    work->areaCount = 0;
    GymHumilau_ClearSlots(work);
}

// The areas of a platform, a cross of a column and a row of three squares
static void GymHumilau_AddRideAreas(HumilauGymWork *work, u8 ride) {
    void *areas = func_ov036_02184590(Field_GetG3DMapper(work->field));
    int x, z;

    GymHumilau_GetRidePlatformPos(ride, &x, &z);
    func_ov036_021ba624(work->areaCount++, x, z - 1, 1, 3, PLATFORM_Y, 0x800000, areas);
    func_ov036_021ba624(work->areaCount++, x - 1, z, 3, 1, PLATFORM_Y, 0x800000, areas);
}

static void GymHumilau_AddPlatformAreas(HumilauGymWork *work, u16 platform) {
    void *areas = func_ov036_02184590(Field_GetG3DMapper(work->field));
    SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, platform);
    int x = FX_Whole(matrix->translation.x - FX32_CONST(8)) / 16;
    int z = FX_Whole(matrix->translation.z - FX32_CONST(8)) / 16;

    func_ov036_021ba624(work->areaCount++, x, z - 1, 1, 3, PLATFORM_Y, 0x800000, areas);
    func_ov036_021ba624(work->areaCount++, x - 1, z, 3, 1, PLATFORM_Y, 0x800000, areas);
}

static void GymHumilau_StartEffect(HumilauGymWork *work, u16 actor, u8 anmCount) {
    TCBManager *tcbMgr = Field_GetTCBMgr(work->field);
    int i;

    for (i = 0; i < EFFECT_COUNT; i++) {
        HumilauGymEffect *effect = &work->effects[i];

        // The stop splash plays once at a time
        if (effect->actor == ACTOR_STOP_SPLASH && actor == ACTOR_STOP_SPLASH && effect->active == TRUE) {
            return;
        }
        if (!effect->active) {
            effect->active = TRUE;
            effect->expObj = work->expObj;
            effect->actor = actor;
            effect->anmCount = anmCount;
            GFL_TCBMgrAddTask(tcbMgr, GymHumilau_EffectTask, effect, 0);
            return;
        }
    }
}

static void GymHumilau_EffectTask(TCB *tcb, void *data) {
    HumilauGymEffect *effect = data;
    int i = 0;

    if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(effect->expObj, 0, effect->actor, 0))) {
        FieldExpObj_SetActorHidden(effect->expObj, 0, effect->actor, TRUE);
        for (; i < effect->anmCount; i++) {
            FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(effect->expObj, 0, effect->actor, i);

            FieldExpObj_SetAnm(effect->expObj, 0, effect->actor, i, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
        }
        effect->active = FALSE;
        GFL_TCBRemove(tcb);
    }
}

static void GymHumilauState_SetPlatformPos(HumilauGymState *state, fx32 x, fx32 z, u16 platform) {
    state->x[platform] = x;
    state->z[platform] = z;
}

static void GymHumilauState_GetPlatformPos(HumilauGymState *state, fx32 *x, fx32 *z, u16 platform) {
    *x = state->x[platform];
    *z = state->z[platform];
}

static void GymHumilauState_SetSaved(HumilauGymState *state) {
    state->saved = TRUE;
}

static BOOL GymHumilauState_IsSaved(HumilauGymState *state) {
    return state->saved;
}

static void GymHumilauState_SetSlot(HumilauGymState *state, int slot, u8 platform) {
    state->platforms[slot] = platform;
}

static u8 GymHumilauState_GetSlot(HumilauGymState *state, int slot) {
    return state->platforms[slot];
}

// The shortcut's flag is in the gimmick's flags after its user data
static void GymHumilau_SetShortcutUsed(GimmickState *gimmickState, BOOL used) {
    u8 *flag = func_0200bacc(gimmickState, GIMMICK_ID);

    if (used == TRUE) {
        *flag = TRUE;
    } else {
        *flag = FALSE;
    }
}

static BOOL GymHumilau_IsShortcutUsed(GimmickState *gimmickState) {
    u8 *flag = func_0200bacc(gimmickState, GIMMICK_ID);

    if (*flag == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static const G3DSceneResourceSetup sGymHumilauResources[25] = {
    { ARCID_GYM_HUMILAU, 0, G3D_SCENE_RES_ARCSYS },  { ARCID_GYM_HUMILAU, 2, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 1, G3D_SCENE_RES_ARCSYS },  { ARCID_GYM_HUMILAU, 6, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 7, G3D_SCENE_RES_ARCSYS },  { ARCID_GYM_HUMILAU, 8, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 3, G3D_SCENE_RES_ARCSYS },  { ARCID_GYM_HUMILAU, 4, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 5, G3D_SCENE_RES_ARCSYS },  { ARCID_GYM_HUMILAU, 9, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 10, G3D_SCENE_RES_ARCSYS }, { ARCID_GYM_HUMILAU, 11, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 12, G3D_SCENE_RES_ARCSYS }, { ARCID_GYM_HUMILAU, 13, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 14, G3D_SCENE_RES_ARCSYS }, { ARCID_GYM_HUMILAU, 15, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 16, G3D_SCENE_RES_ARCSYS }, { ARCID_GYM_HUMILAU, 17, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 22, G3D_SCENE_RES_ARCSYS }, { ARCID_GYM_HUMILAU, 23, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 24, G3D_SCENE_RES_ARCSYS }, { ARCID_GYM_HUMILAU, 18, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 19, G3D_SCENE_RES_ARCSYS }, { ARCID_GYM_HUMILAU, 20, G3D_SCENE_RES_ARCSYS },
    { ARCID_GYM_HUMILAU, 21, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneActorSetup sGymHumilauActors[23] = {
    { 0, 0, 0, 0, sHumilauPlatformAnimations, NELEMS(sHumilauPlatformAnimations) },
    { 0, 0, 0, 0, sHumilauPlatformAnimations, NELEMS(sHumilauPlatformAnimations) },
    { 0, 0, 0, 0, sHumilauPlatformAnimations, NELEMS(sHumilauPlatformAnimations) },
    { 0, 0, 0, 0, sHumilauPlatformAnimations, NELEMS(sHumilauPlatformAnimations) },
    { 0, 0, 0, 0, sHumilauPlatformAnimations, NELEMS(sHumilauPlatformAnimations) },
    { 0, 0, 0, 0, sHumilauPlatformAnimations, NELEMS(sHumilauPlatformAnimations) },
    { 0, 0, 0, 0, sHumilauPlatformAnimations, NELEMS(sHumilauPlatformAnimations) },
    { 0, 0, 0, 0, sHumilauPlatformAnimations, NELEMS(sHumilauPlatformAnimations) },
    { 0, 0, 0, 0, sHumilauPlatformAnimations, NELEMS(sHumilauPlatformAnimations) },
    { 0, 0, 0, 0, sHumilauPlatformAnimations, NELEMS(sHumilauPlatformAnimations) },
    { 0, 0, 0, 0, sHumilauPlatformAnimations, NELEMS(sHumilauPlatformAnimations) },
    { 3, 0, 3, 0, sStopSplashAnimations, NELEMS(sStopSplashAnimations) },
    { 6, 0, 6, 0, sHitSplashAnimations, NELEMS(sHitSplashAnimations) },
    { 9, 0, 9, 0, sWakeAnimations, NELEMS(sWakeAnimations) },
    { 9, 0, 9, 0, sWakeAnimations, NELEMS(sWakeAnimations) },
    { 9, 0, 9, 0, sWakeAnimations, NELEMS(sWakeAnimations) },
    { 9, 0, 9, 0, sWakeAnimations, NELEMS(sWakeAnimations) },
    { 9, 0, 9, 0, sWakeAnimations, NELEMS(sWakeAnimations) },
    { 9, 0, 9, 0, sWakeAnimations, NELEMS(sWakeAnimations) },
    { 12, 0, 12, 0, sRideSplashAnimations, NELEMS(sRideSplashAnimations) },
    { 15, 0, 15, 0, sPushSplashAnimations, NELEMS(sPushSplashAnimations) },
    { 18, 0, 18, 0, sLeaderSplashAnimations, NELEMS(sLeaderSplashAnimations) },
    { 21, 0, 21, 0, sLeaderWavesAnimations, NELEMS(sLeaderWavesAnimations) },
};
