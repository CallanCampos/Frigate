#include "battle/side_handlers.h"
#include "types.h"
#include "battle/btl_calc.h"
#include "battle/btl_event.h"
#include "battle/btl_handler.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_server_flow.h"
#include "constants/battle.h"
#include "constants/types.h"
#include "gfl/std.h"
#include "pml/move.h"

#define NUM_SIDES 2
#define NUM_SIDE_EFFECTS 14

// One entry of the side effect table: an effect, the function that returns its handlers, and how many layers it takes
typedef struct SideEffectEventAddEntry {
    u32 effect;
    const BattleEventHandlerEntry *(*eventAdd)(u32 *numHandlers);
    u32 maxLayers;
} SideEffectEventAddEntry;

// A side effect that is set on a side
typedef struct SideEffectSlot {
    BattleEventItem *item;
    BattleCondition cont;
    u32 turnsElapsed;
    u32 layers;
} SideEffectSlot;

static SideEffectSlot sSideEffects[NUM_SIDES][NUM_SIDE_EFFECTS];

static u8 GetSideEffectLayersOfItem(BattleEventItem *item, u32 side);
static const BattleEventHandlerEntry *EventAddSideReflect(u32 *numHandlers);
static void HandlerSideReflect(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideLightScreen(u32 *numHandlers);
static void HandlerSideLightScreen(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static void CommonScreenEffect(BtlServerFlow *flow, u8 side, u32 screen);
static const BattleEventHandlerEntry *EventAddSideSafeguard(u32 *numHandlers);
static void HandlerSideSafeguardCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static void HandlerSideSafeguardFixFail(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideMist(u32 *numHandlers);
static void HandlerSideMistCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static void HandlerSideMistFixFail(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideTailwind(u32 *numHandlers);
static void HandlerSideTailwind(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideLuckyChant(u32 *numHandlers);
static void HandlerSideLuckyChant(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideStealthRock(u32 *numHandlers);
static void HandlerSideStealthRock(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideWideGuard(u32 *numHandlers);
static void HandlerSideWideGuard(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideQuickGuard(u32 *numHandlers);
static void HandlerSideQuickGuard(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideSpikes(u32 *numHandlers);
static void HandlerSideSpikes(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideToxicSpikes(u32 *numHandlers);
static void HandlerSideToxicSpikes(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideRainbow(u32 *numHandlers);
static void HandlerSideRainbow(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static void HandlerSideRainbowShrink(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideSeaOfFire(u32 *numHandlers);
static void HandlerSideSeaOfFire(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);
static const BattleEventHandlerEntry *EventAddSideSwamp(u32 *numHandlers);
static void HandlerSideSwamp(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work);

// Each side effect and the function that returns its handlers
static const SideEffectEventAddEntry sSideEffectEventAddTable[NUM_SIDE_EFFECTS] = {
    { SIDE_CONDITION_REFLECT, EventAddSideReflect, 1 },
    { SIDE_CONDITION_LIGHT_SCREEN, EventAddSideLightScreen, 1 },
    { SIDE_CONDITION_SAFEGUARD, EventAddSideSafeguard, 1 },
    { SIDE_CONDITION_MIST, EventAddSideMist, 1 },
    { SIDE_CONDITION_TAILWIND, EventAddSideTailwind, 1 },
    { SIDE_CONDITION_LUCKY_CHANT, EventAddSideLuckyChant, 1 },
    { SIDE_CONDITION_SPIKES, EventAddSideSpikes, 3 },
    { SIDE_CONDITION_TOXIC_SPIKES, EventAddSideToxicSpikes, 2 },
    { SIDE_CONDITION_STEALTH_ROCK, EventAddSideStealthRock, 1 },
    { SIDE_CONDITION_WIDE_GUARD, EventAddSideWideGuard, 1 },
    { SIDE_CONDITION_QUICK_GUARD, EventAddSideQuickGuard, 1 },
    { SIDE_CONDITION_RAINBOW, EventAddSideRainbow, 1 },
    { SIDE_CONDITION_SEA_OF_FIRE, EventAddSideSeaOfFire, 1 },
    { SIDE_CONDITION_SWAMP, EventAddSideSwamp, 1 },
};

void SideEffectsInit(void) {
    MI_CpuFill8(sSideEffects, 0, sizeof(sSideEffects));
}

BattleEventItem *SideEffectEventAdd(u8 side, u32 effect, BattleCondition cont) {
    u32 i;
    u32 numHandlers;
    const BattleEventHandlerEntry *handlers;
    BattleEventItem *item;
    SideEffectSlot *slot = &sSideEffects[side][effect];

    for (i = 0; i < NUM_SIDE_EFFECTS; i++) {
        if (effect == sSideEffectEventAddTable[i].effect) {
            if (slot->layers == 0) {
                handlers = sSideEffectEventAddTable[i].eventAdd(&numHandlers);
                item = BattleEvent_AddItem(BTL_EVENT_FACTOR_SIDE, effect, 2, 0, side, handlers, numHandlers);
                BattleEventItem_SetWorkValue(item, 6, cont.raw);
                slot->layers = 1;
                slot->turnsElapsed = 0;
                slot->cont = cont;
                slot->item = item;
                return item;
            }
            if (slot->layers < sSideEffectEventAddTable[i].maxLayers) {
                slot->layers++;
                return slot->item;
            }
        }
    }
    return NULL;
}

u32 GetSideEffectLayers(u8 side, u32 effect) {
    return sSideEffects[side][effect].layers;
}

BOOL IsSideEffectSet(u8 side, u32 effect) {
    if (sSideEffects[side][effect].item != NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL ServerDisplay_RemoveSideEffect(u8 side, u32 effect) {
    SideEffectSlot *slot = &sSideEffects[side][effect];

    if (slot->item != NULL) {
        BattleEventItem_Remove(slot->item);
        slot->item = NULL;
        slot->layers = 0;
        slot->cont.common.type = BTL_CONDITION_TYPE_NONE;
        return TRUE;
    }
    return FALSE;
}

void SideEffectsEndTurn(void (*callback)(u8 side, u32 effect, BtlServerFlow *flow), BtlServerFlow *flow) {
    u32 side;
    u32 effect;
    SideEffectSlot *slot;

    for (side = 0; side < NUM_SIDES; side++) {
        for (effect = 0; effect < NUM_SIDE_EFFECTS; effect++) {
            slot = &sSideEffects[side][effect];
            if (slot->layers != 0 && slot->cont.common.type == BTL_CONDITION_TYPE_TURNS) {
                slot->turnsElapsed++;
                if (slot->turnsElapsed >= slot->cont.common.turns) {
                    slot->layers = 0;
                    slot->cont.common.type = BTL_CONDITION_TYPE_NONE;
                    BattleEventItem_Remove(slot->item);
                    slot->item = NULL;
                    callback(side, effect, flow);
                }
            }
        }
    }
}

// How many layers the side effect of the item has on the side
static u8 GetSideEffectLayersOfItem(BattleEventItem *item, u32 side) {
    s32 effect = BattleEventItem_GetSubID(item);

    if (effect < NUM_SIDE_EFFECTS && side < NUM_SIDES) {
        return sSideEffects[side][effect].layers;
    }
    return 0;
}

static const BattleEventHandlerEntry sHandlersSideReflect[] = {
    { BTL_EVENT_CALC_DAMAGE, HandlerSideReflect },
};

static const BattleEventHandlerEntry *EventAddSideReflect(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideReflect);
    return sHandlersSideReflect;
}

static void HandlerSideReflect(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    CommonScreenEffect(flow, side, 1);
}

static const BattleEventHandlerEntry sHandlersSideLightScreen[] = {
    { BTL_EVENT_CALC_DAMAGE, HandlerSideLightScreen },
};

static const BattleEventHandlerEntry *EventAddSideLightScreen(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideLightScreen);
    return sHandlersSideLightScreen;
}

static void HandlerSideLightScreen(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    CommonScreenEffect(flow, side, 2);
}

// Reflect and Light Screen cut the damage of the screen's category of moves, by a third in a double battle
static void CommonScreenEffect(BtlServerFlow *flow, u8 side, u32 screen) {
    u32 battleStyle;
    u32 multiplier;

    if (side == GetSideFromMonID(BattleEventVar_GetValue(BTL_EVAR_DEFENDER)) &&
        screen == BattleEventVar_GetValue(BTL_EVAR_MOVE_CATEGORY) && BattleEventVar_GetValue(0x45) == 0) {
        battleStyle = BtlServerFlow_GetBattleStyle(flow);
        if (battleStyle == BTL_STYLE_SINGLE || battleStyle == BTL_STYLE_ROTATION) {
            multiplier = 0x800;
        } else {
            multiplier = 0xa8f;
        }
        BattleEventVar_MulValue(BTL_EVAR_RATIO, multiplier);
    }
}

static const BattleEventHandlerEntry sHandlersSideSafeguard[] = {
    { BTL_EVENT_MOVE_CONDITION_CHECK_FAIL, HandlerSideSafeguardCheckFail },
    { BTL_EVENT_ADD_CONDITION_FAILED, HandlerSideSafeguardFixFail },
};

static const BattleEventHandlerEntry *EventAddSideSafeguard(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideSafeguard);
    return sHandlersSideSafeguard;
}

static void HandlerSideSafeguardCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    u8 attackerId;
    u32 condition;

    attackerId = BattleEventVar_GetValue(BTL_EVAR_DEFENDER);
    if (side == GetSideFromMonID(attackerId) && attackerId != BattleEventVar_GetValue(BTL_EVAR_ATTACKER)) {
        condition = BattleEventVar_GetValue(BTL_EVAR_CONDITION);
        if (IsBasicStatus(condition) || condition == CONDITION_CONFUSION || condition == CONDITION_YAWN) {
            work[0] = BattleEventVar_RewriteValue(BTL_EVAR_FAIL_FLAG, 1);
        }
    }
}

static void HandlerSideSafeguardFixFail(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    BattleHandlerMessageParam *message;
    u8 attackerId;

    if (work[0] != 0) {
        attackerId = BattleEventVar_GetValue(BTL_EVAR_DEFENDER);
        message = BattleHandler_PushWork(flow, BTL_HANDLER_MESSAGE, attackerId);
        BattleHandler_StrSetup(&message->string, 2, 0x347);
        BattleHandler_AddArg(&message->string, attackerId);
        BattleHandler_PopWork(flow, message);
        work[0] = 0;
    }
}

static const BattleEventHandlerEntry sHandlersSideMist[] = {
    { BTL_EVENT_CHECK_STAT_CHANGE, HandlerSideMistCheckFail },
    { BTL_EVENT_STAT_CHANGE_FAILED, HandlerSideMistFixFail },
};

static const BattleEventHandlerEntry *EventAddSideMist(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideMist);
    return sHandlersSideMist;
}

static void HandlerSideMistCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    u8 targetId;

    targetId = BattleEventVar_GetValue(BTL_EVAR_MON);
    if (side == GetSideFromMonID(targetId) && (s32)BattleEventVar_GetValue(BTL_EVAR_AMOUNT) < 0 &&
        targetId != BattleEventVar_GetValue(BTL_EVAR_ATTACKER)) {
        work[0] = BattleEventVar_RewriteValue(BTL_EVAR_FAIL_FLAG, 1);
    }
}

static void HandlerSideMistFixFail(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    BattleHandlerMessageParam *message;
    u32 serial;
    u8 targetId;

    if (work[0] != 0) {
        serial = BattleEventVar_GetValue(BTL_EVAR_STAT_CHANGE_SERIAL);
        if (serial == 0 || work[1] != serial) {
            targetId = BattleEventVar_GetValue(BTL_EVAR_MON);
            message = BattleHandler_PushWork(flow, BTL_HANDLER_MESSAGE, targetId);
            BattleHandler_StrSetup(&message->string, 2, 0x34a);
            BattleHandler_AddArg(&message->string, targetId);
            BattleHandler_PopWork(flow, message);
            work[1] = serial;
        }
        work[0] = 0;
    }
}

static const BattleEventHandlerEntry sHandlersSideTailwind[] = {
    { BTL_EVENT_CALC_SPEED, HandlerSideTailwind },
};

static const BattleEventHandlerEntry *EventAddSideTailwind(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideTailwind);
    return sHandlersSideTailwind;
}

static void HandlerSideTailwind(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    if (side == GetSideFromMonID(BattleEventVar_GetValue(BTL_EVAR_MON))) {
        BattleEventVar_MulValue(BTL_EVAR_RATIO, 0x2000);
    }
}

static const BattleEventHandlerEntry sHandlersSideLuckyChant[] = {
    { BTL_EVENT_CHECK_CRITICAL, HandlerSideLuckyChant },
};

static const BattleEventHandlerEntry *EventAddSideLuckyChant(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideLuckyChant);
    return sHandlersSideLuckyChant;
}

static void HandlerSideLuckyChant(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    if (side == GetSideFromMonID(BattleEventVar_GetValue(BTL_EVAR_DEFENDER))) {
        BattleEventVar_RewriteValue(BTL_EVAR_FAIL_FLAG, 1);
    }
}

static const BattleEventHandlerEntry sHandlersSideStealthRock[] = {
    { BTL_EVENT_MEMBER_IN, HandlerSideStealthRock },
};

static const BattleEventHandlerEntry *EventAddSideStealthRock(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideStealthRock);
    return sHandlersSideStealthRock;
}

static void HandlerSideStealthRock(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    u8 monId;
    BattleMon *mon;
    u32 divisor;
    BattleHandlerDamageParam *damage;

    divisor = 2;
    monId = BattleEventVar_GetValue(BTL_EVAR_MON);
    if (side == GetSideFromMonID(monId)) {
        mon = GetBattleMon(flow, monId);
        switch (BtlCalc_GetTypePairEffectiveness(TYPE_ROCK, GetPokeType(mon))) {
        case TYPE_EFFECTIVENESS_IMMUNE:
        case TYPE_EFFECTIVENESS_QUARTER:
        default:
            divisor = 32;
            break;
        case TYPE_EFFECTIVENESS_HALF:
            divisor = 16;
            break;
        case TYPE_EFFECTIVENESS_NORMAL:
            divisor = 8;
            break;
        case TYPE_EFFECTIVENESS_DOUBLE:
            divisor = 4;
            break;
        case TYPE_EFFECTIVENESS_QUADRUPLE:
            break;
        }
        damage = BattleHandler_PushWork(flow, BTL_HANDLER_DAMAGE, 0x1f);
        damage->targetIndex = monId;
        damage->amount = DivideMaxHPZeroCheck(mon, divisor);
        BattleHandler_StrSetup(&damage->string, 2, 0x356);
        BattleHandler_AddArg(&damage->string, monId);
        BattleHandler_PopWork(flow, damage);
    }
}

static const BattleEventHandlerEntry sHandlersSideWideGuard[] = {
    { BTL_EVENT_CHECK_NO_EFFECT_2, HandlerSideWideGuard },
};

static const BattleEventHandlerEntry *EventAddSideWideGuard(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideWideGuard);
    return sHandlersSideWideGuard;
}

static void HandlerSideWideGuard(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    u8 targetId;
    u16 move;
    u32 target;
    BattleHandlerString *string;

    targetId = BattleEventVar_GetValue(BTL_EVAR_DEFENDER);
    if (side == GetSideFromMonID(targetId)) {
        move = BattleEventVar_GetValue(BTL_EVAR_MOVE);
        target = PML_MoveGetParam(move, MOVE_PARAM_TARGET);
        if (PML_MoveIsDamaging(move) && (target == MOVE_TARGET_ALL_ADJACENT || target == MOVE_TARGET_ADJACENT_FOES) &&
            BattleEventVar_RewriteValue(BTL_EVAR_NO_EFFECT_FLAG, 1)) {
            string = (BattleHandlerString *)BattleEventVar_GetValue(BTL_EVAR_STRING);
            BattleHandler_StrSetup(string, 2, 0x31d);
            BattleHandler_AddArg(string, targetId);
        }
    }
}

static const BattleEventHandlerEntry sHandlersSideQuickGuard[] = {
    { BTL_EVENT_CHECK_NO_EFFECT_2, HandlerSideQuickGuard },
};

static const BattleEventHandlerEntry *EventAddSideQuickGuard(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideQuickGuard);
    return sHandlersSideQuickGuard;
}

static void HandlerSideQuickGuard(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    u8 attackerId;
    u16 move;
    BattleHandlerString *string;

    attackerId = BattleEventVar_GetValue(BTL_EVAR_DEFENDER);
    if (side == GetSideFromMonID(attackerId) && attackerId != BattleEventVar_GetValue(BTL_EVAR_ATTACKER)) {
        move = BattleEventVar_GetValue(BTL_EVAR_MOVE);
        if (PML_MoveGetParam(move, MOVE_PARAM_PRIORITY) >= 1 &&
            BattleEventVar_RewriteValue(BTL_EVAR_NO_EFFECT_FLAG, 1)) {
            string = (BattleHandlerString *)BattleEventVar_GetValue(BTL_EVAR_STRING);
            BattleHandler_StrSetup(string, 2, 0x320);
            BattleHandler_AddArg(string, attackerId);
        }
    }
}

static const BattleEventHandlerEntry sHandlersSideSpikes[] = {
    { BTL_EVENT_MEMBER_IN, HandlerSideSpikes },
};

static const BattleEventHandlerEntry *EventAddSideSpikes(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideSpikes);
    return sHandlersSideSpikes;
}

static void HandlerSideSpikes(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    u8 monId;
    BattleMon *mon;
    u32 divisor;
    BattleHandlerDamageParam *damage;

    monId = BattleEventVar_GetValue(BTL_EVAR_MON);
    if (side == GetSideFromMonID(monId) && !BtlServerFlow_IsFloating(flow, monId)) {
        switch (GetSideEffectLayersOfItem(item, side)) {
        case 1:
        default:
            divisor = 8;
            break;
        case 2:
            divisor = 6;
            break;
        case 3:
            divisor = 4;
            break;
        }
        damage = BattleHandler_PushWork(flow, BTL_HANDLER_DAMAGE, 0x1f);
        mon = GetBattleMon(flow, monId);
        damage->targetIndex = monId;
        damage->amount = DivideMaxHPZeroCheck(mon, divisor);
        BattleHandler_StrSetup(&damage->string, 2, 0x353);
        BattleHandler_AddArg(&damage->string, monId);
        BattleHandler_PopWork(flow, damage);
    }
}

static const BattleEventHandlerEntry sHandlersSideToxicSpikes[] = {
    { BTL_EVENT_MEMBER_IN, HandlerSideToxicSpikes },
};

static const BattleEventHandlerEntry *EventAddSideToxicSpikes(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideToxicSpikes);
    return sHandlersSideToxicSpikes;
}

// A grounded Pokémon of the side poisons itself on the spikes, and a Poison type absorbs them
static void HandlerSideToxicSpikes(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    u8 monId;
    BattleHandlerRemoveSideEffectParam *remove;
    BattleHandlerAddConditionParam *condition;

    monId = BattleEventVar_GetValue(BTL_EVAR_MON);
    if (side == GetSideFromMonID(monId) && !BtlServerFlow_IsFloating(flow, monId)) {
        if (DoesMonHaveType(GetBattleMon(flow, monId), TYPE_POISON)) {
            remove = BattleHandler_PushWork(flow, BTL_HANDLER_REMOVE_SIDE_EFFECT, monId);
            remove->side = side;
            BattleHandler_InitFlags(remove->effects, sizeof(remove->effects));
            BattleHandler_SetFlag(remove->effects, SIDE_CONDITION_TOXIC_SPIKES);
            BattleHandler_PopWork(flow, remove);
        } else {
            condition = BattleHandler_PushWork(flow, BTL_HANDLER_ADD_CONDITION, 0x1f);
            condition->condition = CONDITION_POISON;
            if (GetSideEffectLayersOfItem(item, side) > 1) {
                condition->value = MakeConditionBadlyPoisoned();
            } else {
                condition->value = MakeConditionPermanent();
            }
            condition->targetIndex = monId;
            BattleHandler_PopWork(flow, condition);
        }
    }
}

static const BattleEventHandlerEntry sHandlersSideRainbow[] = {
    { BTL_EVENT_CHECK_MOVE_ADD_CONDITION, HandlerSideRainbow },
    { BTL_EVENT_STAT_CHANGE_CHANCE, HandlerSideRainbow },
    { BTL_EVENT_GET_FLINCH_CHANCE, HandlerSideRainbowShrink },
};

static const BattleEventHandlerEntry *EventAddSideRainbow(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideRainbow);
    return sHandlersSideRainbow;
}

// Doubles the chance of a move's added effect
static void HandlerSideRainbow(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    u16 chance;

    if (side == GetSideFromMonID(BattleEventVar_GetValue(BTL_EVAR_ATTACKER))) {
        chance = BattleEventVar_GetValue(BTL_EVAR_CHANCE) * 2;
        BattleEventVar_RewriteValue(BTL_EVAR_CHANCE, chance);
    }
}

static void HandlerSideRainbowShrink(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    if (side == GetSideFromMonID(BattleEventVar_GetValue(BTL_EVAR_ATTACKER))) {
        BattleEventVar_RewriteValue(0x45, 1);
    }
}

static const BattleEventHandlerEntry sHandlersSideSeaOfFire[] = {
    { BTL_EVENT_TURN_CHECK_BEGIN, HandlerSideSeaOfFire },
};

static const BattleEventHandlerEntry *EventAddSideSeaOfFire(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideSeaOfFire);
    return sHandlersSideSeaOfFire;
}

static void HandlerSideSeaOfFire(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    u8 monId;
    BattleMon *mon;
    BattleHandlerEffectAtPosParam *effect;
    BattleHandlerDamageParam *damage;

    monId = BattleEventVar_GetValue(BTL_EVAR_MON);
    if (monId != 0x1f && BtlServerFlow_IsMonInBattle(flow, monId) && !BtlServerFlow_CheckMatchup(flow) &&
        side == GetSideFromMonID(monId)) {
        mon = GetBattleMon(flow, monId);
        if (!DoesMonHaveType(mon, TYPE_FIRE)) {
            effect = BattleHandler_PushWork(flow, BTL_HANDLER_EFFECT_AT_POS, monId);
            effect->pos1 = BtlServerFlow_MonIDToPos(flow, monId);
            effect->pos2 = 6;
            effect->effect = 0x28e;
            BattleHandler_PopWork(flow, effect);

            damage = BattleHandler_PushWork(flow, BTL_HANDLER_DAMAGE, 0x1f);
            damage->targetIndex = monId;
            damage->amount = DivideMaxHPZeroCheck(mon, 8);
            BattleHandler_StrSetup(&damage->string, 2, 0x484);
            BattleHandler_AddArg(&damage->string, monId);
            BattleHandler_PopWork(flow, damage);
        }
    }
}

static const BattleEventHandlerEntry sHandlersSideSwamp[] = {
    { BTL_EVENT_CALC_SPEED, HandlerSideSwamp },
};

static const BattleEventHandlerEntry *EventAddSideSwamp(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSideSwamp);
    return sHandlersSideSwamp;
}

static void HandlerSideSwamp(BattleEventItem *item, BtlServerFlow *flow, u8 side, s32 *work) {
    if (side == GetSideFromMonID(BattleEventVar_GetValue(BTL_EVAR_MON))) {
        BattleEventVar_MulValue(BTL_EVAR_RATIO, 0x400);
    }
}
