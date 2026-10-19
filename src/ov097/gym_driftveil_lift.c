#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/gym_driftveil_lift.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

#define POS_TO_GRID(pos) ((s16)(((pos) >> 4) / FX32_ONE))

// The height of the lift's lower stop; the upper one is 0
#define LIFT_BOTTOM_Y FX32_CONST(-80)
// How far the lift moves a frame
#define LIFT_SPEED FX32_CONST(2)

// The work var that says which lift is in use: 1 for the second one
#define WORK_LIFT 0x4000

typedef struct {
    fx32 y;
    fx32 targetY;
    fx32 speed;
    BOOL down;
    // The camera's binding, taken off while the lift moves
    void *cameraBind;
    u32 lift;
    // The actor riding along on the second lift, if any
    FieldActor *rider;
} LiftWork;

static GameEventReturnCode DriftveilLift_MoveEvent(GameEvent *event, u32 *state, void *data);
static BOOL DriftveilLift_HasArrived(LiftWork *work);
static void DriftveilLift_ResetAnm(FieldExpObjSystem *system, u32 lift);

static const G3DSceneAnimationSetup sAnimations03[] = { { 3, 0 } };
static const G3DSceneAnimationSetup sAnimations01[] = { { 1, 0 } };

static const G3DSceneActorSetup sActors[2] = {
    { 0, 0, 0, 0, sAnimations01, NELEMS(sAnimations01) },
    { 2, 0, 2, 0, sAnimations03, NELEMS(sAnimations03) },
};

static const G3DSceneResourceSetup sResources[] = {
    { 0x8a, 5, 0 },
    { 0x8a, 6, 0 },
    { 0x8a, 11, 0 },
    { 0x8a, 12, 0 },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void DriftveilLift_Init(Field *field) {
    // Where each lift is
    VecFx32 lift0Position = { FX32_CONST(120), 0, FX32_CONST(72) };
    VecFx32 lift1Position = { FX32_CONST(120), 0, FX32_CONST(72) };
    FieldExpObjSystem *system;
    FieldExpObjAnm *anm;

    system = Field_GetExpObjSystem(field);
    Field_AllocGimmickWorkBlock(field, 1, Field_GetHeapID(field), sizeof(LiftWork));
    LoadFieldExpandObjData(system, &sSceneSetup, 0);

    FieldExpObj_GetActorMatrixPtr(system, 0, 0)->translation = lift0Position;
    FieldExpObj_SetActorCulling(system, 0, 0, 1);
    anm = FieldExpObj_GetAnmInfo(system, 0, 0, 0);
    FieldExpObj_SetAnm(system, 0, 0, 0, TRUE);
    FieldExpObjAnm_SetLooped(anm, TRUE);
    FieldExpObjAnm_SetPaused(anm, TRUE);
    DriftveilLift_ResetAnm(system, 0);

    FieldExpObj_GetActorMatrixPtr(system, 0, 1)->translation = lift1Position;
    FieldExpObj_SetActorCulling(system, 0, 1, 1);
    FieldExpObj_SetActorHidden(system, 0, 1, TRUE);
    anm = FieldExpObj_GetAnmInfo(system, 0, 1, 0);
    FieldExpObj_SetAnm(system, 0, 1, 0, TRUE);
    FieldExpObjAnm_SetLooped(anm, TRUE);
    FieldExpObjAnm_SetPaused(anm, TRUE);
    DriftveilLift_ResetAnm(system, 1);

    if (*EventWork_GetWkPtr(GameData_GetEventWork(GSYS_GetGameData(Field_GetGameSystem(field))), WORK_LIFT) == 1) {
        FieldExpObj_SetActorHidden(system, 0, 0, TRUE);
        FieldExpObj_SetActorHidden(system, 0, 1, FALSE);
        FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(system, 0, 1, 0), FALSE);
    }
}

void DriftveilLift_End(Field *field) {
    FieldExpObjSystem *system;

    system = Field_GetExpObjSystem(field);
    Field_GetGimmickWorkBlock(field, 1);
    Field_DeleteGimmickWorkBlock(field, 1);
    FieldExpObj_FreeScene(system, 0);
}

void DriftveilLift_Update(Field *field) {
    FieldExpObj_StepAllAnimations(Field_GetExpObjSystem(field));
}

GameEvent *DriftveilLift_CreateMoveEvent(GameSystem *gsys, BOOL down, BOOL withRider) {
    Field *field;
    LiftWork *work;

    field = GSYS_GetField(gsys);
    work = Field_GetGimmickWorkBlock(field, 1);
    work->down = down;
    work->lift = withRider == TRUE;
    if (withRider == TRUE) {
        work->rider = GetFirstActorOnGPos(Field_GetActorSystem(field), 7, 4, TRUE);
    } else {
        work->rider = NULL;
    }
    if (down) {
        work->y = 0;
        work->targetY = LIFT_BOTTOM_Y;
    } else {
        work->y = LIFT_BOTTOM_Y;
        work->targetY = 0;
    }
    FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(Field_GetExpObjSystem(field), 0, work->lift, 0), FALSE);
    if (work->targetY - work->y < 0) {
        work->speed = -LIFT_SPEED;
    } else {
        work->speed = LIFT_SPEED;
    }
    return GameEvent_Create(gsys, NULL, DriftveilLift_MoveEvent, 0);
}

static GameEventReturnCode DriftveilLift_MoveEvent(GameEvent *event, u32 *state, void *data) {
    Field *field;
    FieldExpObjSystem *system;
    LiftWork *work;
    FieldCamera *camera;
    FieldPlayer *player;
    VecFx32 playerPos;
    VecFx32 riderPos;

    field = GSYS_GetField(GameEvent_GetGameSystem(event));
    system = Field_GetExpObjSystem(field);
    work = Field_GetGimmickWorkBlock(field, 1);
    camera = Field_GetCameraSystem(field);
    switch (*state) {
    case 0:
        if (work->down) {
            work->cameraBind = FieldCamera_GetBind(camera);
            FieldCamera_ClearBind(camera);
        }
        GFL_SndSEPlay(!work->down ? SEQ_SE_FLD_75 : SEQ_SE_FLD_59);
        (*state)++;
        break;
    case 1:
        work->y += work->speed;
        if (DriftveilLift_HasArrived(work)) {
            work->y = work->targetY;
            if (!work->down) {
                GFL_SndSEPlay(SEQ_SE_FLD_60);
            }
            (*state)++;
        }
        FieldExpObj_GetActorMatrixPtr(system, 0, work->lift)->translation.y = work->y;
        player = Field_GetPlayer(field);
        FieldPlayer_GetWPos(player, &playerPos);
        playerPos.y = work->y;
        FieldPlayer_SetWPos(player, &playerPos);
        if (work->rider != NULL) {
            riderPos = *GetMModelWPosPtr(work->rider);
            riderPos.y = work->y;
            SetActorGPosY(work->rider, POS_TO_GRID(riderPos.y));
            SetActorWPosValue(work->rider, &riderPos);
        }
        break;
    case 2:
        if (work->down) {
            return GAMEEVENT_DONE;
        }
        if (work->cameraBind != NULL) {
            FieldCamera_SetBind(camera, work->cameraBind);
        }
        FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(system, 0, work->lift, 0), FALSE);
        (*state)++;
        break;
    case 3:
        if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(system, 0, work->lift, 0))) {
            DriftveilLift_ResetAnm(system, work->lift);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

void DriftveilLift_PlaceAtBottom(GameSystem *gsys, u16 withRider) {
    Field *field;
    FieldExpObjSystem *system;
    LiftWork *work;
    FieldCamera *camera;
    fx32 y;
    FieldPlayer *player;
    VecFx32 playerPos;
    FieldActor *rider;
    VecFx32 riderPos;

    field = GSYS_GetField(gsys);
    system = Field_GetExpObjSystem(field);
    work = Field_GetGimmickWorkBlock(field, 1);
    camera = Field_GetCameraSystem(field);
    work->cameraBind = FieldCamera_GetBind(camera);
    FieldCamera_ClearBind(camera);
    if (withRider == TRUE) {
        FieldExpObj_SetActorHidden(system, 0, 0, TRUE);
        FieldExpObj_SetActorHidden(system, 0, 1, FALSE);
    }
    y = LIFT_BOTTOM_Y;
    FieldExpObj_GetActorMatrixPtr(system, 0, withRider == TRUE)->translation.y = y;
    player = Field_GetPlayer(field);
    FieldPlayer_GetWPos(player, &playerPos);
    playerPos.y = y;
    FieldPlayer_SetWPos(player, &playerPos);
    if (withRider == TRUE) {
        rider = GetFirstActorOnGPos(Field_GetActorSystem(field), 7, 4, TRUE);
        riderPos = *GetMModelWPosPtr(rider);
        riderPos.y = y;
        SetActorGPosY(rider, POS_TO_GRID(riderPos.y));
        SetActorWPosValue(rider, &riderPos);
    }
}

static BOOL DriftveilLift_HasArrived(LiftWork *work) {
    if (work->speed < 0) {
        if (work->targetY >= work->y) {
            return TRUE;
        }
    } else if (work->targetY <= work->y) {
        return TRUE;
    }
    return FALSE;
}

static void DriftveilLift_ResetAnm(FieldExpObjSystem *system, u32 lift) {
    FieldExpObj_SetAnm(system, 0, lift, 0, TRUE);
    FieldExpObj_SetAnmFrame(system, 0, lift, 0, 0);
    FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(system, 0, lift, 0), TRUE);
}
