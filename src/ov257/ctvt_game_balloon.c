#include "app/comm_tvt/ctvt_game_balloon.h"
#include "types.h"
#include "app/comm_tvt/ctvt_game.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nnsys/g3d.h"

// The balloon game's 3D objects: each member's balloon, which grows in three stages with the pumps and pops at the
// fourth, then bounces back in another color, and the puffs of air that rise from the bottom of the screen

#define CTVT_GAME_BALLOON_STAGES 3
#define CTVT_GAME_BALLOON_COLORS 4
// The scene of the pop's model
#define CTVT_GAME_BALLOON_POP_SCENE 4
// The models' alpha, from 0 to 31
#define CTVT_GAME_ALPHA_MAX 31
// How long a new balloon bounces in, in frames
#define CTVT_GAME_BALLOON_BOUNCE_FRAMES 60

// Where the others' balloons are, by the order of their members
static const VecFx32 sBalloonPositions[] = {
    { 0, FX32_CONST(2), FX32_CONST(-20) },
    { FX32_CONST(-87), FX32_CONST(-5), FX32_CONST(-40) },
    { FX32_CONST(93), FX32_CONST(-5), FX32_CONST(-60) },
};

CtvtGameBalloon *CtvtGameBalloon_Create(CtvtGame *game, u16 netId, u8 pos, BOOL active, HeapID heapId) {
    u8 selfNetId = func_02042a6c(func_02040440());
    u8 memberCount = func_02042a78();
    CtvtGameBalloon *balloon = GFL_HeapAllocate(heapId, sizeof(CtvtGameBalloon), TRUE, "ctvt_game_balloon.c", 482);

    balloon->active = active;
    balloon->tcb = NULL;
    balloon->game = game;
    balloon->g3d = CtvtGame_GetG3DManager(game);
    balloon->netId = netId;
    balloon->grow = 0;
    balloon->growing = FALSE;
    balloon->stage = 0;
    balloon->pumpCount = 0;
    balloon->color = 0;
    balloon->bounce = CTVT_GAME_BALLOON_BOUNCE_FRAMES;
    balloon->popShown = FALSE;
    balloon->popAnimating = FALSE;
    balloon->popFrame = 0;
    balloon->alpha = CTVT_GAME_ALPHA_MAX;
    balloon->popping = FALSE;
    balloon->wobbleAngle = 0;
    balloon->wobblePhase = 0;
    balloon->wobbling = FALSE;
    if (netId == selfNetId) {
        MTX_Identity33_(&balloon->srt.rotation);
        balloon->srt.translation.x = 0;
        balloon->srt.translation.y = FX32_CONST(-60);
        balloon->srt.translation.z = 0;
        balloon->isSelf = TRUE;
        balloon->growStep = 0x75;
        balloon->growMax = 0x750;
    } else {
        // With three members, the two others take the outer places
        if (memberCount == 3) {
            pos++;
        }
        MTX_Identity33_(&balloon->srt.rotation);
        balloon->srt.translation.x = sBalloonPositions[pos].x;
        balloon->srt.translation.y = sBalloonPositions[pos].y;
        balloon->srt.translation.z = sBalloonPositions[pos].z;
        balloon->isSelf = FALSE;
        balloon->growStep = 0x27;
        balloon->growMax = 0x180;
    }
    balloon->srt.scale.x = 0;
    balloon->srt.scale.y = 0;
    balloon->srt.scale.z = 0;
    return balloon;
}

void CtvtGameBalloon_Delete(CtvtGameBalloon *balloon) {
    if (balloon->tcb != NULL) {
        GFL_TCBRemove(balloon->tcb);
        balloon->tcb = NULL;
    }
    if (balloon != NULL) {
        GFL_HeapFree(balloon);
    }
}

// Turns the balloon by its wobble, reading the sine and cosine from the table as FX_SinIdx and FX_CosIdx do
static inline void CtvtGameBalloon_SetRotation(CtvtGameBalloon *balloon) {
    int index = ((u16)balloon->wobbleAngle >> 4) * 2;

    MTX_Identity33_(&balloon->srt.rotation);
    MTX_RotZ33_(&balloon->srt.rotation, FX_SinCosTable_[index], FX_SinCosTable_[index + 1]);
}

void CtvtGameBalloon_Update(CtvtGameBalloon *balloon) {
    if (balloon->active == TRUE) {
        if (balloon->growing == TRUE) {
            if (balloon->grow < balloon->growMax) {
                VecFx32 step;

                balloon->grow += balloon->growStep;
                if (balloon->grow > balloon->growMax) {
                    balloon->grow = balloon->growMax;
                }
                step.x = balloon->growStep;
                step.y = balloon->growStep;
                step.z = balloon->growStep;
                VEC_Add(&balloon->srt.scale, &step, &balloon->srt.scale);
            }
            if (balloon->wobbling == TRUE) {
                switch (balloon->wobblePhase) {
                case 0:
                    balloon->wobbleAngle += 0x100;
                    if (balloon->wobbleAngle >= 0xc00) {
                        balloon->wobblePhase = 1;
                    }
                    break;
                case 1:
                    balloon->wobbleAngle -= 0x100;
                    if (balloon->wobbleAngle <= -0xc00) {
                        balloon->wobblePhase = 2;
                    }
                    break;
                case 2:
                    balloon->wobbleAngle += 0x80;
                    if (balloon->wobbleAngle >= 0) {
                        balloon->wobbleAngle = 0;
                        balloon->wobblePhase = 0;
                        balloon->wobbling = FALSE;
                    }
                    break;
                }
                CtvtGameBalloon_SetRotation(balloon);
            }
            if (balloon->grow == balloon->growMax && balloon->wobbling == FALSE) {
                balloon->grow = 0;
                balloon->growing = FALSE;
                switch (balloon->stage) {
                case 0:
                    balloon->stage = 1;
                    break;
                case 1:
                    balloon->stage = 2;
                    break;
                case 2:
                    break;
                }
            }
        } else if (balloon->popping == TRUE) {
            VecFx32 step;
            fx32 speed;

            if (balloon->isSelf == TRUE) {
                speed = 0xe6;
            } else {
                speed = 0x80;
            }
            step.x = speed;
            step.y = speed;
            step.z = speed;
            VEC_Add(&balloon->srt.scale, &step, &balloon->srt.scale);
            balloon->alpha -= 2;
            if (balloon->alpha <= 0) {
                balloon->bounce = CTVT_GAME_BALLOON_BOUNCE_FRAMES;
                balloon->popping = FALSE;
                balloon->alpha = CTVT_GAME_ALPHA_MAX;
                balloon->stage = 0;
                balloon->color++;
                balloon->color %= CTVT_GAME_BALLOON_COLORS;
                balloon->srt.scale.x = 0;
                balloon->srt.scale.y = 0;
                balloon->srt.scale.z = 0;
                CtvtGame_LoadBalloonPictures(balloon->game, balloon->netId, balloon->color);
            }
        } else if (balloon->wobbling == TRUE) {
            switch (balloon->wobblePhase) {
            case 0:
                balloon->wobbleAngle += 0xa0;
                if (balloon->wobbleAngle >= 0x400) {
                    balloon->wobblePhase = 1;
                }
                break;
            case 1:
                balloon->wobbleAngle -= 0x50;
                if (balloon->wobbleAngle <= -0x400) {
                    balloon->wobblePhase = 2;
                }
                break;
            case 2:
                balloon->wobbleAngle += 0x40;
                if (balloon->wobbleAngle >= 0) {
                    balloon->wobbleAngle = 0;
                    balloon->wobblePhase = 0;
                    balloon->wobbling = FALSE;
                }
                break;
            }
            CtvtGameBalloon_SetRotation(balloon);
        }
    }

    if (balloon->bounce != 0) {
        VecFx32 step;
        fx32 speed;

        if (balloon->isSelf == TRUE) {
            speed = 0x26;
        } else {
            speed = 0x15;
        }
        step.x = speed;
        step.y = speed;
        step.z = speed;
        VEC_Add(&balloon->srt.scale, &step, &balloon->srt.scale);
        balloon->bounce--;
        if (balloon->bounce == 0) {
            if (balloon->isSelf == TRUE) {
                balloon->srt.scale.x = 0x900;
                balloon->srt.scale.y = 0x900;
                balloon->srt.scale.z = 0x900;
            } else {
                balloon->srt.scale.x = 0x500;
                balloon->srt.scale.y = 0x500;
                balloon->srt.scale.z = 0x500;
            }
        }
    }
}

void CtvtGameBalloon_Draw(CtvtGameBalloon *balloon) {
    G3DActor *actor;
    u16 first;

    if (balloon->active != TRUE) {
        return;
    }
    if (balloon->popShown == TRUE && balloon->growing == FALSE) {
        fx32 frameCount;
        SRTMatrix srt;
        fx32 scale;

        actor = GFL_G3DMgrGetActor(balloon->g3d,
                                   GFL_G3DMgrGetSceneFirstActorIdx(balloon->g3d, CTVT_GAME_BALLOON_POP_SCENE));
        if (balloon->popAnimating == TRUE) {
            GFL_G3DActorSetAnmFrame(actor, 0, &balloon->popFrame);
            GFL_G3DActorSetAnmFrame(actor, 1, &balloon->popFrame);
            balloon->popFrame += FX32_ONE;
            GFL_G3DActorGetAnmFrameCount(actor, 0, &frameCount);
            if (balloon->popFrame >= frameCount) {
                balloon->popAnimating = FALSE;
                balloon->popShown = FALSE;
            }
        }
        if (balloon->isSelf == TRUE) {
            scale = 0x17a0;
        } else {
            scale = 0x800;
        }
        srt.scale.x = scale;
        srt.scale.y = scale;
        srt.scale.z = scale;
        MTX_Identity33_(&srt.rotation);
        srt.translation.x = balloon->srt.translation.x;
        srt.translation.y = balloon->srt.translation.y;
        srt.translation.z = balloon->srt.translation.z;
        GFL_G3DSysDrawObj(actor, &srt);
    }
    first = GFL_G3DMgrGetSceneFirstActorIdx(balloon->g3d, balloon->netId);
    actor = GFL_G3DMgrGetActor(balloon->g3d, first + balloon->stage);
    func_02068410(GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(actor))->resMdl, balloon->alpha);
    GFL_G3DSysDrawObj(actor, &balloon->srt);
}

BOOL CtvtGameBalloon_Pump(CtvtGameBalloon *balloon) {
    BOOL popped;

    switch (balloon->pumpCount) {
    case 0:
    case 1:
    case 2:
        balloon->wobbling = TRUE;
        balloon->growing = TRUE;
        balloon->pumpCount++;
        popped = FALSE;
        break;
    case CTVT_GAME_BALLOON_STAGES:
        balloon->popping = TRUE;
        balloon->popShown = TRUE;
        balloon->popFrame = 0;
        balloon->popAnimating = TRUE;
        balloon->pumpCount = 0;
        popped = TRUE;
        break;
    }
    return popped;
}

BOOL CtvtGameBalloon_IsIdle(CtvtGameBalloon *balloon) {
    if (balloon->bounce != 0 || balloon->popShown == TRUE || balloon->popping == TRUE) {
        return FALSE;
    }
    return TRUE;
}

void CtvtGameBalloon_Wobble(CtvtGameBalloon *balloon) {
    if (balloon->wobbling == FALSE && balloon->growing == FALSE && balloon->popping == FALSE &&
        balloon->pumpCount < CTVT_GAME_BALLOON_STAGES && balloon->bounce == 0) {
        balloon->wobbling = TRUE;
    }
}

BOOL CtvtGameBalloon_IsBusy(CtvtGameBalloon *balloon) {
    if (balloon->growing == TRUE || balloon->popping == TRUE || balloon->popAnimating == TRUE || balloon->bounce != 0) {
        return TRUE;
    }
    return FALSE;
}

CtvtGameShot *CtvtGameShot_Create(CtvtGame *game, u16 scene, int x, u8 vanishOnHit, fx32 topY, HeapID heapId) {
    CtvtGameShot *shot = GFL_HeapAllocate(heapId, sizeof(CtvtGameShot), TRUE, "ctvt_game_balloon.c", 989);

    shot->active = TRUE;
    shot->hit = FALSE;
    shot->tcb = NULL;
    shot->game = game;
    shot->g3d = CtvtGame_GetG3DManager(game);
    shot->scene = scene;
    shot->alpha = CTVT_GAME_ALPHA_MAX;
    shot->hitFrame = 0;
    shot->hitAnimating = FALSE;
    shot->grow = 0;
    shot->vanishOnHit = vanishOnHit;
    shot->topY = topY;
    shot->srt.scale.x = 0x500;
    shot->srt.scale.y = 0x500;
    shot->srt.scale.z = 0x500;
    MTX_Identity33_(&shot->srt.rotation);
    shot->srt.translation.x = FX32_CONST(x * 60) - FX32_CONST(90);
    shot->srt.translation.y = FX32_CONST(-100);
    shot->srt.translation.z = 0;
    shot->velocity.x = 0;
    shot->velocity.y = 0x480;
    shot->velocity.z = 0;
    return shot;
}

void CtvtGameShot_Delete(CtvtGameShot *shot) {
    if (shot->tcb != NULL) {
        GFL_TCBRemove(shot->tcb);
        shot->tcb = NULL;
    }
    if (shot != NULL) {
        GFL_HeapFree(shot);
    }
}

void CtvtGameShot_Update(CtvtGameShot *shot) {
    if (shot->active != TRUE) {
        return;
    }
    if (shot->hit == TRUE) {
        if (shot->grow < 0x260) {
            VecFx32 step;

            shot->grow += 0x4a;
            if (shot->grow > 0x260) {
                shot->grow = 0x260;
            }
            step.x = 0x4a;
            step.y = 0x4a;
            step.z = 0x4a;
            VEC_Add(&shot->srt.scale, &step, &shot->srt.scale);
        }
        if (shot->grow == 0x260) {
            shot->alpha -= 2;
            if (shot->alpha <= 0) {
                shot->active = FALSE;
            }
        }
    } else {
        VEC_Add(&shot->srt.translation, &shot->velocity, &shot->srt.translation);
        if (shot->srt.translation.y > shot->topY) {
            shot->hit = TRUE;
            shot->hitAnimating = TRUE;
            if (shot->vanishOnHit == TRUE) {
                shot->active = FALSE;
                shot->hit = FALSE;
                shot->hitAnimating = FALSE;
            }
        }
    }
}

void CtvtGameShot_Draw(CtvtGameShot *shot) {
    G3DActor *actor;
    u16 index;

    if (shot->active != TRUE && shot->vanishOnHit != TRUE) {
        return;
    }
    if (shot->hit == TRUE && shot->hitAnimating == TRUE) {
        fx32 frameCount;
        SRTMatrix srt;

        actor = GFL_G3DMgrGetActor(shot->g3d, GFL_G3DMgrGetSceneFirstActorIdx(shot->g3d, CTVT_GAME_BALLOON_POP_SCENE));
        GFL_G3DActorSetAnmFrame(actor, 0, &shot->hitFrame);
        shot->hitFrame += FX32_ONE;
        GFL_G3DActorGetAnmFrameCount(actor, 0, &frameCount);
        if (shot->hitFrame >= frameCount) {
            shot->hitAnimating = FALSE;
        }
        srt.scale.x = shot->srt.scale.x;
        srt.scale.y = shot->srt.scale.y;
        srt.scale.z = shot->srt.scale.z;
        MTX_Identity33_(&srt.rotation);
        srt.translation.x = shot->srt.translation.x;
        srt.translation.y = shot->srt.translation.y;
        srt.translation.z = shot->srt.translation.z - FX32_CONST(3);
        GFL_G3DSysDrawObj(actor, &srt);
    }
    index = GFL_G3DMgrGetSceneFirstActorIdx(shot->g3d, shot->scene);
    if (shot->hit == TRUE) {
        index++;
    }
    actor = GFL_G3DMgrGetActor(shot->g3d, index);
    func_02068410(GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(actor))->resMdl, shot->alpha);
    GFL_G3DSysDrawObj(actor, &shot->srt);
}
