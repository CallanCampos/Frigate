#include "types.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_plasma_frigate_deck.h"
#include "gfl/calctool.h"
#include "gfl/g3d.h"
#include "nitro/fx.h"
#include "system/game_system.h"

// Overlay 118 has no name string. It is gimmick 45 of overlay 36's table, which only zone 552 uses: the Plasma
// Frigate's deck, where the player boards the ship and fights its grunts with Cheren. The name is a guess. It places
// two models of archive 0x120 on the map, one of them turned around

typedef struct {
    FieldExpObjSystem *system;
    Field *field;
} GimmickWork;

static void PlasmaFrigateDeck_LoadObjects(GimmickWork *work);
static void PlasmaFrigateDeck_FreeObjects(GimmickWork *work);
static void PlasmaFrigateDeck_StepAnimations(GimmickWork *work);

static const G3DSceneResourceSetup sResources[] = {
    { 0x120, 0, 0 },
    { 0x120, 1, 0 },
};

static const G3DSceneActorSetup sActors[] = {
    { 0, 0, 0, 0, NULL, 0 },
    { 1, 0, 1, 0, NULL, 0 },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void GimmickPlasmaFrigateDeck_Setup(Field *field) {
    GimmickWork *work;

    // The game data is fetched and discarded
    GSYS_GetGameData(Field_GetGameSystem(field));
    work = Field_AllocGimmickWorkBlock(field, 1, Field_GetHeapID(field), sizeof(GimmickWork));
    work->system = Field_GetExpObjSystem(field);
    work->field = field;
    PlasmaFrigateDeck_LoadObjects(work);
}

void GimmickPlasmaFrigateDeck_End(Field *field) {
    PlasmaFrigateDeck_FreeObjects(Field_GetGimmickWorkBlock(field, 1));
    Field_DeleteGimmickWorkBlock(field, 1);
}

void GimmickPlasmaFrigateDeck_Update(Field *field) {
    PlasmaFrigateDeck_StepAnimations(Field_GetGimmickWorkBlock(field, 1));
}

static void PlasmaFrigateDeck_LoadObjects(GimmickWork *work) {
    s32 i;
    SRTMatrix *matrix;

    LoadFieldExpandObjData(work->system, &sSceneSetup, 0);
    {
        VecFx32 positions[2] = { 0 };
        u16 rotations[2] = { 0 };

        // The two versions swap the models' places and which of them is turned around
#ifdef BLACK2
        VEC_Set(&positions[0], FX32_CONST(296), 0, FX32_CONST(312));
        VEC_Set(&positions[1], FX32_CONST(296), 0, FX32_CONST(664));
        rotations[0] = 0;
        rotations[1] = 0x8000;
#else
        VEC_Set(&positions[0], FX32_CONST(296), 0, FX32_CONST(664));
        VEC_Set(&positions[1], FX32_CONST(296), 0, FX32_CONST(312));
        rotations[0] = 0x8000;
        rotations[1] = 0;
#endif
        for (i = 0; i < 2; i++) {
            matrix = FieldExpObj_GetActorMatrixPtr(work->system, 0, i);
            VEC_Set(&matrix->translation, positions[i].x, positions[i].y, positions[i].z);
            MAT3_RotationEulerZYX(0, rotations[i], 0, &matrix->rotation);
            func_ov036_021b8248(work->system, 0, i, 1);
            FieldExpObj_SetActorHidden(work->system, 0, i, FALSE);
        }
    }
}

static void PlasmaFrigateDeck_FreeObjects(GimmickWork *work) {
    FieldExpObj_FreeScene(work->system, 0);
}

static void PlasmaFrigateDeck_StepAnimations(GimmickWork *work) {
    FieldExpObj_StepAllAnimations(work->system);
}
