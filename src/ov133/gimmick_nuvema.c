// The gimmick of Nuvema Town (zones 389, 397 and 317 load it), a model standing in Nuvema Town. The name is a guess:
// the overlay embeds no file name
#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/field_map.h"
#include "field/gimmick_nuvema.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/rtc.h"

typedef struct {
    BOOL playing;
    BOOL night;
} GimmickWork;

static GameEventReturnCode func_ov133_021eee90(GameEvent *event, u32 *state, void *data);

static const G3DSceneAnimationSetup sAnimations[] = { { 1, 0 }, { 2, 0 } };

static const G3DSceneResourceSetup sDayResources[] = { { 0xe0, 0, 0 }, { 0xe0, 1, 0 }, { 0xe0, 2, 0 } };

static const G3DSceneActorSetup sActors[] = {
    { 0, 0, 0, 0, sAnimations, NELEMS(sAnimations) },
};

static const G3DSceneSetup sDaySceneSetup = { sDayResources, NELEMS(sDayResources), sActors, NELEMS(sActors) };

static const G3DSceneResourceSetup sNightResources[] = { { 0xe0, 3, 0 }, { 0xe0, 4, 0 }, { 0xe0, 5, 0 } };

static const G3DSceneSetup sNightSceneSetup = { sNightResources, NELEMS(sNightResources), sActors, NELEMS(sActors) };

void func_ov133_021eec80(Field *field) {
    u32 period;
    FieldExpObjAnm *anm;
    s32 i;
    GimmickWork *work;
    FieldExpObjSystem *system;
    u32 *state;
    GameData *gameData;

    gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    state = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_NUVEMA);
    system = Field_GetExpObjSystem(field);
    Field_AllocGimmickWorkBlock(field, 1, Field_GetHeapID(field), sizeof(GimmickWork));
    work = Field_GetGimmickWorkBlock(field, 1);
    work->playing = FALSE;
    period = GetRealTimeDayPeriod(GameData_GetSeason(gameData));
    if (period == 3 || period == 4) {
        LoadFieldExpandObjData(system, &sNightSceneSetup, 0);
        work->night = TRUE;
    } else {
        LoadFieldExpandObjData(system, &sDaySceneSetup, 0);
        work->night = FALSE;
    }
    {
        // Tile (784, 752), in Nuvema Town
        VecFx32 position = { FX32_CONST(784 * 16), 0, FX32_CONST(752 * 16) };

        FieldExpObj_GetActorMatrixPtr(system, 0, 0)->translation = position;
    }
    FieldExpObj_SetActorCulling(system, 0, 0, 1);
    FieldExpObj_SetActorHidden(system, 0, 0, *state ? FALSE : TRUE);
    for (i = 0; i < 2; i++) {
        anm = FieldExpObj_GetAnmInfo(system, 0, 0, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(system, 0, 0, i, *state ? TRUE : FALSE);
    }
}

void func_ov133_021eed84(Field *field) {
    FieldExpObj_FreeScene(Field_GetExpObjSystem(field), 0);
    Field_DeleteGimmickWorkBlock(field, 1);
}

void func_ov133_021eed9c(Field *field) {
    FieldExpObjSystem *system;
    GimmickWork *work;
    u32 *state;

    system = Field_GetExpObjSystem(field);
    work = Field_GetGimmickWorkBlock(field, 1);
    if (work->playing) {
        FieldExpObj_StepAllAnimations(system);
        if (!work->night && FieldExpObj_GetAnmFrame(system, 0, 0, 0) == FX32_CONST(60)) {
            GFL_SndSEPlay(SEQ_SE_OPEN_03);
        }
        if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(system, 0, 0, 0))) {
            FieldExpObj_SetActorHidden(system, 0, 0, TRUE);
            work->playing = FALSE;
            state = GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))),
                                             GIMMICK_NUVEMA);
            *state = FALSE;
        }
    }
}

void func_ov133_021eee1c(Field *field) {
    s32 i;
    GimmickWork *work;
    FieldExpObjSystem *system;

    system = Field_GetExpObjSystem(field);
    work = Field_GetGimmickWorkBlock(field, 1);
    work->playing = TRUE;
    FieldExpObj_SetActorHidden(system, 0, 0, FALSE);
    for (i = 0; i < 2; i++) {
        FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(system, 0, 0, i), FALSE);
    }
    if (work->night) {
        GFL_SndSEPlay(SEQ_SE_OPEN_01);
    } else {
        GFL_SndSEPlay(SEQ_SE_OPEN_02);
    }
}

GameEvent *func_ov133_021eee7c(GameSystem *gsys) {
    return GameEvent_Create(gsys, NULL, func_ov133_021eee90, 0);
}

static GameEventReturnCode func_ov133_021eee90(GameEvent *event, u32 *state, void *data) {
    GimmickWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(GameEvent_GetGameSystem(event)), 1);

    if (work->playing) {
        return GAMEEVENT_CONTINUE;
    }
    return GAMEEVENT_DONE;
}
