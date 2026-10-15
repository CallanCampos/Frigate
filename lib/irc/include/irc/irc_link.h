#ifndef POKEBW2_IRC_IRC_LINK_H
#define POKEBW2_IRC_IRC_LINK_H

#include "types.h"

// irc_link.c, in overlay 31: the link of frames between two machines over infrared (see net_devirc.h)

// The frames the link sends between the two machines start with one of these. Any byte under IRC_CMD_MIN is a
// packet of data
#define IRC_CMD_MIN 0xf0
#define IRC_CMD_EMPTY 0xee
#define IRC_CMD_CLOSE 0xf4
#define IRC_CMD_RESEND 0xf6
#define IRC_CMD_ACCEPT 0xf8
#define IRC_CMD_REPLY 0xfa
#define IRC_CMD_PING 0xfc

void IrcLink_Reset(u8 id);
void IrcLink_Ping(void);
BOOL IrcLink_IsParent(void);
BOOL IrcLink_IsLinked(void);
void IrcLink_Close(void);
void IrcLink_Send(const u8 *data, u8 size, u8 cmd);
BOOL IrcLink_Poll(void);
void IrcLink_SetRecvCallback(void (*callback)(u8 *data, u8 size, u8 cmd, u8 id));
u8 IrcLink_GetPeerId(void);

#endif // POKEBW2_IRC_IRC_LINK_H
