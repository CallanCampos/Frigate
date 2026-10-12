#include "types.h"
#include "battle/btl_result.h"
#include "field/event_battle_lose.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/field_map.h"
#include "field/field_script.h"
#include "field/player_state.h"
#include "field/scrcmd_game_state.h"
#include "field/trainer_script.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "gfl/random.h"
#include "nitro/fx.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/config.h"
#include "save/hall_of_fame.h"
#include "save/event_work.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/union_view.h"
#include "system/vm.h"

BOOL s00E0_GameGetVersion(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
#if defined(BLACK2)
    *value = 23;
#else
    *value = 22;
#endif
    return FALSE;
}

BOOL func_ov012_02155608(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    SaveControl *save;
    Config *trainerData;
    u32 mode;

    gameData = FieldScriptEnv_GetGameData(env);
    save = GameData_GetSaveControl(gameData);
    trainerData = (Config *)getTrainerDataBlkAddress(save);
    mode = func_02008a84(trainerData);
    if (mode == 0) {
        mode = 1;
    } else if (mode == 1) {
        mode = 0;
    }
    func_02008a8c(trainerData, mode);
    return FALSE;
}

BOOL func_ov012_02155638(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    SaveControl *save;
    Config *trainerData;
    u16 *value;

    gameData = FieldScriptEnv_GetGameData(env);
    save = GameData_GetSaveControl(gameData);
    trainerData = (Config *)getTrainerDataBlkAddress(save);
    value = ScriptReadVar(vm, env);
    *value = func_02008a84(trainerData);
    return FALSE;
}

BOOL s00CB_Random(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    u16 range = ScriptReadAny(vm, env);
    *value = GFL_RandomLC(range);
    return FALSE;
}

BOOL s00CC_RTGetTextFile(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    *value = GetFieldScriptMsgFileNo(env);
    return FALSE;
}

BOOL func_ov012_0215569c(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    u16 *value;

    gameData = FieldScriptEnv_GetGameData(env);
    value = ScriptReadVar(vm, env);
    *value = func_ov012_02169b78(gameData);
    return FALSE;
}

BOOL s00CD_RTCGetDayPart(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 *value = ScriptReadVar(vm, env);
    *value = GameData_GetDayPeriod(gameData);
    return FALSE;
}

BOOL s00CF_RTCGetWeekDay(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 *value = ScriptReadVar(vm, env);
    *value = getCurrentDayOfWeek(gameData);
    return FALSE;
}

BOOL s00D0_RTCGetDate(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 *month = ScriptReadVar(vm, env);
    u16 *day = ScriptReadVar(vm, env);
    *month = GameData_GetMonth(gameData);
    *day = GameData_GetDay(gameData);
    return FALSE;
}

BOOL s00D1_RTCGetTime(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 *hour = ScriptReadVar(vm, env);
    u16 *minute = ScriptReadVar(vm, env);
    *hour = getCurrentHour(gameData);
    *minute = getCurrentMinute(gameData);
    return FALSE;
}

BOOL s00D2_RTCGetSeason(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    *value = GameData_GetSeason(gameData);
    return FALSE;
}

BOOL s00D4_TrainerCardGetBirthDate(VM *vm, FieldScriptEnv *env) {
    u16 *month;
    u16 *day;
    u8 date[0x58];

    month = ScriptReadVar(vm, env);
    day = ScriptReadVar(vm, env);
    func_0207c3bc(date);
    *month = date[2];
    *day = date[3];
    return FALSE;
}

BOOL s00E1_TrainerCardGetSex(VM *vm, FieldScriptEnv *env) {
    u16 *value;
    GameData *gameData;
    PlayerState *state;

    value = ScriptReadVar(vm, env);
    gameData = FieldScriptEnv_GetGameData(env);
    state = GameData_GetPlayerState(gameData);
    *value = getTrainerGender(&state->playerInfo);
    return FALSE;
}

BOOL s00D5_TrainerCardHasBadge(VM *vm, FieldScriptEnv *env) {
    u16 *value;
    u16 badgeId;
    GameData *gameData;
    TrainerCardSave *card;

    value = ScriptReadVar(vm, env);
    badgeId = ScriptReadAny(vm, env);
    gameData = FieldScriptEnv_GetGameData(env);
    card = getTrainerCardDataBlkAddress(gameData);
    *value = isBadgeObtained(card, badgeId);
    return FALSE;
}

BOOL s00D6_TrainerCardAddBadge(VM *vm, FieldScriptEnv *env) {
    u16 badgeId;
    GameData *gameData;
    TrainerCardSave *card;
    RTCDate date;
    void *timeSig;

    badgeId = ScriptReadAny(vm, env);
    gameData = FieldScriptEnv_GetGameData(env);
    card = getTrainerCardDataBlkAddress(gameData);
    addBadge(card, badgeId);
    RTC_GetCachedDate(&date);
    timeSig = getTimeSigSaveBlock(gameData);
    setBadgeGetSecondsTime(timeSig, badgeId, date.year, date.month, date.day);
    return FALSE;
}

BOOL s00D7_TrainerCardGetBadgeCount(VM *vm, FieldScriptEnv *env) {
    u16 *value;
    GameData *gameData;
    TrainerCardSave *card;

    value = ScriptReadVar(vm, env);
    gameData = FieldScriptEnv_GetGameData(env);
    card = getTrainerCardDataBlkAddress(gameData);
    *value = getBadgeCount(card);
    return FALSE;
}

BOOL s0138_SaveDataGetStatus(VM *vm, FieldScriptEnv *env) {
    SaveControl *save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    u16 *present = ScriptReadVar(vm, env);
    u16 *status = ScriptReadVar(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    u32 a2 = 0;
    u32 a1 = 0;

    *present = SaveControl_IsDataAlreadyPresent(save);
    *status = SaveControl_GetStatus(save);
    func_020074b8(save, &a1, &a2);
    if (a1 * 3 < a2) {
        *result = 0;
    } else {
        *result = 1;
    }
    return FALSE;
}

BOOL s00D3_RTGetZoneID(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    *value = GetScriptEnvZoneID(env);
    return FALSE;
}

BOOL s00D9_FieldSetTeleportZone(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 zone = ScriptReadAny(vm, env);
    if (RangeCheckTeleportZone(zone) == TRUE) {
        SetCurrentTeleportOrDeathZone(gameData, zone);
    }
    return FALSE;
}

BOOL s00DB_FieldSetNextZoneHere(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PlayerState *playerState = GameData_GetPlayerState(gameData);
    VecFx32 *pos = PlayerState_GetWPos(playerState);
    u16 zone = PlayerState_GetZoneID(playerState);
    u16 direction = PlayerState_CalcDirection(playerState);
    ZoneSpawnInfo spawn;

    CreateZoneChangeData(&spawn, zone, direction, pos->x, pos->y, pos->z);
    GameData_SetNextZone(gameData, &spawn);
    return FALSE;
}

BOOL s00DC_FieldSetNextZone(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 zone = ScriptReadAny(vm, env);
    u16 direction = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    ZoneSpawnInfo spawn;

    CreateZoneChangeData(&spawn, zone, (s16)direction, x << 16, y << 16, z << 16);
    GameData_SetNextZone(gameData, &spawn);
    return FALSE;
}

BOOL s00DA_MapReplaceSetEvent(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 uid = ScriptReadAny(vm, env);
    u16 set = ScriptReadAny(vm, env);
    u16 reload = ScriptReadAny(vm, env);

    GameData_SetEventMapReplace(gameData, uid, set);
    if (reload != 0) {
        MapMatrix *matrix = GetMapMatrixSystem(gameData);
        PlayerState *playerState = GameData_GetPlayerState(gameData);
        u16 zoneId = PlayerState_GetZoneID(playerState);
        u16 matrixId = GetZoneMatrixId(zoneId);
        HeapID heapId = FieldScriptEnv_GetHeapID(env);

        MapMatrix_Load(matrix, matrixId, zoneId, heapId);
        MapMatrix_Patch(matrix, gsys, heapId);
    }

    return FALSE;
}

BOOL s00D8_MapReplaceIsEventSet(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 uid = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    *result = GameData_IsMapReplaceEventSet(gameData, uid);
    return FALSE;
}

BOOL s00DE_PokeDexRegist(VM *vm, FieldScriptEnv *env) {
    PokeDexSave *pokedex = GameData_GetPokedex(FieldScriptEnv_GetGameData(env));
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 kind = ScriptReadAny(vm, env);
    u16 species = ScriptReadAny(vm, env);
    PartyPkm *pkm = PokeParty_NewTempPkm(species, 1, 0xffffffff00000000ULL, heapId);

    switch (kind) {
    case 0:
        PokeDex_RegistPkm(pokedex, pkm);
        break;
    case 1:
        break;
    }

    GFL_HeapFree(pkm);
    return FALSE;
}

BOOL s00DF_PokeDexIsRegist(VM *vm, FieldScriptEnv *env) {
    PokeDexSave *pokedex = GameData_GetPokedex(FieldScriptEnv_GetGameData(env));
    u16 kind = ScriptReadAny(vm, env);
    u16 species = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    switch (kind) {
    case 0:
        *result = PokeDex_IsSeen(pokedex, species);
        break;
    case 1:
        *result = PokeDex_IsCaught(pokedex, species);
        break;
    default:
        *result = FALSE;
        break;
    }
    return FALSE;
}

BOOL s00DD_PokeDexGetCount(VM *vm, FieldScriptEnv *env) {
    PokeDexSave *pokedex = GameData_GetPokedex(FieldScriptEnv_GetGameData(env));
    u16 kind = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    switch (kind) {
    case 0:
        *result = PokeDex_GetSeenNoNational(pokedex);
        break;
    case 1:
        *result = PokeDex_GetCaughtNoNational(pokedex);
        break;
    default:
        *result = 0;
        break;
    }
    return FALSE;
}

BOOL s01C6_PokeDexGiveNational(VM *vm, FieldScriptEnv *env) {
    PokeDexSave *pokedex = GameData_GetPokedex(FieldScriptEnv_GetGameData(env));
    PokeDex_EnableHabitatList(pokedex);
    PokeDex_SetNationalObtained(pokedex);
    return FALSE;
}

BOOL s01C7_PokeDexHaveNational(VM *vm, FieldScriptEnv *env) {
    u16 *value = ScriptReadVar(vm, env);
    PokeDexSave *pokedex = GameData_GetPokedex(FieldScriptEnv_GetGameData(env));
    *value = PokeDex_IsNationalObtained(pokedex);
    return FALSE;
}

BOOL s01C8_PokeDexEnable(VM *vm, FieldScriptEnv *env) {
    PokeDexSave *pokedex = GameData_GetPokedex(FieldScriptEnv_GetGameData(env));
    givePlayerPokedex(pokedex);
    return FALSE;
}

BOOL s02D0_PokeDexEnableHabitatList(VM *vm, FieldScriptEnv *env) {
    PokeDexSave *pokedex = GameData_GetPokedex(FieldScriptEnv_GetGameData(env));
    PokeDex_EnableHabitatList(pokedex);
    return FALSE;
}

// The system UI flags that func_ov012_02155bec sets and clears
static const u32 sSystemUIFlags[1] = { 0x40 };

BOOL func_ov012_02155bec(VM *vm, FieldScriptEnv *env) {
    u16 mode = ScriptReadAny(vm, env);
    u16 index = ScriptReadAny(vm, env);

    switch (mode) {
    case 0:
        GCTX_HIDBlockSoftReset(sSystemUIFlags[index]);
        break;
    case 1:
        GCTX_HIDUnblockSoftReset(sSystemUIFlags[index]);
        break;
    }
    return FALSE;
}

BOOL func_ov012_02155c30(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    TrainerCardSave *card = getTrainerCardDataBlkAddress(gameData);

    setOneShotDRObtained(card, ScriptReadAny(vm, env), playerInfo);
    return FALSE;
}

BOOL func_ov012_02155c64(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    TrainerCardSave *card = getTrainerCardDataBlkAddress(gameData);
    u16 flag = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = isOneShotDRObtained(card, flag, playerInfo);
    return FALSE;
}

BOOL s00E2_SaveDataCheckRequired(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    SaveControl *save = GameData_GetSaveControl(gameData);
    Config *config = (Config *)getTrainerDataBlkAddress(save);
    u16 *value = ScriptReadVar(vm, env);

    switch (func_02008ac8(config)) {
    case 0:
        *value = 0;
        break;
    case 1:
        *value = 1;
        break;
    }
    return FALSE;
}

BOOL s00E3_GiveRunningShoes(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    EventWork_FlagSet(GameData_GetEventWork(gameData), 0x963);
    return FALSE;
}

BOOL func_ov012_02155d00(VM *vm, FieldScriptEnv *env) {
    u16 flags = ScriptReadAny(vm, env);
    BoxSaveAccessor *box = GameData_GetBoxSaveAccessor(FieldScriptEnv_GetGameData(env));

    if (flags == 1 || flags == 2) {
        BoxSaveAccessor_SetFlags(box, flags);
    }
    return FALSE;
}

BOOL func_ov012_02155d30(VM *vm, FieldScriptEnv *env) {
    u16 *flag = ScriptReadVar(vm, env);
    u16 *value = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PlayerState *playerState = GameData_GetPlayerState(gameData);
    void *timeSig = getTimeSigBlkAddress(GameData_GetSaveControl(gameData));

    *value = func_0202b5e8(func_02008bf4(&playerState->playerInfo));
    *flag = func_020091d0(timeSig);
    func_020091dc(timeSig);
    return FALSE;
}

BOOL func_ov012_02155d80(VM *vm, FieldScriptEnv *env) {
    u16 item = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = (func_0200ca50(getTrainerCardDataBlkAddress(FieldScriptEnv_GetGameData(env)), item) & 1) ? TRUE : FALSE;
    return FALSE;
}


BOOL func_ov012_02155db4(VM *vm, FieldScriptEnv *env) {
    u16 item = ScriptReadAny(vm, env);

    func_0200ca38(getTrainerCardDataBlkAddress(FieldScriptEnv_GetGameData(env)), item, 1);
    return FALSE;
}

BOOL func_ov012_02155dd4(VM *vm, FieldScriptEnv *env) {
    u16 item = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    if (func_0200ca50(getTrainerCardDataBlkAddress(FieldScriptEnv_GetGameData(env)), item) == 3) {
        *result = TRUE;
    } else {
        *result = FALSE;
    }
    return FALSE;
}

BOOL s00EA_HOFCheckIntegrity(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    u16 *result = ScriptReadVar(vm, env);
    SaveControl *save;

    if (EventWork_FlagGet(eventWork, 0x960)) {
        save = GameData_GetSaveControl(gameData);
        switch (func_020074ec(save, 8, HEAPID_GAMEEVENT)) {
        case 0:
            *result = 0;
            break;
        case 1:
        case 2:
            if (HallOfFameSave_GetTeamCount(getAddressOfExtraSaveBlk(save, 8, 0)) == 0) {
                *result = 0;
            } else {
                *result = 1;
            }
            break;
        default:
            *result = 2;
            break;
        }
        freeIntermediateSaveExtraBlksAfterLoad(save, 8);
    } else {
        *result = 0;
    }
    return FALSE;
}

BOOL s01D7_ObjInitPointGPos(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);
    s16 x = ScriptReadAny(vm, env);
    s16 z = ScriptReadAny(vm, env);
    s16 y = ScriptReadAny(vm, env);

    SetBGEntityLocation(GameData_GetEventData(FieldScriptEnv_GetGameData(env)), index, x, z, y);
    return FALSE;
}

BOOL s01D8_ObjInitWarpGPos(VM *vm, FieldScriptEnv *env) {
    u16 warpId = ScriptReadAny(vm, env);
    s16 x = ScriptReadAny(vm, env);
    s16 y = ScriptReadAny(vm, env);
    s16 z = ScriptReadAny(vm, env);

    SetZoneWarpLocation(GameData_GetEventData(FieldScriptEnv_GetGameData(env)), warpId, x, y, z);
    return FALSE;
}

BOOL s01D9_ObjInitNPCGPos(VM *vm, FieldScriptEnv *env) {
    u16 npcId = ScriptReadAny(vm, env);
    u16 direction = ScriptReadAny(vm, env);
    s16 x = ScriptReadAny(vm, env);
    s16 y = ScriptReadAny(vm, env);
    s16 z = ScriptReadAny(vm, env);

    SetZoneNPCLocation(GameData_GetEventData(FieldScriptEnv_GetGameData(env)), npcId, direction, x, y << 16, z);
    return FALSE;
}

BOOL s0095_TrainerFlagSet(VM *vm, FieldScriptEnv *env) {
    EventWork *eventWork = GameData_GetEventWork(FieldScriptEnv_GetGameData(env));
    u16 trainerId = ScriptReadAny(vm, env);
    setTrainerBattleFlag(eventWork, trainerId);
    return FALSE;
}

BOOL s0096_TrainerFlagReset(VM *vm, FieldScriptEnv *env) {
    EventWork *eventWork = GameData_GetEventWork(FieldScriptEnv_GetGameData(env));
    u16 trainerId = ScriptReadAny(vm, env);
    clearTrainerBattleFlag(eventWork, trainerId);
    return FALSE;
}

BOOL s0097_TrainerFlagGet(VM *vm, FieldScriptEnv *env) {
    EventWork *eventWork = GameData_GetEventWork(FieldScriptEnv_GetGameData(env));
    u16 trainerId = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    *result = TrainerFlagGet(eventWork, trainerId);
    return FALSE;
}

BOOL s008C_CallTrainerLose(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptWork_SetPostEvent(work, EventBattleLose_Create(gsys));
    VM_Halt(vm);
    return TRUE;
}

BOOL s008D_TrainerBattleIsVictory(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u32 battleResult = GameData_GetLastBtlResult(gameData);
    if (IsBattleResultDefeat(battleResult, 1) == TRUE) {
        *result = 0;
    } else {
        *result = 1;
    }
    return FALSE;
}

BOOL s0176_CallWildLose(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptWork_SetPostEvent(work, EventBattleLose_Create(gsys));
    VM_Halt(vm);
    return TRUE;
}

BOOL s0177_WildBattleIsVictory(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u32 battleResult = GameData_GetLastBtlResult(gameData);
    if (IsBattleResultDefeat(battleResult, 0) == TRUE) {
        *result = 0;
    } else {
        *result = 1;
    }
    return FALSE;
}

BOOL s0178_WildBattleGetResult(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u32 battleResult = GameData_GetLastBtlResult(gameData);
    u16 *result = ScriptReadVar(vm, env);
    *result = GetWildBattleResultByCombined(battleResult);
    return FALSE;
}

// The values that func_ov012_02156104 and func_ov012_02156128 give for an index
static const u16 data_ov012_0216c9e4[8] = { 0xf, 0x37, 0x17, 0x34, 0x35, 0x40, 0x12f, 0x128 };
static const u16 data_ov012_0216c9f4[8] = { 0x2f, 0x8a, 0x323, 0x325, 0xa, 0x88, 0x324, 0x326 };

BOOL func_ov012_02156104(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);

    *ScriptReadVar(vm, env) = data_ov012_0216c9e4[index];
    return FALSE;
}

BOOL func_ov012_02156128(VM *vm, FieldScriptEnv *env) {
    u16 index = ScriptReadAny(vm, env);

    *ScriptReadVar(vm, env) = data_ov012_0216c9f4[index];
    return FALSE;
}

BOOL s02D2_FieldOpenRestoreLCD(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptWork_CallEvent(work, EventFieldOpenRestoreLCD_Create(gsys));
    return TRUE;
}
