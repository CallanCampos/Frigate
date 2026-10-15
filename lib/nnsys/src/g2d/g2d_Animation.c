#include "nnsys/g2d.h"
#include <stdlib.h>

// NitroSystem's g2d_Animation.c: the animation controller, which steps through the frames of an animation sequence by
// time, forward or in reverse, loops or stops at its ends and calls back at chosen frames

static void SequenceEdgeHandle_(NNSG2dAnimController *animCtrl);
static BOOL SetAnimCtrlCurrentFrameImpl_(NNSG2dAnimController *animCtrl, u16 frameIdx);

// The controller's play mode, if it overrides the sequence's
static inline u32 GetAnimationPlayMode_(const NNSG2dAnimController *animCtrl) {
    u32 playMode = animCtrl->overriddenPlayMode;

    if (playMode == NNS_G2D_ANIMATIONPLAYMODE_INVALID) {
        playMode = animCtrl->pAnimSequence->playMode;
    }
    return playMode;
}

static inline BOOL IsLoopAnimation_(const NNSG2dAnimController *animCtrl) {
    u32 playMode = GetAnimationPlayMode_(animCtrl);

    return playMode == NNS_G2D_ANIMATIONPLAYMODE_FORWARD_LOOP || playMode == NNS_G2D_ANIMATIONPLAYMODE_REVERSE_LOOP;
}

// Whether the sequence plays forward and then back
static inline BOOL IsReversePlayMode_(const NNSG2dAnimController *animCtrl) {
    u32 playMode = GetAnimationPlayMode_(animCtrl);

    return playMode == NNS_G2D_ANIMATIONPLAYMODE_REVERSE || playMode == NNS_G2D_ANIMATIONPLAYMODE_REVERSE_LOOP;
}

// Whether the frames are stepped forward: a negative speed reverses the direction
static inline BOOL IsForward_(const NNSG2dAnimController *animCtrl) {
    return animCtrl->bReverse ^ (animCtrl->speed > 0);
}

// Whether the current frame has run past the sequence's end, or back past its loop start
static inline BOOL IsReachEdge_(const NNSG2dAnimController *animCtrl) {
    const NNSG2dAnimFrameData *current = animCtrl->pCurrent;

    if (IsForward_(animCtrl)) {
        return current >= animCtrl->pAnimSequence->pAnmFrameArray + animCtrl->pAnimSequence->numFrames;
    }
    return current <= animCtrl->pAnimSequence->pAnmFrameArray + animCtrl->pAnimSequence->loopStartFrameIdx - 1;
}

static inline BOOL IsFrameEnd_(const NNSG2dAnimController *animCtrl) {
    if (animCtrl->bActive && animCtrl->currentTime >= (animCtrl->pCurrent->frames << FX32_SHIFT)) {
        return TRUE;
    }
    return FALSE;
}

static inline u16 GetCurrentFrameIdx_(const NNSG2dAnimController *animCtrl) {
    return ((u32)animCtrl->pCurrent - (u32)animCtrl->pAnimSequence->pAnmFrameArray) / sizeof(NNSG2dAnimFrameData);
}

// At the sequence's end: calls back if asked to, then loops or stops
static inline void EndSequence_(NNSG2dAnimController *animCtrl) {
    if (animCtrl->callbackFunctor.type == NNS_G2D_ANMCALLBACKTYPE_LAST_FRM) {
        animCtrl->callbackFunctor.pFunc(animCtrl->callbackFunctor.param, animCtrl->currentTime);
    }
    if (!IsLoopAnimation_(animCtrl)) {
        animCtrl->bActive = FALSE;
    } else {
        NNS_G2dResetAnimCtrlState(animCtrl);
    }
}

// The sequence's last frame
static inline const NNSG2dAnimFrameData *GetLastFrame_(const NNSG2dAnimSequence *seq) {
    return seq->pAnmFrameArray + seq->numFrames - 1;
}

// Turns a reverse sequence around at its end, or ends the sequence, and keeps the current frame inside it
static void SequenceEdgeHandle_(NNSG2dAnimController *animCtrl) {
    const NNSG2dAnimFrameData *current;

    if (IsReversePlayMode_(animCtrl)) {
        animCtrl->bReverse ^= TRUE;
        if (animCtrl->pCurrent <=
            animCtrl->pAnimSequence->pAnmFrameArray + animCtrl->pAnimSequence->loopStartFrameIdx - 1) {
            EndSequence_(animCtrl);
        }
    } else {
        EndSequence_(animCtrl);
    }

    current = animCtrl->pCurrent;
    if (current > GetLastFrame_(animCtrl->pAnimSequence)) {
        animCtrl->pCurrent = GetLastFrame_(animCtrl->pAnimSequence);
    } else if (current < animCtrl->pAnimSequence->pAnmFrameArray) {
        animCtrl->pCurrent = animCtrl->pAnimSequence->pAnmFrameArray;
    }
}

static BOOL SetAnimCtrlCurrentFrameImpl_(NNSG2dAnimController *animCtrl, u16 frameIdx) {
    if (frameIdx < animCtrl->pAnimSequence->numFrames) {
        animCtrl->pCurrent = &animCtrl->pAnimSequence->pAnmFrameArray[frameIdx];
        if (animCtrl->pCurrent->frames != 0) {
            animCtrl->pActiveCurrent = animCtrl->pCurrent;
        }
        return TRUE;
    }
    return FALSE;
}

void *NNS_G2dGetAnimCtrlCurrentElement(const NNSG2dAnimController *animCtrl) {
    return animCtrl->pActiveCurrent->pContent;
}

// Calls back at the chosen frame or at every frame
static inline void CallBackFuncHandling_(const NNSG2dCallBackFunctor *functor, u16 frameIdx) {
    switch (functor->type) {
    case NNS_G2D_ANMCALLBACKTYPE_SPEC_FRM:
        if (frameIdx == functor->frameIdx) {
            functor->pFunc(functor->param, frameIdx);
        }
        break;
    case NNS_G2D_ANMCALLBACKTYPE_EVER_FRM:
        functor->pFunc(functor->param, frameIdx);
        break;
    }
}

BOOL NNS_G2dTickAnimCtrl(NNSG2dAnimController *animCtrl, fx32 frames) {
    BOOL changed = FALSE;

    if (animCtrl->bActive != TRUE) {
        return FALSE;
    }

    animCtrl->currentTime += abs(FX_MulFunc(animCtrl->speed, frames));
    while (IsFrameEnd_(animCtrl)) {
        changed = TRUE;
        animCtrl->currentTime -= (animCtrl->pCurrent->frames << FX32_SHIFT);
        if (IsForward_(animCtrl)) {
            animCtrl->pCurrent++;
        } else {
            animCtrl->pCurrent--;
        }
        if (IsReachEdge_(animCtrl)) {
            SequenceEdgeHandle_(animCtrl);
        }
        if (animCtrl->pCurrent->frames != 0) {
            animCtrl->pActiveCurrent = animCtrl->pCurrent;
        }

        if (animCtrl->callbackFunctor.type != NNS_G2D_ANMCALLBACKTYPE_NONE) {
            CallBackFuncHandling_(&animCtrl->callbackFunctor, GetCurrentFrameIdx_(animCtrl));
        }
    }
    return changed;
}

BOOL NNS_G2dSetAnimCtrlCurrentFrame(NNSG2dAnimController *animCtrl, u16 frameIdx) {
    BOOL result = SetAnimCtrlCurrentFrameImpl_(animCtrl, frameIdx);

    if (result) {
        animCtrl->currentTime = 0;
    }
    return result;
}

u16 NNS_G2dGetAnimCtrlCurrentFrame(const NNSG2dAnimController *animCtrl) {
    return GetCurrentFrameIdx_(animCtrl);
}

void NNS_G2dInitAnimCtrl(NNSG2dAnimController *animCtrl) {
    NNS_G2dInitAnimCallBackFunctor(&animCtrl->callbackFunctor);
    animCtrl->pCurrent = NULL;
    animCtrl->pActiveCurrent = NULL;
    animCtrl->bReverse = FALSE;
    animCtrl->bActive = TRUE;
    animCtrl->currentTime = 0;
    animCtrl->speed = FX32_ONE;
    animCtrl->overriddenPlayMode = NNS_G2D_ANIMATIONPLAYMODE_INVALID;
    animCtrl->pAnimSequence = NULL;
}

void NNS_G2dInitAnimCallBackFunctor(NNSG2dCallBackFunctor *functor) {
    functor->type = NNS_G2D_ANMCALLBACKTYPE_NONE;
    functor->param = 0;
    functor->pFunc = NULL;
    functor->frameIdx = 0;
}

// Back to the first frame to play: the loop start going forward, the last frame in reverse
void NNS_G2dResetAnimCtrlState(NNSG2dAnimController *animCtrl) {
    if (IsForward_(animCtrl)) {
        animCtrl->pCurrent = animCtrl->pAnimSequence->pAnmFrameArray + animCtrl->pAnimSequence->loopStartFrameIdx;
    } else {
        animCtrl->pCurrent = animCtrl->pAnimSequence->pAnmFrameArray + animCtrl->pAnimSequence->numFrames - 1;
    }
    animCtrl->pActiveCurrent = animCtrl->pCurrent;
    animCtrl->currentTime = 0;
    NNS_G2dTickAnimCtrl(animCtrl, 0);
}

void NNS_G2dBindAnimCtrl(NNSG2dAnimController *animCtrl, const NNSG2dAnimSequence *seq) {
    animCtrl->pAnimSequence = seq;
    NNS_G2dResetAnimCtrlState(animCtrl);
}

void NNS_G2dSetAnimCtrlCallBackFunctor(NNSG2dAnimController *animCtrl, u32 type, u32 param,
                                       NNSG2dAnmCallBackPtr pFunc) {
    animCtrl->callbackFunctor.pFunc = pFunc;
    animCtrl->callbackFunctor.param = param;
    animCtrl->callbackFunctor.type = type;
    animCtrl->callbackFunctor.frameIdx = 0;
}

void NNS_G2dSetAnimCtrlCallBackFunctorAtAnimFrame(NNSG2dAnimController *animCtrl, u32 param, NNSG2dAnmCallBackPtr pFunc,
                                                  u16 frameIdx) {
    animCtrl->callbackFunctor.type = NNS_G2D_ANMCALLBACKTYPE_SPEC_FRM;
    animCtrl->callbackFunctor.pFunc = pFunc;
    animCtrl->callbackFunctor.param = param;
    animCtrl->callbackFunctor.frameIdx = frameIdx;
}

BOOL NNSi_G2dIsAnimCtrlLoopAnim(const NNSG2dAnimController *animCtrl) {
    return IsLoopAnimation_(animCtrl);
}
