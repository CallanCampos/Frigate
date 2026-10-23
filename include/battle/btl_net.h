#ifndef POKEBW2_BATTLE_BTL_NET_H
#define POKEBW2_BATTLE_BTL_NET_H

// Overlay 167's link battle functions, which exchange the clients' data before a link battle starts

#include "types.h"
#include "gfl/heap.h"
#include "gfl/net_command.h"
#include "struct_decls.h"

void BtlNet_Init(NetHandle *netHandle, u16 clientMask, HeapID heapId);
BOOL BtlNet_IsInitSynced(void);
void BtlNet_Exit(void);
// Whether the link failed
BOOL BtlNet_CheckError(void);
BOOL BtlNet_SendVersion(u8 clientId);
BOOL BtlNet_IsServerDecided(void);
BOOL BtlNet_IsServer(void);
u8 BtlNet_GetCommonVersion(void);
BOOL BtlNet_SendSyncData(BtlMainSyncData *data);
BOOL BtlNet_GetSyncData(BtlMainSyncData *data);
u8 BtlNet_SendParty(PokeParty *party);
BOOL BtlNet_ArePartiesReceived(void);
void BtlNet_BeginChatterExchange(void);
BOOL BtlNet_SendChatter(void *chatter);
BOOL BtlNet_AreChattersReceived(void);
void *BtlNet_GetChatter(u8 clientId);
void BtlNet_EndChatterExchange(void);
BOOL BtlNet_SendPartyPacket(BtlNetPartyPacket *data);
BOOL BtlNet_IsClientPartyReceived(u8 clientId);
PokeParty *BtlNet_GetParty(u8 clientId);
void BtlNet_EndPartyExchange(void);
BOOL BtlNet_SendPlayerInfo(PlayerInfo *info);
BOOL BtlNet_ArePlayerInfosReceived(void);
PlayerInfo *BtlNet_GetPlayerInfo(u8 clientId);
BOOL BtlNet_SendTrainer(BtlSetupTrainer *trainer);
BOOL BtlNet_IsTrainerReceived(void);
void *BtlNet_GetTrainer(void);
void BtlNet_FreeTrainer(void);
void BtlNet_EndTrainerExchange(void);
void BtlNet_StartTiming(u8 id);
BOOL BtlNet_CheckTiming(u8 id);
// The server's commands and the clients' replies
BOOL BtlNet_SendServerCmd(u8 clientId, void *data, u32 size);
BOOL BtlNet_AreRepliesReceived(void);
u16 BtlNet_GetReply(u8 clientId, void **data);
void BtlNet_ResetReplies(void);
BOOL BtlNet_IsServerCmdReceived(void);
u16 BtlNet_GetServerCmd(void **data);
BOOL BtlNet_SendReply(void *data, u32 size);
void *BtlNet_CreatePartyPacket(u32 size, HeapID heapId);
void BtlNet_FreePartyPacket(void *data);
void BtlNet_SetPartyPacket(void *data, PokeParty *party, u8 clientId);

// The link battle's commands, registered from 0x100 by the events that start one
extern const NetCommand data_ov167_021d7448[9];

#endif // POKEBW2_BATTLE_BTL_NET_H
