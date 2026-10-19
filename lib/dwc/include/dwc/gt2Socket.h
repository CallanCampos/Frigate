#ifndef POKEBW2_DWC_GT2SOCKET_H
#define POKEBW2_DWC_GT2SOCKET_H

#include "types.h"
#include "dwc/gt2Main.h"

// The sockets of GameSpy's GT2 (gt2Socket.c), in overlay 11, and the table of their connections. The file and the
// functions in the comments are named after GameSpy's, as pret/pokerevo's Wii build has them
// (asm/SDK/DWC/gamespy/gt2Socket.s); the symbols keep their addresses (see dwc/gt2Main.h)

// gti2SocketFindConnection: the connection to an address, or NULL
GTI2Connection *func_ov011_0217e7a4(GTI2Socket *socket, u32 ip, u16 port);
// gti2CreateSocket, with the protocol, gti2CloseSocket, which waits for the callbacks to return, and gti2Listen
int func_ov011_0217e7cc(GTI2Socket **socket, const char *localAddress, int outgoingBufferSize, int incomingBufferSize,
                        GTI2SocketErrorCallback onSocketError, int protocol);
void func_ov011_0217e944(GTI2Socket *socket);
void func_ov011_0217e974(GTI2Socket *socket, GTI2ConnectAttemptCallback callback);
// gti2NewSocketConnection and gti2FreeSocketConnection
int func_ov011_0217e984(GTI2Socket *socket, GTI2Connection **connection, u32 ip, u16 port);
void func_ov011_0217eac8(GTI2Connection *connection);
// gti2SocketSend: sends a datagram to an address; FALSE when a callback closed the socket
BOOL func_ov011_0217eb34(GTI2Socket *socket, u32 ip, u16 port, const void *message, int length);
// gti2SocketConnectionsThink, gti2FreeClosedConnections and gti2SocketError
BOOL func_ov011_0217eca0(GTI2Socket *socket);
void func_ov011_0217eccc(GTI2Socket *socket);
void func_ov011_0217ecf0(GTI2Socket *socket);

#endif // POKEBW2_DWC_GT2SOCKET_H
