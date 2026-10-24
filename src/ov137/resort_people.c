#include "types.h"
#include "field/field_actor.h"
#include "field/resort.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "save/join_avenue.h"
#include "system/resort_binary.h"
#include "system/resort_layout.h"

// The people's actors have the UIDs from RESORT_PERSON_UID_FIRST, by their index
#define RESORT_PERSON_UID_FIRST 0x50
#define RESORT_PERSON_UID_END 0x94

// What a person is created with
typedef struct {
    ResortPersonData *data;
    u16 index;
    u32 zone;
    MMSys *mmSys;
    ResortBinary *table;
    ResortShopData *shops;
    ResortBinary *unk18;
    u32 unk1c;
    BOOL unk20;
} ResortPersonSetup;

// The functions of a type of person: the actor it is created as, and where the actor stands
typedef struct {
    void (*setupNpc)(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc);
    void (*getPos)(ResortPerson *person, u16 *x, u16 *z, u16 *dir);
} ResortPersonType;

struct ResortPerson {
    u32 type;
    u32 index;
    u32 unk8;
    BOOL visible;
    BOOL unk10;
    MMSys *mmSys;
    FieldActor *actor;
    ResortPersonData *data;
    const u16 *row;
    const ResortPersonType *funcs;
    ResortShopData *shops;
    ResortBinary *unk2c;
};

struct ResortPeople {
    ResortPeopleSetup setup;
    HeapID heapId;
    ResortBinary *table;
    ResortPerson *persons[];
};

static void ResortPerson_Free(ResortPerson *person);
static BOOL ResortPerson_HasActor(ResortPerson *person, FieldActor *actor);
static void ResortPerson_Recreate(ResortPerson *person, const ResortPersonSetup *setup);
static BOOL ResortPerson_IsJoinAvenuePerson(ResortPerson *person, JoinAvenuePerson *joinPerson);
static BOOL ResortPerson_HasData(ResortPerson *person, ResortPersonData *data);
static void ResortPerson_SetupOccupantNpc(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc);
static void ResortPerson_GetOccupantPos(ResortPerson *person, u16 *x, u16 *z, u16 *dir);
static void ResortPerson_SetupEntryNpc(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc);
static void ResortPerson_SetupListNpc(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc);
static void ResortPerson_SetupRecordNpc(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc);
static void ResortPerson_GetRecordPos(ResortPerson *person, u16 *x, u16 *z, u16 *dir);
static void ResortPerson_CreateActor(ResortPerson *person, const ResortPersonSetup *setup);
static ResortPerson *ResortPeople_AddPerson(ResortPeople *people, const ResortPersonSource *source, BOOL unk20,
                                            u16 index);
static void ResortPeople_RemovePerson(ResortPeople *people, ResortPerson *person);
static void ResortPeople_FreeAll(ResortPeople *people);
static void ResortPeople_DeleteOrphanActors(ResortPeople *people);
static void ResortPeople_MakeSetup(ResortPeople *people, const ResortPersonSource *source, u32 index,
                                   ResortPersonSetup *setup);
static BOOL ResortPeople_IsPersonUID(int uid);

static const u32 sKinds[] = { 0, 4 };
// clang-format off
static const ResortPersonType sTypes[] = {
    {ResortPerson_SetupOccupantNpc, ResortPerson_GetOccupantPos},
    {ResortPerson_SetupEntryNpc, NULL},
    {ResortPerson_SetupListNpc, NULL},
    {ResortPerson_SetupRecordNpc, ResortPerson_GetRecordPos},
};

// clang-format on
static const ZoneNPC sNpcTemplate = { 0 };

static ResortPerson *ResortPerson_Create(HeapID heapId, const ResortPersonSetup *setup) {
    u32 a;
    u32 b;
    ResortPerson *person = GFL_HeapAllocate(heapId, sizeof(ResortPerson), TRUE, "resort_people.c", 166);

    person->index = setup->index;
    person->mmSys = setup->mmSys;
    person->data = setup->data;
    person->unk10 = setup->unk20;
    person->shops = setup->shops;
    person->unk2c = setup->unk18;
    person->type = ResortData_GetType(person->data);
    a = ResortData_GetParam(person->data, JOIN_AVE_PARAM_SHOP_KIND, NULL);
    b = ResortData_GetParam(person->data, JOIN_AVE_PARAM_GENDER, NULL);
    person->row = ResortBinary_FindRow(setup->table, person->type, a, b);
    person->funcs = &sTypes[person->type];
    person->actor = FindFieldActor(person->mmSys, person->index + RESORT_PERSON_UID_FIRST);
    if (person->actor == NULL) {
        ResortPerson_CreateActor(person, setup);
    }
    person->visible = !IsActorHidden(person->actor);
    return person;
}

static void ResortPerson_Free(ResortPerson *person) {
    GFL_HeapFree(person);
}

static BOOL ResortPerson_HasActor(ResortPerson *person, FieldActor *actor) {
    if (actor == ResortPerson_GetActor(person)) {
        return TRUE;
    }
    return FALSE;
}

JoinAvenuePerson *ResortPerson_GetJoinAvenuePerson(ResortPerson *person) {
    return ResortData_GetPerson(person->data);
}

void ResortPerson_UpdateObjCode(ResortPerson *person) {
    FieldActor *actor = ResortPerson_GetActor(person);
    u16 objCode = ResortData_GetParam(person->data, JOIN_AVE_PARAM_TRAINER_TYPE, NULL);

    FldAct_UpdateBlInfoForNewObjCode(actor, objCode);
}

void ResortPerson_SetTalking(ResortPerson *person, u32 value) {
    person->unk8 = value;
}

static void ResortPerson_Recreate(ResortPerson *person, const ResortPersonSetup *setup) {
    if (person->actor != NULL) {
        DeleteActor(person->actor);
        person->actor = NULL;
    }
    ResortPerson_CreateActor(person, setup);
}

JoinAvenueShop *ResortPerson_GetShop(ResortPerson *person) {
    return JoinAvenuePerson_GetShop(ResortData_GetPerson(person->data));
}

void ResortPerson_MoveToHome(ResortPerson *person) {
    if (person->funcs->getPos != NULL) {
        u16 x;
        u16 z;
        u16 dir;
        VecFx32 pos;
        FieldActor *actor = ResortPerson_GetActor(person);

        person->funcs->getPos(person, &x, &z, &dir);
        pos.x = (x << 16) + 0x8000;
        pos.y = 0;
        pos.z = (z << 16) + 0x8000;
        SetActorWPosValue(actor, &pos);
        SetActorGPosX(actor, x);
        SetActorGPosZ(actor, z);
        CheckSetActorFaceDir(actor, dir);
    }
}

void ResortPerson_GetFacingPos(ResortPerson *person, u16 *x, u16 *z, u16 *dir) {
    person->funcs->getPos(person, x, z, dir);
    if (*dir == DIR_LEFT) {
        *x -= 1;
        *dir = DIR_RIGHT;
    } else {
        *x += 1;
        *dir = DIR_LEFT;
    }
}

static BOOL ResortPerson_IsJoinAvenuePerson(ResortPerson *person, JoinAvenuePerson *joinPerson) {
    if (ResortData_GetPerson(person->data) == joinPerson) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ResortPerson_HasData(ResortPerson *person, ResortPersonData *data) {
    if (person->data == data) {
        return TRUE;
    }
    return FALSE;
}

void ResortPerson_SetVisible(ResortPerson *person, BOOL visible) {
    FieldActor *actor = ResortPerson_GetActor(person);
    SetActorHidden(actor, visible == FALSE);
    SetActorCollisionEnabled(actor, visible);
    person->visible = visible;
}

void ResortPerson_ApplyVisibility(ResortPerson *person, u32 mode) {
    FieldActor *actor = ResortPerson_GetActor(person);
    IsActorHidden(actor);
    switch (mode) {
    case 0:
        SetActorHidden(actor, FALSE);
        break;
    case 1:
        SetActorHidden(actor, TRUE);
        break;
    case 2:
        SetActorHidden(actor, person->visible == FALSE);
        break;
    }
}

void ResortPerson_SetParam(ResortPerson *person, JoinAvenuePersonParam param, u32 value) {
    ResortData_SetParam(person->data, param, value);
}

uptr ResortPerson_GetParam(ResortPerson *person, JoinAvenuePersonParam param, void *buffer) {
    return ResortData_GetParam(person->data, param, buffer);
}

u32 ResortPerson_GetAttr(ResortPerson *person, u32 which) {
    switch (which) {
    case 0:
        return person->type;
    case 1:
        return person->unk10;
    }
    return 0;
}

FieldActor *ResortPerson_GetActor(ResortPerson *person) {
    return person->actor;
}

ResortPersonData *ResortPerson_GetData(ResortPerson *person) {
    return person->data;
}

static void ResortPerson_SetupOccupantNpc(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc) {
    u16 x;
    u16 z;
    u16 dir;

    ResortPerson_GetOccupantPos(person, &x, &z, &dir);
    npc->direction = dir;
    ZoneNPC_SetGridPos(npc, x, z, 0);
}

static void ResortPerson_GetOccupantPos(ResortPerson *person, u16 *x, u16 *z, u16 *dir) {
    u16 index = ResortData_GetIndex(person->data);
    u16 id = JoinAvenueShop_GetField(ResortPerson_GetShop(person), 0);
    u32 column = 0;
    const u16 *row = ResortShopData_GetShop(person->shops, id);

    if (index % 2 == 0) {
        column = 1;
    }
    ResortLayout_GetOccupantPlace(ResortShopData_GetShopParam(row, column + 6), x, z, dir);
    *x += ResortBinary_Get(person->unk2c, index, 1);
    *z += ResortBinary_Get(person->unk2c, index, 2);
}

static void ResortPerson_SetupEntryNpc(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc) {
    npc->direction = 0;
    ZoneNPC_SetGridPos(npc, 0, 0, 0);
}

static void ResortPerson_SetupListNpc(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc) {
    npc->direction = 0;
    ZoneNPC_SetGridPos(npc, 0, 0, 0);
}

static void ResortPerson_SetupRecordNpc(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc) {
    u16 x;
    u16 z;
    u16 dir;

    ResortPerson_GetRecordPos(person, &x, &z, &dir);
    npc->direction = dir;
    ZoneNPC_SetGridPos(npc, x, z, 0);
}

static void ResortPerson_GetRecordPos(ResortPerson *person, u16 *x, u16 *z, u16 *dir) {
    ResortLayout_GetRecordPlace(ResortData_GetParam(person->data, JOIN_AVE_PARAM_SHOP_KIND, NULL), x, z, dir);
}

static void ResortPerson_CreateActor(ResortPerson *person, const ResortPersonSetup *setup) {
    ZoneNPC npc = sNpcTemplate;

    npc.modelId = (u8)ResortData_GetParam(person->data, JOIN_AVE_PARAM_TRAINER_TYPE, NULL);
    npc.scrId = ResortBinary_GetFoundValue(person->row);
    npc.uid = setup->index + RESORT_PERSON_UID_FIRST;
    person->funcs->setupNpc(person, setup, &npc);
    person->actor = CreateNewActorByEntityNoWKOBJCODE(setup->mmSys, &npc, ResortLayout_GetZoneId(setup->zone));
    ResortPerson_SetParam(person, JOIN_AVE_PARAM_SLOT, setup->index);
    ResortPerson_SetParam(person, JOIN_AVE_PARAM_ROW, setup->zone);
    person->visible = !IsActorHidden(person->actor);
}

ResortPeople *ResortPeople_Create(const ResortPeopleSetup *setup, HeapID heapId) {
    ResortPeople *people = GFL_HeapAllocate(heapId, sizeof(ResortPeople) + sizeof(ResortPerson *) * setup->count, TRUE,
                                            "resort_people.c", 712);
    people->setup = *setup;
    people->heapId = heapId;
    people->table = ResortBinary_Load(0, 4, heapId);
    return people;
}

void ResortPeople_Free(ResortPeople *people) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        ResortPerson *person = ResortPeople_GetByIndex(people, i);
        if (person != NULL) {
            FieldActor *actor = ResortPerson_GetActor(person);
            if (FldAct_GetAcmd(actor) == 0xbe) {
                FldAct_FreeAcmd(actor);
            }
        }
    }
    ResortPeople_FreeAll(people);
    ResortBinary_Free(people->table);
    GFL_HeapFree(people);
}

void ResortPeople_Update(ResortPeople *people, FieldPlayer *player) {
}

u32 ResortPeople_GetCount(ResortPeople *people) {
    return people->setup.count;
}

void ResortPeople_CreateFromSlots(ResortPeople *people) {
    u32 row = people->setup.zone;
    int pos;

    for (pos = 0; pos < 64; pos++) {
        if (!ResortSlots_IsEmpty(people->setup.slots, row, pos)) {
            ResortPersonSource source;
            MI_CpuFill8(&source, 0, sizeof(ResortPersonSource));
            source.data = ResortSlots_GetData(people->setup.slots, row, pos);
            source.zone = people->setup.zone;
            ResortPeople_AddPerson(people, &source, TRUE, pos);
        }
    }
    ResortPeople_DeleteOrphanActors(people);
}

void ResortPeople_CreateFromDatas(ResortPeople *people) {
    u32 row = people->setup.zone;
    ResortDataIter iter = ResortDatas_StartIter(people->setup.datas, sKinds, 2);
    ResortPersonSource source;
    ResortPersonData *data;

    while (TRUE) {
        BOOL create;
        u32 pos;
        u16 unk;

        data = ResortDatas_NextIter(people->setup.datas, &iter, sKinds, 2);
        if (data == NULL) {
            break;
        }
        if (ResortData_IsEmpty(data)) {
            continue;
        }
        create = FALSE;
        pos = ResortData_GetParam(data, JOIN_AVE_PARAM_SLOT, NULL);
        if (!ResortSlots_IsEmpty(people->setup.slots, row, pos)) {
            create = TRUE;
        }
        unk = ResortData_GetParam(data, JOIN_AVE_PARAM_SHOP_KIND, NULL);
        if (ResortData_GetKind(data) == 4) {
            if (unk == 2 || unk == 3) {
                create |= TRUE;
            } else {
                create = FALSE;
            }
        }
        if (create) {
            MI_CpuFill8(&source, 0, sizeof(ResortPersonSource));
            source.data = data;
            source.zone = people->setup.zone;
            ResortPeople_AddPerson(people, &source, FALSE, pos);
        }
    }
    ResortPeople_DeleteOrphanActors(people);
}

static inline ResortPerson *ResortPeople_GetPerson(ResortPeople *people, int index) {
    return people->persons[index];
}

static ResortPerson *ResortPeople_AddPerson(ResortPeople *people, const ResortPersonSource *source, BOOL unk20,
                                            u16 index) {
    if (ResortPeople_GetPerson(people, index) == NULL) {
        ResortPersonSetup setup;
        ResortPeople_MakeSetup(people, source, index, &setup);
        setup.unk20 = unk20;
        people->persons[index] = ResortPerson_Create(people->heapId, &setup);
        ResortSlots_Fill(people->setup.slots);
        return people->persons[index];
    }
    return NULL;
}

ResortPerson *ResortPeople_AddAtEmptySlot(ResortPeople *people, const ResortPersonSource *source) {
    u32 pos = ResortSlots_FindEmpty(people->setup.slots, source->zone);
    if (pos != 0xffff) {
        return ResortPeople_AddPerson(people, source, FALSE, pos);
    }
    return NULL;
}

static void ResortPeople_RemovePerson(ResortPeople *people, ResortPerson *person) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        if (person == people->persons[i]) {
            ResortPerson_Free(person);
            people->persons[i] = NULL;
            return;
        }
    }
}

static void ResortPeople_FreeAll(ResortPeople *people) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        if (people->persons[i] != NULL) {
            ResortPerson_Free(people->persons[i]);
            people->persons[i] = NULL;
        }
    }
}

void ResortPeople_ApplyVisibility(ResortPeople *people) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        if (people->persons[i] != NULL) {
            ResortPerson_ApplyVisibility(people->persons[i], 2);
        }
    }
}

void ResortPeople_Recreate(ResortPeople *people, ResortPerson *person) {
    u32 pos = ResortPerson_GetParam(person, JOIN_AVE_PARAM_SLOT, NULL);
    ResortPersonSource source;
    ResortPersonSetup setup;

    MI_CpuFill8(&source, 0, sizeof(ResortPersonSource));
    source.data = ResortPerson_GetData(person);
    source.zone = ResortPerson_GetParam(person, JOIN_AVE_PARAM_ROW, NULL);
    ResortPeople_MakeSetup(people, &source, pos, &setup);
    setup.unk20 = ResortPerson_GetAttr(person, 1);
    ResortPerson_Recreate(person, &setup);
}

void ResortPeople_Delete(ResortPeople *people, ResortPerson *person) {
    DeleteActor(ResortPerson_GetActor(person));
    ResortPeople_RemovePerson(people, person);
    ResortSlots_Fill(people->setup.slots);
}

ResortPerson *ResortPeople_FindByActor(ResortPeople *people, FieldActor *actor) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        ResortPerson *person = people->persons[i];
        if (person != NULL && ResortPerson_HasActor(person, actor)) {
            return person;
        }
    }
    return NULL;
}

ResortPerson *ResortPeople_FindByJoinAvenuePerson(ResortPeople *people, JoinAvenuePerson *joinPerson) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        ResortPerson *person = people->persons[i];
        if (person != NULL && ResortPerson_IsJoinAvenuePerson(person, joinPerson)) {
            return person;
        }
    }
    return NULL;
}

ResortPerson *ResortPeople_GetByIndex(ResortPeople *people, u32 index) {
    return people->persons[index];
}

ResortPerson *ResortPeople_FindByData(ResortPeople *people, ResortPersonData *data) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        ResortPerson *person = people->persons[i];
        if (person != NULL && ResortPerson_HasData(person, data)) {
            return person;
        }
    }
    return NULL;
}

// Deletes the actors of the zone's people that no person has
static void ResortPeople_DeleteOrphanActors(ResortPeople *people) {
    FieldActor *actor;
    u32 index = 0;

    while (NextActor(people->setup.mmSys, &actor, &index) == TRUE) {
        if (ResortPeople_IsPersonUID(GetActorUID(actor)) &&
            GetActorZoneID(actor) == ResortLayout_GetZoneId(people->setup.zone)) {
            if (ResortPeople_FindByActor(people, actor) == NULL) {
                DeleteActor(actor);
            }
        }
    }
}

static void ResortPeople_MakeSetup(ResortPeople *people, const ResortPersonSource *source, u32 index,
                                   ResortPersonSetup *setup) {
    MI_CpuFill8(setup, 0, sizeof(ResortPersonSetup));
    setup->data = source->data;
    setup->index = index;
    setup->zone = source->zone;
    setup->table = people->table;
    setup->mmSys = people->setup.mmSys;
    setup->shops = people->setup.shops;
    setup->unk18 = people->setup.unk18;
}

static BOOL ResortPeople_IsPersonUID(int uid) {
    return uid >= RESORT_PERSON_UID_FIRST && uid < RESORT_PERSON_UID_END;
}
