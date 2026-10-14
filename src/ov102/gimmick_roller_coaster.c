#include "types.h"
#include "constants/directions.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_exp_obj.h"
#include "field/field_script.h"
#include "field/field_sound.h"
#include "field/gimmick_roller_coaster.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nnsys/g3d.h"
#include "nnsys/snd.h"
#include "save/gimmick_savedata.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The roller coaster of Nimbasa City's zone 585. Overlay 102 has no name string, so the name is a guess: it is gimmick
// 48 of overlay 36's gimmick table, which archive 102 gives only zone 585, whose scripts are the roller coaster's
// trainers and switches ("The roller coaster's path has been changed!"). Four cars run their paths as model
// animations and stop at stations for a while; the player rides one by facing up at a station. Each car has a switch
// that picks one of its two paths, saved with each car's frame, and the coaster's ambience (SEQ_SE_FLD_25) is louder
// the faster the nearest car goes

#define GIMMICK_ID 1
#define GIMMICK_STATE_ID 48

// The archive of the coaster's models, animations and the player's ride curves
#define ARCID_ROLLER_COASTER 292

#define CAR_COUNT 4
// Each car has two paths, path car * 2 + switch, played by the car's animation path + 1
#define PATH_COUNT (CAR_COUNT * 2)
#define SPOT_COUNT 9

// The scene's actors: four cars, their four switches and the four tracks that the switches turn. A car's actor is
// `ACTOR_CAR + car` held in a u8 of its own: the original narrows the car's index again for it
#define ACTOR_CAR 0
#define ACTOR_SWITCH 4
#define ACTOR_TRACK 8
#define STATIONS_PER_PATH 2
#define NO_PATH 0xff
#define NO_CAR -1
#define NO_SPOT -1

// How long a car waits at a station, in frames
#define STATION_WAIT 90

// The frames of paths 2 to 5 at which the ride's sounds change: the ambience stops, with or without a sound effect,
// or a sound effect plays
#define PATH2_FRAME_STOP_FLD26 FX32_CONST(191)
#define PATH2_FRAME_STOP FX32_CONST(235)
#define PATH2_FRAME_FLD27 FX32_CONST(255)
#define PATH3_FRAME_STOP_FLD26 FX32_CONST(264)
#define PATH3_FRAME_STOP FX32_CONST(305)
#define PATH3_FRAME_FLD27 FX32_CONST(328)
#define PATH4_FRAME_STOP_FLD67 FX32_CONST(177)
#define PATH5_FRAME_STOP_FLD67 FX32_CONST(182)

// How far a car moves in a frame, which sets the volume of its sound
#define SPEED_FASTEST FX32_CONST(32)
#define SPEED_FAST FX32_CONST(24)
#define SPEED_MEDIUM FX32_CONST(16)
#define SPEED_SLOW FX32_CONST(8)

// Beyond this distance from the nearest car the ambience is silent
#define AMBIENCE_RANGE FX32_CONST(96)

// A grid square is 16 units
#define GRID_SIZE FX32_CONST(16)

// The zone's scripts that the rides call
#define SCRIPT_RIDE_START 10
#define SCRIPT_RIDE_END 11
#define SCRIPT_INTRO_START 12
#define SCRIPT_INTRO_RIDE 6
#define SCRIPT_INTRO_END 7

// The gimmick's saved state
typedef struct {
    fx32 frames[CAR_COUNT];
    u8 switches[CAR_COUNT];
    // Whether the first ride of cars 1 and 2 has been shown
    u16 introSeen[2];
} RollerCoasterSave;

typedef struct {
    TCB *task;
    // The path of the car the player rides
    u8 ridePath;
    // The switch being pressed
    s8 switchId;
    u8 switchState;
    // The car that waits for the player
    s8 waitCar;
    s32 waits[CAR_COUNT];
    G3DCurve *curve;
    u8 curveBuffer[0x21c0];
    // Allocated with the work but never read or written
    u8 unused[8];
    BOOL riding;
    // A switch pressed while its car was past the switch point, applied when the car comes round
    u8 pendingSwitch[CAR_COUNT];
    u8 paths[CAR_COUNT];
    // The spot of the station where each car waits
    s8 stations[CAR_COUNT];
} RollerCoasterWork;

static u8 RollerCoaster_FindSpot(int x, int z);
static u8 RollerCoaster_GetCarPath(RollerCoasterWork *work, u8 car);
static void RollerCoaster_Task(TCB *task, void *data);
static u8 RollerCoaster_GetSpotPath(RollerCoasterWork *work, u8 spot);
static u8 RollerCoaster_GetSpotCar(u8 spot);
static BOOL RollerCoaster_IsCarAtSpot(RollerCoasterWork *work, u8 spot);
static fx32 RollerCoaster_GetAnmFrame(FieldExpObjSystem *expObj, u16 actor, u16 anm);
static void RollerCoaster_SetAnmFrame(FieldExpObjSystem *expObj, u16 actor, u16 anm, fx32 frame);
static GameEventReturnCode RollerCoaster_SwitchEvent(GameEvent *event, u32 *state, void *data);
static BOOL RollerCoaster_IsIntroNeeded(RollerCoasterSave *save, int car);
static void RollerCoaster_SetIntroSeen(RollerCoasterSave *save, u8 car);
static GameEventReturnCode RollerCoaster_RideEvent(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode RollerCoaster_IntroEvent(GameEvent *event, u32 *state, void *data);
static void RollerCoaster_ToggleSwitch(Field *field, FieldExpObjSystem *expObj);
static void RollerCoaster_SwitchPath(FieldExpObjSystem *expObj, u8 oldSwitch, u8 car);
static BOOL RollerCoaster_IsAtStation(u8 path, fx32 frame, u8 *index);
static BOOL RollerCoaster_IsBeforeSwitch(FieldExpObjSystem *expObj, u8 car, u8 path);
static void RollerCoaster_GetJointTrans(const NNSG3dResJntAnm *anm, u32 node, fx32 frame, VecFx32 *trans);
static void RollerCoaster_GetTransData(fx32 *value, fx32 frame, const u32 *data, const NNSG3dResJntAnm *anm);
static void RollerCoaster_UpdateAmbience(Field *field, RollerCoasterWork *work);

// A grid square where the player boards
typedef struct {
    u16 x;
    u16 z;
} RideSpot;

// The tables are declared in this order because MWCC lays out objects of the same size by where each was declared,
// and this is the most regular order found that gives the ROM's `.rodata`. The original's was most likely the plain
// one (the resources, the cars' animations, the switches', the tracks', the actors, the setup) with one more constant
// object among the other tables that the linker dropped: with an unreferenced non-static 16-byte constant there, that
// order gives the ROM's layout too

// The animations of each actor, as indices into sResources. A switch's or a track's animation 0 turns it to state 0
// and animation 1 to state 1
static const G3DSceneAnimationSetup sTrack0Animations[] = { { 23, 0 }, { 24, 0 } };

// A car's animation 0 (resource 6) is its stop at a station, and animation path + 1 (resources 7 to 14) runs it along
// one of the eight paths
static const G3DSceneAnimationSetup sCarAnimations[] = {
    { 6, 0 }, { 7, 0 }, { 8, 0 }, { 9, 0 }, { 10, 0 }, { 11, 0 }, { 12, 0 }, { 13, 0 }, { 14, 0 },
};

static const G3DSceneAnimationSetup sTrack1Animations[] = { { 25, 0 }, { 26, 0 } };

static const G3DSceneAnimationSetup sTrack2Animations[] = { { 27, 0 }, { 28, 0 } };

static const G3DSceneAnimationSetup sTrack3Animations[] = { { 29, 0 }, { 30, 0 } };

// The frames at which a car on each path stops at a station
static const fx32 sStationFrames[PATH_COUNT][STATIONS_PER_PATH] = {
    { 0, FX32_CONST(40) },
    { 0, FX32_CONST(65) },
    { 0, 0 },
    { 0, FX32_CONST(124) },
    { 0, 0 },
    { 0, FX32_CONST(126) },
    { 0, 0 },
    { 0, FX32_CONST(138) },
};

// The last frame of each car's path at which its switch still changes the path
static const u16 sSwitchFrames[CAR_COUNT] = { 13, 105, 93, 70 };

// The files of archive 292: resource 0 is the car's model, 1 the switch's, 2 to 5 the four tracks', 6 to 14 the cars'
// animations, 15 to 22 the switches' and 23 to 30 the tracks'
static const G3DSceneResourceSetup sResources[] = {
    { ARCID_ROLLER_COASTER, 0, 0 },  { ARCID_ROLLER_COASTER, 2, 0 },  { ARCID_ROLLER_COASTER, 20, 0 },
    { ARCID_ROLLER_COASTER, 21, 0 }, { ARCID_ROLLER_COASTER, 22, 0 }, { ARCID_ROLLER_COASTER, 19, 0 },
    { ARCID_ROLLER_COASTER, 1, 0 },  { ARCID_ROLLER_COASTER, 13, 0 }, { ARCID_ROLLER_COASTER, 14, 0 },
    { ARCID_ROLLER_COASTER, 15, 0 }, { ARCID_ROLLER_COASTER, 16, 0 }, { ARCID_ROLLER_COASTER, 17, 0 },
    { ARCID_ROLLER_COASTER, 18, 0 }, { ARCID_ROLLER_COASTER, 11, 0 }, { ARCID_ROLLER_COASTER, 12, 0 },
    { ARCID_ROLLER_COASTER, 3, 0 },  { ARCID_ROLLER_COASTER, 4, 0 },  { ARCID_ROLLER_COASTER, 5, 0 },
    { ARCID_ROLLER_COASTER, 6, 0 },  { ARCID_ROLLER_COASTER, 7, 0 },  { ARCID_ROLLER_COASTER, 8, 0 },
    { ARCID_ROLLER_COASTER, 9, 0 },  { ARCID_ROLLER_COASTER, 10, 0 }, { ARCID_ROLLER_COASTER, 25, 0 },
    { ARCID_ROLLER_COASTER, 26, 0 }, { ARCID_ROLLER_COASTER, 27, 0 }, { ARCID_ROLLER_COASTER, 28, 0 },
    { ARCID_ROLLER_COASTER, 29, 0 }, { ARCID_ROLLER_COASTER, 30, 0 }, { ARCID_ROLLER_COASTER, 23, 0 },
    { ARCID_ROLLER_COASTER, 24, 0 },
};

// The station that each path's station frame stops at, as an index into sSpots
static const s8 sStationSpots[PATH_COUNT][STATIONS_PER_PATH] = {
    { 1, 0 }, { 1, 2 }, { 4, NO_SPOT }, { 4, 3 }, { 6, NO_SPOT }, { 6, 5 }, { 8, NO_SPOT }, { 8, 7 },
};

static const G3DSceneAnimationSetup sSwitch0Animations[] = { { 15, 0 }, { 16, 0 } };

static const G3DSceneAnimationSetup sSwitch1Animations[] = { { 17, 0 }, { 18, 0 } };

static const G3DSceneAnimationSetup sSwitch2Animations[] = { { 19, 0 }, { 20, 0 } };

static const G3DSceneAnimationSetup sSwitch3Animations[] = { { 21, 0 }, { 22, 0 } };

// Where each car's model is, which its animations are relative to
static const VecFx32 sCarOrigins[CAR_COUNT] = {
    { FX32_CONST(360), FX32_CONST(32), FX32_CONST(600) },
    { FX32_CONST(664), FX32_CONST(32), FX32_CONST(616) },
    { FX32_CONST(552), FX32_CONST(32), FX32_CONST(104) },
    { FX32_CONST(88), FX32_CONST(32), FX32_CONST(600) },
};

// The files of the player's ride curve along each path
static const u32 sRideCurves[PATH_COUNT] = { 33, 34, 35, 36, 37, 38, 31, 32 };

// The grid squares where the player boards, and the car that stops at each
static const RideSpot sSpots[SPOT_COUNT] = {
    { 14, 39 }, { 22, 39 }, { 16, 27 }, { 16, 15 }, { 41, 40 }, { 49, 40 }, { 34, 8 }, { 22, 8 }, { 5, 39 },
};

static const u8 sSpotCars[SPOT_COUNT] = { 0, 0, 0, 1, 1, 2, 2, 3, 3 };

// The four cars (actors 0 to 3), their four switches (4 to 7) and the four tracks that the switches turn (8 to 11),
// as model and texture resources and their animations
static const G3DSceneActorSetup sActors[] = {
    { 0, 0, 0, 0, sCarAnimations, NELEMS(sCarAnimations) },
    { 0, 0, 0, 0, sCarAnimations, NELEMS(sCarAnimations) },
    { 0, 0, 0, 0, sCarAnimations, NELEMS(sCarAnimations) },
    { 0, 0, 0, 0, sCarAnimations, NELEMS(sCarAnimations) },
    { 1, 0, 1, 0, sSwitch0Animations, NELEMS(sSwitch0Animations) },
    { 1, 0, 1, 0, sSwitch1Animations, NELEMS(sSwitch1Animations) },
    { 1, 0, 1, 0, sSwitch2Animations, NELEMS(sSwitch2Animations) },
    { 1, 0, 1, 0, sSwitch3Animations, NELEMS(sSwitch3Animations) },
    { 2, 0, 2, 0, sTrack0Animations, NELEMS(sTrack0Animations) },
    { 3, 0, 3, 0, sTrack1Animations, NELEMS(sTrack1Animations) },
    { 4, 0, 4, 0, sTrack2Animations, NELEMS(sTrack2Animations) },
    { 5, 0, 5, 0, sTrack3Animations, NELEMS(sTrack3Animations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

static inline FieldExpObjAnm *RollerCoaster_GetAnm(FieldExpObjSystem *expObj, u8 actor, u8 anm) {
    return FieldExpObj_GetAnmInfo(expObj, 0, actor, anm);
}

static inline void RollerCoaster_SetAnmPaused(FieldExpObjSystem *expObj, u8 actor, u8 anm, BOOL paused) {
    FieldExpObjAnm_SetPaused(RollerCoaster_GetAnm(expObj, actor, anm), paused);
}

// Shows a switch or its track at the end of its animation for the switch's state
static inline void RollerCoaster_ShowSwitchState(FieldExpObjSystem *expObj, u8 actor, u8 state) {
    FieldExpObj_SetAnm(expObj, 0, actor, state, TRUE);
    FieldExpObjAnm_SetLooped(FieldExpObj_GetAnmInfo(expObj, 0, actor, 0), FALSE);
    FieldExpObjAnm_SetLooped(FieldExpObj_GetAnmInfo(expObj, 0, actor, 1), FALSE);
    FieldExpObj_SetAnmFrame(expObj, 0, actor, state,
                            func_ov036_021b8580(FieldExpObj_GetAnmInfo(expObj, 0, actor, state)));
}

static u8 RollerCoaster_FindSpot(int x, int z) {
    u8 i;

    for (i = 0; i < SPOT_COUNT; i++) {
        if (x == sSpots[i].x && z == sSpots[i].z) {
            return i;
        }
    }
    return 0xff;
}

static u8 RollerCoaster_GetCarPath(RollerCoasterWork *work, u8 car) {
    return work->paths[car];
}

void RollerCoaster_Init(Field *field) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    RollerCoasterSave *save = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_STATE_ID);
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);
    RollerCoasterWork *work;
    u8 i;
    int j;
    // Each actor's position is a local of its own: the ROM has twelve 12-byte initializers, each copied straight into
    // its actor's matrix, though the cars' and tracks' repeat sCarOrigins
    VecFx32 car0 = { FX32_CONST(360), FX32_CONST(32), FX32_CONST(600) };
    VecFx32 car1 = { FX32_CONST(664), FX32_CONST(32), FX32_CONST(616) };
    VecFx32 car2 = { FX32_CONST(552), FX32_CONST(32), FX32_CONST(104) };
    VecFx32 car3 = { FX32_CONST(88), FX32_CONST(32), FX32_CONST(600) };
    VecFx32 switch0 = { FX32_CONST(472), FX32_CONST(32), FX32_CONST(536) };
    VecFx32 switch1 = { FX32_CONST(216), FX32_CONST(32), FX32_CONST(264) };
    VecFx32 switch2 = { FX32_CONST(744), FX32_CONST(32), FX32_CONST(664) };
    VecFx32 switch3 = { FX32_CONST(408), FX32_CONST(32), FX32_CONST(152) };
    VecFx32 track0 = { FX32_CONST(360), FX32_CONST(32), FX32_CONST(600) };
    VecFx32 track1 = { FX32_CONST(664), FX32_CONST(32), FX32_CONST(616) };
    VecFx32 track2 = { FX32_CONST(552), FX32_CONST(32), FX32_CONST(104) };
    VecFx32 track3 = { FX32_CONST(88), FX32_CONST(32), FX32_CONST(600) };

    work = Field_AllocGimmickWorkBlock(field, GIMMICK_ID, Field_GetHeapID(field), sizeof(RollerCoasterWork));
    LoadFieldExpandObjData(expObj, &sSceneSetup, 0);
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 0)->translation = car0;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 1)->translation = car1;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 2)->translation = car2;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 3)->translation = car3;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 4)->translation = switch0;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 5)->translation = switch1;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 6)->translation = switch2;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 7)->translation = switch3;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 8)->translation = track0;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 9)->translation = track1;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 10)->translation = track2;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, 11)->translation = track3;

    for (i = 0; i < CAR_COUNT; i++) {
        u8 actor = ACTOR_CAR + i;
        u8 anm;
        FieldExpObjAnm *anmInfo;

        if (save->switches[i]) {
            anm = i * 2 + 2;
        } else {
            anm = i * 2 + 1;
        }
        FieldExpObj_SetAnm(expObj, 0, actor, anm, TRUE);
        FieldExpObj_SetAnm(expObj, 0, actor, 0, TRUE);
        anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, actor, 0);
        FieldExpObjAnm_SetPaused(anmInfo, TRUE);
        FieldExpObjAnm_SetLooped(anmInfo, FALSE);
        FieldExpObj_SetAnmFrame(expObj, 0, actor, anm, save->frames[i]);

        actor = ACTOR_SWITCH + i;
        RollerCoaster_ShowSwitchState(expObj, actor, save->switches[i]);
        actor = ACTOR_TRACK + i;
        RollerCoaster_ShowSwitchState(expObj, actor, save->switches[i]);
    }

    work = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    work->task = GFL_TCBMgrAddTask(Field_GetTCBMgr(field), RollerCoaster_Task, field, 0);
    for (j = 0; j < CAR_COUNT; j++) {
        work->pendingSwitch[j] = FALSE;
        work->paths[j] = j * 2 + save->switches[j];
        work->stations[j] = NO_SPOT;
    }
    work->switchId = -1;
    work->waitCar = NO_CAR;
    work->ridePath = NO_PATH;
    FieldSnd_PlayAmbience(fieldSound, SEQ_SE_FLD_25);
    FieldSnd_SetAmbienceVolume(fieldSound, SEQ_SE_FLD_25, 0);
}

static void RollerCoaster_Task(TCB *task, void *data) {
    u8 i;
    Field *field = data;
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    RollerCoasterWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    RollerCoasterSave *save = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_STATE_ID);
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);

    for (i = 0; i < CAR_COUNT; i++) {
        if (work->pendingSwitch[i]) {
            u8 newSwitch = (save->switches[i] + 1) % 2;
            u8 anm = i * 2 + 1 + newSwitch;
            u8 actor = ACTOR_CAR + i;

            if (RollerCoaster_GetAnmFrame(expObj, actor, anm) == 0) {
                RollerCoaster_SwitchPath(expObj, newSwitch, i);
                work->paths[i] = i * 2 + save->switches[i];
                if (work->ridePath != NO_PATH && work->ridePath / 2 == i && work->ridePath != work->paths[i]) {
                    work->ridePath = work->paths[i];
                    GFL_G3DCurveFree(work->curve);
                    work->curve = GFL_G3DCurveCreateToLoadBufferEntire(
                        HEAPID_TAIL(HEAPID_FIELDMAP), ARCID_ROLLER_COASTER, sRideCurves[work->ridePath],
                        work->curveBuffer, sizeof(work->curveBuffer));
                }
                work->pendingSwitch[i] = FALSE;
            }
        }
    }

    for (i = 0; i < CAR_COUNT; i++) {
        u8 path = work->paths[i];
        u8 anm = path + 1;
        u8 actor = ACTOR_CAR + i;
        FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, actor, anm);

        if (work->waits[i] != 0) {
            if (work->waitCar != i && --work->waits[i] == 0) {
                FieldExpObjAnm_SetPaused(anmInfo, FALSE);
                if (work->ridePath != NO_PATH && i == work->ridePath / 2) {
                    s32 player;

                    FieldSnd_PlayAmbience(fieldSound, SEQ_SE_FLD_25);
                    GFL_SndSEPlay(SEQ_SE_FLD_76);
                    player = GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_76);
                    FieldSnd_SetAmbienceVolume(fieldSound, SEQ_SE_FLD_25, 127);
                    NNS_SndPlayerSetVolume(func_020061a8(player), 0);
                }
                work->stations[i] = NO_SPOT;
            }
        } else {
            u8 index;

            if (RollerCoaster_IsAtStation(path, func_ov036_021b8520(expObj, 0, actor, anm), &index)) {
                FieldExpObjAnm_SetPaused(anmInfo, TRUE);
                if (work->ridePath != NO_PATH && i == work->ridePath / 2) {
                    FieldSnd_StopAmbience(fieldSound, SEQ_SE_FLD_25);
                    GFL_SndStop();
                }
                work->waits[i] = STATION_WAIT;
                work->stations[i] = sStationSpots[path][index];
            }
        }
    }

    if (work->riding) {
        VecFx32 pos = { 0, FX32_CONST(32), 0 };
        u8 car = work->ridePath / 2;
        u8 anm = work->ridePath + 1;
        fx32 frame = func_ov036_021b8520(expObj, 0, car, anm);

        if (GFL_G3DCurveGetNowTranslationLoop(work->curve, &pos, frame / FX32_ONE)) {
            pos.x += sCarOrigins[car].x;
            pos.y += sCarOrigins[car].y;
            pos.z += sCarOrigins[car].z;
            FieldPlayer_SetWPos(Field_GetPlayer(field), &pos);
        }
    } else {
        RollerCoaster_UpdateAmbience(field, work);
    }
}

void RollerCoaster_End(Field *field) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    RollerCoasterWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_ID);

    FieldSnd_StopAmbience(GameData_GetFieldSoundSystem(GSYS_GetGameData(Field_GetGameSystem(field))), SEQ_SE_FLD_25);
    GFL_TCBRemove(work->task);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_ID);
    FieldExpObj_FreeScene(expObj, 0);
}

void RollerCoaster_Update(Field *field) {
    int i;
    RollerCoasterSave *save;
    FieldExpObjSystem *expObj;
    RollerCoasterWork *work;

    expObj = Field_GetExpObjSystem(field);
    work = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    save = GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))),
                                    GIMMICK_STATE_ID);
    FieldExpObj_StepAllAnimations(expObj);
    for (i = 0; i < CAR_COUNT; i++) {
        u8 actor = ACTOR_CAR + i;
        u8 anm = work->paths[i] + 1;

        save->frames[i] = RollerCoaster_GetAnmFrame(expObj, actor, anm);
    }
}

void RollerCoaster_StartAmbience(GameSystem *gsys) {
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(GSYS_GetGameData(gsys));

    FieldSnd_PlayAmbience(fieldSound, SEQ_SE_FLD_25);
    FieldSnd_SetAmbienceVolume(fieldSound, SEQ_SE_FLD_25, 0);
}

static u8 RollerCoaster_GetSpotPath(RollerCoasterWork *work, u8 spot) {
    return RollerCoaster_GetCarPath(work, RollerCoaster_GetSpotCar(spot));
}

static u8 RollerCoaster_GetSpotCar(u8 spot) {
    return sSpotCars[spot];
}

static BOOL RollerCoaster_IsCarAtSpot(RollerCoasterWork *work, u8 spot) {
    s8 station = work->stations[RollerCoaster_GetSpotCar(spot)];

    if (station == NO_SPOT) {
        return FALSE;
    }
    return station == spot;
}

static fx32 RollerCoaster_GetAnmFrame(FieldExpObjSystem *expObj, u16 actor, u16 anm) {
    fx32 frame = 0;

    GFL_G3DActorGetAnmFrame(FieldExpObj_GetActor(expObj, 0, actor), anm, &frame);
    return frame;
}

static void RollerCoaster_SetAnmFrame(FieldExpObjSystem *expObj, u16 actor, u16 anm, fx32 frame) {
    GFL_G3DActorSetAnmFrame(FieldExpObj_GetActor(expObj, 0, actor), anm, &frame);
}

GameEvent *RollerCoaster_CreateSwitchEvent(GameSystem *gsys, u8 switchId) {
    Field *field = GSYS_GetField(gsys);
    RollerCoasterWork *work;
    GameEvent *event;

    // The expansion objects and the saved state are fetched but not used
    Field_GetExpObjSystem(field);
    work = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), GIMMICK_STATE_ID);
    if (switchId >= CAR_COUNT) {
        return NULL;
    }
    event = GameEvent_Create(gsys, NULL, RollerCoaster_SwitchEvent, 0);
    work->switchId = switchId;
    return event;
}

static GameEventReturnCode RollerCoaster_SwitchEvent(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    RollerCoasterWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    RollerCoasterSave *save = GimmickState_GetUserData(
        GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), GIMMICK_STATE_ID);

    switch (*state) {
    case 0: {
        u8 switchActor;
        u8 trackActor;

        work->switchState = save->switches[work->switchId];
        switchActor = work->switchId + 4;
        trackActor = work->switchId + 8;
        if (work->switchState == 0) {
            FieldExpObj_SetAnm(expObj, 0, switchActor, 0, FALSE);
            FieldExpObj_SetAnm(expObj, 0, trackActor, 0, FALSE);
            FieldExpObj_SetAnm(expObj, 0, switchActor, 1, TRUE);
            FieldExpObj_SetAnm(expObj, 0, trackActor, 1, TRUE);
            RollerCoaster_SetAnmPaused(expObj, switchActor, 1, FALSE);
            RollerCoaster_SetAnmPaused(expObj, trackActor, 1, FALSE);
            FieldExpObj_SetAnmFrame(expObj, 0, switchActor, 1, 0);
            FieldExpObj_SetAnmFrame(expObj, 0, trackActor, 1, 0);
            work->switchState = 1;
        } else {
            FieldExpObj_SetAnm(expObj, 0, switchActor, 1, FALSE);
            FieldExpObj_SetAnm(expObj, 0, trackActor, 1, FALSE);
            FieldExpObj_SetAnm(expObj, 0, switchActor, 0, TRUE);
            FieldExpObj_SetAnm(expObj, 0, trackActor, 0, TRUE);
            RollerCoaster_SetAnmPaused(expObj, switchActor, 0, FALSE);
            RollerCoaster_SetAnmPaused(expObj, trackActor, 0, FALSE);
            FieldExpObj_SetAnmFrame(expObj, 0, switchActor, 0, 0);
            FieldExpObj_SetAnmFrame(expObj, 0, trackActor, 0, 0);
            work->switchState = 0;
        }
        GFL_SndSEPlay(SEQ_SE_FLD_24);
        (*state)++;
        break;
    }
    case 1: {
        u8 switchActor = work->switchId + 4;
        u8 trackActor = work->switchId + 8;
        FieldExpObjAnm *switchAnm = FieldExpObj_GetAnmInfo(expObj, 0, switchActor, work->switchState);
        FieldExpObjAnm *trackAnm = FieldExpObj_GetAnmInfo(expObj, 0, trackActor, work->switchState);

        if (FieldExpObjAnm_IsPlaybackFinished(switchAnm) && FieldExpObjAnm_IsPlaybackFinished(trackAnm)) {
            RollerCoaster_ToggleSwitch(field, expObj);
            work->switchId = -1;
            return GAMEEVENT_DONE;
        }
        break;
    }
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *RollerCoaster_CheckRide(GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    RollerCoasterSave *save = GimmickState_GetUserData(
        GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), GIMMICK_STATE_ID);
    RollerCoasterWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    FieldPlayer *player = Field_GetPlayer(field);
    VecFx32 pos;
    u8 spot;

    if (FieldPlayer_GetFaceDir(player) != DIR_UP) {
        return NULL;
    }
    FieldPlayer_GetWPos(player, &pos);
    spot = RollerCoaster_FindSpot(pos.x / GRID_SIZE, pos.z / GRID_SIZE);
    if (spot == 0xff) {
        return NULL;
    }
    if (RollerCoaster_IsCarAtSpot(work, spot)) {
        int car = RollerCoaster_GetSpotCar(spot);

        work->ridePath = RollerCoaster_GetSpotPath(work, spot);
        RollerCoaster_SetAnmPaused(Field_GetExpObjSystem(field), car, work->ridePath + 1, TRUE);
        work->waitCar = car;
        if (RollerCoaster_IsIntroNeeded(save, car)) {
            return GameEvent_Create(gsys, NULL, RollerCoaster_IntroEvent, 0);
        }
        return GameEvent_Create(gsys, NULL, RollerCoaster_RideEvent, 0);
    }
    return NULL;
}

void RollerCoaster_SetCarIntroSeen(GameSystem *gsys, u16 car) {
    RollerCoasterSave *save = GimmickState_GetUserData(
        GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(GSYS_GetField(gsys)))), GIMMICK_STATE_ID);

    RollerCoaster_SetIntroSeen(save, car);
}

static BOOL RollerCoaster_IsIntroNeeded(RollerCoasterSave *save, int car) {
    u32 index;
    BOOL valid = FALSE;

    if (car == 1) {
        valid = TRUE;
        index = 0;
    } else if (car == 2) {
        valid = TRUE;
        index = 1;
    }
    if (valid && save->introSeen[index] == 0) {
        return TRUE;
    }
    return FALSE;
}

static void RollerCoaster_SetIntroSeen(RollerCoasterSave *save, u8 car) {
    u32 index;
    BOOL valid = FALSE;

    if (car == 1) {
        valid = TRUE;
        index = 0;
    } else if (car == 2) {
        valid = TRUE;
        index = 1;
    }
    if (valid) {
        save->introSeen[index] = 1;
    }
}

static GameEventReturnCode RollerCoaster_RideEvent(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    RollerCoasterWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(GSYS_GetGameData(Field_GetGameSystem(field)));
    s32 player = GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_76);
    u8 car = work->ridePath / 2;
    u8 actor = ACTOR_CAR + car;
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, actor, 0);
    u8 anm;
    fx32 frame;
    fx32 speed;
    int volume;

    // The saved state is fetched but not used
    GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), GIMMICK_STATE_ID);
    switch (*state) {
    case 0:
        work->curve = GFL_G3DCurveCreateToLoadBufferEntire(HEAPID_TAIL(HEAPID_FIELDMAP), ARCID_ROLLER_COASTER,
                                                           sRideCurves[work->ridePath], work->curveBuffer,
                                                           sizeof(work->curveBuffer));
        RollerCoaster_SetAnmFrame(expObj, actor, 0, 0);
        FieldExpObjAnm_SetPaused(anmInfo, FALSE);
        GFL_SndSEPlay(SEQ_SE_FLD_22);
        (*state)++;
        break;
    case 1:
        if (RollerCoaster_GetAnmFrame(expObj, actor, 0) >= FX32_CONST(15)) {
            FieldExpObjAnm_SetPaused(anmInfo, TRUE);
            EventScriptCall_Start(event, SCRIPT_RIDE_START, NULL, 0, HEAPID_TAIL(HEAPID_FIELDMAP));
            (*state)++;
        }
        break;
    case 2:
        FieldExpObjAnm_SetPaused(anmInfo, FALSE);
        GFL_SndSEPlay(SEQ_SE_FLD_22);
        (*state)++;
        break;
    case 3:
        if (FieldExpObjAnm_IsPlaybackFinished(anmInfo)) {
            SetActorHidden(FieldPlayer_GetActor(Field_GetPlayer(field)), TRUE);
            FieldExpObjAnm_SetPaused(anmInfo, TRUE);
            RollerCoaster_SetAnmPaused(expObj, actor, work->ridePath + 1, FALSE);
            work->waits[car] = 0;
            work->stations[car] = NO_SPOT;
            work->waitCar = NO_CAR;
            FieldSnd_PlayAmbience(fieldSound, SEQ_SE_FLD_25);
            GFL_SndSEPlay(SEQ_SE_FLD_76);
            NNS_SndPlayerSetVolume(func_020061a8(player), 0);
            FieldSnd_SetAmbienceVolume(fieldSound, SEQ_SE_FLD_25, 127);
            work->riding = TRUE;
            (*state)++;
        }
        break;
    case 4:
        if (work->stations[car] != NO_SPOT && (GCTX_HIDGetHeldKeys() & PAD_KEY_DOWN)) {
            (*state)++;
        }
        break;
    case 5: {
        FieldExpObjAnm *pathAnm = RollerCoaster_GetAnm(expObj, actor, work->ridePath + 1);
        FieldActor *playerActor;

        work->waitCar = car;
        FieldExpObjAnm_SetPaused(pathAnm, TRUE);
        RollerCoaster_SetAnmFrame(expObj, actor, 0, 0);
        FieldExpObjAnm_SetPaused(anmInfo, FALSE);
        GFL_SndSEPlay(SEQ_SE_FLD_22);
        playerActor = FieldPlayer_GetActor(Field_GetPlayer(field));
        CheckSetActorFaceDir(playerActor, DIR_DOWN);
        SetActorHidden(playerActor, FALSE);
        (*state)++;
        break;
    }
    case 6:
        if (RollerCoaster_GetAnmFrame(expObj, actor, 0) >= FX32_CONST(15)) {
            FieldExpObjAnm_SetPaused(anmInfo, TRUE);
            {
                FieldPlayer *fieldPlayer = Field_GetPlayer(field);
                VecFx32 pos = { 0, FX32_CONST(32), 0 };
                u8 spot = work->stations[car];

                pos.x = sSpots[spot].x * GRID_SIZE + GRID_SIZE / 2;
                pos.z = (sSpots[spot].z - 2) * GRID_SIZE + GRID_SIZE / 2;
                FieldPlayer_SetWPos(fieldPlayer, &pos);
            }
            work->riding = FALSE;
            (*state)++;
        }
        break;
    case 7:
        EventScriptCall_Start(event, SCRIPT_RIDE_END, NULL, 0, HEAPID_TAIL(HEAPID_FIELDMAP));
        (*state)++;
        break;
    case 8:
        FieldExpObjAnm_SetPaused(anmInfo, FALSE);
        GFL_SndSEPlay(SEQ_SE_FLD_22);
        (*state)++;
        break;
    case 9:
        if (FieldExpObjAnm_IsPlaybackFinished(anmInfo)) {
            FieldExpObjAnm_SetPaused(anmInfo, TRUE);
            RollerCoaster_SetAnmPaused(expObj, actor, work->ridePath + 1, FALSE);
            work->waits[car] = 0;
            work->stations[car] = NO_SPOT;
            GFL_G3DCurveFree(work->curve);
            work->waitCar = NO_CAR;
            work->ridePath = NO_PATH;
            FieldSnd_PlayAmbience(fieldSound, SEQ_SE_FLD_25);
            FieldSnd_SetAmbienceVolume(fieldSound, SEQ_SE_FLD_25, 0);
            GFL_SndPlayerStop(player);
            return GAMEEVENT_DONE;
        }
        break;
    }

    // Paths 2 to 5 go through tunnels, where the ambience stops
    if (work->ridePath == 2 || work->ridePath == 3 || work->ridePath == 4 || work->ridePath == 5) {
        anm = work->ridePath + 1;
        frame = RollerCoaster_GetAnmFrame(expObj, actor, anm);
        switch (work->ridePath) {
        case 2:
            if (frame == PATH2_FRAME_STOP_FLD26) {
                FieldSnd_StopAmbience(fieldSound, SEQ_SE_FLD_25);
                GFL_SndStop();
                GFL_SndSEPlay(SEQ_SE_FLD_26);
            } else if (frame == PATH2_FRAME_STOP) {
                FieldSnd_StopAmbience(fieldSound, SEQ_SE_FLD_25);
                GFL_SndStop();
            } else if (frame == PATH2_FRAME_FLD27) {
                GFL_SndSEPlay(SEQ_SE_FLD_27);
            }
            break;
        case 3:
            if (frame == PATH3_FRAME_STOP_FLD26) {
                FieldSnd_StopAmbience(fieldSound, SEQ_SE_FLD_25);
                GFL_SndStop();
                GFL_SndSEPlay(SEQ_SE_FLD_26);
            } else if (frame == PATH3_FRAME_STOP) {
                FieldSnd_StopAmbience(fieldSound, SEQ_SE_FLD_25);
                GFL_SndStop();
            } else if (frame == PATH3_FRAME_FLD27) {
                GFL_SndSEPlay(SEQ_SE_FLD_27);
            }
            break;
        case 4:
            if (frame == PATH4_FRAME_STOP_FLD67) {
                FieldSnd_StopAmbience(fieldSound, SEQ_SE_FLD_25);
                GFL_SndStop();
                GFL_SndSEPlay(SEQ_SE_FLD_67);
            }
            break;
        case 5:
            if (frame == PATH5_FRAME_STOP_FLD67) {
                FieldSnd_StopAmbience(fieldSound, SEQ_SE_FLD_25);
                GFL_SndStop();
                GFL_SndSEPlay(SEQ_SE_FLD_67);
            }
            break;
        }
    }

    // The ride's sound is louder the faster the car goes: how far the car moves in a frame, as RollerCoaster_UpdateAmbience
    // works it out for the nearest car
    {
        G3DActor *g3dActor = FieldExpObj_GetActor(expObj, 0, actor);
        fx32 lastFrame;
        fx32 nextFrame;
        NNSG3dAnmObj *anmObj;
        VecFx32 trans;
        VecFx32 nextTrans;
        VecFx32 move;

        anm = work->ridePath + 1;
        lastFrame = func_ov036_021b8580(FieldExpObj_GetAnmInfo(expObj, 0, actor, anm));
        frame = RollerCoaster_GetAnmFrame(expObj, actor, anm);
        nextFrame = frame + FX32_ONE;
        if (nextFrame >= lastFrame) {
            nextFrame = 0;
        }
        anmObj = GFL_G3DAnmGetRenderObj(GFL_G3DActorGetAnm(g3dActor, anm));
        RollerCoaster_GetJointTrans(anmObj->resAnm, 0, frame, &trans);
        RollerCoaster_GetJointTrans(anmObj->resAnm, 0, nextFrame, &nextTrans);
        VEC_Subtract(&trans, &nextTrans, &move);
        speed = VEC_Mag(&move);
    }
    volume = 0;
    if (speed >= SPEED_FASTEST) {
        volume = 127;
    } else if (speed >= SPEED_FAST) {
        volume = 90;
    } else if (speed >= SPEED_MEDIUM) {
        volume = 60;
    } else if (speed >= SPEED_SLOW) {
        volume = 30;
    }
    NNS_SndPlayerSetVolume(func_020061a8(player), volume);
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode RollerCoaster_IntroEvent(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    RollerCoasterWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    u8 car = work->ridePath / 2;
    u8 actor = ACTOR_CAR + car;
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, actor, 0);

    // The saved state is fetched but not used
    GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), GIMMICK_STATE_ID);
    switch (*state) {
    case 0:
        EventScriptCall_Start(event, SCRIPT_INTRO_START, NULL, 0, HEAPID_TAIL(HEAPID_FIELDMAP));
        (*state)++;
        break;
    case 1:
        RollerCoaster_SetAnmFrame(expObj, actor, 0, 0);
        FieldExpObjAnm_SetPaused(anmInfo, FALSE);
        GFL_SndSEPlay(SEQ_SE_FLD_22);
        (*state)++;
        break;
    case 2:
        if (RollerCoaster_GetAnmFrame(expObj, actor, 0) >= FX32_CONST(15)) {
            FieldExpObjAnm_SetPaused(anmInfo, TRUE);
            EventScriptCall_Start(event, SCRIPT_INTRO_RIDE, NULL, 0, HEAPID_TAIL(HEAPID_FIELDMAP));
            (*state)++;
        }
        break;
    case 3:
        FieldExpObjAnm_SetPaused(anmInfo, FALSE);
        GFL_SndSEPlay(SEQ_SE_FLD_22);
        (*state)++;
        break;
    case 4:
        if (FieldExpObjAnm_IsPlaybackFinished(anmInfo)) {
            FieldExpObjAnm_SetPaused(anmInfo, TRUE);
            RollerCoaster_SetAnmPaused(expObj, actor, work->ridePath + 1, FALSE);
            work->waits[car] = 0;
            work->stations[car] = NO_SPOT;
            work->ridePath = NO_PATH;
            EventScriptCall_Replace(event, SCRIPT_INTRO_END, NULL, HEAPID_TAIL(HEAPID_FIELDMAP));
            (*state)++;
        }
        break;
    case 5:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void RollerCoaster_ToggleSwitch(Field *field, FieldExpObjSystem *expObj) {
    RollerCoasterWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_ID);
    RollerCoasterSave *save = GimmickState_GetUserData(
        GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), GIMMICK_STATE_ID);
    s8 switchId = work->switchId;
    u8 oldSwitch;

    if (switchId == -1) {
        return;
    }
    oldSwitch = save->switches[switchId];
    save->switches[switchId] = (oldSwitch + 1) % 2;
    if (RollerCoaster_IsBeforeSwitch(expObj, switchId, work->paths[work->switchId])) {
        RollerCoaster_SwitchPath(expObj, oldSwitch, work->switchId);
        work->paths[work->switchId] = work->switchId * 2 + save->switches[work->switchId];
    } else if (work->pendingSwitch[work->switchId]) {
        work->pendingSwitch[work->switchId] = FALSE;
    } else {
        work->pendingSwitch[work->switchId] = TRUE;
    }
}

// Moves a car to its other path at the same frame
static void RollerCoaster_SwitchPath(FieldExpObjSystem *expObj, u8 oldSwitch, u8 car) {
    u8 oldAnm;
    u8 newAnm;
    u8 actor;
    fx32 frame;
    u8 paused;

    if (oldSwitch == 0) {
        oldAnm = car * 2 + 1;
        newAnm = oldAnm + 1;
    } else {
        newAnm = car * 2 + 1;
        oldAnm = newAnm + 1;
    }
    actor = ACTOR_CAR + car;
    frame = RollerCoaster_GetAnmFrame(expObj, actor, oldAnm);
    FieldExpObj_SetAnm(expObj, 0, actor, oldAnm, FALSE);
    FieldExpObj_SetAnm(expObj, 0, actor, newAnm, TRUE);
    RollerCoaster_SetAnmFrame(expObj, actor, newAnm, frame);
    paused = func_ov036_021b84ec(FieldExpObj_GetAnmInfo(expObj, 0, actor, oldAnm));
    FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(expObj, 0, actor, newAnm), paused);
}

static BOOL RollerCoaster_IsAtStation(u8 path, fx32 frame, u8 *index) {
    u8 i;

    for (i = 0; i < STATIONS_PER_PATH; i++) {
        if (frame == sStationFrames[path][i]) {
            *index = i;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL RollerCoaster_IsBeforeSwitch(FieldExpObjSystem *expObj, u8 car, u8 path) {
    u8 actor = ACTOR_CAR + car;

    if (RollerCoaster_GetAnmFrame(expObj, actor, path + 1) <= sSwitchFrames[car] * FX32_ONE) {
        return TRUE;
    }
    return FALSE;
}

// The translation of a joint of an animation at a frame, as NitroSystem's joint animation computes it
static void RollerCoaster_GetJointTrans(const NNSG3dResJntAnm *anm, u32 node, fx32 frame, VecFx32 *trans) {
    const u32 *tag = (const u32 *)((const u8 *)anm + anm->ofsTag[node]);
    u32 flags = *tag;
    const u32 *data = tag + 1;
    fx32 x;
    fx32 y;
    fx32 z;

    if (flags & (NNS_G3D_JNTANM_SRTINFO_IDENTITY_T | NNS_G3D_JNTANM_SRTINFO_BASE_T)) {
        return;
    }
    if (!(flags & NNS_G3D_JNTANM_SRTINFO_CONST_TX)) {
        RollerCoaster_GetTransData(&x, frame, data, anm);
        data += 2;
        trans->x = x;
    }
    if (!(flags & NNS_G3D_JNTANM_SRTINFO_CONST_TY)) {
        RollerCoaster_GetTransData(&y, frame, data, anm);
        data += 2;
        trans->y = y;
    }
    if (!(flags & NNS_G3D_JNTANM_SRTINFO_CONST_TZ)) {
        RollerCoaster_GetTransData(&z, frame, data, anm);
        trans->z = z;
    }
}

// A translation component at a frame, from values stored every 1, 2 or 4 frames and interpolated between, as
// NitroSystem's joint animation reads them. The two gotos are the shared ends that the original jumps to from several
// branches: written as calls of inline helpers, each branch gets a copy and the function is 0x90 bytes longer. The
// interpolation reads its pair of values as `*(array + index)` and `*(array + index + 1)`, which offsets the second
// read from the first, as the original does; `array[index + 1]` computes its address again
static void RollerCoaster_GetTransData(fx32 *value, fx32 frame, const u32 *data, const NNSG3dResJntAnm *anm) {
    u32 whole = frame >> FX32_SHIFT;
    const void *values = (const u8 *)anm + data[1];
    u32 lastInterp;
    u32 index;
    u32 subIndex;

    if (!(data[0] & NNS_G3D_JNTANM_TINFO_STEP_MASK)) {
        index = whole;
        goto noInterp;
    }
    lastInterp = (data[0] & NNS_G3D_JNTANM_TINFO_LAST_INTERP_MASK) >> NNS_G3D_JNTANM_TINFO_LAST_INTERP_SHIFT;
    if (data[0] & NNS_G3D_JNTANM_TINFO_STEP_2) {
        if (whole & 1) {
            if (whole > lastInterp) {
                index = (lastInterp >> 1) + 1;
                goto noInterp;
            } else {
                index = whole >> 1;
                goto interp2;
            }
        } else {
            index = whole >> 1;
            goto noInterp;
        }
    } else {
        if (whole & 3) {
            if (whole > lastInterp) {
                index = (lastInterp >> 2) + (whole & 3);
                goto noInterp;
            }
            if (whole & 1) {
                fx32 v;
                fx32 subV;

                if (whole & 2) {
                    subIndex = whole >> 2;
                    index = subIndex + 1;
                } else {
                    index = whole >> 2;
                    subIndex = index + 1;
                }
                if (data[0] & NNS_G3D_JNTANM_TINFO_FX16ARR) {
                    const fx16 *array = values;

                    v = array[index];
                    subV = array[subIndex];
                    *value = (v + v + v + subV) >> 2;
                } else {
                    const fx32 *array = values;
                    fx64 tripled = (fx64)array[index] * 3;
                    fx64 sub = array[subIndex];

                    *value = (fx32)((tripled + sub) >> 2);
                }
                return;
            } else {
                index = whole >> 2;
                goto interp2;
            }
        } else {
            index = whole >> 2;
            goto noInterp;
        }
    }

interp2:
    if (data[0] & NNS_G3D_JNTANM_TINFO_FX16ARR) {
        const fx16 *array = values;

        *value = (*(array + index) + *(array + index + 1)) >> 1;
    } else {
        const fx32 *array = values;

        *value = (*(array + index) >> 1) + (*(array + index + 1) >> 1);
    }
    return;

noInterp:
    if (data[0] & NNS_G3D_JNTANM_TINFO_FX16ARR) {
        *value = ((const fx16 *)values)[index];
    } else {
        *value = ((const fx32 *)values)[index];
    }
}

// Sets the ambience's volume by the speed of the car nearest the player
static void RollerCoaster_UpdateAmbience(Field *field, RollerCoasterWork *work) {
    fx32 nearestDist;
    int nearestCar;
    int nearestAnm;
    BOOL first;
    FieldSound *fieldSound;
    FieldExpObjSystem *expObj;
    int i;
    VecFx32 pos;
    VecFx32 playerPos;
    VecFx32 diff;
    VecFx32 trans;
    VecFx32 nextTrans;
    VecFx32 move;
    fx32 dist;
    G3DActor *actor;
    fx32 frame;
    fx32 speed;
    u16 volume;

    expObj = Field_GetExpObjSystem(field);
    fieldSound = GameData_GetFieldSoundSystem(GSYS_GetGameData(Field_GetGameSystem(field)));
    first = TRUE;
    for (i = 0; i < CAR_COUNT; i++) {
        u8 car = i;
        u8 carActor = ACTOR_CAR + car;
        u8 anm;

        actor = FieldExpObj_GetActor(expObj, 0, carActor);
        anm = work->paths[car] + 1;
        // The animation's info is fetched but not used
        FieldExpObj_GetAnmInfo(expObj, 0, carActor, anm);
        frame = RollerCoaster_GetAnmFrame(expObj, carActor, anm);
        RollerCoaster_GetJointTrans(GFL_G3DAnmGetRenderObj(GFL_G3DActorGetAnm(actor, anm))->resAnm, 0, frame, &pos);
        VEC_Add(&pos, &sCarOrigins[car], &pos);
        FieldPlayer_GetWPos(Field_GetPlayer(field), &playerPos);
        VEC_Subtract(&pos, &playerPos, &diff);
        dist = VEC_Mag(&diff);
        // The original compares nearestDist before testing first, so the first car reads it before it is set
        if (nearestDist > dist || first) {
            nearestDist = dist;
            nearestCar = carActor;
            nearestAnm = work->paths[car] + 1;
            first = FALSE;
        }
    }

    if (nearestDist >= AMBIENCE_RANGE) {
        volume = 0;
    } else {
        G3DActor *nearestActor = FieldExpObj_GetActor(expObj, 0, nearestCar);
        fx32 lastFrame;
        fx32 nextFrame;
        NNSG3dAnmObj *anmObj;

        lastFrame = func_ov036_021b8580(FieldExpObj_GetAnmInfo(expObj, 0, nearestCar, nearestAnm));
        frame = RollerCoaster_GetAnmFrame(expObj, nearestCar, nearestAnm);
        nextFrame = frame + FX32_ONE;
        if (nextFrame >= lastFrame) {
            nextFrame = 0;
        }
        anmObj = GFL_G3DAnmGetRenderObj(GFL_G3DActorGetAnm(nearestActor, nearestAnm));
        RollerCoaster_GetJointTrans(anmObj->resAnm, 0, frame, &trans);
        RollerCoaster_GetJointTrans(anmObj->resAnm, 0, nextFrame, &nextTrans);
        VEC_Subtract(&trans, &nextTrans, &move);
        speed = VEC_Mag(&move);
        volume = 0;
        if (speed >= SPEED_FASTEST) {
            volume = 127;
        } else if (speed >= SPEED_FAST) {
            volume = 120;
        } else if (speed >= SPEED_MEDIUM) {
            volume = 90;
        } else if (speed >= SPEED_SLOW) {
            volume = 60;
        }
    }
    FieldSnd_SetAmbienceVolume(fieldSound, SEQ_SE_FLD_25, volume);
}
