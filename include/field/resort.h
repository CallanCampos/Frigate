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
    // The twelve entries of JoinAvenueEntryList_Get
    JoinAvenueEntryList *entries;
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
    ResortShopData *shops;
    // A table whose columns 1 and 2 are added to a person's x and z, by the person's index
    ResortBinary *unk18;
} ResortPeopleSetup;

// A data and the zone of the person it makes
typedef struct {
    ResortPersonData *data;
    u32 zone;
} ResortPersonSource;

// A walk through the data of a list of kinds, for ResortDatas_StartIter and ResortDatas_NextIter
typedef struct {
    u16 kindIndex;
    u16 pos;
} ResortDataIter;

// What the avenue's NPCs are created with
typedef struct {
    MMSys *mmSys;
    u32 unk4;
    // The index of the avenue's zone, for ResortLayout_GetZoneId
    u32 zone;
    JoinAvenueInfo *info;
    // The NPCs' table, of ResortBinary_Load
    ResortBinary *table;
} ResortNPCSetup;

// The gimmick of the avenue's main zone, which the field sets up, ends and updates, and its parts
void ResortField_Setup(Field *field);
void ResortField_Delete(Field *field);
void ResortField_Update(Field *field);
ResortPeople *ResortField_GetPeople(Field *field);
ResortNPC *ResortField_GetNPC(Field *field);
ResortSys *ResortField_GetSys(Field *field);
void ResortField_RemoveWalker(Field *field, ResortPerson *person);
// Starts, checks the end of and ends an animation of the scene, 0 or 1
void ResortField_StartSceneAnim(Field *field, u32 index);
BOOL ResortField_IsSceneAnimDone(Field *field, u32 index);
void ResortField_EndSceneAnim(Field *field, u32 index);
void ResortField_RefreshPeople(Field *field);
void ResortField_MarkPeopleChanged(Field *field);
void ResortField_EnterEventMode(Field *field);
// Starts or stops the people's balloons
void ResortField_SetBubblesActive(Field *field, BOOL start);
// The gimmick of the avenue's other zone
void ResortField0_Setup(Field *field);
void ResortField0_Delete(Field *field);
void ResortField0_Update(Field *field);
ResortPeople *ResortField0_GetPeople(Field *field);
ResortSys *ResortField0_GetSys(Field *field);
ResortNPC *ResortField0_GetNPC(Field *field);

// resort_people.c
JoinAvenuePerson *ResortPerson_GetJoinAvenuePerson(ResortPerson *person);
void ResortPerson_UpdateObjCode(ResortPerson *person);
void ResortPerson_SetTalking(ResortPerson *person, u32 value);
JoinAvenueShop *ResortPerson_GetShop(ResortPerson *person);
// Moves the person's actor to where its type puts it
void ResortPerson_MoveToHome(ResortPerson *person);
void ResortPerson_GetFacingPos(ResortPerson *person, u16 *x, u16 *z, u16 *dir);
void ResortPerson_SetVisible(ResortPerson *person, BOOL visible);
// 0 shows the person's actor, 1 hides it, and 2 shows it if the person is visible
void ResortPerson_ApplyVisibility(ResortPerson *person, u32 mode);
void ResortPerson_SetParam(ResortPerson *person, JoinAvenuePersonParam param, u32 value);
u32 ResortPerson_GetParam(ResortPerson *person, JoinAvenuePersonParam param, void *buffer);
u32 ResortPerson_GetAttr(ResortPerson *person, u32 which);
FieldActor *ResortPerson_GetActor(ResortPerson *person);
ResortPersonData *ResortPerson_GetData(ResortPerson *person);
ResortPeople *ResortPeople_Create(const ResortPeopleSetup *setup, HeapID heapId);
void ResortPeople_Free(ResortPeople *people);
void ResortPeople_Update(ResortPeople *people, FieldPlayer *player);
u32 ResortPeople_GetCount(ResortPeople *people);
// Creates the people of the slots of the zone's row, and of the occupants and entries that have slots there
void ResortPeople_CreateFromSlots(ResortPeople *people);
void ResortPeople_CreateFromDatas(ResortPeople *people);
// Creates a person at the first empty slot of the source's zone, or returns NULL
ResortPerson *ResortPeople_AddAtEmptySlot(ResortPeople *people, const ResortPersonSource *source);
void ResortPeople_ApplyVisibility(ResortPeople *people);
void ResortPeople_Recreate(ResortPeople *people, ResortPerson *person);
void ResortPeople_Delete(ResortPeople *people, ResortPerson *person);
ResortPerson *ResortPeople_FindByActor(ResortPeople *people, FieldActor *actor);
ResortPerson *ResortPeople_FindByJoinAvenuePerson(ResortPeople *people, JoinAvenuePerson *person);
ResortPerson *ResortPeople_GetByIndex(ResortPeople *people, u32 index);
ResortPerson *ResortPeople_FindByData(ResortPeople *people, ResortPersonData *data);

// resort_data_manager.c. The data of the avenue's people and records, 40 of them, for kinds 0 to 4: the eight
// occupants, the four records, the eight of each list and the twelve entries. Each calls the functions of its type
ResortSlots *ResortSlots_Create(ResortPersonData **datas, HeapID heapId);
void ResortSlots_Free(ResortSlots *slots);
// Places each data that is not empty in the slot of its params 0 and 1, the slot and the row (1 or 2)
void ResortSlots_Fill(ResortSlots *slots);
u16 ResortData_GetTypeOfKind(u32 kind);
u32 ResortData_GetMakeKind(u32 kind);
// Whether a slot of a row is empty, the data in it, and the first empty one (0xffff if none)
BOOL ResortSlots_IsEmpty(ResortSlots *slots, u32 row, u32 pos);
ResortPersonData *ResortSlots_GetData(ResortSlots *slots, u32 row, u32 pos);
u32 ResortSlots_FindEmpty(ResortSlots *slots, u32 row);
u16 ResortData_RaffleShopItem(ResortPersonData *data, ResortBinary *a1, ResortBinary *a2, JoinAvenueInfo *info);
// person is a JoinAvenuePerson, JoinAvenueRecord or JoinAvenueEntry, by the kind
ResortPersonData *ResortData_Create(HeapID heapId, void *person, u32 kind, u16 index);
void ResortData_Free(ResortPersonData *data);
void ResortData_Clear(ResortPersonData *data);
BOOL ResortData_IsEmpty(ResortPersonData *data);
u32 ResortData_GetParam(ResortPersonData *data, JoinAvenuePersonParam param, void *buffer);
void ResortData_SetParam(ResortPersonData *data, JoinAvenuePersonParam param, u32 value);
// The person, entry or record of the data, by its kind: a JoinAvenuePerson, JoinAvenueEntry or JoinAvenueRecord
void *ResortData_GetPerson(ResortPersonData *data);
u32 ResortData_GetType(ResortPersonData *data);
u32 ResortData_GetKind(ResortPersonData *data);
u32 ResortData_GetIndex(ResortPersonData *data);
u32 ResortData_Roll(ResortPersonData *data, u32 a1, u32 a2);
u32 ResortData_RollPair(ResortPersonData *data, ResortPersonData *other, u32 a2, u32 a3);
ResortPersonData **ResortDatas_Create(const ResortSysSetup *setup, HeapID heapId);
void ResortDatas_Free(ResortPersonData **datas);
ResortDataIter ResortDatas_StartIter(ResortPersonData **datas, const u32 *kinds, u32 count);
// The next data of the kinds, or NULL
ResortPersonData *ResortDatas_NextIter(ResortPersonData **datas, ResortDataIter *iter, const u32 *kinds, u32 count);
// The data of a person, among what ResortSys_GetDatas returns
ResortPersonData *ResortDatas_FindByPerson(ResortPersonData **datas, JoinAvenuePerson *person);
ResortPersonData *ResortDatas_Get(ResortPersonData **datas, u32 kind, u16 index);
// The same for the scripts' numbering of the kinds: 0 the occupants, 1 the entries, 2 the people of both lists and 3
// the records
ResortPersonData *ResortDatas_GetByScript(ResortPersonData **datas, u32 which, u16 index);
// The first empty data of a kind, or NULL
ResortPersonData *ResortDatas_FindEmpty(ResortPersonData **datas, u32 kind);

// resort_npc.c
ResortNPC *ResortNPC_Create(const ResortNPCSetup *setup, HeapID heapId);
void ResortNPC_Free(ResortNPC *npc);
// Stops the NPCs' movement while an event is running
void ResortNPC_Update(ResortNPC *npc, Field *field);
// The actor of an NPC, or NULL
FieldActor *ResortNPC_GetActor(ResortNPC *npc, u32 index);
// The number of NPCs created
u32 ResortNPC_GetCount(ResortNPC *npc);
void ResortNPC_SetVisible(ResortNPC *npc, BOOL visible);
// A column of an NPC's row of texts
u16 ResortNPC_GetTextEntry(ResortNPC *npc, u32 row, u32 column);
u16 ResortNPC_GetTextRow(ResortNPC *npc, u32 row);

// resort_sys.c
ResortSys *ResortSys_Create(const ResortSysSetup *setup, ResortWork *resortWork, HeapID heapId);
void ResortSys_Free(ResortSys *sys);
// The Join Avenue's tables of the system: the shops, and the binaries of its ranks, texts and the like
ResortShopData *ResortSys_GetShopData(ResortSys *sys);
ResortBinary *ResortSys_GetRanks(ResortSys *sys);
ResortBinary *ResortSys_GetPosOffsets(ResortSys *sys);
ResortBinary *ResortSys_GetUnk24Table(ResortSys *sys);
ResortBinary *ResortSys_GetTexts(ResortSys *sys);
ResortBinary *ResortSys_GetShopOpenings(ResortSys *sys);
ResortPersonData **ResortSys_GetDatas(ResortSys *sys);
ResortSlots *ResortSys_GetSlots(ResortSys *sys);
JoinAvenueOccupants *ResortSys_GetOccupants(ResortSys *sys);
JoinAvenuePersonList *ResortSys_GetList(ResortSys *sys);
JoinAvenuePersonList *ResortSys_GetList2(ResortSys *sys);
JoinAvenueEntryList *ResortSys_GetEntries(ResortSys *sys);
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
MoneyWin *ResortRecords_CreateWindow(FieldMsgBG *msgBGSys, HeapID heapId);
void ResortRecords_PrintPage(MoneyWin *window, JoinAvenueOccupants *occupants, FieldMsgBG *msgBGSys, WordSet *wordSet,
                             GameData *gameData, u32 page, HeapID heapId);
u32 ResortSys_GetUnlock(ResortSys *sys, GameData *gameData);
u32 ResortSys_GetUnlockForRank(ResortSys *sys, GameData *gameData, u32 rank);
u32 ResortSys_GetTrainerTypeText(ResortBinary *texts, ResortPersonData *data, u32 column);
void ResortSys_HideOverlappingActors(ResortPeople *people, ResortNPC *npc, Field *field, ResortPerson *person,
                                     u32 mode);
ResortSys *ResortSys_Get(Field *field);
// The person of the script's actor
ResortPerson *ResortSys_GetScriptPerson(FieldScriptEnv *env, ResortPeople *people);
u32 ResortSys_MakeData(ResortSys *sys, ResortPersonData *data, u32 kind);
StrBuf *ResortSys_FormatShopText(ResortShopData *shops, ResortPersonData *data, u16 shopId, HeapID heapId);
void ResortSys_MakeShopEntry(ResortSys *sys, GameData *gameData, JoinAvenueEntry *entry, u32 shopKind, u32 salt,
                             HeapID heapId);
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
