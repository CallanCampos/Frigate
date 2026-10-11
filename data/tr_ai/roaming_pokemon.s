#include "asm/tr_ai.inc"

// AI flag 11, RoamingPokemon: flees, unless the Pokemon is trapped.

RoamingPokemon_Main:
    IfCondition AI_BATTLER_ATTACKER, CONDITION_BIND, RoamingPokemon_Trapped
    IfCondition AI_BATTLER_ATTACKER, CONDITION_MEAN_LOOK, RoamingPokemon_Trapped
    LoadAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_SHADOW_TAG, RoamingPokemon_Trapped
    LoadAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_LEVITATE, RoamingPokemon_NotTrapped
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_FLYING, RoamingPokemon_NotTrapped
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_FLYING, RoamingPokemon_NotTrapped
    LoadAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_ARENA_TRAP, RoamingPokemon_Trapped

RoamingPokemon_NotTrapped:
    Escape

RoamingPokemon_Trapped:
    End
