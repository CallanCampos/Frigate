// The field effects: the effects that a map has loaded, by ID, and the tasks that play them. The name is the ROM's
// own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_effect.h"
#include "field/field_map.h"
#include "gfl/arc.h"
#include "gfl/billboard_actor.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nnsys/g3d.h"
#include "system/area_data.h"
#include "system/palanm.h"
#include "system/season.h"

// What an effect loads and frees
typedef struct {
    u32 id;
    void *(*create)(FieldEffects *effects, HeapID heapId);
    void (*free)(FieldEffects *effects, void *data);
} FieldEffectVTable;

// A loaded effect
typedef struct {
    u32 id;
    void *data;
} FieldEffectHandle;

struct FieldEffectTaskStore {
    u32 count;
    u16 heapId;
    FieldEffectTask *tasks;
    TCBManager *tcbMgr;
    FieldEffects *effects;
};

struct FieldEffects {
    int count;
    u16 heapId;
    Field *field;
    ArcTool *arc;
    FieldEffectHandle *handles;
    u16 taskCount;
    u8 season;
    u8 isExterior;
    void *tcbMgrWork;
    TCBManager *tcbMgr;
    FieldEffectTaskStore *taskStore;
    const u8 *luminanceTable;
};

static void FieldEffects_InitEffectBuffer(FieldEffects *effects);
static void FieldEffects_FreeEffectBuffer(FieldEffects *effects);
static FieldEffectHandle *FieldEffects_GetHandleByID(FieldEffects *effects, u32 id);
static const FieldEffectVTable *FieldEffects_GetVTable(u32 id);
static void FieldEffects_ExitObjectSystem(FieldEffects *effects);
static void FieldEffects_TCBManagerUpdate(FieldEffects *effects);
static void FieldEffects_DrawObjects(FieldEffects *effects);
static FieldEffectTaskStore *FieldEffectTCBStore_Create(HeapID heapId, TCBManager *tcbMgr, u32 count,
                                                        FieldEffects *effects);
static void FieldEffects_DeleteObjectMgr(FieldEffectTaskStore *store);
static void FieldEffects_DrawObjectMgr(FieldEffectTaskStore *store);
static FieldEffectTask *FieldTerrainEffectTCB_Create(FieldEffectTaskStore *store, const FieldEffectTaskVTable *vtable,
                                                     const VecFx32 *pos, u32 mode, void *params, u32 priority);
static void FieldEffects_ObjectTask(TCB *tcb, void *data);
static void FieldEffects_InitObject(FieldEffectTask *task);
static void FieldEffects_DrawObject(FieldEffectTask *task);

const u32 STATIC_LOADED_FIELD_EFFECT_COUNT = 21;

static const VecFx32 FIELD_EFFECT_SCALE = { FX32_CONST(8), FX32_CONST(8), FX32_CONST(8) };

const u32 STATIC_LOADED_FIELD_EFFECT_IDS[21] = {
    FLDEFF_SHADOW,
    FLDEFF_DUST,
    FLDEFF_GRASS,
    FLDEFF_SURF_POKEMON,
    FLDEFF_EMOTION_BUBBLE,
    FLDEFF_FOOTMARK,
    FLDEFF_REFLECT,
    FLDEFF_RIPPLE,
    FLDEFF_HIDE,
    FLDEFF_BUBBLE,
    FLDEFF_SPLASH,
    FLDEFF_FISHING,
    FLDEFF_CUT,
    FLDEFF_LEAVES,
    FLDEFF_FES_SPARKLE,
    FLDEFF_ENCOUNT_GRASS,
    FLDEFF_ENCOUNT_REALLY_TALL_GRASS,
    FLDEFF_ENCOUNT_DUST_CLOUD,
    FLDEFF_ENCOUNT_WATER_0,
    FLDEFF_ENCOUNT_WATER_1,
    FLDEFF_ENCOUNT_FLYING_SHADOW,
};

static const FieldEffectVTable FIELD_EFFECT_VTABLES[FLDEFF_MAX + 1] = {
    { FLDEFF_SHADOW, FieldEffect_Shadow_Create, FieldEffect_Shadow_Free },
    { FLDEFF_DUST, playSmokeEffect, freeSmokeEffect },
    { FLDEFF_GRASS, loadGrassEffects, freeGrassEffects },
    { FLDEFF_SURF_POKEMON, FldeffSurfPokemon_Create, FldeffSurfPokemon_Delete },
    { FLDEFF_EMOTION_BUBBLE, FldeffEmotionBubble_Create, FldeffEmotionBubble_Delete },
    { FLDEFF_FOOTMARK, Footmark_Init, Footmark_Delete },
    { FLDEFF_REFLECT, FldeffReflect_Create, FldeffReflect_Delete },
    { FLDEFF_RIPPLE, FldeffRipple_Create, FldeffRipple_Delete },
    { FLDEFF_SURF_POKEMON_SPLASH, FldeffSurfPokemonSplash_Create, FldeffSurfPokemonSplash_Delete },
    { FLDEFF_ROCK_SMASH, FldeffRockSmash_Create, FldeffRockSmash_Delete },
    { FLDEFF_D06_ELECTRIC, StoneTask_Create, D06Electric_Delete },
    { FLDEFF_CUT, FldeffCut_Create, FldeffCut_Delete },
    { FLDEFF_FISHING, FldeffFishing_Create, FldeffFishing_Delete },
    { FLDEFF_BTRAIN, FieldEffect_BTrain_Create, FieldEffect_BTrain_Free },
    { FLDEFF_HIDE, Hide_CreateResources, Hide_DeleteResources },
    { FLDEFF_BUBBLE, FldeffBubble_CreateRes, FldeffBubble_DeleteRes },
    { FLDEFF_SPLASH, FldeffSplash_CreateModelRes, FldeffSplash_DeleteModelRes },
    { FLDEFF_LEAVES, FldeffLeaves_Create, FldeffLeaves_Delete },
    { FLDEFF_FES_SPARKLE, FldeffFesSparkle_Create, FldeffFesSparkle_Delete },
    { FLDEFF_ENCOUNT_GRASS, EncountTask_CreateGrass, EncountTask_Delete },
    { FLDEFF_ENCOUNT_REALLY_TALL_GRASS, EncountTask_CreateReallyTallGrass, EncountTask_Delete },
    { FLDEFF_ENCOUNT_DUST_CLOUD, EncountTask_CreateDustCloud, EncountTask_Delete },
    { FLDEFF_ENCOUNT_WATER_0, EncountTask_CreateWater0, EncountTask_Delete },
    { FLDEFF_ENCOUNT_WATER_1, EncountTask_CreateWater1, EncountTask_Delete },
    { FLDEFF_ENCOUNT_FLYING_SHADOW, EncountTask_CreateFlyingShadow, EncountTask_Delete },
    { FLDEFF_NONE, NULL, NULL },
};

FieldEffects *FieldEffects_Create(Field *field, int count, HeapID heapId) {
    VecFx32 scale;
    u8 polyId;
    BillboardActorScene *scene;
    FieldEffects *effects = GFL_HeapAllocate(heapId, sizeof(FieldEffects), TRUE, "field_effect.c", 128);

    effects->count = count;
    effects->heapId = heapId;
    effects->field = field;
    effects->arc = GFL_ArcSysCreateFileHandle(74, heapId);
    effects->season = GameSystem_GetSeason(Field_GetGameSystem(field));
    effects->isExterior = AreaData_IsExterior(Field_GetAreaData(field));
    FieldEffects_InitEffectBuffer(effects);
    scene = BillboardActorSys_GetScene(Field_GetEffectBillboardActor(effects->field));
    BillboardActorScene_SetGeomOrigin(scene, 0);
    scale = FIELD_EFFECT_SCALE;
    BillboardActorScene_SetScale(scene, &scale);
    polyId = 0;
    BillboardActorScene_SetBasePolyID(scene, &polyId);
    return effects;
}

void FieldEffects_Free(FieldEffects *effects) {
    FieldEffects_ExitObjectSystem(effects);
    FieldEffects_FreeEffectBuffer(effects);
    GFL_ArcToolFree(effects->arc);
    GFL_HeapFree(effects);
}

void FieldEffects_Update(FieldEffects *effects) {
    FieldEffects_TCBManagerUpdate(effects);
}

void FieldEffects_Draw(FieldEffects *effects) {
    FieldEffects_DrawObjects(effects);
}

Field *FieldEffects_GetField(FieldEffects *effects) {
    return effects->field;
}

struct FieldG3DObjSystem *FieldEffects_GetFieldMapData(FieldEffects *effects) {
    return Field_GetG3DObjSys(effects->field);
}

ArcTool *FieldEffects_GetArc(FieldEffects *effects) {
    return effects->arc;
}

u32 FieldEffects_GetSeason(FieldEffects *effects) {
    return effects->season;
}

BOOL FieldEffects_GetAreaIsExterior(FieldEffects *effects) {
    return effects->isExterior;
}

void FieldEffects_Load(FieldEffects *effects, const u32 *ids, u32 count) {
    FieldEffectHandle *handle;
    const FieldEffectVTable *vtable;

    while (count != 0) {
        handle = FieldEffects_GetHandleByID(effects, FLDEFF_NONE);
        vtable = FieldEffects_GetVTable(*ids);
        handle->id = *ids;
        handle->data = vtable->create(effects, effects->heapId);
        ids++;
        count--;
    }
}

void FieldEffects_FreeEffect(FieldEffects *effects, u32 id) {
    FieldEffectHandle *handle = FieldEffects_GetHandleByID(effects, id);

    if (handle != NULL) {
        FieldEffects_GetVTable(id)->free(effects, handle->data);
        handle->id = FLDEFF_NONE;
        handle->data = NULL;
    }
}

BOOL FieldEffects_IsLoaded(FieldEffects *effects, u32 id) {
    if (FieldEffects_GetHandleByID(effects, id) != NULL) {
        return TRUE;
    }
    return FALSE;
}

void *FieldEffects_GetHandleData(FieldEffects *effects, u32 id) {
    return FieldEffects_GetHandleByID(effects, id)->data;
}

static void FieldEffects_InitEffectBuffer(FieldEffects *effects) {
    int i;
    FieldEffectHandle *handle;

    handle = GFL_HeapAllocate(effects->heapId, effects->count * sizeof(FieldEffectHandle), TRUE, "field_effect.c", 373);
    effects->handles = handle;
    for (i = 0; i < effects->count; i++, handle++) {
        handle->id = FLDEFF_NONE;
    }
}

static void FieldEffects_FreeEffectBuffer(FieldEffects *effects) {
    int i;
    FieldEffectHandle *handle = effects->handles;

    for (i = 0; i < effects->count; i++, handle++) {
        if (handle->id != FLDEFF_NONE) {
            FieldEffects_GetVTable(handle->id)->free(effects, handle->data);
            handle->id = FLDEFF_NONE;
            handle->data = NULL;
        }
    }
    GFL_HeapFree(effects->handles);
    effects->handles = NULL;
}

static FieldEffectHandle *FieldEffects_GetHandleByID(FieldEffects *effects, u32 id) {
    u32 count = effects->count;
    FieldEffectHandle *handle = effects->handles;

    for (; count != 0; count--, handle++) {
        if (handle->id == id) {
            return handle;
        }
    }
    return NULL;
}

// The effect's functions, or the empty last entry for an unknown ID
static const FieldEffectVTable *FieldEffects_GetVTable(u32 id) {
    int i;
    const FieldEffectVTable *vtable = FIELD_EFFECT_VTABLES;

    for (i = 0; i < FLDEFF_MAX; i++, vtable++) {
        if (vtable->id == id) {
            return vtable;
        }
    }
    return vtable;
}

void FieldEffects_TCBManagerInit(FieldEffects *effects, u32 count) {
    effects->taskCount = count;
    effects->tcbMgrWork =
        GFL_HeapAllocate(effects->heapId, GFL_TCBMgrCalcAllocSize(count), FALSE, "field_effect.c", 476);
    effects->tcbMgr = GFL_TCBMgrCreate(count, effects->tcbMgrWork);
    effects->taskStore = FieldEffectTCBStore_Create(effects->heapId, effects->tcbMgr, count, effects);
}

FieldEffectTask *FieldEffects_TCBCreate(FieldEffects *effects, const FieldEffectTaskVTable *vtable, const VecFx32 *pos,
                                        u32 mode, void *params, u32 priority) {
    return FieldTerrainEffectTCB_Create(effects->taskStore, vtable, pos, mode, params, priority);
}

static void FieldEffects_ExitObjectSystem(FieldEffects *effects) {
    if (effects->taskStore != NULL) {
        FieldEffects_DeleteObjectMgr(effects->taskStore);
        GFL_TCBMgrDelete(effects->tcbMgr);
        GFL_HeapFree(effects->tcbMgrWork);
    }
}

static void FieldEffects_TCBManagerUpdate(FieldEffects *effects) {
    if (effects->tcbMgr != NULL) {
        GFL_TCBMgrUpdate(effects->tcbMgr);
    }
}

static void FieldEffects_DrawObjects(FieldEffects *effects) {
    if (effects->taskStore != NULL) {
        FieldEffects_DrawObjectMgr(effects->taskStore);
    }
}

static FieldEffectTaskStore *FieldEffectTCBStore_Create(HeapID heapId, TCBManager *tcbMgr, u32 count,
                                                        FieldEffects *effects) {
    FieldEffectTaskStore *store = GFL_HeapAllocate(heapId, sizeof(FieldEffectTaskStore), TRUE, "field_effect.c", 571);

    store->count = count;
    store->heapId = heapId;
    store->tasks = GFL_HeapAllocate(heapId, count * sizeof(FieldEffectTask), TRUE, "field_effect.c", 575);
    store->tcbMgr = tcbMgr;
    store->effects = effects;
    return store;
}

static void FieldEffects_DeleteObjectMgr(FieldEffectTaskStore *store) {
    u32 i;

    for (i = 0; i < store->count; i++) {
        if (store->tasks[i].active) {
            FieldEffects_DeleteObject(&store->tasks[i]);
        }
    }
    GFL_HeapFree(store->tasks);
    GFL_HeapFree(store);
}

static void FieldEffects_DrawObjectMgr(FieldEffectTaskStore *store) {
    u32 i;

    for (i = 0; i < store->count; i++) {
        if (store->tasks[i].active) {
            FieldEffects_DrawObject(&store->tasks[i]);
        }
    }
}

static FieldEffectTask *FieldTerrainEffectTCB_Create(FieldEffectTaskStore *store, const FieldEffectTaskVTable *vtable,
                                                     const VecFx32 *pos, u32 mode, void *params, u32 priority) {
    u32 i;
    FieldEffectTask *task = store->tasks;

    i = 0;
    do {
        if (!task->active) {
            MI_CpuFill8(task, 0, sizeof(FieldEffectTask));
            task->active = TRUE;
            task->vtable = *vtable;
            task->mode = mode;
            task->params = params;
            if (pos != NULL) {
                task->pos = *pos;
            }
            task->store = store;
            task->tcb = GFL_TCBMgrAddTask(store->tcbMgr, FieldEffects_ObjectTask, task, priority);
            break;
        }
        i++;
        task++;
    } while (i < store->count);
    if (i < store->count) {
        FieldEffects_InitObject(task);
        return task;
    }
    return NULL;
}

static void FieldEffects_ObjectTask(TCB *tcb, void *data) {
    FieldEffects_UpdateObject(data);
}

static void FieldEffects_InitObject(FieldEffectTask *task) {
    if (task != NULL) {
        task->vtable.init(task, task->work);
    }
}

void FieldEffects_DeleteObject(FieldEffectTask *task) {
    if (task != NULL) {
        task->vtable.delete(task, task->work);
        GFL_TCBRemove(task->tcb);
        task->active = FALSE;
    }
}

void FieldEffects_UpdateObject(FieldEffectTask *task) {
    if (task != NULL) {
        task->vtable.update(task, task->work);
    }
}

static void FieldEffects_DrawObject(FieldEffectTask *task) {
    if (task != NULL) {
        task->vtable.draw(task, task->work);
    }
}

u32 FieldEffects_GetObjectMode(FieldEffectTask *task) {
    if (task != NULL) {
        return task->mode;
    }
    return 0;
}

void *FieldEffects_GetObjectParam(FieldEffectTask *task) {
    if (task != NULL) {
        return task->params;
    }
    return NULL;
}

void FieldEffects_GetObjectPos(FieldEffectTask *task, VecFx32 *pos) {
    if (task != NULL) {
        *pos = task->pos;
    }
}

void FieldEffects_SetObjectPos(FieldEffectTask *task, const VecFx32 *pos) {
    if (task != NULL) {
        task->pos = *pos;
    }
}

void *FieldEffects_GetObjectWork(FieldEffectTask *task) {
    if (task != NULL) {
        return task->work;
    }
    return NULL;
}

void FieldEffects_SetLuminanceTable(FieldEffects *effects, const u8 *table) {
    effects->luminanceTable = table;
}

// Recolor the texture's palettes through the luminance table, if the map has one
void FieldEffects_ApplyLuminanceTable(FieldEffects *effects, G3DResource *resource) {
    NNSG3dResTex *tex;

    if (effects->luminanceTable != NULL) {
        tex = NNS_G3dGetTex(GFL_G3DResGetResData(resource));
        ColorFilter_ApplyLUT((u16 *)((u8 *)tex + tex->plttInfo.ofsPlttData),
                             (tex->plttInfo.sizePltt << 3) / sizeof(u16), effects->luminanceTable);
    }
}
