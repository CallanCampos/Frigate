#include "types.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/script_text_banks.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_script.h"
#include "field/game_beacon_set.h"
#include "field/resort.h"
#include "field/scrcmd_resort_shop.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "pml/item.h"
#include "save/bag.h"
#include "save/join_avenue.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
#include "system/bmp_menu.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/nsbtx_to_clwk.h"
#include "system/printsys.h"
#include "system/resort_binary.h"
#include "system/text_speed.h"
#include "system/vm.h"
#include "system/wordset.h"
#include "text/script/scrcmd_resort_shop.h"

// The Join Avenue's shops (plugin 8's command 1000 while this overlay takes overlay 59's place). The command opens a
// menu over the field: mode 0 is a shop's items, mode 1 the four records, and mode 2 the order of the records (sub
// mode 0) or of the avenue's people (sub mode 1)

// A row of the menu. For the people, item is the object code
typedef struct {
    u16 id;
    u16 kind;
    u16 item;
    u16 count;
    u32 price;
    StrBuf *label;
    StrBuf *name;
    StrBuf *description;
    void *person;
} ResortShopEntry;

// Cell actor resources, as the func_0204b81c family returns them
typedef struct {
    u32 palette;
    u32 chars;
    u32 cellAnims;
} ResortShopClActRes;

typedef BOOL (*ResortShopUpdateFunc)(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
typedef void (*ResortShopFunc)(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
typedef void (*ResortShopListFunc)(ResortShopWork *wk, u8 mode, u8 subMode);
typedef BOOL (*ResortShopFadeFunc)(ResortShop *shop, Field *field, int *seq);

struct ResortShopWork {
    u8 state;
    u8 nextState;
    u16 unk2;
    HeapID heapId;
    Font *font;
    TrainerCardSave *trainerCard;
    BagSave *bag;
    GameRecords *records;
    ResortShopEntry entries[17];
    u16 entryCount;
    BmpWin *windows[7];
    ListMenuOption *options;
    BmpMenu *confirmDialog;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *priceFormat;
    StrBuf *message;
    StrBuf *boughtLabel;
    BmpMenuList *list;
    PrintWindow printWindow;
    PrintQueue *printQueue;
    TCBExManager *tcbManager;
    PrintStream *printStream;
    KeyCursor *keyCursor;
    ClActUnit *clactUnit;
    ClActor *actors[6];
    ResortShopClActRes res[3];
    ArcTool *iconArc;
    ResortShopEntry *selected;
    u16 listShown;
    ResortShopUpdateFunc update;
    ResortPeople *people;
    void *npc;
    ResortSys *sys;
    void *shops;
    const u16 *row;
    void *table;
    void *flags;
    JoinAvenueOccupants *occupants;
    void *datas;
    ResortPerson *person;
    JoinAvenueInfo *info;
    u16 *vars[5];
    u16 cameraMode;
    // Added to the messages' IDs
    u16 msgBase;
    ResortShopFunc create;
    ResortShopFunc destroy;
    ResortShopListFunc swapEntries;
    ResortShopListFunc applyOrder;
    BmpMenuListCursorCallback cursorCallback;
    BmpMenuListPrintCallback printCallback;
    u16 mode;
    u16 msgs[3];
    BOOL unk300;
    u8 moveFrom;
    u8 moveTo;
    BOOL moving;
    u16 moveTop;
    u16 moveCursor;
    BOOL moved;
    u16 initialOrder[8];
    u16 order[8];
};

struct ResortShop {
    u8 state;
    u8 unk1;
    u8 mode;
    u8 subMode;
    BOOL useBoundary;
    ResortShopWork work;
    int seq;
    ResortShopFadeFunc fadeIn;
    ResortShopFadeFunc fadeOut;
    GameSystem *gsys;
};

static inline ResortShopEntry *ResortShop_GetEntry(ResortShopWork *wk, u32 index) {
    return &wk->entries[index];
}

typedef struct {
    u32 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u16 palette;
    u16 unkA;
} ResortShopWindowSetup;

// An area of the avenue in grid coordinates, and the camera setup for it
typedef struct {
    s16 xMin;
    s16 xMax;
    s16 zMin;
    s16 zMax;
    u32 camera;
} ResortShopArea;

typedef struct {
    u16 yaw;
    fx32 dx;
    fx32 dz;
} ResortShopCamera;

typedef struct {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
} ResortShopRect;

// Reading the layouts before their definitions keeps the unused entries in MWCC's
// shared constant pool, as in the original. This inline accessor emits no code.
extern const ResortShopWindowSetup sUnk021e8a6c;
extern const ResortShopWindowSetup sWindow0;
extern const ResortShopWindowSetup sWindow2;
extern const ResortShopWindowSetup sWindow3;
extern const ResortShopWindowSetup sUnk021e8a60;
extern const ResortShopWindowSetup sWindow6;
extern const ResortShopWindowSetup sWindow5;
extern const ResortShopWindowSetup sUnk021e8ab4;
extern const ResortShopWindowSetup sWindow1;
extern const ResortShopWindowSetup sWindow4;
extern const u8 sUnk021e8acc[16];

static inline u32 ResortShop_GetLayoutBG(u32 index) {
    switch (index) {
    case 0: return sUnk021e8a6c.bg;
    case 1: return sWindow0.bg;
    case 2: return sWindow2.bg;
    case 3: return sWindow3.bg;
    case 4: return sUnk021e8a60.bg;
    case 5: return sWindow6.bg;
    case 6: return sWindow5.bg;
    case 7: return sUnk021e8ab4.bg;
    case 8: return sWindow1.bg;
    case 9: return sWindow4.bg;
    default: return sUnk021e8acc[0];
    }
}

static const ConfirmDialogSetup sConfirmDialogSetup = { 1, 24, 13, 11, 25 };

const ResortShopWindowSetup sUnk021e8a6c = { 1, 1, 18, 30, 6, 11, 469 };

const ResortShopWindowSetup sWindow2 = { 1, 12, 1, 19, 16, 2, 54 };

const ResortShopWindowSetup sWindow6 = { 1, 1, 19, 30, 4, 11, 631 };

const ResortShopWindowSetup sWindow4 = { 1, 18, 15, 13, 2, 11, 447 };

const ResortShopWindowSetup sWindow1 = { 1, 1, 3, 9, 2, 11, 38 };

const ResortShopWindowSetup sUnk021e8ab4 = { 1, 2, 2, 8, 2, 11, 38 };

const ResortShopWindowSetup sWindow5 = { 1, 5, 18, 27, 6, 11, 469 };

const ResortShopWindowSetup sWindow0 = { 1, 1, 1, 9, 2, 11, 20 };

const ResortShopWindowSetup sUnk021e8a60 = { 1, 5, 2, 5, 2, 11, 38 };

const ResortShopWindowSetup sWindow3 = { 1, 1, 15, 15, 2, 11, 415 };

const u8 sUnk021e8acc[] = { 2, 3, 6, 5, 4, 5, 2, 0, 1, 3, 6, 5, 4, 7, 7, 0 };

static const ResortShopRect sListRects[] = {
    { 0, 12, 32, 12 },
    { 0, 12, 32, 6 },
};

static const ResortShopArea sAreas[] = {
    { 17, 32, 0, 65, 3 },
    { 0, 17, 0, 80, 1 },
    { 17, 32, 65, 80, 1 },
};

static const ResortShopCamera sCameras[] = {
    { 0x8000, FX32_CONST(-70), 0 },
    { 0, FX32_CONST(70), 0 },
    { 0x3fff, 0, FX32_CONST(-70) },
    { 0xc000, 0, FX32_CONST(70) },
};

static const ClActorSetup sActorSetups[] = {
    { 172, 22, 0, 0, 1 },
    { 172, 92, 0, 0, 0 },
    { 172, 132, 1, 0, 0 },
    { 224, 128, 2, 0, 0 },
    { 21, 172, 0, 0, 1 },
    { 18, 168, 0, 0, 1 },
    { 18, 168, 1, 0, 1 },
};

static const ResortShopWindowSetup *sWindowSetups[] = {
    &sWindow0, &sWindow1, &sWindow2, &sWindow3, &sWindow4, &sWindow5, &sWindow6,
};

static BOOL func_ov060_021e5c8c(VM *vm, void *env);
static void func_ov060_021e5d60(ResortShopWork *wk);
static void func_ov060_021e6064(ResortShop *shop);
static BOOL func_ov060_021e60ac(ResortShop *shop);
static void func_ov060_021e6100(ResortShop *shop);
static BOOL func_ov060_021e6140(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static BOOL func_ov060_021e64b0(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static BOOL func_ov060_021e66c4(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e688c(ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e6a28(ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e6c90(ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e6d94(ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e6fd8(ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e70e0(ResortShopWork *wk);
static u8 func_ov060_021e7128(ResortShopWork *wk, u8 mode);
static void func_ov060_021e7178(ResortShopWork *wk);
static void func_ov060_021e71c8(ResortShopWork *wk, u32 mode);
static void func_ov060_021e7274(ResortShopWork *wk);
static void func_ov060_021e7294(ResortShopWork *wk);
static void func_ov060_021e7318(ResortShopWork *wk);
static void func_ov060_021e737c(ResortShopWork *wk, u16 item);
static void func_ov060_021e7404(ResortShopWork *wk, u16 objCode);
static void func_ov060_021e7470(ResortShopWork *wk, const ClActorSetup *iconSetup);
static void func_ov060_021e75b8(ResortShopWork *wk);
static void func_ov060_021e75dc(ResortShopWork *wk);
static void func_ov060_021e7604(ResortShopWork *wk);
static void func_ov060_021e766c(ResortShopWork *wk);
static void func_ov060_021e76d0(ResortShopWork *wk);
static void func_ov060_021e7750(ResortShopWork *wk);
static void func_ov060_021e77a8(ResortShopWork *wk);
static void func_ov060_021e77d4(ResortShopWork *wk);
static void func_ov060_021e786c(ResortShopWork *wk, u16 item);
static void func_ov060_021e791c(ResortShopWork *wk, StrBuf *strbuf);
static void func_ov060_021e7974(ResortShopWork *wk, BmpMenuListCursorCallback cursorCallback,
                                BmpMenuListPrintCallback printCallback);
static void func_ov060_021e7a70(BmpMenuList *list, s32 value, u8 y);
static void func_ov060_021e7b68(ResortShopWork *wk);
static BOOL func_ov060_021e7b7c(u32 event);
static void func_ov060_021e7b80(ResortShopWork *wk, u32 msgId, u16 item, u16 count);
static void func_ov060_021e7bc8(ResortShopWork *wk, u32 msgId);
static void func_ov060_021e7c94(ResortShopWork *wk, u32 msgId);
static void func_ov060_021e7d3c(BmpWin *window);
static void func_ov060_021e7d58(ResortShopWork *wk, u32 msgId);
static void func_ov060_021e7e00(ResortShopWork *wk, u32 msgId, ResortShopEntry *entry);
static void func_ov060_021e7e84(u32 which);
static ResortPeople *func_ov060_021e7ecc(Field *field);
static void *func_ov060_021e7efc(Field *field);
static void func_ov060_021e7f2c(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e7f58(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e7f5c(BmpMenuList *list, s32 value, u8 a2);
static void func_ov060_021e7fe0(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e800c(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e8010(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e803c(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static StrBuf *func_ov060_021e8040(ResortShopWork *wk, JoinAvenuePerson *person);
static void func_ov060_021e8190(ResortShopWork *wk, ResortPersonData *data, u32 index, ResortShopEntry *entry);
static void func_ov060_021e827c(BmpMenuList *list, s32 value, u8 a2);
static void func_ov060_021e82f0(BmpMenuList *list, s32 value, u8 a2);
static void func_ov060_021e837c(ResortShopWork *wk);
static void func_ov060_021e841c(ResortShopWork *wk, u8 mode, u8 subMode);
static void func_ov060_021e85b0(ResortShopWork *wk, u8 mode, u8 subMode);
static u8 func_ov060_021e8708(u32 index);
static void func_ov060_021e8720(ArcTool *handle, u16 objCode, u8 *texture, u16 *fileId);
static u32 func_ov060_021e8758(ResortShopWork *wk, u32 price);
static void func_ov060_021e8804(ResortShop *shop, Field *field, u16 cameraMode);
static BOOL func_ov060_021e8924(ResortShop *shop, Field *field, int *seq);
static BOOL func_ov060_021e89bc(ResortShop *shop, Field *field, int *seq);

BOOL func_ov060_021e58c0(VM *vm, FieldScriptEnv *env) {
    u16 mode = ScriptReadAny(vm, env);
    u16 subMode = ScriptReadAny(vm, env);
    u16 cameraMode = ScriptReadAny(vm, env);
    u16 *var0 = ScriptReadVar(vm, env);
    u16 *var1 = ScriptReadVar(vm, env);
    u16 *var2 = ScriptReadVar(vm, env);
    u16 *var3 = ScriptReadVar(vm, env);
    u16 *var4 = ScriptReadVar(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    void **ptr = ScriptWork_GetUserHeapPtr(FieldScriptEnv_GetScriptWork(env));
    void *msgBGSys = Field_GetMsgBGSys(field);
    ResortSys *sys = ResortSys_Get(field);
    ResortShop *shop = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(ResortShop), TRUE, "scrcmd_resort_shop.c", 375);

    *ptr = shop;
    shop->work.font = func_ov036_0218799c(msgBGSys);
    shop->work.trainerCard = getTrainerCardDataBlkAddress(gameData);
    shop->work.heapId = HEAPID_FIELDMAP;
    shop->work.bag = GameData_GetBag(gameData);
    shop->work.records = GameData_GetRecords(gameData);
    shop->work.vars[0] = var0;
    shop->work.vars[1] = var1;
    shop->work.vars[2] = var2;
    shop->work.vars[3] = var3;
    shop->work.vars[4] = var4;
    shop->work.shops = ResortSys_GetShopData(sys);
    shop->work.table = ResortSys_GetUnk24Table(sys);
    shop->work.occupants = ResortSys_GetOccupants(sys);
    shop->work.datas = ResortSys_GetDatas(sys);
    shop->work.info = ResortSys_GetInfo(sys);
    shop->work.sys = sys;
    shop->work.cameraMode = cameraMode;
    shop->work.people = func_ov060_021e7ecc(field);
    shop->work.npc = func_ov060_021e7efc(field);
    shop->gsys = gsys;
    shop->work.person = ResortSys_GetScriptPerson(env, shop->work.people);
    if (JoinAvenue_GetParam(shop->work.info, 13, 0) != 0) {
        shop->work.unk300 = TRUE;
    } else {
        shop->work.unk300 = FALSE;
    }
    shop->fadeIn = func_ov060_021e8924;
    shop->fadeOut = func_ov060_021e89bc;
    shop->work.msgBase = func_ov137_021f10e8(shop->work.person, 2, NULL);
    *shop->work.vars[0] = 0;
    shop->work.mode = mode;
    switch (mode) {
    case 0:
    default:
        func_ov012_02160574();
        shop->mode = mode;
        shop->subMode = 0;
        shop->work.update = func_ov060_021e6140;
        shop->work.create = func_ov060_021e7f2c;
        shop->work.destroy = func_ov060_021e7f58;
        shop->work.cursorCallback = func_ov060_021e7f5c;
        shop->work.printCallback = func_ov060_021e7a70;
        shop->work.flags = func_02038470(func_ov137_021f0f58(shop->work.person));
        shop->work.row = ResortShopData_GetPersonShop(shop->work.shops, func_ov137_021f0f58(shop->work.person));
        break;
    case 1:
        shop->mode = mode;
        shop->subMode = subMode;
        shop->work.update = func_ov060_021e64b0;
        switch (subMode) {
        case 0:
        default:
            shop->work.create = func_ov060_021e7fe0;
            shop->work.destroy = func_ov060_021e800c;
            shop->work.msgs[1] = 52;
            shop->work.msgs[2] = 53;
            shop->work.cursorCallback = func_ov060_021e827c;
            shop->work.printCallback = NULL;
            break;
        case 1:
            shop->work.create = func_ov060_021e7fe0;
            shop->work.destroy = func_ov060_021e800c;
            shop->work.msgs[1] = 50;
            shop->work.msgs[2] = 51;
            shop->work.cursorCallback = func_ov060_021e827c;
            shop->work.printCallback = NULL;
            break;
        case 2:
            shop->work.create = func_ov060_021e8010;
            shop->work.destroy = func_ov060_021e803c;
            shop->work.msgs[1] = 54;
            shop->work.msgs[2] = 55;
            shop->work.cursorCallback = func_ov060_021e827c;
            shop->work.printCallback = NULL;
            break;
        case 3:
            shop->work.create = func_ov060_021e8010;
            shop->work.destroy = func_ov060_021e803c;
            shop->work.msgs[1] = 56;
            shop->work.msgs[2] = 57;
            shop->work.cursorCallback = func_ov060_021e827c;
            shop->work.printCallback = NULL;
            break;
        }
        break;
    case 2:
        shop->mode = mode;
        shop->subMode = subMode;
        shop->work.update = func_ov060_021e66c4;
        switch (subMode) {
        case 0:
            shop->work.create = func_ov060_021e7fe0;
            shop->work.destroy = func_ov060_021e800c;
            shop->work.swapEntries = func_ov060_021e6d94;
            shop->work.applyOrder = func_ov060_021e841c;
            shop->work.msgs[0] = 67;
            shop->work.msgs[1] = 59;
            shop->work.msgs[2] = 68;
            shop->work.cursorCallback = func_ov060_021e82f0;
            shop->work.printCallback = NULL;
            break;
        case 1:
            shop->work.create = func_ov060_021e8010;
            shop->work.destroy = func_ov060_021e803c;
            shop->work.swapEntries = func_ov060_021e6fd8;
            shop->work.applyOrder = func_ov060_021e85b0;
            shop->work.msgs[0] = 58;
            shop->work.msgs[1] = 59;
            shop->work.msgs[2] = 60;
            shop->work.cursorCallback = func_ov060_021e82f0;
            shop->work.printCallback = NULL;
            break;
        }
        break;
    }
    VM_SetNativeCallback(vm, func_ov060_021e5c8c);
    return TRUE;
}

static BOOL func_ov060_021e5c8c(VM *vm, void *env) {
    void **ptr = ScriptWork_GetUserHeapPtr(FieldScriptEnv_GetScriptWork(env));
    ResortShop *shop = *ptr;
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);

    Field_GetCameraSystem(field);
    switch (shop->state) {
    case 0:
        if (shop->fadeIn(shop, field, &shop->seq)) {
            setAlphaBlend_wrapper(FALSE);
            shop->work.state = 0;
            shop->state++;
        }
        break;
    case 1:
        if (shop->work.update(gsys, &shop->work, shop->mode, shop->subMode)) {
            shop->state++;
        }
        break;
    case 2:
        if (shop->fadeOut(shop, field, &shop->seq)) {
            setAlphaBlend_wrapper(TRUE);
            shop->state++;
        }
        break;
    case 3:
        GFL_HeapFree(*ptr);
        return TRUE;
    }
    return FALSE;
}

static void func_ov060_021e5d34(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    GetGameDataPlayerInfo(GSYS_GetGameData(gsys));
    func_ov060_021e7128(wk, mode);
    func_ov060_021e5d60(wk);
    func_ov060_021e76d0(wk);
    func_ov060_021e75b8(wk);
}

static void func_ov060_021e5d60(ResortShopWork *wk) {
    wk->priceFormat = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty);
    wk->boughtLabel = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty_2);
}

static u32 func_ov060_021e5d84(ResortShopWork *wk) {
    return getCash(wk->trainerCard);
}

// Buying the chosen item: the answers when it was bought already, does not fit in the bag or costs too much, and else
// the question
static void func_ov060_021e5d90(ResortShopWork *wk) {
    u32 cash = func_ov060_021e5d84(wk);

    if (func_02036434(wk->flags, wk->selected->id) == TRUE) {
        if (wk->selected->kind == 0) {
            func_ov060_021e7bc8(wk, wk->msgBase + 17);
        } else {
            func_ov060_021e7bc8(wk, wk->msgBase + 21);
        }
        wk->state = 8;
        wk->nextState = 2;
        return;
    }
    if (wk->selected->kind == 0 &&
        !BagSave_CheckAvailItemSpace(wk->bag, wk->selected->item, wk->selected->count, wk->heapId)) {
        func_ov060_021e7bc8(wk, wk->msgBase + 10);
        wk->state = 8;
        wk->nextState = 2;
        return;
    }
    if (cash < wk->selected->price) {
        func_ov060_021e7bc8(wk, wk->msgBase);
        wk->state = 8;
        wk->nextState = 2;
        return;
    }
    if (wk->selected->kind == 0) {
        func_ov060_021e7e00(wk, wk->msgBase + 4, wk->selected);
    } else {
        func_ov060_021e7e00(wk, wk->msgBase + 6, wk->selected);
    }
    wk->state = 8;
    wk->nextState = 3;
}

static void func_ov060_021e5e54(ResortShopWork *wk) {
    s32 result = BmpMenuList_Update(wk->list);

    if (result == BMPMENULIST_NULL) {
        return;
    }
    if (result == BMPMENULIST_CANCEL) {
        wk->selected = NULL;
        wk->state = 9;
        return;
    }
    wk->selected = &wk->entries[result];
    func_ov060_021e5d90(wk);
}

static void func_ov060_021e5e94(ResortShopWork *wk) {
    s32 result = BmpMenuList_Update(wk->list);

    if (result == BMPMENULIST_NULL) {
        return;
    }
    if (result == BMPMENULIST_CANCEL) {
        wk->selected = NULL;
        wk->state = 8;
        return;
    }
    wk->selected = &wk->entries[result];
    func_ov060_021e7e00(wk, wk->msgs[1], wk->selected);
    wk->state = 7;
    wk->nextState = 3;
}

// The list of mode 2, where the first choice picks an entry up and the second puts it down
static void func_ov060_021e5ee0(ResortShopWork *wk) {
    s32 result = BmpMenuList_Update(wk->list);

    if (result == BMPMENULIST_NULL) {
        return;
    }
    if (result == BMPMENULIST_CANCEL) {
        if (!wk->moving) {
            wk->selected = NULL;
            wk->state = 9;
            return;
        }
        wk->moving = FALSE;
        func_0204c124(wk->actors[5], FALSE);
        return;
    }
    if (wk->moving) {
        wk->moveTo = result;
        if (wk->moveTo != wk->moveFrom) {
            wk->moved = TRUE;
            wk->state = 4;
        }
        return;
    }
    wk->moveFrom = result;
    wk->moving = TRUE;
    wk->selected = &wk->entries[result];
    BmpMenuList_GetPos(wk->list, &wk->moveTop, &wk->moveCursor);
    func_ov060_021e837c(wk);
}

// Prints the message, returning the next state once it is done
static u8 func_ov060_021e5f88(ResortShopWork *wk) {
    u32 state = func_020223b4(wk->printStream);

    if (wk->keyCursor != NULL) {
        KeyCursor_Update(wk->keyCursor, wk->printStream, wk->windows[6]);
    }
    if (state == PRINT_STREAM_DONE) {
        func_020223cc(wk->printStream);
        return wk->nextState;
    }
    if (state == PRINT_STREAM_RUNNING) {
        if ((GCTX_HIDGetHeldKeys() & PAD_BUTTON_A) || (GCTX_HIDGetHeldKeys() & PAD_BUTTON_B)) {
            func_020223e0(wk->printStream, 0);
        }
    } else if (state == PRINT_STREAM_PAUSED) {
        BOOL next = FALSE;

        if (wk->keyCursor != NULL) {
            if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
                next = TRUE;
            }
        } else if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) ||
                   (GCTX_HIDGetPressedKeys() & PAD_PLUS_KEY_MASK)) {
            next = TRUE;
        }
        if (next) {
            func_020223bc(wk->printStream);
            GFL_SndSEPlay(1351);
        }
    }
    return wk->state;
}

static void func_ov060_021e6044(ResortShopWork *wk, u32 price) {
    subCashFromTotal(wk->trainerCard, price);
    RecordAdd(wk->records, 22, price);
    RecordAddOne(wk->records, 21);
}

static void func_ov060_021e6064(ResortShop *shop) {
    ResortShopWork *wk = &shop->work;

    func_ov060_021e5d34(shop->gsys, wk, shop->mode, shop->subMode);
    func_ov060_021e71c8(wk, shop->mode);
    if (shop->mode == 0) {
        func_ov060_021e7750(wk);
        func_ov060_021e77d4(wk);
    }
    wk->create(shop->gsys, wk, shop->mode, shop->subMode);
}

// Shows the list's window once everything is printed
static BOOL func_ov060_021e60ac(ResortShop *shop) {
    ResortShopWork *wk = &shop->work;

    if (!func_02021c0c(wk->printQueue)) {
        return FALSE;
    }
    if (wk->listShown == 0) {
        BmpWin_TransferNow(wk->windows[2]);
        wk->listShown++;
        return TRUE;
    }
    return FALSE;
}

static void func_ov060_021e6100(ResortShop *shop) {
    ResortShopWork *wk = &shop->work;

    wk->destroy(shop->gsys, wk, shop->mode, shop->subMode);
    func_ov060_021e7b68(wk);
    func_ov060_021e766c(wk);
    func_ov060_021e77a8(wk);
    func_ov060_021e7274(wk);
    func_ov060_021e70e0(wk);
    func_ov060_021e7178(wk);
}

// Mode 0, a shop
static BOOL func_ov060_021e6140(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    u32 result;

    switch (wk->state) {
    case 0:
        func_ov060_021e5e54(wk);
        break;
    case 1:
        break;
    case 2:
        func_ov060_021e7e84(0);
        BmpWin_FlushMap(wk->windows[2]);
        BmpWin_FlushMap(wk->windows[5]);
        GFL_BGSysQueueScrLoad(1);
        func_0204c124(wk->actors[3], FALSE);
        BmpMenuList_Redraw(wk->list);
        wk->state = 0;
        break;
    case 3:
        func_ov060_021e7e84(1);
        BmpWin_FlushMap(wk->windows[2]);
        func_0204c124(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        if (wk->selected->kind == 0) {
            func_ov060_021e786c(wk, wk->selected->item);
        }
        wk->state = 5;
        break;
    case 4:
        func_ov060_021e7e84(1);
        BmpWin_FlushMap(wk->windows[2]);
        func_0204c124(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        wk->state = 5;
        break;
    case 5:
        result = ConfirmDialog_Update(wk->confirmDialog);
        if (result != BMPMENU_NULL) {
            if (result == 0) {
                wk->state = 6;
                func_ov060_021e7e84(1);
                BmpMenuList_Redraw(wk->list);
                BmpWin_FlushMap(wk->windows[2]);
                GFL_BGSysQueueScrLoad(1);
            } else if (result == BMPMENU_CANCEL) {
                wk->state = 2;
            }
        }
        break;
    case 6:
        if (wk->selected->kind == 0) {
            GFL_SndSEPlay(1621);
            func_ov060_021e7b80(wk, wk->msgBase + 8, wk->selected->item, wk->selected->count);
            func_ov060_021e6044(wk, wk->selected->price);
            func_ov060_021e77d4(wk);
            BagSave_AddItem(wk->bag, wk->selected->item, wk->selected->count, wk->heapId);
            func_02036448(wk->flags, wk->selected->id, TRUE);
            wk->state = 8;
            wk->nextState = 2;
        } else if (wk->selected->kind == 11) {
            GFL_SndSEPlay(1621);
            func_ov060_021e7e00(wk, wk->msgBase + 61, wk->selected);
            func_ov060_021e6044(wk, wk->selected->price);
            func_ov060_021e77d4(wk);
            func_02036448(wk->flags, wk->selected->id, TRUE);
            wk->state = 8;
            wk->nextState = 7;
        } else {
            func_ov060_021e7bc8(wk, wk->msgBase + 15);
            wk->state = 8;
            wk->nextState = 9;
        }
        break;
    case 7: {
        u16 row = wk->selected->item;
        ResortPersonData *data = func_ov137_021f1110(wk->person);
        u32 prize = ResortBinary_Get(
            wk->table, row,
            join_ave_raffle_shop(wk->table, row,
                                 func_ov137_021f1990(data, wk->selected->id + 5, ResortBinary_Get(wk->table, row, 0))) *
                    2 +
                3);

        func_0202437c(wk->wordSet, 2, wk->selected->name, 2, 1, 2);
        if (BagSave_CheckAvailItemSpace(wk->bag, prize, 1, wk->heapId)) {
            func_ov060_021e7b80(wk, wk->msgBase + 63, prize, 1);
            BagSave_AddItem(wk->bag, prize, 1, wk->heapId);
        } else {
            func_ov060_021e7b80(wk, wk->msgBase + 65, prize, 1);
        }
        wk->state = 8;
        wk->nextState = 2;
        break;
    }
    case 8:
        wk->state = func_ov060_021e5f88(wk);
        break;
    case 9:
        if (wk->selected != NULL) {
            *wk->vars[0] = wk->selected->kind;
            *wk->vars[1] = wk->selected->item;
            *wk->vars[2] = wk->selected->count;
            *wk->vars[3] = wk->selected->id;
            *wk->vars[4] = wk->selected->price / 10;
        } else {
            *wk->vars[0] = 0xff;
            *wk->vars[1] = 0;
            *wk->vars[2] = 0;
            *wk->vars[3] = 0;
            *wk->vars[4] = 0;
        }
        return TRUE;
    }
    func_02021a3c(wk->printQueue);
    PrintWindow_Flush(&wk->printWindow, wk->printQueue);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    return FALSE;
}

// Mode 1, the records
static BOOL func_ov060_021e64b0(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    u32 result;

    switch (wk->state) {
    case 0:
        func_ov060_021e5e94(wk);
        break;
    case 1:
        break;
    case 2:
        func_ov060_021e7e84(0);
        BmpWin_FlushMap(wk->windows[2]);
        BmpWin_FlushMap(wk->windows[5]);
        GFL_BGSysQueueScrLoad(1);
        func_0204c124(wk->actors[3], FALSE);
        BmpMenuList_Redraw(wk->list);
        wk->state = 0;
        break;
    case 3:
        func_ov060_021e7e84(1);
        BmpWin_FlushMap(wk->windows[2]);
        func_0204c124(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        wk->state = 5;
        break;
    case 4:
        func_ov060_021e7e84(1);
        BmpWin_FlushMap(wk->windows[2]);
        func_0204c124(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        wk->state = 5;
        break;
    case 5:
        result = ConfirmDialog_Update(wk->confirmDialog);
        if (result != BMPMENU_NULL) {
            if (result == 0) {
                wk->state = 6;
                func_ov060_021e7e84(1);
                BmpMenuList_Redraw(wk->list);
                BmpWin_FlushMap(wk->windows[2]);
                GFL_BGSysQueueScrLoad(1);
            } else if (result == BMPMENU_CANCEL) {
                wk->state = 2;
            }
        }
        break;
    case 6: {
        StrBuf *strbuf = GFL_StrBufCreate(64, wk->heapId);
        u16 name[8];
        u8 gender;

        joinAveTextHandler(wk->selected->person, JOIN_AVE_PARAM_NAME, name);
        GFL_StrBufLoadString(strbuf, name);
        gender = joinAveTextHandler(wk->selected->person, 2, NULL);
        func_0202437c(wk->wordSet, 2, strbuf, gender, 1, 2);
        GFL_StrBufFree(strbuf);
        func_ov060_021e7bc8(wk, wk->msgs[2]);
        wk->state = 7;
        wk->nextState = 8;
        break;
    }
    case 7:
        wk->state = func_ov060_021e5f88(wk);
        break;
    case 8:
        if (wk->selected != NULL) {
            *wk->vars[0] = wk->selected->kind;
            *wk->vars[1] = 0;
            *wk->vars[2] = 0;
            *wk->vars[3] = wk->selected->id;
            *wk->vars[4] = wk->selected->price;
        } else {
            *wk->vars[0] = 0xff;
            *wk->vars[1] = 0;
            *wk->vars[2] = 0;
            *wk->vars[3] = 0;
            *wk->vars[4] = 0;
        }
        return TRUE;
    }
    func_02021a3c(wk->printQueue);
    PrintWindow_Flush(&wk->printWindow, wk->printQueue);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    return FALSE;
}

// Mode 2, the order of the records or the people
static BOOL func_ov060_021e66c4(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    u32 result;

    switch (wk->state) {
    case 0:
        func_ov060_021e7c94(wk, wk->msgs[0]);
        wk->state = 8;
        wk->nextState = 3;
        break;
    case 1:
        func_ov060_021e5ee0(wk);
        break;
    case 2:
        break;
    case 3:
        func_ov060_021e7e84(0);
        BmpWin_FlushMap(wk->windows[2]);
        BmpWin_FlushMap(wk->windows[5]);
        GFL_BGSysQueueScrLoad(1);
        func_0204c124(wk->actors[3], FALSE);
        BmpMenuList_Redraw(wk->list);
        wk->state = 1;
        break;
    case 4:
        wk->swapEntries(wk, mode, subMode);
        wk->moving = FALSE;
        func_0204c124(wk->actors[5], FALSE);
        func_ov060_021e7b68(wk);
        func_ov060_021e7974(wk, wk->cursorCallback, wk->printCallback);
        BmpWin_FlushMap(wk->windows[2]);
        GFL_BGSysQueueScrLoad(1);
        func_ov060_021e7bc8(wk, wk->msgs[2]);
        wk->state = 8;
        wk->nextState = 3;
        break;
    case 5:
        func_ov060_021e7e84(1);
        BmpWin_FlushMap(wk->windows[2]);
        func_0204c124(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        wk->state = 7;
        break;
    case 6:
        func_ov060_021e7e84(1);
        BmpWin_FlushMap(wk->windows[2]);
        func_0204c124(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        wk->state = 7;
        break;
    case 7:
        result = ConfirmDialog_Update(wk->confirmDialog);
        if (result != BMPMENU_NULL) {
            if (result == 0) {
                wk->state = 4;
            } else if (result == BMPMENU_CANCEL) {
                wk->state = 3;
            }
        }
        break;
    case 8:
        wk->state = func_ov060_021e5f88(wk);
        break;
    case 9:
        wk->applyOrder(wk, mode, subMode);
        *wk->vars[0] = GFL_STD_MemCmp(wk->order, wk->initialOrder, sizeof(wk->order)) != 0 ? TRUE : FALSE;
        return TRUE;
    }
    func_02021a3c(wk->printQueue);
    PrintWindow_Flush(&wk->printWindow, wk->printQueue);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    return FALSE;
}

// The shop's items
static void func_ov060_021e688c(ResortShopWork *wk, u8 mode, u8 subMode) {
    int i;
    int count;
    MsgData *itemNames;
    MsgData *scriptMsgData;
    MsgData *itemDescriptions;
    int n = 0;

    itemNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ITEM_NAMES, wk->heapId);
    scriptMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, wk->heapId);
    itemDescriptions = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ITEM_DESCRIPTIONS, wk->heapId);
    count = ResortShopData_GetGoodsCount(wk->shops, wk->row);
    wk->options = ListMenuCore_CreateOptionList(count + 1, wk->heapId);
    for (i = 0; i < count; i++) {
        const u16 *item = ResortShopData_GetGoods(wk->shops, wk->row, i);
        u16 price = ResortShopData_GetGoodsParam(item, 4);

        if (ResortShopData_GetGoodsParam(item, 0) != 0 && !wk->unk300) {
            continue;
        }
        wk->entries[n].id = i;
        wk->entries[n].kind = ResortShopData_GetGoodsParam(item, 5);
        wk->entries[n].price = func_ov060_021e8758(wk, price * 10);
        wk->entries[n].item = ResortShopData_GetGoodsParam(item, 6);
        wk->entries[n].count = ResortShopData_GetGoodsParam(item, 7);
        if (ResortShopData_GetGoodsParam(item, 1) != 0) {
            u16 nameId = ResortShopData_GetGoodsParam(item, 2);
            u16 descriptionId = ResortShopData_GetGoodsParam(item, 3);

            wk->entries[n].label = GFL_MsgDataLoadStrbufNew(scriptMsgData, nameId);
            wk->entries[n].name = GFL_MsgDataLoadStrbufNew(scriptMsgData, nameId);
            wk->entries[n].description = GFL_MsgDataLoadStrbufNew(scriptMsgData, descriptionId);
        } else {
            wk->entries[n].label = GFL_MsgDataLoadStrbufNew(itemNames, wk->entries[n].item);
            wk->entries[n].name = GFL_MsgDataLoadStrbufNew(itemNames, wk->entries[n].item);
            wk->entries[n].description = GFL_MsgDataLoadStrbufNew(itemDescriptions, wk->entries[n].item);
        }
        ListMenuCore_AppendStrBufOption(&wk->options[n], wk->entries[n].label, n, wk->heapId);
        n++;
    }
    wk->entryCount = n;
    ListMenuCore_AppendMsgOption(&wk->options[n], wk->msgData, 12, BMPMENULIST_CANCEL, wk->heapId);
    GFL_MsgDataFree(itemDescriptions);
    GFL_MsgDataFree(scriptMsgData);
    GFL_MsgDataFree(itemNames);
}

// The four records
static void func_ov060_021e6a28(ResortShopWork *wk, u8 mode, u8 subMode) {
    int i;
    int count;
    int n;
    u16 name[8];

    count = func_0203889c(wk->occupants);
    wk->options = ListMenuCore_CreateOptionList(count + 1, wk->heapId);
    n = 0;
    for (i = 0; i < 4; i++) {
        void *record = func_0203888c(wk->occupants, i);
        u16 a;
        u16 b;
        u16 c;
        StrBuf *labelFormat;
        StrBuf *descriptionFormat;
        u8 gender;
        StrBuf *nameBuf;
        StrBuf *label;
        StrBuf *description;

        wk->initialOrder[i] = i;
        wk->order[i] = i;
        if (func_020384e0(record)) {
            continue;
        }
        wk->entries[n].id = i;
        wk->entries[n].kind = 0;
        wk->entries[n].price = 0;
        wk->entries[n].item = func_020385a8(record, 3, NULL);
        wk->entries[n].count = 0;
        wk->entries[n].person = record;
        a = func_020385a8(record, 21, NULL);
        b = func_020385a8(record, 22, NULL);
        c = func_020385a8(record, 23, NULL);
        labelFormat = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty_10);
        descriptionFormat = GFL_MsgDataLoadStrbufNew(wk->msgData, n + 37);
        gender = func_020385a8(record, 2, NULL);
        func_020385a8(record, 4, name);
        nameBuf = GFL_StrBufCreate(8, wk->heapId);
        GFL_StrBufLoadString(nameBuf, name);
        label = GFL_StrBufCreate(64, wk->heapId);
        description = GFL_StrBufCreate(128, wk->heapId);
        WordSetNumber(wk->wordSet, 1, n + 1, 1, NUM_PAD_NONE, 1);
        func_0202437c(wk->wordSet, 0, nameBuf, gender, 1, 2);
        GFL_WordSetFormatStrbuf(wk->wordSet, label, labelFormat);
        WordSetNumber(wk->wordSet, 4, a, 2, NUM_PAD_ZERO, 1);
        WordSetNumber(wk->wordSet, 5, b, 2, NUM_PAD_NONE, 1);
        WordSetNumber(wk->wordSet, 6, c, 2, NUM_PAD_NONE, 1);
        GFL_WordSetFormatStrbuf(wk->wordSet, description, descriptionFormat);
        wk->entries[n].label = label;
        wk->entries[n].name = GFL_StrBufClone(nameBuf, wk->heapId);
        wk->entries[n].description = description;
        GFL_StrBufFree(nameBuf);
        GFL_StrBufFree(descriptionFormat);
        GFL_StrBufFree(labelFormat);
        ListMenuCore_AppendStrBufOption(&wk->options[n], wk->entries[n].label, n, wk->heapId);
        n++;
    }
    wk->entryCount = count;
    ListMenuCore_AppendMsgOption(&wk->options[n], wk->msgData, 12, BMPMENULIST_CANCEL, wk->heapId);
}

// The avenue's people, from the last
static void func_ov060_021e6c90(ResortShopWork *wk, u8 mode, u8 subMode) {
    int count = func_02038868(wk->occupants);
    int n;
    int i;

    wk->options = ListMenuCore_CreateOptionList(count + 1, wk->heapId);
    n = 0;
    for (i = 7; i >= 0; i--) {
        JoinAvenuePerson *person = func_02038860(wk->occupants, i);
        ResortPersonData *data;

        if (JoinAvenuePerson_IsEmpty(person)) {
            continue;
        }
        data = func_ov137_021f1b6c(wk->datas, person);
        wk->initialOrder[n] = i;
        wk->order[n] = i;
        wk->entries[n].id = i;
        wk->entries[n].kind = 0;
        wk->entries[n].price = 0;
        wk->entries[n].item = joinAveTextHandler(person, 3, NULL);
        wk->entries[n].count = 0;
        wk->entries[n].person = person;
        func_ov060_021e8190(wk, data, n, &wk->entries[n]);
        ListMenuCore_AppendStrBufOption(&wk->options[n], wk->entries[n].label, n, wk->heapId);
        n++;
    }
    wk->entryCount = count;
    ListMenuCore_AppendMsgOption(&wk->options[n], wk->msgData, 12, BMPMENULIST_CANCEL, wk->heapId);
}

// Swaps the moved record with the one it was put on, and remakes the list
static void func_ov060_021e6d94(ResortShopWork *wk, u8 mode, u8 subMode) {
    ResortShopEntry entry;
    int i;
    u16 name[8];

    entry = wk->entries[wk->moveFrom];
    wk->entries[wk->moveFrom] = wk->entries[wk->moveTo];
    wk->entries[wk->moveTo] = entry;
    wk->selected = NULL;
    ListMenuCore_FreeOptionList(wk->options);
    wk->options = ListMenuCore_CreateOptionList(wk->entryCount + 1, wk->heapId);
    for (i = 0; i < wk->entryCount; i++) {
        void *record = wk->entries[i].person;
        StrBuf *labelFormat = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty_10);
        StrBuf *descriptionFormat = GFL_MsgDataLoadStrbufNew(wk->msgData, i + 37);
        u8 gender = func_020385a8(record, 2, NULL);
        u16 a = func_020385a8(record, 21, NULL);
        u16 b = func_020385a8(record, 22, NULL);
        u16 c = func_020385a8(record, 23, NULL);
        StrBuf *nameBuf;

        func_020385a8(record, 4, name);
        nameBuf = GFL_StrBufCreate(8, wk->heapId);
        GFL_StrBufLoadString(nameBuf, name);
        WordSetNumber(wk->wordSet, 1, i + 1, 1, NUM_PAD_NONE, 1);
        func_0202437c(wk->wordSet, 0, nameBuf, gender, 1, 2);
        WordSetNumber(wk->wordSet, 4, a, 2, NUM_PAD_ZERO, 1);
        WordSetNumber(wk->wordSet, 5, b, 2, NUM_PAD_NONE, 1);
        WordSetNumber(wk->wordSet, 6, c, 2, NUM_PAD_NONE, 1);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->entries[i].label, labelFormat);
        // The copy goes into the new name, rather than from it
        GFL_StrBufCopy(nameBuf, wk->entries[i].name);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->entries[i].description, descriptionFormat);
        GFL_StrBufFree(nameBuf);
        GFL_StrBufFree(descriptionFormat);
        GFL_StrBufFree(labelFormat);
        ListMenuCore_AppendStrBufOption(&wk->options[i], wk->entries[i].label, i, wk->heapId);
    }
    ListMenuCore_AppendMsgOption(&wk->options[i], wk->msgData, 12, BMPMENULIST_CANCEL, wk->heapId);
}

// The same for the people
static void func_ov060_021e6fd8(ResortShopWork *wk, u8 mode, u8 subMode) {
    ResortShopEntry entry;
    int i;

    entry = wk->entries[wk->moveFrom];
    wk->entries[wk->moveFrom] = wk->entries[wk->moveTo];
    wk->entries[wk->moveTo] = entry;
    wk->selected = NULL;
    ListMenuCore_FreeOptionList(wk->options);
    wk->options = ListMenuCore_CreateOptionList(wk->entryCount + 1, wk->heapId);
    for (i = 0; i < wk->entryCount; i++) {
        func_ov060_021e8190(wk, func_ov137_021f1b6c(wk->datas, wk->entries[i].person), i, &wk->entries[i]);
        ListMenuCore_AppendStrBufOption(&wk->options[i], wk->entries[i].label, i, wk->heapId);
    }
    ListMenuCore_AppendMsgOption(&wk->options[i], wk->msgData, 12, BMPMENULIST_CANCEL, wk->heapId);
}

static void func_ov060_021e70e0(ResortShopWork *wk) {
    int i;

    ListMenuCore_FreeOptionList(wk->options);
    for (i = 0; i < 17; i++) {
        if (wk->entries[i].description != NULL) {
            GFL_StrBufFree(wk->entries[i].description);
            wk->entries[i].description = NULL;
        }
        if (wk->entries[i].name != NULL) {
            GFL_StrBufFree(wk->entries[i].name);
            wk->entries[i].name = NULL;
        }
        if (wk->entries[i].label != NULL) {
            GFL_StrBufFree(wk->entries[i].label);
            wk->entries[i].label = NULL;
        }
    }
}

static u8 func_ov060_021e7128(ResortShopWork *wk, u8 mode) {
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_SCRCMD_RESORT_SHOP, wk->heapId);
    wk->wordSet = GFL_WordSetSystemCreate(8, 64, wk->heapId);
    wk->message = GFL_StrBufCreate(200, wk->heapId);
    wk->tcbManager = GFL_TCBExMgrCreate(wk->heapId, wk->heapId, 32, 32);
    return mode;
}

static void func_ov060_021e7178(ResortShopWork *wk) {
    GFL_TCBExMgrFree(wk->tcbManager);
    GFL_WordSetSystemFree(wk->wordSet);
    if (wk->keyCursor != NULL) {
        KeyCursor_Free(wk->keyCursor);
        wk->keyCursor = NULL;
    }
    GFL_StrBufFree(wk->message);
    GFL_StrBufFree(wk->boughtLabel);
    GFL_StrBufFree(wk->priceFormat);
    GFL_MsgDataFree(wk->msgData);
}

static void func_ov060_021e71c8(ResortShopWork *wk, u32 mode) {
    ArcTool *arc;

    G2_BlendNone();
    arc = GFL_ArcSysCreateFileHandle(53, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 0, 0, 0x60, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, 2, 100, 0, FALSE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 2, 2, 0, 100, 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
    LoadSysMsgBox(1, 228, 9, 0, wk->heapId);
    GFL_BGSysClearScr(1);
    GFL_BGSysSetBGPriority(2, 1);
    if (mode == 1 || mode == 2) {
        GFL_BGSysFillScrArea(2, 0, 0, 0, 11, 17, 16);
        GFL_BGSysLoadScr(2);
    }
}

static void func_ov060_021e7274(ResortShopWork *wk) {
    GFL_BGSysClearScr(1);
    GFL_BGSysClearScr(2);
    GFL_BGSysSetBGPriority(2, 0);
    setAlphaBlend_wrapper(TRUE);
}

static void func_ov060_021e7294(ResortShopWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(53, wk->heapId);

    wk->res[0].palette = func_0204bbb8(arc, 11, 0, 0, 0, 2, wk->heapId);
    wk->res[0].chars = func_0204b81c(arc, 8, 0, 0, wk->heapId);
    wk->res[1].chars = func_0204b81c(arc, 5, 0, 0, wk->heapId);
    wk->res[0].cellAnims = func_0204bde0(arc, 9, 10, wk->heapId);
    wk->res[1].cellAnims = func_0204bde0(arc, 6, 7, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void func_ov060_021e7318(ResortShopWork *wk) {
    wk->res[2].palette = func_0204bbb8(wk->iconArc, GetItemGraphicsDatID(ITEM_MASTER_BALL, 2), 0, 64, 0, 1, wk->heapId);
    wk->res[2].chars = func_0204b81c(wk->iconArc, GetItemGraphicsDatID(ITEM_MASTER_BALL, 1), 0, 0, wk->heapId);
    wk->res[2].cellAnims = func_0204bde0(wk->iconArc, 1, 0, wk->heapId);
}

// Shows an item's icon, or hides the icon for anything past the last item
static void func_ov060_021e737c(ResortShopWork *wk, u16 item) {
    NNSG2dCharacterData *character;
    NNSG2dPaletteData *palette;
    void *nclr;
    void *ncgr;

    if (item <= ITEM_REVEAL_GLASS) {
        nclr = GFL_G2DIOReadNCLRArc(wk->iconArc, GetItemGraphicsDatID(item, 2), &palette, wk->heapId);
        ncgr = GFL_G2DIOReadOBJNCGRArc(wk->iconArc, GetItemGraphicsDatID(item, 1), FALSE, &character, wk->heapId);
        func_0204bd10(wk->res[2].palette, palette, 1);
        func_0204ba40(wk->res[2].chars, character);
        GFL_HeapFree(nclr);
        GFL_HeapFree(ncgr);
        func_0204c124(wk->actors[4], TRUE);
    } else {
        func_0204c124(wk->actors[4], FALSE);
    }
}

// Shows a person's model as the icon, or hides it for 0xffff
static void func_ov060_021e7404(ResortShopWork *wk, u16 objCode) {
    ArcTool *arc;
    u8 texture;
    u16 fileId;

    if (objCode != 0xffff) {
        arc = GFL_ArcSysCreateFileHandle(ARCID_MMODEL_TBL, wk->heapId);
        func_ov060_021e8720(arc, objCode, &texture, &fileId);
        GFL_ArcToolFree(arc);
        NsbtxToClwk_LoadTexture(wk->actors[4], ARCID_MMODEL_GRA, fileId, texture, 4, 4, 0, 0, wk->heapId);
        func_0204c124(wk->actors[4], TRUE);
    } else {
        func_0204c124(wk->actors[4], FALSE);
    }
}

static void func_ov060_021e7470(ResortShopWork *wk, const ClActorSetup *iconSetup) {
    wk->actors[0] = func_0204c040(wk->clactUnit, wk->res[0].chars, wk->res[0].palette, wk->res[0].cellAnims, &sActorSetups[0], 0,
                                  wk->heapId);
    wk->actors[1] = func_0204c040(wk->clactUnit, wk->res[1].chars, wk->res[0].palette, wk->res[1].cellAnims, &sActorSetups[1], 0,
                                  wk->heapId);
    wk->actors[2] = func_0204c040(wk->clactUnit, wk->res[1].chars, wk->res[0].palette, wk->res[1].cellAnims, &sActorSetups[2], 0,
                                  wk->heapId);
    wk->actors[3] = func_0204c040(wk->clactUnit, wk->res[1].chars, wk->res[0].palette, wk->res[1].cellAnims, &sActorSetups[3], 0,
                                  wk->heapId);
    func_0204c520(wk->actors[3], TRUE);
    wk->actors[4] = func_0204c040(wk->clactUnit, wk->res[2].chars, wk->res[2].palette, wk->res[2].cellAnims, iconSetup,
                                  0, wk->heapId);
    func_0204c378(wk->actors[4], 0, 1);
    wk->actors[5] = func_0204c040(wk->clactUnit, wk->res[0].chars, wk->res[0].palette, wk->res[0].cellAnims, &sActorSetups[6], 0,
                                  wk->heapId);
    func_0204c124(wk->actors[5], FALSE);
    func_0204c124(wk->actors[1], FALSE);
    func_0204c124(wk->actors[2], FALSE);
    func_0204c124(wk->actors[3], FALSE);
}

static void func_ov060_021e75b8(ResortShopWork *wk) {
    wk->clactUnit = func_0204bf1c(6, 1, wk->heapId);
    func_0204bfd4(wk->clactUnit, TRUE);
    func_ov060_021e7294(wk);
}

static void func_ov060_021e75dc(ResortShopWork *wk) {
    wk->iconArc = GFL_ArcSysCreateFileHandle(ARCID_ITEMGRA, wk->heapId);
    func_ov060_021e7318(wk);
    func_ov060_021e7470(wk, &sActorSetups[4]);
}

static void func_ov060_021e7604(ResortShopWork *wk) {
    wk->iconArc = GFL_ArcSysCreateFileHandle(31, wk->heapId);
    wk->res[2].palette = func_0204bbb8(wk->iconArc, 0, 0, 64, 2, 1, wk->heapId);
    wk->res[2].chars = func_0204b81c(wk->iconArc, 49, 0, 0, wk->heapId);
    wk->res[2].cellAnims = func_0204bde0(wk->iconArc, 65, 66, wk->heapId);
    func_ov060_021e7470(wk, &sActorSetups[5]);
}

static void func_ov060_021e766c(ResortShopWork *wk) {
    func_0204be64(wk->res[2].cellAnims);
    func_0204be64(wk->res[1].cellAnims);
    func_0204be64(wk->res[0].cellAnims);
    func_0204b98c(wk->res[2].chars);
    func_0204b98c(wk->res[1].chars);
    func_0204b98c(wk->res[0].chars);
    func_0204bcd0(wk->res[2].palette);
    func_0204bcd0(wk->res[0].palette);
    GFL_ArcToolFree(wk->iconArc);
    func_0204bf98(wk->clactUnit);
}

static void func_ov060_021e76d0(ResortShopWork *wk) {
    int i;

    for (i = 0; i < 7; i++) {
        const ResortShopWindowSetup *setup = sWindowSetups[i];

        wk->windows[i] = BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height,
                                              setup->palette, 1);
    }
    wk->printWindow.window = wk->windows[2];
    wk->printWindow.flushPending = FALSE;
    wk->printQueue = func_02021998(wk->heapId);
    BmpWin_TransferNow(wk->windows[1]);
}

static void func_ov060_021e7750(ResortShopWork *wk) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Money);

    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(wk->windows[0]), 0, 0, strbuf, wk->font, PRINT_COLOR(15, 2, 15));
    GFL_StrBufFree(strbuf);
    BmpWin_TransferNow(wk->windows[0]);
}

static void func_ov060_021e77a8(ResortShopWork *wk) {
    int i;

    func_02021c44(wk->printQueue);
    func_02021a18(wk->printQueue);
    for (i = 0; i < 7; i++) {
        BmpWin_Free(wk->windows[i]);
    }
}

// The player's money
static void func_ov060_021e77d4(ResortShopWork *wk) {
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty_6);
    StrBuf *strbuf = GFL_StrBufCreate(10, wk->heapId);
    GFLBitmap *bitmap;

    WordSetNumber(wk->wordSet, 0, getCash(wk->trainerCard), 7, 1, 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    bitmap = BmpWin_GetBitmap(wk->windows[1]);
    GFL_BitmapFill(bitmap, 0);
    GFL_TextRendererDrawToBitmapEx(bitmap, 0, 0, strbuf, wk->font, PRINT_COLOR(15, 2, 15));
    BmpWin_TransferNow(wk->windows[1]);
    GFL_StrBufFree(strbuf);
    GFL_StrBufFree(format);
}

// How many of the item the bag holds
static void func_ov060_021e786c(ResortShopWork *wk, u16 item) {
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Bag);
    StrBuf *strbuf = GFL_StrBufCreate(20, wk->heapId);
    GFLBitmap *bitmap;

    BmpWin_DrawFrame(wk->windows[3], WINFRAME_TRANSFER_NONE, 228, 9);
    WordSetNumber(wk->wordSet, 0, BagSave_GetItemCountByID(wk->bag, item, wk->heapId), 3, NUM_PAD_NONE, 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    bitmap = BmpWin_GetBitmap(wk->windows[3]);
    GFL_BitmapFill(bitmap, 15);
    GFL_TextRendererDrawToBitmapEx(bitmap, 0, 0, strbuf, wk->font, PRINT_COLOR(1, 2, 15));
    BmpWin_TransferNow(wk->windows[3]);
    GFL_StrBufFree(strbuf);
    GFL_StrBufFree(format);
}

// The description of the entry at the cursor
static void func_ov060_021e791c(ResortShopWork *wk, StrBuf *strbuf) {
    GFLBitmap *bitmap = BmpWin_GetBitmap(wk->windows[5]);

    GFL_BitmapFill(bitmap, 0);
    if (strbuf != NULL) {
        GFL_TextRendererDrawToBitmapEx(bitmap, 0, 0, strbuf, wk->font, PRINT_COLOR(15, 2, 0));
    }
    BmpWin_TransferNow(wk->windows[5]);
}

static void func_ov060_021e7974(ResortShopWork *wk, BmpMenuListCursorCallback cursorCallback,
                                BmpMenuListPrintCallback printCallback) {
    BmpMenuListHeader header;

    header.options = wk->options;
    header.cursorCallback = cursorCallback;
    header.printCallback = printCallback;
    header.count = ListMenuCore_GetFirstFreeIndex(wk->options);
    header.maxShown = 7;
    header.labelX = 0;
    header.itemX = 0;
    header.cursorX = 0;
    header.y = 8;
    header.fgColor = 12;
    header.bgColor = 0;
    header.shadowColor = 13;
    header.letterSpacing = 0;
    header.lineSpacing = 0;
    header.pageSkip = BMPMENULIST_SKIP_LR_KEY;
    header.fontId = 0;
    header.cursorDisplay = BMPMENULIST_CURSOR_HIDE;
    header.work = wk;
    header.fontSizeX = 12;
    header.fontSizeY = 16;
    header.msgData = NULL;
    header.printWindow = &wk->printWindow;
    header.queue = wk->printQueue;
    header.font = wk->font;
    header.wait = 0;
    wk->list = BmpMenuList_Create(&header, 0, 0, wk->heapId);
}

// Prints straight to the bitmap in the first row, and through the print queue below it
static void func_ov060_021e7a2c(PrintQueue *queue, GFLBitmap *bitmap, int x, int y, const StrBuf *strbuf, Font *font,
                                u16 color) {
    if (y <= 8) {
        GFL_TextRendererDrawToBitmapEx(bitmap, x, y, strbuf, font, color);
    } else {
        func_02021c7c(queue, bitmap, x, y, strbuf, font, color);
    }
}

// Prints an item's price, or that it was bought, at the right of its row
static void func_ov060_021e7a70(BmpMenuList *list, s32 value, u8 y) {
    ResortShopWork *wk = BmpMenuList_GetWork(list);
    s32 width;
    u32 windowWidth;

    if (value == BMPMENULIST_CANCEL) {
        return;
    }
    if (func_02036434(wk->flags, wk->entries[value].id)) {
        width = GFL_FontGetBlockWidth(wk->boughtLabel, wk->font, 0);
        windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[2]));
        func_ov060_021e7a2c(wk->printQueue, BmpWin_GetBitmap(wk->windows[2]), windowWidth - width, y, wk->boughtLabel,
                            wk->font, PRINT_COLOR(12, 13, 0));
        return;
    }
    WordSetNumber(wk->wordSet, 1, wk->entries[value].price, 5, 1, 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->message, wk->priceFormat);
    width = GFL_FontGetBlockWidth(wk->message, wk->font, 0);
    windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[2]));
    func_ov060_021e7a2c(wk->printQueue, BmpWin_GetBitmap(wk->windows[2]), windowWidth - width, y, wk->message, wk->font,
                        PRINT_COLOR(12, 13, 0));
}

static void func_ov060_021e7b68(ResortShopWork *wk) {
    u16 a1;
    u16 a2;

    BmpMenuList_Free(wk->list, &a1, &a2);
}

static BOOL func_ov060_021e7b7c(u32 event) {
    return FALSE;
}

// Puts an item's name and pocket in the word set, and prints a message
static void func_ov060_021e7b80(ResortShopWork *wk, u32 msgId, u16 item, u16 count) {
    u32 pocket = BagSave_GetActualItemPocket(wk->bag, item);

    loadItemText(wk->wordSet, 0, item, count != 1, FALSE);
    loadBagPocketNameToStrbuf(wk->wordSet, 1, pocket);
    func_ov060_021e7bc8(wk, msgId);
}

// Prints a message in a new frame, waiting for a button at its end
static void func_ov060_021e7bc8(ResortShopWork *wk, u32 msgId) {
    StrBuf *strbuf;
    BmpWin *window;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[6]), 15);
    BmpWin_DrawFrame(wk->windows[6], WINFRAME_TRANSFER_NOW, 228, 9);
    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->message, strbuf);
    GFL_StrBufFree(strbuf);
    wk->printStream = func_02022294(wk->windows[6], 0, 0, wk->message, wk->font, func_02017bcc(), wk->tcbManager, 0,
                                    wk->heapId, 0xffff, func_ov060_021e7b7c);
    if (wk->keyCursor != NULL) {
        KeyCursor_Free(wk->keyCursor);
        wk->keyCursor = NULL;
    }
    wk->keyCursor = KeyCursor_Create(15, TRUE, FALSE, wk->heapId);
    BmpWin_TransferNow(wk->windows[6]);
}

// The same without waiting for a button
static void func_ov060_021e7c94(ResortShopWork *wk, u32 msgId) {
    StrBuf *strbuf;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[6]), 15);
    BmpWin_DrawFrame(wk->windows[6], WINFRAME_TRANSFER_NOW, 228, 9);
    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->message, strbuf);
    GFL_StrBufFree(strbuf);
    wk->printStream = func_02022294(wk->windows[6], 0, 0, wk->message, wk->font, func_02017bcc(), wk->tcbManager, 0,
                                    wk->heapId, 0xffff, func_ov060_021e7b7c);
    BmpWin_TransferNow(wk->windows[6]);
}

static void func_ov060_021e7d3c(BmpWin *window) {
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    BmpWin_DrawFrame(window, WINFRAME_TRANSFER_NOW, 228, 9);
}

// func_ov060_021e7bc8 in the frame that is there already
static void func_ov060_021e7d58(ResortShopWork *wk, u32 msgId) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    GFL_WordSetFormatStrbuf(wk->wordSet, wk->message, strbuf);
    GFL_StrBufFree(strbuf);
    wk->printStream = func_02022294(wk->windows[6], 0, 0, wk->message, wk->font, func_02017bcc(), wk->tcbManager, 0,
                                    wk->heapId, 0xffff, NULL);
    if (wk->keyCursor != NULL) {
        KeyCursor_Free(wk->keyCursor);
        wk->keyCursor = NULL;
    }
    wk->keyCursor = KeyCursor_Create(15, TRUE, FALSE, wk->heapId);
    BmpWin_TransferNow(wk->windows[6]);
}

// A message about an entry, with its name, count and price in the word set
static void func_ov060_021e7e00(ResortShopWork *wk, u32 msgId, ResortShopEntry *entry) {
    func_ov060_021e7d3c(wk->windows[6]);
    if (entry->kind == 0 && wk->mode == 0) {
        loadItemNameToStrbuf(wk->wordSet, 0, entry->item);
    } else {
        func_0202437c(wk->wordSet, 0, entry->name, 2, 1, 2);
    }
    WordSetNumber(wk->wordSet, 1, entry->count, 2, NUM_PAD_NONE, 1);
    WordSetNumber(wk->wordSet, 2, entry->price, 7, NUM_PAD_NONE, 1);
    func_ov060_021e7d58(wk, msgId);
}

static void func_ov060_021e7e84(u32 which) {
    GFL_BGSysFillScrArea(1, 0, sListRects[which].x, sListRects[which].y, sListRects[which].width,
                         sListRects[which].height, 0);
}

static ResortPeople *func_ov060_021e7ecc(Field *field) {
    ResortPeople *people = NULL;

    if (Field_CheckGimmickWorkPassword(field, 0)) {
        people = func_ov137_021f0e74(field);
    } else if (Field_CheckGimmickWorkPassword(field, 1)) {
        people = func_ov137_021eeeac(field);
    }
    return people;
}

static void *func_ov060_021e7efc(Field *field) {
    void *npc = NULL;

    if (Field_CheckGimmickWorkPassword(field, 0)) {
        npc = func_ov137_021f0e8c(field);
    } else if (Field_CheckGimmickWorkPassword(field, 1)) {
        npc = func_ov137_021eeebc(field);
    }
    return npc;
}

static void func_ov060_021e7f2c(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    func_ov060_021e75dc(wk);
    func_ov060_021e688c(wk, mode, subMode);
    func_ov060_021e7974(wk, wk->cursorCallback, wk->printCallback);
}

static void func_ov060_021e7f58(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
}

// Moves the cursor's arrow, and shows the entry's description and icon
static void func_ov060_021e7f5c(BmpMenuList *list, s32 value, u8 a2) {
    ResortShopWork *wk = BmpMenuList_GetWork(list);
    u16 top;
    u16 cursor;
    ClActorPos pos;

    BmpMenuList_GetPos(list, &top, &cursor);
    pos.x = 172;
    pos.y = cursor * 16 + 22;
    func_0204c140(wk->actors[0], &pos, 0);
    if (value < NELEMS(wk->entries)) {
        func_ov060_021e791c(wk, wk->entries[value].description);
        if (wk->entries[value].kind == 0) {
            func_ov060_021e737c(wk, wk->entries[value].item);
        } else {
            func_ov060_021e737c(wk, 0xffff);
        }
    } else {
        func_ov060_021e791c(wk, NULL);
        func_ov060_021e737c(wk, 0xffff);
    }
}

static void func_ov060_021e7fe0(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    func_ov060_021e7604(wk);
    func_ov060_021e6a28(wk, mode, subMode);
    func_ov060_021e7974(wk, wk->cursorCallback, wk->printCallback);
}

static void func_ov060_021e800c(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
}

static void func_ov060_021e8010(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    func_ov060_021e7604(wk);
    func_ov060_021e6c90(wk, mode, subMode);
    func_ov060_021e7974(wk, wk->cursorCallback, wk->printCallback);
}

static void func_ov060_021e803c(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
}

// A person's shop, its level and the date, as the description of the person
static StrBuf *func_ov060_021e8040(ResortShopWork *wk, JoinAvenuePerson *person) {
    void *flags = func_02038470(person);
    StrBuf *strbuf = GFL_StrBufCreate(128, wk->heapId);
    u16 shop = ResortShopData_GetShopParam(ResortShopData_GetPersonShop(wk->shops, person), 0);
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(wk->heapId));
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, shop + 42);
    u16 a = joinAveTextHandler(person, 21, NULL);
    u16 b = joinAveTextHandler(person, 22, NULL);
    u16 c = joinAveTextHandler(person, 23, NULL);
    u32 level = func_020363e0(flags, 1);
    StrBuf *shopName = GFL_MsgDataLoadStrbufNew(msgData, shop + 280);

    func_0202437c(wk->wordSet, 1, shopName, 2, 1, 2);
    WordSetNumber(wk->wordSet, 2, level + 1, 2, NUM_PAD_NONE, 1);
    WordSetNumber(wk->wordSet, 4, a, 2, NUM_PAD_ZERO, 1);
    WordSetNumber(wk->wordSet, 5, b, 2, NUM_PAD_NONE, 1);
    WordSetNumber(wk->wordSet, 6, c, 2, NUM_PAD_NONE, 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    GFL_StrBufFree(shopName);
    GFL_StrBufFree(format);
    GFL_MsgDataFree(msgData);
    return strbuf;
}

// Makes a person's entry's strings, or remakes them
static void func_ov060_021e8190(ResortShopWork *wk, ResortPersonData *data, u32 index, ResortShopEntry *entry) {
    JoinAvenuePerson *person = func_ov137_021f1980(data);
    u16 flag = func_020363e0(func_02038470(person), 0);
    StrBuf *label = GFL_StrBufCreate(64, wk->heapId);
    StrBuf *name = ResortSys_FormatShopText(wk->shops, data, flag, wk->heapId);
    StrBuf *description = func_ov060_021e8040(wk, person);
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty_11);
    u8 gender = joinAveTextHandler(person, 2, NULL);

    WordSetNumber(wk->wordSet, 1, index + 1, 1, NUM_PAD_NONE, 1);
    func_0202437c(wk->wordSet, 0, name, gender, 1, 2);
    GFL_WordSetFormatStrbuf(wk->wordSet, label, format);
    GFL_StrBufFree(format);
    if (entry->label == NULL) {
        entry->label = label;
        entry->name = name;
        entry->description = description;
    } else {
        GFL_StrBufCopy(entry->label, label);
        GFL_StrBufCopy(entry->name, name);
        GFL_StrBufCopy(entry->description, description);
        GFL_StrBufFree(label);
        GFL_StrBufFree(name);
        GFL_StrBufFree(description);
    }
}

// Moves the cursor's arrow, and shows the entry's description and model
static void func_ov060_021e827c(BmpMenuList *list, s32 value, u8 a2) {
    ResortShopWork *wk = BmpMenuList_GetWork(list);
    u16 top;
    u16 cursor;
    ClActorPos pos;

    BmpMenuList_GetPos(list, &top, &cursor);
    pos.x = 172;
    pos.y = cursor * 16 + 22;
    func_0204c140(wk->actors[0], &pos, 0);
    if (value < NELEMS(wk->entries)) {
        func_ov060_021e791c(wk, wk->entries[value].description);
        func_ov060_021e7404(wk, wk->entries[value].item);
    } else {
        func_ov060_021e791c(wk, NULL);
        func_ov060_021e7404(wk, 0xffff);
    }
}

// The same, and moves the arrow of the entry being moved
static void func_ov060_021e82f0(BmpMenuList *list, s32 value, u8 a2) {
    ResortShopWork *wk = BmpMenuList_GetWork(list);
    u16 top;
    u16 cursor;
    ClActorPos pos;

    BmpMenuList_GetPos(list, &top, &cursor);
    pos.x = 172;
    pos.y = cursor * 16 + 22;
    func_0204c140(wk->actors[0], &pos, 0);
    if (value < NELEMS(wk->entries)) {
        func_ov060_021e791c(wk, wk->entries[value].description);
        func_ov060_021e7404(wk, wk->entries[value].item);
    } else {
        func_ov060_021e791c(wk, NULL);
        func_ov060_021e7404(wk, 0xffff);
    }
    if (wk->moving) {
        func_ov060_021e837c(wk);
    } else {
        func_0204c124(wk->actors[5], FALSE);
    }
}

// Puts the arrow of the entry being moved at its row, showing it while the row is on the screen
static void func_ov060_021e837c(ResortShopWork *wk) {
    u16 top;
    u16 cursor;
    ClActorPos pos;
    u32 rowHeight;
    int row;
    int y;

    BmpMenuList_GetPos(wk->list, &top, &cursor);
    rowHeight = BmpMenuList_GetParam(wk->list, BMPMENULIST_PARAM_ROW_HEIGHT);
    row = wk->moveCursor + wk->moveTop - top;
    y = row * rowHeight;
    pos.x = 172;
    pos.y = y + 22;
    func_0204c140(wk->actors[5], &pos, 0);
    if (pos.y == 6) {
        func_0204c488(wk->actors[5], 3);
    } else if (pos.y == 134) {
        func_0204c488(wk->actors[5], 2);
    } else {
        func_0204c488(wk->actors[5], 1);
    }
    if (pos.y > 0 && pos.y < 192) {
        func_0204c124(wk->actors[5], TRUE);
    } else {
        func_0204c124(wk->actors[5], FALSE);
    }
}

// Puts the records in the new order, keeping fields 0, 1 and 31 in their places
static void func_ov060_021e841c(ResortShopWork *wk, u8 mode, u8 subMode) {
    int pass;
    int i;
    u16 tmp;
    u8 record[0x58];

    if (!wk->moved) {
        return;
    }
    for (pass = 0; pass < wk->entryCount; pass++) {
        for (i = 0; i < wk->entryCount; i++) {
            if (i != wk->entries[i].id) {
                u16 otherId = wk->entries[wk->entries[i].id].id;
                void *a = func_0203888c(wk->occupants, wk->entries[i].id);
                u32 a1 = func_020385a8(a, 1, NULL);
                u16 a0 = func_020385a8(a, 0, NULL);
                u16 a31 = func_020385a8(a, 31, NULL);
                void *b = func_0203888c(wk->occupants, otherId);
                u32 b1 = func_020385a8(b, 1, NULL);
                u16 b0 = func_020385a8(b, 0, NULL);
                u16 b31 = func_020385a8(b, 31, NULL);

                func_02038680(a, 0, b0);
                func_02038680(a, 1, b1);
                func_02038680(a, 31, b31);
                func_02038680(b, 0, a0);
                func_02038680(b, 1, a1);
                func_02038680(b, 31, a31);
                sys_memcpy(a, record, sizeof(record));
                sys_memcpy(b, a, sizeof(record));
                sys_memcpy(record, b, sizeof(record));
                tmp = wk->order[i];
                wk->order[i] = wk->order[wk->entries[i].id];
                wk->order[wk->entries[i].id] = tmp;
                tmp = wk->entries[wk->entries[i].id].id;
                wk->entries[wk->entries[i].id].id = wk->entries[i].id;
                wk->entries[i].id = tmp;
            }
        }
    }
}

// Puts the people in the new order, keeping fields 0 and 1 in their places. The list runs from the last person
static void func_ov060_021e85b0(ResortShopWork *wk, u8 mode, u8 subMode) {
    int pass;
    int i;
    u16 tmp;
    u8 person[0xc4];

    if (!wk->moved) {
        return;
    }
    for (pass = 0; pass < wk->entryCount; pass++) {
        for (i = 0; i < wk->entryCount; i++) {
            int j = wk->entryCount - 1 - wk->entries[i].id;

            if (i != j) {
                u16 otherId = ResortShop_GetEntry(wk, j)->id;
                JoinAvenuePerson *a = func_02038860(wk->occupants, wk->entries[i].id);
                u32 a1 = joinAveTextHandler(a, 1, NULL);
                u16 a0 = joinAveTextHandler(a, 0, NULL);
                JoinAvenuePerson *b = func_02038860(wk->occupants, otherId);
                u32 b1 = joinAveTextHandler(b, 1, NULL);
                u16 b0 = joinAveTextHandler(b, 0, NULL);

                JoinAvenuePerson_SetParam(a, 0, b0);
                JoinAvenuePerson_SetParam(a, 1, b1);
                JoinAvenuePerson_SetParam(b, 0, a0);
                JoinAvenuePerson_SetParam(b, 1, a1);
                sys_memcpy(a, person, sizeof(person));
                sys_memcpy(b, a, sizeof(person));
                sys_memcpy(person, b, sizeof(person));
                tmp = wk->order[i];
                wk->order[i] = wk->order[j];
                wk->order[j] = tmp;
                tmp = wk->entries[j].id;
                wk->entries[j].id = wk->entries[i].id;
                wk->entries[i].id = tmp;
            }
        }
    }
}

static u8 func_ov060_021e8708(u32 index) {
    return data_ov036_021cf1c8[index].unk0[1]->unk0_0;
}

static void func_ov060_021e8720(ArcTool *handle, u16 objCode, u8 *texture, u16 *fileId) {
    FieldActorConfig record;
    u32 offset = GetIndexOfObjID(objCode) * sizeof(FieldActorConfig) + 4;

    GFL_ArcToolReadRange(handle, 0, offset, sizeof(record), &record);
    *texture = func_ov060_021e8708(record.spriteControllerType);
    *fileId = record.rscIndices.res1;
}

// A price with the avenue's discount, which grows with its rank up to 39 and is 40% from rank 40, 75% more with
// ResortSys_GetInfoFlag1b, and rounded to 10
static u32 func_ov060_021e8758(ResortShopWork *wk, u32 price) {
    fx32 rate;
    u32 rank;
    u32 discount;

    if (ResortSys_GetInfoFlag1b(wk->sys)) {
        rate = FX32_CONST(1.75);
    } else {
        rate = FX32_ONE;
    }
    rank = JoinAvenue_GetParam(wk->info, 2, 0);
    if (rank > 40) {
        rank = 40;
    }
    if (rank != 0 && rank < 40) {
        rank--;
    } else {
        rank = 40;
    }
    discount = price * rank / 100;
    price -= FX_Whole(FX_Mul(FX32_CONST(discount), rate));
    discount = price % 10;
    if (discount != 0) {
        if (discount >= 5) {
            price += 10 - discount;
        } else {
            price -= discount;
        }
    }
    return price;
}

// Points the camera at the shop, from the side of the avenue the player is on or the person's side
static void func_ov060_021e8804(ResortShop *shop, Field *field, u16 cameraMode) {
    FieldCamera *camera = Field_GetCameraSystem(field);
    int camIndex = 0;
    VecFx32 target;

    shop->useBoundary = FieldCamera_IsUseBoundaryEnable(camera);
    FieldCamera_SetUseBoundaryEnable(camera, FALSE);
    FieldCamera_ClearBind(camera);
    switch (cameraMode) {
    case 1: {
        JoinAvenuePerson *person = func_ov137_021f0f58(shop->work.person);

        for (camIndex = 0; camIndex < 8; camIndex++) {
            if (func_02038860(shop->work.occupants, camIndex) == person) {
                break;
            }
        }
        if (camIndex % 2) {
            camIndex = 2;
        } else {
            camIndex = 3;
        }
        break;
    }
    case 0:
    default: {
        FieldPlayer *player = Field_GetPlayer(field);
        s16 x;
        s16 y;
        s16 z;
        u32 i;

        GetPlayerGPosPlusDir(player, FieldPlayer_GetFaceDir(player), &x, &y, &z);
        for (i = 0; i < NELEMS(sAreas); i++) {
            if (sAreas[i].xMin <= x && x < sAreas[i].xMax && sAreas[i].zMin <= z && z < sAreas[i].zMax) {
                camIndex = sAreas[i].camera;
                break;
            }
        }
        break;
    }
    case 2:
        camIndex = 1;
        break;
    }
    FieldCamera_CoordsSetYaw(camera, sCameras[camIndex].yaw);
    FieldCamera_CoordsGetTarget(camera, &target);
    target.x += sCameras[camIndex].dx;
    target.z += sCameras[camIndex].dz;
    FieldCamera_CoordsSetTarget(camera, &target);
}

static BOOL func_ov060_021e8924(ResortShop *shop, Field *field, int *seq) {
    switch (*seq) {
    case 0:
        GFL_FadeSet(3, 0, 16, 1);
        (*seq)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            (*seq)++;
        }
        break;
    case 2:
        func_ov060_021e8804(shop, field, shop->work.cameraMode);
        ResortSys_HideOverlappingActors(shop->work.people, shop->work.npc, field, shop->work.person, 0);
        (*seq)++;
        break;
    case 3:
        func_ov060_021e6064(shop);
        (*seq)++;
        break;
    case 4:
        if (func_ov060_021e60ac(shop)) {
            (*seq)++;
        }
        break;
    case 5:
        GFL_FadeSet(3, 16, 0, 1);
        (*seq)++;
        break;
    case 6:
        if (!GFL_FadeIsRunning()) {
            *seq = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov060_021e89bc(ResortShop *shop, Field *field, int *seq) {
    FieldCamera *camera = Field_GetCameraSystem(field);

    switch (*seq) {
    case 0:
        GFL_FadeSet(3, 0, 16, 1);
        (*seq)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            (*seq)++;
        }
        break;
    case 2:
        func_ov137_021f152c(shop->work.people);
        func_ov137_021f1d04(shop->work.npc, 1);
        func_ov060_021e6100(shop);
        FieldCamera_LoadDefaults(camera);
        FieldCamera_ResetBind(camera);
        FieldCamera_SetUseBoundaryEnable(camera, shop->useBoundary);
        GFL_FadeSet(3, 16, 0, 1);
        (*seq)++;
        break;
    case 3:
        if (!GFL_FadeIsRunning()) {
            *seq = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}
