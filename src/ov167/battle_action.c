#include "types.h"
#include "battle/btl_action.h"

// Function names from swan.
void BattleAction_SetFightParam(BattleAction *action, u16 move, u8 target) {
    action->raw = 0;
    action->bits.action = BATTLE_ACTION_FIGHT;
    action->bits.target = target;
    action->bits.move = move;
}

void BattleAction_ChangeFightTargetPos(BattleAction *action, u8 target) {
    if (action->bits.action == BATTLE_ACTION_FIGHT && target != 6) {
        action->bits.target = target;
    }
}

void BattleAction_SetMoveInfoFlag(BattleAction *action) {
    if (action->bits.action == BATTLE_ACTION_FIGHT) {
        action->bits.unk26 = 1;
    }
}

BOOL BattleAction_GetMoveInfoFlag(const BattleAction *action) {
    if (action->bits.action == BATTLE_ACTION_FIGHT) {
        return action->bits.unk26;
    }
    return FALSE;
}

u16 BattleAction_GetFightMove(const BattleAction *action) {
    if (action->bits.action == BATTLE_ACTION_FIGHT) {
        return action->bits.move;
    }
    return 0;
}

void BattleAction_SetItemParam(BattleAction *action, u16 item, u8 target, u8 param) {
    action->raw = 0;
    action->item.action = BATTLE_ACTION_ITEM;
    action->item.item = item;
    action->item.target = target;
    action->item.param = param;
}

void BattleAction_SetChangeParam(BattleAction *action, u8 pos, u8 slot) {
    action->raw = 0;
    action->change.action = BATTLE_ACTION_CHANGE;
    action->change.unk4 = pos;
    action->change.slot = slot;
    action->change.unk10 = 0;
}

void BattleAction_SetChangeDepleteParam(BattleAction *action) {
    action->raw = 0;
    action->change.action = BATTLE_ACTION_CHANGE;
    action->change.slot = 0;
    action->change.unk10 = 1;
}

BOOL BattleAction_IsChangeDeplete(const BattleAction *action) {
    if (action->change.action == BATTLE_ACTION_CHANGE && action->change.unk10 == 1) {
        return TRUE;
    }
    return FALSE;
}

void BattleAction_SetRotationParam(BattleAction *action, u8 target) {
    action->raw = 0;
    action->bits.action = BATTLE_ACTION_ROTATION;
    action->bits.target = target;
}

void BattleAction_SetEscape(BattleAction *action) {
    action->bits.action = BATTLE_ACTION_ESCAPE;
}

void BattleAction_SetShift(BattleAction *action) {
    action->bits.action = BATTLE_ACTION_SHIFT;
}

// Function name from swan.
void BattleAction_SetNull(BattleAction *action) {
    action->bits.action = BATTLE_ACTION_NULL;
    action->raw &= 0xf;
}

void BattleAction_SetSkip(BattleAction *action) {
    action->bits.action = BATTLE_ACTION_SKIP;
}

u32 BattleAction_GetAction(const BattleAction *action) {
    return action->bits.action;
}

void BattleAction_SetRecTimeout(BattleAction *action) {
    action->bits.action = BATTLE_ACTION_REC_TIMEOUT;
    action->raw &= 0xf;
}

void BattleAction_SetRecError(BattleAction *action) {
    action->bits.action = BATTLE_ACTION_REC_ERROR;
    action->raw &= 0xf;
}
