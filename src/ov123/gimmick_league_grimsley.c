#include "types.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/field_map.h"
#include "field/gimmick_league_grimsley.h"
#include "gfl/g3d.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_system.h"

#define GIMMICK_WORK_ID 1
#define GIMMICK_STATE_ID 40
// The archive of the room's models, a/2/7/6
#define GIMMICK_ARCID 0x116

#define MODEL_0 0
// The models after the first, in two groups: the first group has the last two, the second the four before
#define GROUP_MODEL_COUNT 6

// Whether each of the two groups has played, which the save keeps
typedef struct {
    BOOL group0Played;
    BOOL group1Played;
} GrimsleyRoomState;

// The group that has been shown and plays, until it is done
typedef struct {
    FieldExpObjSystem *expObj;
    BOOL playing;
    u16 group;
} GroupPlayback;

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    GrimsleyRoomState *state;
    GroupPlayback playback;
} GrimsleyRoomWork;

static void func_ov123_021eed54(GrimsleyRoomWork *work);
static void func_ov123_021eee70(GrimsleyRoomWork *work);
static void func_ov123_021eee7c(GrimsleyRoomWork *work);
static void func_ov123_021eee88(GrimsleyRoomWork *work);
static void func_ov123_021eeefc(GrimsleyRoomWork *work, GroupPlayback *playback, u16 group);
static void func_ov123_021eef18(GroupPlayback *playback);
static BOOL func_ov123_021eef3c(GroupPlayback *playback);
static void func_ov123_021eefa4(GroupPlayback *playback, int group, u8 mode);
static void func_ov123_021ef0d8(GrimsleyRoomWork *work, u16 animation);
static void func_ov123_021ef144(GrimsleyRoomState *state, BOOL played);
static BOOL func_ov123_021ef148(GrimsleyRoomState *state);
static void func_ov123_021ef14c(GrimsleyRoomState *state, BOOL played);
static BOOL func_ov123_021ef150(GrimsleyRoomState *state);

static const G3DSceneResourceSetup sResources[8] = {
    { GIMMICK_ARCID, 0, 0 }, { GIMMICK_ARCID, 1, 0 }, { GIMMICK_ARCID, 2, 0 }, { GIMMICK_ARCID, 3, 0 },
    { GIMMICK_ARCID, 4, 0 }, { GIMMICK_ARCID, 5, 0 }, { GIMMICK_ARCID, 6, 0 }, { GIMMICK_ARCID, 7, 0 },
};

static const G3DSceneAnimationSetup sModel0Animations[] = { { 1, 0 }, { 2, 0 } };
// The models of the groups: two animations that play once, and two that loop
static const G3DSceneAnimationSetup sGroupModelAnimations[] = { { 4, 0 }, { 5, 0 }, { 6, 0 }, { 7, 0 } };

static const G3DSceneActorSetup sActors[7] = {
    { 0, 0, 0, 0, sModel0Animations, NELEMS(sModel0Animations) },
    { 3, 0, 3, 0, sGroupModelAnimations, NELEMS(sGroupModelAnimations) },
    { 3, 0, 3, 0, sGroupModelAnimations, NELEMS(sGroupModelAnimations) },
    { 3, 0, 3, 0, sGroupModelAnimations, NELEMS(sGroupModelAnimations) },
    { 3, 0, 3, 0, sGroupModelAnimations, NELEMS(sGroupModelAnimations) },
    { 3, 0, 3, 0, sGroupModelAnimations, NELEMS(sGroupModelAnimations) },
    { 3, 0, 3, 0, sGroupModelAnimations, NELEMS(sGroupModelAnimations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void func_ov123_021eec80(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    GrimsleyRoomWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(GrimsleyRoomWork));

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    work->state = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_STATE_ID);
    work->playback.expObj = Field_GetExpObjSystem(field);
    work->playback.playing = FALSE;
    work->playback.group = 0;
    func_ov123_021eed54(work);
    func_ov123_021eee88(work);
}

void func_ov123_021eecd8(Field *field) {
    func_ov123_021eee70(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void func_ov123_021eecf0(Field *field) {
    GrimsleyRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    func_ov123_021eee7c(work);
    func_ov123_021eef18(&work->playback);
}

void func_ov123_021eed08(GameSystem *gsys, int group) {
    GrimsleyRoomWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    BOOL played = TRUE;

    func_ov123_021eeefc(work, &work->playback, group);
    if (group == 0) {
        func_ov123_021ef144(work->state, played);
    } else {
        func_ov123_021ef14c(work->state, played);
    }
}

void func_ov123_021eed3c(GameSystem *gsys, int animation) {
    func_ov123_021ef0d8(Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID), animation);
}

// Places the models, hidden
// Places the group models from the index-th on, hidden, with their animations set and paused
static inline void GrimsleyRoom_InitGroupModels(GrimsleyRoomWork *work, u8 index) {
    const VecFx32 positions[GROUP_MODEL_COUNT] = {
        { FX32_CONST(536), FX32_CONST(224), FX32_CONST(416) }, { FX32_CONST(472), FX32_CONST(224), FX32_CONST(416) },
        { FX32_CONST(568), FX32_CONST(224), FX32_CONST(336) }, { FX32_CONST(440), FX32_CONST(224), FX32_CONST(336) },
        { FX32_CONST(546), FX32_CONST(28), FX32_CONST(720) },  { FX32_CONST(462), FX32_CONST(28), FX32_CONST(720) },
    };
    u16 model;
    u16 anim;
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;

    for (; index < GROUP_MODEL_COUNT; index++) {
        model = index + 1;
        matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, model);
        matrix->translation.x = positions[index].x;
        matrix->translation.y = positions[index].y;
        matrix->translation.z = positions[index].z;
        func_ov036_021b8248(work->expObj, 0, model, 1);
        FieldExpObj_SetActorHidden(work->expObj, 0, model, TRUE);
        for (anim = 0; anim < NELEMS(sGroupModelAnimations); anim++) {
            anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, anim);
            FieldExpObjAnm_SetLooped(anm, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
            FieldExpObj_SetAnm(work->expObj, 0, model, anim, FALSE);
        }
    }
}

// Places the models, hidden
static void func_ov123_021eed54(GrimsleyRoomWork *work) {
    u8 i;
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;

    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, MODEL_0);
    matrix->translation.x = FX32_CONST(256);
    matrix->translation.y = 0;
    matrix->translation.z = FX32_CONST(256);
    func_ov036_021b8248(work->expObj, 0, MODEL_0, 1);
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, FALSE);
    for (i = 0; i < NELEMS(sModel0Animations); i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_0, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(work->expObj, 0, MODEL_0, i, FALSE);
    }
    GrimsleyRoom_InitGroupModels(work, 0);
}

static void func_ov123_021eee70(GrimsleyRoomWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void func_ov123_021eee7c(GrimsleyRoomWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Shows the groups that the save says have played, as they were left
static void func_ov123_021eee88(GrimsleyRoomWork *work) {
    BOOL group0Played = func_ov123_021ef148(work->state);
    BOOL group1Played = func_ov123_021ef150(work->state);
    FieldExpObjAnm *anm;

    if (group0Played == TRUE) {
        func_ov123_021eefa4(&work->playback, 0, 1);
    }
    if (group1Played == TRUE) {
        func_ov123_021eefa4(&work->playback, 1, 1);
    }
    if (group0Played == TRUE && group1Played == FALSE) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_0, 0);
        FieldExpObj_SetAnm(work->expObj, 0, MODEL_0, 0, TRUE);
        FieldExpObj_SetAnmFrame(work->expObj, 0, MODEL_0, 0, func_ov036_021b8580(anm));
    }
}

// Shows the group and has it play
static void func_ov123_021eeefc(GrimsleyRoomWork *work, GroupPlayback *playback, u16 group) {
    FieldExpObjSystem *expObj = work->expObj;

    func_ov123_021eefa4(playback, group, 0);
    playback->expObj = expObj;
    playback->playing = TRUE;
    playback->group = group;
}

// Once the group has played, has it shown as it ends
static void func_ov123_021eef18(GroupPlayback *playback) {
    if (playback->playing != FALSE) {
        if (func_ov123_021eef3c(playback)) {
            func_ov123_021eefa4(playback, playback->group, 1);
            playback->playing = FALSE;
        }
    }
}

// The last model of each group
static const int sGroupLastModels[2] = { 6, 4 };
// The first model of each group
static const int sGroupFirstModels[2] = { 5, 1 };

// Whether every animation of the group's models has finished
static BOOL func_ov123_021eef3c(GroupPlayback *playback) {
    int lastModel = sGroupLastModels[playback->group];
    u8 model;
    u8 anim;
    FieldExpObjAnm *anm;

    for (model = sGroupFirstModels[playback->group]; model <= lastModel; model++) {
        for (anim = 0; anim <= 1; anim++) {
            anm = FieldExpObj_GetAnmInfo(playback->expObj, 0, model, anim);
            func_ov036_021b8520(playback->expObj, 0, model, anim);
            if (!FieldExpObjAnm_IsPlaybackFinished(anm)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// Shows the group's models with their animations stopped and, if mode is 0, the first two playing once, and
// otherwise the last two looping
static void func_ov123_021eefa4(GroupPlayback *playback, int group, u8 mode) {
    FieldExpObjSystem *expObj = playback->expObj;
    int firstModel = sGroupFirstModels[group];
    int lastModel = sGroupLastModels[group];
    u8 model;
    u8 anim;
    FieldExpObjAnm *anm;

    for (model = firstModel; model <= lastModel; model++) {
        FieldExpObj_SetActorHidden(expObj, 0, model, FALSE);
        for (anim = 0; anim < 4; anim++) {
            anm = FieldExpObj_GetAnmInfo(expObj, 0, model, anim);
            FieldExpObjAnm_SetPaused(anm, TRUE);
            FieldExpObj_SetAnm(expObj, 0, model, anim, FALSE);
            FieldExpObj_SetAnmFrame(expObj, 0, model, anim, 0);
        }
    }
    for (model = firstModel; model <= lastModel; model++) {
        if (mode == 0) {
            for (anim = 0; anim <= 1; anim++) {
                anm = FieldExpObj_GetAnmInfo(expObj, 0, model, anim);
                FieldExpObjAnm_SetPaused(anm, FALSE);
                FieldExpObjAnm_SetLooped(anm, FALSE);
                FieldExpObj_SetAnm(expObj, 0, model, anim, TRUE);
                FieldExpObj_SetAnmFrame(expObj, 0, model, anim, 0);
            }
        } else {
            for (anim = 2; anim <= 3; anim++) {
                anm = FieldExpObj_GetAnmInfo(expObj, 0, model, anim);
                FieldExpObjAnm_SetPaused(anm, FALSE);
                FieldExpObjAnm_SetLooped(anm, TRUE);
                FieldExpObj_SetAnm(expObj, 0, model, anim, TRUE);
                FieldExpObj_SetAnmFrame(expObj, 0, model, anim, 0);
            }
        }
    }
}

// Plays one of model 0's animations and pauses the other
static void func_ov123_021ef0d8(GrimsleyRoomWork *work, u16 animation) {
    u32 i;
    FieldExpObjAnm *anm;

    for (i = 0; i < 2; i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_0, i);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(work->expObj, 0, MODEL_0, i, FALSE);
    }
    anm = FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_0, animation);
    FieldExpObjAnm_SetPaused(anm, FALSE);
    FieldExpObj_SetAnm(work->expObj, 0, MODEL_0, animation, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, MODEL_0, animation, 0);
}

static void func_ov123_021ef144(GrimsleyRoomState *state, BOOL played) {
    state->group0Played = played;
}

static BOOL func_ov123_021ef148(GrimsleyRoomState *state) {
    return state->group0Played;
}

static void func_ov123_021ef14c(GrimsleyRoomState *state, BOOL played) {
    state->group1Played = played;
}

static BOOL func_ov123_021ef150(GrimsleyRoomState *state) {
    return state->group1Played;
}
