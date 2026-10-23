#include "types.h"
#include "field/field.h"
#include "field/underwater_effect.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"

struct UnderwaterEffectWork {
    G3DResource *resources[7];
    G3DModel *models[3];
    G3DAnimation *animations[4];
    G3DActor *actors[3];
};

static const u32 sModelResourceIds[3] = { 0, 1, 2 };
static const u32 sAnimationModelIds[4] = { 0, 1, 2, 0 };
static const u32 sAnimationResourceIds[4] = { 3, 4, 5, 6 };
static const u32 sArchiveFileIds[7] = { 1, 3, 5, 2, 4, 6, 0 };
static const SRTMatrix sUnderwaterMatrix = {
    { 0x100000, 0, 0x100000 },
    { 0x1000, 0x1000, 0x1000 },
    { { { 0x1000, 0, 0 }, { 0, 0x1000, 0 }, { 0, 0, 0x1000 } } },
};

static void UnderwaterEffect_Init(FieldAsyncProc *proc, Field *field, void *data);
static void UnderwaterEffect_Free(FieldAsyncProc *proc, Field *field, void *data);
static void UnderwaterEffect_Update(FieldAsyncProc *proc, Field *field, void *data);
static void UnderwaterEffect_Draw(FieldAsyncProc *proc, Field *field, void *data);

const FieldAsyncProcDef UNDERWATER_EFFECT_PROC = {
    0x80,
    sizeof(UnderwaterEffectWork),
    UnderwaterEffect_Init,
    UnderwaterEffect_Free,
    UnderwaterEffect_Update,
    UnderwaterEffect_Draw,
};

static void UnderwaterEffect_Init(FieldAsyncProc *proc, Field *field, void *data) {
    UnderwaterEffectWork *work = data;
    HeapID heapId = Field_GetHeapID(field);
    ArcTool *arc = GFL_ArcSysCreateFileHandle(0xbb, HEAPID_TAIL(heapId));
    int i;

    for (i = 0; i < 7; i++) {
        work->resources[i] = GFL_G3DSysReadArcToolResource(arc, sArchiveFileIds[i]);
        if (GFL_G3DResCheckType(work->resources[i], G3D_RES_CHECK_TEX)) {
            GFL_G3DResUploadTexData(work->resources[i]);
        }
    }
    for (i = 0; i < 3; i++) {
        u32 index = sModelResourceIds[i];
        if (GFL_G3DResCheckType(work->resources[index], G3D_RES_CHECK_TEX)) {
            work->models[i] = GFL_G3DMdlCreate(work->resources[index], 0, work->resources[index]);
        } else {
            work->models[i] = GFL_G3DMdlCreate(work->resources[index], 0, NULL);
        }
    }
    for (i = 0; i < 4; i++) {
        work->animations[i] =
            GFL_G3DAnmCreate(work->models[sAnimationModelIds[i]], work->resources[sAnimationResourceIds[i]], 0);
    }
    for (i = 0; i < 3; i++) {
        work->actors[i] = GFL_G3DActorCreate(work->models[i], work->animations, 4);
    }
    GFL_G3DActorBindAnm(work->actors[0], 0);
    GFL_G3DActorBindAnm(work->actors[0], 3);
    GFL_G3DActorBindAnm(work->actors[1], 1);
    GFL_G3DActorBindAnm(work->actors[2], 2);
    GFL_G3DSysSetSwapBufferParams(1, 1);
    GFL_ArcToolFree(arc);
}

static void UnderwaterEffect_Free(FieldAsyncProc *proc, Field *field, void *data) {
    UnderwaterEffectWork *work = data;
    int i;
    for (i = 0; i < 3; i++) {
        GFL_G3DActorFree(work->actors[i]);
    }
    for (i = 0; i < 4; i++) {
        GFL_G3DAnmFree(work->animations[i]);
    }
    for (i = 0; i < 3; i++) {
        GFL_G3DMdlFree(work->models[i]);
    }
    for (i = 0; i < 7; i++) {
        if (GFL_G3DResCheckType(work->resources[i], G3D_RES_CHECK_TEX)) {
            GFL_G3DResFreeTexData(work->resources[i]);
        }
        GFL_G3DResFree(work->resources[i]);
    }
}

static void UnderwaterEffect_Update(FieldAsyncProc *proc, Field *field, void *data) {
    UnderwaterEffectWork *work = data;
    GFL_G3DActorStepAnmFrameLoop(work->actors[0], 0, 0x1000);
    GFL_G3DActorStepAnmFrameLoop(work->actors[0], 3, 0x1000);
    GFL_G3DActorStepAnmFrameLoop(work->actors[1], 1, 0x1000);
    GFL_G3DActorStepAnmFrameLoop(work->actors[2], 2, 0x1000);
    GFL_G3DSysSetSwapBufferParams(1, 1);
}

static void UnderwaterEffect_Draw(FieldAsyncProc *proc, Field *field, void *data) {
    UnderwaterEffectWork *work = data;
    GFL_G3DSysDrawObj(work->actors[0], &sUnderwaterMatrix);
    GFL_G3DSysDrawObj(work->actors[1], &sUnderwaterMatrix);
    GFL_G3DSysDrawObj(work->actors[2], &sUnderwaterMatrix);
}
