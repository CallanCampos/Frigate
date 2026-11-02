#include "types.h"
#include "battle/btl_calc.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_pospoke.h"
#include "battle/btl_result.h"
#include "battle/btl_setup.h"
#include "battle/btl_tables.h"
#include "battle/trainer_data.h"
#include "constants/battle.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/types.h"
#include "gfl/heap.h"
#include "pml/item.h"
#include "pml/move.h"
#include "pml/personal.h"
#include "pml/poke_party.h"

// A numerator and a denominator
typedef struct {
    u8 numerator;
    u8 denominator;
} BtlCalcRatio;

typedef struct {
    u16 heapId;
    // Room for every move, which BtlCalc_PickRandomMove picks from
    u16 *moves;
    MATHRandContext32 rand;
} BtlCalcWork;

BtlCalcWork g_BtlCalcWork;

// MWCC sorts these tables by size, swapping each pair of the same size, so the two 6-byte tables and the two
// stage ratio tables are each declared in the reverse of the ROM's order

// The chance of a critical hit at each critical hit stage is 1 in this
const u8 CRITICAL_HIT_CHANCES[5] = { 16, 8, 4, 3, 2 };

// The damage multiplier in quarters of each type effectiveness
const u8 TYPE_EFFECTIVENESS_QUARTERS[6] = { 0, 1, 2, 4, 8, 16 };

// The percentages below which a move that hits 2 to 5 times hits each number of times
const u8 MULTI_HIT_CHANCES[6] = { 0, 0, 35, 70, 85, 100 };

// The multiplier of the money lost, by BtlCalc_GetLostMoney's index up to 8
const u8 LOST_MONEY_MULTIPLIERS[9] = { 2, 4, 6, 9, 12, 16, 20, 25, 30 };

const u16 data_ov167_021d74ba[9] = { 0x73, 0x871, 0x10db, 0x1836, 0x216e, 0x297d, 0x30bf, 0x3986, 0x41be };

// The ratio of each accuracy and evasion stage, -6 to +6
const BtlCalcRatio ACCURACY_STAGE_RATIOS[13] = {
    { 6, 18 }, { 6, 16 }, { 6, 14 }, { 6, 12 }, { 6, 10 }, { 6, 8 },  { 6, 6 },
    { 8, 6 },  { 10, 6 }, { 12, 6 }, { 14, 6 }, { 16, 6 }, { 18, 6 },
};

// The ratio of each stat stage, -6 to +6
const BtlCalcRatio STAT_STAGE_RATIOS[13] = {
    { 2, 8 }, { 2, 7 }, { 2, 6 }, { 2, 5 }, { 2, 4 }, { 2, 3 }, { 2, 2 },
    { 3, 2 }, { 4, 2 }, { 5, 2 }, { 6, 2 }, { 7, 2 }, { 8, 2 },
};

// The damage of each attacking type against each defending type in halves: 0, 2 (half), 4 (normal) or 8 (double)
const u8 TYPE_CHART[17][17] = {
    { 4, 4, 4, 4, 4, 2, 4, 0, 2, 4, 4, 4, 4, 4, 4, 4, 4 }, { 8, 4, 2, 2, 4, 8, 2, 0, 8, 4, 4, 4, 4, 2, 8, 4, 8 },
    { 4, 8, 4, 4, 4, 2, 8, 4, 2, 4, 4, 8, 2, 4, 4, 4, 4 }, { 4, 4, 4, 2, 2, 2, 4, 2, 0, 4, 4, 8, 4, 4, 4, 4, 4 },
    { 4, 4, 0, 8, 4, 8, 2, 4, 8, 8, 4, 2, 8, 4, 4, 4, 4 }, { 4, 2, 8, 4, 2, 4, 8, 4, 2, 8, 4, 4, 4, 4, 8, 4, 4 },
    { 4, 2, 2, 2, 4, 4, 4, 2, 2, 2, 4, 8, 4, 8, 4, 4, 8 }, { 0, 4, 4, 4, 4, 4, 4, 8, 2, 4, 4, 4, 4, 8, 4, 4, 2 },
    { 4, 4, 4, 4, 4, 8, 4, 4, 2, 2, 2, 4, 2, 4, 8, 4, 4 }, { 4, 4, 4, 4, 4, 2, 8, 4, 8, 2, 2, 8, 4, 4, 8, 2, 4 },
    { 4, 4, 4, 4, 8, 8, 4, 4, 4, 8, 2, 2, 4, 4, 4, 2, 4 }, { 4, 4, 2, 2, 8, 8, 2, 4, 2, 2, 8, 2, 4, 4, 4, 2, 4 },
    { 4, 4, 8, 4, 0, 4, 4, 4, 4, 4, 8, 2, 2, 4, 4, 2, 4 }, { 4, 8, 4, 8, 4, 4, 4, 4, 2, 4, 4, 4, 4, 2, 4, 4, 0 },
    { 4, 4, 8, 4, 8, 4, 4, 4, 2, 2, 2, 8, 4, 4, 2, 8, 4 }, { 4, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 8, 4 },
    { 4, 2, 4, 4, 4, 4, 4, 8, 2, 4, 4, 4, 4, 8, 4, 4, 2 },
};

// Reconstructed, not known from the ROM. MWCC puts data_ov167_021d74ba with the file's other small tables, as the
// original has it, only when some code reads it, even code it never emits. This accessor is that code; nothing
// calls it.
static inline u16 BtlCalc_GetUnkTableValue(u32 index) {
    return data_ov167_021d74ba[index];
}

void BtlCalc_Init(const MATHRandContext32 *rand, HeapID heapId) {
    g_BtlCalcWork.moves = GFL_HeapAllocate(heapId, 0x230 * sizeof(u16), TRUE, "btl_calc.c", 0x50);
    g_BtlCalcWork.rand = *rand;
}

void BtlCalc_ResetRand(const MATHRandContext32 *rand) {
    g_BtlCalcWork.rand = *rand;
}

void BtlCalc_Exit(void) {
    if (g_BtlCalcWork.moves != NULL) {
        GFL_HeapFree(g_BtlCalcWork.moves);
        g_BtlCalcWork.moves = NULL;
    }
}

u32 BattleRandom(u32 max) {
    return MATH_Rand32(&g_BtlCalcWork.rand, max);
}

u16 GetBoostFromStatStage(u16 value, u8 stage) {
    u32 result = (u16)(value * STAT_STAGE_RATIOS[stage].numerator);

    return result / STAT_STAGE_RATIOS[stage].denominator;
}

u8 BtlCalc_ApplyAccuracyStage(u32 value, u32 stage) {
    value = (s32)(value * ACCURACY_STAGE_RATIOS[stage].numerator) / ACCURACY_STAGE_RATIOS[stage].denominator;
    if (value > 100) {
        value = 100;
    }
    return value;
}

BOOL BtlCalc_RollCritical(u8 critStage) {
    u8 roll = BattleRandom(CRITICAL_HIT_CHANCES[critStage]);

    if (roll == 0) {
        return TRUE;
    }
    return FALSE;
}

u32 GetTypeEffectiveness(u8 attackType, u8 defenseType) {
    if (attackType == TYPE_NULL || defenseType == TYPE_NULL) {
        return TYPE_EFFECTIVENESS_NORMAL;
    }
    switch (TYPE_CHART[attackType][defenseType]) {
    case 0:
        return TYPE_EFFECTIVENESS_IMMUNE;
    case 2:
        return TYPE_EFFECTIVENESS_HALF;
    case 4:
        return TYPE_EFFECTIVENESS_NORMAL;
    case 8:
        return TYPE_EFFECTIVENESS_DOUBLE;
    }
    return TYPE_EFFECTIVENESS_IMMUNE;
}

s32 BtlCalc_GetTypePairEffectiveness(u8 attackType, PokeTypePair defenseTypes) {
    u8 type1;
    u8 type2;
    u32 effectiveness;

    PokeTypePair_Split(defenseTypes, &type1, &type2);
    if (PokeTypePair_IsMonotype(defenseTypes)) {
        return GetTypeEffectiveness(attackType, type1);
    }
    effectiveness = GetTypeEffectiveness(attackType, type1);
    return GetTypeEffectivenessMultiplier(effectiveness, GetTypeEffectiveness(attackType, type2));
}

u32 GetTypeEffectivenessMultiplier(u32 effectiveness1, u32 effectiveness2) {
    u32 multiplier = TYPE_EFFECTIVENESS_QUARTERS[effectiveness1] * TYPE_EFFECTIVENESS_QUARTERS[effectiveness2] / 4;

    switch (multiplier) {
    case 0:
        return TYPE_EFFECTIVENESS_IMMUNE;
    case 1:
        return TYPE_EFFECTIVENESS_QUARTER;
    case 2:
        return TYPE_EFFECTIVENESS_HALF;
    case 4:
        return TYPE_EFFECTIVENESS_NORMAL;
    case 8:
        return TYPE_EFFECTIVENESS_DOUBLE;
    case 16:
        return TYPE_EFFECTIVENESS_QUADRUPLE;
    }
    return TYPE_EFFECTIVENESS_IMMUNE;
}

u32 CalcBaseDamage(u32 power, u32 attack, u32 level, u32 defense) {
    return power * attack * (level * 2 / 5 + 2) / defense / 50 + 2;
}

u32 TypeEffectivenessPowerMod(u32 damage, u32 effectiveness) {
    switch (effectiveness) {
    case TYPE_EFFECTIVENESS_IMMUNE:
        return 0;
    case TYPE_EFFECTIVENESS_QUARTER:
        return damage >> 2;
    case TYPE_EFFECTIVENESS_HALF:
        return damage >> 1;
    case TYPE_EFFECTIVENESS_NORMAL:
        return damage;
    case TYPE_EFFECTIVENESS_DOUBLE:
        return damage << 1;
    case TYPE_EFFECTIVENESS_QUADRUPLE:
        return damage << 2;
    }
    return damage;
}

u8 GetTypeWeaknesses(u8 attackType, u8 *defenseTypes) {
    u8 count;
    u8 i;

    for (i = 0, count = 0; i < 17; i++) {
        if (TYPE_CHART[attackType][i] == 0 || TYPE_CHART[attackType][i] == 2) {
            defenseTypes[count++] = i;
        }
    }
    return count;
}

u32 BtlCalc_GetEffectivenessCategory(s32 value) {
    if (value > TYPE_EFFECTIVENESS_NORMAL) {
        return 2;
    }
    if (value == TYPE_EFFECTIVENESS_NORMAL) {
        return 1;
    }
    if (value != TYPE_EFFECTIVENESS_IMMUNE) {
        return 3;
    }
    return 0;
}

// Function name from swan.
BOOL RollEffectChance(u32 chance) {
    if (BattleRandom(100) < chance) {
        return TRUE;
    }
    return FALSE;
}

s32 BtlCalc_AtLeast(s32 value, s32 minimum) {
    if (value < minimum) {
        value = minimum;
    }
    return value;
}

// Function names from swan.
u32 fixed_round(u32 value, u32 ratio) {
    u32 fraction;

    value *= ratio;
    fraction = value & 0xfff;
    value >>= 12;
    if (fraction > 0x800) {
        value++;
    }
    return value;
}

// Function name from swan.
u32 GetRatioOverZero(u32 value, u32 ratio) {
    u32 result;

    result = fixed_round(value, ratio);
    if (result == 0) {
        result = 1;
    }
    return result;
}

u32 MultiplyValueByRatio(u32 value, u32 ratio) {
    u32 remainder;
    u32 result;

    value *= ratio;
    remainder = value % 100;
    value /= 100;
    if (remainder >= 50) {
        value++;
    }
    return value;
}

// Function names from swan.
u32 DivideMaxHp(BattleMon *mon, u32 divisor) {
    return GetBattleMonStat(mon, BATTLEMON_MAX_HP) / divisor;
}

u32 DivideMaxHPZeroCheck(BattleMon *mon, u32 divisor) {
    u32 result;

    result = GetBattleMonStat(mon, BATTLEMON_MAX_HP) / divisor;
    if (result == 0) {
        result = 1;
    }
    return result;
}

// A random number from min to max, either way round
u32 BtlCalc_RandomRange(u32 min, u32 max) {
    if (min > max) {
        u32 temp = min;
        min = max;
        max = temp;
    }
    return min + BattleRandom(max - min + 1);
}

// The number of hits of a move that hits hits times, 5 meaning 2 to 5 at random
u8 BtlCalc_RollHitCount(u8 hits) {
    if (hits == 5) {
        u8 roll = BattleRandom(100);
        u8 i;

        for (i = 0; i < 6; i++) {
            if (roll < MULTI_HIT_CHANCES[i]) {
                return i;
            }
        }
        return hits;
    }
    return hits;
}

// The damage the weather does to a mon at the end of a turn
u32 BtlCalc_GetWeatherDamage(BattleMon *mon, u32 weather) {
    u16 damage;

    switch (weather) {
    case BTL_WEATHER_SANDSTORM:
        if (DoesMonHaveType(mon, TYPE_ROCK)) {
            return 0;
        }
        if (DoesMonHaveType(mon, TYPE_STEEL)) {
            return 0;
        }
        if (DoesMonHaveType(mon, TYPE_GROUND)) {
            return 0;
        }
        break;
    case BTL_WEATHER_HAIL:
        if (DoesMonHaveType(mon, TYPE_ICE)) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    damage = (s32)GetBattleMonStat(mon, BATTLEMON_MAX_HP) / 16;
    if (damage == 0) {
        damage = 1;
    }
    return damage;
}

fx32 WeatherPowerMod(u32 weather, u32 moveType) {
    switch (weather) {
    case BTL_WEATHER_SUN:
        if (moveType == TYPE_FIRE) {
            return FX32_CONST(1.5);
        }
        if (moveType == TYPE_WATER) {
            return FX32_CONST(0.5);
        }
        break;
    case BTL_WEATHER_RAIN:
        if (moveType == TYPE_FIRE) {
            return FX32_CONST(0.5);
        }
        if (moveType == TYPE_WATER) {
            return FX32_CONST(1.5);
        }
        break;
    }
    return FX32_ONE;
}

void BtlCalc_MakeMoveCondition(MoveConditionParam param, BattleMon *mon, BattleCondition *out) {
    switch (param.type) {
    case 3:
        *out = MakeConditionDependPoke(GetMonID(mon));
        break;
    case 2:
        *out = SetConditionTurns(BtlCalc_RandomRange(param.min, param.max));
        break;
    case 1:
        *out = MakeConditionPermanentTurnsParam(param.max, param.min);
        break;
    case 4: {
        u8 monId = GetMonID(mon);
        u8 turns = BtlCalc_RandomRange(param.min, param.max);
        *out = MakeConditionDependPokeTurns(monId, turns);
        break;
    }
    }
}

BattleCondition BtlCalc_MakeStatusCondition(u32 status) {
    BattleCondition condition;

    condition.raw = 0;
    switch (status) {
    case CONDITION_PARALYSIS:
    case CONDITION_FREEZE:
    case CONDITION_BURN:
    case CONDITION_POISON:
        condition.common.type = BTL_CONDITION_TYPE_PERMANENT;
        break;
    case CONDITION_SLEEP:
        condition.common.type = BTL_CONDITION_TYPE_TURNS;
        condition.common.turns = BtlCalc_RandomRange(2, 4);
        break;
    default:
        condition.common.type = BTL_CONDITION_TYPE_NONE;
        break;
    }
    return condition;
}

BOOL IsBasicStatus(s32 condition) {
    if (condition < 6) {
        return TRUE;
    }
    return FALSE;
}

BattleCondition BtlCalc_MakeTurnCondition(u32 turns) {
    BattleCondition condition;

    condition.raw = 0;
    condition.common.type = BTL_CONDITION_TYPE_TURNS;
    condition.common.turns = turns;
    return condition;
}

BattleCondition BtlCalc_MakePokeCondition(u32 monId) {
    BattleCondition condition;

    condition.raw = 0;
    condition.common.type = BTL_CONDITION_TYPE_DEPEND_MON;
    condition.common.turns = monId;
    return condition;
}

void BtlCalc_MakeDefaultCondition(s32 condition, BattleMon *mon, BattleCondition *out) {
    if (condition < 6) {
        *out = BtlCalc_MakeStatusCondition(condition);
        return;
    }
    out->raw = 0;
    switch (condition) {
    case CONDITION_ATTRACT:
        *out = MakeConditionDependPoke(GetMonID(mon));
        break;
    case CONDITION_CONFUSION:
        *out = SetConditionTurns(BtlCalc_RandomRange(2, 5));
        break;
    default:
        *out = MakeConditionPermanent();
        break;
    }
}

// The first condition of mon in the list of ov169's, or 0 if it has none of them
u32 BtlCalc_FindMentalHerbCondition(BattleMon *mon) {
    u32 i = 0;

    while (TRUE) {
        u32 condition = GetMentalHerbCondition(i++);
        if (condition == 0) {
            return condition;
        }
        if (CheckCondition(mon, condition)) {
            return condition;
        }
    }
}

// A random move that isn't in excluded
u16 BtlCalc_PickRandomMove(const u16 *excluded, u16 count) {
    u16 move;
    u16 numMoves = 0;
    u16 index;

    for (move = 1; move < 0x230; move++) {
        if (!BtlCalc_IsMoveInList(excluded, count, move)) {
            g_BtlCalcWork.moves[numMoves++] = move;
        }
    }
    index = BattleRandom(numMoves);
    return g_BtlCalcWork.moves[index];
}

BOOL BtlCalc_IsMoveInList(const u16 *moves, u32 count, u16 move) {
    u32 i;

    for (i = 0; i < count; i++) {
        if (move == moves[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

u32 CalcBaseExpGain(BattleMon *mon, s32 levelDiff) {
    u32 species = GetBattleMonSpecies(mon);
    u16 form = GetBattleMonStat(mon, BATTLEMON_FORM);
    u16 level;
    u32 baseExp;

    if (levelDiff < 0) {
        levelDiff = -levelDiff;
    } else {
        levelDiff = 0;
    }
    level = (u16)levelDiff + GetBattleMonStat(mon, BATTLEMON_LEVEL);
    baseExp = PML_PersonalGetParamSingle(species, form, PERSONAL_BASE_EXP);
    return baseExp * level / 5;
}

BOOL BtlCalc_IsDoubleOrTripleStyle(u32 value) {
    if (value != BTL_STYLE_SINGLE && value != BTL_STYLE_ROTATION) {
        return TRUE;
    }
    return FALSE;
}

u32 BtlCalc_GetFrontPosCount(u32 value) {
    switch (value) {
    case BTL_STYLE_SINGLE:
        return 1;
    case BTL_STYLE_DOUBLE:
        return 2;
    case BTL_STYLE_TRIPLE:
        return 3;
    case BTL_STYLE_ROTATION:
        return 1;
    }
    return 1;
}

// Function name from swan.
u8 GetNumMonsOnField(u32 battleStyle, u8 count) {
    if (battleStyle == BTL_STYLE_ROTATION) {
        count = 3;
    }
    return count;
}

BOOL BtlCalc_IsGymLeaderClass(u32 trainerClass) {
    if (GetTrainerClassBGMGroupId(trainerClass) == 0) {
        return TRUE;
    }
    return FALSE;
}

BOOL BtlCalc_IsEliteFourClass(u32 trainerClass) {
    if (GetTrainerClassBGMGroupId(trainerClass) == 1) {
        return TRUE;
    }
    return FALSE;
}

BOOL BtlCalc_IsBossTrainerClass(u16 trainerClass) {
    if (BtlCalc_IsGymLeaderClass(trainerClass)) {
        return TRUE;
    }
    if (BtlCalc_IsEliteFourClass(trainerClass)) {
        return TRUE;
    }
    return trainerClass == 0x59;
}

// The prize money of the opposing trainers
u32 BtlCalc_GetPrizeMoney(BtlSetup *setup) {
    u32 money = 0;

    if (setup->battleType == 1) {
        if (setup->trainers[1] != NULL && setup->party[1] != NULL) {
            money += BtlCalc_GetTrainerPrizeMoney(setup->trainers[1], setup->party[1]);
        }
        if (setup->trainers[3] != NULL && setup->party[3] != NULL) {
            money += BtlCalc_GetTrainerPrizeMoney(setup->trainers[3], setup->party[3]);
        }
    }
    return money;
}

// A trainer's prize money: their base money times four times the level of their last Pokémon
u32 BtlCalc_GetTrainerPrizeMoney(BtlSetupTrainer *trainer, PokeParty *party) {
    if (party != NULL) {
        u8 count = PokeParty_GetPkmCount(party);

        if (count != 0) {
            PartyPkm *pkm = PokeParty_GetPkm(party, count - 1);
            u32 baseMoney = TrainerData_GetParam(trainer->trainerId, TRAINER_PARAM_MONEY);

            return PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL) * baseMoney * 4;
        }
    }
    return 0;
}

// The money lost: four times the highest level in the party times a multiplier
u32 BtlCalc_GetLostMoney(u32 index, BattleParty *party) {
    u32 count = GetNumMonsInParty(party);
    u32 maxLevel = 1;
    u32 i;

    for (i = 0; i < count; i++) {
        u32 level = GetBattleMonStat(GetBattleMonFromParty(party, i), BATTLEMON_LEVEL);

        if (level > maxLevel) {
            maxLevel = level;
        }
    }
    if (index >= 9) {
        index = 8;
    }
    return maxLevel * 4 * LOST_MONEY_MULTIPLIERS[index];
}

void BtlCalc_SetHeapID(HeapID heapId) {
    g_BtlCalcWork.heapId = heapId;
}

void func_ov167_021bd880(void) {
}

u32 ItemGetParam(u16 item, u32 param) {
    return GetItemParam(item, param, g_BtlCalcWork.heapId);
}

// Packs six 5-bit values
u32 BtlCalc_PackMonIDs(const u8 *values) {
    u32 packed = 0;
    u32 i;

    for (i = 0; i < 6; i++) {
        packed |= (values[i] & 0x1f) << (i * 5);
    }
    return packed;
}

void BtlCalc_UnpackMonIDs(u32 packed, u8 *values) {
    u32 i;

    for (i = 0; i < 6; i++) {
        values[i] = packed & 0x1f;
        packed >>= 5;
    }
}

// Curse's move target: the user unless it's a Ghost type
u32 BtlCalc_GetCurseTarget(BattleMon *mon) {
    u32 target = MOVE_TARGET_USER;

    if (DoesMonHaveType(mon, TYPE_GHOST)) {
        target = MOVE_TARGET_SELECTED;
    }
    return target;
}

u8 DecideMoveTargetAuto(BtlMainModule *mainModule, BtlPokeCon *pokeCon, BattleMon *mon, u16 move) {
    u32 battleStyle = BtlSetup_GetBattleStyle(mainModule);
    u8 pos = MonIDToBattlePos(mainModule, pokeCon, GetMonID(mon));
    u32 target = PML_MoveGetParam(move, MOVE_PARAM_TARGET);
    u8 monIds[6];
    u8 count;

    if (move == MOVE_CURSE) {
        target = BtlCalc_GetCurseTarget(mon);
    }
    if (battleStyle == BTL_STYLE_SINGLE) {
        switch (target) {
        case MOVE_TARGET_SELECTED:
        case MOVE_TARGET_FOE:
        case MOVE_TARGET_ALL_ADJACENT:
        case MOVE_TARGET_ADJACENT_FOES:
        case MOVE_TARGET_RANDOM_FOE:
            return GetOpposingPos(mainModule, pos, 0);
        case MOVE_TARGET_USER_OR_ALLY:
        case MOVE_TARGET_USER:
            return pos;
        case MOVE_TARGET_ALLY:
        case MOVE_TARGET_USER_PARTY:
        case MOVE_TARGET_ALL:
        case MOVE_TARGET_FIELD:
        case MOVE_TARGET_FOE_SIDE:
        case MOVE_TARGET_USER_SIDE:
        case MOVE_TARGET_DEPENDS:
        default:
            return BTL_POS_NONE;
        }
    } else {
        u16 posFlags;
        u8 farTarget = FALSE;

        if (battleStyle == BTL_STYLE_TRIPLE) {
            farTarget = getMoveFlag(move, 0xb);
        }
        switch (target) {
        case MOVE_TARGET_SELECTED:
        case MOVE_TARGET_FOE:
        case MOVE_TARGET_RANDOM_FOE:
            if (farTarget) {
                posFlags = (6 << 8) | pos;
            } else {
                posFlags = (1 << 8) | pos;
            }
            break;
        case MOVE_TARGET_USER_OR_ALLY:
            posFlags = (3 << 8) | pos;
            break;
        case MOVE_TARGET_ALLY:
            if (farTarget) {
                posFlags = (7 << 8) | pos;
            } else {
                posFlags = (4 << 8) | pos;
            }
            break;
        case MOVE_TARGET_USER:
            return pos;
        default:
            return BTL_POS_NONE;
        }
        count = ExpandPosFlagsToMonIDs(mainModule, pokeCon, posFlags, monIds);
        if (count != 0) {
            u8 index = BattleRandom(count);
            return MonIDToBattlePos(mainModule, pokeCon, monIds[index]);
        }
        count = ExpandPosFlags(mainModule, posFlags, monIds);
        if (count != 0) {
            u8 index = BattleRandom(count);
            return monIds[index];
        }
        return BTL_POS_NONE;
    }
}

// Function name from swan.
u8 DecideMoveTargetAutoForClient(BtlMainModule *mainModule, BtlPokeCon *pokeCon, BattleMon *mon, u16 move,
                                 MATHRandContext32 *saved) {
    u8 pos;

    *saved = g_BtlCalcWork.rand;
    pos = DecideMoveTargetAuto(mainModule, pokeCon, mon, move);
    g_BtlCalcWork.rand = *saved;
    return pos;
}

void BtlClientIDList_Init(BtlClientIDList *list) {
    u32 i;

    for (i = 0; i < 4; i++) {
        list->clientIds[i] = 4;
    }
    list->count = 0;
}

void BtlClientIDList_Add(BtlClientIDList *list, u8 clientId) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (clientId == list->clientIds[i]) {
            return;
        }
        if (list->clientIds[i] == 4) {
            list->clientIds[i] = clientId;
            list->count++;
            return;
        }
    }
}

u32 BtlClientIDList_GetCount(BtlClientIDList *list) {
    return list->count;
}

u32 BtlClientIDList_GetEscapeResult(BtlClientIDList *list, u8 clientId, u32 value) {
    BOOL hasAlly = FALSE;
    BOOL hasOpponent = FALSE;
    u32 i;

    for (i = 0; i < list->count; i++) {
        if (IsAllyClientID(list->clientIds[i], clientId)) {
            hasAlly = TRUE;
        } else {
            hasOpponent = TRUE;
        }
    }
    if (hasAlly && hasOpponent) {
        if (value <= 1) {
            return BTL_RESULT_FLEE;
        }
        return BTL_RESULT_DRAW;
    }
    return hasAlly ? BTL_RESULT_FLEE : BTL_RESULT_FLEE_FOE;
}
