#include "asm/tr_ai.inc"

// AI flag 2, Expert: scores each move effect by how useful it is in the situation, from Expert_MoveEffectTable.

Expert_Main:
    IfTargetIsPartner Terminate
    GoToByMoveEffect 0, 337, Expert_MoveEffectTable
    End

Expert_MoveEffectTable:
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_StatusSleep, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_DrainMove, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Explosion, Expert_MoveEffectTable
    LabelDistance Expert_DreamEater, Expert_MoveEffectTable
    LabelDistance Expert_MirrorMove, Expert_MoveEffectTable
    LabelDistance Expert_StatusAttackUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusDefenseUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpeedUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpAttackUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpDefenseUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusAccuracyUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusEvasionUp, Expert_MoveEffectTable
    LabelDistance Expert_BypassAccuracyMove, Expert_MoveEffectTable
    LabelDistance Expert_StatusAttackDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusDefenseDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpeedDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpAttackDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpDefenseDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusAccuracyDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusEvasionDown, Expert_MoveEffectTable
    LabelDistance Expert_Haze, Expert_MoveEffectTable
    LabelDistance Expert_Bide, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_ForceSwitch, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Conversion, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Recovery, Expert_MoveEffectTable
    LabelDistance Expert_ToxicLeechSeed, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_LightScreen, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Rest, Expert_MoveEffectTable
    LabelDistance Expert_OHKOMove, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnNoInvuln, Expert_MoveEffectTable
    LabelDistance Expert_SuperFang, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_BindingMove, Expert_MoveEffectTable
    LabelDistance Expert_HighCritical, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_RecoilMove, Expert_MoveEffectTable
    LabelDistance Expert_StatusConfuse, Expert_MoveEffectTable
    LabelDistance Expert_StatusAttackUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusDefenseUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpeedUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpAttackUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpDefenseUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusAccuracyUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusEvasionUp, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_StatusAttackDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusDefenseDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpeedDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpAttackDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpDefenseDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusAccuracyDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusEvasionDown, Expert_MoveEffectTable
    LabelDistance Expert_Reflect, Expert_MoveEffectTable
    LabelDistance Expert_StatusPoison, Expert_MoveEffectTable
    LabelDistance Expert_StatusParalyze, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_SpeedDownOnHit, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnNoInvuln, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_VitalThrow, Expert_MoveEffectTable
    LabelDistance Expert_Substitute, Expert_MoveEffectTable
    LabelDistance Expert_RechargeTurn, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_ToxicLeechSeed, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Disable, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Counter, Expert_MoveEffectTable
    LabelDistance Expert_Encore, Expert_MoveEffectTable
    LabelDistance Expert_PainSplit, Expert_MoveEffectTable
    LabelDistance Expert_Nightmare, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_LockOn, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_SleepTalk, Expert_MoveEffectTable
    LabelDistance Expert_DestinyBond, Expert_MoveEffectTable
    LabelDistance Expert_Reversal, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_HealBell, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Thief, Expert_MoveEffectTable
    LabelDistance Expert_BindingMove, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_StatusEvasionUp, Expert_MoveEffectTable
    LabelDistance Expert_Curse, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Protect, Expert_MoveEffectTable
    LabelDistance Expert_Spikes, Expert_MoveEffectTable
    LabelDistance Expert_Foresight, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Endure, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Swagger, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_BatonPass, Expert_MoveEffectTable
    LabelDistance Expert_Pursuit, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Synthesis, Expert_MoveEffectTable
    LabelDistance Expert_Synthesis, Expert_MoveEffectTable
    LabelDistance Expert_Synthesis, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_RainDance, Expert_MoveEffectTable
    LabelDistance Expert_SunnyDay, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_BellyDrum, Expert_MoveEffectTable
    LabelDistance Expert_PsychUp, Expert_MoveEffectTable
    LabelDistance Expert_MirrorCoat, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnNoInvuln, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnNoInvuln, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnNoInvuln_CheckEffectivenessAndWeather, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnWithInvuln, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Recovery, Expert_MoveEffectTable
    LabelDistance Expert_FakeOut, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Stockpile, Expert_MoveEffectTable
    LabelDistance Expert_SpitUp, Expert_MoveEffectTable
    LabelDistance Expert_Recovery, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Hail, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Flatter, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Explosion, Expert_MoveEffectTable
    LabelDistance Expert_Facade, Expert_MoveEffectTable
    LabelDistance Expert_FocusPunch, Expert_MoveEffectTable
    LabelDistance Expert_SmellingSalts, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Trick, Expert_MoveEffectTable
    LabelDistance Expert_ChangeUserAbility, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Ingrain, Expert_MoveEffectTable
    LabelDistance Expert_Superpower, Expert_MoveEffectTable
    LabelDistance Expert_MagicCoat, Expert_MoveEffectTable
    LabelDistance Expert_Recycle, Expert_MoveEffectTable
    LabelDistance Expert_Revenge, Expert_MoveEffectTable
    LabelDistance Expert_BrickBreak, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_KnockOff, Expert_MoveEffectTable
    LabelDistance Expert_Endeavor, Expert_MoveEffectTable
    LabelDistance Expert_WaterSpout, Expert_MoveEffectTable
    LabelDistance Expert_ChangeUserAbility, Expert_MoveEffectTable
    LabelDistance Expert_Imprison, Expert_MoveEffectTable
    LabelDistance Expert_Refresh, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Snatch, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_RecoilMove, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_HighCritical, Expert_MoveEffectTable
    LabelDistance Expert_MudSport, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Overheat, Expert_MoveEffectTable
    LabelDistance Expert_StatusDefenseDown, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpDefenseUp, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_StatusDefenseUp, Expert_MoveEffectTable
    LabelDistance Expert_HighCritical, Expert_MoveEffectTable
    LabelDistance Expert_WaterSport, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpDefenseUp, Expert_MoveEffectTable
    LabelDistance Expert_DragonDance, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Recovery, Expert_MoveEffectTable
    LabelDistance Expert_Gravity, Expert_MoveEffectTable
    LabelDistance Expert_MiracleEye, Expert_MoveEffectTable
    LabelDistance Expert_WakeUpSlap, Expert_MoveEffectTable
    LabelDistance Expert_HammerArm, Expert_MoveEffectTable
    LabelDistance Expert_GyroBall, Expert_MoveEffectTable
    LabelDistance Expert_HealingWish, Expert_MoveEffectTable
    LabelDistance Expert_Brine, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Feint, Expert_MoveEffectTable
    LabelDistance Expert_Pluck, Expert_MoveEffectTable
    LabelDistance Expert_Tailwind, Expert_MoveEffectTable
    LabelDistance Expert_Acupressure, Expert_MoveEffectTable
    LabelDistance Expert_MetalBurst, Expert_MoveEffectTable
    LabelDistance Expert_UTurn, Expert_MoveEffectTable
    LabelDistance Expert_CloseCombat, Expert_MoveEffectTable
    LabelDistance Expert_Payback, Expert_MoveEffectTable
    LabelDistance Expert_Assurance, Expert_MoveEffectTable
    LabelDistance Expert_Embargo, Expert_MoveEffectTable
    LabelDistance Expert_Fling, Expert_MoveEffectTable
    LabelDistance Expert_PsychoShift, Expert_MoveEffectTable
    LabelDistance Expert_TrumpCard, Expert_MoveEffectTable
    LabelDistance Expert_HealBlock, Expert_MoveEffectTable
    LabelDistance Expert_WringOut, Expert_MoveEffectTable
    LabelDistance Expert_PowerTrick, Expert_MoveEffectTable
    LabelDistance Expert_GastroAcid, Expert_MoveEffectTable
    LabelDistance Expert_LuckyChant, Expert_MoveEffectTable
    LabelDistance Expert_MeFirst, Expert_MoveEffectTable
    LabelDistance Expert_Copycat, Expert_MoveEffectTable
    LabelDistance Expert_PowerSwap, Expert_MoveEffectTable
    LabelDistance Expert_GuardSwap, Expert_MoveEffectTable
    LabelDistance Expert_Punishment, Expert_MoveEffectTable
    LabelDistance Expert_LastResort, Expert_MoveEffectTable
    LabelDistance Expert_WorrySeed, Expert_MoveEffectTable
    LabelDistance Expert_SuckerPunch, Expert_MoveEffectTable
    LabelDistance Expert_ToxicSpikes, Expert_MoveEffectTable
    LabelDistance Expert_HeartSwap, Expert_MoveEffectTable
    LabelDistance Expert_AquaRing, Expert_MoveEffectTable
    LabelDistance Expert_MagnetRise, Expert_MoveEffectTable
    LabelDistance Expert_RecoilMove, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnWithInvuln, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnWithInvuln, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Defog, Expert_MoveEffectTable
    LabelDistance Expert_TrickRoom, Expert_MoveEffectTable
    LabelDistance Expert_Blizzard, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_RecoilMove, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnWithInvuln, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_Captivate, Expert_MoveEffectTable
    LabelDistance Expert_StealthRock, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_RecoilMove, Expert_MoveEffectTable
    LabelDistance Expert_HealingWish, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_ShadowForce, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_HoneClaws, Expert_MoveEffectTable
    LabelDistance Expert_WideGuard, Expert_MoveEffectTable
    LabelDistance Expert_GuardSplit, Expert_MoveEffectTable
    LabelDistance Expert_PowerSplit, Expert_MoveEffectTable
    LabelDistance Expert_WonderRoom, Expert_MoveEffectTable
    LabelDistance Expert_Psyshock, Expert_MoveEffectTable
    LabelDistance Expert_Venoshock, Expert_MoveEffectTable
    LabelDistance Expert_Autotomize, Expert_MoveEffectTable
    LabelDistance Expert_Telekinesis, Expert_MoveEffectTable
    LabelDistance Expert_MagicRoom, Expert_MoveEffectTable
    LabelDistance Expert_SmackDown, Expert_MoveEffectTable
    LabelDistance Expert_StormThrow, Expert_MoveEffectTable
    LabelDistance Expert_FlameBurst, Expert_MoveEffectTable
    LabelDistance Expert_QuiverDance, Expert_MoveEffectTable
    LabelDistance Expert_HeavySlam, Expert_MoveEffectTable
    LabelDistance Expert_Synchronoise, Expert_MoveEffectTable
    LabelDistance Expert_ElectroBall, Expert_MoveEffectTable
    LabelDistance Expert_Soak, Expert_MoveEffectTable
    LabelDistance Expert_FlameCharge, Expert_MoveEffectTable
    LabelDistance Expert_AcidSpray, Expert_MoveEffectTable
    LabelDistance Expert_FoulPlay, Expert_MoveEffectTable
    LabelDistance Expert_SimpleBeam, Expert_MoveEffectTable
    LabelDistance Expert_Entrainment, Expert_MoveEffectTable
    LabelDistance Expert_AfterYou, Expert_MoveEffectTable
    LabelDistance Expert_Round, Expert_MoveEffectTable
    LabelDistance Expert_EchoedVoice, Expert_MoveEffectTable
    LabelDistance Expert_ChipAway, Expert_MoveEffectTable
    LabelDistance Expert_ClearSmog, Expert_MoveEffectTable
    LabelDistance Expert_StoredPower, Expert_MoveEffectTable
    LabelDistance Expert_QuickGuard, Expert_MoveEffectTable
    LabelDistance Expert_AllySwitch, Expert_MoveEffectTable
    LabelDistance Expert_ShellSmash, Expert_MoveEffectTable
    LabelDistance Expert_HealPulse, Expert_MoveEffectTable
    LabelDistance Expert_Hex, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnWithInvuln_CheckConditions, Expert_MoveEffectTable
    LabelDistance Expert_ShiftGear, Expert_MoveEffectTable
    LabelDistance Expert_ForceSwitch, Expert_MoveEffectTable
    LabelDistance Expert_Incinerate, Expert_MoveEffectTable
    LabelDistance Expert_Quash, Expert_MoveEffectTable
    LabelDistance Expert_Growth, Expert_MoveEffectTable
    LabelDistance Expert_Acrobatics, Expert_MoveEffectTable
    LabelDistance Expert_ReflectType, Expert_MoveEffectTable
    LabelDistance Expert_Retaliate, Expert_MoveEffectTable
    LabelDistance Expert_FinalGambit, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpAttackUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusDefenseUp, Expert_MoveEffectTable
    LabelDistance Expert_Bestow, Expert_MoveEffectTable
    LabelDistance Expert_WaterPledge, Expert_MoveEffectTable
    LabelDistance Expert_FirePledge, Expert_MoveEffectTable
    LabelDistance Expert_GrassPledge, Expert_MoveEffectTable
    LabelDistance Expert_WorkUp, Expert_MoveEffectTable
    LabelDistance Expert_StatusSpAttackUp, Expert_MoveEffectTable
    LabelDistance Expert_RelicSong, Expert_MoveEffectTable
    LabelDistance Expert_Glaciate, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnNoInvuln, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnNoInvuln, Expert_MoveEffectTable
    LabelDistance Expert_Unused333, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Terminate, Expert_MoveEffectTable
    LabelDistance Expert_ChargeTurnNoInvuln_CheckEffectivenessAndWeather, Expert_MoveEffectTable

Terminate:
    End

Expert_StatusSleep:
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_RECOVER_DAMAGE_SLEEP, Expert_StatusSleep_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_STATUS_NIGHTMARE, Expert_StatusSleep_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_USE_RANDOM_LEARNED_MOVE_SLEEP, Expert_StatusSleep_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_APPLY_MAGIC_COAT, Expert_StatusSleep_TryScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_DAMAGE_WHILE_ASLEEP, Expert_StatusSleep_TryScoreMinus1
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_NO_GUARD, Expert_StatusSleep_TryScorePlus1
    IfLoadedEqualTo ABILITY_EARLY_BIRD, Expert_StatusSleep_TryScoreMinus1
    GoTo Expert_StatusSleep_End

Expert_StatusSleep_ScoreMinus1:
    AddToMoveScore -1

Expert_StatusSleep_TryScoreMinus1:
    IfRandomLessThan 128, Expert_StatusSleep_End
    AddToMoveScore -1
    GoTo Expert_StatusSleep_End

Expert_StatusSleep_TryScorePlus1:
    IfRandomLessThan 128, Expert_StatusSleep_End
    AddToMoveScore 1

Expert_StatusSleep_End:
    End

Expert_DrainMove:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_DrainMove_TryScoreMinus3
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_DrainMove_TryScoreMinus3
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_DrainMove_TryScoreMinus3
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_LIQUID_OOZE, Expert_DrainMove_TryScoreMinus3
    GoTo Expert_DrainMove_End

Expert_DrainMove_TryScoreMinus3:
    IfRandomLessThan 50, Expert_DrainMove_End
    AddToMoveScore -3

Expert_DrainMove_End:
    End

Expert_Explosion:
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 7, Expert_Explosion_CheckUserHighHP
    AddToMoveScore -1
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 10, Expert_Explosion_CheckUserHighHP
    IfRandomLessThan 128, Expert_Explosion_CheckUserHighHP
    AddToMoveScore -1

Expert_Explosion_CheckUserHighHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 80, Expert_Explosion_CheckUserMediumHP
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Explosion_CheckUserMediumHP
    IfTurnRandomLessThan 50, Expert_Explosion_End
    GoTo ScoreMinus3

Expert_Explosion_CheckUserMediumHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_Explosion_TryScoreMinus1
    IfTurnRandomGreaterThan 128, Expert_Explosion_CheckUserLowHP
    AddToMoveScore 1

Expert_Explosion_CheckUserLowHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 30, Expert_Explosion_End
    IfTurnRandomGreaterThan 128, Expert_Explosion_End
    AddToMoveScore 1
    GoTo Expert_Explosion_End

Expert_Explosion_TryScoreMinus1:
    IfTurnRandomLessThan 50, Expert_Explosion_End
    AddToMoveScore -1

Expert_Explosion_End:
    End

Expert_DreamEater:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_DreamEater_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_DreamEater_TryScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_DreamEater_TryScoreMinus1
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_LIQUID_OOZE, Expert_DreamEater_ScoreMinus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, Expert_DreamEater_TryScorePlus3
    GoTo Expert_DreamEater_End

Expert_DreamEater_TryScorePlus3:
    IfRandomLessThan 51, Expert_DreamEater_End
    AddToMoveScore 3
    GoTo Expert_DreamEater_End

Expert_DreamEater_ScoreMinus1:
    IfRandomLessThan 50, Expert_DreamEater_TryScoreMinus1
    AddToMoveScore -2

Expert_DreamEater_TryScoreMinus1:
    IfRandomLessThan 50, Expert_DreamEater_End
    AddToMoveScore -1

Expert_DreamEater_End:
    End

Expert_MirrorMove:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_MirrorMove_TryScoreMinus1
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    IfLoadedNotInTable Expert_MirrorMove_MoveTable, Expert_MirrorMove_TryScoreMinus1
    IfRandomLessThan 128, Expert_MirrorMove_End
    AddToMoveScore 2
    GoTo Expert_MirrorMove_End

Expert_MirrorMove_TryScoreMinus1:
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    IfLoadedInTable Expert_MirrorMove_MoveTable, Expert_MirrorMove_End
    IfRandomLessThan 80, Expert_MirrorMove_End
    AddToMoveScore -1

Expert_MirrorMove_End:
    End

Expert_MirrorMove_MoveTable:
    TableEntry MOVE_SLEEP_POWDER
    TableEntry MOVE_LOVELY_KISS
    TableEntry MOVE_SPORE
    TableEntry MOVE_HYPNOSIS
    TableEntry MOVE_SING
    TableEntry MOVE_GRASS_WHISTLE
    TableEntry MOVE_SHADOW_PUNCH
    TableEntry MOVE_SAND_ATTACK
    TableEntry MOVE_SMOKE_SCREEN
    TableEntry MOVE_TOXIC
    TableEntry MOVE_GUILLOTINE
    TableEntry MOVE_HORN_DRILL
    TableEntry MOVE_FISSURE
    TableEntry MOVE_SHEER_COLD
    TableEntry MOVE_CROSS_CHOP
    TableEntry MOVE_AEROBLAST
    TableEntry MOVE_CONFUSE_RAY
    TableEntry MOVE_SWEET_KISS
    TableEntry MOVE_SCREECH
    TableEntry MOVE_COTTON_SPORE
    TableEntry MOVE_SCARY_FACE
    TableEntry MOVE_FAKE_TEARS
    TableEntry MOVE_METAL_SOUND
    TableEntry MOVE_THUNDER_WAVE
    TableEntry MOVE_GLARE
    TableEntry MOVE_POISON_POWDER
    TableEntry MOVE_SHADOW_BALL
    TableEntry MOVE_DYNAMIC_PUNCH
    TableEntry MOVE_HYPER_BEAM
    TableEntry MOVE_EXTREME_SPEED
    TableEntry MOVE_THIEF
    TableEntry MOVE_COVET
    TableEntry MOVE_ATTRACT
    TableEntry MOVE_SWAGGER
    TableEntry MOVE_TORMENT
    TableEntry MOVE_FLATTER
    TableEntry MOVE_TRICK
    TableEntry MOVE_SUPERPOWER
    TableEntry MOVE_SKILL_SWAP
    TableEntry MOVE_PSYCHO_SHIFT
    TableEntry MOVE_POWER_SWAP
    TableEntry MOVE_GUARD_SWAP
    TableEntry MOVE_SUCKER_PUNCH
    TableEntry MOVE_HEART_SWAP
    TableEntry MOVE_SWITCHEROO
    TableEntry MOVE_CAPTIVATE
    TableEntry MOVE_DARK_VOID
    TableEntry MOVE_GLACIATE
    TableEntry MOVE_SNARL
    TableEntry TABLE_END

Expert_StatusAttackUp:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 9, Expert_StatusAttackUp_CheckUserAtMaxHP
    IfRandomLessThan 100, Expert_StatusAttackUp_CheckUserHPRange
    AddToMoveScore -1
    GoTo Expert_StatusAttackUp_CheckUserHPRange

Expert_StatusAttackUp_CheckUserAtMaxHP:
    IfHPPercentNotEqualTo AI_BATTLER_ATTACKER, 100, Expert_StatusAttackUp_CheckUserHPRange
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_PASS_STATS_AND_STATUS, Expert_StatusAttackUp_ScorePlus2
    IfRandomLessThan 128, Expert_StatusAttackUp_CheckUserHPRange

Expert_StatusAttackUp_ScorePlus2:
    AddToMoveScore 2

Expert_StatusAttackUp_CheckUserHPRange:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_StatusAttackUp_End
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_StatusAttackUp_ScoreMinus2
    IfRandomLessThan 40, Expert_StatusAttackUp_End

Expert_StatusAttackUp_ScoreMinus2:
    AddToMoveScore -2

Expert_StatusAttackUp_End:
    End

Expert_StatusDefenseUp:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 9, Expert_StatusDefenseUp_CheckUserAtMaxHP
    IfRandomLessThan 100, Expert_StatusDefenseUp_ScorePlus2
    AddToMoveScore -1
    GoTo Expert_StatusDefenseUp_ScorePlus2

Expert_StatusDefenseUp_CheckUserAtMaxHP:
    IfHPPercentNotEqualTo AI_BATTLER_ATTACKER, 100, Expert_StatusDefenseUp_CheckUserHighHP
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_PASS_STATS_AND_STATUS, Expert_StatusDefenseUp_ScorePlus2
    IfRandomLessThan 128, Expert_StatusDefenseUp_CheckUserHighHP

Expert_StatusDefenseUp_ScorePlus2:
    AddToMoveScore 2

Expert_StatusDefenseUp_CheckUserHighHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_StatusDefenseUp_CheckUserMediumHP
    IfRandomLessThan 200, Expert_StatusDefenseUp_End

Expert_StatusDefenseUp_CheckUserMediumHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_StatusDefenseUp_ScoreMinus2
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadPowerOfLoadedMove
    IfLoadedEqualTo 0, Expert_StatusDefenseUp_UserAtLowHP
    LoadDefenderLastUsedMoveClass
    IfLoadedEqualTo MOVE_CATEGORY_SPECIAL, Expert_StatusDefenseUp_ScoreMinus2
    IfRandomLessThan 60, Expert_StatusDefenseUp_End

Expert_StatusDefenseUp_UserAtLowHP:
    IfRandomLessThan 60, Expert_StatusDefenseUp_End

Expert_StatusDefenseUp_ScoreMinus2:
    AddToMoveScore -2

Expert_StatusDefenseUp_End:
    End

Expert_Autotomize:
    IfMoveEffectNotKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_INCREASE_POWER_WITH_WEIGHT, Expert_StatusSpeedUp
    IfRandomLessThan 60, Expert_StatusSpeedUp
    AddToMoveScore 1

Expert_StatusSpeedUp:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_StatusSpeedUp_TryScorePlus3
    AddToMoveScore -3
    GoTo Expert_StatusSpeedUp_End

Expert_StatusSpeedUp_TryScorePlus3:
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_FLINCH_HIT, Expert_StatusSpeedUp_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_RESTORE_HALF_HP, Expert_StatusSpeedUp_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_REST, Expert_StatusSpeedUp_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_HEAL_HALF_MORE_IN_SUN, Expert_StatusSpeedUp_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_HEAL_HALF_REMOVE_FLYING_TYPE, Expert_StatusSpeedUp_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_SET_SUBSTITUTE, Expert_StatusSpeedUp_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_AVERAGE_HP, Expert_StatusSpeedUp_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_FLINCH_MINIMIZE_DOUBLE_HIT, Expert_StatusSpeedUp_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_KO_MON_THAT_DEFEATED_USER, Expert_StatusSpeedUp_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_PASS_STATS_AND_STATUS, Expert_StatusSpeedUp_TryScorePlus2
    GoTo Expert_StatusSpeedUp_TryScorePlus2_2

Expert_StatusSpeedUp_TryScorePlus2:
    IfRandomLessThan 70, Expert_StatusSpeedUp_TryScorePlus2_2
    AddToMoveScore 2

Expert_StatusSpeedUp_TryScorePlus2_2:
    IfRandomLessThan 70, Expert_StatusSpeedUp_End
    AddToMoveScore 2

Expert_StatusSpeedUp_End:
    End

Expert_StatusSpAttackUp:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 9, Expert_StatusSpAttackUp_CheckUserAtMaxHP
    IfRandomLessThan 100, Expert_StatusSpAttackUp_CheckUserHPRange
    AddToMoveScore -1
    GoTo Expert_StatusSpAttackUp_CheckUserHPRange

Expert_StatusSpAttackUp_CheckUserAtMaxHP:
    IfHPPercentNotEqualTo AI_BATTLER_ATTACKER, 100, Expert_StatusSpAttackUp_CheckUserHPRange
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_PASS_STATS_AND_STATUS, Expert_StatusSpAttackUp_ScorePlus2
    IfRandomLessThan 128, Expert_StatusSpAttackUp_CheckUserHPRange

Expert_StatusSpAttackUp_ScorePlus2:
    AddToMoveScore 2

Expert_StatusSpAttackUp_CheckUserHPRange:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_StatusSpAttackUp_End
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_StatusSpAttackUp_ScoreMinus2
    IfRandomLessThan 70, Expert_StatusSpAttackUp_End

Expert_StatusSpAttackUp_ScoreMinus2:
    AddToMoveScore -2

Expert_StatusSpAttackUp_End:
    End

Expert_StatusSpDefenseUp:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 9, Expert_StatusSpDefenseUp_CheckUserAtMaxHP
    IfRandomLessThan 100, Expert_StatusSpDefenseUp_CheckUserHighHP
    AddToMoveScore -1
    GoTo Expert_StatusSpDefenseUp_CheckUserHighHP

Expert_StatusSpDefenseUp_CheckUserAtMaxHP:
    IfHPPercentNotEqualTo AI_BATTLER_ATTACKER, 100, Expert_StatusSpDefenseUp_CheckUserHighHP
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_PASS_STATS_AND_STATUS, Expert_StatusSpDefenseUp_ScorePlus2
    IfRandomLessThan 128, Expert_StatusSpDefenseUp_CheckUserHighHP

Expert_StatusSpDefenseUp_ScorePlus2:
    AddToMoveScore 2

Expert_StatusSpDefenseUp_CheckUserHighHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_StatusSpDefenseUp_CheckUserMediumHP
    IfRandomLessThan 200, Expert_StatusSpDefenseUp_End

Expert_StatusSpDefenseUp_CheckUserMediumHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_StatusSpDefenseUp_TryScoreMinus2
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadPowerOfLoadedMove
    IfLoadedEqualTo 0, Expert_StatusSpDefenseUp_UserAtLowHP
    LoadDefenderLastUsedMoveClass
    IfLoadedEqualTo MOVE_CATEGORY_PHYSICAL, Expert_StatusSpDefenseUp_TryScoreMinus2
    IfRandomLessThan 60, Expert_StatusSpDefenseUp_End

Expert_StatusSpDefenseUp_UserAtLowHP:
    IfRandomLessThan 60, Expert_StatusSpDefenseUp_End

Expert_StatusSpDefenseUp_TryScoreMinus2:
    AddToMoveScore -2

Expert_StatusSpDefenseUp_End:
    End

Expert_StatusAccuracyUp:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 9, Expert_StatusAccuracyUp_TryScoreMinus2
    IfRandomLessThan 50, Expert_StatusAccuracyUp_TryScoreMinus2
    AddToMoveScore -2

Expert_StatusAccuracyUp_TryScoreMinus2:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_StatusAccuracyUp_End
    AddToMoveScore -2

Expert_StatusAccuracyUp_End:
    End

Expert_StatusEvasionUp:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_NO_GUARD, Expert_StatusEvasionUp_ScoreMinus2_2
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_NO_GUARD, Expert_StatusEvasionUp_ScoreMinus2_2
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadEffectOfLoadedMove
    IfLoadedEqualTo BATTLE_EFFECT_BYPASS_ACCURACY, Expert_StatusEvasionUp_ScoreMinus2_2
    IfLoadedEqualTo BATTLE_EFFECT_HIGHER_POWER_WHEN_LOW_PP, Expert_StatusEvasionUp_ScoreMinus2_2
    IfLoadedEqualTo BATTLE_EFFECT_SHADOW_FORCE, Expert_StatusEvasionUp_ScoreMinus2_2
    IfCondition AI_BATTLER_ATTACKER, CONDITION_CURSE, Expert_StatusEvasionUp_ScoreMinus2_2
    IfCondition AI_BATTLER_ATTACKER, CONDITION_FORESIGHT, Expert_StatusEvasionUp_ScoreMinus2_2
    LoadCurrentWeather
    IfLoadedNotEqualTo BTL_WEATHER_RAIN, Expert_StatusEvasionUp_CheckHail
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadEffectOfLoadedMove
    IfLoadedEqualTo BATTLE_EFFECT_THUNDER, Expert_StatusEvasionUp_ScoreMinus2_2

Expert_StatusEvasionUp_CheckHail:
    LoadCurrentWeather
    IfLoadedNotEqualTo BTL_WEATHER_HAIL, Expert_StatusEvasionUp_CheckResidualDamage
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadEffectOfLoadedMove
    IfLoadedEqualTo BATTLE_EFFECT_BLIZZARD, Expert_StatusEvasionUp_ScoreMinus2_2

Expert_StatusEvasionUp_CheckResidualDamage:
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_RESTORE_HALF_HP, Expert_StatusEvasionUp_CheckEnemyCursed
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_HEAL_HALF_MORE_IN_SUN, Expert_StatusEvasionUp_CheckEnemyCursed
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_DEF_UP_DOUBLE_ROLLOUT_POWER, Expert_StatusEvasionUp_CheckEnemyCursed
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_SWALLOW, Expert_StatusEvasionUp_CheckEnemyCursed
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_HEAL_HALF_REMOVE_FLYING_TYPE, Expert_StatusEvasionUp_CheckEnemyCursed
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, Expert_StatusEvasionUp_CheckEnemyCursed
    IfCondition AI_BATTLER_ATTACKER, CONDITION_LEECH_SEED, Expert_StatusEvasionUp_TryScoreMinus1
    IfBadlyPoisoned AI_BATTLER_ATTACKER, Expert_StatusEvasionUp_CheckPoisonHeal
    IfCondition AI_BATTLER_ATTACKER, CONDITION_POISON, Expert_StatusEvasionUp_CheckPoisonHeal
    IfCondition AI_BATTLER_ATTACKER, CONDITION_BURN, Expert_StatusEvasionUp_TryScoreMinus1
    GoTo Expert_StatusEvasionUp_CheckEnemyCursed

Expert_StatusEvasionUp_CheckPoisonHeal:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_POISON_HEAL, Expert_StatusEvasionUp_TryScorePlus1
    GoTo Expert_StatusEvasionUp_TryScoreMinus1

Expert_StatusEvasionUp_TryScorePlus1:
    IfRandomLessThan 50, Expert_StatusEvasionUp_CheckEnemyCursed
    AddToMoveScore 1
    GoTo Expert_StatusEvasionUp_CheckEnemyCursed

Expert_StatusEvasionUp_TryScoreMinus1:
    IfRandomLessThan 50, Expert_StatusEvasionUp_CheckEnemyCursed
    AddToMoveScore -1

Expert_StatusEvasionUp_CheckEnemyCursed:
    IfCondition AI_BATTLER_ATTACKER, CONDITION_INGRAIN, Expert_StatusEvasionUp_CheckHPRanges
    IfNotCondition AI_BATTLER_ATTACKER, 35, Expert_StatusEvasionUp_CheckHPRanges
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_LEFTOVERS, Expert_StatusEvasionUp_CheckHPRanges
    IfLoadedNotEqualTo 2, Expert_StatusEvasionUp_End
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_DRY_SKIN, Expert_StatusEvasionUp_CheckHPRanges
    IfLoadedEqualTo ABILITY_RAIN_DISH, Expert_StatusEvasionUp_CheckHPRanges
    GoTo Expert_StatusEvasionUp_End

Expert_StatusEvasionUp_CheckHPRanges:
    IfRandomLessThan 50, Expert_StatusEvasionUp_End

Expert_StatusEvasionUp_ScoreMinus2:
    AddToMoveScore 1

Expert_StatusEvasionUp_End:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, Expert_StatusEvasionUp_CheckEvasionStage
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CURSE, Expert_StatusEvasionUp_TryScorePlus1_2
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_RESTORE_HALF_HP, Expert_StatusEvasionUp_CheckEvasionStage
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HEAL_HALF_MORE_IN_SUN, Expert_StatusEvasionUp_CheckEvasionStage
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_DEF_UP_DOUBLE_ROLLOUT_POWER, Expert_StatusEvasionUp_CheckEvasionStage
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_SWALLOW, Expert_StatusEvasionUp_CheckEvasionStage
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HEAL_HALF_REMOVE_FLYING_TYPE, Expert_StatusEvasionUp_CheckEvasionStage
    IfCondition AI_BATTLER_DEFENDER, CONDITION_LEECH_SEED, Expert_StatusEvasionUp_TryScorePlus1_2
    IfCondition AI_BATTLER_DEFENDER, CONDITION_BURN, Expert_StatusEvasionUp_TryScorePlus1_2
    IfBadlyPoisoned AI_BATTLER_DEFENDER, Expert_StatusEvasionUp_CheckTargetPoisonHeal
    IfCondition AI_BATTLER_DEFENDER, CONDITION_POISON, Expert_StatusEvasionUp_CheckTargetPoisonHeal
    IfLoadedNotEqualTo ABILITY_STENCH, Expert_StatusEvasionUp_CheckEvasionStage
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_DRY_SKIN, Expert_StatusEvasionUp_TryScorePlus1_2
    GoTo Expert_StatusEvasionUp_CheckEvasionStage

Expert_StatusEvasionUp_CheckTargetPoisonHeal:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_POISON_HEAL, Expert_StatusEvasionUp_CheckEvasionStage
    GoTo Expert_StatusEvasionUp_TryScorePlus1_2

Expert_StatusEvasionUp_TryScorePlus1_2:
    IfRandomLessThan 50, Expert_StatusEvasionUp_CheckEvasionStage
    AddToMoveScore 1
    GoTo Expert_StatusEvasionUp_CheckEvasionStage

Expert_StatusEvasionUp_CheckEvasionStage:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_EVASION_STAGE, 9, Expert_StatusEvasionUp_CheckBatonPass
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_StatusEvasionUp_CheckBatonPass
    IfRandomLessThan 50, Expert_StatusEvasionUp_CheckBatonPass
    AddToMoveScore -1

Expert_StatusEvasionUp_CheckBatonPass:
    IfMoveEffectNotKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_PASS_STATS_AND_STATUS, Expert_StatusEvasionUp_CheckHP
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_StatusEvasionUp_CheckHP
    IfRandomLessThan 50, Expert_StatusEvasionUp_CheckHP
    AddToMoveScore 1

Expert_StatusEvasionUp_CheckHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_StatusEvasionUp_End_2
    IfRandomLessThan 70, Expert_StatusEvasionUp_End_2

Expert_StatusEvasionUp_ScoreMinus2_2:
    AddToMoveScore -2

Expert_StatusEvasionUp_End_2:
    End

Expert_BypassAccuracyMove:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 10, Expert_BypassAccuracyMove_ScorePlus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 2, Expert_BypassAccuracyMove_ScorePlus1
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 8, Expert_BypassAccuracyMove_TryScorePlus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 4, Expert_BypassAccuracyMove_TryScorePlus1
    GoTo Expert_BypassAccuracyMove_End

Expert_BypassAccuracyMove_ScorePlus1:
    AddToMoveScore 1

Expert_BypassAccuracyMove_TryScorePlus1:
    IfRandomLessThan 100, Expert_BypassAccuracyMove_End
    AddToMoveScore 1

Expert_BypassAccuracyMove_End:
    End

Expert_StatusAttackDown:
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 6, Expert_StatusAttackDown_CheckTargetHP
    AddToMoveScore -1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 90, Expert_StatusAttackDown_CheckTargetStatStage
    AddToMoveScore -1

Expert_StatusAttackDown_CheckTargetStatStage:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 3, Expert_StatusAttackDown_CheckTargetHP
    IfRandomLessThan 50, Expert_StatusAttackDown_CheckTargetHP
    AddToMoveScore -2

Expert_StatusAttackDown_CheckTargetHP:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, Expert_StatusAttackDown_CheckLastUsedMove
    AddToMoveScore -2

Expert_StatusAttackDown_CheckLastUsedMove:
    LoadDefenderLastUsedMoveClass
    IfLoadedNotEqualTo MOVE_CATEGORY_SPECIAL, Expert_StatusAttackDown_End
    IfRandomLessThan 128, Expert_StatusAttackDown_End
    AddToMoveScore -2

Expert_StatusAttackDown_End:
    End

Expert_StatusAttackDown_PreSplitPhysicalTypes:
    TableEntry TYPE_NORMAL
    TableEntry TYPE_FIGHTING
    TableEntry TYPE_GROUND
    TableEntry TYPE_ROCK
    TableEntry TYPE_BUG
    TableEntry TYPE_STEEL
    TableEntry TABLE_END

Expert_StatusDefenseDown:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_StatusDefenseDown_TryScoreMinus2
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 3, Expert_StatusDefenseDown_CheckTargetHP

Expert_StatusDefenseDown_TryScoreMinus2:
    IfRandomLessThan 50, Expert_StatusDefenseDown_CheckTargetHP
    AddToMoveScore -2

Expert_StatusDefenseDown_CheckTargetHP:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, Expert_StatusDefenseDown_End
    AddToMoveScore -2

Expert_StatusDefenseDown_End:
    End

Expert_SpeedDownOnHit:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_SpeedDownOnHit_End
    IfMoveEqualTo MOVE_ICY_WIND, Expert_StatusSpeedDown
    IfMoveEqualTo MOVE_ROCK_TOMB, Expert_StatusSpeedDown
    IfMoveEqualTo MOVE_MUD_SHOT, Expert_StatusSpeedDown
    IfMoveEqualTo MOVE_LOW_SWEEP, Expert_StatusSpeedDown
    IfMoveEqualTo MOVE_ELECTROWEB, Expert_StatusSpeedDown
    IfMoveEqualTo MOVE_BULLDOZE, Expert_StatusSpeedDown
    IfMoveEqualTo MOVE_GLACIATE, Expert_StatusSpeedDown
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_SpeedDownOnHit_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_SpeedDownOnHit_End
    End

Expert_SpeedDownOnHit_End:
    End

Expert_StatusSpeedDown:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_StatusSpeedDown_TryScorePlus2
    AddToMoveScore -3
    GoTo Expert_StatusSpeedDown_End

Expert_StatusSpeedDown_TryScorePlus2:
    IfRandomLessThan 70, Expert_StatusSpeedDown_End
    AddToMoveScore 2

Expert_StatusSpeedDown_End:
    End

Expert_StatusSpAttackDown:
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 6, Expert_StatusSpAttackDown_CheckTargetHP
    AddToMoveScore -1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 90, Expert_StatusSpAttackDown_CheckTargetStatStage
    AddToMoveScore -1

Expert_StatusSpAttackDown_CheckTargetStatStage:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 3, Expert_StatusSpAttackDown_CheckTargetHP
    IfRandomLessThan 50, Expert_StatusSpAttackDown_CheckTargetHP
    AddToMoveScore -2

Expert_StatusSpAttackDown_CheckTargetHP:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, Expert_StatusSpAttackDown_CheckLastUsedMove
    AddToMoveScore -2

Expert_StatusSpAttackDown_CheckLastUsedMove:
    LoadDefenderLastUsedMoveClass
    IfLoadedNotEqualTo MOVE_CATEGORY_PHYSICAL, Expert_StatusSpAttackDown_End
    IfRandomLessThan 128, Expert_StatusSpAttackDown_End
    AddToMoveScore -2

Expert_StatusSpAttackDown_End:
    End

Expert_StatusSpAttackDown_PreSplitSpecialTypes:
    TableEntry TYPE_FIRE
    TableEntry TYPE_WATER
    TableEntry TYPE_GRASS
    TableEntry TYPE_ELECTRIC
    TableEntry TYPE_PSYCHIC
    TableEntry TYPE_ICE
    TableEntry TYPE_DRAGON
    TableEntry TYPE_DARK
    TableEntry TABLE_END

Expert_StatusSpDefenseDown:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_StatusSpDefenseDown_TryScoreMinus2
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 3, Expert_StatusSpDefenseDown_CheckTargetHP

Expert_StatusSpDefenseDown_TryScoreMinus2:
    IfRandomLessThan 50, Expert_StatusSpDefenseDown_CheckTargetHP
    AddToMoveScore -2

Expert_StatusSpDefenseDown_CheckTargetHP:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, Expert_StatusSpDefenseDown_End
    AddToMoveScore -2

Expert_StatusSpDefenseDown_End:
    End

Expert_StatusAccuracyDown:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_StatusAccuracyDown_TryScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, Expert_StatusAccuracyDown_CheckUserAccuracy

Expert_StatusAccuracyDown_TryScoreMinus1:
    IfRandomLessThan 100, Expert_StatusAccuracyDown_CheckUserAccuracy
    AddToMoveScore -1

Expert_StatusAccuracyDown_CheckUserAccuracy:
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 4, Expert_StatusAccuracyDown_CheckTargetBadlyPoisoned
    IfRandomLessThan 80, Expert_StatusAccuracyDown_CheckTargetBadlyPoisoned
    AddToMoveScore -2

Expert_StatusAccuracyDown_CheckTargetBadlyPoisoned:
    IfNotBadlyPoisoned AI_BATTLER_DEFENDER, Expert_StatusAccuracyDown_CheckTargetSeeded
    IfRandomLessThan 70, Expert_StatusAccuracyDown_CheckTargetSeeded
    AddToMoveScore 2

Expert_StatusAccuracyDown_CheckTargetSeeded:
    IfNotCondition AI_BATTLER_DEFENDER, CONDITION_LEECH_SEED, Expert_StatusAccuracyDown_CheckUserIngrained
    IfRandomLessThan 70, Expert_StatusAccuracyDown_CheckUserIngrained
    AddToMoveScore 2

Expert_StatusAccuracyDown_CheckUserIngrained:
    IfNotCondition AI_BATTLER_ATTACKER, CONDITION_INGRAIN, Expert_StatusAccuracyDown_CheckUserAquaRing
    IfRandomLessThan 128, Expert_StatusAccuracyDown_CheckTargetCursed
    AddToMoveScore 1
    GoTo Expert_StatusAccuracyDown_CheckTargetCursed

Expert_StatusAccuracyDown_CheckUserAquaRing:
    IfNotCondition AI_BATTLER_ATTACKER, 35, Expert_StatusAccuracyDown_CheckTargetCursed
    IfRandomLessThan 128, Expert_StatusAccuracyDown_CheckTargetCursed
    AddToMoveScore 1

Expert_StatusAccuracyDown_CheckTargetCursed:
    IfNotCondition AI_BATTLER_DEFENDER, CONDITION_CURSE, Expert_StatusAccuracyDown_CheckHPRanges
    IfRandomLessThan 70, Expert_StatusAccuracyDown_CheckHPRanges
    AddToMoveScore 2

Expert_StatusAccuracyDown_CheckHPRanges:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_StatusAccuracyDown_End
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_ACCURACY_STAGE, 6, Expert_StatusAccuracyDown_End
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_StatusAccuracyDown_ScoreMinus2
    IfHPPercentLessThan AI_BATTLER_DEFENDER, 40, Expert_StatusAccuracyDown_ScoreMinus2
    IfRandomLessThan 70, Expert_StatusAccuracyDown_End

Expert_StatusAccuracyDown_ScoreMinus2:
    AddToMoveScore -2

Expert_StatusAccuracyDown_End:
    End

Expert_StatusEvasionDown:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_StatusEvasionDown_TryScoreMinus2
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 3, Expert_StatusEvasionDown_CheckTargetHP

Expert_StatusEvasionDown_TryScoreMinus2:
    IfRandomLessThan 50, Expert_StatusEvasionDown_CheckTargetHP
    AddToMoveScore -2

Expert_StatusEvasionDown_CheckTargetHP:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, Expert_StatusEvasionDown_End
    AddToMoveScore -2

Expert_StatusEvasionDown_End:
    End

Expert_Haze:
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 8, Expert_Haze_TryScoreMinus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 8, Expert_Haze_TryScoreMinus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 8, Expert_Haze_TryScoreMinus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 8, Expert_Haze_TryScoreMinus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_EVASION_STAGE, 8, Expert_Haze_TryScoreMinus3
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 4, Expert_Haze_TryScoreMinus3
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 4, Expert_Haze_TryScoreMinus3
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 4, Expert_Haze_TryScoreMinus3
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 4, Expert_Haze_TryScoreMinus3
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_ACCURACY_STAGE, 4, Expert_Haze_TryScoreMinus3
    GoTo Expert_Haze_CheckToEncourage

Expert_Haze_TryScoreMinus3:
    IfRandomLessThan 50, Expert_Haze_CheckToEncourage
    AddToMoveScore -3

Expert_Haze_CheckToEncourage:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 8, Expert_Haze_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 8, Expert_Haze_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 8, Expert_Haze_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 8, Expert_Haze_TryScorePlus3
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 8, Expert_Haze_TryScorePlus3
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 4, Expert_Haze_TryScorePlus3
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 4, Expert_Haze_TryScorePlus3
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 4, Expert_Haze_TryScorePlus3
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 4, Expert_Haze_TryScorePlus3
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 4, Expert_Haze_TryScorePlus3
    IfRandomLessThan 50, Expert_Haze_End
    AddToMoveScore -1
    GoTo Expert_Haze_End

Expert_Haze_TryScorePlus3:
    IfRandomLessThan 50, Expert_Haze_End
    AddToMoveScore 3

Expert_Haze_End:
    End

Expert_Bide:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 90, Expert_Bide_End
    AddToMoveScore -2

Expert_Bide_End:
    End

Expert_ForceSwitch:
    LoadBattlerUnk60 AI_BATTLER_DEFENDER
    IfLoadedGreaterThan 3, Expert_ForceSwitch_75PercentScorePlus2
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SPIKES, Expert_ForceSwitch_50PercentScorePlus2
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_STEALTH_ROCK, Expert_ForceSwitch_50PercentScorePlus2
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_TOXIC_SPIKES, Expert_ForceSwitch_50PercentScorePlus2
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 8, Expert_ForceSwitch_50PercentScorePlus2
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 8, Expert_ForceSwitch_50PercentScorePlus2
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 8, Expert_ForceSwitch_50PercentScorePlus2
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 8, Expert_ForceSwitch_50PercentScorePlus2
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 8, Expert_ForceSwitch_50PercentScorePlus2
    AddToMoveScore -3
    GoTo Expert_ForceSwitch_End

Expert_ForceSwitch_75PercentScorePlus2:
    IfRandomLessThan 64, Expert_ForceSwitch_50PercentScorePlus2
    AddToMoveScore 2

Expert_ForceSwitch_50PercentScorePlus2:
    IfRandomLessThan 128, Expert_ForceSwitch_End
    AddToMoveScore 2

Expert_ForceSwitch_End:
    End

Expert_Conversion:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 90, Expert_Conversion_CheckTurnCount
    AddToMoveScore -2

Expert_Conversion_CheckTurnCount:
    LoadTurnCount
    IfLoadedEqualTo 0, Expert_Conversion_End
    IfRandomLessThan 200, ScoreMinus2

Expert_Conversion_End:
    End

Expert_Synthesis:
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_HAIL, Expert_Synthesis_ScoreMinus2
    IfLoadedEqualTo BTL_WEATHER_RAIN, Expert_Synthesis_ScoreMinus2
    IfLoadedEqualTo BTL_WEATHER_SANDSTORM, Expert_Synthesis_ScoreMinus2
    GoTo Expert_Recovery

Expert_Synthesis_ScoreMinus2:
    AddToMoveScore -2

Expert_Recovery:
    IfHPPercentEqualTo AI_BATTLER_ATTACKER, 100, Expert_Recovery_ScoreMinus3AndEnd
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Recovery_CheckHP
    AddToMoveScore -8
    GoTo Expert_Recovery_End
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 50, Expert_Recovery_CheckForSnatch
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 80, Expert_Recovery_ScoreMinus3AndEnd
    IfRandomLessThan 70, Expert_Recovery_CheckForSnatch

Expert_Recovery_ScoreMinus3AndEnd:
    AddToMoveScore -3
    GoTo Expert_Recovery_End

Expert_Recovery_CheckHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_Recovery_CheckForSnatch
    IfRandomLessThan 30, Expert_Recovery_CheckForSnatch
    AddToMoveScore -3
    GoTo Expert_Recovery_End

Expert_Recovery_CheckForSnatch:
    IfMoveEffectNotKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_STEAL_STATUS_MOVE, Expert_Recovery_TryScorePlus2
    IfRandomLessThan 100, Expert_Recovery_End

Expert_Recovery_TryScorePlus2:
    IfRandomLessThan 20, Expert_Recovery_End
    AddToMoveScore 2

Expert_Recovery_End:
    End

Expert_ToxicLeechSeed:
    IfAttackerHasNoDamagingMoves Expert_ToxicLeechSeed_CheckMoveEffectsKnown
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_ToxicLeechSeed_CheckTargetHP
    IfRandomLessThan 50, Expert_ToxicLeechSeed_CheckTargetHP
    AddToMoveScore -3

Expert_ToxicLeechSeed_CheckTargetHP:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 50, Expert_ToxicLeechSeed_CheckMoveEffectsKnown
    IfRandomLessThan 50, Expert_ToxicLeechSeed_CheckMoveEffectsKnown
    AddToMoveScore -3

Expert_ToxicLeechSeed_CheckMoveEffectsKnown:
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_SP_DEF_UP, Expert_ToxicLeechSeed_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_PROTECT, Expert_ToxicLeechSeed_TryScorePlus2
    GoTo Expert_ToxicLeechSeed_End

Expert_ToxicLeechSeed_TryScorePlus2:
    IfRandomLessThan 60, Expert_ToxicLeechSeed_End
    AddToMoveScore 2

Expert_ToxicLeechSeed_End:
    End

Expert_LightScreen:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 50, Expert_LightScreen_ScoreMinus2
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 90, Expert_LightScreen_CheckLastUsedMove
    IfRandomLessThan 30, Expert_LightScreen_CheckLastUsedMove
    AddToMoveScore 1

Expert_LightScreen_CheckLastUsedMove:
    IfAttackGreaterThanSpAttack AI_BATTLER_DEFENDER, Expert_LightScreen_ScoreMinus2
    IfRandomLessThan 64, Expert_LightScreen_End
    AddToMoveScore 1
    GoTo Expert_LightScreen_End

Expert_LightScreen_ScoreMinus2:
    IfRandomLessThan 30, Expert_LightScreen_End
    AddToMoveScore -2

Expert_LightScreen_End:
    End

Expert_Rest:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Rest_SlowerCheckHP
    IfHPPercentNotEqualTo AI_BATTLER_ATTACKER, 100, Expert_Rest_FasterCheckHP
    AddToMoveScore -8
    GoTo Expert_Rest_End

Expert_Rest_FasterCheckHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_Rest_CheckForSnatch
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_Rest_FasterScoreMinus3
    IfRandomLessThan 70, Expert_Rest_CheckForSnatch

Expert_Rest_FasterScoreMinus3:
    AddToMoveScore -3
    GoTo Expert_Rest_End

Expert_Rest_SlowerCheckHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 60, Expert_Rest_CheckForSnatch
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_Rest_SlowerScoreMinus3
    IfRandomLessThan 50, Expert_Rest_CheckForSnatch

Expert_Rest_SlowerScoreMinus3:
    AddToMoveScore -3
    GoTo Expert_Rest_End

Expert_Rest_CheckForSnatch:
    IfMoveEffectNotKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_STEAL_STATUS_MOVE, Expert_Rest_TryScorePlus3
    IfRandomLessThan 50, Expert_Rest_End

Expert_Rest_TryScorePlus3:
    IfRandomLessThan 10, Expert_Rest_End
    AddToMoveScore 3

Expert_Rest_End:
    End

Expert_OHKOMove:
    IfRandomLessThan 192, Expert_OHKOMove_End
    AddToMoveScore 1

Expert_OHKOMove_End:
    End

Expert_SuperFang:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 60, Expert_SuperFang_End
    AddToMoveScore -1

Expert_SuperFang_End:
    End

Expert_BindingMove:
    IfBadlyPoisoned AI_BATTLER_DEFENDER, Expert_BindingMove_TryScorePlus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CURSE, Expert_BindingMove_TryScorePlus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_PERISH_SONG, Expert_BindingMove_TryScorePlus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_ATTRACT, Expert_BindingMove_TryScorePlus1
    GoTo Expert_BindingMove_End

Expert_BindingMove_TryScorePlus1:
    IfRandomLessThan 128, Expert_BindingMove_End
    AddToMoveScore 1

Expert_BindingMove_End:
    End

Expert_HighCritical:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_HighCritical_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_HighCritical_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_HighCritical_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_DOUBLE, Expert_HighCritical_TryScorePlus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUADRUPLE, Expert_HighCritical_TryScorePlus1
    IfRandomLessThan 128, Expert_HighCritical_End

Expert_HighCritical_TryScorePlus1:
    IfRandomLessThan 128, Expert_HighCritical_End
    AddToMoveScore 1

Expert_HighCritical_End:
    End

Expert_Swagger:
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_PSYCH_UP, Expert_Swagger_PsychUp

Expert_Flatter:
    IfRandomLessThan 128, Expert_StatusConfuse
    AddToMoveScore 1

Expert_StatusConfuse:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, Expert_StatusConfuse_End
    IfRandomLessThan 128, Expert_StatusConfuse_CheckHP
    AddToMoveScore -1

Expert_StatusConfuse_CheckHP:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 50, Expert_StatusConfuse_End
    AddToMoveScore -1
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 30, Expert_StatusConfuse_End
    AddToMoveScore -1

Expert_StatusConfuse_End:
    End

Expert_Swagger_PsychUp:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 3, Expert_Swagger_ScoreMinus5
    AddToMoveScore 3
    LoadTurnCount
    IfLoadedNotEqualTo 0, Expert_Swagger_End
    AddToMoveScore 2
    GoTo Expert_Swagger_End

Expert_Swagger_ScoreMinus5:
    AddToMoveScore -5

Expert_Swagger_End:
    End

Expert_Reflect:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 50, Expert_Reflect_ScoreMinus2
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 90, Expert_Reflect_CheckLastUsedMove
    IfRandomLessThan 30, Expert_Reflect_CheckLastUsedMove
    AddToMoveScore 1

Expert_Reflect_CheckLastUsedMove:
    IfAttackLessThanSpAttack AI_BATTLER_DEFENDER, Expert_Reflect_ScoreMinus2
    IfRandomLessThan 64, Expert_Reflect_End
    AddToMoveScore 1
    GoTo Expert_Reflect_End

Expert_Reflect_ScoreMinus2:
    IfRandomLessThan 30, Expert_LightScreen_End
    AddToMoveScore -2

Expert_Reflect_End:
    End

Expert_StatusPoison:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 50, Expert_StatusPoison_ScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 50, Expert_StatusPoison_End

Expert_StatusPoison_ScoreMinus1:
    AddToMoveScore -1

Expert_StatusPoison_End:
    End

Expert_StatusParalyze:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_StatusParalyze_TryScorePlus3
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_StatusParalyze_End
    AddToMoveScore -1
    GoTo Expert_StatusParalyze_End

Expert_StatusParalyze_TryScorePlus3:
    IfRandomLessThan 20, Expert_StatusParalyze_End
    AddToMoveScore 3

Expert_StatusParalyze_End:
    End

Expert_VitalThrow:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_VitalThrow_End
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 60, Expert_VitalThrow_End
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_VitalThrow_TryScoreMinus1
    IfRandomLessThan 180, Expert_VitalThrow_End

Expert_VitalThrow_TryScoreMinus1:
    IfRandomLessThan 50, Expert_VitalThrow_End
    AddToMoveScore -1

Expert_VitalThrow_End:
    End

Expert_Substitute:
    IfMoveNotKnown AI_BATTLER_ATTACKER, MOVE_FOCUS_PUNCH, Expert_Substitute_CheckUserHP
    IfRandomLessThan 96, Expert_Substitute_CheckUserHP
    AddToMoveScore 1

Expert_Substitute_CheckUserHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 90, Expert_Substitute_CheckTargetLastMove
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_Substitute_TryScoreMinus1_FinalRound
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_Substitute_TryScoreMinus1_SecondRound
    IfRandomLessThan 100, Expert_Substitute_TryScoreMinus1_SecondRound
    AddToMoveScore -1

Expert_Substitute_TryScoreMinus1_SecondRound:
    IfRandomLessThan 100, Expert_Substitute_TryScoreMinus1_FinalRound
    AddToMoveScore -1

Expert_Substitute_TryScoreMinus1_FinalRound:
    IfRandomLessThan 100, Expert_Substitute_CheckTargetLastMove
    AddToMoveScore -1

Expert_Substitute_CheckTargetLastMove:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Substitute_End
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadEffectOfLoadedMove
    IfLoadedEqualTo BATTLE_EFFECT_STATUS_SLEEP, Expert_Substitute_CheckTargetStatus
    IfLoadedEqualTo BATTLE_EFFECT_STATUS_BADLY_POISON, Expert_Substitute_CheckTargetStatus
    IfLoadedEqualTo BATTLE_EFFECT_STATUS_POISON, Expert_Substitute_CheckTargetStatus
    IfLoadedEqualTo BATTLE_EFFECT_STATUS_PARALYZE, Expert_Substitute_CheckTargetStatus
    IfLoadedEqualTo BATTLE_EFFECT_STATUS_BURN, Expert_Substitute_CheckTargetStatus
    IfLoadedEqualTo BATTLE_EFFECT_STATUS_CONFUSE, Expert_Substitute_CheckTargetConfused
    IfLoadedEqualTo BATTLE_EFFECT_STATUS_LEECH_SEED, Expert_Substitute_CheckTargetSeeded
    GoTo Expert_Substitute_End

Expert_Substitute_CheckTargetStatus:
    IfNotStatus AI_BATTLER_DEFENDER, Expert_Substitute_TryScorePlus1
    GoTo Expert_Substitute_End

Expert_Substitute_CheckTargetConfused:
    IfNotCondition AI_BATTLER_DEFENDER, CONDITION_CONFUSION, Expert_Substitute_TryScorePlus1
    GoTo Expert_Substitute_End

Expert_Substitute_CheckTargetSeeded:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_LEECH_SEED, Expert_Substitute_End

Expert_Substitute_TryScorePlus1:
    IfRandomLessThan 100, Expert_Substitute_End
    AddToMoveScore 1

Expert_Substitute_End:
    End

Expert_RechargeTurn:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_RechargeTurn_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_RechargeTurn_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_RechargeTurn_ScoreMinus1
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_TRUANT, Expert_RechargeTurn_TryScorePlus1
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_RechargeTurn_CheckUserHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 40, Expert_RechargeTurn_ScoreMinus1
    GoTo Expert_RechargeTurn_End

Expert_RechargeTurn_TryScorePlus1:
    IfRandomLessThan 80, Expert_RechargeTurn_End
    AddToMoveScore 1
    GoTo Expert_RechargeTurn_End

Expert_RechargeTurn_CheckUserHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 60, Expert_RechargeTurn_End

Expert_RechargeTurn_ScoreMinus1:
    AddToMoveScore -1

Expert_RechargeTurn_End:
    End

Expert_Disable:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Disable_End
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadPowerOfLoadedMove
    IfLoadedEqualTo 0, Expert_Disable_TryScoreMinus1
    AddToMoveScore 1
    GoTo Expert_Disable_End

Expert_Disable_TryScoreMinus1:
    IfRandomLessThan 100, Expert_Disable_End
    AddToMoveScore -1

Expert_Disable_End:
    End

Expert_Counter:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, Expert_Counter_ScoreMinus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_ATTRACT, Expert_Counter_ScoreMinus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CONFUSION, Expert_Counter_ScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 30, Expert_Counter_CheckAboveHalfHP
    IfRandomLessThan 10, Expert_Counter_CheckAboveHalfHP
    AddToMoveScore -1

Expert_Counter_CheckAboveHalfHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_Counter_CheckLastUsedMove
    IfRandomLessThan 100, Expert_Counter_CheckLastUsedMove
    AddToMoveScore -1

Expert_Counter_CheckLastUsedMove:
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_MIRROR_COAT, Expert_Counter_TryScorePlus4
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadPowerOfLoadedMove
    IfLoadedEqualTo 0, Expert_Counter_TryScorePlus1
    IfTargetIsNotTaunted Expert_Counter_CheckPhysicalMove
    IfRandomLessThan 100, Expert_Counter_CheckPhysicalMove
    AddToMoveScore 1

Expert_Counter_CheckPhysicalMove:
    LoadDefenderLastUsedMoveClass
    IfLoadedNotEqualTo MOVE_CATEGORY_PHYSICAL, Expert_Counter_ScoreMinus1
    IfRandomLessThan 100, Expert_Counter_End2
    AddToMoveScore 1
    GoTo Expert_Counter_End2

Expert_Counter_TryScorePlus1:
    IfTargetIsNotTaunted Expert_Counter_CheckOpponentTypes
    IfRandomLessThan 100, Expert_Counter_CheckOpponentTypes
    AddToMoveScore 1

Expert_Counter_CheckOpponentTypes:
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedInTable Expert_Counter_PhysicalTypes, Expert_Counter_End2
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedInTable Expert_Counter_PhysicalTypes, Expert_Counter_End2
    IfRandomLessThan 50, Expert_Counter_End2

Expert_Counter_TryScorePlus4:
    IfRandomLessThan 100, Expert_Counter_End
    AddToMoveScore 4

Expert_Counter_End:
    End

Expert_Counter_ScoreMinus1:
    AddToMoveScore -1

Expert_Counter_End2:
    End

Expert_Counter_PhysicalTypes:
    TableEntry TYPE_NORMAL
    TableEntry TYPE_FIGHTING
    TableEntry TYPE_FLYING
    TableEntry TYPE_POISON
    TableEntry TYPE_GROUND
    TableEntry TYPE_ROCK
    TableEntry TYPE_BUG
    TableEntry TYPE_GHOST
    TableEntry TYPE_STEEL
    TableEntry TABLE_END

Expert_Encore:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_DISABLE, Expert_Encore_TryScorePlus3
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Encore_ScoreMinus2
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadEffectOfLoadedMove
    IfLoadedNotInTable Expert_Encore_EncouragedMoveEffects, Expert_Encore_ScoreMinus2

Expert_Encore_TryScorePlus3:
    IfRandomLessThan 30, Expert_Encore_End
    AddToMoveScore 3
    GoTo Expert_Encore_End

Expert_Encore_ScoreMinus2:
    AddToMoveScore -2

Expert_Encore_End:
    End

Expert_Encore_EncouragedMoveEffects:
    TableEntry BATTLE_EFFECT_RECOVER_DAMAGE_SLEEP
    TableEntry BATTLE_EFFECT_ATK_UP
    TableEntry BATTLE_EFFECT_DEF_UP
    TableEntry BATTLE_EFFECT_SPEED_UP
    TableEntry BATTLE_EFFECT_SP_ATK_UP
    TableEntry BATTLE_EFFECT_RESET_STAT_CHANGES
    TableEntry BATTLE_EFFECT_FORCE_SWITCH
    TableEntry BATTLE_EFFECT_CONVERSION
    TableEntry BATTLE_EFFECT_STATUS_BADLY_POISON
    TableEntry BATTLE_EFFECT_SET_LIGHT_SCREEN
    TableEntry BATTLE_EFFECT_REST
    TableEntry BATTLE_EFFECT_HALVE_HP
    TableEntry BATTLE_EFFECT_SP_DEF_UP_2
    TableEntry BATTLE_EFFECT_STATUS_CONFUSE
    TableEntry BATTLE_EFFECT_STATUS_POISON
    TableEntry BATTLE_EFFECT_STATUS_PARALYZE
    TableEntry BATTLE_EFFECT_STATUS_LEECH_SEED
    TableEntry BATTLE_EFFECT_DO_NOTHING
    TableEntry BATTLE_EFFECT_ATK_UP_2
    TableEntry BATTLE_EFFECT_ENCORE
    TableEntry BATTLE_EFFECT_CONVERSION2
    TableEntry BATTLE_EFFECT_NEXT_ATTACK_ALWAYS_HITS
    TableEntry BATTLE_EFFECT_CURE_PARTY_STATUS
    TableEntry BATTLE_EFFECT_PREVENT_ESCAPE
    TableEntry BATTLE_EFFECT_STATUS_NIGHTMARE
    TableEntry BATTLE_EFFECT_PROTECT
    TableEntry BATTLE_EFFECT_SWITCH_ABILITIES
    TableEntry BATTLE_EFFECT_FORESIGHT
    TableEntry BATTLE_EFFECT_ALL_FAINT_3_TURNS
    TableEntry BATTLE_EFFECT_WEATHER_SANDSTORM
    TableEntry BATTLE_EFFECT_SURVIVE_WITH_1_HP
    TableEntry BATTLE_EFFECT_ATK_UP_2_STATUS_CONFUSION
    TableEntry BATTLE_EFFECT_INFATUATE
    TableEntry BATTLE_EFFECT_PREVENT_STATUS
    TableEntry BATTLE_EFFECT_WEATHER_RAIN
    TableEntry BATTLE_EFFECT_WEATHER_SUN
    TableEntry BATTLE_EFFECT_MAX_ATK_LOSE_HALF_MAX_HP
    TableEntry BATTLE_EFFECT_COPY_STAT_CHANGES
    TableEntry BATTLE_EFFECT_HIT_IN_3_TURNS
    TableEntry BATTLE_EFFECT_ALWAYS_FLINCH_FIRST_TURN_ONLY
    TableEntry BATTLE_EFFECT_STOCKPILE
    TableEntry BATTLE_EFFECT_SPIT_UP
    TableEntry BATTLE_EFFECT_SWALLOW
    TableEntry BATTLE_EFFECT_WEATHER_HAIL
    TableEntry BATTLE_EFFECT_TORMENT
    TableEntry BATTLE_EFFECT_STATUS_BURN
    TableEntry BATTLE_EFFECT_MAKE_GLOBAL_TARGET
    TableEntry BATTLE_EFFECT_SP_DEF_UP_DOUBLE_ELECTRIC_POWER
    TableEntry BATTLE_EFFECT_SWITCH_HELD_ITEMS
    TableEntry BATTLE_EFFECT_COPY_ABILITY
    TableEntry BATTLE_EFFECT_GROUND_TRAP_USER_CONTINUOUS_HEAL
    TableEntry BATTLE_EFFECT_RECYCLE
    TableEntry BATTLE_EFFECT_REMOVE_HELD_ITEM
    TableEntry BATTLE_EFFECT_SWITCH_ABILITIES
    TableEntry BATTLE_EFFECT_MAKE_SHARED_MOVES_UNUSEABLE
    TableEntry BATTLE_EFFECT_HEAL_STATUS
    TableEntry BATTLE_EFFECT_REMOVE_ALL_PP_ON_DEFEAT
    TableEntry BATTLE_EFFECT_CONFUSE_ALL
    TableEntry BATTLE_EFFECT_HALVE_ELECTRIC_DAMAGE
    TableEntry BATTLE_EFFECT_HALVE_FIRE_DAMAGE
    TableEntry BATTLE_EFFECT_ATK_SPD_UP
    TableEntry BATTLE_EFFECT_CAMOUFLAGE
    TableEntry BATTLE_EFFECT_GRAVITY
    TableEntry BATTLE_EFFECT_IGNORE_EVATION_REMOVE_DARK_IMMUNE
    TableEntry BATTLE_EFFECT_FAINT_AND_FULL_HEAL_NEXT_MON
    TableEntry BATTLE_EFFECT_NATURAL_GIFT
    TableEntry BATTLE_EFFECT_REMOVE_PROTECT
    TableEntry BATTLE_EFFECT_DOUBLE_SPEED_3_TURNS
    TableEntry BATTLE_EFFECT_RANDOM_STAT_UP_2
    TableEntry BATTLE_EFFECT_FLING
    TableEntry BATTLE_EFFECT_TRANSFER_STATUS
    TableEntry BATTLE_EFFECT_PREVENT_HEALING
    TableEntry BATTLE_EFFECT_SWAP_ATK_DEF
    TableEntry BATTLE_EFFECT_SUPRESS_ABILITY
    TableEntry BATTLE_EFFECT_PREVENT_CRITS
    TableEntry BATTLE_EFFECT_SWAP_ATK_SP_ATK_STAT_CHANGES
    TableEntry BATTLE_EFFECT_SWAP_DEF_SP_DEF_STAT_CHANGES
    TableEntry BATTLE_EFFECT_SET_ABILITY_TO_INSOMNIA
    TableEntry BATTLE_EFFECT_SWAP_STAT_CHANGES
    TableEntry BATTLE_EFFECT_RESTORE_HP_EVERY_TURN
    TableEntry BATTLE_EFFECT_GIVE_GROUND_IMMUNITY
    TableEntry BATTLE_EFFECT_TRICK_ROOM
    TableEntry TABLE_END

Expert_PainSplit:
    IfHPPercentLessThan AI_BATTLER_DEFENDER, 80, Expert_PainSplit_ScoreMinus1
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_PainSplit_CheckUserHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 40, Expert_PainSplit_ScoreMinus1
    AddToMoveScore 1
    GoTo Expert_PainSplit_End

Expert_PainSplit_CheckUserHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 60, Expert_PainSplit_ScoreMinus1
    AddToMoveScore 1
    GoTo Expert_PainSplit_End

Expert_PainSplit_ScoreMinus1:
    AddToMoveScore -1

Expert_PainSplit_End:
    End

Expert_Nightmare:
    AddToMoveScore 2
    End

Expert_LockOn:
    IfRandomLessThan 128, Expert_LockOn_End
    AddToMoveScore 2

Expert_LockOn_End:
    End

Expert_SleepTalk:
    IfCondition AI_BATTLER_ATTACKER, CONDITION_SLEEP, ScorePlus10
    AddToMoveScore -5
    End

Expert_DestinyBond:
    AddToMoveScore -1
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_DestinyBond_End
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_DestinyBond_End
    IfRandomLessThan 128, Expert_DestinyBond_CheckUserMediumHP
    AddToMoveScore 1

Expert_DestinyBond_CheckUserMediumHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_DestinyBond_End
    IfRandomLessThan 128, Expert_DestinyBond_CheckUserLowHP
    AddToMoveScore 1

Expert_DestinyBond_CheckUserLowHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 30, Expert_DestinyBond_End
    IfRandomLessThan 100, Expert_DestinyBond_End
    AddToMoveScore 2

Expert_DestinyBond_End:
    End

Expert_Reversal:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Reversal_SlowerCheckHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 33, Expert_Reversal_ScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 20, Expert_Reversal_End
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 8, Expert_Reversal_ScorePlus1
    GoTo Expert_Reversal_TryScorePlus1

Expert_Reversal_SlowerCheckHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 60, Expert_Reversal_ScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 40, Expert_Reversal_End
    GoTo Expert_Reversal_TryScorePlus1

Expert_Reversal_ScorePlus1:
    AddToMoveScore 1

Expert_Reversal_TryScorePlus1:
    IfRandomLessThan 100, Expert_Reversal_End
    AddToMoveScore 1
    GoTo Expert_Reversal_End

Expert_Reversal_ScoreMinus1:
    AddToMoveScore -1

Expert_Reversal_End:
    End

Expert_HealBell:
    IfStatus AI_BATTLER_ATTACKER, Expert_HealBell_End
    IfPartyMemberNotStatus AI_BATTLER_ATTACKER, Expert_HealBell_End
    AddToMoveScore -5

Expert_HealBell_End:
    End

Expert_Thief:
    LoadHeldItemEffect AI_BATTLER_DEFENDER
    IfLoadedNotInTable Expert_Thief_EncouragedItemEffects, Expert_Thief_ScoreMinus2
    IfRandomLessThan 50, Expert_Thief_End
    AddToMoveScore 1
    GoTo Expert_Thief_End

Expert_Thief_ScoreMinus2:
    AddToMoveScore -2

Expert_Thief_End:
    End

Expert_Thief_EncouragedItemEffects:
    TableEntry HOLD_EFFECT_SLP_RESTORE
    TableEntry HOLD_EFFECT_STATUS_RESTORE
    TableEntry HOLD_EFFECT_HP_RESTORE
    TableEntry HOLD_EFFECT_ACC_REDUCE
    TableEntry HOLD_EFFECT_HP_RESTORE_GRADUAL
    TableEntry HOLD_EFFECT_PIKA_SPATK_UP
    TableEntry HOLD_EFFECT_CUBONE_ATK_UP
    TableEntry HOLD_EFFECT_WEAKEN_SE_FIRE
    TableEntry HOLD_EFFECT_WEAKEN_SE_WATER
    TableEntry HOLD_EFFECT_WEAKEN_SE_ELECTRIC
    TableEntry HOLD_EFFECT_WEAKEN_SE_GRASS
    TableEntry HOLD_EFFECT_WEAKEN_SE_ICE
    TableEntry HOLD_EFFECT_WEAKEN_SE_FIGHT
    TableEntry HOLD_EFFECT_WEAKEN_SE_POISON
    TableEntry HOLD_EFFECT_WEAKEN_SE_GROUND
    TableEntry HOLD_EFFECT_WEAKEN_SE_FLYING
    TableEntry HOLD_EFFECT_WEAKEN_SE_PSYCHIC
    TableEntry HOLD_EFFECT_WEAKEN_SE_BUG
    TableEntry HOLD_EFFECT_WEAKEN_SE_ROCK
    TableEntry HOLD_EFFECT_WEAKEN_SE_GHOST
    TableEntry HOLD_EFFECT_WEAKEN_SE_DRAGON
    TableEntry HOLD_EFFECT_WEAKEN_SE_DARK
    TableEntry HOLD_EFFECT_WEAKEN_SE_STEEL
    TableEntry HOLD_EFFECT_WEAKEN_NORMAL
    TableEntry HOLD_EFFECT_HP_RESTORE_PSN_TYPE
    TableEntry TABLE_END

Expert_Curse:
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_GHOST, Expert_Curse_GhostCheckHP
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_GHOST, Expert_Curse_GhostCheckHP
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 9, Expert_Curse_End
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_GYRO_BALL, Expert_Curse_HighChanceScorePlus1
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_TRICK_ROOM, Expert_Curse_HighChanceScorePlus1
    GoTo Expert_Curse_FlipCoinScorePlus1

Expert_Curse_HighChanceScorePlus1:
    IfRandomLessThan 32, Expert_Curse_CheckDefenseStage
    AddToMoveScore 1

Expert_Curse_FlipCoinScorePlus1:
    IfRandomLessThan 128, Expert_Curse_CheckDefenseStage
    AddToMoveScore 1

Expert_Curse_CheckDefenseStage:
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 7, Expert_Curse_End
    IfRandomLessThan 128, Expert_Curse_CheckDefenseStageAnyBoosts
    AddToMoveScore 1

Expert_Curse_CheckDefenseStageAnyBoosts:
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 6, Expert_Curse_End
    IfRandomLessThan 128, Expert_Curse_End
    AddToMoveScore 1
    GoTo Expert_Curse_End

Expert_Curse_GhostCheckHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 80, Expert_Curse_End
    AddToMoveScore -1

Expert_Curse_End:
    End

Expert_Protect:
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_FEINT, Expert_Protect_TryScoreMinus2
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_SHADOW_FORCE, Expert_Protect_TryScoreMinus2
    GoTo Expert_Protect_CheckStatusConditions

Expert_Protect_TryScoreMinus2:
    IfTurnRandomLessThan 128, Expert_Protect_CheckStatusConditions
    AddToMoveScore -2

Expert_Protect_CheckStatusConditions:
    LoadProtectChain AI_BATTLER_ATTACKER
    IfLoadedGreaterThan 1, Expert_Protect_ScoreMinus2
    IfBadlyPoisoned AI_BATTLER_ATTACKER, Expert_Protect_CheckAttackerLockedOnto
    IfCondition AI_BATTLER_ATTACKER, CONDITION_CURSE, Expert_Protect_CheckAttackerLockedOnto
    IfCondition AI_BATTLER_ATTACKER, CONDITION_PERISH_SONG, Expert_Protect_CheckAttackerLockedOnto
    IfCondition AI_BATTLER_ATTACKER, CONDITION_ATTRACT, Expert_Protect_CheckAttackerLockedOnto
    IfCondition AI_BATTLER_ATTACKER, CONDITION_LEECH_SEED, Expert_Protect_CheckAttackerLockedOnto
    IfCondition AI_BATTLER_ATTACKER, CONDITION_YAWN, Expert_Protect_CheckAttackerLockedOnto
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CURSE, Expert_Protect_ScorePlus2
    IfCondition AI_BATTLER_DEFENDER, CONDITION_PERISH_SONG, Expert_Protect_ScorePlus2
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_RESTORE_HALF_HP, Expert_Protect_CheckAttackerLockedOnto
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_DEF_UP_DOUBLE_ROLLOUT_POWER, Expert_Protect_CheckAttackerLockedOnto
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HEAL_HALF_MORE_IN_SUN, Expert_Protect_CheckAttackerLockedOnto
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_SWALLOW, Expert_Protect_CheckAttackerLockedOnto
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HEAL_HALF_REMOVE_FLYING_TYPE, Expert_Protect_CheckAttackerLockedOnto
    IfBadlyPoisoned AI_BATTLER_DEFENDER, Expert_Protect_ScorePlus2
    IfCondition AI_BATTLER_DEFENDER, CONDITION_ATTRACT, Expert_Protect_ScorePlus2
    IfCondition AI_BATTLER_DEFENDER, CONDITION_LEECH_SEED, Expert_Protect_ScorePlus2
    IfCondition AI_BATTLER_DEFENDER, CONDITION_YAWN, Expert_Protect_ScorePlus2
    IfCondition AI_BATTLER_ATTACKER, 29, Expert_Protect_ScorePlus2
    IfRandomLessThan 85, Expert_Protect_ScorePlus2
    GoTo Expert_Protect_TryScoreMinus1

Expert_Protect_ScorePlus2:
    AddToMoveScore 2

Expert_Protect_TryScoreMinus1:
    IfTurnRandomLessThan 128, Expert_Protect_CheckEmptyChain
    AddToMoveScore -2

Expert_Protect_CheckEmptyChain:
    LoadProtectChain AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, Expert_Protect_End
    AddToMoveScore -1
    IfRandomLessThan 128, Expert_Protect_End
    AddToMoveScore -1
    GoTo Expert_Protect_End

Expert_Protect_CheckAttackerLockedOnto:
    IfCondition AI_BATTLER_ATTACKER, 29, Expert_Protect_End

Expert_Protect_ScoreMinus2:
    AddToMoveScore -2

Expert_Protect_End:
    End

Expert_Spikes:
    IfRandomLessThan 128, Expert_Spikes_End
    AddToMoveScore 1
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_ROAR, Expert_Spikes_TryScorePlus1
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_WHIRLWIND, Expert_Spikes_TryScorePlus1
    GoTo Expert_Spikes_End

Expert_Spikes_TryScorePlus1:
    IfRandomLessThan 64, Expert_Spikes_End
    AddToMoveScore 1

Expert_Spikes_End:
    End

// Bug: checks the attacker's type where Ghost-type targets were meant, as in Gen 4
Expert_Foresight:
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_GHOST, Expert_Foresight_FirstRoll
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_GHOST, Expert_Foresight_FirstRoll
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 8, Expert_Foresight_SecondRoll
    AddToMoveScore -2
    GoTo Expert_Foresight_End

Expert_Foresight_FirstRoll:
    IfRandomLessThan 80, Expert_Foresight_End

Expert_Foresight_SecondRoll:
    IfRandomLessThan 80, Expert_Foresight_End
    AddToMoveScore 2

Expert_Foresight_End:
    End

Expert_Endure:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 4, Expert_Endure_ScoreMinus1
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 35, Expert_Endure_TryScorePlus1

Expert_Endure_ScoreMinus1:
    AddToMoveScore -1
    GoTo Expert_Endure_End

Expert_Endure_TryScorePlus1:
    IfRandomLessThan 70, Expert_Endure_End
    AddToMoveScore 1

Expert_Endure_End:
    End

Expert_BatonPass:
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 8, Expert_BatonPass_HighStatStage_CheckSpeedAndHP
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 8, Expert_BatonPass_HighStatStage_CheckSpeedAndHP
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 8, Expert_BatonPass_HighStatStage_CheckSpeedAndHP
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 8, Expert_BatonPass_HighStatStage_CheckSpeedAndHP
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_EVASION_STAGE, 8, Expert_BatonPass_HighStatStage_CheckSpeedAndHP
    GoTo Expert_BatonPass_CheckMediumStatStage

Expert_BatonPass_HighStatStage_CheckSpeedAndHP:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_BatonPass_HighStatStage_SlowerCheckHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 60, Expert_BatonPass_End
    GoTo Expert_BatonPass_HighStatStage_TryScorePlus2

Expert_BatonPass_HighStatStage_SlowerCheckHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_BatonPass_End

Expert_BatonPass_HighStatStage_TryScorePlus2:
    IfRandomLessThan 80, Expert_BatonPass_End
    AddToMoveScore 2
    GoTo Expert_BatonPass_End

Expert_BatonPass_CheckMediumStatStage:
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 7, Expert_BatonPass_MediumStatStage_CheckSpeedAndHP
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 7, Expert_BatonPass_MediumStatStage_CheckSpeedAndHP
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 7, Expert_BatonPass_MediumStatStage_CheckSpeedAndHP
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 7, Expert_BatonPass_MediumStatStage_CheckSpeedAndHP
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_EVASION_STAGE, 7, Expert_BatonPass_MediumStatStage_CheckSpeedAndHP
    GoTo Expert_BatonPass_ScoreMinus2

Expert_BatonPass_MediumStatStage_CheckSpeedAndHP:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_BatonPass_MediumStatStage_SlowerCheckHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 60, Expert_BatonPass_ScoreMinus2
    GoTo Expert_BatonPass_End

Expert_BatonPass_MediumStatStage_SlowerCheckHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_BatonPass_End

Expert_BatonPass_ScoreMinus2:
    AddToMoveScore -2

Expert_BatonPass_End:
    End

Expert_Pursuit:
    LoadIsFirstTurnInBattle AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo FALSE, Expert_Pursuit_TryScorePlus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_GHOST, Expert_Pursuit_TryScorePlus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_PSYCHIC, Expert_Pursuit_TryScorePlus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_GHOST, Expert_Pursuit_TryScorePlus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_PSYCHIC, Expert_Pursuit_TryScorePlus1
    GoTo Expert_Pursuit_CheckUturn

Expert_Pursuit_TryScorePlus1:
    IfRandomLessThan 128, Expert_Pursuit_CheckUturn
    AddToMoveScore 1

Expert_Pursuit_CheckUturn:
    IfMoveNotKnown AI_BATTLER_DEFENDER, MOVE_U_TURN, Expert_Pursuit_End
    IfRandomLessThan 128, Expert_Pursuit_End
    AddToMoveScore 1

Expert_Pursuit_End:
    End

Expert_RainDance:
    IfSpeedCompareEqualTo COMPARE_SPEED_FASTER, Expert_RainDance_OtherChecks
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_SWIFT_SWIM, Expert_RainDance_ScorePlus1

Expert_RainDance_OtherChecks:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_RainDance_ScoreMinus1
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_HAIL, Expert_RainDance_ScorePlus1
    IfLoadedEqualTo BTL_WEATHER_SUN, Expert_RainDance_ScorePlus1
    IfLoadedEqualTo BTL_WEATHER_SANDSTORM, Expert_RainDance_ScorePlus1
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_RAIN_DISH, Expert_RainDance_ScorePlus1
    IfLoadedEqualTo ABILITY_DRY_SKIN, Expert_RainDance_ScorePlus1
    IfLoadedNotEqualTo ABILITY_HYDRATION, Expert_RainDance_End
    IfStatus AI_BATTLER_ATTACKER, Expert_RainDance_ScorePlus1
    GoTo Expert_RainDance_End

Expert_RainDance_ScorePlus1:
    AddToMoveScore 1
    GoTo Expert_RainDance_End

Expert_RainDance_ScoreMinus1:
    AddToMoveScore -1

Expert_RainDance_End:
    End

Expert_SunnyDay:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_SunnyDay_ScoreMinus1
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_HAIL, Expert_SunnyDay_ScorePlus1
    IfLoadedEqualTo BTL_WEATHER_RAIN, Expert_SunnyDay_ScorePlus1
    IfLoadedEqualTo BTL_WEATHER_SANDSTORM, Expert_SunnyDay_ScorePlus1
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_FLOWER_GIFT, Expert_SunnyDay_ScorePlus1
    IfLoadedNotEqualTo ABILITY_LEAF_GUARD, Expert_SunnyDay_End
    // Bug: Leaf Guard only prevents new status conditions, so this should check for none, as in Gen 4
    IfStatus AI_BATTLER_ATTACKER, Expert_SunnyDay_ScorePlus1
    GoTo Expert_SunnyDay_End

Expert_SunnyDay_ScorePlus1:
    AddToMoveScore 1
    GoTo Expert_SunnyDay_End

Expert_SunnyDay_ScoreMinus1:
    AddToMoveScore -1

Expert_SunnyDay_End:
    End

Expert_BellyDrum:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 90, Expert_BellyDrum_ScoreMinus2
    GoTo Expert_BellyDrum_End

Expert_BellyDrum_ScoreMinus2:
    AddToMoveScore -2

Expert_BellyDrum_End:
    End

Expert_PsychUp:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 8, Expert_PsychUp_CheckUserStatStages
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 8, Expert_PsychUp_CheckUserStatStages
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 8, Expert_PsychUp_CheckUserStatStages
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 8, Expert_PsychUp_CheckUserStatStages
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 8, Expert_PsychUp_CheckUserStatStages
    GoTo Expert_PsychUp_ScoreMinus2

Expert_PsychUp_CheckUserStatStages:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 7, Expert_PsychUp_ScorePlus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 7, Expert_PsychUp_ScorePlus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 7, Expert_PsychUp_ScorePlus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 7, Expert_PsychUp_ScorePlus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_EVASION_STAGE, 7, Expert_PsychUp_ScorePlus2
    IfRandomLessThan 50, Expert_PsychUp_End
    GoTo Expert_PsychUp_ScoreMinus2

Expert_PsychUp_ScorePlus2:
    AddToMoveScore 1

Expert_PsychUp_ScorePlus1:
    AddToMoveScore 1
    End

Expert_PsychUp_ScoreMinus2:
    AddToMoveScore -2

Expert_PsychUp_End:
    End

Expert_MirrorCoat:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, Expert_MirrorCoat_ScoreMinus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_ATTRACT, Expert_MirrorCoat_ScoreMinus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CONFUSION, Expert_MirrorCoat_ScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 30, Expert_MirrorCoat_CheckAboveHalfHP
    IfRandomLessThan 10, Expert_MirrorCoat_CheckAboveHalfHP
    AddToMoveScore -1

Expert_MirrorCoat_CheckAboveHalfHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_MirrorCoat_CheckLastUsedMove
    IfRandomLessThan 100, Expert_MirrorCoat_CheckLastUsedMove
    AddToMoveScore -1

Expert_MirrorCoat_CheckLastUsedMove:
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_COUNTER, Expert_MirrorCoat_TryScorePlus4
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadPowerOfLoadedMove
    IfLoadedEqualTo 0, Expert_MirrorCoat_TryScorePlus1
    IfTargetIsNotTaunted Expert_MirrorCoat_CheckSpecialMove
    IfRandomLessThan 100, Expert_MirrorCoat_CheckSpecialMove
    AddToMoveScore 1

Expert_MirrorCoat_CheckSpecialMove:
    LoadDefenderLastUsedMoveClass
    IfLoadedNotEqualTo MOVE_CATEGORY_SPECIAL, Expert_MirrorCoat_ScoreMinus1
    IfRandomLessThan 100, Expert_MirrorCoat_End2
    AddToMoveScore 1
    GoTo Expert_MirrorCoat_End2

Expert_MirrorCoat_TryScorePlus1:
    IfTargetIsNotTaunted Expert_MirrorCoat_CheckOpponentTypes
    IfRandomLessThan 100, Expert_MirrorCoat_CheckOpponentTypes
    AddToMoveScore 1

Expert_MirrorCoat_CheckOpponentTypes:
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedInTable Expert_MirrorCoat_SpecialTypes, Expert_MirrorCoat_End2
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedInTable Expert_MirrorCoat_SpecialTypes, Expert_MirrorCoat_End2
    IfRandomLessThan 50, Expert_MirrorCoat_End2

Expert_MirrorCoat_TryScorePlus4:
    IfRandomLessThan 100, Expert_MirrorCoat_End
    AddToMoveScore 4

Expert_MirrorCoat_End:
    End

Expert_MirrorCoat_ScoreMinus1:
    AddToMoveScore -1

Expert_MirrorCoat_End2:
    End

Expert_MirrorCoat_SpecialTypes:
    TableEntry TYPE_FIRE
    TableEntry TYPE_WATER
    TableEntry TYPE_GRASS
    TableEntry TYPE_ELECTRIC
    TableEntry TYPE_PSYCHIC
    TableEntry TYPE_ICE
    TableEntry TYPE_DRAGON
    TableEntry TYPE_DARK
    TableEntry TABLE_END

Expert_ChargeTurnNoInvuln:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_ChargeTurnNoInvuln_ScoreMinus2
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_ChargeTurnNoInvuln_ScoreMinus2
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_ChargeTurnNoInvuln_ScoreMinus2
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_SKIP_CHARGE_TURN_IN_SUN, Expert_ChargeTurnNoInvuln_CheckForSunnyWeather
    GoTo Expert_ChargeTurnNoInvuln_CheckForPowerHerb

Expert_ChargeTurnNoInvuln_CheckForSunnyWeather:
    LoadCurrentWeather
    IfLoadedNotEqualTo BTL_WEATHER_SUN, Expert_ChargeTurnNoInvuln_CheckForPowerHerb
    AddToMoveScore 2
    GoTo Expert_ChargeTurnNoInvuln_End

Expert_ChargeTurnNoInvuln_CheckForPowerHerb:
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_POWER_HERB, Expert_ChargeTurnNoInvuln_ScorePlus2
    GoTo Expert_ChargeTurnNoInvuln_CheckForProtectAndHP

Expert_ChargeTurnNoInvuln_ScorePlus2:
    AddToMoveScore 2
    GoTo Expert_ChargeTurnNoInvuln_End

Expert_ChargeTurnNoInvuln_CheckForProtectAndHP:
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_PROTECT, Expert_ChargeTurnNoInvuln_ScoreMinus2
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 38, Expert_ChargeTurnNoInvuln_End
    AddToMoveScore -1
    GoTo Expert_ChargeTurnNoInvuln_End

Expert_ChargeTurnNoInvuln_ScoreMinus2:
    AddToMoveScore -2

Expert_ChargeTurnNoInvuln_End:
    End

Expert_ChargeTurnNoInvuln_CheckEffectivenessAndWeather:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_ChargeTurnNoInvuln_TryScoreMinus3
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_ChargeTurnNoInvuln_TryScoreMinus3
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_ChargeTurnNoInvuln_TryScoreMinus3
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_SUN, Expert_ChargeTurnNoInvuln_TryScoreMinus3
    IfLoadedNotEqualTo BTL_WEATHER_RAIN, Expert_ChargeTurnNoInvuln_End_2
    AddToMoveScore 1
    GoTo Expert_ChargeTurnNoInvuln_End_2

Expert_ChargeTurnNoInvuln_TryScoreMinus3:
    IfRandomLessThan 50, Expert_ChargeTurnNoInvuln_End_2
    AddToMoveScore -3

Expert_ChargeTurnNoInvuln_End_2:
    End

Expert_ChargeTurnWithInvuln:
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_POWER_HERB, Expert_ChargeTurnNoInvuln_ScorePlus2
    IfMoveEffectNotKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_PROTECT, Expert_ShadowForce
    AddToMoveScore -1
    GoTo Expert_ChargeTurnWithInvuln_End

Expert_ShadowForce:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_ChargeTurnWithInvuln_ScorePlus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_ChargeTurnWithInvuln_ScorePlus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_ChargeTurnWithInvuln_ScorePlus1
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_POWER_HERB, Expert_ChargeTurnWithInvuln_ScorePlus1AndEnd
    GoTo Expert_ChargeTurnWithInvuln_CheckConditions

Expert_ChargeTurnWithInvuln_ScorePlus1AndEnd:
    AddToMoveScore 1
    GoTo Expert_ChargeTurnWithInvuln_End

Expert_ChargeTurnWithInvuln_CheckConditions:
    IfSpeedCompareEqualTo COMPARE_SPEED_FASTER, Expert_ChargeTurnWithInvuln_CheckTargetConditions
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 24, Expert_ChargeTurnWithInvuln_CheckTargetConditions
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_CUSTAP_BERRY, Expert_ChargeTurnWithInvuln_TryScorePlus1

Expert_ChargeTurnWithInvuln_CheckTargetConditions:
    IfBadlyPoisoned AI_BATTLER_DEFENDER, Expert_ChargeTurnWithInvuln_TryScorePlus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CURSE, Expert_ChargeTurnWithInvuln_TryScorePlus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_LEECH_SEED, Expert_ChargeTurnWithInvuln_TryScorePlus1
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_SANDSTORM, Expert_ChargeTurnWithInvuln_CheckSandImmuneType
    IfLoadedEqualTo BTL_WEATHER_HAIL, Expert_ChargeTurnWithInvuln_CheckHailImmuneType
    GoTo Expert_ChargeTurnWithInvuln_CompareSpeed

Expert_ChargeTurnWithInvuln_CheckSandImmuneType:
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedInTable Expert_ChargeTurnWithInvuln_SandImmuneTypes, Expert_ChargeTurnWithInvuln_TryScorePlus1
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedInTable Expert_ChargeTurnWithInvuln_SandImmuneTypes, Expert_ChargeTurnWithInvuln_TryScorePlus1
    GoTo Expert_ChargeTurnWithInvuln_CompareSpeed

Expert_ChargeTurnWithInvuln_CheckHailImmuneType:
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_ICE, Expert_ChargeTurnWithInvuln_TryScorePlus1
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_ICE, Expert_ChargeTurnWithInvuln_TryScorePlus1
    GoTo Expert_ChargeTurnWithInvuln_CompareSpeed

Expert_ChargeTurnWithInvuln_CompareSpeed:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_ChargeTurnWithInvuln_End
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadEffectOfLoadedMove
    IfLoadedNotEqualTo BATTLE_EFFECT_NEXT_ATTACK_ALWAYS_HITS, Expert_ChargeTurnWithInvuln_TryScorePlus1
    GoTo Expert_ChargeTurnWithInvuln_End

Expert_ChargeTurnWithInvuln_TryScorePlus1:
    IfRandomLessThan 80, Expert_ChargeTurnWithInvuln_End
    AddToMoveScore 1

Expert_ChargeTurnWithInvuln_End:
    End

Expert_ChargeTurnWithInvuln_ScorePlus1:
    AddToMoveScore 1
    End

Expert_ChargeTurnWithInvuln_SandImmuneTypes:
    TableEntry TYPE_GROUND
    TableEntry TYPE_ROCK
    TableEntry TYPE_STEEL
    TableEntry TABLE_END

Expert_FakeOut:
    AddToMoveScore 2
    End

Expert_Stockpile:
    IfHPPercentNotEqualTo AI_BATTLER_ATTACKER, 100, Expert_Stockpile_CheckHP
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_PASS_STATS_AND_STATUS, Expert_Stockpile_ScorePlus2
    IfRandomLessThan 128, Expert_Stockpile_CheckHP

Expert_Stockpile_ScorePlus2:
    AddToMoveScore 2

Expert_Stockpile_CheckHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_Stockpile_CheckLowHP
    IfRandomLessThan 200, Expert_Stockpile_End

Expert_Stockpile_CheckLowHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_Stockpile_ScoreMinus2
    IfRandomLessThan 60, Expert_Stockpile_End

Expert_Stockpile_ScoreMinus2:
    AddToMoveScore -2

Expert_Stockpile_End:
    End

Expert_SpitUp:
    LoadStockpileCount AI_BATTLER_ATTACKER
    IfLoadedLessThan 2, Expert_SpitUp_End
    IfRandomLessThan 80, Expert_SpitUp_End
    AddToMoveScore 2

Expert_SpitUp_End:
    End

Expert_Hail:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_Hail_ScoreMinus1
    LoadCurrentWeather
    IfLoadedEqualTo BTL_WEATHER_SUN, Expert_Hail_ScorePlus1AndCheckBlizzard
    IfLoadedEqualTo BTL_WEATHER_RAIN, Expert_Hail_ScorePlus1AndCheckBlizzard
    IfLoadedEqualTo BTL_WEATHER_SANDSTORM, Expert_Hail_ScorePlus1AndCheckBlizzard
    GoTo Expert_Hail_End

Expert_Hail_ScorePlus1AndCheckBlizzard:
    AddToMoveScore 1
    IfMoveNotKnown AI_BATTLER_ATTACKER, MOVE_BLIZZARD, Expert_Hail_CheckIceBody
    AddToMoveScore 2

Expert_Hail_CheckIceBody:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo ABILITY_ICE_BODY, Expert_Hail_End
    AddToMoveScore 2
    GoTo Expert_Hail_End

Expert_Hail_ScoreMinus1:
    AddToMoveScore -1

Expert_Hail_End:
    End

// Bug: checks the target's status, where Facade is stronger when the user has one, as in Gen 4
Expert_Facade:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_POISON, Expert_Facade_ScorePlus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_BURN, Expert_Facade_ScorePlus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_PARALYSIS, Expert_Facade_ScorePlus1
    IfNotBadlyPoisoned AI_BATTLER_DEFENDER, Expert_Facade_End

Expert_Facade_ScorePlus1:
    AddToMoveScore 1

Expert_Facade_End:
    End

Expert_FocusPunch:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_FocusPunch_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_FocusPunch_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_FocusPunch_ScoreMinus1
    IfBattlerHasSubstitute AI_BATTLER_ATTACKER, ScorePlus2
    IfCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, ScorePlus2
    IfCondition AI_BATTLER_DEFENDER, CONDITION_YAWN, ScorePlus2
    IfCondition AI_BATTLER_DEFENDER, CONDITION_ATTRACT, Expert_FocusPunch_TryScorePlus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CONFUSION, Expert_FocusPunch_TryScorePlus1
    LoadIsFirstTurnInBattle AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo FALSE, Expert_FocusPunch_End
    IfRandomLessThan 200, Expert_FocusPunch_End
    AddToMoveScore 1
    GoTo Expert_FocusPunch_End

Expert_FocusPunch_ScoreMinus1:
    AddToMoveScore -1
    GoTo Expert_FocusPunch_End

Expert_FocusPunch_TryScorePlus1:
    IfRandomLessThan 100, Expert_FocusPunch_End

Expert_FocusPunch_ScorePlus1:
    AddToMoveScore 1

Expert_FocusPunch_End:
    End

Expert_SmellingSalts:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_PARALYSIS, Expert_SmellingSalts_ScorePlus1
    GoTo Expert_SmellingSalts_End

Expert_SmellingSalts_ScorePlus1:
    AddToMoveScore 1

Expert_SmellingSalts_End:
    End

Expert_Trick:
    LoadHeldItemEffect AI_BATTLER_ATTACKER
    IfLoadedInTable Expert_Trick_DisruptiveItems, Expert_Trick_CheckOpponentItem
    IfLoadedInTable Expert_Trick_PoisoningItems, Expert_Trick_CheckOpponentForPoison
    IfLoadedInTable Expert_Trick_BurningItems, Expert_Trick_CheckOpponentForBurn
    IfLoadedInTable Expert_Trick_BlackSludge, Expert_Trick_CheckOpponentForSludge
    IfLoadedInTable Expert_Trick_FlavorBerries, Expert_Trick_CheckOpponentForFlavorBerry

Expert_Trick_ScoreMinus3:
    AddToMoveScore -3
    GoTo Expert_Trick_End

Expert_Trick_CheckOpponentItem:
    LoadHeldItemEffect AI_BATTLER_DEFENDER
    IfLoadedInTable Expert_Trick_BadOpponentItems, Expert_Trick_ScoreMinus3
    AddToMoveScore 5
    GoTo Expert_Trick_End

Expert_Trick_CheckOpponentForPoison:
    LoadHeldItemEffect AI_BATTLER_DEFENDER
    IfLoadedInTable Expert_Trick_BadOpponentItems, Expert_Trick_ScoreMinus3
    IfStatus AI_BATTLER_DEFENDER, Expert_Trick_CheckAttackerForPoison
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, Expert_Trick_CheckAttackerForPoison
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_STEEL, Expert_Trick_CheckAttackerForPoison
    IfLoadedEqualTo TYPE_POISON, Expert_Trick_CheckAttackerForPoison
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_STEEL, Expert_Trick_CheckAttackerForPoison
    IfLoadedEqualTo TYPE_POISON, Expert_Trick_CheckAttackerForPoison
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_IMMUNITY, Expert_Trick_CheckAttackerForPoison
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, Expert_Trick_CheckAttackerForPoison
    IfLoadedEqualTo ABILITY_POISON_HEAL, Expert_Trick_CheckAttackerForPoison
    IfLoadedEqualTo ABILITY_TOXIC_BOOST, Expert_Trick_CheckAttackerForPoison
    AddToMoveScore 5
    GoTo Expert_Trick_End

Expert_Trick_CheckAttackerForPoison:
    IfStatus AI_BATTLER_ATTACKER, Expert_Trick_ScoreMinus3
    IfSideCondition AI_BATTLER_ATTACKER, SIDE_CONDITION_SAFEGUARD, Expert_Trick_ScoreMinus3
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_STEEL, Expert_Trick_ScoreMinus3
    IfLoadedEqualTo TYPE_POISON, Expert_Trick_ScoreMinus3
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_STEEL, Expert_Trick_ScoreMinus3
    IfLoadedEqualTo TYPE_POISON, Expert_Trick_ScoreMinus3
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_IMMUNITY, Expert_Trick_ScoreMinus3
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, Expert_Trick_ScoreMinus3
    IfLoadedEqualTo ABILITY_POISON_HEAL, Expert_Trick_ScoreMinus3
    IfLoadedEqualTo ABILITY_KLUTZ, Expert_Trick_ScoreMinus3
    IfLoadedEqualTo ABILITY_TOXIC_BOOST, Expert_Trick_ScoreMinus3
    AddToMoveScore 5
    GoTo Expert_Trick_End

Expert_Trick_CheckOpponentForBurn:
    LoadHeldItemEffect AI_BATTLER_DEFENDER
    IfLoadedInTable Expert_Trick_BadOpponentItems, Expert_Trick_ScoreMinus3
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_WATER_VEIL, Expert_Trick_CheckAttackerForBurn
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, Expert_Trick_CheckAttackerForBurn
    IfLoadedEqualTo ABILITY_FLARE_BOOST, Expert_Trick_CheckAttackerForBurn
    IfStatus AI_BATTLER_DEFENDER, Expert_Trick_CheckAttackerForBurn
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SAFEGUARD, Expert_Trick_CheckAttackerForBurn
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_FIRE, Expert_Trick_CheckAttackerForBurn
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_FIRE, Expert_Trick_CheckAttackerForBurn
    AddToMoveScore 5
    GoTo Expert_Trick_End

Expert_Trick_CheckAttackerForBurn:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_WATER_VEIL, Expert_Trick_ScoreMinus3
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, Expert_Trick_ScoreMinus3
    IfLoadedEqualTo ABILITY_KLUTZ, ScoreMinus5
    IfLoadedEqualTo ABILITY_FLARE_BOOST, Expert_Trick_ScoreMinus3
    IfStatus AI_BATTLER_ATTACKER, Expert_Trick_ScoreMinus3
    IfSideCondition AI_BATTLER_ATTACKER, SIDE_CONDITION_SAFEGUARD, Expert_Trick_ScoreMinus3
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_FIRE, Expert_Trick_ScoreMinus3
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_FIRE, Expert_Trick_ScoreMinus3
    AddToMoveScore 5
    GoTo Expert_Trick_End

Expert_Trick_CheckOpponentForSludge:
    LoadHeldItemEffect AI_BATTLER_DEFENDER
    IfLoadedInTable Expert_Trick_BadOpponentItems, Expert_Trick_ScoreMinus3
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_POISON, Expert_Trick_CheckAttackerForSludge
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_POISON, Expert_Trick_CheckAttackerForSludge
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, Expert_Trick_CheckAttackerForPoison
    IfLoadedEqualTo ABILITY_TOXIC_BOOST, Expert_Trick_CheckAttackerForPoison
    AddToMoveScore 5
    GoTo Expert_Trick_End

Expert_Trick_CheckAttackerForSludge:
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedEqualTo TYPE_POISON, Expert_Trick_ScoreMinus3
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedEqualTo TYPE_POISON, Expert_Trick_ScoreMinus3
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, Expert_Trick_ScoreMinus3
    IfLoadedEqualTo ABILITY_KLUTZ, Expert_Trick_ScoreMinus3
    IfLoadedEqualTo ABILITY_TOXIC_BOOST, Expert_Trick_ScoreMinus3
    AddToMoveScore 5
    GoTo Expert_Trick_End

Expert_Trick_CheckOpponentForFlavorBerry:
    LoadHeldItemEffect AI_BATTLER_DEFENDER
    IfLoadedInTable Expert_Trick_BadOpponentItemsAndFlavorBerries, Expert_Trick_ScoreMinus3
    IfRandomLessThan 50, Expert_Trick_End
    AddToMoveScore 2

Expert_Trick_End:
    End

Expert_Trick_FlavorBerries:
    TableEntry HOLD_EFFECT_HP_RESTORE_SPICY
    TableEntry HOLD_EFFECT_HP_RESTORE_DRY
    TableEntry HOLD_EFFECT_HP_RESTORE_SWEET
    TableEntry HOLD_EFFECT_HP_RESTORE_BITTER
    TableEntry HOLD_EFFECT_HP_RESTORE_SOUR
    TableEntry TABLE_END

// Bug: the Defense EV item's effect is here twice, and the Macho Brace's is missing, as in Gen 4
Expert_Trick_DisruptiveItems:
    TableEntry HOLD_EFFECT_CHOICE_ATK
    TableEntry HOLD_EFFECT_CHOICE_SPATK
    TableEntry HOLD_EFFECT_CHOICE_SPEED
    TableEntry HOLD_EFFECT_SPEED_DOWN_GROUNDED
    TableEntry HOLD_EFFECT_PRIORITY_DOWN
    TableEntry HOLD_EFFECT_DMG_USER_CONTACT_XFR
    TableEntry HOLD_EFFECT_LVLUP_ATK_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_DEF_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_SPATK_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_DEF_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_SPDEF_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_SPEED_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_HP_EV_UP
    TableEntry TABLE_END

Expert_Trick_PoisoningItems:
    TableEntry HOLD_EFFECT_PSN_USER
    TableEntry TABLE_END

Expert_Trick_BurningItems:
    TableEntry HOLD_EFFECT_BRN_USER
    TableEntry TABLE_END

Expert_Trick_BlackSludge:
    TableEntry HOLD_EFFECT_HP_RESTORE_PSN_TYPE
    TableEntry TABLE_END

Expert_Trick_BadOpponentItemsAndFlavorBerries:
    TableEntry HOLD_EFFECT_HP_RESTORE_SPICY
    TableEntry HOLD_EFFECT_HP_RESTORE_DRY
    TableEntry HOLD_EFFECT_HP_RESTORE_SWEET
    TableEntry HOLD_EFFECT_HP_RESTORE_BITTER
    TableEntry HOLD_EFFECT_HP_RESTORE_SOUR
    TableEntry HOLD_EFFECT_EVS_UP_SPEED_DOWN
    TableEntry HOLD_EFFECT_CHOICE_ATK
    TableEntry HOLD_EFFECT_CHOICE_SPATK
    TableEntry HOLD_EFFECT_CHOICE_SPEED
    TableEntry HOLD_EFFECT_SPEED_DOWN_GROUNDED
    TableEntry HOLD_EFFECT_PRIORITY_DOWN
    TableEntry HOLD_EFFECT_DMG_USER_CONTACT_XFR
    TableEntry HOLD_EFFECT_LVLUP_ATK_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_DEF_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_SPATK_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_SPDEF_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_SPEED_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_HP_EV_UP
    TableEntry HOLD_EFFECT_PSN_USER
    TableEntry HOLD_EFFECT_BRN_USER
    TableEntry HOLD_EFFECT_HP_RESTORE_PSN_TYPE
    TableEntry TABLE_END

Expert_Trick_BadOpponentItems:
    TableEntry HOLD_EFFECT_EVS_UP_SPEED_DOWN
    TableEntry HOLD_EFFECT_CHOICE_ATK
    TableEntry HOLD_EFFECT_CHOICE_SPATK
    TableEntry HOLD_EFFECT_CHOICE_SPEED
    TableEntry HOLD_EFFECT_SPEED_DOWN_GROUNDED
    TableEntry HOLD_EFFECT_PRIORITY_DOWN
    TableEntry HOLD_EFFECT_DMG_USER_CONTACT_XFR
    TableEntry HOLD_EFFECT_LVLUP_ATK_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_DEF_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_SPATK_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_SPDEF_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_SPEED_EV_UP
    TableEntry HOLD_EFFECT_LVLUP_HP_EV_UP
    TableEntry HOLD_EFFECT_PSN_USER
    TableEntry HOLD_EFFECT_BRN_USER
    TableEntry HOLD_EFFECT_HP_RESTORE_PSN_TYPE
    TableEntry TABLE_END

Expert_ChangeUserAbility:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedInTable Expert_ChangeUserAbility_DesirableAbilities, Expert_ChangeUserAbility_ScoreMinus1
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedInTable Expert_ChangeUserAbility_DesirableAbilities, Expert_ChangeUserAbility_TryScorePlus2
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, Expert_ChangeUserAbility_ScoreMinus1
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedInTable Expert_ChangeUserAbility_DesirableMultiBattleAbilities, Expert_ChangeUserAbility_TryScorePlus2

Expert_ChangeUserAbility_ScoreMinus1:
    AddToMoveScore -1
    GoTo Expert_ChangeUserAbility_End

Expert_ChangeUserAbility_TryScorePlus2:
    IfRandomLessThan 50, Expert_ChangeUserAbility_End
    AddToMoveScore 2

Expert_ChangeUserAbility_End:
    End

Expert_ChangeUserAbility_DesirableAbilities:
    TableEntry ABILITY_SPEED_BOOST
    TableEntry ABILITY_FLASH_FIRE
    TableEntry ABILITY_INTIMIDATE
    TableEntry ABILITY_SWIFT_SWIM
    TableEntry ABILITY_CHLOROPHYLL
    TableEntry ABILITY_HUGE_POWER
    TableEntry ABILITY_RAIN_DISH
    TableEntry ABILITY_GUTS
    TableEntry ABILITY_PURE_POWER
    TableEntry ABILITY_MOTOR_DRIVE
    TableEntry ABILITY_DRY_SKIN
    TableEntry ABILITY_POISON_HEAL
    TableEntry ABILITY_ADAPTABILITY
    TableEntry ABILITY_SOLAR_POWER
    TableEntry ABILITY_TECHNICIAN
    TableEntry ABILITY_ICE_BODY
    TableEntry ABILITY_CONTRARY
    TableEntry ABILITY_CURSED_BODY
    TableEntry ABILITY_TOXIC_BOOST
    TableEntry ABILITY_FLARE_BOOST
    TableEntry ABILITY_HARVEST
    TableEntry ABILITY_SAND_RUSH
    TableEntry ABILITY_MAGIC_BOUNCE
    TableEntry ABILITY_PRANKSTER
    TableEntry TABLE_END

Expert_ChangeUserAbility_DesirableMultiBattleAbilities:
    TableEntry ABILITY_SHADOW_TAG
    TableEntry ABILITY_ARENA_TRAP
    TableEntry ABILITY_TELEPATHY
    TableEntry ABILITY_FRIEND_GUARD
    TableEntry ABILITY_OWN_TEMPO
    TableEntry TABLE_END

Expert_Ingrain:
    End

Expert_Superpower:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_Superpower_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_Superpower_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_Superpower_ScoreMinus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 6, Expert_Superpower_ScoreMinus1
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Superpower_CheckUserHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 40, Expert_Superpower_ScoreMinus1
    GoTo Expert_Superpower_End

Expert_Superpower_CheckUserHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 60, Expert_Superpower_End

Expert_Superpower_ScoreMinus1:
    AddToMoveScore -1

Expert_Superpower_End:
    End

Expert_MagicCoat:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 30, Expert_MagicCoat_CheckUserFirstTurn
    IfRandomLessThan 100, Expert_MagicCoat_CheckUserFirstTurn
    AddToMoveScore -1

Expert_MagicCoat_CheckUserFirstTurn:
    LoadIsFirstTurnInBattle AI_BATTLER_ATTACKER
    IfLoadedEqualTo FALSE, Expert_MagicCoat_TryScoreMinus1
    IfRandomLessThan 150, Expert_MagicCoat_End
    AddToMoveScore 1
    GoTo Expert_MagicCoat_End
    IfRandomLessThan 50, Expert_MagicCoat_End

Expert_MagicCoat_TryScoreMinus1:
    IfRandomLessThan 30, Expert_MagicCoat_End
    AddToMoveScore -1

Expert_MagicCoat_End:
    End

Expert_Recycle:
    LoadRecycleItem AI_BATTLER_ATTACKER
    IfLoadedNotInTable Expert_Recycle_DesirableItems, Expert_Recycle_ScoreMinus2
    IfRandomLessThan 50, Expert_Recycle_End
    AddToMoveScore 1
    GoTo Expert_Recycle_End

Expert_Recycle_ScoreMinus2:
    AddToMoveScore -2

Expert_Recycle_End:
    End

Expert_Recycle_DesirableItems:
    TableEntry ITEM_CHESTO_BERRY
    TableEntry ITEM_LUM_BERRY
    TableEntry ITEM_STARF_BERRY
    TableEntry TABLE_END

Expert_Revenge:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, Expert_Revenge_ScoreMinus2
    IfCondition AI_BATTLER_DEFENDER, CONDITION_ATTRACT, Expert_Revenge_ScoreMinus2
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CONFUSION, Expert_Revenge_ScoreMinus2
    IfRandomLessThan 180, Expert_Revenge_ScoreMinus2
    AddToMoveScore 2
    GoTo Expert_Revenge_End

Expert_Revenge_ScoreMinus2:
    AddToMoveScore -2

Expert_Revenge_End:
    End

Expert_BrickBreak:
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_REFLECT, Expert_BrickBreak_ScorePlus1
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_LIGHT_SCREEN, Expert_BrickBreak_ScorePlus1
    GoTo Expert_BrickBreak_End

Expert_BrickBreak_ScorePlus1:
    AddToMoveScore 1

Expert_BrickBreak_End:
    End

Expert_KnockOff:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_HARVEST, ScoreMinus5
    IfHPPercentLessThan AI_BATTLER_DEFENDER, 30, Expert_KnockOff_End
    LoadIsFirstTurnInBattle AI_BATTLER_ATTACKER
    IfLoadedGreaterThan FALSE, Expert_KnockOff_End
    IfRandomLessThan 180, Expert_KnockOff_End
    AddToMoveScore 1

Expert_KnockOff_End:
    End

Expert_Endeavor:
    IfHPPercentLessThan AI_BATTLER_DEFENDER, 70, Expert_Endeavor_ScoreMinus1
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Endeavor_SlowerCheckHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 40, Expert_Endeavor_ScoreMinus1
    AddToMoveScore 1
    GoTo Expert_Endeavor_End

Expert_Endeavor_SlowerCheckHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_Endeavor_ScoreMinus1
    AddToMoveScore 1
    GoTo Expert_Endeavor_End

Expert_Endeavor_ScoreMinus1:
    AddToMoveScore -1

Expert_Endeavor_End:
    End

Expert_WaterSpout:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_WaterSpout_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_WaterSpout_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_WaterSpout_ScoreMinus1
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_WaterSpout_SlowerCheckHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 90, Expert_WaterSpout_TryScorePlus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_WaterSpout_End
    GoTo Expert_WaterSpout_ScoreMinus1

Expert_WaterSpout_TryScorePlus1:
    IfRandomLessThan 30, Expert_WaterSpout_End
    AddToMoveScore 1
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_CHOICE_SCARF, Expert_WaterSpout_ScorePlus1
    GoTo Expert_WaterSpout_End

Expert_WaterSpout_ScorePlus1:
    AddToMoveScore 1
    GoTo Expert_WaterSpout_End

Expert_WaterSpout_SlowerCheckHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 80, Expert_WaterSpout_End

Expert_WaterSpout_ScoreMinus1:
    AddToMoveScore -1

Expert_WaterSpout_End:
    End

Expert_Imprison:
    LoadIsFirstTurnInBattle AI_BATTLER_ATTACKER
    IfLoadedGreaterThan FALSE, Expert_Imprison_End
    IfRandomLessThan 100, Expert_Imprison_End
    AddToMoveScore 2

Expert_Imprison_End:
    End

Expert_Refresh:
    IfHPPercentLessThan AI_BATTLER_DEFENDER, 50, Expert_Refresh_ScoreMinus1
    GoTo Expert_Refresh_End

Expert_Refresh_ScoreMinus1:
    AddToMoveScore -1

Expert_Refresh_End:
    End

Expert_Snatch:
    LoadIsFirstTurnInBattle AI_BATTLER_ATTACKER
    IfLoadedEqualTo TRUE, Expert_Snatch_TryScorePlus2
    IfRandomLessThan 30, Expert_Snatch_End
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Snatch_UserIsSlower
    IfHPPercentNotEqualTo AI_BATTLER_ATTACKER, 100, Expert_Snatch_TryScoreMinus2
    IfHPPercentLessThan AI_BATTLER_DEFENDER, 70, Expert_Snatch_TryScoreMinus2
    IfRandomLessThan 60, Expert_Snatch_End
    GoTo Expert_Snatch_TryScoreMinus2

Expert_Snatch_UserIsSlower:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 25, Expert_Snatch_TryScoreMinus2
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_RESTORE_HALF_HP, Expert_Snatch_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_DEF_UP_DOUBLE_ROLLOUT_POWER, Expert_Snatch_TryScorePlus2
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HEAL_HALF_REMOVE_FLYING_TYPE, Expert_Snatch_TryScorePlus2
    GoTo Expert_Snatch_TryScorePlus1

Expert_Snatch_TryScorePlus2:
    IfRandomLessThan 150, Expert_Snatch_End
    AddToMoveScore 2
    GoTo Expert_Snatch_End

Expert_Snatch_TryScorePlus1:
    IfRandomLessThan 230, Expert_Snatch_TryScoreMinus2
    AddToMoveScore 1
    GoTo Expert_Snatch_End

Expert_Snatch_TryScoreMinus2:
    IfRandomLessThan 30, Expert_Snatch_End
    AddToMoveScore -2

Expert_Snatch_End:
    End

Expert_MudSport:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 50, Expert_MudSport_ScoreMinus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_ELECTRIC, Expert_MudSport_ScorePlus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_ELECTRIC, Expert_MudSport_ScorePlus1
    GoTo Expert_MudSport_ScoreMinus1

Expert_MudSport_ScorePlus1:
    AddToMoveScore 1
    GoTo Expert_MudSport_End

Expert_MudSport_ScoreMinus1:
    AddToMoveScore -1

Expert_MudSport_End:
    End

Expert_Overheat:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_Overheat_ScoreMinus1
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_CONTRARY, Expert_Overheat_CheckEffectiveness
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_Overheat_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_Overheat_ScoreMinus1
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Overheat_SlowerCheckHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 60, Expert_Overheat_End
    GoTo Expert_Overheat_ScoreMinus1

Expert_Overheat_CheckEffectiveness:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_Overheat_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_Overheat_SlowerCheckHP
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 50, Expert_Overheat_End
    IfRandomLessThan 50, Expert_Gravity_TryScorePlus1
    AddToMoveScore 2
    GoTo Expert_Overheat_End

Expert_Overheat_SlowerCheckHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 80, Expert_Overheat_End

Expert_Overheat_ScoreMinus1:
    AddToMoveScore -1

Expert_Overheat_End:
    End

Expert_WaterSport:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 50, Expert_WaterSport_ScoreMinus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_FIRE, Expert_WaterSport_ScorePlus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_FIRE, Expert_WaterSport_ScorePlus1
    GoTo Expert_WaterSport_ScoreMinus1

Expert_WaterSport_ScorePlus1:
    AddToMoveScore 1
    GoTo Expert_WaterSport_End

Expert_WaterSport_ScoreMinus1:
    AddToMoveScore -1

Expert_WaterSport_End:
    End

Expert_DragonDance:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_DragonDance_TryScorePlus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_DragonDance_End
    IfRandomLessThan 50, Expert_DragonDance_End
    AddToMoveScore -1
    GoTo Expert_DragonDance_End

Expert_DragonDance_TryScorePlus1:
    IfRandomLessThan 50, Expert_DragonDance_End
    AddToMoveScore 1

Expert_DragonDance_End:
    End

Expert_Gravity:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_LEVITATE, Expert_Gravity_TryScorePlus1
    IfCondition AI_BATTLER_DEFENDER, 30, Expert_Gravity_TryScorePlus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_FLYING, Expert_Gravity_TryScorePlus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_FLYING, Expert_Gravity_TryScorePlus1
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 60, Expert_Gravity_End
    IfRandomLessThan 128, Expert_Gravity_TryScorePlus1
    GoTo Expert_Gravity_End

Expert_Gravity_TryScorePlus1:
    IfRandomLessThan 64, Expert_Gravity_End
    AddToMoveScore 1

Expert_Gravity_End:
    End

Expert_MiracleEye:
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_DARK, Expert_MiracleEye_ExtraRandomGate
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_DARK, Expert_MiracleEye_ExtraRandomGate
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 8, Expert_MiracleEye_ScorePlus2
    AddToMoveScore -2
    End

Expert_MiracleEye_ExtraRandomGate:
    IfRandomLessThan 80, Expert_MiracleEye_End

Expert_MiracleEye_ScorePlus2:
    IfRandomLessThan 80, Expert_MiracleEye_End
    AddToMoveScore 2

Expert_MiracleEye_End:
    End

Expert_WakeUpSlap:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_WakeUpSlap_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_WakeUpSlap_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_WakeUpSlap_ScoreMinus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, Expert_WakeUpSlap_ScorePlus1
    GoTo Expert_WakeUpSlap_End

Expert_WakeUpSlap_ScoreMinus1:
    AddToMoveScore -1
    GoTo Expert_WakeUpSlap_End

Expert_WakeUpSlap_ScorePlus1:
    AddToMoveScore 1

Expert_WakeUpSlap_End:
    End

Expert_HammerArm:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_HammerArm_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_HammerArm_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_HammerArm_ScoreMinus1
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_HammerArm_ScorePlus1
    GoTo Expert_HammerArm_End

Expert_HammerArm_ScoreMinus1:
    AddToMoveScore -1
    End

Expert_HammerArm_ScorePlus1:
    AddToMoveScore 1

Expert_HammerArm_End:
    End

Expert_GyroBall:
    End

Expert_Brine:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_Brine_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_Brine_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_Brine_ScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 50, Expert_Brine_End
    AddToMoveScore 1
    IfRandomLessThan 128, Expert_Brine_End
    AddToMoveScore 1
    GoTo Expert_Brine_End

Expert_Brine_ScoreMinus1:
    AddToMoveScore -1

Expert_Brine_End:
    End

Expert_Feint:
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_PROTECT, Expert_Feint_CheckConditions
    IfRandomLessThan 64, Expert_Feint_CheckConditions
    GoTo Expert_Feint_End

Expert_Feint_CheckConditions:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo ABILITY_GUTS, Expert_Feint_CheckAttackerConditions
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_FLAME_ORB, Expert_Feint_CheckFirstTurn
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_TOXIC_ORB, Expert_Feint_CheckFirstTurn
    GoTo Expert_Feint_CheckAttackerConditions

Expert_Feint_CheckFirstTurn:
    LoadTurnCount
    IfLoadedNotEqualTo 0, Expert_Feint_CheckAttackerConditions
    AddToMoveScore 2

Expert_Feint_CheckAttackerConditions:
    IfBadlyPoisoned AI_BATTLER_ATTACKER, Expert_Feint_TryScorePlus1
    IfCondition AI_BATTLER_ATTACKER, CONDITION_CURSE, Expert_Feint_TryScorePlus1
    IfCondition AI_BATTLER_ATTACKER, CONDITION_PERISH_SONG, Expert_Feint_TryScorePlus1
    IfCondition AI_BATTLER_ATTACKER, CONDITION_ATTRACT, Expert_Feint_TryScorePlus1
    IfCondition AI_BATTLER_ATTACKER, CONDITION_LEECH_SEED, Expert_Feint_TryScorePlus1
    IfCondition AI_BATTLER_ATTACKER, CONDITION_YAWN, Expert_Feint_TryScorePlus1
    IfHPPercentEqualTo AI_BATTLER_DEFENDER, 100, Expert_Feint_CheckProtectChain
    LoadHeldItemEffect AI_BATTLER_DEFENDER
    IfLoadedNotInTable Expert_Feint_HealingHoldEffects, Expert_Feint_CheckProtectChain

Expert_Feint_TryScorePlus1:
    IfRandomLessThan 128, Expert_Feint_CheckProtectChain
    AddToMoveScore 1

Expert_Feint_CheckProtectChain:
    LoadProtectChain AI_BATTLER_DEFENDER
    IfLoadedEqualTo 0, Expert_Feint_TryScorePlus1_2
    IfLoadedEqualTo 1, Expert_Feint_TryScorePlus1_3
    IfLoadedGreaterThan 2, Expert_Feint_ScoreMinus2

Expert_Feint_TryScorePlus1_2:
    IfRandomLessThan 128, Expert_Feint_End
    AddToMoveScore 1
    GoTo Expert_Feint_End

Expert_Feint_TryScorePlus1_3:
    IfRandomLessThan 192, Expert_Feint_End
    AddToMoveScore 1
    GoTo Expert_Feint_End

Expert_Feint_ScoreMinus2:
    AddToMoveScore -2

Expert_Feint_End:
    End

Expert_Feint_HealingHoldEffects:
    TableEntry HOLD_EFFECT_HP_RESTORE_GRADUAL
    TableEntry HOLD_EFFECT_HP_RESTORE_PSN_TYPE
    TableEntry TABLE_END

Expert_Pluck:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_Pluck_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_Pluck_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_Pluck_ScoreMinus1
    LoadIsFirstTurnInBattle AI_BATTLER_ATTACKER
    IfLoadedEqualTo FALSE, Expert_Pluck_TryScorePlus1
    IfRandomLessThan 64, Expert_Pluck_TryScorePlus1
    AddToMoveScore 1

Expert_Pluck_TryScorePlus1:
    IfRandomLessThan 128, Expert_Pluck_End
    AddToMoveScore 1
    GoTo Expert_Pluck_End

Expert_Pluck_ScoreMinus1:
    AddToMoveScore -1

Expert_Pluck_End:
    End

Expert_Tailwind:
    IfRandomLessThan 64, Expert_Tailwind_End
    IfSpeedCompareEqualTo COMPARE_SPEED_FASTER, Expert_Tailwind_ScoreMinus1
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 31, Expert_Tailwind_ScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 75, Expert_Tailwind_ScorePlus1
    IfRandomLessThan 64, Expert_Tailwind_End

Expert_Tailwind_ScorePlus1:
    AddToMoveScore 1
    GoTo Expert_Tailwind_End

Expert_Tailwind_ScoreMinus1:
    AddToMoveScore -1

Expert_Tailwind_End:
    End

Expert_Acupressure:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 51, Expert_Acupressure_ScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 90, Expert_Acupressure_TryScorePlus1
    IfRandomLessThan 128, Expert_Acupressure_End

Expert_Acupressure_TryScorePlus1:
    IfRandomLessThan 64, Expert_Acupressure_End
    AddToMoveScore 1
    GoTo Expert_Acupressure_End

Expert_Acupressure_ScoreMinus1:
    AddToMoveScore -1

Expert_Acupressure_End:
    End

Expert_MetalBurst:
    IfCondition AI_BATTLER_DEFENDER, CONDITION_SLEEP, Expert_MetalBurst_ScoreMinus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_ATTRACT, Expert_MetalBurst_ScoreMinus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_CONFUSION, Expert_MetalBurst_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_DOUBLE_POWER_IF_HIT, Expert_MetalBurst_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HIT_LAST_WHIFF_IF_HIT, Expert_MetalBurst_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_PRIORITY_NEG_1_BYPASS_ACCURACY, Expert_MetalBurst_ScoreMinus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 30, Expert_MetalBurst_MediumHPTryScoreMinus1
    IfRandomLessThan 10, Expert_MetalBurst_MediumHPTryScoreMinus1
    AddToMoveScore -1

Expert_MetalBurst_MediumHPTryScoreMinus1:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_MetalBurst_HighHPTryScorePlus1
    IfRandomLessThan 100, Expert_MetalBurst_HighHPTryScorePlus1
    AddToMoveScore -1

Expert_MetalBurst_HighHPTryScorePlus1:
    IfRandomLessThan 192, Expert_MetalBurst_CheckLastUsedMove
    AddToMoveScore 1

Expert_MetalBurst_CheckLastUsedMove:
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    LoadPowerOfLoadedMove
    IfLoadedEqualTo 0, Expert_MetalBurst_TryScorePlus1
    IfTargetIsNotTaunted Expert_MetalBurst_TryScorePlus1
    IfRandomLessThan 100, Expert_MetalBurst_TryScorePlus1
    AddToMoveScore 1

Expert_MetalBurst_TryScorePlus1:
    IfTargetIsNotTaunted Expert_MetalBurst_End
    IfRandomLessThan 100, Expert_MetalBurst_End
    AddToMoveScore 1
    GoTo Expert_MetalBurst_End

Expert_MetalBurst_ScoreMinus1:
    AddToMoveScore -1

Expert_MetalBurst_End:
    End

Expert_UTurn:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_UTurn_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_UTurn_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_UTurn_ScoreMinus1
    CountAlivePartyBattlers AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, Expert_UTurn_End
    IfHasSuperEffectiveMove Expert_UTurn_TryScoreMinus2
    GoTo Expert_UTurn_CheckPartyDamage

Expert_UTurn_ScoreMinus1:
    AddToMoveScore -1
    GoTo Expert_UTurn_End

Expert_UTurn_TryScoreMinus2:
    IfRandomLessThan 64, Expert_UTurn_CheckPartyDamage
    AddToMoveScore -2

Expert_UTurn_CheckPartyDamage:
    IfPartyMemberDealsMoreDamage USE_MIN_DAMAGE, Expert_UTurn_CheckTargetHP
    IfRandomLessThan 64, Expert_UTurn_CheckTargetHP
    AddToMoveScore -2
    GoTo Expert_UTurn_End

Expert_UTurn_CheckTargetHP:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, Expert_UTurn_75PercentScorePlus1
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 30, Expert_UTurn_50PercentScorePlus1
    IfRandomLessThan 128, Expert_UTurn_CheckSpeed
    GoTo Expert_UTurn_50PercentScorePlus1

Expert_UTurn_75PercentScorePlus1:
    IfRandomLessThan 64, Expert_UTurn_50PercentScorePlus1
    AddToMoveScore 1

Expert_UTurn_50PercentScorePlus1:
    IfRandomLessThan 128, Expert_UTurn_CheckSpeed
    AddToMoveScore 1

Expert_UTurn_CheckSpeed:
    IfSpeedCompareEqualTo COMPARE_SPEED_FASTER, Expert_UTurn_ScorePlus1
    IfRandomLessThan 128, Expert_UTurn_End

Expert_UTurn_ScorePlus1:
    AddToMoveScore 1

Expert_UTurn_End:
    End

Expert_CloseCombat:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_CloseCombat_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_CloseCombat_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_CloseCombat_ScoreMinus1
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_CloseCombat_SlowerCheckHP
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 60, Expert_CloseCombat_End
    GoTo Expert_CloseCombat_ScoreMinus1

Expert_CloseCombat_SlowerCheckHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 80, Expert_CloseCombat_End

Expert_CloseCombat_ScoreMinus1:
    AddToMoveScore -1

Expert_CloseCombat_End:
    End

Expert_Payback:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_Payback_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_Payback_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_Payback_ScoreMinus1
    IfSpeedCompareEqualTo COMPARE_SPEED_FASTER, Expert_Payback_End
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 30, Expert_Payback_End
    IfRandomLessThan 64, Expert_Payback_End
    AddToMoveScore 1
    End

Expert_Payback_ScoreMinus1:
    AddToMoveScore -1

Expert_Payback_End:
    End

Expert_Assurance:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_Assurance_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_Assurance_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_Assurance_ScoreMinus1
    IfSpeedCompareEqualTo COMPARE_SPEED_FASTER, Expert_Assurance_End
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_ROUGH_SKIN, Expert_Assurance_TryScorePlus1
    LoadHeldItem AI_BATTLER_ATTACKER
    IfLoadedInTable Expert_Assurance_RecoilBerries, Expert_Assurance_TryScorePlus1
    IfRandomLessThan 128, Expert_Assurance_TryScorePlus1
    GoTo Expert_Assurance_End

Expert_Assurance_ScoreMinus1:
    AddToMoveScore -1
    GoTo Expert_Assurance_End

Expert_Assurance_TryScorePlus1:
    IfRandomLessThan 128, Expert_Assurance_End
    AddToMoveScore 1

Expert_Assurance_End:
    End

Expert_Assurance_RecoilBerries:
    TableEntry ITEM_JABOCA_BERRY
    TableEntry ITEM_ROWAP_BERRY
    TableEntry TABLE_END

Expert_Embargo:
    IfRandomLessThan 128, Expert_Embargo_End
    AddToMoveScore 1

Expert_Embargo_End:
    End

Expert_Fling:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_Fling_CheckAttackerItem
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_Fling_CheckAttackerItem
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_Fling_CheckAttackerItem
    LoadFlingPower AI_BATTLER_ATTACKER
    IfLoadedLessThan 30, Expert_Fling_ScoreMinus2
    IfLoadedGreaterThan 90, Expert_Fling_CheckWeakness
    IfLoadedGreaterThan 60, Expert_Fling_TryScorePlus1
    IfRandomLessThan 128, Expert_Fling_End
    AddToMoveScore -1
    GoTo Expert_Fling_End

Expert_Fling_ScoreMinus2:
    AddToMoveScore -2
    GoTo Expert_Fling_End

Expert_Fling_CheckWeakness:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_DOUBLE, Expert_Fling_ScorePlus4
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUADRUPLE, Expert_Fling_ScorePlus4
    IfRandomLessThan 128, Expert_Fling_TryScorePlus1
    AddToMoveScore 1
    GoTo Expert_Fling_TryScorePlus1

Expert_Fling_ScorePlus4:
    AddToMoveScore 4

Expert_Fling_TryScorePlus1:
    IfRandomLessThan 64, Expert_Fling_End
    AddToMoveScore 1
    GoTo Expert_Fling_End

Expert_Fling_CheckAttackerItem:
    LoadHeldItemEffect AI_BATTLER_ATTACKER
    IfLoadedInTable Expert_Fling_DesirableFlingEffects, Expert_Fling_End
    AddToMoveScore -1

Expert_Fling_End:
    End

Expert_Fling_DesirableFlingEffects:
    TableEntry HOLD_EFFECT_SOMETIMES_FLINCH
    TableEntry HOLD_EFFECT_STRENGTHEN_POISON
    TableEntry HOLD_EFFECT_PSN_USER
    TableEntry HOLD_EFFECT_BRN_USER
    TableEntry HOLD_EFFECT_PIKA_SPATK_UP
    TableEntry TABLE_END

Expert_PsychoShift:
    IfNotStatus AI_BATTLER_ATTACKER, ScoreMinus10
    IfRandomLessThan 128, Expert_PsychoShift_End
    IfHPPercentLessThan AI_BATTLER_DEFENDER, 30, Expert_PsychoShift_End
    AddToMoveScore 1

Expert_PsychoShift_End:
    End

Expert_TrumpCard:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_TrumpCard_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_TrumpCard_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_TrumpCard_ScoreMinus1
    LoadCurrentMovePP
    IfLoadedEqualTo 1, Expert_TrumpCard_ScorePlus3
    IfLoadedEqualTo 2, Expert_TrumpCard_ScorePlus1Maybe2
    IfLoadedEqualTo 3, Expert_TrumpCard_ScorePlus1
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedNotEqualTo ABILITY_PRESSURE, Expert_TrumpCard_CheckStats
    IfRandomLessThan 30, Expert_TrumpCard_CheckStats
    AddToMoveScore 1

Expert_TrumpCard_CheckStats:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 10, Expert_TrumpCard_ScorePlus1Maybe2
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 2, Expert_TrumpCard_ScorePlus1Maybe2
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 8, Expert_TrumpCard_ScorePlus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 4, Expert_TrumpCard_ScorePlus1
    GoTo Expert_TrumpCard_End

Expert_TrumpCard_ScorePlus1Maybe2:
    AddToMoveScore 1

Expert_TrumpCard_ScorePlus1:
    IfRandomLessThan 100, Expert_TrumpCard_End
    AddToMoveScore 1
    GoTo Expert_TrumpCard_End

Expert_TrumpCard_ScorePlus3:
    AddToMoveScore 3
    GoTo Expert_TrumpCard_End

Expert_TrumpCard_ScoreMinus1:
    AddToMoveScore -1

Expert_TrumpCard_End:
    End

Expert_HealBlock:
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_RECOVER_DAMAGE_SLEEP, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_RESTORE_HALF_HP, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HEAL_HALF_REMOVE_FLYING_TYPE, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_UNUSED_157, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HEAL_HALF_MORE_IN_SUN, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_REST, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_SWALLOW, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_RECOVER_HALF_DAMAGE_DEALT, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_GROUND_TRAP_USER_CONTINUOUS_HEAL, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_RESTORE_HP_EVERY_TURN, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_STATUS_LEECH_SEED, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_FAINT_AND_FULL_HEAL_NEXT_MON, Expert_HealBlock_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_FAINT_FULL_RESTORE_NEXT_MON, Expert_HealBlock_TryScorePlus1
    IfCondition AI_BATTLER_ATTACKER, CONDITION_LEECH_SEED, Expert_HealBlock_TryScorePlus1
    IfCondition AI_BATTLER_DEFENDER, 35, Expert_HealBlock_TryScorePlus1
    IfCondition AI_BATTLER_DEFENDER, CONDITION_INGRAIN, Expert_HealBlock_TryScorePlus1
    IfRandomLessThan 96, Expert_HealBlock_TryScorePlus1
    GoTo Expert_HealBlock_End

Expert_HealBlock_TryScorePlus1:
    IfRandomLessThan 25, Expert_HealBlock_End
    AddToMoveScore 1

Expert_HealBlock_End:
    End

Expert_WringOut:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_WringOut_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_WringOut_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_WringOut_ScoreMinus1
    IfHPPercentLessThan AI_BATTLER_DEFENDER, 50, Expert_WringOut_ScoreMinus1
    IfHPPercentEqualTo AI_BATTLER_DEFENDER, 100, Expert_WringOut_CheckSpeed
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 85, Expert_WringOut_TryScorePlus1
    GoTo Expert_WringOut_End

Expert_WringOut_CheckSpeed:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_WringOut_ScorePlus1
    AddToMoveScore 1

Expert_WringOut_ScorePlus1:
    AddToMoveScore 1

Expert_WringOut_TryScorePlus1:
    IfRandomLessThan 25, Expert_WringOut_End
    AddToMoveScore 1
    GoTo Expert_WringOut_End

Expert_WringOut_ScoreMinus1:
    AddToMoveScore -1

Expert_WringOut_End:
    End

Expert_PowerTrick:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 90, Expert_PowerTrick_LikelyScorePlus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 60, Expert_PowerTrick_CoinFlipScorePlus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 30, Expert_PowerTrick_UnlikelyScorePlus1
    GoTo ScoreMinus2

Expert_PowerTrick_LikelyScorePlus1:
    IfRandomLessThan 96, Expert_PowerTrick_End
    AddToMoveScore 1
    GoTo Expert_PowerTrick_End

Expert_PowerTrick_CoinFlipScorePlus1:
    IfRandomLessThan 128, Expert_PowerTrick_End
    AddToMoveScore 1
    GoTo Expert_PowerTrick_End

Expert_PowerTrick_UnlikelyScorePlus1:
    IfRandomLessThan 164, Expert_PowerTrick_End
    AddToMoveScore 1
    GoTo Expert_PowerTrick_End

Expert_PowerTrick_End:
    End

Expert_SimpleBeam:
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_ATK_UP, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_DEF_UP, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_SPEED_UP, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_SP_ATK_UP, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_SP_DEF_UP, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_ACC_UP, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_EVA_UP, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_ATK_UP_2, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_DEF_UP_2, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_SPEED_UP_2, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_SP_ATK_UP_2, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_SP_DEF_UP_2, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_ACC_UP_2, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_EVA_UP_2, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_CURSE, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_DEF_SPD_UP, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_ATK_DEF_UP, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_SP_ATK_SP_DEF_UP, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_ATK_SPD_UP, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_RANDOM_STAT_UP_2, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HONE_CLAWS, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_QUIVER_DANCE, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_SHELL_SMASH, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_GROWTH, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_COIL, Expert_GastroAcid_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_ATK_DOWN, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_DEF_DOWN, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_SPEED_DOWN, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_SP_ATK_DOWN, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_SP_DEF_DOWN, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_ACC_DOWN, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_EVA_DOWN, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_ATK_DOWN_2, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_DEF_DOWN_2, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_SPEED_DOWN_2, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_SP_ATK_DOWN_2, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_SP_DEF_DOWN_2, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_EVA_DOWN_2, Expert_GastroAcid_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_ACC_DOWN_2, Expert_GastroAcid_TryScorePlus1

// Checks the attacker's own ability against the abilities worth suppressing
Expert_GastroAcid:
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedInTable Expert_GastroAcid_Abilities, Expert_GastroAcid_TryScorePlus1
    IfRandomLessThan 64, Expert_GastroAcid_End
    AddToMoveScore 1
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, Expert_GastroAcid_End
    IfRandomLessThan 128, Expert_GastroAcid_ContinueHPCheck
    AddToMoveScore -1

Expert_GastroAcid_ContinueHPCheck:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 50, Expert_GastroAcid_End
    AddToMoveScore -1
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 30, Expert_GastroAcid_End
    AddToMoveScore -1
    GoTo Expert_GastroAcid_End

Expert_GastroAcid_ScoreMinus1:
    AddToMoveScore -1
    GoTo Expert_GastroAcid_End

Expert_GastroAcid_TryScorePlus1:
    IfRandomLessThan 40, Expert_GastroAcid_End
    AddToMoveScore 1

Expert_GastroAcid_End:
    End

Expert_GastroAcid_Abilities:
    TableEntry ABILITY_NO_GUARD
    TableEntry ABILITY_PRANKSTER
    TableEntry ABILITY_WONDER_SKIN
    TableEntry ABILITY_HARVEST
    TableEntry ABILITY_FLARE_BOOST
    TableEntry ABILITY_TOXIC_BOOST
    TableEntry ABILITY_TECHNICIAN
    TableEntry ABILITY_MAGIC_GUARD
    TableEntry ABILITY_NORMALIZE
    TableEntry ABILITY_POISON_HEAL
    TableEntry ABILITY_HUGE_POWER
    TableEntry ABILITY_WONDER_GUARD
    TableEntry ABILITY_SHADOW_TAG
    TableEntry TABLE_END
    IfLoadedEqualTo 99, Expert_StatusSleep_TryScorePlus1

Expert_LuckyChant:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_LuckyChant_ScoreMinus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HIGH_CRITICAL, Expert_LuckyChant_ScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HIGH_CRITICAL_BURN_HIT, Expert_LuckyChant_ScorePlus1
    IfMoveEffectKnown AI_BATTLER_DEFENDER, BATTLE_EFFECT_HIGH_CRITICAL_POISON_HIT, Expert_LuckyChant_ScorePlus1
    IfRandomLessThan 64, Expert_LuckyChant_ScorePlus1
    GoTo Expert_LuckyChant_End

Expert_LuckyChant_ScorePlus1:
    AddToMoveScore 1
    GoTo Expert_LuckyChant_End

Expert_LuckyChant_ScoreMinus1:
    AddToMoveScore -1

Expert_LuckyChant_End:
    End

Expert_MeFirst:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_MeFirst_ScoreMinus2
    IfBattlerDealsMoreDamage AI_BATTLER_DEFENDER, USE_MIN_DAMAGE, Expert_MeFirst_TryScorePlus1
    GoTo Expert_MeFirst_CheckLastUsedMove

Expert_MeFirst_TryScorePlus1:
    IfRandomLessThan 32, Expert_MeFirst_CheckLastUsedMove
    AddToMoveScore 1

Expert_MeFirst_CheckLastUsedMove:
    LoadDefenderLastUsedMoveClass
    IfLoadedEqualTo MOVE_CATEGORY_STATUS, Expert_MeFirst_TryScorePlus1AndEnd
    IfRandomLessThan 128, Expert_MeFirst_End
    AddToMoveScore 1

Expert_MeFirst_TryScorePlus1AndEnd:
    IfRandomLessThan 64, Expert_MeFirst_End
    AddToMoveScore 1
    GoTo Expert_MeFirst_End

Expert_MeFirst_ScoreMinus2:
    AddToMoveScore -2

Expert_MeFirst_End:
    End

Expert_Copycat:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Copycat_CheckMoveEncouraged
    IfBattlerDealsMoreDamage AI_BATTLER_DEFENDER, USE_MIN_DAMAGE, Expert_Copycat_TryScorePlus2
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    IfLoadedNotInTable Expert_Copycat_EncouragedMoves, Expert_Copycat_CheckMoveEncouraged
    IfRandomLessThan 128, Expert_Copycat_End
    AddToMoveScore 2
    GoTo Expert_Copycat_End

Expert_Copycat_TryScorePlus2:
    IfRandomLessThan 32, Expert_Copycat_End
    AddToMoveScore 2
    GoTo Expert_Copycat_End

Expert_Copycat_CheckMoveEncouraged:
    IfBattlerDealsMoreDamage AI_BATTLER_DEFENDER, USE_MIN_DAMAGE, Expert_Copycat_End
    LoadBattlerPreviousMove AI_BATTLER_DEFENDER
    IfLoadedInTable Expert_Copycat_EncouragedMoves, Expert_Copycat_End
    IfRandomLessThan 80, Expert_Copycat_End
    AddToMoveScore -1

Expert_Copycat_End:
    End

Expert_Copycat_EncouragedMoves:
    TableEntry MOVE_SLEEP_POWDER
    TableEntry MOVE_LOVELY_KISS
    TableEntry MOVE_SPORE
    TableEntry MOVE_HYPNOSIS
    TableEntry MOVE_SING
    TableEntry MOVE_GRASS_WHISTLE
    TableEntry MOVE_SHADOW_PUNCH
    TableEntry MOVE_SAND_ATTACK
    TableEntry MOVE_SMOKE_SCREEN
    TableEntry MOVE_TOXIC
    TableEntry MOVE_GUILLOTINE
    TableEntry MOVE_HORN_DRILL
    TableEntry MOVE_FISSURE
    TableEntry MOVE_SHEER_COLD
    TableEntry MOVE_CROSS_CHOP
    TableEntry MOVE_AEROBLAST
    TableEntry MOVE_CONFUSE_RAY
    TableEntry MOVE_SWEET_KISS
    TableEntry MOVE_SCREECH
    TableEntry MOVE_COTTON_SPORE
    TableEntry MOVE_SCARY_FACE
    TableEntry MOVE_FAKE_TEARS
    TableEntry MOVE_METAL_SOUND
    TableEntry MOVE_THUNDER_WAVE
    TableEntry MOVE_GLARE
    TableEntry MOVE_POISON_POWDER
    TableEntry MOVE_SHADOW_BALL
    TableEntry MOVE_DYNAMIC_PUNCH
    TableEntry MOVE_HYPER_BEAM
    TableEntry MOVE_EXTREME_SPEED
    TableEntry MOVE_THIEF
    TableEntry MOVE_COVET
    TableEntry MOVE_ATTRACT
    TableEntry MOVE_SWAGGER
    TableEntry MOVE_TORMENT
    TableEntry MOVE_FLATTER
    TableEntry MOVE_TRICK
    TableEntry MOVE_SUPERPOWER
    TableEntry MOVE_SKILL_SWAP
    TableEntry MOVE_PSYCHO_SHIFT
    TableEntry MOVE_POWER_SWAP
    TableEntry MOVE_GUARD_SWAP
    TableEntry MOVE_SUCKER_PUNCH
    TableEntry MOVE_HEART_SWAP
    TableEntry MOVE_SWITCHEROO
    TableEntry MOVE_CAPTIVATE
    TableEntry MOVE_DARK_VOID
    TableEntry TABLE_END

Expert_PowerSwap:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE
    IfLoadedGreaterThan 3, Expert_PowerSwap_CheckSpAttack_HighDiff
    IfLoadedGreaterThan 1, Expert_PowerSwap_CheckSpAttack_MediumDiff
    IfLoadedGreaterThan 0, Expert_PowerSwap_CheckSpAttack_LowDiff
    IfLoadedEqualTo 0, Expert_PowerSwap_CheckSpAttack_NoDiff
    GoTo Expert_PowerSwap_CheckSpAttack_End

Expert_PowerSwap_CheckSpAttack_HighDiff:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE
    IfLoadedGreaterThan 3, Expert_PowerSwap_TryScorePlus5
    IfLoadedGreaterThan 1, Expert_PowerSwap_TryScorePlus4
    IfLoadedEqualTo 0, Expert_PowerSwap_TryScorePlus3
    GoTo Expert_PowerSwap_CheckSpAttack_End

Expert_PowerSwap_CheckSpAttack_MediumDiff:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE
    IfLoadedGreaterThan 3, Expert_PowerSwap_TryScorePlus4
    IfLoadedGreaterThan 1, Expert_PowerSwap_TryScorePlus3
    IfLoadedEqualTo 0, Expert_PowerSwap_TryScorePlus2
    GoTo Expert_PowerSwap_CheckSpAttack_End

Expert_PowerSwap_CheckSpAttack_LowDiff:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE
    IfLoadedGreaterThan 3, Expert_PowerSwap_TryScorePlus3
    IfLoadedGreaterThan 1, Expert_PowerSwap_TryScorePlus2
    IfLoadedEqualTo 0, Expert_PowerSwap_TryScorePlus1
    GoTo Expert_PowerSwap_CheckSpAttack_End

Expert_PowerSwap_CheckSpAttack_NoDiff:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE
    IfLoadedGreaterThan 3, Expert_PowerSwap_TryScorePlus3
    IfLoadedGreaterThan 1, Expert_PowerSwap_TryScorePlus2
    IfLoadedGreaterThan 0, Expert_PowerSwap_TryScorePlus1
    GoTo Expert_PowerSwap_CheckSpAttack_End

Expert_PowerSwap_TryScorePlus5:
    IfRandomLessThan 128, Expert_PowerSwap_TryScorePlus4
    AddToMoveScore 5
    GoTo Expert_PowerSwap_CheckSpAttack_End

Expert_PowerSwap_TryScorePlus4:
    IfRandomLessThan 128, Expert_PowerSwap_TryScorePlus3
    AddToMoveScore 4
    GoTo Expert_PowerSwap_CheckSpAttack_End

Expert_PowerSwap_TryScorePlus3:
    IfRandomLessThan 128, Expert_PowerSwap_TryScorePlus2
    AddToMoveScore 3
    GoTo Expert_PowerSwap_CheckSpAttack_End

Expert_PowerSwap_TryScorePlus2:
    IfRandomLessThan 128, Expert_PowerSwap_TryScorePlus1
    AddToMoveScore 2
    GoTo Expert_PowerSwap_CheckSpAttack_End

Expert_PowerSwap_TryScorePlus1:
    IfRandomLessThan 128, Expert_PowerSwap_CheckSpAttack_End
    AddToMoveScore 1
    GoTo Expert_PowerSwap_CheckSpAttack_End

Expert_PowerSwap_CheckSpAttack_End:
    End

Expert_GuardSwap:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE
    IfLoadedGreaterThan 3, Expert_GuardSwap_CheckSpDefense_HighDiff
    IfLoadedGreaterThan 1, Expert_GuardSwap_CheckSpDefense_MediumDiff
    IfLoadedGreaterThan 0, Expert_GuardSwap_CheckSpDefense_LowDiff
    IfLoadedEqualTo 0, Expert_GuardSwap_CheckSpDefense_NoDiff
    GoTo Expert_GuardSwap_End

Expert_GuardSwap_CheckSpDefense_HighDiff:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE
    IfLoadedGreaterThan 3, Expert_GuardSwap_TryScorePlus5
    IfLoadedGreaterThan 1, Expert_GuardSwap_TryScorePlus4
    IfLoadedEqualTo 0, Expert_GuardSwap_TryScorePlus3
    GoTo Expert_GuardSwap_End

Expert_GuardSwap_CheckSpDefense_MediumDiff:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE
    IfLoadedGreaterThan 3, Expert_GuardSwap_TryScorePlus4
    IfLoadedGreaterThan 1, Expert_GuardSwap_TryScorePlus3
    IfLoadedEqualTo 0, Expert_GuardSwap_TryScorePlus2
    GoTo Expert_GuardSwap_End

Expert_GuardSwap_CheckSpDefense_LowDiff:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE
    IfLoadedGreaterThan 3, Expert_GuardSwap_TryScorePlus3
    IfLoadedGreaterThan 1, Expert_GuardSwap_TryScorePlus2
    IfLoadedEqualTo 0, Expert_GuardSwap_TryScorePlus1
    GoTo Expert_GuardSwap_End

Expert_GuardSwap_CheckSpDefense_NoDiff:
    DiffStatStages AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE
    IfLoadedGreaterThan 3, Expert_GuardSwap_TryScorePlus3
    IfLoadedGreaterThan 1, Expert_GuardSwap_TryScorePlus2
    IfLoadedGreaterThan 0, Expert_GuardSwap_TryScorePlus1
    GoTo Expert_GuardSwap_End

Expert_GuardSwap_TryScorePlus5:
    IfRandomLessThan 128, Expert_GuardSwap_TryScorePlus4
    AddToMoveScore 5
    GoTo Expert_GuardSwap_End

Expert_GuardSwap_TryScorePlus4:
    IfRandomLessThan 128, Expert_GuardSwap_TryScorePlus3
    AddToMoveScore 4
    GoTo Expert_GuardSwap_End

Expert_GuardSwap_TryScorePlus3:
    IfRandomLessThan 128, Expert_GuardSwap_TryScorePlus2
    AddToMoveScore 3
    GoTo Expert_GuardSwap_End

Expert_GuardSwap_TryScorePlus2:
    IfRandomLessThan 128, Expert_GuardSwap_TryScorePlus1
    AddToMoveScore 2
    GoTo Expert_GuardSwap_End

Expert_GuardSwap_TryScorePlus1:
    IfRandomLessThan 128, Expert_GuardSwap_End
    AddToMoveScore 1
    GoTo Expert_GuardSwap_End

Expert_GuardSwap_End:
    End

Expert_Punishment:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_Punishment_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_Punishment_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_Punishment_End
    SumPositiveStatStages AI_BATTLER_DEFENDER
    IfLoadedGreaterThan 6, Expert_Punishment_TryScorePlus4
    IfLoadedGreaterThan 5, Expert_Punishment_TryScorePlus3
    IfLoadedGreaterThan 4, Expert_Punishment_TryScorePlus2
    IfLoadedGreaterThan 3, Expert_Punishment_TryScorePlus1
    IfLoadedGreaterThan 2, Expert_Punishment_TryScorePlus1
    GoTo Expert_Punishment_End

Expert_Punishment_TryScorePlus4:
    IfRandomLessThan 128, Expert_Punishment_TryScorePlus3
    AddToMoveScore 4

Expert_Punishment_TryScorePlus3:
    IfRandomLessThan 128, Expert_Punishment_TryScorePlus2
    AddToMoveScore 3

Expert_Punishment_TryScorePlus2:
    IfRandomLessThan 128, Expert_Punishment_TryScorePlus1
    AddToMoveScore 2

Expert_Punishment_TryScorePlus1:
    IfRandomLessThan 128, Expert_Punishment_End
    AddToMoveScore 1

Expert_Punishment_End:
    End

Expert_LastResort:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_LastResort_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_LastResort_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_LastResort_ScoreMinus1
    IfCanUseLastResort AI_BATTLER_ATTACKER, Expert_LastResort_ScorePlus1
    GoTo Expert_LastResort_End

Expert_LastResort_ScoreMinus1:
    AddToMoveScore -1
    GoTo Expert_LastResort_End

Expert_LastResort_ScorePlus1:
    AddToMoveScore 1

Expert_LastResort_End:
    End

Expert_WorrySeed:
    IfMoveNotKnown AI_BATTLER_DEFENDER, MOVE_REST, Expert_WorrySeed_CheckUserHP
    AddToMoveScore 1

Expert_WorrySeed_CheckUserHP:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 50, Expert_WorrySeed_TryScorePlus1
    IfRandomLessThan 128, Expert_WorrySeed_TryScorePlus1
    AddToMoveScore 1

Expert_WorrySeed_TryScorePlus1:
    IfRandomLessThan 64, Expert_WorrySeed_End
    AddToMoveScore 1
    GoTo Expert_WorrySeed_End

Expert_WorrySeed_End:
    End

Expert_SuckerPunch:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_SuckerPunch_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_SuckerPunch_ScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_SuckerPunch_ScoreMinus1
    IfRandomLessThan 64, Expert_SuckerPunch_End
    AddToMoveScore 1
    GoTo Expert_SuckerPunch_End

Expert_SuckerPunch_ScoreMinus1:
    AddToMoveScore -1

Expert_SuckerPunch_End:
    End

Expert_ToxicSpikes:
    IfRandomLessThan 128, Expert_ToxicSpikes_End
    AddToMoveScore 1
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_ROAR, Expert_ToxicSpikes_TryScorePlus1
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_WHIRLWIND, Expert_ToxicSpikes_TryScorePlus1
    GoTo Expert_ToxicSpikes_End

Expert_ToxicSpikes_TryScorePlus1:
    IfRandomLessThan 64, Expert_ToxicSpikes_End
    AddToMoveScore 1

Expert_ToxicSpikes_End:
    End

Expert_HeartSwap:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 7, Expert_HeartSwap_CheckUserStages
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 7, Expert_HeartSwap_CheckUserStages
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 7, Expert_HeartSwap_CheckUserStages
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 7, Expert_HeartSwap_CheckUserStages
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 7, Expert_HeartSwap_CheckUserStages
    IfConditionFlag AI_BATTLER_DEFENDER, 9, Expert_HeartSwap_CheckUserStages
    GoTo Expert_HeartSwap_ScoreMinus2

Expert_HeartSwap_CheckUserStages:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 7, Expert_HeartSwap_ScorePlus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 7, Expert_HeartSwap_ScorePlus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 7, Expert_HeartSwap_ScorePlus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 7, Expert_HeartSwap_ScorePlus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_EVASION_STAGE, 7, Expert_HeartSwap_ScorePlus2
    IfNotConditionFlag AI_BATTLER_ATTACKER, 9, Expert_HeartSwap_ScorePlus1
    IfRandomLessThan 50, Expert_HeartSwap_End
    GoTo Expert_HeartSwap_ScoreMinus2

Expert_HeartSwap_ScorePlus2:
    AddToMoveScore 1

Expert_HeartSwap_ScorePlus1:
    AddToMoveScore 1
    End

Expert_HeartSwap_ScoreMinus2:
    AddToMoveScore -2

Expert_HeartSwap_End:
    End

Expert_AquaRing:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 50, Expert_AquaRing_End
    IfRandomLessThan 128, Expert_AquaRing_End
    AddToMoveScore 1

Expert_AquaRing_End:
    End

Expert_MagnetRise:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 50, Expert_MagnetRise_End
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_EARTHQUAKE, Expert_MagnetRise_InitialScorePlus1
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_EARTH_POWER, Expert_MagnetRise_InitialScorePlus1
    IfMoveKnown AI_BATTLER_DEFENDER, MOVE_FISSURE, Expert_MagnetRise_InitialScorePlus1
    GoTo Expert_MagnetRise_CheckOpponentTyping

Expert_MagnetRise_InitialScorePlus1:
    AddToMoveScore 1

Expert_MagnetRise_CheckOpponentTyping:
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_GROUND, Expert_MagnetRise_ScorePlus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_GROUND, Expert_MagnetRise_ScorePlus1
    IfRandomLessThan 128, Expert_MagnetRise_End

Expert_MagnetRise_ScorePlus1:
    AddToMoveScore 1

Expert_MagnetRise_End:
    End
    End
    End
    End
    End

Expert_Defog:
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_LIGHT_SCREEN, Expert_Defog_ScreenScrubbing
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_REFLECT, Expert_Defog_ScreenScrubbing
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SPIKES, Expert_Defog_ScoreMinus2AndEnd
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_STEALTH_ROCK, Expert_Defog_ScoreMinus2AndEnd
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_TOXIC_SPIKES, Expert_Defog_ScoreMinus2AndEnd
    GoTo Expert_Defog_CheckUserHPAndOpponentEvasion

Expert_Defog_ScreenScrubbing:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 30, Expert_Defog_ScreenScrubbingCheckHazards
    CountAlivePartyBattlers AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, Expert_Defog_TryScoreMinus2

Expert_Defog_ScreenScrubbingCheckHazards:
    AddToMoveScore 1
    CountAlivePartyBattlers AI_BATTLER_DEFENDER
    IfLoadedEqualTo 0, Expert_Defog_End
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_SPIKES, Expert_Defog_TryScoreMinus1
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_STEALTH_ROCK, Expert_Defog_TryScoreMinus1
    IfSideCondition AI_BATTLER_DEFENDER, SIDE_CONDITION_TOXIC_SPIKES, Expert_Defog_TryScoreMinus1
    GoTo Expert_Defog_CheckUserHPAndOpponentEvasion

Expert_Defog_ScoreMinus2AndEnd:
    AddToMoveScore -2
    GoTo Expert_Defog_CheckUserHPAndOpponentEvasion

Expert_Defog_TryScoreMinus1:
    IfRandomLessThan 128, Expert_Defog_CheckUserHPAndOpponentEvasion
    AddToMoveScore -1
    GoTo Expert_Defog_CheckUserHPAndOpponentEvasion

Expert_Defog_CheckUserHPAndOpponentEvasion:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 70, Expert_Defog_TryScoreMinus2
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 3, Expert_Defog_CheckOpponentHP

Expert_Defog_TryScoreMinus2:
    IfRandomLessThan 50, Expert_Defog_CheckOpponentHP
    AddToMoveScore -2

Expert_Defog_CheckOpponentHP:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, Expert_Defog_End
    AddToMoveScore -2

Expert_Defog_End:
    End

Expert_TrickRoom:
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_DOUBLE, Expert_TrickRoom_End
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 30, Expert_TrickRoom_CheckSpeed
    CountAlivePartyBattlers AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, Expert_TrickRoom_End

Expert_TrickRoom_CheckSpeed:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_TrickRoom_TryScorePlus3
    AddToMoveScore -1
    GoTo Expert_TrickRoom_End

Expert_TrickRoom_TryScorePlus3:
    IfRandomLessThan 64, Expert_TrickRoom_End
    AddToMoveScore 3

Expert_TrickRoom_End:
    End

Expert_Blizzard:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_Blizzard_TryScoreMinus3
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_Blizzard_TryScoreMinus3
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_Blizzard_TryScoreMinus3
    LoadCurrentWeather
    IfLoadedNotEqualTo BTL_WEATHER_HAIL, Expert_Blizzard_End
    AddToMoveScore 1
    GoTo Expert_Blizzard_End

Expert_Blizzard_TryScoreMinus3:
    IfRandomLessThan 50, Expert_Blizzard_End
    AddToMoveScore -3

Expert_Blizzard_End:
    End
    End

Expert_Captivate:
    IfStatStageEqualTo AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 6, Expert_Captivate_CheckOpponentHP
    AddToMoveScore -1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 90, Expert_Captivate_CheckLowStatStage
    AddToMoveScore -1

Expert_Captivate_CheckLowStatStage:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 3, Expert_Captivate_CheckOpponentHP
    IfRandomLessThan 50, Expert_Captivate_CheckOpponentHP
    AddToMoveScore -2

Expert_Captivate_CheckOpponentHP:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 70, Expert_Captivate_CheckOpponentLastMove
    AddToMoveScore -2

Expert_Captivate_CheckOpponentLastMove:
    LoadDefenderLastUsedMoveClass
    IfLoadedNotEqualTo MOVE_CATEGORY_PHYSICAL, Expert_Captivate_End
    IfRandomLessThan 64, Expert_Captivate_End
    AddToMoveScore -1

Expert_Captivate_End:
    End

Expert_StealthRock:
    IfRandomLessThan 128, Expert_StealthRock_End
    AddToMoveScore 1
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_ROAR, Expert_StealthRock_TryScorePlus1
    IfMoveKnown AI_BATTLER_ATTACKER, MOVE_WHIRLWIND, Expert_StealthRock_TryScorePlus1
    GoTo Expert_StealthRock_End

Expert_StealthRock_TryScorePlus1:
    IfRandomLessThan 64, Expert_StealthRock_End
    AddToMoveScore 1

Expert_StealthRock_End:
    End
    End
    End

Expert_RecoilMove:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_RecoilMove_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_RecoilMove_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_RecoilMove_End
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_ROCK_HEAD, Expert_RecoilMove_ScorePlus1
    IfLoadedEqualTo ABILITY_MAGIC_GUARD, Expert_RecoilMove_ScorePlus1
    GoTo Expert_RecoilMove_End

Expert_RecoilMove_ScorePlus1:
    AddToMoveScore 1

Expert_RecoilMove_End:
    End

Expert_HealingWish:
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 80, Expert_HealingWish_HappyPath
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_HealingWish_HappyPath
    IfRandomLessThan 192, Expert_HealingWish_End
    GoTo ScoreMinus5

Expert_HealingWish_HappyPath:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_HealingWish_TryScoreMinus1
    IfRandomLessThan 192, Expert_HealingWish_CheckUserAtLowHP
    AddToMoveScore 1
    IfHasSuperEffectiveMove Expert_HealingWish_CheckPartyMemberDamage
    IfRandomLessThan 192, Expert_HealingWish_CheckPartyMemberDamage
    AddToMoveScore 1

Expert_HealingWish_CheckPartyMemberDamage:
    IfPartyMemberDealsMoreDamage USE_MIN_DAMAGE, Expert_HealingWish_TryScorePlus1
    GoTo Expert_HealingWish_CheckUserAtLowHP

Expert_HealingWish_TryScorePlus1:
    IfRandomLessThan 128, Expert_HealingWish_CheckUserAtLowHP
    AddToMoveScore 1

Expert_HealingWish_CheckUserAtLowHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 30, Expert_HealingWish_End
    IfRandomLessThan 128, Expert_HealingWish_End
    AddToMoveScore 1
    GoTo Expert_HealingWish_End

Expert_HealingWish_TryScoreMinus1:
    IfRandomLessThan 50, Expert_HealingWish_End
    AddToMoveScore -1

Expert_HealingWish_End:
    End

Expert_HoneClaws:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 9, Expert_HoneClaws_CheckHP
    IfRandomLessThan 50, Expert_HoneClaws_CheckHP
    AddToMoveScore -2

Expert_HoneClaws_CheckHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 80, Expert_HoneClaws_End
    AddToMoveScore -2

Expert_HoneClaws_End:
    End

Expert_WideGuard:
    End

Expert_GuardSplit:
    LoadIsFirstTurnInBattle AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo FALSE, Expert_GuardSplit_ScoreMinus1
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 7, Expert_GuardSplit_ScoreMinus1
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 7, Expert_GuardSplit_ScoreMinus1
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 8, Expert_GuardSplit_CheckSpecies
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 8, Expert_GuardSplit_CheckSpecies
    IfRandomLessThan 50, Expert_GuardSplit_CheckTargetStatStages
    AddToMoveScore 1

Expert_GuardSplit_CheckTargetStatStages:
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 10, Expert_GuardSplit_CheckSpecies
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 10, Expert_GuardSplit_CheckSpecies
    IfRandomLessThan 50, Expert_GuardSplit_CheckSpecies
    AddToMoveScore 1

Expert_GuardSplit_CheckSpecies:
    LoadSpecies AI_BATTLER_ATTACKER
    IfLoadedInTable Expert_GuardSplit_HighDefenseSpecies, Expert_GuardSplit_CheckBattleStyle
    IfLoadedInTable Expert_GuardSplit_HighSpDefenseSpecies, Expert_GuardSplit_CheckBattleStyle
    LoadSpecies AI_BATTLER_DEFENDER
    IfLoadedInTable Expert_GuardSplit_HighDefenseSpecies, Expert_GuardSplit_TryScorePlus1
    IfLoadedInTable Expert_GuardSplit_HighSpDefenseSpecies, Expert_GuardSplit_TryScorePlus1
    GoTo Expert_GuardSplit_End

Expert_GuardSplit_CheckBattleStyle:
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, ScoreMinus10
    GoTo Expert_GuardSplit_End

Expert_GuardSplit_TryScorePlus1:
    IfRandomLessThan 50, Expert_GuardSplit_End
    AddToMoveScore 1
    GoTo Expert_GuardSplit_End

Expert_GuardSplit_ScoreMinus1:
    AddToMoveScore -1

Expert_GuardSplit_End:
    End

Expert_GuardSplit_HighDefenseSpecies:
    TableEntry SPECIES_SHUCKLE
    TableEntry SPECIES_REGIROCK
    TableEntry SPECIES_STEELIX
    TableEntry SPECIES_AGGRON
    TableEntry SPECIES_CLOYSTER
    TableEntry SPECIES_BASTIODON
    TableEntry SPECIES_ONIX
    TableEntry SPECIES_REGISTEEL
    TableEntry SPECIES_PROBOPASS
    TableEntry SPECIES_GROUDON
    TableEntry SPECIES_TORKOAL
    TableEntry SPECIES_LAIRON
    TableEntry SPECIES_SKARMORY
    TableEntry SPECIES_FORRETRESS
    TableEntry SPECIES_DUSKNOIR
    TableEntry SPECIES_NOSEPASS
    TableEntry SPECIES_UXIE
    TableEntry SPECIES_LEAFEON
    TableEntry SPECIES_RHYPERIOR
    TableEntry SPECIES_METAGROSS
    TableEntry SPECIES_RELICANTH
    TableEntry SPECIES_DUSCLOPS
    TableEntry SPECIES_LUGIA
    TableEntry SPECIES_GOLEM
    TableEntry SPECIES_COFAGRIGUS
    TableEntry SPECIES_CARRACOSTA
    TableEntry SPECIES_FERROTHORN
    TableEntry SPECIES_GIGALITH
    TableEntry SPECIES_COBALION
    TableEntry TABLE_END

Expert_GuardSplit_HighSpDefenseSpecies:
    TableEntry SPECIES_SHUCKLE
    TableEntry SPECIES_REGICE
    TableEntry SPECIES_HO_OH
    TableEntry SPECIES_LUGIA
    TableEntry SPECIES_PROBOPASS
    TableEntry SPECIES_REGISTEEL
    TableEntry SPECIES_KYOGRE
    TableEntry SPECIES_MANTINE
    TableEntry SPECIES_BASTIODON
    TableEntry SPECIES_DUSKNOIR
    TableEntry SPECIES_BLISSEY
    TableEntry SPECIES_CRESSELIA
    TableEntry SPECIES_UXIE
    TableEntry SPECIES_LATIAS
    TableEntry SPECIES_DUSCLOPS
    TableEntry SPECIES_UMBREON
    TableEntry SPECIES_MILOTIC
    TableEntry SPECIES_ARTICUNO
    TableEntry SPECIES_CRYOGONAL
    TableEntry SPECIES_VIRIZION
    TableEntry TABLE_END

Expert_PowerSplit:
    LoadIsFirstTurnInBattle AI_BATTLER_ATTACKER
    IfLoadedNotEqualTo FALSE, Expert_PowerSplit_ScoreMinus1
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 7, Expert_PowerSplit_ScoreMinus1
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 7, Expert_PowerSplit_ScoreMinus1
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 8, Expert_PowerSplit_CheckSpecies
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 8, Expert_PowerSplit_CheckSpecies
    IfRandomLessThan 50, Expert_PowerSplit_CheckTargetStatStages
    AddToMoveScore 1

Expert_PowerSplit_CheckTargetStatStages:
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 10, Expert_PowerSplit_CheckSpecies
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 10, Expert_PowerSplit_CheckSpecies
    IfRandomLessThan 50, Expert_PowerSplit_CheckSpecies
    AddToMoveScore 1

Expert_PowerSplit_CheckSpecies:
    LoadSpecies AI_BATTLER_ATTACKER
    IfLoadedInTable Expert_PowerSplit_HighAttackSpecies, Expert_PowerSplit_CheckBattleStyle
    IfLoadedInTable Expert_PowerSplit_HighSpAttackSpecies, Expert_PowerSplit_CheckBattleStyle
    LoadSpecies AI_BATTLER_DEFENDER
    IfLoadedInTable Expert_PowerSplit_HighAttackSpecies, Expert_PowerSplit_TryScorePlus1
    IfLoadedInTable Expert_PowerSplit_HighSpAttackSpecies, Expert_PowerSplit_TryScorePlus1
    GoTo Expert_PowerSplit_End

Expert_PowerSplit_CheckBattleStyle:
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, ScoreMinus10
    GoTo Expert_PowerSplit_End

Expert_PowerSplit_TryScorePlus1:
    IfRandomLessThan 50, Expert_PowerSplit_End
    AddToMoveScore 1
    GoTo Expert_PowerSplit_End

Expert_PowerSplit_ScoreMinus1:
    AddToMoveScore -1

Expert_PowerSplit_End:
    End

Expert_PowerSplit_HighAttackSpecies:
    TableEntry SPECIES_RAMPARDOS
    TableEntry SPECIES_REGIGIGAS
    TableEntry SPECIES_SLAKING
    TableEntry SPECIES_DEOXYS
    TableEntry SPECIES_RAYQUAZA
    TableEntry SPECIES_GROUDON
    TableEntry SPECIES_RHYPERIOR
    TableEntry SPECIES_METAGROSS
    TableEntry SPECIES_SALAMENCE
    TableEntry SPECIES_TYRANITAR
    TableEntry SPECIES_DRAGONITE
    TableEntry SPECIES_MAMOSWINE
    TableEntry SPECIES_GARCHOMP
    TableEntry SPECIES_ABSOL
    TableEntry SPECIES_BRELOOM
    TableEntry SPECIES_HO_OH
    TableEntry SPECIES_URSARING
    TableEntry SPECIES_SCIZOR
    TableEntry SPECIES_FLAREON
    TableEntry SPECIES_RHYDON
    TableEntry SPECIES_KINGLER
    TableEntry SPECIES_MACHAMP
    TableEntry SPECIES_RESHIRAM
    TableEntry SPECIES_LIEPARD
    TableEntry SPECIES_CONKELDURR
    TableEntry SPECIES_DARMANITAN
    TableEntry SPECIES_ARCHEOPS
    TableEntry SPECIES_EXCADRILL
    TableEntry SPECIES_ESCAVALIER
    TableEntry SPECIES_KYUREM
    TableEntry SPECIES_GIGALITH
    TableEntry SPECIES_TERRAKION
    TableEntry TABLE_END

Expert_PowerSplit_HighSpAttackSpecies:
    TableEntry SPECIES_MEWTWO
    TableEntry SPECIES_PALKIA
    TableEntry SPECIES_DIALGA
    TableEntry SPECIES_DEOXYS
    TableEntry SPECIES_RAYQUAZA
    TableEntry SPECIES_KYOGRE
    TableEntry SPECIES_DARKRAI
    TableEntry SPECIES_PORYGON_Z
    TableEntry SPECIES_ALAKAZAM
    TableEntry SPECIES_HEATRAN
    TableEntry SPECIES_GLACEON
    TableEntry SPECIES_MAGNEZONE
    TableEntry SPECIES_LATIOS
    TableEntry SPECIES_ESPEON
    TableEntry SPECIES_GENGAR
    TableEntry SPECIES_RESHIRAM
    TableEntry SPECIES_CHANDELURE
    TableEntry SPECIES_VOLCARONA
    TableEntry SPECIES_KYUREM
    TableEntry SPECIES_KELDEO
    TableEntry TABLE_END

Expert_WonderRoom:
    End

Expert_Psyshock:
    End

Expert_Venoshock:
    End

Expert_Telekinesis:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ACCURACY_STAGE, 9, Expert_Telekinesis_CheckHP
    IfRandomLessThan 50, Expert_Telekinesis_CheckHP
    AddToMoveScore -2

Expert_Telekinesis_CheckHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_Telekinesis_CheckEvasionStage
    IfRandomLessThan 50, Expert_Telekinesis_CheckEvasionStage
    AddToMoveScore -2

Expert_Telekinesis_CheckEvasionStage:
    IfStatStageLessThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 9, Expert_Telekinesis_End
    IfRandomLessThan 50, Expert_Telekinesis_End
    AddToMoveScore 1

Expert_Telekinesis_End:
    End

Expert_MagicRoom:
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_NONE, Expert_MagicRoom_TryScorePlus1
    IfHeldItemEqualTo AI_BATTLER_DEFENDER, ITEM_LEFTOVERS, Expert_MagicRoom_TryScorePlus1
    IfHeldItemEqualTo AI_BATTLER_DEFENDER, ITEM_CHOICE_SCARF, Expert_MagicRoom_TryScorePlus1
    LoadBattleStyle
    IfLoadedEqualTo BTL_STYLE_SINGLE, Expert_MagicRoom_End
    IfHeldItemEqualTo AI_BATTLER_DEFENDER, ITEM_AIR_BALLOON, Expert_MagicRoom_TryScorePlus1
    GoTo Expert_MagicRoom_End

Expert_MagicRoom_TryScorePlus1:
    IfRandomLessThan 128, Expert_MagicRoom_End
    AddToMoveScore 1

Expert_MagicRoom_End:
    End

Expert_SmackDown:
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_LEVITATE, Expert_SmackDown_TryScorePlus1
    IfCondition AI_BATTLER_DEFENDER, 30, Expert_SmackDown_TryScorePlus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_1
    IfLoadedEqualTo TYPE_FLYING, Expert_SmackDown_TryScorePlus1
    LoadTypeFrom LOAD_DEFENDER_TYPE_2
    IfLoadedEqualTo TYPE_FLYING, Expert_SmackDown_TryScorePlus1
    GoTo Expert_SmackDown_End

Expert_SmackDown_TryScorePlus1:
    IfRandomLessThan 64, Expert_SmackDown_End
    AddToMoveScore 1

Expert_SmackDown_End:
    End

Expert_StormThrow:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_StormThrow_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_StormThrow_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_StormThrow_End
    IfMoveEqualTo MOVE_STORM_THROW, Expert_StormThrow_CheckDefenseStage
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 8, Expert_StormThrow_End
    GoTo Expert_StormThrow_TryScorePlus1

Expert_StormThrow_CheckDefenseStage:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 8, Expert_StormThrow_End

Expert_StormThrow_TryScorePlus1:
    IfRandomLessThan 64, Expert_StormThrow_End
    AddToMoveScore 1

Expert_StormThrow_End:
    End

Expert_FlameBurst:
    End

Expert_QuiverDance:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_QuiverDance_TryScorePlus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_QuiverDance_End
    IfRandomLessThan 50, Expert_QuiverDance_End
    AddToMoveScore -1
    GoTo Expert_QuiverDance_End

Expert_QuiverDance_TryScorePlus1:
    IfRandomLessThan 50, Expert_QuiverDance_End
    AddToMoveScore 1

Expert_QuiverDance_End:
    End

Expert_HeavySlam:
    End

Expert_Synchronoise:
    End

Expert_ElectroBall:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_ElectroBall_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_ElectroBall_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_ElectroBall_End
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_ElectroBall_ScoreMinus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SPEED_STAGE, 8, Expert_ElectroBall_End
    IfRandomLessThan 70, Expert_ElectroBall_End
    AddToMoveScore 1
    GoTo Expert_QuiverDance_End

Expert_ElectroBall_ScoreMinus1:
    AddToMoveScore -1

Expert_ElectroBall_End:
    End

Expert_Soak:
    End

Expert_FlameCharge:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_FlameCharge_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_FlameCharge_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_FlameCharge_End
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_FlameCharge_TryScorePlus1
    IfRandomLessThan 50, Expert_FlameCharge_End
    AddToMoveScore -1
    GoTo Expert_FlameCharge_End

Expert_FlameCharge_TryScorePlus1:
    IfRandomLessThan 50, Expert_FlameCharge_End
    AddToMoveScore 1

Expert_FlameCharge_End:
    End

Expert_AcidSpray:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 5, Expert_AcidSpray_End
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 3, Expert_AcidSpray_TryScoreMinus1
    AddToMoveScore -1

Expert_AcidSpray_TryScoreMinus1:
    IfRandomLessThan 128, Expert_AcidSpray_End
    AddToMoveScore -1

Expert_AcidSpray_End:
    End

Expert_FoulPlay:
    End

Expert_Entrainment:
    IfRandomLessThan 128, Expert_Entrainment_End
    AddToMoveScore 1

Expert_Entrainment_End:
    End

Expert_AfterYou:
    End

Expert_Round:
    End

Expert_EchoedVoice:
    End

Expert_ChipAway:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_ChipAway_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_ChipAway_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_ChipAway_End
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 9, Expert_ChipAway_End
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 9, Expert_ChipAway_End
    IfRandomLessThan 128, Expert_ChipAway_End
    AddToMoveScore 1

Expert_ChipAway_End:
    End

Expert_ClearSmog:
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_ATTACK_STAGE, 8, Expert_ClearSmog_TryScorePlus1
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_DEFENSE_STAGE, 8, Expert_ClearSmog_TryScorePlus1
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_ATTACK_STAGE, 8, Expert_ClearSmog_TryScorePlus1
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_SP_DEFENSE_STAGE, 8, Expert_ClearSmog_TryScorePlus1
    IfStatStageGreaterThan AI_BATTLER_DEFENDER, BATTLEMON_EVASION_STAGE, 8, Expert_ClearSmog_TryScorePlus1
    IfRandomLessThan 50, Expert_ClearSmog_End
    AddToMoveScore -1
    GoTo Expert_ClearSmog_End

Expert_ClearSmog_TryScorePlus1:
    IfRandomLessThan 50, Expert_ClearSmog_End
    AddToMoveScore 1

Expert_ClearSmog_End:
    End

Expert_StoredPower:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_ChipAway_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_ChipAway_End
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_ChipAway_End
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 8, Expert_StoredPower_CheckBoostingMoves
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 8, Expert_StoredPower_CheckBoostingMoves
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 8, Expert_StoredPower_CheckBoostingMoves
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 8, Expert_StoredPower_CheckBoostingMoves
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_EVASION_STAGE, 8, Expert_StoredPower_CheckBoostingMoves
    GoTo Expert_StoredPower_End

Expert_StoredPower_CheckBoostingMoves:
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_CURSE, Expert_StoredPower_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_DEF_SPD_UP, Expert_StoredPower_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_ATK_DEF_UP, Expert_StoredPower_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_SP_ATK_SP_DEF_UP, Expert_StoredPower_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_ATK_SPD_UP, Expert_StoredPower_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_RANDOM_STAT_UP_2, Expert_StoredPower_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_HONE_CLAWS, Expert_StoredPower_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_QUIVER_DANCE, Expert_StoredPower_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_SHELL_SMASH, Expert_StoredPower_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_GROWTH, Expert_StoredPower_TryScorePlus1
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_COIL, Expert_StoredPower_TryScorePlus1
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 10, Expert_StoredPower_TryScorePlus1
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 10, Expert_StoredPower_TryScorePlus1
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 10, Expert_StoredPower_TryScorePlus1
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 10, Expert_StoredPower_TryScorePlus1
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_EVASION_STAGE, 10, Expert_StoredPower_TryScorePlus1
    GoTo Expert_StoredPower_End

Expert_StoredPower_TryScorePlus1:
    IfRandomLessThan 50, Expert_StoredPower_End
    AddToMoveScore 1

Expert_StoredPower_End:
    End

Expert_QuickGuard:
    LoadIsFirstTurnInBattle AI_BATTLER_DEFENDER
    IfLoadedEqualTo FALSE, Expert_QuickGuard_TryScorePlus1
    AddToMoveScore -1

Expert_QuickGuard_TryScorePlus1:
    IfRandomLessThan 128, Expert_QuickGuard_End
    AddToMoveScore 1

Expert_QuickGuard_End:
    End

Expert_AllySwitch:
    End

Expert_ShellSmash:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_DEFENSE_STAGE, 6, Expert_ShellSmash_ScoreMinus1
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_DEFENSE_STAGE, 6, Expert_ShellSmash_ScoreMinus1
    GoTo Expert_ShellSmash_End

Expert_ShellSmash_ScoreMinus1:
    AddToMoveScore -1

Expert_ShellSmash_End:
    End

Expert_HealPulse:
    End

Expert_Hex:
    End

Expert_ShiftGear:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_ShiftGear_TryScorePlus1
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 50, Expert_ShiftGear_End
    IfRandomLessThan 50, Expert_ShiftGear_End
    AddToMoveScore -1
    GoTo Expert_ShiftGear_End

Expert_ShiftGear_TryScorePlus1:
    IfRandomLessThan 50, Expert_ShiftGear_End
    AddToMoveScore 1

Expert_ShiftGear_End:
    End

Expert_Incinerate:
    LoadTurnCount
    IfLoadedNotEqualTo 0, Expert_Incinerate_End
    IfRandomLessThan 50, Expert_Incinerate_End
    AddToMoveScore 1

Expert_Incinerate_End:
    End

Expert_Quash:
    End

Expert_Growth:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_Growth_End
    IfRandomLessThan 50, Expert_Growth_End
    AddToMoveScore -1

Expert_Growth_End:
    End

Expert_Acrobatics:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, Expert_Acrobatics_TryScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUARTER, Expert_Acrobatics_TryScoreMinus1
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_HALF, Expert_Acrobatics_TryScoreMinus1
    IfHeldItemEqualTo AI_BATTLER_ATTACKER, ITEM_NONE, Expert_Acrobatics_TryScorePlus1

Expert_Acrobatics_TryScoreMinus1:
    IfRandomLessThan 50, Expert_Acrobatics_End
    AddToMoveScore -1
    GoTo Expert_Acrobatics_End

Expert_Acrobatics_TryScorePlus1:
    IfRandomLessThan 50, Expert_Acrobatics_End
    AddToMoveScore 1

Expert_Acrobatics_End:
    End

Expert_ReflectType:
    LoadTypeFrom LOAD_ATTACKER_TYPE_1
    IfLoadedInTable Expert_ReflectType_TypesToCopy, Expert_ReflectType_TryScorePlus1
    IfLoadedInTable Expert_ReflectType_TypesToAvoid, Expert_ReflectType_TryScoreMinus1
    LoadTypeFrom LOAD_ATTACKER_TYPE_2
    IfLoadedInTable Expert_ReflectType_TypesToCopy, Expert_ReflectType_TryScorePlus1
    IfLoadedInTable Expert_ReflectType_TypesToAvoid, Expert_ReflectType_TryScoreMinus1
    GoTo Expert_ReflectType_End

Expert_ReflectType_TryScoreMinus1:
    IfRandomLessThan 50, Expert_ReflectType_End
    AddToMoveScore -1
    GoTo Expert_ReflectType_End

Expert_ReflectType_TryScorePlus1:
    IfRandomLessThan 50, Expert_ReflectType_End
    AddToMoveScore 1

Expert_ReflectType_End:
    End

Expert_ReflectType_TypesToCopy:
    TableEntry TYPE_FIRE
    TableEntry TYPE_WATER
    TableEntry TYPE_GRASS
    TableEntry TYPE_ELECTRIC
    TableEntry TYPE_ICE
    TableEntry TYPE_POISON
    TableEntry TYPE_PSYCHIC
    TableEntry TYPE_DARK
    TableEntry TYPE_STEEL
    TableEntry TABLE_END

Expert_ReflectType_TypesToAvoid:
    TableEntry TYPE_GHOST
    TableEntry TYPE_DRAGON
    TableEntry TABLE_END

Expert_Retaliate:
    End

Expert_FinalGambit:
    IfBattlerHasSubstitute AI_BATTLER_DEFENDER, Expert_FinalGambit_ScoreMinus2
    IfHasSuperEffectiveMove Expert_FinalGambit_ScoreMinus2
    IfHPPercentLessThan AI_BATTLER_DEFENDER, 40, Expert_FinalGambit_ScoreMinus2
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 40, Expert_FinalGambit_End
    IfRandomLessThan 50, Expert_FinalGambit_End

Expert_FinalGambit_ScoreMinus2:
    AddToMoveScore -2

Expert_FinalGambit_End:
    End

Expert_Bestow:
    End

Expert_WaterPledge:
    End

Expert_FirePledge:
    End

Expert_GrassPledge:
    End

Expert_WorkUp:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 9, Expert_WorkUp_CheckSpAttackStage
    IfRandomLessThan 100, Expert_WorkUp_ScorePlus2
    AddToMoveScore -1
    GoTo Expert_WorkUp_ScorePlus2

Expert_WorkUp_CheckSpAttackStage:
    IfStatStageLessThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 9, Expert_WorkUp_CheckFullHP
    IfRandomLessThan 100, Expert_WorkUp_ScorePlus2
    AddToMoveScore -1
    GoTo Expert_WorkUp_ScorePlus2

Expert_WorkUp_CheckFullHP:
    IfHPPercentNotEqualTo AI_BATTLER_ATTACKER, 100, Expert_WorkUp_CheckHP
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_PASS_STATS_AND_STATUS, Expert_WorkUp_ScorePlus2
    IfRandomLessThan 128, Expert_WorkUp_CheckHP

Expert_WorkUp_ScorePlus2:
    AddToMoveScore 2

Expert_WorkUp_CheckHP:
    IfHPPercentGreaterThan AI_BATTLER_ATTACKER, 70, Expert_WorkUp_End
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 40, Expert_WorkUp_ScoreMinus2
    IfRandomLessThan 40, Expert_WorkUp_End

Expert_WorkUp_ScoreMinus2:
    AddToMoveScore -2

Expert_WorkUp_End:
    End

Expert_RelicSong:
    End

Expert_Glaciate:
    IfSpeedCompareEqualTo COMPARE_SPEED_SLOWER, Expert_Glaciate_TryScorePlus2
    AddToMoveScore -1
    GoTo Expert_Glaciate_End

Expert_Glaciate_TryScorePlus2:
    IfRandomLessThan 70, Expert_Glaciate_End
    AddToMoveScore 2

Expert_Glaciate_End:
    End

Expert_Unused333:
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
