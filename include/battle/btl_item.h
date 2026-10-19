#ifndef POKEBW2_BATTLE_BTL_ITEM_H
#define POKEBW2_BATTLE_BTL_ITEM_H

#include "types.h"
#include "struct_decls.h"

BattleEventItem *ItemEvent_TempAdd(BattleMon *mon, u16 item);
void ItemEvent_TempRemove(void *temp);
void ItemEvent_ItemRotationWake(BattleMon *mon);
BattleEventItem *ItemEvent_AddItem(BattleMon *mon);
void ItemEvent_RemoveItem(BattleMon *mon);
void ItemEvent_ItemRotationSleep(BattleMon *mon);

#endif // POKEBW2_BATTLE_BTL_ITEM_H
