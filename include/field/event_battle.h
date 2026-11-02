#ifndef POKEBW2_FIELD_EVENT_BATTLE_H
#define POKEBW2_FIELD_EVENT_BATTLE_H

// Overlay 12's event_battle.c: the events that run battles from the field. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A wild battle, and the same with the index of the special Pokémon met, 0xff for none
// callerReturns is whether the caller returns to the field after the battle, and handles a loss, itself
GameEvent *EventWildBattleCall_CreateCore(GameSystem *gsys, Field *field, BtlSetup *setup, u8 callerReturns,
                                          u32 encountMode, u8 specialIndex);
GameEvent *EventWildBattleCall_Create(GameSystem *gsys, Field *field, BtlSetup *setup, u8 callerReturns,
                                      u32 encountMode);
// A battle against a trainer of the field, with flags for its kind
GameEvent *EventTrainerBattleCall_Create(GameSystem *gsys, Field *field, u32 style, u32 partnerId, u32 trainerId,
                                         u32 trainerId2, u32 flags);
GameEvent *EventBSubwayBattleCall_Create(GameSystem *gsys, Field *field, BtlSetup *setup);
GameEvent *CreateTrialHouseBattleEvent(GameSystem *gsys, Field *field, BtlSetup *setup);
GameEvent *EventInfinityDungeonBattleCall_Create(GameSystem *gsys, Field *field, BtlSetup *setup);
GameEvent *EventWbtBattleCall_Create(GameSystem *gsys, Field *field, BtlSetup *setup);
// The capture demonstration's battle
GameEvent *EventCaptureDemo_Create(GameSystem *gsys, Field *field, HeapID heapId);
// A battle against a trainer, who sends out the Pokémon of the in-game trade of the index once it is done
GameEvent *LoadTradedPokemonBattleStats(GameSystem *gsys, Field *field, u32 style, u32 partnerId, u32 trainerId,
                                        u32 trainerId2, u32 flags, u32 index);

#endif // POKEBW2_FIELD_EVENT_BATTLE_H
