#ifndef POKEBW2_SYSTEM_GAME_DATA_H
#define POKEBW2_SYSTEM_GAME_DATA_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The city of the player's version, which a key from Unova Link can switch
#define CITY_BLACK_CITY 0
#define CITY_WHITE_FOREST 1

// What GameData_MakeBoxPkm makes a Pokémon from
struct BoxPkmCreateParams {
    u16 heapId;
    u16 pad;
    u32 species;
    u32 form;
    u32 level;
    u32 item;
    s32 ability;
    s32 sex;
    s32 param1C;
    u32 ball;
    BOOL hiddenAbility;
};

struct CityState {
    u16 initialized;
    u16 city;
};

void SetAllowVersionSpecificZone(u32 side, u32 allow);
void func_ov012_0215cd58(CityState *state);
void func_ov012_0215cd8c(CityState *state);
BOOL func_ov012_0215cd98(s32 value);

GameData *GameData_Create(HeapID heapId);
void GameData_Free(GameData *gameData);
BOOL GameData_CheckPairFlag(GameData *gameData);
BagSave *GameData_GetBag(GameData *gameData);
BGMInfo *GameData_GetBGMInfo(GameData *gameData);
void *func_0201734c(GameData *gameData);
BoxSaveAccessor *GameData_GetBoxSaveAccessor(GameData *gameData);
ZoneSpawnInfo *GameData_GetEntralinkParentSpawnInfo(GameData *gameData);
ZoneSpawnInfo *GameData_GetEscapeRopeZone(GameData *gameData);
EventData *GameData_GetEventData(GameData *gameData);
EventWork *GameData_GetEventWork(GameData *gameData);
// The Royal Unova's cruise while the player is aboard, or NULL
PleasureBoat **GameData_GetPleasureBoatPtr(GameData *gameData);
FieldSound *GameData_GetFieldSoundSystem(GameData *gameData);
FieldStatus *GameData_GetFieldStatus(GameData *gameData);
GimmickState *GameData_GetGimmickState(GameData *gameData);
JoinAvenuePersonList **GameData_GetJoinAvenuePersonListPtr(GameData *gameData);
u32 GameData_GetLastSubscreen(GameData *gameData);
MMSys *GameData_GetMMSys(GameData *gameData);
CityState *GameData_GetMyCityState(GameData *gameData);
// Adds seconds to the play time
void GameData_UpdateTime(GameData *gameData, u32 seconds);
// The levels that unlock the pass powers, which func_0200c5dc and func_0200c5e0 read
void *func_02017208(GameData *gameData);
u16 func_02017220(GameData *gameData);
// The C-Gear power request, which GameData_SaveCGearPowerRequest puts aside
u8 func_02017404(GameData *gameData);
void func_0201740c(GameData *gameData, u8 value);
// A byte of the game data, which func_020173f8 sets
u8 func_020173ec(GameData *gameData);
// Save block 0x39, the Battle Subway's scores
BSubwayScoreData *func_0201795c(GameData *gameData);
BSubwayScrWork *func_0201794c(GameData *gameData);
u8 func_02017b8c(GameData *gameData);
// Adds one to a counter of the game data, to at most 99
void func_02017b98(GameData *gameData);
void func_02017bb4(GameData *gameData);
void func_02017884(GameData *gameData);
// Starts saving, and runs the save, returning 2 once it ends
void func_0201782c(GameData *gameData);
int func_02017850(GameData *gameData);
void *getChatterDataAddress(GameData *gameData);
MusicalSave *getMusicalInfoBlkAddress(GameData *gameData);
ZoneSpawnInfo *GameData_GetNextZone(GameData *gameData);
PokeParty *GameData_GetParty(GameData *gameData);
PlayerState *GameData_GetPlayerState(GameData *gameData);
void *getTimeSigSaveBlock(GameData *gameData);
PlayerState *func_020171e8(GameData *gameData, s32 index);
PokeDexSave *GameData_GetPokedex(GameData *gameData);
PalPadFriendList *func_02017980(GameData *gameData);
// Whether the key item is registered to Y, and registers it or not
BOOL GameData_IsShortcutRegistered(GameData *gameData, u32 item);
void GameData_SetKeyItemRegistration(GameData *gameData, u32 item, BOOL registered);
GameRecords *GameData_GetRecords(GameData *gameData);
BOOL GameData_AddBoxPkm(GameData *gameData, BoxPkmCreateParams *params);
BOOL addPkmToParty(GameData *gameData, BoxPkmCreateParams *params);
PartyPkm *GameData_MakeBoxPkm(GameData *gameData, BoxPkmCreateParams *params);
// Whether a full day has passed since the last check
BOOL checkForMidnight(GameData *gameData);
SaveControl *GameData_GetSaveControl(GameData *gameData);
void *func_02017670(GameData *gameData);
// The support another player gave over communication (system/comm_player_support.h)
CommPlayerSupport *GameData_GetCommPlayerSupport(GameData *gameData);
// Save block 0x41, the Pokémon traded in game, which save/traded_pokemon.h reads
void *GetTradedPokemonBlock(GameData *gameData);
SaveControl *GameData_GetSaveControl_(GameData *gameData);
BeaconStatus *func_020174d4(GameData *gameData);
DreamWorldSave *func_020179e4(GameData *gameData);
u8 GameData_GetSeason(GameData *gameData);
void GameData_SetSeason(GameData *gameData, u8 season);
u8 func_02017a24(GameData *gameData);
// Sets the area of the Entree Forest the player is in, which func_02017a24 returns
void func_02017a18(GameData *gameData, u8 area);
u16 GameData_GetDayPeriod(GameData *gameData);
u32 GameData_GetMonth(GameData *gameData);
u32 GameData_GetDay(GameData *gameData);
u32 getCurrentDayOfWeek(GameData *gameData);
u32 getCurrentHour(GameData *gameData);
u32 getCurrentMinute(GameData *gameData);
WifiList *GameData_GetWifiList(GameData *gameData);
void GameData_InitEncountTerrain(GameData *gameData, Field *field);
EncountState *GameData_GetEncountState(GameData *gameData);
BOOL GameData_IsForceSeasonSync(GameData *gameData);
BOOL GameData_IsLensFlareRequested(GameData *gameData);
void GameData_RestoreCGearPowerRequest(GameData *gameData);
void GameData_SaveCGearPowerRequest(GameData *gameData);
void GameData_SetEntralinkParentSpawnInfo(GameData *gameData, ZoneSpawnInfo *spawn);
void GameData_SetEscapeRopeZone(GameData *gameData, ZoneSpawnInfo *spawn);
void GameData_SetForceSeasonSync(GameData *gameData, BOOL force);
void GameData_SetLastSubscreen(GameData *gameData, u8 subscreen);
void GameData_SetLastBtlResult(GameData *gameData, u32 result);
u32 GameData_GetLastBtlResult(GameData *gameData);
void GameData_SetLensFlareRequested(GameData *gameData, BOOL requested);
// Whether a Funfest mission is running, which plays its BGM everywhere (a guess from that use)
BOOL func_02017b34(GameData *gameData);
// Sets whether a Funfest mission is running
void func_02017b40(GameData *gameData, BOOL flag);
u32 GameData_GetLensFlareEntryIdx(GameData *gameData);
void func_020173f8(GameData *gameData, u8 value);
void GameData_SetLensFlareEntryIdx(GameData *gameData, u32 index);
void GameData_SetNextZone(GameData *gameData, ZoneSpawnInfo *spawn);
ZoneSpawnInfo *GetGameDataNowSpawnZone(GameData *gameData);
PlayerInfo *GetGameDataPlayerInfo(GameData *gameData);
MapMatrix *GetMapMatrixSystem(GameData *gameData);
ZoneSpawnInfo *GetOutboundWarpRememberSpawnInfo(GameData *gameData);
void SetGameDataOutboundWarpRememberInfo(GameData *gameData, ZoneSpawnInfo *spawn);
u16 GetReturnLocationIdx(GameData *gameData);
void SetCurrentTeleportOrDeathZone(GameData *gameData, u16 respawnLocation);
void SetGameDataNowSpawnZone(GameData *gameData, ZoneSpawnInfo *spawn);
PlayerInfo *func_02017378(GameData *gameData, u32 netId);
// The player's net ID
u8 func_020175cc(GameData *gameData);
// The number of players
u8 func_020175bc(GameData *gameData);
// The net ID of the player whose beacon is shown
u8 func_020175fc(GameData *gameData);
// The player's Entralink levels, as func_02017208 returns for the player's own
void *func_020171fc(GameData *gameData, u32 netId);
void func_020175c4(GameData *gameData, u32 a1);
void func_020175d8(GameData *gameData, u32 a1);
void func_02017608(GameData *gameData, u32 a1);
// Whether the game shows other players' beacons
BOOL func_02017614(GameData *gameData);
void func_020178c4(GameData *gameData, u32 block);
u32 func_020178f4(GameData *gameData, u32 block);
void func_02017954(GameData *gameData, BSubwayScrWork *bsw);
// Where Pokéstar Studios keeps its PokewoodSystem while the player makes a movie
PokewoodSystem **func_02017a04(GameData *gameData);
// Where the Pokémon World Tournament keeps its WbtSystem while the player is at the tournament
WbtSystem **func_020179f0(GameData *gameData);
// The Pokémon World Tournament's save data
void *func_020179f8(GameData *gameData);
// The save's play time
PlayTime *func_02017a40(GameData *gameData);
void func_02017b64(GameData *gameData, u8 a1);
ResortWork *func_02017b84(GameData *gameData);
u32 GetScrPluginNo(GameData *gameData);
void SetScrPluginNo(GameData *gameData, u32 pluginNo);

Calendar *GetCalendar(GameData *gameData);
void SetNowWeather(GameData *gameData, u8 weather);
u32 GetNowWeather(GameData *gameData);
BOOL GameData_CheckEventsPaused(GameData *gameData);
// The Battle Subway's save data
void *func_02017968(GameData *gameData);
// Where the Trial House work is kept
TrialHouseWork **GetTrialHouseWkPPtr(GameData *gameData);
// The follower of field/pair_sys.h
FieldFollowWk *GetFieldFollowerCfg(GameData *gameData);
void GameData_ResetSkipFrame(GameData *gameData);
void GameData_Set30FPSMode(GameData *gameData, BOOL enable);
u8 GameData_Is30FPSSkipFrame(GameData *gameData);
BOOL GameData_Is30FPSMode(GameData *gameData);
u32 getCurrentStepTotal(GameData *gameData);
void setCurrentStepTotal(GameData *gameData, u32 stepTotal);
// A byte of the game data, which overlay 169 uses
void func_020174bc(GameData *gameData, u8 value);
u8 func_020174c8(GameData *gameData);
// Sets the option that func_02017614 returns
void func_0201762c(GameData *gameData, BOOL flag);
// Save block 0x40
void *func_020179c8(GameData *gameData);
// Draws three different values of fifteen at random, and whether a value is one of the three
void func_02017a78(GameData *gameData);
BOOL func_02017af0(GameData *gameData, u32 value);
// A byte of the game data, 0xff at first, which overlay 145 uses
void func_02017b4c(GameData *gameData, u8 value);
u8 func_02017b58(GameData *gameData);
u8 func_02017b70(GameData *gameData);
CityState *func_0201722c(GameData *gameData, u32 index);

#endif // POKEBW2_SYSTEM_GAME_DATA_H
