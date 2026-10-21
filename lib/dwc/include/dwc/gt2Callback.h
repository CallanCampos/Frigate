#ifndef POKEBW2_DWC_GT2CALLBACK_H
#define POKEBW2_DWC_GT2CALLBACK_H

#include "types.h"
#include "dwc/gt2Main.h"

// The calls of GameSpy's GT2 into the app's callbacks (gt2Callback.c), in overlay 11. Each calls the callback, if
// there is one, with NULL and 0 for an empty message, and frees the socket if a callback closed it; it returns FALSE
// when it did. The file and its functions are named after GameSpy's, as pret/pokerevo's Wii build has them
// (asm/SDK/DWC/gamespy/gt2Callback.s) and GameSpy's gt2Callback.c names them

BOOL gti2SocketErrorCallback(GTI2Socket *socket);
BOOL gti2ConnectAttemptCallback(GTI2Socket *socket, GTI2Connection *connection, u32 ip, u16 port, u32 latency,
                                const void *message, int length);
// gti2ConnectedCallback also keeps the result
BOOL gti2ConnectedCallback(GTI2Connection *connection, int result, const void *message, int length);
BOOL gti2ReceivedCallback(GTI2Connection *connection, const void *message, int length, BOOL reliable);
BOOL gti2ClosedCallback(GTI2Connection *connection, int reason);
BOOL gti2PingCallback(GTI2Connection *connection, u32 latency);
// Call the filter with the index
BOOL gti2SendFilterCallback(GTI2Connection *connection, int filterId, const void *message, int length, BOOL reliable);
BOOL gti2ReceiveFilterCallback(GTI2Connection *connection, int filterId, const void *message, int length,
                               BOOL reliable);
// gti2DumpCallback calls the socket's send or receive dump, and gti2UnrecognizedMessageCallback writes whether the app
// handled the message
BOOL gti2DumpCallback(GTI2Socket *socket, GTI2Connection *connection, u32 ip, u16 port, BOOL reset, const void *message,
                      int length, BOOL send);
BOOL gti2UnrecognizedMessageCallback(GTI2Socket *socket, u32 ip, u16 port, const void *message, int length,
                                     BOOL *handled);

#endif // POKEBW2_DWC_GT2CALLBACK_H
