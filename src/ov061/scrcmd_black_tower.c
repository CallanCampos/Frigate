#include "types.h"
#include "field/battle_facility.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_script.h"
#include "field/infinity_dungeon.h"
#include "field/infinity_dungeon_gimmick.h"
#include "field/infinity_dungeon_rooms.h"
#include "gfl/random.h"
#include "gfl/str.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

// The script plugin of the Black Tower and White Treehollow (plugin 9), commands from 1000. The actors that the
// commands take are numbered from 0xb0

typedef struct {
    u32 unk0;
    u32 unk4;
} BlackTowerUnkPair;

// The one value here that differs by version
#ifdef BLACK2
#define AREA_VERSION_UNK 0x17
#else
#define AREA_VERSION_UNK 0x16
#endif

static const BlackTowerUnkPair *func_ov061_021e5f94(FieldScriptEnv *env);

static BOOL Scrcmd_BlackTower_CreateWalls(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    InfinityDungeon_CreateCurrentWalls(gsys, InfinityDungeonGimmick_GetDungeon(GSYS_GetField(gsys)));
    return FALSE;
}

static BOOL Scrcmd_BlackTower_ChangeRoom(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 a = ScriptReadAny(vm, env);
    u16 b = ScriptReadAny(vm, env);
    u16 c = ScriptReadAny(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    InfinityDungeonGimmick_CallRoomChange(work, gsys, InfinityDungeonGimmick_GetDungeon(GSYS_GetField(gsys)), a, c, b);
    return TRUE;
}

static BOOL Scrcmd_BlackTower_ChangeFloor(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 a = ScriptReadAny(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    InfinityDungeonGimmick_CallFloorChange(work, gsys, InfinityDungeonGimmick_GetDungeon(GSYS_GetField(gsys)), a);
    return TRUE;
}

static BOOL Scrcmd_BlackTower_SetProgress(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    SaveControl *save = GameData_GetSaveControl(gameData);
    u8 value = ScriptReadAny(vm, env);
    u16 unk = GameData_GetMyCity(gameData);

    KeyDataSave_SetProgress(getKeyDataBlkAddress(save), value, unk);
    return FALSE;
}

static BOOL Scrcmd_BlackTower_StartRun(VM *vm, FieldScriptEnv *env) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    u32 random = GFL_RandomMT();
    KeyDataSave *keyData = getKeyDataBlkAddress(save);
    TrainerGameInfoSave *gameInfo = getTrainerGameInfoAddress(save);

    KeyDataSave_ClearRun(keyData);
    KeyDataSave_SetMoney(keyData, getCash(gameInfo));
    KeyDataSave_SetSeed(keyData, random);
    return FALSE;
}

static BOOL Scrcmd_BlackTower_GetFloorInArea(VM *vm, FieldScriptEnv *env) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    u16 *var = ScriptReadVar(vm, env);

    *var = KeyDataSave_GetFloorInArea(getKeyDataBlkAddress(save));
    return FALSE;
}

static BOOL Scrcmd_BlackTower_GetArea(VM *vm, FieldScriptEnv *env) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    u16 *var = ScriptReadVar(vm, env);

    *var = KeyDataSave_GetArea(getKeyDataBlkAddress(save));
    return FALSE;
}

static BOOL Scrcmd_BlackTower_CallBattleEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u8 actor = GetActorUID(ScriptWork_GetParentActor(work)) - 0xb0;

    ScriptWork_CallEvent(work, InfinityDungeon_CreateBattleEvent(gsys, field, actor));
    return TRUE;
}

static BOOL Scrcmd_BlackTower_CallDoorEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u16 a = ScriptReadAny(vm, env);
    u16 b = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(work, InfinityDungeonGimmick_CreateDoorEvent(gsys, field, b, a));
    return TRUE;
}

static BOOL Scrcmd_BlackTower_CallMachineEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(work, InfinityDungeonGimmick_CreateMachineEvent(gsys, GSYS_GetField(gsys)));
    return TRUE;
}

static BOOL Scrcmd_BlackTower_CallLiftEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(work, InfinityDungeonGimmick_CreateLiftEvent(gsys, GSYS_GetField(gsys)));
    return TRUE;
}

static BOOL func_ov061_021e5a90(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = func_ov061_021e5f94(env)->unk0;
    return FALSE;
}

static BOOL Scrcmd_BlackTower_SetTrainerDefeated(VM *vm, FieldScriptEnv *env) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    u16 actor = ScriptReadAny(vm, env) - 0xb0;
    u16 set = ScriptReadAny(vm, env);
    KeyDataSave *keyData = getKeyDataBlkAddress(save);

    KeyDataSave_SetTrainerDefeated(keyData, KeyDataSave_GetFloorInArea(keyData), actor, set);
    return FALSE;
}

static BOOL Scrcmd_BlackTower_IsTrainerDefeated(VM *vm, FieldScriptEnv *env) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    u16 actor = ScriptReadAny(vm, env) - 0xb0;
    u16 *var = ScriptReadVar(vm, env);
    KeyDataSave *keyData = getKeyDataBlkAddress(save);

    *var = KeyDataSave_IsTrainerDefeated(keyData, KeyDataSave_GetFloorInArea(keyData), actor);
    return FALSE;
}

static BOOL Scrcmd_BlackTower_GetNpcScript(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    u16 *var = ScriptReadVar(vm, env);
    FieldActor *actor = ScriptWork_GetParentActor(FieldScriptEnv_GetScriptWork(env));
    SaveControl *save = GameData_GetSaveControl(gameData);
    u16 objCode = FldAct_GetObjCode(actor);
    InfinityDungeon *dungeon = InfinityDungeonGimmick_GetDungeon(GSYS_GetField(gsys));
    u8 index = GetActorUID(actor) - 0xb0;

    *var = InfinityDungeon_GetNpcScript(getKeyDataBlkAddress(save), dungeon, objCode, index);
    return FALSE;
}

static BOOL func_ov061_021e5bac(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = func_ov061_021e5f94(env)->unk4;
    return FALSE;
}

static BOOL func_ov061_021e5bc4(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 objCode = FldAct_GetObjCode(ScriptWork_GetParentActor(work));
    u16 *var = ScriptReadVar(vm, env);

    if (objCode == 0x32) {
        *var = 2;
    } else if (objCode == 0x33) {
        *var = 1;
    } else {
        *var = 0;
    }
    return FALSE;
}

static BOOL Scrcmd_BlackTower_SetSpecialTrainerFlag(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    Field *field = GSYS_GetField(gsys);
    u16 a = ScriptReadAny(vm, env);
    u16 set = ScriptReadAny(vm, env);
    KeyDataSave *keyData = getKeyDataBlkAddress(save);

    KeyDataSave_SetFlag(keyData, InfinityDungeon_FindSpecialTrainer(InfinityDungeonGimmick_GetDungeon(field), a), set);
    return FALSE;
}

static BOOL Scrcmd_BlackTower_GetSpecialTrainerFlag(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    SaveControl *save = GameData_GetSaveControl(gameData);
    u16 a = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    KeyDataSave *keyData = getKeyDataBlkAddress(save);

    *var =
        KeyDataSave_IsFlagSet(keyData, InfinityDungeon_FindSpecialTrainer(InfinityDungeonGimmick_GetDungeon(field), a));
    return FALSE;
}

static BOOL Scrcmd_BlackTower_GetGoalFloor(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    InfinityDungeon *dungeon = InfinityDungeonGimmick_GetDungeon(GSYS_GetField(gsys));
    u16 *var = ScriptReadVar(vm, env);
    u8 level = KeyDataSave_GetArea(getKeyDataBlkAddress(save));
    u32 unk = InfinityDungeon_GetGoalFloor(dungeon);

    *var = unk + KeyDataSave_GetAreaFirstFloor(level) + 1;
    return FALSE;
}

static BOOL Scrcmd_BlackTower_GetRoomNpcCount(VM *vm, FieldScriptEnv *env) {
    InfinityDungeon *dungeon = InfinityDungeonGimmick_GetDungeon(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    u16 *var = ScriptReadVar(vm, env);

    *var = InfinityDungeon_GetRoomNpcCount(dungeon->floors[dungeon->targetFloor].rooms[dungeon->targetRoom].type);
    return FALSE;
}

static BOOL Scrcmd_BlackTower_GetTrainerClass(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    HeapID heapId = Field_GetHeapID(GSYS_GetField(gsys));
    u16 a = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = BtlInst_GetRomTrainerClass(0x106, a, heapId);
    return FALSE;
}

static BOOL func_ov061_021e5da4(VM *vm, FieldScriptEnv *env) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    u16 *var = ScriptReadVar(vm, env);

    *var = func_020102d0(getKeyDataBlkAddress(save));
    return FALSE;
}

static BOOL func_ov061_021e5dd4(VM *vm, FieldScriptEnv *env) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    u8 value = ScriptReadAny(vm, env);

    func_02010334(getKeyDataBlkAddress(save), value);
    return FALSE;
}

static BOOL Scrcmd_BlackTower_GetTrainerId(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    InfinityDungeon *dungeon = InfinityDungeonGimmick_GetDungeon(GSYS_GetField(gsys));
    u8 actor = ScriptReadAny(vm, env) - 0xb0;
    u16 *var = ScriptReadVar(vm, env);

    *var = InfinityDungeon_GetTrainerId(dungeon, KeyDataSave_GetFloorInArea(getKeyDataBlkAddress(save)), actor);
    return FALSE;
}

static BOOL Scrcmd_BlackTower_GetVersion(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    InfinityDungeon *dungeon = InfinityDungeonGimmick_GetDungeon(GSYS_GetField(gsys));
    u16 *var = ScriptReadVar(vm, env);

    *var = InfinityDungeon_GetVersion(dungeon);
    return FALSE;
}

static BOOL Scrcmd_BlackTower_GetInfo(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    InfinityDungeon *dungeon = InfinityDungeonGimmick_GetDungeon(GSYS_GetField(gsys));
    u16 info = ScriptReadAny(vm, env);
    u16 arg = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    KeyDataSave *keyData =
        getKeyDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    u8 unk;
    u8 unk2;

    switch (info) {
    case 0:
        *var = KeyDataSave_GetAreaFirstFloor((u8)arg);
        break;
    case 1:
        *var = KeyDataSave_GetAreaFloorCount(arg);
        break;
    case 2:
        *var = KeyDataSave_GetRoom(keyData);
        break;
    case 6:
        *var = KeyDataSave_GetFloorInArea(keyData);
        break;
    case 3:
        *var = InfinityDungeon_CountTrainers(dungeon);
        break;
    case 4:
        *var = KeyDataSave_CountDefeatedTrainers(keyData);
        break;
    case 5:
        unk = KeyDataSave_GetRoom(keyData);
        *var = InfinityDungeon_IsBelowGoal(dungeon, KeyDataSave_GetFloorInArea(keyData), unk);
        break;
    case 9:
        *var = dungeon->rewardItem;
        break;
    case 10:
        *var = dungeon->rewardCount;
        break;
    case 11:
        unk = KeyDataSave_GetProgress(keyData, 0);
        unk2 = KeyDataSave_GetProgress(keyData, 1);
        if (unk == 0 && unk2 == 0) {
            *var = TRUE;
        } else {
            *var = FALSE;
        }
        break;
    }
    return FALSE;
}

// Used in the area where InfinityDungeon_GetVersion returns 0x17
static const BlackTowerUnkPair sUnkPairs17[] = {
    { 0xb8, 0x22 }, { 0xba, 0x23 }, { 0xbc, 0x24 }, { 0xbe, 0x25 },  { 0xc0, 0x20 },  { 0xc2, 0x1f },
    { 0xc4, 0x1e }, { 0xc6, 0x21 }, { 0xc8, 0x20 }, { 0xca, 0x130 }, { 0x1f9, 0x21 },
};

static const BlackTowerUnkPair sUnkPairs[] = {
    { 0xb7, 0x23 }, { 0xb9, 0x22 }, { 0xbb, 0x25 }, { 0xbd, 0x24 },  { 0xbf, 0x21 },  { 0xc1, 0x1e },
    { 0xc3, 0x1f }, { 0xc5, 0x20 }, { 0xc7, 0x21 }, { 0xc9, 0x130 }, { 0x1f8, 0x20 },
};

static const BlackTowerUnkPair *func_ov061_021e5f94(FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    InfinityDungeon *dungeon = InfinityDungeonGimmick_GetDungeon(GSYS_GetField(gsys));
    u8 level = KeyDataSave_GetArea(getKeyDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))));
    u32 area = InfinityDungeon_GetVersion(dungeon);
    const BlackTowerUnkPair *pairs = area == 0x17 ? sUnkPairs17 : sUnkPairs;

    if (level == 9 && area != AREA_VERSION_UNK) {
        level++;
    }
    return &pairs[level];
}

static BOOL Scrcmd_BlackTower_IsTargetSlot(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    InfinityDungeon *dungeon = InfinityDungeonGimmick_GetDungeon(GSYS_GetField(gsys));
    KeyDataSave *keyData = getKeyDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    u16 actor = ScriptReadAny(vm, env) - 0xb0;
    u16 *var = ScriptReadVar(vm, env);

    *var = InfinityDungeon_IsTargetSlot(dungeon, KeyDataSave_GetFloorInArea(keyData), actor);
    return FALSE;
}

static BOOL Scrcmd_BlackTower_GetNpcScriptByResult(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    u16 a = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    FieldActor *actor = ScriptWork_GetParentActor(FieldScriptEnv_GetScriptWork(env));
    SaveControl *save = GameData_GetSaveControl(gameData);
    u16 objCode = FldAct_GetObjCode(actor);
    InfinityDungeon *dungeon = InfinityDungeonGimmick_GetDungeon(GSYS_GetField(gsys));

    *var = InfinityDungeon_GetNpcScriptByResult(getKeyDataBlkAddress(save), dungeon, objCode, a);
    return FALSE;
}

static BOOL Scrcmd_BlackTower_HasEarnedMoney(VM *vm, FieldScriptEnv *env) {
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    KeyDataSave *keyData = getKeyDataBlkAddress(save);
    TrainerGameInfoSave *gameInfo = getTrainerGameInfoAddress(save);
    u16 *var = ScriptReadVar(vm, env);
    u32 cash = getCash(gameInfo);

    if (cash > KeyDataSave_GetMoney(keyData)) {
        *var = TRUE;
    } else {
        *var = FALSE;
    }
    return FALSE;
}

static BOOL Scrcmd_BlackTower_TakeEarnedMoney(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    KeyDataSave *keyData = getKeyDataBlkAddress(save);
    TrainerGameInfoSave *gameInfo = getTrainerGameInfoAddress(save);
    u16 wordIndex = ScriptReadAny(vm, env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u32 cash = getCash(gameInfo);
    u32 start = KeyDataSave_GetMoney(keyData);
    u32 amount;

    if (cash > start) {
        amount = cash - start;
        if (amount > 9999999) {
            amount = 9999999;
        }
        if (amount > cash) {
            amount = cash;
        }
        WordSetNumber(wordSet, wordIndex, amount, 7, 0, 1);
        subCashFromTotal(gameInfo, amount);
    }
    return FALSE;
}

const FieldScriptCommand BLACK_TOWER_SCRIPT_COMMANDS[] = {
    Scrcmd_BlackTower_CreateWalls,
    Scrcmd_BlackTower_ChangeRoom,
    NULL,
    Scrcmd_BlackTower_StartRun,
    Scrcmd_BlackTower_GetFloorInArea,
    Scrcmd_BlackTower_ChangeFloor,
    Scrcmd_BlackTower_GetArea,
    Scrcmd_BlackTower_CallBattleEvent,
    func_ov061_021e5a90,
    Scrcmd_BlackTower_SetTrainerDefeated,
    func_ov061_021e5bac,
    Scrcmd_BlackTower_IsTrainerDefeated,
    Scrcmd_BlackTower_GetNpcScript,
    func_ov061_021e5bc4,
    Scrcmd_BlackTower_GetSpecialTrainerFlag,
    Scrcmd_BlackTower_SetSpecialTrainerFlag,
    Scrcmd_BlackTower_CallDoorEvent,
    Scrcmd_BlackTower_GetGoalFloor,
    Scrcmd_BlackTower_GetTrainerClass,
    Scrcmd_BlackTower_SetProgress,
    func_ov061_021e5da4,
    func_ov061_021e5dd4,
    Scrcmd_BlackTower_GetTrainerId,
    Scrcmd_BlackTower_GetVersion,
    Scrcmd_BlackTower_GetInfo,
    Scrcmd_BlackTower_CallMachineEvent,
    Scrcmd_BlackTower_IsTargetSlot,
    Scrcmd_BlackTower_GetNpcScriptByResult,
    Scrcmd_BlackTower_GetRoomNpcCount,
    Scrcmd_BlackTower_HasEarnedMoney,
    Scrcmd_BlackTower_CallLiftEvent,
    Scrcmd_BlackTower_TakeEarnedMoney,
    (FieldScriptCommand)0xFFFFFFFF,
};
