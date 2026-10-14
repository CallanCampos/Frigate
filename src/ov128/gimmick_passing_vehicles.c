#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/field_player.h"
#include "field/field_sound.h"
#include "field/gimmick_passing_vehicles.h"
#include "gfl/g3d.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "nitro/math.h"
#include "system/game_data.h"
#include "system/game_system.h"

// A gimmick of models that cross the zone along a road, one way in one lane and the other way in the other, waiting a
// random time between crossings. The models are the six of archive 217, each twice: a pair of models make one vehicle,
// and the lanes use the copies. A vehicle's sound plays once, when it is next to the player. The file name is a guess

#define GIMMICK_WORK_ID 1
// The archive of the models, which has no name yet
#define GIMMICK_ARCID 217
#define MODEL_COUNT 6
#define LANE_COUNT 3
// The vehicles' road: where they start and where they end, along x
#define ROAD_START_X FX32_CONST(6000)
#define ROAD_END_X FX32_CONST(7264)
// A vehicle is two models, the second behind the first
#define VEHICLE_LENGTH FX32_CONST(-96)

// A vehicle on its way across
typedef struct {
    // How many frames it has moved, in units of 16
    s32 distance;
    // How many frames it still waits before the next crossing
    u16 wait;
    // Which pair of models it is
    u16 lane;
    // Its sound has played
    BOOL soundPlayed;
} Vehicle;

typedef struct {
    u32 unused[2];
    // The one that goes up the road, and the one that comes down
    Vehicle vehicles[2];
} GimmickWork;

static void func_ov128_021eecfc(FieldSound *sound, const VecFx32 *playerPos, Vehicle *vehicle, fx32 vehicleX, u32 se);
static void func_ov128_021eeea0(GimmickWork *work, Field *field);
static void func_ov128_021eeec8(GimmickWork *work, Field *field, u32 first, u32 second);

static const G3DSceneResourceSetup sResources[MODEL_COUNT] = {
    { GIMMICK_ARCID, 0, 0 }, { GIMMICK_ARCID, 1, 0 }, { GIMMICK_ARCID, 2, 0 },
    { GIMMICK_ARCID, 3, 0 }, { GIMMICK_ARCID, 4, 0 }, { GIMMICK_ARCID, 5, 0 },
};

static const G3DSceneActorSetup sActors[MODEL_COUNT * 2] = {
    { 0, 0, 0, 0, NULL, 0 }, { 1, 0, 1, 0, NULL, 0 }, { 0, 0, 0, 0, NULL, 0 }, { 1, 0, 1, 0, NULL, 0 },
    { 2, 0, 2, 0, NULL, 0 }, { 3, 0, 3, 0, NULL, 0 }, { 2, 0, 2, 0, NULL, 0 }, { 3, 0, 3, 0, NULL, 0 },
    { 4, 0, 4, 0, NULL, 0 }, { 5, 0, 5, 0, NULL, 0 }, { 4, 0, 4, 0, NULL, 0 }, { 5, 0, 5, 0, NULL, 0 },
};

static G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void func_ov128_021eec80(Field *field) {
    FieldExpObjSystem *expObj;
    GimmickWork *work;
    HeapID heapId;

    heapId = Field_GetHeapID(field);
    expObj = Field_GetExpObjSystem(field);
    GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(GimmickWork));
    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    LoadFieldExpandObjData(expObj, &sSceneSetup, 0);
    func_ov128_021eeea0(work, field);
}

void func_ov128_021eecd4(Field *field) {
    GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    FieldExpObj_FreeScene(Field_GetExpObjSystem(field), 0);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

// Plays the sound of a vehicle when it passes the player
static void func_ov128_021eecfc(FieldSound *sound, const VecFx32 *playerPos, Vehicle *vehicle, fx32 vehicleX, u32 se) {
    if (playerPos->z < FX32_CONST(8224)) {
        return;
    }
    if (playerPos->z > FX32_CONST(8512)) {
        return;
    }
    if (playerPos->x - vehicleX < 0) {
        return;
    }
    if (playerPos->x - vehicleX > FX32_CONST(64)) {
        return;
    }
    if (vehicle->soundPlayed != FALSE) {
        return;
    }
    if (GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(se))) {
        return;
    }
    vehicle->soundPlayed = TRUE;
    FieldSnd_PlayAmbience(sound, se);
}

void func_ov128_021eed48(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    FieldExpObjSystem *expObj;
    GimmickWork *work;
    FieldSound *sound;
    VecFx32 playerPos;
    Vehicle *vehicle;
    SRTMatrix *matrix;
    int model;
    int downModel;
    Vehicle *down;
    fx32 x;

    GameData_GetGimmickState(gameData);
    expObj = Field_GetExpObjSystem(field);
    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    sound = GameData_GetFieldSoundSystem(gameData);
    FieldPlayer_GetWPos(Field_GetPlayer(field), &playerPos);

    vehicle = &work->vehicles[0];
    if (vehicle->wait != 0) {
        vehicle->wait--;
    } else {
        model = vehicle->lane * 4;
        vehicle->distance++;
        matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
        x = ROAD_START_X + vehicle->distance * FX32_CONST(16);
        matrix->translation.x = x;
        if (x >= ROAD_END_X) {
            vehicle->distance = 0;
            work->vehicles[0].soundPlayed = FALSE;
            vehicle->wait = 30 + GFL_RandomLC(30);
            vehicle->lane = GFL_RandomLC(LANE_COUNT);
        } else {
            func_ov128_021eecfc(sound, &playerPos, vehicle, x, SEQ_SE_FLD_140);
        }
        matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model + 1);
        x = ROAD_START_X + VEHICLE_LENGTH + vehicle->distance * FX32_CONST(16);
        matrix->translation.x = x;
        if (x >= ROAD_END_X) {
            vehicle->distance = 0;
        }
    }

    down = &work->vehicles[1];
    if (down->wait != 0) {
        down->wait--;
        return;
    }
    downModel = down->lane * 4 + 2;
    down->distance++;
    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, downModel);
    x = ROAD_END_X - down->distance * FX32_CONST(16);
    matrix->translation.x = x;
    if (x <= ROAD_START_X) {
        down->distance = 0;
        work->vehicles[1].soundPlayed = FALSE;
        down->wait = GFL_RandomLC(30);
        down->lane = GFL_RandomLC(LANE_COUNT);
    } else {
        func_ov128_021eecfc(sound, &playerPos, down, x, SEQ_SE_FLD_141);
    }
    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, downModel + 1);
    x = ROAD_END_X - VEHICLE_LENGTH - down->distance * FX32_CONST(16);
    matrix->translation.x = x;
    if (x <= ROAD_START_X) {
        down->distance = 0;
    }
}

static void func_ov128_021eeea0(GimmickWork *work, Field *field) {
    func_ov128_021eeec8(work, field, 0, 1);
    func_ov128_021eeec8(work, field, 4, 5);
    func_ov128_021eeec8(work, field, 8, 9);
}

// Places a vehicle's models for both lanes, the first model and the one behind it
static void func_ov128_021eeec8(GimmickWork *work, Field *field, u32 first, u32 second) {
    HeapID heapId;
    FieldExpObjSystem *expObj;
    SRTMatrix *matrix;
    fx32 cos;
    fx32 sin;
    fx32 cos2;
    fx32 sin2;

    heapId = Field_GetHeapID(field);
    expObj = Field_GetExpObjSystem(field);

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, first);
    matrix->translation.x = ROAD_START_X;
    matrix->translation.y = FX32_CONST(80);
    matrix->translation.z = FX32_CONST(8448);
    sin = FX_SinIdx(DEG_TO_IDX(90));
    cos = FX_CosIdx(DEG_TO_IDX(90));
    MTX_RotY33_(&matrix->rotation, sin, cos);
    func_ov036_021b8248(expObj, 0, first, 1);

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, second);
    matrix->translation.x = ROAD_START_X + VEHICLE_LENGTH;
    matrix->translation.y = FX32_CONST(80);
    matrix->translation.z = FX32_CONST(8448);
    MTX_RotY33_(&matrix->rotation, sin, cos);
    func_ov036_021b8248(expObj, 0, second, 1);

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, first + 2);
    matrix->translation.x = ROAD_END_X;
    matrix->translation.y = FX32_CONST(80);
    matrix->translation.z = FX32_CONST(8368);
    sin2 = FX_SinIdx(DEG_TO_IDX(270));
    cos2 = FX_CosIdx(DEG_TO_IDX(270));
    MTX_RotY33_(&matrix->rotation, sin2, cos2);
    func_ov036_021b8248(expObj, 0, first + 2, 1);

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, second + 2);
    matrix->translation.x = ROAD_END_X - VEHICLE_LENGTH;
    matrix->translation.y = FX32_CONST(80);
    matrix->translation.z = FX32_CONST(8368);
    MTX_RotY33_(&matrix->rotation, sin2, cos2);
    func_ov036_021b8248(expObj, 0, second + 2, 1);
}
