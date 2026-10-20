#ifndef POKEBW2_DWC_GT2CALLBACK_H
#define POKEBW2_DWC_GT2CALLBACK_H

#include "types.h"
#include "dwc/gt2Main.h"

// The calls of GameSpy's GT2 into the app's callbacks (gt2Callback.c), in overlay 11. Each calls the callback, if
// there is one, with NULL and 0 for an empty message, and frees the socket if a callback closed it; it returns FALSE
// when it did. The file and the functions in the comments are named after GameSpy's, as pret/pokerevo's Wii
// build has them (asm/SDK/DWC/gamespy/gt2Callback.s); the symbols keep their addresses (see dwc/gt2Main.h)

// gti2SocketErrorCallback and gti2ConnectAttemptCallback
BOOL func_ov011_0217c9f4(GTI2Socket *socket);
BOOL func_ov011_0217ca30(GTI2Socket *socket, GTI2Connection *connection, u32 ip, u16 port, u32 latency,
                         const void *message, int length);
// gti2ConnectedCallback, which also keeps the result, gti2ReceivedCallback, gti2ClosedCallback and gti2PingCallback
BOOL func_ov011_0217cab0(GTI2Connection *connection, int result, const void *message, int length);
BOOL func_ov011_0217cb10(GTI2Connection *connection, const void *message, int length, BOOL reliable);
BOOL func_ov011_0217cb6c(GTI2Connection *connection, int reason);
BOOL func_ov011_0217cbbc(GTI2Connection *connection, u32 latency);
// gti2SendFilterCallback and gti2ReceiveFilterCallback, which call the filter with the index
BOOL func_ov011_0217cc0c(GTI2Connection *connection, int filterId, const void *message, int length, BOOL reliable);
BOOL func_ov011_0217cc8c(GTI2Connection *connection, int filterId, const void *message, int length, BOOL reliable);
// gti2DumpCallback, with the socket's send or receive dump; and gti2UnrecognizedMessageCallback, which writes whether
// the app handled the message
BOOL func_ov011_0217cd0c(GTI2Socket *socket, GTI2Connection *connection, u32 ip, u16 port, BOOL reset,
                         const void *message, int length, BOOL send);
BOOL func_ov011_0217cd98(GTI2Socket *socket, u32 ip, u16 port, const void *message, int length, BOOL *handled);

#endif // POKEBW2_DWC_GT2CALLBACK_H
