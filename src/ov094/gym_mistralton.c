#include "types.h"
#include "constants/sound.h"
#include "field/event_data.h"
#include "field/event_mapchange.h"
#include "field/event_trainer_eye.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_sound.h"
#include "field/field_task.h"
#include "field/gym_mistralton.h"
#include "field/trainer_script.h"
#include "field/zone.h"
#include "gfl/calctool.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// Mistralton City's gym. A long room with a grid of fans and a cannon at the top: the player steps on the cannon, is
// shot down the room past the fans and a pair of doors, and ends up in the next room by a warp. The fans spin faster
// the longer the player stays (the stage, 0 to 2, from the frames spent in the gym), and the ride gets rougher.

// The gimmick's block of work, and the gimmick state's slot for what it saves
#define WORK_BLOCK 1
#define SAVE_SLOT 5

#define SCENE 0
// The scene's actors: the centre piece, 17 fans, 2 doors, and 3 wind effects
#define ACTOR_CENTER 0
#define ACTOR_FAN_FIRST 1
#define FAN_COUNT 17
#define ACTOR_DOOR_FIRST 0x12
#define DOOR_COUNT 2
#define ACTOR_EFFECT_FIRST 0x14
#define EFFECT_COUNT 3

#define GRID_ROWS 51
#define GRID_COLUMNS 23
// What a cell of the grid is: where the player is stopped, where the first step is, where the cannon is, and where the
// player is held up
#define GRID_FREE 0
#define GRID_WALL 1
#define GRID_START 2
#define GRID_CANNON 3
#define GRID_PLATFORM 4

// The frames in the gym at which the stage becomes 1 and 2
#define STAGE_1_FRAMES 0x3c
#define STAGE_2_FRAMES 0x78

#define ANM_SPEED_CENTER_STEP 0x260
#define ANM_SPEED_FAN_STEP 0x300
#define FAN_PITCH_STEP 0x10

// The acmds that the ride sets on the player's actor: face a direction, jump and turn on the spot
#define ACMD_FACE_DIR 0
#define ACMD_JUMP 0x4b
#define ACMD_FACE_DIR_BASE 0x24

#define FORME_RIDING 5

// What the gym saves in the gimmick state
typedef struct {
    // 1 once the player has stepped on the start
    int started;
    u16 frames;
} MistraltonSave;

// How a fan animation speeds up or down
typedef struct {
    fx32 speed;
    fx32 targetSpeed;
    FieldExpObjSystem *system;
} AnmSpeed;

// How the fan ambience's pitch moves
typedef struct {
    s32 pitch;
    s32 targetPitch;
} FanPitch;

typedef struct {
    u16 unused00;
    u8 stage;
    // 1 once the player has stepped on the start
    int started;
    u16 frames;
    // The effects' alpha, 0 to 31
    s8 alpha[2];
    // 1 while an effect fades in
    int fadingIn[2];
    // 1 while the effects fade out
    int fadingOut;
    AnmSpeed centerAnm;
    AnmSpeed fanAnm;
    FanPitch fanPitch;
    FieldExpObjSystem *system;
    FieldPlayer *player;
    FieldSound *sound;
    MistraltonSave *save;
} MistraltonWork;

// What the ride events keep: the cannon ride, the bump and the start trigger share it
typedef struct {
    GameSystem *gsys;
    Field *field;
    FieldPlayer *player;
    FieldActor *actor;
    // How the ride was stopped: 0 by a wall, 1 by an actor, 2 if it wasn't yet
    int stopReason;
    u16 timer;
    u8 turns;
    // The cell ahead is a wall or has an actor on it
    int blocked;
    u16 startTimer;
} RideEvent;

typedef struct {
    GameSystem *gsys;
    Field *field;
    FieldPlayer *player;
    FieldActor *actor;
    u8 turns;
} CannonEvent;

typedef struct {
    u8 state;
    Field *field;
    int frame;
    int duration;
    VecFx32 start;
    VecFx32 end;
    FieldActor *actor;
} LerpTask;

typedef struct {
    u8 state;
    Field *field;
    int unused08;
    VecFx32 unused0c;
    FieldActor *actor;
    s16 amplitude;
    u8 count;
    u32 period;
    u32 frame;
} BobTask;

// A position in the gym, in tiles
typedef struct {
    int x;
    int y;
    int z;
} TilePos;

static void MistraltonSave_SetStarted(MistraltonSave *save, int started);
static int MistraltonSave_GetStarted(MistraltonSave *save);
static void MistraltonSave_SetFrames(MistraltonSave *save, u16 frames);
static u16 MistraltonSave_GetFrames(MistraltonSave *save);
static FieldTask *Mistralton_CreateLerpTask(Field *field, FieldActor *actor, u32 duration, VecFx32 *end);
static BOOL MistraltonLerpTask_Run(void *data);
static FieldTask *Mistralton_CreateBobTask(Field *field, FieldActor *actor, u16 amplitude, u32 period, u8 count);
static BOOL MistraltonBobTask_Run(void *data);
static void Mistralton_ApplyFrames(MistraltonWork *work, u16 frames);
static u32 Mistralton_GetStage(u32 frames);
static void Mistralton_SetStage(Field *field, int stage);
static GameEvent *Mistralton_CreateRideEvent(GameEvent *parent, GameSystem *gsys);
static GameEventReturnCode MistraltonTravelEvent_Run(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode MistraltonBumpEvent_Run(GameEvent *event, u32 *state, void *data);
static GameEvent *Mistralton_CreateCannonEvent(GameEvent *parent, GameSystem *gsys);
static GameEventReturnCode MistraltonCannonEvent_Run(GameEvent *event, u32 *state, void *data);
static GameEvent *Mistralton_CreateTriggerEvent(GameEvent *parent, GameSystem *gsys, u16 actorId);
static GameEventReturnCode MistraltonTriggerEvent_Run(GameEvent *event, u32 *state, void *data);
static u8 Mistralton_CountFreeCells(Field *field, int *stopReason, s16 x, s16 z);
static BOOL Mistralton_StopTrainerAhead(Field *field, FieldPlayer *player);
static void Mistralton_LoadScene(MistraltonWork *work);
static void Mistralton_FreeScene(MistraltonWork *work);
static void Mistralton_StepAnimations(MistraltonWork *work);
static void Mistralton_StepCenterAnm(AnmSpeed *anm);
static BOOL Mistralton_IsCenterAnmDone(AnmSpeed *anm);
static void Mistralton_StepFanAnm(AnmSpeed *anm);
static BOOL Mistralton_IsFanAnmDone(AnmSpeed *anm);
static void Mistralton_StepFanPitch(FanPitch *pitch, u16 frames);
static void Mistralton_SetFanPitchTarget(FanPitch *pitch, u32 stage);

static const G3DSceneAnimationSetup sMistraltonAnimations12[] = { { 12, 0 } };
static const G3DSceneAnimationSetup sMistraltonAnimations08[] = { { 8, 0 } };
static const G3DSceneAnimationSetup sMistraltonAnimations10[] = { { 10, 0 } };
static const G3DSceneAnimationSetup sMistraltonAnimations03[] = { { 3, 0 } };
static const G3DSceneAnimationSetup sMistraltonAnimations01[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sMistraltonAnimations05[] = { { 5, 0 }, { 6, 0 } };

static const G3DSceneResourceSetup sMistraltonResources[] = {
    { 0x7a, 0, 0, 0 }, { 0x7a, 1, 0, 0 }, { 0x7a, 2, 0, 0 }, { 0x7a, 3, 0, 0 },  { 0x7a, 4, 0, 0 },
    { 0x7a, 5, 0, 0 }, { 0x7a, 6, 0, 0 }, { 0x7a, 7, 0, 0 }, { 0x7a, 8, 0, 0 },  { 0x7a, 9, 0, 0 },
    { 0x7a, 10, 0, 0 }, { 0x7a, 11, 0, 0 }, { 0x7a, 12, 0, 0 },
};

#define FAN_ACTOR { 2, 0, 2, 0, sMistraltonAnimations03, NELEMS(sMistraltonAnimations03) }

static const G3DSceneActorSetup sMistraltonActors[] = {
    { 0, 0, 0, 0, sMistraltonAnimations01, NELEMS(sMistraltonAnimations01) },
    FAN_ACTOR, FAN_ACTOR, FAN_ACTOR, FAN_ACTOR, FAN_ACTOR, FAN_ACTOR, FAN_ACTOR, FAN_ACTOR,
    FAN_ACTOR, FAN_ACTOR, FAN_ACTOR, FAN_ACTOR, FAN_ACTOR, FAN_ACTOR, FAN_ACTOR, FAN_ACTOR,
    FAN_ACTOR,
    { 4, 0, 4, 0, sMistraltonAnimations05, NELEMS(sMistraltonAnimations05) },
    { 4, 0, 4, 0, sMistraltonAnimations05, NELEMS(sMistraltonAnimations05) },
    { 7, 0, 7, 0, sMistraltonAnimations08, NELEMS(sMistraltonAnimations08) },
    { 9, 0, 9, 0, sMistraltonAnimations10, NELEMS(sMistraltonAnimations10) },
    { 11, 0, 11, 0, sMistraltonAnimations12, NELEMS(sMistraltonAnimations12) },
};

static const G3DSceneSetup sMistraltonScene = { sMistraltonResources, NELEMS(sMistraltonResources), sMistraltonActors, NELEMS(sMistraltonActors) };

static const int sMistraltonGrid[GRID_ROWS][GRID_COLUMNS] = {
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 4, 4, 4, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 4, 4, 4, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0 },
    { 0, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
    { 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
    { 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

void GymMistralton_Init(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    MistraltonWork *work;

    work = Field_AllocGimmickWorkBlock(field, WORK_BLOCK, Field_GetHeapID(field), sizeof(MistraltonWork));
    work->unused00 = ACTOR_EFFECT_FIRST + 1;
    work->alpha[0] = 0;
    work->alpha[1] = 0;
    work->fadingIn[0] = 0;
    work->fadingIn[1] = 0;
    work->fadingOut = 0;
    work->stage = 0;
    work->system = Field_GetExpObjSystem(field);
    work->player = Field_GetPlayer(field);
    work->sound = GameData_GetFieldSoundSystem(gameData);
    work->save = GimmickState_GetUserData(GameData_GetGimmickState(gameData), SAVE_SLOT);
    work->centerAnm.system = work->system;
    work->fanAnm.system = work->system;
    work->started = MistraltonSave_GetStarted(work->save);
    work->frames = MistraltonSave_GetFrames(work->save);
    Mistralton_ApplyFrames(work, work->frames);
}

void GymMistralton_End(Field *field) {
    MistraltonWork *work = Field_GetGimmickWorkBlock(field, WORK_BLOCK);

    MistraltonSave_SetStarted(work->save, work->started);
    MistraltonSave_SetFrames(work->save, work->frames);
    FieldSnd_StopAmbience(work->sound, SEQ_SE_GYM_H02);
    Mistralton_FreeScene(work);
    Field_DeleteGimmickWorkBlock(field, WORK_BLOCK);
}

void GymMistralton_Update(Field *field) {
    MistraltonWork *work = Field_GetGimmickWorkBlock(field, WORK_BLOCK);
    u8 i;

    for (i = 0; i < EFFECT_COUNT - 1; i++) {
        if (work->fadingIn[i] == 1) {
            work->alpha[i] += 2;
            if (work->alpha[i] > 31) {
                work->alpha[i] = 31;
                work->fadingIn[i] = 0;
            }
        }
        if (work->fadingOut == 1) {
            if (i == 1) {
                if (work->alpha[i] > 0) {
                    work->alpha[i] -= 2;
                }
                if (work->alpha[i] < 0) {
                    FieldExpObj_SetActorHidden(work->system, SCENE, ACTOR_EFFECT_FIRST, FALSE);
                }
            }
            if (i == 0 && work->alpha[1] == 0) {
                if (work->alpha[i] > 0) {
                    work->alpha[i] -= 4;
                }
            }
            if (work->alpha[i] < 0) {
                work->alpha[i] = 0;
                FieldExpObj_SetActorHidden(work->system, SCENE, ACTOR_EFFECT_FIRST + 1 + i, TRUE);
                FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(work->system, SCENE, ACTOR_EFFECT_FIRST + 1 + i, 0),
                                         TRUE);
                if (i == 0) {
                    work->fadingOut = 0;
                }
            }
        }
        func_02068410(GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(
                                                   FieldExpObj_GetActor(work->system, SCENE, ACTOR_EFFECT_FIRST + 1 + i)))
                          ->resMdl,
                      work->alpha[i]);
    }
    Mistralton_StepAnimations(work);
    Mistralton_StepFanPitch(&work->fanPitch, work->frames);
}

GameEvent *GymMistralton_CheckStep(GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    MistraltonWork *work = Field_GetGimmickWorkBlock(field, WORK_BLOCK);
    FieldPlayer *player = Field_GetPlayer(field);
    s16 x;
    s16 y;
    s16 z;
    u32 stage;

    FieldPlayer_GetGPos(player, &x, &y, &z);
    if (work == NULL) {
        return NULL;
    }
    if (work->started == 0) {
        if (sMistraltonGrid[z][x] == GRID_START) {
            work->started = 1;
            MistraltonSave_SetStarted(work->save, 1);
        }
        return NULL;
    }
    if (sMistraltonGrid[z][x] == GRID_PLATFORM) {
        return NULL;
    }
    if (sMistraltonGrid[z][x] == GRID_CANNON) {
        return Mistralton_CreateCannonEvent(NULL, gsys);
    }
    work->frames++;
    stage = Mistralton_GetStage(work->frames);
    if (work->stage != stage) {
        Mistralton_SetStage(field, stage);
        if (work->stage == 2) {
            return Mistralton_CreateRideEvent(NULL, gsys);
        }
    }
    return NULL;
}

GameEvent *GymMistralton_StartCannon(GameSystem *gsys, u16 actorId) {
    Field *field = GSYS_GetField(gsys);

    Field_GetGimmickWorkBlock(field, WORK_BLOCK);
    Field_GetPlayer(field);
    return Mistralton_CreateTriggerEvent(NULL, gsys, actorId);
}

static FieldTask *Mistralton_CreateLerpTask(Field *field, FieldActor *actor, u32 duration, VecFx32 *end) {
    FieldTask *task = FieldTask_Create(Field_GetHeapID(field), sizeof(LerpTask), MistraltonLerpTask_Run);
    LerpTask *data = FieldTask_GetData(task);

    data->state = 0;
    data->field = field;
    data->frame = 0;
    data->duration = duration;
    VEC_Set(&data->end, end->x, end->y, end->z);
    data->actor = actor;
    return task;
}

static BOOL MistraltonLerpTask_Run(void *taskData) {
    LerpTask *data = taskData;
    VecFx32 fromStart;
    VecFx32 toEnd;
    VecFx32 pos;
    fx32 t;
    int duration;

    switch (data->state) {
    case 0:
        CopyActorWPos(data->actor, &data->start);
        data->state++;
        break;
    case 1:
        data->frame++;
        duration = data->duration;
        t = FX32_CONST((f32)data->frame / (f32)duration);
        vecfx_mul(&data->start, FX32_ONE - t, &fromStart);
        vecfx_mul(&data->end, t, &toEnd);
        VEC_Add(&fromStart, &toEnd, &pos);
        SetActorWPosValue(data->actor, &pos);
        CheckRecalcActorY(data->actor);
        if (data->duration <= data->frame) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static FieldTask *Mistralton_CreateBobTask(Field *field, FieldActor *actor, u16 amplitude, u32 period, u8 count) {
    FieldTask *task = FieldTask_Create(Field_GetHeapID(field), sizeof(BobTask), MistraltonBobTask_Run);
    BobTask *data = FieldTask_GetData(task);

    data->state = 0;
    data->field = field;
    data->unused08 = 0;
    data->actor = actor;
    CopyActorWPos(actor, &data->unused0c);
    data->amplitude = amplitude;
    data->period = period;
    data->count = count;
    data->frame = 0;
    return task;
}

static BOOL MistraltonBobTask_Run(void *taskData) {
    BobTask *data = taskData;
    BOOL done = FALSE;
    VecFx32 offset;
    u32 period;
    u32 frame;
    u16 angle;

    switch (data->state) {
    case 0:
        period = data->period;
        frame = data->frame + 1;
        data->frame = frame;
        angle = (frame << 16) / period;
        if (frame >= period) {
            data->frame = 0;
            data->count--;
        }
        if (data->count == 0) {
            data->frame = 0;
            done = TRUE;
        }
        if (done) {
            offset.x = 0;
            offset.y = 0;
            offset.z = 0;
            SetActorWPosOffset(data->actor, &offset);
            data->state++;
        } else {
            offset.x = FX_SIN_COS_TABLE[(angle >> 4) * 2] * data->amplitude;
            offset.y = 0;
            offset.z = 0;
            SetActorWPosOffset(data->actor, &offset);
        }
        break;
    case 1:
        return TRUE;
    }
    return FALSE;
}

static void Mistralton_ApplyFrames(MistraltonWork *work, u16 frames) {
    u32 stage = Mistralton_GetStage(frames);
    const fx32 centerSpeeds[4] = { FX32_CONST(1), FX32_CONST(2), FX32_CONST(3), FX32_CONST(4) };
    const fx32 fanSpeeds[4] = { FX32_CONST(2), FX32_CONST(5), FX32_CONST(11), FX32_CONST(15) };

    work->stage = stage;
    work->centerAnm.speed = centerSpeeds[work->stage];
    work->centerAnm.targetSpeed = centerSpeeds[work->stage];
    work->fanAnm.speed = fanSpeeds[work->stage];
    work->fanAnm.targetSpeed = fanSpeeds[work->stage];
    Mistralton_LoadScene(work);
    Mistralton_SetFanPitchTarget(&work->fanPitch, stage);
    work->fanPitch.pitch = work->fanPitch.targetPitch;
}

static u32 Mistralton_GetStage(u32 frames) {
    if (frames >= STAGE_2_FRAMES) {
        return 2;
    }
    if (frames >= STAGE_1_FRAMES) {
        return 1;
    }
    return 0;
}

static void Mistralton_SetStage(Field *field, int stage) {
    MistraltonWork *work = Field_GetGimmickWorkBlock(field, WORK_BLOCK);
    const fx32 centerSpeeds[4] = { FX32_CONST(1), FX32_CONST(2), FX32_CONST(3), FX32_CONST(4) };
    const fx32 fanSpeeds[4] = { FX32_CONST(2), FX32_CONST(5), FX32_CONST(11), FX32_CONST(15) };
    u32 count;
    u16 actor;
    u8 i;

    work->stage = stage;
    work->centerAnm.targetSpeed = centerSpeeds[work->stage];
    work->fanAnm.targetSpeed = fanSpeeds[work->stage];
    switch (work->stage) {
    case 0:
        work->fadingOut = 1;
        break;
    case 1:
        count = 1;
        actor = ACTOR_EFFECT_FIRST + 1;
        break;
    case 2:
        count = 1;
        work->frames = 0;
        FieldExpObj_SetActorHidden(work->system, SCENE, ACTOR_EFFECT_FIRST, TRUE);
        actor = ACTOR_EFFECT_FIRST + 2;
        break;
    case 3:
        count = 2;
        actor = ACTOR_EFFECT_FIRST + 1;
        break;
    }
    if (work->stage != 0) {
        for (i = 0; i < count; i++) {
            FieldExpObjAnm *anm;

            FieldExpObj_SetActorHidden(work->system, SCENE, actor, FALSE);
            anm = FieldExpObj_GetAnmInfo(work->system, SCENE, actor, 0);
            if (func_ov036_021b84ec(anm) == TRUE) {
                FieldExpObj_SetAnmFrame(work->system, SCENE, actor, 0, 0);
                FieldExpObjAnm_SetPaused(anm, FALSE);
            }
            work->fadingIn[actor - (ACTOR_EFFECT_FIRST + 1) + i] = 1;
        }
    }
    Mistralton_SetFanPitchTarget(&work->fanPitch, stage);
}

static GameEvent *Mistralton_CreateRideEvent(GameEvent *parent, GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    FieldPlayer *player = Field_GetPlayer(field);
    GameEvent *event;
    RideEvent *data;
    s16 x;
    s16 y;
    s16 z;

    FieldPlayer_GetGPos(player, &x, &y, &z);
    if (sMistraltonGrid[z][x] != GRID_FREE) {
        event = GameEvent_Create(gsys, parent, MistraltonBumpEvent_Run, sizeof(RideEvent));
    } else {
        event = GameEvent_Create(gsys, parent, MistraltonTravelEvent_Run, sizeof(RideEvent));
    }
    data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->player = player;
    data->actor = FieldPlayer_GetActor(player);
    data->stopReason = 2;
    data->timer = 0;
    data->startTimer = 0;
    data->turns = 0;
    if (sMistraltonGrid[z + 1][x] == GRID_WALL || GetFirstActorOnGPos(Field_GetActorSystem(field), x, z + 1, FALSE) != NULL) {
        data->blocked = TRUE;
    } else {
        data->blocked = FALSE;
    }
    return event;
}

static GameEventReturnCode MistraltonTravelEvent_Run(GameEvent *event, u32 *state, void *eventData) {
    RideEvent *data = eventData;
    MistraltonWork *work;
    FieldTask *task;
    VecFx32 pos;
    FieldCameraShakeSetup shake;
    s16 x;
    s16 y;
    s16 z;
    u16 dir;
    u8 distance;
    // The frames after which the bump ends and the ride waits for the tasks to finish
    static const u16 sEndFrames[2] = { 0x5a, 0x78 };

    work = Field_GetGimmickWorkBlock(data->field, WORK_BLOCK);
    switch (*state) {
    case 0:
        if (func_ov012_02166ecc(data->actor)) {
            FldAct_SetAcmd(data->actor, ACMD_JUMP);
            GFL_SndSEPlay(SEQ_SE_GYM_H07);
            *state = 1;
        }
        break;
    case 1:
        if (!GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H05))) {
            GFL_SndSEPlay(SEQ_SE_GYM_H05);
            *state = 2;
        }
        break;
    case 2:
        if (func_ov012_02166ecc(data->actor)) {
            dir = (s32)FieldPlayer_GetFaceDir(data->player) % 4;
            FldAct_SetAcmd(data->actor, ACMD_FACE_DIR_BASE + dir);
            data->turns++;
            if (data->turns >= 4) {
                *state = 3;
            }
        }
        break;
    case 3:
        if (func_ov012_02166ecc(data->actor)) {
            FieldPlayer_GetGPos(data->player, &x, &y, &z);
            distance = Mistralton_CountFreeCells(data->field, &data->stopReason, x, z);
            FieldPlayer_GetWPos(data->player, &pos);
            pos.z += distance << 16;
            task = FieldPlayerMoveTask_Create(data->field, distance * 3, &pos);
            FieldTaskManager_AddTask(Field_GetTaskManager(data->field), task, 0);
            SetActorFlag(data->actor, 0x200);
            if (!data->blocked) {
                sys_memset(&shake, 0, sizeof(shake));
                shake.amplitudeX = 0;
                shake.amplitudeY = 2;
                shake.period = 3;
                shake.steps = 4;
                shake.unk18 = 0;
                shake.delay = distance * 3 - 6;
                shake.mode = 1;
                task = FieldCameraShakeTask_Create(data->field, &shake);
                FieldTaskManager_AddTask(Field_GetTaskManager(data->field), task, 0);
            }
            if (!data->blocked) {
                GFL_SndSEPlay(SEQ_SE_GYM_H01);
            }
            *state = 6;
        }
        break;
    case 6:
        if (func_ov012_02166ecc(data->actor)) {
            dir = FieldPlayer_GetFaceDir(data->player) + 1;
            dir = dir % 4;
            FldAct_SetAcmd(data->actor, ACMD_FACE_DIR + dir);
        }
        if (FieldTaskManager_GetActiveTaskCount(Field_GetTaskManager(data->field)) <= 1) {
            if (!data->blocked) {
                GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H01));
            }
            if (!data->blocked) {
                switch (data->stopReason) {
                case 0:
                    GFL_SndSEPlay(SEQ_SE_GYM_H08);
                    break;
                case 1:
                    GFL_SndSEPlay(SEQ_SE_GYM_H03);
                    break;
                }
            }
            *state = 4;
        }
        break;
    case 4: {
        BOOL ready = FALSE;

        if (func_ov012_02166ecc(data->actor)) {
            dir = FieldPlayer_GetFaceDir(data->player) + 1;
            dir = dir % 4;
            FldAct_SetAcmd(data->actor, ACMD_FACE_DIR + dir);
        }
        if (data->timer >= sEndFrames[1]) {
            ready = TRUE;
        }
        if (!FieldTaskManager_IsIdle(Field_GetTaskManager(data->field))) {
            ready = FALSE;
        }
        if (ready == TRUE) {
            if (!data->blocked) {
                *state = 5;
            } else {
                *state = 7;
            }
        }
        break;
    }
    case 5:
        if (!GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H08))) {
            *state = 7;
        }
        if (!GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H03))) {
            *state = 7;
        }
        break;
    case 7:
        ClearActorFlag(data->actor, 0x200);
        if (data->stopReason == 1 && !data->blocked) {
            if (Mistralton_StopTrainerAhead(data->field, data->player) == TRUE) {
                FldAct_SetAcmd(data->actor, 1);
            } else {
                FldAct_SetAcmd(data->actor, 0);
            }
        } else {
            FldAct_SetAcmd(data->actor, 0);
        }
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H05));
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H07));
        return GAMEEVENT_DONE;
    }
    data->timer++;
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode MistraltonBumpEvent_Run(GameEvent *event, u32 *state, void *eventData) {
    RideEvent *data = eventData;
    FieldTask *task;
    u32 code;
    BOOL ready;

    Field_GetGimmickWorkBlock(data->field, WORK_BLOCK);
    switch (*state) {
    case 0:
        if (func_ov012_02166ecc(data->actor)) {
            FldAct_SetAcmd(data->actor, ACMD_JUMP);
            GFL_SndSEPlay(SEQ_SE_GYM_H07);
            *state = 1;
        }
        break;
    case 1:
        if (!GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H05))) {
            GFL_SndSEPlay(SEQ_SE_GYM_H05);
            *state = 2;
        }
        break;
    case 2:
        if (func_ov012_02166ecc(data->actor)) {
            code = FieldPlayer_GetObjCodeByForme(FieldPlayer_GetSex(data->player), FORME_RIDING);
            if (code != FldAct_GetObjCode(data->actor)) {
                FieldPlayer_ForceObjCode(data->player, code);
            }
            data->startTimer = data->timer;
            *state = 3;
        }
        break;
    case 3:
        if (data->timer - data->startTimer > 20) {
            task = Mistralton_CreateBobTask(data->field, data->actor, 1, 4, 6);
            FieldTaskManager_AddTask(Field_GetTaskManager(data->field), task, 0);
            *state = 4;
        }
        break;
    case 4:
        ready = FALSE;
        if (data->timer >= 0x5a) {
            ready = TRUE;
        }
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(data->field)) && ready == TRUE) {
            *state = 5;
        }
        break;
    case 5:
        code = FieldPlayer_GetObjCodeByForme(FieldPlayer_GetSex(data->player), 0);
        if (code != FldAct_GetObjCode(data->actor)) {
            FieldPlayer_ForceObjCode(data->player, code);
            FieldPlayer_ClearObjCode(data->player);
        }
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H05));
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H07));
        return GAMEEVENT_DONE;
    }
    data->timer++;
    return GAMEEVENT_CONTINUE;
}

static GameEvent *Mistralton_CreateCannonEvent(GameEvent *parent, GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    FieldPlayer *player = Field_GetPlayer(field);
    GameEvent *event = GameEvent_Create(gsys, parent, MistraltonCannonEvent_Run, sizeof(CannonEvent));
    CannonEvent *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->field = field;
    data->player = player;
    data->actor = FieldPlayer_GetActor(player);
    data->turns = 0;
    return event;
}

static GameEventReturnCode MistraltonCannonEvent_Run(GameEvent *event, u32 *state, void *eventData) {
    CannonEvent *data = eventData;
    MistraltonWork *work;

    work = Field_GetGimmickWorkBlock(data->field, WORK_BLOCK);
    switch (*state) {
    case 0:
        FieldExpObjAnm_SetFrameStep(FieldExpObj_GetAnmInfo(work->system, SCENE, ACTOR_EFFECT_FIRST + 1, 0),
                                    FX32_CONST(1.5));
        FieldExpObjAnm_SetFrameStep(FieldExpObj_GetAnmInfo(work->system, SCENE, ACTOR_EFFECT_FIRST + 2, 0),
                                    FX32_CONST(1.5));
        *state = 1;
        break;
    case 1:
        if (func_ov012_02166ecc(data->actor)) {
            FldAct_SetAcmd(data->actor, ACMD_JUMP);
            GFL_SndSEPlay(SEQ_SE_GYM_H07);
            *state = 2;
        }
        break;
    case 2:
        if (!GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H05))) {
            GFL_SndSEPlay(SEQ_SE_GYM_H05);
            *state = 3;
        }
        break;
    case 3: {
        FieldTask *pitchTask;
        FieldTask *zoomTask;
        FieldTaskManager *manager;

        Mistralton_SetStage(data->field, 2);
        pitchTask = FieldCameraMoveTaskPitch_Callback(data->field, 30, 0x20d8);
        zoomTask = FieldCameraMoveTaskZoom_Create(data->field, 30, 0xd5000);
        manager = Field_GetTaskManager(data->field);
        FieldTaskManager_AddTask(manager, pitchTask, 0);
        FieldTaskManager_AddTask(manager, zoomTask, 0);
        *state = 4;
        break;
    }
    case 4:
        if (Mistralton_IsCenterAnmDone(&work->centerAnm) == TRUE && Mistralton_IsFanAnmDone(&work->fanAnm) == TRUE) {
            *state = 5;
        }
        break;
    case 5:
        if (func_ov012_02166ecc(data->actor)) {
            u16 dir = (s32)FieldPlayer_GetFaceDir(data->player) % 4;

            FldAct_SetAcmd(data->actor, ACMD_FACE_DIR_BASE + dir);
            data->turns++;
            if (FieldTaskManager_IsIdle(Field_GetTaskManager(data->field)) && data->turns >= 4) {
                *state = 6;
            }
        }
        break;
    case 6:
        if (func_ov012_02166ecc(data->actor)) {
            VecFx32 pos;
            FieldTask *moveTask;
            FieldTask *spinTask;
            s16 x;
            s16 y;
            s16 z;
            u8 distance;
            int duration;

            FieldPlayer_GetGPos(data->player, &x, &y, &z);
            distance = 50 - z;
            FieldPlayer_GetWPos(data->player, &pos);
            pos.z += distance << 16;
            duration = distance * 2;
            moveTask = FieldPlayerMoveTask_Create(data->field, duration, &pos);
            FieldTaskManager_AddTask(Field_GetTaskManager(data->field), moveTask, 0);
            duration += 10;
            spinTask = FieldActorSpinTask_CreatePlayer(data->field, duration, duration / 4);
            FieldTaskManager_AddTask(Field_GetTaskManager(data->field), spinTask, 0);
            SetActorFlag(data->actor, 0x200);
            GFL_SndSEPlay(SEQ_SE_GYM_H01);
            *state = 7;
        }
        break;
    case 7: {
        FieldExpObjAnm *anm;
        s16 x;
        s16 y;
        s16 z;

        FieldPlayer_GetGPos(data->player, &x, &y, &z);
        if (z == 26) {
            anm = FieldExpObj_GetAnmInfo(work->system, SCENE, ACTOR_DOOR_FIRST, 0);
            if (func_ov036_021b84ec(anm) == TRUE) {
                FieldExpObj_SetAnmFrame(work->system, SCENE, ACTOR_DOOR_FIRST, 0, 0);
                FieldExpObjAnm_SetPaused(anm, FALSE);
                GFL_SndSEPlay(SEQ_SE_GYM_H06);
            }
        }
        if (z == 36) {
            anm = FieldExpObj_GetAnmInfo(work->system, SCENE, ACTOR_DOOR_FIRST + 1, 0);
            if (func_ov036_021b84ec(anm) == TRUE) {
                FieldExpObj_SetAnmFrame(work->system, SCENE, ACTOR_DOOR_FIRST + 1, 0, 0);
                FieldExpObjAnm_SetPaused(anm, FALSE);
                GFL_SndSEPlay(SEQ_SE_GYM_H06);
            }
        }
        if (FieldTaskManager_GetActiveTaskCount(Field_GetTaskManager(data->field)) <= 1) {
            GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H01));
            GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H01 + 4));
            ClearActorFlag(data->actor, 0x200);
            *state = 9;
        }
        break;
    }
    case 9: {
        VecFx32 pos;
        EventData *eventDataBase;
        ZoneWarp *warp;
        GameEvent *next;
        GameSystem *gsys;

        gsys = Field_GetGameSystem(data->field);
        eventDataBase = GameData_GetEventData(GSYS_GetGameData(gsys));
        FieldPlayer_GetWPos(data->player, &pos);
        warp = GetZoneWarpByID(eventDataBase, GetWarpAtPosition(eventDataBase, &pos));
        next = EventMapChangeWarp_CreateFromEntity(gsys, data->field, warp, ZoneWarp_CalcPosWeightBitsGrid(warp, &pos));
        GameEvent_Replace(event, next);
        *state = 8;
        break;
    }
    case 8:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *Mistralton_CreateTriggerEvent(GameEvent *parent, GameSystem *gsys, u16 actorId) {
    Field *field = GSYS_GetField(gsys);
    MMSys *actorSystem = Field_GetActorSystem(field);
    FieldPlayer *player = Field_GetPlayer(field);
    GameEvent *event = GameEvent_Create(gsys, parent, MistraltonTriggerEvent_Run, sizeof(RideEvent));
    RideEvent *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->field = field;
    data->player = player;
    data->actor = FindFieldActor(actorSystem, actorId);
    data->stopReason = 2;
    data->timer = 0;
    data->startTimer = 0;
    data->turns = 0;
    return event;
}

static GameEventReturnCode MistraltonTriggerEvent_Run(GameEvent *event, u32 *state, void *eventData) {
    RideEvent *data = eventData;
    MistraltonWork *work;

    work = Field_GetGimmickWorkBlock(data->field, WORK_BLOCK);
    switch (*state) {
    case 0:
        if (func_ov012_02166ecc(data->actor)) {
            GFL_SndSEPlay(SEQ_SE_GYM_H07);
            *state = 1;
        }
        break;
    case 1:
        if (data->timer++ > 20) {
            GFL_SndSEPlay(SEQ_SE_GYM_H05);
            *state = 2;
        }
        break;
    case 2:
        Mistralton_SetStage(data->field, 2);
        *state = 3;
        break;
    case 3:
        if (Mistralton_IsCenterAnmDone(&work->centerAnm) == TRUE && Mistralton_IsFanAnmDone(&work->fanAnm) == TRUE) {
            *state = 4;
        }
        break;
    case 4:
        if (func_ov012_02166ecc(data->actor)) {
            FieldTask *task;
            VecFx32 pos;
            FieldCameraShakeSetup shake;

            CopyActorWPos(data->actor, &pos);
            pos.z += 10 << 16;
            task = Mistralton_CreateLerpTask(data->field, data->actor, 30, &pos);
            FieldTaskManager_AddTask(Field_GetTaskManager(data->field), task, 0);
            SetActorFlag(data->actor, 0x200);
            sys_memset(&shake, 0, sizeof(shake));
            shake.amplitudeX = 0;
            shake.amplitudeY = 2;
            shake.period = 3;
            shake.steps = 4;
            shake.unk18 = 0;
            shake.delay = 0x18;
            shake.mode = 1;
            task = FieldCameraShakeTask_Create(data->field, &shake);
            FieldTaskManager_AddTask(Field_GetTaskManager(data->field), task, 0);
            GFL_SndSEPlay(SEQ_SE_GYM_H01);
            *state = 5;
        }
        break;
    case 5:
        if (func_ov012_02166ecc(data->actor)) {
            u16 dir = GetActorFaceDir(data->actor) + 1;

            dir = dir % 4;
            FldAct_SetAcmd(data->actor, ACMD_FACE_DIR + dir);
        }
        if (FieldTaskManager_GetActiveTaskCount(Field_GetTaskManager(data->field)) <= 1) {
            GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H01));
            GFL_SndSEPlay(SEQ_SE_GYM_H03);
            Mistralton_SetStage(data->field, 0);
            *state = 6;
        }
        break;
    case 6: {
        FieldTask *task;
        VecFx32 pos;

        FieldPlayer_GetWPos(data->player, &pos);
        pos.z += 1 << 16;
        task = FieldPlayerMoveTask_Create(data->field, 2, &pos);
        FieldTaskManager_AddTask(Field_GetTaskManager(data->field), task, 0);
        *state = 7;
        break;
    }
    case 7:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(data->field))) {
            *state = 8;
        }
        break;
    case 8: {
        VecFx32 pos;

        ClearActorFlag(data->actor, 0x200);
        CopyActorWPos(data->actor, &pos);
        SetActorWPosAll(data->actor, &pos, TRUE);
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H05));
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H07));
        return GAMEEVENT_DONE;
    }
    }
    return GAMEEVENT_CONTINUE;
}

static u8 Mistralton_CountFreeCells(Field *field, int *stopReason, s16 x, s16 z) {
    u8 count = 0;

    while (z < GRID_ROWS) {
        z++;
        if (sMistraltonGrid[z][x] != GRID_FREE) {
            *stopReason = 0;
            break;
        }
        if (GetFirstActorOnGPos(Field_GetActorSystem(field), x, z, TRUE) != NULL) {
            *stopReason = 1;
            break;
        }
        count++;
    }
    return count;
}

static BOOL Mistralton_StopTrainerAhead(Field *field, FieldPlayer *player) {
    s16 x;
    s16 y;
    s16 z;
    FieldActor *actor;

    FieldPlayer_GetGPos(player, &x, &y, &z);
    z++;
    actor = GetFirstActorOnGPos(Field_GetActorSystem(field), x, z, TRUE);
    if (actor != NULL) {
        EventWork *eventWork = GameData_GetEventWork(GSYS_GetGameData(Field_GetGameSystem(field)));

        if (!TrainerFlagGet(eventWork, GetNPCTrainerID(actor))) {
            FldAct_SetAcmd(actor, 0);
            return TRUE;
        }
    }
    return FALSE;
}

static void Mistralton_LoadScene(MistraltonWork *work) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    u16 actor;
    u8 i;

    LoadFieldExpandObjData(work->system, &sMistraltonScene, SCENE);
    matrix = FieldExpObj_GetActorMatrixPtr(work->system, SCENE, ACTOR_CENTER);
    matrix->translation.x = 0;
    matrix->translation.y = 0;
    matrix->translation.z = 0;
    func_ov036_021b8248(work->system, SCENE, ACTOR_CENTER, TRUE);
    FieldExpObj_SetActorHidden(work->system, SCENE, ACTOR_CENTER, FALSE);
    anm = FieldExpObj_GetAnmInfo(work->system, SCENE, ACTOR_CENTER, 0);
    FieldExpObj_SetAnm(work->system, SCENE, ACTOR_CENTER, 0, TRUE);
    FieldExpObjAnm_SetLooped(anm, TRUE);
    FieldExpObjAnm_SetPaused(anm, FALSE);
    FieldExpObjAnm_SetFrameStep(anm, work->centerAnm.speed);

    {
        const TilePos fanPositions[FAN_COUNT] = {
            { 10, 0, 8 },  { 12, 0, 8 },  { 1, 0, 13 },  { 12, 0, 17 }, { 21, 0, 18 }, { 7, 0, 20 },
            { 1, 0, 22 },  { 19, 0, 25 }, { 6, 0, 26 },  { 16, 0, 27 }, { 4, 0, 28 },  { 21, 0, 33 },
            { 3, 0, 34 },  { 1, 0, 41 },  { 21, 0, 41 }, { 9, 0, 42 },  { 13, 0, 42 },
        };

        for (i = 0; i < FAN_COUNT; i++) {
            actor = ACTOR_FAN_FIRST + i;
            matrix = FieldExpObj_GetActorMatrixPtr(work->system, SCENE, actor);
            matrix->translation.x = FX32_CONST(fanPositions[i].x * 16);
            matrix->translation.y = FX32_CONST(fanPositions[i].y * 16);
            matrix->translation.z = FX32_CONST(fanPositions[i].z * 16);
            func_ov036_021b8248(work->system, SCENE, actor, TRUE);
            FieldExpObj_SetActorHidden(work->system, SCENE, actor, FALSE);
            anm = FieldExpObj_GetAnmInfo(work->system, SCENE, actor, 0);
            FieldExpObj_SetAnm(work->system, SCENE, actor, 0, TRUE);
            FieldExpObjAnm_SetLooped(anm, TRUE);
            FieldExpObjAnm_SetPaused(anm, FALSE);
            FieldExpObjAnm_SetFrameStep(anm, work->fanAnm.speed);
        }
    }

    {
        const TilePos doorPositions[DOOR_COUNT] = { { 11, 0, 26 }, { 11, 0, 36 } };

        for (i = 0; i < DOOR_COUNT; i++) {
            actor = ACTOR_DOOR_FIRST + i;
            matrix = FieldExpObj_GetActorMatrixPtr(work->system, SCENE, actor);
            matrix->translation.x = FX32_CONST(doorPositions[i].x * 16);
            matrix->translation.y = FX32_CONST(doorPositions[i].y * 16);
            matrix->translation.z = FX32_CONST(doorPositions[i].z * 16);
            func_ov036_021b8248(work->system, SCENE, actor, TRUE);
            FieldExpObj_SetActorHidden(work->system, SCENE, actor, FALSE);
            anm = FieldExpObj_GetAnmInfo(work->system, SCENE, actor, 0);
            FieldExpObj_SetAnm(work->system, SCENE, actor, 0, TRUE);
            FieldExpObjAnm_SetLooped(anm, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
        }
    }

    for (actor = ACTOR_EFFECT_FIRST; actor <= ACTOR_EFFECT_FIRST + EFFECT_COUNT - 1; actor++) {
        matrix = FieldExpObj_GetActorMatrixPtr(work->system, SCENE, actor);
        matrix->translation.x = 0;
        matrix->translation.y = 0;
        matrix->translation.z = 0;
        func_ov036_021b8248(work->system, SCENE, actor, FALSE);
        if (actor == ACTOR_EFFECT_FIRST || (actor == ACTOR_EFFECT_FIRST + 1 && work->stage == 1)) {
            FieldExpObj_SetActorHidden(work->system, SCENE, actor, FALSE);
        } else {
            FieldExpObj_SetActorHidden(work->system, SCENE, actor, TRUE);
        }
        anm = FieldExpObj_GetAnmInfo(work->system, SCENE, actor, 0);
        FieldExpObj_SetAnm(work->system, SCENE, actor, 0, TRUE);
        FieldExpObjAnm_SetLooped(anm, TRUE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}

static void Mistralton_FreeScene(MistraltonWork *work) {
    FieldExpObj_FreeScene(work->system, SCENE);
}

static void Mistralton_StepAnimations(MistraltonWork *work) {
    Mistralton_StepCenterAnm(&work->centerAnm);
    Mistralton_StepFanAnm(&work->fanAnm);
    FieldExpObj_StepAllAnimations(work->system);
}

static void Mistralton_StepCenterAnm(AnmSpeed *anm) {
    if (anm->speed != anm->targetSpeed) {
        if (anm->speed < anm->targetSpeed) {
            anm->speed += ANM_SPEED_CENTER_STEP;
            if (anm->speed > anm->targetSpeed) {
                anm->speed = anm->targetSpeed;
            }
        } else {
            anm->speed -= ANM_SPEED_CENTER_STEP;
            if (anm->speed < anm->targetSpeed) {
                anm->speed = anm->targetSpeed;
            }
        }
        FieldExpObjAnm_SetFrameStep(FieldExpObj_GetAnmInfo(anm->system, SCENE, ACTOR_CENTER, 0), anm->speed);
    }
}

static BOOL Mistralton_IsCenterAnmDone(AnmSpeed *anm) {
    if (anm->speed == anm->targetSpeed) {
        return TRUE;
    }
    return FALSE;
}

static void Mistralton_StepFanAnm(AnmSpeed *anm) {
    u8 i;

    if (anm->speed != anm->targetSpeed) {
        if (anm->speed < anm->targetSpeed) {
            anm->speed += ANM_SPEED_FAN_STEP;
            if (anm->speed > anm->targetSpeed) {
                anm->speed = anm->targetSpeed;
            }
        } else {
            anm->speed -= ANM_SPEED_FAN_STEP;
            if (anm->speed < anm->targetSpeed) {
                anm->speed = anm->targetSpeed;
            }
        }
        for (i = 0; i < FAN_COUNT; i++) {
            FieldExpObjAnm_SetFrameStep(FieldExpObj_GetAnmInfo(anm->system, SCENE, ACTOR_FAN_FIRST + i, 0), anm->speed);
        }
    }
}

static BOOL Mistralton_IsFanAnmDone(AnmSpeed *anm) {
    if (anm->speed == anm->targetSpeed) {
        return TRUE;
    }
    return FALSE;
}

static void Mistralton_StepFanPitch(FanPitch *pitch, u16 frames) {
    if (pitch->pitch != pitch->targetPitch) {
        if (pitch->pitch < pitch->targetPitch) {
            pitch->pitch += FAN_PITCH_STEP;
            if (pitch->pitch > pitch->targetPitch) {
                pitch->pitch = pitch->targetPitch;
            }
        } else {
            pitch->pitch -= FAN_PITCH_STEP;
            if (pitch->pitch < pitch->targetPitch) {
                pitch->pitch = pitch->targetPitch;
            }
        }
    }
    GFL_SndPlayerSetParams(GFL_SndSeqGetPlayerIndex(SEQ_SE_GYM_H02), -1, pitch->pitch, -1);
}

static void Mistralton_SetFanPitchTarget(FanPitch *pitch, u32 stage) {
    const s32 targets[4] = { -256, 128, 256, 768 };

    pitch->targetPitch = targets[stage];
}

static void MistraltonSave_SetStarted(MistraltonSave *save, int started) {
    save->started = started;
}

static int MistraltonSave_GetStarted(MistraltonSave *save) {
    return save->started;
}

static void MistraltonSave_SetFrames(MistraltonSave *save, u16 frames) {
    save->frames = frames;
}

static u16 MistraltonSave_GetFrames(MistraltonSave *save) {
    return save->frames;
}
