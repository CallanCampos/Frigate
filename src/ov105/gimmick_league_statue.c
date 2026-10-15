#include "types.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_league_statue.h"
#include "gfl/g3d.h"
#include "nitro/fx.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_system.h"

typedef struct {
    u16 heapId;
} GimmickWork;

void func_ov105_021eecd4(GimmickWork *work, Field *field);
void func_ov105_021eee24(GimmickWork *work, Field *field);
void func_ov105_021eee28(GimmickWork *work, Field *field);

static const G3DSceneAnimationSetup sAnimations01[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sAnimations08[] = { { 8, 0 }, { 9, 0 } };
static const G3DSceneAnimationSetup sAnimations03[] = { { 3, 0 } };
static const G3DSceneAnimationSetup sAnimations05[] = { { 5, 0 }, { 6, 0 } };
static const G3DSceneAnimationSetup sAnimations0B[] = { { 11, 0 }, { 12, 0 } };
static const G3DSceneAnimationSetup sAnimations0E[] = { { 14, 0 }, { 15, 0 } };

// clang-format off
static const G3DSceneActorSetup sActors[6] = {
    { 0, 0, 0, 0, sAnimations01, NELEMS(sAnimations01) },
    { 2, 0, 2, 0, sAnimations03, NELEMS(sAnimations03) },
    { 4, 0, 4, 0, sAnimations05, NELEMS(sAnimations05) },
    { 7, 0, 7, 0, sAnimations08, NELEMS(sAnimations08) },
    { 10, 0, 10, 0, sAnimations0B, NELEMS(sAnimations0B) },
    { 13, 0, 13, 0, sAnimations0E, NELEMS(sAnimations0E) },
};

// clang-format on

// clang-format off
static const G3DSceneResourceSetup sResources[] = {
    { 0x9b, 0, 0 },  { 0x9b, 10, 0 }, { 0x9b, 2, 0 },  { 0x9b, 13, 0 },
    { 0x9b, 4, 0 },  { 0x9b, 19, 0 }, { 0x9b, 15, 0 }, { 0x9b, 5, 0 },
    { 0x9b, 20, 0 }, { 0x9b, 16, 0 }, { 0x9b, 6, 0 },  { 0x9b, 21, 0 },
    { 0x9b, 17, 0 }, { 0x9b, 7, 0 },  { 0x9b, 22, 0 }, { 0x9b, 18, 0 },
};

// clang-format on

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

// Where each actor is
// clang-format off
static const VecFx32 sActorPositions[6] = {
    { FX32_CONST(512), 0, FX32_CONST(672) },
    { FX32_CONST(512), 0, FX32_CONST(672) },
    { FX32_CONST(544), 0, FX32_CONST(704) },
    { FX32_CONST(448), 0, FX32_CONST(624) },
    { FX32_CONST(448), 0, FX32_CONST(704) },
    { FX32_CONST(544), 0, FX32_CONST(624) },
};

// clang-format on

void func_ov105_021eec80(Field *field) {
    u16 heapId;
    GimmickWork *work;

    heapId = Field_GetHeapID(field);
    work = Field_AllocGimmickWorkBlock(field, 0, heapId, sizeof(GimmickWork));
    work->heapId = heapId;
    func_ov105_021eecd4(work, field);
    func_ov105_021eee24(work, field);
}

void func_ov105_021eecac(Field *field) {
    GimmickWork *work;

    work = Field_GetGimmickWorkBlock(field, 0);
    func_ov105_021eee28(work, field);
    Field_DeleteGimmickWorkBlock(field, 0);
}

void func_ov105_021eecc8(Field *field) {
    FieldExpObj_StepAllAnimations(Field_GetExpObjSystem(field));
}

void func_ov105_021eecd4(GimmickWork *work, Field *field) {
    EventWork *eventWork;
    FieldExpObjSystem *system;
    SRTMatrix *matrix;
    s32 flags[4];
    s32 i;

    eventWork = GameData_GetEventWork(GSYS_GetGameData(Field_GetGameSystem(field)));
    flags[0] = EventWork_FlagGet(eventWork, 0x969);
    flags[1] = EventWork_FlagGet(eventWork, 0x968);
    flags[2] = EventWork_FlagGet(eventWork, 0x967);
    flags[3] = EventWork_FlagGet(eventWork, 0x96a);
    system = Field_GetExpObjSystem(field);
    LoadFieldExpandObjData(system, &sSceneSetup, 0);
    for (i = 0; i < 6; i++) {
        matrix = FieldExpObj_GetActorMatrixPtr(system, 0, i);
        matrix->translation.x = sActorPositions[i].x;
        matrix->translation.y = sActorPositions[i].y;
        matrix->translation.z = sActorPositions[i].z;
    }
    if (flags[0] && flags[1] && flags[2] && flags[3]) {
        FieldExpObj_SetAnm(system, 0, 0, 0, TRUE);
        FieldExpObj_SetAnm(system, 0, 1, 0, TRUE);
    }
    if (flags[0]) {
        FieldExpObj_SetAnm(system, 0, 2, 1, TRUE);
    } else {
        FieldExpObj_SetAnm(system, 0, 2, 0, TRUE);
    }
    if (flags[1]) {
        FieldExpObj_SetAnm(system, 0, 3, 1, TRUE);
    } else {
        FieldExpObj_SetAnm(system, 0, 3, 0, TRUE);
    }
    if (flags[2]) {
        FieldExpObj_SetAnm(system, 0, 4, 1, TRUE);
    } else {
        FieldExpObj_SetAnm(system, 0, 4, 0, TRUE);
    }
    if (flags[3]) {
        FieldExpObj_SetAnm(system, 0, 5, 1, TRUE);
    } else {
        FieldExpObj_SetAnm(system, 0, 5, 0, TRUE);
    }
}

void func_ov105_021eee24(GimmickWork *work, Field *field) {
}

void func_ov105_021eee28(GimmickWork *work, Field *field) {
}