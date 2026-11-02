#include "types.h"
#include "battle/btl_pokeparam.h"

// Function names from swan.
BattleCondition ZeroConditionTurns(void) {
    BattleCondition condition;

    condition.raw = 0;
    condition.common.type = BTL_CONDITION_TYPE_NONE;
    return condition;
}

BOOL Condition_IsNull(BattleCondition condition) {
    if (condition.common.type == BTL_CONDITION_TYPE_NONE) {
        return TRUE;
    }
    return FALSE;
}

BattleCondition SetConditionTurns(u8 turns) {
    BattleCondition condition;

    condition.raw = 0;
    condition.common.type = BTL_CONDITION_TYPE_TURNS;
    condition.common.turns = turns;
    condition.timed.param = 0;
    return condition;
}

BattleCondition AddTurnCondition(u32 turns, u16 param) {
    BattleCondition condition;

    condition.raw = 0;
    condition.common.type = BTL_CONDITION_TYPE_TURNS;
    condition.common.turns = turns;
    condition.timed.param = param;
    return condition;
}

BattleCondition MakeConditionDependPoke(u32 monId) {
    BattleCondition condition;

    condition.raw = 0;
    condition.common.type = BTL_CONDITION_TYPE_DEPEND_MON;
    condition.common.turns = monId;
    return condition;
}

BattleCondition MakeConditionPermanent(void) {
    BattleCondition condition;

    condition.raw = 0;
    condition.common.type = BTL_CONDITION_TYPE_PERMANENT;
    return condition;
}

BattleCondition MakeConditionParamPermanent(u16 param) {
    BattleCondition condition;

    condition.raw = 0;
    condition.common.type = BTL_CONDITION_TYPE_PERMANENT;
    condition.timed.param = param;
    return condition;
}

BattleCondition MakeConditionPermanentTurnsParam(u8 turns, u16 param) {
    BattleCondition condition;

    condition.raw = 0;
    condition.common.type = BTL_CONDITION_TYPE_PERMANENT;
    condition.common.turns = turns;
    condition.timed.param = param;
    return condition;
}

BattleCondition MakeConditionDependPokeTurns(u32 monId, u8 turns) {
    BattleCondition condition;

    condition.raw = 0;
    condition.common.type = BTL_CONDITION_TYPE_DEPEND_MON_TURNS;
    condition.common.turns = turns;
    condition.mon.monId = monId;
    return condition;
}

BattleCondition MakeConditionBadlyPoisoned(void) {
    BattleCondition condition;

    condition.raw = 0;
    condition.common.type = BTL_CONDITION_TYPE_PERMANENT;
    condition.common.turns = BTL_CONDITION_BADLY_POISONED_TURNS;
    return condition;
}

BOOL Condition_IsBadlyPoisoned(BattleConditionCont cont) {
    u32 turns;
    u32 type;

    turns = cont.common.turns;
    type = cont.common.type;
    return type == BTL_CONDITION_TYPE_PERMANENT && turns == BTL_CONDITION_BADLY_POISONED_TURNS;
}

u8 Condition_GetMonID(BattleCondition condition) {
    u32 type;
    u32 mon1;
    u32 mon2;

    type = condition.common.type;
    mon1 = condition.common.turns;
    mon2 = condition.mon.monId;
    if (type == BTL_CONDITION_TYPE_DEPEND_MON) {
        return mon1;
    }
    if (type == BTL_CONDITION_TYPE_DEPEND_MON_TURNS) {
        return mon2;
    }
    return 31;
}

void Condition_SetMonID(BattleCondition *condition, u32 value) {
    if (condition->common.type == BTL_CONDITION_TYPE_DEPEND_MON) {
        condition->common.turns = value;
        return;
    }
    if (condition->common.type == BTL_CONDITION_TYPE_DEPEND_MON_TURNS) {
        condition->mon.monId = value;
    }
}

u8 Condition_GetTurns(BattleCondition condition) {
    u32 type;
    u32 turns;

    type = condition.common.type;
    turns = condition.common.turns;
    if (type == BTL_CONDITION_TYPE_TURNS) {
        return turns;
    }
    if (type == BTL_CONDITION_TYPE_DEPEND_MON_TURNS) {
        return turns;
    }
    return 0;
}

void Condition_SetParam(BattleCondition *condition, u16 value) {
    if (condition->common.type == BTL_CONDITION_TYPE_PERMANENT) {
        condition->timed.param = value;
        return;
    }
    if (condition->common.type == BTL_CONDITION_TYPE_DEPEND_MON) {
        condition->timed.param = value;
        return;
    }
    if (condition->common.type == BTL_CONDITION_TYPE_DEPEND_MON_TURNS) {
        condition->raw = (condition->raw & 0x80007fff) | (((u32)value << 16) >> 1);
        return;
    }
    if (condition->common.type == BTL_CONDITION_TYPE_TURNS) {
        condition->timed.param = value;
    }
}

u16 Condition_GetParam(BattleCondition condition) {
    if (condition.common.type == BTL_CONDITION_TYPE_PERMANENT) {
        return (u16)((condition.raw << 7) >> 16);
    }
    if (condition.common.type == BTL_CONDITION_TYPE_DEPEND_MON) {
        return (u16)((condition.raw << 7) >> 16);
    }
    if (condition.common.type == BTL_CONDITION_TYPE_DEPEND_MON_TURNS) {
        return (u16)((condition.raw << 1) >> 16);
    }
    if (condition.common.type == BTL_CONDITION_TYPE_TURNS) {
        return (u16)((condition.raw << 7) >> 16);
    }
    return 0;
}

void SetConditionFlag(BattleCondition *condition, u32 flag) {
    if (condition->common.type == BTL_CONDITION_TYPE_PERMANENT) {
        condition->one.flag = flag;
        return;
    }
    if (condition->common.type == BTL_CONDITION_TYPE_DEPEND_MON) {
        condition->one.flag = flag;
        return;
    }
    if (condition->common.type == BTL_CONDITION_TYPE_DEPEND_MON_TURNS) {
        condition->four.flag = flag;
        return;
    }
    if (condition->common.type == BTL_CONDITION_TYPE_TURNS) {
        condition->one.flag = flag;
    }
}

u32 GetConditionFlag(BattleCondition condition) {
    if (condition.common.type == BTL_CONDITION_TYPE_PERMANENT) {
        return condition.one.flag;
    }
    if (condition.common.type == BTL_CONDITION_TYPE_DEPEND_MON) {
        return condition.one.flag;
    }
    if (condition.common.type == BTL_CONDITION_TYPE_DEPEND_MON_TURNS) {
        return condition.four.flag;
    }
    if (condition.common.type == BTL_CONDITION_TYPE_TURNS) {
        return condition.one.flag;
    }
    return 0;
}

void IncrementTurn(BattleCondition *condition, u32 amount) {
    if (condition->common.type == BTL_CONDITION_TYPE_TURNS && condition->common.turns < 8) {
        condition->common.turns += amount;
    }
    if (condition->common.type == BTL_CONDITION_TYPE_DEPEND_MON_TURNS && condition->common.turns < 8) {
        condition->common.turns += amount;
    }
}

void SetTurns(BattleCondition *condition, u32 turns) {
    if (condition->common.type == BTL_CONDITION_TYPE_TURNS && condition->common.turns < 8) {
        condition->common.turns = turns;
    }
    if (condition->common.type == BTL_CONDITION_TYPE_DEPEND_MON_TURNS && condition->common.turns < 8) {
        condition->common.turns = turns;
    }
}
