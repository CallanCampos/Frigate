#include "types.h"
#include "gfl/areaman.h"
#include "gfl/billboard_actor.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"

// The billboard actor system: runs of materials and actors in a scene, and the animations of faces actors play

typedef BOOL (*BillboardActorAnimCmdFunc)(BillboardActorHandle *handle, const BillboardActorAnimStep *step);

static void BillboardActorTexMat_Reset(BillboardActorTexMat *texMat);
static void BillboardActorHandle_ResetAnim(BillboardActorHandle *handle);
static void BillboardActorHandle_Reset(BillboardActorHandle *handle);
static void BillboardActorSys_UpdateHandleAnim(BillboardActorSys *sys, BillboardActorHandle *handle);
static void BillboardActorSys_TransferFace(BillboardActorSys *sys, BillboardActorTexMat *texMat, u16 face);
static void BillboardActorSys_TransferPltt(BillboardActorSys *sys, BillboardActorTexMat *texMat);
static BOOL BillboardActorAnimStep_IsCmd(const BillboardActorAnimStep *step);
static BOOL BillboardActorHandle_RunCmd(BillboardActorHandle *handle, const BillboardActorAnimStep *step);
static BillboardActorAnimCmdFunc BillboardActorAnim_GetCmdFunc(u32 cmd);
static BOOL BillboardActorAnimCmd_End(BillboardActorHandle *handle, const BillboardActorAnimStep *step);
static BOOL BillboardActorAnimCmd_Change(BillboardActorHandle *handle, const BillboardActorAnimStep *step);
static BOOL BillboardActorAnimCmd_Loop(BillboardActorHandle *handle, const BillboardActorAnimStep *step);
static BOOL BillboardActorAnimCmd_Goto(BillboardActorHandle *handle, const BillboardActorAnimStep *step);

const BillboardActorSceneSetup BILLBOARD_ACTOR_DEFAULT_CONFIG = {
    0,
    0,
    { FX32_ONE, FX32_ONE, FX32_ONE },
    GX_RGB(31, 31, 31),
    GX_RGB(16, 16, 16),
    GX_RGB(16, 16, 16),
    GX_RGB(0, 0, 0),
    63,
    BILLBOARD_ACTOR_ORIGIN_CENTER,
};

BillboardActorSys *BillboardActorSys_Create(u16 texMatCount, u16 handleCount, BillboardActorTransferFunc transfer,
                                            HeapID heapId) {
    BillboardActorSys *sys = GFL_HeapAllocate(heapId, sizeof(BillboardActorSys), TRUE, "billboard_act.c", 94);
    BillboardActorSceneSetup setup;
    int i;

    sys->heapId = heapId;
    sys->transfer = transfer;
    sys->texMatCount = texMatCount;
    sys->handleCount = handleCount;
    setup = BILLBOARD_ACTOR_DEFAULT_CONFIG;
    setup.materialCount = sys->texMatCount;
    setup.actorCount = sys->handleCount;
    sys->scene = BillboardActorScene_Create(&setup, heapId);
    sys->texMats =
        GFL_HeapAllocate(heapId, sys->texMatCount * sizeof(BillboardActorTexMat), TRUE, "billboard_act.c", 119);
    sys->texMatAlloc = GFL_AreaManCreate(sys->texMatCount, heapId);
    sys->handles =
        GFL_HeapAllocate(heapId, sys->handleCount * sizeof(BillboardActorHandle), TRUE, "billboard_act.c", 123);
    sys->handleAlloc = GFL_AreaManCreate(sys->handleCount, heapId);
    for (i = 0; i < sys->texMatCount; i++) {
        BillboardActorTexMat_Reset(&sys->texMats[i]);
    }
    for (i = 0; i < sys->handleCount; i++) {
        BillboardActorHandle_Reset(&sys->handles[i]);
    }
    return sys;
}

void BillboardActorSys_Free(BillboardActorSys *sys) {
    int i;

    for (i = 0; i < sys->handleCount; i++) {
        if (sys->handles[i].actor != BILLBOARD_ACTOR_NONE) {
            BillboardActorScene_ClearActorMaterial(sys->scene, sys->handles[i].actor);
        }
    }
    GFL_AreaManFree(sys->handleAlloc);
    GFL_HeapFree(sys->handles);
    GFL_AreaManFree(sys->texMatAlloc);
    GFL_HeapFree(sys->texMats);
    BillboardActorScene_Delete(sys->scene);
    GFL_HeapFree(sys);
}

BillboardActorScene *BillboardActorSys_GetScene(BillboardActorSys *sys) {
    return sys->scene;
}

static void BillboardActorTexMat_Reset(BillboardActorTexMat *texMat) {
    texMat->material = BILLBOARD_ACTOR_NONE;
    texMat->texResource = NULL;
}

static void BillboardActorHandle_ResetAnim(BillboardActorHandle *handle) {
    handle->step = 0;
    handle->wait = 0;
    handle->loops = 0;
    handle->cmd = 0;
}

static void BillboardActorHandle_Reset(BillboardActorHandle *handle) {
    handle->actor = BILLBOARD_ACTOR_NONE;
    handle->anims = NULL;
    handle->animCount = 0;
    handle->anim = 0;
    BillboardActorHandle_ResetAnim(handle);
    handle->animate = FALSE;
    handle->callback = NULL;
    handle->callbackData = NULL;
}

u16 BillboardActorSys_ExecMatLoadRequests(BillboardActorSys *sys, const BillboardActorMatRequest *requests, u32 count) {
    u32 first = GFL_AreaManAllocDefault(sys->texMatAlloc, count);
    u32 i;

    // BUG: unlike BillboardActorSys_ExecActorRequests, this does not check that the materials fit, and writes before
    // the table when they do not
#ifdef BUGFIX
    if (first == AREAMAN_FAIL) {
        return BILLBOARD_ACTOR_NONE;
    }
#endif
    for (i = 0; i < count; i++) {
        const BillboardActorMatRequest *request = &requests[i];
        u32 material;

        switch (request->mode) {
        case BILLBOARD_ACTOR_LOAD_KEEP:
            material = BillboardActorScene_AddMaterialNewTex(sys->scene, request->texResource, request->format,
                                                             request->size, request->faceWidth, request->faceHeight);
            sys->texMats[first + i].material = material;
            sys->texMats[first + i].texResource = NULL;
            break;
        case BILLBOARD_ACTOR_LOAD_TRIM:
            material = BillboardActorScene_AddMaterialNewTex(sys->scene, request->texResource, request->format,
                                                             request->size, request->faceWidth, request->faceHeight);
            BillboardActorScene_TrimMatTex(sys->scene, material);
            sys->texMats[first + i].material = material;
            sys->texMats[first + i].texResource = NULL;
            break;
        case BILLBOARD_ACTOR_LOAD_TRANSFER:
            sys->texMats[first + i].material = BILLBOARD_ACTOR_NONE;
            sys->texMats[first + i].texResource = request->texResource;
            break;
        }
    }
    return first;
}

u16 BillboardActorSys_AddTexMat(BillboardActorSys *sys, u32 material, G3DResource *texResource) {
    u32 idx = GFL_AreaManAllocDefault(sys->texMatAlloc, 1);

    sys->texMats[idx].material = material;
    sys->texMats[idx].texResource = texResource;
    return idx;
}

void BillboardActorSys_FreeMaterials(BillboardActorSys *sys, u16 first, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        if (sys->texMats[first + i].material != BILLBOARD_ACTOR_NONE) {
            BillboardActorScene_FreeMaterial(sys->scene, sys->texMats[first + i].material);
        }
        if (sys->texMats[first + i].texResource != NULL) {
            GFL_G3DResFree(sys->texMats[first + i].texResource);
        }
        BillboardActorTexMat_Reset(&sys->texMats[first + i]);
    }
    GFL_AreaManDeAlloc(sys->texMatAlloc, first, count);
}

void BillboardActorSys_FreeTexMat(BillboardActorSys *sys, u32 texMat) {
    BillboardActorTexMat_Reset(&sys->texMats[texMat]);
    GFL_AreaManDeAlloc(sys->texMatAlloc, texMat, 1);
}

u16 BillboardActorSys_GetTexMatMaterial(BillboardActorSys *sys, u32 texMat) {
    return sys->texMats[texMat].material;
}

static inline BillboardActorHandle *BillboardActorSys_GetHandle(BillboardActorSys *sys, u32 idx) {
    return &sys->handles[idx];
}

u16 BillboardActorSys_ExecActorRequests(BillboardActorSys *sys, u16 firstTexMat, const BillboardActorRequest *requests,
                                        u32 count, u32 type) {
    u32 first = GFL_AreaManAllocDefault(sys->handleAlloc, count);
    u32 i;

    if (first == AREAMAN_FAIL) {
        return BILLBOARD_ACTOR_NONE;
    }
    for (i = 0; i < count; i++) {
        BillboardActorHandle *handle = BillboardActorSys_GetHandle(sys, first + i);
        const BillboardActorRequest *request = &requests[i];
        u32 actor;

        BillboardActorHandle_Reset(handle);
        handle->texMat = firstTexMat + request->texMat;
        actor = BillboardActorScene_AddNewActor(sys->scene, sys->texMats[handle->texMat].material, request->scaleX,
                                                request->scaleY, &request->pos, request->alpha, request->lights, type);
        handle->actor = actor;
        handle->callback = request->callback;
        handle->callbackData = request->callbackData;
        BillboardActorScene_SetActorHidden(sys->scene, actor, &request->visible);
        handle->cmd = BILLBOARD_ACTOR_ANIM_CMD_END;
    }
    return first;
}

void BillboardActorSys_DeleteActors(BillboardActorSys *sys, u32 first, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        BillboardActorScene_ClearActorMaterial(sys->scene, sys->handles[first + i].actor);
        BillboardActorHandle_Reset(&sys->handles[first + i]);
    }
    GFL_AreaManDeAlloc(sys->handleAlloc, first, count);
}

void BillboardActorSys_UpdateActors(BillboardActorSys *sys) {
    int i;

    for (i = 0; i < sys->handleCount; i++) {
        BillboardActorHandle *handle = &sys->handles[i];

        if (handle->actor != BILLBOARD_ACTOR_NONE) {
            if (handle->callback != NULL) {
                handle->callback(sys, i, handle->callbackData);
            }
            BillboardActorSys_UpdateHandleAnim(sys, handle);
        }
    }
}

static void BillboardActorSys_UpdateHandleAnim(BillboardActorSys *sys, BillboardActorHandle *handle) {
    const BillboardActorAnimStep *step;
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
    while (BillboardActorAnimStep_IsCmd(step)) {
        if (BillboardActorHandle_RunCmd(handle, step)) {
            return;
        }
        step = &handle->anims[handle->anim][handle->step];
    }
    handle->step++;
    face = step->face;
    wait = step->arg;
    if (sys->texMats[handle->texMat].texResource != NULL) {
        BillboardActorSys_TransferFace(sys, &sys->texMats[handle->texMat], face);
    } else {
        BillboardActorScene_SetActorFace(sys->scene, handle->actor, &face);
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
    BillboardActorScene_SetActorFlipS(sys->scene, handle->actor, &flipS);
    BillboardActorScene_SetActorFlipT(sys->scene, handle->actor, &flipT);
    if (wait != 0) {
        wait--;
    }
    handle->wait = wait;
}

// Sends a face's characters to the material's place in VRAM
static void BillboardActorSys_TransferFace(BillboardActorSys *sys, BillboardActorTexMat *texMat, u16 face) {
    u8 *image = GFL_G3DResGetTexImageData(texMat->texResource);
    u32 texAddr;
    u32 offset;
    u32 size;

    BillboardActorScene_GetMaterialTexAddr(sys->scene, texMat->material, &texAddr);
    BillboardActorScene_GetMaterialFaceLocation(sys->scene, texMat->material, face, &offset, &size, FALSE);
    if (sys->transfer != NULL) {
        sys->transfer(FALSE, texAddr, image + offset, size);
    }
}

static void BillboardActorSys_TransferPltt(BillboardActorSys *sys, BillboardActorTexMat *texMat) {
    void *pltt = GFL_G3DResGetTexPaletteData(texMat->texResource);
    u32 plttAddr;

    BillboardActorScene_GetMaterialPlttAddr(sys->scene, texMat->material, &plttAddr);
    if (sys->transfer != NULL) {
        sys->transfer(TRUE, plttAddr, pltt, 0x20);
    }
}

void BillboardActorSys_Draw(BillboardActorSys *sys, G3DCamera *camera, G3DLight *lights) {
    BillboardActorScene_Draw(sys->scene, camera, lights);
}

void BillboardActorSys_SetActorVisible(BillboardActorSys *sys, u32 actor, BOOL visible) {
    BillboardActorScene_SetActorHidden(sys->scene, sys->handles[actor].actor, &visible);
}

BOOL BillboardActorSys_IsActorVisible(BillboardActorSys *sys, u16 actor) {
    return BillboardActorScene_IsActorVisible(sys->scene, sys->handles[actor].actor);
}

void BillboardActorSys_SetActorLight(BillboardActorSys *sys, u32 actor, BOOL enable, u32 light) {
    u32 mask = 1 << light;

    if (enable) {
        BillboardActorScene_EnableActorLight(sys->scene, sys->handles[actor].actor, mask);
    } else {
        BillboardActorScene_DisableActorLight(sys->scene, sys->handles[actor].actor, mask);
    }
}

void BillboardActorSys_SetActorAnimating(BillboardActorSys *sys, u32 actor, BOOL animate) {
    sys->handles[actor].animate = animate;
}

void BillboardActorSys_SetActorAnims(BillboardActorSys *sys, u32 actor, const BillboardActorAnimStep *const *anims,
                                     u16 animCount) {
    BillboardActorHandle *handle = &sys->handles[actor];

    handle->anims = anims;
    handle->animCount = animCount;
    handle->anim = 0;
    BillboardActorHandle_ResetAnim(handle);
}

void BillboardActorSys_SetActorAnim(BillboardActorSys *sys, u32 actor, u16 anim) {
    BillboardActorHandle *handle = &sys->handles[actor];

    handle->anim = anim;
    BillboardActorHandle_ResetAnim(handle);
}

u16 BillboardActorSys_GetActorAnim(BillboardActorSys *sys, u32 actor) {
    return sys->handles[actor].anim;
}

void BillboardActorSys_SetActorStep(BillboardActorSys *sys, u32 actor, u16 step) {
    BillboardActorHandle *handle = &sys->handles[actor];

    BillboardActorHandle_ResetAnim(handle);
    handle->step = step;
}

u16 BillboardActorSys_GetActorStep(BillboardActorSys *sys, u32 actor) {
    return sys->handles[actor].step;
}

void BillboardActorSys_ShareActorPltt(BillboardActorSys *sys, u32 actor, u32 texMat) {
    BillboardActorTexMat *dst = &sys->texMats[sys->handles[actor].texMat];

    dst->texResource = sys->texMats[texMat].texResource;
    BillboardActorSys_TransferPltt(sys, dst);
}

u32 BillboardActorSys_GetSceneActor(BillboardActorSys *sys, u16 actor) {
    return sys->handles[actor].actor;
}

u16 BillboardActorSys_GetMatByActor(BillboardActorSys *sys, u32 actor) {
    return sys->handles[actor].texMat;
}

BOOL BillboardActorSys_GetActorLastCmd(BillboardActorSys *sys, u16 actor, u16 *cmd) {
    u16 last = sys->handles[actor].cmd;

    *cmd = last;
    if (last == BILLBOARD_ACTOR_ANIM_CMD_GOTO || last == BILLBOARD_ACTOR_ANIM_CMD_LOOP ||
        last == BILLBOARD_ACTOR_ANIM_CMD_CHANGE || last == BILLBOARD_ACTOR_ANIM_CMD_END) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BillboardActorAnimStep_IsCmd(const BillboardActorAnimStep *step) {
    if (BillboardActorAnim_GetCmdFunc(step->face) != NULL) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BillboardActorHandle_RunCmd(BillboardActorHandle *handle, const BillboardActorAnimStep *step) {
    BillboardActorAnimCmdFunc func = BillboardActorAnim_GetCmdFunc(step->face);

    handle->cmd = step->face;
    return func(handle, step);
}

static BillboardActorAnimCmdFunc BillboardActorAnim_GetCmdFunc(u32 cmd) {
    switch (cmd) {
    case BILLBOARD_ACTOR_ANIM_CMD_END:
        return BillboardActorAnimCmd_End;
    case BILLBOARD_ACTOR_ANIM_CMD_CHANGE:
        return BillboardActorAnimCmd_Change;
    case BILLBOARD_ACTOR_ANIM_CMD_LOOP:
        return BillboardActorAnimCmd_Loop;
    case BILLBOARD_ACTOR_ANIM_CMD_GOTO:
        return BillboardActorAnimCmd_Goto;
    }
    return NULL;
}

static BOOL BillboardActorAnimCmd_End(BillboardActorHandle *handle, const BillboardActorAnimStep *step) {
    return TRUE;
}

static BOOL BillboardActorAnimCmd_Change(BillboardActorHandle *handle, const BillboardActorAnimStep *step) {
    handle->anim = step->arg;
    BillboardActorHandle_ResetAnim(handle);
    return FALSE;
}

static BOOL BillboardActorAnimCmd_Loop(BillboardActorHandle *handle, const BillboardActorAnimStep *step) {
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

static BOOL BillboardActorAnimCmd_Goto(BillboardActorHandle *handle, const BillboardActorAnimStep *step) {
    handle->step = step->arg;
    handle->wait = 0;
    return FALSE;
}
