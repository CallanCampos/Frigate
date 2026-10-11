#ifndef POKEBW2_FIELD_UNITY_TOWER_H
#define POKEBW2_FIELD_UNITY_TOWER_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "struct_decls.h"

// The visitors on the floor of the Unity Tower the player is on, which the game data keeps
typedef struct {
    u8 floor;
    u8 count;
    u8 value;
    // The trainer class and the index of each visitor
    u8 trainerClasses[5];
    u8 visitors[5];
} UnityTowerFloor;

UnityTowerFloor *GameData_GetUnityTowerSave(GameData *gameData);
// A trainer met through a trade, as the survey keeps them: what a trade (or the GTS) sends of its player for the
// other's records
typedef struct {
    PlayerInfo info;
    u16 sentSpecies;
    u16 receivedSpecies;
    // The trainer's hobby, which getPlayerSurveys gives
    u8 hobby;
    // The countries the trainer had met
    u8 countryCount;
    u8 unk26_0 : 3;
    // A flag of the Unity Tower's scripts, which a new record of the same trainer keeps
    u8 scriptFlag : 1;
    // Whether the entry of the survey's list is used
    u8 valid : 1;
    u8 unk26_5 : 3;
    u8 unk27;
} UnityTowerVisitor;

// The survey's visitors, the latest last, which start the survey
#define UNITY_TOWER_VISITOR_MAX 20

// What UnityTower_GetVisitorParam gives of a visitor
#define UNITY_TOWER_VISITOR_PARAM_SENT_SPECIES 0
#define UNITY_TOWER_VISITOR_PARAM_RECEIVED_SPECIES 1
#define UNITY_TOWER_VISITOR_PARAM_HOBBY 2
#define UNITY_TOWER_VISITOR_PARAM_COUNTRY_COUNT 3
#define UNITY_TOWER_VISITOR_PARAM_UNK26_0 4
#define UNITY_TOWER_VISITOR_PARAM_SCRIPT_FLAG 5
#define UNITY_TOWER_VISITOR_PARAM_VALID 6

// The two bits the survey keeps per region of a country
#define UNITY_TOWER_REGION_NONE 0
#define UNITY_TOWER_REGION_TODAY 1
#define UNITY_TOWER_REGION_BEFORE 2
#define UNITY_TOWER_REGION_OWN 3
// 64 regions of each country from 1 on, two bits each
#define UNITY_TOWER_REGION_BYTES (255 * 16)

// The survey's save block 0x1d: the trainers met through trades and the GTS, and the countries and regions they
// came from
struct UnityTowerSurveySave {
    UnityTowerVisitor visitors[UNITY_TOWER_VISITOR_MAX];
    // A bit per country a visitor has come from
    u8 countriesMet[32];
    u8 unk340[4];
    // Set once a region outside Japan is recorded
    u8 foreignRegionMet;
    u8 countryCount;
    // The player's own answer to the survey, below 9
    u8 hobby;
    // Below 5, sent in trades as the visitor's unk26_0
    u8 unk347;
    // UNITY_TOWER_REGION_* for each of 64 regions of each country from 1 on
    u8 regions[UNITY_TOWER_REGION_BYTES];
}; // size: 0x1338

// The size of the survey's block and its init
u32 func_02009b5c(void);
void func_02009b64(UnityTowerSurveySave *save);
// Gives the player the country and region and records them as the player's own
void func_02009b84(UnityTowerSurveySave *save, PlayerInfo *info, u8 country, u8 region);
// The UNITY_TOWER_REGION_* state of a country's region, and recording one
u32 func_02009ba4(UnityTowerSurveySave *save, u8 country, u8 region);
void func_02009be0(UnityTowerSurveySave *save, u8 country, u8 region, u32 state);
u8 func_02009c38(UnityTowerSurveySave *save);
// A day has passed: every region recorded today becomes one recorded before
void func_02009c48(UnityTowerSurveySave *save);
// The countries met, and without the player's own unless withOwnCountry
u8 func_02009ca0(UnityTowerSurveySave *save);
u8 func_02009cac(UnityTowerSurveySave *save, PlayerInfo *playerInfo, BOOL withOwnCountry);
// How many visitors are listed
u8 func_02009ce4(UnityTowerSurveySave *save);
// Counts one more country met
void func_02009d04(UnityTowerSurveySave *save);
void func_02009d18(UnityTowerSurveySave *save, s32 value);
u8 func_02009d28(UnityTowerSurveySave *save);
void setPlayerSurveys(UnityTowerSurveySave *save, s32 hobby);
u8 getPlayerSurveys(UnityTowerSurveySave *save);
u32 UnityTower_GetVisitorParam(UnityTowerSurveySave *save, u32 index, u32 param);
// Sets a visitor's UNITY_TOWER_VISITOR_PARAM_UNK26_0, SCRIPT_FLAG or VALID
void func_02009db4(UnityTowerSurveySave *save, int index, u32 param, u32 value);
// A visitor, whose PlayerInfo comes first, or NULL out of range
void *UnityTower_GetVisitor(UnityTowerSurveySave *save, int index);
UnityTowerVisitor *func_02009e68(UnityTowerSurveySave *save);
// Records the country met, returning whether it is a new one
BOOL func_02009e6c(UnityTowerSurveySave *save, u32 country);
// Whether a visitor from the country has come
BOOL func_02009eb0(UnityTowerSurveySave *save, u32 country);
u8 UnityTowerVisitor_GetProvince(PlayerInfo *playerInfo);
u8 UnityTowerVisitor_GetCountry(PlayerInfo *playerInfo);
// Records a trade's partner as the latest visitor, unless they have no country: 1 for a visitor met before, who
// moves to the end of the list, 2 for a new one, who takes the place of the first from the same country once five
// are listed, or of the first visitor once the list is full
u32 UnityTowerSurvey_RegisterTrade(UnityTowerSurveySave *save, UnityTowerVisitor *visitor);
void func_ov033_0217aa1c(GameSystem *gsys, s32 floor, u32 value);
void func_ov033_0217aa50(UnityTowerSurveySave *save, UnityTowerFloor *output, s32 floor, u32 value);
u32 func_ov033_0217aac4(UnityTowerFloor *output, u32 index);
u32 func_ov033_0217ab58(u32 gender, u32 id, u32 province, u32 a3, u32 hasProvince);
void UnityTowerSave_Init(UnityTowerFloor *save);
u32 func_ov033_0217aad8(WordSet *wordSet, GameSystem *gsys, UnityTowerFloor *save, s32 index, u32 param);

void LoadUnityTowerVisitorWordSet(WordSet *wordSet, GameData *gameData, u32 index);

// Overlay 12's scrcmd_unity_tower.c
BOOL func_ov012_02160cdc(VM *vm, FieldScriptEnv *env);
BOOL s01CF_UnityTowerGetStateParam(VM *vm, FieldScriptEnv *env);
BOOL s01D0_UnityTowerCallFloorSelect(VM *vm, FieldScriptEnv *env);

// Overlay 311, the floor select
extern const GameProcFunctions data_ov311_0219e440;

#endif // POKEBW2_FIELD_UNITY_TOWER_H
