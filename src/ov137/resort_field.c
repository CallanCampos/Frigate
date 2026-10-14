#include "types.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
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
    void *entries;
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
    void *entries;
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
    void *shops;
    void *unkb1c;
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
    void *msgBGSys;
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

static ResortWatch *func_ov137_021eef7c(HeapID heapId, const ResortWatchSetup *setup);
static void func_ov137_021eefc4(ResortWatch *watch);
static void func_ov137_021eefd8(ResortWatch *watch);
static void func_ov137_021eeff8(ResortWatch *watch, ResortWatchCallback callback);
static void func_ov137_021eeffc(ResortWatch *watch);
static void func_ov137_021ef004(ResortWatch *watch);
static void func_ov137_021ef0dc(ResortWatch *watch);
static ResortWalkers *func_ov137_021ef124(HeapID heapId, MMSys *mmSys, FieldPlayer *player, void *shops,
                                          void *unkb1c, JoinAvenueOccupants *occupants);
static void func_ov137_021ef1b4(ResortWalkers *walkers);
static void func_ov137_021ef1e8(ResortWalkers *walkers, Field *field);
static void func_ov137_021ef234(ResortWalkers *walkers, u32 mode);
static void func_ov137_021ef250(ResortWalkers *walkers, ResortPerson *person);
static void func_ov137_021ef2bc(ResortWalkers *walkers, ResortPerson *person);
static void func_ov137_021ef2f4(ResortWalkers *walkers, ResortPerson *person);
static void func_ov137_021ef304(FieldActor *actor, u16 x, u16 z);
static ResortWalker *func_ov137_021ef340(ResortWalkers *walkers, ResortPerson *person);
static void func_ov137_021ef388(ResortWalker *walker, HeapID heapId);
static void func_ov137_021ef390(ResortWalker *walker);
static void func_ov137_021ef3b0(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static void func_ov137_021ef418(ResortWalker *walker, ResortPerson *person, void *shops, void *unkb1c,
                                JoinAvenueOccupants *occupants);
static void func_ov137_021ef470(ResortWalker *walker);
static BOOL func_ov137_021ef488(const ResortWalker *walker, ResortPerson *person);
static BOOL func_ov137_021ef498(const ResortWalker *walker);
static void func_ov137_021ef4a8(ResortWalker *walker, u16 acmd, u16 dir, u16 x);
static void func_ov137_021ef4b8(ResortWalker *walker, u16 acmd, u16 dir, u16 x, u16 z);
static BOOL func_ov137_021ef5b8(ResortWalker *walker);
static BOOL func_ov137_021ef5c8(ResortWalker *walker);
static void func_ov137_021ef5f0(ResortWalker *walker);
static BOOL func_ov137_021ef5f8(ResortWalker *walker);
static BOOL func_ov137_021ef624(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player, u32 a3, u32 a4);
static void func_ov137_021ef644(ResortWalker *walker);
static BOOL func_ov137_021ef6d4(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static BOOL func_ov137_021ef710(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static BOOL func_ov137_021ef798(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static BOOL func_ov137_021ef7bc(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static BOOL func_ov137_021ef8e4(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static void func_ov137_021ef984(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player);
static void func_ov137_021ef9a8(ResortWalker *walker, void *shops, void *unkb1c, JoinAvenueOccupants *occupants);
static BOOL func_ov137_021efa68(ResortWalker *walker, FieldPlayer *player);
static void func_ov137_021efaf0(FieldActor *actor);
static void func_ov137_021efaf4(FieldActor *actor);
static void func_ov137_021efb48(FieldActor *actor);
static void func_ov137_021efb4c(FieldActor *actor);
static void func_ov137_021efb50(ResortWalkerGroup *group, u16 x, u16 dir, HeapID heapId);
static void func_ov137_021efb78(ResortWalkerGroup *group);
static void func_ov137_021efb84(ResortWalkerGroup *group, FieldPlayer *player);
static void func_ov137_021efc78(ResortWalkerGroup *group, u32 mode);
static void func_ov137_021efd2c(ResortWalkerGroup *group, u16 slot, ResortWalker *walker);
static void func_ov137_021efd38(ResortWalkerGroup *group, u16 slot);
static BOOL func_ov137_021efd48(ResortPerson *person, u16 *group, u16 *slot);
static ResortBubbles *func_ov137_021efdac(HeapID heapId, void *msgBGSys, ResortPersonData **datas,
                                          JoinAvenueInfo *info, JoinAvenueOccupants *occupants, BOOL enabled);
static void func_ov137_021efe1c(ResortBubbles *bubbles);
static void func_ov137_021efe54(ResortBubbles *bubbles, Field *field);
static void func_ov137_021f007c(ResortBubbles *bubbles);
static void func_ov137_021f009c(ResortBubbles *bubbles);
static void func_ov137_021f00a4(ResortBubbles *bubbles);
static void func_ov137_021f00ac(ResortBubbles *bubbles, BOOL start);
static void func_ov137_021f00d8(ResortBubbles *bubbles);
static ResortPersonData *func_ov137_021f0164(ResortBubbles *bubbles);
static void func_ov137_021f022c(ResortBubble *bubble, u16 index, HeapID heapId);
static void func_ov137_021f024c(ResortBubble *bubble);
static void func_ov137_021f0264(ResortBubble *bubble, void *msgBGSys);
static BOOL func_ov137_021f0280(u16 a, u16 b, BOOL mode);
static void func_ov137_021f02f0(ResortBubble *bubble, void *msgBGSys, ResortPersonData *data, JoinAvenueInfo *info,
                                JoinAvenueOccupants *occupants, MsgData *msgData, u32 kind, BOOL mode);
static void func_ov137_021f03f8(ResortBubble *bubble, void *msgBGSys);
static void func_ov137_021f0414(ResortBubble *bubble);
static BOOL func_ov137_021f041c(ResortBubble *bubble);
static ResortFieldPeople *func_ov137_021f042c(HeapID heapId, ResortSys *sys, MMSys *mmSys, FieldPlayer *player,
                                              BOOL flag, u32 a5);
static void func_ov137_021f0508(ResortFieldPeople *people, BOOL flag, u32 a2);
static void func_ov137_021f0540(ResortFieldPeople *people);
static void func_ov137_021f0560(ResortFieldPeople *people, Field *field);
static void func_ov137_021f0584(ResortFieldPeople *people);
static ResortPeople *func_ov137_021f059c(ResortFieldPeople *people);
static void func_ov137_021f05a0(ResortFieldPeople *people, ResortPerson *person);
static ResortWalkers *func_ov137_021f05ac(ResortFieldPeople *people);
static void func_ov137_021f05b0(ResortFieldPeople *people, ResortSys *sys, ResortNPC *npc);
static void func_ov137_021f0670(ResortFieldPeople *people);
static BOOL func_ov137_021f067c(void *context, void *person, u32 a2, u32 event);
static BOOL func_ov137_021f0720(void *context, void *person, u32 a2, u32 event);
static ResortFieldScene *func_ov137_021f0758(HeapID heapId, FieldExpObjSystem *expObj, FieldCamera *camera);
static void func_ov137_021f0790(ResortFieldScene *scene);
static void func_ov137_021f07a4(ResortFieldScene *scene);
static void func_ov137_021f07d0(ResortFieldScene *scene, u32 index);
static BOOL func_ov137_021f07ec(ResortFieldScene *scene, u32 index);
static void func_ov137_021f0800(ResortFieldScene *scene, u32 index);
static void func_ov137_021f0818(ResortFieldScene *scene);
static BOOL func_ov137_021f08dc(ResortFieldScene *scene);
static void func_ov137_021f09b4(ResortFieldScene *scene);
static void func_ov137_021f0a38(ResortFieldScene *scene);
static BOOL func_ov137_021f0a80(ResortFieldScene *scene);
static void func_ov137_021f0ad8(ResortFieldScene *scene);
static void func_ov137_021f0ae0(ResortSys *sys, GameData *gameData, HeapID heapId);
static void func_ov137_021f0be4(ResortSys *sys, GameData *gameData, HeapID heapId);

// Not referenced, as reading it is folded: the angle of the scene's second animation
const u32 RESORT_FIELD_SCENE_ANGLE = 0xdd00;
static const u32 sWalkerOccupantKinds[] = {0};
static const u8 sCountByRank[] = {1, 2, 3, 4, 5};
static const u8 sCountByUnk[] = {0, 1, 2, 3, 4, 5};
static const G3DSceneAnimationSetup sSceneAnimations1[] = {{4, 0}, {5, 0}};
static const G3DSceneAnimationSetup sSceneAnimations0[] = {{1, 0}, {2, 0}};
static const u8 sCountByBadges[] = {0, 1, 1, 1, 1, 2, 2, 2, 2};
static const u16 sGroupXs[RESORT_WALKER_GROUPS] = {12, 13, 14, 16, 17, 18};
static const u16 sGroupDirs[RESORT_WALKER_GROUPS] = {0, 1, 0, 1, 0, 1};
static const ResortWalkerPlace sWalkerPlaces[] = {
    {2, 0, 0, 0}, {2, 1, 1, 0}, {2, 2, 2, 0}, {2, 3, 3, 0}, {2, 4, 4, 0}, {2, 5, 5, 0}, {2, 6, 0, 1},
    {2, 7, 1, 1}, {3, 0, 2, 1}, {3, 1, 3, 1}, {3, 2, 4, 1}, {3, 3, 5, 1}, {3, 4, 0, 2}, {3, 5, 1, 2},
    {3, 6, 2, 2}, {3, 7, 3, 2}, {4, 0, 4, 2}, {4, 1, 5, 2}, {4, 2, 0, 3}, {4, 3, 1, 3}, {4, 4, 2, 3},
    {4, 5, 3, 3}, {4, 6, 4, 3}, {4, 7, 5, 3}, {4, 8, 0, 4}, {4, 9, 1, 4}, {4, 10, 2, 4}, {4, 11, 3, 4},
};
static const u8 sBubbleYs[RESORT_BUBBLES] = {6, 2, 19, 14, 17, 10, 14, 2, 10, 17, 10, 6, 2, 6, 14, 19};
static const u8 sBubbleXs1[RESORT_BUBBLES] = {1, 16, 2, 15, 4, 16, 3, 13, 1, 13, 2, 16, 3, 15, 4, 14};
static const u8 sBubbleXs0[RESORT_BUBBLES] = {1, 18, 2, 16, 4, 18, 3, 15, 1, 15, 2, 18, 3, 17, 4, 16};
static const u32 sBubbleIntervalKinds[] = {0, 2, 3, 4};
static const u32 sBubbleKinds[] = {0, 2, 3, 4};
static BOOL (*const sWalkerStates[])(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) = {
    func_ov137_021ef6d4, func_ov137_021ef710, func_ov137_021ef798, func_ov137_021ef7bc, func_ov137_021ef8e4,
};
static const FieldActorMoveCode sWalkerMoveCode = {
    0, func_ov137_021efaf0, func_ov137_021efaf4, func_ov137_021efb48, func_ov137_021efb4c,
};
static const ResortSceneAnim sSceneAnims[] = {
    {func_ov137_021f0818, func_ov137_021f08dc, func_ov137_021f09b4},
    {func_ov137_021f0a38, func_ov137_021f0a80, func_ov137_021f0ad8},
};
static const G3DSceneActorSetup sSceneActors[] = {
    {0, 0, 0, 0, sSceneAnimations0, 2},
    {3, 0, 3, 0, sSceneAnimations1, 2},
};
static const G3DSceneResourceSetup sSceneResources[] = {
    {298, 1, 0}, {298, 0, 0}, {298, 2, 0}, {298, 4, 0}, {298, 3, 0}, {298, 5, 0},
};
static const G3DSceneSetup sScene = {sSceneResources, 6, sSceneActors, 2};

void func_ov137_021eec80(Field *field) {
    void *block;
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
    void *msgBGSys;
    ResortPersonData **datas;
    JoinAvenueInfo *info;
    JoinAvenueOccupants *occupants;
    FieldExpObjSystem *expObj;
    BOOL bubblesEnabled;
    u32 i;
    void *entries;

    heapId = Field_GetHeapID(field);
    gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    unk = func_02017b84(gameData);
    work = Field_AllocGimmickWorkBlock(field, 1, heapId, sizeof(ResortFieldWork));
    bubblesEnabled = TRUE;
    joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
    list2 = GameData_GetJoinAvenuePersonListPtr(gameData);
    sys_memset(&sysSetup, 0, sizeof(ResortSysSetup));
    sysSetup.occupants = getAddressOfBeginningOfOccupants(joinAvenue);
    sysSetup.list = JoinAvenue_GetPersonList(joinAvenue);
    sysSetup.list2 = *list2;
    sysSetup.entries = func_02010054(joinAvenue);
    sysSetup.info = JoinAvenue_GetInfo(joinAvenue);
    work->sys = ResortSys_Create(&sysSetup, unk, heapId);
    flag = JoinAvenue_GetParam(ResortSys_GetInfo(work->sys), 7, NULL);
    func_ov137_021f0be4(work->sys, gameData, heapId);
    func_ov137_021f0ae0(work->sys, gameData, heapId);
    unk2 = ResortWork_Get(unk, 8);
    mmSys = GameData_GetMMSys(gameData);
    work->people = func_ov137_021f042c(heapId, work->sys, mmSys, Field_GetPlayer(field), flag, unk2);
    func_ov137_021f0508(work->people, flag, unk2);

    sys_memset(&npcSetup, 0, sizeof(ResortNPCSetup));
    npcSetup.mmSys = GameData_GetMMSys(gameData);
    npcSetup.unk4 = 16;
    npcSetup.info = ResortSys_GetInfo(work->sys);
    npcSetup.zone = func_02039518(Field_GetPlayerStateZoneID(field));
    npcSetup.table = ResortSys_GetShopOpenings(work->sys);
    work->npc = func_ov137_021f1c24(&npcSetup, heapId);

    msgBGSys = Field_GetMsgBGSys(field);
    datas = ResortSys_GetDatas(work->sys);
    info = ResortSys_GetInfo(work->sys);
    occupants = ResortSys_GetOccupants(work->sys);
    if (flag) {
        bubblesEnabled = FALSE;
    }
    work->bubbles = func_ov137_021efdac(heapId, msgBGSys, datas, info, occupants, bubblesEnabled);
    expObj = Field_GetExpObjSystem(field);
    work->scene = func_ov137_021f0758(heapId, expObj, Field_GetCameraSystem(field));

    block = getHollow_RivalBlk(GameData_GetSaveControl(gameData));
    entries = ResortSys_GetEntries(work->sys);
    for (i = 0; i < func_02037ed4(entries); i++) {
        void *entry = func_02037f04(entries, i);
        if (!func_02037a90(entry) && (u16)func_02037b38(entry, 3, NULL) == 223) {
            func_0200ff50(block, 2, 15);
            return;
        }
    }
}

void func_ov137_021eee4c(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    func_ov137_021f0790(work->scene);
    func_ov137_021efe1c(work->bubbles);
    func_ov137_021f1c74(work->npc);
    func_ov137_021f0540(work->people);
    ResortSys_Free(work->sys);
    Field_DeleteGimmickWorkBlock(field, 1);
}

void func_ov137_021eee80(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    func_ov137_021f0560(work->people, field);
    func_ov137_021f1c88(work->npc, field);
    func_ov137_021efe54(work->bubbles, field);
    func_ov137_021f07a4(work->scene);
}

ResortPeople *func_ov137_021eeeac(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    return func_ov137_021f059c(work->people);
}

ResortNPC *func_ov137_021eeebc(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    return work->npc;
}

ResortSys *func_ov137_021eeec8(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    return work->sys;
}

void func_ov137_021eeed4(Field *field, ResortPerson *person) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    func_ov137_021f05a0(work->people, person);
}

void func_ov137_021eeee8(Field *field, u32 index) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    func_ov137_021f07d0(work->scene, index);
}

BOOL func_ov137_021eeefc(Field *field, u32 index) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    return func_ov137_021f07ec(work->scene, index);
}

void func_ov137_021eef10(Field *field, u32 index) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    func_ov137_021f0800(work->scene, index);
}

void func_ov137_021eef24(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    func_ov137_021f0584(work->people);
    func_ov137_021f009c(work->bubbles);
}

void func_ov137_021eef3c(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    func_ov137_021f0670(work->people);
}

void func_ov137_021eef4c(Field *field) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    func_ov137_021f00a4(work->bubbles);
    func_ov137_021f05b0(work->people, work->sys, work->npc);
}

void func_ov137_021eef68(Field *field, BOOL start) {
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    func_ov137_021f00ac(work->bubbles, start);
}

static ResortWatch *func_ov137_021eef7c(HeapID heapId, const ResortWatchSetup *setup) {
    ResortWatch *watch = GFL_HeapAllocate(heapId, sizeof(ResortWatch), TRUE, "resort_field.c", 659);
    watch->setup = *setup;
    watch->list = setup->list;
    watch->entries = setup->entries;
    watch->list2 = setup->list2;
    GameBeaconSys_SetAvenuePeople(watch->list2);
    func_ov137_021eeffc(watch);
    return watch;
}

static void func_ov137_021eefc4(ResortWatch *watch) {
    GameBeaconSys_SetAvenuePeople(NULL);
    GFL_HeapFree(watch);
}

static void func_ov137_021eefd8(ResortWatch *watch) {
    if (watch->timer++ > watch->setup.interval) {
        func_ov137_021ef004(watch);
        func_ov137_021ef0dc(watch);
        watch->timer = 0;
    }
}

static void func_ov137_021eeff8(ResortWatch *watch, ResortWatchCallback callback) {
    watch->setup.callback = callback;
}

static void func_ov137_021eeffc(ResortWatch *watch) {
    watch->changed = TRUE;
}

static void func_ov137_021ef004(ResortWatch *watch) {
    u32 i;
    u32 j;

    for (i = 0; i < JoinAvenuePersonList_GetCount(watch->list); i++) {
        JoinAvenuePerson *person = JoinAvenuePersonList_Get(watch->list, i);
        u32 value = joinAveTextHandler(person, 28, NULL);
        if (!JoinAvenuePerson_IsEmpty(person) && value != 0) {
            JoinAvenuePerson_SetParam(person, 28, 0);
            if (!watch->setup.callback(watch->setup.context, person, 2, value)) {
                JoinAvenuePerson_SetParam(person, 28, value);
            }
        }
    }
    for (j = 0; j < JoinAvenuePersonList_GetCount(watch->list2); j++) {
        JoinAvenuePerson *person = JoinAvenuePersonList_Get(watch->list2, j);
        u32 value = joinAveTextHandler(person, 28, NULL);
        if (!JoinAvenuePerson_IsEmpty(person) && value != 0) {
            JoinAvenuePerson_SetParam(person, 28, 0);
            if (!watch->setup.callback(watch->setup.context, person, 2, value)) {
                JoinAvenuePerson_SetParam(person, 28, value);
            }
        }
    }
}

static void func_ov137_021ef0dc(ResortWatch *watch) {
    if (watch->changed) {
        u32 i;
        for (i = 0; i < func_02037ed4(watch->entries); i++) {
            void *entry = func_02037f04(watch->entries, i);
            if (!func_02037a90(entry)) {
                watch->setup.callback(watch->setup.context, entry, 1, 2);
            }
        }
        watch->changed = FALSE;
    }
}

static ResortWalkers *func_ov137_021ef124(HeapID heapId, MMSys *mmSys, FieldPlayer *player, void *shops,
                                          void *unkb1c, JoinAvenueOccupants *occupants) {
    int i;
    ResortWalkers *walkers = GFL_HeapAllocate(heapId, sizeof(ResortWalkers), TRUE, "resort_field.c", 950);

    walkers->player = player;
    walkers->mmSys = mmSys;
    walkers->shops = shops;
    walkers->unkb1c = unkb1c;
    walkers->occupants = occupants;
    for (i = 0; i < RESORT_WALKERS; i++) {
        func_ov137_021ef388(&walkers->walkers[i], heapId);
    }
    for (i = 0; i < RESORT_WALKER_GROUPS; i++) {
        func_ov137_021efb50(&walkers->groups[i], sGroupXs[i], sGroupDirs[i], heapId);
    }
    return walkers;
}

static void func_ov137_021ef1b4(ResortWalkers *walkers) {
    int i;
    for (i = 0; i < RESORT_WALKER_GROUPS; i++) {
        func_ov137_021efb78(&walkers->groups[i]);
    }
    for (i = 0; i < RESORT_WALKERS; i++) {
        func_ov137_021ef390(&walkers->walkers[i]);
    }
    GFL_HeapFree(walkers);
}

static void func_ov137_021ef1e8(ResortWalkers *walkers, Field *field) {
    int i;
    if (!Field_IsEventRunning(field)) {
        for (i = 0; i < RESORT_WALKER_GROUPS; i++) {
            func_ov137_021efb84(&walkers->groups[i], walkers->player);
        }
    }
    for (i = 0; i < RESORT_WALKERS; i++) {
        func_ov137_021ef3b0(&walkers->walkers[i], walkers->mmSys, walkers->player);
    }
}

static void func_ov137_021ef234(ResortWalkers *walkers, u32 mode) {
    int i;
    for (i = 0; i < RESORT_WALKER_GROUPS; i++) {
        func_ov137_021efc78(&walkers->groups[i], mode);
    }
}

static void func_ov137_021ef250(ResortWalkers *walkers, ResortPerson *person) {
    u16 group;
    u16 slot;
    int i;

    if (func_ov137_021efd48(person, &group, &slot)) {
        for (i = 0; i < RESORT_WALKERS; i++) {
            if (!func_ov137_021ef498(&walkers->walkers[i])) {
                func_ov137_021ef418(&walkers->walkers[i], person, walkers->shops, walkers->unkb1c,
                                    walkers->occupants);
                func_ov137_021efd2c(&walkers->groups[group], slot, &walkers->walkers[i]);
                return;
            }
        }
    }
}

static void func_ov137_021ef2bc(ResortWalkers *walkers, ResortPerson *person) {
    u16 group;
    u16 slot;

    if (func_ov137_021efd48(person, &group, &slot)) {
        ResortWalker *walker = func_ov137_021ef340(walkers, person);
        if (walker != NULL) {
            func_ov137_021efd38(&walkers->groups[group], slot);
            func_ov137_021ef390(walker);
        }
    }
}

static void func_ov137_021ef2f4(ResortWalkers *walkers, ResortPerson *person) {
    ResortWalker *walker = func_ov137_021ef340(walkers, person);
    if (walker != NULL) {
        func_ov137_021ef5f0(walker);
    }
}

static void func_ov137_021ef304(FieldActor *actor, u16 x, u16 z) {
    VecFx32 pos;
    pos.x = (x << 16) + 0x8000;
    pos.y = 0;
    pos.z = (z << 16) + 0x8000;
    SetActorWPosValue(actor, &pos);
    SetActorGPosX(actor, x);
    SetActorGPosZ(actor, z);
}

static ResortWalker *func_ov137_021ef340(ResortWalkers *walkers, ResortPerson *person) {
    u16 group;
    u16 slot;
    int i;

    if (func_ov137_021efd48(person, &group, &slot)) {
        for (i = 0; i < RESORT_WALKERS; i++) {
            if (func_ov137_021ef488(&walkers->walkers[i], person)) {
                return &walkers->walkers[i];
            }
        }
    }
    return NULL;
}

static void func_ov137_021ef388(ResortWalker *walker, HeapID heapId) {
    func_ov137_021ef470(walker);
}

static void func_ov137_021ef390(ResortWalker *walker) {
    if (func_ov137_021ef498(walker)) {
        ChangeActorMoveCodeSeq(func_ov137_021f110c(walker->person), 0);
    }
    func_ov137_021ef470(walker);
}

static void func_ov137_021ef3b0(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    if (func_ov137_021ef498(walker)) {
        FieldActor *actor = func_ov137_021f110c(walker->person);
        if (walker->unk14 && !walker->unk15) {
            switch (walker->unk54) {
            case 0:
                func_ov012_02166eb0(actor, 0xbe);
                walker->unk54++;
            case 1:
                if (func_ov036_0218f01c(actor)) {
                    walker->unk54 = 0;
                    walker->unk14 = FALSE;
                    walker->unk15 = TRUE;
                }
                break;
            }
        }
        if (walker->objCodeChanged && func_ov137_021ef5b8(walker)) {
            func_ov137_021f0f64(walker->person);
            walker->objCodeChanged = FALSE;
        }
    }
}

static void func_ov137_021ef418(ResortWalker *walker, ResortPerson *person, void *shops, void *unkb1c,
                                JoinAvenueOccupants *occupants) {
    func_ov137_021ef470(walker);
    walker->person = person;
    func_ov012_021682e8(func_ov137_021f110c(person), &sWalkerMoveCode);
    func_ov137_021f1070(walker->person, FALSE);
    func_ov137_021ef9a8(walker, shops, unkb1c, occupants);
    switch (func_ov137_021f10f4(walker->person, 0)) {
    case 1:
    case 2:
        walker->start = func_ov137_021ef644;
        walker->update = func_ov137_021ef984;
        break;
    }
}

static void func_ov137_021ef470(ResortWalker *walker) {
    sys_memset(walker, 0, sizeof(ResortWalker));
    walker->magic = 0x765f573;
}

static BOOL func_ov137_021ef488(const ResortWalker *walker, ResortPerson *person) {
    if (person == walker->person) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov137_021ef498(const ResortWalker *walker) {
    if (walker->person != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov137_021ef4a8(ResortWalker *walker, u16 acmd, u16 dir, u16 x) {
    func_ov137_021ef4b8(walker, acmd, dir, x, 0);
}

static void func_ov137_021ef4b8(ResortWalker *walker, u16 acmd, u16 dir, u16 x, u16 z) {
    FieldActor *actor;

    walker->state = 0;
    walker->active = TRUE;
    walker->acmd = acmd;
    walker->dir = dir;
    actor = func_ov137_021f110c(walker->person);
    if (func_ov137_021ef5c8(walker)) {
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
        func_ov137_021ef304(actor, x, z);
    } else {
        s16 actorZ = GetGPosZ(actor);
        if (walker->dir == DIR_UP) {
            if (actorZ < 9) {
                func_ov137_021ef304(actor, x, 77);
                walker->steps = 69;
                if (walker->steps <= 0) {
                    walker->steps = 1;
                }
            } else {
                walker->steps = actorZ - 8;
                if (walker->steps <= 0) {
                    walker->steps = 1;
                }
                func_ov137_021ef304(actor, x, actorZ);
            }
        } else if (walker->dir == DIR_DOWN) {
            if (actorZ > 77) {
                func_ov137_021ef304(actor, x, 9);
                walker->steps = 69;
                if (walker->steps <= 0) {
                    walker->steps = 1;
                }
            } else {
                walker->steps = 78 - actorZ;
                if (walker->steps <= 0) {
                    walker->steps = 1;
                }
                func_ov137_021ef304(actor, x, actorZ);
            }
        }
    }
    func_ov137_021f1070(walker->person, TRUE);
    func_ov137_021f10dc(walker->person, 38, 1);
    if (walker->start != NULL) {
        walker->start(walker);
    }
}

static BOOL func_ov137_021ef5b8(ResortWalker *walker) {
    if (walker->active == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov137_021ef5c8(ResortWalker *walker) {
    FieldActor *actor = func_ov137_021f110c(walker->person);
    s16 x = GetGPosX(actor);
    s16 z = GetGPosZ(actor);
    if (x == 0 && z == 0) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov137_021ef5f0(ResortWalker *walker) {
    walker->objCodeChanged = TRUE;
}

static BOOL func_ov137_021ef5f8(ResortWalker *walker) {
    u32 type = func_ov137_021f10f4(walker->person, 0);
    u32 value = func_ov137_021f10e8(walker->person, 31, NULL);
    if (type == 1 && (value == 2 || value == 3)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov137_021ef624(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player, u32 a3, u32 a4) {
    if (ActorRouteCollCheckOneTileInDir(func_ov137_021f110c(walker->person), walker->dir) & 4) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov137_021ef644(ResortWalker *walker) {
    s16 stepSpeed;
    u16 speed = func_ov137_021f10e8(walker->person, 34, NULL);

    if (func_ov137_021f10f4(walker->person, 0) == 1) {
        u32 value = func_ov137_021f10e8(walker->person, 31, NULL);
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

static BOOL func_ov137_021ef6d4(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    if (walker->steps <= 0) {
        walker->active = FALSE;
        walker->state = 0;
        func_ov137_021f1070(walker->person, FALSE);
        func_ov137_021ef304(func_ov137_021f110c(walker->person), 0, 0);
        func_ov137_021f10dc(walker->person, 38, 0);
        return FALSE;
    }
    walker->state = 1;
    return TRUE;
}

static BOOL func_ov137_021ef710(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    FieldActor *actor = func_ov137_021f110c(walker->person);

    if (func_ov137_021ef624(walker, mmSys, player, 1, 0)) {
        func_ov012_02166eb0(actor, 0x3f);
        func_ov012_021674b0(actor);
        walker->state = 2;
    } else if (func_ov137_021efa68(walker, player)) {
        walker->state = 4;
    } else if (walker->unk26 == walker->steps) {
        walker->lookState = 0;
        walker->state = 3;
    } else {
        func_ov012_02166eb0(actor, GetAcmdForDir(walker->dir, walker->acmd));
        SetActorMotionDir(actor, walker->dir);
        func_ov012_021674b0(actor);
        walker->steps--;
        walker->state = 2;
    }
    return TRUE;
}

static BOOL func_ov137_021ef798(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    FieldActor *actor = func_ov137_021f110c(walker->person);
    if (func_ov036_0218f01c(actor)) {
        func_ov012_021674bc(actor);
        walker->state = 0;
    }
    return FALSE;
}

static BOOL func_ov137_021ef7bc(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    FieldActor *actor = func_ov137_021f110c(walker->person);

    switch (walker->lookState) {
    case 0:
        walker->lookDir = GFL_RandomMTRange(2) ? DIR_LEFT : DIR_RIGHT;
        func_ov012_02166eb0(actor, walker->lookDir);
        walker->lookState++;
    case 1:
        if (func_ov036_0218f01c(actor)) {
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
        func_ov012_02166eb0(actor, walker->lookDir);
        walker->lookState++;
    case 4:
        if (func_ov036_0218f01c(actor)) {
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

static BOOL func_ov137_021ef8e4(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    FieldActor *actor = func_ov137_021f110c(walker->person);

    switch (walker->stopState) {
    case 0:
        func_ov012_021674bc(actor);
        func_ov012_021674e8(actor);
        func_ov012_02166eb0(actor, walker->stopAcmd[walker->stop]);
        walker->stopState++;
    case 1:
        if (func_ov036_0218f01c(actor)) {
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

static void func_ov137_021ef984(ResortWalker *walker, MMSys *mmSys, FieldPlayer *player) {
    while (sWalkerStates[walker->state](walker, mmSys, player) == TRUE) {
    }
}

static void func_ov137_021ef9a8(ResortWalker *walker, void *shops, void *unkb1c, JoinAvenueOccupants *occupants) {
    int i;
    u16 id = func_ov137_021f10e8(walker->person, 19, NULL);
    u16 shop = ResortShopData_GetShopParam(ResortShopData_GetShop(shops, id), 0);

    GetGPosZ(func_ov137_021f110c(walker->person));
    for (i = 0; i < RESORT_WALKER_STOPS; i++) {
        JoinAvenuePerson *occupant = func_02038860(occupants, i);
        if (!JoinAvenuePerson_IsEmpty(occupant) && shop == ResortShopData_GetShopParam(ResortShopData_GetPersonShop(shops, occupant), 0)) {
            u16 z = ResortBinary_Get(unkb1c, i, 2);
            walker->stopZ[walker->stopCount] = z + GFL_RandomMTRange(7);
            walker->stopAcmd[walker->stopCount] = i % 2 == 0 ? DIR_RIGHT : DIR_LEFT;
            walker->stopCount++;
        }
    }
    walker->stop = walker->stopCount;
}

static BOOL func_ov137_021efa68(ResortWalker *walker, FieldPlayer *player) {
    s16 x;
    s16 y;
    s16 z;
    int i;
    FieldActor *actor = func_ov137_021f110c(walker->person);
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

static void func_ov137_021efaf0(FieldActor *actor) {
}

static void func_ov137_021efaf4(FieldActor *actor) {
    MMSys *mmSys = GetActorMModelSystem(actor);
    Field *field = GetMMSysField(mmSys);
    ResortFieldWork *work = Field_GetGimmickWorkBlock(field, 1);
    ResortWalkers *walkers = func_ov137_021f05ac(work->people);
    ResortWalker *walker = func_ov137_021ef340(walkers, func_ov137_021f15cc(func_ov137_021f059c(work->people), actor));

    if (walker->update != NULL && walker->active) {
        walker->update(walker, mmSys, Field_GetPlayer(field));
    }
}

static void func_ov137_021efb48(FieldActor *actor) {
}

static void func_ov137_021efb4c(FieldActor *actor) {
}

static void func_ov137_021efb50(ResortWalkerGroup *group, u16 x, u16 dir, HeapID heapId) {
    sys_memset(group, 0, sizeof(ResortWalkerGroup));
    group->x = x;
    group->dir = dir;
    group->timer = GFL_RandomMTRange(239) + 1;
}

static void func_ov137_021efb78(ResortWalkerGroup *group) {
    sys_memset(group, 0, sizeof(ResortWalkerGroup));
}

static void func_ov137_021efb84(ResortWalkerGroup *group, FieldPlayer *player) {
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
        if (group->walkers[i] != NULL && !func_ov137_021ef5b8(group->walkers[i])) {
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
    if (func_ov137_021ef5f8(group->walkers[group->next])) {
        group->timer = 0;
    }
    if (group->timer-- <= 0) {
        if (func_ov137_021ef5b8(group->walkers[group->next])) {
            func_ov137_021ef4a8(group->walkers[group->next], 12, group->dir, group->x);
            group->timer = GFL_RandomMTRange(239) + 1;
        }
        group->next++;
        group->next %= RESORT_WALKER_GROUP_SIZE;
    }
}

static void func_ov137_021efc78(ResortWalkerGroup *group, u32 mode) {
    int i;
    for (i = 0; i < RESORT_WALKER_GROUP_SIZE; i++) {
        if (group->walkers[i] != NULL && func_ov137_021ef498(group->walkers[i])) {
            if (mode == 0) {
                if (func_ov137_021ef5c8(group->walkers[i])) {
                    u16 z;
                    if (group->dir == DIR_UP) {
                        z = (GFL_RandomMTRange(8) + 8) * i;
                    } else {
                        z = (GFL_RandomMTRange(8) + 8) * (4 - i);
                    }
                    func_ov137_021ef4b8(group->walkers[i], 12, group->dir, group->x, z);
                    group->next = i + 1;
                    group->next %= RESORT_WALKER_GROUP_SIZE;
                }
            } else if (mode == 1) {
                if (!func_ov137_021ef5c8(group->walkers[i])) {
                    func_ov137_021ef4a8(group->walkers[i], 12, group->dir, group->x);
                    group->timer = GFL_RandomMTRange(239) + 1;
                    group->next = i + 1;
                    group->next %= RESORT_WALKER_GROUP_SIZE;
                }
            }
        }
    }
}

static void func_ov137_021efd2c(ResortWalkerGroup *group, u16 slot, ResortWalker *walker) {
    group->walkers[slot] = walker;
    group->count++;
}

static void func_ov137_021efd38(ResortWalkerGroup *group, u16 slot) {
    group->count--;
    group->walkers[slot] = NULL;
}

static BOOL func_ov137_021efd48(ResortPerson *person, u16 *group, u16 *slot) {
    u32 i;
    ResortPersonData *data = func_ov137_021f1110(person);
    u32 kind = func_ov137_021f1988(data);
    u16 index = func_ov137_021f198c(data);

    for (i = 0; i < NELEMS(sWalkerPlaces); i++) {
        if (kind == sWalkerPlaces[i].kind && index == sWalkerPlaces[i].index) {
            *group = sWalkerPlaces[i].group;
            *slot = sWalkerPlaces[i].slot;
            return TRUE;
        }
    }
    return FALSE;
}

static ResortBubbles *func_ov137_021efdac(HeapID heapId, void *msgBGSys, ResortPersonData **datas,
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
        func_ov137_021f022c(&bubbles->bubbles[i], i, heapId);
    }
    func_ov137_021f00d8(bubbles);
    return bubbles;
}

static void func_ov137_021efe1c(ResortBubbles *bubbles) {
    int i;
    for (i = 0; i < RESORT_BUBBLES; i++) {
        func_ov137_021f03f8(&bubbles->bubbles[i], bubbles->msgBGSys);
        func_ov137_021f024c(&bubbles->bubbles[i]);
    }
    GFL_MsgDataFree(bubbles->msgData);
    GFL_HeapFree(bubbles);
}

static void func_ov137_021efe54(ResortBubbles *bubbles, Field *field) {
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
            func_ov137_021f00d8(bubbles);
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
            data = func_ov137_021f0164(bubbles);
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
                } while (func_ov137_021f0280(bubble, bubbles->bubbleIndexes[0], bubbles->mode));
                break;
            case 2:
                do {
                    bubble = GFL_RandomMTRange(RESORT_BUBBLES);
                    if (tries++ >= 30) {
                        bubbles->count--;
                        bubbles->state = 2;
                        return;
                    }
                } while (func_ov137_021f0280(bubble, bubbles->bubbleIndexes[0], bubbles->mode) ||
                         func_ov137_021f0280(bubble, bubbles->bubbleIndexes[1], bubbles->mode));
                break;
            }
            bubbles->bubbleIndexes[bubbles->shown] = bubble;
            if (bubbles->mode == TRUE) {
                bubbles->unk10 = GFL_RandomMTRange(4) + 4;
            }
            func_ov137_021f02f0(&bubbles->bubbles[bubble], bubbles->msgBGSys, data, bubbles->info, bubbles->occupants,
                                bubbles->msgData, bubbles->unk10, bubbles->mode);
            if (bubbles->shown++ >= bubbles->count) {
                bubbles->state = 2;
            }
            break;
        }
        case 2: {
            BOOL done = TRUE;
            for (i = 0; i < bubbles->count; i++) {
                done &= func_ov137_021f041c(&bubbles->bubbles[bubbles->bubbleIndexes[i]]);
            }
            if (done) {
                bubbles->state = 0;
                bubbles->active = FALSE;
            }
            break;
        }
        }
        for (i = 0; i < RESORT_BUBBLES; i++) {
            func_ov137_021f0264(&bubbles->bubbles[i], bubbles->msgBGSys);
        }
    } else if (clear) {
        func_ov137_021f007c(bubbles);
    }
}

static void func_ov137_021f007c(ResortBubbles *bubbles) {
    int i;
    for (i = 0; i < RESORT_BUBBLES; i++) {
        func_ov137_021f03f8(&bubbles->bubbles[i], bubbles->msgBGSys);
    }
}

static void func_ov137_021f009c(ResortBubbles *bubbles) {
    bubbles->enabled = TRUE;
}

static void func_ov137_021f00a4(ResortBubbles *bubbles) {
    bubbles->mode = TRUE;
}

static void func_ov137_021f00ac(ResortBubbles *bubbles, BOOL start) {
    if (bubbles->mode == TRUE) {
        if (start) {
            bubbles->state = 0;
            bubbles->timer = bubbles->interval;
            bubbles->active = TRUE;
        } else {
            func_ov137_021f007c(bubbles);
            bubbles->active = FALSE;
        }
    }
}

// The more people there are, the more often the bubbles show
static void func_ov137_021f00d8(ResortBubbles *bubbles) {
    u32 count = 0;
    ResortDataIter iter = func_ov137_021f1ae0(bubbles->datas, sBubbleIntervalKinds, 4);
    ResortPersonData *data;

    while (TRUE) {
        data = func_ov137_021f1b0c(bubbles->datas, &iter, sBubbleIntervalKinds, 4);
        if (data == NULL) {
            break;
        }
        if (!func_ov137_021f195c(data)) {
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
static ResortPersonData *func_ov137_021f0164(ResortBubbles *bubbles) {
    int count = 0;
    ResortDataIter iter = func_ov137_021f1ae0(bubbles->datas, sBubbleKinds, 4);
    ResortPersonData *data;

    while (TRUE) {
        data = func_ov137_021f1b0c(bubbles->datas, &iter, sBubbleKinds, 4);
        if (data == NULL) {
            break;
        }
        if (func_ov137_021f195c(data)) {
            continue;
        }
        sDataKinds[count] = func_ov137_021f1988(data);
        sDataIndexes[count] = func_ov137_021f198c(data);
        count++;
    }
    if (count == 0) {
        return NULL;
    }
    if (count == 1) {
        return func_ov137_021f1b94(bubbles->datas, sDataKinds[0], sDataIndexes[0]);
    }
    {
        int n = 0;
        u32 chosen = GFL_RandomMTRange(count);
        int i;

        for (i = 0; i < count; i++) {
            if (n++ == chosen) {
                return func_ov137_021f1b94(bubbles->datas, sDataKinds[i], sDataIndexes[i]);
            }
        }
    }
    return NULL;
}

static void func_ov137_021f022c(ResortBubble *bubble, u16 index, HeapID heapId) {
    sys_memset(bubble, 0, sizeof(ResortBubble));
    bubble->strbuf = GFL_StrBufCreate(33, heapId);
    bubble->index = index;
}

static void func_ov137_021f024c(ResortBubble *bubble) {
    GFL_StrBufFree(bubble->strbuf);
    sys_memset(bubble, 0, sizeof(ResortBubble));
}

static void func_ov137_021f0264(ResortBubble *bubble, void *msgBGSys) {
    if (bubble->active) {
        bubble->timer++;
        if (bubble->timer >= 60) {
            func_ov137_021f03f8(bubble, msgBGSys);
        }
    }
}

// Whether the bubbles at two places overlap
static BOOL func_ov137_021f0280(u16 a, u16 b, BOOL mode) {
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

static void func_ov137_021f02f0(ResortBubble *bubble, void *msgBGSys, ResortPersonData *data, JoinAvenueInfo *info,
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
        func_ov137_021f1968(data, 5, name);
        GFL_StrBufLoadString(bubble->strbuf, name);
        break;
    }
    case 1:
    case 3:
    default: {
        u16 text[9];
        func_ov137_021f1968(data, 88, text);
        GFL_StrBufLoadString(bubble->strbuf, text);
        style = 1;
        break;
    }
    case 2: {
        u16 text[9];
        func_ov137_021f1968(data, 89, text);
        GFL_StrBufLoadString(bubble->strbuf, text);
        style = 2;
        break;
    }
    case 4: {
        u16 text[9];
        joinAveTextHandler(func_02038a18(occupants), 88, text);
        GFL_StrBufLoadString(bubble->strbuf, text);
        style = 1;
        break;
    }
    case 5: {
        u16 text[9];
        joinAveTextHandler(func_02038a18(occupants), 89, text);
        GFL_StrBufLoadString(bubble->strbuf, text);
        style = 1;
        break;
    }
    case 6: {
        u16 name[11];
        JoinAvenue_GetParam(info, 1, name);
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
    func_ov036_02188dfc(msgBGSys, bubble->strbuf, bubble->index, xs[bubble->index], sBubbleYs[bubble->index], width, 2,
                        style);
}

static void func_ov137_021f03f8(ResortBubble *bubble, void *msgBGSys) {
    if (bubble->active) {
        func_ov036_02188e90(msgBGSys, bubble->index);
        func_ov137_021f0414(bubble);
    }
}

static void func_ov137_021f0414(ResortBubble *bubble) {
    bubble->timer = 0;
    bubble->active = FALSE;
}

static BOOL func_ov137_021f041c(ResortBubble *bubble) {
    if (bubble->active == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static ResortFieldPeople *func_ov137_021f042c(HeapID heapId, ResortSys *sys, MMSys *mmSys, FieldPlayer *player,
                                              BOOL flag, u32 a5) {
    ResortWatchSetup watchSetup;
    ResortPeopleSetup peopleSetup;
    ResortFieldPeople *people = GFL_HeapAllocate(heapId, sizeof(ResortFieldPeople), TRUE, "resort_field.c", 3601);

    people->sys = sys;
    sys_memset(&watchSetup, 0, sizeof(ResortWatchSetup));
    watchSetup.list = ResortSys_GetList(sys);
    watchSetup.list2 = ResortSys_GetList2(sys);
    watchSetup.entries = ResortSys_GetEntries(sys);
    watchSetup.context = people;
    watchSetup.callback = flag ? func_ov137_021f0720 : func_ov137_021f067c;
    watchSetup.interval = 0;
    people->watch = func_ov137_021eef7c(heapId, &watchSetup);

    sys_memset(&peopleSetup, 0, sizeof(ResortPeopleSetup));
    peopleSetup.mmSys = mmSys;
    peopleSetup.slots = ResortSys_GetSlots(sys);
    peopleSetup.datas = ResortSys_GetDatas(sys);
    peopleSetup.count = 64;
    peopleSetup.zone = 1;
    peopleSetup.shops = ResortSys_GetShopData(sys);
    peopleSetup.unk18 = ResortSys_GetUnk20Table(sys);
    people->people = func_ov137_021f12b4(&peopleSetup, heapId);
    people->walkers = func_ov137_021ef124(heapId, mmSys, player, ResortSys_GetShopData(sys), ResortSys_GetUnk20Table(sys),
                                          ResortSys_GetOccupants(sys));
    return people;
}

static void func_ov137_021f0508(ResortFieldPeople *people, BOOL flag, u32 a2) {
    int i;

    if (flag) {
        func_ov137_021f13a4(people->people);
    } else {
        func_ov137_021f1350(people->people);
    }
    for (i = 0; i < 64; i++) {
        ResortPerson *person = func_ov137_021f1634(people->people, i);
        if (person != NULL) {
            func_ov137_021ef250(people->walkers, person);
        }
    }
    func_ov137_021ef234(people->walkers, 1);
}

static void func_ov137_021f0540(ResortFieldPeople *people) {
    func_ov137_021ef1b4(people->walkers);
    func_ov137_021f1300(people->people);
    func_ov137_021eefc4(people->watch);
    GFL_HeapFree(people);
}

static void func_ov137_021f0560(ResortFieldPeople *people, Field *field) {
    func_ov137_021eefd8(people->watch);
    func_ov137_021f1348(people->people, Field_GetPlayer(field));
    func_ov137_021ef1e8(people->walkers, field);
}

static void func_ov137_021f0584(ResortFieldPeople *people) {
    func_ov137_021eeffc(people->watch);
    func_ov137_021eeff8(people->watch, func_ov137_021f067c);
}

static ResortPeople *func_ov137_021f059c(ResortFieldPeople *people) {
    return people->people;
}

static void func_ov137_021f05a0(ResortFieldPeople *people, ResortPerson *person) {
    func_ov137_021ef2bc(people->walkers, person);
}

static ResortWalkers *func_ov137_021f05ac(ResortFieldPeople *people) {
    return people->walkers;
}

// Moves the occupants' actors to the shops' doors at x 11 or 19, and turns the NPCs to them
static void func_ov137_021f05b0(ResortFieldPeople *people, ResortSys *sys, ResortNPC *npc) {
    u32 i;
    ResortPersonData **datas = ResortSys_GetDatas(people->sys);
    ResortDataIter iter = func_ov137_021f1ae0(datas, sWalkerOccupantKinds, 1);
    ResortPersonData *data;

    while (TRUE) {
        FieldActor *actor;
        s16 x;
        u16 dir;

        data = func_ov137_021f1b0c(datas, &iter, sWalkerOccupantKinds, 1);
        if (data == NULL) {
            break;
        }
        if (func_ov137_021f195c(data)) {
            continue;
        }
        actor = func_ov137_021f110c(func_ov137_021f163c(people->people, data));
        if (GetGPosX(actor) < 15) {
            x = 11;
            dir = DIR_RIGHT;
        } else {
            x = 19;
            dir = DIR_LEFT;
        }
        SetActorGPos(actor, x, 0, GetGPosZ(actor), dir);
    }
    for (i = 0; i < func_ov137_021f1d00(npc); i++) {
        FieldActor *actor = func_ov137_021f1cc8(npc, i);
        if (actor != NULL) {
            ChangeActorMoveCodeSeq(actor, (GetGPosX(actor) < 15 ? DIR_RIGHT : DIR_LEFT) + 14);
        }
    }
}

static void func_ov137_021f0670(ResortFieldPeople *people) {
    func_ov137_021eeffc(people->watch);
}

static BOOL func_ov137_021f067c(void *context, void *person, u32 a2, u32 event) {
    ResortFieldPeople *people = context;

    switch (event) {
    default:
        return TRUE;
    case 0:
        return TRUE;
    case 3:
        func_ov137_021ef2f4(people->walkers, func_ov137_021f1600(people->people, person));
        return TRUE;
    case 4:
        return TRUE;
    case 1: {
        ResortPerson *resortPerson = func_ov137_021f1600(people->people, person);
        if (resortPerson != NULL) {
            func_ov137_021f05a0(people, resortPerson);
            func_ov137_021f15ac(people->people, resortPerson);
        }
    }
    case 2:
        if (func_ov137_021f1600(people->people, person) == NULL) {
            ResortPersonSource source;
            sys_memset(&source, 0, sizeof(ResortPersonSource));
            source.data = func_ov137_021f1b6c(ResortSys_GetDatas(people->sys), person);
            source.zone = 1;
            func_ov137_021ef250(people->walkers, func_ov137_021f14a8(people->people, &source));
        }
        return TRUE;
    }
    return TRUE;
}

static BOOL func_ov137_021f0720(void *context, void *person, u32 a2, u32 event) {
    BOOL handle = FALSE;

    if (a2 == 1) {
        u32 value = func_02037b38(person, 31, NULL);
        if (value == 2 || value == 3) {
            handle = TRUE;
        }
    }
    if (handle) {
        return func_ov137_021f067c(context, person, a2, event);
    }
    return FALSE;
}

static ResortFieldScene *func_ov137_021f0758(HeapID heapId, FieldExpObjSystem *expObj, FieldCamera *camera) {
    ResortFieldScene *scene = GFL_HeapAllocate(heapId, sizeof(ResortFieldScene), TRUE, "resort_field.c", 4098);
    scene->expObj = expObj;
    scene->camera = camera;
    FieldExpObj_AddScene(expObj, &sScene, 0);
    return scene;
}

static void func_ov137_021f0790(ResortFieldScene *scene) {
    FieldExpObj_FreeScene(scene->expObj, 0);
    GFL_HeapFree(scene);
}

static void func_ov137_021f07a4(ResortFieldScene *scene) {
    int i;

    FieldExpObj_StepAllAnimations(scene->expObj);
    for (i = 0; i < 2; i++) {
        if (scene->anims[i] != NULL && scene->anims[i]->update(scene)) {
            scene->anims[i] = NULL;
        }
    }
}

static void func_ov137_021f07d0(ResortFieldScene *scene, u32 index) {
    scene->anims[index] = &sSceneAnims[index];
    scene->anims[index]->start(scene);
}

static BOOL func_ov137_021f07ec(ResortFieldScene *scene, u32 index) {
    if (scene->anims[index] == NULL) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov137_021f0800(ResortFieldScene *scene, u32 index) {
    if (scene->anims[index] != NULL) {
        scene->anims[index]->end(scene);
        scene->anims[index] = NULL;
    }
}

static void func_ov137_021f0818(ResortFieldScene *scene) {
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

static BOOL func_ov137_021f08dc(ResortFieldScene *scene) {
    u32 i = 0;
    VecFx32 target;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(scene->expObj, 0, 0, 0);
    SRTMatrix *matrix0;
    SRTMatrix *matrix1;

    if (!func_ov036_021b8268(scene->expObj, 0, 0) && FieldExpObjAnm_IsPlaybackFinished(anm) == TRUE) {
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

static void func_ov137_021f09b4(ResortFieldScene *scene) {
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

static void func_ov137_021f0a38(ResortFieldScene *scene) {
    SRTMatrix *matrix0;
    SRTMatrix *matrix1;
    fx32 cos;
    fx32 sin;

    func_ov137_021f0818(scene);
    matrix0 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 0);
    matrix1 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 1);
    cos = FX_CosIdx(RESORT_FIELD_SCENE_ANGLE);
    sin = FX_SinIdx(RESORT_FIELD_SCENE_ANGLE);
    MTX_RotX33_(&matrix0->rotation, sin, cos);
    MTX_RotX33_(&matrix1->rotation, sin, cos);
}

static BOOL func_ov137_021f0a80(ResortFieldScene *scene) {
    VecFx32 target;
    BOOL done = func_ov137_021f08dc(scene);
    SRTMatrix *matrix0 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 0);
    SRTMatrix *matrix1 = FieldExpObj_GetActorMatrixPtr(scene->expObj, 0, 1);

    GFL_G3DCameraGetLookatPos(FieldCamera_GetG3DCamera(scene->camera), &target);
    matrix0->translation.x = target.x - FX32_CONST(121);
    matrix0->translation.z = target.z - FX32_CONST(200);
    matrix0->translation.y = target.y - FX32_CONST(200);
    matrix1->translation = matrix0->translation;
    return done;
}

static void func_ov137_021f0ad8(ResortFieldScene *scene) {
    func_ov137_021f09b4(scene);
}

// Adds people to the avenue from entries, by the player's progress, the first time
static void func_ov137_021f0ae0(ResortSys *sys, GameData *gameData, HeapID heapId) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);

    if (JoinAvenue_GetParam(info, 7, NULL) == 0 && JoinAvenue_GetParam(info, 6, NULL) == 0) {
        SaveControl *save = GameData_GetSaveControl(gameData);
        JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(save);
        void *entry;
        u32 unk;
        u8 badges;
        u8 rank;
        u32 count;
        u32 added;
        int tries;

        ResortSys_GetEntries(sys);
        entry = func_02037a40(HEAPID_TAIL(heapId));
        unk = func_ov012_02169b78(gameData);
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
            result = func_02010078(joinAvenue, gameData, entry, 2);
            if (result == 2) {
                added++;
            }
            if (result == 0 || tries++ > 100) {
                break;
            }
        }
        func_02037a68(entry);
        func_02039064(info, 6, 1);
    }
}

static void func_ov137_021f0be4(ResortSys *sys, GameData *gameData, HeapID heapId) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    void *table = ResortSys_GetRanks(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(save);
    BOOL add = FALSE;

    if (EventWork_FlagGet(eventWork, 2400) && JoinAvenue_GetParam(info, 24, NULL) == 0) {
        void *entries = ResortSys_GetEntries(sys);
        u32 i;

        add = TRUE;
        for (i = 0; i < func_02037ed4(entries); i++) {
            void *entry = func_02037f04(entries, i);
            if (!func_02037a90(entry) && func_02037b38(entry, 31, NULL) == 5) {
                add = FALSE;
                break;
            }
        }
        func_02039064(info, 24, 1);
    }
    if (add) {
        u16 chance = ResortBinary_Get(table, ResortBinary_FindRange(table, JoinAvenue_GetParam(info, 2, NULL)), 10);
        if (func_020393e4(info, 21, 100) < chance) {
            void *entry = func_02037a40(HEAPID_TAIL(heapId));
            ResortSys_MakeShopEntry(sys, gameData, entry, 5, 0, heapId);
            if (func_02010078(joinAvenue, gameData, entry, 2) == 2) {
                func_02039064(info, 25, 0);
            }
            func_02037a68(entry);
        }
    }
}

void func_ov137_021f0d10(Field *field) {
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
    unk = func_02017b84(gameData);
    fill = FALSE;
    work = Field_AllocGimmickWorkBlock(field, 0, heapId, sizeof(ResortFieldWork0));
    joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
    sys_memset(&sysSetup, 0, sizeof(ResortSysSetup));
    sysSetup.occupants = getAddressOfBeginningOfOccupants(joinAvenue);
    sysSetup.list = JoinAvenue_GetPersonList(joinAvenue);
    sysSetup.list2 = NULL;
    sysSetup.entries = func_02010054(joinAvenue);
    sysSetup.info = JoinAvenue_GetInfo(joinAvenue);
    work->sys = ResortSys_Create(&sysSetup, unk, heapId);
    if (JoinAvenue_GetParam(ResortSys_GetInfo(work->sys), 7, NULL) == 0) {
        fill = TRUE;
    }

    sys_memset(&peopleSetup, 0, sizeof(ResortPeopleSetup));
    peopleSetup.mmSys = GameData_GetMMSys(gameData);
    peopleSetup.slots = ResortSys_GetSlots(work->sys);
    peopleSetup.datas = ResortSys_GetDatas(work->sys);
    peopleSetup.count = 16;
    peopleSetup.zone = 2;
    peopleSetup.shops = ResortSys_GetShopData(work->sys);
    peopleSetup.unk18 = ResortSys_GetUnk20Table(work->sys);
    work->people = func_ov137_021f12b4(&peopleSetup, heapId);
    if (fill) {
        func_ov137_021f1350(work->people);
    }

    sys_memset(&npcSetup, 0, sizeof(ResortNPCSetup));
    npcSetup.mmSys = GameData_GetMMSys(gameData);
    npcSetup.unk4 = 16;
    npcSetup.info = ResortSys_GetInfo(work->sys);
    npcSetup.zone = 2;
    npcSetup.table = ResortSys_GetShopOpenings(work->sys);
    work->npc = func_ov137_021f1c24(&npcSetup, heapId);
}

void func_ov137_021f0e28(Field *field) {
    ResortFieldWork0 *work = Field_GetGimmickWorkBlock(field, 0);
    func_ov137_021f1c74(work->npc);
    func_ov137_021f1300(work->people);
    ResortSys_Free(work->sys);
    Field_DeleteGimmickWorkBlock(field, 0);
}

void func_ov137_021f0e50(Field *field) {
    ResortFieldWork0 *work = Field_GetGimmickWorkBlock(field, 0);
    func_ov137_021f1c88(work->npc, field);
    func_ov137_021f1348(work->people, Field_GetPlayer(field));
}

ResortPeople *func_ov137_021f0e74(Field *field) {
    ResortFieldWork0 *work = Field_GetGimmickWorkBlock(field, 0);
    return work->people;
}

ResortSys *func_ov137_021f0e80(Field *field) {
    ResortFieldWork0 *work = Field_GetGimmickWorkBlock(field, 0);
    return work->sys;
}

ResortNPC *func_ov137_021f0e8c(Field *field) {
    ResortFieldWork0 *work = Field_GetGimmickWorkBlock(field, 0);
    return work->npc;
}
