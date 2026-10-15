#ifndef POKEBW2_FIELD_RESORT_H
#define POKEBW2_FIELD_RESORT_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "save/join_avenue.h"
#include "struct_decls.h"

// The Join Avenue's gimmick, overlay 137, whose files are resort_field.c, resort_people.c, resort_data_manager.c,
// resort_npc.c and resort_sys.c. Script plugin 8 (overlays 58 to 60) drives it. The objects are named after the file
// they come from: the system of resort_sys.c, and the people of resort_people.c, each with its data, which reads and
// writes its fields through a table of functions

typedef struct ResortSys ResortSys;
typedef struct ResortPeople ResortPeople;
typedef struct ResortPerson ResortPerson;
typedef struct ResortPersonData ResortPersonData;
typedef struct ResortNPC ResortNPC;
typedef struct ResortSlots ResortSlots;

// What the gimmick's system is created with
typedef struct {
    JoinAvenueOccupants *occupants;
    JoinAvenuePersonList *list;
    // A second list, or NULL
    JoinAvenuePersonList *list2;
    // The twelve entries of func_02037f04
    void *entries;
    JoinAvenueInfo *info;
} ResortSysSetup;

// What the people are created with
typedef struct {
    MMSys *mmSys;
    ResortSlots *slots;
    ResortPersonData **datas;
    // The number of people
    u32 count;
    // The index of the avenue's zone, which is also the row of the slots
    u32 zone;
    void *shops;
    // A table whose columns 1 and 2 are added to a person's x and z, by the person's index
    void *unk18;
} ResortPeopleSetup;

// A data and the zone of the person it makes
typedef struct {
    ResortPersonData *data;
    u32 zone;
} ResortPersonSource;

// A walk through the data of a list of kinds, for func_ov137_021f1ae0 and func_ov137_021f1b0c
typedef struct {
    u16 kindIndex;
    u16 pos;
} ResortDataIter;

// What the avenue's NPCs are created with
typedef struct {
    MMSys *mmSys;
    u32 unk4;
    // The index of the avenue's zone, for func_0203950c
    u32 zone;
    JoinAvenueInfo *info;
    // The NPCs' table, of ResortBinary_Load
    void *table;
} ResortNPCSetup;

// The gimmick of the avenue's main zone, which the field sets up, ends and updates, and its parts
void func_ov137_021eec80(Field *field);
void func_ov137_021eee4c(Field *field);
void func_ov137_021eee80(Field *field);
ResortPeople *func_ov137_021eeeac(Field *field);
ResortNPC *func_ov137_021eeebc(Field *field);
ResortSys *func_ov137_021eeec8(Field *field);
void func_ov137_021eeed4(Field *field, ResortPerson *person);
// Starts, checks the end of and ends an animation of the scene, 0 or 1
void func_ov137_021eeee8(Field *field, u32 index);
BOOL func_ov137_021eeefc(Field *field, u32 index);
void func_ov137_021eef10(Field *field, u32 index);
void func_ov137_021eef24(Field *field);
void func_ov137_021eef3c(Field *field);
void func_ov137_021eef4c(Field *field);
// Starts or stops the people's balloons
void func_ov137_021eef68(Field *field, BOOL start);
// The gimmick of the avenue's other zone
void func_ov137_021f0d10(Field *field);
void func_ov137_021f0e28(Field *field);
void func_ov137_021f0e50(Field *field);
ResortPeople *func_ov137_021f0e74(Field *field);
ResortSys *func_ov137_021f0e80(Field *field);
ResortNPC *func_ov137_021f0e8c(Field *field);

// resort_people.c
JoinAvenuePerson *func_ov137_021f0f58(ResortPerson *person);
void func_ov137_021f0f64(ResortPerson *person);
void func_ov137_021f0f84(ResortPerson *person, u32 value);
void *func_ov137_021f0fa8(ResortPerson *person);
// Moves the person's actor to where its type puts it
void func_ov137_021f0fb8(ResortPerson *person);
void func_ov137_021f1020(ResortPerson *person, u16 *x, u16 *z, u16 *dir);
void func_ov137_021f1070(ResortPerson *person, BOOL visible);
// 0 shows the person's actor, 1 hides it, and 2 shows it if the person is visible
void func_ov137_021f1098(ResortPerson *person, u32 mode);
void func_ov137_021f10dc(ResortPerson *person, JoinAvenuePersonParam param, u32 value);
u32 func_ov137_021f10e8(ResortPerson *person, JoinAvenuePersonParam param, void *buffer);
u32 func_ov137_021f10f4(ResortPerson *person, u32 which);
FieldActor *func_ov137_021f110c(ResortPerson *person);
ResortPersonData *func_ov137_021f1110(ResortPerson *person);
ResortPeople *func_ov137_021f12b4(const ResortPeopleSetup *setup, HeapID heapId);
void func_ov137_021f1300(ResortPeople *people);
void func_ov137_021f1348(ResortPeople *people, FieldPlayer *player);
u32 func_ov137_021f134c(ResortPeople *people);
// Creates the people of the slots of the zone's row, and of the occupants and entries that have slots there
void func_ov137_021f1350(ResortPeople *people);
void func_ov137_021f13a4(ResortPeople *people);
// Creates a person at the first empty slot of the source's zone, or returns NULL
ResortPerson *func_ov137_021f14a8(ResortPeople *people, const ResortPersonSource *source);
void func_ov137_021f152c(ResortPeople *people);
void func_ov137_021f1554(ResortPeople *people, ResortPerson *person);
void func_ov137_021f15ac(ResortPeople *people, ResortPerson *person);
ResortPerson *func_ov137_021f15cc(ResortPeople *people, FieldActor *actor);
ResortPerson *func_ov137_021f1600(ResortPeople *people, JoinAvenuePerson *person);
ResortPerson *func_ov137_021f1634(ResortPeople *people, u32 index);
ResortPerson *func_ov137_021f163c(ResortPeople *people, ResortPersonData *data);

// resort_data_manager.c. The data of the avenue's people and records, 40 of them, for kinds 0 to 4: the eight
// occupants, the four records, the eight of each list and the twelve entries. Each calls the functions of its type
ResortSlots *func_ov137_021f1710(ResortPersonData **datas, HeapID heapId);
void func_ov137_021f1740(ResortSlots *slots);
// Places each data that is not empty in the slot of its params 0 and 1, the slot and the row (1 or 2)
void func_ov137_021f1748(ResortSlots *slots);
u16 func_ov137_021f17ec(u32 kind);
u32 func_ov137_021f17fc(u32 kind);
// Whether a slot of a row is empty, the data in it, and the first empty one (0xffff if none)
BOOL func_ov137_021f1808(ResortSlots *slots, u32 row, u32 pos);
ResortPersonData *func_ov137_021f1824(ResortSlots *slots, u32 row, u32 pos);
u32 func_ov137_021f184c(ResortSlots *slots, u32 row);
u16 func_ov137_021f1878(ResortPersonData *data, void *a1, void *a2, JoinAvenueInfo *info);
ResortPersonData *func_ov137_021f18f0(HeapID heapId, void *person, u32 kind, u16 index);
void func_ov137_021f1920(ResortPersonData *data);
void func_ov137_021f1950(ResortPersonData *data);
BOOL func_ov137_021f195c(ResortPersonData *data);
u32 func_ov137_021f1968(ResortPersonData *data, JoinAvenuePersonParam param, void *buffer);
void func_ov137_021f1974(ResortPersonData *data, JoinAvenuePersonParam param, u32 value);
// The person, entry or record of the data, by its kind
void *func_ov137_021f1980(ResortPersonData *data);
u32 func_ov137_021f1984(ResortPersonData *data);
u32 func_ov137_021f1988(ResortPersonData *data);
u32 func_ov137_021f198c(ResortPersonData *data);
u32 func_ov137_021f1990(ResortPersonData *data, u32 a1, u32 a2);
u32 func_ov137_021f199c(ResortPersonData *data, ResortPersonData *other, u32 a2, u32 a3);
ResortPersonData **func_ov137_021f19c4(const ResortSysSetup *setup, HeapID heapId);
void func_ov137_021f1ac0(ResortPersonData **datas);
ResortDataIter func_ov137_021f1ae0(ResortPersonData **datas, const u32 *kinds, u32 count);
// The next data of the kinds, or NULL
ResortPersonData *func_ov137_021f1b0c(ResortPersonData **datas, ResortDataIter *iter, const u32 *kinds, u32 count);
// The data of a person, among what ResortSys_GetDatas returns
ResortPersonData *func_ov137_021f1b6c(ResortPersonData **datas, JoinAvenuePerson *person);
ResortPersonData *func_ov137_021f1b94(ResortPersonData **datas, u32 kind, u16 index);
// The same for the scripts' numbering of the kinds: 0 the occupants, 1 the entries, 2 the people of both lists and 3
// the records
ResortPersonData *func_ov137_021f1ba8(ResortPersonData **datas, u32 which, u16 index);
// The first empty data of a kind, or NULL
ResortPersonData *func_ov137_021f1bd0(ResortPersonData **datas, u32 kind);

// resort_npc.c
ResortNPC *func_ov137_021f1c24(const ResortNPCSetup *setup, HeapID heapId);
void func_ov137_021f1c74(ResortNPC *npc);
// Stops the NPCs' movement while an event is running
void func_ov137_021f1c88(ResortNPC *npc, Field *field);
// The actor of an NPC, or NULL
FieldActor *func_ov137_021f1cc8(ResortNPC *npc, u32 index);
// The number of NPCs created
u32 func_ov137_021f1d00(ResortNPC *npc);
void func_ov137_021f1d04(ResortNPC *npc, BOOL visible);
// A column of an NPC's row of texts
u16 func_ov137_021f1d60(ResortNPC *npc, u32 row, u32 column);
u16 func_ov137_021f1f00(ResortNPC *npc, u32 row);

// resort_sys.c
ResortSys *ResortSys_Create(const ResortSysSetup *setup, ResortWork *resortWork, HeapID heapId);
void ResortSys_Free(ResortSys *sys);
// The Join Avenue's tables of the system: the shops, and the binaries of its ranks, texts and the like
ResortShopData *ResortSys_GetShopData(ResortSys *sys);
ResortBinary *ResortSys_GetRanks(ResortSys *sys);
ResortBinary *ResortSys_GetUnk20Table(ResortSys *sys);
ResortBinary *ResortSys_GetUnk24Table(ResortSys *sys);
ResortBinary *ResortSys_GetTexts(ResortSys *sys);
ResortBinary *ResortSys_GetShopOpenings(ResortSys *sys);
ResortPersonData **ResortSys_GetDatas(ResortSys *sys);
ResortSlots *ResortSys_GetSlots(ResortSys *sys);
JoinAvenueOccupants *ResortSys_GetOccupants(ResortSys *sys);
JoinAvenuePersonList *ResortSys_GetList(ResortSys *sys);
JoinAvenuePersonList *ResortSys_GetList2(ResortSys *sys);
void *ResortSys_GetEntries(ResortSys *sys);
JoinAvenueInfo *ResortSys_GetInfo(ResortSys *sys);
ResortWork *ResortSys_GetWork(ResortSys *sys);
// The answer to a question of the scripts: a message of SCRIPT_TEXT_GLOBAL_10685 for the person, or a number
u16 ResortSys_GetScriptValue(u32 query, ResortSys *sys, ResortPersonData *data, GameData *gameData, WordSet *wordSet,
                        HeapID heapId);
BOOL ResortSys_GetPersonVisit(ResortPersonData *data, JoinAvenuePerson *person, ResortSys *sys, u16 *a3, u16 *a4,
                         u16 *a5);
// Adds popularity points to the avenue: 0 nothing came of it, 1 a new rank, 2 no more ranks, 3 a new shop, 4 a new
// rank with new texts, 5 both
u32 ResortSys_AddPopularity(ResortSys *sys, GameData *gameData, u32 points, u16 *rank);
// Adds experience to a person's shop: 0 nothing, 1 a new level, 2 a new level of another kind of shop, 3 the highest
u32 ResortSys_AddShopExperience(JoinAvenuePerson *person, ResortSys *sys, u32 points, u16 *level);
u32 ResortSys_GetInfoFlag1b(ResortSys *sys);
// Whether the occupants' shops are all at the highest level
BOOL ResortSys_AreAllShopsMaxLevel(ResortSys *sys);
void ResortSys_SetScriptWord(u32 type, u32 who, u16 index, u32 wordIndex, ResortSys *sys, ResortPerson *person,
                         WordSet *wordSet, GameData *gameData, HeapID heapId);
void *ResortRecords_CreateWindow(void *msgBGSys, HeapID heapId);
void ResortRecords_PrintPage(void *window, JoinAvenueOccupants *occupants, void *msgBGSys, WordSet *wordSet,
                         GameData *gameData, u32 page, HeapID heapId);
u32 ResortSys_GetUnlock(ResortSys *sys, GameData *gameData);
u32 ResortSys_GetUnlockForRank(ResortSys *sys, GameData *gameData, u32 rank);
u32 ResortSys_GetTrainerTypeText(ResortBinary *texts, ResortPersonData *data, u32 column);
void ResortSys_HideOverlappingActors(ResortPeople *people, ResortNPC *npc, Field *field, ResortPerson *person, u32 mode);
ResortSys *ResortSys_Get(Field *field);
// The person of the script's actor
ResortPerson *ResortSys_GetScriptPerson(FieldScriptEnv *env, ResortPeople *people);
u32 ResortSys_MakeData(ResortSys *sys, ResortPersonData *data, u32 kind);
StrBuf *ResortSys_FormatShopText(ResortShopData *shops, ResortPersonData *data, u16 shopId, HeapID heapId);
void ResortSys_MakeShopEntry(ResortSys *sys, GameData *gameData, void *entry, u32 shopKind, u32 salt, HeapID heapId);
// Adds an entry (mode 1) or a record (mode 3) of a kind of shop
void ResortSys_AddEntryOrRecord(ResortSys *sys, PlayerInfo *playerInfo, u32 mode, u32 shopKind, HeapID heapId);
void ResortSys_ChooseVisitGroup(ResortSys *sys, ResortPersonData *a1, ResortPersonData *data);
BOOL ResortSys_PickPersonToMeet(ResortSys *sys, ResortPersonData *a1, ResortPersonData *data, u32 *out);
u32 ResortSys_GetVisitPoints(ResortSys *sys, ResortPersonData *data);
u32 ResortSys_GetVisitScore(ResortSys *sys, ResortPersonData *data);
void ResortSys_MarkMet(ResortSys *sys, ResortPersonData *a1, u32 a2);
BOOL ResortSys_HasShopKind(ResortSys *sys, GameData *gameData, u32 a2);
// Whether all eight of the avenue's people are there
BOOL ResortSys_IsFull(ResortSys *sys);

#endif // POKEBW2_FIELD_RESORT_H
