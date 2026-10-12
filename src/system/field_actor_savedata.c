#include "types.h"
#include "save/field_actor_savedata.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "gfl/std.h"

// The save block of the field's actors, and the index of an object code's record, which the actors' system reads its
// configs with. The ROM has no string for the file, so the name is descriptive. Block 0x29.

#define GRID_CENTER(n) ((n) * FX32_CONST(16) + FX32_CONST(8))

static void ActorSave_SaveActor(FieldActor *actor, FieldActorSaveEntry *entry);
static void ActorSave_LoadActor(FieldActor *actor, const FieldActorSaveEntry *entry, MMSys *system);
static BOOL ActorSave_IterateSaveActor(const MMSys *system, FieldActor **actor, u32 *index);
static FieldActor *ActorSave_IterateLoadActor(const MMSys *system);

u32 ActorSave_GetBlockSize(void) {
    return sizeof(FieldActorSave);
}

void ActorSave_Init(FieldActorSave *save) {
    sys_memset(save, 0, ActorSave_GetBlockSize());
}

void ActorSave_Save(MMSys *system, FieldActorSave *save) {
    u32 index = 0;
    FieldActor *actor;
    FieldActorSaveEntry *entry = save->actors;

    ActorSave_Init(save);
    while (ActorSave_IterateSaveActor(system, &actor, &index) == TRUE) {
        if (!(actor->movementFlags & 0x100000)) {
            ActorSave_SaveActor(actor, entry);
            entry++;
        }
    }
}

void ActorSave_Load(MMSys *system, FieldActorSave *save) {
    u32 i;
    FieldActorSaveEntry *entry = save->actors;

    for (i = 0; i < FIELD_ACTOR_SAVE_COUNT; i++) {
        if (entry->flags & 1) {
            ActorSave_LoadActor(ActorSave_IterateLoadActor(system), entry, system);
        }
        entry++;
    }
    system->actorCount = i;
}

static void ActorSave_SaveActor(FieldActor *actor, FieldActorSaveEntry *entry) {
    entry->flags = actor->flags;
    entry->collisionWidth = actor->collisionWidth;
    entry->collisionHeight = actor->collisionHeight;
    entry->uid = actor->uid;
    entry->zoneId = actor->zoneId;
    entry->modelId = actor->modelId;
    entry->moveCode = actor->moveCode;
    entry->evType = actor->evType;
    entry->spawnFlag = actor->spawnFlag;
    entry->scrId = actor->scrId;
    entry->defaultDir = actor->defaultDir;
    entry->faceDir = actor->faceDir;
    entry->motionDir = actor->motionDir;
    entry->param0 = actor->param0;
    entry->param1 = actor->param1;
    entry->param2 = actor->param2;
    entry->areaW = actor->areaW;
    entry->areaH = actor->areaH;
    entry->defaultGPosX = actor->defaultGPos.x;
    entry->defaultGPosY = actor->defaultGPos.y;
    entry->defaultGPosZ = actor->defaultGPos.z;
    entry->gPosX = actor->gPos.x;
    entry->gPosY = actor->gPos.y;
    entry->gPosZ = actor->gPos.z;
    entry->wPosY = actor->wPos.y;
    sys_memcpy(&actor->defaultRailPos, entry->railPos, sizeof(entry->railPos));
    sys_memcpy(actor->unkA4, entry->unk40, sizeof(entry->unk40));
}

// Puts the actor at the center of its tile, at the saved height
static inline void ActorSave_SetActorPos(FieldActor *actor, const FieldActorSaveEntry *entry) {
    VecFx32 pos = { 0, 0, 0 };

    actor->initGPos.x = actor->gPos.x;
    pos.x = GRID_CENTER(actor->gPos.x);
    actor->initGPos.y = actor->gPos.y;
    pos.y = entry->wPosY;
    actor->initGPos.z = actor->gPos.z;
    pos.z = GRID_CENTER(actor->gPos.z);
    actor->wPos = pos;
}

static void ActorSave_LoadActor(FieldActor *actor, const FieldActorSaveEntry *entry, MMSys *system) {
    const FieldActorConfig *config;

    sys_memset(actor, 0, sizeof(FieldActor));
    actor->flags = entry->flags;
    actor->collisionWidth = entry->collisionWidth;
    actor->collisionHeight = entry->collisionHeight;
    actor->uid = entry->uid;
    actor->zoneId = entry->zoneId;
    actor->modelId = entry->modelId;
    actor->moveCode = entry->moveCode;
    actor->evType = entry->evType;
    actor->spawnFlag = entry->spawnFlag;
    actor->scrId = entry->scrId;
    actor->defaultDir = entry->defaultDir;
    actor->faceDir = entry->faceDir;
    actor->motionDir = entry->motionDir;
    actor->param0 = entry->param0;
    actor->param1 = entry->param1;
    actor->param2 = entry->param2;
    actor->areaW = entry->areaW;
    actor->areaH = entry->areaH;
    actor->defaultGPos.x = entry->defaultGPosX;
    actor->defaultGPos.y = entry->defaultGPosY;
    actor->defaultGPos.z = entry->defaultGPosZ;
    actor->gPos.x = entry->gPosX;
    actor->gPos.y = entry->gPosY;
    actor->gPos.z = entry->gPosZ;
    sys_memcpy(entry->railPos, &actor->defaultRailPos, sizeof(entry->railPos));
    sys_memcpy(entry->unk40, actor->unkA4, sizeof(entry->unk40));
    actor->actorSystem = system;

    ActorSave_SetActorPos(actor, entry);

    actor->flags &= ~1;
    GetNPCMdlInfoForOBJCODE(system, actor->modelId, &actor->config);
    actor->flags |= 1;
    config = &actor->config;
    actor->modelPosOffsetX = config->wPosOffsetX;
    actor->modelPosOffsetY = config->wPosOffsetY;
    actor->modelPosOffsetZ = config->wPosOffsetZ;

    actor->flags |= 1;
    actor->movementFlags |= 0x800;
    if (actor->flags & 2) {
        actor->movementFlags |= 1;
    }
}

// Steps through the system's actors from *index, as fldmmdl.c's NextActor does
static BOOL ActorSave_IterateSaveActor(const MMSys *system, FieldActor **actor, u32 *index) {
    u32 limit = system->actorCapacity;
    FieldActor *candidate;

    if (*index < limit) {
        candidate = &system->actorHeap[*index];
        do {
            (*index)++;
            if (candidate->flags & 1) {
                *actor = candidate;
                return TRUE;
            }
            candidate++;
        } while (*index < limit);
    }
    return FALSE;
}

// The first actor of the system that isn't in use, or NULL
static FieldActor *ActorSave_IterateLoadActor(const MMSys *system) {
    int i = 0;
    int limit = system->actorCapacity;
    FieldActor *actor = system->actorHeap;

    do {
        if (!(actor->flags & 1)) {
            return actor;
        }
        i++;
        actor++;
    } while (i < limit);
    return NULL;
}

u32 GetIndexOfObjID(u16 objCode) {
    if (objCode < 0x179) {
        return objCode;
    }
    if (objCode >= 0x1000 && objCode < 0x126c) {
        return (u16)(objCode - 0x1000 + 0x179);
    }
    if (objCode >= 0x2000 && objCode < 0x200b) {
        return (u16)(objCode - 0x2000 + 0x3e5);
    }
    return 10;
}
