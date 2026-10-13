#include "field/infinity_dungeon_gimmick.h"
#include "types.h"
#include "constants/sound.h"
#include "constants/version.h"
#include "constants/zones.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_event.h"
#include "field/field_exp_obj.h"
#include "field/field_g3d_mapper.h"
#include "field/field_prop.h"
#include "field/field_script.h"
#include "field/infinity_dungeon_rooms.h"
#include "field/player_state.h"
#include "field/event_mapchange.h"
#include "field/encounter.h"
#include "field/zone_change.h"
#include "gfl/calctool.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The field gimmick of the Black Tower and White Treehollow. It holds the dungeon, shows the models of a floor's
// rooms (the doors, the lift to the next floor and the machine at the end), animates them in events, and moves the
// player between rooms with the room change event

// The objects that the models of the scene give: the doors by their index, then the pair that the machine shows one
// of, and the lift
#define OBJECT_MACHINE_A 1
#define OBJECT_MACHINE_B 2
#define OBJECT_LIFT 3


// The archive of the models
#define ARC_INFINITY_DUNGEON 0x104

// The kind of a door event that opens the door, where any other closes it
#define DOOR_OPEN 1


// The direction of the arrival on a floor
#define DIR_ARRIVAL 4

#define SCENE_ANM_COUNT 2

// The bytes of the gimmick that its init clears, up to the dungeon
#define INIT_CLEAR_SIZE 0x54


// Where an object of a zone's scene stands
typedef struct {
    u16 zoneId;
    u16 object;
    s32 x;
    s32 y;
    s32 z;
} ObjectPlacement;

// The event of a door, which the machine event shares for the gimmick it needs
typedef struct {
    Field *field;
    InfinityDungeonGimmick *gimmick;
    u8 object;
    u32 kind;
} ObjectEvent;

typedef struct {
    Field *field;
    InfinityDungeonGimmick *gimmick;
    u32 defaultsIndex;
} LiftEvent;

static void InfinityDungeonGimmick_Init(InfinityDungeonGimmick *gimmick, Field *field, KeyDataSave *keyData, u16 a3);
static void InfinityDungeonGimmick_SetupScene(InfinityDungeon *dungeon, InfinityDungeonGimmick *gimmick);
static const ObjectPlacement *InfinityDungeonGimmick_GetPlacements(InfinityDungeonGimmick *gimmick);
static GameEventReturnCode InfinityDungeonGimmick_DoorEvent(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode InfinityDungeonGimmick_MachineEvent(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode InfinityDungeonGimmick_LiftEvent(GameEvent *event, u32 *state, void *data);
static void InfinityDungeonGimmick_ResetLiftAnimations(LiftEvent *data);
static void InfinityDungeonGimmick_HideLift(LiftEvent *data);
static void InfinityDungeonGimmick_StartLiftAnimations(LiftEvent *data);
static BOOL InfinityDungeonGimmick_LiftAnimationsFinished(LiftEvent *data);
static InfinityDungeonObject *InfinityDungeonGimmick_GetLiftObject(InfinityDungeonGimmick *gimmick, int index);
static u8 InfinityDungeonGimmick_GetLiftAnimationCount(InfinityDungeon *dungeon);
static void InfinityDungeonGimmick_HideAllActors(LiftEvent *data, BOOL hidden);
static void InfinityDungeonGimmick_FreeWalls(InfinityDungeonGimmick *gimmick);
static void InfinityDungeonGimmick_SetupDungeon(InfinityDungeon *dungeon, GameData *gameData, HeapID heapId);
static void InfinityDungeonGimmick_SeedDungeon(InfinityDungeon *dungeon, KeyDataSave *keyData);
static u32 InfinityDungeonGimmick_GetDungeonSize(void);
static u8 InfinityDungeonGimmick_GetFloorInLevel(u8 floor, u8 level);
static GameEventReturnCode InfinityDungeonRoomChange_Event(GameEvent *event, u32 *state, void *data);
static void InfinityDungeonRoomChange_SetNextPosition(InfinityDungeonRoomChange *roomChange);
static InfinityDungeonRoom *InfinityDungeonRoomChange_GetNextRoom(InfinityDungeonRoomChange *roomChange);
static void InfinityDungeonRoomChange_SetNextRoom(InfinityDungeonRoomChange *roomChange);
static void InfinityDungeonRoomChange_SetStartRoom(InfinityDungeonRoomChange *roomChange);
static void InfinityDungeonRoomChange_SetRoomPosition(InfinityDungeonRoomChange *roomChange);
static void InfinityDungeonRoomChange_SetArrivalZone(InfinityDungeonRoomChange *roomChange);
static void InfinityDungeonRoomChange_SetFloorPosition(InfinityDungeonRoomChange *roomChange);
static void InfinityDungeonRoomChange_SetArrivalPosition(InfinityDungeonRoomChange *roomChange);
static void InfinityDungeonGimmick_SetTilePosition(VecFx32 *pos, s32 x, s32 y, s32 z);

static const G3DSceneResourceSetup sBlackResources[9];
static const G3DSceneActorSetup sBlackActors[4];
static const G3DSceneResourceSetup sWhiteResources[9];
static const G3DSceneActorSetup sWhiteActors[4];

static const G3DSceneAnimationSetup sAnimationsLiftBlack[] = { { 7, 0 } };
static const G3DSceneAnimationSetup sAnimationsDoor[] = { { 2, 0 }, { 1, 0 } };
static const G3DSceneAnimationSetup sAnimationsMachine[] = { { 5, 0 }, { 4, 0 } };
static const G3DSceneAnimationSetup sAnimationsLiftWhite[] = { { 7, 0 }, { 8, 0 } };

static const G3DSceneSetup sWhiteScene = { sWhiteResources, 9, sWhiteActors, 4 };
static const G3DSceneSetup sBlackScene = { sBlackResources, 9, sBlackActors, 4 };

static const G3DSceneActorSetup sWhiteActors[4] = {
    { 0, 0, 0, 0, sAnimationsDoor, 2 },
    { 0, 0, 0, 0, sAnimationsDoor, 2 },
    { 3, 0, 3, 0, sAnimationsMachine, 2 },
    { 6, 0, 6, 0, sAnimationsLiftWhite, 2 },
};

static const G3DSceneActorSetup sBlackActors[4] = {
    { 0, 0, 0, 0, sAnimationsDoor, 2 },
    { 0, 0, 0, 0, sAnimationsDoor, 2 },
    { 3, 0, 3, 0, sAnimationsMachine, 2 },
    { 6, 0, 6, 0, sAnimationsLiftBlack, 1 },
};

static const G3DSceneResourceSetup sBlackResources[9] = {
    { ARC_INFINITY_DUNGEON, 0, 0, 0 }, { ARC_INFINITY_DUNGEON, 1, 0, 0 },  { ARC_INFINITY_DUNGEON, 2, 0, 0 },
    { ARC_INFINITY_DUNGEON, 3, 0, 0 }, { ARC_INFINITY_DUNGEON, 4, 0, 0 },  { ARC_INFINITY_DUNGEON, 5, 0, 0 },
    { ARC_INFINITY_DUNGEON, 12, 0, 0 }, { ARC_INFINITY_DUNGEON, 13, 0, 0 }, { 0, 0, 0, 0 },
};

static const G3DSceneResourceSetup sWhiteResources[9] = {
    { ARC_INFINITY_DUNGEON, 6, 0, 0 },  { ARC_INFINITY_DUNGEON, 7, 0, 0 },  { ARC_INFINITY_DUNGEON, 8, 0, 0 },
    { ARC_INFINITY_DUNGEON, 9, 0, 0 },  { ARC_INFINITY_DUNGEON, 10, 0, 0 }, { ARC_INFINITY_DUNGEON, 11, 0, 0 },
    { ARC_INFINITY_DUNGEON, 14, 0, 0 }, { ARC_INFINITY_DUNGEON, 15, 0, 0 }, { ARC_INFINITY_DUNGEON, 16, 0, 0 },
};

// The props of a room's walls, for its north, south, east and west passages. In the ROM they lie after the placements,
// at 0x021f233c, where this file's .rodata puts them among its small tables: they may belong to infinity_dungeon.c

static const ObjectPlacement sPlacements[8] = {
    { ZONE_BLACK_TOWER, 0, 120, 0, 32 },
    { ZONE_BLACK_TOWER, 1, 182, 0, 32 },
    { ZONE_BLACK_TOWER, 2, 182, 0, 32 },
    { ZONE_BLACK_TOWER_2, 0, 104, 0, 0 },
    { ZONE_WHITE_TREEHOLLOW, 0, 120, 0, 32 },
    { ZONE_WHITE_TREEHOLLOW, 1, 184, 0, 32 },
    { ZONE_WHITE_TREEHOLLOW, 2, 184, 0, 32 },
    { ZONE_WHITE_TREEHOLLOW_2, 0, 104, 0, 0 },
};

void InfinityDungeonGimmick_Setup(Field *field) {
    HeapID heapId = Field_GetHeapID(field);
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    InfinityDungeonGimmick *gimmick = Field_AllocGimmickWorkBlock(field, 0, heapId, sizeof(InfinityDungeonGimmick));
    sys_memset(gimmick, 0, sizeof(InfinityDungeonGimmick));
    KeyDataSave *keyData = getKeyDataBlkAddress(GameData_GetSaveControl(gameData));
    InfinityDungeonGimmick_Init(gimmick, field, keyData, func_02017220(gameData));
    gimmick->dungeon = GFL_HeapAllocate(heapId, InfinityDungeonGimmick_GetDungeonSize(), FALSE, "infinity_dungeon_gimmick.c", 510);
    InfinityDungeonGimmick_SetupDungeon(gimmick->dungeon, gameData, heapId);
    InfinityDungeonGimmick_SetupScene(gimmick->dungeon, gimmick);
    func_ov012_02166fc4(Field_GetActorSystem(field), 8);
}

void InfinityDungeonGimmick_Delete(Field *field) {
    InfinityDungeonGimmick *gimmick = Field_GetGimmickWorkBlock(field, 0);
    FieldExpObj_FreeScene(gimmick->expObjSystem, 0);
    GFL_HeapFree(gimmick->dungeon);
    Field_DeleteGimmickWorkBlock(field, 0);
    func_ov012_02166fc4(Field_GetActorSystem(field), 1);
}

void InfinityDungeonGimmick_Update(Field *field) {
    InfinityDungeonGimmick *gimmick = Field_GetGimmickWorkBlock(field, 0);
    FieldExpObj_StepAllAnimations(gimmick->expObjSystem);
}

// Loads the scene, puts its objects in place for the zone and shows the machine and the lift as the save says
static void InfinityDungeonGimmick_SetupScene(InfinityDungeon *dungeon, InfinityDungeonGimmick *gimmick) {
    int i;
    u32 j;
    const ObjectPlacement *placements;
    u32 anm;
    InfinityDungeonObject *object;

    if (func_ov127_021f0dd8(dungeon) == VERSION_BLACK2) {
        LoadFieldExpandObjData(gimmick->expObjSystem, &sBlackScene, 0);
    } else {
        LoadFieldExpandObjData(gimmick->expObjSystem, &sWhiteScene, 0);
    }
    for (i = 0; i < INFINITY_DUNGEON_OBJECT_COUNT; i++) {
        object = &gimmick->objects[i];
        object->scene = 0;
        object->actor = i;
        object->expObj = gimmick->expObjSystem;
        FieldExpObj_SetActorHidden(object->expObj, object->scene, object->actor, TRUE);
    }

    placements = InfinityDungeonGimmick_GetPlacements(gimmick);
    for (j = 0; j < 8; j++) {
        object = &gimmick->objects[placements[j].object];

        if (gimmick->zoneId == placements[j].zoneId) {
            SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(object->expObj, 0, object->actor);

            matrix->translation.x = FX32_CONST(placements[j].x);
            matrix->translation.y = FX32_CONST(placements[j].y);
            matrix->translation.z = FX32_CONST(placements[j].z);
            FieldExpObj_SetActorHidden(object->expObj, 0, object->actor, FALSE);
            for (anm = 0; anm < SCENE_ANM_COUNT; anm++) {
                FieldExpObj_SetAnm(object->expObj, 0, object->actor, anm, TRUE);
                FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(object->expObj, 0, object->actor, anm), TRUE);
            }
        }
    }

    if (gimmick->zoneId - ZONE_WHITE_TREEHOLLOW <= 1) {
        u32 level = func_02010274(gimmick->keyData, gimmick->unk50);
        InfinityDungeonObject *machineA = &gimmick->objects[OBJECT_MACHINE_A];
        InfinityDungeonObject *machineB = &gimmick->objects[OBJECT_MACHINE_B];

        if (level <= 4) {
            FieldExpObj_SetActorHidden(machineA->expObj, 0, machineA->actor, TRUE);
            FieldExpObj_SetActorHidden(machineB->expObj, 0, machineB->actor, FALSE);
        } else {
            FieldExpObj_SetActorHidden(machineA->expObj, 0, machineA->actor, FALSE);
            FieldExpObj_SetActorHidden(machineB->expObj, 0, machineB->actor, TRUE);
        }
    } else {
        u8 floor = func_020102d4(gimmick->keyData);
        u8 room = func_020102ec(gimmick->keyData);

        if (func_ov127_021f093c(dungeon, floor, room)) {
            u8 anm;
            u8 count;
            InfinityDungeonObject *lift = &gimmick->objects[OBJECT_LIFT];
            BOOL done = func_ov127_021f096c(dungeon, gimmick->keyData);

            FieldExpObj_SetActorHidden(lift->expObj, 0, lift->actor, FALSE);
            count = InfinityDungeonGimmick_GetLiftAnimationCount(dungeon);
            for (anm = 0; anm < count; anm++) {
                FieldExpObjAnm *info;

                FieldExpObj_SetAnm(lift->expObj, 0, lift->actor, anm, TRUE);
                info = FieldExpObj_GetAnmInfo(lift->expObj, 0, lift->actor, anm);
                FieldExpObjAnm_SetPaused(info, TRUE);
                if (done == TRUE) {
                    FieldExpObj_SetAnmFrame(lift->expObj, 0, lift->actor, anm, func_ov036_021b8580(info));
                }
            }
        }
    }
}

static void InfinityDungeonGimmick_Init(InfinityDungeonGimmick *gimmick, Field *field, KeyDataSave *keyData, u16 a3) {
    sys_memset(gimmick, 0, INIT_CLEAR_SIZE);
    gimmick->field = field;
    gimmick->zoneId = Field_GetPlayerStateZoneID(field);
    gimmick->expObjSystem = Field_GetExpObjSystem(gimmick->field);
    gimmick->keyData = keyData;
    gimmick->unk50 = a3;
}

static const ObjectPlacement *InfinityDungeonGimmick_GetPlacements(InfinityDungeonGimmick *gimmick) {
    return sPlacements;
}

InfinityDungeon *InfinityDungeonGimmick_GetDungeon(Field *field) {
    InfinityDungeonGimmick *gimmick = Field_GetGimmickWorkBlock(field, 0);
    return gimmick->dungeon;
}

GameEvent *InfinityDungeonGimmick_CreateDoorEvent(GameSystem *gsys, Field *field, u16 kind, u16 object) {
    GameEvent *event = GameEvent_Create(gsys, NULL, InfinityDungeonGimmick_DoorEvent, sizeof(ObjectEvent));
    ObjectEvent *data = GameEvent_GetData(event);

    data->field = field;
    data->gimmick = Field_GetGimmickWorkBlock(field, 0);
    data->object = object;
    data->kind = kind;
    return event;
}

GameEvent *InfinityDungeonGimmick_CreateMachineEvent(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, InfinityDungeonGimmick_MachineEvent, sizeof(ObjectEvent));
    ObjectEvent *data = GameEvent_GetData(event);

    data->field = field;
    data->gimmick = Field_GetGimmickWorkBlock(field, 0);
    return event;
}

// Opens or closes a door by playing its animations over a range of frames, with the sound of it
static GameEventReturnCode InfinityDungeonGimmick_DoorEvent(GameEvent *event, u32 *state, void *data) {
    u32 anm;
    ObjectEvent *door = data;
    InfinityDungeonGimmick *gimmick = door->gimmick;
    InfinityDungeonObject *object = &gimmick->objects[door->object];
    InfinityDungeon *dungeon = gimmick->dungeon;
    u32 se;
    fx32 endFrame;
    fx32 startFrame;

    if (door->kind == DOOR_OPEN) {
        startFrame = 0;
        endFrame = FX32_CONST(14);
        if (func_ov127_021f0dd8(dungeon) == VERSION_BLACK2) {
            se = SEQ_SE_SW_MDUN_BL_01;
        } else {
            se = SEQ_SE_SW_MDUN_WH_01;
        }
    } else {
        startFrame = FX32_CONST(15);
        endFrame = FX32_CONST(29);
        if (func_ov127_021f0dd8(dungeon) == VERSION_BLACK2) {
            se = SEQ_SE_SW_MDUN_BL_02;
        } else {
            se = SEQ_SE_SW_MDUN_WH_02;
        }
    }

    switch (*state) {
    case 0:
        for (anm = 0; anm < SCENE_ANM_COUNT; anm++) {
            FieldExpObjAnm *info = FieldExpObj_GetAnmInfo(object->expObj, 0, object->actor, anm);
            FieldExpObj_SetAnmFrame(object->expObj, 0, object->actor, anm, startFrame);
            FieldExpObjAnm_SetPaused(info, FALSE);
        }
        GFL_SndSEPlay(se);
        (*state)++;
        break;
    case 1:
        if ((fx32)func_ov036_021b8520(object->expObj, 0, object->actor, 0) >= endFrame) {
            for (anm = 0; anm < SCENE_ANM_COUNT; anm++) {
                FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(object->expObj, 0, object->actor, anm), TRUE);
            }
            (*state)++;
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

// Plays the machine's animation, then shows its second model in place of the first
static GameEventReturnCode InfinityDungeonGimmick_MachineEvent(GameEvent *event, u32 *state, void *data) {
    u32 anm;
    ObjectEvent *machine = data;
    InfinityDungeonGimmick *gimmick = machine->gimmick;
    InfinityDungeonObject *machineB = &gimmick->objects[OBJECT_MACHINE_B];
    InfinityDungeonObject *machineA = &gimmick->objects[OBJECT_MACHINE_A];

    switch (*state) {
    case 0:
        for (anm = 0; anm < SCENE_ANM_COUNT; anm++) {
            FieldExpObjAnm *info = FieldExpObj_GetAnmInfo(machineB->expObj, machineB->scene, machineB->actor, anm);
            FieldExpObj_SetAnmFrame(machineB->expObj, machineB->scene, machineB->actor, anm, 0);
            FieldExpObjAnm_SetPaused(info, FALSE);
        }
        GFL_SndSEPlay(SEQ_SE_SW_MDUN_BL_06);
        (*state)++;
        break;
    case 1: {
        if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(machineB->expObj, 0, machineB->actor, 0)) == TRUE) {
            FieldExpObj_SetActorHidden(machineB->expObj, machineB->scene, machineB->actor, TRUE);
            FieldExpObj_SetActorHidden(machineA->expObj, machineA->scene, machineA->actor, FALSE);
            (*state)++;
        }
        break;
    }
    case 2:
        if (!GFL_SndPlayerIsActiveAny()) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *InfinityDungeonGimmick_CreateLiftEvent(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, InfinityDungeonGimmick_LiftEvent, sizeof(LiftEvent));
    LiftEvent *data = GameEvent_GetData(event);

    data->field = field;
    data->gimmick = Field_GetGimmickWorkBlock(field, 0);
    return event;
}

// The lift to the next floor: the camera looks at the lift and the lift runs, then the arrival on the next floor is
// set up
static GameEventReturnCode InfinityDungeonGimmick_LiftEvent(GameEvent *event, u32 *state, void *data) {
    LiftEvent *lift = data;
    FieldCamera *camera;

    switch (*state) {
    case 0:
        camera = Field_GetCameraSystem(lift->field);
        GFL_FadeSet(3, 0, 16, 4);
        FieldCamera_EVCameraInit(camera);
        FieldCamera_FinishDelay(camera);
        (*state)++;
        break;
    case 1:
        camera = Field_GetCameraSystem(lift->field);
        if (FieldCamera_IsDelayActive(camera) || GFL_FadeIsRunning()) {
            break;
        }
        InfinityDungeonGimmick_HideAllActors(lift, TRUE);
        InfinityDungeonGimmick_ResetLiftAnimations(lift);
        InfinityDungeonGimmick_FreeWalls(lift->gimmick);
        GFL_FadeSet(3, 16, 0, 4);
        lift->defaultsIndex = FieldCamera_GetDefaultsIndex(camera);
        FieldCamera_ClearBind(camera);
        {
            FieldEvCameraAnimationSetup setup;

            setup.targetCoords.pitch = 0x1758;
            setup.targetCoords.yaw = 0;
            setup.targetCoords.distance = 181 * FX32_ONE;
            setup.targetCoords.targetPos.x = 106 * FX32_ONE;
            setup.targetCoords.targetPos.y = 17 * FX32_ONE;
            setup.targetCoords.targetPos.z = 28 * FX32_ONE;
            setup.flags.animateExtraTranslation = FALSE;
            setup.flags.animatePitch = TRUE;
            setup.flags.animateYaw = TRUE;
            setup.flags.animateTargetDistance = TRUE;
            setup.flags.animateFOV = FALSE;
            setup.flags.animateTargetPos = TRUE;
            FieldCameraAnm_SetAnimation(camera, &setup, 1);
        }
        (*state)++;
        break;
    case 2:
        if (FieldCamera_IsAnimating(Field_GetCameraSystem(lift->field)) || GFL_FadeIsRunning()) {
            break;
        }
        InfinityDungeonGimmick_StartLiftAnimations(lift);
        if (func_ov127_021f0dd8(lift->gimmick->dungeon) == VERSION_BLACK2) {
            GFL_SndSEPlay(SEQ_SE_SW_MDUN_BL_07);
        } else {
            GFL_SndSEPlay(SEQ_SE_SW_MDUN_WH_07);
        }
        (*state)++;
        break;
    case 3:
        if (!InfinityDungeonGimmick_LiftAnimationsFinished(lift) || GFL_SndPlayerIsActiveAny()) {
            break;
        }
        GFL_FadeSet(3, 0, 16, 4);
        (*state)++;
        break;
    case 4:
        if (GFL_FadeIsRunning()) {
            break;
        }
        InfinityDungeonGimmick_HideAllActors(lift, FALSE);
        InfinityDungeonGimmick_HideLift(lift);
        func_ov127_021efd60(Field_GetGameSystem(lift->field), lift->gimmick->dungeon);
        camera = Field_GetCameraSystem(lift->field);
        FieldCameraAnm_SetLoadDefaultsAnimation(camera, 1);
        FieldCameraAnm_EVCameraEnd(camera);
        FieldCamera_EnableDelay(camera);
        FieldCamera_ResetBind(camera);
        (*state)++;
        break;
    case 5:
        if (FieldCamera_IsAnimating(Field_GetCameraSystem(lift->field))) {
            break;
        }
        GFL_FadeSet(3, 16, 0, 4);
        (*state)++;
        break;
    case 6:
        if (GFL_FadeIsRunning()) {
            break;
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Shows the lift and rewinds its animations
static void InfinityDungeonGimmick_ResetLiftAnimations(LiftEvent *data) {
    u8 anm;
    InfinityDungeonObject *lift = InfinityDungeonGimmick_GetLiftObject(data->gimmick, OBJECT_LIFT);
    InfinityDungeon *dungeon = data->gimmick->dungeon;
    u8 count;

    FieldExpObj_SetActorHidden(lift->expObj, 0, lift->actor, FALSE);
    count = InfinityDungeonGimmick_GetLiftAnimationCount(dungeon);
    for (anm = 0; anm < count; anm++) {
        FieldExpObjAnm *info;

        FieldExpObj_SetAnm(lift->expObj, 0, lift->actor, anm, TRUE);
        info = FieldExpObj_GetAnmInfo(lift->expObj, 0, lift->actor, anm);
        FieldExpObj_SetAnmFrame(lift->expObj, 0, lift->actor, anm, 0);
        FieldExpObjAnm_SetPaused(info, TRUE);
        FieldExpObjAnm_SetLooped(info, FALSE);
    }
}

static void InfinityDungeonGimmick_HideLift(LiftEvent *data) {
    InfinityDungeonObject *lift = InfinityDungeonGimmick_GetLiftObject(data->gimmick, OBJECT_LIFT);
    FieldExpObj_SetActorHidden(lift->expObj, 0, lift->actor, TRUE);
}

static void InfinityDungeonGimmick_StartLiftAnimations(LiftEvent *data) {
    u8 anm;
    InfinityDungeonObject *lift = InfinityDungeonGimmick_GetLiftObject(data->gimmick, OBJECT_LIFT);
    InfinityDungeon *dungeon = data->gimmick->dungeon;
    u8 count;

    FieldExpObj_SetActorHidden(lift->expObj, 0, lift->actor, FALSE);
    count = InfinityDungeonGimmick_GetLiftAnimationCount(dungeon);
    for (anm = 0; anm < count; anm++) {
        FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(lift->expObj, 0, lift->actor, anm), FALSE);
    }
}

static BOOL InfinityDungeonGimmick_LiftAnimationsFinished(LiftEvent *data) {
    u8 anm;
    u8 count;
    InfinityDungeonObject *lift = InfinityDungeonGimmick_GetLiftObject(data->gimmick, OBJECT_LIFT);

    count = InfinityDungeonGimmick_GetLiftAnimationCount(data->gimmick->dungeon);
    for (anm = 0; anm < count; anm++) {
        if (!FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(lift->expObj, 0, lift->actor, anm))) {
            return FALSE;
        }
    }
    return TRUE;
}

// The index is not used: the lift is the only object that the callers ask for
static InfinityDungeonObject *InfinityDungeonGimmick_GetLiftObject(InfinityDungeonGimmick *gimmick, int index) {
    return &gimmick->objects[OBJECT_LIFT];
}

// The black lift has one animation, the white one two
static u8 InfinityDungeonGimmick_GetLiftAnimationCount(InfinityDungeon *dungeon) {
    if (func_ov127_021f0dd8(dungeon) == VERSION_BLACK2) {
        return 1;
    }
    return 2;
}

static void InfinityDungeonGimmick_HideAllActors(LiftEvent *data, BOOL hidden) {
    FieldActor *actor;
    u32 index = 0;
    MMSys *actorSystem = Field_GetActorSystem(data->field);

    while (NextActor(actorSystem, &actor, &index) == TRUE) {
        SetActorHidden(actor, hidden);
    }
}

static void InfinityDungeonGimmick_FreeWalls(InfinityDungeonGimmick *gimmick) {
    int i;

    for (i = 0; i < INFINITY_DUNGEON_WALL_COUNT; i++) {
        if (gimmick->walls[i] != NULL) {
            FieldPropHandle_Free(gimmick->walls[i]);
        }
    }
}

void InfinityDungeonGimmick_SetWall(InfinityDungeonGimmick *gimmick, FieldPropHandle *handle, u32 index) {
    gimmick->walls[index] = handle;
}

// Fills in a dungeon from the save: its version, then everything its seed gives
static void InfinityDungeonGimmick_SetupDungeon(InfinityDungeon *dungeon, GameData *gameData, HeapID heapId) {
    KeyDataSave *keyData = getKeyDataBlkAddress(GameData_GetSaveControl(gameData));
    u16 version = func_02017220(gameData);

    sys_memset(dungeon, 0, sizeof(InfinityDungeon));
    if (version == 0) {
        dungeon->version = VERSION_BLACK2;
    } else {
        dungeon->version = VERSION_WHITE2;
    }
    InfinityDungeon_Seed(dungeon, func_02010284(keyData));
    func_ov127_021f0040(keyData, dungeon);
}

static void InfinityDungeonGimmick_SeedDungeon(InfinityDungeon *dungeon, KeyDataSave *keyData) {
    InfinityDungeon_Seed(dungeon, func_02010284(keyData));
    func_ov127_021f0040(keyData, dungeon);
}

static u32 InfinityDungeonGimmick_GetDungeonSize(void) {
    return sizeof(InfinityDungeon);
}

// Moves up a floor of a dungeon that the player has just started, or into the arrival
void InfinityDungeonGimmick_CallRoomChange(ScriptWork *work, GameSystem *gsys, InfinityDungeon *dungeon, u16 direction, u8 inTransition,
                         u8 outTransition) {
    GameEvent *event = GameEvent_Create(gsys, NULL, InfinityDungeonRoomChange_Event, sizeof(InfinityDungeonRoomChange));
    InfinityDungeonRoomChange *roomChange;

    ScriptWork_CallEvent(work, event);
    roomChange = GameEvent_GetData(event);
    roomChange->dungeon = dungeon;
    roomChange->direction = direction;
    roomChange->mode = 0;
    roomChange->inTransition = inTransition;
    roomChange->outTransition = outTransition;
}

void InfinityDungeonGimmick_CallFloorChange(ScriptWork *work, GameSystem *gsys, InfinityDungeon *dungeon, u16 a3) {
    KeyDataSave *keyData = getKeyDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    GameEvent *event;
    InfinityDungeonRoomChange *roomChange;
    u8 level;

    func_02010338(keyData, a3);
    level = func_02010378(keyData);
    event = GameEvent_Create(gsys, NULL, InfinityDungeonRoomChange_Event, sizeof(InfinityDungeonRoomChange));
    ScriptWork_CallEvent(work, event);
    roomChange = GameEvent_GetData(event);
    roomChange->nextFloor = InfinityDungeonGimmick_GetFloorInLevel(a3, level);
    roomChange->mode = 1;
    roomChange->dungeon = dungeon;
    roomChange->direction = 1;
    roomChange->outTransition = 8;
    roomChange->inTransition = 7;
    InfinityDungeonGimmick_SeedDungeon(dungeon, keyData);
}

// How many floors of the level the player is on
static u8 InfinityDungeonGimmick_GetFloorInLevel(u8 floor, u8 level) {
    return floor - func_020103a0(level);
}

void InfinityDungeonRoomChange_Init(InfinityDungeonRoomChange *roomChange, GameSystem *gsys) {
    u8 version = func_ov127_021f0dd8(roomChange->dungeon);

    roomChange->heapId = HEAPID_FIELDMAP;
    roomChange->gsys = gsys;
    roomChange->field = GSYS_GetField(gsys);
    roomChange->gameData = GSYS_GetGameData(roomChange->gsys);
    roomChange->keyData = getKeyDataBlkAddress(GameData_GetSaveControl(roomChange->gameData));
    roomChange->roomZones = func_ov127_021f0acc(version);
    roomChange->room = func_020102ec(roomChange->keyData);
    roomChange->floor = func_020102d4(roomChange->keyData);
    roomChange->g3dMapper = Field_GetG3DMapper(roomChange->field);
    roomChange->propSystem = FieldG3DMapper_GetBMSystem(roomChange->g3dMapper);
    roomChange->actorSystem = Field_GetActorSystem(roomChange->field);
    roomChange->level = func_02010378(roomChange->keyData);
}

// Moves the player to another room: the fade out, the new room's zone, then the fade in with the room's walls and
// trainers in place
static GameEventReturnCode InfinityDungeonRoomChange_Event(GameEvent *event, u32 *state, void *data) {
    InfinityDungeonRoomChange *roomChange = data;

    switch (*state) {
    case 0:
        InfinityDungeonRoomChange_Init(roomChange, GameEvent_GetGameSystem(event));
        DisableAllActorsMovement(Field_GetActorSystem(roomChange->field));
        if (roomChange->outTransition == 8) {
            if (func_ov127_021f08d0(roomChange)) {
                GFL_FadeSet(3, 0, 16, 4);
            } else {
                GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(roomChange->gsys,
                                                                                    roomChange->field, 0, 0));
            }
        }
        (*state)++;
        break;
    case 1: {
        InfinityDungeonDestination *destination;

        if (GFL_FadeIsRunning()) {
            break;
        }
        destination = &roomChange->destination;
        InfinityDungeonRoomChange_SetNextPosition(roomChange);
        func_ov012_0215ef2c(Field_GetActorSystem(roomChange->field));
        if (roomChange->direction == DIR_ARRIVAL) {
            SetPlayerSpecialState(GameData_GetPlayerState(roomChange->gameData), 0);
            func_ov012_021591b4(roomChange->gameData);
        }
        GameEvent_ChainNext(event, EventMapChange_CreateGridDefault(roomChange->gsys, roomChange->field,
                                                                    destination->zoneId, &destination->pos,
                                                                    func_ov127_021efdac(roomChange->direction)));
        (*state)++;
        break;
    }
    case 2:
        if (FldActSys_IsAsyncLoadPending(Field_GetActorSystem(roomChange->field)) == TRUE) {
            break;
        }
        roomChange->dungeon = InfinityDungeonGimmick_GetDungeon(roomChange->field);
        if (roomChange->direction != DIR_ARRIVAL) {
            InfinityDungeonRoom *room = InfinityDungeonRoomChange_GetNextRoom(roomChange);

            if (room->kind != ROOM_KIND_GOAL) {
                InfinityDungeonRoomChange_CreateWalls(roomChange, room, roomChange->nextFloor, roomChange->nextRoom);
                InfinityDungeonRoomChange_CreateTrainers(roomChange, room);
                func_ov127_021efde0(roomChange, room);
            }
        }
        if (roomChange->inTransition == 8) {
            if (func_ov127_021f08d0(roomChange)) {
                FieldG3D_RestoreSurface(roomChange->field);
                GFL_FadeSet(3, 16, 0, 4);
            } else {
                GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(roomChange->gsys, roomChange->field, 0, 0,
                                                                            1, 0, 0));
            }
        } else {
            FieldG3D_RestoreSurface(roomChange->field);
        }
        (*state)++;
        break;
    case 3:
        EnableAllActorsMovement(Field_GetActorSystem(roomChange->field));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void InfinityDungeonRoomChange_SetNextPosition(InfinityDungeonRoomChange *roomChange) {
    if (roomChange->mode == 0) {
        if (roomChange->direction != DIR_ARRIVAL) {
            InfinityDungeonRoomChange_SetNextRoom(roomChange);
            InfinityDungeonRoomChange_SetRoomPosition(roomChange);
        } else {
            InfinityDungeonRoomChange_SetArrivalZone(roomChange);
            InfinityDungeonRoomChange_SetArrivalPosition(roomChange);
        }
    } else if (roomChange->mode == 1) {
        InfinityDungeonRoomChange_SetStartRoom(roomChange);
        InfinityDungeonRoomChange_SetFloorPosition(roomChange);
    }
}

static InfinityDungeonRoom *InfinityDungeonRoomChange_GetNextRoom(InfinityDungeonRoomChange *roomChange) {
    return &roomChange->dungeon->floors[roomChange->nextFloor].rooms[roomChange->nextRoom];
}

InfinityDungeonRoom *InfinityDungeonRoomChange_GetRoom(InfinityDungeonRoomChange *roomChange) {
    return &roomChange->dungeon->floors[roomChange->floor].rooms[roomChange->room];
}

// The room that a direction leads to, which stays the same when it would leave the floor's grid
static void InfinityDungeonRoomChange_SetNextRoom(InfinityDungeonRoomChange *roomChange) {
    InfinityDungeonRoom *room;
    InfinityDungeonDestination *destination = &roomChange->destination;
    u16 direction = roomChange->direction;

    roomChange->nextRoom = roomChange->room;
    roomChange->nextFloor = roomChange->floor;
    switch (direction) {
    case DIR_UP:
        roomChange->nextRoom = roomChange->room - 4;
        break;
    case DIR_DOWN:
        roomChange->nextRoom = roomChange->room + 4;
        break;
    case DIR_LEFT:
        roomChange->nextRoom = roomChange->room - 1;
        break;
    case DIR_RIGHT:
        roomChange->nextRoom = roomChange->room + 1;
        break;
    }
    if (roomChange->nextRoom < 0) {
        roomChange->nextRoom = roomChange->room;
    } else if (roomChange->nextRoom >= INFINITY_DUNGEON_ROOM_COUNT) {
        roomChange->nextRoom = roomChange->room;
    }
    room = InfinityDungeonRoomChange_GetNextRoom(roomChange);
    destination->zoneId = roomChange->roomZones[room->kind];
    func_0201033c(roomChange->keyData, roomChange->nextRoom);
}

// The first room of the floor that has no template yet
static void InfinityDungeonRoomChange_SetStartRoom(InfinityDungeonRoomChange *roomChange) {
    InfinityDungeonDestination *destination = &roomChange->destination;
    int i;

    roomChange->nextRoom = 0;
    for (i = 0; i < INFINITY_DUNGEON_ROOM_COUNT; i++) {
        if (InfinityDungeonRoomChange_GetNextRoom(roomChange)->kind == ROOM_KIND_START) {
            break;
        }
        roomChange->nextRoom++;
    }
    destination->zoneId = roomChange->roomZones[InfinityDungeonRoomChange_GetNextRoom(roomChange)->kind];
    func_0201033c(roomChange->keyData, roomChange->nextRoom);
    roomChange->floor = roomChange->nextFloor;
}

static void InfinityDungeonRoomChange_SetRoomPosition(InfinityDungeonRoomChange *roomChange) {
    u8 version = func_ov127_021f0dd8(roomChange->dungeon);
    InfinityDungeonRoom *room = InfinityDungeonRoomChange_GetNextRoom(roomChange);
    const InfinityDungeonPos *pos = func_ov127_021f0b04(version, room->kind, roomChange->direction);

    InfinityDungeonGimmick_SetTilePosition(&roomChange->destination.pos, pos->x, pos->y, pos->z);
}

static void InfinityDungeonRoomChange_SetArrivalZone(InfinityDungeonRoomChange *roomChange) {
    roomChange->destination.zoneId = roomChange->roomZones[5];
    InfinityDungeonRoomChange_SetFloorPosition(roomChange);
}

static void InfinityDungeonRoomChange_SetFloorPosition(InfinityDungeonRoomChange *roomChange) {
    u8 version = func_ov127_021f0dd8(roomChange->dungeon);
    const InfinityDungeonPos *pos = func_ov127_021f0ae0(version);

    InfinityDungeonGimmick_SetTilePosition(&roomChange->destination.pos, pos->x, pos->y, pos->z);
}

static void InfinityDungeonRoomChange_SetArrivalPosition(InfinityDungeonRoomChange *roomChange) {
    InfinityDungeonDestination *destination = &roomChange->destination;
    const InfinityDungeonPos *pos;

    if (roomChange->level <= 4) {
        pos = func_ov127_021f0af4(0);
    } else {
        pos = func_ov127_021f0af4(1);
    }
    InfinityDungeonGimmick_SetTilePosition(&destination->pos, pos->x, pos->y, pos->z);
}

// A position in the middle of a tile, 16 units wide
static void InfinityDungeonGimmick_SetTilePosition(VecFx32 *pos, s32 x, s32 y, s32 z) {
    pos->x = ((x << 4) + 8) << FX32_SHIFT;
    pos->y = ((y << 4) + 8) << FX32_SHIFT;
    pos->z = ((z << 4) + 8) << FX32_SHIFT;
}
