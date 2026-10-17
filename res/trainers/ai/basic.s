#include "asm/tr_ai.inc"

// AI flag 0, Basic: discourages moves that would fail or be wasted. Moves that the target is immune to lose 10 or 12
// points, and each move effect has its own checks, from Basic_MoveEffectTable, such as a status move on a target that
// already has a status, or a stat change past its limit.

Basic_Main:
    IfTargetIsPartner Terminate
    IfMoveEqualTo MOVE_FISSURE, Basic_CheckForImmunity
    IfMoveEqualTo MOVE_HORN_DRILL, Basic_CheckForImmunity
    FlagMoveDamageScore USE_MIN_DAMAGE
    IfLoadedEqualTo AI_MOVE_DEALS_NO_DAMAGE, Basic_CheckSoundproof

Basic_CheckForImmunity:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckSoundproof
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_VOLT_ABSORB, Basic_CheckElectricAbsorption
    IfLoadedEqualTo ABILITY_MOTOR_DRIVE, Basic_CheckElectricAbsorption
    IfLoadedEqualTo ABILITY_LIGHTNINGROD, Basic_CheckElectricAbsorption
    IfLoadedEqualTo ABILITY_WATER_ABSORB, Basic_CheckWaterAbsorption
    IfLoadedEqualTo ABILITY_FLASH_FIRE, Basic_CheckFireAbsorption
    IfLoadedEqualTo ABILITY_WONDER_GUARD, Basic_CheckWonderGuard
    IfLoadedEqualTo ABILITY_LEVITATE, Basic_CheckGroundAbsorption
    // Bug: Levitate again, where Dry Skin was likely meant, as in Gen 4
    IfLoadedEqualTo ABILITY_LEVITATE, Basic_CheckWaterAbsorption2
    IfLoadedEqualTo ABILITY_SAP_SIPPER, Basic_CheckGrassAbsorption
    GoTo Basic_CheckSoundproof

Basic_CheckElectricAbsorption:
    LoadTypeFrom LOAD_MOVE_TYPE
    IfTempEqualTo TYPE_ELECTRIC, ScoreMinus12
    GoTo Basic_CheckSoundproof

Basic_CheckWaterAbsorption:
    LoadTypeFrom LOAD_MOVE_TYPE
    IfTempEqualTo TYPE_WATER, ScoreMinus12
    GoTo Basic_CheckSoundproof

Basic_CheckFireAbsorption:
    LoadTypeFrom LOAD_MOVE_TYPE
    IfTempEqualTo TYPE_FIRE, ScoreMinus12
    GoTo Basic_CheckSoundproof

Basic_CheckWonderGuard:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_DOUBLE, Basic_CheckSoundproof
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUADRUPLE, Basic_CheckSoundproof
    GoTo ScoreMinus12

Basic_CheckGroundAbsorption:
    LoadTypeFrom LOAD_MOVE_TYPE
    IfTempEqualTo TYPE_GROUND, ScoreMinus12
    GoTo Basic_CheckSoundproof

Basic_CheckWaterAbsorption2:
    LoadTypeFrom LOAD_MOVE_TYPE
    IfTempEqualTo TYPE_WATER, ScoreMinus12
    GoTo Basic_CheckSoundproof

Basic_CheckGrassAbsorption:
    LoadTypeFrom LOAD_MOVE_TYPE
    IfTempEqualTo TYPE_GRASS, ScoreMinus12
    GoTo Basic_CheckSoundproof

Basic_CheckSoundproof:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedNotEqualTo ABILITY_SOUNDPROOF, Basic_ScoreMoveEffect
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_ScoreMoveEffect
    IfMoveEqualTo MOVE_GROWL, ScoreMinus10
    IfMoveEqualTo MOVE_ROAR, ScoreMinus10
    IfMoveEqualTo MOVE_SING, ScoreMinus10
    IfMoveEqualTo MOVE_SUPERSONIC, ScoreMinus10
    IfMoveEqualTo MOVE_SCREECH, ScoreMinus10
    IfMoveEqualTo MOVE_SNORE, ScoreMinus10
    IfMoveEqualTo MOVE_UPROAR, ScoreMinus10
    IfMoveEqualTo MOVE_METAL_SOUND, ScoreMinus10
    IfMoveEqualTo MOVE_GRASS_WHISTLE, ScoreMinus10
    IfMoveEqualTo MOVE_BUG_BUZZ, ScoreMinus10
    IfMoveEqualTo MOVE_CHATTER, ScoreMinus10
    IfMoveEqualTo MOVE_ROUND, ScoreMinus10
    IfMoveEqualTo MOVE_ECHOED_VOICE, ScoreMinus10
    IfMoveEqualTo MOVE_RELIC_SONG, ScoreMinus10
    IfMoveEqualTo MOVE_SNARL, ScoreMinus10

Basic_ScoreMoveEffect:
    GoToByMoveEffect 0, 337, Basic_MoveEffectTable
    End

Basic_MoveEffectTable:
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotSleep, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotExplode, Basic_MoveEffectTable
    LabelDistance Basic_CheckDreamEater, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Attack, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Defense, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Speed, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_SpAttack, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_SpDefense, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Accuracy, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Evasion, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_Attack, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_Defense, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_Speed, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_SpAttack, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_SpDefense, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_Accuracy, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_Evasion, Basic_MoveEffectTable
    LabelDistance Basic_CheckStatStageImbalance, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanForceSwitch, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanRecoverHP, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotPoison, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckAlreadyUnderLightScreen, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckOHKOWouldFail, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckAlreadyUnderMist, Basic_MoveEffectTable
    LabelDistance Basic_CheckAlreadyPumpedUp, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotConfuse, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Attack, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Defense, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Speed, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_SpAttack, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_SpDefense, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Accuracy, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Evasion, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_Attack, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_Defense, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_Speed, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_SpAttack, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_SpDefense, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_Accuracy, Basic_MoveEffectTable
    LabelDistance Basic_CheckLowStatStage_Evasion, Basic_MoveEffectTable
    LabelDistance Basic_CheckAlreadyUnderReflect, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotPoison, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotParalyze, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotSubstitute, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotLeechSeed, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotDisable, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotEncore, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckAttackerAsleep, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckLockOn, Basic_MoveEffectTable
    LabelDistance Basic_CheckAttackerAsleep, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckMeanLook, Basic_MoveEffectTable
    LabelDistance Basic_CheckNightmare, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Evasion, Basic_MoveEffectTable
    LabelDistance Basic_CheckCurse, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckSpikes, Basic_MoveEffectTable
    LabelDistance Basic_CheckForesight, Basic_MoveEffectTable
    LabelDistance Basic_CheckPerishSong, Basic_MoveEffectTable
    LabelDistance Basic_CheckSandstorm, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotConfuse, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotAttract, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckAlreadyUnderSafeguard, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckMagnitude, Basic_MoveEffectTable
    LabelDistance Basic_CheckBatonPass, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanRecoverHP, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanRecoverHP, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanRecoverHP, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckRainDance, Basic_MoveEffectTable
    LabelDistance Basic_CheckSunnyDay, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckBellyDrum, Basic_MoveEffectTable
    LabelDistance Basic_CheckStatStageImbalance, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckFutureSight, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance ScoreMinus10, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckHighStatStage_Defense, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanRecoverHP, Basic_MoveEffectTable
    LabelDistance Basic_CheckFirstTurnInBattle, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckMaxStockpile, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanSpitUpOrSwallow, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanSpitUpOrSwallow, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckHail, Basic_MoveEffectTable
    LabelDistance Basic_CheckTorment, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotConfuse, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotBurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckMemento, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckMakeGlobalTarget, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckTaunt, Basic_MoveEffectTable
    LabelDistance Basic_CheckHelpingHand, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanRemoveItem, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckAlreadyIngrained, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanRecycle, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCannotSleep, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanRemoveItem, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanImprison, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanRefreshStatus, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanMudSport, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckTickle, Basic_MoveEffectTable
    LabelDistance Basic_CheckCosmicPower, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckBulkUp, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckWaterSport, Basic_MoveEffectTable
    LabelDistance Basic_CheckCalmMind, Basic_MoveEffectTable
    LabelDistance Basic_CheckDragonDance, Basic_MoveEffectTable
    LabelDistance Basic_CheckCamouflage, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanRecoverHP, Basic_MoveEffectTable
    LabelDistance Basic_CheckGravityActive, Basic_MoveEffectTable
    LabelDistance Basic_CheckMiracleEye, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckHealingWish, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckNaturalGift, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckTailwind, Basic_MoveEffectTable
    LabelDistance Basic_CheckAcupressure, Basic_MoveEffectTable
    LabelDistance Basic_CheckMetalBurst, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckEmbargo, Basic_MoveEffectTable
    LabelDistance Basic_CheckFling, Basic_MoveEffectTable
    LabelDistance Basic_CheckCanPsychoShift, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckHealBlock, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckPowerTrick, Basic_MoveEffectTable
    LabelDistance Basic_CheckGastroAcid, Basic_MoveEffectTable
    LabelDistance Basic_CheckLuckyChant, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCopycat, Basic_MoveEffectTable
    LabelDistance Basic_CheckPowerSwap, Basic_MoveEffectTable
    LabelDistance Basic_CheckGuardSwap, Basic_MoveEffectTable
    LabelDistance Basic_CheckNonStandardDamageOrChargeTurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckLastResort, Basic_MoveEffectTable
    LabelDistance Basic_CheckWorrySeed, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckToxicSpikes, Basic_MoveEffectTable
    LabelDistance Basic_CheckStatStageImbalance, Basic_MoveEffectTable
    LabelDistance Basic_CheckAquaRing, Basic_MoveEffectTable
    LabelDistance Basic_CheckMagnetRise, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckDefog, Basic_MoveEffectTable
    LabelDistance Basic_CheckTrickRoom, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckCaptivate, Basic_MoveEffectTable
    LabelDistance Basic_CheckStealthRock, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckLunarDance, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Basic_CheckHoneClaws, Basic_MoveEffectTable
    LabelDistance Basic_CheckWideGuard, Basic_MoveEffectTable
    LabelDistance Basic_CheckGuardSplit, Basic_MoveEffectTable
    LabelDistance Basic_CheckPowerSplit, Basic_MoveEffectTable
    LabelDistance Basic_CheckWonderRoom, Basic_MoveEffectTable
    LabelDistance Basic_CheckPsyshock, Basic_MoveEffectTable
    LabelDistance Basic_CheckVenoshock, Basic_MoveEffectTable
    LabelDistance Basic_CheckAutotomize, Basic_MoveEffectTable
    LabelDistance Basic_CheckTelekinesis, Basic_MoveEffectTable
    LabelDistance Basic_CheckMagicRoom, Basic_MoveEffectTable
    LabelDistance Basic_CheckSmackDown, Basic_MoveEffectTable
    LabelDistance Basic_CheckStormThrow, Basic_MoveEffectTable
    LabelDistance Basic_CheckFlameBurst, Basic_MoveEffectTable
    LabelDistance Basic_CheckQuiverDance, Basic_MoveEffectTable
    LabelDistance Basic_CheckHeavySlam, Basic_MoveEffectTable
    LabelDistance Basic_CheckSynchronoise, Basic_MoveEffectTable
    LabelDistance Basic_CheckElectroBall, Basic_MoveEffectTable
    LabelDistance Basic_CheckSoak, Basic_MoveEffectTable
    LabelDistance Basic_CheckFlameCharge, Basic_MoveEffectTable
    LabelDistance Basic_CheckAcidSpray, Basic_MoveEffectTable
    LabelDistance Basic_CheckFoulPlay, Basic_MoveEffectTable
    LabelDistance Basic_CheckSimpleBeam, Basic_MoveEffectTable
    LabelDistance Basic_CheckEntrainment, Basic_MoveEffectTable
    LabelDistance Basic_CheckAfterYou, Basic_MoveEffectTable
    LabelDistance Basic_CheckRound, Basic_MoveEffectTable
    LabelDistance Basic_CheckEchoedVoice, Basic_MoveEffectTable
    LabelDistance Basic_CheckChipAway, Basic_MoveEffectTable
    LabelDistance Basic_CheckClearSmog, Basic_MoveEffectTable
    LabelDistance Basic_CheckStoredPower, Basic_MoveEffectTable
    LabelDistance Basic_CheckQuickGuard, Basic_MoveEffectTable
    LabelDistance Basic_CheckAllySwitch, Basic_MoveEffectTable
    LabelDistance Basic_CheckShellSmash, Basic_MoveEffectTable
    LabelDistance Basic_CheckHealPulse, Basic_MoveEffectTable
    LabelDistance Basic_CheckHex, Basic_MoveEffectTable
    LabelDistance Basic_CheckSkyDrop, Basic_MoveEffectTable
    LabelDistance Basic_CheckShiftGear, Basic_MoveEffectTable
    LabelDistance Basic_CheckCircleThrow, Basic_MoveEffectTable
    LabelDistance Basic_CheckIncinerate, Basic_MoveEffectTable
    LabelDistance Basic_CheckQuash, Basic_MoveEffectTable
    LabelDistance Basic_CheckGrowth, Basic_MoveEffectTable
    LabelDistance Basic_CheckAcrobatics, Basic_MoveEffectTable
    LabelDistance Basic_CheckReflectType, Basic_MoveEffectTable
    LabelDistance Basic_CheckRetaliate, Basic_MoveEffectTable
    LabelDistance Basic_CheckFinalGambit, Basic_MoveEffectTable
    LabelDistance Basic_CheckTailGlow, Basic_MoveEffectTable
    LabelDistance Basic_CheckCoil, Basic_MoveEffectTable
    LabelDistance Basic_CheckBestow, Basic_MoveEffectTable
    LabelDistance Basic_CheckWaterPledge, Basic_MoveEffectTable
    LabelDistance Basic_CheckFirePledge, Basic_MoveEffectTable
    LabelDistance Basic_CheckGrassPledge, Basic_MoveEffectTable
    LabelDistance Basic_CheckWorkUp, Basic_MoveEffectTable
    LabelDistance Basic_CheckCottonGuard, Basic_MoveEffectTable
    LabelDistance Basic_CheckRelicSong, Basic_MoveEffectTable
    LabelDistance Basic_CheckGlaciate, Basic_MoveEffectTable
    LabelDistance Basic_CheckFreezeShock, Basic_MoveEffectTable
    LabelDistance Basic_CheckIceBurn, Basic_MoveEffectTable
    LabelDistance Basic_CheckUnused333, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable
    LabelDistance Terminate, Basic_MoveEffectTable

Terminate:
    End

Basic_CheckCannotSleep:
    IfStatus AI_BATTLER_DEFENDER, ScoreMinus10
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_INSOMNIA, ScoreMinus10
    IfLoadedEqualTo ABILITY_VITAL_SPIRIT, ScoreMinus10
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus10
    End

Basic_CheckCannotExplode:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckLastMon
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_DAMP, ScoreMinus10

Basic_CheckLastMon:
    CountAlivePartyBattlers AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo 0, Basic_Explode_Terminate
    CountAlivePartyBattlers AI_BATTLER_DEFENDER
    IfLoadedNotEqualTo 0, ScoreMinus10
    GoTo ScoreMinus1

Basic_Explode_Terminate:
    End

Basic_CheckNightmare:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_NIGHTMARE, ScoreMinus10
    IfNotCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, ScoreMinus8
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, ScoreMinus10
    End

Basic_CheckDreamEater:
    IfNotCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, ScoreMinus8
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, ScoreMinus10
    End

Basic_CheckBellyDrum:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 51, ScoreMinus10

Basic_CheckHighStatStage_Attack:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    End

Basic_CheckHighStatStage_Defense:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 12, ScoreMinus10
    End

Basic_CheckHighStatStage_Speed:
    IfFieldCondition FIELD_CONDITION_TRICK_ROOM, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SPEED_STAGE, 12, ScoreMinus10
    End

Basic_CheckHighStatStage_SpAttack:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 12, ScoreMinus10
    End

Basic_CheckHighStatStage_SpDefense:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 12, ScoreMinus10
    End

Basic_CheckHighStatStage_Accuracy:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_NO_GUARD, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_NO_GUARD, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 12, ScoreMinus10
    End

Basic_CheckHighStatStage_Evasion:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_NO_GUARD, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_NO_GUARD, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_EVASION_STAGE, 12, ScoreMinus10
    End

Basic_CheckLowStatStage_Attack:
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 0, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_DEFIANT, ScoreMinus12
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckClearBodyEffect
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_HYPER_CUTTER, ScoreMinus10
    GoTo Basic_CheckClearBodyEffect

Basic_CheckLowStatStage_Defense:
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 0, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckClearBodyEffect
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_BIG_PECKS, ScoreMinus10
    GoTo Basic_CheckClearBodyEffect

Basic_CheckLowStatStage_Speed:
    IfFieldCondition FIELD_CONDITION_TRICK_ROOM, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_SPEED_STAGE, 0, ScoreMinus10
    CheckBattlerAbility AI_BATTLER_DEFENDER, ABILITY_SPEED_BOOST
    IfLoadedEqualTo TRUE, ScoreMinus10
    GoTo Basic_CheckClearBodyEffect

Basic_CheckLowStatStage_SpAttack:
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 0, ScoreMinus10
    GoTo Basic_CheckClearBodyEffect

Basic_CheckLowStatStage_SpDefense:
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 0, ScoreMinus10
    GoTo Basic_CheckClearBodyEffect

Basic_CheckLowStatStage_Accuracy:
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_ACCURACY_STAGE, 0, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_NO_GUARD, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_NO_GUARD, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckClearBodyEffect
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_KEEN_EYE, ScoreMinus10
    GoTo Basic_CheckClearBodyEffect

Basic_CheckLowStatStage_Evasion:
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 0, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_NO_GUARD, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_NO_GUARD, ScoreMinus10

Basic_CheckClearBodyEffect:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_CONTRARY, ScoreMinus12
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckClearBodyEffect_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_CLEAR_BODY, ScoreMinus10
    IfLoadedEqualTo ABILITY_WHITE_SMOKE, ScoreMinus10
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckClearBodyEffect_End:
    End

Basic_CheckStatStageImbalance:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SPEED_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_EVASION_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SPEED_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_ACCURACY_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 6, Basic_CheckStatStageImbalance_Terminate
    GoTo ScoreMinus10

Basic_CheckStatStageImbalance_Terminate:
    End

Basic_CheckCanForceSwitch:
    CountAlivePartyBattlers AI_BATTLER_DEFENDER
    IfLoadedEqualTo 0, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckCanForceSwitch_Terminate
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_SUCTION_CUPS, ScoreMinus10
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckCanForceSwitch_Terminate:
    End

Basic_CheckCanRecoverHP:
    IfHPPercentNotEqualTo AI_BATTLER_ATTACKER, 100, Basic_CheckCanRecoverHP_Terminate
    AddToMoveScore -8

Basic_CheckCanRecoverHP_Terminate:
    End

Basic_CheckCannotPoison:
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_STEEL, ScoreMinus10
    IfLoadedEqualTo TYPE_POISON, ScoreMinus10
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_STEEL, ScoreMinus10
    IfLoadedEqualTo TYPE_POISON, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_IMMUNITY, ScoreMinus10
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, ScoreMinus10
    IfLoadedEqualTo ABILITY_POISON_HEAL, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckCannotPoison_Hydration
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12
    IfLoadedNotEqualTo ABILITY_LEAF_GUARD, Basic_CheckCannotPoison_Hydration
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_SUN, ScoreMinus10

Basic_CheckCannotPoison_Hydration:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedNotEqualTo ABILITY_HYDRATION, Basic_CheckCannotPoison_StatusOrSafeguard
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_RAIN, ScoreMinus10

Basic_CheckCannotPoison_StatusOrSafeguard:
    IfStatus AI_BATTLER_DEFENDER, ScoreMinus10
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, ScoreMinus10
    End

Basic_CheckAlreadyUnderLightScreen:
    IfSideCondition AI_BATTLER_ATTACKER, SIDE_CONDITION_LIGHT_SCREEN, ScoreMinus8
    End

Basic_CheckOHKOWouldFail:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckOHKOWouldFail_Levels
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_STURDY, ScoreMinus10

Basic_CheckOHKOWouldFail_Levels:
    IfLevel CHECK_LOWER_THAN_TARGET, ScoreMinus10
    End

Basic_CheckMagnitude:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckNonStandardDamageOrChargeTurn
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_LEVITATE, ScoreMinus10

Basic_CheckNonStandardDamageOrChargeTurn:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedNotEqualTo ABILITY_WONDER_GUARD, Basic_CheckNonStandardDamageOrChargeTurn_Terminate
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckNonStandardDamageOrChargeTurn_Terminate
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_DOUBLE, Basic_CheckNonStandardDamageOrChargeTurn_Terminate
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUADRUPLE, Basic_CheckNonStandardDamageOrChargeTurn_Terminate
    GoTo ScoreMinus10

Basic_CheckNonStandardDamageOrChargeTurn_Terminate:
    End

Basic_CheckAlreadyUnderMist:
    IfSideCondition AI_BATTLER_ATTACKER, SIDE_CONDITION_MIST, ScoreMinus8
    End

Basic_CheckAlreadyPumpedUp:
    IfConditionFlag AI_BATTLER_ATTACKER, 9, ScoreMinus10
    End

Basic_CheckCannotConfuse:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CONFUSION, ScoreMinus5
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_OWN_TEMPO, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckCannotConfuse_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckCannotConfuse_End:
    End

Basic_CheckAlreadyUnderReflect:
    IfSideCondition AI_BATTLER_ATTACKER, SIDE_CONDITION_REFLECT, ScoreMinus8
    End

Basic_CheckCannotParalyze:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_LIMBER, ScoreMinus10
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckCannotParalyze_ImmuneToStatus
    IfMoveEqualTo MOVE_THUNDER_WAVE, Basic_CheckCannotParalyze_ThunderWave
    GoTo Basic_CheckCannotParalyze_ImmuneToStatus

Basic_CheckCannotParalyze_ThunderWave:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MOTOR_DRIVE, ScoreMinus10
    IfLoadedEqualTo ABILITY_VOLT_ABSORB, ScoreMinus10
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckCannotParalyze_ImmuneToStatus:
    IfStatus AI_BATTLER_DEFENDER, ScoreMinus10
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, ScoreMinus10
    End

Basic_CheckCannotSubstitute:
    IfBattlerHasSubstitute AI_BATTLER_ATTACKER, ScoreMinus8
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 26, ScoreMinus10
    End

Basic_CheckCannotLeechSeed:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_LEECH_SEED, ScoreMinus10
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_GRASS, ScoreMinus10
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_GRASS, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, ScoreMinus10
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12
    End

Basic_CheckCannotDisable:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_DISABLE, ScoreMinus8
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckCannotDisable_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckCannotDisable_End:
    End

Basic_CheckCannotEncore:
    IfCondition AI_BATTLER_DEFENDER, 23, ScoreMinus8
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckCannotEncore_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckCannotEncore_End:
    End

// Snore and Sketch come here while Sleep Talk does not, which looks like Sleep Talk's entry went to Sketch
Basic_CheckAttackerAsleep:
    IfNotCondition AI_BATTLER_ATTACKER, CONDITION_SLEEP, ScoreMinus8
    End

Basic_CheckLockOn:
    IfCondition AI_BATTLER_DEFENDER, 29, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_NO_GUARD, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_NO_GUARD, ScoreMinus10
    End

Basic_CheckMeanLook:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_MEAN_LOOK, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckMeanLook_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckMeanLook_End:
    End

Basic_CheckCurse:
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_GHOST, Basic_CheckCurse_GhostType
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_GHOST, Basic_CheckCurse_GhostType
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 12, ScoreMinus8
    End

Basic_CheckCurse_GhostType:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CURSE, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, ScoreMinus10
    End

Basic_CheckSpikes:
    LoadSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SPIKES
    IfLoadedEqualTo 3, ScoreMinus10
    CountAlivePartyBattlers AI_BATTLER_DEFENDER
    IfLoadedEqualTo 0, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckSpikes_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckSpikes_End:
    End

Basic_CheckForesight:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_FORESIGHT, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckForesight_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckForesight_End:
    End

Basic_CheckPerishSong:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_PERISH_SONG, ScoreMinus10
    End

Basic_CheckSandstorm:
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_SANDSTORM, ScoreMinus8
    End

Basic_CheckCannotAttract:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_ATTRACT, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_OBLIVIOUS, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckCannotAttract_Gender
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckCannotAttract_Gender:
    LoadGender AI_BATTLER_ATTACKER
    IfLoadedEqualTo GENDER_MALE, Basic_CheckCannotAttract_BothMale
    IfLoadedEqualTo GENDER_FEMALE, Basic_CheckCannotAttract_BothFemale
    GoTo ScoreMinus10

Basic_CheckCannotAttract_BothMale:
    LoadGender AI_BATTLER_DEFENDER
    IfLoadedEqualTo GENDER_FEMALE, Basic_CheckCannotAttract_Terminate
    GoTo ScoreMinus10

Basic_CheckCannotAttract_BothFemale:
    LoadGender AI_BATTLER_DEFENDER
    IfLoadedEqualTo GENDER_MALE, Basic_CheckCannotAttract_Terminate
    GoTo ScoreMinus10

Basic_CheckCannotAttract_Terminate:
    End

Basic_CheckAlreadyUnderSafeguard:
    IfSideCondition AI_BATTLER_ATTACKER, SIDE_CONDITION_SAFEGUARD, ScoreMinus8
    End

Basic_CheckMemento:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_CONTRARY, ScoreMinus12
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckMemento_CheckStatStages
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_CLEAR_BODY, ScoreMinus10
    IfLoadedEqualTo ABILITY_WHITE_SMOKE, ScoreMinus10

Basic_CheckMemento_CheckStatStages:
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 0, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 0, ScoreMinus8
    CountAlivePartyBattlers AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, ScoreMinus10
    End

Basic_CheckBatonPass:
    CountAlivePartyBattlers AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, ScoreMinus10
    End

Basic_CheckRainDance:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_SWIFT_SWIM, Basic_CheckCurrentWeatherIsRain
    IfLoadedEqualTo ABILITY_HYDRATION, Basic_CheckCurrentWeatherIsRain
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedNotEqualTo ABILITY_HYDRATION, Basic_CheckCurrentWeatherIsRain
    IfStatus AI_BATTLER_DEFENDER, ScoreMinus8

Basic_CheckCurrentWeatherIsRain:
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_RAIN, ScoreMinus8
    End

Basic_CheckSunnyDay:
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_SUN, ScoreMinus8
    End

Basic_CheckFutureSight:
    IfUnk74 AI_BATTLER_DEFENDER, ScoreMinus12
    End

Basic_CheckFirstTurnInBattle:
    LoadIsFirstTurnInBattle AI_BATTLER_ATTACKER
    IfLoadedEqualTo FALSE, ScoreMinus10
    End

Basic_CheckMaxStockpile:
    LoadStockpileCount AI_BATTLER_ATTACKER
    IfLoadedEqualTo 3, ScoreMinus10
    End

Basic_CheckCanSpitUpOrSwallow:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, ScoreMinus10
    LoadStockpileCount AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, ScoreMinus10
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_SWALLOW, Basic_CheckCanRecoverHP
    End

Basic_CheckHail:
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_HAIL, ScoreMinus8

Basic_CheckHail_Terminate:
    End

Basic_CheckTorment:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_TORMENT, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckTorment_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckTorment_End:
    End

Basic_CheckCannotBurn:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_WATER_VEIL, ScoreMinus10
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, ScoreMinus10
    IfStatus AI_BATTLER_DEFENDER, ScoreMinus10
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_FIRE, ScoreMinus10
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_FIRE, ScoreMinus10
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckCannotBurn_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckCannotBurn_End:
    End

Basic_CheckMakeGlobalTarget:
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, ScoreMinus10
    End

Basic_CheckTaunt:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_TAUNT, ScoreMinus10
    End

Basic_CheckHelpingHand:
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, ScoreMinus10
    End

Basic_CheckCanRemoveItem:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_STICKY_HOLD, ScoreMinus10
    LoadHeldItem AI_BATTLER_DEFENDER
    IfLoadedEqualTo ITEM_NONE, ScoreMinus10
    End

Basic_CheckAlreadyIngrained:
    IfCondition AI_BATTLER_ATTACKER, CONDITION_INGRAIN, ScoreMinus10
    End

Basic_CheckCanRecycle:
    LoadRecycleItem AI_BATTLER_ATTACKER
    IfLoadedEqualTo ITEM_NONE, ScoreMinus10
    End

Basic_CheckCanImprison:
    IfFieldCondition 3, ScoreMinus10
    End

Basic_CheckCanRefreshStatus:
    IfCondition AI_BATTLER_ATTACKER, CONDITION_POISON, Basic_CheckCanRefreshStatus_End
    IfBadlyPoisoned AI_BATTLER_ATTACKER, Basic_CheckCanRefreshStatus_End
    IfCondition AI_BATTLER_ATTACKER, CONDITION_PARALYSIS, Basic_CheckCanRefreshStatus_End
    IfCondition AI_BATTLER_ATTACKER, CONDITION_BURN, Basic_CheckCanRefreshStatus_End
    GoTo ScoreMinus10

Basic_CheckCanRefreshStatus_End:
    End

Basic_CheckCanMudSport:
    IfFieldCondition 5, ScoreMinus10
    End

Basic_CheckTickle:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_CONTRARY, ScoreMinus12
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckTickle_CheckStatStages
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_CLEAR_BODY, ScoreMinus10
    IfLoadedEqualTo ABILITY_WHITE_SMOKE, ScoreMinus10
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckTickle_CheckStatStages:
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 0, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 0, ScoreMinus8
    End

Basic_CheckCosmicPower:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 12, ScoreMinus8
    End

Basic_CheckBulkUp:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 12, ScoreMinus8
    End

Basic_CheckWaterSport:
    IfFieldCondition 4, ScoreMinus10
    End

Basic_CheckCalmMind:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 12, ScoreMinus8
    End

Basic_CheckDragonDance:
    IfFieldCondition FIELD_CONDITION_TRICK_ROOM, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SPEED_STAGE, 12, ScoreMinus8
    End

Basic_CheckCamouflage:
    End

Basic_CheckGravityActive:
    IfFieldCondition FIELD_CONDITION_GRAVITY, ScoreMinus10
    End

Basic_CheckMiracleEye:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_FORESIGHT, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckMiracleEye_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckMiracleEye_End:
    End

Basic_CheckHealingWish:
    AddToMoveScore -20
    CountAlivePartyBattlers AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, ScoreMinus10
    IfPartyMemberNotStatus AI_BATTLER_ATTACKER, Basic_CheckHealingWish_Terminate
    IfAnyPartyMemberIsWounded AI_BATTLER_ATTACKER, Basic_CheckHealingWish_Terminate
    GoTo ScoreMinus10

Basic_CheckHealingWish_Terminate:
    End

Basic_CheckNaturalGift:
    LoadHeldItem AI_BATTLER_ATTACKER
    IfLoadedNotInTable Basic_NaturalGiftBerries, ScoreMinus10
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, ScoreMinus10
    End

Basic_NaturalGiftBerries:
    TableEntry ITEM_CHERI_BERRY
    TableEntry ITEM_CHESTO_BERRY
    TableEntry ITEM_PECHA_BERRY
    TableEntry ITEM_RAWST_BERRY
    TableEntry ITEM_ASPEAR_BERRY
    TableEntry ITEM_LEPPA_BERRY
    TableEntry ITEM_ORAN_BERRY
    TableEntry ITEM_PERSIM_BERRY
    TableEntry ITEM_LUM_BERRY
    TableEntry ITEM_SITRUS_BERRY
    TableEntry ITEM_FIGY_BERRY
    TableEntry ITEM_WIKI_BERRY
    TableEntry ITEM_MAGO_BERRY
    TableEntry ITEM_AGUAV_BERRY
    TableEntry ITEM_IAPAPA_BERRY
    TableEntry ITEM_RAZZ_BERRY
    TableEntry ITEM_BLUK_BERRY
    TableEntry ITEM_NANAB_BERRY
    TableEntry ITEM_WEPEAR_BERRY
    TableEntry ITEM_PINAP_BERRY
    TableEntry ITEM_POMEG_BERRY
    TableEntry ITEM_KELPSY_BERRY
    TableEntry ITEM_QUALOT_BERRY
    TableEntry ITEM_HONDEW_BERRY
    TableEntry ITEM_GREPA_BERRY
    TableEntry ITEM_TAMATO_BERRY
    TableEntry ITEM_CORNN_BERRY
    TableEntry ITEM_MAGOST_BERRY
    TableEntry ITEM_RABUTA_BERRY
    TableEntry ITEM_NOMEL_BERRY
    TableEntry ITEM_SPELON_BERRY
    TableEntry ITEM_PAMTRE_BERRY
    TableEntry ITEM_WATMEL_BERRY
    TableEntry ITEM_DURIN_BERRY
    TableEntry ITEM_BELUE_BERRY
    TableEntry ITEM_OCCA_BERRY
    TableEntry ITEM_PASSHO_BERRY
    TableEntry ITEM_WACAN_BERRY
    TableEntry ITEM_RINDO_BERRY
    TableEntry ITEM_YACHE_BERRY
    TableEntry ITEM_CHOPLE_BERRY
    TableEntry ITEM_KEBIA_BERRY
    TableEntry ITEM_SHUCA_BERRY
    TableEntry ITEM_COBA_BERRY
    TableEntry ITEM_PAYAPA_BERRY
    TableEntry ITEM_TANGA_BERRY
    TableEntry ITEM_CHARTI_BERRY
    TableEntry ITEM_KASIB_BERRY
    TableEntry ITEM_HABAN_BERRY
    TableEntry ITEM_COLBUR_BERRY
    TableEntry ITEM_BABIRI_BERRY
    TableEntry ITEM_CHILAN_BERRY
    TableEntry ITEM_LIECHI_BERRY
    TableEntry ITEM_GANLON_BERRY
    TableEntry ITEM_SALAC_BERRY
    TableEntry ITEM_PETAYA_BERRY
    TableEntry ITEM_APICOT_BERRY
    TableEntry ITEM_LANSAT_BERRY
    TableEntry ITEM_STARF_BERRY
    TableEntry ITEM_ENIGMA_BERRY
    TableEntry ITEM_MICLE_BERRY
    TableEntry ITEM_CUSTAP_BERRY
    TableEntry ITEM_JABOCA_BERRY
    TableEntry ITEM_ROWAP_BERRY
    TableEntry TABLE_END

Basic_CheckTailwind:
    IfFieldCondition FIELD_CONDITION_TRICK_ROOM, ScoreMinus8
    IfSideCondition AI_BATTLER_ATTACKER, SIDE_CONDITION_TAILWIND, ScoreMinus10
    End

Basic_CheckAcupressure:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SPEED_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_EVASION_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 12, ScoreMinus10
    End

Basic_CheckMetalBurst:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_STALL, ScoreMinus10
    LoadHeldItemEffect AI_BATTLER_DEFENDER
    IfLoadedEqualTo HOLD_EFFECT_PRIORITY_DOWN, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_STALL, Basic_CheckMetalBurst_Terminate
    LoadHeldItemEffect AI_BATTLER_ATTACKER
    IfLoadedEqualTo HOLD_EFFECT_PRIORITY_DOWN, Basic_CheckMetalBurst_Terminate
    IfSpeedCompareEqualTo COMPARE_SPEED_FASTER, ScoreMinus10

Basic_CheckMetalBurst_Terminate:
    End

Basic_CheckEmbargo:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_EMBARGO, ScoreMinus10
    LoadRecycleItem AI_BATTLER_DEFENDER
    IfLoadedEqualTo ITEM_NONE, Basic_CheckEmbargo_Terminate
    LoadBattleType
    IfLoadedEqualTo 2, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckEmbargo_Terminate
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckEmbargo_Terminate:
    End

Basic_CheckFling:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, ScoreMinus10
    LoadFlingPower AI_BATTLER_ATTACKER
    IfLoadedLessThan 10, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MULTITYPE, ScoreMinus10
    LoadHeldItemEffect AI_BATTLER_ATTACKER
    IfLoadedInTable Basic_FlingItems_Poison, Basic_FlingPoison
    IfLoadedInTable Basic_FlingItems_Burn, Basic_FlingBurn
    IfLoadedInTable Basic_FlingItems_Paralyze, Basic_FlingParalyze
    End

Basic_FlingPoison:
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, Basic_FlingPoison_AttackerChecks
    IfStatus AI_BATTLER_DEFENDER, Basic_FlingPoison_AttackerChecks
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_POISON_HEAL, Basic_FlingPoison_AttackerChecks
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_POISON, Basic_FlingPoison_AttackerChecks
    IfLoadedEqualTo TYPE_STEEL, Basic_FlingPoison_AttackerChecks
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_POISON, Basic_FlingPoison_AttackerChecks
    IfLoadedEqualTo TYPE_STEEL, Basic_FlingPoison_AttackerChecks
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_IMMUNITY, Basic_FlingPoison_AttackerChecks
    IfLoadedEqualTo ABILITY_POISON_HEAL, Basic_FlingPoison_AttackerChecks
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, Basic_FlingPoison_AttackerChecks
    End

Basic_FlingPoison_AttackerChecks:
    IfSideCondition AI_BATTLER_ATTACKER, SIDE_CONDITION_SAFEGUARD, ScoreMinus5
    IfStatus AI_BATTLER_ATTACKER, ScoreMinus5
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_POISON, ScoreMinus5
    IfLoadedEqualTo TYPE_STEEL, ScoreMinus5
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_POISON, ScoreMinus5
    IfLoadedEqualTo TYPE_STEEL, ScoreMinus5
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_KLUTZ, ScoreMinus5
    IfLoadedEqualTo ABILITY_IMMUNITY, ScoreMinus5
    IfLoadedEqualTo ABILITY_POISON_HEAL, ScoreMinus5
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, ScoreMinus5
    IfLoadedEqualTo ABILITY_GUTS, ScoreMinus5
    End

Basic_FlingBurn:
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, Basic_FlingBurn_AttackerChecks
    IfStatus AI_BATTLER_DEFENDER, Basic_FlingBurn_AttackerChecks
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_FIRE, Basic_FlingBurn_AttackerChecks
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_FIRE, Basic_FlingBurn_AttackerChecks
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, Basic_FlingBurn_AttackerChecks
    IfLoadedEqualTo ABILITY_WATER_VEIL, Basic_FlingBurn_AttackerChecks
    End

Basic_FlingBurn_AttackerChecks:
    IfSideCondition AI_BATTLER_ATTACKER, SIDE_CONDITION_SAFEGUARD, ScoreMinus5
    IfStatus AI_BATTLER_ATTACKER, ScoreMinus5
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_FIRE, ScoreMinus5
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_FIRE, ScoreMinus5
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_KLUTZ, ScoreMinus5
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, ScoreMinus5
    IfLoadedEqualTo ABILITY_WATER_VEIL, ScoreMinus5
    IfLoadedEqualTo ABILITY_GUTS, ScoreMinus5
    End

Basic_FlingParalyze:
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, ScoreMinus5
    IfStatus AI_BATTLER_DEFENDER, ScoreMinus5
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_LIMBER, ScoreMinus5
    End

Basic_FlingItems_Poison:
    TableEntry HOLD_EFFECT_PSN_USER
    TableEntry HOLD_EFFECT_STRENGTHEN_POISON
    TableEntry TABLE_END

Basic_FlingItems_Burn:
    TableEntry HOLD_EFFECT_BRN_USER
    TableEntry TABLE_END

Basic_FlingItems_Paralyze:
    TableEntry HOLD_EFFECT_PIKA_SPATK_UP
    TableEntry TABLE_END

Basic_CheckCanPsychoShift:
    IfNotStatus AI_BATTLER_ATTACKER, ScoreMinus10
    IfStatus AI_BATTLER_DEFENDER, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_PsychoShift_CheckSafeguard
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus10

Basic_PsychoShift_CheckSafeguard:
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, ScoreMinus10
    IfCondition AI_BATTLER_ATTACKER, CONDITION_POISON, Basic_PsychoShift_Poison
    IfBadlyPoisoned AI_BATTLER_ATTACKER, Basic_PsychoShift_Poison
    IfCondition AI_BATTLER_ATTACKER, CONDITION_BURN, Basic_PsychoShift_Burn
    IfCondition AI_BATTLER_ATTACKER, CONDITION_PARALYSIS, Basic_PsychoShift_Paralysis
    GoTo Basic_PsychoShift_Terminate

Basic_PsychoShift_Poison:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_POISON_HEAL, ScoreMinus10
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_POISON, ScoreMinus10
    IfLoadedEqualTo TYPE_STEEL, ScoreMinus10
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_POISON, ScoreMinus10
    IfLoadedEqualTo TYPE_STEEL, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_IMMUNITY, ScoreMinus10
    IfLoadedEqualTo ABILITY_POISON_HEAL, ScoreMinus10
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, ScoreMinus10
    GoTo Basic_PsychoShift_Terminate

Basic_PsychoShift_Burn:
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_FIRE, ScoreMinus10
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_FIRE, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, ScoreMinus10
    IfLoadedEqualTo ABILITY_WATER_VEIL, ScoreMinus10
    GoTo Basic_PsychoShift_Terminate

Basic_PsychoShift_Paralysis:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_LIMBER, ScoreMinus10

Basic_PsychoShift_Terminate:
    End

Basic_CheckHealBlock:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_HEAL_BLOCK, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckHealBlock_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckHealBlock_End:
    End

Basic_CheckPowerTrick:
    IfConditionFlag AI_BATTLER_ATTACKER, 10, ScoreMinus10
    End

Basic_CheckGastroAcid:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_GASTRO_ACID, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MULTITYPE, ScoreMinus10
    IfLoadedEqualTo ABILITY_TRUANT, ScoreMinus10
    IfLoadedEqualTo ABILITY_DEFEATIST, ScoreMinus10
    IfLoadedEqualTo ABILITY_SLOW_START, ScoreMinus10
    IfLoadedEqualTo ABILITY_STENCH, ScoreMinus10
    IfLoadedEqualTo ABILITY_RUN_AWAY, ScoreMinus10
    IfLoadedEqualTo ABILITY_PICKUP, ScoreMinus10
    IfLoadedEqualTo ABILITY_HONEY_GATHER, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckGastroAcid_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckGastroAcid_End:
    End

Basic_CheckLuckyChant:
    IfSideCondition AI_BATTLER_ATTACKER, SIDE_CONDITION_LUCKY_CHANT, ScoreMinus10
    End

Basic_CheckCopycat:
    LoadTurnCount
    IfLoadedNotEqualTo 0, Basic_CheckCopycat_Terminate
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Basic_CheckCopycat_Terminate
    IfSpeedCompareEqualTo COMPARE_SPEED_FASTER, ScoreMinus10

Basic_CheckCopycat_Terminate:
    End

Basic_CheckPowerSwap:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE
    IfLoadedLessThan 1, Basic_CheckGuardSwap_SpAttack
    GoTo Basic_CheckPowerSwap_Terminate

Basic_CheckGuardSwap_SpAttack:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE
    IfLoadedLessThan 1, ScoreMinus10

Basic_CheckPowerSwap_Terminate:
    End

Basic_CheckGuardSwap:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE
    IfLoadedLessThan 1, Basic_CheckGuardSwap_SpDefense
    GoTo Basic_CheckGuardSwap_Terminate

Basic_CheckGuardSwap_SpDefense:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE
    IfLoadedLessThan 1, ScoreMinus10

Basic_CheckGuardSwap_Terminate:
    End

Basic_CheckLastResort:
    IfCanUseLastResort AI_BATTLER_ATTACKER, Basic_CheckLastResort_Terminate
    AddToMoveScore -10

Basic_CheckLastResort_Terminate:
    End

Basic_CheckWorrySeed:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_TRUANT, ScoreMinus10
    IfLoadedEqualTo ABILITY_DEFEATIST, ScoreMinus10
    IfLoadedEqualTo ABILITY_INSOMNIA, ScoreMinus10
    IfLoadedEqualTo ABILITY_VITAL_SPIRIT, ScoreMinus10
    IfLoadedEqualTo ABILITY_MULTITYPE, ScoreMinus10
    IfNotCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, Basic_CheckWorrySeed_Terminate
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_SLEEP_TALK, Basic_CheckWorrySeed_Terminate
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_SNORE, Basic_CheckWorrySeed_Terminate
    AddToMoveScore -10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckWorrySeed_Terminate
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckWorrySeed_Terminate:
    End

Basic_CheckToxicSpikes:
    LoadSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_TOXIC_SPIKES
    IfLoadedEqualTo 2, ScoreMinus10
    CountAlivePartyBattlers AI_BATTLER_DEFENDER
    IfLoadedEqualTo 0, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckToxicSpikes_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckToxicSpikes_End:
    End

Basic_CheckAquaRing:
    IfCondition AI_BATTLER_ATTACKER, 35, ScoreMinus10
    End

Basic_CheckMagnetRise:
    IfCondition AI_BATTLER_ATTACKER, 30, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_LEVITATE, ScoreMinus10
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_FLYING, ScoreMinus10
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_FLYING, ScoreMinus10
    End

Basic_CheckDefog:
    IfStatStageNotEqualTo AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 0, Basic_CheckDefog_Terminate
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_LIGHT_SCREEN, Basic_CheckDefog_Terminate
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_REFLECT, Basic_CheckDefog_Terminate
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckDefog_Weather
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckDefog_Weather:
    LoadCurrentWeather
    // Gen 4 skips the rest in deep fog, which Defog clears. Here it skips when the weather is 0
    IfLoadedEqualTo 0, Basic_CheckDefog_Terminate
    CountAlivePartyBattlers AI_BATTLER_DEFENDER
    IfLoadedEqualTo 0, ScoreMinus10
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SPIKES, Basic_CheckDefog_Terminate
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_STEALTH_ROCK, Basic_CheckDefog_Terminate
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_TOXIC_SPIKES, Basic_CheckDefog_Terminate
    GoTo ScoreMinus10

Basic_CheckDefog_Terminate:
    End

Basic_CheckTrickRoom:
    IfSpeedCompareEqualTo COMPARE_SPEED_FASTER, ScoreMinus10
    IfSpeedCompareEqualTo COMPARE_SPEED_TIE, ScoreMinus10
    End

Basic_CheckCaptivate:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_CONTRARY, ScoreMinus12
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckCaptivate_CheckGender
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_OBLIVIOUS, ScoreMinus10
    IfLoadedEqualTo ABILITY_CLEAR_BODY, ScoreMinus10
    IfLoadedEqualTo ABILITY_WHITE_SMOKE, ScoreMinus10
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckCaptivate_CheckGender:
    LoadGender AI_BATTLER_ATTACKER
    IfLoadedEqualTo GENDER_MALE, Basic_CheckCaptivate_CheckMale
    IfLoadedEqualTo GENDER_FEMALE, Basic_CheckCaptivate_CheckFemale
    GoTo ScoreMinus10

Basic_CheckCaptivate_CheckMale:
    LoadGender AI_BATTLER_DEFENDER
    IfLoadedEqualTo GENDER_FEMALE, Basic_CheckCaptivate_CheckStatStage
    GoTo ScoreMinus10

Basic_CheckCaptivate_CheckFemale:
    LoadGender AI_BATTLER_DEFENDER
    IfLoadedEqualTo GENDER_MALE, Basic_CheckCaptivate_CheckStatStage
    GoTo ScoreMinus10

Basic_CheckCaptivate_CheckStatStage:
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 1, ScoreMinus10
    End

Basic_CheckStealthRock:
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_STEALTH_ROCK, ScoreMinus10
    CountAlivePartyBattlers AI_BATTLER_DEFENDER
    IfLoadedEqualTo 0, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckStealthRock_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckStealthRock_End:
    End

Basic_CheckLunarDance:
    AddToMoveScore -20
    CountAlivePartyBattlers AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, ScoreMinus10
    IfAnyPartyMemberIsWounded AI_BATTLER_ATTACKER, Basic_CheckLunarDance_Terminate
    IfPartyMemberNotStatus AI_BATTLER_ATTACKER, Basic_CheckLunarDance_Terminate
    IfAnyPartyMemberUsedPP AI_BATTLER_ATTACKER, Basic_CheckLunarDance_Terminate
    GoTo ScoreMinus10

Basic_CheckLunarDance_Terminate:
    End

Basic_CheckHoneClaws:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo ABILITY_SIMPLE, Basic_CheckHoneClaws_StatStages
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 8, ScoreMinus10
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 8, ScoreMinus8

Basic_CheckHoneClaws_StatStages:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 12, ScoreMinus8
    End

Basic_CheckWideGuard:
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, ScoreMinus10
    End

Basic_CheckGuardSplit:
    End

Basic_CheckPowerSplit:
    End

Basic_CheckWonderRoom:
    IfFieldCondition 6, ScoreMinus10
    End

Basic_CheckPsyshock:
    End

Basic_CheckVenoshock:
    End

Basic_CheckAutotomize:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SPEED_STAGE, 12, ScoreMinus10
    End

Basic_CheckTelekinesis:
    IfCondition AI_BATTLER_DEFENDER, 32, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckTelekinesis_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckTelekinesis_End:
    End

Basic_CheckMagicRoom:
    IfFieldCondition 7, ScoreMinus10
    End

Basic_CheckSmackDown:
    End

Basic_CheckStormThrow:
    End

Basic_CheckFlameBurst:
    End

Basic_CheckQuiverDance:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 12, ScoreMinus8
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SPEED_STAGE, 12, ScoreMinus6
    End

Basic_CheckHeavySlam:
    End

Basic_CheckSynchronoise:
    End

Basic_CheckElectroBall:
    End

Basic_CheckSoak:
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfTempNotEqualTo TYPE_WATER, Basic_CheckSoak_End
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfTempEqualTo TYPE_WATER, ScoreMinus10

Basic_CheckSoak_End:
    End

Basic_CheckFlameCharge:
    End

Basic_CheckAcidSpray:
    End

Basic_CheckFoulPlay:
    End

Basic_CheckSimpleBeam:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MULTITYPE, ScoreMinus10
    IfLoadedEqualTo ABILITY_TRUANT, ScoreMinus10
    IfLoadedEqualTo ABILITY_DEFEATIST, ScoreMinus10
    IfLoadedEqualTo ABILITY_SLOW_START, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckSimpleBeam_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckSimpleBeam_End:
    End

Basic_CheckEntrainment:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckEntrainment_End
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_BOUNCE, ScoreMinus12

Basic_CheckEntrainment_End:
    End

Basic_CheckAfterYou:
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, ScoreMinus10
    End

Basic_CheckRound:
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, ScoreMinus10
    End

Basic_CheckEchoedVoice:
    End

Basic_CheckChipAway:
    End

Basic_CheckClearSmog:
    End

Basic_CheckStoredPower:
    End

Basic_CheckQuickGuard:
    End

Basic_CheckAllySwitch:
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, ScoreMinus10
    End

Basic_CheckShellSmash:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 12, ScoreMinus8
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SPEED_STAGE, 12, ScoreMinus6
    End

Basic_CheckHealPulse:
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, ScoreMinus10
    End

Basic_CheckHex:
    End

Basic_CheckSkyDrop:
    End

Basic_CheckShiftGear:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SPEED_STAGE, 12, ScoreMinus8
    End

Basic_CheckCircleThrow:
    End

Basic_CheckIncinerate:
    End

Basic_CheckQuash:
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, ScoreMinus10
    End

Basic_CheckGrowth:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 12, ScoreMinus8
    End

Basic_CheckAcrobatics:
    End

Basic_CheckReflectType:
    End

Basic_CheckRetaliate:
    End

Basic_CheckFinalGambit:
    CountAlivePartyBattlers AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo 0, Basic_CheckFinalGambit_End
    CountAlivePartyBattlers AI_BATTLER_DEFENDER
    IfLoadedNotEqualTo 0, ScoreMinus10
    GoTo ScoreMinus1

Basic_CheckFinalGambit_End:
    End

Basic_CheckTailGlow:
    End

Basic_CheckCoil:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 12, ScoreMinus8
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 12, ScoreMinus6
    End

Basic_CheckBestow:
    LoadHeldItem AI_BATTLER_DEFENDER
    IfLoadedNotEqualTo ITEM_NONE, ScoreMinus10
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, ScoreMinus10
    End

Basic_CheckWaterPledge:
    End

Basic_CheckFirePledge:
    End

Basic_CheckGrassPledge:
    End

Basic_CheckWorkUp:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 12, ScoreMinus10
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 12, ScoreMinus8
    End

Basic_CheckCottonGuard:
    IfStatStageEqualTo AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 12, ScoreMinus10
    End

Basic_CheckRelicSong:
    End

Basic_CheckGlaciate:
    End

Basic_CheckFreezeShock:
    End

Basic_CheckIceBurn:
    End

Basic_CheckUnused333:
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
    .balign 4, 0
