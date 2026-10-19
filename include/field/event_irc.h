#ifndef POKEBW2_FIELD_EVENT_IRC_H
#define POKEBW2_FIELD_EVENT_IRC_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"
#include "system/game_event.h"

// The infrared event of overlay 12's event_ircbattle.c: battles and trades with nearby players. Function names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

// The infrared menu, in overlay 36
extern const GameProcFunctions IRC_MENU_PROC;

void setPartyLv50(PokeParty *party);
void battleBoxToLv50Party(BOOL useBattleBox, EventIRCWork *work);
void TrimPartyTo3Members(PokeParty *party);
void TrimPartiesTo3Members(EventIRCWork *work);
void EventIRC_RecordFriendBattles(EventIRCWork *work, GameData *gameData);
void EventIRC_SetBattlePlayers(EventIRCWork *work, GameSystem *gsys);
GameEventReturnCode EventIRC_Callback(GameEvent *event, u32 *state, void *data);
// Starts the event, as a new event or in place of event's callback
GameEvent *CallIRC(GameSystem *gsys, Field *field, GameEvent *event, BOOL create);
void EventIRC_FreeParties(EventIRCWork *work);
void EventIRC_RestoreBgm(EventIRCWork *work);
// Accessors for the infrared menu of overlay 36
void EventIRC_SetMode(EventIRCWork *work, u32 mode);
GameSystem *EventIRC_GetGameSystem(EventIRCWork *work);
SaveControl *EventIRC_GetSaveControl(EventIRCWork *work);

#endif // POKEBW2_FIELD_EVENT_IRC_H
