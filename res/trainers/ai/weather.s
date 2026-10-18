#include "asm/tr_ai.inc"

// AI flag 9, Weather: on the first turn, a weather move gains 5 points when its weather isn't already active and it
// is the attacker's first turn in battle.

Weather_Main:
    IfTargetIsPartner Weather_Terminate
    LoadTurnCount
    IfLoadedNotEqualTo 0, Weather_Terminate
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_WEATHER_SUN, Weather_Sun
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_WEATHER_RAIN, Weather_Rain
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_WEATHER_SANDSTORM, Weather_Sand
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_WEATHER_HAIL, Weather_Hail
    // Moves with other effects go on to the sun check

Weather_Sun:
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_SUN, Weather_Terminate
    GoTo Weather_ScorePlus5

Weather_Rain:
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_RAIN, Weather_Terminate
    GoTo Weather_ScorePlus5

Weather_Sand:
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_SANDSTORM, Weather_Terminate
    GoTo Weather_ScorePlus5

Weather_Hail:
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_HAIL, Weather_Terminate
    GoTo Weather_ScorePlus5

Weather_ScorePlus5:
    LoadIsFirstTurnInBattle AI_BATTLER_ATTACKER
    IfLoadedEqualTo FALSE, Weather_Terminate
    AddToMoveScore 5

Weather_Terminate:
    End
    .balign 4, 0
