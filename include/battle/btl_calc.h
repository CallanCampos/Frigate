#ifndef POKEBW2_BATTLE_BTL_CALC_H
#define POKEBW2_BATTLE_BTL_CALC_H

// Overlay 167's btl_calc.c: the battle's random numbers, type matchups, stat stage ratios, condition makers and target
// selection. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "battle/btl_pokeparam.h"
#include "gfl/heap.h"
#include "nitro/math.h"
#include "pml/waza.h"
#include "struct_decls.h"

// Up to four client IDs without repeats, the unused ones 4
typedef struct {
    u32 count;
    u8 clientIds[4];
} BtlClientIDList;

void BtlCalc_Init(const MATHRandContext32 *rand, HeapID heapId);
void BtlCalc_ResetRand(const MATHRandContext32 *rand);
void BtlCalc_Exit(void);
u32 BattleRandom(u32 max);
u16 GetBoostFromStatStage(u16 value, u8 stage);
u8 BtlCalc_ApplyAccuracyStage(u32 value, u32 stage);
BOOL BtlCalc_RollCritical(u8 critStage);
u32 GetTypeEffectiveness(u8 attackType, u8 defenseType);
s32 BtlCalc_GetTypePairEffectiveness(u8 attackType, PokeTypePair defenseTypes);
u32 GetTypeEffectivenessMultiplier(u32 effectiveness1, u32 effectiveness2);
u32 CalcBaseDamage(u32 power, u32 attack, u32 level, u32 defense);
u32 TypeEffectivenessPowerMod(u32 damage, u32 effectiveness);
u8 GetTypeWeaknesses(u8 attackType, u8 *defenseTypes);
u32 BtlCalc_GetEffectivenessCategory(s32 value);
BOOL RollEffectChance(u32 chance);
s32 BtlCalc_AtLeast(s32 value, s32 minimum);
u32 fixed_round(u32 value, u32 ratio);
u32 GetRatioOverZero(u32 value, u32 ratio);
u32 MultiplyValueByRatio(u32 value, u32 ratio);
u32 DivideMaxHp(BattleMon *mon, u32 divisor);
u32 DivideMaxHPZeroCheck(BattleMon *mon, u32 divisor);
u32 BtlCalc_RandomRange(u32 min, u32 max);
u8 BtlCalc_RollHitCount(u8 hits);
u32 BtlCalc_GetWeatherDamage(BattleMon *mon, u32 weather);
fx32 WeatherPowerMod(u32 weather, u32 moveType);
void BtlCalc_MakeMoveCondition(MoveConditionParam param, BattleMon *mon, BattleCondition *out);
BattleCondition BtlCalc_MakeStatusCondition(u32 status);
u32 BtlCalc_FindMentalHerbCondition(BattleMon *mon);
BOOL IsBasicStatus(s32 condition);
BattleCondition BtlCalc_MakeTurnCondition(u32 turns);
BattleCondition BtlCalc_MakePokeCondition(u32 turns);
void BtlCalc_MakeDefaultCondition(s32 condition, BattleMon *mon, BattleCondition *out);
u16 BtlCalc_PickRandomMove(const u16 *excluded, u16 count);
BOOL BtlCalc_IsMoveInList(const u16 *moves, u32 count, u16 move);
u32 CalcBaseExpGain(BattleMon *mon, s32 levelDiff);
BOOL BtlCalc_IsDoubleOrTripleStyle(u32 value);
u32 BtlCalc_GetFrontPosCount(u32 value);
u8 GetNumMonsOnField(u32 battleStyle, u8 count);
BOOL BtlCalc_IsGymLeaderClass(u32 trainerClass);
BOOL BtlCalc_IsEliteFourClass(u32 trainerClass);
BOOL BtlCalc_IsBossTrainerClass(u16 trainerClass);
u32 BtlCalc_GetPrizeMoney(BtlSetup *setup);
u32 BtlCalc_GetTrainerPrizeMoney(BtlSetupTrainer *trainer, PokeParty *party);
u32 BtlCalc_GetLostMoney(u32 index, BattleParty *party);
void BtlCalc_SetHeapID(HeapID heapId);
void func_ov167_021bd880(void);
u32 ItemGetParam(u16 item, u32 param);
u32 BtlCalc_PackMonIDs(const u8 *values);
void BtlCalc_UnpackMonIDs(u32 packed, u8 *values);
u32 BtlCalc_GetCurseTarget(BattleMon *mon);
u8 DecideMoveTargetAuto(BtlMainModule *mainModule, BtlPokeCon *pokeCon, BattleMon *mon, u16 move);
u8 DecideMoveTargetAutoForClient(BtlMainModule *mainModule, BtlPokeCon *pokeCon, BattleMon *mon, u16 move,
                                 MATHRandContext32 *saved);
void BtlClientIDList_Init(BtlClientIDList *list);
void BtlClientIDList_Add(BtlClientIDList *list, u8 clientId);
u32 BtlClientIDList_GetCount(BtlClientIDList *list);
u32 BtlClientIDList_GetEscapeResult(BtlClientIDList *list, u8 clientId, u32 value);

#endif // POKEBW2_BATTLE_BTL_CALC_H
