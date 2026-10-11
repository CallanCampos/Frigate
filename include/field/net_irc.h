#ifndef POKEBW2_FIELD_NET_IRC_H
#define POKEBW2_FIELD_NET_IRC_H

#include "types.h"
#include "gfl/net.h"

// net_irc.c, in overlay 31: NetIrcSys, the packets of the infrared net device and their turns. Named descriptively
// after the system its asserts name (see net_devirc.h)

void NetIrc_Init(u32 timeout, u8 gameCode);
void NetIrc_Exit(void);
void NetIrc_SetRecvCallback(GFLNetRecvFunc callback);
void NetIrc_SetInitialized(void);
BOOL NetIrc_IsInitialized(void);
u8 NetIrc_GetSendTurn(void);
u8 NetIrc_GetUnkCF0(void);
BOOL NetIrc_CanSend(void);
void NetIrc_Send(const u8 *data, u8 size, u8 cmd);
void NetIrc_SendPending(void);
BOOL NetIrc_GetNetId(void);
BOOL NetIrc_IsError(void);
void NetIrc_StartLink(void);
BOOL NetIrc_GetPeerSeen(void);
BOOL NetIrc_IsLinked(void);
void NetIrc_Update(void);

#endif // POKEBW2_FIELD_NET_IRC_H
