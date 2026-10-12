#ifndef POKEBW2_FIELD_EVENT_POKEWOOD_H
#define POKEBW2_FIELD_EVENT_POKEWOOD_H

#include "types.h"
#include "app/pkwd_selectscreen.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 23's event_pokewood.c, which runs Pokéstar Studios' movies for its script plugin (overlay 62)

#define OVERLAY_EVENT_POKEWOOD OVERLAY_ID(23)

// The cast of one side of a movie (0 the player's, 1 the opponent's) as a party
void EventPokewood_CreateCastParty(PlayerInfo *playerInfo, PokeParty *dest, HeapID heapId, u32 movie, u32 side);
void EventPokewood_CreateCastPartyFromSave(PlayerInfo *playerInfo, PokeParty *dest, HeapID heapId, u32 movie, u32 side);
// Unlocks the movies whose conditions the player now meets, after the movie was filmed with the result and score
void EventPokewood_UnlockMovies(PokewoodSave *save, GameData *gameData, u32 movie, u32 result, s32 score);
// How the movie unlocks, 1 to 7, or 0 for none
u8 EventPokewood_GetUnlockKind(u32 movie);

// The screens of overlays 322 to 325 that the events put up, which are not decompiled yet
extern const GameProcFunctions data_ov322_021a0834;
extern const GameProcFunctions data_ov324_0219f698;
extern const GameProcFunctions data_ov324_0219f6a4;
extern const GameProcFunctions data_ov325_0219f47c;

// Events for GameEvent_CreateOverlayDelegate
GameEvent *EventPokewood_CreateFilmEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateWatchSlotEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateWatchBlockEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateMoviePickerEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateMenuEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateCastSelectEvent(GameSystem *gsys, void *args);
GameEvent *EventPokewood_CreateScreenEvent(GameSystem *gsys, void *args);

#endif // POKEBW2_FIELD_EVENT_POKEWOOD_H
