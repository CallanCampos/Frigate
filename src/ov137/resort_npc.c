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
#define RESORT_NPC_COL_MODEL 0
#define RESORT_NPC_COL_ZONE 1
#define RESORT_NPC_COL_X 2
#define RESORT_NPC_COL_Z 3
#define RESORT_NPC_COL_RANK 4
#define RESORT_NPC_COL_MOVE_CODE 5
#define RESORT_NPC_COL_AREA_W 6
#define RESORT_NPC_COL_AREA_H 7
#define RESORT_NPC_COL_SCRIPT 8
#define RESORT_NPC_COL_TEXT_ROW 9

struct ResortNPC {
    void *table;
    u32 unk4;
    void *texts;
    ResortNPCSetup setup;
    BOOL eventRunning;
    u32 count;
    BOOL changed;
};

static void func_ov137_021f1d7c(ResortNPC *npc);
static u32 func_ov137_021f1e90(ResortNPC *npc, BOOL stop);
static BOOL func_ov137_021f1f0c(int uid);

ResortNPC *func_ov137_021f1c24(const ResortNPCSetup *setup, HeapID heapId) {
    ResortNPC *npc = GFL_HeapAllocate(heapId, sizeof(ResortNPC), TRUE, "resort_npc.c", 66);
    npc->setup = *setup;
    npc->table = setup->table;
    npc->unk4 = 0;
    npc->texts = ResortBinary_Load(6, 20, heapId);
    func_ov137_021f1d7c(npc);
    return npc;
}

void func_ov137_021f1c74(ResortNPC *npc) {
    ResortBinary_Free(npc->texts);
    GFL_HeapFree(npc);
}

void func_ov137_021f1c88(ResortNPC *npc, Field *field) {
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
        if (func_ov137_021f1e90(npc, npc->eventRunning) == 0) {
            npc->changed = FALSE;
        }
    }
}

FieldActor *func_ov137_021f1cc8(ResortNPC *npc, u32 index) {
    u32 i;
    for (i = 0; i < ResortBinary_GetRowCount(npc->table); i++) {
        FieldActor *actor = FindFieldActor(npc->setup.mmSys, i + RESORT_NPC_UID_FIRST);
        if (actor != NULL && i == index) {
            return actor;
        }
    }
    return NULL;
}

u32 func_ov137_021f1d00(ResortNPC *npc) {
    return npc->count;
}

void func_ov137_021f1d04(ResortNPC *npc, BOOL visible) {
    u32 index = 0;
    FieldActor *actor;
    while (NextActor(npc->setup.mmSys, &actor, &index) == TRUE) {
        if (func_ov137_021f1f0c(GetActorUID(actor)) &&
            GetActorZoneID(actor) == ResortLayout_GetZoneId(npc->setup.zone)) {
            SetActorHidden(actor, !visible);
        }
    }
}

u16 func_ov137_021f1d60(ResortNPC *npc, u32 row, u32 column) {
    return ResortBinary_Get(npc->texts, ResortBinary_Get(npc->table, row, RESORT_NPC_COL_TEXT_ROW), column);
}

static void func_ov137_021f1d7c(ResortNPC *npc) {
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
            func_ov012_021682c0(&zoneNpc, ResortBinary_Get(npc->table, i, RESORT_NPC_COL_X),
                                ResortBinary_Get(npc->table, i, RESORT_NPC_COL_Z), 0);
            CreateNewActorByEntityNoWKOBJCODE(npc->setup.mmSys, &zoneNpc, ResortLayout_GetZoneId(npc->setup.zone));
            npc->count++;
        }
    }
}

static u32 func_ov137_021f1e90(ResortNPC *npc, BOOL stop) {
    u32 index = 0;
    FieldActor *actor;
    u32 count = 0;
    while (NextActor(npc->setup.mmSys, &actor, &index) == TRUE) {
        if (func_ov137_021f1f0c(GetActorUID(actor)) &&
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

u16 func_ov137_021f1f00(ResortNPC *npc, u32 row) {
    return ResortBinary_Get(npc->table, row, RESORT_NPC_COL_TEXT_ROW);
}

static BOOL func_ov137_021f1f0c(int uid) {
    return uid >= RESORT_NPC_UID_FIRST && uid < RESORT_NPC_UID_END;
}
