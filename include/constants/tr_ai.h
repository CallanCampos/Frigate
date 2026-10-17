#ifndef POKEBW2_CONSTANTS_TR_AI_H
#define POKEBW2_CONSTANTS_TR_AI_H

// Constants of the trainer AI scripts, see src/ov170/tr_ai.c. Names follow pokeplatinum's where the command is the same

// The AI flags of a trainer. Each flag runs the script with its bit number, res/trainers/ai/NN_*.s
#define AI_FLAG_BASIC (1 << 0)
#define AI_FLAG_EVAL_ATTACK (1 << 1)
#define AI_FLAG_EXPERT (1 << 2)
#define AI_FLAG_SETUP_FIRST_TURN (1 << 3)
#define AI_FLAG_TARGET_HP (1 << 4)
#define AI_FLAG_FUSION_MOVES (1 << 5)
#define AI_FLAG_BATON_PASS (1 << 6)
#define AI_FLAG_TAG_STRATEGY (1 << 7)
#define AI_FLAG_CHECK_HP (1 << 8)
#define AI_FLAG_WEATHER (1 << 9)
#define AI_FLAG_HARASSMENT (1 << 10)
#define AI_FLAG_ROAMING_POKEMON (1 << 11)
#define AI_FLAG_SAFARI (1 << 12)
#define AI_FLAG_CATCH_TUTORIAL (1 << 13)

// The Pokemon that a command refers to
#define AI_BATTLER_DEFENDER 0
#define AI_BATTLER_ATTACKER 1
#define AI_BATTLER_DEFENDER_PARTNER 2
#define AI_BATTLER_ATTACKER_PARTNER 3

// Ends a table for IfLoadedInTable
#define TABLE_END 0xFFFFFFFF

// The type that LoadTypeFrom loads
#define LOAD_DEFENDER_TYPE_1 0
#define LOAD_ATTACKER_TYPE_1 1
#define LOAD_DEFENDER_TYPE_2 2
#define LOAD_ATTACKER_TYPE_2 3
#define LOAD_MOVE_TYPE 4
#define LOAD_DEFENDER_PARTNER_TYPE_1 5
#define LOAD_ATTACKER_PARTNER_TYPE_1 6
#define LOAD_DEFENDER_PARTNER_TYPE_2 7
#define LOAD_ATTACKER_PARTNER_TYPE_2 8

// The damage that the damage commands calculate: the lowest of the random rolls (85%), or a random roll. Gen 4 uses
// the highest roll where this game uses the lowest
#define USE_MIN_DAMAGE 0
#define ROLL_FOR_DAMAGE 1

// What FlagMoveDamageScore and CheckIfHighestDamageWithPartner load
#define AI_MOVE_DEALS_NO_DAMAGE 0
#define AI_NOT_HIGHEST_DAMAGE 1
#define AI_MOVE_IS_HIGHEST_DAMAGE 2

// How IfSpeedCompareEqualTo compares the attacker's speed with the defender's
#define COMPARE_SPEED_FASTER 0
#define COMPARE_SPEED_SLOWER 1
#define COMPARE_SPEED_TIE 2

// How IfLevel compares the attacker's level with the defender's
#define CHECK_HIGHER_THAN_TARGET 0
#define CHECK_LOWER_THAN_TARGET 1
#define CHECK_EQUAL_TO_TARGET 2

#endif // POKEBW2_CONSTANTS_TR_AI_H
