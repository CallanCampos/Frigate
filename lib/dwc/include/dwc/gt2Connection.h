#ifndef POKEBW2_DWC_GT2CONNECTION_H
#define POKEBW2_DWC_GT2CONNECTION_H

#include "dwc/gt2Main.h"
#include "dwc/nonport.h"

// The connections of GameSpy's GT2 (gt2Connection.c), in overlay 11: opening them, their handshake, their timers
// and closing them. The file and its functions are named after GameSpy's, as pret/pokerevo's Wii build has them
// (asm/SDK/DWC/gamespy/gt2Connection.s) and GameSpy's gt2Connection.c names them

// A connection to an address, in the first state of the side that connects or listens
GT2Result gti2NewOutgoingConnection(GT2Socket socket, GT2Connection *connection, unsigned int ip, unsigned short port);
GT2Result gti2NewIncomingConnection(GT2Socket socket, GT2Connection *connection, unsigned int ip, unsigned short port);
// Keeps the message and the callbacks and sends the client's challenge
GT2Result gti2StartConnectionAttempt(GT2Connection connection, const GT2Byte *message, int length,
                                     GT2ConnectionCallbacks *callbacks);
// Of a connection waiting for the app
GT2Bool gti2AcceptConnection(GT2Connection connection, GT2ConnectionCallbacks *callbacks);
void gti2RejectConnection(GT2Connection connection, const GT2Byte *message, int length);
// Keeps the time it sent
GT2Bool gti2ConnectionSendData(GT2Connection connection, const GT2Byte *data, int length);
// The timeout of the handshake, keep-alives, resends and acks
GT2Bool gti2ConnectionThink(GT2Connection connection, gsi_time now);
// Closes the connection: at once for a hard close, which only sends the peer the note that
// it is closed, otherwise with a reliable close that the peer answers
void gti2CloseConnection(GT2Connection connection, GT2Bool hard);
// gti2ConnectionClosed moves it to the socket's closed connections, and gti2ConnectionCleanup frees it
void gti2ConnectionClosed(GT2Connection connection);
void gti2ConnectionCleanup(GT2Connection connection);

#endif // POKEBW2_DWC_GT2CONNECTION_H
