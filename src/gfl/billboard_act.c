#include "types.h"
#include "gfl/areaman.h"
#include "gfl/blact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"

// The billboard actor system: runs of materials and actors in a scene, and the animations of faces actors play

typedef BOOL (*BlActAnimCmdFunc)(BlActHandle *handle, const BlActAnimStep *step);

static void BlActTexMat_Reset(BlActTexMat *texMat);
static void BlActHandle_ResetAnim(BlActHandle *handle);
static void BlActHandle_Reset(BlActHandle *handle);
static void BlActSys_UpdateHandleAnim(BlActSys *sys, BlActHandle *handle);
static void BlActSys_TransferFace(BlActSys *sys, BlActTexMat *texMat, u16 face);
static void BlActSys_TransferPltt(BlActSys *sys, BlActTexMat *texMat);
static BOOL BlActAnimStep_IsCmd(const BlActAnimStep *step);
static BOOL BlActHandle_RunCmd(BlActHandle *handle, const BlActAnimStep *step);
static BlActAnimCmdFunc BlActAnim_GetCmdFunc(u32 cmd);
static BOOL BlActAnimCmd_End(BlActHandle *handle, const BlActAnimStep *step);
static BOOL BlActAnimCmd_Change(BlActHandle *handle, const BlActAnimStep *step);
static BOOL BlActAnimCmd_Loop(BlActHandle *handle, const BlActAnimStep *step);
static BOOL BlActAnimCmd_Goto(BlActHandle *handle, const BlActAnimStep *step);

const BlActSceneSetup BLACT_DEFAULT_CONFIG = {
    0,
    0,
    { FX32_ONE, FX32_ONE, FX32_ONE },
    GX_RGB(31, 31, 31),
    GX_RGB(16, 16, 16),
    GX_RGB(16, 16, 16),
    GX_RGB(0, 0, 0),
    63,
    BLACT_ORIGIN_CENTER,
};

BlActSys *BlActSys_Create(u16 texMatCount, u16 handleCount, BlActTransferFunc transfer, HeapID heapId) {
    BlActSys *sys = GFL_HeapAllocate(heapId, sizeof(BlActSys), TRUE, "billboard_act.c", 94);
    BlActSceneSetup setup;
    int i;

    sys->heapId = heapId;
    sys->transfer = transfer;
    sys->texMatCount = texMatCount;
    sys->handleCount = handleCount;
    setup = BLACT_DEFAULT_CONFIG;
    setup.materialCount = sys->texMatCount;
    setup.actorCount = sys->handleCount;
    sys->scene = BlActScene_Create(&setup, heapId);
    sys->texMats = GFL_HeapAllocate(heapId, sys->texMatCount * sizeof(BlActTexMat), TRUE, "billboard_act.c", 119);
    sys->texMatAlloc = GFL_AreaManCreate(sys->texMatCount, heapId);
    sys->handles = GFL_HeapAllocate(heapId, sys->handleCount * sizeof(BlActHandle), TRUE, "billboard_act.c", 123);
    sys->handleAlloc = GFL_AreaManCreate(sys->handleCount, heapId);
    for (i = 0; i < sys->texMatCount; i++) {
        BlActTexMat_Reset(&sys->texMats[i]);
    }
    for (i = 0; i < sys->handleCount; i++) {
        BlActHandle_Reset(&sys->handles[i]);
    }
    return sys;
}

void BlActSys_Free(BlActSys *sys) {
    int i;

    for (i = 0; i < sys->handleCount; i++) {
        if (sys->handles[i].actor != BLACT_NONE) {
            BlActScene_ClearActorMaterial(sys->scene, sys->handles[i].actor);
        }
    }
    GFL_AreaManFree(sys->handleAlloc);
    GFL_HeapFree(sys->handles);
    GFL_AreaManFree(sys->texMatAlloc);
    GFL_HeapFree(sys->texMats);
    BlActScene_Delete(sys->scene);
    GFL_HeapFree(sys);
}

BlActScene *BlActSys_GetScene(BlActSys *sys) {
    return sys->scene;
}

static void BlActTexMat_Reset(BlActTexMat *texMat) {
    texMat->material = BLACT_NONE;
    texMat->texResource = NULL;
}

static void BlActHandle_ResetAnim(BlActHandle *handle) {
    handle->step = 0;
    handle->wait = 0;
    handle->loops = 0;
    handle->cmd = 0;
}

static void BlActHandle_Reset(BlActHandle *handle) {
    handle->actor = BLACT_NONE;
    handle->anims = NULL;
    handle->animCount = 0;
    handle->anim = 0;
    BlActHandle_ResetAnim(handle);
    handle->animate = FALSE;
    handle->callback = NULL;
    handle->callbackData = NULL;
}

u16 BlActSys_ExecMatLoadRequests(BlActSys *sys, const BlActMatRequest *requests, u32 count) {
    u32 first = GFL_AreaManAllocDefault(sys->texMatAlloc, count);
    u32 i;

    // BUG: unlike BlActSys_ExecActorRequests, this does not check that the materials fit, and writes before the
    // table when they do not
#ifdef BUGFIX
    if (first == AREAMAN_FAIL) {
        return BLACT_NONE;
    }
#endif
    for (i = 0; i < count; i++) {
        const BlActMatRequest *request = &requests[i];
        u32 material;

        switch (request->mode) {
        case BLACT_LOAD_KEEP:
            material = BlActScene_AddMaterialNewTex(sys->scene, request->texResource, request->format, request->size,
                                                    request->faceWidth, request->faceHeight);
            sys->texMats[first + i].material = material;
            sys->texMats[first + i].texResource = NULL;
            break;
        case BLACT_LOAD_TRIM:
            material = BlActScene_AddMaterialNewTex(sys->scene, request->texResource, request->format, request->size,
                                                    request->faceWidth, request->faceHeight);
            BlActScene_TrimMatTex(sys->scene, material);
            sys->texMats[first + i].material = material;
            sys->texMats[first + i].texResource = NULL;
            break;
        case BLACT_LOAD_TRANSFER:
            sys->texMats[first + i].material = BLACT_NONE;
            sys->texMats[first + i].texResource = request->texResource;
            break;
        }
    }
    return first;
}

u16 BlActSys_AddTexMat(BlActSys *sys, u32 material, void *texResource) {
    u32 idx = GFL_AreaManAllocDefault(sys->texMatAlloc, 1);

    sys->texMats[idx].material = material;
    sys->texMats[idx].texResource = texResource;
    return idx;
}

void BlActSys_FreeMaterials(BlActSys *sys, u16 first, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        if (sys->texMats[first + i].material != BLACT_NONE) {
            BlActScene_FreeMaterial(sys->scene, sys->texMats[first + i].material);
        }
        if (sys->texMats[first + i].texResource != NULL) {
            GFL_G3DResFree(sys->texMats[first + i].texResource);
        }
        BlActTexMat_Reset(&sys->texMats[first + i]);
    }
    GFL_AreaManDeAlloc(sys->texMatAlloc, first, count);
}

void BlActSys_FreeTexMat(BlActSys *sys, u32 texMat) {
    BlActTexMat_Reset(&sys->texMats[texMat]);
    GFL_AreaManDeAlloc(sys->texMatAlloc, texMat, 1);
}

u16 BlActSys_GetTexMatMaterial(BlActSys *sys, u32 texMat) {
    return sys->texMats[texMat].material;
}

static inline BlActHandle *BlActSys_GetHandle(BlActSys *sys, u32 idx) {
    return &sys->handles[idx];
}

u16 BlActSys_ExecActorRequests(BlActSys *sys, u16 firstTexMat, const BlActActorRequest *requests, u32 count, u32 type) {
    u32 first = GFL_AreaManAllocDefault(sys->handleAlloc, count);
    u32 i;

    if (first == AREAMAN_FAIL) {
        return BLACT_NONE;
    }
    for (i = 0; i < count; i++) {
        BlActHandle *handle = BlActSys_GetHandle(sys, first + i);
        const BlActActorRequest *request = &requests[i];
        u32 actor;

        BlActHandle_Reset(handle);
        handle->texMat = firstTexMat + request->texMat;
        actor = BlActScene_AddNewActor(sys->scene, sys->texMats[handle->texMat].material, request->scaleX,
                                       request->scaleY, &request->pos, request->alpha, request->lights, type);
        handle->actor = actor;
        handle->callback = request->callback;
        handle->callbackData = request->callbackData;
        BlActScene_SetActorHidden(sys->scene, actor, &request->visible);
        handle->cmd = BLACT_ANIM_CMD_END;
    }
    return first;
}

void BlActSys_DeleteActors(BlActSys *sys, u32 first, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        BlActScene_ClearActorMaterial(sys->scene, sys->handles[first + i].actor);
        BlActHandle_Reset(&sys->handles[first + i]);
    }
    GFL_AreaManDeAlloc(sys->handleAlloc, first, count);
}

void BlActSys_UpdateActors(BlActSys *sys) {
    int i;

    for (i = 0; i < sys->handleCount; i++) {
        BlActHandle *handle = &sys->handles[i];

        if (handle->actor != BLACT_NONE) {
            if (handle->callback != NULL) {
                handle->callback(sys, i, handle->callbackData);
            }
            BlActSys_UpdateHandleAnim(sys, handle);
        }
    }
}

static void BlActSys_UpdateHandleAnim(BlActSys *sys, BlActHandle *handle) {
    const BlActAnimStep *step;
    u16 face;
    u16 wait;
    BOOL flipS;
    BOOL flipT;

    if (handle->anims == NULL || handle->animate == FALSE) {
        return;
    }
    if (handle->wait != 0) {
        handle->wait--;
        return;
    }
    step = &handle->anims[handle->anim][handle->step];
    while (BlActAnimStep_IsCmd(step)) {
        if (BlActHandle_RunCmd(handle, step)) {
            return;
        }
        step = &handle->anims[handle->anim][handle->step];
    }
    handle->step++;
    face = step->face;
    wait = step->arg;
    if (sys->texMats[handle->texMat].texResource != NULL) {
        BlActSys_TransferFace(sys, &sys->texMats[handle->texMat], face);
    } else {
        BlActScene_SetActorFace(sys->scene, handle->actor, &face);
    }
    if (step->flipS) {
        flipS = TRUE;
    } else {
        flipS = FALSE;
    }
    if (step->flipT) {
        flipT = TRUE;
    } else {
        flipT = FALSE;
    }
    BlActScene_SetActorFlipS(sys->scene, handle->actor, &flipS);
    BlActScene_SetActorFlipT(sys->scene, handle->actor, &flipT);
    if (wait != 0) {
        wait--;
    }
    handle->wait = wait;
}

// Sends a face's characters to the material's place in VRAM
static void BlActSys_TransferFace(BlActSys *sys, BlActTexMat *texMat, u16 face) {
    u8 *image = GFL_G3DResGetTexImageData(texMat->texResource);
    u32 texAddr;
    u32 offset;
    u32 size;

    BlActScene_GetMaterialTexAddr(sys->scene, texMat->material, &texAddr);
    BlActScene_GetMaterialFaceLocation(sys->scene, texMat->material, face, &offset, &size, FALSE);
    if (sys->transfer != NULL) {
        sys->transfer(FALSE, texAddr, image + offset, size);
    }
}

static void BlActSys_TransferPltt(BlActSys *sys, BlActTexMat *texMat) {
    void *pltt = GFL_G3DResGetTexPaletteData(texMat->texResource);
    u32 plttAddr;

    BlActScene_GetMaterialPlttAddr(sys->scene, texMat->material, &plttAddr);
    if (sys->transfer != NULL) {
        sys->transfer(TRUE, plttAddr, pltt, 0x20);
    }
}

void BlActSys_Draw(BlActSys *sys, G3DCamera *camera, G3DLight *lights) {
    BlActScene_Draw(sys->scene, camera, lights);
}

void BlActSys_SetActorVisible(BlActSys *sys, u32 actor, BOOL visible) {
    BlActScene_SetActorHidden(sys->scene, sys->handles[actor].actor, &visible);
}

BOOL BlActSys_IsActorVisible(BlActSys *sys, u16 actor) {
    return BlActScene_IsActorVisible(sys->scene, sys->handles[actor].actor);
}

void BlActSys_SetActorLight(BlActSys *sys, u32 actor, BOOL enable, u32 light) {
    u32 mask = 1 << light;

    if (enable) {
        BlActScene_EnableActorLight(sys->scene, sys->handles[actor].actor, mask);
    } else {
        BlActScene_DisableActorLight(sys->scene, sys->handles[actor].actor, mask);
    }
}

void BlActSys_SetActorAnimating(BlActSys *sys, u32 actor, BOOL animate) {
    sys->handles[actor].animate = animate;
}

void BlActSys_SetActorAnims(BlActSys *sys, u32 actor, const BlActAnimStep *const *anims, u16 animCount) {
    BlActHandle *handle = &sys->handles[actor];

    handle->anims = anims;
    handle->animCount = animCount;
    handle->anim = 0;
    BlActHandle_ResetAnim(handle);
}

void BlActSys_SetActorAnim(BlActSys *sys, u32 actor, u16 anim) {
    BlActHandle *handle = &sys->handles[actor];

    handle->anim = anim;
    BlActHandle_ResetAnim(handle);
}

u16 BlActSys_GetActorAnim(BlActSys *sys, u32 actor) {
    return sys->handles[actor].anim;
}

void BlActSys_SetActorStep(BlActSys *sys, u32 actor, u16 step) {
    BlActHandle *handle = &sys->handles[actor];

    BlActHandle_ResetAnim(handle);
    handle->step = step;
}

u16 BlActSys_GetActorStep(BlActSys *sys, u32 actor) {
    return sys->handles[actor].step;
}

void BlActSys_ShareActorPltt(BlActSys *sys, u32 actor, u32 texMat) {
    BlActTexMat *dst = &sys->texMats[sys->handles[actor].texMat];

    dst->texResource = sys->texMats[texMat].texResource;
    BlActSys_TransferPltt(sys, dst);
}

u32 BlActSys_GetSceneActor(BlActSys *sys, u16 actor) {
    return sys->handles[actor].actor;
}

u16 BlActSys_GetMatByActor(BlActSys *sys, u32 actor) {
    return sys->handles[actor].texMat;
}

BOOL BlActSys_GetActorLastCmd(BlActSys *sys, u16 actor, u16 *cmd) {
    u16 last = sys->handles[actor].cmd;

    *cmd = last;
    if (last == BLACT_ANIM_CMD_GOTO || last == BLACT_ANIM_CMD_LOOP || last == BLACT_ANIM_CMD_CHANGE ||
        last == BLACT_ANIM_CMD_END) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BlActAnimStep_IsCmd(const BlActAnimStep *step) {
    if (BlActAnim_GetCmdFunc(step->face) != NULL) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BlActHandle_RunCmd(BlActHandle *handle, const BlActAnimStep *step) {
    BlActAnimCmdFunc func = BlActAnim_GetCmdFunc(step->face);

    handle->cmd = step->face;
    return func(handle, step);
}

static BlActAnimCmdFunc BlActAnim_GetCmdFunc(u32 cmd) {
    switch (cmd) {
    case BLACT_ANIM_CMD_END:
        return BlActAnimCmd_End;
    case BLACT_ANIM_CMD_CHANGE:
        return BlActAnimCmd_Change;
    case BLACT_ANIM_CMD_LOOP:
        return BlActAnimCmd_Loop;
    case BLACT_ANIM_CMD_GOTO:
        return BlActAnimCmd_Goto;
    }
    return NULL;
}

static BOOL BlActAnimCmd_End(BlActHandle *handle, const BlActAnimStep *step) {
    return TRUE;
}

static BOOL BlActAnimCmd_Change(BlActHandle *handle, const BlActAnimStep *step) {
    handle->anim = step->arg;
    BlActHandle_ResetAnim(handle);
    return FALSE;
}

static BOOL BlActAnimCmd_Loop(BlActHandle *handle, const BlActAnimStep *step) {
    u8 target = step->arg & 0xff;
    u8 times = (step->arg & 0xff00) >> 8;

    if (handle->loops < times) {
        handle->step = target;
        handle->wait = 0;
        handle->loops++;
    } else {
        handle->loops = 0;
        handle->wait = 0;
        handle->step++;
    }
    return FALSE;
}

static BOOL BlActAnimCmd_Goto(BlActHandle *handle, const BlActAnimStep *step) {
    handle->step = step->arg;
    handle->wait = 0;
    return FALSE;
}
