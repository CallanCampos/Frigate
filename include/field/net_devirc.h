#ifndef POKEBW2_FIELD_NET_DEVIRC_H
#define POKEBW2_FIELD_NET_DEVIRC_H

#include "types.h"
#include "gfl/net.h"

// net_devirc.c, in overlay 31, named descriptively (no string names the file; its asserts name the infrared system
// NetIrcSys, and its table is a GFLNetDevTable like net_devwireless.c's): the GFL net device for infrared. A table of
// functions over a small packet system (NetIrcSys) and over the card's infrared chip, which it talks to through the
// backup SPI bus (as src/ov338/ir_check.c does)

// The device table that main's NetDefine_LoadDevice gives for the infrared net type
const GFLNetDevTable *NetDevIrc_GetTable(void);
// How many frames the link may go without a peer before it reports an error, 0 for no limit
void NetDevIrc_SetTimeout(u32 frames);
// Sets the check that decides whether a peer's game is one to link with, called with this machine's game code and the
// peer's, or NULL to require that they are the same
void NetDevIrc_SetPeerFilter(BOOL (*filter)(int gameCode, int peerGameCode));
// Whether the link is up, or still within its time to come up
BOOL NetDevIrc_IsConnected(void);

#endif // POKEBW2_FIELD_NET_DEVIRC_H
