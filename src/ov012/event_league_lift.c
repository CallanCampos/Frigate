// The Pokémon League's lift down to the Champion's room: the camera turns, the lift goes down with the player, the
// zone changes, and the lift lands with a shake. The name is descriptive; EventLeagueLiftWarp_Callback and
// EventLeagueLiftWarp_Create are swan's names (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/event_league_lift.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_event.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_league_lift.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_event.h"

#define SEQ_SE_LIFT 0x7a0
#define SEQ_SE_LIFT_STOP 0x6ef
// The Champion's room
#define ZONE_LEAGUE_CHAMPION 0x8a

typedef struct {
    GameSystem *gsys;
    Field *field;
    FieldCamera *camera;
    FieldTaskManager *taskManager;
    G3DCurve *curve;
    FieldCameraAnimation animation;
    FieldCameraAnimationController *cameraController;
} LeagueLiftWarpData;

typedef struct {
    u16 heapId;
    GameSystem *gsys;
    Field *field;
    FieldCamera *camera;
    G3DCurve *curve;
} LeagueLiftArrivalData;

static const FieldCameraCoords data_ov012_0216dc3c = {
    0x1d7d, 0, 0x16c000, {0x1f8000, -0x40000, 0x264000}, {0, 0x9d000, 0x90000},
};

static GameEvent *func_ov012_02165e80(GameSystem *gsys, Field *field);

static GameEventReturnCode EventLeagueLiftWarp_Callback(GameEvent *event, u32 *state, void *work) {
    LeagueLiftWarpData *data = work;
    GameSystem *gsys = data->gsys;
    Field *field = data->field;
    FieldTaskManager *taskManager = data->taskManager;
    VecFx32 translation;
    VecFx32 pos;
    FieldPlayer *player;
    VecFx32 spawnPos;

    switch (*state) {
    case 0:
        FieldCameraAnimationController_PrepareCamera(data->cameraController);
        FieldCamera_GetAnimationCoords(data->camera, &data->animation.src);
        FieldCameraAnimationController_StartAnimation(data->cameraController);
        (*state)++;
        break;
    case 1:
        if (FieldTaskManager_IsIdle(taskManager)) {
            FieldCameraAnimationController_FreeAnimation(data->cameraController);
            (*state)++;
        }
        break;
    case 2:
        data->curve = GFL_G3DCurveLoadFileAll(Field_GetHeapID(field), 0x9b, 0x18);
        GFL_SEPlayKeepVol(SEQ_SE_LIFT, GFL_SndSeqGetPlayerIndex(SEQ_SE_LIFT));
        (*state)++;
        break;
    case 3:
        GFL_G3DCurveGetNowTranslation(data->curve, &translation);
        FieldExpObj_GetActorMatrixPtr(Field_GetExpObjSystem(field), 0, 0)->translation.y = translation.y;
        player = Field_GetPlayer(field);
        FieldPlayer_GetWPos(player, &pos);
        pos.y = translation.y;
        FieldPlayer_SetWPos(player, &pos);
        if (GFL_G3DCurveFrameStepLoop(data->curve, FX32_ONE)) {
            GFL_G3DCurveFree(data->curve);
            (*state)++;
        }
        break;
    case 4:
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, field, 0, 0));
        (*state)++;
        break;
    case 5:
        spawnPos.x = 0;
        spawnPos.y = 0;
        spawnPos.z = 0;
        GameEvent_ChainNext(event, EventMapChange_CreateGridDefault(gsys, field, ZONE_LEAGUE_CHAMPION, &spawnPos, 1));
        (*state)++;
        break;
    case 6:
        GameEvent_Replace(event, func_ov012_02165e80(gsys, field));
        (*state)++;
        break;
    case 7:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventLeagueLiftWarp_Create(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventLeagueLiftWarp_Callback, sizeof(LeagueLiftWarpData));
    LeagueLiftWarpData *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->field = field;
    data->curve = NULL;
    data->camera = Field_GetCameraSystem(field);
    data->taskManager = Field_GetTaskManager(field);
    data->animation.frames = 20;
    data->animation.dst = data_ov012_0216dc3c;
    data->animation.unk48 = TRUE;
    data->animation.unk4C = TRUE;
    data->cameraController = Field_CreateCameraAnimationController(field);
    FieldCameraAnimationController_SetAnimation(data->cameraController, &data->animation);
    return event;
}

static BOOL func_ov012_02165ca4(LeagueLiftArrivalData *data) {
    return GFL_G3DCurveFrameStepLoop(data->curve, FX32_ONE);
}

// Moves the lift to the curve
static void func_ov012_02165cb4(LeagueLiftArrivalData *data) {
    VecFx32 translation;

    GFL_G3DCurveGetNowTranslation(data->curve, &translation);
    FieldExpObj_GetActorMatrixPtr(Field_GetExpObjSystem(data->field), 0, 0)->translation.y = translation.y;
}

// Puts the player on the lift, in the middle of its square
static void func_ov012_02165cd8(LeagueLiftArrivalData *data) {
    SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(Field_GetExpObjSystem(data->field), 0, 0);
    FieldPlayer *player = Field_GetPlayer(data->field);
    VecFx32 pos;

    VEC_Set(&pos, matrix->translation.x, matrix->translation.y, matrix->translation.z + FX32_CONST(32));
    pos.x = FX_Whole(pos.x) / 16 * FX32_CONST(16) + FX32_CONST(8);
    pos.z = FX_Whole(pos.z) / 16 * FX32_CONST(16) + FX32_CONST(8);
    FieldPlayer_SetWPos(player, &pos);
}

static GameEventReturnCode func_ov012_02165d30(GameEvent *event, u32 *state, void *work) {
    LeagueLiftArrivalData *data = work;
    BOOL done;
    FieldEvCameraShake shake;
    FieldPlayer *player;
    FieldActor *actor;
    VecFx32 pos;
    fx32 height;

    Field_GetExpObjSystem(data->field);
    switch (*state) {
    case 0:
        data->curve = GFL_G3DCurveLoadFileAll(data->heapId, 0x9b, 0x19);
        func_ov012_02165cb4(data);
        func_ov012_02165cd8(data);
        FieldCamera_FinishDelay(data->camera);
        (*state)++;
        break;
    case 1:
        if (!FieldCamera_IsDelayActive(data->camera)) {
            (*state)++;
        }
        break;
    case 2:
        func_ov106_021eecd4(data->field);
        func_ov106_021eed78(data->field);
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(data->gsys, data->field, 0, 0, 1, 0, 0));
        (*state)++;
        break;
    case 3:
        done = func_ov012_02165ca4(data);
        if (!done) {
            func_ov012_02165cb4(data);
            func_ov012_02165cd8(data);
        }
        if (done) {
            GFL_G3DCurveFree(data->curve);
            GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_LIFT));
            GFL_SndSEPlay(SEQ_SE_LIFT_STOP);
            shake.unk00 = 2;
            shake.unk02 = 0;
            shake.unk14 = 3;
            shake.unk04 = 2;
            shake.unk10 = 2;
            shake.unk18 = 0;
            shake.unk06 = 0;
            shake.unk08 = 1;
            shake.unk0A = 0;
            shake.unk0C = 0;
            shake.unk0E = 2;
            GameEvent_ChainNext(event, EventEvCameraShake_Create(data->gsys, &shake));
            (*state)++;
        }
        break;
    case 4:
        if (func_ov106_021eed18(data->field) && func_ov106_021eedc8(data->field)) {
            func_ov106_021eed04(data->field);
            func_ov106_021eed48(data->field);
            (*state)++;
        }
        break;
    case 5:
        player = Field_GetPlayer(data->field);
        actor = FieldPlayer_GetActor(player);
        FieldPlayer_GetWPos(player, &pos);
        GetHeightFromMap(actor, &pos, &height);
        pos.y = height;
        FieldPlayer_SetWPos(player, &pos);
        (*state)++;
        break;
    case 6:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *func_ov012_02165e80(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov012_02165d30, sizeof(LeagueLiftArrivalData));
    LeagueLiftArrivalData *data = GameEvent_GetData(event);

    data->heapId = Field_GetHeapID(field);
    data->gsys = gsys;
    data->field = field;
    data->camera = Field_GetCameraSystem(field);
    data->curve = NULL;
    return event;
}
