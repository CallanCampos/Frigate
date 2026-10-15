#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "demo/demo_particle.h"
#include "demo/egg_demo.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "pml/poke_party.h"
#include "system/mcss.h"

// The egg and the Pokémon that hatches from it: the egg's sprite shakes with sounds until it turns white and a 3D egg
// cracks open around it with particles, then the Pokémon fades in from white and cries. Manaphy's egg has its own
// sounds and timing

// The frame at which the egg hatches
#define HATCH_FRAME 380
#define HATCH_FRAME_MANAPHY 300
// The egg's cracking sound repeats from this frame until it hatches, with a wait between plays until CRACK_SE_WAIT_END
#define CRACK_SE_START 150
#define CRACK_SE_WAIT_END 309

// The steps of the view
enum {
    VIEW_WAIT_START,
    VIEW_SHAKE,
    VIEW_WAIT_WHITE,
    VIEW_WAIT_CRACK,
    VIEW_HATCHED,
    VIEW_SHOW_POKEMON,
    VIEW_WAIT_REVEAL,
    VIEW_WAIT_CRY,
    VIEW_CRY,
    VIEW_DONE,
};

// What the 3D egg's cracking animations are doing
enum {
    EGG_3D_IDLE,
    EGG_3D_CRACK,
    EGG_3D_CRACKED,
    EGG_3D_CLOSE,
};

typedef struct {
    u16 actor;
    SRTMatrix srt;
    BOOL visible;
} ViewActor;

struct EggDemoView {
    HeapID heapId;
    PartyPkm *pkm;
    u16 species;
    s32 state;
    BOOL started;
    BOOL hatched;
    u32 frame;
    BOOL crackSEPlaying;
    s32 crackSEPlayer;
    u32 crackSEWait;
    u32 manaphySEStep;
    TCB *vblankTask;
    MCSSSystem *mcssSystem;
    MCSS *mcss;
    // Set when the sprite's animation stops at its last frame
    BOOL animationEnded;
    BOOL alphaFading;
    u8 targetAlpha;
    DemoParticle *particle;
    G3DManager *g3dManager;
    u16 scene;
    u16 eggActor;
    u16 actorCount;
    ViewActor *actors;
    u32 frame3D;
    u32 egg3DState;
};

// Where to put some Pokémon's sprites once they hatch
typedef struct {
    u32 species;
    f32 x;
    f32 y;
} SpritePosition;

static ViewActor *EggDemoView_CreateActors(HeapID heapId, u16 count);
static void EggDemoView_FreeActors(ViewActor *actors);
static void EggDemoView_VBlank(TCB *tcb, void *data);
static void EggDemoView_InitMcss(EggDemoView *view);
static void EggDemoView_FreeMcss(EggDemoView *view);
static void EggDemoView_AddMcss(EggDemoView *view);
static void EggDemoView_RemoveMcss(EggDemoView *view);
static void EggDemoView_HideMcss(EggDemoView *view);
static void EggDemoView_SetAnimation(EggDemoView *view, u32 animation, BOOL stopAtEnd);
static void EggDemoView_OnAnimationEnd(u32 param, fx32 frame);
static void EggDemoView_UpdateAlpha(EggDemoView *view);
static void EggDemoView_FadeToWhite(EggDemoView *view);
static BOOL EggDemoView_IsFadeDone(EggDemoView *view);
static void EggDemoView_FadeFromWhite(EggDemoView *view);
static DemoParticle *EggDemoParticle_Create(HeapID heapId, u16 count, const DemoParticleEvent *events);
static void EggDemoParticle_Free(DemoParticle *particle);
static void EggDemoParticle_Update(DemoParticle *particle);
static void EggDemoParticle_Draw(DemoParticle *particle);
static void EggDemoParticle_Start(DemoParticle *particle);
static void EggDemoView_Init3D(EggDemoView *view);
static void EggDemoView_Free3D(EggDemoView *view);
static void EggDemoView_Draw3D(EggDemoView *view);
static void EggDemoView_AddScene(EggDemoView *view);
static void EggDemoView_DeleteScene(EggDemoView *view);
static void EggDemoView_Update3D(EggDemoView *view);
static void EggDemoView_CrackEgg(EggDemoView *view);
static BOOL EggDemoView_IsEggCracked(EggDemoView *view);
static void EggDemoView_CloseEgg(EggDemoView *view);

// Not referenced. Its value, FX32_ONE, is the step of the egg's looping animation and the scale of its actor
const fx32 EGG_DEMO_VIEW_UNK_FX32 = FX32_ONE;

static const G3DSceneResourceSetup sEggResources[] = {
    { ARCID_EGG_DEMO, 8, 0 },
    { ARCID_EGG_DEMO, 9, 0 },
    { ARCID_EGG_DEMO, 6, 0 },
    { ARCID_EGG_DEMO, 7, 0 },
};

static const G3DSceneAnimationSetup sEggAnimations[] = { { 1, 0 }, { 2, 0 }, { 3, 0 } };

static const G3DSceneActorSetup sEggActors[] = {
    { 0, 0, 0, 0, sEggAnimations, NELEMS(sEggAnimations) },
};

static const G3DSceneSetup sEggScene = { sEggResources, NELEMS(sEggResources), sEggActors, NELEMS(sEggActors) };

static const DemoParticleEvent sParticleEvents[] = {
    { HATCH_FRAME, 0, 0 },
    { HATCH_FRAME, 0, 1 },
    { HATCH_FRAME, 0, 2 },
};

static const DemoParticleResource sParticleResources[DEMO_PARTICLE_UNIT_COUNT] = { { ARCID_EGG_DEMO, 10 } };

static const DemoParticleEvent sManaphyParticleEvents[] = {
    { 0, 0, 3 },
    { 0, 0, 4 },
    { HATCH_FRAME_MANAPHY, 0, 0 },
    { HATCH_FRAME_MANAPHY, 0, 1 },
    { HATCH_FRAME_MANAPHY, 0, 2 },
};

static const SpritePosition sSpritePositions[] = {
    { SPECIES_CYNDAQUIL, -0.5f, -190.0f },
    { SPECIES_SLUGMA, -0.5f, -189.9f },
    { SPECIES_WYNAUT, 0.0f, -189.8f },
    { SPECIES_MIENFOO, 0.0f, -190.2f },
};

static ViewActor *EggDemoView_CreateActors(HeapID heapId, u16 count) {
    ViewActor *actors = GFL_HeapAllocate(heapId, sizeof(ViewActor) * count, TRUE, "egg_demo_view.c", 283);
    u16 i;

    for (i = 0; i < count; i++) {
        actors[i].srt.translation.x = 0;
        actors[i].srt.translation.y = 0;
        actors[i].srt.translation.z = 0;
        actors[i].srt.scale.x = FX32_ONE;
        actors[i].srt.scale.y = FX32_ONE;
        actors[i].srt.scale.z = FX32_ONE;
        MTX_Identity33_(&actors[i].srt.rotation);
    }
    return actors;
}

static void EggDemoView_FreeActors(ViewActor *actors) {
    GFL_HeapFree(actors);
}

EggDemoView *EggDemoView_Create(HeapID heapId, PartyPkm *pkm) {
    EggDemoView *view = GFL_HeapAllocate(heapId, sizeof(EggDemoView), TRUE, "egg_demo_view.c", 433);

    view->heapId = heapId;
    view->pkm = pkm;
    view->species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    view->state = VIEW_WAIT_START;
    view->started = FALSE;
    view->hatched = FALSE;
    view->frame = 0;
    view->crackSEPlaying = FALSE;
    view->crackSEWait = 0;
    view->manaphySEStep = 0;
    view->vblankTask = GFL_VBlankTCBAdd(EggDemoView_VBlank, view, 1);
    EggDemoView_InitMcss(view);
    EggDemoView_AddMcss(view);
    if (view->species == SPECIES_MANAPHY) {
        view->particle = EggDemoParticle_Create(view->heapId, NELEMS(sManaphyParticleEvents), sManaphyParticleEvents);
    } else {
        view->particle = EggDemoParticle_Create(view->heapId, NELEMS(sParticleEvents), sParticleEvents);
    }
    EggDemoView_Init3D(view);
    EggDemoView_AddScene(view);
    return view;
}

void EggDemoView_Free(EggDemoView *view) {
    EggDemoView_DeleteScene(view);
    EggDemoView_Free3D(view);
    EggDemoParticle_Free(view->particle);
    EggDemoView_RemoveMcss(view);
    EggDemoView_FreeMcss(view);
    GFL_TCBRemove(view->vblankTask);
    GFL_HeapFree(view);
}

void EggDemoView_Update(EggDemoView *view) {
    u32 hatchFrame;

    switch (view->state) {
    case VIEW_WAIT_START:
        if (view->started) {
            view->state = VIEW_SHAKE;
            view->frame = 0;
            EggDemoView_SetAnimation(view, 1, TRUE);
            EggDemoParticle_Start(view->particle);
        }
        break;
    case VIEW_SHAKE:
        if (view->species == SPECIES_MANAPHY) {
            hatchFrame = HATCH_FRAME_MANAPHY;
        } else {
            hatchFrame = HATCH_FRAME;
        }
        view->frame++;
        if (view->species == SPECIES_MANAPHY) {
            switch (view->manaphySEStep) {
            case 0:
                if (view->frame == 10) {
                    GFL_SndSEPlay(SEQ_SE_EDEMO_02);
                    view->manaphySEStep = 1;
                }
                break;
            case 1:
                if (GFL_SndPlayerIsActiveAny() == FALSE) {
                    GFL_SndSEPlay(SEQ_SE_EDEMO_03);
                    view->manaphySEStep = 2;
                }
                break;
            case 2:
                if (GFL_SndPlayerIsActiveAny() == FALSE) {
                    view->manaphySEStep = 3;
                }
                break;
            case 3:
                break;
            }
        } else {
            if (view->frame == 20) {
                GFL_SndSEPlay(SEQ_SE_EDEMO_01);
            } else if (view->frame == 109) {
                GFL_SndSEPlay(SEQ_SE_EDEMO_01);
            } else if (view->frame == 218) {
                GFL_SndSEPlay(SEQ_SE_W181_01);
            }
            if (view->frame == 10) {
                GFL_SndSEPlay(SEQ_SE_EDEMO_05);
            } else if (view->frame == 15) {
                GFL_SndSEPlay(SEQ_SE_EDEMO_05);
            } else if (view->frame == 85) {
                GFL_SndSEPlay(SEQ_SE_EDEMO_05);
            } else if (view->frame == 90) {
                GFL_SndSEPlay(SEQ_SE_EDEMO_05);
            }
            if (view->crackSEPlaying && GFL_SndPlayerIsActive(view->crackSEPlayer) == FALSE) {
                view->crackSEPlaying = FALSE;
                if (view->frame >= CRACK_SE_START && view->frame < CRACK_SE_WAIT_END) {
                    view->crackSEWait = 20;
                } else if (view->frame > CRACK_SE_WAIT_END) {
                    view->crackSEWait = 0;
                }
            }
            if (view->frame >= CRACK_SE_START && view->frame < hatchFrame) {
                if (view->crackSEPlaying == FALSE) {
                    if (view->crackSEWait == 0) {
                        view->crackSEPlayer = GFL_SndSeqGetPlayerIndex(SEQ_SE_EDEMO_04);
                        GFL_SEPlayKeepVol(SEQ_SE_EDEMO_04, view->crackSEPlayer);
                        view->crackSEPlaying = TRUE;
                    } else {
                        view->crackSEWait--;
                    }
                }
            } else if (view->frame == hatchFrame && view->crackSEPlaying &&
                       GFL_SndPlayerIsActive(view->crackSEPlayer)) {
                GFL_SndPlayerStop(view->crackSEPlayer);
                view->crackSEPlaying = FALSE;
            }
        }
        if (view->frame == hatchFrame) {
            EggDemoView_FadeToWhite(view);
            EggDemoView_CrackEgg(view);
            GFL_SndSEPlay(SEQ_SE_TDEMO_011);
            view->state = VIEW_WAIT_WHITE;
        }
        break;
    case VIEW_WAIT_WHITE:
        if (EggDemoView_IsFadeDone(view)) {
            view->state = VIEW_WAIT_CRACK;
        }
        break;
    case VIEW_WAIT_CRACK:
        if (EggDemoView_IsEggCracked(view)) {
            EggDemoView_HideMcss(view);
            view->hatched = TRUE;
            view->state = VIEW_HATCHED;
        }
        break;
    case VIEW_HATCHED:
        view->hatched = FALSE;
        break;
    case VIEW_WAIT_REVEAL:
        if (++view->frame >= 120) {
            EggDemoView_FadeFromWhite(view);
            EggDemoView_CloseEgg(view);
            view->state = VIEW_WAIT_CRY;
            view->frame = 0;
        }
        break;
    case VIEW_WAIT_CRY:
        if (++view->frame >= 65) {
            view->state = VIEW_CRY;
            PokeVoice_Play(PokeParty_GetParam(view->pkm, PKM_PARAM_SPECIES, NULL),
                           PokeParty_GetParam(view->pkm, PKM_PARAM_FORM, NULL), 64, 0, 0, 0, 0, 0);
        }
        break;
    case VIEW_CRY:
        if (PokeVoice_IsPlayingAny() == FALSE) {
            view->state = VIEW_DONE;
        }
        break;
    case VIEW_SHOW_POKEMON:
    case VIEW_DONE:
        break;
    }
    EggDemoView_UpdateAlpha(view);
    MCSSSys_Update(view->mcssSystem);
    EggDemoParticle_Update(view->particle);
    EggDemoView_Update3D(view);
}

void EggDemoView_Draw(EggDemoView *view) {
    MCSSSys_Draw(view->mcssSystem);
    EggDemoView_Draw3D(view);
    EggDemoParticle_Draw(view->particle);
}

void EggDemoView_Start(EggDemoView *view) {
    view->started = TRUE;
}

BOOL EggDemoView_IsHatched(EggDemoView *view) {
    return view->hatched;
}

void EggDemoView_ShowPokemon(EggDemoView *view, PartyPkm *pkm) {
    EggDemoView_RemoveMcss(view);
    view->pkm = pkm;
    EggDemoView_AddMcss(view);
    EggDemoView_SetAnimation(view, 0, FALSE);
    MCSS_Hide(view->mcss);
    func_0201ae2c(view->mcss, 16, 16, 0, GX_RGB(31, 31, 31));
    view->state = VIEW_SHOW_POKEMON;
}

BOOL EggDemoView_IsWhite(EggDemoView *view) {
    return func_0201aee8(view->mcss) == FALSE;
}

void EggDemoView_Reveal(EggDemoView *view) {
    view->state = VIEW_WAIT_REVEAL;
    view->frame = 0;
}

BOOL EggDemoView_IsDone(EggDemoView *view) {
    return view->state >= VIEW_DONE;
}

static void EggDemoView_VBlank(TCB *tcb, void *data) {
}

static void EggDemoView_InitMcss(EggDemoView *view) {
    view->mcssSystem = MCSSSys_Create(1, view->heapId);
    func_0201aefc(view->mcssSystem, 0x30000);
    func_0201aacc(view->mcssSystem);
    view->alphaFading = FALSE;
    view->targetAlpha = 0;
}

static void EggDemoView_FreeMcss(EggDemoView *view) {
    MCSSSys_Free(view->mcssSystem);
}

static void EggDemoView_AddMcss(EggDemoView *view) {
    u8 i = 0;
    u32 species;
    VecFx32 pos;
    VecFx32 scale;
    VecFx32 offset;
    f32 height;
    f32 top;
    f32 left;

    view->mcss = func_0201c14c(view->mcssSystem, view->pkm, 0, 0, FX32_CONST(-190), FX32_CONST(-800));
    func_0201aecc(view->mcss, 1);
    species = PokeParty_GetParam(view->pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
    if (species != SPECIES_EGG) {
        for (i = 0; i < NELEMS(sSpritePositions); i++) {
            if (species == sSpritePositions[i].species) {
                pos.x = FX32_CONST(sSpritePositions[i].x);
                pos.y = FX32_CONST(sSpritePositions[i].y);
                pos.z = FX32_CONST(-800);
                MCSS_SetPosition(view->mcss, &pos);
                break;
            }
        }
    }
    scale.x = FX32_CONST(16);
    scale.y = FX32_CONST(16);
    scale.z = FX32_ONE;
    MCSS_SetScale(view->mcss, &scale);
    height = func_0201ade8(view->mcss);
    top = func_0201adf8(view->mcss);
    left = func_0201adf0(view->mcss);
    if (height > 96.0f) {
        height = 96.0f;
    }
    height = (96.0f - height) / 2.0f + top;
    left = 0.0f - left;
    offset.x = FX32_CONST(left);
    offset.y = FX32_CONST(height);
    offset.z = 0;
    func_0201ab54(view->mcss, &offset);
    MCSS_PauseAnimation(view->mcss);
}

static void EggDemoView_RemoveMcss(EggDemoView *view) {
    MCSS_Hide(view->mcss);
    MCSSSys_Remove(view->mcssSystem, view->mcss);
}

static void EggDemoView_HideMcss(EggDemoView *view) {
    MCSS_Hide(view->mcss);
}

static void EggDemoView_SetAnimation(EggDemoView *view, u32 animation, BOOL stopAtEnd) {
    NNSG2dMultiCellAnimation *mcAnim;

    MCSS_SetAnimation(view->mcss, animation);
    mcAnim = func_0201adc4(view->mcss);
    NNS_G2dRestartMCAnimation(mcAnim);
    MCSS_ResumeAnimation(view->mcss);
    if (stopAtEnd) {
        NNS_G2dSetAnimCtrlCallBackFunctor(&mcAnim->animCtrl, NNS_G2D_ANMCALLBACKTYPE_LAST_FRM, (u32)view,
                                          EggDemoView_OnAnimationEnd);
        view->animationEnded = FALSE;
    } else {
        func_0201c290(view->mcss);
    }
}

static void EggDemoView_OnAnimationEnd(u32 param, fx32 frame) {
    EggDemoView *view = (EggDemoView *)param;

    MCSS_PauseAnimation(view->mcss);
    view->animationEnded = TRUE;
}

// Steps the sprite's alpha toward targetAlpha
static void EggDemoView_UpdateAlpha(EggDemoView *view) {
    u8 alpha;

    if (view->alphaFading) {
        alpha = func_0201ae88(view->mcss);
        if (alpha > view->targetAlpha) {
            alpha--;
        } else {
            alpha++;
        }
        MCSS_SetAlpha(view->mcss, alpha);
        if (alpha == view->targetAlpha) {
            view->alphaFading = FALSE;
        }
    }
}

static void EggDemoView_FadeToWhite(EggDemoView *view) {
    MCSS_PauseAnimation(view->mcss);
    func_0201ae2c(view->mcss, 0, 16, 0, GX_RGB(31, 31, 31));
}

static BOOL EggDemoView_IsFadeDone(EggDemoView *view) {
    return func_0201aee8(view->mcss) == FALSE;
}

static void EggDemoView_FadeFromWhite(EggDemoView *view) {
    MCSS_Show(view->mcss);
    func_0201ae2c(view->mcss, 16, 0, 2, GX_RGB(31, 31, 31));
}

static DemoParticle *EggDemoParticle_Create(HeapID heapId, u16 count, const DemoParticleEvent *events) {
    // Not used, like the camera target of the evolution demo's copy of this function
    VecFx32 pos = { 0, 0, 0 };
    DemoParticle *particle = GFL_HeapAllocate(heapId, sizeof(DemoParticle), TRUE, "egg_demo_view.c", 1282);
    void *resource;
    u32 i;

    particle->frame = 0;
    particle->index = 0;
    particle->count = count;
    particle->events = events;
    particle->active = FALSE;
    particle->stopTimer = -1;
    func_0204f918(heapId);
    for (i = 0; i < DEMO_PARTICLE_UNIT_COUNT; i++) {
        particle->units[i].system = func_0204f968(particle->units[i].buffer, DEMO_PARTICLE_BUFFER_SIZE, TRUE, heapId);
        resource = func_0204fdf8(sParticleResources[i].arcId, sParticleResources[i].fileId, heapId);
        particle->units[i].resourceCount = func_020503f0(resource);
        func_0204fe04(particle->units[i].system, resource, TRUE, FALSE);
    }
    return particle;
}

static void EggDemoParticle_Free(DemoParticle *particle) {
    func_0204fb4c();
    GFL_HeapFree(particle);
}

// Emits the particles of the events at the current frame
static void EggDemoParticle_Update(DemoParticle *particle) {
    if (particle->active) {
        VecFx32 pos = { 0, FX32_CONST(-0.5), 0 };

        while (particle->index < particle->count) {
            if (particle->frame != particle->events[particle->index].frame) {
                break;
            }
            func_0205006c(particle->units[particle->events[particle->index].unit].system,
                          particle->events[particle->index].emitter, &pos);
            particle->index++;
        }
        particle->frame++;
    }
    if (particle->stopTimer >= 0) {
        if (particle->stopTimer == 0) {
            func_020500b0(particle->units[0].system);
        }
        particle->stopTimer--;
    }
}

static void EggDemoParticle_Draw(DemoParticle *particle) {
    func_0204f954();
}

static void EggDemoParticle_Start(DemoParticle *particle) {
    particle->active = TRUE;
}

static void EggDemoView_Init3D(EggDemoView *view) {
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_MANUAL, GX_BUFFERMODE_Z);
    view->g3dManager = GFL_G3DMgrCreate(4, 1, view->heapId);
    view->actorCount = 0;
}

static void EggDemoView_Free3D(EggDemoView *view) {
    GFL_G3DMgrFree(view->g3dManager);
}

static void EggDemoView_Draw3D(EggDemoView *view) {
    u16 i;
    ViewActor *actor;

    for (i = 0; i < view->actorCount; i++) {
        actor = &view->actors[i];
        if (actor->visible) {
            GFL_G3DSysDrawObj(GFL_G3DMgrGetActor(view->g3dManager, actor->actor), &actor->srt);
        }
    }
}

static void EggDemoView_AddScene(EggDemoView *view) {
    u16 i;
    u16 first;
    ViewActor *actor;
    G3DActor *egg;
    fx32 frame;

    view->scene = GFL_G3DMgrNewScene(view->g3dManager, &sEggScene);
    view->actorCount = 1;
    view->actors = EggDemoView_CreateActors(view->heapId, view->actorCount);
    first = GFL_G3DMgrGetSceneFirstActorIdx(view->g3dManager, view->scene);
    view->eggActor = 0;
    actor = &view->actors[0];
    actor->actor = first;
    actor->srt.translation.x = 0;
    actor->srt.translation.y = 0;
    actor->srt.translation.z = 0;
    actor->visible = TRUE;
    for (i = 0; i < view->actorCount; i++) {
        GFL_G3DActorBindAnm(GFL_G3DMgrGetActor(view->g3dManager, view->actors[i].actor), 0);
    }
    view->frame3D = 0;
    egg = GFL_G3DMgrGetActor(view->g3dManager, view->actors[view->eggActor].actor);
    frame = 0;
    view->egg3DState = EGG_3D_IDLE;
    GFL_G3DActorBindAnm(egg, 1);
    GFL_G3DActorBindAnm(egg, 2);
    GFL_G3DActorSetAnmFrame(egg, 1, &frame);
    GFL_G3DActorSetAnmFrame(egg, 2, &frame);
}

static void EggDemoView_DeleteScene(EggDemoView *view) {
    GFL_G3DMgrDeleteScene(view->g3dManager, view->scene);
    EggDemoView_FreeActors(view->actors);
    view->actorCount = 0;
}

static void EggDemoView_Update3D(EggDemoView *view) {
    u16 i;
    G3DActor *egg;
    BOOL running1;
    BOOL running2;

    for (i = 0; i < view->actorCount; i++) {
        GFL_G3DActorStepAnmFrameLoop(GFL_G3DMgrGetActor(view->g3dManager, view->actors[i].actor), 0, FX32_ONE);
    }
    view->frame3D++;
    egg = GFL_G3DMgrGetActor(view->g3dManager, view->actors[view->eggActor].actor);
    if (view->egg3DState == EGG_3D_CRACK) {
        running1 = GFL_G3DActorStepAnmFrame(egg, 1, FX32_CONST(5));
        running2 = GFL_G3DActorStepAnmFrame(egg, 2, FX32_CONST(5));
        if (running1 == FALSE && running2 == FALSE) {
            view->egg3DState = EGG_3D_CRACKED;
        }
    } else if (view->egg3DState == EGG_3D_CLOSE) {
        running1 = GFL_G3DActorStepAnmFrame(egg, 1, -FX32_CONST(2));
        running2 = GFL_G3DActorStepAnmFrame(egg, 2, -FX32_CONST(2));
        if (running1 == FALSE && running2 == FALSE) {
            view->egg3DState = EGG_3D_IDLE;
        }
    }
}

static void EggDemoView_CrackEgg(EggDemoView *view) {
    view->egg3DState = EGG_3D_CRACK;
}

static BOOL EggDemoView_IsEggCracked(EggDemoView *view) {
    return view->egg3DState == EGG_3D_CRACKED;
}

static void EggDemoView_CloseEgg(EggDemoView *view) {
    view->egg3DState = EGG_3D_CLOSE;
}
