#include "types.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/field_msgbg.h"
#include "field/resort.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "save/event_work.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/save_control.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/resort_binary.h"
#include "system/resort_layout.h"
#include "system/resort_work.h"

// The gimmick of the avenue's main zone, made of the objects below, and of its other zone, which has only its people
// and NPCs

#define RESORT_WALKER_GROUPS 6
#define RESORT_WALKER_GROUP_SIZE 5
#define RESORT_WALKERS 30
#define RESORT_WALKER_STOPS 8
#define RESORT_BUBBLES 16

typedef struct ResortWalker ResortWalker;

// Calls the callback of a watch on the people of the lists and the entries
typedef BOOL (*ResortWatchCallback)(void *context, void *person, u32 a2, u32 event);

typedef struct {
    JoinAvenuePersonList *list;
    JoinAvenuePersonList *list2;
    JoinAvenueEntryList *entries;
    void *context;
    ResortWatchCallback callback;
    u32 interval;
} ResortWatchSetup;

// Every interval frames, calls the callback with event 2 for each person of the lists whose param 28 is set, and puts
// the param back if the callback returns FALSE; when changed, calls it with event 1 for each entry
typedef struct {
    ResortWatchSetup setup;
    JoinAvenuePersonList *list;
    JoinAvenuePersonList *list2;
    JoinAvenueEntryList *entries;
    u32 timer;
    BOOL changed;
} ResortWatch;

// A person who walks up or down the avenue, stopping at the shops of its kind
struct ResortWalker {
    u32 magic;
    BOOL active;
    ResortPerson *person;
    u16 acmd;
    u16 dir;
    u16 state;
    s16 steps;
    u8 unk14;
    u8 unk15;
    u16 objCodeChanged;
    void (*start)(ResortWalker *walker);
    void (*update)(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
    s16 waitTimer;
    s16 waitTime;
    s16 unk24;
    s16 unk26;
    s16 unk28;
    u8 lookState;
    u8 lookDir;
    u16 stopZ[RESORT_WALKER_STOPS];
    u16 stopAcmd[RESORT_WALKER_STOPS];
    u16 stopState;
    u16 stopCount;
    u16 stop;
    u32 unk54;
};

// The walkers that leave from a place at x, one at a time
typedef struct {
    ResortWalker *walkers[RESORT_WALKER_GROUP_SIZE];
    u16 unk14;
    u16 next;
    s16 timer;
    u16 count;
    u16 dir;
    u16 x;
} ResortWalkerGroup;

typedef struct {
    ResortWalkerGroup groups[RESORT_WALKER_GROUPS];
    ResortWalker walkers[RESORT_WALKERS];
    FieldPlayer *player;
    MMSys *mmSys;
    ResortShopData *shops;
    ResortBinary *unkb1c;
    JoinAvenueOccupants *occupants;
} ResortWalkers;

typedef struct {
    ResortPeople *people;
    ResortWalkers *walkers;
    ResortWatch *watch;
    ResortSys *sys;
} ResortFieldPeople;

// A balloon of the field's message BG, shown for 60 frames
typedef struct {
    StrBuf *strbuf;
    u16 active;
    u16 unk6;
    u16 index;
    s16 timer;
} ResortBubble;

// Shows the people's names and greetings in balloons now and then
typedef struct {
    BOOL active;
    BOOL enabled;
    s16 timer;
    s16 interval;
    s16 delay;
    s16 delayTime;
    u32 unk10;
    u16 shown;
    u16 count;
    u16 bubbleIndexes[3];
    u16 state;
    FieldMsgBG *msgBGSys;
    ResortPersonData **datas;
    JoinAvenueInfo *info;
    JoinAvenueOccupants *occupants;
    u32 unk30;
    ResortBubble bubbles[RESORT_BUBBLES];
    BOOL mode;
    MsgData *msgData;
} ResortBubbles;

// An animation of the scene: started, updated until it returns TRUE, and ended
typedef struct ResortFieldScene ResortFieldScene;

typedef struct {
    void (*start)(ResortFieldScene *scene);
    BOOL (*update)(ResortFieldScene *scene);
    void (*end)(ResortFieldScene *scene);
} ResortSceneAnim;

struct ResortFieldScene {
    FieldExpObjSystem *expObj;
    FieldCamera *camera;
    const ResortSceneAnim *anims[2];
};

// The gimmick's work in the main zone
typedef struct {
    ResortFieldPeople *people;
    ResortNPC *npc;
    ResortSys *sys;
    ResortBubbles *bubbles;
    ResortFieldScene *scene;
} ResortFieldWork;

// And in the other zone
typedef struct {
    ResortPeople *people;
    ResortSys *sys;
    ResortNPC *npc;
} ResortFieldWork0;

// The walkers' places, by the kind and index of the data
typedef struct {
    u32 kind;
    u16 index;
    u16 group;
    u32 slot;
} ResortWalkerPlace;

static ResortWatch *ResortWatch_Create(HeapID heapId, const ResortWatchSetup *setup);
static void ResortWatch_Free(ResortWatch *watch);
static void ResortWatch_Update(ResortWatch *watch);
static void ResortWatch_SetCallback(ResortWatch *watch, ResortWatchCallback callback);
static void ResortWatch_MarkChanged(ResortWatch *watch);
static void ResortWatch_CheckLists(ResortWatch *watch);
static void ResortWatch_CheckEntries(ResortWatch *watch);
static ResortWalkers *ResortWalkers_Create(HeapID heapId, MMSys *mmSys, FieldPlayer *player, ResortShopData *shops,
                                           ResortBinary *unkb1c, JoinAvenueOccupants *occupants);
static void ResortWalkers_Free(ResortWalkers *walkers);
static void ResortWalkers_Update(ResortWalkers *walkers, Field *field);
static void ResortWalkers_Place(ResortWalkers *walkers, u32 mode);
static void ResortWalkers_Add(ResortWalkers *walkers, ResortPerson *person);
static void ResortWalkers_Remove(ResortWalkers *walkers, ResortPerson *person);
static void ResortWalkers_MarkObjCodeChanged(ResortWalkers *walkers, ResortPerson *person);
static void ResortField_SetActorPos(FieldActor *actor, u16 x, u16 z);
static ResortWalker *ResortWalkers_Find(ResortWalkers *walkers, ResortPerson *person);
static void ResortWalker_Init(ResortWalker *walker, HeapID heapId);
static void ResortWalker_Release(ResortWalker *walker);
static void ResortWalker_Update(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static void ResortWalker_Assign(ResortWalker *walker, ResortPerson *person, ResortShopData *shops, ResortBinary *unkb1c,
                                JoinAvenueOccupants *occupants);
static void ResortWalker_Clear(ResortWalker *walker);
static BOOL ResortWalker_IsPerson(const ResortWalker *walker, ResortPerson *person);
static BOOL ResortWalker_IsAssigned(const ResortWalker *walker);
static void ResortWalker_Start(ResortWalker *walker, u16 acmd, u16 dir, u16 x);
static void ResortWalker_StartAt(ResortWalker *walker, u16 acmd, u16 dir, u16 x, u16 z);
static BOOL ResortWalker_IsIdle(ResortWalker *walker);
static BOOL ResortWalker_IsAtOrigin(ResortWalker *walker);
static void ResortWalker_MarkObjCodeChanged(ResortWalker *walker);
static BOOL ResortWalker_IsFast(ResortWalker *walker);
static BOOL ResortWalker_IsBlocked(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player, u32 a3, u32 a4);
static void ResortWalker_SetupWalk(ResortWalker *walker);
static BOOL ResortWalker_StateCheckSteps(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static BOOL ResortWalker_StateStep(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static BOOL ResortWalker_StateWaitStep(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static BOOL ResortWalker_StateLookAround(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static BOOL ResortWalker_StateStopAtShop(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static void ResortWalker_RunStates(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static void ResortWalker_FindStops(ResortWalker *walker, ResortShopData *shops, ResortBinary *unkb1c,
                                   JoinAvenueOccupants *occupants);
static BOOL ResortWalker_CheckStop(ResortWalker *walker, FieldPlayer *player);
static void ResortWalkerMove_Unk4(FieldActor *actor);
static void ResortWalkerMove_Update(FieldActor *actor);
static void ResortWalkerMove_Unk0C(FieldActor *actor);
static void ResortWalkerMove_Unk10(FieldActor *actor);
static void ResortWalkerGroup_Init(ResortWalkerGroup *group, u16 x, u16 dir, HeapID heapId);
static void ResortWalkerGroup_Clear(ResortWalkerGroup *group);
static void ResortWalkerGroup_Update(ResortWalkerGroup *group, FieldPlayer *player);
static void ResortWalkerGroup_Place(ResortWalkerGroup *group, u32 mode);
static void ResortWalkerGroup_Add(ResortWalkerGroup *group, u16 slot, ResortWalker *walker);
static void ResortWalkerGroup_Remove(ResortWalkerGroup *group, u16 slot);
static BOOL ResortField_GetWalkerPlace(ResortPerson *person, u16 *group, u16 *slot);
static ResortBubbles *ResortBubbles_Create(HeapID heapId, FieldMsgBG *msgBGSys, ResortPersonData **datas,
                                           JoinAvenueInfo *info, JoinAvenueOccupants *occupants, BOOL enabled);
static void ResortBubbles_Free(ResortBubbles *bubbles);
static void ResortBubbles_Update(ResortBubbles *bubbles, Field *field);
static void ResortBubbles_HideAll(ResortBubbles *bubbles);
static void ResortBubbles_Enable(ResortBubbles *bubbles);
static void ResortBubbles_SetEventMode(ResortBubbles *bubbles);
static void ResortBubbles_SetActive(ResortBubbles *bubbles, BOOL start);
static void ResortBubbles_UpdateInterval(ResortBubbles *bubbles);
static ResortPersonData *ResortBubbles_PickData(ResortBubbles *bubbles);
static void ResortBubble_Init(ResortBubble *bubble, u16 index, HeapID heapId);
static void ResortBubble_Free(ResortBubble *bubble);
static void ResortBubble_Update(ResortBubble *bubble, FieldMsgBG *msgBGSys);
static BOOL ResortBubble_Overlaps(u16 a, u16 b, BOOL mode);
static void ResortBubble_Show(ResortBubble *bubble, FieldMsgBG *msgBGSys, ResortPersonData *data, JoinAvenueInfo *info,
                              JoinAvenueOccupants *occupants, MsgData *msgData, u32 kind, BOOL mode);
static void ResortBubble_Hide(ResortBubble *bubble, FieldMsgBG *msgBGSys);
static void ResortBubble_Reset(ResortBubble *bubble);
static BOOL ResortBubble_IsDone(ResortBubble *bubble);
static ResortFieldPeople *ResortFieldPeople_Create(HeapID heapId, ResortSys *sys, MMSys *mmSys, FieldPlayer *player,
                                                   BOOL flag, u32 a5);
static void ResortFieldPeople_CreatePeople(ResortFieldPeople *people, BOOL flag, u32 a2);
static void ResortFieldPeople_Free(ResortFieldPeople *people);
static void ResortFieldPeople_Update(ResortFieldPeople *people, Field *field);
static void ResortFieldPeople_UseNormalWatch(ResortFieldPeople *people);
static ResortPeople *ResortFieldPeople_GetPeople(ResortFieldPeople *people);
static void ResortFieldPeople_RemoveWalker(ResortFieldPeople *people, ResortPerson *person);
static ResortWalkers *ResortFieldPeople_GetWalkers(ResortFieldPeople *people);
static void ResortFieldPeople_PlaceAtShops(ResortFieldPeople *people, ResortSys *sys, ResortNPC *npc);
static void ResortFieldPeople_MarkChanged(ResortFieldPeople *people);
static BOOL ResortFieldPeople_WatchCallback(void *context, void *person, u32 a2, u32 event);
static BOOL ResortFieldPeople_WatchCallbackFiltered(void *context, void *person, u32 a2, u32 event);
static ResortFieldScene *ResortFieldScene_Create(HeapID heapId, FieldExpObjSystem *expObj, FieldCamera *camera);
static void ResortFieldScene_Free(ResortFieldScene *scene);
static void ResortFieldScene_Update(ResortFieldScene *scene);
static void ResortFieldScene_StartAnim(ResortFieldScene *scene, u32 index);
static BOOL ResortFieldScene_IsAnimDone(ResortFieldScene *scene, u32 index);
static void ResortFieldScene_EndAnim(ResortFieldScene *scene, u32 index);
static void ResortSceneAnim0_Start(ResortFieldScene *scene);
static BOOL ResortSceneAnim0_Update(ResortFieldScene *scene);
static void ResortSceneAnim0_End(ResortFieldScene *scene);
static void ResortSceneAnim1_Start(ResortFieldScene *scene);
static BOOL ResortSceneAnim1_Update(ResortFieldScene *scene);
static void ResortSceneAnim1_End(ResortFieldScene *scene);
static void ResortField_AddInitialEntries(ResortSys *sys, GameData *gameData, HeapID heapId);
static void ResortField_AddSpecialEntry(ResortSys *sys, GameData *gameData, HeapID heapId);

// Not referenced, as reading it is folded: the angle of the scene's second animation
const u32 RESORT_FIELD_SCENE_ANGLE = 0xdd00;
static const u32 sWalkerOccupantKinds[] = { 0 };
static const u8 sCountByRank[] = { 1, 2, 3, 4, 5 };
static const u8 sCountByUnk[] = { 0, 1, 2, 3, 4, 5 };
static const G3DSceneAnimationSetup sSceneAnimations1[] = { { 4, 0 }, { 5, 0 } };
static const G3DSceneAnimationSetup sSceneAnimations0[] = { { 1, 0 }, { 2, 0 } };
static const u8 sCountByBadges[] = { 0, 1, 1, 1, 1, 2, 2, 2, 2 };
static const u16 sGroupXs[RESORT_WALKER_GROUPS] = { 12, 13, 14, 16, 17, 18 };
static const u16 sGroupDirs[RESORT_WALKER_GROUPS] = { 0, 1, 0, 1, 0, 1 };
// clang-format off
static const ResortWalkerPlace sWalkerPlaces[] = {
    {2, 0, 0, 0}, {2, 1, 1, 0}, {2, 2, 2, 0}, {2, 3, 3, 0}, {2, 4, 4, 0}, {2, 5, 5, 0}, {2, 6, 0, 1},
    {2, 7, 1, 1}, {3, 0, 2, 1}, {3, 1, 3, 1}, {3, 2, 4, 1}, {3, 3, 5, 1}, {3, 4, 0, 2}, {3, 5, 1, 2},
    {3, 6, 2, 2}, {3, 7, 3, 2}, {4, 0, 4, 2}, {4, 1, 5, 2}, {4, 2, 0, 3}, {4, 3, 1, 3}, {4, 4, 2, 3},
    {4, 5, 3, 3}, {4, 6, 4, 3}, {4, 7, 5, 3}, {4, 8, 0, 4}, {4, 9, 1, 4}, {4, 10, 2, 4}, {4, 11, 3, 4},
};

// clang-format on
static const u8 sBubbleYs[RESORT_BUBBLES] = { 6, 2, 19, 14, 17, 10, 14, 2, 10, 17, 10, 6, 2, 6, 14, 19 };
static const u8 sBubbleXs1[RESORT_BUBBLES] = { 1, 16, 2, 15, 4, 16, 3, 13, 1, 13, 2, 16, 3, 15, 4, 14 };
static const u8 sBubbleXs0[RESORT_BUBBLES] = { 1, 18, 2, 16, 4, 18, 3, 15, 1, 15, 2, 18, 3, 17, 4, 16 };
static const u32 sBubbleIntervalKinds[] = { 0, 2, 3, 4 };
static const u32 sBubbleKinds[] = { 0, 2, 3, 4 };
static BOOL (*const sWalkerStates[])(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) = {
    ResortWalker_StateCheckSteps, ResortWalker_StateStep,       ResortWalker_StateWaitStep,
    ResortWalker_StateLookAround, ResortWalker_StateStopAtShop,
};
static const FieldActorMoveCode sWalkerMoveCode = {
    MOVE_CODE_NONE, ResortWalkerMove_Unk4, ResortWalkerMove_Update, ResortWalkerMove_Unk0C, ResortWalkerMove_Unk10,
};
static const ResortSceneAnim sSceneAnims[] = {
    { ResortSceneAnim0_Start, ResortSceneAnim0_Update, ResortSceneAnim0_End },
    { ResortSceneAnim1_Start, ResortSceneAnim1_Update, ResortSceneAnim1_End },
};
static const G3DSceneActorSetup sSceneActors[] = {
    { 0, 0, 0, 0, sSceneAnimations0, 2 },
    { 3, 0, 3, 0, sSceneAnimations1, 2 },
};
static const G3DSceneResourceSetup sSceneResources[] = {
    { 298, 1, 0 }, { 298, 0, 0 }, { 298, 2, 0 }, { 298, 4, 0 }, { 298, 3, 0 }, { 298, 5, 0 },
};
static const G3DSceneSetup sScene = { sSceneResources, 6, sSceneActors, 2 };

void ResortField_Setup(Field *field) {
    HollowRivalSave *block;
    GameData *gameData;
    ResortWork *unk;
    ResortFieldWork *work;
    HeapID heapId;
    JoinAvenueSave *joinAvenue;
    JoinAvenuePersonList **list2;
    ResortSysSetup sysSetup;
    BOOL flag;
    u32 unk2;
    MMSys *mmSys;
    ResortNPCSetup npcSetup;
    FieldMsgBG *msgBGSys;
    ResortPersonData **datas;
    JoinAvenueInfo *info;
    JoinAvenueOccupants *occupants;
    FieldExpObjSystem *expObj;
    BOOL bubblesEnabled;
    u32 i;
    JoinAvenueEntryList *entries;

    heapId = Field_GetHeapID(field);
    gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    unk = GameData_GetResortWork(gameData);
    work = Field_AllocGimmickWorkBlock(field, 1, heapId, sizeof(ResortFieldWork));
    bubblesEnabled = TRUE;
    joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
    list2 = GameData_GetJoinAvenuePersonListPtr(gameData);
    MI_CpuFill8(&sysSetup, 0, sizeof(ResortSysSetup));
    sysSetup.occupants = getAddressOfBeginningOfOccupants(joinAvenue);
    sysSetup.list = JoinAvenue_GetPersonList(joinAvenue);
    sysSetup.list2 = *list2;
    sysSetup.entries = JoinAvenue_GetEntryList(joinAvenue);
    sysSetup.info = JoinAvenue_GetInfo(joinAvenue);
    work->sys = ResortSys_Create(&sysSetup, unk, heapId);
    flag = JoinAvenue_GetParam(ResortSys_GetInfo(work->sys), JOIN_AVE_INFO_UNOPENED, NULL);
    ResortField_AddSpecialEntry(work->sys, gameData, heapId);
    ResortField_AddInitialEntries(work->sys, gameData, heapId);
    unk2 = ResortWork_Get(unk, 8);
    mmSys = GameData_GetMMSys(gameData);
    work->people = ResortFieldPeople_Create(heapId, work->sys, mmSys, Field_GetPlayer(field), flag, unk2);
    ResortFieldPeople_CreatePeople(work->people, flag, unk2);

    MI_CpuFill8(&npcSetup, 0, sizeof(ResortNPCSetup));
    npcSetup.mmSys = GameData_GetMMSys(gameData);
    npcSetup.unk4 = 16;
    npcSetup.info = ResortSys_GetInfo(work->sys);
    npcSetup.zone = ResortLayout_GetZoneIndex(Field_GetPlayerStateZoneID(field));
    npcSetup.table = ResortSys_GetShopOpenings(work->sys);
    work->npc = ResortNPC_Create(&npcSetup, heapId);

    msgBGSys = Field_GetMsgBGSys(field);
    datas = ResortSys_GetDatas(work->sys);
    info = ResortSys_GetInfo(work->sys);
    occupants = ResortSys_GetOccupants(work->sys);
    if (flag) {
        bubblesEnabled = FALSE;
    }
    work->bubbles = ResortBubbles_Create(heapId, msgBGSys, datas, info, occupants, bubblesEnabled);
    expObj = Field_GetExpObjSystem(field);
    work->scene = ResortFieldScene_Create(heapId, expObj, Field_GetCameraSystem(field));

    block = getHollow_RivalBlk(GameData_GetSaveControl(gameData));
    entries = ResortSys_GetEntries(work->sys);
    for (i = 0; i < JoinAvenueEntryList_GetCount(entries); i++) {
        JoinAvenueEntry *entry = JoinAvenueEntryList_Get(entries, i);
        if (!JoinAvenueEntry_IsEmpty(entry) &&
            (u16)JoinAvenueEntry_GetParam(entry, JOIN_AVE_PARAM_TRAINER_TYPE, NULL) == 223) {
            HollowRivalSave_SetValue(block, 2, 15);
            return;
        }
    }
}

void ResortField_Delete(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    ResortFieldScene_Free(work->scene);
    ResortBubbles_Free(work->bubbles);
    ResortNPC_Free(work->npc);
    ResortFieldPeople_Free(work->people);
    ResortSys_Free(work->sys);
    Field_DeleteGimmickWorkBlock(field, 1);
}

void ResortField_Update(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    ResortFieldPeople_Update(work->people, field);
    ResortNPC_Update(work->npc, field);
    ResortBubbles_Update(work->bubbles, field);
    ResortFieldScene_Update(work->scene);
}

ResortPeople *ResortField_GetPeople(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    return ResortFieldPeople_GetPeople(work->people);
}

ResortNPC *ResortField_GetNPC(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    return work->npc;
}

ResortSys *ResortField_GetSys(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    return work->sys;
}

void ResortField_RemoveWalker(Field *field, ResortPerson *person) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    ResortFieldPeople_RemoveWalker(work->people, person);
}

void ResortField_StartSceneAnim(Field *field, u32 index) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    ResortFieldScene_StartAnim(work->scene, index);
}

BOOL ResortField_IsSceneAnimDone(Field *field, u32 index) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    return ResortFieldScene_IsAnimDone(work->scene, index);
}

void ResortField_EndSceneAnim(Field *field, u32 index) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    ResortFieldScene_EndAnim(work->scene, index);
}

void ResortField_RefreshPeople(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    ResortFieldPeople_UseNormalWatch(work->people);
    ResortBubbles_Enable(work->bubbles);
}

void ResortField_MarkPeopleChanged(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    ResortFieldPeople_MarkChanged(work->people);
}

void ResortField_EnterEventMode(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    ResortBubbles_SetEventMode(work->bubbles);
    ResortFieldPeople_PlaceAtShops(work->people, work->sys, work->npc);
}

void ResortField_SetBubblesActive(Field *field, BOOL start) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    ResortBubbles_SetActive(work->bubbles, start);
}

static ResortWatch *ResortWatch_Create(HeapID heapId, const ResortWatchSetup *setup) {
    ResortWatch *watch = GFL_HeapAllocate(heapId, sizeof(ResortWatch), TRUE, "resort_field.c", 659);
    watch->setup = *setup;
    watch->list = setup->list;
    watch->entries = setup->entries;
    watch->list2 = setup->list2;
    GameBeaconSys_SetAvenuePeople(watch->list2);
    ResortWatch_MarkChanged(watch);
    return watch;
}

static void ResortWatch_Free(ResortWatch *watch) {
    GameBeaconSys_SetAvenuePeople(NULL);
    GFL_HeapFree(watch);
}

static void ResortWatch_Update(ResortWatch *watch) {
    if (watch->timer++ > watch->setup.interval) {
        ResortWatch_CheckLists(watch);
        ResortWatch_CheckEntries(watch);
        watch->timer = 0;
    }
}

static void ResortWatch_SetCallback(ResortWatch *watch, ResortWatchCallback callback) {
    watch->setup.callback = callback;
}

static void ResortWatch_MarkChanged(ResortWatch *watch) {
    watch->changed = TRUE;
}

static void ResortWatch_CheckLists(ResortWatch *watch) {
    u32 i;
    u32 j;

    for (i = 0; i < JoinAvenuePersonList_GetCount(watch->list); i++) {
        JoinAvenuePerson *person = JoinAvenuePersonList_Get(watch->list, i);
        u32 value = joinAveTextHandler(person, JOIN_AVE_PARAM_STATUS, NULL);
        if (!JoinAvenuePerson_IsEmpty(person) && value != JOIN_AVE_STATUS_NONE) {
            JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_NONE);
            if (!watch->setup.callback(watch->setup.context, person, 2, value)) {
                JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, value);
            }
        }
    }
    for (j = 0; j < JoinAvenuePersonList_GetCount(watch->list2); j++) {
        JoinAvenuePerson *person = JoinAvenuePersonList_Get(watch->list2, j);
        u32 value = joinAveTextHandler(person, JOIN_AVE_PARAM_STATUS, NULL);
        if (!JoinAvenuePerson_IsEmpty(person) && value != JOIN_AVE_STATUS_NONE) {
            JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_NONE);
            if (!watch->setup.callback(watch->setup.context, person, 2, value)) {
                JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, value);
            }
        }
    }
}

static void ResortWatch_CheckEntries(ResortWatch *watch) {
    if (watch->changed) {
        u32 i;
        for (i = 0; i < JoinAvenueEntryList_GetCount(watch->entries); i++) {
            JoinAvenueEntry *entry = JoinAvenueEntryList_Get(watch->entries, i);
            if (!JoinAvenueEntry_IsEmpty(entry)) {
                watch->setup.callback(watch->setup.context, entry, 1, 2);
            }
        }
        watch->changed = FALSE;
    }
}

static ResortWalkers *ResortWalkers_Create(HeapID heapId, MMSys *mmSys, FieldPlayer *player, ResortShopData *shops,
                                           ResortBinary *unkb1c, JoinAvenueOccupants *occupants) {
    int i;
    ResortWalkers *walkers = GFL_HeapAllocate(heapId, sizeof(ResortWalkers), TRUE, "resort_field.c", 950);

    walkers->player = player;
    walkers->mmSys = mmSys;
    walkers->shops = shops;
    walkers->unkb1c = unkb1c;
    walkers->occupants = occupants;
    for (i = 0; i < RESORT_WALKERS; i++) {
        ResortWalker_Init(&walkers->walkers[i], heapId);
    }
    for (i = 0; i < RESORT_WALKER_GROUPS; i++) {
        ResortWalkerGroup_Init(&walkers->groups[i], sGroupXs[i], sGroupDirs[i], heapId);
    }
    return walkers;
}

static void ResortWalkers_Free(ResortWalkers *walkers) {
    int i;
    for (i = 0; i < RESORT_WALKER_GROUPS; i++) {
        ResortWalkerGroup_Clear(&walkers->groups[i]);
    }
    for (i = 0; i < RESORT_WALKERS; i++) {
        ResortWalker_Release(&walkers->walkers[i]);
    }
    GFL_HeapFree(walkers);
}

static void ResortWalkers_Update(ResortWalkers *walkers, Field *field) {
    int i;
    if (!Field_IsEventRunning(field)) {
        for (i = 0; i < RESORT_WALKER_GROUPS; i++) {
            ResortWalkerGroup_Update(&walkers->groups[i], walkers->player);
        }
    }
    for (i = 0; i < RESORT_WALKERS; i++) {
        ResortWalker_Update(&walkers->walkers[i], walkers->mmSys, walkers->player);
    }
}

static void ResortWalkers_Place(ResortWalkers *walkers, u32 mode) {
    int i;
    for (i = 0; i < RESORT_WALKER_GROUPS; i++) {
        ResortWalkerGroup_Place(&walkers->groups[i], mode);
    }
}

static void ResortWalkers_Add(ResortWalkers *walkers, ResortPerson *person) {
    u16 group;
    u16 slot;
    int i;

    if (ResortField_GetWalkerPlace(person, &group, &slot)) {
        for (i = 0; i < RESORT_WALKERS; i++) {
            if (!ResortWalker_IsAssigned(&walkers->walkers[i])) {
                ResortWalker_Assign(&walkers->walkers[i], person, walkers->shops, walkers->unkb1c, walkers->occupants);
                ResortWalkerGroup_Add(&walkers->groups[group], slot, &walkers->walkers[i]);
                return;
            }
        }
    }
}

static void ResortWalkers_Remove(ResortWalkers *walkers, ResortPerson *person) {
    u16 group;
    u16 slot;

    if (ResortField_GetWalkerPlace(person, &group, &slot)) {
        ResortWalker *walker = ResortWalkers_Find(walkers, person);
        if (walker != NULL) {
            ResortWalkerGroup_Remove(&walkers->groups[group], slot);
            ResortWalker_Release(walker);
        }
    }
}

static void ResortWalkers_MarkObjCodeChanged(ResortWalkers *walkers, ResortPerson *person) {
    ResortWalker *walker = ResortWalkers_Find(walkers, person);
    if (walker != NULL) {
        ResortWalker_MarkObjCodeChanged(walker);
    }
}

static void ResortField_SetActorPos(FieldActor *actor, u16 x, u16 z) {
    VecFx32 pos;
    pos.x = (x << 16) + 0x8000;
    pos.y = 0;
    pos.z = (z << 16) + 0x8000;
    SetActorWPosValue(actor, &pos);
    SetActorGPosX(actor, x);
    SetActorGPosZ(actor, z);
}

static ResortWalker *ResortWalkers_Find(ResortWalkers *walkers, ResortPerson *person) {
    u16 group;
    u16 slot;
    int i;

    if (ResortField_GetWalkerPlace(person, &group, &slot)) {
        for (i = 0; i < RESORT_WALKERS; i++) {
            if (ResortWalker_IsPerson(&walkers->walkers[i], person)) {
                return &walkers->walkers[i];
            }
        }
    }
    return NULL;
}

static void ResortWalker_Init(ResortWalker *walker, HeapID heapId) {
    ResortWalker_Clear(walker);
}

static void ResortWalker_Release(ResortWalker *walker) {
    if (ResortWalker_IsAssigned(walker)) {
        ChangeActorMoveCodeSeq(ResortPerson_GetActor(walker->person), MOVE_CODE_NONE);
    }
    ResortWalker_Clear(walker);
}

static void ResortWalker_Update(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    if (ResortWalker_IsAssigned(walker)) {
        FieldActor *actor = ResortPerson_GetActor(walker->person);
        if (walker->unk14 && !walker->unk15) {
            switch (walker->unk54) {
            case 0:
                FldAct_SetLocalAcmd(actor, 0xbe);
                walker->unk54++;
            case 1:
                if (FieldAcmdTCB_Update(actor)) {
                    walker->unk54 = 0;
                    walker->unk14 = FALSE;
                    walker->unk15 = TRUE;
                }
                break;
            }
        }
        if (walker->objCodeChanged && ResortWalker_IsIdle(walker)) {
            ResortPerson_UpdateObjCode(walker->person);
            walker->objCodeChanged = FALSE;
        }
    }
}

static void ResortWalker_Assign(ResortWalker *walker, ResortPerson *person, ResortShopData *shops, ResortBinary *unkb1c,
                                JoinAvenueOccupants *occupants) {
    ResortWalker_Clear(walker);
    walker->person = person;
    FldAct_SetMoveCodeFuncs(ResortPerson_GetActor(person), &sWalkerMoveCode);
    ResortPerson_SetVisible(walker->person, FALSE);
    ResortWalker_FindStops(walker, shops, unkb1c, occupants);
    switch (ResortPerson_GetAttr(walker->person, 0)) {
    case 1:
    case 2:
        walker->start = ResortWalker_SetupWalk;
        walker->update = ResortWalker_RunStates;
        break;
    }
}

static void ResortWalker_Clear(ResortWalker *walker) {
    MI_CpuFill8(walker, 0, sizeof(ResortWalker));
    walker->magic = 0x765f573;
}

static BOOL ResortWalker_IsPerson(const ResortWalker *walker, ResortPerson *person) {
    if (person == walker->person) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ResortWalker_IsAssigned(const ResortWalker *walker) {
    if (walker->person != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void ResortWalker_Start(ResortWalker *walker, u16 acmd, u16 dir, u16 x) {
    ResortWalker_StartAt(walker, acmd, dir, x, 0);
}

static void ResortWalker_StartAt(ResortWalker *walker, u16 acmd, u16 dir, u16 x, u16 z) {
    FieldActor *actor;

    walker->state = 0;
    walker->active = TRUE;
    walker->acmd = acmd;
    walker->dir = dir;
    actor = ResortPerson_GetActor(walker->person);
    if (ResortWalker_IsAtOrigin(walker)) {
        if (walker->dir == DIR_UP) {
            if (z == 0) {
                z = 77;
            }
            walker->steps = z - 8;
            if (walker->steps <= 0) {
                walker->steps = 1;
            }
        } else {
            if (z == 0) {
                z = 9;
            }
            walker->steps = 78 - z;
            if (walker->steps <= 0) {
                walker->steps = 1;
            }
        }
        ResortField_SetActorPos(actor, x, z);
    } else {
        s16 actorZ = GetGPosZ(actor);
        if (walker->dir == DIR_UP) {
            if (actorZ < 9) {
                ResortField_SetActorPos(actor, x, 77);
                walker->steps = 69;
                if (walker->steps <= 0) {
                    walker->steps = 1;
                }
            } else {
                walker->steps = actorZ - 8;
                if (walker->steps <= 0) {
                    walker->steps = 1;
                }
                ResortField_SetActorPos(actor, x, actorZ);
            }
        } else if (walker->dir == DIR_DOWN) {
            if (actorZ > 77) {
                ResortField_SetActorPos(actor, x, 9);
                walker->steps = 69;
                if (walker->steps <= 0) {
                    walker->steps = 1;
                }
            } else {
                walker->steps = 78 - actorZ;
                if (walker->steps <= 0) {
                    walker->steps = 1;
                }
                ResortField_SetActorPos(actor, x, actorZ);
            }
        }
    }
    ResortPerson_SetVisible(walker->person, TRUE);
    ResortPerson_SetParam(walker->person, JOIN_AVE_PARAM_UNK_38, 1);
    if (walker->start != NULL) {
        walker->start(walker);
    }
}

static BOOL ResortWalker_IsIdle(ResortWalker *walker) {
    if (walker->active == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ResortWalker_IsAtOrigin(ResortWalker *walker) {
    FieldActor *actor = ResortPerson_GetActor(walker->person);
    s16 x = GetGPosX(actor);
    s16 z = GetGPosZ(actor);
    if (x == 0 && z == 0) {
        return TRUE;
    }
    return FALSE;
}

static void ResortWalker_MarkObjCodeChanged(ResortWalker *walker) {
    walker->objCodeChanged = TRUE;
}

static BOOL ResortWalker_IsFast(ResortWalker *walker) {
    u32 type = ResortPerson_GetAttr(walker->person, 0);
    u32 value = ResortPerson_GetParam(walker->person, JOIN_AVE_PARAM_SHOP_KIND, NULL);
    if (type == 1 && (value == 2 || value == 3)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ResortWalker_IsBlocked(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player, u32 a3, u32 a4) {
    if (ActorRouteCollCheckOneTileInDir(ResortPerson_GetActor(walker->person), walker->dir) & 4) {
        return TRUE;
    }
    return FALSE;
}

static void ResortWalker_SetupWalk(ResortWalker *walker) {
    s16 stepSpeed;
    u16 speed = ResortPerson_GetParam(walker->person, JOIN_AVE_PARAM_DATA_COUNT, NULL);

    if (ResortPerson_GetAttr(walker->person, 0) == 1) {
        u32 value = ResortPerson_GetParam(walker->person, JOIN_AVE_PARAM_SHOP_KIND, NULL);
        if (value == 2 || value == 3) {
            speed = 7;
        }
    }
    stepSpeed = walker->steps * speed / 69;
    walker->lookState = 0;
    walker->waitTimer = 0;
    walker->waitTime = 15;
    walker->unk24 = stepSpeed;
    walker->unk28 = walker->steps / walker->unk24 + 2 - GFL_RandomMTRange(5);
    if (walker->unk28 <= 0) {
        walker->unk28 = 1;
    }
    walker->unk26 = walker->steps - walker->unk28;
}

static BOOL ResortWalker_StateCheckSteps(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    if (walker->steps <= 0) {
        walker->active = FALSE;
        walker->state = 0;
        ResortPerson_SetVisible(walker->person, FALSE);
        ResortField_SetActorPos(ResortPerson_GetActor(walker->person), 0, 0);
        ResortPerson_SetParam(walker->person, JOIN_AVE_PARAM_UNK_38, 0);
        return FALSE;
    }
    walker->state = 1;
    return TRUE;
}

static BOOL ResortWalker_StateStep(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    FieldActor *actor = ResortPerson_GetActor(walker->person);

    if (ResortWalker_IsBlocked(walker, mmSys, player, 1, 0)) {
        FldAct_SetLocalAcmd(actor, 0x3f);
        FldAct_SetMoving(actor);
        walker->state = 2;
    } else if (ResortWalker_CheckStop(walker, player)) {
        walker->state = 4;
    } else if (walker->unk26 == walker->steps) {
        walker->lookState = 0;
        walker->state = 3;
    } else {
        FldAct_SetLocalAcmd(actor, GetAcmdForDir(walker->dir, walker->acmd));
        SetActorMotionDir(actor, walker->dir);
        FldAct_SetMoving(actor);
        walker->steps--;
        walker->state = 2;
    }
    return TRUE;
}

static BOOL ResortWalker_StateWaitStep(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    FieldActor *actor = ResortPerson_GetActor(walker->person);
    if (FieldAcmdTCB_Update(actor)) {
        FldAct_ClearMoving(actor);
        walker->state = 0;
    }
    return FALSE;
}

static BOOL ResortWalker_StateLookAround(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    FieldActor *actor = ResortPerson_GetActor(walker->person);

    switch (walker->lookState) {
    case 0:
        walker->lookDir = GFL_RandomMTRange(2) ? DIR_LEFT : DIR_RIGHT;
        FldAct_SetLocalAcmd(actor, walker->lookDir);
        walker->lookState++;
    case 1:
        if (FieldAcmdTCB_Update(actor)) {
            walker->lookState++;
        } else {
            return FALSE;
        }
    case 2:
        if (walker->waitTimer++ >= walker->waitTime) {
            walker->waitTimer = 0;
            walker->lookState++;
        } else {
            return FALSE;
        }
    case 3:
        walker->lookDir = walker->lookDir == DIR_RIGHT ? DIR_LEFT : DIR_RIGHT;
        FldAct_SetLocalAcmd(actor, walker->lookDir);
        walker->lookState++;
    case 4:
        if (FieldAcmdTCB_Update(actor)) {
            walker->lookState++;
        } else {
            return FALSE;
        }
    case 5:
        if (walker->waitTimer++ >= walker->waitTime) {
            walker->lookState = 0;
            walker->waitTimer = 0;
            walker->unk26 = walker->steps - walker->unk28;
            walker->state = 0;
        } else {
            return FALSE;
        }
        break;
    }
    return TRUE;
}

static BOOL ResortWalker_StateStopAtShop(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    FieldActor *actor = ResortPerson_GetActor(walker->person);

    switch (walker->stopState) {
    case 0:
        FldAct_ClearMoving(actor);
        FldAct_ClearMoveEnd(actor);
        FldAct_SetLocalAcmd(actor, walker->stopAcmd[walker->stop]);
        walker->stopState++;
    case 1:
        if (FieldAcmdTCB_Update(actor)) {
            walker->stopState++;
        } else {
            return FALSE;
        }
    case 2:
        walker->unk54 = 0;
        walker->unk14 = TRUE;
        walker->unk15 = FALSE;
        walker->stopState++;
    case 3:
        if (walker->unk15) {
            walker->state = 2;
        } else {
            return FALSE;
        }
        break;
    }
    return TRUE;
}

static void ResortWalker_RunStates(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    while (sWalkerStates[walker->state](walker, mmSys, player) == TRUE) {
    }
}

static void ResortWalker_FindStops(ResortWalker *walker, ResortShopData *shops, ResortBinary *unkb1c,
                                   JoinAvenueOccupants *occupants) {
    int i;
    u16 id = ResortPerson_GetParam(walker->person, JOIN_AVE_PARAM_SHOP_ID, NULL);
    u16 shop = ResortShopData_GetShopParam(ResortShopData_GetShop(shops, id), 0);

    GetGPosZ(ResortPerson_GetActor(walker->person));
    for (i = 0; i < RESORT_WALKER_STOPS; i++) {
        JoinAvenuePerson *occupant = JoinAvenueOccupants_GetPerson(occupants, i);
        if (!JoinAvenuePerson_IsEmpty(occupant) &&
            shop == ResortShopData_GetShopParam(ResortShopData_GetPersonShop(shops, occupant), 0)) {
            u16 z = ResortBinary_Get(unkb1c, i, 2);
            walker->stopZ[walker->stopCount] = z + GFL_RandomMTRange(7);
            walker->stopAcmd[walker->stopCount] = i % 2 == 0 ? DIR_RIGHT : DIR_LEFT;
            walker->stopCount++;
        }
    }
    walker->stop = walker->stopCount;
}

static BOOL ResortWalker_CheckStop(ResortWalker *walker, FieldPlayer *player) {
    s16 x;
    s16 y;
    s16 z;
    int i;
    FieldActor *actor = ResortPerson_GetActor(walker->person);
    s16 actorZ;

    FieldPlayer_GetGPos(player, &x, &y, &z);
    actorZ = GetGPosZ(actor);
    if (z - 20 >= actorZ || actorZ >= z + 7) {
        return FALSE;
    }
    for (i = 0; i < walker->stopCount; i++) {
        if (walker->stopZ[i] == GetGPosZ(actor) && walker->stop != i) {
            walker->stopState = 0;
            walker->stop = i;
            return TRUE;
        }
    }
    return FALSE;
}

static void ResortWalkerMove_Unk4(FieldActor *actor) {
}

static void ResortWalkerMove_Update(FieldActor *actor) {
    MMSys *mmSys = GetActorMModelSystem(actor);
    Field *field = GetMMSysField(mmSys);
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    ResortWalkers *walkers = ResortFieldPeople_GetWalkers(work->people);
    ResortWalker *walker =
        ResortWalkers_Find(walkers, ResortPeople_FindByActor(ResortFieldPeople_GetPeople(work->people), actor));

    if (walker->update != NULL && walker->active) {
        walker->update(walker, mmSys, Field_GetPlayer(field));
    }
}

static void ResortWalkerMove_Unk0C(FieldActor *actor) {
}

static void ResortWalkerMove_Unk10(FieldActor *actor) {
}

static void ResortWalkerGroup_Init(ResortWalkerGroup *group, u16 x, u16 dir, HeapID heapId) {
    MI_CpuFill8(group, 0, sizeof(ResortWalkerGroup));
    group->x = x;
    group->dir = dir;
    group->timer = GFL_RandomMTRange(239) + 1;
}

static void ResortWalkerGroup_Clear(ResortWalkerGroup *group) {
    MI_CpuFill8(group, 0, sizeof(ResortWalkerGroup));
}

static void ResortWalkerGroup_Update(ResortWalkerGroup *group, FieldPlayer *player) {
    int i;
    u32 active = 0;
    FieldActor *actor = FieldPlayer_GetActor(player);
    s16 z = GetGPosZ(actor);
    s16 x = GetGPosX(actor);

    if (x == group->x) {
        if (group->dir == DIR_UP) {
            if (z == 77 || z == 78) {
                return;
            }
        } else if (group->dir == DIR_DOWN) {
            if (z == 9 || z == 10) {
                return;
            }
        }
    }
    for (i = 0; i < RESORT_WALKER_GROUP_SIZE; i++) {
        if (group->walkers[i] != NULL && !ResortWalker_IsIdle(group->walkers[i])) {
            active++;
        }
    }
    if (group->count == 0 || active >= 1) {
        return;
    }
    while (group->walkers[group->next] == NULL) {
        group->next++;
        group->next %= RESORT_WALKER_GROUP_SIZE;
    }
    if (ResortWalker_IsFast(group->walkers[group->next])) {
        group->timer = 0;
    }
    if (group->timer-- <= 0) {
        if (ResortWalker_IsIdle(group->walkers[group->next])) {
            ResortWalker_Start(group->walkers[group->next], 12, group->dir, group->x);
            group->timer = GFL_RandomMTRange(239) + 1;
        }
        group->next++;
        group->next %= RESORT_WALKER_GROUP_SIZE;
    }
}

static void ResortWalkerGroup_Place(ResortWalkerGroup *group, u32 mode) {
    int i;
    for (i = 0; i < RESORT_WALKER_GROUP_SIZE; i++) {
        if (group->walkers[i] != NULL && ResortWalker_IsAssigned(group->walkers[i])) {
            if (mode == 0) {
                if (ResortWalker_IsAtOrigin(group->walkers[i])) {
                    u16 z;
                    if (group->dir == DIR_UP) {
                        z = (GFL_RandomMTRange(8) + 8) * i;
                    } else {
                        z = (GFL_RandomMTRange(8) + 8) * (4 - i);
                    }
                    ResortWalker_StartAt(group->walkers[i], 12, group->dir, group->x, z);
                    group->next = i + 1;
                    group->next %= RESORT_WALKER_GROUP_SIZE;
                }
            } else if (mode == 1) {
                if (!ResortWalker_IsAtOrigin(group->walkers[i])) {
                    ResortWalker_Start(group->walkers[i], 12, group->dir, group->x);
                    group->timer = GFL_RandomMTRange(239) + 1;
                    group->next = i + 1;
                    group->next %= RESORT_WALKER_GROUP_SIZE;
                }
            }
        }
    }
}

static void ResortWalkerGroup_Add(ResortWalkerGroup *group, u16 slot, ResortWalker *walker) {
    group->walkers[slot] = walker;
    group->count++;
}

static void ResortWalkerGroup_Remove(ResortWalkerGroup *group, u16 slot) {
    group->count--;
    group->walkers[slot] = NULL;
}

static BOOL ResortField_GetWalkerPlace(ResortPerson *person, u16 *group, u16 *slot) {
    u32 i;
    ResortPersonData *data = ResortPerson_GetData(person);
    u32 kind = ResortData_GetKind(data);
    u16 index = ResortData_GetIndex(data);

    for (i = 0; i < NELEMS(sWalkerPlaces); i++) {
        if (kind == sWalkerPlaces[i].kind && index == sWalkerPlaces[i].index) {
            *group = sWalkerPlaces[i].group;
            *slot = sWalkerPlaces[i].slot;
            return TRUE;
        }
    }
    return FALSE;
}

static ResortBubbles *ResortBubbles_Create(HeapID heapId, FieldMsgBG *msgBGSys, ResortPersonData **datas,
                                           JoinAvenueInfo *info, JoinAvenueOccupants *occupants, BOOL enabled) {
    int i;
    ResortBubbles *bubbles = GFL_HeapAllocate(heapId, sizeof(ResortBubbles), TRUE, "resort_field.c", 2612);

    bubbles->msgBGSys = msgBGSys;
    bubbles->datas = datas;
    bubbles->info = info;
    bubbles->occupants = occupants;
    bubbles->enabled = enabled;
    bubbles->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_JOIN_AVENUE, heapId);
    for (i = 0; i < RESORT_BUBBLES; i++) {
        ResortBubble_Init(&bubbles->bubbles[i], i, heapId);
    }
    ResortBubbles_UpdateInterval(bubbles);
    return bubbles;
}

static void ResortBubbles_Free(ResortBubbles *bubbles) {
    int i;
    for (i = 0; i < RESORT_BUBBLES; i++) {
        ResortBubble_Hide(&bubbles->bubbles[i], bubbles->msgBGSys);
        ResortBubble_Free(&bubbles->bubbles[i]);
    }
    GFL_MsgDataFree(bubbles->msgData);
    GFL_HeapFree(bubbles);
}

static void ResortBubbles_Update(ResortBubbles *bubbles, Field *field) {
    BOOL run = FALSE;
    BOOL clear = FALSE;
    int i;

    if (!bubbles->enabled) {
        return;
    }
    if (bubbles->mode == FALSE) {
        if (!Field_IsEventRunning(field)) {
            run = TRUE;
        } else {
            clear = TRUE;
        }
    } else if (bubbles->active) {
        run = TRUE;
    }
    if (run) {
        switch (bubbles->state) {
        case 0:
            if (bubbles->timer++ < bubbles->interval) {
                break;
            }
            ResortBubbles_UpdateInterval(bubbles);
            bubbles->shown = 0;
            bubbles->count = GFL_RandomMTRange(2) + 1;
            bubbles->delay = 0;
            if (bubbles->mode == FALSE) {
                bubbles->delayTime = 30;
                bubbles->unk10 = GFL_RandomMTRange(3);
            } else {
                bubbles->delayTime = 10;
            }
            bubbles->timer = 0;
            bubbles->state++;
            break;
        case 1: {
            ResortPersonData *data;
            u16 bubble;
            u32 tries;

            if (bubbles->delay++ < bubbles->delayTime) {
                break;
            }
            data = ResortBubbles_PickData(bubbles);
            tries = 0;
            bubbles->delay = 0;
            if (data == NULL) {
                break;
            }
            switch (bubbles->shown) {
            case 0:
            default:
                bubble = GFL_RandomMTRange(RESORT_BUBBLES);
                break;
            case 1:
                do {
                    bubble = GFL_RandomMTRange(RESORT_BUBBLES);
                    if (tries++ >= 30) {
                        bubbles->count--;
                        bubbles->state = 2;
                        return;
                    }
                } while (ResortBubble_Overlaps(bubble, bubbles->bubbleIndexes[0], bubbles->mode));
                break;
            case 2:
                do {
                    bubble = GFL_RandomMTRange(RESORT_BUBBLES);
                    if (tries++ >= 30) {
                        bubbles->count--;
                        bubbles->state = 2;
                        return;
                    }
                } while (ResortBubble_Overlaps(bubble, bubbles->bubbleIndexes[0], bubbles->mode) ||
                         ResortBubble_Overlaps(bubble, bubbles->bubbleIndexes[1], bubbles->mode));
                break;
            }
            bubbles->bubbleIndexes[bubbles->shown] = bubble;
            if (bubbles->mode == TRUE) {
                bubbles->unk10 = GFL_RandomMTRange(4) + 4;
            }
            ResortBubble_Show(&bubbles->bubbles[bubble], bubbles->msgBGSys, data, bubbles->info, bubbles->occupants,
                              bubbles->msgData, bubbles->unk10, bubbles->mode);
            if (bubbles->shown++ >= bubbles->count) {
                bubbles->state = 2;
            }
            break;
        }
        case 2: {
            BOOL done = TRUE;
            for (i = 0; i < bubbles->count; i++) {
                done &= ResortBubble_IsDone(&bubbles->bubbles[bubbles->bubbleIndexes[i]]);
            }
            if (done) {
                bubbles->state = 0;
                bubbles->active = FALSE;
            }
            break;
        }
        }
        for (i = 0; i < RESORT_BUBBLES; i++) {
            ResortBubble_Update(&bubbles->bubbles[i], bubbles->msgBGSys);
        }
    } else if (clear) {
        ResortBubbles_HideAll(bubbles);
    }
}

static void ResortBubbles_HideAll(ResortBubbles *bubbles) {
    int i;
    for (i = 0; i < RESORT_BUBBLES; i++) {
        ResortBubble_Hide(&bubbles->bubbles[i], bubbles->msgBGSys);
    }
}

static void ResortBubbles_Enable(ResortBubbles *bubbles) {
    bubbles->enabled = TRUE;
}

static void ResortBubbles_SetEventMode(ResortBubbles *bubbles) {
    bubbles->mode = TRUE;
}

static void ResortBubbles_SetActive(ResortBubbles *bubbles, BOOL start) {
    if (bubbles->mode == TRUE) {
        if (start) {
            bubbles->state = 0;
            bubbles->timer = bubbles->interval;
            bubbles->active = TRUE;
        } else {
            ResortBubbles_HideAll(bubbles);
            bubbles->active = FALSE;
        }
    }
}

// The more people there are, the more often the bubbles show
static void ResortBubbles_UpdateInterval(ResortBubbles *bubbles) {
    u32 count = 0;
    ResortDataIter iter = ResortDatas_StartIter(bubbles->datas, sBubbleIntervalKinds, 4);
    ResortPersonData *data;

    while (TRUE) {
        data = ResortDatas_NextIter(bubbles->datas, &iter, sBubbleIntervalKinds, 4);
        if (data == NULL) {
            break;
        }
        if (!ResortData_IsEmpty(data)) {
            count++;
        }
    }
    if (count >= 22) {
        bubbles->interval = 60;
    } else if (count >= 16) {
        bubbles->interval = 120;
    } else if (count >= 10) {
        bubbles->interval = 180;
    } else if (count >= 4) {
        bubbles->interval = 240;
    } else if (count >= 1) {
        bubbles->interval = 360;
    } else {
        bubbles->interval = 0;
    }
}

static u16 sDataIndexes[36];
static u32 sDataKinds[36];

// A random data of the people who are not empty
static ResortPersonData *ResortBubbles_PickData(ResortBubbles *bubbles) {
    int count = 0;
    ResortDataIter iter = ResortDatas_StartIter(bubbles->datas, sBubbleKinds, 4);
    ResortPersonData *data;

    while (TRUE) {
        data = ResortDatas_NextIter(bubbles->datas, &iter, sBubbleKinds, 4);
        if (data == NULL) {
            break;
        }
        if (ResortData_IsEmpty(data)) {
            continue;
        }
        sDataKinds[count] = ResortData_GetKind(data);
        sDataIndexes[count] = ResortData_GetIndex(data);
        count++;
    }
    if (count == 0) {
        return NULL;
    }
    if (count == 1) {
        return ResortDatas_Get(bubbles->datas, sDataKinds[0], sDataIndexes[0]);
    }
    {
        int n = 0;
        u32 chosen = GFL_RandomMTRange(count);
        int i;

        for (i = 0; i < count; i++) {
            if (n++ == chosen) {
                return ResortDatas_Get(bubbles->datas, sDataKinds[i], sDataIndexes[i]);
            }
        }
    }
    return NULL;
}

static void ResortBubble_Init(ResortBubble *bubble, u16 index, HeapID heapId) {
    MI_CpuFill8(bubble, 0, sizeof(ResortBubble));
    bubble->strbuf = GFL_StrBufCreate(33, heapId);
    bubble->index = index;
}

static void ResortBubble_Free(ResortBubble *bubble) {
    GFL_StrBufFree(bubble->strbuf);
    MI_CpuFill8(bubble, 0, sizeof(ResortBubble));
}

static void ResortBubble_Update(ResortBubble *bubble, FieldMsgBG *msgBGSys) {
    if (bubble->active) {
        bubble->timer++;
        if (bubble->timer >= 60) {
            ResortBubble_Hide(bubble, msgBGSys);
        }
    }
}

// Whether the bubbles at two places overlap
static BOOL ResortBubble_Overlaps(u16 a, u16 b, BOOL mode) {
    const u8 *xs = sBubbleXs0;
    int width = 14;
    s16 bx;
    s16 ax;
    s16 ay;
    s16 by;
    s16 aBottom;
    s16 bBottom;
    s16 bRight;
    s16 aRight;

    if (mode == TRUE) {
        xs = sBubbleXs1;
        width += 2;
    }
    bx = xs[b];
    ax = xs[a];
    ay = sBubbleYs[a];
    by = sBubbleYs[b];
    aBottom = ay + 4;
    bBottom = by + 4;
    bRight = width + bx;
    aRight = width + ax;
    return (aBottom >= by) & ((ay <= bBottom) & ((ax <= bRight) & (aRight >= bx)));
}

static void ResortBubble_Show(ResortBubble *bubble, FieldMsgBG *msgBGSys, ResortPersonData *data, JoinAvenueInfo *info,
                              JoinAvenueOccupants *occupants, MsgData *msgData, u32 kind, BOOL mode) {
    u32 style;
    u8 width;
    u8 height;
    const u8 *xs;

    bubble->active = TRUE;
    bubble->timer = 0;
    style = 0;
    switch (kind) {
    case 0: {
        u16 name[10];
        ResortData_GetParam(data, JOIN_AVE_PARAM_GREETING, name);
        GFL_StrBufLoadString(bubble->strbuf, name);
        break;
    }
    case 1:
    case 3:
    default: {
        u16 text[9];
        ResortData_GetParam(data, JOIN_AVE_PARAM_MESSAGE1, text);
        GFL_StrBufLoadString(bubble->strbuf, text);
        style = 1;
        break;
    }
    case 2: {
        u16 text[9];
        ResortData_GetParam(data, JOIN_AVE_PARAM_MESSAGE2, text);
        GFL_StrBufLoadString(bubble->strbuf, text);
        style = 2;
        break;
    }
    case 4: {
        u16 text[9];
        joinAveTextHandler(JoinAvenueOccupants_GetPlayerPerson(occupants), JOIN_AVE_PARAM_MESSAGE1, text);
        GFL_StrBufLoadString(bubble->strbuf, text);
        style = 1;
        break;
    }
    case 5: {
        u16 text[9];
        joinAveTextHandler(JoinAvenueOccupants_GetPlayerPerson(occupants), JOIN_AVE_PARAM_MESSAGE2, text);
        GFL_StrBufLoadString(bubble->strbuf, text);
        style = 1;
        break;
    }
    case 6: {
        u16 name[11];
        JoinAvenue_GetParam(info, JOIN_AVE_INFO_TITLE, name);
        GFL_StrBufLoadString(bubble->strbuf, name);
        style = 1;
        break;
    }
    case 7:
        GFL_MsgDataLoadStrbuf(msgData, 29, bubble->strbuf);
        style = 1;
        break;
    }
    xs = sBubbleXs0;
    CalcMsgWindowDimensions(msgBGSys, bubble->strbuf, &width, &height);
    if (mode == TRUE) {
        xs = sBubbleXs1;
    }
    MsgBGPrinter_AddSubMsgWinStrFrame(msgBGSys, bubble->strbuf, bubble->index, xs[bubble->index],
                                      sBubbleYs[bubble->index], width, 2, style);
}

static void ResortBubble_Hide(ResortBubble *bubble, FieldMsgBG *msgBGSys) {
    if (bubble->active) {
        MsgBGPrinter_DeleteSubMsgWin(msgBGSys, bubble->index);
        ResortBubble_Reset(bubble);
    }
}

static void ResortBubble_Reset(ResortBubble *bubble) {
    bubble->timer = 0;
    bubble->active = FALSE;
}

static BOOL ResortBubble_IsDone(ResortBubble *bubble) {
    if (bubble->active == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static ResortFieldPeople *ResortFieldPeople_Create(HeapID heapId, ResortSys *sys, MMSys *mmSys, FieldPlayer *player,
                                                   BOOL flag, u32 a5) {
    ResortWatchSetup watchSetup;
    ResortPeopleSetup peopleSetup;
    ResortFieldPeople *people = GFL_HeapAllocate(heapId, sizeof(ResortFieldPeople), TRUE, "resort_field.c", 3601);

    people->sys = sys;
    MI_CpuFill8(&watchSetup, 0, sizeof(ResortWatchSetup));
    watchSetup.list = ResortSys_GetList(sys);
    watchSetup.list2 = ResortSys_GetList2(sys);
    watchSetup.entries = ResortSys_GetEntries(sys);
    watchSetup.context = people;
    watchSetup.callback = flag ? ResortFieldPeople_WatchCallbackFiltered : ResortFieldPeople_WatchCallback;
    watchSetup.interval = 0;
    people->watch = ResortWatch_Create(heapId, &watchSetup);

    MI_CpuFill8(&peopleSetup, 0, sizeof(ResortPeopleSetup));
    peopleSetup.mmSys = mmSys;
    peopleSetup.slots = ResortSys_GetSlots(sys);
    peopleSetup.datas = ResortSys_GetDatas(sys);
    peopleSetup.count = 64;
    peopleSetup.zone = 1;
    peopleSetup.shops = ResortSys_GetShopData(sys);
    peopleSetup.unk18 = ResortSys_GetPosOffsets(sys);
    people->people = ResortPeople_Create(&peopleSetup, heapId);
    people->walkers = ResortWalkers_Create(heapId, mmSys, player, ResortSys_GetShopData(sys),
                                           ResortSys_GetPosOffsets(sys), ResortSys_GetOccupants(sys));
    return people;
}

static void ResortFieldPeople_CreatePeople(ResortFieldPeople *people, BOOL flag, u32 a2) {
    int i;

    if (flag) {
        ResortPeople_CreateFromDatas(people->people);
    } else {
        ResortPeople_CreateFromSlots(people->people);
    }
    for (i = 0; i < 64; i++) {
        ResortPerson *person = ResortPeople_GetByIndex(people->people, i);
        if (person != NULL) {
            ResortWalkers_Add(people->walkers, person);
        }
    }
    ResortWalkers_Place(people->walkers, 1);
}

static void ResortFieldPeople_Free(ResortFieldPeople *people) {
    ResortWalkers_Free(people->walkers);
    ResortPeople_Free(people->people);
    ResortWatch_Free(people->watch);
    GFL_HeapFree(people);
}

static void ResortFieldPeople_Update(ResortFieldPeople *people, Field *field) {
    ResortWatch_Update(people->watch);
    ResortPeople_Update(people->people, Field_GetPlayer(field));
    ResortWalkers_Update(people->walkers, field);
}

static void ResortFieldPeople_UseNormalWatch(ResortFieldPeople *people) {
    ResortWatch_MarkChanged(people->watch);
    ResortWatch_SetCallback(people->watch, ResortFieldPeople_WatchCallback);
}

static ResortPeople *ResortFieldPeople_GetPeople(ResortFieldPeople *people) {
    return people->people;
}

static void ResortFieldPeople_RemoveWalker(ResortFieldPeople *people, ResortPerson *person) {
    ResortWalkers_Remove(people->walkers, person);
}

static ResortWalkers *ResortFieldPeople_GetWalkers(ResortFieldPeople *people) {
    return people->walkers;
}

// Moves the occupants' actors to the shops' doors at x 11 or 19, and turns the NPCs to them
static void ResortFieldPeople_PlaceAtShops(ResortFieldPeople *people, ResortSys *sys, ResortNPC *npc) {
    u32 i;
    ResortPersonData **datas = ResortSys_GetDatas(people->sys);
    ResortDataIter iter = ResortDatas_StartIter(datas, sWalkerOccupantKinds, 1);
    ResortPersonData *data;

    while (TRUE) {
        FieldActor *actor;
        s16 x;
        u16 dir;

        data = ResortDatas_NextIter(datas, &iter, sWalkerOccupantKinds, 1);
        if (data == NULL) {
            break;
        }
        if (ResortData_IsEmpty(data)) {
            continue;
        }
        actor = ResortPerson_GetActor(ResortPeople_FindByData(people->people, data));
        if (GetGPosX(actor) < 15) {
            x = 11;
            dir = DIR_RIGHT;
        } else {
            x = 19;
            dir = DIR_LEFT;
        }
        SetActorGPos(actor, x, 0, GetGPosZ(actor), dir);
    }
    for (i = 0; i < ResortNPC_GetCount(npc); i++) {
        FieldActor *actor = ResortNPC_GetActor(npc, i);
        if (actor != NULL) {
            ChangeActorMoveCodeSeq(actor, (GetGPosX(actor) < 15 ? DIR_RIGHT : DIR_LEFT) + MOVE_CODE_FACE_UP);
        }
    }
}

static void ResortFieldPeople_MarkChanged(ResortFieldPeople *people) {
    ResortWatch_MarkChanged(people->watch);
}

static BOOL ResortFieldPeople_WatchCallback(void *context, void *person, u32 a2, u32 event) {
    ResortFieldPeople *people = context;

    switch (event) {
    default:
        return TRUE;
    case 0:
        return TRUE;
    case 3:
        ResortWalkers_MarkObjCodeChanged(people->walkers, ResortPeople_FindByJoinAvenuePerson(people->people, person));
        return TRUE;
    case 4:
        return TRUE;
    case 1: {
        ResortPerson *resortPerson = ResortPeople_FindByJoinAvenuePerson(people->people, person);
        if (resortPerson != NULL) {
            ResortFieldPeople_RemoveWalker(people, resortPerson);
            ResortPeople_Delete(people->people, resortPerson);
        }
    }
    case 2:
        if (ResortPeople_FindByJoinAvenuePerson(people->people, person) == NULL) {
            ResortPersonSource source;
            MI_CpuFill8(&source, 0, sizeof(ResortPersonSource));
            source.data = ResortDatas_FindByPerson(ResortSys_GetDatas(people->sys), person);
            source.zone = 1;
            ResortWalkers_Add(people->walkers, ResortPeople_AddAtEmptySlot(people->people, &source));
        }
        return TRUE;
    }
    return TRUE;
}

static BOOL ResortFieldPeople_WatchCallbackFiltered(void *context, void *person, u32 a2, u32 event) {
    BOOL handle = FALSE;

    if (a2 == 1) {
        u32 value = JoinAvenueEntry_GetParam(person, JOIN_AVE_PARAM_SHOP_KIND, NULL);
        if (value == 2 || value == 3) {
            handle = TRUE;
        }
    }
    if (handle) {
        return ResortFieldPeople_WatchCallback(context, person, a2, event);
    }
    return FALSE;
}

static ResortFieldScene *ResortFieldScene_Create(HeapID heapId, FieldExpObjSystem *expObj, FieldCamera *camera) {
    ResortFieldScene *scene = GFL_HeapAllocate(heapId, sizeof(ResortFieldScene), TRUE, "resort_field.c", 4098);
    scene->expObj = expObj;
    scene->camera = camera;
    FieldExpObj_AddScene(expObj, &sScene, 0);
    return scene;
}

static void ResortFieldScene_Free(ResortFieldScene *scene) {
    FieldExpObj_FreeScene(scene->expObj, 0);
    GFL_HeapFree(scene);
}

static void ResortFieldScene_Update(ResortFieldScene *scene) {
    int i;

    FieldExpObj_StepAllAnimations(scene->expObj);
    for (i = 0; i < 2; i++) {
        if (scene->anims[i] != NULL && scene->anims[i]->update(scene)) {
            scene->anims[i] = NULL;
        }
    }
}

static void ResortFieldScene_StartAnim(ResortFieldScene *scene, u32 index) {
    scene->anims[index] = &sSceneAnims[index];
    scene->anims[index]->start(scene);
}

static BOOL ResortFieldScene_IsAnimDone(ResortFieldScene *scene, u32 index) {
    if (scene->anims[index] == NULL) {
        return TRUE;
    }
    return FALSE;
}

static void ResortFieldScene_EndAnim(ResortFieldScene *scene, u32 index) {
    if (scene->anims[index] != NULL) {
        scene->anims[index]->end(scene);
        scene->anims[index] = NULL;
    }
}

static void ResortSceneAnim0_Start(ResortFieldScene *scene) {
    u32 i = 0;
    SRTMatrix *matrix0 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 0);
    SRTMatrix *matrix1 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 1);
    fx32 cos = FX_CosIdx(0);
    fx32 sin = FX_SinIdx(0);

    MTX_RotX33_(&matrix0->rotation, sin, cos);
    MTX_RotX33_(&matrix1->rotation, sin, cos);
    matrix0->translation.x = FX32_CONST(127);
    matrix0->translation.z = FX32_CONST(500);
    matrix1->translation = matrix0->translation;
    FieldExpObj_SetActorHidden(scene->expObj, 0, 0, FALSE);
    FieldExpObj_SetActorHidden(scene->expObj, 0, 1, TRUE);
    for (i = 0; i < 2; i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(scene->expObj, 0, 0, i);
        FieldExpObj_SetAnmFrame(scene->expObj, 0, 0, i, 0);
        FieldExpObj_SetAnm(scene->expObj, 0, 0, i, TRUE);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}

static BOOL ResortSceneAnim0_Update(ResortFieldScene *scene) {
    u32 i = 0;
    VecFx32 target;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(scene->expObj, 0, 0, 0);
    SRTMatrix *matrix0;
    SRTMatrix *matrix1;

    if (!FieldExpObj_IsActorHidden(scene->expObj, 0, 0) && FieldExpObjAnm_IsPlaybackFinished(anm) == TRUE) {
        FieldExpObj_SetActorHidden(scene->expObj, 0, 0, TRUE);
        FieldExpObj_SetActorHidden(scene->expObj, 0, 1, FALSE);
        for (i = 0; i < 2; i++) {
            anm = FieldExpObj_GetAnmInfo(scene->expObj, 0, 1, i);
            FieldExpObj_SetAnmFrame(scene->expObj, 0, 1, i, 0);
            FieldExpObj_SetAnm(scene->expObj, 0, 1, i, TRUE);
            FieldExpObjAnm_SetLooped(anm, TRUE);
            FieldExpObjAnm_SetPaused(anm, FALSE);
        }
    }
    matrix0 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 0);
    matrix1 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 1);
    GFL_G3DCameraGetLookatPos(FieldCamera_GetG3DCamera(scene->camera), &target);
    matrix0->translation.x = target.x - FX32_CONST(121);
    matrix0->translation.z = target.z - FX32_CONST(257);
    matrix1->translation = matrix0->translation;
    return FALSE;
}

static void ResortSceneAnim0_End(ResortFieldScene *scene) {
    u32 i;

    FieldExpObj_SetActorHidden(scene->expObj, 0, 0, TRUE);
    FieldExpObj_SetActorHidden(scene->expObj, 0, 1, TRUE);
    for (i = 0; i < 2; i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(scene->expObj, 0, 0, i);
        FieldExpObj_SetAnm(scene->expObj, 0, 0, i, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
    }
    for (i = 0; i < 2; i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(scene->expObj, 0, 1, i);
        FieldExpObj_SetAnm(scene->expObj, 0, 1, i, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
    }
}

static void ResortSceneAnim1_Start(ResortFieldScene *scene) {
    SRTMatrix *matrix0;
    SRTMatrix *matrix1;
    fx32 cos;
    fx32 sin;

    ResortSceneAnim0_Start(scene);
    matrix0 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 0);
    matrix1 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 1);
    cos = FX_CosIdx(RESORT_FIELD_SCENE_ANGLE);
    sin = FX_SinIdx(RESORT_FIELD_SCENE_ANGLE);
    MTX_RotX33_(&matrix0->rotation, sin, cos);
    MTX_RotX33_(&matrix1->rotation, sin, cos);
}

static BOOL ResortSceneAnim1_Update(ResortFieldScene *scene) {
    VecFx32 target;
    BOOL done = ResortSceneAnim0_Update(scene);
    SRTMatrix *matrix0 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 0);
    SRTMatrix *matrix1 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 1);

    GFL_G3DCameraGetLookatPos(FieldCamera_GetG3DCamera(scene->camera), &target);
    matrix0->translation.x = target.x - FX32_CONST(121);
    matrix0->translation.z = target.z - FX32_CONST(200);
    matrix0->translation.y = target.y - FX32_CONST(200);
    matrix1->translation = matrix0->translation;
    return done;
}

static void ResortSceneAnim1_End(ResortFieldScene *scene) {
    ResortSceneAnim0_End(scene);
}

// Adds people to the avenue from entries, by the player's progress, the first time
static void ResortField_AddInitialEntries(ResortSys *sys, GameData *gameData, HeapID heapId) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);

    if (JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNOPENED, NULL) == 0 &&
        JoinAvenue_GetParam(info, JOIN_AVE_INFO_ENTRIES_ADDED, NULL) == 0) {
        SaveControl *save = GameData_GetSaveControl(gameData);
        JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(save);
        JoinAvenueEntry *entry;
        u32 unk;
        u8 badges;
        u8 rank;
        u32 count;
        u32 added;
        int tries;

        ResortSys_GetEntries(sys);
        entry = JoinAvenueEntry_Create(HEAPID_TAIL(heapId));
        unk = TrainerCard_CountStars(gameData);
        badges = getBadgeCount(getTrainerGameInfoAddress(save));
        rank = MedalBox_GetRank(SaveControl_GetMedalBox(save));
        count = 0;
        if (unk < NELEMS(sCountByUnk)) {
            count += sCountByUnk[unk];
        } else {
            count += 5;
        }
        if (badges < NELEMS(sCountByBadges)) {
            count += sCountByBadges[badges];
        } else {
            count += 2;
        }
        if (rank < NELEMS(sCountByRank)) {
            count += sCountByRank[rank];
        } else {
            count += 5;
        }
        added = 0;
        tries = 0;
        while (added < count) {
            u32 result;
            ResortSys_MakeShopEntry(sys, gameData, entry, 4, tries, heapId);
            result = JoinAvenue_Add(joinAvenue, gameData, entry, 2);
            if (result == 2) {
                added++;
            }
            if (result == 0 || tries++ > 100) {
                break;
            }
        }
        JoinAvenueEntry_Free(entry);
        JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_ENTRIES_ADDED, 1);
    }
}

static void ResortField_AddSpecialEntry(ResortSys *sys, GameData *gameData, HeapID heapId) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    ResortBinary *table = ResortSys_GetRanks(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(save);
    BOOL add = FALSE;

    if (EventWork_FlagGet(eventWork, 2400) &&
        JoinAvenue_GetParam(info, JOIN_AVE_INFO_SPECIAL_ENTRY_CHECKED, NULL) == 0) {
        JoinAvenueEntryList *entries = ResortSys_GetEntries(sys);
        u32 i;

        add = TRUE;
        for (i = 0; i < JoinAvenueEntryList_GetCount(entries); i++) {
            JoinAvenueEntry *entry = JoinAvenueEntryList_Get(entries, i);
            if (!JoinAvenueEntry_IsEmpty(entry) &&
                JoinAvenueEntry_GetParam(entry, JOIN_AVE_PARAM_SHOP_KIND, NULL) == 5) {
                add = FALSE;
                break;
            }
        }
        JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_SPECIAL_ENTRY_CHECKED, 1);
    }
    if (add) {
        u16 chance = ResortBinary_Get(
            table, ResortBinary_FindRange(table, JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, NULL)), 10);
        if (JoinAvenueInfo_Random(info, 21, 100) < chance) {
            JoinAvenueEntry *entry = JoinAvenueEntry_Create(HEAPID_TAIL(heapId));
            ResortSys_MakeShopEntry(sys, gameData, entry, 5, 0, heapId);
            if (JoinAvenue_Add(joinAvenue, gameData, entry, 2) == 2) {
                JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_UNK_DC_13, 0);
            }
            JoinAvenueEntry_Free(entry);
        }
    }
}

void ResortField0_Setup(Field *field) {
    GameData *gameData;
    HeapID heapId;
    ResortFieldWork0 *work;
    ResortWork *unk;
    BOOL fill;
    JoinAvenueSave *joinAvenue;
    ResortSysSetup sysSetup;
    ResortPeopleSetup peopleSetup;
    ResortNPCSetup npcSetup;

    heapId = Field_GetHeapID(field);
    gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
    unk = GameData_GetResortWork(gameData);
    fill = FALSE;
    work = Field_AllocGimmickWorkBlock(field, 0, heapId, sizeof(ResortFieldWork0));
    joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
    MI_CpuFill8(&sysSetup, 0, sizeof(ResortSysSetup));
    sysSetup.occupants = getAddressOfBeginningOfOccupants(joinAvenue);
    sysSetup.list = JoinAvenue_GetPersonList(joinAvenue);
    sysSetup.list2 = NULL;
    sysSetup.entries = JoinAvenue_GetEntryList(joinAvenue);
    sysSetup.info = JoinAvenue_GetInfo(joinAvenue);
    work->sys = ResortSys_Create(&sysSetup, unk, heapId);
    if (JoinAvenue_GetParam(ResortSys_GetInfo(work->sys), JOIN_AVE_INFO_UNOPENED, NULL) == 0) {
        fill = TRUE;
    }

    MI_CpuFill8(&peopleSetup, 0, sizeof(ResortPeopleSetup));
    peopleSetup.mmSys = GameData_GetMMSys(gameData);
    peopleSetup.slots = ResortSys_GetSlots(work->sys);
    peopleSetup.datas = ResortSys_GetDatas(work->sys);
    peopleSetup.count = 16;
    peopleSetup.zone = 2;
    peopleSetup.shops = ResortSys_GetShopData(work->sys);
    peopleSetup.unk18 = ResortSys_GetPosOffsets(work->sys);
    work->people = ResortPeople_Create(&peopleSetup, heapId);
    if (fill) {
        ResortPeople_CreateFromSlots(work->people);
    }

    MI_CpuFill8(&npcSetup, 0, sizeof(ResortNPCSetup));
    npcSetup.mmSys = GameData_GetMMSys(gameData);
    npcSetup.unk4 = 16;
    npcSetup.info = ResortSys_GetInfo(work->sys);
    npcSetup.zone = 2;
    npcSetup.table = ResortSys_GetShopOpenings(work->sys);
    work->npc = ResortNPC_Create(&npcSetup, heapId);
}

void ResortField0_Delete(Field *field) {
    ResortFieldWork0 *work = Field_GetGimmickWorkBlock(field, 0);
    ResortNPC_Free(work->npc);
    ResortPeople_Free(work->people);
    ResortSys_Free(work->sys);
    Field_DeleteGimmickWorkBlock(field, 0);
}

void ResortField0_Update(Field *field) {
    ResortFieldWork0 *work = Field_GetGimmickWorkBlock(field, 0);
    ResortNPC_Update(work->npc, field);
    ResortPeople_Update(work->people, Field_GetPlayer(field));
}

ResortPeople *ResortField0_GetPeople(Field *field) {
    ResortFieldWork0 *work = Field_GetGimmickWorkBlock(field, 0);
    return work->people;
}

ResortSys *ResortField0_GetSys(Field *field) {
    ResortFieldWork0 *work = Field_GetGimmickWorkBlock(field, 0);
    return work->sys;
}

ResortNPC *ResortField0_GetNPC(Field *field) {
    ResortFieldWork0 *work = Field_GetGimmickWorkBlock(field, 0);
    return work->npc;
}
