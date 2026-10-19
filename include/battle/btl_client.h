#ifndef POKEBW2_BATTLE_BTL_CLIENT_H
#define POKEBW2_BATTLE_BTL_CLIENT_H

// Overlay 167's battle clients, each the player or an opponent

#include "types.h"
#include "battle/btl_calc.h"
#include "gfl/heap.h"
#include "nitro/math.h"
#include "struct_decls.h"

void BattleClient_SetForceQuit(BtlClient *client);
void *BattleClient_GetRecordData(BtlClient *client, u32 *size);
void BattleClient_SetViewCore(BtlClient *client, BtlvCore *viewCore);
void BattleClient_StartChapterSkip(BtlClient *client, u32 arg1);
void BattleClient_EndChapterSkip(BtlClient *client);
u32 BattleClient_GetFirstActionMove(BtlClient *client);
void BattleClient_AddShooterEnergy(BtlClient *client, u8 value);
BOOL BattleClient_IsGameTimeUp(BtlClient *client);
BOOL BattleClient_IsRecPlayCanceled(BtlClient *client);

BtlClient *BattleClient_Create(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 commMode, void *netHandle,
                               u16 clientId, u16 numCoverPos, u8 isAI, u32 arg7, BOOL recPlay, MATHRandContext32 *rand,
                               HeapID heapId);
void BattleClient_Delete(BtlClient *client);
void BattleClient_SetRecPlayData(BtlClient *client, void *data);
void BattleClient_SetCmdCheckServer(BtlClient *client, BtlServer *server);
BtlAdapter *BattleClient_GetAdapter(BtlClient *client);
BOOL BattleClient_Main(BtlClient *client);
void BattleClient_GetClientIDList(BtlClient *client, BtlClientIDList *list);

BOOL BattleClient_IsRecPlay(BtlClient *client);
u16 BattleClient_GetRecMaxChapter(BtlClient *client);
BOOL BattleClient_IsChapterSkipping(BtlClient *client);
u8 BattleClient_GetClientId(BtlClient *client);
BattleParty *BattleClient_GetParty(BtlClient *client);
u8 BattleClient_GetShooterEnergy(BtlClient *client);

u8 BattleClient_GetWeather(BtlClient *client);
u16 BattleClient_GetAudienceSize(BtlClient *client);
u32 BattleClient_GetAudienceMask(BtlClient *client);

#endif // POKEBW2_BATTLE_BTL_CLIENT_H
