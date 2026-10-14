#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "demo/intro.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/sound.h"
#include "nitro/hw.h"

// The intro's 3D model, which is only loaded in the parts of the intro that show it

#define SCENE_NONE 0xffff

struct IntroG3d {
    IntroGraphic *graphic;
    HeapID heapId;
    G3DManager *manager;
    G3DCamera *camera;
    u16 scene;
    BOOL visible;
    BOOL unk18;
    u32 mode;
    u32 unk20;
    u32 state;
    u32 frame;
    u32 unk2C;
};

static const VecFx32 sPosition = { 0, 0, FX32_ONE };
static const VecFx32 sUpVector = { 0, FX32_ONE, 0 };
static const VecFx32 sTarget = { 0, 0, -FX32_ONE };

// The model and its animations, which differ by version
static const G3DSceneResourceSetup sResources[] = {
#ifdef BLACK2
    { ARCID_INTRO, 22, 0 }, { ARCID_INTRO, 16, 0 }, { ARCID_INTRO, 17, 0 }, { ARCID_INTRO, 18, 0 }, { ARCID_INTRO, 12, 0 }, { ARCID_INTRO, 13, 0 },
#else
    { ARCID_INTRO, 23, 0 }, { ARCID_INTRO, 19, 0 }, { ARCID_INTRO, 20, 0 }, { ARCID_INTRO, 21, 0 }, { ARCID_INTRO, 14, 0 }, { ARCID_INTRO, 15, 0 },
#endif
};

static const G3DSceneAnimationSetup sAnimations[] = {
    { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 }, { 5, 0 },
};

static const G3DSceneActorSetup sActors[] = {
    { 0, 0, 0, 0, sAnimations, NELEMS(sAnimations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

IntroG3d *IntroG3d_Create(IntroGraphic *graphic, u32 mode, HeapID heapId) {
    IntroG3d *g3d = GFL_HeapAllocate(heapId, sizeof(IntroG3d), TRUE, "intro_g3d.c", 176);
    VecFx32 position;
    VecFx32 upVector;
    VecFx32 target;
    G3DActor *actor;

    g3d->heapId = heapId;
    g3d->graphic = graphic;
    g3d->manager = GFL_G3DMgrCreate(10, 16, heapId);
    position = sPosition;
    upVector = sUpVector;
    target = sTarget;
    g3d->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, FX32_CONST(6), -FX32_CONST(6), -FX32_CONST(8),
                                      FX32_CONST(8), FX32_ONE, FX32_CONST(1024), FX32_ONE, &position, &upVector, &target,
                                      heapId);
    if (mode != INTRO_MODE_PLAYER_NAMED && mode != INTRO_MODE_RIVAL_NAMED) {
        g3d->scene = GFL_G3DMgrNewScene(g3d->manager, &sSceneSetup);
        actor = GFL_G3DMgrGetActor(g3d->manager, g3d->scene);
        GFL_G3DActorBindAnm(actor, 3);
        GFL_G3DActorBindAnm(actor, 4);
        GFL_G3DActorUnbindAnm(actor, 0);
        GFL_G3DActorUnbindAnm(actor, 1);
        GFL_G3DActorUnbindAnm(actor, 2);
    } else {
        g3d->scene = SCENE_NONE;
    }
    g3d->unk18 = TRUE;
    return g3d;
}

void IntroG3d_Free(IntroG3d *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    if (g3d->scene != SCENE_NONE) {
        GFL_G3DMgrDeleteScene(g3d->manager, g3d->scene);
    }
    GFL_G3DMgrFree(g3d->manager);
    GFL_HeapFree(g3d);
}

void IntroG3d_Draw(IntroG3d *g3d) {
    SRTMatrix transform;

    GFL_G3DCameraFlush(g3d->camera);
    if (g3d->visible && g3d->unk18) {
        transform.translation.x = 0;
        transform.translation.y = 0;
        transform.translation.z = 0;
        transform.scale.x = FX32_ONE;
        transform.scale.y = FX32_ONE;
        transform.scale.z = FX32_ONE;
        MTX_Identity33_(&transform.rotation);
        GFL_G3DSysDrawObj(GFL_G3DMgrGetActor(g3d->manager, g3d->scene), &transform);
    }
}

BOOL IntroG3d_Open(IntroG3d *g3d) {
    G3DActor *actor;
    BOOL playing3;
    BOOL playing4;

    switch (g3d->state) {
    case 0:
        gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 1, 9, 0, 0);
        GFL_SndSEPlay(SEQ_SE_OPEN2);
        g3d->state++;
    case 1:
        actor = GFL_G3DMgrGetActor(g3d->manager, g3d->scene);
        playing3 = GFL_G3DActorStepAnmFrame(actor, 3, FX32_ONE);
        playing4 = GFL_G3DActorStepAnmFrame(actor, 4, FX32_ONE);
        if (!playing3 && !playing4) {
            GFL_G3DActorUnbindAnm(actor, 3);
            GFL_G3DActorUnbindAnm(actor, 4);
            GFL_G3DActorBindAnm(actor, 0);
            GFL_G3DActorBindAnm(actor, 1);
            GFL_G3DActorBindAnm(actor, 2);
            IntroG3d_SetFrame(g3d, 60);
            g3d->state++;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

void IntroG3d_SetVisible(IntroG3d *g3d, BOOL visible) {
    g3d->visible = visible;
}

void IntroG3d_SetMode(IntroG3d *g3d, u32 mode) {
    g3d->mode = mode;
    g3d->unk20 = 0;
    switch (mode) {
    case 0:
        g3d->frame = 60;
        g3d->mode = 1;
        break;
    case 1:
        g3d->frame = 70;
        break;
    case 2:
        g3d->frame = 50;
        break;
    case 3:
        g3d->frame = 50;
        break;
    case 4:
        g3d->frame = 70;
        break;
    }
}

BOOL IntroG3d_Animate(IntroG3d *g3d) {
    switch (g3d->mode) {
    case 0:
        break;
    case 1:
        g3d->frame--;
        IntroG3d_SetFrame(g3d, g3d->frame);
        if (g3d->frame == 50) {
            return TRUE;
        }
        break;
    case 2:
        g3d->frame++;
        IntroG3d_SetFrame(g3d, g3d->frame);
        if (g3d->frame == 70) {
            return TRUE;
        }
        break;
    case 3:
        g3d->frame--;
        IntroG3d_SetFrame(g3d, g3d->frame);
        if (g3d->frame == 0) {
            return TRUE;
        }
        break;
    case 4:
        g3d->frame++;
        IntroG3d_SetFrame(g3d, g3d->frame);
        if (g3d->frame == 120) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL IntroG3d_AnimateBack(IntroG3d *g3d) {
    if (g3d->mode == 3) {
        IntroG3d_SetFrame(g3d, ++g3d->frame);
    } else {
        IntroG3d_SetFrame(g3d, --g3d->frame);
    }
    if (g3d->frame == 60) {
        return TRUE;
    }
    return FALSE;
}

void IntroG3d_SetFrame(IntroG3d *g3d, u32 frame) {
    G3DActor *actor = GFL_G3DMgrGetActor(g3d->manager, g3d->scene);
    fx32 value = FX32_CONST(frame);

    GFL_G3DActorSetAnmFrame(actor, 0, &value);
    GFL_G3DActorSetAnmFrame(actor, 1, &value);
    GFL_G3DActorSetAnmFrame(actor, 2, &value);
}
