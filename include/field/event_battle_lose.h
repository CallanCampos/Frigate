#ifndef POKEBW2_FIELD_EVENT_BATTLE_LOSE_H
#define POKEBW2_FIELD_EVENT_BATTLE_LOSE_H

#include "types.h"
#include "struct_decls.h"
#include "gfl/proc.h"
#include "system/game_event.h"

struct BattleLoseData {
    GameSystem *gsys;
    BOOL returnNonLeague;
    PlayerInfo *playerInfo;
};

GameEvent *EventBattleLose_Create(GameSystem *gsys);
GameEventReturnCode EventBattleLose_Callback(GameEvent *event, u32 *state, void *data);

// Overlay 12
// The screen of overlay 295 that a lost battle shows
extern const GameProcFunctions BATTLE_LOSE_MESSAGE_PROC_FUNCTIONS;

#endif // POKEBW2_FIELD_EVENT_BATTLE_LOSE_H
