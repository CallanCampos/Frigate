#ifndef POKEBW2_DWC_GT2SOCKET_H
#define POKEBW2_DWC_GT2SOCKET_H

#include "dwc/gt2Main.h"
#include "dwc/nonport.h"

// The sockets of GameSpy's GT2 (gt2Socket.c), in overlay 11, and the table of their connections. The file and its
// functions are named after GameSpy's, as pret/pokerevo's Wii build has them (asm/SDK/DWC/gamespy/gt2Socket.s) and
// GameSpy's gt2Socket.c names them

// The connection to an address, or NULL
GT2Connection gti2SocketFindConnection(GT2Socket socket, unsigned int ip, unsigned short port);
// gti2CreateSocket takes the protocol, and gti2CloseSocket waits for the callbacks to return
GT2Result gti2CreateSocket(GT2Socket *socket, const char *localAddress, int outgoingBufferSize, int incomingBufferSize,
                           gt2SocketErrorCallback onSocketError, GTI2ProtocolType protocol);
void gti2CloseSocket(GT2Socket socket);
void gti2Listen(GT2Socket socket, gt2ConnectAttemptCallback callback);
GT2Result gti2NewSocketConnection(GT2Socket socket, GT2Connection *connection, unsigned int ip, unsigned short port);
void gti2FreeSocketConnection(GT2Connection connection);
// Sends a datagram to an address; gsi_false when a callback closed the socket
GT2Bool gti2SocketSend(GT2Socket socket, unsigned int ip, unsigned short port, const GT2Byte *message, int length);
GT2Bool gti2SocketConnectionsThink(GT2Socket socket);
void gti2FreeClosedConnections(GT2Socket socket);
void gti2SocketError(GT2Socket socket);

#endif // POKEBW2_DWC_GT2SOCKET_H
