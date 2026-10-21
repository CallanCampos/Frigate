#ifndef POKEBW2_DWC_GT2MESSAGE_H
#define POKEBW2_DWC_GT2MESSAGE_H

#include "types.h"
#include "dwc/gt2Main.h"

// The messages of GameSpy's GT2 (gt2Message.c), in overlay 11: receiving datagrams and handling them, and building
// the messages a connection sends. Each returns FALSE when a callback closed the socket. The file and its functions
// are named after GameSpy's, as pret/pokerevo's Wii build has them (asm/SDK/DWC/gamespy/gt2Message.s) and GameSpy's
// gt2Message.c names them

// The socket errors of a datagram to an address
BOOL gti2HandleConnectionReset(GTI2Socket *socket, u32 ip, u16 port);
BOOL gti2HandleHostUnreachable(GTI2Socket *socket, u32 ip, u16 port, BOOL send);
// Handles every datagram waiting on the socket
BOOL gti2ReceiveMessages(GTI2Socket *socket);
BOOL gti2SendClientChallenge(GTI2Connection *connection, const u8 *challenge);
BOOL gti2SendAccept(GTI2Connection *connection);
BOOL gti2SendReject(GTI2Connection *connection, const void *message, int length);
BOOL gti2SendClose(GTI2Connection *connection);
BOOL gti2SendKeepAlive(GTI2Connection *connection);
BOOL gti2SendAck(GTI2Connection *connection);
// Tells the peer the connection is closed
BOOL gti2SendClosed(GTI2Connection *connection);
// gti2ResendMessage resends a reliable message the peer hasn't acked, and gti2Send sends the app's message
BOOL gti2ResendMessage(GTI2Connection *connection, GTI2OutgoingMessage *message);
BOOL gti2Send(GTI2Connection *connection, const void *message, int length, BOOL reliable);

#endif // POKEBW2_DWC_GT2MESSAGE_H
