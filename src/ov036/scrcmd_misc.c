// Script commands between the Pokémon commands and field_rail.c: Castelia City's crowds, a name from a save block by
// the DS owner's birthday, the elevators, the item collectors and two tables by day. The ROM has no name for the
// file, and its commands may come from more than one; the item collectors and the day tables share one block of
// .rodata. scrcmd_misc.c is descriptive. Function names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0)
#include "types.h"
#include "field/castelia_rush.h"
#include "field/ev_time.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "field/field_async_proc.h"
#include "field/field_script.h"
#include "field/scrcmd_misc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "nitro/os.h"
#include "save/bag.h"
#include "save/musical_savedata.h"
#include "save/save_control.h"
#include "save/trainer_log.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

#define ELEVATOR_FLOOR_MAX 8
#define ELEVATOR_TABLE_END 0xffff
#define ITEM_COLLECTOR_GROUP_COUNT 8
#define ARC_ITEM_COLLECTOR 205

// The world position of a tile
#define GRID_TO_FX32(n) ((n) * FX32_CONST(16))

// A floor of an elevator: its name, and where it goes
typedef struct {
    u16 messageId;
    u16 zoneId;
    u16 x;
    u16 y;
    u16 z;
} ElevatorFloor;

// What an item collector pays for an item
typedef struct {
    u32 item;
    u32 price;
} ItemCollectorPrice;

static ElevatorFloor *ElevatorTable_GetEntry(ElevatorFloor *table, u32 index);
static void func_ov036_021afdd0(ElevatorFloor *floor);
static u32 ElevatorTable_CountEntries(ElevatorFloor *table);
static BOOL func_ov036_021b01b8(u32 month, u32 day);

// The file of archive 205 of each item collector's prices
static const u8 ITEM_COLLECTOR_FILES[ITEM_COLLECTOR_GROUP_COUNT] = { 0, 4, 3, 1, 2, 0, 0, 0 };

// The days whose value is 10, as the month in the high byte and the day in the low
static const u16 SPECIAL_DAYS[5] = { 0x0a01, 0x0a0e, 0x0c1e, 0x0201, 0x060c };

// The value of the other days, by the last digit of the day
static const u8 DAY_DIGIT_VALUES[10] = { 9, 1, 3, 5, 4, 2, 6, 8, 0, 7 };

static const u16 data_ov036_021d0f80[11] = { 117, 117, 118, 133, 248, 249, 250, 251, 252, 253, 254 };

BOOL s0134_CasteliaRushInit(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));

    FieldAsyncProcManager_AddProc(OVERLAY_CASTELIA_RUSH, Field_GetAsyncProcMgr(fieldWork->field),
                                  &CASTELIA_RUSH_PROC_DEF);
    return FALSE;
}

// Set a word to the name of the entry of the DS owner's birthday, with its article unless it is flagged
BOOL func_ov036_021afcfc(VM *vm, FieldScriptEnv *env) {
    OSOwnerInfo ownerInfo;
    u16 wordIndex;
    TrainerLogSave *data;
    RTCDate *date;
    u32 index;
    u32 article;
    const u16 *name;
    WordSet *wordSet;
    StrBuf *strBuf;
    GameData *gameData = FieldScriptEnv_GetGameData(env);

    wordIndex = ScriptReadAny(vm, env);
    data = TrainerLogSave_Get(GameData_GetSaveControl(gameData));
    date = getAddressAdventureTimeBlk(gameData);
    OS_GetOwnerInfo(&ownerInfo);
    index = func_0200b05c(data, ownerInfo.birthday.month, ownerInfo.birthday.day, date);
    if (func_0200b014(data, index) == TRUE) {
        article = FALSE;
    } else {
        article = TRUE;
    }
    name = TrainerLogSave_GetEntryName(data, index);
    wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    strBuf = GFL_StrBufCreate(8, HEAPID_TAIL(HEAPID_USER));
    GFL_StrBufLoadString(strBuf, name);
    func_0202437c(wordSet, wordIndex, strBuf, article, 1, 2);
    GFL_StrBufFree(strBuf);
    return FALSE;
}

BOOL func_ov036_021afd9c(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 *result = ScriptReadVar(vm, env);

    *result = TrainerLogSave_CountEntries(TrainerLogSave_Get(GameData_GetSaveControl(gameData)));
    return FALSE;
}

static ElevatorFloor *ElevatorTable_GetEntry(ElevatorFloor *table, u32 index) {
    return &table[index];
}

static void func_ov036_021afdd0(ElevatorFloor *floor) {
}

// The number of floors, or 0 if the table has no end
static u32 ElevatorTable_CountEntries(ElevatorFloor *table) {
    int i;

    for (i = 0; i < ELEVATOR_FLOOR_MAX; i++) {
        ElevatorFloor *floor = ElevatorTable_GetEntry(table, i);

        func_ov036_021afdd0(floor);
        if (floor->messageId == ELEVATOR_TABLE_END) {
            return i;
        }
    }
    return 0;
}

// The table of floors follows the command in the script
BOOL s01C1_ElevatorSetTablePtr(VM *vm, FieldScriptEnv *env) {
    s32 offset = VM_Read32(vm);

    FieldScriptEnv_SetElevatorTable(env, (void *)(vm->pc + offset));
    return FALSE;
}

// A list of the floors other than this one
BOOL s01C2_ElevatorBuildListMenu(VM *vm, FieldScriptEnv *env) {
    ElevatorFloor *table = FieldScriptEnv_GetElevatorTable(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    StrBuf *expanded = ScriptWork_GetMainStrBuf(work);
    StrBuf *temp = ScriptWork_GetAltStrBuf(work);
    u16 zoneId = GetScriptEnvZoneID(env);
    u32 count = ElevatorTable_CountEntries(table);
    u32 i;

    for (i = 0; i < count; i++) {
        ElevatorFloor *floor = ElevatorTable_GetEntry(table, i);

        if (zoneId != floor->zoneId) {
            AddItemToListMenu(env, floor->messageId, 0xffff, i, expanded, temp);
        }
    }
    return FALSE;
}

BOOL s01C2_ElevatorChangeMap(VM *vm, FieldScriptEnv *env) {
    VecFx32 pos;
    u16 index = ScriptReadAny(vm, env);
    ElevatorFloor *table = FieldScriptEnv_GetElevatorTable(env);

    GetScriptEnvZoneID(env);
    if (index < ElevatorTable_CountEntries(table)) {
        ElevatorFloor *floor = ElevatorTable_GetEntry(table, index);
        ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
        GameSystem *gsys = ScriptWork_GetGameSystem(work);

        pos.x = GRID_TO_FX32(floor->x);
        pos.y = GRID_TO_FX32(floor->y);
        pos.z = GRID_TO_FX32(floor->z);
        ScriptWork_CallEvent(work, EventMapChange_CreateGridDefault(gsys, GSYS_GetField(gsys), floor->zoneId, &pos, 1));
    }
    return TRUE;
}

// What the item collector of the group pays for the item, or 0
BOOL s01FC_ItemCollectorGetPrice(VM *vm, FieldScriptEnv *env) {
    u32 size;
    u32 i;
    ItemCollectorPrice *prices;
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    HeapID heapId = Field_GetHeapID(fieldWork->field);
    u16 item = ScriptReadAny(vm, env);
    u16 group = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    prices = GFL_ArcSysReadHeapNewLZGetLen(ARC_ITEM_COLLECTOR, ITEM_COLLECTOR_FILES[group], FALSE, HEAPID_TAIL(heapId),
                                           &size);
    size /= sizeof(ItemCollectorPrice);
    *result = 0;
    for (i = 0; i < size; i++) {
        if (item == prices[i].item) {
            *result = prices[i].price;
            break;
        }
    }
    GFL_HeapFree(prices);
    return FALSE;
}

// Whether the player has any item that the item collector of the group buys
BOOL s01FB_ItemCollectorCheckGroup(VM *vm, FieldScriptEnv *env) {
    u32 size;
    u32 i;
    ItemCollectorPrice *prices;
    BagSave *bag;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    HeapID heapId = Field_GetHeapID(fieldWork->field);
    u16 group = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    bag = GameData_GetBag(gameData);
    prices = GFL_ArcSysReadHeapNewLZGetLen(ARC_ITEM_COLLECTOR, ITEM_COLLECTOR_FILES[group], FALSE, HEAPID_TAIL(heapId),
                                           &size);
    size /= sizeof(ItemCollectorPrice);
    *result = FALSE;
    for (i = 0; i < size; i++) {
        if (BagSave_CheckAmount(bag, (u16)prices[i].item, 1, heapId)) {
            *result = TRUE;
            break;
        }
    }
    GFL_HeapFree(prices);
    return FALSE;
}

BOOL s0236_WordSetLoadItemCollectorPrice(VM *vm, FieldScriptEnv *env) {
    u32 size;
    ItemCollectorPrice *prices;
    u32 i;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    HeapID heapId = Field_GetHeapID(fieldWork->field);
    u16 item = ScriptReadAny(vm, env);
    u16 group = ScriptReadAny(vm, env);
    u16 wordIndex = ScriptReadAny(vm, env);
    u16 digits = ScriptReadAny(vm, env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);

    prices = GFL_ArcSysReadHeapNewLZGetLen(ARC_ITEM_COLLECTOR, ITEM_COLLECTOR_FILES[group], FALSE, HEAPID_TAIL(heapId),
                                           &size);
    size /= sizeof(ItemCollectorPrice);
    for (i = 0; i < size; i++) {
        if (item == prices[i].item) {
            WordSetNumber(wordSet, wordIndex, prices[i].price, digits, 0, TRUE);
            break;
        }
    }
    GFL_HeapFree(prices);
    return FALSE;
}

BOOL s0237_ItemCollectorSell(VM *vm, FieldScriptEnv *env) {
    u32 size;
    ItemCollectorPrice *prices;
    u32 i;
    GameData *gameData;
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    HeapID heapId = Field_GetHeapID(fieldWork->field);
    u16 item = ScriptReadAny(vm, env);
    u16 group = ScriptReadAny(vm, env);

    gameData = FieldScriptEnv_GetGameData(env);
    GameData_GetPlayerState(gameData);
    prices = GFL_ArcSysReadHeapNewLZGetLen(ARC_ITEM_COLLECTOR, ITEM_COLLECTOR_FILES[group], FALSE, HEAPID_TAIL(heapId),
                                           &size);
    size /= sizeof(ItemCollectorPrice);
    for (i = 0; i < size; i++) {
        if (item == prices[i].item) {
            addCashToTotal(getTrainerCardDataBlkAddress(gameData), prices[i].price);
            break;
        }
    }
    GFL_HeapFree(prices);
    return FALSE;
}

BOOL func_ov036_021b018c(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    if (index >= 11) {
        index = 0;
    }
    *result = data_ov036_021d0f80[index];
    return FALSE;
}

static BOOL func_ov036_021b01b8(u32 month, u32 day) {
    u32 i;
    u16 date = (month << 8) | day;

    for (i = 0; i < 5; i++) {
        if (date == SPECIAL_DAYS[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL func_ov036_021b01e0(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u32 month = GameData_GetMonth(gameData);
    u32 day = GameData_GetDay(gameData);
    u16 value;

    if (func_ov036_021b01b8(month, day) == TRUE) {
        value = 10;
    } else {
        value = DAY_DIGIT_VALUES[day % 10];
    }
    *result = value;
    return FALSE;
}
