#include "system/game_data.h"
#include "types.h"
#include "constants/directions.h"
#include "field/calender.h"
#include "field/encounter.h"
#include "field/event_data.h"
#include "field/field_actor.h"
#include "field/field_map.h"
#include "field/field_sound.h"
#include "field/field_status.h"
#include "field/musical.h"
#include "field/pair_sys.h"
#include "field/player_state.h"
#include "field/unity_tower.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "save/bag.h"
#include "save/box.h"
#include "save/bsubway_save.h"
#include "save/chatter.h"
#include "save/config.h"
#include "save/field_actor_savedata.h"
#include "save/gimmick_savedata.h"
#include "save/high_link.h"
#include "save/key_info.h"
#include "save/player_info.h"
#include "save/player_savedata.h"
#include "save/playtime.h"
#include "save/resh_zek.h"
#include "save/save_control.h"
#include "save/shortcut.h"
#include "save/timesig_savedata.h"
#include "save/trainer_data_savedata.h"
#include "save/unity_tower_floor_savedata.h"
#include "save/wbt_save.h"
#include "save/wifi_list.h"
#include "system/beacon_status.h"
#include "system/comm_player_support.h"
#include "system/resort_work.h"
#include "system/season.h"
#include "system/shortcut_util.h"
#include "system/wih_dwc.h"

// The game's data while it runs: the save control, the works the field and its applications share, and the copies
// of the save's blocks that the report writes back. Names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0) where the functions have them; the fields' names are ours

#define SAVE_BLOCK_BAG 0x19
#define SAVE_BLOCK_PARTY 0x1a
#define SAVE_BLOCK_RECORDS 0x26
#define SAVE_BLOCK_FIELD_ACTORS 0x29
#define SAVE_BLOCK_MUSICAL 0x2a
#define SAVE_BLOCK_EVENT_WORK 0x2d
#define SAVE_BLOCK_STRENGTH_ROCKS 0x33
#define SAVE_BLOCK_40 0x40
#define SAVE_BLOCK_TRADED_POKEMON 0x41

// The players of the game data: the four of a communication, by net ID, then the player's own
#define GAME_DATA_PLAYER_MAX 5
#define GAME_DATA_PLAYER_MINE 4
// The pass power levels are kept for the four net IDs, and the cities for the other player and the player's own
#define GAME_DATA_LEVELS_MAX 4
#define GAME_DATA_LEVELS_MINE 3
#define GAME_DATA_CITY_MAX 2
#define GAME_DATA_CITY_MINE 1

#define GAME_DATA_PICK_MAX 3
#define GAME_DATA_PICK_TRIES 100
#define GAME_DATA_COUNTER_MAX 99

// The respawn locations of Black City and White Forest
#define RETURN_LOCATION_BLACK_CITY 15
#define RETURN_LOCATION_WHITE_FOREST 16

#define SAVE_STATUS_NEW 1
// What the stepwise save returns once it ends
#define SAVE_RESULT_OK 2
#define SAVE_RESULT_NG 3

#define SLEEP_BLOCK_SAVE 0x80
#define SOFT_RESET_BLOCK_SAVE 0x2

struct GameData {
    SaveControl *saveControl;
    PlayerState playerStates[GAME_DATA_PLAYER_MAX];
    EventData *eventData;
    PlayerSave *playerSave;
    ZoneSpawnInfo *nowSpawn;
    ZoneSpawnInfo *outboundWarpSpawn;
    ZoneSpawnInfo *nextSpawn;
    ZoneSpawnInfo *escapeRopeSpawn;
    ZoneSpawnInfo entralinkParentSpawn;
    // The battle bag's cursor memory (save/bag.h)
    void *bagCursor;
    BagSave *bag;
    PokeParty *party;
    Config *config;
    BoxSaveAccessor *boxes;
    WifiList *wifiList;
    BGMInfo *bgmInfo;
    MMSys *mmSys;
    EventWork *eventWork;
    MapMatrix *mapMatrix;
    FieldSound *fieldSound;
    FieldStatus *fieldStatus;
    u32 lastBtlResult;
    // Set while the save is being written
    u8 saving;
    u8 unk1C1;
    u8 entreeForestArea;
    u32 stepTotal;
    u8 season;
    u8 weather;
    u8 lastSubscreen;
    u8 unk1CB;
    u8 cGearPowerRequest;
    u8 savedCGearPowerRequest;
    u8 skipFrame;
    u8 is30FPSMode;
    u8 playerCount;
    u8 netId;
    u8 forceSeasonSync;
    u8 beaconNetId;
    u8 padding1D4[3];
    u8 counter;
    PassPowerLevel levels[GAME_DATA_LEVELS_MAX];
    CityState cityStates[GAME_DATA_CITY_MAX];
    EncountState *encountState;
    ShortcutMenuPos shortcutMenuPos;
    GimmickState gimmickState;
    PleasureBoat *pleasureBoat;
    CommPlayerSupport *commPlayerSupport;
    BSubwayScrWork *bsubwayScrWork;
    BeaconStatus *beaconStatus;
    Calendar *calendar;
    UnityTowerFloor unityTowerFloor;
    TrialHouseWork *trialHouseWork;
    MusicalCommWork *musicalCommWork;
    WbtSystem *wbtSystem;
    PokewoodSystem *pokewoodSystem;
    PlayTime *playTime;
    FieldFollowWk *followWk;
    u32 scrPluginNo;
    // Three different values of sPickValues, drawn when the game data is created
    u32 picks[GAME_DATA_PICK_MAX];
    u32 lensFlareEntryIdx;
    u8 lensFlareRequested;
    u8 unk30D;
    u8 unk30E;
    u8 funfestMission;
    JoinAvenuePersonList *joinAvenuePersonList;
    ResortWork *resortWork;
}; // size: 0x318

static void PlayerState_Init(PlayerState *playerState);
static void GameData_LoadBackup(GameData *gameData);
static void GameData_SaveBackup(GameData *gameData);

// The player's rotation facing each direction
static const u16 DIRECTION_TO_3D_ANGLE[] = {
    0x0000, // DIR_UP
    0x8000, // DIR_DOWN
    0x4000, // DIR_LEFT
    0xc000, // DIR_RIGHT
};

static const u32 sPickValues[] = {
    62, 96, 107, 406, 412, 458, 329, 331, 337, 348, 365, 368, 370, 374, 383,
};

GameData *GameData_Create(HeapID heapId) {
    int i;
    u32 size;
    PlayerInfo *playerInfo;
    GameData *gameData = GFL_HeapAllocate(heapId, sizeof(GameData), TRUE, "game_data.c", 207);

    gameData->saveControl = SaveControl_GetInstance();
    gameData->season = Season_GetRealTime();
    gameData->lastSubscreen = 0;
    gameData->cGearPowerRequest = FALSE;
    gameData->savedCGearPowerRequest = FALSE;

    gameData->playerSave = SaveControl_GetPlayerSave(gameData->saveControl);
    gameData->nowSpawn = PlayerSave_GetNowSpawnZone(gameData->playerSave);
    gameData->outboundWarpSpawn = PlayerSave_GetNowSpawnZone(gameData->playerSave);
    gameData->nextSpawn = PlayerSave_GetNextSpawnZone(gameData->playerSave);
    gameData->escapeRopeSpawn = PlayerSave_GetEscapeRopeZone(gameData->playerSave);

    gameData->bgmInfo = BGMInfo_Create(heapId);

    for (i = 0; i < GAME_DATA_PLAYER_MAX; i++) {
        PlayerState_Init(&gameData->playerStates[i]);
    }
    size = PlayerInfo_GetSize();
    playerInfo = GetGameDataPlayerInfo(gameData);
    sys_memcpy(SaveControl_GetPlayerInfo(gameData->saveControl), playerInfo, size);

    gameData->eventData = EventData_Create(heapId);
    EventData_Reset(gameData->eventData);

    gameData->eventWork = getConstDataBlock(gameData->saveControl);
    gameData->mapMatrix = InitMapMatrix(heapId);
    gameData->mmSys = FldActSys_Create(heapId, 64,
                                       SaveControl_GetBlockPtr(gameData->saveControl, SAVE_BLOCK_STRENGTH_ROCKS));
    gameData->fieldSound = FieldSnd_Create(gameData, heapId);
    gameData->fieldStatus = FieldStatus_Create(heapId);

    gameData->bagCursor = BagCursor_Create(heapId);
    func_020088ec(gameData->bagCursor, 0);
    gameData->bag = SaveControl_GetBlockPtr(gameData->saveControl, SAVE_BLOCK_BAG);
    gameData->party = SaveControl_GetBlockPtr(gameData->saveControl, SAVE_BLOCK_PARTY);
    gameData->config = &getTrainerDataBlkAddress(gameData->saveControl)->config;
    gameData->boxes = BoxSaveAccessor_Create(heapId, gameData->saveControl);
    gameData->wifiList = populateWifiList(heapId);

    for (i = 0; i < GAME_DATA_LEVELS_MAX; i++) {
        setupProbabilityTable(&gameData->levels[i]);
    }
    for (i = 0; i < GAME_DATA_CITY_MAX; i++) {
        func_ov012_0215cd8c(&gameData->cityStates[i]);
    }

    gameData->playTime = setupPlaytimeCounters(heapId);
    gameData->stepTotal = 0;
    SaveControl_GetStatus(gameData->saveControl);
    gameData->encountState = EncountState_Create(heapId);
    ShortcutMenuPos_Init(&gameData->shortcutMenuPos);
    gameData->followWk = InitFieldFollowWk(heapId);

    GameData_LoadBackup(gameData);

    gameData->pleasureBoat = NULL;
    gameData->bsubwayScrWork = NULL;
    gameData->commPlayerSupport = CommPlayerSupport_Create(heapId);
    gameData->beaconStatus = BeaconStatus_Create(heapId, HEAPID_USER | HEAPID_TAIL_BIT);
    gameData->calendar = Calendar_Create(gameData, heapId);
    gameData->trialHouseWork = NULL;
    gameData->musicalCommWork = NULL;
    gameData->wbtSystem = NULL;
    gameData->pokewoodSystem = NULL;
    gameData->scrPluginNo = 0;
    gameData->resortWork = ResortWork_Create(heapId, gameData->saveControl);
    gameData->unk30D = 0xff;
    gameData->lensFlareEntryIdx = 67;

    func_02017a78(gameData);
    return gameData;
}

void GameData_Free(GameData *gameData) {
    ResortWork_Free(gameData->resortWork);
    FreeFieldFollowWk(gameData->followWk);
    GFL_HeapFree(gameData->playTime);
    Calendar_Free(gameData->calendar);
    BeaconStatus_Free(gameData->beaconStatus);
    CommPlayerSupport_Free(gameData->commPlayerSupport);
    EncountState_Free(gameData->encountState);
    BGMInfo_Free(gameData->bgmInfo);
    GFL_HeapFree(gameData->wifiList);
    BoxSaveAccessor_Free(gameData->boxes);
    GFL_HeapFree(gameData->bagCursor);
    FldActSys_Free(gameData->mmSys);
    FreeMapMatrix(gameData->mapMatrix);
    EventData_Free(gameData->eventData);
    FieldStatus_Free(gameData->fieldStatus);
    FieldSnd_Free(gameData->fieldSound);
    GFL_HeapFree(gameData);
}

PlayerState *func_020171e8(GameData *gameData, s32 index) {
    return &gameData->playerStates[index];
}

PlayerState *GameData_GetPlayerState(GameData *gameData) {
    return &gameData->playerStates[GAME_DATA_PLAYER_MINE];
}

void *func_020171fc(GameData *gameData, u32 netId) {
    return &gameData->levels[netId];
}

void *func_02017208(GameData *gameData) {
    return func_020171fc(gameData, GAME_DATA_LEVELS_MINE);
}

CityState *GameData_GetMyCityState(GameData *gameData) {
    return func_0201722c(gameData, GAME_DATA_CITY_MINE);
}

u16 func_02017220(GameData *gameData) {
    return GameData_GetMyCityState(gameData)->city;
}

CityState *func_0201722c(GameData *gameData, u32 index) {
    return &gameData->cityStates[index];
}

WifiList *GameData_GetWifiList(GameData *gameData) {
    return gameData->wifiList;
}

EventData *GameData_GetEventData(GameData *gameData) {
    return gameData->eventData;
}

ZoneSpawnInfo *GetGameDataNowSpawnZone(GameData *gameData) {
    return gameData->nowSpawn;
}

void SetGameDataNowSpawnZone(GameData *gameData, ZoneSpawnInfo *spawn) {
    *gameData->nowSpawn = *spawn;
}

ZoneSpawnInfo *GetOutboundWarpRememberSpawnInfo(GameData *gameData) {
    return gameData->outboundWarpSpawn;
}

void SetGameDataOutboundWarpRememberInfo(GameData *gameData, ZoneSpawnInfo *spawn) {
    *gameData->outboundWarpSpawn = *spawn;
}

ZoneSpawnInfo *GameData_GetNextZone(GameData *gameData) {
    return gameData->nextSpawn;
}

void GameData_SetNextZone(GameData *gameData, ZoneSpawnInfo *spawn) {
    *gameData->nextSpawn = *spawn;
}

ZoneSpawnInfo *GameData_GetEscapeRopeZone(GameData *gameData) {
    return gameData->escapeRopeSpawn;
}

void GameData_SetEscapeRopeZone(GameData *gameData, ZoneSpawnInfo *spawn) {
    *gameData->escapeRopeSpawn = *spawn;
}

ZoneSpawnInfo *GameData_GetEntralinkParentSpawnInfo(GameData *gameData) {
    return &gameData->entralinkParentSpawn;
}

void GameData_SetEntralinkParentSpawnInfo(GameData *gameData, ZoneSpawnInfo *spawn) {
    gameData->entralinkParentSpawn = *spawn;
}

void SetCurrentTeleportOrDeathZone(GameData *gameData, u16 respawnLocation) {
    *PlayerSave_GetReturnZoneIDPtr(gameData->playerSave) = respawnLocation;
}

// The respawn location. The one of Black City or White Forest follows the city of the version, or the other city
// when the save has the key that switches them
u16 GetReturnLocationIdx(GameData *gameData) {
    u16 respawnLocation = *PlayerSave_GetReturnZoneIDPtr(gameData->playerSave);

    if (respawnLocation == RETURN_LOCATION_BLACK_CITY || respawnLocation == RETURN_LOCATION_WHITE_FOREST) {
#ifdef BLACK2
        if (KeyInfo_GetCityKey(getKeyInfoSaveBlk(GameData_GetSaveControl(gameData))) == 0) {
            return RETURN_LOCATION_BLACK_CITY;
        }
        return RETURN_LOCATION_WHITE_FOREST;
#else
        if (KeyInfo_GetCityKey(getKeyInfoSaveBlk(GameData_GetSaveControl(gameData))) == 0) {
            return RETURN_LOCATION_WHITE_FOREST;
        }
        return RETURN_LOCATION_BLACK_CITY;
#endif
    }
    return respawnLocation;
}

void *func_0201734c(GameData *gameData) {
    return gameData->bagCursor;
}

BagSave *GameData_GetBag(GameData *gameData) {
    return gameData->bag;
}

PokeParty *GameData_GetParty(GameData *gameData) {
    return gameData->party;
}

BoxSaveAccessor *GameData_GetBoxSaveAccessor(GameData *gameData) {
    return gameData->boxes;
}

PlayerInfo *GetGameDataPlayerInfo(GameData *gameData) {
    return &GameData_GetPlayerState(gameData)->playerInfo;
}

PlayerInfo *func_02017378(GameData *gameData, u32 netId) {
    return &gameData->playerStates[netId].playerInfo;
}

BGMInfo *GameData_GetBGMInfo(GameData *gameData) {
    return gameData->bgmInfo;
}

MMSys *GameData_GetMMSys(GameData *gameData) {
    return gameData->mmSys;
}

EventWork *GameData_GetEventWork(GameData *gameData) {
    return gameData->eventWork;
}

MapMatrix *GetMapMatrixSystem(GameData *gameData) {
    return gameData->mapMatrix;
}

Calendar *GetCalendar(GameData *gameData) {
    return gameData->calendar;
}

u8 GameData_GetSeason(GameData *gameData) {
    return gameData->season;
}

void GameData_SetSeason(GameData *gameData, u8 season) {
    gameData->season = season;
}

u32 GetNowWeather(GameData *gameData) {
    return gameData->weather;
}

void SetNowWeather(GameData *gameData, u8 weather) {
    gameData->weather = weather;
}

u32 GameData_GetLastSubscreen(GameData *gameData) {
    return gameData->lastSubscreen;
}

void GameData_SetLastSubscreen(GameData *gameData, u8 subscreen) {
    gameData->lastSubscreen = subscreen;
}

u8 func_020173ec(GameData *gameData) {
    return gameData->unk1CB;
}

void func_020173f8(GameData *gameData, u8 value) {
    gameData->unk1CB = value;
}

u8 func_02017404(GameData *gameData) {
    return gameData->cGearPowerRequest;
}

void func_0201740c(GameData *gameData, u8 value) {
    gameData->cGearPowerRequest = value;
}

void GameData_SaveCGearPowerRequest(GameData *gameData) {
    gameData->savedCGearPowerRequest = gameData->cGearPowerRequest;
    gameData->cGearPowerRequest = FALSE;
}

void GameData_RestoreCGearPowerRequest(GameData *gameData) {
    gameData->cGearPowerRequest = gameData->savedCGearPowerRequest;
    gameData->savedCGearPowerRequest = FALSE;
}

void GameData_Set30FPSMode(GameData *gameData, BOOL enable) {
    gameData->is30FPSMode = enable;
    if (enable) {
        gameData->skipFrame = 0;
    }
}

void GameData_ResetSkipFrame(GameData *gameData) {
    gameData->skipFrame = 0;
}

// In the 30 FPS mode, counts the frame and returns its number before
u8 GameData_Is30FPSSkipFrame(GameData *gameData) {
    if (gameData->is30FPSMode) {
        return gameData->skipFrame++;
    }
    return 0;
}

BOOL GameData_Is30FPSMode(GameData *gameData) {
    return gameData->is30FPSMode;
}

u32 getCurrentStepTotal(GameData *gameData) {
    return gameData->stepTotal;
}

void setCurrentStepTotal(GameData *gameData, u32 stepTotal) {
    gameData->stepTotal = stepTotal;
}

FieldSound *GameData_GetFieldSoundSystem(GameData *gameData) {
    return gameData->fieldSound;
}

EncountState *GameData_GetEncountState(GameData *gameData) {
    return gameData->encountState;
}

u32 GameData_GetLastBtlResult(GameData *gameData) {
    return gameData->lastBtlResult;
}

void GameData_SetLastBtlResult(GameData *gameData, u32 result) {
    gameData->lastBtlResult = result;
}

void func_020174bc(GameData *gameData, u8 value) {
    gameData->unk1C1 = value;
}

u8 func_020174c8(GameData *gameData) {
    return gameData->unk1C1;
}

BeaconStatus *func_020174d4(GameData *gameData) {
    return gameData->beaconStatus;
}

static void PlayerState_Init(PlayerState *playerState) {
    playerState->playerInfo.gender = GENDER_MALE;
    playerState->playerInfo.region = 0;
    playerState->zoneId = 0;
    playerState->position.x = 0;
    playerState->position.y = 0;
    playerState->position.z = 0;
    playerState->rotation = 0;
    playerState->isRail = FALSE;
    playerState->playerInfo.id = 0;
    playerState->railPos.componentIsLine = 0;
    playerState->railPos.railDirection = 0;
    playerState->railPos.componentId = 0;
    playerState->railPos.posFront = 0;
    playerState->railPos.posSide = 0;
    playerState->objCode = 0xffff;
}

void PlayerState_SetWPos(PlayerState *playerState, VecFx32 *pos) {
    playerState->position = *pos;
}

VecFx32 *PlayerState_GetWPos(PlayerState *playerState) {
    return &playerState->position;
}

void PlayerState_SetRailPos(PlayerState *playerState, const RailPosition *pos) {
    playerState->railPos = *pos;
}

RailPosition *PlayerState_GetRailPos(PlayerState *playerState) {
    return &playerState->railPos;
}

void PlayerState_SetIsRail(PlayerState *playerState, BOOL isRail) {
    playerState->isRail = isRail;
}

u8 func_0201753c(PlayerState *playerState) {
    return playerState->isRail;
}

void PlayerState_SetZoneID(PlayerState *playerState, u16 zoneId) {
    playerState->zoneId = zoneId;
}

u16 PlayerState_GetZoneID(PlayerState *playerState) {
    return playerState->zoneId;
}

void PlayerState_SetRotation(PlayerState *playerState, u16 angle) {
    playerState->rotation = angle;
}

u16 func_0201754c(PlayerState *playerState) {
    return playerState->rotation;
}

void PlayerState_SetRotationByDir(PlayerState *playerState, u16 dir) {
    playerState->rotation = DIRECTION_TO_3D_ANGLE[dir];
}

u16 PlayerState_CalcDirection(PlayerState *playerState) {
    u16 dir = DIR_UP;
    u16 angle = playerState->rotation;

    if (angle > 0x2000 && angle < 0x6000) {
        dir = DIR_LEFT;
    } else if (angle >= 0x6000 && angle <= 0xa000) {
        dir = DIR_DOWN;
    } else if (angle > 0xa000 && angle < 0xe000) {
        dir = DIR_RIGHT;
    }
    return dir;
}

u32 FieldPlayerState_GetExState(PlayerState *playerState) {
    return playerState->exState;
}

void SetPlayerSpecialState(PlayerState *playerState, u32 state) {
    playerState->exState = state;
}

void FieldPlayerState_SetObjCode(PlayerState *playerState, u16 objCode) {
    playerState->objCode = objCode;
}

u16 FieldPlayerState_GetObjCode(PlayerState *playerState) {
    return playerState->objCode;
}

FieldStatus *GameData_GetFieldStatus(GameData *gameData) {
    return gameData->fieldStatus;
}

u8 func_020175bc(GameData *gameData) {
    return gameData->playerCount;
}

void func_020175c4(GameData *gameData, u32 playerCount) {
    gameData->playerCount = playerCount;
}

u8 func_020175cc(GameData *gameData) {
    return gameData->netId;
}

void func_020175d8(GameData *gameData, u32 netId) {
    gameData->netId = netId;
}

BOOL GameData_IsForceSeasonSync(GameData *gameData) {
    return gameData->forceSeasonSync;
}

void GameData_SetForceSeasonSync(GameData *gameData, BOOL force) {
    gameData->forceSeasonSync = force;
}

u8 func_020175fc(GameData *gameData) {
    return gameData->beaconNetId;
}

void func_02017608(GameData *gameData, u32 netId) {
    gameData->beaconNetId = netId;
}

BOOL func_02017614(GameData *gameData) {
    if (func_02008ae8(gameData->config) == 1) {
        return TRUE;
    }
    return FALSE;
}

void func_0201762c(GameData *gameData, BOOL flag) {
    func_02008af0(gameData->config, flag ? 1 : 0);
}

void GameData_SetKeyItemRegistration(GameData *gameData, u32 item, BOOL registered) {
    ShortcutSave_SetKeyItemRegistration(SaveControl_GetShortcutSaveCore(GameData_GetSaveControl(gameData)), item,
                                        registered);
}

BOOL GameData_IsShortcutRegistered(GameData *gameData, u32 item) {
    return ShortcutSave_IsRegistered(SaveControl_GetShortcutSave(GameData_GetSaveControl_(gameData)), item);
}

void *func_02017670(GameData *gameData) {
    return &gameData->shortcutMenuPos;
}

// Copies the save's blocks into the game data, unless the save is new
static void GameData_LoadBackup(GameData *gameData) {
    PlayerState *playerState;
    MMSys *mmSys;
    CityState *cityState;
    PlayerSave *playerSave;
    WifiList *wifiList;

    if (SaveControl_GetStatus(gameData->saveControl) == SAVE_STATUS_NEW) {
        return;
    }

    playerState = GameData_GetPlayerState(gameData);
    func_02008ea8(gameData->saveControl, playerState);
    SaveControl_LoadPlayerState(gameData->saveControl, playerState);

    mmSys = GameData_GetMMSys(gameData);
    ActorSave_Load(mmSys, SaveControl_GetBlockPtr(gameData->saveControl, SAVE_BLOCK_FIELD_ACTORS));

    func_0200c650(gameData->saveControl, func_02017208(gameData));
    cityState = GameData_GetMyCityState(gameData);
    func_0200af9c(getReshZekBlkAddress(gameData->saveControl), (u32 *)cityState);
    GimmickState_LoadSave(gameData->saveControl, &gameData->gimmickState);

    playerSave = gameData->playerSave;
    PlayerSave_LoadFieldStatus(playerSave, gameData->fieldStatus);
    PlayerSave_LoadSeason(playerSave, &gameData->season);
    PlayerSave_LoadFollowWk(playerSave, gameData->followWk);

    wifiList = GameData_GetWifiList(gameData);
    copyWifilist(func_0200a424(gameData->saveControl), wifiList);

    PlayerSave_LoadWeather(gameData->playerSave, &gameData->weather);
    UnityTowerSave_Load(gameData->saveControl, &gameData->unityTowerFloor);
    func_02008cdc(gameData->playTime, func_02008de8(gameData->saveControl));
}

// Copies the game data's copies back into the save's blocks
static void GameData_SaveBackup(GameData *gameData) {
    PlayerState *playerState;
    MMSys *mmSys;
    CityState *cityState;
    PlayerSave *playerSave;
    WifiList *wifiList;
    PlayTime *playTime;

    playerState = GameData_GetPlayerState(gameData);
    func_02008e88(gameData->saveControl, playerState);
    SaveControl_SavePlayerState(gameData->saveControl, playerState);

    mmSys = GameData_GetMMSys(gameData);
    ActorSave_Save(mmSys, SaveControl_GetBlockPtr(gameData->saveControl, SAVE_BLOCK_FIELD_ACTORS));

    func_0200c638(gameData->saveControl, func_02017208(gameData));
    cityState = GameData_GetMyCityState(gameData);
    func_0200af8c(getReshZekBlkAddress(gameData->saveControl), (u32 *)cityState);
    GimmickState_StoreSave(&gameData->gimmickState, gameData->saveControl);

    playerSave = gameData->playerSave;
    PlayerSave_SaveFieldStatus(playerSave, gameData->fieldStatus);
    PlayerSave_SaveSeason(playerSave, gameData->season);
    PlayerSave_SaveFollowWk(playerSave, gameData->followWk);

    wifiList = GameData_GetWifiList(gameData);
    copyWifilist(wifiList, func_0200a424(gameData->saveControl));

    PlayerSave_SaveWeather(gameData->playerSave, gameData->weather);
    UnityTowerSave_Store(&gameData->unityTowerFloor, gameData->saveControl);

    playTime = func_02008de8(gameData->saveControl);
    func_02008cf8(gameData->playTime);
    func_02008cdc(playTime, gameData->playTime);
}

void func_0201782c(GameData *gameData) {
    GameData_SaveBackup(gameData);
    func_020129e0();
    gameData->saving = TRUE;
    GCTX_HIDBlockSoftReset(SOFT_RESET_BLOCK_SAVE);
    func_020073ac(gameData->saveControl);
}

int func_02017850(GameData *gameData) {
    int result;

    if (func_02005cbc() == TRUE) {
        return 0;
    }

    result = func_020073c4(gameData->saveControl);
    if (result == SAVE_RESULT_OK || result == SAVE_RESULT_NG) {
        GCTX_HIDUnblockSoftReset(SOFT_RESET_BLOCK_SAVE);
        func_020129f8();
        gameData->saving = FALSE;
    }
    return result;
}

// Cancels a save that is being written
void func_02017884(GameData *gameData) {
    if (gameData->saving == TRUE) {
        GCTX_HIDUnblockSoftReset(SOFT_RESET_BLOCK_SAVE);
        func_020129f8();
        gameData->saving = FALSE;
        func_02007424(gameData->saveControl);
        if (GCTX_HIDIsSleepBlocked(SLEEP_BLOCK_SAVE) == TRUE) {
            GCTX_HIDUnblockSleep(SLEEP_BLOCK_SAVE);
        }
    }
}

BOOL GameData_CheckEventsPaused(GameData *gameData) {
    return gameData->saving;
}

void func_020178c4(GameData *gameData, u32 block) {
    GCTX_HIDBlockSleep(SLEEP_BLOCK_SAVE);
    GameData_SaveBackup(gameData);
    func_020129e0();
    gameData->saving = TRUE;
    GCTX_HIDBlockSoftReset(SOFT_RESET_BLOCK_SAVE);
    func_0200760c(gameData->saveControl, block);
}

u32 func_020178f4(GameData *gameData, u32 block) {
    u32 result;

    if (func_02005cbc() == TRUE) {
        return 0;
    }

    result = func_0200764c(gameData->saveControl, block);
    if (result == SAVE_RESULT_OK || result == SAVE_RESULT_NG) {
        GCTX_HIDUnblockSoftReset(SOFT_RESET_BLOCK_SAVE);
        func_020129f8();
        gameData->saving = FALSE;
        GCTX_HIDUnblockSleep(SLEEP_BLOCK_SAVE);
    }
    return result;
}

SaveControl *GameData_GetSaveControl(GameData *gameData) {
    return gameData->saveControl;
}

SaveControl *GameData_GetSaveControl_(GameData *gameData) {
    return gameData->saveControl;
}

GimmickState *GameData_GetGimmickState(GameData *gameData) {
    return &gameData->gimmickState;
}

PleasureBoat **GameData_GetPleasureBoatPtr(GameData *gameData) {
    return &gameData->pleasureBoat;
}

BSubwayScrWork *func_0201794c(GameData *gameData) {
    return gameData->bsubwayScrWork;
}

void func_02017954(GameData *gameData, BSubwayScrWork *bsw) {
    gameData->bsubwayScrWork = bsw;
}

BSubwayScoreData *func_0201795c(GameData *gameData) {
    return SaveControl_GetBlockPtr(gameData->saveControl, SAVE_BLOCK_BSUBWAY_SCORE);
}

void *func_02017968(GameData *gameData) {
    return SaveControl_GetBlockPtr(gameData->saveControl, SAVE_BLOCK_BSUBWAY_3A);
}

TrainerCardSave *getTrainerCardDataBlkAddress(GameData *gameData) {
    return getTrainerGameInfoAddress(gameData->saveControl);
}

PalPadFriendList *func_02017980(GameData *gameData) {
    return getPalPadFriendListAddress(gameData->saveControl);
}

CommPlayerSupport *GameData_GetCommPlayerSupport(GameData *gameData) {
    return gameData->commPlayerSupport;
}

GameRecords *GameData_GetRecords(GameData *gameData) {
    return SaveControl_GetBlockPtr(gameData->saveControl, SAVE_BLOCK_RECORDS);
}

UnityTowerFloor *GameData_GetUnityTowerSave(GameData *gameData) {
    return &gameData->unityTowerFloor;
}

MusicalSave *getMusicalInfoBlkAddress(GameData *gameData) {
    return SaveControl_GetBlockPtr(gameData->saveControl, SAVE_BLOCK_MUSICAL);
}

TrialHouseWork **GetTrialHouseWkPPtr(GameData *gameData) {
    return &gameData->trialHouseWork;
}

void *getTimeSigSaveBlock(GameData *gameData) {
    return getTimeSigBlkAddress(gameData->saveControl);
}

void *func_020179c8(GameData *gameData) {
    return SaveControl_GetBlockPtr(gameData->saveControl, SAVE_BLOCK_40);
}

void func_020179d4(GameData *gameData, MusicalCommWork *work) {
    gameData->musicalCommWork = work;
}

MusicalCommWork *func_020179dc(GameData *gameData) {
    return gameData->musicalCommWork;
}

DreamWorldSave *func_020179e4(GameData *gameData) {
    return getDreamWorldStuffAddress(gameData->saveControl);
}

WbtSystem **func_020179f0(GameData *gameData) {
    return &gameData->wbtSystem;
}

void *func_020179f8(GameData *gameData) {
    return func_0200fea0(gameData->saveControl);
}

PokewoodSystem **func_02017a04(GameData *gameData) {
    return &gameData->pokewoodSystem;
}

void *getChatterDataAddress(GameData *gameData) {
    return getChatterBlockAddress(gameData->saveControl);
}

void func_02017a18(GameData *gameData, u8 area) {
    gameData->entreeForestArea = area;
}

u8 func_02017a24(GameData *gameData) {
    return gameData->entreeForestArea;
}

void GameData_UpdateTime(GameData *gameData, u32 seconds) {
    updateGameTime(gameData->playTime, seconds);
}

PlayTime *func_02017a40(GameData *gameData) {
    return gameData->playTime;
}

FieldFollowWk *GetFieldFollowerCfg(GameData *gameData) {
    return gameData->followWk;
}

void *GetTradedPokemonBlock(GameData *gameData) {
    return SaveControl_GetBlockPtr(gameData->saveControl, SAVE_BLOCK_TRADED_POKEMON);
}

EventWork *getConstDataBlock(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_EVENT_WORK);
}

u32 GetScrPluginNo(GameData *gameData) {
    return gameData->scrPluginNo;
}

void SetScrPluginNo(GameData *gameData, u32 pluginNo) {
    gameData->scrPluginNo = pluginNo;
}

// Draws the three picks, again while two of them are the same
void func_02017a78(GameData *gameData) {
    int tries = 0;
    int i, j;
    BOOL different;
    int k;

    sys_memset(gameData->picks, 0, sizeof(gameData->picks));
    do {
        for (i = 0; i < GAME_DATA_PICK_MAX; i++) {
            gameData->picks[i] = sPickValues[GFL_RandomLC(NELEMS(sPickValues))];
        }

        different = TRUE;
        for (j = 0; j < GAME_DATA_PICK_MAX; j++) {
            for (k = 0; k < GAME_DATA_PICK_MAX; k++) {
                if (j != k && gameData->picks[j] == gameData->picks[k]) {
                    different = FALSE;
                }
            }
        }
        if (different) {
            break;
        }
    } while (tries++ <= GAME_DATA_PICK_TRIES);
}

// Whether the value is one of the three picks
BOOL func_02017af0(GameData *gameData, u32 value) {
    int i;

    for (i = 0; i < GAME_DATA_PICK_MAX; i++) {
        if (value == gameData->picks[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

u32 GameData_GetLensFlareEntryIdx(GameData *gameData) {
    return gameData->lensFlareEntryIdx;
}

void GameData_SetLensFlareEntryIdx(GameData *gameData, u32 index) {
    gameData->lensFlareEntryIdx = index;
}

BOOL GameData_IsLensFlareRequested(GameData *gameData) {
    return gameData->lensFlareRequested;
}

void GameData_SetLensFlareRequested(GameData *gameData, BOOL requested) {
    gameData->lensFlareRequested = requested;
}

BOOL func_02017b34(GameData *gameData) {
    return gameData->funfestMission;
}

void func_02017b40(GameData *gameData, BOOL flag) {
    gameData->funfestMission = flag;
}

void func_02017b4c(GameData *gameData, u8 value) {
    gameData->unk30D = value;
}

u8 func_02017b58(GameData *gameData) {
    return gameData->unk30D;
}

void func_02017b64(GameData *gameData, u8 a1) {
    gameData->unk30E = a1;
}

u8 func_02017b70(GameData *gameData) {
    return gameData->unk30E;
}

JoinAvenuePersonList **GameData_GetJoinAvenuePersonListPtr(GameData *gameData) {
    return &gameData->joinAvenuePersonList;
}

ResortWork *func_02017b84(GameData *gameData) {
    return gameData->resortWork;
}

u8 func_02017b8c(GameData *gameData) {
    return gameData->counter;
}

void func_02017b98(GameData *gameData) {
    gameData->counter++;
    gameData->counter = gameData->counter > GAME_DATA_COUNTER_MAX ? GAME_DATA_COUNTER_MAX : gameData->counter;
}

void func_02017bb4(GameData *gameData) {
    gameData->counter = 0;
}
