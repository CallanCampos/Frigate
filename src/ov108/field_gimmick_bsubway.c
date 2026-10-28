#include "types.h"
#include "field/field.h"
#include "field/field_effect.h"
#include "field/field_g3d_mapper.h"
#include "field/field_gimmick_bsubway.h"
#include "field/field_map.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_system.h"

// field_gimmick_bsubway.c, named by the string it passes to GFL_HeapAllocate. Each of the Battle Subway's zones 67
// to 76 keeps this in its gimmick state's user data. Zone 75's train shakes the map now and then; the others load
// the field effect of the train, which script plugin 1 starts and drives

typedef struct {
    u16 unk0;
    // The zone's entry in sZoneGimmicks and sGimmickFuncs
    u16 index;
    u16 gimmickId;
    u16 unk6;
    u32 zoneId;
    u16 heapId;
    u16 unkE;
    u8 unk10[12];
    GameSystem *gsys;
    Field *field;
    // A TrainWork, or zone 75's ShakeWork
    void *data;
} BSubwayGimmickWork;

typedef struct {
    FieldEffectTask *effect;
} TrainWork;

typedef struct {
    // Set by script to stop the shaking
    u8 stop;
    u8 state;
    u8 waitIndex;
    s32 wait;
    // How far the map is moved up, alternating in sign as it dies down
    s32 shake;
} ShakeWork;

typedef struct {
    u32 zoneId;
    u32 gimmickId;
} BSubwayZoneGimmick;

typedef void (*BSubwayGimmickFunc)(BSubwayGimmickWork *work, Field *field);

typedef struct {
    BSubwayGimmickFunc setup;
    BSubwayGimmickFunc end;
    BSubwayGimmickFunc move;
} BSubwayGimmickFuncs;

static void TrainSetup(BSubwayGimmickWork *work, Field *field);
static void TrainEnd(BSubwayGimmickWork *work, Field *field);
static void ShakeSetup(BSubwayGimmickWork *work, Field *field);
static void ShakeEnd(BSubwayGimmickWork *work, Field *field);
static void ShakeMove(BSubwayGimmickWork *work, Field *field);
static void GetZoneGimmick(u32 zoneId, u32 *gimmickId, u32 *index);

static const BSubwayZoneGimmick sZoneGimmicks[10] = {
    { 67, 0x10 }, { 68, 0x11 }, { 69, 0x12 }, { 70, 0x13 }, { 71, 0x14 },
    { 72, 0x15 }, { 73, 0x16 }, { 75, 0x17 }, { 76, 0x18 }, { 74, 0x1f },
};

// clang-format off
static const BSubwayGimmickFuncs sGimmickFuncs[10] = {
    { TrainSetup, TrainEnd, NULL },
    { TrainSetup, TrainEnd, NULL },
    { TrainSetup, TrainEnd, NULL },
    { TrainSetup, TrainEnd, NULL },
    { TrainSetup, TrainEnd, NULL },
    { TrainSetup, TrainEnd, NULL },
    { TrainSetup, TrainEnd, NULL },
    { ShakeSetup, ShakeEnd, ShakeMove },
    { TrainSetup, TrainEnd, NULL },
    { TrainSetup, TrainEnd, NULL },
};

// clang-format on

void FieldGimmickBSubway_Setup(Field *field) {
    u16 heapId;
    GameSystem *gsys;
    GimmickState *state;
    u32 zoneId;
    u32 index;
    u32 gimmickId;
    BSubwayGimmickWork *work;

    heapId = Field_GetHeapID(field);
    gsys = Field_GetGameSystem(field);
    state = GameData_GetGimmickState(GSYS_GetGameData(gsys));
    zoneId = Field_GetPlayerStateZoneID(field);
    GetZoneGimmick(zoneId, &gimmickId, &index);
    work = GimmickState_GetUserData(state, gimmickId);
    work->index = index;
    work->gimmickId = gimmickId;
    work->zoneId = zoneId;
    work->heapId = heapId;
    work->gsys = gsys;
    work->field = field;
    if (sGimmickFuncs[work->index].setup != NULL) {
        sGimmickFuncs[work->index].setup(work, field);
    }
}

void FieldGimmickBSubway_End(Field *field) {
    u16 heapId;
    GimmickState *state;
    u32 index;
    u32 gimmickId;
    BSubwayGimmickWork *work;

    heapId = Field_GetHeapID(field);
    state = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    GetZoneGimmick(Field_GetPlayerStateZoneID(field), &gimmickId, &index);
    work = GimmickState_GetUserData(state, gimmickId);
    if (sGimmickFuncs[work->index].end != NULL) {
        sGimmickFuncs[work->index].end(work, field);
    }
}

void FieldGimmickBSubway_Move(Field *field) {
    GimmickState *state;
    u32 index;
    u32 gimmickId;

    state = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    GetZoneGimmick(Field_GetPlayerStateZoneID(field), &gimmickId, &index);
    if (sGimmickFuncs[index].move != NULL) {
        sGimmickFuncs[index].move(GimmickState_GetUserData(state, gimmickId), field);
    }
}

void FieldGimmickBSubway_StartTrainEffect(Field *field, u32 a1, const VecFx32 *pos) {
    u16 heapId;
    GimmickState *state;
    u32 index;
    u32 gimmickId;
    TrainWork *train;

    heapId = Field_GetHeapID(field);
    state = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    GetZoneGimmick(Field_GetPlayerStateZoneID(field), &gimmickId, &index);
    train = ((BSubwayGimmickWork *)GimmickState_GetUserData(state, gimmickId))->data;
    train->effect = FldeffBtrain_Start(Field_GetFieldEffects(field), a1, pos);
}

FieldEffectTask *FieldGimmickBSubway_GetTrainEffect(Field *field) {
    u16 heapId;
    GimmickState *state;
    u32 index;
    u32 gimmickId;
    TrainWork *train;

    heapId = Field_GetHeapID(field);
    state = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    GetZoneGimmick(Field_GetPlayerStateZoneID(field), &gimmickId, &index);
    train = ((BSubwayGimmickWork *)GimmickState_GetUserData(state, gimmickId))->data;
    return train->effect;
}

static void TrainSetup(BSubwayGimmickWork *work, Field *field) {
    FieldEffects *effects;
    u32 id;

    effects = Field_GetFieldEffects(field);
    work->data = GFL_HeapAllocate(work->heapId, sizeof(TrainWork), TRUE, "field_gimmick_bsubway.c", 274);
    id = FLDEFF_BTRAIN;
    FieldEffects_Load(effects, &id, 1);
}

static void TrainEnd(BSubwayGimmickWork *work, Field *field) {
    GFL_HeapFree(work->data);
}

void FieldGimmickBSubway_StopShake(Field *field) {
    GimmickState *state;
    BSubwayGimmickWork *work;
    ShakeWork *shake;

    state = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    if (Field_GetPlayerStateZoneID(field) == 75) {
        work = GimmickState_GetUserData(state, 0x17);
        if (work != NULL) {
            shake = work->data;
            if (shake != NULL) {
                shake->stop = TRUE;
            }
        }
    }
}

static void ShakeSetup(BSubwayGimmickWork *work, Field *field) {
    work->data = GFL_HeapAllocate(work->heapId, sizeof(ShakeWork), TRUE, "field_gimmick_bsubway.c", 356);
}

static void ShakeEnd(BSubwayGimmickWork *work, Field *field) {
    GFL_HeapFree(work->data);
}

static void ShakeMove(BSubwayGimmickWork *work, Field *field) {
    VecFx32 pos = { 0, 0, 0 };
    s32 waits[] = { 60, 10, 60, 10 };
    ShakeWork *shake;
    FieldG3DMapper *mapper;

    shake = work->data;
    mapper = Field_GetG3DMapper(field);
    switch (shake->state) {
    case 0:
        if (shake->stop == TRUE) {
            break;
        }
        shake->wait = waits[shake->waitIndex];
        shake->waitIndex++;
        shake->waitIndex %= NELEMS(waits);
        shake->state++;
    case 1:
        if (--shake->wait > 0) {
            break;
        }
        shake->shake = 1;
        shake->state++;
        break;
    case 2:
        pos.y = shake->shake * FX32_ONE;
        FieldG3DMapper_SetOriginOffset(mapper, &pos);
        if (shake->shake == 0) {
            shake->state = 0;
        }
        if (shake->shake < 0) {
            shake->shake += 2;
            if (shake->shake > 0) {
                shake->shake = 0;
            }
        }
        shake->shake = -shake->shake;
        break;
    }
}

static void GetZoneGimmick(u32 zoneId, u32 *gimmickId, u32 *index) {
    s32 i;

    for (i = 0; i < 10; i++) {
        if (zoneId == sZoneGimmicks[i].zoneId) {
            *gimmickId = sZoneGimmicks[i].gimmickId;
            *index = i;
            return;
        }
    }
    *gimmickId = 0;
    *index = 0;
}
