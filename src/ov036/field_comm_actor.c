// The actors of the other players in a field communication. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_actor.h"
#include "field/field_comm_actor.h"
#include "gfl/heap.h"
#include "nitro/mi.h"

typedef struct {
    u32 netId;
    u16 unk4;
    u16 unk6;
    FieldActor *actor;
} FldCommActEntry;

struct FldCommActSys {
    u16 count;
    u16 unk2;
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
static void func_ov036_0218b110(FieldActor *actor, const u16 *dir, const VecFx32 *pos, const u32 *hidden,
                                FldCommActEntry *entry);
static u16 func_ov036_0218b128(u16 dir);

FldCommActSys *FldCommActSys_Create(u8 count, MMSys *actorSystem, HeapID heapId, u8 a3) {
    FldCommActSys *sys = GFL_HeapAllocate(heapId, sizeof(FldCommActSys), TRUE, "field_comm_actor.c", 93);

    sys->count = count;
    sys->unk2 = a3;
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

    func_ov036_0218b128(*dir);
    for (i = 0; i < sys->count; i++, entry++) {
        if (entry->actor == NULL) {
            entry->actor = FieldCommActor_Add(sys, modelId, dir, pos, hidden, entry);
            entry->netId = netId;
            entry->unk6 = sys->unk2;
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
    if (entry->unk4 == 0) {
        DeleteActor(entry->actor);
    }
    MI_CpuFill8(entry, 0, sizeof(FldCommActEntry));
}

// The entity that the actors are created from, with the model of each player filled in
static const ZoneNPC FIELD_COMM_ACTOR_NPC = {
    0xfc, 0, 0x44, 0, 0, 0x7d0, 0, 0, 0, 0, 4, 4, FALSE, { { 0, 0, 0 } },
};

static FieldActor *FieldCommActor_Add(FldCommActSys *sys, u16 modelId, const u16 *dir, const VecFx32 *pos,
                                      const u32 *hidden, FldCommActEntry *entry) {
    MMSys *actorSystem = sys->actorSystem;
    ZoneNPC npc = FIELD_COMM_ACTOR_NPC;
    FieldActor *actor;

    npc.modelId = modelId;
    actor = CreateNewActorByEntityNoWKOBJCODE(actorSystem, &npc, 0);
    func_ov036_0218b110(actor, dir, pos, hidden, entry);
    SetActorWPosAll(actor, pos, func_ov036_0218b128(*dir));
    func_ov012_0216763c(actor, TRUE);
    func_ov012_02167788(actor, TRUE);
    SetActorFlag32(actor, TRUE);
    func_ov012_02167580(actor, FALSE);
    func_ov012_02167754(actor, TRUE);
    func_ov012_021677bc(actor, TRUE);
    return actor;
}

void func_ov036_0218b044(FieldActor *actor) {
    ClearActorPositionBlock(actor, sizeof(FldCommActLink));
}

void func_ov036_0218b050(FieldActor *actor) {
    FldCommActLink *link = (FldCommActLink *)GetNPCRailPosPtrAddr(actor);

    link->entry->unk4 = 1;
}

void func_ov036_0218b060(FieldActor *actor) {
    FldCommActLink *link = (FldCommActLink *)GetNPCRailPosPtrAddr(actor);
    VecFx32 pos;
    u32 state;

    CheckSetActorFaceDir(actor, func_ov036_0218b128(*link->dir));
    state = 0;
    CopyActorWPos(actor, &pos);
    if (pos.x != link->pos->x || pos.y != link->pos->y || pos.z != link->pos->z) {
        SetActorWPosAll(actor, link->pos, GetActorFaceDir(actor));
        state = 3;
        if (link->entry->unk6 != 0) {
            u16 objCode = FldAct_GetObjCode(actor);

            if (objCode == 0xe7 || objCode == 0xf0) {
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
                if (dist + dz >= 0x4000) {
                    state = 10;
                }
            }
        }
    }
    func_ov012_021670f4(actor, state);
    if (link->hidden != NULL) {
        BOOL hide = FALSE;

        if (*link->hidden == 1) {
            hide = TRUE;
        }
        SetActorHidden(actor, hide);
    }
}

static void func_ov036_0218b110(FieldActor *actor, const u16 *dir, const VecFx32 *pos, const u32 *hidden,
                                FldCommActEntry *entry) {
    FldCommActLink *link = (FldCommActLink *)GetNPCRailPosPtrAddr(actor);

    link->dir = dir;
    link->pos = pos;
    link->hidden = hidden;
    link->entry = entry;
}

static u16 func_ov036_0218b128(u16 dir) {
    return dir;
}
