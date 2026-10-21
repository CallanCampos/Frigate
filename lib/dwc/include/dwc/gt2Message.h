#ifndef POKEBW2_DWC_GT2MESSAGE_H
#define POKEBW2_DWC_GT2MESSAGE_H

#include "dwc/gt2Main.h"
#include "dwc/nonport.h"

// The messages of GameSpy's GT2 (gt2Message.c), in overlay 11: receiving datagrams and handling them, and building
// the messages a connection sends. Each returns gsi_false when a callback closed the socket. The file and its functions
// are named after GameSpy's, as pret/pokerevo's Wii build has them (asm/SDK/DWC/gamespy/gt2Message.s) and GameSpy's
// gt2Message.c names them

// The socket errors of a datagram to an address
gsi_bool gti2HandleConnectionReset(GTI2Socket *socket, gsi_u32 ip, gsi_u16 port);
gsi_bool gti2HandleHostUnreachable(GTI2Socket *socket, gsi_u32 ip, gsi_u16 port, gsi_bool send);
// Handles every datagram waiting on the socket
gsi_bool gti2ReceiveMessages(GTI2Socket *socket);
gsi_bool gti2SendClientChallenge(GTI2Connection *connection, const gsi_u8 *challenge);
gsi_bool gti2SendAccept(GTI2Connection *connection);
gsi_bool gti2SendReject(GTI2Connection *connection, const void *message, int length);
gsi_bool gti2SendClose(GTI2Connection *connection);
gsi_bool gti2SendKeepAlive(GTI2Connection *connection);
gsi_bool gti2SendAck(GTI2Connection *connection);
// Tells the peer the connection is closed
gsi_bool gti2SendClosed(GTI2Connection *connection);
// gti2ResendMessage resends a reliable message the peer hasn't acked, and gti2Send sends the app's message
gsi_bool gti2ResendMessage(GTI2Connection *connection, GTI2OutgoingMessage *message);
gsi_bool gti2Send(GTI2Connection *connection, const void *message, int length, gsi_bool reliable);

#endif // POKEBW2_DWC_GT2MESSAGE_H
