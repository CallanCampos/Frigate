#ifndef POKEBW2_BATTLE_BTL_ACTION_H
#define POKEBW2_BATTLE_BTL_ACTION_H

#include "types.h"
#include "struct_decls.h"

union BattleAction {
    u32 raw;
    struct {
        u32 action : 4;
        u32 target : 3;
        u32 move : 16;
        u32 unk23 : 3;
        u32 unk26 : 1;
        u32 unk27 : 5;
    } bits;
    // An item use
    struct {
        u32 action : 4;
        u32 target : 3;
        u32 item : 16;
        u32 param : 8;
        u32 unk31 : 1;
    } item;
    // A switch, to the party slot
    struct {
        u32 action : 4;
        u32 unk4 : 3;
        u32 slot : 3;
        u32 unk10 : 1;
        u32 unk11 : 21;
    } change;
};

// The move of a fight action, 0 for any other action
static inline u16 BattleAction_GetMove(const BattleAction *action) {
    if (action->bits.action == 1) {
        return action->bits.move;
    }
    return 0;
}

void BattleAction_SetFightParam(BattleAction *action, u16 move, u8 target);
void BattleAction_ChangeFightTargetPos(BattleAction *action, u8 target);
void BattleAction_SetMoveInfoFlag(BattleAction *action);
BOOL BattleAction_GetMoveInfoFlag(const BattleAction *action);
u16 BattleAction_GetFightMove(const BattleAction *action);
void BattleAction_SetItemParam(BattleAction *action, u16 item, u8 target, u8 param);
void BattleAction_SetChangeParam(BattleAction *action, u8 pos, u8 slot);
void BattleAction_SetChangeDepleteParam(BattleAction *action);
BOOL BattleAction_IsChangeDeplete(const BattleAction *action);
void BattleAction_SetRotationParam(BattleAction *action, u8 target);
void BattleAction_SetEscape(BattleAction *action);
void BattleAction_SetShift(BattleAction *action);
void BattleAction_SetNull(BattleAction *action);
void BattleAction_SetSkip(BattleAction *action);
u32 BattleAction_GetAction(const BattleAction *action);
void BattleAction_SetRecTimeout(BattleAction *action);
void BattleAction_SetRecError(BattleAction *action);

#endif // POKEBW2_BATTLE_BTL_ACTION_H
