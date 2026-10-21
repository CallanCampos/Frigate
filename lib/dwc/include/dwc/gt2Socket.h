#ifndef POKEBW2_DWC_GT2SOCKET_H
#define POKEBW2_DWC_GT2SOCKET_H

#include "dwc/gt2Main.h"
#include "dwc/nonport.h"

// The sockets of GameSpy's GT2 (gt2Socket.c), in overlay 11, and the table of their connections. The file and its
// functions are named after GameSpy's, as pret/pokerevo's Wii build has them (asm/SDK/DWC/gamespy/gt2Socket.s) and
// GameSpy's gt2Socket.c names them

// The connection to an address, or NULL
GTI2Connection *gti2SocketFindConnection(GTI2Socket *socket, gsi_u32 ip, gsi_u16 port);
// gti2CreateSocket takes the protocol, and gti2CloseSocket waits for the callbacks to return
int gti2CreateSocket(GTI2Socket **socket, const char *localAddress, int outgoingBufferSize, int incomingBufferSize,
                     GTI2SocketErrorCallback onSocketError, int protocol);
void gti2CloseSocket(GTI2Socket *socket);
void gti2Listen(GTI2Socket *socket, GTI2ConnectAttemptCallback callback);
int gti2NewSocketConnection(GTI2Socket *socket, GTI2Connection **connection, gsi_u32 ip, gsi_u16 port);
void gti2FreeSocketConnection(GTI2Connection *connection);
// Sends a datagram to an address; gsi_false when a callback closed the socket
gsi_bool gti2SocketSend(GTI2Socket *socket, gsi_u32 ip, gsi_u16 port, const void *message, int length);
gsi_bool gti2SocketConnectionsThink(GTI2Socket *socket);
void gti2FreeClosedConnections(GTI2Socket *socket);
void gti2SocketError(GTI2Socket *socket);

#endif // POKEBW2_DWC_GT2SOCKET_H
