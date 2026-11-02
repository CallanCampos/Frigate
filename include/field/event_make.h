#ifndef POKEBW2_FIELD_EVENT_MAKE_H
#define POKEBW2_FIELD_EVENT_MAKE_H

// eventMakeFunc name from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).

#include "types.h"
#include "battle/battle_proc.h"
#include "struct_decls.h"
#include "system/game_event.h"

// The event that runs overlay 10's battle proc
struct EventMakeArgs {
    BtlSetup *setup;
    BattlePlayers *players;
    // Set, the battle proc marks a network error shown when it ends
    u32 markNetErrShown;
};

typedef struct {
    GameSystem *gsys;
    BtlSetup *setup;
    BattlePlayers *players;
    u32 markNetErrShown;
    BattleParam param;
} EventMakeWork;

GameEvent *EventMake_Create(GameSystem *gsys, BtlSetup *setup, BattlePlayers *players, u32 markNetErrShown);
GameEventReturnCode EventMake_Callback(GameEvent *event, u32 *state, void *data);
// A GameEventProvider, for GameEvent_CreateOverlayDelegate
GameEvent *eventMakeFunc(GameSystem *gsys, void *data);

#endif // POKEBW2_FIELD_EVENT_MAKE_H
