#ifndef POKEBW2_DWC_GT2MESSAGE_H
#define POKEBW2_DWC_GT2MESSAGE_H

#include "dwc/gt2Main.h"
#include "dwc/nonport.h"

// The messages of GameSpy's GT2 (gt2Message.c), in overlay 11: receiving datagrams and handling them, and building
// the messages a connection sends. Each returns gsi_false when a callback closed the socket. The file and its functions
// are named after GameSpy's, as pret/pokerevo's Wii build has them (asm/SDK/DWC/gamespy/gt2Message.s) and GameSpy's
// gt2Message.c names them

// The socket errors of a datagram to an address
GT2Bool gti2HandleConnectionReset(GT2Socket socket, unsigned int ip, unsigned short port);
GT2Bool gti2HandleHostUnreachable(GT2Socket socket, unsigned int ip, unsigned short port, GT2Bool send);
// Handles every datagram waiting on the socket
GT2Bool gti2ReceiveMessages(GT2Socket socket);
GT2Bool gti2SendClientChallenge(GT2Connection connection, const char challenge[]);
GT2Bool gti2SendAccept(GT2Connection connection);
GT2Bool gti2SendReject(GT2Connection connection, const GT2Byte *message, int length);
GT2Bool gti2SendClose(GT2Connection connection);
GT2Bool gti2SendKeepAlive(GT2Connection connection);
GT2Bool gti2SendAck(GT2Connection connection);
// Tells the peer the connection is closed
GT2Bool gti2SendClosed(GT2Connection connection);
// gti2ResendMessage resends a reliable message the peer hasn't acked, and gti2Send sends the app's message
GT2Bool gti2ResendMessage(GT2Connection connection, GTI2OutgoingBufferMessage *message);
GT2Bool gti2Send(GT2Connection connection, const GT2Byte *message, int length, GT2Bool reliable);

#endif // POKEBW2_DWC_GT2MESSAGE_H
