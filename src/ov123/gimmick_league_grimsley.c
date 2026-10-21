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

static void GrimsleyRoom_Init(GrimsleyRoomWork *work);
static void GrimsleyRoom_Free(GrimsleyRoomWork *work);
static void GrimsleyRoom_StepAnimations(GrimsleyRoomWork *work);
static void GrimsleyRoom_ShowSavedState(GrimsleyRoomWork *work);
static void GrimsleyRoom_StartGroup(GrimsleyRoomWork *work, GroupPlayback *playback, u16 group);
static void GrimsleyRoom_UpdatePlayback(GroupPlayback *playback);
static BOOL GrimsleyRoom_IsGroupFinished(GroupPlayback *playback);
static void GrimsleyRoom_ShowGroup(GroupPlayback *playback, int group, u8 mode);
static void GrimsleyRoom_PlayModel0Animation(GrimsleyRoomWork *work, u16 animation);
static void GrimsleyRoom_SetGroup0Played(GrimsleyRoomState *state, BOOL played);
static BOOL GrimsleyRoom_GetGroup0Played(GrimsleyRoomState *state);
static void GrimsleyRoom_SetGroup1Played(GrimsleyRoomState *state, BOOL played);
static BOOL GrimsleyRoom_GetGroup1Played(GrimsleyRoomState *state);

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

void GimmickGrimsley_Setup(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    GrimsleyRoomWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(GrimsleyRoomWork));

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    work->state = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_STATE_ID);
    work->playback.expObj = Field_GetExpObjSystem(field);
    work->playback.playing = FALSE;
    work->playback.group = 0;
    GrimsleyRoom_Init(work);
    GrimsleyRoom_ShowSavedState(work);
}

void GimmickGrimsley_End(Field *field) {
    GrimsleyRoom_Free(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void GimmickGrimsley_Update(Field *field) {
    GrimsleyRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    GrimsleyRoom_StepAnimations(work);
    GrimsleyRoom_UpdatePlayback(&work->playback);
}

void GimmickGrimsley_PlayGroup(GameSystem *gsys, int group) {
    GrimsleyRoomWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    BOOL played = TRUE;

    GrimsleyRoom_StartGroup(work, &work->playback, group);
    if (group == 0) {
        GrimsleyRoom_SetGroup0Played(work->state, played);
    } else {
        GrimsleyRoom_SetGroup1Played(work->state, played);
    }
}

void GimmickGrimsley_PlayModel0Animation(GameSystem *gsys, int animation) {
    GrimsleyRoom_PlayModel0Animation(Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID), animation);
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
        FieldExpObj_SetActorCulling(work->expObj, 0, model, 1);
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
static void GrimsleyRoom_Init(GrimsleyRoomWork *work) {
    u8 i;
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;

    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, MODEL_0);
    matrix->translation.x = FX32_CONST(256);
    matrix->translation.y = 0;
    matrix->translation.z = FX32_CONST(256);
    FieldExpObj_SetActorCulling(work->expObj, 0, MODEL_0, 1);
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, FALSE);
    for (i = 0; i < NELEMS(sModel0Animations); i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_0, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(work->expObj, 0, MODEL_0, i, FALSE);
    }
    GrimsleyRoom_InitGroupModels(work, 0);
}

static void GrimsleyRoom_Free(GrimsleyRoomWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void GrimsleyRoom_StepAnimations(GrimsleyRoomWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Shows the groups that the save says have played, as they were left
static void GrimsleyRoom_ShowSavedState(GrimsleyRoomWork *work) {
    BOOL group0Played = GrimsleyRoom_GetGroup0Played(work->state);
    BOOL group1Played = GrimsleyRoom_GetGroup1Played(work->state);
    FieldExpObjAnm *anm;

    if (group0Played == TRUE) {
        GrimsleyRoom_ShowGroup(&work->playback, 0, 1);
    }
    if (group1Played == TRUE) {
        GrimsleyRoom_ShowGroup(&work->playback, 1, 1);
    }
    if (group0Played == TRUE && group1Played == FALSE) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_0, 0);
        FieldExpObj_SetAnm(work->expObj, 0, MODEL_0, 0, TRUE);
        FieldExpObj_SetAnmFrame(work->expObj, 0, MODEL_0, 0, FieldExpObjAnm_GetLastFrame(anm));
    }
}

// Shows the group and has it play
static void GrimsleyRoom_StartGroup(GrimsleyRoomWork *work, GroupPlayback *playback, u16 group) {
    FieldExpObjSystem *expObj = work->expObj;

    GrimsleyRoom_ShowGroup(playback, group, 0);
    playback->expObj = expObj;
    playback->playing = TRUE;
    playback->group = group;
}

// Once the group has played, has it shown as it ends
static void GrimsleyRoom_UpdatePlayback(GroupPlayback *playback) {
    if (playback->playing != FALSE) {
        if (GrimsleyRoom_IsGroupFinished(playback)) {
            GrimsleyRoom_ShowGroup(playback, playback->group, 1);
            playback->playing = FALSE;
        }
    }
}

// The last model of each group
static const int sGroupLastModels[2] = { 6, 4 };
// The first model of each group
static const int sGroupFirstModels[2] = { 5, 1 };

// Whether every animation of the group's models has finished
static BOOL GrimsleyRoom_IsGroupFinished(GroupPlayback *playback) {
    int lastModel = sGroupLastModels[playback->group];
    u8 model;
    u8 anim;
    FieldExpObjAnm *anm;

    for (model = sGroupFirstModels[playback->group]; model <= lastModel; model++) {
        for (anim = 0; anim <= 1; anim++) {
            anm = FieldExpObj_GetAnmInfo(playback->expObj, 0, model, anim);
            FieldExpObj_GetAnmFrame(playback->expObj, 0, model, anim);
            if (!FieldExpObjAnm_IsPlaybackFinished(anm)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// Shows the group's models with their animations stopped and, if mode is 0, the first two playing once, and
// otherwise the last two looping
static void GrimsleyRoom_ShowGroup(GroupPlayback *playback, int group, u8 mode) {
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
static void GrimsleyRoom_PlayModel0Animation(GrimsleyRoomWork *work, u16 animation) {
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

static void GrimsleyRoom_SetGroup0Played(GrimsleyRoomState *state, BOOL played) {
    state->group0Played = played;
}

static BOOL GrimsleyRoom_GetGroup0Played(GrimsleyRoomState *state) {
    return state->group0Played;
}

static void GrimsleyRoom_SetGroup1Played(GrimsleyRoomState *state, BOOL played) {
    state->group1Played = played;
}

static BOOL GrimsleyRoom_GetGroup1Played(GrimsleyRoomState *state) {
    return state->group1Played;
}
