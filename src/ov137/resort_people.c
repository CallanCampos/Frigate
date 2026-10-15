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
    void *table;
    void *shops;
    void *unk18;
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
    void *shops;
    void *unk2c;
};

struct ResortPeople {
    ResortPeopleSetup setup;
    HeapID heapId;
    void *table;
    ResortPerson *persons[];
};

static void func_ov137_021f0f3c(ResortPerson *person);
static BOOL func_ov137_021f0f44(ResortPerson *person, FieldActor *actor);
static void func_ov137_021f0f88(ResortPerson *person, const ResortPersonSetup *setup);
static BOOL func_ov137_021f1048(ResortPerson *person, JoinAvenuePerson *joinPerson);
static BOOL func_ov137_021f1060(ResortPerson *person, ResortPersonData *data);
static void func_ov137_021f1114(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc);
static void func_ov137_021f113c(ResortPerson *person, u16 *x, u16 *z, u16 *dir);
static void func_ov137_021f11b8(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc);
static void func_ov137_021f11cc(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc);
static void func_ov137_021f11e0(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc);
static void func_ov137_021f1208(ResortPerson *person, u16 *x, u16 *z, u16 *dir);
static void func_ov137_021f122c(ResortPerson *person, const ResortPersonSetup *setup);
static ResortPerson *func_ov137_021f1464(ResortPeople *people, const ResortPersonSource *source, BOOL unk20, u16 index);
static void func_ov137_021f14d8(ResortPeople *people, ResortPerson *person);
static void func_ov137_021f1504(ResortPeople *people);
static void func_ov137_021f1670(ResortPeople *people);
static void func_ov137_021f16d0(ResortPeople *people, const ResortPersonSource *source, u32 index,
                                ResortPersonSetup *setup);
static BOOL func_ov137_021f1700(int uid);

static const u32 sKinds[] = { 0, 4 };
// clang-format off
static const ResortPersonType sTypes[] = {
    {func_ov137_021f1114, func_ov137_021f113c},
    {func_ov137_021f11b8, NULL},
    {func_ov137_021f11cc, NULL},
    {func_ov137_021f11e0, func_ov137_021f1208},
};

// clang-format on
static const ZoneNPC sNpcTemplate = { 0 };

static ResortPerson *func_ov137_021f0e98(HeapID heapId, const ResortPersonSetup *setup) {
    u32 a;
    u32 b;
    ResortPerson *person = GFL_HeapAllocate(heapId, sizeof(ResortPerson), TRUE, "resort_people.c", 166);

    person->index = setup->index;
    person->mmSys = setup->mmSys;
    person->data = setup->data;
    person->unk10 = setup->unk20;
    person->shops = setup->shops;
    person->unk2c = setup->unk18;
    person->type = func_ov137_021f1984(person->data);
    a = func_ov137_021f1968(person->data, 31, NULL);
    b = func_ov137_021f1968(person->data, 2, NULL);
    person->row = ResortBinary_FindRow(setup->table, person->type, a, b);
    person->funcs = &sTypes[person->type];
    person->actor = FindFieldActor(person->mmSys, person->index + RESORT_PERSON_UID_FIRST);
    if (person->actor == NULL) {
        func_ov137_021f122c(person, setup);
    }
    person->visible = !func_ov012_02167520(person->actor);
    return person;
}

static void func_ov137_021f0f3c(ResortPerson *person) {
    GFL_HeapFree(person);
}

static BOOL func_ov137_021f0f44(ResortPerson *person, FieldActor *actor) {
    if (actor == func_ov137_021f110c(person)) {
        return TRUE;
    }
    return FALSE;
}

JoinAvenuePerson *func_ov137_021f0f58(ResortPerson *person) {
    return func_ov137_021f1980(person->data);
}

void func_ov137_021f0f64(ResortPerson *person) {
    FieldActor *actor = func_ov137_021f110c(person);
    u16 objCode = func_ov137_021f1968(person->data, 3, NULL);

    FldAct_UpdateBlInfoForNewObjCode(actor, objCode);
}

void func_ov137_021f0f84(ResortPerson *person, u32 value) {
    person->unk8 = value;
}

static void func_ov137_021f0f88(ResortPerson *person, const ResortPersonSetup *setup) {
    if (person->actor != NULL) {
        DeleteActor(person->actor);
        person->actor = NULL;
    }
    func_ov137_021f122c(person, setup);
}

void *func_ov137_021f0fa8(ResortPerson *person) {
    return func_02038470(func_ov137_021f1980(person->data));
}

void func_ov137_021f0fb8(ResortPerson *person) {
    if (person->funcs->getPos != NULL) {
        u16 x;
        u16 z;
        u16 dir;
        VecFx32 pos;
        FieldActor *actor = func_ov137_021f110c(person);

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

void func_ov137_021f1020(ResortPerson *person, u16 *x, u16 *z, u16 *dir) {
    person->funcs->getPos(person, x, z, dir);
    if (*dir == DIR_LEFT) {
        *x -= 1;
        *dir = DIR_RIGHT;
    } else {
        *x += 1;
        *dir = DIR_LEFT;
    }
}

static BOOL func_ov137_021f1048(ResortPerson *person, JoinAvenuePerson *joinPerson) {
    if (func_ov137_021f1980(person->data) == joinPerson) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov137_021f1060(ResortPerson *person, ResortPersonData *data) {
    if (person->data == data) {
        return TRUE;
    }
    return FALSE;
}

void func_ov137_021f1070(ResortPerson *person, BOOL visible) {
    FieldActor *actor = func_ov137_021f110c(person);
    SetActorHidden(actor, visible == FALSE);
    func_ov012_02167580(actor, visible);
    person->visible = visible;
}

void func_ov137_021f1098(ResortPerson *person, u32 mode) {
    FieldActor *actor = func_ov137_021f110c(person);
    func_ov012_02167520(actor);
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

void func_ov137_021f10dc(ResortPerson *person, JoinAvenuePersonParam param, u32 value) {
    func_ov137_021f1974(person->data, param, value);
}

u32 func_ov137_021f10e8(ResortPerson *person, JoinAvenuePersonParam param, void *buffer) {
    return func_ov137_021f1968(person->data, param, buffer);
}

u32 func_ov137_021f10f4(ResortPerson *person, u32 which) {
    switch (which) {
    case 0:
        return person->type;
    case 1:
        return person->unk10;
    }
    return 0;
}

FieldActor *func_ov137_021f110c(ResortPerson *person) {
    return person->actor;
}

ResortPersonData *func_ov137_021f1110(ResortPerson *person) {
    return person->data;
}

static void func_ov137_021f1114(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc) {
    u16 x;
    u16 z;
    u16 dir;

    func_ov137_021f113c(person, &x, &z, &dir);
    npc->direction = dir;
    func_ov012_021682c0(npc, x, z, 0);
}

static void func_ov137_021f113c(ResortPerson *person, u16 *x, u16 *z, u16 *dir) {
    u16 index = func_ov137_021f198c(person->data);
    u16 id = func_020363e0(func_ov137_021f0fa8(person), 0);
    u32 column = 0;
    const u16 *row = ResortShopData_GetShop(person->shops, id);

    if (index % 2 == 0) {
        column = 1;
    }
    func_02039538(ResortShopData_GetShopParam(row, column + 6), x, z, dir);
    *x += ResortBinary_Get(person->unk2c, index, 1);
    *z += ResortBinary_Get(person->unk2c, index, 2);
}

static void func_ov137_021f11b8(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc) {
    npc->direction = 0;
    func_ov012_021682c0(npc, 0, 0, 0);
}

static void func_ov137_021f11cc(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc) {
    npc->direction = 0;
    func_ov012_021682c0(npc, 0, 0, 0);
}

static void func_ov137_021f11e0(ResortPerson *person, const ResortPersonSetup *setup, ZoneNPC *npc) {
    u16 x;
    u16 z;
    u16 dir;

    func_ov137_021f1208(person, &x, &z, &dir);
    npc->direction = dir;
    func_ov012_021682c0(npc, x, z, 0);
}

static void func_ov137_021f1208(ResortPerson *person, u16 *x, u16 *z, u16 *dir) {
    func_02039578(func_ov137_021f1968(person->data, 31, NULL), x, z, dir);
}

static void func_ov137_021f122c(ResortPerson *person, const ResortPersonSetup *setup) {
    ZoneNPC npc = sNpcTemplate;

    npc.modelId = (u8)func_ov137_021f1968(person->data, 3, NULL);
    npc.scrId = ResortBinary_GetFoundValue(person->row);
    npc.uid = setup->index + RESORT_PERSON_UID_FIRST;
    person->funcs->setupNpc(person, setup, &npc);
    person->actor = CreateNewActorByEntityNoWKOBJCODE(setup->mmSys, &npc, func_0203950c(setup->zone));
    func_ov137_021f10dc(person, 0, setup->index);
    func_ov137_021f10dc(person, 1, setup->zone);
    person->visible = !func_ov012_02167520(person->actor);
}

ResortPeople *func_ov137_021f12b4(const ResortPeopleSetup *setup, HeapID heapId) {
    ResortPeople *people = GFL_HeapAllocate(heapId, sizeof(ResortPeople) + sizeof(ResortPerson *) * setup->count, TRUE,
                                            "resort_people.c", 712);
    people->setup = *setup;
    people->heapId = heapId;
    people->table = ResortBinary_Load(0, 4, heapId);
    return people;
}

void func_ov137_021f1300(ResortPeople *people) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        ResortPerson *person = func_ov137_021f1634(people, i);
        if (person != NULL) {
            FieldActor *actor = func_ov137_021f110c(person);
            if (FldAct_GetAcmd(actor) == 0xbe) {
                func_ov012_02166f2c(actor);
            }
        }
    }
    func_ov137_021f1504(people);
    ResortBinary_Free(people->table);
    GFL_HeapFree(people);
}

void func_ov137_021f1348(ResortPeople *people, FieldPlayer *player) {
}

u32 func_ov137_021f134c(ResortPeople *people) {
    return people->setup.count;
}

void func_ov137_021f1350(ResortPeople *people) {
    u32 row = people->setup.zone;
    int pos;

    for (pos = 0; pos < 64; pos++) {
        if (!func_ov137_021f1808(people->setup.slots, row, pos)) {
            ResortPersonSource source;
            sys_memset(&source, 0, sizeof(ResortPersonSource));
            source.data = func_ov137_021f1824(people->setup.slots, row, pos);
            source.zone = people->setup.zone;
            func_ov137_021f1464(people, &source, TRUE, pos);
        }
    }
    func_ov137_021f1670(people);
}

void func_ov137_021f13a4(ResortPeople *people) {
    u32 row = people->setup.zone;
    ResortDataIter iter = func_ov137_021f1ae0(people->setup.datas, sKinds, 2);
    ResortPersonSource source;
    ResortPersonData *data;

    while (TRUE) {
        BOOL create;
        u32 pos;
        u16 unk;

        data = func_ov137_021f1b0c(people->setup.datas, &iter, sKinds, 2);
        if (data == NULL) {
            break;
        }
        if (func_ov137_021f195c(data)) {
            continue;
        }
        create = FALSE;
        pos = func_ov137_021f1968(data, 0, NULL);
        if (!func_ov137_021f1808(people->setup.slots, row, pos)) {
            create = TRUE;
        }
        unk = func_ov137_021f1968(data, 31, NULL);
        if (func_ov137_021f1988(data) == 4) {
            if (unk == 2 || unk == 3) {
                create |= TRUE;
            } else {
                create = FALSE;
            }
        }
        if (create) {
            sys_memset(&source, 0, sizeof(ResortPersonSource));
            source.data = data;
            source.zone = people->setup.zone;
            func_ov137_021f1464(people, &source, FALSE, pos);
        }
    }
    func_ov137_021f1670(people);
}

static inline ResortPerson *ResortPeople_GetPerson(ResortPeople *people, int index) {
    return people->persons[index];
}

static ResortPerson *func_ov137_021f1464(ResortPeople *people, const ResortPersonSource *source, BOOL unk20,
                                         u16 index) {
    if (ResortPeople_GetPerson(people, index) == NULL) {
        ResortPersonSetup setup;
        func_ov137_021f16d0(people, source, index, &setup);
        setup.unk20 = unk20;
        people->persons[index] = func_ov137_021f0e98(people->heapId, &setup);
        func_ov137_021f1748(people->setup.slots);
        return people->persons[index];
    }
    return NULL;
}

ResortPerson *func_ov137_021f14a8(ResortPeople *people, const ResortPersonSource *source) {
    u32 pos = func_ov137_021f184c(people->setup.slots, source->zone);
    if (pos != 0xffff) {
        return func_ov137_021f1464(people, source, FALSE, pos);
    }
    return NULL;
}

static void func_ov137_021f14d8(ResortPeople *people, ResortPerson *person) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        if (person == people->persons[i]) {
            func_ov137_021f0f3c(person);
            people->persons[i] = NULL;
            return;
        }
    }
}

static void func_ov137_021f1504(ResortPeople *people) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        if (people->persons[i] != NULL) {
            func_ov137_021f0f3c(people->persons[i]);
            people->persons[i] = NULL;
        }
    }
}

void func_ov137_021f152c(ResortPeople *people) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        if (people->persons[i] != NULL) {
            func_ov137_021f1098(people->persons[i], 2);
        }
    }
}

void func_ov137_021f1554(ResortPeople *people, ResortPerson *person) {
    u32 pos = func_ov137_021f10e8(person, 0, NULL);
    ResortPersonSource source;
    ResortPersonSetup setup;

    sys_memset(&source, 0, sizeof(ResortPersonSource));
    source.data = func_ov137_021f1110(person);
    source.zone = func_ov137_021f10e8(person, 1, NULL);
    func_ov137_021f16d0(people, &source, pos, &setup);
    setup.unk20 = func_ov137_021f10f4(person, 1);
    func_ov137_021f0f88(person, &setup);
}

void func_ov137_021f15ac(ResortPeople *people, ResortPerson *person) {
    DeleteActor(func_ov137_021f110c(person));
    func_ov137_021f14d8(people, person);
    func_ov137_021f1748(people->setup.slots);
}

ResortPerson *func_ov137_021f15cc(ResortPeople *people, FieldActor *actor) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        ResortPerson *person = people->persons[i];
        if (person != NULL && func_ov137_021f0f44(person, actor)) {
            return person;
        }
    }
    return NULL;
}

ResortPerson *func_ov137_021f1600(ResortPeople *people, JoinAvenuePerson *joinPerson) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        ResortPerson *person = people->persons[i];
        if (person != NULL && func_ov137_021f1048(person, joinPerson)) {
            return person;
        }
    }
    return NULL;
}

ResortPerson *func_ov137_021f1634(ResortPeople *people, u32 index) {
    return people->persons[index];
}

ResortPerson *func_ov137_021f163c(ResortPeople *people, ResortPersonData *data) {
    u32 i;
    for (i = 0; i < people->setup.count; i++) {
        ResortPerson *person = people->persons[i];
        if (person != NULL && func_ov137_021f1060(person, data)) {
            return person;
        }
    }
    return NULL;
}

// Deletes the actors of the zone's people that no person has
static void func_ov137_021f1670(ResortPeople *people) {
    FieldActor *actor;
    u32 index = 0;

    while (NextActor(people->setup.mmSys, &actor, &index) == TRUE) {
        if (func_ov137_021f1700(GetActorUID(actor)) && GetActorZoneID(actor) == func_0203950c(people->setup.zone)) {
            if (func_ov137_021f15cc(people, actor) == NULL) {
                DeleteActor(actor);
            }
        }
    }
}

static void func_ov137_021f16d0(ResortPeople *people, const ResortPersonSource *source, u32 index,
                                ResortPersonSetup *setup) {
    sys_memset(setup, 0, sizeof(ResortPersonSetup));
    setup->data = source->data;
    setup->index = index;
    setup->zone = source->zone;
    setup->table = people->table;
    setup->mmSys = people->setup.mmSys;
    setup->shops = people->setup.shops;
    setup->unk18 = people->setup.unk18;
}

static BOOL func_ov137_021f1700(int uid) {
    return uid >= RESORT_PERSON_UID_FIRST && uid < RESORT_PERSON_UID_END;
}
