#ifndef POKEBW2_DWC_GT2CONNECTION_H
#define POKEBW2_DWC_GT2CONNECTION_H

#include "types.h"
#include "dwc/gt2Main.h"

// The connections of GameSpy's GT2 (gt2Connection.c), in overlay 11: opening them, their handshake, their timers
// and closing them. The file and the functions in the comments are named after GameSpy's, as pret/pokerevo's Wii build
// has them (asm/SDK/DWC/gamespy/gt2Connection.s); the symbols keep their addresses (see dwc/gt2Main.h)

// gti2NewOutgoingConnection and gti2NewIncomingConnection: a connection to an address, in the first state of the side
// that connects or listens
int func_ov011_0217ce00(GTI2Socket *socket, GTI2Connection **connection, u32 ip, u16 port);
int func_ov011_0217ce1c(GTI2Socket *socket, GTI2Connection **connection, u32 ip, u16 port);
// gti2StartConnectionAttempt: keeps the message and the callbacks and sends the client's challenge
int func_ov011_0217ce38(GTI2Connection *connection, const void *message, int length,
                        const GTI2ConnectionCallbacks *callbacks);
// gti2AcceptConnection and gti2RejectConnection, of a connection waiting for the app
BOOL func_ov011_0217ceac(GTI2Connection *connection, const GTI2ConnectionCallbacks *callbacks);
void func_ov011_0217cee8(GTI2Connection *connection, const void *message, int length);
// gti2ConnectionSendData, which keeps the time it sent
BOOL func_ov011_0217cf18(GTI2Connection *connection, const void *data, int length);
// gti2ConnectionThink: the timeout of the handshake, keep-alives, resends and acks
BOOL func_ov011_0217d038(GTI2Connection *connection, u32 now);
// Closes the connection (pokerevo's func_80349848): at once for a hard close, which only sends the peer the note that
// it is closed, otherwise with a reliable close that the peer answers
void func_ov011_0217d080(GTI2Connection *connection, BOOL hard);
// gti2ConnectionClosed, which moves it to the socket's closed connections, and gti2ConnectionCleanup, which frees it
void func_ov011_0217d0b4(GTI2Connection *connection);
void func_ov011_0217d0e8(GTI2Connection *connection);

#endif // POKEBW2_DWC_GT2CONNECTION_H
