#include "battle/pos_handlers.h"
#include "types.h"
#include "battle/btl_calc.h"
#include "battle/btl_event.h"
#include "battle/btl_handler.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_server_flow.h"
#include "constants/battle.h"

// One entry of the position effect table: an effect and the function that returns its handlers
typedef struct PosEventAddEntry {
    u32 effect;
    const BattleEventHandlerEntry *(*eventAdd)(u32 *numHandlers);
} PosEventAddEntry;

static BOOL IsPosEffectFreeCore(u32 effect, u8 pos);
static const BattleEventHandlerEntry *EventAddPosWish(u32 *numHandlers);
static void HandlerPosWish(BattleEventItem *item, BtlServerFlow *flow, u8 pos, s32 *work);
static const BattleEventHandlerEntry *EventAddPosLunarDance(u32 *numHandlers);
static void HandlerPosLunarDance(BattleEventItem *item, BtlServerFlow *flow, u8 pos, s32 *work);
static const BattleEventHandlerEntry *EventAddPosHealingWish(u32 *numHandlers);
static void HandlerPosHealingWish(BattleEventItem *item, BtlServerFlow *flow, u8 pos, s32 *work);
static const BattleEventHandlerEntry *EventAddPosFutureSight(u32 *numHandlers);
static void HandlerPosFutureSight(BattleEventItem *item, BtlServerFlow *flow, u8 pos, s32 *work);
static const BattleEventHandlerEntry *EventAddPosBatonPass(u32 *numHandlers);
static void HandlerPosBatonPass(BattleEventItem *item, BtlServerFlow *flow, u8 pos, s32 *work);

// Each position effect and the function that returns its handlers
static const PosEventAddEntry sPosEventAddTable[] = {
    { 0, EventAddPosWish },        { 1, EventAddPosLunarDance }, { 2, EventAddPosHealingWish },
    { 3, EventAddPosFutureSight }, { 4, EventAddPosBatonPass },
};

BOOL PosEventAdd(u32 effect, u8 pos, u8 monId, const u32 *args, u8 argCount) {
    u32 i;
    u32 j;
    u32 numHandlers;
    const BattleEventHandlerEntry *handlers;
    BattleEventItem *item;

    for (i = 0; i < NELEMS(sPosEventAddTable); i++) {
        if (effect == sPosEventAddTable[i].effect) {
            if (!IsPosEffectFreeCore(effect, pos)) {
                break;
            }
            handlers = sPosEventAddTable[i].eventAdd(&numHandlers);
            item = BattleEvent_AddItem(BTL_EVENT_FACTOR_POS, effect, 1, 0, pos, handlers, numHandlers);
            if (item != NULL) {
                for (j = 0; j < argCount; j++) {
                    BattleEventItem_SetWorkValue(item, j, args[j]);
                }
                BattleEventItem_SetWorkValue(item, 6, monId);
            }
            return (BOOL)item;
        }
    }
    return FALSE;
}

BOOL IsPosEffectFree(u32 effect, u8 pos) {
    return IsPosEffectFreeCore(effect, pos) ? FALSE : TRUE;
}

// Whether an event of the effect is already set
static BOOL IsPosEffectFreeCore(u32 effect, u8 pos) {
    BattleEventItem *item;

    for (item = BattleEvent_SeekItem(BTL_EVENT_FACTOR_POS, pos); item != NULL; item = BattleEvent_GetNextItem(item)) {
        if (effect == BattleEventItem_GetSubID(item)) {
            return FALSE;
        }
    }
    return TRUE;
}

static const BattleEventHandlerEntry sHandlersPosWish[] = {
    { BTL_EVENT_TURN_CHECK_BEGIN, HandlerPosWish },
};

static const BattleEventHandlerEntry *EventAddPosWish(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersPosWish);
    return sHandlersPosWish;
}

static void HandlerPosWish(BattleEventItem *item, BtlServerFlow *flow, u8 pos, s32 *work) {
    BattleMon *wisher;
    BattleHandlerRecoverHPParam *recover;
    u8 wisherId;
    u8 targetId;

    if (BattleEventVar_GetValue(BTL_EVAR_MON) == 0x1f && !BtlServerFlow_CheckMatchup(flow)) {
        if (work[0] == 0) {
            work[1] = GetTurnCounter(flow);
            work[0] = 1;
            return;
        }
        if (GetTurnCounter(flow) > (u32)work[1]) {
            targetId = BtlServerFlow_PosToMonID(flow, pos);
            if (targetId != 0x1f && !IsMonFullHP(GetBattleMon(flow, targetId))) {
                wisherId = work[6];
                wisher = GetBattleMon(flow, wisherId);
                recover = BattleHandler_PushWork(flow, BTL_HANDLER_RECOVER_HP, wisherId);
                recover->targetIndex = targetId;
                recover->amount = DivideMaxHPZeroCheck(wisher, 2);
                BattleHandler_StrSetup(&recover->string, 2, 0x2bc);
                BattleHandler_AddArg(&recover->string, wisherId);
                BattleHandler_PopWork(flow, recover);
            }
            BattleEventItem_Remove(item);
        }
    }
}

static const BattleEventHandlerEntry sHandlersPosLunarDance[] = {
    { BTL_EVENT_MEMBER_IN, HandlerPosLunarDance },
    { BTL_EVENT_AFTER_MOVE, HandlerPosLunarDance },
};

static const BattleEventHandlerEntry *EventAddPosLunarDance(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersPosLunarDance);
    return sHandlersPosLunarDance;
}

static void HandlerPosLunarDance(BattleEventItem *item, BtlServerFlow *flow, u8 pos, s32 *work) {
    u8 monId;
    BattleMon *mon;
    BOOL needsHeal;
    u32 i;
    u32 j;
    u32 moveCount;
    u32 ppUsed;
    BattleHandlerEffectAtPosParam *effect;
    BattleHandlerRecoverHPParam *recover;
    BattleHandlerCureConditionParam *cure;
    BattleHandlerPPParam *pp;

    monId = BattleEventVar_GetValue(BTL_EVAR_MON);
    if (pos != BtlServerFlow_MonIDToPos(flow, monId)) {
        return;
    }
    mon = GetBattleMon(flow, monId);
    needsHeal = FALSE;
    if (!IsMonFullHP(mon)) {
        needsHeal = TRUE;
    }
    if (GetBattleMonStatus(mon)) {
        needsHeal = TRUE;
    }
    moveCount = GetBattleMonMoveCount(mon);
    for (i = 0; i < moveCount; i++) {
        if (GetMovePPUsed(mon, i)) {
            needsHeal = TRUE;
        }
    }
    if (needsHeal) {
        effect = BattleHandler_PushWork(flow, BTL_HANDLER_EFFECT_AT_POS, monId);
        effect->effect = 0x280;
        effect->pos1 = pos;
        effect->pos2 = 6;
        effect->hideMessageWindow = 1;
        BattleHandler_StrSetup(&effect->string, 2, 0x2b6);
        BattleHandler_AddArg(&effect->string, monId);
        BattleHandler_PopWork(flow, effect);

        if (!IsMonFullHP(mon)) {
            recover = BattleHandler_PushWork(flow, BTL_HANDLER_RECOVER_HP, monId);
            recover->targetIndex = monId;
            recover->amount = GetBattleMonStat(mon, BATTLEMON_MAX_HP) - GetBattleMonStat(mon, BATTLEMON_HP);
            BattleHandler_PopWork(flow, recover);
        }
        if (GetBattleMonStatus(mon)) {
            cure = BattleHandler_PushWork(flow, BTL_HANDLER_CURE_CONDITION, monId);
            cure->count = 1;
            cure->monIds[0] = monId;
            cure->condition = 0x24;
            BattleHandler_PopWork(flow, cure);
        }
        moveCount = GetBattleMonMoveCount(mon);
        for (j = 0; j < moveCount; j++) {
            ppUsed = GetMovePPUsed(mon, j);
            if (ppUsed) {
                pp = BattleHandler_PushWork(flow, BTL_HANDLER_RECOVER_PP, monId);
                pp->monIndex = monId;
                pp->amount = ppUsed;
                pp->moveIndex = j;
                BattleHandler_PopWork(flow, pp);
            }
        }
    }
    BattleEventItem_Remove(item);
}

static const BattleEventHandlerEntry sHandlersPosHealingWish[] = {
    { BTL_EVENT_MEMBER_IN, HandlerPosHealingWish },
    { BTL_EVENT_AFTER_MOVE, HandlerPosHealingWish },
};

static const BattleEventHandlerEntry *EventAddPosHealingWish(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersPosHealingWish);
    return sHandlersPosHealingWish;
}

static void HandlerPosHealingWish(BattleEventItem *item, BtlServerFlow *flow, u8 pos, s32 *work) {
    u8 monId;
    BattleMon *mon;
    BOOL needsHeal;
    BattleHandlerEffectAtPosParam *effect;
    BattleHandlerRecoverHPParam *recover;
    BattleHandlerCureConditionParam *cure;

    monId = BattleEventVar_GetValue(BTL_EVAR_MON);
    if (pos != BtlServerFlow_MonIDToPos(flow, monId)) {
        return;
    }
    mon = GetBattleMon(flow, monId);
    needsHeal = FALSE;
    if (!IsMonFullHP(mon)) {
        needsHeal = TRUE;
    }
    if (GetBattleMonStatus(mon)) {
        needsHeal = TRUE;
    }
    if (needsHeal) {
        effect = BattleHandler_PushWork(flow, BTL_HANDLER_EFFECT_AT_POS, monId);
        effect->effect = 0x27f;
        effect->pos1 = pos;
        effect->pos2 = 6;
        BattleHandler_StrSetup(&effect->string, 2, 0x2b9);
        BattleHandler_AddArg(&effect->string, monId);
        BattleHandler_PopWork(flow, effect);

        if (!IsMonFullHP(mon)) {
            recover = BattleHandler_PushWork(flow, BTL_HANDLER_RECOVER_HP, monId);
            recover->targetIndex = monId;
            recover->amount = GetBattleMonStat(mon, BATTLEMON_MAX_HP) - GetBattleMonStat(mon, BATTLEMON_HP);
            BattleHandler_PopWork(flow, recover);
        }
        if (GetBattleMonStatus(mon)) {
            cure = BattleHandler_PushWork(flow, BTL_HANDLER_CURE_CONDITION, monId);
            cure->count = 1;
            cure->monIds[0] = monId;
            cure->condition = 0x24;
            BattleHandler_PopWork(flow, cure);
        }
    }
    BattleEventItem_Remove(item);
}

static const BattleEventHandlerEntry sHandlersPosFutureSight[] = {
    { BTL_EVENT_TURN_CHECK_BEGIN, HandlerPosFutureSight },
};

static const BattleEventHandlerEntry *EventAddPosFutureSight(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersPosFutureSight);
    return sHandlersPosFutureSight;
}

static void HandlerPosFutureSight(BattleEventItem *item, BtlServerFlow *flow, u8 pos, s32 *work) {
    u8 turn;
    u8 targetId;
    u8 attackerId;
    BattleHandlerMessageParam *msg;
    BattleHandlerDelayMoveDamageParam *damage;

    if (BattleEventVar_GetValue(BTL_EVAR_MON) == 0x1f && !BtlServerFlow_CheckMatchup(flow)) {
        turn = GetTurnCounter(flow);
        if (turn >= work[0]) {
            targetId = BtlServerFlow_PosToMonID(flow, pos);
            attackerId = work[6];
            if (targetId != 0x1f && targetId != attackerId && !IsFainted(GetBattleMon(flow, targetId))) {
                msg = BattleHandler_PushWork(flow, BTL_HANDLER_MESSAGE, attackerId);
                BattleHandler_StrSetup(&msg->string, 2, 0x438);
                BattleHandler_AddArg(&msg->string, targetId);
                BattleHandler_AddArg(&msg->string, work[1]);
                BattleHandler_PopWork(flow, msg);

                damage = BattleHandler_PushWork(flow, BTL_HANDLER_DELAY_MOVE_DAMAGE, attackerId);
                damage->attackerIndex = attackerId;
                damage->targetIndex = targetId;
                damage->move = work[1];
                BattleHandler_PopWork(flow, damage);
            }
            BattleEventItem_Remove(item);
        }
    }
}

static const BattleEventHandlerEntry sHandlersPosBatonPass[] = {
    { BTL_EVENT_MEMBER_IN, HandlerPosBatonPass },
};

static const BattleEventHandlerEntry *EventAddPosBatonPass(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersPosBatonPass);
    return sHandlersPosBatonPass;
}

static void HandlerPosBatonPass(BattleEventItem *item, BtlServerFlow *flow, u8 pos, s32 *work) {
    u8 monId;
    BattleHandlerBatonPassParam *param;

    monId = BattleEventVar_GetValue(BTL_EVAR_MON);
    if (monId == BtlServerFlow_PosToMonID(flow, pos)) {
        param = BattleHandler_PushWork(flow, BTL_HANDLER_BATON_PASS, 0x1f);
        param->sourceMonIndex = work[0];
        param->targetMonIndex = monId;
        BattleHandler_PopWork(flow, param);
        BattleEventItem_Remove(item);
    }
}
