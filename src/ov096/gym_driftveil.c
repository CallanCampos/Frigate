#include "types.h"
#include "constants/sound.h"
#include "constants/vars.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_task.h"
#include "field/gym_driftveil.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "save/event_work.h"
#include "save/player_info.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The Driftveil City gym's upper floor, named after the zone: the overlay has no string. The floor has 82 pads, each
// on a tile and facing one way, that slide the player 4 to 18 tiles when he stands on one; two pads share a tile
// model, which tilts one way or the other. 36 switches in the floor stay pressed once the player slides onto them,
// which the save keeps. The elevators and the freight elevator carry the player between floors. The names are ours

#define GIMMICK_WORK_ID 1
// The gimmick state's flags area of the pressed switches
#define GIMMICK_FLAGS_ID 7
// The archive of the gym's models
#define GIMMICK_ARCID 0x8a

#define PAD_COUNT 82
#define PAD_MODEL_COUNT 41
#define SWITCH_COUNT 36
#define SWITCH_COLUMNS 6
#define ELEVATOR_COUNT 4

// The actors of the scene: the pads' tile models, the switches, the elevators, the freight elevator in its two forms,
// and the models that show the player and the rider on it
#define ACTOR_SWITCH 0x29
#define ACTOR_ELEVATOR 0x4d
#define ACTOR_FREIGHT_RIDER 0x51
#define ACTOR_FREIGHT 0x52
#define ACTOR_PLAYER_ELEVATOR_MALE 0x53
#define ACTOR_PLAYER_ELEVATOR_FEMALE 0x54
#define ACTOR_PLAYER_FREIGHT_MALE 0x55
#define ACTOR_PLAYER_FREIGHT_FEMALE 0x56
#define ACTOR_RIDER 0x57
#define ACTOR_COUNT 88

// The switch that the player presses to ride up, which the first visit marks as pressed
#define SWITCH_ELEVATOR 0x1e

// A distance in tiles, and half a tile, in fx32
#define TILE(n) ((n) << 16)
// Unsigned, which keeps MWCC from folding it into the tile offset it is added to
#define HALF_TILE (1u << 15)

// How high the elevators rise: 5 tiles
#define ELEVATOR_HEIGHT TILE(5)

// The animation of an actor that loops and stays paused until it is told to run
#define ANM_MAIN 0

// What the pads do: a pad tilts, and the slide takes a number of frames per tile
#define SLIDE_FRAMES_PER_TILE 4
// How the pad model's animation runs forward, and backward
#define PAD_ANM_STEP 0x6500

#define ACMD_WALK_FIRST 0x34
#define ACTOR_FLAG_NO_COLLISION 0x200

#define PAD_NONE 0xff

#define STATE_END 5

typedef struct {
    u16 heapId;
    FieldExpObjSystem *expObj;
    Field *field;
    FieldPlayer *player;
    // Whether each switch is pressed
    BOOL pressed[SWITCH_COUNT];
    // Whether the player has pressed the switch to ride up
    BOOL elevatorSwitchPressed;
    // What the camera followed, while the freight elevator runs
    void *cameraBind;
} DriftveilGymWork;

// Where the pad slides the player to: how far it goes, which way, and the switch that it lands on
typedef struct {
    u32 distance;
    u8 x;
    u8 z;
    u8 direction;
    u8 landing;
} DriftveilGymPad;

typedef struct {
    GameSystem *gsys;
    Field *field;
    FieldPlayer *player;
    u8 pad;
    u8 frame;
    u8 duration;
} DriftveilGymSlideEvent;

typedef struct {
    GameSystem *gsys;
    Field *field;
    FieldPlayer *player;
    u32 elevator;
    u8 down;
    fx32 height;
    fx32 target;
    fx32 step;
    u16 model;
} DriftveilGymElevatorEvent;

// The camera shake at the end of the freight elevator's ride
typedef struct {
    u8 state;
    u8 count;
    u8 limit;
    u8 toggle;
    fx32 amplitude;
    FieldCamera *camera;
    void *bind;
    VecFx32 offset;
} DriftveilGymShake;

typedef struct {
    fx32 height;
    fx32 target;
    fx32 step;
    DriftveilGymShake shake;
    u32 up;
    u32 model;
    FieldActor *rider;
    BOOL hasRider;
} DriftveilGymFreightEvent;

static void DriftveilGym_InitObjects(DriftveilGymWork *work);
static void DriftveilGym_FreeScene(DriftveilGymWork *work);
static void DriftveilGym_StepAnimations(DriftveilGymWork *work);
static GameEvent *DriftveilGym_CreateSlideEvent(GameEvent *parent, GameSystem *gsys, u8 pad);
static GameEventReturnCode DriftveilGym_SlideEventCallback(GameEvent *event, u32 *state, void *data);
static u8 DriftveilGym_FindPad(s16 x, s16 z, u32 direction);
static void DriftveilGym_StartPadAnimation(FieldExpObjSystem *expObj, u32 pad, u32 direction);
static void DriftveilGym_StopPadAnimation(FieldExpObjSystem *expObj, u32 pad);
static void DriftveilGym_ShowPressedSwitches(DriftveilGymWork *work);
static void DriftveilGym_PressSwitch(DriftveilGymWork *work, u32 index);
static GameEvent *DriftveilGym_CreateElevator(GameEvent *parent, GameSystem *gsys, u32 elevator, u32 down);
static GameEventReturnCode DriftveilGym_ElevatorEventCallback(GameEvent *event, u32 *state, void *data);
static void DriftveilGym_MoveElevator(DriftveilGymElevatorEvent *ev);
static void DriftveilGym_ShowElevatorRider(DriftveilGymElevatorEvent *ev, BOOL atFloor, u16 model);
static BOOL DriftveilGym_HasReachedHeight(DriftveilGymElevatorEvent *ev);
static GameEvent *DriftveilGym_CreateFreight(GameEvent *parent, GameSystem *gsys, BOOL up, BOOL withRider);
static GameEventReturnCode DriftveilGym_FreightEventCallback(GameEvent *event, u32 *state, void *data);
static BOOL DriftveilGym_HasFreightReachedTarget(DriftveilGymFreightEvent *ev);
static void DriftveilGym_InitShake(int step, BOOL light, FieldCamera *camera, DriftveilGymShake *shake);
static BOOL DriftveilGym_UpdateShake(DriftveilGymShake *shake);
static void DriftveilGym_ResetFreight(FieldExpObjSystem *expObj);
static void DriftveilGym_SaveSwitches(GimmickState *gimmickState, const BOOL *pressed);
static void DriftveilGym_LoadSwitches(GimmickState *gimmickState, BOOL *pressed);

static const G3DSceneAnimationSetup sFreightAnimations[] = { { 0xa, 0 } };
static const G3DSceneAnimationSetup sElevatorAnimations[] = { { 6, 0 } };
static const G3DSceneAnimationSetup sFreightRiderAnimations[] = { { 8, 0 }, { 0, 0xb }, { 0, 7 } };
static const G3DSceneAnimationSetup sPadAnimations[] = { { 4, 0 }, { 0, 0xa }, { 0, 6 }, { 0xc, 0 } };

static const G3DSceneActorSetup sGymDriftveilActors[ACTOR_COUNT] = {
    { 0, 0, 0, 0, sPadAnimations, 1 }, { 0, 0, 0, 0, sPadAnimations, 1 },
    { 0, 0, 0, 0, sPadAnimations, 1 }, { 0, 0, 0, 0, sPadAnimations, 1 },
    { 0, 0, 0, 0, sPadAnimations, 1 }, { 0, 0, 0, 0, sPadAnimations, 1 },
    { 0, 0, 0, 0, sPadAnimations, 1 }, { 0, 0, 0, 0, sPadAnimations, 1 },
    { 0, 0, 0, 0, sPadAnimations, 1 }, { 0, 0, 0, 0, sPadAnimations, 1 },
    { 0, 0, 0, 0, sPadAnimations, 1 }, { 0, 0, 0, 0, sPadAnimations, 1 },
    { 0, 0, 0, 0, sPadAnimations, 1 }, { 0, 0, 0, 0, sPadAnimations, 1 },
    { 0, 0, 0, 0, sPadAnimations, 1 }, { 0, 0, 0, 0, sPadAnimations, 1 },
    { 0, 0, 0, 0, sPadAnimations, 1 }, { 0, 0, 0, 0, sPadAnimations, 1 },
    { 1, 0, 1, 0, sPadAnimations, 1 }, { 1, 0, 1, 0, sPadAnimations, 1 },
    { 1, 0, 1, 0, sPadAnimations, 1 }, { 1, 0, 1, 0, sPadAnimations, 1 },
    { 1, 0, 1, 0, sPadAnimations, 1 }, { 1, 0, 1, 0, sPadAnimations, 1 },
    { 1, 0, 1, 0, sPadAnimations, 1 }, { 1, 0, 1, 0, sPadAnimations, 1 },
    { 1, 0, 1, 0, sPadAnimations, 1 }, { 1, 0, 1, 0, sPadAnimations, 1 },
    { 1, 0, 1, 0, sPadAnimations, 1 }, { 1, 0, 1, 0, sPadAnimations, 1 },
    { 1, 0, 1, 0, sPadAnimations, 1 }, { 1, 0, 1, 0, sPadAnimations, 1 },
    { 1, 0, 1, 0, sPadAnimations, 1 }, { 1, 0, 1, 0, sPadAnimations, 1 },
    { 1, 0, 1, 0, sPadAnimations, 1 }, { 1, 0, 1, 0, sPadAnimations, 1 },
    { 1, 0, 1, 0, sPadAnimations, 1 }, { 2, 0, 2, 0, sPadAnimations, 1 },
    { 2, 0, 2, 0, sPadAnimations, 1 }, { 2, 0, 2, 0, sPadAnimations, 1 },
    { 3, 0, 3, 0, sPadAnimations, 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 11, 0, 11, 0, (sPadAnimations + 3), 1 },
    { 11, 0, 11, 0, (sPadAnimations + 3), 1 }, { 5, 0, 5, 0, sElevatorAnimations, 1 },
    { 5, 0, 5, 0, sElevatorAnimations, 1 }, { 5, 0, 5, 0, sElevatorAnimations, 1 },
    { 5, 0, 5, 0, sElevatorAnimations, 1 }, { 7, 0, 7, 0, sFreightRiderAnimations, 1 },
    { 9, 0, 9, 0, sFreightAnimations, 1 }, { 13, 0, 13, 0, NULL, 0 },
    { 14, 0, 14, 0, NULL, 0 }, { 15, 0, 15, 0, NULL, 0 },
    { 16, 0, 16, 0, NULL, 0 }, { 17, 0, 17, 0, NULL, 0 },
};

static const G3DSceneResourceSetup sGymDriftveilResources[18] = {
    { GIMMICK_ARCID, 0, 0 },    { GIMMICK_ARCID, 1, 0 },    { GIMMICK_ARCID, 2, 0 },    { GIMMICK_ARCID, 3, 0 },
    { GIMMICK_ARCID, 4, 0 },    { GIMMICK_ARCID, 9, 0 },    { GIMMICK_ARCID, 0xa, 0 },  { GIMMICK_ARCID, 0xb, 0 },
    { GIMMICK_ARCID, 0xc, 0 },  { GIMMICK_ARCID, 5, 0 },    { GIMMICK_ARCID, 6, 0 },    { GIMMICK_ARCID, 7, 0 },
    { GIMMICK_ARCID, 8, 0 },    { GIMMICK_ARCID, 0xd, 0 },  { GIMMICK_ARCID, 0xe, 0 },  { GIMMICK_ARCID, 0xf, 0 },
    { GIMMICK_ARCID, 0x10, 0 }, { GIMMICK_ARCID, 0x11, 0 },
};

static const G3DSceneSetup sGymDriftveilSceneSetup = { sGymDriftveilResources, NELEMS(sGymDriftveilResources), sGymDriftveilActors, NELEMS(sGymDriftveilActors) };

// Each pad: how far it slides the player in tiles by the kind of pad, the tile it is on and the way it faces, and the
// switch that the slide lands on
static const DriftveilGymPad sPads[PAD_COUNT] = {
    { 0, 6, 8, 1, 6 }, { 0, 6, 17, 0, 0 }, { 0, 16, 8, 1, 7 },
    { 0, 16, 17, 0, 1 }, { 0, 26, 8, 1, 8 }, { 0, 26, 17, 0, 2 },
    { 0, 36, 8, 1, 9 }, { 0, 36, 17, 0, 3 }, { 0, 6, 19, 1, 12 },
    { 0, 6, 28, 0, 6 }, { 0, 16, 19, 1, 13 }, { 0, 16, 28, 0, 7 },
    { 0, 26, 19, 1, 14 }, { 0, 26, 28, 0, 8 }, { 0, 56, 19, 1, 17 },
    { 0, 56, 28, 0, 11 }, { 0, 6, 30, 1, 18 }, { 0, 6, 39, 0, 12 },
    { 0, 26, 30, 1, 20 }, { 0, 26, 39, 0, 14 }, { 0, 56, 30, 1, 23 },
    { 0, 56, 39, 0, 17 }, { 0, 16, 41, 1, 25 }, { 0, 16, 50, 0, 19 },
    { 0, 56, 41, 1, 29 }, { 0, 56, 50, 0, 23 }, { 0, 6, 52, 1, 30 },
    { 0, 6, 61, 0, 24 }, { 0, 16, 52, 1, 31 }, { 0, 16, 61, 0, 25 },
    { 0, 36, 52, 1, 33 }, { 0, 36, 61, 0, 27 }, { 0, 46, 52, 1, 34 },
    { 0, 46, 61, 0, 28 }, { 0, 56, 52, 1, 35 }, { 0, 56, 61, 0, 29 },
    { 1, 8, 7, 3, 1 }, { 1, 14, 7, 2, 0 }, { 1, 38, 7, 3, 4 },
    { 1, 44, 7, 2, 3 }, { 1, 48, 7, 3, 5 }, { 1, 54, 7, 2, 4 },
    { 1, 8, 18, 3, 7 }, { 1, 14, 18, 2, 6 }, { 1, 18, 18, 3, 8 },
    { 1, 24, 18, 2, 7 }, { 1, 28, 18, 3, 9 }, { 1, 34, 18, 2, 8 },
    { 1, 18, 29, 3, 14 }, { 1, 24, 29, 2, 13 }, { 1, 28, 29, 3, 15 },
    { 1, 34, 29, 2, 14 }, { 1, 38, 29, 3, 16 }, { 1, 44, 29, 2, 15 },
    { 1, 48, 29, 3, 17 }, { 1, 54, 29, 2, 16 }, { 1, 38, 40, 3, 22 },
    { 1, 44, 40, 2, 21 }, { 1, 8, 51, 3, 25 }, { 1, 14, 51, 2, 24 },
    { 1, 48, 51, 3, 29 }, { 1, 54, 51, 2, 28 }, { 1, 38, 62, 3, 34 },
    { 1, 44, 62, 2, 33 }, { 1, 48, 62, 3, 35 }, { 1, 54, 62, 2, 34 },
    { 1, 28, 51, 3, 27 }, { 1, 34, 51, 2, 26 }, { 1, 38, 51, 3, 28 },
    { 1, 44, 51, 2, 27 }, { 1, 18, 62, 3, 32 }, { 1, 24, 62, 2, 31 },
    { 1, 28, 62, 3, 33 }, { 1, 34, 62, 2, 32 }, { 2, 36, 19, 1, 21 },
    { 2, 36, 39, 0, 9 }, { 2, 46, 19, 1, 22 }, { 2, 46, 39, 0, 10 },
    { 2, 26, 41, 1, 32 }, { 2, 26, 61, 0, 20 }, { 3, 18, 7, 3, 3 },
    { 3, 34, 7, 2, 1 },
};

void DriftveilGym_Init(Field *field) {
    GimmickState *gimmickState = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    DriftveilGymWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(DriftveilGymWork));

    work->heapId = HEAPID_FIELDMAP;
    work->field = field;
    work->expObj = Field_GetExpObjSystem(field);
    work->player = Field_GetPlayer(field);
    DriftveilGym_InitObjects(work);
    DriftveilGym_LoadSwitches(gimmickState, work->pressed);
    DriftveilGym_ShowPressedSwitches(work);
}

void DriftveilGym_End(Field *field) {
    GimmickState *gimmickState = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    DriftveilGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    DriftveilGym_SaveSwitches(gimmickState, work->pressed);
    DriftveilGym_FreeScene(work);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void DriftveilGym_Update(Field *field) {
    DriftveilGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    s16 x;
    s16 y;
    s16 z;

    FieldPlayer_GetGPos(Field_GetPlayer(field), &x, &y, &z);
    if (!work->elevatorSwitchPressed && x == 6 && z == 0x3f) {
        work->elevatorSwitchPressed = TRUE;
        DriftveilGym_PressSwitch(work, SWITCH_ELEVATOR);
    }
    DriftveilGym_StepAnimations(work);
}

GameEvent *DriftveilGym_CheckPadEvent(GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    FieldPlayer *player;
    s16 x;
    s16 y;
    s16 z;
    u8 pad;

    Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    player = Field_GetPlayer(field);
    if (*EventWork_GetWkPtr(GameData_GetEventWork(GSYS_GetGameData(gsys)), EVENT_WORK_0x40b7) == 1) {
        return NULL;
    }
    FieldPlayer_GetGPos(player, &x, &y, &z);
    pad = DriftveilGym_FindPad(x, z, FieldPlayer_GetFaceDir(player));
    if (pad != PAD_NONE) {
        return DriftveilGym_CreateSlideEvent(NULL, gsys, pad);
    }
    return NULL;
}

GameEvent *DriftveilGym_CreateElevatorEvent(GameSystem *gsys, u16 elevator, u16 down) {
    return DriftveilGym_CreateElevator(NULL, gsys, elevator, down);
}

GameEvent *DriftveilGym_CreateFreightEvent(GameSystem *gsys, BOOL up, BOOL withRider) {
    return DriftveilGym_CreateFreight(NULL, gsys, up, withRider);
}

void DriftveilGym_PrepareFreight(GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    DriftveilGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    FieldCamera *camera = Field_GetCameraSystem(field);
    SRTMatrix *matrix;
    FieldPlayer *player;
    VecFx32 pos;
    fx32 height = ELEVATOR_HEIGHT;

    work->cameraBind = FieldCamera_GetBind(camera);
    FieldCamera_ClearBind(camera);
    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, ACTOR_FREIGHT);
    matrix->translation.y = height;
    player = Field_GetPlayer(field);
    FieldPlayer_GetWPos(player, &pos);
    pos.y = height;
    FieldPlayer_SetWPos(player, &pos);
}

static void DriftveilGym_InitObjects(DriftveilGymWork *work) {
    u8 i;
    u16 actor;
    FieldExpObjAnm *anm;
    SRTMatrix *matrix;

    LoadFieldExpandObjData(work->expObj, &sGymDriftveilSceneSetup, 0);
    {
        const VecFx32 pads[PAD_MODEL_COUNT] = {
            { FX32_CONST(96), FX32_CONST(0), FX32_CONST(200) }, { FX32_CONST(256), FX32_CONST(0), FX32_CONST(200) },
            { FX32_CONST(416), FX32_CONST(0), FX32_CONST(200) }, { FX32_CONST(576), FX32_CONST(0), FX32_CONST(200) },
            { FX32_CONST(96), FX32_CONST(0), FX32_CONST(376) }, { FX32_CONST(256), FX32_CONST(0), FX32_CONST(376) },
            { FX32_CONST(416), FX32_CONST(0), FX32_CONST(376) }, { FX32_CONST(896), FX32_CONST(0), FX32_CONST(376) },
            { FX32_CONST(96), FX32_CONST(0), FX32_CONST(552) }, { FX32_CONST(416), FX32_CONST(0), FX32_CONST(552) },
            { FX32_CONST(896), FX32_CONST(0), FX32_CONST(552) }, { FX32_CONST(256), FX32_CONST(0), FX32_CONST(728) },
            { FX32_CONST(896), FX32_CONST(0), FX32_CONST(728) }, { FX32_CONST(96), FX32_CONST(0), FX32_CONST(904) },
            { FX32_CONST(256), FX32_CONST(0), FX32_CONST(904) }, { FX32_CONST(576), FX32_CONST(0), FX32_CONST(904) },
            { FX32_CONST(736), FX32_CONST(0), FX32_CONST(904) }, { FX32_CONST(896), FX32_CONST(0), FX32_CONST(904) },
            { FX32_CONST(176), FX32_CONST(0), FX32_CONST(112) }, { FX32_CONST(656), FX32_CONST(0), FX32_CONST(112) },
            { FX32_CONST(816), FX32_CONST(0), FX32_CONST(112) }, { FX32_CONST(176), FX32_CONST(0), FX32_CONST(288) },
            { FX32_CONST(336), FX32_CONST(0), FX32_CONST(288) }, { FX32_CONST(496), FX32_CONST(0), FX32_CONST(288) },
            { FX32_CONST(336), FX32_CONST(0), FX32_CONST(464) }, { FX32_CONST(496), FX32_CONST(0), FX32_CONST(464) },
            { FX32_CONST(656), FX32_CONST(0), FX32_CONST(464) }, { FX32_CONST(816), FX32_CONST(0), FX32_CONST(464) },
            { FX32_CONST(656), FX32_CONST(0), FX32_CONST(640) }, { FX32_CONST(176), FX32_CONST(0), FX32_CONST(816) },
            { FX32_CONST(816), FX32_CONST(0), FX32_CONST(816) }, { FX32_CONST(656), FX32_CONST(0), FX32_CONST(992) },
            { FX32_CONST(816), FX32_CONST(0), FX32_CONST(992) }, { FX32_CONST(496), FX32_CONST(0), FX32_CONST(816) },
            { FX32_CONST(656), FX32_CONST(0), FX32_CONST(816) }, { FX32_CONST(336), FX32_CONST(0), FX32_CONST(992) },
            { FX32_CONST(496), FX32_CONST(0), FX32_CONST(992) }, { FX32_CONST(576), FX32_CONST(0), FX32_CONST(464) },
            { FX32_CONST(736), FX32_CONST(0), FX32_CONST(464) }, { FX32_CONST(416), FX32_CONST(0), FX32_CONST(816) },
            { FX32_CONST(416), FX32_CONST(0), FX32_CONST(112) },
        };

        for (i = 0; i < PAD_MODEL_COUNT; i++) {
            matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, i);
            matrix->translation.x = pads[i].x + FX32_CONST(8);
            matrix->translation.y = pads[i].y;
            matrix->translation.z = pads[i].z + FX32_CONST(8);
            func_ov036_021b8248(work->expObj, 0, i, 1);
            FieldExpObj_SetActorHidden(work->expObj, 0, i, FALSE);
            anm = FieldExpObj_GetAnmInfo(work->expObj, 0, i, ANM_MAIN);
            FieldExpObj_SetAnm(work->expObj, 0, i, ANM_MAIN, TRUE);
            FieldExpObjAnm_SetLooped(anm, TRUE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
            FieldExpObjAnm_SetFrameStep(anm, PAD_ANM_STEP);
        }
    }
    for (i = 0; i < SWITCH_COUNT; i++) {
        actor = ACTOR_SWITCH + i;
        matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, actor);
        matrix->translation.x = TILE(10) * (i % SWITCH_COLUMNS) + TILE(6) + HALF_TILE;
        matrix->translation.y = 0;
        matrix->translation.z = TILE(11) * (i / SWITCH_COLUMNS) + TILE(7) + HALF_TILE;
        func_ov036_021b8248(work->expObj, 0, actor, 1);
        FieldExpObj_SetActorHidden(work->expObj, 0, actor, TRUE);
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, actor, ANM_MAIN);
        FieldExpObj_SetAnm(work->expObj, 0, actor, ANM_MAIN, TRUE);
        FieldExpObjAnm_SetLooped(anm, TRUE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
    }
    {
        const VecFx32 elevators[ELEVATOR_COUNT] = {
            { FX32_CONST(264), FX32_CONST(0), FX32_CONST(472) },
            { FX32_CONST(744), FX32_CONST(0), FX32_CONST(472) },
            { FX32_CONST(264), FX32_CONST(0), FX32_CONST(1000) },
            { FX32_CONST(744), FX32_CONST(0), FX32_CONST(1000) },
        };

        for (i = 0; i < ELEVATOR_COUNT; i++) {
            actor = ACTOR_ELEVATOR + i;
            matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, actor);
            matrix->translation.x = elevators[i].x;
            matrix->translation.y = elevators[i].y;
            matrix->translation.z = elevators[i].z;
            func_ov036_021b8248(work->expObj, 0, actor, 1);
            FieldExpObj_SetActorHidden(work->expObj, 0, actor, FALSE);
            anm = FieldExpObj_GetAnmInfo(work->expObj, 0, actor, ANM_MAIN);
            FieldExpObj_SetAnm(work->expObj, 0, actor, ANM_MAIN, TRUE);
            FieldExpObjAnm_SetLooped(anm, TRUE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
        }
    }
    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, ACTOR_FREIGHT_RIDER);
    matrix->translation.x = FX32_CONST(744);
    matrix->translation.y = 0;
    matrix->translation.z = FX32_CONST(296);
    func_ov036_021b8248(work->expObj, 0, ACTOR_FREIGHT_RIDER, 1);
    FieldExpObj_SetActorHidden(work->expObj, 0, ACTOR_FREIGHT_RIDER, FALSE);
    anm = FieldExpObj_GetAnmInfo(work->expObj, 0, ACTOR_FREIGHT_RIDER, ANM_MAIN);
    FieldExpObj_SetAnm(work->expObj, 0, ACTOR_FREIGHT_RIDER, ANM_MAIN, TRUE);
    FieldExpObjAnm_SetLooped(anm, TRUE);
    FieldExpObjAnm_SetPaused(anm, FALSE);
    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, ACTOR_FREIGHT);
    matrix->translation.x = FX32_CONST(184);
    matrix->translation.y = 0;
    matrix->translation.z = FX32_CONST(1400);
    func_ov036_021b8248(work->expObj, 0, ACTOR_FREIGHT, 1);
    FieldExpObj_SetActorHidden(work->expObj, 0, ACTOR_FREIGHT, FALSE);
    anm = FieldExpObj_GetAnmInfo(work->expObj, 0, ACTOR_FREIGHT, ANM_MAIN);
    FieldExpObj_SetAnm(work->expObj, 0, ACTOR_FREIGHT, ANM_MAIN, TRUE);
    FieldExpObjAnm_SetLooped(anm, TRUE);
    FieldExpObjAnm_SetPaused(anm, TRUE);
    for (i = 0; i < 5; i++) {
        actor = ACTOR_PLAYER_ELEVATOR_MALE + i;
        matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, actor);
        matrix->translation.x = 0;
        matrix->translation.y = 0;
        matrix->translation.z = 0;
        func_ov036_021b8248(work->expObj, 0, actor, 1);
        FieldExpObj_SetActorHidden(work->expObj, 0, actor, TRUE);
    }
}

static void DriftveilGym_FreeScene(DriftveilGymWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void DriftveilGym_StepAnimations(DriftveilGymWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

static GameEvent *DriftveilGym_CreateSlideEvent(GameEvent *parent, GameSystem *gsys, u8 pad) {
    Field *field = GSYS_GetField(gsys);
    FieldPlayer *player = Field_GetPlayer(field);
    GameEvent *event = GameEvent_Create(gsys, parent, DriftveilGym_SlideEventCallback, sizeof(DriftveilGymSlideEvent));
    DriftveilGymSlideEvent *ev = GameEvent_GetData(event);

    ev->gsys = gsys;
    ev->field = field;
    ev->player = player;
    ev->pad = pad;
    ev->frame = 0;
    ev->duration = 0;
    return event;
}

static GameEventReturnCode DriftveilGym_SlideEventCallback(GameEvent *event, u32 *state, void *data) {
    DriftveilGymSlideEvent *ev = data;
    DriftveilGymWork *work = Field_GetGimmickWorkBlock(ev->field, GIMMICK_WORK_ID);
    int tiles;
    u32 direction;
    VecFx32 pos;
    FieldTask *task;

    switch (*state) {
    case 0:
        if (func_ov036_0219a870(ev->player)) {
            direction = FieldPlayer_GetFaceDir(ev->player);
            FldAct_SetAcmd(FieldPlayer_GetActor(ev->player), direction + ACMD_WALK_FIRST);
            *state = 1;
        }
        break;
    case 1:
        if (func_ov036_0219a870(ev->player)) {
            switch (sPads[ev->pad].distance) {
            case 0:
                tiles = 7;
                break;
            case 1:
                tiles = 4;
                break;
            case 2:
                tiles = 18;
                break;
            case 3:
                tiles = 14;
                break;
            }
            ev->duration = tiles * SLIDE_FRAMES_PER_TILE;
            FieldPlayer_GetWPos(ev->player, &pos);
            switch (sPads[ev->pad].direction) {
            case 0:
                pos.z -= TILE(tiles);
                break;
            case 1:
                pos.z += TILE(tiles);
                break;
            case 2:
                pos.x -= TILE(tiles);
                break;
            case 3:
                pos.x += TILE(tiles);
                break;
            }
            task = FieldPlayerMoveTask_Create(ev->field, tiles * SLIDE_FRAMES_PER_TILE, &pos);
            FieldTaskManager_AddTask(Field_GetTaskManager(ev->field), task, 0);
            DriftveilGym_StartPadAnimation(work->expObj, ev->pad, sPads[ev->pad].direction);
            *state = 2;
        }
        break;
    case 2:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(ev->field))) {
            *state = 3;
        }
        if (sPads[ev->pad].distance - 2 <= 1) {
            ev->frame++;
            if (ev->frame == 12) {
                SetActorFlag(FieldPlayer_GetActor(ev->player), ACTOR_FLAG_NO_COLLISION);
            }
            if (ev->frame == ev->duration - 12) {
                ClearActorFlag(FieldPlayer_GetActor(ev->player), ACTOR_FLAG_NO_COLLISION);
            }
        }
        break;
    case 3:
        DriftveilGym_PressSwitch(work, sPads[ev->pad].landing);
        *state = 4;
        break;
    case 4:
        if (func_ov036_0219a870(ev->player)) {
            direction = FieldPlayer_GetFaceDir(ev->player);
            FldAct_SetAcmd(FieldPlayer_GetActor(ev->player), direction + ACMD_WALK_FIRST);
            *state = STATE_END;
        }
        break;
    case STATE_END:
        if (func_ov036_0219a870(ev->player)) {
            DriftveilGym_StopPadAnimation(work->expObj, ev->pad);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The pad on the tile, if the player faces the way it does
static u8 DriftveilGym_FindPad(s16 x, s16 z, u32 direction) {
    u8 i;

    for (i = 0; i < PAD_COUNT; i++) {
        if (x == sPads[i].x && z == sPads[i].z && direction == sPads[i].direction) {
            return i;
        }
    }
    return PAD_NONE;
}

// The pad's model tilts the way the player slides
static void DriftveilGym_StartPadAnimation(FieldExpObjSystem *expObj, u32 pad, u32 direction) {
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(expObj, 0, pad >> 1, ANM_MAIN);
    fx32 step;

    if (func_ov036_021b84ec(anm) == 1) {
        switch (direction) {
        case 0:
        case 3:
            step = PAD_ANM_STEP;
            break;
        case 1:
        case 2:
            step = -PAD_ANM_STEP;
            break;
        }
        FieldExpObjAnm_SetFrameStep(anm, step);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
    GFL_SndSEPlay(SEQ_SE_SW_GRGYM_01);
}

static void DriftveilGym_StopPadAnimation(FieldExpObjSystem *expObj, u32 pad) {
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(expObj, 0, pad >> 1, ANM_MAIN);

    if (!func_ov036_021b84ec(anm)) {
        FieldExpObjAnm_SetPaused(anm, TRUE);
    }
    GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_SW_GRGYM_01));
}

static void DriftveilGym_ShowPressedSwitches(DriftveilGymWork *work) {
    u8 i;
    u16 actor;
    FieldExpObjAnm *anm;

    for (i = 0; i < SWITCH_COUNT; i++) {
        if (work->pressed[i] == 1) {
            actor = ACTOR_SWITCH + i;
            FieldExpObj_SetActorHidden(work->expObj, 0, actor, FALSE);
            anm = FieldExpObj_GetAnmInfo(work->expObj, 0, actor, ANM_MAIN);
            FieldExpObj_SetAnm(work->expObj, 0, actor, ANM_MAIN, TRUE);
            FieldExpObjAnm_SetPaused(anm, FALSE);
            if (i == SWITCH_ELEVATOR) {
                work->elevatorSwitchPressed = TRUE;
            }
        }
    }
}

static void DriftveilGym_PressSwitch(DriftveilGymWork *work, u32 index) {
    u16 actor;
    FieldExpObjAnm *anm;

    if (work->pressed[index] != 1) {
        work->pressed[index] = TRUE;
        actor = ACTOR_SWITCH + index;
        FieldExpObj_SetActorHidden(work->expObj, 0, actor, FALSE);
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, actor, ANM_MAIN);
        FieldExpObj_SetAnm(work->expObj, 0, actor, ANM_MAIN, TRUE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
        GFL_SndSEPlay(SEQ_SE_SW_GRGYM_02);
        DriftveilGym_SaveSwitches(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(work->field))),
                                  work->pressed);
    }
}

static GameEvent *DriftveilGym_CreateElevator(GameEvent *parent, GameSystem *gsys, u32 elevator, u32 down) {
    Field *field = GSYS_GetField(gsys);
    FieldPlayer *player = Field_GetPlayer(field);
    GameEvent *event = GameEvent_Create(gsys, parent, DriftveilGym_ElevatorEventCallback, sizeof(DriftveilGymElevatorEvent));
    DriftveilGymElevatorEvent *ev = GameEvent_GetData(event);

    ev->gsys = gsys;
    ev->field = field;
    ev->player = player;
    ev->elevator = elevator;
    ev->down = down;
    if (down == 0) {
        ev->height = 0;
        ev->target = ELEVATOR_HEIGHT;
        ev->step = 3 << 12;
    } else {
        ev->target = 0;
        ev->height = ELEVATOR_HEIGHT;
        ev->step = -0x3000;
    }
    if (getTrainerGender(GetGameDataPlayerInfo(GSYS_GetGameData(gsys))) == 0) {
        ev->model = ACTOR_PLAYER_ELEVATOR_FEMALE;
    } else {
        ev->model = ACTOR_PLAYER_ELEVATOR_MALE;
    }
    return event;
}

static GameEventReturnCode DriftveilGym_ElevatorEventCallback(GameEvent *event, u32 *state, void *data) {
    DriftveilGymElevatorEvent *ev = data;
    DriftveilGymWork *work = Field_GetGimmickWorkBlock(ev->field, GIMMICK_WORK_ID);
    FieldCamera *camera = Field_GetCameraSystem(ev->field);
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(ev->field);
    FieldExpObjAnm *anm;

    switch (*state) {
    case 0:
        if (ev->down == 0) {
            anm = FieldExpObj_GetAnmInfo(expObj, 0, ev->elevator + ACTOR_ELEVATOR, ANM_MAIN);
            FieldExpObjAnm_SetPaused(anm, FALSE);
            DriftveilGym_ShowElevatorRider(ev, FALSE, ev->model);
        }
        *state = 1;
        break;
    case 1:
        GFL_SndSEPlay(SEQ_SE_FLD_59);
        *state = 2;
    case 2:
        ev->height += ev->step;
        if (DriftveilGym_HasReachedHeight(ev)) {
            ev->height = ev->target;
            *state = 3;
        }
        DriftveilGym_MoveElevator(ev);
        break;
    case 3:
        if (ev->down == 1) {
            u32 actor = ev->elevator + ACTOR_ELEVATOR;

            anm = FieldExpObj_GetAnmInfo(expObj, 0, actor, ANM_MAIN);
            FieldExpObj_SetAnmFrame(expObj, 0, actor, ANM_MAIN, 0);
            FieldExpObjAnm_SetPaused(anm, TRUE);
            DriftveilGym_ShowElevatorRider(ev, TRUE, ev->model);
        }
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_59));
        return TRUE;
    }
    return FALSE;
}

static void DriftveilGym_MoveElevator(DriftveilGymElevatorEvent *ev) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(ev->field);
    SRTMatrix *matrix;
    SRTMatrix *riderMatrix;
    VecFx32 pos;

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, ev->elevator + ACTOR_ELEVATOR);
    matrix->translation.y = ev->height;
    FieldPlayer_GetWPos(ev->player, &pos);
    pos.y = ev->height;
    FieldPlayer_SetWPos(ev->player, &pos);
    riderMatrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, ev->model);
    riderMatrix->translation.y = ev->height - func_ov036_02181324(ev->field);
}

// Shows the player's model on the elevator and hides the player, or the other way around once the ride is over
static void DriftveilGym_ShowElevatorRider(DriftveilGymElevatorEvent *ev, BOOL atFloor, u16 model) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(ev->field);
    NNSG3dResMdl *resMdl =
        GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(FieldExpObj_GetActor(expObj, 0, ev->elevator + ACTOR_ELEVATOR)))->resMdl;
    SRTMatrix *matrix;
    VecFx32 pos;
    FieldActor *playerActor;
    BOOL hidden;

    NNS_G3dMdlSetMdlFogEnableFlag(resMdl, 0, atFloor);
    NNS_G3dMdlSetMdlFogEnableFlag(resMdl, 1, atFloor);
    NNS_G3dMdlSetMdlFogEnableFlag(resMdl, 2, atFloor);
    NNS_G3dMdlSetMdlFogEnableFlag(resMdl, 3, atFloor);
    hidden = TRUE;
    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
    FieldPlayer_GetWPos(ev->player, &pos);
    matrix->translation.x = pos.x;
    matrix->translation.y = pos.y;
    matrix->translation.z = pos.z;
    FieldExpObj_SetActorHidden(expObj, 0, model, atFloor);
    playerActor = FieldPlayer_GetActor(ev->player);
    if (atFloor) {
        hidden = FALSE;
    }
    SetActorHidden(playerActor, hidden);
}

static BOOL DriftveilGym_HasReachedHeight(DriftveilGymElevatorEvent *ev) {
    if (ev->step < 0) {
        if (ev->target >= ev->height) {
            return TRUE;
        }
    } else {
        if (ev->target <= ev->height) {
            return TRUE;
        }
    }
    return FALSE;
}

static GameEvent *DriftveilGym_CreateFreight(GameEvent *parent, GameSystem *gsys, BOOL up, BOOL withRider) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, parent, DriftveilGym_FreightEventCallback, sizeof(DriftveilGymFreightEvent));
    DriftveilGymFreightEvent *ev = GameEvent_GetData(event);
    FieldExpObjAnm *anm;

    u32 model = ACTOR_FREIGHT_RIDER;

    ev->up = up;
    if (withRider != 1) {
        model = ACTOR_FREIGHT;
    }
    ev->model = model;
    if (withRider == 1) {
        ev->rider = GetFirstActorOnGPos(Field_GetActorSystem(field), 46, 18, TRUE);
        ev->hasRider = TRUE;
    } else {
        ev->rider = NULL;
        ev->hasRider = FALSE;
    }
    if (up != 0) {
        ev->height = 0;
        ev->target = ELEVATOR_HEIGHT;
    } else {
        ev->height = ELEVATOR_HEIGHT;
        ev->target = 0;
    }
    anm = FieldExpObj_GetAnmInfo(Field_GetExpObjSystem(field), 0, ev->model, ANM_MAIN);
    FieldExpObjAnm_SetPaused(anm, FALSE);
    if (ev->target - ev->height < 0) {
        ev->step = -0x2000;
    } else {
        ev->step = 2 << 12;
    }
    return event;
}

static GameEventReturnCode DriftveilGym_FreightEventCallback(GameEvent *event, u32 *state, void *data) {
    DriftveilGymFreightEvent *ev = data;
    GameSystem *gsys = GameEvent_GetGameSystem(event);
    Field *field = GSYS_GetField(gsys);
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    DriftveilGymWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    FieldCamera *camera = Field_GetCameraSystem(field);
    SRTMatrix *matrix;
    VecFx32 pos;
    VecFx32 playerPos;
    VecFx32 riderPos;
    u32 model;
    FieldPlayer *player;

    switch (*state) {
    case 0:
        if (ev->up != 0) {
            work->cameraBind = FieldCamera_GetBind(camera);
            FieldCamera_ClearBind(camera);
        }
        if (ev->up == 0) {
            GFL_SndSEPlay(SEQ_SE_FLD_75);
        } else {
            GFL_SndSEPlay(SEQ_SE_FLD_59);
        }
        if (ev->hasRider == 1) {
            player = Field_GetPlayer(field);
            FieldPlayer_GetWPos(player, &pos);
            if (getTrainerGender(GetGameDataPlayerInfo(GSYS_GetGameData(gsys))) == 0) {
                model = ACTOR_PLAYER_FREIGHT_FEMALE;
            } else {
                model = ACTOR_PLAYER_FREIGHT_MALE;
            }
            matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
            matrix->translation.x = pos.x;
            matrix->translation.y = pos.y;
            matrix->translation.z = pos.z;
            FieldExpObj_SetActorHidden(expObj, 0, model, FALSE);
            matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, ACTOR_RIDER);
            CopyActorWPos(ev->rider, &pos);
            matrix->translation.x = pos.x;
            matrix->translation.y = pos.y;
            matrix->translation.z = pos.z;
            FieldExpObj_SetActorHidden(expObj, 0, ACTOR_RIDER, FALSE);
            SetActorHidden(FieldPlayer_GetActor(player), TRUE);
            SetActorHidden(ev->rider, TRUE);
            NNS_G3dMdlSetMdlFogEnableFlagAll(
                GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(FieldExpObj_GetActor(expObj, 0, ACTOR_FREIGHT_RIDER)))->resMdl,
                FALSE);
        }
        *state += 1;
        break;
    case 1:
        ev->height += ev->step;
        if (DriftveilGym_HasFreightReachedTarget(ev)) {
            ev->height = ev->target;
            if (ev->up == 0) {
                DriftveilGym_InitShake(ev->step, FALSE, camera, &ev->shake);
                GFL_SndSEPlay(SEQ_SE_FLD_60);
            }
            *state += 1;
        }
        matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, ev->model);
        matrix->translation.y = ev->height;
        if (ev->hasRider == 0) {
            player = Field_GetPlayer(field);
            FieldPlayer_GetWPos(player, &playerPos);
            playerPos.y = ev->height;
            FieldPlayer_SetWPos(player, &playerPos);
            if (ev->rider != NULL) {
                riderPos = *GetMModelWPosPtr(ev->rider);
                riderPos.y = ev->height;
                SetActorGPosY(ev->rider, (ev->height >> 4) / 4096);
                SetActorWPosValue(ev->rider, &riderPos);
            }
        } else {
            if (getTrainerGender(GetGameDataPlayerInfo(GSYS_GetGameData(gsys))) == 0) {
                model = ACTOR_PLAYER_FREIGHT_FEMALE;
            } else {
                model = ACTOR_PLAYER_FREIGHT_MALE;
            }
            matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
            matrix->translation.y = ev->height;
            matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, ACTOR_RIDER);
            matrix->translation.y = ev->height;
        }
        break;
    case 2:
        if (ev->up != 0) {
            return TRUE;
        }
        if (DriftveilGym_UpdateShake(&ev->shake)) {
            if (work->cameraBind != NULL) {
                FieldCamera_SetBind(camera, work->cameraBind);
            }
            FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(expObj, 0, ev->model, ANM_MAIN), FALSE);
            *state += 1;
        }
        break;
    case 3:
        if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(expObj, 0, ev->model, ANM_MAIN))) {
            DriftveilGym_ResetFreight(expObj);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL DriftveilGym_HasFreightReachedTarget(DriftveilGymFreightEvent *ev) {
    if (ev->step < 0) {
        if (ev->target >= ev->height) {
            return TRUE;
        }
    } else {
        if (ev->target <= ev->height) {
            return TRUE;
        }
    }
    return FALSE;
}

static void DriftveilGym_InitShake(int step, BOOL light, FieldCamera *camera, DriftveilGymShake *shake) {
    shake->state = 0;
    shake->count = 0;
    shake->camera = camera;
    shake->bind = FieldCamera_GetBind(camera);
    FieldCamera_CoordsGetTargetOffset(camera, &shake->offset);
    if (light) {
        shake->amplitude = 1 << 12;
        shake->limit = 2;
    } else {
        shake->amplitude = 2 << 12;
        shake->limit = 4;
    }
    if (step < 0) {
        shake->amplitude *= -1;
    }
}

static BOOL DriftveilGym_UpdateShake(DriftveilGymShake *shake) {
    VecFx32 offset;

    switch (shake->state) {
    case 0:
        FieldCamera_ClearBind(shake->camera);
        shake->state++;
    case 1:
        if (shake->toggle % 2 == 0) {
            FieldCamera_CoordsGetTargetOffset(shake->camera, &offset);
            offset.y += shake->amplitude;
            FieldCamera_CoordsSetTargetOffset(shake->camera, &offset);
            shake->amplitude *= -1;
            shake->count++;
            if (shake->count >= shake->limit * 2) {
                if (shake->bind != NULL) {
                    FieldCamera_SetBind(shake->camera, shake->bind);
                }
                FieldCamera_CoordsSetTargetOffset(shake->camera, &shake->offset);
                return TRUE;
            }
        }
        shake->toggle = (shake->toggle + 1) % 2;
        break;
    }
    return FALSE;
}

static void DriftveilGym_ResetFreight(FieldExpObjSystem *expObj) {
    FieldExpObj_SetAnm(expObj, 0, ACTOR_FREIGHT, ANM_MAIN, TRUE);
    FieldExpObj_SetAnmFrame(expObj, 0, ACTOR_FREIGHT, ANM_MAIN, 0);
    FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(expObj, 0, ACTOR_FREIGHT, ANM_MAIN), TRUE);
}

// Stores the switches that are pressed in the save, a bit each
static void DriftveilGym_SaveSwitches(GimmickState *gimmickState, const BOOL *pressed) {
    u8 byte = 0;
    u8 *flags = func_0200bac8(gimmickState, GIMMICK_FLAGS_ID);
    u8 i;

    for (i = 0; i < SWITCH_COUNT; i++) {
        int bit = i % 8;

        if (bit == 0 && i != 0) {
            byte++;
        }
        if (pressed[i] == 1) {
            flags[byte] |= 1 << bit;
        }
    }
}

static void DriftveilGym_LoadSwitches(GimmickState *gimmickState, BOOL *pressed) {
    u8 byte = 0;
    u8 *flags = func_0200bac8(gimmickState, GIMMICK_FLAGS_ID);
    u8 i;

    for (i = 0; i < SWITCH_COUNT; i++) {
        int bit = i % 8;

        if (bit == 0 && i != 0) {
            byte++;
        }
        if (flags[byte] & (1 << bit)) {
            pressed[i] = TRUE;
        } else {
            pressed[i] = FALSE;
        }
    }
}
