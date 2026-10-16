#include "types.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/gfd.h"
#include "spl/spl.h"
#include "spl_internal.h"

// The manager: the resource file's emitters and textures, the pools of emitters and particles, and the update and
// drawing of the active emitters

// VRAM for the textures and palettes from NitroSystem's default managers, as addresses
static u32 SPLManager_AllocTexVram(u32 size, BOOL is4x4comp) {
    return NNS_GfdGetTexKeyAddr(NNS_GfdAllocTexVram(size, is4x4comp, 0));
}

static u32 SPLManager_AllocPlttVram(u32 size, BOOL is4pltt) {
    return NNS_GfdGetPlttKeyAddr(NNS_GfdAllocPlttVram(size, is4pltt, 0));
}

SPLManager *SPLManager_New(SPLAllocFunc alloc, u16 maxEmitters, u16 maxParticles, u16 fixPolyID, u16 minPolyID,
                           u16 maxPolyID) {
    SPLManager *mgr = alloc(sizeof(SPLManager));
    SPLEmitter *emitters;
    SPLParticle *particles;
    int i;

    MI_CpuFill8(mgr, 0, sizeof(SPLManager));
    mgr->maxEmitters = maxEmitters;
    mgr->maxParticles = maxParticles;
    mgr->minPolygonID = minPolyID;
    mgr->maxPolygonID = maxPolyID;
    mgr->currentPolygonID = mgr->minPolygonID;
    mgr->fixPolygonID = fixPolyID;
    mgr->reverseDrawOrder = FALSE;
    mgr->reserved = 0;
    mgr->alloc = alloc;
    mgr->activeEmitters.count = 0;
    mgr->inactiveEmitters.count = 0;
    mgr->unusedParticles.count = 0;
    mgr->activeEmitters.first = NULL;
    mgr->inactiveEmitters.first = NULL;
    mgr->unusedParticles.first = NULL;
    mgr->polygonAttr = 0;
    mgr->frame = 0;

    emitters = alloc(maxEmitters * sizeof(SPLEmitter));
    MI_CpuFill8(emitters, 0, maxEmitters * sizeof(SPLEmitter));
    for (i = 0; i < maxEmitters; i++) {
        SPLList_PushFront((SPLList *)&mgr->inactiveEmitters, (SPLNode *)&emitters[i]);
    }

    particles = alloc(maxParticles * sizeof(SPLParticle));
    MI_CpuFill8(particles, 0, maxParticles * sizeof(SPLParticle));
    for (i = 0; i < maxParticles; i++) {
        SPLList_PushFront(&mgr->unusedParticles, (SPLNode *)&particles[i]);
    }

    mgr->resources = NULL;
    mgr->textures = NULL;
    mgr->resourceCount = mgr->textureCount = 0;
    return mgr;
}

// Points the resources into the file, which must stay loaded, at their header, animations, child resource and
// behaviors, each there only if the header's flags say so
void SPLManager_LoadResources(SPLManager *mgr, const void *data) {
    const SPLFileHeader *file = data;
    u32 offset = sizeof(SPLFileHeader);
    int i;

    mgr->resourceCount = file->resourceCount;
    mgr->textureCount = file->textureCount;
    mgr->resources = mgr->alloc(mgr->resourceCount * sizeof(SPLResource));
    MI_CpuFill8(mgr->resources, 0, mgr->resourceCount * sizeof(SPLResource));

    for (i = 0; i < mgr->resourceCount; i++) {
        SPLResource *resource;
        SPLResourceFlags flags;

        resource = &mgr->resources[i];
        resource->header = (SPLResourceHeader *)((u8 *)data + offset);
        flags = resource->header->flags;
        offset += sizeof(SPLResourceHeader);

        if (flags.hasScaleAnim) {
            resource->scaleAnim = (SPLScaleAnim *)((u8 *)data + offset);
            offset += sizeof(SPLScaleAnim);
        } else {
            resource->scaleAnim = NULL;
        }
        if (flags.hasColorAnim) {
            resource->colorAnim = (SPLColorAnim *)((u8 *)data + offset);
            offset += sizeof(SPLColorAnim);
        } else {
            resource->colorAnim = NULL;
        }
        if (flags.hasAlphaAnim) {
            resource->alphaAnim = (SPLAlphaAnim *)((u8 *)data + offset);
            offset += sizeof(SPLAlphaAnim);
        } else {
            resource->alphaAnim = NULL;
        }
        if (flags.hasTextureAnim) {
            resource->textureAnim = (SPLTextureAnim *)((u8 *)data + offset);
            offset += sizeof(SPLTextureAnim);
        } else {
            resource->textureAnim = NULL;
        }
        if (flags.hasChildResource) {
            resource->childResource = (SPLChildResource *)((u8 *)data + offset);
            offset += sizeof(SPLChildResource);
        } else {
            resource->childResource = NULL;
        }

        resource->behaviorCount = flags.hasGravityBehavior + flags.hasRandomBehavior + flags.hasMagnetBehavior +
                                  flags.hasSpinBehavior + flags.hasCollisionPlaneBehavior +
                                  flags.hasConvergenceBehavior;
        if (resource->behaviorCount != 0) {
            SPLBehavior *behavior;

            resource->behaviors = mgr->alloc(resource->behaviorCount * sizeof(SPLBehavior));
            behavior = resource->behaviors;
            if (flags.hasGravityBehavior) {
                behavior->object = (u8 *)data + offset;
                behavior->applyFunc = SPLBehavior_ApplyGravity;
                offset += sizeof(SPLGravityBehavior);
                behavior++;
            }
            if (flags.hasRandomBehavior) {
                behavior->object = (u8 *)data + offset;
                behavior->applyFunc = SPLBehavior_ApplyRandom;
                offset += sizeof(SPLRandomBehavior);
                behavior++;
            }
            if (flags.hasMagnetBehavior) {
                behavior->object = (u8 *)data + offset;
                behavior->applyFunc = SPLBehavior_ApplyMagnet;
                offset += sizeof(SPLMagnetBehavior);
                behavior++;
            }
            if (flags.hasSpinBehavior) {
                behavior->object = (u8 *)data + offset;
                behavior->applyFunc = SPLBehavior_ApplySpin;
                offset += sizeof(SPLSpinBehavior);
                behavior++;
            }
            if (flags.hasCollisionPlaneBehavior) {
                behavior->object = (u8 *)data + offset;
                behavior->applyFunc = SPLBehavior_ApplyCollisionPlane;
                offset += sizeof(SPLCollisionPlaneBehavior);
                behavior++;
            }
            if (flags.hasConvergenceBehavior) {
                behavior->object = (u8 *)data + offset;
                behavior->applyFunc = SPLBehavior_ApplyConvergence;
                offset += sizeof(SPLConvergenceBehavior);
            }
        } else {
            resource->behaviors = NULL;
        }
    }

    mgr->textures = mgr->alloc(mgr->textureCount * sizeof(SPLTexture));
    MI_CpuFill8(mgr->textures, 0, mgr->textureCount * sizeof(SPLTexture));
    for (i = 0; i < mgr->textureCount; i++) {
        SPLTexture *entry = &mgr->textures[i];
        const SPLTextureResource *texture = (const SPLTextureResource *)((u8 *)data + offset);

        entry->data = texture;
        entry->width = 1 << (texture->param.s + 3);
        entry->height = 1 << (texture->param.t + 3);
        entry->param = texture->param;
        offset += texture->resourceSize;
    }
}

BOOL SPLManager_UploadTexturesEx(SPLManager *mgr, SPLTexVRAMAllocFunc vramAlloc) {
    int i;

    GX_BeginLoadTex();
    for (i = 0; i < mgr->textureCount; i++) {
        SPLTexture *texture = &mgr->textures[i];
        const SPLTextureResource *data = texture->data;

        if (data->param.useSharedTexture) {
            texture->texAddr = mgr->textures[data->param.sharedTexID].texAddr;
        } else {
            u32 addr = vramAlloc(data->textureSize, data->param.format == GX_TEXFMT_COMP4x4 ? TRUE : FALSE);

            GX_LoadTex((u8 *)texture->data + sizeof(SPLTextureResource), addr, data->textureSize);
            texture->texAddr = addr;
        }
    }
    GX_EndLoadTex();
    return TRUE;
}

BOOL SPLManager_UploadPalettesEx(SPLManager *mgr, SPLPalVRAMAllocFunc vramAlloc) {
    int i;

    GX_BeginLoadTexPltt();
    for (i = 0; i < mgr->textureCount; i++) {
        SPLTexture *texture = &mgr->textures[i];
        const SPLTextureResource *data = texture->data;
        u32 addr = 0;

        if (data->paletteSize != 0) {
            addr = vramAlloc(data->paletteSize, data->param.format == GX_TEXFMT_PLTT4 ? TRUE : FALSE);
            GX_LoadTexPltt((u8 *)texture->data + data->paletteOffset, addr, data->paletteSize);
        }
        texture->palAddr = addr;
    }
    GX_EndLoadTexPltt();
    return TRUE;
}

BOOL SPLManager_UploadTextures(SPLManager *mgr) {
    return SPLManager_UploadTexturesEx(mgr, SPLManager_AllocTexVram);
}

BOOL SPLManager_UploadPalettes(SPLManager *mgr) {
    return SPLManager_UploadPalettesEx(mgr, SPLManager_AllocPlttVram);
}

void SPLManager_Update(SPLManager *mgr) {
    SPLEmitter *emitter;
    SPLEmitter *next;

    for (emitter = mgr->activeEmitters.first; emitter != NULL; emitter = next) {
        SPLResourceHeader *header = emitter->resource->header;

        next = emitter->next;
        if (!emitter->state.started && emitter->age >= header->startDelay) {
            emitter->state.started = TRUE;
            emitter->age = 0;
        }

        // Emitters with an update cycle update on one frame of every two
        if (!emitter->state.paused && (emitter->updateCycle == 0 || mgr->frame == emitter->updateCycle - 1)) {
            SPLEmitter_Update(mgr, emitter);
        }

        if ((header->flags.autoTerminate && header->emitterLifeTime != 0 && emitter->state.started &&
             emitter->age > header->emitterLifeTime) ||
            emitter->state.terminate) {
            if (emitter->particles.count == 0 && emitter->childParticles.count == 0) {
                SPLList_PushFront((SPLList *)&mgr->inactiveEmitters,
                                  SPLList_Erase((SPLList *)&mgr->activeEmitters, (SPLNode *)emitter));
            }
        }
    }

    mgr->frame++;
    if (mgr->frame > 1) {
        mgr->frame = 0;
    }
}

void SPLManager_Draw(SPLManager *mgr, const MtxFx43 *viewMatrix) {
    SPLEmitter *emitter;

    G3X_AlphaBlend(TRUE);
    mgr->viewMatrix = viewMatrix;
    if (!mgr->reverseDrawOrder) {
        for (emitter = mgr->activeEmitters.first; emitter != NULL; emitter = emitter->next) {
            mgr->drawEmitter = emitter;
            if (!emitter->state.hidden) {
                SPLEmitter_Draw(mgr);
            }
        }
    } else {
        for (emitter = mgr->activeEmitters.last; emitter != NULL; emitter = emitter->prev) {
            mgr->drawEmitter = emitter;
            if (!emitter->state.hidden) {
                SPLEmitter_Draw(mgr);
            }
        }
    }
}

SPLEmitter *SPLManager_CreateEmitter(SPLManager *mgr, int resourceID, const VecFx32 *pos) {
    SPLEmitter *emitter = NULL;

    if (mgr->inactiveEmitters.first != NULL) {
        emitter = (SPLEmitter *)SPLList_PopFront((SPLList *)&mgr->inactiveEmitters);
        SPLEmitter_Init(emitter, &mgr->resources[resourceID], pos);
        SPLList_PushFront((SPLList *)&mgr->activeEmitters, (SPLNode *)emitter);
        if (emitter->resource->header->flags.autoTerminate) {
            emitter = NULL;
        }
    }
    return emitter;
}

// Creates an emitter at the origin, which the callback can set up before it is made active
SPLEmitter *SPLManager_CreateEmitterWithCallback(SPLManager *mgr, int resourceID, SPLEmitterCallback initCallback) {
    SPLEmitter *emitter = NULL;

    if (mgr->inactiveEmitters.first != NULL) {
        VecFx32 pos = { 0, 0, 0 };

        emitter = (SPLEmitter *)SPLList_PopFront((SPLList *)&mgr->inactiveEmitters);
        SPLEmitter_Init(emitter, &mgr->resources[resourceID], &pos);
        if (initCallback != NULL) {
            initCallback(emitter);
        }
        SPLList_PushFront((SPLList *)&mgr->activeEmitters, (SPLNode *)emitter);
        if (emitter->resource->header->flags.autoTerminate) {
            emitter = NULL;
        }
    }
    return emitter;
}

void SPLManager_DeleteEmitter(SPLManager *mgr, SPLEmitter *emitter) {
    SPLNode *node;

    while ((node = SPLList_PopFront(&emitter->particles)) != NULL) {
        SPLList_PushFront(&mgr->unusedParticles, node);
    }
    while ((node = SPLList_PopFront(&emitter->childParticles)) != NULL) {
        SPLList_PushFront(&mgr->unusedParticles, node);
    }
    SPLList_Erase((SPLList *)&mgr->activeEmitters, (SPLNode *)emitter);
    SPLList_PushFront((SPLList *)&mgr->inactiveEmitters, (SPLNode *)emitter);
}

void SPLManager_DeleteAllEmitters(SPLManager *mgr) {
    SPLEmitter *emitter = mgr->activeEmitters.first;
    SPLEmitter *next;

    while (emitter != NULL) {
        next = emitter->next;
        SPLManager_DeleteEmitter(mgr, emitter);
        emitter = next;
    }
}
