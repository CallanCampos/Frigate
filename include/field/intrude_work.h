#ifndef POKEBW2_FIELD_INTRUDE_WORK_H
#define POKEBW2_FIELD_INTRUDE_WORK_H

// Overlay 12's intrude_work.c (the name is a guess, after Black and White's intrusion functions): what the field asks
// of the communication about other players. Most of it is left as stubs that report no one. getGameOrigin and
// getSeasonFromPlayerData are swan's names

#include "types.h"
#include "struct_decls.h"

// Whether the season may change, which it may not while it is kept in sync with another player
BOOL Intrude_CanBootGameComm(GameSystem *gsys);
void *Intrude_GetCommWork(GameCommSys *commSys);
// The game the other player plays, this one's
u32 getGameOrigin(GameCommSys *commSys);
u8 getSeasonFromPlayerData(GameCommSys *commSys);
u32 Intrude_GetAreaOwnerVersionClass(GameCommSys *commSys, GameData *gameData);
u32 Intrude_GetVisitProgress(GameCommSys *commSys);
void Intrude_SetCompleted(GameCommSys *commSys);
u32 Intrude_GetAlertState(GameCommSys *commSys);
// For the Xtransceiver's screen in the field (comm_tvt_field.c): the number of the player calling, and whether that
// caller is ready, given the name
u32 Intrude_CountEventPeers(GameCommSys *commSys);
BOOL Intrude_GetRequesterStatus(GameCommSys *commSys, StrBuf *name);
u32 Intrude_IsSessionEndPending(GameCommSys *commSys);
void func_ov012_02153668(GameCommSys *commSys);

#endif // POKEBW2_FIELD_INTRUDE_WORK_H
