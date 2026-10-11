#ifndef POKEBW2_BATTLE_BTL_SERVER_FLOW_H
#define POKEBW2_BATTLE_BTL_SERVER_FLOW_H

#include "types.h"
#include "battle/btl_handler.h"
#include "battle/btl_server.h"
#include "constants/battle.h"
#include "struct_decls.h"

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

BtlServerFlow *func_ov167_0219f390(BtlServer *server, BtlMainModule *mainModule, BtlPokeCon *pokeCon,
                                   BtlServerCmdQueue *queue, u32 a4, HeapID heapId);
void func_ov167_0219f3f8(BtlServerFlow *serverFlow);
void func_ov167_0219f570(BtlServerFlow *serverFlow);
u8 func_ov167_0219f588(BtlServerFlow *serverFlow);
void func_ov167_0219f65c(BtlServerFlow *serverFlow);
u32 func_ov167_0219f66c(BtlServerFlow *serverFlow, BtlClientActions *clientActions);
void func_ov167_0219f748(BtlServerFlow *serverFlow);
u32 func_ov167_0219f754(BtlServerFlow *serverFlow, BtlClientActions *clientActions);
void func_ov167_0219f7a8(BtlServerFlow *serverFlow);
u32 func_ov167_0219f7b4(BtlServerFlow *serverFlow, BtlClientActions *clientActions);
u32 func_ov167_0219fdf4(BtlServerFlow *serverFlow);
BOOL func_ov167_0219fe24(BtlServerFlow *serverFlow);
BtlClientIDList *func_ov167_0219ffe4(BtlServerFlow *serverFlow);
u8 func_ov167_0219fff0(BtlServerFlow *serverFlow);
u32 func_ov167_021ac018(BtlServerFlow *serverFlow);

// The damage of a move, with the type effectiveness if withEffectiveness is set. damageRoll is USE_MIN_DAMAGE for the
// lowest random roll, or ROLL_FOR_DAMAGE for a random one
u32 AICalcDamage(BtlServerFlow *serverFlow, u8 attackerId, u8 defenderId, u16 move, BOOL withEffectiveness,
                 u32 damageRoll);
u16 GetTurnCounter(BtlServerFlow *serverFlow);
BattleMon *GetBattleMon(BtlServerFlow *serverFlow, u8 monId);
u8 func_ov167_021abb50(BtlServerFlow *serverFlow, u8 monId);
BOOL func_ov167_021abb8c(BtlServerFlow *flow, u8 monId, BattleAction *action);
BOOL func_ov167_021abbec(BtlServerFlow *flow, u8 monId);
u16 func_ov167_021abc54(BtlServerFlow *flow);
u8 *func_ov167_021abc6c(BtlServerFlow *flow);
u8 *func_ov167_021abc70(BtlServerFlow *flow);
BOOL func_ov167_021abc8c(BtlServerFlow *flow, u8 monId);
u32 func_ov167_021abca8(BtlServerFlow *flow);
u32 GetBattleTerrain(BtlServerFlow *flow);
u32 func_ov167_021abcc0(BtlServerFlow *flow);
BOOL CheckEvolution(BtlServerFlow *flow, u8 monId);
BOOL func_ov167_021abf14(BtlServerFlow *flow);
u8 func_ov167_021ab874(BtlServerFlow *flow, u8 monId);
u8 func_ov167_021ab884(BtlServerFlow *flow, u8 pos);
u8 func_ov167_021ab894(BtlServerFlow *flow, u8 monId, u8 *monIds);
u32 func_ov167_021abc9c(BtlServerFlow *flow);
BOOL func_ov167_021abd74(BtlServerFlow *flow, u8 monId);
BOOL func_ov167_021abd8c(BtlServerFlow *flow, u8 monId);
BOOL func_ov167_021abe04(BtlServerFlow *flow, u8 side, u32 sideEffect);
u8 func_ov167_021abe40(BtlServerFlow *flow, u8 clientId);
BOOL IsMonSwitchingOut(BtlServerFlow *flow);
void AddSwitchOutInterrupt(BtlServerFlow *flow, u8 monId);
BOOL func_ov167_021abe78(BtlServerFlow *flow, u8 monId);
u8 func_ov167_021abb60(BtlServerFlow *flow, u8 pos);
u32 CalcMoveEffectiveness(BtlServerFlow *serverFlow, u8 attackerId, u8 defenderId, u16 move);
BOOL func_ov167_021aba04(BtlServerFlow *flow);
u8 func_ov167_021aba18(BtlServerFlow *flow, u8 monId);
u8 func_ov167_021aba2c(BtlServerFlow *flow, u8 monId);
u8 func_ov167_021aba44(BtlServerFlow *flow, u8 monId);
BOOL func_ov167_021aba64(BtlServerFlow *flow, u8 monId);
BOOL func_ov167_021aba8c(BtlServerFlow *flow);
BattleParty *func_ov167_021abb0c(BtlServerFlow *flow, u8 monId);
BattleParty *func_ov167_021abb20(BtlServerFlow *flow, u8 monId);
u8 func_ov167_021abb70(BtlServerFlow *flow, u8 monId, u16 move);
u16 func_ov167_021abd08(BtlServerFlow *serverFlow, BattleMon *mon, BOOL a2);
u32 func_ov167_021abd10(BtlServerFlow *serverFlow, BattleMon *mon, BOOL a2);
u32 func_ov167_021abe10(BtlServerFlow *serverFlow, u8 pos, u32 sideEffect);
BOOL func_ov167_021abe34(BtlServerFlow *serverFlow, u8 pos, u32 a2);
BOOL ServerControl_HideTurnCancel(BtlServerFlow *handler, BattleMon *mon, u32 flag);
BOOL ServerControl_FlinchCore(BtlServerFlow *handler, BattleMon *mon, u8 chance);
void ServerControl_SwitchInFillSlot(BtlServerFlow *handler, u8 target, u8 slot, u8 slotAgain, BOOL flag);
BOOL ServerControl_AfterSwitchIn(BtlServerFlow *handler);
void ServerControl_SetMonCounter(BtlServerFlow *handler, BattleMon *mon, u32 counter, u8 value);
void ServerControl_CheckItemReaction(BtlServerFlow *handler, BattleMon *mon, u32 reaction);
void ServerControl_ChangeHeldItem(BtlServerFlow *handler, BattleMon *mon, u16 item, BOOL consume);
BOOL ServerControl_UseHeldItem(BtlServerFlow *handler, BattleMon *mon);
BOOL ServerControl_EscapeSub(BtlServerFlow *handler, BattleMon *mon, u32 flag);
BOOL ServerControl_CheckMatchup(BtlServerFlow *handler);
BOOL func_ov167_021abe88(BtlServerFlow *flow, u8 monId);
BOOL func_ov167_021abeb4(BtlServerFlow *handler, u8 monIndex);
s32 func_ov167_021abee0(BtlServerFlow *flow, u8 monId);
u32 func_ov167_021abf0c(BtlServerFlow *flow);
void SetMoveEffectIndex(BtlServerFlow *flow, u8 index);
BOOL func_ov167_021abf28(BtlServerFlow *flow, u32 money, u8 monId);
void func_ov167_021abf48(BtlServerFlow *flow, u8 monId);
void func_ov167_021abf74(BtlServerFlow *flow, u8 monId, u8 targetId);
BOOL func_ov167_021abfac(BtlServerFlow *flow, u8 attackerId, u8 targetId, BOOL *failed);
void func_ov167_021abfd4(BtlServerFlow *flow, u8 monId);
BOOL func_ov167_021abfec(BtlServerFlow *flow, BattleMon *mon, u8 *flag);
void func_ov167_021ac010(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021ac020(BtlServerFlow *flow, u8 monId);
void func_ov167_021ac034(BtlServerFlow *flow, u8 monId);
void func_ov167_021ac114(BtlServerFlow *flow, u32 depth, BOOL clear);
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

u16 func_ov167_021ab7fc(BtlServerFlow *flow);
u16 func_ov167_021ab804(BtlServerFlow *flow);
u16 func_ov167_021ab810(BtlServerFlow *flow);
u16 func_ov167_021ab81c(BtlServerFlow *flow);
u16 func_ov167_021ab828(BtlServerFlow *flow);
u8 func_ov167_021abc80(BtlServerFlow *flow, u32 arg1);
u32 func_ov167_021ae320(BtlServerFlow *flow);

// Not decompiled yet
void func_ov167_0219f400(BtlServerFlow *flow);
void func_ov167_0219f6fc(BtlServerFlow *flow);
u32 func_ov167_0219f9d0(BtlServerFlow *flow, u32 i);
BOOL func_ov167_0219fc74(BtlServerFlow *flow, BtlClientActions *clientActions);
u8 func_ov167_021a00a4(BtlServerFlow *flow, BtlClientActions *clientActions, ActionOrderEntry *order, u8 max);
void func_ov167_021a0d5c(BtlFlowMonIter *iter, BtlServerFlow *flow);
BOOL func_ov167_021a0df4(BtlFlowMonIter *iter, BtlServerFlow *flow, BattleMon **mon);
void func_ov167_021a8f8c(BtlFlowUnk1B54 *work);
void func_ov167_021ab730(u16 *counts);
void func_ov167_021ac028(BtlServerFlow *flow);
void func_ov167_021ac0c8(BtlServerFlow *flow);
BOOL ServerControl_ChangeWeather(BtlServerFlow *flow, u8 weather, u8 turns);
void ServerControl_SwitchInCore(BtlServerFlow *flow, u8 clientId, u8 pos, u8 slot);
void func_ov167_0219fb3c(BtlServerFlow *flow, ActionOrderEntry *order, u32 count);
void func_ov167_0219fe44(BtlServerFlow *flow);
void func_ov167_0219feac(BtlServerFlow *flow, u8 clientId, u8 slot);
BOOL func_ov167_0219ff70(BtlServerFlow *flow, BtlFlowClientList *list);
void func_ov167_0219fffc(BtlServerFlow *flow);
void func_ov167_021a0308(ActionOrderEntry *order, u32 count);
u8 func_ov167_021a0380(BtlServerFlow *flow, u16 move, BattleMon *mon);
u16 ServerEvent_CalculateSpeed(BtlServerFlow *flow, BattleMon *mon, BOOL flag);
ActionOrderEntry *func_ov167_021a04e8(BtlServerFlow *flow, ActionOrderEntry *after, u8 monId);
ActionOrderEntry *func_ov167_021a05ac(BtlServerFlow *flow, u16 move, u8 monId, u8 target);
u8 func_ov167_021a0600(BtlServerFlow *flow, ActionOrderEntry *entry);
u32 func_ov167_021a0778(BtlServerFlow *flow, ActionOrderEntry *entry);
BOOL ServerControl_Escape(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a16d4(BtlServerFlow *flow);
void func_ov167_021a1740(BtlServerFlow *flow, BattleMon *mon, u8 slot);
void ServerControl_ClearMonDependentEffects(BtlServerFlow *flow, BattleMon *mon, BOOL flag);
BOOL func_ov167_021a7f1c(BtlServerFlow *flow);
void StoreBattleMonsSpeedOrder(BtlServerFlow *flow, void *monSet);
void func_ov167_021a81f4(BtlServerFlow *flow);
BOOL func_ov167_021a82e8(BtlServerFlow *flow, void *monSet, u32 event);
BOOL func_ov167_021a83ec(BtlServerFlow *flow, void *monSet);
void func_ov167_021a864c(BtlServerFlow *flow);
void func_ov167_021a86e4(BtlServerFlow *flow);
BOOL func_ov167_021a87dc(BtlServerFlow *flow, void *monSet);
void func_ov167_021a83c0(BtlServerFlow *flow, u8 monId, u32 event);
void func_ov167_021a8524(BtlServerFlow *flow, BattleMon *mon, u32 condition, u32 damage);
u32 func_ov167_021a85fc(BtlServerFlow *flow, BattleMon *mon, u32 condition, u32 damage);
void ServerControl_SideEffectEndMessage(u32 side, u32 effect, BtlServerFlow *flow);
void func_ov167_021a866c(BtlServerFlow *flow, u32 effect, u32 side);
void func_ov167_021a8700(u32 effect, BtlServerFlow *flow);
s32 func_ov167_021a88f8(BtlServerFlow *flow, BattleMon *mon, u32 weather, s32 damage);
void func_ov167_021a8964(BtlServerFlow *flow, BattleMon *mon, u32 weather, s32 damage);
u32 GetEnemyMaxLevel(BtlServerFlow *flow);
void ServerEvent_BeforeFaint(BtlServerFlow *flow, BattleMon *mon);
BOOL func_ov167_021a8dec(BtlServerFlow *flow, BattleMon *mon);
BOOL func_ov167_021a8e68(BtlServerFlow *flow, BattleParty *party, BtlFlowExpEntry *entries);
void func_ov167_021a9058(BtlServerFlow *flow, BattleMon *mon, u32 damage);
void func_ov167_021a9268(BtlServerFlow *flow, BattleMon *mon);
BOOL func_ov167_0219fda4(BtlServerFlow *flow);
void func_ov167_021a80c4(BtlServerFlow *flow);
BOOL func_ov167_021a8cc0(BtlServerFlow *flow);
void func_ov167_021a9c70(BtlServerFlow *flow, BtlFlowClientList *list);
u8 func_ov167_021a9e68(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a9eac(BtlServerFlow *flow, BattleMon *mon, u16 move);
BOOL func_ov167_021ac074(BtlServerFlow *flow);
void func_ov167_021a0994(BtlServerFlow *flow, BattleMon *mon, u32 action);
void func_ov167_021a09cc(BtlServerFlow *flow, BattleMon *mon, u32 action);
void func_ov167_021a0a08(BtlServerFlow *flow, BattleMon *mon, u32 action);
u32 func_ov167_021a0a44(BtlServerFlow *flow, BattleMon *mon);
BOOL func_ov167_021a0b28(BtlServerFlow *flow, BattleMon *mon);
BOOL ActionOrder_InterruptProc(BtlServerFlow *flow, u8 monId, u8 targetId);
void func_ov167_021a0c88(BattleMoveEffectState *targets);
void func_ov167_021a0ca8(BattleMoveEffectState *targets, BtlServerFlow *flow, BattleMon *mon, void *monSet);
void func_ov167_021a0de0(BtlFlowMonIter *iter, BtlServerFlow *flow);
void func_ov167_021a0e90(BtlServerFlow *flow, BattleMon *mon);
void ServerControl_AfterMoveCore(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a0ffc(BtlServerFlow *flow, BattleMon *mon);
BOOL func_ov167_021a1160(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a2508(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
BOOL func_ov167_021a12f8(BtlServerFlow *flow, BattleMon *mon);
BOOL func_ov167_021a1354(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a1630(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a1660(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a1694(BtlServerFlow *flow);
void func_ov167_021a1700(BtlServerFlow *flow);
void func_ov167_021a1720(BtlServerFlow *flow, BattleMon *mon);
u32 ServerEvent_InterruptSwitch(BtlServerFlow *flow, BattleMon *mon);
void ServerControl_SwitchOutCore(BtlServerFlow *flow, BattleMon *mon, u32 effect);
void ServerControl_SwitchOutConfirm(BtlServerFlow *flow, BattleMon *mon);
BOOL func_ov167_021a1e50(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param);
void func_ov167_021a1ea8(BtlServerFlow *flow, u16 move);
void func_ov167_021a1fc0(BtlFlowReactionList *list);
BOOL func_ov167_021a2320(BtlServerFlow *flow, BattleMon *mon, u8 target);
void func_ov167_021a236c(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a239c(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a2404(BtlServerFlow *flow, BattleMon *mon, u16 move);
void func_ov167_021a2478(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 result);
void func_ov167_021a9230(BtlServerFlow *flow, BattleMon *mon, u16 move);
void func_ov167_021a1fd4(BtlFlowReactionList *list, u8 monId, u8 arg2, u8 target);
void func_ov167_021a2150(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 event);
void func_ov167_021a24bc(BtlServerFlow *flow, BattleMon *mon, BattleMon *attacker, u16 move);
void func_ov167_021a2508(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
void func_ov167_021a3fc4(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 cause);
void func_ov167_021a1ff8(BtlServerFlow *flow, BattleMon *mon, u16 move, void *targets);
void func_ov167_021a20c8(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param);
void func_ov167_021a2114(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 event);
BOOL func_ov167_021a2194(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target);
BOOL func_ov167_021a228c(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target, BtlFlowFightWork *work);
void func_ov167_021a23cc(BtlServerFlow *flow, BattleMon *mon, u16 move);
void func_ov167_021a243c(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 result);
BOOL func_ov167_021a255c(BtlServerFlow *flow, BattleMon *mon, u16 move, void *targets, BtlFlowReactionList *list);
void func_ov167_021a2680(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target);
BOOL func_ov167_021a2700(BtlServerFlow *flow, BattleMon *mon, u16 move, void *targets);
BOOL func_ov167_021a25bc(BtlServerFlow *flow, BattleMon *mon, u16 move, void *targets, u8 *monId, u8 *target);
BOOL func_ov167_021a26b0(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a26d4(BtlServerFlow *flow, BattleMon *mon, u16 move);
void func_ov167_021a2af4(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets);
void func_ov167_021a2b8c(BtlServerFlow *flow, BattleMon *mon, void *targets, BtlFlowMoveParam *param, BOOL isDamage);
BOOL func_ov167_021a2c10(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon);
void func_ov167_021a2c5c(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets, BOOL flag);
void func_ov167_021a2cec(BtlServerFlow *flow, u8 monId, u16 move);
void func_ov167_021a2d24(BtlServerFlow *flow, u8 monId, u16 move);
void func_ov167_021a2d5c(BtlServerFlow *flow, u8 monId, u16 move);
void func_ov167_021a2d94(BtlServerFlow *flow, u8 monId, u16 move, u32 event);
BOOL IsGuaranteedHit(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender);
BOOL func_ov167_021a34a4(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
BOOL func_ov167_021a3190(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, BattleMon *target, void *data,
                         u32 event);
BOOL func_ov167_021a3230(BtlServerFlow *flow, BtlFlowMoveParam *param, u32 event, BattleMon *mon, BattleMon *target,
                         void *data, BattleHandlerString *string, BOOL *silent);
BOOL func_ov167_021a3448(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, BtlFlowMoveParam *param);
BOOL func_ov167_021a3504(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, BtlFlowMoveParam *param);
BOOL func_ov167_021aa180(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
BOOL func_ov167_021aa460(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
void func_ov167_021a9244(BtlServerFlow *flow, BattleMon *mon, u16 move);
u32 func_ov167_021aa954(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, BtlFlowMoveParam *param, BOOL flag);
BOOL func_ov167_021aaa24(BtlServerFlow *flow, BattleMon *mon, BOOL flag);
void func_ov167_021aaad8(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021aab20(BtlServerFlow *flow, BattleMon *mon);
u32 func_ov167_021aab50(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u8 moveType, u8 defenseType);
void func_ov167_021ab73c(u16 *counts, BtlServerFlow *flow, BattleMon *mon, BattleMon *target, s32 effectiveness);
void func_ov167_021a2e80(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets, void *data);
void func_ov167_021a2f54(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets, void *data);
void func_ov167_021a32e0(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets);
void func_ov167_021a3378(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets);
void func_ov167_021a3674(BtlServerFlow *flow, u16 move, BattleMoveEffectState *effect, u32 reserved);
u32 func_ov167_021a36ac(BtlServerFlow *flow, BattleMon *mon, void *targets, u16 move);
void func_ov167_021a43c0(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets, void *data,
                         u32 arg5);
void ServerControl_SimpleEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets);
void ServerControl_SimpleCondition(BtlServerFlow *flow, u16 move, BattleMon *mon, void *targets);
void func_ov167_021a6c34(BtlServerFlow *flow, const BtlFlowMoveParam *param, BattleMon *mon, void *targets);
void func_ov167_021a6d24(BtlServerFlow *flow, u16 move, BattleMon *mon, void *targets);
void ServerControl_OHKO(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets);
void ServerControl_ForceSwitch(BtlServerFlow *flow, u16 move, BattleMon *mon, void *targets);
void ServerControl_FieldEffect(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon);
void func_ov167_021a77b8(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets);
BOOL ServerControl_CheckFainted(BtlServerFlow *flow, BattleMon *mon);
u32 func_ov167_021aa0c0(BtlServerFlow *flow, BattleMon *mon, u16 move, void *targets);
void func_ov167_021a3904(BtlServerFlow *flow, BattleMon *mon);
BOOL func_ov167_021a3950(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BOOL *failed);
BOOL func_ov167_021a3d18(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 status);
u32 func_ov167_021a3d70(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 event);
BOOL func_ov167_021a3dc0(BtlServerFlow *flow, BattleMon *mon);
BOOL func_ov167_021a3e50(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a3e7c(BtlServerFlow *flow, BattleMon *mon, u16 move);
u16 func_ov167_021a9ee8(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021aa07c(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 cause);
BOOL func_ov167_021a3ea8(BtlServerFlow *flow, BattleMon *mon, u16 move);
void ServerControl_AddCondition(BtlServerFlow *flow, BattleMon *target, BattleMon *attacker, u32 condition,
                                BattleCondition value, BOOL showMessage, BOOL skipItemReaction,
                                const BattleHandlerString *string);
void func_ov167_021a8fe0(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a9014(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021a9094(BtlServerFlow *flow, BattleMon *mon, u32 status, BOOL flag);
void ServerDisplay_AddEffectAtPosition(BtlServerFlow *flow, BattleMon *mon, u32 effect);
BOOL func_ov167_021a37c8(BtlServerFlow *flow, BattleMon *mon, u8 pos, void *targets, u16 move);
void ServerDisplay_AddCondition(BtlServerFlow *flow, BattleMon *mon, s32 condition, BattleCondition value);
BOOL func_ov167_021aa1c4(BtlServerFlow *flow, BattleMon *mon, void *targets);
BOOL func_ov167_021aa238(BtlServerFlow *flow, BattleMon *mon, u16 move);
BOOL func_ov167_021aa284(BtlServerFlow *flow, BattleMon *mon, void *targets, u16 move, u8 *monId, BOOL *failed);
void func_ov167_021aa360(BtlServerFlow *flow, BattleMon *mon);
void func_ov167_021aa390(BtlServerFlow *flow, BattleMon *mon, u16 move);
u32 func_ov167_021aa3c0(BtlServerFlow *flow, BattleMon *mon, void *targets, u16 move);
BOOL func_ov167_021aa4d0(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 move);
u32 func_ov167_021aa51c(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, BtlFlowMoveParam *param);
BOOL func_ov167_021a3ac0(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 flag);
BOOL func_ov167_021a3cf0(BtlServerFlow *flow, BattleMon *mon, u16 move);
void func_ov167_021a3ef4(BtlServerFlow *flow, BattleMon *mon, u16 move, s32 cause);
void func_ov167_021a4250(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 moveSlot, void *targets);
u32 func_ov167_021a4278(BtlServerFlow *flow, BattleMon *mon, u8 moveSlot, u16 move, void *targets);
void func_ov167_021a44f0(BtlServerFlow *flow, BattleMon *attacker, void *targets, const BtlFlowMoveParam *param,
                         void *effectiveness, u32 arg5, BtlFlowDamageList *list);
u32 func_ov167_021a46d4(BtlFlowDamageList *list);
u32 func_ov167_021a4754(BtlServerFlow *flow, BtlFlowDamageList *list, BattleMon **mons);
u32 func_ov167_021a4788(BtlServerFlow *flow, BtlFlowDamageList *list, BattleMon **mons, u16 *damages,
                        u32 *effectiveness, u8 *critical, u8 *unk9);
u32 func_ov167_021a4810(BtlFlowDamageList *list);
u32 func_ov167_021a4c44(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, void *monSet,
                        BtlFlowDamageList *list, BtlFlowHitWork *hitWork, u32 ratio, BtlFlowDamageFlags flags);
u32 func_ov167_021a46d8(BtlServerFlow *flow, BtlFlowDamageList *list, BattleMon **mons, u16 *damages,
                        u32 *effectiveness, u8 *critical);
u32 func_ov167_021a5074(BattleMon *mon, u32 damage);
u32 func_ov167_021a5118(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u32 arg3, u16 *damage);
BOOL func_ov167_021aa710(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u16 move);
BOOL ServerEvent_CalcDamage(BtlServerFlow *flow, BattleMon *attacker, BattleMon *defender,
                            const BtlFlowMoveParam *param, u32 effectiveness, u32 ratio, BOOL critical, BOOL fixedRoll,
                            u16 *damage);
void func_ov167_021a4370(BtlServerFlow *flow, BattleMon *mon, u8 moveIndex, u8 amount);
u32 func_ov167_021a4830(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets, void *data,
                        u32 *reserved, u32 arg6);
u32 func_ov167_021a49c4(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets, void *data);
BOOL func_ov167_021a4c24(BtlFlowHitWork *hitWork);
u8 func_ov167_021a4c34(BtlFlowHitWork *hitWork);
void func_ov167_021a4c38(BtlFlowHitWork *hitWork, u32 value);
u32 func_ov167_021a4c90(BtlServerFlow *flow, BattleMon *attacker, void *monSet, BtlFlowDamageList *list,
                        BtlFlowMoveParam *param, BtlFlowHitWork *hitWork, u32 ratio, BtlFlowDamageFlags flags);
void func_ov167_021a504c(BtlServerFlow *flow, s32 effectiveness);
void func_ov167_021a5088(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BtlFlowMoveParam *param);
void func_ov167_021a50c4(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BtlFlowMoveParam *param);
void func_ov167_021a51f8(BtlServerFlow *flow, BattleMon *mon, u32 cause);
void func_ov167_021a526c(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, u8 count, BattleMon **mons);
void func_ov167_021a53b0(BtlServerFlow *flow, BattleMon *attacker, BtlFlowMoveParam *param, u32 damage);
void ServerEvent_CheckItemReaction(BtlServerFlow *flow, BattleMon *mon, u32 reaction);
void func_ov167_021a54b0(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param);
void func_ov167_021a55fc(BtlServerFlow *flow, BattleMon *mon, void *monSet, BtlFlowMoveParam *param, u32 arg4,
                         BOOL flag, u32 event);
void func_ov167_021a5728(BtlServerFlow *flow, BattleMon *mon, void *monSet, BtlFlowMoveParam *param, u32 arg4);
void func_ov167_021a5784(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *monSet);
u8 func_ov167_021ab17c(BtlServerFlow *flow, u16 move, BattleMon *attacker);
BOOL ServerEvent_CheckFlinch(BtlServerFlow *flow, BattleMon *mon, u8 chance);
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
BOOL func_ov167_021a6914(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, BattleMon *target,
                         BOOL showFail);
u32 func_ov167_021a68fc(BtlServerFlow *flow);
void ServerEvent_GetMoveStatChangeValue(BtlServerFlow *flow, u16 move, u32 index, BattleMon *attacker,
                                        BattleMon *target, u32 *stat, s32 *change);
void func_ov167_021ab3c0(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 stat, s32 change);
BOOL func_ov167_021a6ab8(BtlServerFlow *flow, u8 monId, BattleMon *mon, u32 stat, s32 change, u8 attackerId,
                         u16 context, u32 value, BOOL showFail, BOOL flag);
s32 ServerEvent_CheckSubstituteInteraction(BtlServerFlow *flow, BattleMon *mon, u32 stat, u8 attackerId, u16 context,
                                           s32 change);
void func_ov167_021a9564(BtlServerFlow *flow, BattleMon *mon, u32 stat, s32 change);
BOOL func_ov167_021ab2c8(BtlServerFlow *flow, BattleMon *mon, u32 stat, u8 monId, s32 change, u32 value);
void func_ov167_021a95a4(BtlServerFlow *flow, BattleMon *mon, u32 stat, s32 change, u16 context, BOOL flag);
void func_ov167_021ab374(BtlServerFlow *flow, u8 monId, BattleMon *mon, u32 stat, s32 change);
void func_ov167_021ab338(BtlServerFlow *flow, BattleMon *mon, u32 value);
u32 ServerEvent_CalcMoveHealAmount(BtlServerFlow *flow, u16 move, BattleMon *mon);
BOOL ServerControl_RecoverHPCheckFailSpecial(BtlServerFlow *flow, BattleMon *mon, BOOL showMessage);
void ServerControl_RecoverHPCore(BtlServerFlow *flow, BattleMon *mon, u16 amount);
BOOL func_ov167_021aa5b4(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u16 move);
void func_ov167_021a928c(BtlServerFlow *flow, BattleMon *mon);
void ServerControl_OHKOSuccess(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 critical);
void func_ov167_021a7198(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 critical, u32 cause,
                         u16 damage);
u16 func_ov167_021a71d0(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u32 critical);
void func_ov167_021a9b64(BtlServerFlow *flow, BattleMon *mon, u32 damage);
void func_ov167_021a7c70(BtlServerFlow *flow, BattleMon *mon);
BOOL ServerControl_ForceSwitchCore(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BOOL forced,
                                   BOOL *failed, u16 effect, BOOL ignoreLevel, const BattleHandlerString *string);
u32 func_ov167_021a747c(BtlServerFlow *flow);
s32 func_ov167_021a74a4(BtlServerFlow *flow, BtlServerClient *client);
BOOL func_ov167_021a74fc(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target);
void ServerControl_ChangeWeatherAfter(BtlServerFlow *flow, u8 weather);
void ServerEvent_AfterWeatherChange(BtlServerFlow *flow, u8 weather);
u8 ServerEvent_IncreaseMoveWeatherTurns(BtlServerFlow *flow, u8 weather, BattleMon *mon);
void func_ov167_021a777c(BtlServerFlow *flow, BattleMon *mon, u16 move);
void func_ov167_021a78bc(BtlServerFlow *flow, BattleMon *mon, void *targets);
BOOL func_ov167_021a7ae4(BtlServerFlow *flow, BattleMon *mon);
BOOL func_ov167_021a7db4(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *targets, BOOL *showFail);
void func_ov167_021a7d18(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BtlFlowMoveParam *param,
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
void func_ov167_021a5198(BtlServerFlow *flow, BattleMon *mon, u32 cause);
void func_ov167_021a5228(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, u8 count, BattleMon **mons);
void func_ov167_021a52c8(BtlServerFlow *flow, u8 attackerPos, BattleMon *attacker, BattleMon *target,
                         BtlFlowMoveParam *param, u16 damage);
void func_ov167_021a5320(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, BattleMon *target,
                         u16 damage, BOOL flag);
void func_ov167_021a576c(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, BattleMon *target);
void ServerControl_DamageDrain(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *attacker, BattleMon *target,
                               u32 damage);
u16 func_ov167_021a7bb4(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, u16 damage, u32 effectiveness,
                        u8 critical, BtlFlowMoveParam *param);
void func_ov167_021a7cc8(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BtlFlowMoveParam *param,
                         u32 effectiveness, u32 damage, u32 critical, BOOL flag);
void func_ov167_021a92b0(BtlServerFlow *flow, BtlFlowMoveParam *param, u32 count, u32 *effectiveness, BattleMon **mons,
                         u16 *damages, u8 *critical, BOOL multipleTargets);
void func_ov167_021a9358(BtlServerFlow *flow, u32 count, u32 *effectiveness, BattleMon **mons, BOOL multipleTargets);
void func_ov167_021a94dc(BtlServerFlow *flow, u32 count, BattleMon **mons, u8 *critical, BOOL multipleTargets);
BOOL func_ov167_021aa674(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target, BtlFlowMoveParam *param);
void func_ov167_021aa6d0(BtlServerFlow *flow, BattleMon *attacker, BattleMon *target);
void func_ov167_021a5374(BtlServerFlow *flow, BattleMon *attacker, BtlFlowMoveParam *param, u32 damage);
void func_ov167_021a4f80(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, void *targets);
void func_ov167_021a5478(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param);
void func_ov167_021a54f4(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, void *monSet, u32 damage,
                         u32 arg5);
void ServerControl_CalcRecoil(BtlServerFlow *flow, BattleMon *mon, u16 move, u32 damage);
BOOL ServerEvent_CheckMultihitHits(BtlServerFlow *flow, BattleMon *mon, u16 move, BtlFlowHitWork *hitWork);
void func_ov167_021a911c(BtlServerFlow *flow, BattleMon *mon, u16 move);
BOOL func_ov167_021a9df0(BtlServerFlow *flow, BattleMon *mon, u16 move, u8 target, BtlFlowCalledMove *called);
BOOL func_ov167_021a9f70(BtlServerFlow *flow, BattleMon *mon, u16 move, u16 actualMove, BattleHandlerString *string);
void ServerEvent_GetMoveParam(BtlServerFlow *flow, u16 move, BattleMon *mon, BtlFlowMoveParam *param);
void ServerControl_SkyDropCheckRelease(BtlServerFlow *flow, BattleMon *mon, BOOL flag);
void func_ov167_021a16b4(BtlServerFlow *flow);
BOOL func_ov167_021a11b0(BtlServerFlow *flow, BattleMon *mon, BOOL arg2, BOOL arg3);
u16 func_ov167_021a18f0(BattleMon *mon, BattleAction *action);
void func_ov167_021a1940(BtlServerFlow *flow, BattleMon *mon, BattleAction *action, u32 key);
void func_ov167_021a8fd4(BtlServerFlow *flow, BattleMon *mon);
void ServerDisplay_SkyDropTargetAppear(BtlServerFlow *flow, BattleMon *mon, u16 effect);
void func_ov167_021ac0dc(BtlServerFlow *flow);
void func_ov167_021ac0f8(BtlServerFlow *flow);

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
