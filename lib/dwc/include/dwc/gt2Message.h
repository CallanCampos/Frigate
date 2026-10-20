#ifndef POKEBW2_DWC_GT2MESSAGE_H
#define POKEBW2_DWC_GT2MESSAGE_H

#include "types.h"
#include "dwc/gt2Main.h"

// The messages of GameSpy's GT2 (gt2Message.c), in overlay 11: receiving datagrams and handling them, and building
// the messages a connection sends. Each returns FALSE when a callback closed the socket. The file and the functions in
// the comments are named after GameSpy's, as pret/pokerevo's Wii build has them (asm/SDK/DWC/gamespy/gt2Message.s);
// the symbols keep their addresses (see dwc/gt2Main.h)

// gti2HandleConnectionReset and gti2HandleHostUnreachable, the socket errors of a datagram to an address
BOOL func_ov011_0217df34(GTI2Socket *socket, u32 ip, u16 port);
BOOL func_ov011_0217dfc4(GTI2Socket *socket, u32 ip, u16 port, BOOL send);
// gti2ReceiveMessages: handles every datagram waiting on the socket
BOOL func_ov011_0217e020(GTI2Socket *socket);
// gti2SendClientChallenge, gti2SendAccept, gti2SendReject, gti2SendClose, gti2SendKeepAlive and gti2SendAck
BOOL func_ov011_0217e2c0(GTI2Connection *connection, const u8 *challenge);
BOOL func_ov011_0217e3cc(GTI2Connection *connection);
BOOL func_ov011_0217e40c(GTI2Connection *connection, const void *message, int length);
BOOL func_ov011_0217e460(GTI2Connection *connection);
BOOL func_ov011_0217e4a0(GTI2Connection *connection);
BOOL func_ov011_0217e598(GTI2Connection *connection);
// gti2SendClosed, which tells the peer the connection is closed
BOOL func_ov011_0217e694(GTI2Connection *connection);
// gti2ResendMessage, of a reliable message the peer hasn't acked, and gti2Send, of the app's message
BOOL func_ov011_0217e6f8(GTI2Connection *connection, GTI2OutgoingMessage *message);
BOOL func_ov011_0217e754(GTI2Connection *connection, const void *message, int length, BOOL reliable);

#endif // POKEBW2_DWC_GT2MESSAGE_H
