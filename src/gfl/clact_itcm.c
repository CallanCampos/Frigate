#include "types.h"
#include "gfl/clact.h"
#include "gfl/clact_itcm.h"
#include "nitro/fx.h"
#include "nnsys/g2d.h"

// The renderers of the cell actor system's units, which draw each visible actor of a unit with NitroSystem's 2D
// renderer and advance the animations that play by themselves. A unit's sharedParams sets the renderer up once, with
// its first actor, rather than for each actor, and its visibleAnimOnly leaves hidden actors' animations still. The ROM
// embeds no name for this file. The renderers are likely clact.c's own functions, placed in ITCM for speed, and are
// here only because a source file can't span two modules; the name is a guess after it. mainRenderer is swan's name;
// visibleAnimRenderer, sharedParamsRenderer and sharedParamsVisibleAnimRenderer are ours, after it

// Sets the renderer's transforms and the OAM attributes it overwrites to the actor's, and the OAMs' palette to the
// actor's, and returns the hint to begin rendering with
static inline u32 ClActor_SetRendererParams(ClActRenderer *renderer, ClActor *actor) {
    BOOL flipV;
    BOOL flipH;
    u32 hint;
    u8 overwrite;
    // A u8, as clact.c's getter returns it
    u8 bgPriority;
    // Both read before the palettes' locations are; the palette is a u8, as clact.c's getter returns it
    u32 paletteMode;
    u8 palette;

    NNS_G2dSetRndCoreAffineOverwriteMode(&renderer->renderer.rendererCore, actor->affineMode);
    if (actor->affineMode == NNS_G2D_RND_AFFINE_OVERWRITE_NONE) {
        flipV = ClActor_GetFlip(actor, 0);
        flipH = ClActor_GetFlip(actor, 1);
        NNS_G2dSetRndCoreFlipMode(&renderer->renderer.rendererCore, flipH, flipV);
        if (actor->cellOnly) {
            hint = renderer->opzHint | NNS_G2D_RDR_OPZHINT_NOT_SR;
        } else {
            hint = renderer->opzHint;
        }
    } else {
        hint = renderer->opzHint;
    }
    // Overwrites exactly the actor's attributes: its flags on, every other off
    overwrite = actor->overwrite;
    NNS_G2dSetRendererOverwriteEnable(&renderer->renderer, overwrite);
    NNS_G2dSetRendererOverwriteDisable(&renderer->renderer, ~overwrite);
    NNS_G2dSetRendererOverwriteMosaicFlag(&renderer->renderer, actor->mosaic);
    NNS_G2dSetRendererOverwriteOBJMode(&renderer->renderer, actor->objMode);
    bgPriority = actor->bgPriority;
    NNS_G2dSetRendererOverwritePriority(&renderer->renderer, bgPriority);
    paletteMode = actor->paletteMode;
    palette = actor->palette;
    g_ClActSys->palettes[0] = NNS_G2dGetImagePaletteLocation(&actor->pltProxy, NNS_G2D_VRAM_TYPE_2DMAIN) / 32;
    g_ClActSys->palettes[1] = NNS_G2dGetImagePaletteLocation(&actor->pltProxy, NNS_G2D_VRAM_TYPE_2DSUB) / 32;
    g_ClActSys->palettes[0] += palette;
    g_ClActSys->palettes[1] += palette;
    g_ClActSys->paletteMode = paletteMode;
    return hint;
}

// Draws the actor where it is, with its affine transform
static inline void ClActor_Draw(ClActor *actor) {
    NNS_G2dTranslate(FX32_CONST(actor->pos.x), FX32_CONST(actor->pos.y), 0);
    if (actor->affineMode != NNS_G2D_RND_AFFINE_OVERWRITE_NONE) {
        NNS_G2dTranslate(FX32_CONST(actor->affineCenter.x), FX32_CONST(actor->affineCenter.y), 0);
        NNS_G2dScale(actor->scale.x, actor->scale.y, FX32_ONE);
        NNS_G2dRotZ(FX_SinIdx(actor->rotation), FX_CosIdx(actor->rotation));
        NNS_G2dTranslate(-FX32_CONST(actor->affineCenter.x), -FX32_CONST(actor->affineCenter.y), 0);
    }
    switch (actor->anim.type) {
    case CLACT_ANIM_CELL:
        if (actor->cellOnly == TRUE) {
            NNS_G2dDrawCell(actor->anim.cellAnim.pCurrentCell);
        } else {
            NNS_G2dDrawCellAnimation(&actor->anim.cellAnim);
        }
        break;
    case CLACT_ANIM_VRAM_TRANSFER:
        NNS_G2dDrawCellAnimation(&actor->anim.cellAnim);
        break;
    case CLACT_ANIM_MULTICELL:
        NNS_G2dDrawMultiCellAnimation(&actor->anim.mcAnim);
        break;
    }
}

// Advances the actor's animation by its speed, when it plays by itself
static inline void ClActor_AutoAnimate(ClActor *actor) {
    if (actor->autoAnim == TRUE) {
        ClActAnim_SetSpeed(&actor->anim, actor->autoAnimSpeed);
        ClActAnim_Tick(&actor->anim, FX32_ONE);
    }
}

void mainRenderer(ClActUnit *unit) {
    ClActor *actor = unit->actorList;
    ClActRenderer *renderer;

    do {
        if (actor->visible) {
            renderer = unit->renderer;
            NNS_G2dSetRendererImageProxy(&renderer->renderer, &actor->imgProxy, &actor->pltProxy);
            NNS_G2dBeginRenderingEx(&renderer->renderer, ClActor_SetRendererParams(renderer, actor));
            ClActor_Draw(actor);
            NNS_G2dEndRendering();
        }
        ClActor_AutoAnimate(actor);
        actor = actor->next;
    } while (actor != unit->actorList);
}

void visibleAnimRenderer(ClActUnit *unit) {
    ClActor *actor = unit->actorList;
    ClActRenderer *renderer;

    do {
        if (actor->visible) {
            renderer = unit->renderer;
            NNS_G2dSetRendererImageProxy(&renderer->renderer, &actor->imgProxy, &actor->pltProxy);
            NNS_G2dBeginRenderingEx(&renderer->renderer, ClActor_SetRendererParams(renderer, actor));
            ClActor_Draw(actor);
            NNS_G2dEndRendering();
            ClActor_AutoAnimate(actor);
        }
        actor = actor->next;
    } while (actor != unit->actorList);
}

void sharedParamsRenderer(ClActUnit *unit) {
    ClActor *actor = unit->actorList;
    u32 hint;

    NNS_G2dSetRendererImageProxy(&unit->renderer->renderer, &actor->imgProxy, &actor->pltProxy);
    hint = ClActor_SetRendererParams(unit->renderer, actor);
    do {
        if (actor->visible) {
            NNS_G2dBeginRenderingEx(&unit->renderer->renderer, hint);
            ClActor_Draw(actor);
            NNS_G2dEndRendering();
        }
        ClActor_AutoAnimate(actor);
        actor = actor->next;
    } while (actor != unit->actorList);
}

void sharedParamsVisibleAnimRenderer(ClActUnit *unit) {
    ClActor *actor = unit->actorList;
    u32 hint;

    NNS_G2dSetRendererImageProxy(&unit->renderer->renderer, &actor->imgProxy, &actor->pltProxy);
    hint = ClActor_SetRendererParams(unit->renderer, actor);
    do {
        if (actor->visible) {
            NNS_G2dBeginRenderingEx(&unit->renderer->renderer, hint);
            ClActor_Draw(actor);
            NNS_G2dEndRendering();
            ClActor_AutoAnimate(actor);
        }
        actor = actor->next;
    } while (actor != unit->actorList);
}
