#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/resort.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "save/join_avenue.h"
#include "system/resort_binary.h"
#include "system/resort_layout.h"

// The avenue's NPCs, one per row of a table, whose actors have the UIDs from RESORT_NPC_UID_FIRST
#define RESORT_NPC_UID_FIRST 0x94
#define RESORT_NPC_UID_END 0xa6

// The columns of the NPCs' table
enum {
    RESORT_NPC_COL_MODEL,
    RESORT_NPC_COL_ZONE,
    RESORT_NPC_COL_X,
    RESORT_NPC_COL_Z,
    RESORT_NPC_COL_RANK,
    RESORT_NPC_COL_MOVE_CODE,
    RESORT_NPC_COL_AREA_W,
    RESORT_NPC_COL_AREA_H,
    RESORT_NPC_COL_SCRIPT,
    RESORT_NPC_COL_TEXT_ROW,
};

struct ResortNPC {
    void *table;
    u32 unk4;
    void *texts;
    ResortNPCSetup setup;
    BOOL eventRunning;
    u32 count;
    BOOL changed;
};

static void ResortNPC_CreateActors(ResortNPC *npc);
static u32 ResortNPC_SetMovementStopped(ResortNPC *npc, BOOL stop);
static BOOL ResortNPC_IsNPCUID(int uid);

ResortNPC *ResortNPC_Create(const ResortNPCSetup *setup, HeapID heapId) {
    ResortNPC *npc = GFL_HeapAllocate(heapId, sizeof(ResortNPC), TRUE, "resort_npc.c", 66);
    npc->setup = *setup;
    npc->table = setup->table;
    npc->unk4 = 0;
    npc->texts = ResortBinary_Load(6, 20, heapId);
    ResortNPC_CreateActors(npc);
    return npc;
}

void ResortNPC_Free(ResortNPC *npc) {
    ResortBinary_Free(npc->texts);
    GFL_HeapFree(npc);
}

void ResortNPC_Update(ResortNPC *npc, Field *field) {
    if (Field_IsEventRunning(field)) {
        if (npc->eventRunning == FALSE) {
            npc->changed = TRUE;
            npc->eventRunning = TRUE;
        }
    } else {
        if (npc->eventRunning == TRUE) {
            npc->changed = TRUE;
            npc->eventRunning = FALSE;
        }
    }
    if (npc->changed) {
        if (ResortNPC_SetMovementStopped(npc, npc->eventRunning) == 0) {
            npc->changed = FALSE;
        }
    }
}

FieldActor *ResortNPC_GetActor(ResortNPC *npc, u32 index) {
    u32 i;
    for (i = 0; i < ResortBinary_GetRowCount(npc->table); i++) {
        FieldActor *actor = FindFieldActor(npc->setup.mmSys, i + RESORT_NPC_UID_FIRST);
        if (actor != NULL && i == index) {
            return actor;
        }
    }
    return NULL;
}

u32 ResortNPC_GetCount(ResortNPC *npc) {
    return npc->count;
}

void ResortNPC_SetVisible(ResortNPC *npc, BOOL visible) {
    u32 index = 0;
    FieldActor *actor;
    while (NextActor(npc->setup.mmSys, &actor, &index) == TRUE) {
        if (ResortNPC_IsNPCUID(GetActorUID(actor)) &&
            GetActorZoneID(actor) == ResortLayout_GetZoneId(npc->setup.zone)) {
            SetActorHidden(actor, !visible);
        }
    }
}

u16 ResortNPC_GetTextEntry(ResortNPC *npc, u32 row, u32 column) {
    return ResortBinary_Get(npc->texts, ResortBinary_Get(npc->table, row, RESORT_NPC_COL_TEXT_ROW), column);
}

static void ResortNPC_CreateActors(ResortNPC *npc) {
    u32 i;
    for (i = 0; i < ResortBinary_GetRowCount(npc->table); i++) {
        u16 rank = JoinAvenue_GetParam(npc->setup.info, 2, 0);
        u32 minRank = ResortBinary_Get(npc->table, i, RESORT_NPC_COL_RANK);
        u32 zone = ResortBinary_Get(npc->table, i, RESORT_NPC_COL_ZONE);
        FieldActor *actor = FindFieldActor(npc->setup.mmSys, i + RESORT_NPC_UID_FIRST);
        if (minRank <= rank && actor == NULL && npc->setup.zone == zone) {
            ZoneNPC zoneNpc;
            MI_CpuFill8(&zoneNpc, 0, sizeof(ZoneNPC));
            zoneNpc.uid = i + RESORT_NPC_UID_FIRST;
            zoneNpc.modelId = ResortBinary_Get(npc->table, i, RESORT_NPC_COL_MODEL);
            zoneNpc.moveCode = ResortBinary_Get(npc->table, i, RESORT_NPC_COL_MOVE_CODE);
            zoneNpc.direction = DIR_DOWN;
            switch (ResortBinary_Get(npc->table, i, RESORT_NPC_COL_SCRIPT)) {
            case 1:
                zoneNpc.scrId = 10695;
                break;
            case 0:
                zoneNpc.scrId = 10694;
                break;
            }
            zoneNpc.areaW = ResortBinary_Get(npc->table, i, RESORT_NPC_COL_AREA_W);
            zoneNpc.areaH = ResortBinary_Get(npc->table, i, RESORT_NPC_COL_AREA_H);
            ZoneNPC_SetGridPos(&zoneNpc, ResortBinary_Get(npc->table, i, RESORT_NPC_COL_X),
                               ResortBinary_Get(npc->table, i, RESORT_NPC_COL_Z), 0);
            CreateNewActorByEntityNoWKOBJCODE(npc->setup.mmSys, &zoneNpc, ResortLayout_GetZoneId(npc->setup.zone));
            npc->count++;
        }
    }
}

static u32 ResortNPC_SetMovementStopped(ResortNPC *npc, BOOL stop) {
    u32 index = 0;
    FieldActor *actor;
    u32 count = 0;
    while (NextActor(npc->setup.mmSys, &actor, &index) == TRUE) {
        if (ResortNPC_IsNPCUID(GetActorUID(actor)) &&
            GetActorZoneID(actor) == ResortLayout_GetZoneId(npc->setup.zone)) {
            if (IsActorFlag16(actor)) {
                count++;
            } else if (stop) {
                DisableActorMovement(actor);
            } else {
                EnableActorMovement(actor);
            }
        }
    }
    return count;
}

u16 ResortNPC_GetTextRow(ResortNPC *npc, u32 row) {
    return ResortBinary_Get(npc->table, row, RESORT_NPC_COL_TEXT_ROW);
}

static BOOL ResortNPC_IsNPCUID(int uid) {
    return uid >= RESORT_NPC_UID_FIRST && uid < RESORT_NPC_UID_END;
}
