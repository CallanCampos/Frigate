#ifndef POKEBW2_APP_LIVEBATTLEMATCH_IRC_H
#define POKEBW2_APP_LIVEBATTLEMATCH_IRC_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 261's livebattlematch_irc.c (named by its embedded string): the infrared link of the live competitions. It
// starts the network, sends the player's data, team and ready flag to the other machine and hands the other's over,
// and receives a Wi-Fi Download Station's competition data (the delivery). Its functions are called by
// livebattlematch_flow.c and, for the team exchange, by overlay 263. None has a name yet

// The bit of the competition data's versions (field 1 of a tournament entry, and the mask of the delivery's data) that
// is this game's: Black 2's is one above White 2's
#ifdef BLACK2
#define LIVEBATTLEMATCH_VERSION_MASK (1 << 23)
#else
#define LIVEBATTLEMATCH_VERSION_MASK (1 << 22)
#endif

typedef struct LiveBattleMatchIrc LiveBattleMatchIrc;

LiveBattleMatchIrc *LiveBattleMatchIrc_Create(GameData *gameData, HeapID heapId);
void LiveBattleMatchIrc_Delete(LiveBattleMatchIrc *irc);
// Called every frame; does nothing
void LiveBattleMatchIrc_Main(LiveBattleMatchIrc *irc);
// Checks for a network error, and if there is one, ends the delivery and the network's error screen and returns TRUE
BOOL LiveBattleMatchIrc_CheckError(LiveBattleMatchIrc *irc);
// Resets the connection's steps, then TRUE
BOOL LiveBattleMatchIrc_ResetLink(LiveBattleMatchIrc *irc);
// Starts the infrared network and waits for the other machine: 0 while it goes on, 1 once connected, 2 if the other
// machine left first
int LiveBattleMatchIrc_StepLink(LiveBattleMatchIrc *irc);
// Deletes the commands and the callback
void LiveBattleMatchIrc_CancelLink(LiveBattleMatchIrc *irc);
// Ends the network after the commands were deleted by LiveBattleMatchIrc_CancelLink, then TRUE once it ended
BOOL LiveBattleMatchIrc_StepCancel(LiveBattleMatchIrc *irc);
// Resets the steps of LiveBattleMatchIrc_StepEnd, then TRUE
BOOL LiveBattleMatchIrc_ResetEnd(LiveBattleMatchIrc *irc);
// Deletes the commands and ends the network, then TRUE once it ended
BOOL LiveBattleMatchIrc_StepEnd(LiveBattleMatchIrc *irc);
// Sends the player's data, with the team picked, to the other machine
BOOL LiveBattleMatchIrc_SendPlayerData(LiveBattleMatchIrc *irc, WifiBattleMatchPlayer *data);
// If the other machine's data came, sets *data to it and returns TRUE
BOOL LiveBattleMatchIrc_TakePlayerData(LiveBattleMatchIrc *irc, void **data);
// Sends the team picked, and then is TRUE once it is sent
BOOL LiveBattleMatchIrc_SendParty(LiveBattleMatchIrc *irc, PokeParty *party);
// Copies the other player's team into party, and then is TRUE, once it has arrived
BOOL LiveBattleMatchIrc_TakeParty(LiveBattleMatchIrc *irc, PokeParty *party);
// Sends 4 bytes, the ready flag
BOOL LiveBattleMatchIrc_SendReady(LiveBattleMatchIrc *irc, const u32 *flag);
// If the other machine's 4 bytes came, copies them to *flag and returns TRUE
BOOL LiveBattleMatchIrc_TakeReady(LiveBattleMatchIrc *irc, u32 *flag);
// Starts receiving the competition data of a Download Station into buffer (0x160 bytes)
void LiveBattleMatchIrc_StartDelivery(LiveBattleMatchIrc *irc, void *buffer);
// Steps the delivery: 0 while it goes on, then 1 if the data came, 3 if there was none, 2 otherwise
int LiveBattleMatchIrc_StepDelivery(LiveBattleMatchIrc *irc);
// Resets the steps of LiveBattleMatchIrc_StepDeliveryEnd
void LiveBattleMatchIrc_ResetDeliveryEnd(LiveBattleMatchIrc *irc);
// Ends the delivery, then TRUE once the network is free
BOOL LiveBattleMatchIrc_StepDeliveryEnd(LiveBattleMatchIrc *irc);

#endif // POKEBW2_APP_LIVEBATTLEMATCH_IRC_H
