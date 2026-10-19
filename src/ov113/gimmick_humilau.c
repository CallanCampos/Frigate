#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_humilau.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_event.h"
#include "system/game_system.h"

// Overlay 113 has no name string. It is gimmick 54 of overlay 36's table, which Route 21, Humilau City and Route 22
// (zones 463, 465 and 474) use, so the name is a guess. It loads a model of archive 0x114 with two animations, which
// script plugin 15 shows and plays at a grid square with SEQ_SE_SW_SHIZUI. The plugin's other command makes an actor
// jump up from under the map at grid square (791, 151) and land on another square

// A grid square is 16 units
#define GRID_SIZE FX32_CONST(16)

typedef struct {
    FieldExpObjSystem *system;
    Field *field;
} GimmickWork;

typedef struct {
    FieldActor *actor;
    s16 frame;
    VecFx32 start;
    VecFx32 distance;
    VecFx32 end;
} JumpWork;

static void GimmickHumilau_LoadSplash(GimmickWork *work);
static void GimmickHumilau_FreeScene(GimmickWork *work);
static void GimmickHumilau_StepAnimations(GimmickWork *work);
static void GimmickHumilau_PlayActor(FieldExpObjSystem *system, u16 actor);
static GameEventReturnCode GimmickHumilau_JumpEvent(GameEvent *event, u32 *state, void *data);

static const G3DSceneResourceSetup sResources[] = {
    { 0x114, 22, 0 },
    { 0x114, 23, 0 },
    { 0x114, 24, 0 },
};

static const G3DSceneAnimationSetup sAnimations[] = { { 1, 0 }, { 2, 0 } };

static const G3DSceneActorSetup sActors[] = {
    { 0, 0, 0, 0, sAnimations, NELEMS(sAnimations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

// The height of the jump on each frame
static const fx32 sJumpHeights[] = {
    FX32_CONST(2),  FX32_CONST(6),  FX32_CONST(10), FX32_CONST(14), FX32_CONST(18), FX32_CONST(21),
    FX32_CONST(24), FX32_CONST(27), FX32_CONST(30), FX32_CONST(32), FX32_CONST(34), FX32_CONST(36),
    FX32_CONST(38), FX32_CONST(38), FX32_CONST(36), FX32_CONST(34), FX32_CONST(33), FX32_CONST(31),
    FX32_CONST(29), FX32_CONST(28), FX32_CONST(26), FX32_CONST(24), FX32_CONST(22),
};

void GimmickHumilau_Init(Field *field) {
    GimmickWork *work;

    Field_AllocGimmickWorkBlock(field, 1, Field_GetHeapID(field), sizeof(GimmickWork));
    work = Field_GetGimmickWorkBlock(field, 1);
    work->field = field;
    work->system = Field_GetExpObjSystem(field);
    GimmickHumilau_LoadSplash(work);
}

void GimmickHumilau_End(Field *field) {
    GimmickHumilau_FreeScene(Field_GetGimmickWorkBlock(field, 1));
    Field_DeleteGimmickWorkBlock(field, 1);
}

void GimmickHumilau_Update(Field *field) {
    GimmickHumilau_StepAnimations(Field_GetGimmickWorkBlock(field, 1));
}

void GimmickHumilau_ShowSplash(GameSystem *gsys, u16 x, u16 y, u16 z) {
    GameData *gameData;
    Field *field;
    GimmickWork *work;
    FieldExpObjSystem *system;
    SRTMatrix *matrix;

    // The game data is fetched and never used
    gameData = GSYS_GetGameData(gsys);
    field = GSYS_GetField(gsys);
    work = Field_GetGimmickWorkBlock(field, 1);
    system = Field_GetExpObjSystem(field);
    matrix = FieldExpObj_GetActorMatrixPtr(system, 0, 0);
    matrix->translation.x = x * GRID_SIZE;
    matrix->translation.y = y * GRID_SIZE;
    matrix->translation.z = z * GRID_SIZE;
    FieldExpObj_SetActorHidden(system, 0, 0, FALSE);
    GimmickHumilau_PlayActor(work->system, 0);
    GFL_SndSEPlay(SEQ_SE_SW_SHIZUI);
}

GameEvent *GimmickHumilau_CreateJumpEvent(GameSystem *gsys, u16 actorId, u16 x, u16 z, u16 a4) {
    GameData *gameData;
    FieldActor *actor;
    GameEvent *event;
    JumpWork *work;
    fx32 height;
    VecFx32 start;
    VecFx32 end;

    // The game data is fetched and never used
    gameData = GSYS_GetGameData(gsys);
    actor = FindFieldActor(Field_GetActorSystem(GSYS_GetField(gsys)), actorId);
    if (actor == NULL) {
        return NULL;
    }
    start.x = 791 * GRID_SIZE + GRID_SIZE / 2;
    start.y = -5 * GRID_SIZE;
    start.z = 151 * GRID_SIZE + GRID_SIZE / 2;
    SetActorWPosValue(actor, &start);

    height = 0;
    end.x = x * GRID_SIZE + GRID_SIZE / 2;
    end.y = FX32_CONST(250);
    end.z = z * GRID_SIZE + GRID_SIZE / 2;
    GetHeightFromMap(actor, &end, &height);
    end.y = height;

    event = GameEvent_Create(gsys, NULL, GimmickHumilau_JumpEvent, sizeof(JumpWork));
    work = GameEvent_GetData(event);
    work->actor = actor;
    work->frame = 0;
    work->start = start;
    work->end = end;
    VEC_Subtract(&end, &start, &work->distance);
    return event;
}

static void GimmickHumilau_LoadSplash(GimmickWork *work) {
    u32 i;
    FieldExpObjAnm *anm;
    SRTMatrix *matrix;

    LoadFieldExpandObjData(work->system, &sSceneSetup, 0);
    matrix = FieldExpObj_GetActorMatrixPtr(work->system, 0, 0);
    matrix->translation.x = 0;
    matrix->translation.y = 0;
    matrix->translation.z = 0;
    func_ov036_021b8248(work->system, 0, 0, 1);
    FieldExpObj_SetActorHidden(work->system, 0, 0, TRUE);
    for (i = 0; i < NELEMS(sAnimations); i++) {
        anm = FieldExpObj_GetAnmInfo(work->system, 0, 0, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(work->system, 0, 0, i, FALSE);
    }
}

static void GimmickHumilau_FreeScene(GimmickWork *work) {
    FieldExpObj_FreeScene(work->system, 0);
}

static void GimmickHumilau_StepAnimations(GimmickWork *work) {
    FieldExpObj_StepAllAnimations(work->system);
}

// Shows an actor and plays its animations once from the start
static void GimmickHumilau_PlayActor(FieldExpObjSystem *system, u16 actor) {
    u32 i;
    FieldExpObjAnm *anm;

    FieldExpObj_SetActorHidden(system, 0, actor, FALSE);
    for (i = 0; i < NELEMS(sAnimations); i++) {
        anm = FieldExpObj_GetAnmInfo(system, 0, actor, i);
        FieldExpObj_SetAnm(system, 0, actor, i, TRUE);
        FieldExpObj_SetAnmFrame(system, 0, actor, i, 0);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}

// Moves the actor along the jump, then lowers it to the ground
static GameEventReturnCode GimmickHumilau_JumpEvent(GameEvent *event, u32 *state, void *data) {
    JumpWork *work = data;
    VecFx32 pos;

    switch (*state) {
    case 0:
        pos = work->start;
        pos.y += sJumpHeights[work->frame];
        pos.x += FX_Div(FX_Mul(work->distance.x, work->frame * FX32_ONE), FX32_CONST(22));
        pos.z += FX_Div(FX_Mul(work->distance.z, work->frame * FX32_ONE), FX32_CONST(22));
        SetActorWPosValue(work->actor, &pos);
        work->frame++;
        if (work->frame > NELEMS(sJumpHeights) - 1) {
            (*state)++;
        }
        break;
    case 1:
        CopyActorWPos(work->actor, &pos);
        if (pos.y > work->end.y) {
            pos.y -= FX32_ONE;
            SetActorWPosValue(work->actor, &pos);
        } else {
            pos.y = work->end.y;
            pos.x = work->end.x;
            pos.z = work->end.z;
            SetActorWPosAll(work->actor, &pos, GetActorFaceDir(work->actor));
            GFL_SndSEPlay(SEQ_SE_FLD_17);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}
