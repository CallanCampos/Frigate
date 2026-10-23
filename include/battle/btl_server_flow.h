#ifndef POKEBW2_BATTLE_BTL_SERVER_FLOW_H
#define POKEBW2_BATTLE_BTL_SERVER_FLOW_H

#include "types.h"
#include "battle/btl_handler.h"
#include "battle/btl_server.h"
#include "constants/battle.h"
#include "struct_decls.h"

// The stat that a move's stat change affects, as PML_MoveGetStatChangeStage returns it: one stat from 1, a
// BATTLEMON_*_STAGE, or MOVE_STAT_CHANGE_ALL for the five from attack to speed at once. An enum, which MWCC loads again
// where it is passed on after tests of it that share one load, as ServerControl_ApplyMoveStatChanges does
typedef enum {
    MOVE_STAT_CHANGE_NONE = 0,
    // Every stat, as the move data of Ancient Power, Ominous Wind and Silver Wind gives it and Clear Body guards
    // against: dropped after the move's handlers unless one of them turns it into MOVE_STAT_CHANGE_ALL
    MOVE_STAT_CHANGE_MULTI = 8,
    MOVE_STAT_CHANGE_ALL = 10,
} MoveStatChangeStat;

// The move that a move calls instead of itself, such as Metronome's, and its target
typedef struct {
    u16 move;
    u8 target;
} BtlFlowCalledMove;

// What a move's use reports back
typedef struct {
    u8 result;
    BtlFlowCalledMove called;
} BtlFlowFightWork;

// Walks the mons in battle of every client
typedef struct {
    u8 clientId;
    u8 index;
    u8 done;
    // Set in rotation battles, to walk all three slots
    u8 rotation;
} BtlFlowMonIter;

typedef struct {
    u32 multipleTargets : 1;
    u32 unk1 : 1;
    u32 unk2 : 30;
} BtlFlowDamageFlags;

BtlServerFlow *BtlServerFlow_Create(BtlServer *server, BtlMainModule *mainModule, BtlPokeCon *pokeCon,
                                    BtlServerCmdQueue *queue, u32 a4, HeapID heapId);
void BtlServerFlow_ResetSystem(BtlServerFlow *serverFlow);
void BtlServerFlow_Delete(BtlServerFlow *serverFlow);
BOOL BtlServerFlow_StartBattle(BtlServerFlow *serverFlow);
void BtlServerFlow_PrepareTurn(BtlServerFlow *serverFlow);
u32 BtlServerFlow_RunTurn(BtlServerFlow *serverFlow, BtlClientActions *clientActions);
void BtlServerFlow_PrepareMidTurnSwitch(BtlServerFlow *serverFlow);
u32 BtlServerFlow_RunMidTurnSwitch(BtlServerFlow *serverFlow, BtlClientActions *clientActions);
void BtlServerFlow_PrepareFaintSwitch(BtlServerFlow *serverFlow);
u32 BtlServerFlow_RunFaintSwitch(BtlServerFlow *serverFlow, BtlClientActions *clientActions);
u32 BtlServerFlow_TryEscape(BtlServerFlow *serverFlow);
BOOL func_ov167_0219fe24(BtlServerFlow *serverFlow);
BtlClientIDList *BtlServerFlow_GetClientIDList(BtlServerFlow *serverFlow);
u8 func_ov167_0219fff0(BtlServerFlow *serverFlow);
u32 BtlServerFlow_GetBattleResult(BtlServerFlow *serverFlow);

// The damage of a move, with the type effectiveness if withEffectiveness is set. damageRoll is USE_MIN_DAMAGE for the
// lowest random roll, or ROLL_FOR_DAMAGE for a random one
u32 AICalcDamage(BtlServerFlow *serverFlow, u8 attackerId, u8 defenderId, u16 move, BOOL withEffectiveness,
                 u32 damageRoll);
u16 GetTurnCounter(BtlServerFlow *serverFlow);
BattleMon *GetBattleMon(BtlServerFlow *serverFlow, u8 monId);
u8 BtlServerFlow_MonIDToPos(BtlServerFlow *serverFlow, u8 monId);
BOOL BtlServerFlow_GetPendingAction(BtlServerFlow *flow, u8 monId, BattleAction *action);
BOOL BtlServerFlow_IsLastToAct(BtlServerFlow *flow, u8 monId);
u16 BtlServerFlow_GetLastUsedMove(BtlServerFlow *flow);
BtlWazaRec *BtlServerFlow_GetWazaRec(BtlServerFlow *flow);
BtlFaintRec *BtlServerFlow_GetFaintRec(BtlServerFlow *flow);
BOOL BtlServerFlow_IsMonInBattle(BtlServerFlow *flow, u8 monId);
u32 BtlServerFlow_GetBattleType(BtlServerFlow *flow);
u32 GetBattleTerrain(BtlServerFlow *flow);
u32 BtlServerFlow_GetFrontPosCount(BtlServerFlow *flow);
BOOL CheckEvolution(BtlServerFlow *flow, u8 monId);
BOOL BtlServerFlow_CheckMatchup(BtlServerFlow *flow);
u8 BtlServerFlow_GetPosOfMon(BtlServerFlow *flow, u8 monId);
u8 BtlServerFlow_GetMonIDAtPos(BtlServerFlow *flow, u8 pos);
u8 BtlServerFlow_GetOpposingMonIDs(BtlServerFlow *flow, u8 monId, u8 *monIds);
u32 BtlServerFlow_GetBattleStyle(BtlServerFlow *flow);
BOOL BtlServerFlow_IsFloating(BtlServerFlow *flow, u8 monId);
BOOL BtlServerFlow_CanUseHeldItem(BtlServerFlow *flow, u8 monId);
BOOL BtlServerFlow_IsSideEffectSet(BtlServerFlow *flow, u8 side, u32 sideEffect);
u8 BtlServerFlow_GetAllyClientID(BtlServerFlow *flow, u8 clientId);
BOOL IsMonSwitchingOut(BtlServerFlow *flow);
void AddSwitchOutInterrupt(BtlServerFlow *flow, u8 monId);
BOOL BtlServerFlow_IsSemiInvuln(BtlServerFlow *flow, u8 monId);
u8 BtlServerFlow_PosToMonID(BtlServerFlow *flow, u8 pos);
u32 CalcMoveEffectiveness(BtlServerFlow *serverFlow, u8 attackerId, u8 defenderId, u16 move);
BOOL BtlServerFlow_IsSimulating(BtlServerFlow *flow);
u8 BtlServerFlow_GetChatterEffect(BtlServerFlow *flow, u8 monId);
u8 BtlServerFlow_GetClientCoverPosCount(BtlServerFlow *flow, u8 monId);
u8 BtlServerFlow_GetClientFrontSlotCount(BtlServerFlow *flow, u8 monId);
BOOL BtlServerFlow_HasReserveMon(BtlServerFlow *flow, u8 monId);
BOOL func_ov167_021aba8c(BtlServerFlow *flow);
BattleParty *BtlServerFlow_GetMonParty(BtlServerFlow *flow, u8 monId);
BattleParty *BtlServerFlow_GetAllyParty(BtlServerFlow *flow, u8 monId);
u8 BtlServerFlow_GetMoveTargetPos(BtlServerFlow *flow, u8 monId, u16 move);
u16 BtlServerFlow_CalcSpeed(BtlServerFlow *serverFlow, BattleMon *mon, BOOL a2);
u32 BtlServerFlow_CountFasterMons(BtlServerFlow *serverFlow, BattleMon *mon, BOOL a2);
u32 BtlServerFlow_GetSideEffectLayers(BtlServerFlow *serverFlow, u8 pos, u32 sideEffect);
BOOL BtlServerFlow_IsPosEffectFree(BtlServerFlow *serverFlow, u8 pos, u32 a2);
BOOL ServerControl_HideTurnCancel(BtlServerFlow *handler, BattleMon *mon, u32 flag);
BOOL ServerControl_FlinchCore(BtlServerFlow *handler, BattleMon *mon, u32 chance);
void ServerControl_SwitchInFillSlot(BtlServerFlow *handler, u8 target, u8 slot, u8 slotAgain, BOOL flag);
BOOL ServerControl_AfterSwitchIn(BtlServerFlow *handler);
void ServerControl_SetMonCounter(BtlServerFlow *handler, BattleMon *mon, u32 counter, u8 value);
void ServerControl_CheckItemReaction(BtlServerFlow *handler, BattleMon *mon, u32 reaction);
void ServerControl_ChangeHeldItem(BtlServerFlow *handler, BattleMon *mon, u16 item, BOOL consume);
BOOL ServerControl_UseHeldItem(BtlServerFlow *handler, BattleMon *mon);
BOOL ServerControl_EscapeSub(BtlServerFlow *handler, BattleMon *mon, u32 flag);
BOOL ServerControl_CheckMatchup(BtlServerFlow *handler);
BOOL BtlServerFlow_IsSkyDropAttacker(BtlServerFlow *flow, u8 monId);
BOOL BtlServerFlow_IsInSkyDrop(BtlServerFlow *handler, u8 monIndex);
s32 BtlServerFlow_GetMonWeight(BtlServerFlow *flow, u8 monId);
u32 BtlServerFlow_GetNextSerial(BtlServerFlow *flow);
void SetMoveEffectIndex(BtlServerFlow *flow, u8 index);
BOOL BtlServerFlow_AddPayDayMoney(BtlServerFlow *flow, u32 money, u8 monId);
void BtlServerFlow_DoublePrizeMoney(BtlServerFlow *flow, u8 monId);
void BtlServerFlow_AddMagicCoatReflect(BtlServerFlow *flow, u8 monId, u8 targetId);
BOOL BtlServerFlow_StartSkyDrop(BtlServerFlow *flow, u8 attackerId, u8 targetId, BOOL *failed);
void BtlServerFlow_SkyDropRelease(BtlServerFlow *flow, u8 monId);
BOOL BtlServerFlow_UseHeldItem(BtlServerFlow *flow, BattleMon *mon, u8 *flag);
void BtlServerFlow_TriggerAbility(BtlServerFlow *flow, BattleMon *mon);
void BtlServerFlow_NotifyRevive(BtlServerFlow *flow, u8 monId);
void BtlServerFlow_AddRevivedMon(BtlServerFlow *flow, u8 monId);
void BtlServerFlow_SetFrame(BtlServerFlow *flow, u32 depth, BOOL clear);
BOOL ServerControl_SwitchOut(BtlServerFlow *handler, BattleMon *mon, u8 flag);
BOOL ServerControl_FieldEffectCore(BtlServerFlow *handler, u32 effect, BattleCondition value, u8 dependPoke);
void ServerControl_FieldEffectEnd(BtlServerFlow *handler, u32 effect);
BOOL ServerControl_DecrementPP(BtlServerFlow *handler, BattleMon *mon, u8 moveIndex, u8 amount);
BOOL ServerEvent_DecrementPP(BtlServerFlow *handler, BattleMon *mon, u8 moveIndex);
void ServerEvent_EquipTempItem(BtlServerFlow *handler, BattleMon *mon, u8 monIndex);
void ServerEvent_GastroAcidConfirmed(BtlServerFlow *handler, BattleMon *mon);
void ServerControl_MoveCore(BtlServerFlow *handler, u8 clientId, u8 firstSlot, u8 secondSlot, u32 flag);
void ServerControl_AfterMove(BtlServerFlow *handler, u8 clientId, u8 firstSlot, u8 secondSlot);
BOOL ServerControl_ChangeWeatherCheck(BtlServerFlow *handler, u8 weather, u8 duration);
void ServerControl_ChangeWeatherCore(BtlServerFlow *handler, u8 weather, u8 duration);
void ServerEvent_NotifyAirLock(BtlServerFlow *handler);
BOOL ServerEvent_CheckFloating(BtlServerFlow *handler, BattleMon *mon, BOOL flag);
void ServerControl_CureCondition(BtlServerFlow *handler, BattleMon *mon, s32 condition, BattleCondition *prev);
u32 ServerEvent_CheckItemSet(BtlServerFlow *handler, BattleMon *mon, u16 item);
void ServerEvent_ItemSetFailed(BtlServerFlow *handler, BattleMon *mon);
void ServerEvent_ChangeAbilityAfter(BtlServerFlow *handler, u8 monIndex);
void ServerEvent_ChangeAbilityBefore(BtlServerFlow *handler, u8 monIndex, u16 oldAbility, u16 newAbility);
void ServerControl_UnnerveAction(BtlServerFlow *handler, BattleMon *mon);
BOOL ServerControl_DrainCore(BtlServerFlow *handler, BattleMon *mon, BattleMon *source, u16 amount);
BOOL ServerControl_CheckSimpleDamageEnabled(BtlServerFlow *handler, BattleMon *mon, u32 damage);
void ServerControl_ViewEffect(BtlServerFlow *handler, u16 effect, u8 pos1, u8 pos2, BOOL reserved, u32 reserve);
BOOL ServerControl_SimpleDamageCore(BtlServerFlow *handler, BattleMon *mon, u32 damage, BattleHandlerString *string);
void ServerControl_FaintPokemon(BtlServerFlow *handler, BattleMon *mon);

u16 BtlServerFlow_GetPlayerImmuneHitCount(BtlServerFlow *flow);
u16 BtlServerFlow_GetPlayerSuperEffectiveHitCount(BtlServerFlow *flow);
u16 BtlServerFlow_GetPlayerNotVeryEffectiveHitCount(BtlServerFlow *flow);
u16 BtlServerFlow_GetFoeImmuneHitCount(BtlServerFlow *flow);
u16 BtlServerFlow_GetFoeNotVeryEffectiveHitCount(BtlServerFlow *flow);
u8 BtlServerFlow_GetSwitchCount(BtlServerFlow *flow, u32 arg1);
u32 func_ov167_021ae320(BtlServerFlow *flow);

// Not decompiled yet
void BtlServerFlow_InitWork(BtlServerFlow *flow);
void ServerControl_ClearRechargeFlags(BtlServerFlow *flow);
u32 ActionOrder_Run(BtlServerFlow *flow, u32 i);
BOOL ServerControl_MidTurnSwitchIn(BtlServerFlow *flow, BtlClientActions *clientActions);
u8 ActionOrder_Build(BtlServerFlow *flow, BtlClientActions *clientActions, ActionOrderEntry *order, u8 max);
void BtlFlowMonIter_Init(BtlFlowMonIter *iter, BtlServerFlow *flow);
BOOL BtlFlowMonIter_Next(BtlFlowMonIter *iter, BtlServerFlow *flow, BattleMon **mon);
void func_ov167_021a8f8c(BtlFlowUnk1B54 *work);
void BtlServerFlow_ClearEffectivenessCounts(u16 *counts);
void BtlServerFlow_ClearRevivedMons(BtlServerFlow *flow);
void BtlServerFlow_ResetFrames(BtlServerFlow *flow);
BOOL ServerControl_ChangeWeather(BtlServerFlow *flow, u8 weather, u8 turns);
void ServerControl_SwitchInCore(BtlServerFlow *flow, u8 clientId, u8 pos, u8 slot);
void ActionOrder_RecalcAndSort(BtlServerFlow *flow, ActionOrderEntry *order, u32 count);
void func_ov167_0219fe44(BtlServerFlow *flow);
void ServerControl_Rotation(BtlServerFlow *flow, u8 clientId, u8 slot);
BOOL ServerControl_RequestChangeForEmptyPos(BtlServerFlow *flow, BtlFlowClientList *list);
void ServerControl_BeforeFirstFight(BtlServerFlow *flow);
void ActionOrder_Sort(ActionOrderEntry *order, u32 count);
u8 ServerEvent_GetMovePriority(BtlServerFlow *flow, u16 move, BattleMon *mon);
u16 ServerEvent_CalculateSpeed(BtlServerFlow *flow, BattleMon *mon, BOOL flag);
ActionOrderEntry *ActionOrder_SearchNextByMonID(BtlServerFlow *flow, ActionOrderEntry *after, u8 monId);
ActionOrderEntry *ActionOrder_SearchAllyByMove(BtlServerFlow *flow, u16 move, u8 monId, u8 target);
u8 ActionOrder_GetIndex(BtlServerFlow *flow, ActionOrderEntry *entry);
u32 ActionOrder_Proc(BtlServerFlow *flow, ActionOrderEntry *entry);
BOOL ServerControl_Escape(BtlServerFlow *flow, BattleMon *mon);
void ServerControl_RotationIn(BtlServerFlow *flow);
void ServerControl_ChangePokemon(BtlServerFlow *flow, BattleMon *mon, u8 slot);
void ServerControl_ClearMonDependentEffects(BtlServerFlow *flow, BattleMon *mon, BOOL flag);
BOOL ServerControl_TurnCheck(BtlServerFlow *flow);
void StoreBattleMonsSpeedOrder(BtlServerFlow *flow, BtlMonSet *monSet);
void ServerControl_CommPlayerSupportHeal(BtlServerFlow *flow);
BOOL ServerControl_TurnCheckEvent(BtlServerFlow *flow, BtlMonSet *monSet, u32 event);
BOOL ServerControl_TurnCheckConditions(BtlServerFlow *flow, BtlMonSet *monSet);
void ServerControl_TurnCheckSideEffects(BtlServerFlow *flow);
void ServerControl_TurnCheckFieldEffects(BtlServerFlow *flow);
BOOL ServerControl_TurnCheckWeather(BtlServerFlow *flow, BtlMonSet *monSet);
void ServerEvent_TurnCheck(BtlServerFlow *flow, u8 monId, u32 event);
void ServerControl_ConditionDamage(BtlServerFlow *flow, BattleMon *mon, u32 condition, u32 damage);
u32 ServerEvent_ConditionDamage(BtlServerFlow *flow, BattleMon *mon, u32 condition, u32 damage);
void ServerControl_SideEffectEndMessage(u8 side, u32 effect, BtlServerFlow *flow);
void ServerDisplay_SideEffectEnd(BtlServerFlow *flow, u32 effect, u32 side);
void ServerControl_FieldEffectEndCallback(u32 effect, BtlServerFlow *flow);
s32 ServerEvent_CheckWeatherDamage(BtlServerFlow *flow, BattleMon *mon, u32 weather, s32 damage);
void ServerControl_WeatherDamage(BtlServerFlow *flow, BattleMon *mon, u32 weather, s32 damage);
u32 GetEnemyMaxLevel(BtlServerFlow *flow);
void ServerEvent_BeforeFaint(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerControl_GainExpFromFainted(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerControl_GiveExp(BtlServerFlow *flow, BattleParty *party, const BtlFlowExpEntry *entries);
void ServerDisplay_ConfusionDamage(BtlServerFlow *flow, BattleMon *mon, u32 damage);
void ServerDisplay_DoesntAffect(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerControl_PlayerSideHasMons(BtlServerFlow *flow);
void ServerControl_TripleBattleShiftToCenter(BtlServerFlow *flow);
BOOL ServerControl_CheckExpGain(BtlServerFlow *flow);
void ServerControl_UpdateIllusion(BtlServerFlow *flow, BtlFlowClientList *list);
u8 ServerEvent_CheckSpecialPriority(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_BeforeFight(BtlServerFlow *flow, BattleMon *mon, u16 move);
BOOL BtlServerFlow_CheckRevivedSwitchIn(BtlServerFlow *flow);
void ServerControl_ActionStart(BtlServerFlow *flow, BattleMon *mon, u32 action);
void ServerEvent_ActionStart(BtlServerFlow *flow, BattleMon *mon, u32 action);
void ServerEvent_ActionEnd(BtlServerFlow *flow, BattleMon *mon, u32 action);
u32 ServerControl_CheckObedience(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerEvent_CheckSleepPrevented(BtlServerFlow *flow, BattleMon *mon);
BOOL ActionOrder_InterruptProc(BtlServerFlow *flow, u8 monId, u8 targetId);
void BattleMoveEffectState_Init(BattleMoveEffectState *targets);
void BattleMoveEffectState_SetTargets(BattleMoveEffectState *targets, BtlServerFlow *flow, BattleMon *mon,
                                      BtlMonSet *monSet);
void BtlFlowMonIter_SetRotation(BtlFlowMonIter *iter, BtlServerFlow *flow);
void ServerControl_TripleShift(BtlServerFlow *flow, BattleMon *mon);
void ServerControl_AfterMoveCore(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_AfterMove(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerEvent_CheckSureEscape(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_ReflectMove(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
BOOL ServerEvent_CheckEscapeBlocked(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerEvent_EscapeMessage(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_MemberIn(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_AbilityMemberIn(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_MemberInPrev(BtlServerFlow *flow);
void ServerEvent_RotationIn(BtlServerFlow *flow);
void ServerDisplay_RecallMessage(BtlServerFlow *flow, BattleMon *mon);
u32 ServerEvent_InterruptSwitch(BtlServerFlow *flow, BattleMon *mon);
void ServerControl_SwitchOutCore(BtlServerFlow *flow, BattleMon *mon, u32 effect);
void ServerControl_SwitchOutConfirm(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerControl_CheckDelegateMoveFail(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param);
void ServerControl_ReflectMoves(BtlServerFlow *flow, u16 move);
void BtlFlowReactionList_Init(BtlFlowReactionList *list);
BOOL ServerEvent_CheckDelayMove(BtlServerFlow *flow, BattleMon *mon, u8 target);
void ServerControl_DelayMoveDecide(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_DelayMoveDecide(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_MoveSeqStart(BtlServerFlow *flow, BattleMon *mon, u16 move);
void ServerEvent_MoveSeqEnd(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 result);
void ServerDisplay_MoveFailed(BtlServerFlow *flow, BattleMon *mon, u16 move);
void BtlFlowReactionList_Add(BtlFlowReactionList *list, u8 monId, u8 arg2, u8 target);
void ServerEvent_CallMoveEvent(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 event);
void ServerEvent_StealMove(BtlServerFlow *flow, BattleMon *mon, BattleMon *attacker, u16 move);
void ServerEvent_ReflectMove(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
void ServerDisplay_MoveFailMessage(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 cause);
void ServerControl_StealMove(BtlServerFlow *flow, BattleMon *mon, u16 move, BtlMonSet *targets);
void ServerEvent_RewriteMoveType(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param);
void ServerControl_CallMoveEvent(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 event);
BOOL ServerControl_CheckPledgeCombo(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target);
BOOL ServerControl_CheckDelayMove(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target, BtlFlowFightWork *work);
void ServerControl_MoveSeqStart(BtlServerFlow *flow, BattleMon *mon, u16 move);
void ServerControl_MoveSeqEnd(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 result);
BOOL ServerControl_CheckMoveSteal(BtlServerFlow *flow, BattleMon *mon, u16 move, BtlMonSet *targets,
                                  BtlFlowReactionList *list);
void ServerDisplay_MoveAnimation(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target);
BOOL ServerControl_ExecuteMove(BtlServerFlow *flow, BattleMon *mon, u16 move, BtlMonSet *targets);
BOOL ServerEvent_CheckMoveSteal(BtlServerFlow *flow, BattleMon *mon, u16 move, BtlMonSet *targets, u8 *monId,
                                u8 *target);
BOOL IsPlayerMon(BtlServerFlow *flow, BattleMon *mon);
void ServerControl_RecordMoveUse(BtlServerFlow *flow, BattleMon *mon, u16 move);
void ServerControl_RemoveOutOfRangeTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon,
                                           BtlMonSet *targets);
void ServerControl_RemoveSelfTarget(BtlServerFlow *flow, BattleMon *mon, BtlMonSet *targets, BtlFlowMoveParam *param,
                                    BOOL isDamage);
BOOL ServerEvent_CheckTypeImmunityEnabled(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon);
void ServerControl_RemoveSubstituteTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon,
                                           BtlMonSet *targets, BOOL flag);
void ServerControl_MoveExecuteEffective(BtlServerFlow *flow, u8 monId, u16 move);
void ServerControl_MoveExecuteNoEffect(BtlServerFlow *flow, u8 monId, u16 move);
void ServerControl_MoveExecuteEnd(BtlServerFlow *flow, u8 monId, u16 move);
void ServerEvent_MoveExecuteEvent(BtlServerFlow *flow, u8 monId, u16 move, u32 event);
BOOL IsGuaranteedHit(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender);
BOOL IsTargetOutOfRange(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
BOOL ServerControl_CheckNoEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BattleMon *target,
                                 u32 *data, u32 event);
BOOL ServerEvent_CheckNoEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, u32 event, BattleMon *mon,
                               BattleMon *target, u32 *data, BattleHandlerString *string, BOOL *silent);
BOOL ServerEvent_SkipAccuracyCheck(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, BtlFlowMoveParam *param);
BOOL ServerEvent_CheckHit(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, BtlFlowMoveParam *param);
BOOL ServerEvent_CheckProtectBreak(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
BOOL ServerEvent_CheckSemiInvulnAvoid(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
void ServerDisplay_MoveAvoided(BtlServerFlow *flow, BattleMon *mon, u16 move);
u32 ServerControl_CheckMoveEffectiveness(BtlServerFlow *flow, BattleMon *mon, BattleMon *target,
                                         BtlFlowMoveParam *param, BOOL flag);
BOOL ServerControl_CheckFloating(BtlServerFlow *flow, BattleMon *mon, BOOL flag);
void ServerControl_FloatingImmune(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_FloatingImmune(BtlServerFlow *flow, BattleMon *mon);
u32 ServerEvent_CheckTypeEffectiveness(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u8 moveType,
                                       u8 defenseType);
void BtlServerFlow_CountEffectiveness(u16 *counts, BtlServerFlow *flow, BattleMon *mon, BattleMon *target,
                                      s32 effectiveness);
void ServerControl_RemoveTypeImmuneTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon,
                                           BtlMonSet *targets, u32 *data);
void ServerControl_RemoveNoEffectTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon,
                                         BtlMonSet *targets, u32 *data);
void ServerControl_RemoveHiddenTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon,
                                       BtlMonSet *targets);
void ServerControl_RemoveMissedTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon,
                                       BtlMonSet *targets);
void ServerControl_PutMoveAnimation(BtlServerFlow *flow, u16 move, BattleMoveEffectState *effect, u32 reserved);
u32 ServerControl_CheckChargeTurn(BtlServerFlow *flow, BattleMon *mon, BtlMonSet *targets, u16 move);
void ServerControl_DamageMove(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BtlMonSet *targets,
                              u32 *data, u32 arg5);
void ServerControl_SimpleEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BtlMonSet *targets);
void ServerControl_SimpleCondition(BtlServerFlow *flow, u16 move, BattleMon *mon, BtlMonSet *targets);
void ServerControl_StatChangeAndCondition(BtlServerFlow *flow, const BtlFlowMoveParam *param, BattleMon *mon,
                                          BtlMonSet *targets);
void ServerControl_SimpleRecover(BtlServerFlow *flow, u16 move, BattleMon *mon, BtlMonSet *targets);
void ServerControl_OHKO(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BtlMonSet *targets);
void ServerControl_ForceSwitch(BtlServerFlow *flow, u16 move, BattleMon *mon, BtlMonSet *targets);
void ServerControl_FieldEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon);
void ServerControl_UncategorizedMove(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BtlMonSet *targets);
BOOL ServerControl_CheckFainted(BtlServerFlow *flow, BattleMon *mon);
u32 ServerEvent_MoveExecuteStart(BtlServerFlow *flow, BattleMon *mon, u16 move, BtlMonSet *targets);
void ServerControl_ClearChargeState(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerControl_SkyDropCatch(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BOOL *failed);
BOOL ServerEvent_CanUseMoveAsleep(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 status);
u32 ServerEvent_CheckMoveFail(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 event);
BOOL ServerControl_CheckConfusion(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerControl_CheckAttract(BtlServerFlow *flow, BattleMon *mon);
void ServerControl_CheckWakeUp(BtlServerFlow *flow, BattleMon *mon, u16 move);
u16 ServerControl_ConfusionDamage(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_MoveExecuteFail(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 cause);
BOOL ServerControl_CheckThaw(BtlServerFlow *flow, BattleMon *mon, u16 move);
void ServerControl_AddCondition(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition,
                                BattleCondition value, BOOL showMessage, BOOL skipItemReaction,
                                const BattleHandlerString *string);
void ServerDisplay_Confused(BtlServerFlow *flow, BattleMon *mon);
void ServerDisplay_InLove(BtlServerFlow *flow, BattleMon *mon);
void ServerControl_CureStatus(BtlServerFlow *flow, BattleMon *mon, u32 status, BOOL flag);
void ServerDisplay_AddEffectAtPosition(BtlServerFlow *flow, BattleMon *mon, u32 effect);
BOOL ServerControl_StartChargeTurn(BtlServerFlow *flow, BattleMon *mon, u8 pos, BtlMonSet *targets, u16 move);
void ServerDisplay_AddCondition(BtlServerFlow *flow, BattleMon *mon, s32 condition, BattleCondition value);
BOOL ServerEvent_CheckChargeFail(BtlServerFlow *flow, BattleMon *mon, BtlMonSet *targets);
BOOL ServerEvent_CheckChargeSkip(BtlServerFlow *flow, BattleMon *mon, u16 move);
BOOL ServerEvent_ChargeStart(BtlServerFlow *flow, BattleMon *mon, BtlMonSet *targets, u16 move, u8 *monId,
                             BOOL *failed);
void ServerEvent_ChargeStartFixed(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_ChargeSkipFixed(BtlServerFlow *flow, BattleMon *mon, u16 move);
u32 ServerEvent_ChargeRelease(BtlServerFlow *flow, BattleMon *mon, BtlMonSet *targets, u16 move);
BOOL ServerEvent_CheckAlwaysHit(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
u32 ServerEvent_GetMoveAccuracy(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, BtlFlowMoveParam *param);
BOOL ServerControl_CheckMoveFail(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 flag);
BOOL ServerControl_CheckMoveFailLate(BtlServerFlow *flow, BattleMon *mon, u16 move);
void ServerControl_MoveFail(BtlServerFlow *flow, BattleMon *mon, u16 move, s32 cause);
void ServerControl_ConsumePP(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 moveSlot, BtlMonSet *targets);
u32 ServerEvent_CalcPPDecrement(BtlServerFlow *flow, BattleMon *mon, u8 moveSlot, u16 move, BtlMonSet *targets);
void ServerControl_CalcDamageList(BtlServerFlow *flow, BattleMon *attacker, BtlMonSet *targets,
                                  const BtlFlowMoveParam *param, u32 *effectiveness, u32 arg5, BtlFlowDamageList *list);
u32 BtlFlowDamageList_GetCount(const BtlFlowDamageList *list);
u32 BtlFlowDamageList_GetMons(BtlServerFlow *flow, const BtlFlowDamageList *list, BattleMon **mons);
u32 BtlFlowDamageList_GetDirectHits(BtlServerFlow *flow, const BtlFlowDamageList *list, BattleMon **mons, u16 *damages,
                                    u32 *effectiveness, u8 *critical, u8 *unk9);
u32 BtlFlowDamageList_GetTotalDamage(const BtlFlowDamageList *list);
u32 ServerControl_DamageList(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, BtlMonSet *monSet,
                             BtlFlowDamageList *list, BtlFlowHitWork *hitWork, u32 ratio, BtlFlowDamageFlags flags);
u32 BtlFlowDamageList_GetSubstituteHits(BtlServerFlow *flow, const BtlFlowDamageList *list, BattleMon **mons,
                                        u16 *damages, u32 *effectiveness, u8 *critical);
u32 ClampDamageToHP(BattleMon *mon, u32 damage);
u32 ServerEvent_CheckEndure(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u32 arg3, u16 *damage);
BOOL ServerEvent_CheckCritical(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u16 move);
BOOL ServerEvent_CalcDamage(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender,
                            const BtlFlowMoveParam *param, u32 effectiveness, u32 ratio, BOOL critical, BOOL fixedRoll,
                            u16 *damage);
void ServerControl_DecrementPPCore(BtlServerFlow *flow, BattleMon *mon, u8 moveIndex, u8 amount);
u32 ServerControl_DamageAllTargets(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BtlMonSet *targets,
                                   u32 *data, u32 *reserved, u32 arg6);
u32 ServerControl_DamageMultiHit(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BtlMonSet *targets,
                                 u32 *data);
BOOL BtlFlowHitWork_IsFirstHit(BtlFlowHitWork *hitWork);
u8 BtlFlowHitWork_IsMultiHit(BtlFlowHitWork *hitWork);
void BtlFlowHitWork_SetEffectiveness(BtlFlowHitWork *hitWork, u32 value);
u32 ServerControl_DamageListCore(BtlServerFlow *flow, BattleMon *attacker, BtlMonSet *monSet, BtlFlowDamageList *list,
                                 BtlFlowMoveParam *param, BtlFlowHitWork *hitWork, u32 ratio, BtlFlowDamageFlags flags);
void ServerDisplay_EffectivenessMessage(BtlServerFlow *flow, s32 effectiveness);
void ServerControl_DamageDetermine(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target,
                                   BtlFlowMoveParam *param);
void ServerEvent_DamageDetermine(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BtlFlowMoveParam *param);
void ServerEvent_EndureUsed(BtlServerFlow *flow, BattleMon *mon, u32 cause);
void ServerEvent_BeforeDamageDisplay(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, u8 count,
                                     BattleMon **mons);
void ServerEvent_AfterDamage(BtlServerFlow *flow, BattleMon *attacker, BtlFlowMoveParam *param, u32 damage);
void ServerEvent_CheckItemReaction(BtlServerFlow *flow, BattleMon *mon, u32 reaction);
void ServerEvent_DamageProcessStart(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param);
void ServerEvent_DamageProcessEndSub(BtlServerFlow *flow, BattleMon *mon, BtlMonSet *monSet, BtlFlowMoveParam *param,
                                     u32 arg4, BOOL flag, u32 event);
void ServerEvent_DamageProcessEnd(BtlServerFlow *flow, BattleMon *mon, BtlMonSet *monSet, BtlFlowMoveParam *param,
                                  u32 arg4);
void ServerControl_CheckFireThaw(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BtlMonSet *monSet);
u32 ServerEvent_GetFlinchChance(BtlServerFlow *flow, u16 move, BattleMon *attacker);
BOOL ServerEvent_CheckFlinch(BtlServerFlow *flow, BattleMon *mon, u32 chance);
void ServerEvent_FlinchFail(BtlServerFlow *flow, BattleMon *mon);
u16 ServerEvent_CalcDrainAmount(BtlServerFlow *flow, BattleMon *mon, BattleMon *source, u16 amount);
BOOL ServerControl_RecoverHPCheckFail(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerControl_RecoverHP(BtlServerFlow *flow, BattleMon *mon, u16 amount, BOOL flag);
u16 ServerEvent_GetMovePower(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender,
                             const BtlFlowMoveParam *param);
u16 ServerEvent_GetAttackPower(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender,
                               const BtlFlowMoveParam *param, BOOL critical);
u16 ServerEvent_GetTargetDefenses(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender,
                                  const BtlFlowMoveParam *param, BOOL critical);
u8 ServerEvent_GetWeather(BtlServerFlow *flow);
BOOL func_ov167_021ae30c(BtlServerFlow *flow);
fx32 ServerEvent_SameTypeAttackBonus(BtlServerFlow *flow, BattleMon *attacker, u8 type);
s32 ServerEvent_CalcRecoil(BtlServerFlow *flow, BattleMon *mon, u16 move, s32 damage, BOOL *forced);
BOOL ServerEvent_CheckSimpleDamageEnabled(BtlServerFlow *flow, BattleMon *mon, u32 damage);
BOOL ServerEvent_CheckHeldItemFail(BtlServerFlow *flow, BattleMon *mon, u16 item);
void ServerEvent_EquipItem(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_ItemSetDecide(BtlServerFlow *flow, BattleMon *mon, u16 item);
void ServerEvent_ItemSetFixed(BtlServerFlow *flow, BattleMon *mon);
void ServerEvent_CheckSideEffectParam(BtlServerFlow *flow, u8 monId, u32 effect, u8 side, BattleCondition *cont);
void ServerDisplay_FaintPokemon(BtlServerFlow *flow, BattleMon *mon, u32 flag);
u32 ServerEvent_DecideSpecialMoveCondition(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target,
                                           BattleHandlerString *string);
void ServerEvent_AddMoveConditionString(BtlServerFlow *flow, u32 condition, BattleMon *attacker, BattleMon *target,
                                        BattleHandlerString *string);
void ServerEvent_MoveConditionContinue(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u32 condition,
                                       BattleCondition *value);
BOOL ServerControl_AddConditionCheckFail(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition,
                                         BattleCondition value, u8 overwrite, BOOL showFail);
u32 AddConditionCheckFailOverwrite(BtlServerFlow *flow, BattleMon *mon, s32 condition, BattleCondition value,
                                   u8 overwrite);
void AddConditionCheckFailStandard(BtlServerFlow *flow, BattleMon *mon, u32 cause, u32 condition);
BOOL ServerEvent_MoveConditionCheckFail(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u32 condition);
void ServerEvent_AddConditionFailed(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition);
void ServerEvent_ConditionConfirmed(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition,
                                    BattleCondition value);
void ServerEvent_MoveStatusConfirmed(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition);
fx32 ServerEvent_GetWeightRatio(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerEvent_RollStatDropEffectChance(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker,
                                          BattleMon *target);
BOOL ServerControl_ApplyMoveStatChanges(BtlServerFlow *flow, const BtlFlowMoveParam *param, BattleMon *attacker,
                                        BattleMon *target, BOOL showFail);
u32 ServerControl_NextSerial(BtlServerFlow *flow);
void ServerEvent_GetMoveStatChangeValue(BtlServerFlow *flow, u16 move, u32 index, BattleMon *attacker,
                                        BattleMon *target, MoveStatChangeStat *stat, s32 *change);
void ServerEvent_MoveStatChangeApplied(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 stat, s32 change);
BOOL ServerControl_StatChangeCore(BtlServerFlow *flow, u8 monId, BattleMon *mon, MoveStatChangeStat stat, s32 change,
                                  u8 attackerId, u16 context, u32 value, BOOL showFail, BOOL flag);
s32 ServerEvent_CheckSubstituteInteraction(BtlServerFlow *flow, BattleMon *mon, u32 stat, u8 attackerId, u16 context,
                                           s32 change);
void ServerDisplay_StatChangeLimit(BtlServerFlow *flow, BattleMon *mon, u32 stat, s32 change);
BOOL ServerEvent_CheckStatChange(BtlServerFlow *flow, BattleMon *mon, u32 stat, u8 monId, s32 change, u32 value);
void ServerDisplay_StatChange(BtlServerFlow *flow, BattleMon *mon, u32 stat, s32 change, u16 context, BOOL flag);
void ServerEvent_StatChangeApplied(BtlServerFlow *flow, u8 monId, BattleMon *mon, u32 stat, s32 change);
void ServerEvent_StatChangeFailed(BtlServerFlow *flow, BattleMon *mon, u32 value);
u32 ServerEvent_CalcMoveHealAmount(BtlServerFlow *flow, u16 move, BattleMon *mon);
BOOL ServerControl_RecoverHPCheckFailSpecial(BtlServerFlow *flow, BattleMon *mon, BOOL showMessage);
void ServerControl_RecoverHPCore(BtlServerFlow *flow, BattleMon *mon, u16 amount);
BOOL ServerEvent_CheckOHKOHit(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u16 move);
void ServerDisplay_Unaffected(BtlServerFlow *flow, BattleMon *mon);
void ServerControl_OHKOSuccess(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 critical);
void ServerControl_OHKOSurvived(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 critical, u32 cause,
                                u16 damage);
u16 ServerControl_OHKOSubstitute(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 critical);
void ServerDisplay_DecreaseHP(BtlServerFlow *flow, BattleMon *mon, u32 damage);
void ServerControl_BreakSubstitute(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerControl_ForceSwitchCore(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BOOL forced,
                                   BOOL *failed, u16 effect, BOOL ignoreLevel, const BattleHandlerString *string);
u32 ServerControl_GetForceSwitchMode(BtlServerFlow *flow);
s32 ServerControl_GetRandomSwitchSlot(BtlServerFlow *flow, BtlServerClient *client);
BOOL ServerEvent_CheckForceSwitchBlocked(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target);
void ServerControl_ChangeWeatherAfter(BtlServerFlow *flow, u8 weather);
void ServerEvent_AfterWeatherChange(BtlServerFlow *flow, u8 weather);
u8 ServerEvent_IncreaseMoveWeatherTurns(BtlServerFlow *flow, u8 weather, BattleMon *mon);
void ServerEvent_FieldEffectMove(BtlServerFlow *flow, BattleMon *mon, u16 move);
void ServerControl_SkillSwap(BtlServerFlow *flow, BattleMon *mon, BtlMonSet *targets);
BOOL ServerControl_Substitute(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerEvent_UncategorizedMove(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BtlMonSet *targets,
                                   BOOL *showFail);
void ServerEvent_DamageReaction(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BtlFlowMoveParam *param,
                                u32 effectiveness, u32 damage, u32 critical, BOOL flag);
BOOL ServerEvent_AddCondition(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition,
                              BattleCondition value, BOOL flag, BOOL defaultMessage);
u32 ServerEvent_CheckMoveAddCondition(BtlServerFlow *flow, u16 move, BattleMon *attacker, BattleMon *target,
                                      BattleCondition *value);
BOOL ServerControl_MoveConditionCore(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u16 move,
                                     u32 condition, BattleCondition value, BOOL flag);
void ServerEvent_DamageAddEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, BattleMon *target);
void ServerControl_DamageAddCondition(BtlServerFlow *flow, const BtlFlowMoveParam *param, BattleMon *attacker,
                                      BattleMon *target);
void ServerControl_Endure(BtlServerFlow *flow, BattleMon *mon, u32 cause);
void ServerControl_BeforeDamageDisplay(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, u8 count,
                                       BattleMon **mons);
void ServerControl_RecordDamage(BtlServerFlow *flow, u8 attackerPos, BattleMon *attacker, BattleMon *target,
                                BtlFlowMoveParam *param, u16 damage);
void ServerControl_DamageAdditionalEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker,
                                          BattleMon *target, u16 damage, BOOL flag);
void ServerControl_DamageFlinch(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, BattleMon *target);
void ServerControl_DamageDrain(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, BattleMon *target,
                               u32 damage);
u16 ServerControl_DamageSubstitute(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u16 damage,
                                   u32 effectiveness, u8 critical, BtlFlowMoveParam *param);
void ServerControl_DamageReaction(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BtlFlowMoveParam *param,
                                  u32 effectiveness, u32 damage, u32 critical, BOOL flag);
void ServerDisplay_Damage(BtlServerFlow *flow, BtlFlowMoveParam *param, u32 count, u32 *effectiveness, BattleMon **mons,
                          u16 *damages, u8 *critical, BOOL multipleTargets);
void ServerDisplay_EffectivenessMessageMulti(BtlServerFlow *flow, u32 count, u32 *effectiveness, BattleMon **mons,
                                             BOOL multipleTargets);
void ServerDisplay_CriticalHitMessage(BtlServerFlow *flow, u32 count, BattleMon **mons, u8 *critical,
                                      BOOL multipleTargets);
BOOL ServerEvent_CheckDamageToRecover(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target,
                                      BtlFlowMoveParam *param);
void ServerEvent_ApplyDamageToRecover(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target);
void ServerControl_AfterDamage(BtlServerFlow *flow, BattleMon *attacker, BtlFlowMoveParam *param, u32 damage);
void ServerControl_CheckDamageToRecover(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param,
                                        BtlMonSet *targets);
void ServerControl_DamageProcessStart(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param);
void ServerControl_DamageProcessEnd(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BtlMonSet *monSet,
                                    u32 damage, u32 arg5);
void ServerControl_CalcRecoil(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 damage);
BOOL ServerEvent_CheckMultihitHits(BtlServerFlow *flow, BattleMon *mon, u16 move, BtlFlowHitWork *hitWork);
void ServerDisplay_MoveMessage(BtlServerFlow *flow, BattleMon *mon, u16 move);
BOOL ServerEvent_GetCalledMove(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target, BtlFlowCalledMove *called);
BOOL ServerEvent_SetMoveMessage(BtlServerFlow *flow, BattleMon *mon, u16 move, u16 actualMove,
                                BattleHandlerString *string);
void ServerEvent_GetMoveParam(BtlServerFlow *flow, u16 move, BattleMon *mon, BtlFlowMoveParam *param);
void ServerControl_SkyDropCheckRelease(BtlServerFlow *flow, BattleMon *mon, BOOL flag);
void ServerEvent_MemberInComplete(BtlServerFlow *flow);
BOOL ServerControl_EscapeCore(BtlServerFlow *flow, BattleMon *mon, BOOL arg2, BOOL arg3);
u16 GetEncoreReplacementMove(BattleMon *mon, BattleAction *action);
void ServerControl_Fight(BtlServerFlow *flow, BattleMon *mon, BattleAction *action, u32 key);
void ServerDisplay_MustRecharge(BtlServerFlow *flow, BattleMon *mon);
void ServerDisplay_SkyDropTargetAppear(BtlServerFlow *flow, BattleMon *mon, u16 effect);
void BtlServerFlow_PushFrame(BtlServerFlow *flow);
void BtlServerFlow_PopFrame(BtlServerFlow *flow);

// A mon that is there and hasn't fainted
static inline BOOL BtlFlow_IsMonAlive(BattleMon *mon) {
    if (mon != NULL) {
        if (!IsFainted(mon)) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

// The ID of the mon that a mon carries off with Sky Drop, 0x1f for none
static inline u8 BtlFlow_GetSkyDropTarget(BattleMon *mon) {
    u8 targetId;
    u8 counter = GetConditionCount(mon, 4);

    if (counter == 0 || (targetId = counter - 1) >= 0x18) {
        targetId = 0x1f;
    }
    return targetId;
}

#endif // POKEBW2_BATTLE_BTL_SERVER_FLOW_H
