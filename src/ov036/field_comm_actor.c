// The actors of the other players in a field communication. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); FieldCommActor_MoveInit and FieldCommActor_Move are ours.
#include "types.h"
#include "constants/obj_codes.h"
#include "field/field_actor.h"
#include "field/field_comm_actor.h"
#include "gfl/heap.h"
#include "nitro/mi.h"

typedef struct {
    u32 netId;
    u16 actorDeleted;
    // Whether a player's actor that moves far in a frame is drawn running
    u16 allowRun;
    FieldActor *actor;
} FldCommActEntry;

struct FldCommActSys {
    u16 count;
    u16 allowRun;
    MMSys *actorSystem;
    FldCommActEntry *entries;
};

// What an actor's position block holds while it is another player's actor
typedef struct {
    const u16 *dir;
    const VecFx32 *pos;
    const u32 *hidden;
    FldCommActEntry *entry;
} FldCommActLink;

static void FieldCommActor_Reset(FldCommActEntry *entry);
static FieldActor *FieldCommActor_Add(FldCommActSys *sys, u16 modelId, const u16 *dir, const VecFx32 *pos,
                                      const u32 *hidden, FldCommActEntry *entry);
static void FieldCommActor_SetMoveWork(FieldActor *actor, const u16 *dir, const VecFx32 *pos, const u32 *hidden,
                                       FldCommActEntry *entry);
static u16 FieldCommActor_ConvertDir(u16 dir);

FldCommActSys *FldCommActSys_Create(u8 count, MMSys *actorSystem, HeapID heapId, BOOL allowRun) {
    FldCommActSys *sys = GFL_HeapAllocate(heapId, sizeof(FldCommActSys), TRUE, "field_comm_actor.c", 93);

    sys->count = count;
    sys->allowRun = allowRun;
    sys->actorSystem = actorSystem;
    sys->entries = GFL_HeapAllocate(heapId, sizeof(FldCommActEntry) * count, TRUE, "field_comm_actor.c", 99);
    return sys;
}

void FldCommActSys_Free(FldCommActSys *sys) {
    u16 i;
    FldCommActEntry *entry = sys->entries;

    for (i = 0; i < sys->count; i++, entry++) {
        if (entry->actor != NULL) {
            FieldCommActor_Reset(entry);
        }
    }
    GFL_HeapFree(sys->entries);
    GFL_HeapFree(sys);
}

void FldCommActSys_CreateActor(FldCommActSys *sys, u32 netId, u16 modelId, const u16 *dir, const VecFx32 *pos,
                               const u32 *hidden) {
    u16 i;
    FldCommActEntry *entry = sys->entries;

    FieldCommActor_ConvertDir(*dir);
    for (i = 0; i < sys->count; i++, entry++) {
        if (entry->actor == NULL) {
            entry->actor = FieldCommActor_Add(sys, modelId, dir, pos, hidden, entry);
            entry->netId = netId;
            entry->allowRun = sys->allowRun;
            return;
        }
    }
}

void FldCommActSys_DeleteActor(FldCommActSys *sys, u32 netId) {
    u16 i;
    FldCommActEntry *entry = sys->entries;

    for (i = 0; i < sys->count; i++, entry++) {
        if (entry->actor != NULL && entry->netId == netId) {
            FieldCommActor_Reset(entry);
            return;
        }
    }
}

BOOL FldCommActSys_FindActor(FldCommActSys *sys, s16 x, s16 y, s16 z, u32 *netId, u32 *index) {
    u16 count = sys->count;
    FldCommActEntry *entry;

    if (*index < count) {
        entry = &sys->entries[*index];
        do {
            s16 dy;

            (*index)++;
            if (entry->actor == NULL) {
                // BUG: only the index moves past an empty entry, so the search stays on it to the end and misses the
                // actors of the entries after it
#ifdef BUGFIX
                entry++;
#endif
                continue;
            }
            dy = FldAct_GetGPosY(entry->actor) - y;
            if (dy < 0) {
                dy = -dy;
            }
            if (dy < 1 && CheckActorVolumeOverGPos(entry->actor, x, z, TRUE)) {
                *netId = entry->netId;
                return TRUE;
            }
            entry++;
        } while (*index < count);
    }
    return FALSE;
}

static void FieldCommActor_Reset(FldCommActEntry *entry) {
    if (entry->actorDeleted == 0) {
        DeleteActor(entry->actor);
    }
    MI_CpuFill8(entry, 0, sizeof(FldCommActEntry));
}

// The entity that the actors are created from, with the model of each player filled in
static const ZoneNPC FIELD_COMM_ACTOR_NPC = {
    0xfc, 0, MOVE_CODE_COMM_ACTOR, ACTOR_EVENT_TYPE_NONE, 0, 0x7d0, 0, 0, 0, 0, 4, 4, FALSE, { { 0, 0, 0 } },
};

static FieldActor *FieldCommActor_Add(FldCommActSys *sys, u16 modelId, const u16 *dir, const VecFx32 *pos,
                                      const u32 *hidden, FldCommActEntry *entry) {
    MMSys *actorSystem = sys->actorSystem;
    ZoneNPC npc = FIELD_COMM_ACTOR_NPC;
    FieldActor *actor;

    npc.modelId = modelId;
    actor = CreateNewActorByEntityNoWKOBJCODE(actorSystem, &npc, 0);
    FieldCommActor_SetMoveWork(actor, dir, pos, hidden, entry);
    SetActorWPosAll(actor, pos, FieldCommActor_ConvertDir(*dir));
    FldAct_SetHeightCalcDisabled(actor, TRUE);
    FldAct_SetMapAttrDisabled(actor, TRUE);
    SetActorFlag32(actor, TRUE);
    SetActorCollisionEnabled(actor, FALSE);
    FldAct_SetIgnoreDisableAll(actor, TRUE);
    FldAct_SetMoveBitAttrEffectOff(actor, TRUE);
    return actor;
}

void FieldCommActor_MoveInit(FieldActor *actor) {
    ClearActorPositionBlock(actor, sizeof(FldCommActLink));
}

void FieldCommActor_MoveDelete(FieldActor *actor) {
    FldCommActLink *link = GetNPCRailPosPtrAddr(actor);

    link->entry->actorDeleted = 1;
}

void FieldCommActor_Move(FieldActor *actor) {
    FldCommActLink *link = GetNPCRailPosPtrAddr(actor);
    VecFx32 pos;
    u32 state;

    CheckSetActorFaceDir(actor, FieldCommActor_ConvertDir(*link->dir));
    state = DRAW_STATUS_STOP;
    CopyActorWPos(actor, &pos);
    if (pos.x != link->pos->x || pos.y != link->pos->y || pos.z != link->pos->z) {
        SetActorWPosAll(actor, link->pos, GetActorFaceDir(actor));
        state = DRAW_STATUS_WALK_8F;
        if (link->entry->allowRun != 0) {
            u16 objCode = FldAct_GetObjCode(actor);

            if (objCode == OBJ_CODE_HERO || objCode == OBJ_CODE_HEROINE) {
                s32 dx;
                s32 dz;
                s32 dist = 0;

                dx = pos.x - link->pos->x;
                if (dx < 0) {
                    dx = -dx;
                }
                dist += dx;
                dz = pos.z - link->pos->z;
                if (dz < 0) {
                    dz = -dz;
                }
                if (dist + dz >= FX32_CONST(4)) {
                    state = DRAW_STATUS_ANIME_10;
                }
            }
        }
    }
    FldAct_SetDrawStatus(actor, state);
    if (link->hidden != NULL) {
        BOOL hide = FALSE;

        if (*link->hidden == 1) {
            hide = TRUE;
        }
        SetActorHidden(actor, hide);
    }
}

static void FieldCommActor_SetMoveWork(FieldActor *actor, const u16 *dir, const VecFx32 *pos, const u32 *hidden,
                                       FldCommActEntry *entry) {
    FldCommActLink *link = GetNPCRailPosPtrAddr(actor);

    link->dir = dir;
    link->pos = pos;
    link->hidden = hidden;
    link->entry = entry;
}

static u16 FieldCommActor_ConvertDir(u16 dir) {
    return dir;
}
