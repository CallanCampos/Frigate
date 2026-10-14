#include "app/comm_tvt/ctvt_game_target.h"
#include "types.h"
#include "app/comm_tvt/ctvt_game.h"
#include "gfl/g3d.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nnsys/g3d.h"

// The faces that float up in the target game. No string names this file; ctvt_game_target.c is a guess, from the
// order of ctvt_game.c's data, which ends before this file's bounds table

static BOOL CtvtGameTarget_UpdateHit(CtvtGameTarget *target);
static BOOL CtvtGameTarget_Fade(CtvtGameTarget *target);

// Where a target of each depth leaves the screen
static const struct {
    fx32 maxY;
    fx32 minX;
    fx32 maxX;
} sTargetBounds[] = {
    { FX32_CONST(270), FX32_CONST(-254), FX32_CONST(254) },
    { FX32_CONST(260), FX32_CONST(-233), FX32_CONST(233) },
    { FX32_CONST(250), FX32_CONST(-200), FX32_CONST(200) },
};

void CtvtGameTarget_Init(CtvtGameTarget *target) {
    target->srt.scale.x = FX32_ONE;
    target->srt.scale.y = FX32_ONE;
    target->srt.scale.z = FX32_ONE;
    MTX_Identity33_(&target->srt.rotation);
    target->depth = 2;
    target->alpha = 31;
    target->fadeFrames = 0;
    target->srt.translation.x = 0;
    target->srt.translation.y = 0;
    target->srt.translation.z = 0;
    target->velocity.x = 0;
    target->velocity.y = 0;
    target->velocity.z = 0;
    target->active = FALSE;
    target->scene = 0;
    target->actor = 0;
    target->hitFrame = 0;
    target->hitAnimating = FALSE;
    target->grow = 0;
    target->hitKind = 0;
    target->hitPending = FALSE;
    target->tcb = NULL;
}

void CtvtGameTarget_Delete(CtvtGameTarget *target) {
    if (target->tcb != NULL) {
        GFL_TCBRemove(target->tcb);
        target->tcb = NULL;
    }
    CtvtGameTarget_Init(target);
}

void CtvtGameTarget_Update(CtvtGameTarget *target) {
    u16 frame = CtvtGame_GetFrame(target->game);
    int elapsed = frame - target->frame;
    u8 i;

    if (elapsed <= 0) {
        return;
    }
    target->frame = frame;
    for (i = 0; i < elapsed; i++) {
        if (target->actor != 0) {
            if (target->alpha == 31) {
                if (CtvtGameTarget_UpdateHit(target) == TRUE) {
                    target->alpha--;
                }
            } else if (CtvtGameTarget_Fade(target) == TRUE) {
                CtvtGameTarget_Delete(target);
                return;
            }
        } else {
            VEC_Add(&target->srt.translation, &target->velocity, &target->srt.translation);
        }
    }
    if (target->srt.translation.y > sTargetBounds[target->depth - 1].maxY ||
        target->srt.translation.x > sTargetBounds[target->depth - 1].maxX ||
        target->srt.translation.x < sTargetBounds[target->depth - 1].minX) {
        CtvtGameTarget_Delete(target);
    }
}

void CtvtGameTarget_Draw(CtvtGameTarget *target) {
    G3DActor *actor;
    fx32 count;
    u16 first;
    u16 index;

    if (target->active == TRUE) {
        if (target->actor != 0 && target->hitAnimating == TRUE) {
            index = GFL_G3DMgrGetSceneFirstActorIdx(target->g3d, 4);
            index += target->hitKind;
            actor = GFL_G3DMgrGetActor(target->g3d, index);
            GFL_G3DActorSetAnmFrame(actor, 0, &target->hitFrame);
            GFL_G3DActorSetAnmFrame(actor, 1, &target->hitFrame);
            target->hitFrame += FX32_ONE;
            GFL_G3DActorGetAnmFrameCount(actor, 0, &count);
            if (target->hitFrame >= count) {
                target->hitAnimating = FALSE;
            }
            GFL_G3DSysDrawObj(actor, &target->srt);
        }
        first = GFL_G3DMgrGetSceneFirstActorIdx(target->g3d, target->scene);
        actor = GFL_G3DMgrGetActor(target->g3d, first + target->actor);
        func_02068410(GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(actor))->resMdl, target->alpha);
        GFL_G3DSysDrawObj(actor, &target->srt);
    }
}

static BOOL CtvtGameTarget_UpdateHit(CtvtGameTarget *target) {
    VecFx32 grow;
    u16 first;

    switch (target->hitKind) {
    case 0:
    case 1:
        first = GFL_G3DMgrGetSceneFirstActorIdx(target->g3d, target->scene);
        if (!GFL_G3DActorStepAnmFrameLoop(GFL_G3DMgrGetActor(target->g3d, first + target->actor), 2, FX32_ONE)) {
            return TRUE;
        }
        break;
    case 2:
    case 3:
        if (target->grow < 0x310) {
            target->grow += 0x4a;
            if (target->grow > 0x310) {
                target->grow = 0x310;
            }
            grow.x = 0x4a;
            grow.y = 0x4a;
            grow.z = 0x4a;
            VEC_Add(&target->srt.scale, &grow, &target->srt.scale);
        }
        if (target->grow == 0x310) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL CtvtGameTarget_Fade(CtvtGameTarget *target) {
    target->alpha -= 3;
    if (target->alpha <= 0) {
        target->alpha = 0;
        if (target->hitKind <= 1) {
            return TRUE;
        }
        if (target->active == TRUE) {
            target->active = FALSE;
            target->fadeFrames = 10;
        } else {
            target->fadeFrames--;
            if (target->fadeFrames <= 0) {
                CtvtGameTarget_Delete(target);
                return TRUE;
            }
        }
    }
    return FALSE;
}
