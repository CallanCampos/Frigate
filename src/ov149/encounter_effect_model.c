#include "field/encounter_effect_model.h"
#include "types.h"
#include "constants/sound.h"
#include "field/encounter_effect.h"
#include "field/field.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/screentex.h"

// The encounter effects that play an animated 3D model, textured with a capture of the screen. The name is
// descriptive, since the overlay embeds no name

// The archive of the effects' models and animations
#define ARCID_ENCOUNT_EFFECT_MODEL 130

typedef struct EncounterModelWork {
    // The flashes so far
    s32 flashes;
    Field *field;
    u32 texBanks;
    // Whether the effect added bank D to the texture banks
    BOOL addedTexBank;
    u32 renderMode;
    G3DResource *modelRes;
    G3DResource *screenTex;
    G3DResource *animRes;
    G3DModel *model;
    G3DAnimation *anim;
    G3DActor *actor;
    u32 modelId;
    u32 animId;
    u32 fadeMode;
    BOOL fadeStarted;
    // The animation frame at which the closing fade starts
    fx32 fadeFrame;
    // The sound effect to play with the animation, or -1
    s32 se;
} EncounterModelWork;

GameEvent *EncEffModel_CreateModel7(GameSystem *gsys, Field *field, BOOL white) {
    return EncEffModel_Create(gsys, field, 7, 6, white);
}

GameEvent *EncEffModel_CreateModel9(GameSystem *gsys, Field *field, BOOL white) {
    return EncEffModel_Create(gsys, field, 9, 8, white);
}

GameEvent *EncEffModel_CreateModel5(GameSystem *gsys, Field *field, BOOL white) {
    return EncEffModel_Create(gsys, field, 5, 4, white);
}

GameEvent *EncEffModel_CreateModel3(GameSystem *gsys, Field *field, BOOL white) {
    return EncEffModel_Create(gsys, field, 3, 2, white);
}

GameEvent *EncEffModel_CreateModel1(GameSystem *gsys, Field *field, BOOL white) {
    return EncEffModel_Create(gsys, field, 1, 0, white);
}

GameEvent *EncEffModel_CreateZoroark(GameSystem *gsys, Field *field, BOOL white) {
    GameEvent *event;
    EncounterModelWork *work;

    event = EncEffModel_Create(gsys, field, 11, 10, white);
    work = EncEff_GetWorkArea(Field_GetEncEff(field));
    work->se = SEQ_SE_ZORO_01;
    return event;
}

GameEvent *EncEffModel_Create(GameSystem *gsys, Field *field, u32 modelId, u32 animId, BOOL white) {
    EncounterModelWork *work;
    GameEvent *event;

    work = EncEff_AllocWorkArea(Field_GetEncEff(GSYS_GetField(gsys)), sizeof(EncounterModelWork), 0x50);
    event = GameEvent_Create(gsys, NULL, EncEffModel_EventCallback, 0);
    work->field = field;
    work->modelId = modelId;
    work->animId = animId;
    work->se = -1;
    if (white) {
        work->fadeMode = FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE;
    } else {
        work->fadeMode = FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK;
    }
    return event;
}

void EncEffModel_Render(EncEff *effect) {
    EncounterModelWork *work = EncEff_GetWorkArea(effect);

    GFL_G3DAnmMdlDrawHeadless(work->actor);
}

static inline fx32 GetAnimFrame(G3DActor *actor) {
    fx32 frame;

    GFL_G3DActorGetAnmFrame(actor, 0, &frame);
    return frame;
}

GameEventReturnCode EncEffModel_EventCallback(GameEvent *event, u32 *state, void *data) {
    EncounterModelWork *work;
    fx32 frameCount;
    fx32 frame;

    work = EncEff_GetWorkArea(Field_GetEncEff(GSYS_GetField(GameEvent_GetGameSystem(event))));

    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_WHITE, 0, 16, 0);
        (*state)++;
        // Fall through to check the fade in the same frame.
    case 1:
        if (!GFL_FadeIsRunning()) {
            GFL_FadeSet(FADE_ENGINE_A_WHITE, 16, 0, 0);
            (*state)++;
        }
        break;
    case 2:
        if (!GFL_FadeIsRunning()) {
            work->flashes++;
            if (work->flashes < 2) {
                GFL_FadeSet(FADE_ENGINE_A_WHITE, 0, 16, 0);
                *state = 1;
            } else {
                (*state)++;
            }
        }
        break;
    case 3:
        work->texBanks = GX_GetBankForTex();
        work->addedTexBank = FALSE;
        if (!(work->texBanks & GX_VRAM_D)) {
            work->addedTexBank = TRUE;
            work->texBanks |= GX_VRAM_D;
            GX_SetBankForTex(work->texBanks);
        }
        work->modelRes = GFL_G3DSysReadArcSysResource(ARCID_ENCOUNT_EFFECT_MODEL, work->modelId);
        work->animRes = GFL_G3DSysReadArcSysResource(ARCID_ENCOUNT_EFFECT_MODEL, work->animId);
        work->screenTex = GFL_G3DScreenTexCreate(Field_GetHeapID(work->field), SCREENTEX_BANK_D);
        work->model = GFL_G3DMdlCreate(work->modelRes, 0, work->screenTex);
        work->anim = GFL_G3DAnmCreate(work->model, work->animRes, 0);
        work->actor = GFL_G3DActorCreate(work->model, &work->anim, 1);
        GFL_G3DActorBindAnm(work->actor, 0);
        GFL_G3DScreenTexCapture(SCREENTEX_BANK_D, SCREENTEX_BANK_D);
        work->renderMode = Field_GetRenderMode(work->field);
        Field_SetRenderMode(work->field, 2);
        work->fadeStarted = FALSE;
        GFL_G3DActorGetAnmFrameCount(work->actor, 0, &frameCount);
        work->fadeFrame = frameCount - FX32_CONST(20);
        if (work->se != -1) {
            GFL_SndSEPlay(work->se);
        }
        (*state)++;
        break;
    case 4:
        frame = GetAnimFrame(work->actor);
        if (!work->fadeStarted && frame >= work->fadeFrame) {
            GFL_FadeSet(work->fadeMode, 0, 16, 3);
            work->fadeStarted = TRUE;
        }
        if (!GFL_G3DActorStepAnmFrame(work->actor, 0, FX32_ONE) && !GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case 5:
        if (work->addedTexBank == TRUE) {
            work->texBanks &= ~GX_VRAM_D;
            GX_SetBankForTex(work->texBanks);
        }
        GFL_G3DActorFree(work->actor);
        GFL_G3DAnmFree(work->anim);
        GFL_G3DMdlFree(work->model);
        GFL_G3DResFree(work->animRes);
        GFL_G3DResFree(work->screenTex);
        GFL_G3DResFree(work->modelRes);
        Field_SetRenderMode(work->field, work->renderMode);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
