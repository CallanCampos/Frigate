#ifndef POKEBW2_DWC_GT2CONNECTION_H
#define POKEBW2_DWC_GT2CONNECTION_H

#include "types.h"
#include "dwc/gt2Main.h"

// The connections of GameSpy's GT2 (gt2Connection.c), in overlay 11: opening them, their handshake, their timers
// and closing them. The file and its functions are named after GameSpy's, as pret/pokerevo's Wii build has them
// (asm/SDK/DWC/gamespy/gt2Connection.s) and GameSpy's gt2Connection.c names them

// A connection to an address, in the first state of the side that connects or listens
int gti2NewOutgoingConnection(GTI2Socket *socket, GTI2Connection **connection, u32 ip, u16 port);
int gti2NewIncomingConnection(GTI2Socket *socket, GTI2Connection **connection, u32 ip, u16 port);
// Keeps the message and the callbacks and sends the client's challenge
int gti2StartConnectionAttempt(GTI2Connection *connection, const void *message, int length,
                               const GTI2ConnectionCallbacks *callbacks);
// Of a connection waiting for the app
BOOL gti2AcceptConnection(GTI2Connection *connection, const GTI2ConnectionCallbacks *callbacks);
void gti2RejectConnection(GTI2Connection *connection, const void *message, int length);
// Keeps the time it sent
BOOL gti2ConnectionSendData(GTI2Connection *connection, const void *data, int length);
// The timeout of the handshake, keep-alives, resends and acks
BOOL gti2ConnectionThink(GTI2Connection *connection, u32 now);
// Closes the connection: at once for a hard close, which only sends the peer the note that
// it is closed, otherwise with a reliable close that the peer answers
void gti2CloseConnection(GTI2Connection *connection, BOOL hard);
// gti2ConnectionClosed moves it to the socket's closed connections, and gti2ConnectionCleanup frees it
void gti2ConnectionClosed(GTI2Connection *connection);
void gti2ConnectionCleanup(GTI2Connection *connection);

#endif // POKEBW2_DWC_GT2CONNECTION_H
