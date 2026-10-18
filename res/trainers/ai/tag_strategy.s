#include "asm/tr_ai.inc"

// AI flag 7, TagStrategy: double and triple battles only. Scores moves by how they affect the attacker's partners,
// such as spread moves that also hit them, and from TagStrategy_Partner, moves aimed at a partner.

// Only for double and triple battles
TagStrategy_Main:
    LoadBattleStyle

TagStrategy_CheckBattleStyle:
    IfLoadedEqualTo BTL_STYLE_DOUBLE, TagStrategy_MultiBattle
    IfLoadedEqualTo BTL_STYLE_TRIPLE, TagStrategy_MultiBattle
    End

TagStrategy_MultiBattle:
    IfTargetIsPartner TagStrategy_Partner
    FlagMoveDamageScore USE_MIN_DAMAGE
    IfLoadedEqualTo AI_MOVE_DEALS_NO_DAMAGE, TagStrategy_CheckSpecialScoring
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_ONE_HIT_KO, TagStrategy_ScoreMove
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_40_DAMAGE_FLAT, TagStrategy_ScoreMove
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_LEVEL_DAMAGE_FLAT, TagStrategy_ScoreMove
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_RANDOM_DAMAGE_1_TO_150_LEVEL, TagStrategy_ScoreMove
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_20_DAMAGE_FLAT, TagStrategy_ScoreMove
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, TagStrategy_TryScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, TagStrategy_TryScoreMinus2
    GoTo TagStrategy_ScoreMove

TagStrategy_TryScoreMinus1:
    IfCurrentMoveKills 0, TagStrategy_ScoreMove
    IfHPPercentEqualTo AI_BATTLER_DEFENDER_PARTNER, 0, TagStrategy_ScoreMove
    IfRandomLessThan 64, TagStrategy_ScoreMove
    AddToMoveScore -1
    GoTo TagStrategy_ScoreMove

TagStrategy_TryScoreMinus2:
    IfCurrentMoveKills 0, TagStrategy_ScoreMove
    IfHPPercentEqualTo AI_BATTLER_DEFENDER_PARTNER, 0, TagStrategy_ScoreMove
    IfRandomLessThan 64, TagStrategy_ScoreMove
    AddToMoveScore -2
    GoTo TagStrategy_ScoreMove

TagStrategy_ScoreMove:
    CheckIfHighestDamageWithPartner USE_MIN_DAMAGE
    IfLoadedNotEqualTo AI_MOVE_IS_HIGHEST_DAMAGE, TagStrategy_CheckBeforeScoring
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_HALVE_DEFENSE, TagStrategy_CheckSpecialScoring
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_PRIORITY_1, TagStrategy_TryScorePlus1
    IfRandomLessThan 128, TagStrategy_CheckBeforeScoring
    AddToMoveScore 1
    GoTo TagStrategy_CheckSpecialScoring

TagStrategy_TryScorePlus1:
    IfRandomLessThan 50, TagStrategy_CheckBeforeScoring
    AddToMoveScore 1
    GoTo TagStrategy_CheckSpecialScoring

TagStrategy_CheckBeforeScoring:
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_ONE_HIT_KO, TagStrategy_CheckSpecialScoring
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_40_DAMAGE_FLAT, TagStrategy_CheckSpecialScoring
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_LEVEL_DAMAGE_FLAT, TagStrategy_CheckSpecialScoring
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_RANDOM_DAMAGE_1_TO_150_LEVEL, TagStrategy_CheckSpecialScoring
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_20_DAMAGE_FLAT, TagStrategy_CheckSpecialScoring
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_DOUBLE, TagStrategy_TryPrioritizingDoubleEffective
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUADRUPLE, TagStrategy_TryPrioritizingQuadEffective
    GoTo TagStrategy_CheckSpecialScoring

TagStrategy_TryPrioritizingDoubleEffective:
    IfRandomLessThan 100, TagStrategy_CheckSpecialScoring
    AddToMoveScore 1
    GoTo TagStrategy_CheckSpecialScoring

TagStrategy_TryPrioritizingQuadEffective:
    IfRandomLessThan 64, TagStrategy_CheckSpecialScoring
    AddToMoveScore 1
    GoTo TagStrategy_CheckSpecialScoring

TagStrategy_CheckSpecialScoring:
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_CIRCLE_THROW, TagStrategy_CircleThrow
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_DECREASE_POWER_WITH_LESS_USER_HP, TagStrategy_CircleThrow
    IfMoveEqualTo MOVE_BLIZZARD, TagStrategy_CircleThrow
    IfMoveEqualTo MOVE_BLIZZARD, TagStrategy_CircleThrow
    IfMoveEqualTo MOVE_BLIZZARD, TagStrategy_CircleThrow
    IfMoveEqualTo MOVE_WIDE_GUARD, TagStrategy_WideGuard
    IfMoveEqualTo MOVE_ROUND, TagStrategy_Round
    IfMoveEqualTo MOVE_ALLY_SWITCH, TagStrategy_AllySwitch
    IfMoveEqualTo MOVE_QUASH, TagStrategy_Quash
    IfMoveEqualTo MOVE_BESTOW, TagStrategy_Bestow
    IfMoveEqualTo MOVE_SKILL_SWAP, TagStrategy_SkillSwap
    LoadTypeFrom LOAD_MOVE_TYPE
    IfMoveEqualTo MOVE_EARTHQUAKE, TagStrategy_Earthquake
    IfMoveEqualTo MOVE_MAGNITUDE, TagStrategy_Earthquake
    IfMoveEqualTo MOVE_FUTURE_SIGHT, TagStrategy_FutureSight
    IfMoveEqualTo MOVE_DOOM_DESIRE, TagStrategy_FutureSight
    IfMoveEqualTo MOVE_RAIN_DANCE, TagStrategy_RainDance
    IfMoveEqualTo MOVE_SUNNY_DAY, TagStrategy_SunnyDay
    IfMoveEqualTo MOVE_HAIL, TagStrategy_Hail
    IfMoveEqualTo MOVE_SANDSTORM, TagStrategy_Sandstorm
    IfMoveEqualTo MOVE_GRAVITY, TagStrategy_Gravity
    IfMoveEqualTo MOVE_TRICK_ROOM, TagStrategy_TrickRoom
    IfMoveEqualTo MOVE_FOLLOW_ME, TagStrategy_FollowMe
    IfMoveEqualTo MOVE_HEAL_PULSE, TagStrategy_CircleThrow_ScoreMinus40
    IfMoveEqualTo MOVE_AFTER_YOU, TagStrategy_CircleThrow_ScoreMinus40
    IfMoveEqualTo MOVE_HELPING_HAND, TagStrategy_CircleThrow_ScoreMinus40
    LoadTypeFrom LOAD_MOVE_TYPE
    IfLoadedEqualTo TYPE_ELECTRIC, TagStrategy_CheckElectricMove
    IfLoadedEqualTo TYPE_FIRE, TagStrategy_CheckFireMove
    IfLoadedEqualTo TYPE_WATER, TagStrategy_CheckWaterMove
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_HELPING_HAND, TagStrategy_PartnerKnowsHelpingHand
    End

TagStrategy_RainDance:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_HYDRATION, TagStrategy_RainDance_SelfHasHydration
    IfLoadedEqualTo ABILITY_DRY_SKIN, TagStrategy_RainDance_SelfScorePlus2
    GoTo TagStrategy_RainDance_CheckPartner

TagStrategy_RainDance_SelfHasHydration:
    IfNotStatus AI_BATTLER_ATTACKER, TagStrategy_RainDance_CheckPartner

TagStrategy_RainDance_SelfScorePlus2:
    AddToMoveScore 2
    GoTo TagStrategy_RainDance_CheckPartner

TagStrategy_RainDance_CheckPartner:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_HYDRATION
    IfLoadedEqualTo TRUE, TagStrategy_RainDance_PartnerHasHydration
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_DRY_SKIN
    IfLoadedEqualTo TRUE, TagStrategy_RainDance_PartnerScorePlus2
    GoTo TagStrategy_RainDance_End

TagStrategy_RainDance_PartnerHasHydration:
    IfNotStatus AI_BATTLER_ATTACKER_PARTNER, TagStrategy_RainDance_End

TagStrategy_RainDance_PartnerScorePlus2:
    AddToMoveScore 2
    GoTo TagStrategy_RainDance_End

TagStrategy_RainDance_End:
    End

TagStrategy_SunnyDay:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_LEAF_GUARD, TagStrategy_SunnyDay_SelfHasLeafGuard
    IfLoadedEqualTo ABILITY_FLOWER_GIFT, TagStrategy_SunnyDay_SelfScorePlus2
    IfLoadedEqualTo ABILITY_DRY_SKIN, TagStrategy_SunnyDay_SelfScoreMinus2
    IfLoadedEqualTo ABILITY_SOLAR_POWER, TagStrategy_SunnyDay_SelfHasSolarPower
    GoTo TagStrategy_SunnyDay_CheckPartner

TagStrategy_SunnyDay_SelfHasLeafGuard:
    IfStatus AI_BATTLER_ATTACKER, TagStrategy_SunnyDay_CheckPartner
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 30, TagStrategy_SunnyDay_CheckPartner

TagStrategy_SunnyDay_SelfScorePlus2:
    AddToMoveScore 2
    GoTo TagStrategy_SunnyDay_CheckPartner

TagStrategy_SunnyDay_SelfScoreMinus2:
    AddToMoveScore -2
    GoTo TagStrategy_SunnyDay_CheckPartner

TagStrategy_SunnyDay_SelfHasSolarPower:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 50, TagStrategy_SunnyDay_SelfTryScoreMinus2
    AddToMoveScore 1

TagStrategy_SunnyDay_SelfTryScoreMinus2:
    IfRandomLessThan 128, TagStrategy_SunnyDay_CheckPartner
    AddToMoveScore -2

TagStrategy_SunnyDay_CheckPartner:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_LEAF_GUARD
    IfLoadedEqualTo TRUE, TagStrategy_SunnyDay_PartnerHasLeafGuard
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_FLOWER_GIFT
    IfLoadedEqualTo TRUE, TagStrategy_SunnyDay_PartnerScorePlus2
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_DRY_SKIN
    IfLoadedEqualTo TRUE, TagStrategy_SunnyDay_PartnerScoreMinus2
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_SOLAR_POWER
    IfLoadedEqualTo TRUE, TagStrategy_SunnyDay_PartnerHasSolarPower
    GoTo TagStrategy_SunnyDay_End

TagStrategy_SunnyDay_PartnerHasLeafGuard:
    IfStatus AI_BATTLER_ATTACKER_PARTNER, TagStrategy_SunnyDay_End
    IfHPPercentLessThan AI_BATTLER_ATTACKER_PARTNER, 30, TagStrategy_SunnyDay_End

TagStrategy_SunnyDay_PartnerScorePlus2:
    AddToMoveScore 2
    GoTo TagStrategy_SunnyDay_End

TagStrategy_SunnyDay_PartnerScoreMinus2:
    AddToMoveScore -2
    GoTo TagStrategy_SunnyDay_End

TagStrategy_SunnyDay_PartnerHasSolarPower:
    IfHPPercentLessThan AI_BATTLER_ATTACKER_PARTNER, 50, TagStrategy_SunnyDay_PartnerTryScoreMinus2
    AddToMoveScore 1

TagStrategy_SunnyDay_PartnerTryScoreMinus2:
    IfRandomLessThan 128, TagStrategy_SunnyDay_End
    AddToMoveScore -2

TagStrategy_SunnyDay_End:
    End

TagStrategy_Hail:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_ICE_BODY, TagStrategy_Hail_SelfScorePlus2
    IfLoadedEqualTo ABILITY_SNOW_CLOAK, TagStrategy_Hail_SelfScorePlus2
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_BLIZZARD, TagStrategy_Hail_SelfScorePlus2
    GoTo TagStrategy_Hail_CheckPartner

TagStrategy_Hail_SelfScorePlus2:
    AddToMoveScore 2

TagStrategy_Hail_CheckPartner:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_ICE_BODY
    IfLoadedEqualTo TRUE, TagStrategy_Hail_PartnerScorePlus2
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_SNOW_CLOAK
    IfLoadedEqualTo TRUE, TagStrategy_Hail_PartnerScorePlus2
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_BLIZZARD, TagStrategy_Hail_PartnerScorePlus2
    GoTo TagStrategy_Hail_End

TagStrategy_Hail_PartnerScorePlus2:
    AddToMoveScore 2

TagStrategy_Hail_End:
    End

TagStrategy_Sandstorm:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_SAND_VEIL, TagStrategy_Sandstorm_SelfScorePlus2
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_ROCK, TagStrategy_Sandstorm_SelfScorePlus2
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_ROCK, TagStrategy_Sandstorm_SelfScorePlus2
    GoTo TagStrategy_Sandstorm_CheckPartner

TagStrategy_Sandstorm_SelfScorePlus2:
    AddToMoveScore 2
    GoTo TagStrategy_Sandstorm_CheckPartner

TagStrategy_Sandstorm_CheckPartner:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_SAND_VEIL
    IfLoadedEqualTo TRUE, TagStrategy_Sandstorm_PartnerScorePlus2
    LoadTypeFrom LOAD_ATTACKER_PARTNER_TYPE_1
    IfLoadedEqualTo TYPE_ROCK, TagStrategy_Sandstorm_PartnerScorePlus2
    LoadTypeFrom LOAD_ATTACKER_PARTNER_TYPE_2
    IfLoadedEqualTo TYPE_ROCK, TagStrategy_Sandstorm_PartnerScorePlus2
    GoTo TagStrategy_Sandstorm_End

TagStrategy_Sandstorm_PartnerScorePlus2:
    AddToMoveScore 2

TagStrategy_Sandstorm_End:
    End

TagStrategy_Gravity:
    IfFieldCondition FIELD_CONDITION_GRAVITY, TagStrategy_PartnerScoreMinus30
    CheckBattlerAbility AI_BATTLER_ATTACKER, ABILITY_LEVITATE
    IfLoadedEqualTo TRUE, TagStrategy_Gravity_SelfScoreMinus5
    FlagBattlerIsType AI_BATTLER_ATTACKER, TYPE_FLYING
    IfLoadedEqualTo TRUE, TagStrategy_Gravity_SelfScoreMinus5
    IfCondition AI_BATTLER_ATTACKER, 30, TagStrategy_Gravity_SelfScoreMinus5
    GoTo TagStrategy_Gravity_CheckPartner

TagStrategy_Gravity_SelfScoreMinus5:
    AddToMoveScore -5
    GoTo TagStrategy_Gravity_CheckPartner

TagStrategy_Gravity_CheckPartner:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_LEVITATE
    IfLoadedEqualTo TRUE, TagStrategy_Gravity_PartnerScoreMinus5
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_FLYING
    IfLoadedEqualTo TRUE, TagStrategy_Gravity_PartnerScoreMinus5
    IfCondition AI_BATTLER_ATTACKER_PARTNER, 30, TagStrategy_Gravity_PartnerScoreMinus5
    GoTo TagStrategy_Gravity_CheckTarget

TagStrategy_Gravity_PartnerScoreMinus5:
    AddToMoveScore -5
    GoTo TagStrategy_Gravity_CheckTarget

TagStrategy_Gravity_CheckTarget:
    CheckBattlerAbility AI_BATTLER_DEFENDER, ABILITY_LEVITATE
    IfLoadedEqualTo TRUE, TagStrategy_Gravity_TargetTryScorePlus3
    FlagBattlerIsType AI_BATTLER_DEFENDER, TYPE_FLYING
    IfLoadedEqualTo TRUE, TagStrategy_Gravity_TargetTryScorePlus3
    IfCondition AI_BATTLER_DEFENDER, 30, TagStrategy_Gravity_TargetTryScorePlus3
    GoTo TagStrategy_Gravity_CheckTargetPartner

TagStrategy_Gravity_TargetTryScorePlus3:
    IfRandomLessThan 64, TagStrategy_Gravity_CheckTargetPartner
    AddToMoveScore 3
    GoTo TagStrategy_Gravity_CheckTargetPartner

TagStrategy_Gravity_CheckTargetPartner:
    CheckBattlerAbility AI_BATTLER_DEFENDER_PARTNER, ABILITY_LEVITATE
    IfLoadedEqualTo TRUE, TagStrategy_Gravity_TargetPartnerTryScorePlus3
    FlagBattlerIsType AI_BATTLER_DEFENDER_PARTNER, TYPE_FLYING
    IfLoadedEqualTo TRUE, TagStrategy_Gravity_TargetPartnerTryScorePlus3
    IfCondition AI_BATTLER_DEFENDER_PARTNER, 30, TagStrategy_Gravity_TargetPartnerTryScorePlus3
    GoTo TagStrategy_Gravity_End

TagStrategy_Gravity_TargetPartnerTryScorePlus3:
    IfRandomLessThan 64, TagStrategy_Gravity_End
    AddToMoveScore 3
    GoTo TagStrategy_Gravity_End

TagStrategy_Gravity_End:
    End

TagStrategy_TrickRoom:
    IfHPPercentEqualTo AI_BATTLER_ATTACKER_PARTNER, 0, ScoreMinus30
    IfHPPercentEqualTo AI_BATTLER_DEFENDER_PARTNER, 0, ScoreMinus30
    IfHPPercentEqualTo AI_BATTLER_DEFENDER, 0, ScoreMinus30
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, TagStrategy_TrickRoom_SelfMovesFirst
    IfLoadedEqualTo 1, TagStrategy_TrickRoom_SelfMovesSecond
    IfLoadedEqualTo 2, TagStrategy_TrickRoom_SelfMovesThird
    IfLoadedEqualTo 3, TagStrategy_TrickRoom_SelfMovesLast
    GoTo TagStrategy_TrickRoom_End

TagStrategy_TrickRoom_SelfMovesFirst:
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedEqualTo 1, ScoreMinus30
    IfLoadedEqualTo 0, ScoreMinus30
    GoTo TagStrategy_TrickRoom_ScoreMinus5

TagStrategy_TrickRoom_SelfMovesSecond:
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedEqualTo 0, ScoreMinus30
    GoTo TagStrategy_TrickRoom_ScoreMinus5

TagStrategy_TrickRoom_SelfMovesThird:
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedNotEqualTo 3, TagStrategy_TrickRoom_ScoreMinus5
    IfRandomLessThan 64, TagStrategy_TrickRoom_ScoreMinus5
    AddToMoveScore 5
    GoTo TagStrategy_TrickRoom_End

TagStrategy_TrickRoom_SelfMovesLast:
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedNotEqualTo 2, TagStrategy_TrickRoom_ScoreMinus5
    IfRandomLessThan 64, TagStrategy_TrickRoom_ScoreMinus5
    AddToMoveScore 5
    GoTo TagStrategy_TrickRoom_End

TagStrategy_TrickRoom_ScoreMinus5:
    AddToMoveScore -5

TagStrategy_TrickRoom_End:
    End

TagStrategy_FollowMe:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 90, TagStrategy_FollowMe_SelfHighHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, TagStrategy_FollowMe_SelfMediumHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 30, TagStrategy_FollowMe_SelfLowHP
    IfRandomLessThan 64, TagStrategy_FollowMe_End
    GoTo ScoreMinus5

TagStrategy_FollowMe_SelfHighHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 90, TagStrategy_FollowMe_TryScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 50, TagStrategy_FollowMe_TryScorePlus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 30, TagStrategy_FollowMe_TryScorePlus2
    GoTo TagStrategy_FollowMe_TryScorePlus3

TagStrategy_FollowMe_SelfMediumHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 90, TagStrategy_FollowMe_TryScoreMinus2
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 50, TagStrategy_FollowMe_TryScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 30, TagStrategy_FollowMe_TryScorePlus1
    GoTo TagStrategy_FollowMe_TryScorePlus2

TagStrategy_FollowMe_SelfLowHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 90, TagStrategy_FollowMe_TryScoreMinus2
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 50, TagStrategy_FollowMe_TryScoreMinus2
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 30, TagStrategy_FollowMe_TryScorePlus1
    GoTo TagStrategy_FollowMe_TryScorePlus2

TagStrategy_FollowMe_TryScoreMinus1:
    IfRandomLessThan 64, TagStrategy_FollowMe_End
    AddToMoveScore -1
    GoTo TagStrategy_FollowMe_End

TagStrategy_FollowMe_TryScoreMinus2:
    IfRandomLessThan 64, TagStrategy_FollowMe_End
    AddToMoveScore -2
    GoTo TagStrategy_FollowMe_End

TagStrategy_FollowMe_TryScorePlus1:
    IfRandomLessThan 64, TagStrategy_FollowMe_End
    AddToMoveScore 1
    GoTo TagStrategy_FollowMe_End

TagStrategy_FollowMe_TryScorePlus2:
    IfRandomLessThan 64, TagStrategy_FollowMe_End
    AddToMoveScore 2
    GoTo TagStrategy_FollowMe_End

TagStrategy_FollowMe_TryScorePlus3:
    IfRandomLessThan 64, TagStrategy_FollowMe_End
    AddToMoveScore 3
    GoTo TagStrategy_FollowMe_End

TagStrategy_FollowMe_End:
    End

TagStrategy_PartnerKnowsHelpingHand:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, TagStrategy_PartnerKnowsHelpingHand_CheckMove
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedLessThan 1, TagStrategy_PartnerKnowsHelpingHand_CheckMove
    GoTo TagStrategy_PartnerKnowsHelpingHand_End

TagStrategy_PartnerKnowsHelpingHand_CheckMove:
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_ONE_HIT_KO, TagStrategy_PartnerKnowsHelpingHand_End
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_40_DAMAGE_FLAT, TagStrategy_PartnerKnowsHelpingHand_End
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_LEVEL_DAMAGE_FLAT, TagStrategy_PartnerKnowsHelpingHand_End
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_RANDOM_DAMAGE_1_TO_150_LEVEL, TagStrategy_PartnerKnowsHelpingHand_End
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_20_DAMAGE_FLAT, TagStrategy_PartnerKnowsHelpingHand_End
    FlagMoveDamageScore USE_MIN_DAMAGE
    IfLoadedEqualTo AI_MOVE_DEALS_NO_DAMAGE, TagStrategy_PartnerKnowsHelpingHand_End
    IfTurnRandomLessThan 128, TagStrategy_PartnerKnowsHelpingHand_End
    AddToMoveScore 3

TagStrategy_PartnerKnowsHelpingHand_End:
    End

TagStrategy_Unused_1:
    IfStatus AI_BATTLER_ATTACKER, TagStrategy_Unused_2
    End

TagStrategy_Unused_2:
    FlagMoveDamageScore USE_MIN_DAMAGE
    IfLoadedEqualTo AI_MOVE_DEALS_NO_DAMAGE, ScoreMinus5
    AddToMoveScore 1
    IfLoadedEqualTo AI_MOVE_IS_HIGHEST_DAMAGE, ScorePlus2
    End

TagStrategy_Earthquake:
    IfCondition AI_BATTLER_ATTACKER_PARTNER, 30, ScorePlus2
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_LEVITATE
    IfLoadedEqualTo TRUE, ScorePlus2
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_FLYING
    IfLoadedEqualTo TRUE, ScorePlus2
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_FIRE
    IfLoadedEqualTo TRUE, ScoreMinus10
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_ELECTRIC
    IfLoadedEqualTo TRUE, ScoreMinus10
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_POISON
    IfLoadedEqualTo TRUE, ScoreMinus10
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_ROCK
    IfLoadedEqualTo TRUE, ScoreMinus10
    GoTo ScoreMinus3

TagStrategy_FutureSight:
    IfHPPercentEqualTo AI_BATTLER_ATTACKER_PARTNER, 0, TagStrategy_FutureSight_End
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_FUTURE_SIGHT, TagStrategy_FutureSight_CheckSelfSpeed
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_DOOM_DESIRE, TagStrategy_FutureSight_CheckSelfSpeed
    GoTo TagStrategy_FutureSight_End

TagStrategy_FutureSight_CheckSelfSpeed:
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedEqualTo 3, ScoreMinus3
    IfLoadedEqualTo 2, TagStrategy_FutureSight_SelfMovesThird
    IfLoadedEqualTo 1, TagStrategy_FutureSight_SelfMovesSecond
    IfLoadedEqualTo 0, TagStrategy_FutureSight_SelfMovesFirst
    GoTo TagStrategy_FutureSight_End

TagStrategy_FutureSight_SelfMovesThird:
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedEqualTo 0, ScoreMinus3
    IfLoadedEqualTo 1, ScoreMinus3
    IfRandomLessThan 128, TagStrategy_FutureSight_End
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedEqualTo 2, ScoreMinus3
    GoTo TagStrategy_FutureSight_End

TagStrategy_FutureSight_SelfMovesSecond:
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedEqualTo 0, ScoreMinus3
    IfRandomLessThan 128, TagStrategy_FutureSight_End
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedEqualTo 1, ScoreMinus3
    GoTo TagStrategy_FutureSight_End

TagStrategy_FutureSight_SelfMovesFirst:
    IfRandomLessThan 128, TagStrategy_FutureSight_End
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedEqualTo 0, ScoreMinus3
    GoTo TagStrategy_FutureSight_End

TagStrategy_FutureSight_End:
    End

TagStrategy_SkillSwap:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_TRUANT, ScorePlus5
    IfLoadedEqualTo ABILITY_SLOW_START, ScorePlus5
    IfLoadedEqualTo ABILITY_STALL, ScorePlus5
    IfLoadedEqualTo ABILITY_KLUTZ, ScorePlus5
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_SHADOW_TAG, ScorePlus2
    IfLoadedEqualTo ABILITY_PURE_POWER, ScorePlus2
    IfLoadedEqualTo ABILITY_HUGE_POWER, ScorePlus2
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, ScorePlus2
    IfLoadedEqualTo ABILITY_SOLID_ROCK, ScorePlus2
    IfLoadedEqualTo ABILITY_FILTER, ScorePlus2
    IfLoadedEqualTo ABILITY_FLOWER_GIFT, ScorePlus2
    End

TagStrategy_CheckElectricMove:
    IfMoveEqualTo MOVE_DISCHARGE, TagStrategy_SpreadElectricMove
    CheckBattlerAbility AI_BATTLER_DEFENDER_PARTNER, ABILITY_LIGHTNINGROD
    IfLoadedEqualTo TRUE, TagStrategy_TargetProtectedByLightningRod
    GoTo TagStrategy_PartnerHasLightningRod

TagStrategy_TargetProtectedByLightningRod:
    AddToMoveScore -1
    FlagBattlerIsType AI_BATTLER_DEFENDER_PARTNER, TYPE_GROUND
    IfLoadedEqualTo FALSE, TagStrategy_PartnerHasLightningRod
    AddToMoveScore -8

TagStrategy_PartnerHasLightningRod:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_LIGHTNINGROD
    IfLoadedEqualTo TRUE, ScoreMinus10
    IfMoveEqualTo MOVE_DISCHARGE, TagStrategy_SpreadElectricMove
    GoTo TagStrategy_CheckElectric_End

TagStrategy_SpreadElectricMove:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_MOTOR_DRIVE
    IfLoadedEqualTo TRUE, ScorePlus3
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_VOLT_ABSORB
    IfLoadedEqualTo TRUE, ScorePlus3
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_WATER
    IfLoadedEqualTo TRUE, ScoreMinus10
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_FLYING
    IfLoadedEqualTo TRUE, ScoreMinus10
    // Bug: this comes after the other type checks, so a partner that is also Water or Flying type loses points, as in
    // Gen 4
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_GROUND
    IfLoadedEqualTo TRUE, ScorePlus3
    AddToMoveScore -3

TagStrategy_CheckElectric_End:
    End

TagStrategy_CheckWaterMove:
    IfMoveEqualTo MOVE_SURF, TagStrategy_SpreadWaterMove
    CheckBattlerAbility AI_BATTLER_DEFENDER_PARTNER, ABILITY_STORM_DRAIN
    IfLoadedEqualTo FALSE, TagStrategy_CheckPartnerStormDrain
    AddToMoveScore -1

TagStrategy_CheckPartnerStormDrain:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_STORM_DRAIN
    IfLoadedEqualTo TRUE, ScoreMinus10
    IfMoveEqualTo MOVE_SURF, TagStrategy_SpreadWaterMove
    GoTo TagStrategy_CheckWater_End

// Bug: a Rock-type partner is not checked for, as in Gen 4
TagStrategy_SpreadWaterMove:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_DRY_SKIN
    IfLoadedEqualTo TRUE, ScorePlus3
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_WATER_ABSORB
    IfLoadedEqualTo TRUE, ScorePlus3
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_GROUND
    IfLoadedEqualTo TRUE, ScoreMinus10
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_FIRE
    IfLoadedEqualTo TRUE, ScoreMinus10
    AddToMoveScore -3

TagStrategy_CheckWater_End:
    End

TagStrategy_CheckFireMove:
    IfActivatedFlashFire AI_BATTLER_ATTACKER, TagStrategy_FlashFireScorePlus1
    GoTo TagStrategy_CheckLavaPlume

TagStrategy_FlashFireScorePlus1:
    AddToMoveScore 1

TagStrategy_CheckLavaPlume:
    IfMoveEqualTo MOVE_LAVA_PLUME, TagStrategy_SpreadFireMove
    GoTo TagStrategy_CheckFire_End

TagStrategy_SpreadFireMove:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_DRY_SKIN
    IfLoadedEqualTo TRUE, ScoreMinus3
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_FLASH_FIRE
    IfLoadedEqualTo TRUE, ScorePlus3
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_GRASS
    IfLoadedEqualTo TRUE, ScoreMinus10
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_STEEL
    IfLoadedEqualTo TRUE, ScoreMinus10
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_ICE
    IfLoadedEqualTo TRUE, ScoreMinus10
    FlagBattlerIsType AI_BATTLER_ATTACKER_PARTNER, TYPE_BUG
    IfLoadedEqualTo TRUE, ScoreMinus10
    AddToMoveScore -3

TagStrategy_CheckFire_End:
    End

TagStrategy_WideGuard:
    IfRandomLessThan 50, TagStrategy_WideGuard_CheckTargetMove
    LoadBattlerPreviousMove AI_BATTLER_ATTACKER
    IfLoadedEqualTo MOVE_WIDE_GUARD, TagStrategy_WideGuard_ScoreMinus4

TagStrategy_WideGuard_CheckTargetMove:
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    IfLoadedNotInTable TagStrategy_SpreadMoves, TagStrategy_WideGuard_TryScorePlus2
    End

TagStrategy_WideGuard_ScoreMinus4:
    AddToMoveScore -4
    GoTo TagStrategy_WideGuard_End

TagStrategy_WideGuard_TryScorePlus2:
    IfRandomLessThan 80, TagStrategy_WideGuard_End
    AddToMoveScore 2

TagStrategy_WideGuard_End:
    End

TagStrategy_SpreadMoves:
    TableEntry MOVE_BLIZZARD
    TableEntry MOVE_ROCK_SLIDE
    TableEntry MOVE_HEAT_WAVE
    TableEntry MOVE_ERUPTION
    TableEntry MOVE_WATER_SPOUT
    TableEntry MOVE_MUDDY_WATER
    TableEntry MOVE_GLACIATE
    TableEntry MOVE_SNARL
    TableEntry MOVE_SURF
    TableEntry MOVE_EARTHQUAKE
    TableEntry MOVE_DISCHARGE
    TableEntry MOVE_LAVA_PLUME
    TableEntry MOVE_SLUDGE_WAVE
    TableEntry MOVE_BULLDOZE
    TableEntry MOVE_SEARING_SHOT
    TableEntry TABLE_END

TagStrategy_Round:
    IfMoveNotKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_ROUND, TagStrategy_Round_ScoreMinus1
    IfTurnRandomLessThan 128, TagStrategy_Round_ScoreMinus1
    AddToMoveScore 3
    GoTo TagStrategy_Round_End

TagStrategy_Round_ScoreMinus1:
    AddToMoveScore -1

TagStrategy_Round_End:
    End

TagStrategy_AllySwitch:
    LoadSpecies AI_BATTLER_DEFENDER
    IfLoadedInTable TagStrategy_AllySwitch_Species, TagStrategy_AllySwitch_CheckFirstTurn
    GoTo TagStrategy_AllySwitch_End

TagStrategy_AllySwitch_CheckFirstTurn:
    LoadIsFirstTurnInBattle AI_BATTLER_DEFENDER
    IfLoadedNotEqualTo FALSE, TagStrategy_AllySwitch_End
    IfRandomLessThan 128, TagStrategy_AllySwitch_End
    AddToMoveScore 2
    GoTo TagStrategy_AllySwitch_End
    IfRandomLessThan 128, TagStrategy_AllySwitch_End
    AddToMoveScore -1

TagStrategy_AllySwitch_End:
    End

TagStrategy_AllySwitch_Species:
    TableEntry SPECIES_BLASTOISE
    TableEntry SPECIES_PERSIAN
    TableEntry SPECIES_DEWGONG
    TableEntry SPECIES_KANGASKHAN
    TableEntry SPECIES_MR_MIME
    TableEntry SPECIES_PIKACHU
    TableEntry SPECIES_RAICHU
    TableEntry SPECIES_AMBIPOM
    TableEntry SPECIES_WEAVILE
    TableEntry SPECIES_HITMONCHAN
    TableEntry SPECIES_HITMONLEE
    TableEntry SPECIES_HITMONTOP
    TableEntry SPECIES_JYNX
    TableEntry SPECIES_LUDICOLO
    TableEntry SPECIES_SHIFTRY
    TableEntry SPECIES_HARIYAMA
    TableEntry SPECIES_DELCATTY
    TableEntry SPECIES_SABLEYE
    TableEntry SPECIES_MEDICHAM
    TableEntry SPECIES_SPINDA
    TableEntry SPECIES_KECLEON
    TableEntry SPECIES_INFERNAPE
    TableEntry SPECIES_LOPUNNY
    TableEntry SPECIES_PURUGLY
    TableEntry SPECIES_CROAGUNK
    TableEntry SPECIES_DELIBIRD
    TableEntry SPECIES_SCRAFTY
    TableEntry SPECIES_LIEPARD
    TableEntry TABLE_END

TagStrategy_Bestow:
    GoTo ScoreMinus10

TagStrategy_Quash:
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedEqualTo 0, ScoreMinus10
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedEqualTo 1, ScoreMinus10
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, TagStrategy_Quash_TryScorePlus1
    GoTo ScoreMinus10

TagStrategy_Quash_TryScorePlus1:
    IfTurnRandomLessThan 128, TagStrategy_Quash_End
    AddToMoveScore 1

TagStrategy_Quash_End:
    End

TagStrategy_CircleThrow:
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_AFTER_YOU, TagStrategy_CircleThrow_PartnerKnowsAfterYou
    End

TagStrategy_CircleThrow_PartnerKnowsAfterYou:
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedNotEqualTo 0, ScoreMinus10
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, ScoreMinus10
    IfLoadedEqualTo 1, ScoreMinus10
    IfTurnRandomLessThan 128, TagStrategy_CircleThrow_End
    AddToMoveScore 3

TagStrategy_CircleThrow_End:
    End

TagStrategy_CircleThrow_ScoreMinus40:
    AddToMoveScore -40
    End

TagStrategy_Partner:
    IfBattlerFainted AI_BATTLER_ATTACKER_PARTNER, TagStrategy_PartnerScoreMinus30
    FlagMoveDamageScore USE_MIN_DAMAGE
    IfLoadedEqualTo AI_MOVE_DEALS_NO_DAMAGE, TagStrategy_PartnerStatusMove
    LoadTypeFrom LOAD_MOVE_TYPE
    IfLoadedEqualTo TYPE_FIRE, TagStrategy_CheckPartnerFireAbsorption
    IfLoadedEqualTo TYPE_ELECTRIC, TagStrategy_CheckPartnerElectricAbsorption
    IfLoadedEqualTo TYPE_WATER, TagStrategy_CheckPartnerWaterAbsorption
    IfMoveEqualTo MOVE_FLING, TagStrategy_PartnerTrick

TagStrategy_ScoreMinus30:
    GoTo ScoreMinus30

TagStrategy_CheckPartnerFireAbsorption:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_FLASH_FIRE
    IfLoadedEqualTo TRUE, TagStrategy_CheckPartnerFlashFireActive
    GoTo TagStrategy_ScoreMinus30

TagStrategy_CheckPartnerFlashFireActive:
    IfActivatedFlashFire AI_BATTLER_ATTACKER_PARTNER, TagStrategy_ScoreMinus30
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, TagStrategy_ScoreMinus30
    IfLoadedEqualTo ABILITY_TURBOBLAZE, TagStrategy_ScoreMinus30
    IfLoadedEqualTo ABILITY_TERAVOLT, TagStrategy_ScoreMinus30
    IfRandomLessThan 150, TagStrategy_ScoreMinus30
    GoTo ScorePlus1

TagStrategy_CheckPartnerElectricAbsorption:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_MOTOR_DRIVE
    IfLoadedEqualTo TRUE, TagStrategy_CheckPartnerMotorDrive
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_VOLT_ABSORB
    IfLoadedEqualTo TRUE, TagStrategy_CheckPartnerVoltAbsorb
    GoTo TagStrategy_ScoreMinus30

TagStrategy_CheckPartnerMotorDrive:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, TagStrategy_ScoreMinus30
    IfLoadedEqualTo ABILITY_TURBOBLAZE, TagStrategy_ScoreMinus30
    IfLoadedEqualTo ABILITY_TERAVOLT, TagStrategy_ScoreMinus30
    IfRandomLessThan 160, TagStrategy_CheckElectricAbsorption_End
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SPEED_STAGE, 7, TagStrategy_ScoreMinus30
    GoTo ScorePlus1

TagStrategy_CheckPartnerVoltAbsorb:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, TagStrategy_ScoreMinus30
    IfLoadedEqualTo ABILITY_TURBOBLAZE, TagStrategy_ScoreMinus30
    IfLoadedEqualTo ABILITY_TERAVOLT, TagStrategy_ScoreMinus30
    IfRandomLessThan 150, TagStrategy_ScoreMinus30
    IfHPPercentEqualTo AI_BATTLER_ATTACKER_PARTNER, 100, ScoreMinus10
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 90, TagStrategy_CheckElectricAbsorption_End
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 75, TagStrategy_PartnerVoltAbsorb_75PercentHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 50, TagStrategy_PartnerVoltAbsorb_50PercentHP
    GoTo TagStrategy_PartnerVoltAbsorb_LessThan50PercentHP

TagStrategy_PartnerVoltAbsorb_75PercentHP:
    IfRandomLessThan 64, ScorePlus1
    GoTo TagStrategy_CheckElectricAbsorption_End

TagStrategy_PartnerVoltAbsorb_50PercentHP:
    IfRandomLessThan 128, ScorePlus1
    GoTo TagStrategy_CheckElectricAbsorption_End

TagStrategy_PartnerVoltAbsorb_LessThan50PercentHP:
    IfRandomLessThan 192, ScorePlus1
    GoTo TagStrategy_CheckElectricAbsorption_End

TagStrategy_CheckElectricAbsorption_End:
    End

TagStrategy_CheckPartnerWaterAbsorption:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_WATER_ABSORB
    IfLoadedEqualTo TRUE, TagStrategy_PartnerWaterAbsorb
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_DRY_SKIN
    IfLoadedEqualTo TRUE, TagStrategy_PartnerWaterAbsorb
    GoTo TagStrategy_ScoreMinus30

TagStrategy_PartnerWaterAbsorb:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, TagStrategy_ScoreMinus30
    IfLoadedEqualTo ABILITY_TURBOBLAZE, TagStrategy_ScoreMinus30
    IfLoadedEqualTo ABILITY_TERAVOLT, TagStrategy_ScoreMinus30
    IfRandomLessThan 150, TagStrategy_ScoreMinus30
    IfHPPercentEqualTo AI_BATTLER_ATTACKER_PARTNER, 100, ScoreMinus10
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 90, TagStrategy_CheckWaterAbsorption_End
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 75, TagStrategy_PartnerWaterAbsorb_75PercentHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 50, TagStrategy_PartnerWaterAbsorb_50PercentHP
    GoTo TagStrategy_PartnerWaterAbsorb_LessThan50PercentHP

TagStrategy_PartnerWaterAbsorb_75PercentHP:
    IfRandomLessThan 64, ScorePlus1
    GoTo TagStrategy_CheckWaterAbsorption_End

TagStrategy_PartnerWaterAbsorb_50PercentHP:
    IfRandomLessThan 128, ScorePlus1
    GoTo TagStrategy_CheckWaterAbsorption_End

TagStrategy_PartnerWaterAbsorb_LessThan50PercentHP:
    IfRandomLessThan 192, ScorePlus1
    GoTo TagStrategy_CheckWaterAbsorption_End

TagStrategy_CheckWaterAbsorption_End:
    End

TagStrategy_PartnerStatusMove:
    IfMoveEqualTo MOVE_SKILL_SWAP, TagStrategy_PartnerSkillSwap
    IfMoveEqualTo MOVE_ROLE_PLAY, TagStrategy_PartnerRolePlay
    IfMoveEqualTo MOVE_WILL_O_WISP, TagStrategy_PartnerWillOWisp
    IfMoveEqualTo MOVE_THUNDER_WAVE, TagStrategy_PartnerThunderWave
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_STATUS_BADLY_POISON, TagStrategy_PartnerPoisonStatus
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_STATUS_POISON, TagStrategy_PartnerPoisonStatus
    IfMoveEqualTo MOVE_HELPING_HAND, TagStrategy_PartnerUsingHelpingHand
    IfMoveEqualTo MOVE_SWAGGER, TagStrategy_PartnerSwagger
    IfMoveEqualTo MOVE_TRICK, TagStrategy_PartnerTrick
    IfMoveEqualTo MOVE_SWITCHEROO, TagStrategy_PartnerTrick
    IfMoveEqualTo MOVE_BESTOW, TagStrategy_PartnerTrick
    IfMoveEqualTo MOVE_GASTRO_ACID, TagStrategy_PartnerGastroAcid
    IfMoveEqualTo MOVE_ACUPRESSURE, TagStrategy_PartnerAcupressure
    IfMoveEqualTo MOVE_AFTER_YOU, TagStrategy_PartnerAfterYou
    IfMoveEqualTo MOVE_HEAL_PULSE, TagStrategy_PartnerHealPulse
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerSkillSwap:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_TRUANT, ScorePlus10
    IfLoadedEqualTo ABILITY_SLOW_START, ScorePlus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_LEVITATE, TagStrategy_PartnerSkillSwap_GiveLevitate
    IfLoadedEqualTo ABILITY_COMPOUNDEYES, TagStrategy_PartnerSkillSwap_PartnerHasInaccurateMove
    IfLoadedEqualTo ABILITY_NO_GUARD, TagStrategy_PartnerSkillSwap_PartnerHasInaccurateMove
    IfLoadedEqualTo ABILITY_INSOMNIA, TagStrategy_PartnerSkillSwap_GiveInsomnia
    IfLoadedEqualTo ABILITY_OWN_TEMPO, TagStrategy_PartnerSkillSwap_GiveOwnTempo
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerSkillSwap_GiveLevitate:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_LEVITATE, ScoreMinus10
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_FLYING, ScoreMinus8
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_FLYING, ScoreMinus8
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_GRASS, ScoreMinus5
    IfLoadedEqualTo TYPE_BUG, ScoreMinus5
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_GRASS, ScoreMinus5
    IfLoadedEqualTo TYPE_BUG, ScoreMinus5
    IfCondition AI_BATTLER_DEFENDER, 30, ScoreMinus2
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_ELECTRIC, ScorePlus3
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_ELECTRIC, ScorePlus3
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerSkillSwap_PartnerHasInaccurateMove:
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_FIRE_BLAST, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_THUNDER, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_CROSS_CHOP, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_HYDRO_PUMP, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_DYNAMIC_PUNCH, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_BLIZZARD, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_ZAP_CANNON, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_MEGAHORN, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_FOCUS_BLAST, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_GUNK_SHOT, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_MAGMA_STORM, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_POWER_WHIP, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_SEED_FLARE, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_HEAD_SMASH, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_SHEER_COLD, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_FISSURE, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_GUILLOTINE, TagStrategy_PartnerSkillSwap_ScorePlus3
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_HORN_DRILL, TagStrategy_PartnerSkillSwap_ScorePlus3
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerSkillSwap_ScorePlus3:
    GoTo ScorePlus3

TagStrategy_PartnerSkillSwap_GiveInsomnia:
    IfNotCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, ScoreMinus8
    GoTo ScorePlus3

TagStrategy_PartnerSkillSwap_GiveOwnTempo:
    IfNotCondition AI_BATTLER_DEFENDER, CONDITION_CONFUSION, ScoreMinus8
    GoTo ScorePlus3

TagStrategy_PartnerRolePlay:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedInTable TagStrategy_PartnerRolePlay_Abilities, TagStrategy_PartnerRolePlay_TryScorePlus1
    IfRandomLessThan 128, TagStrategy_PartnerScoreMinus30
    IfLoadedInTable TagStrategy_PartnerRolePlay_Abilities2, TagStrategy_PartnerRolePlay_TryScorePlus1
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerRolePlay_TryScorePlus1:
    IfRandomLessThan 50, TagStrategy_PartnerRolePlay_End
    AddToMoveScore 1

TagStrategy_PartnerRolePlay_End:
    End

TagStrategy_PartnerRolePlay_Abilities:
    TableEntry ABILITY_TELEPATHY
    TableEntry ABILITY_FRIEND_GUARD
    TableEntry ABILITY_SPEED_BOOST
    TableEntry ABILITY_INTIMIDATE
    TableEntry ABILITY_PURE_POWER
    TableEntry ABILITY_CURSED_BODY
    TableEntry TABLE_END

TagStrategy_PartnerRolePlay_Abilities2:
    TableEntry ABILITY_SOLAR_POWER
    TableEntry ABILITY_DRY_SKIN
    TableEntry ABILITY_MOTOR_DRIVE
    TableEntry ABILITY_RAIN_DISH
    TableEntry ABILITY_HUGE_POWER
    TableEntry ABILITY_CHLOROPHYLL
    TableEntry ABILITY_SWIFT_SWIM
    TableEntry ABILITY_ICE_BODY
    TableEntry ABILITY_HARVEST
    TableEntry ABILITY_SAND_RUSH
    TableEntry ABILITY_MAGIC_BOUNCE
    TableEntry ABILITY_PRANKSTER
    TableEntry TABLE_END

TagStrategy_PartnerWillOWisp:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_FLASH_FIRE
    IfLoadedEqualTo TRUE, TagStrategy_CheckPartnerFireAbsorption
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_GUTS
    IfLoadedNotEqualTo TRUE, TagStrategy_PartnerScoreMinus30
    IfStatus AI_BATTLER_ATTACKER_PARTNER, TagStrategy_PartnerScoreMinus30
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_FIRE, TagStrategy_PartnerScoreMinus30
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_FIRE, TagStrategy_PartnerScoreMinus30
    IfHeldItemEqualTo AI_BATTLER_ATTACKER_PARTNER, ITEM_FLAME_ORB, TagStrategy_PartnerScoreMinus30
    IfHeldItemEqualTo AI_BATTLER_ATTACKER_PARTNER, ITEM_TOXIC_ORB, TagStrategy_PartnerScoreMinus30
    IfHPPercentLessThan AI_BATTLER_ATTACKER_PARTNER, 81, TagStrategy_PartnerScoreMinus30
    GoTo ScorePlus5

TagStrategy_PartnerThunderWave:
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_GROUND, TagStrategy_PartnerScoreMinus30
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_GROUND, TagStrategy_PartnerScoreMinus30
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_MOTOR_DRIVE
    IfLoadedEqualTo TRUE, TagStrategy_CheckPartnerElectricAbsorption
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_VOLT_ABSORB
    IfLoadedEqualTo TRUE, TagStrategy_CheckPartnerElectricAbsorption
    GoTo TagStrategy_PartnerScoreMinus30

// Bug: does not check whether the partner is Poison or Steel type, as in Gen 4
TagStrategy_PartnerPoisonStatus:
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_POISON_HEAL
    IfLoadedNotEqualTo TRUE, TagStrategy_PartnerScoreMinus30
    IfStatus AI_BATTLER_DEFENDER, TagStrategy_PartnerScoreMinus30
    IfHeldItemEqualTo AI_BATTLER_ATTACKER_PARTNER, ITEM_TOXIC_ORB, TagStrategy_PartnerScoreMinus30
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 91, TagStrategy_PartnerScoreMinus30
    GoTo ScorePlus5

TagStrategy_PartnerUsingHelpingHand:
    IfHPPercentEqualTo AI_BATTLER_ATTACKER_PARTNER, 0, ScoreMinus30
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 50, TagStrategy_PartnerUsingHelpingHand_TryScorePlus2
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedLessThan 1, TagStrategy_PartnerUsingHelpingHand_TryScorePlus2
    AddToMoveScore -1
    GoTo TagStrategy_PartnerUsingHelpingHand_End

TagStrategy_PartnerUsingHelpingHand_TryScorePlus2:
    IfTurnRandomLessThan 128, TagStrategy_PartnerUsingHelpingHand_End
    AddToMoveScore 3

TagStrategy_PartnerUsingHelpingHand_End:
    End

TagStrategy_PartnerSwagger:
    IfAttackLessThanSpAttack AI_BATTLER_DEFENDER, TagStrategy_PartnerSwagger_ScoreMinus30
    IfHeldItemEqualTo AI_BATTLER_DEFENDER, ITEM_PERSIM_BERRY, TagStrategy_PartnerSwagger_TryScorePlus3
    IfHeldItemEqualTo AI_BATTLER_DEFENDER, ITEM_LUM_BERRY, TagStrategy_PartnerSwagger_TryScorePlus3
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_OWN_TEMPO, TagStrategy_PartnerSwagger_TryScorePlus3
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, TagStrategy_PartnerSwagger_TryScorePlus3

TagStrategy_PartnerSwagger_ScoreMinus30:
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerSwagger_TryScorePlus3:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 7, TagStrategy_PartnerSwagger_End
    AddToMoveScore 3

TagStrategy_PartnerSwagger_End:
    End

TagStrategy_PartnerTrick:
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_LUM_BERRY, TagStrategy_PartnerTrick_LumBerry
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_CHESTO_BERRY, TagStrategy_PartnerTrick_ChestoBerry
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_CHERI_BERRY, TagStrategy_PartnerTrick_CheriBerry
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_RAWST_BERRY, TagStrategy_PartnerTrick_RawstBerry
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_ASPEAR_BERRY, TagStrategy_PartnerTrick_AspearBerry
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_PECHA_BERRY, TagStrategy_PartnerTrick_PechaBerry
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_PERSIM_BERRY, TagStrategy_PartnerTrick_PersimBerry
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_WHITE_HERB, TagStrategy_PartnerTrick_WhiteHerb
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_SITRUS_BERRY, TagStrategy_PartnerTrick_SitrusBerry
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_MENTAL_HERB, TagStrategy_PartnerTrick_MentalHerb
    GoTo TagStrategy_PartnerScoreMinus30

// Bug: checks freeze twice, and the first leads to the burn check
TagStrategy_PartnerTrick_LumBerry:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, TagStrategy_PartnerTrick_PartnerAsleep
    IfCondition AI_BATTLER_DEFENDER, CONDITION_PARALYSIS, TagStrategy_PartnerTrick_PartnerParalyzed
    IfCondition AI_BATTLER_DEFENDER, CONDITION_FREEZE, TagStrategy_PartnerTrick_PartnerBurned
    IfCondition AI_BATTLER_DEFENDER, CONDITION_FREEZE, TagStrategy_PartnerTrick_PartnerFrozen
    IfBadlyPoisoned AI_BATTLER_DEFENDER, TagStrategy_PartnerTrick_PartnerPoisoned
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CONFUSION, TagStrategy_PartnerTrick_PartnerConfused
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerTrick_ChestoBerry:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, TagStrategy_PartnerTrick_PartnerAsleep
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerTrick_CheriBerry:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_PARALYSIS, TagStrategy_PartnerTrick_PartnerParalyzed
    GoTo TagStrategy_PartnerScoreMinus30

// Bug: checks freeze where the Rawst Berry cures burns
TagStrategy_PartnerTrick_RawstBerry:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_FREEZE, TagStrategy_PartnerTrick_PartnerBurned
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerTrick_AspearBerry:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_FREEZE, TagStrategy_PartnerTrick_PartnerFrozen
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerTrick_PechaBerry:
    IfBadlyPoisoned AI_BATTLER_DEFENDER, TagStrategy_PartnerTrick_PartnerPoisoned
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerTrick_PersimBerry:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CONFUSION, TagStrategy_PartnerTrick_PartnerConfused
    GoTo TagStrategy_PartnerScoreMinus30

// Checks Special Defense and evasion twice, and not accuracy
TagStrategy_PartnerTrick_WhiteHerb:
    IfStatStageLessThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_ATTACK_STAGE, 5, TagStrategy_PartnerTrick_PartnerStatsLowered
    IfStatStageLessThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_DEFENSE_STAGE, 5, TagStrategy_PartnerTrick_PartnerStatsLowered
    IfStatStageLessThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_ATTACK_STAGE, 5, TagStrategy_PartnerTrick_PartnerStatsLowered
    IfStatStageLessThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_DEFENSE_STAGE, 5, TagStrategy_PartnerTrick_PartnerStatsLowered
    IfStatStageLessThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_EVASION_STAGE, 5, TagStrategy_PartnerTrick_PartnerStatsLowered
    IfStatStageLessThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SPEED_STAGE, 5, TagStrategy_PartnerTrick_PartnerStatsLowered
    IfStatStageLessThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_DEFENSE_STAGE, 5, TagStrategy_PartnerTrick_PartnerStatsLowered
    IfStatStageLessThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_EVASION_STAGE, 5, TagStrategy_PartnerTrick_PartnerStatsLowered
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerTrick_MentalHerb:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_ATTRACT, TagStrategy_PartnerTrick_PartnerInfatuatedOrTaunted
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CONFUSION, TagStrategy_PartnerTrick_PartnerConfused
    IfCondition AI_BATTLER_DEFENDER, CONDITION_TORMENT, TagStrategy_PartnerTrick_PartnerInfatuatedOrTaunted
    IfCondition AI_BATTLER_DEFENDER, CONDITION_TAUNT, TagStrategy_PartnerTrick_PartnerInfatuatedOrTaunted
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerTrick_SitrusBerry:
    IfHPPercentLessThan AI_BATTLER_ATTACKER_PARTNER, 50, TagStrategy_PartnerScoreMinus30
    GoTo TagStrategy_PartnerTrick_TryScorePlus3

TagStrategy_PartnerTrick_PartnerAsleep:
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_SNORE, TagStrategy_PartnerScoreMinus30
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_SLEEP_TALK, TagStrategy_PartnerScoreMinus30
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 40, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_EVASION_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo 0, TagStrategy_PartnerScoreMinus30
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedNotEqualTo 1, TagStrategy_PartnerScoreMinus30
    GoTo TagStrategy_PartnerTrick_TryScorePlus3

TagStrategy_PartnerTrick_PartnerParalyzed:
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_REST, TagStrategy_PartnerScoreMinus30
    IfHPPercentLessThan AI_BATTLER_ATTACKER_PARTNER, 80, TagStrategy_PartnerScoreMinus30
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedNotEqualTo 3, TagStrategy_PartnerScoreMinus30
    GoTo TagStrategy_PartnerTrick_TryScorePlus3

TagStrategy_PartnerTrick_PartnerBurned:
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_REST, TagStrategy_PartnerScoreMinus30
    IfHPPercentLessThan AI_BATTLER_ATTACKER_PARTNER, 40, TagStrategy_PartnerScoreMinus30
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_EVASION_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfAttackLessThanSpAttack AI_BATTLER_ATTACKER_PARTNER, TagStrategy_PartnerScoreMinus30
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 80, TagStrategy_PartnerTrick_TryScorePlus3
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo 0, TagStrategy_PartnerScoreMinus30
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedNotEqualTo 1, TagStrategy_PartnerScoreMinus30
    GoTo TagStrategy_PartnerTrick_TryScorePlus3

TagStrategy_PartnerTrick_PartnerFrozen:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 40, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_EVASION_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo 0, TagStrategy_PartnerScoreMinus30
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedNotEqualTo 1, TagStrategy_PartnerScoreMinus30
    GoTo TagStrategy_PartnerTrick_TryScorePlus3

TagStrategy_PartnerTrick_PartnerConfused:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 40, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_EVASION_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo 0, TagStrategy_PartnerScoreMinus30
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedNotEqualTo 1, TagStrategy_PartnerScoreMinus30
    GoTo TagStrategy_PartnerTrick_TryScorePlus3

TagStrategy_PartnerTrick_PartnerPoisoned:
    IfMoveKnown AI_BATTLER_ATTACKER_PARTNER, MOVE_REST, TagStrategy_PartnerScoreMinus30
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 80, TagStrategy_PartnerTrick_TryScorePlus3
    IfHPPercentLessThan AI_BATTLER_ATTACKER_PARTNER, 40, TagStrategy_PartnerScoreMinus30
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_EVASION_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    GoTo TagStrategy_PartnerScoreMinus30

TagStrategy_PartnerTrick_PartnerStatsLowered:
    IfHPPercentLessThan AI_BATTLER_ATTACKER_PARTNER, 40, TagStrategy_PartnerScoreMinus30
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_EVASION_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 80, TagStrategy_PartnerTrick_TryScorePlus3
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo 0, TagStrategy_PartnerScoreMinus30
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedNotEqualTo 1, TagStrategy_PartnerScoreMinus30
    GoTo TagStrategy_PartnerTrick_TryScorePlus3

TagStrategy_PartnerTrick_PartnerInfatuatedOrTaunted:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 40, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_DEFENSE_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_EVASION_STAGE, 7, TagStrategy_PartnerTrick_TryScorePlus3
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo 0, TagStrategy_PartnerScoreMinus30
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedNotEqualTo 1, TagStrategy_PartnerScoreMinus30
    GoTo TagStrategy_PartnerTrick_TryScorePlus3

TagStrategy_PartnerTrick_TryScorePlus3:
    IfTurnRandomLessThan 128, TagStrategy_PartnerTrick_End
    AddToMoveScore 3

TagStrategy_PartnerTrick_End:
    End

TagStrategy_PartnerGastroAcid:
    IfCondition AI_BATTLER_ATTACKER_PARTNER, CONDITION_GASTRO_ACID, TagStrategy_PartnerScoreMinus30
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_TRUANT
    IfLoadedEqualTo TRUE, TagStrategy_PartnerGastroAcid_ScorePlus5
    CheckBattlerAbility AI_BATTLER_ATTACKER_PARTNER, ABILITY_SLOW_START
    IfLoadedEqualTo TRUE, TagStrategy_PartnerGastroAcid_ScorePlus5
    GoTo TagStrategy_PartnerGastroAcid_End

TagStrategy_PartnerGastroAcid_ScorePlus5:
    AddToMoveScore 5

TagStrategy_PartnerGastroAcid_End:
    End

TagStrategy_PartnerAcupressure:
    IfStatStageEqualTo AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_ATTACK_STAGE, 12, TagStrategy_PartnerScoreMinus30
    IfStatStageEqualTo AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_DEFENSE_STAGE, 12, TagStrategy_PartnerScoreMinus30
    IfStatStageEqualTo AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SPEED_STAGE, 12, TagStrategy_PartnerScoreMinus30
    IfStatStageEqualTo AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_ATTACK_STAGE, 12, TagStrategy_PartnerScoreMinus30
    IfStatStageEqualTo AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_DEFENSE_STAGE, 12, TagStrategy_PartnerScoreMinus30
    IfStatStageEqualTo AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_EVASION_STAGE, 12, TagStrategy_PartnerScoreMinus30
    IfStatStageEqualTo AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_ACCURACY_STAGE, 12, TagStrategy_PartnerScoreMinus30
    IfHPPercentLessThan AI_BATTLER_ATTACKER_PARTNER, 51, TagStrategy_PartnerAcupressure_ScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER_PARTNER, 90, TagStrategy_PartnerAcupressure_TryScorePlus2
    IfRandomLessThan 128, TagStrategy_PartnerAcupressure_CheckHP

TagStrategy_PartnerAcupressure_TryScorePlus2:
    IfRandomLessThan 80, TagStrategy_PartnerAcupressure_CheckHP
    AddToMoveScore 2
    GoTo TagStrategy_PartnerAcupressure_CheckHP

TagStrategy_PartnerAcupressure_ScoreMinus1:
    AddToMoveScore -1

TagStrategy_PartnerAcupressure_CheckHP:
    End

TagStrategy_PartnerAfterYou:
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo 0, ScoreMinus10
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedEqualTo 0, ScoreMinus10
    IfLoadedEqualTo 1, ScoreMinus10
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_CIRCLE_THROW, TagStrategy_PartnerAfterYou_TryScorePlus3
    IfFieldCondition FIELD_CONDITION_TRICK_ROOM, TagStrategy_PartnerAfterYou_CheckTargetMoves
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_TRICK_ROOM, TagStrategy_PartnerAfterYou_TryScorePlus3

TagStrategy_PartnerAfterYou_CheckTargetMoves:
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_DECREASE_POWER_WITH_LESS_USER_HP, TagStrategy_PartnerAfterYou_TryScorePlus3
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_DARK_VOID, TagStrategy_PartnerAfterYou_TryScorePlus3
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_BLIZZARD, TagStrategy_PartnerAfterYou_TryScorePlus3
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_ROCK_SLIDE, TagStrategy_PartnerAfterYou_TryScorePlus3
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_ROCK_SLIDE, TagStrategy_PartnerAfterYou_TryScorePlus3
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_FORCE_SWITCH, TagStrategy_PartnerAfterYou_End
    IfRandomLessThan 50, TagStrategy_PartnerAfterYou_End
    AddToMoveScore -2
    GoTo TagStrategy_PartnerAfterYou_End

TagStrategy_PartnerAfterYou_TryScorePlus3:
    IfTurnRandomLessThan 128, TagStrategy_PartnerAfterYou_End
    AddToMoveScore 3

TagStrategy_PartnerAfterYou_End:
    End

TagStrategy_PartnerHealPulse:
    IfHPPercentEqualTo AI_BATTLER_DEFENDER, 0, TagStrategy_PartnerScoreMinus30
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 100, TagStrategy_PartnerHealPulse_ScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, TagStrategy_PartnerHealPulse_HighHP
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 30, TagStrategy_PartnerHealPulse_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_DEFENSE_STAGE, 7, TagStrategy_PartnerHealPulse_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_SP_DEFENSE_STAGE, 7, TagStrategy_PartnerHealPulse_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER_PARTNER, BATTLEMON_EVASION_STAGE, 7, TagStrategy_PartnerHealPulse_TryScorePlus3
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, TagStrategy_PartnerHealPulse_TryScorePlus3
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER_PARTNER
    IfLoadedNotEqualTo 0, TagStrategy_PartnerHealPulse_ScoreMinus1
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedEqualTo 1, TagStrategy_PartnerHealPulse_TryScorePlus3
    GoTo TagStrategy_PartnerHealPulse_ScoreMinus1

TagStrategy_PartnerHealPulse_HighHP:
    LoadBattlerSpeedRank AI_BATTLER_ATTACKER
    IfLoadedEqualTo 3, TagStrategy_PartnerHealPulse_TryScorePlus3
    GoTo TagStrategy_PartnerHealPulse_ScoreMinus1

TagStrategy_PartnerHealPulse_TryScorePlus3:
    IfTurnRandomLessThan 128, TagStrategy_PartnerHealPulse_ScoreMinus1
    AddToMoveScore 3
    GoTo TagStrategy_PartnerHealPulse_ScoreMinus1

TagStrategy_PartnerHealPulse_ScoreMinus1:
    AddToMoveScore -1
    End

TagStrategy_PartnerScoreMinus30:
    AddToMoveScore -30
    End

ScoreMinus1:
    AddToMoveScore -1
    End

ScoreMinus2:
    AddToMoveScore -2
    End

ScoreMinus3:
    AddToMoveScore -3
    End

ScoreMinus5:
    AddToMoveScore -5
    End

ScoreMinus6:
    AddToMoveScore -6
    End

ScoreMinus8:
    AddToMoveScore -8
    End

ScoreMinus10:
    AddToMoveScore -10
    End

ScoreMinus12:
    AddToMoveScore -12
    End

ScoreMinus30:
    AddToMoveScore -30
    End

ScorePlus1:
    AddToMoveScore 1
    End

ScorePlus2:
    AddToMoveScore 2
    End

ScorePlus3:
    AddToMoveScore 3
    End

ScorePlus5:
    AddToMoveScore 5
    End

ScorePlus10:
    AddToMoveScore 10
    End
