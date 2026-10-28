// Script commands between the Pokémon commands and field_rail.c: Castelia City's crowds, a name from a save block by
// the DS owner's birthday, the elevators, the item collectors and Anville Town's daily train. The ROM has no name for
// the file, and its commands may come from more than one; the item collectors and the day tables share one block of
// .rodata. scrcmd_misc.c is descriptive. Function names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0). Anville train names and collector group names are our descriptive names from the scripts and resources.
#include "types.h"
#include "constants/directions.h"
#include "constants/language.h"
#include "constants/pokemon.h"
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
#include "system/str_tool.h"
#include "system/vm.h"
#include "system/wordset.h"

#define ELEVATOR_FLOOR_MAX 8
#define ELEVATOR_TABLE_END 0xffff
#define ITEM_COLLECTOR_GROUP_COUNT 8
#define ARC_ITEM_COLLECTOR 205

// The world position of a tile
#define GRID_TO_FX32(n) ((n) * FX32_CONST(16))

// The groups bought by the collectors, identified by their price lists and script dialogue.
// The last three table slots also use the gourmet price list.
enum {
    ITEM_COLLECTOR_GOURMET,
    ITEM_COLLECTOR_MINERALS,
    ITEM_COLLECTOR_TREASURES,
    ITEM_COLLECTOR_RARE_BONES,
    ITEM_COLLECTOR_MULCH,
};

// The Anville trains: the ordinary ones that the last digit of the day picks, then the one of the special dates
enum {
    ANVILLE_TRAIN_SPECIAL = 10,
    ANVILLE_TRAIN_MAX,
};

// What an item collector pays for an item
typedef struct {
    u32 item;
    u32 price;
} ItemCollectorPrice;

static ElevatorFloor *ElevatorTable_GetEntry(ElevatorFloor *table, u32 index);
static void ElevatorTable_NoopWarpEntry(ElevatorFloor *floor);
static u32 ElevatorTable_CountEntries(ElevatorFloor *table);
static BOOL AnvilleTrain_IsSpecialDate(u32 month, u32 day);

// The archive member containing each collector group's item/price records.
static const u8 ITEM_COLLECTOR_FILES[ITEM_COLLECTOR_GROUP_COUNT] = {
    [ITEM_COLLECTOR_GOURMET] = 0,
    [ITEM_COLLECTOR_MINERALS] = 4,
    [ITEM_COLLECTOR_TREASURES] = 3,
    [ITEM_COLLECTOR_RARE_BONES] = 1,
    [ITEM_COLLECTOR_MULCH] = 2,
    [5] = 0,
    [6] = 0,
    [7] = 0,
};

// Dates that select Anville's special train, with the month in the high byte and the day in the low
static const u16 ANVILLE_SPECIAL_TRAIN_DATES[5] = { 0x0a01, 0x0a0e, 0x0c1e, 0x0201, 0x060c };

// The ordinary Anville train index, selected by the last digit of the day of the month
static const u8 ANVILLE_TRAIN_INDICES_BY_DAY_DIGIT[10] = { 9, 1, 3, 5, 4, 2, 6, 8, 0, 7 };

// The field prop model for each Anville train index; index 0 uses the same model as index 1.
static const u16 ANVILLE_TRAIN_MODEL_IDS[ANVILLE_TRAIN_MAX] = { 117, 117, 118, 133, 248, 249, 250, 251, 252, 253, 254 };

BOOL Scrcmd_Misc_CasteliaRushInit(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));

    FieldAsyncProcManager_AddProc(OVERLAY_CASTELIA_RUSH, Field_GetAsyncProcMgr(fieldWork->field),
                                  &CASTELIA_RUSH_PROC_DEF);
    return FALSE;
}

// Set a word to the name of the trainer log entry that the DS owner's birthday picks, with that trainer's gender
BOOL Scrcmd_Misc_ElevatorRegisterBirthdayEntry(VM *vm, FieldScriptEnv *env) {
    OSOwnerInfo ownerInfo;
    u16 wordIndex;
    TrainerLogSave *data;
    RTCDate *date;
    u32 index;
    u32 gender;
    const u16 *name;
    WordSet *wordSet;
    StrBuf *strBuf;
    GameData *gameData = FieldScriptEnv_GetGameData(env);

    wordIndex = ScriptReadAny(vm, env);
    data = TrainerLogSave_Get(GameData_GetSaveControl(gameData));
    date = getAddressAdventureTimeBlk(gameData);
    OS_GetOwnerInfo(&ownerInfo);
    index = TrainerLogSave_FindFortuneEntry(data, ownerInfo.birthday.month, ownerInfo.birthday.day, date);
    if (TrainerLogSave_IsEntryMale(data, index) == TRUE) {
        gender = GENDER_MALE;
    } else {
        gender = GENDER_FEMALE;
    }
    name = TrainerLogSave_GetEntryName(data, index);
    wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    strBuf = GFL_StrBufCreate(8, HEAPID_TAIL(HEAPID_USER));
    GFL_StrBufLoadString(strBuf, name);
    GFL_WordSetSetStrbuf(wordSet, wordIndex, strBuf, gender, TRUE, GAME_LANGUAGE);
    GFL_StrBufFree(strBuf);
    return FALSE;
}

BOOL Scrcmd_Misc_ElevatorGetMysteryEntryCount(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 *result = ScriptReadVar(vm, env);

    *result = TrainerLogSave_CountEntries(TrainerLogSave_Get(GameData_GetSaveControl(gameData)));
    return FALSE;
}

static ElevatorFloor *ElevatorTable_GetEntry(ElevatorFloor *table, u32 index) {
    return &table[index];
}

// Empty in the game, though called for each floor counted: perhaps a check left out of the release build
static void ElevatorTable_NoopWarpEntry(ElevatorFloor *floor) {
}

// The number of floors, or 0 if the table has no end
static u32 ElevatorTable_CountEntries(ElevatorFloor *table) {
    int i;

    for (i = 0; i < ELEVATOR_FLOOR_MAX; i++) {
        ElevatorFloor *floor = ElevatorTable_GetEntry(table, i);

        ElevatorTable_NoopWarpEntry(floor);
        if (floor->messageId == ELEVATOR_TABLE_END) {
            return i;
        }
    }
    return 0;
}

// The table of floors follows the command in the script
BOOL Scrcmd_Misc_ElevatorSetTablePtr(VM *vm, FieldScriptEnv *env) {
    s32 offset = VM_Read32(vm);

    FieldScriptEnv_SetElevatorTable(env, (ElevatorFloor *)(vm->pc + offset));
    return FALSE;
}

// A list of the floors other than this one
BOOL Scrcmd_Misc_ElevatorBuildListMenu(VM *vm, FieldScriptEnv *env) {
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

BOOL Scrcmd_Misc_ElevatorChangeMap(VM *vm, FieldScriptEnv *env) {
    VecFx32 pos;
    u16 index = ScriptReadAny(vm, env);
    ElevatorFloor *table = FieldScriptEnv_GetElevatorTable(env);

    // The current zone is read but not used
    GetScriptEnvZoneID(env);
    if (index < ElevatorTable_CountEntries(table)) {
        ElevatorFloor *floor = ElevatorTable_GetEntry(table, index);
        ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
        GameSystem *gsys = ScriptWork_GetGameSystem(work);
        Field *field = GSYS_GetField(gsys);
        u16 zoneId = floor->zoneId;
        u16 x = floor->x;
        u16 y = floor->y;
        u16 z = floor->z;

        pos.x = GRID_TO_FX32(x);
        pos.y = GRID_TO_FX32(y);
        pos.z = GRID_TO_FX32(z);
        ScriptWork_CallEvent(work, EventMapChange_CreateGridDefault(gsys, field, zoneId, &pos, DIR_DOWN));
    }
    return TRUE;
}

// What the item collector of the group pays for the item, or 0
BOOL Scrcmd_Misc_ItemCollectorGetPrice(VM *vm, FieldScriptEnv *env) {
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
BOOL Scrcmd_Misc_ItemCollectorCheckGroup(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    HeapID heapId = Field_GetHeapID(fieldWork->field);
    u16 group = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    BagSave *bag = GameData_GetBag(gameData);
    u32 size;
    ItemCollectorPrice *prices;
    u32 i;

    prices = GFL_ArcSysReadHeapNewLZGetLen(ARC_ITEM_COLLECTOR, ITEM_COLLECTOR_FILES[group], FALSE, HEAPID_TAIL(heapId),
                                           &size);
    size /= sizeof(ItemCollectorPrice);
    *result = FALSE;
    for (i = 0; i < size; i++) {
        if (BagSave_CheckAmount(bag, prices[i].item, 1, heapId)) {
            *result = TRUE;
            break;
        }
    }
    GFL_HeapFree(prices);
    return FALSE;
}

BOOL Scrcmd_Misc_WordSetLoadItemCollectorPrice(VM *vm, FieldScriptEnv *env) {
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
            WordSetNumber(wordSet, wordIndex, prices[i].price, digits, NUM_PAD_NONE, TRUE);
            break;
        }
    }
    GFL_HeapFree(prices);
    return FALSE;
}

BOOL Scrcmd_Misc_ItemCollectorSell(VM *vm, FieldScriptEnv *env) {
    u32 size;
    ItemCollectorPrice *prices;
    u32 i;
    GameData *gameData;
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    HeapID heapId = Field_GetHeapID(fieldWork->field);
    u16 item = ScriptReadAny(vm, env);
    u16 group = ScriptReadAny(vm, env);

    gameData = FieldScriptEnv_GetGameData(env);
    // The player's state is read but not used
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

// Resolve the script's train index to the field prop model used by BMChangeMdlID.
BOOL Scrcmd_Misc_AnvilleTrainGetModelID(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    if (index >= ANVILLE_TRAIN_MAX) {
        index = 0;
    }
    *result = ANVILLE_TRAIN_MODEL_IDS[index];
    return FALSE;
}

static BOOL AnvilleTrain_IsSpecialDate(u32 month, u32 day) {
    u32 i;
    u16 date = (month << 8) | day;

    for (i = 0; i < NELEMS(ANVILLE_SPECIAL_TRAIN_DATES); i++) {
        if (date == ANVILLE_SPECIAL_TRAIN_DATES[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

// The index also selects the train enthusiast's explanation in Anville Town.
BOOL Scrcmd_Misc_AnvilleTrainGetDailyIndex(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u32 month = GameData_GetMonth(gameData);
    s32 day = GameData_GetDay(gameData);
    u16 value;

    if (AnvilleTrain_IsSpecialDate(month, day) == TRUE) {
        value = ANVILLE_TRAIN_SPECIAL;
    } else {
        value = ANVILLE_TRAIN_INDICES_BY_DAY_DIGIT[day % 10];
    }
    *result = value;
    return FALSE;
}
