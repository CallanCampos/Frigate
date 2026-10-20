#include "types.h"
#include "constants/sound.h"
#include "constants/version.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_dragonspiral_tower_9.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/version.h"

#define GIMMICK_WORK_ID 1
// The archive of the scene's models, a/2/8/2
#define GIMMICK_ARCID 0x132

// The models of the scene
enum {
    MODEL_RELEASE,
    MODEL_THROWN,
};

typedef struct {
    // The fade of the flash
    u32 fadeMode;
    // Frames to wait, with the screen white, before it fades back
    s32 fadeWait;
    // The sounds of the release that were played
    BOOL played[4];
    // The throw of the second model
    VecFx32 from;
    VecFx32 to;
    fx32 height;
    u16 frames;
    u16 frame;
} DT9GimmickWork;

static GameEventReturnCode GimmickDT9_ReleaseEvent(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode GimmickDT9_ThrowEvent(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode GimmickDT9_PastFifthFrameEvent(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode GimmickDT9_ThrownDoneEvent(GameEvent *event, u32 *state, void *data);

static const G3DSceneAnimationSetup sThrownAnimations[] = { { 6, 0 }, { 7, 0 }, { 8, 0 } };
static const G3DSceneAnimationSetup sReleaseAnimations[] = { { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 } };

// The Light Stone's model is the fifth file in White 2, the Dark Stone's in Black 2
static const G3DSceneResourceSetup sResourcesWhite2[9] = {
    { GIMMICK_ARCID, 0, 0 }, { GIMMICK_ARCID, 1, 0 }, { GIMMICK_ARCID, 2, 0 },
    { GIMMICK_ARCID, 3, 0 }, { GIMMICK_ARCID, 4, 0 }, { GIMMICK_ARCID, 6, 0 },
    { GIMMICK_ARCID, 7, 0 }, { GIMMICK_ARCID, 9, 0 }, { GIMMICK_ARCID, 8, 0 },
};

static const G3DSceneResourceSetup sResourcesBlack2[9] = {
    { GIMMICK_ARCID, 0, 0 }, { GIMMICK_ARCID, 1, 0 }, { GIMMICK_ARCID, 2, 0 },
    { GIMMICK_ARCID, 3, 0 }, { GIMMICK_ARCID, 5, 0 }, { GIMMICK_ARCID, 6, 0 },
    { GIMMICK_ARCID, 7, 0 }, { GIMMICK_ARCID, 9, 0 }, { GIMMICK_ARCID, 8, 0 },
};

static const G3DSceneActorSetup sActors[2] = {
    { 0, 0, 0, 0, sReleaseAnimations, NELEMS(sReleaseAnimations) },
    { 5, 0, 5, 0, sThrownAnimations, NELEMS(sThrownAnimations) },
};

static const G3DSceneSetup sSceneSetupBlack2 = { sResourcesBlack2, NELEMS(sResourcesBlack2), sActors, NELEMS(sActors) };
static const G3DSceneSetup sSceneSetupWhite2 = { sResourcesWhite2, NELEMS(sResourcesWhite2), sActors, NELEMS(sActors) };

void GimmickDT9_Setup(Field *field) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    DT9GimmickWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(DT9GimmickWork));
    FieldExpObjAnm *anm;
    int i;

    work->fadeWait = 0;
    if (getGameVersion() == VERSION_BLACK2) {
        LoadFieldExpandObjData(expObj, &sSceneSetupBlack2, 0);
    } else {
        LoadFieldExpandObjData(expObj, &sSceneSetupWhite2, 0);
    }
    work->fadeMode = FADE_ENGINE_A_WHITE;
    FieldExpObj_SetActorHidden(expObj, 0, MODEL_RELEASE, TRUE);
    FieldExpObj_SetActorHidden(expObj, 0, MODEL_THROWN, TRUE);
    for (i = 0; i < 4; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, MODEL_RELEASE, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(expObj, 0, MODEL_RELEASE, i, FALSE);
    }
    for (i = 0; i < 3; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, MODEL_THROWN, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(expObj, 0, MODEL_THROWN, i, FALSE);
    }
}

void GimmickDT9_End(Field *field) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);

    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
    FieldExpObj_FreeScene(expObj, 0);
}

void GimmickDT9_Update(Field *field) {
    FieldExpObj_StepAllAnimations(Field_GetExpObjSystem(field));
}

GameEvent *GimmickDT9_CreateReleaseEvent(GameSystem *gsys) {
    int i;
    FieldExpObjAnm *anm;
    VecFx32 pos;
    Field *field = GSYS_GetField(gsys);
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);

    FieldPlayer_GetWPos(Field_GetPlayer(field), &pos);
    FieldExpObj_GetActorMatrixPtr(expObj, 0, MODEL_RELEASE)->translation = pos;
    FieldExpObj_SetActorHidden(expObj, 0, MODEL_RELEASE, FALSE);
    for (i = 0; i < 4; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, MODEL_RELEASE, i);
        FieldExpObjAnm_SetPaused(anm, FALSE);
        FieldExpObj_SetAnm(expObj, 0, MODEL_RELEASE, i, TRUE);
        FieldExpObj_SetAnmFrame(expObj, 0, MODEL_RELEASE, i, 0);
    }
    return GameEvent_Create(gsys, NULL, GimmickDT9_ReleaseEvent, 0);
}

// Plays the sounds at frames of the release, and flashes the screen white at the 101st
static GameEventReturnCode GimmickDT9_ReleaseEvent(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    DT9GimmickWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    fx32 frame = FieldExpObj_GetAnmFrame(expObj, 0, MODEL_RELEASE, 0);
    fx32 lastFrame = FieldExpObjAnm_GetLastFrame(FieldExpObj_GetAnmInfo(expObj, 0, MODEL_RELEASE, 0));

    if (frame >= 0 && work->played[0] == FALSE) {
        GFL_SndStop();
        GFL_SndSEPlay(SEQ_SE_FLD_165);
        work->played[0] = TRUE;
    } else if (frame >= FX32_CONST(131) && work->played[1] == FALSE) {
        GFL_SndStop();
        GFL_SndSEPlay(SEQ_SE_FLD_162);
        work->played[1] = TRUE;
    } else if (frame >= FX32_CONST(108) && work->played[2] == FALSE) {
        GFL_SndStop();
        GFL_SndSEPlay(SEQ_SE_FLD_163);
        work->played[2] = TRUE;
    } else if (frame >= FX32_CONST(250) && work->played[3] == FALSE) {
        GFL_SndStop();
        GFL_SndSEPlay(SEQ_SE_FLD_175);
        work->played[3] = TRUE;
    }

    switch (*state) {
    case 0:
        if (frame == FX32_CONST(101)) {
            GFL_FadeSet(work->fadeMode, 0, 16, 0);
            work->fadeWait = 15;
            (*state)++;
        }
        break;
    case 1:
        if (GFL_FadeIsRunning() == FALSE) {
            if (work->fadeWait <= 0) {
                GFL_FadeSet(work->fadeMode, 16, 0, 0);
                (*state)++;
            } else {
                work->fadeWait--;
            }
        }
        break;
    case 2:
        if (GFL_FadeIsRunning() == FALSE) {
            (*state)++;
        }
        break;
    }
    return frame >= lastFrame;
}

GameEvent *GimmickDT9_CreateThrowEvent(GameSystem *gsys, const VecFx32 *from, const VecFx32 *to, fx32 height,
                                       u32 frames) {
    int i;
    FieldExpObjAnm *anm;
    Field *field = GSYS_GetField(gsys);
    DT9GimmickWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);

    GFL_SndSEPlay(SEQ_SE_NAGERU);
    FieldExpObj_GetActorMatrixPtr(expObj, 0, MODEL_THROWN)->translation = *from;
    FieldExpObj_SetActorHidden(expObj, 0, MODEL_THROWN, FALSE);
    for (i = 0; i < 3; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, MODEL_THROWN, i);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(expObj, 0, MODEL_THROWN, i, FALSE);
        FieldExpObj_SetAnmFrame(expObj, 0, MODEL_THROWN, i, 0);
    }
    work->from = *from;
    work->to = *to;
    work->frames = frames;
    work->frame = 0;
    work->height = height;
    return GameEvent_Create(gsys, NULL, GimmickDT9_ThrowEvent, 0);
}

void GimmickDT9_PlayThrown(GameSystem *gsys, const VecFx32 *pos) {
    int i;
    FieldExpObjAnm *anm;
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(GSYS_GetField(gsys));

    GFL_SndSEPlay(SEQ_SE_BOWA2);
    FieldExpObj_GetActorMatrixPtr(expObj, 0, MODEL_THROWN)->translation = *pos;
    FieldExpObj_SetActorHidden(expObj, 0, MODEL_THROWN, FALSE);
    for (i = 0; i < 3; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, MODEL_THROWN, i);
        FieldExpObjAnm_SetPaused(anm, FALSE);
        FieldExpObj_SetAnm(expObj, 0, MODEL_THROWN, i, TRUE);
        FieldExpObj_SetAnmFrame(expObj, 0, MODEL_THROWN, i, 0);
    }
}

GameEvent *GimmickDT9_CreateWaitFifthFrameEvent(GameSystem *gsys) {
    GSYS_GetField(gsys);
    return GameEvent_Create(gsys, NULL, GimmickDT9_PastFifthFrameEvent, 0);
}

GameEvent *GimmickDT9_CreateWaitThrownDoneEvent(GameSystem *gsys) {
    GSYS_GetField(gsys);
    return GameEvent_Create(gsys, NULL, GimmickDT9_ThrownDoneEvent, 0);
}

// Moves the thrown model along its arc, a frame at a time
static GameEventReturnCode GimmickDT9_ThrowEvent(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    DT9GimmickWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    VecFx32 pos;
    fx32 start;
    fx32 delta;
    int frame;

    work->frame++;
    start = work->from.x;
    delta = work->to.x - start;
    pos.x = start + delta * work->frame / work->frames;
    start = work->from.y;
    delta = work->to.y - start;
    pos.y = start + delta * work->frame / work->frames;
    start = work->from.z;
    delta = work->to.z - start;
    pos.z = start + delta * work->frame / work->frames;
    frame = work->frame;
    if (frame >= (work->frames + 1) / 2) {
        frame = work->frames - frame;
    }
    pos.y += work->height * frame / work->frames;
    FieldExpObj_GetActorMatrixPtr(expObj, 0, MODEL_THROWN)->translation = pos;
    if (work->frame >= work->frames) {
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode GimmickDT9_PastFifthFrameEvent(GameEvent *event, u32 *state, void *data) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(GSYS_GetField(GameEvent_GetGameSystem(event)));
    fx32 frame = FieldExpObj_GetAnmFrame(expObj, 0, MODEL_THROWN, 0);

    if (frame >= FX32_CONST(5)) {
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode GimmickDT9_ThrownDoneEvent(GameEvent *event, u32 *state, void *data) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(GSYS_GetField(GameEvent_GetGameSystem(event)));

    if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(expObj, 0, MODEL_THROWN, 0))) {
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
